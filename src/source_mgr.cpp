/* -*- C++ -*-
 * Serene Programming Language
 *
 * Copyright (c) 2019-2023 Sameer Rahmani <lxsameer@gnu.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "source_mgr.h"

#include "errors.h"
#include "jit/jit.h"
#include "location.h"
#include "reader.h"
#include "utils.h"

#include <system_error>

#include <llvm/Support/Error.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/Locale.h>
#include <llvm/Support/MemoryBufferRef.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include <mlir/Support/LogicalResult.h>

namespace serene {

std::string SourceMgr::convertNamespaceToPath(std::string ns_name) {
  std::replace(ns_name.begin(), ns_name.end(), '.', '/');

  llvm::SmallString<MAX_PATH_SLOTS> path;
  path.append(ns_name);
  llvm::sys::path::native(path);

  return std::string(path);
};

bool SourceMgr::isValidBufferID(unsigned i) const {
  return i != 0 && i <= buffers.size();
};

SourceMgr::MemBufPtr SourceMgr::findFileInLoadPath(const std::string &name,
                                                   std::string &importedFile) {

  auto path = convertNamespaceToPath(name);

  // If the file didn't exist directly, see if it's in an include path.
  for (unsigned i = 0, e = loadPaths.size(); i != e; ++i) {

    // TODO: Ugh, Udgly, fix this using llvm::sys::path functions
    importedFile = loadPaths[i] + llvm::sys::path::get_separator().data() +
                   path + "." + DEFAULT_SUFFIX;

    SMGR_LOG("Try to load the ns from: " + importedFile);
    auto newBufOrErr = llvm::MemoryBuffer::getFile(importedFile);

    if (auto err = newBufOrErr.getError()) {
      llvm::consumeError(llvm::errorCodeToError(err));
      continue;
    }

    return std::move(*newBufOrErr);
  }

  return nullptr;
};

ast::MaybeNS SourceMgr::readNamespace(std::string name,
                                      const LocationRange &importLoc) {
  std::string importedFile;

  SMGR_LOG("Attempt to load namespace: " + name);
  MemBufPtr newBufOrErr(findFileInLoadPath(name, importedFile));

  if (newBufOrErr == nullptr) {
    auto msg = llvm::formatv("Couldn't find namespace '{0}'", name).str();
    return errors::make(errors::Type::NSLoadError, importLoc, msg);
  }

  auto bufferId = AddNewSourceBuffer(std::move(newBufOrErr), importLoc);

  UNUSED(nsTable.insert_or_assign(name, bufferId));

  if (bufferId == 0) {
    auto msg = llvm::formatv("Couldn't add namespace '{0}'", name).str();
    return errors::make(errors::Type::NSAddToSMError, importLoc, msg);
  }

  // Since we moved the buffer to be added as the source storage we
  // need to get a pointer to it again
  const auto *buf = getMemoryBuffer(bufferId);

  // Read the content of the buffer by passing it the reader
  auto maybeAst = read(buf->getBuffer(), name,
                       std::optional(llvm::StringRef(importedFile)));

  if (!maybeAst) {
    SMGR_LOG("Couldn't Read namespace: " + name);
    return maybeAst.takeError();
  }

  // Create the NS and set the AST
  auto ns = ast::makeAndCast<ast::Namespace>(
      importLoc, name, std::optional(llvm::StringRef(importedFile)));

  if (auto errs = ns->ExpandTree(*maybeAst)) {
    SMGR_LOG("Couldn't set thre AST for namespace: " + name);
    return errs;
  }

  return ns;
};

unsigned SourceMgr::AddNewSourceBuffer(std::unique_ptr<llvm::MemoryBuffer> f,
                                       const LocationRange &includeLoc) {
  SrcBuffer nb;
  nb.buffer    = std::move(f);
  nb.importLoc = includeLoc;
  buffers.push_back(std::move(nb));
  return buffers.size();
};

template <typename T>
static std::vector<T> &GetOrCreateOffsetCache(void *&offsetCache,
                                              llvm::MemoryBuffer *buffer) {
  if (offsetCache) {
    return *static_cast<std::vector<T> *>(offsetCache);
  }

  // Lazily fill in the offset cache.
  auto *offsets = new std::vector<T>();
  size_t sz     = buffer->getBufferSize();

  // TODO: Replace this assert with a realtime check
  assert(sz <= std::numeric_limits<T>::max());

  llvm::StringRef s = buffer->getBuffer();
  for (size_t n = 0; n < sz; ++n) {
    if (s[n] == '\n') {
      offsets->push_back(static_cast<T>(n));
    }
  }

  offsetCache = offsets;
  return *offsets;
}

template <typename T>
const char *SourceMgr::SrcBuffer::getPointerForLineNumberSpecialized(
    unsigned lineNo) const {
  std::vector<T> &offsets =
      GetOrCreateOffsetCache<T>(offsetCache, buffer.get());

  // We start counting line and column numbers from 1.
  if (lineNo != 0) {
    --lineNo;
  }

  const char *bufStart = buffer->getBufferStart();

  // The offset cache contains the location of the \n for the specified line,
  // we want the start of the line.  As such, we look for the previous entry.
  if (lineNo == 0) {
    return bufStart;
  }

  if (lineNo > offsets.size()) {
    return nullptr;
  }
  return bufStart + offsets[lineNo - 1] + 1;
}

/// Return a pointer to the first character of the specified line number or
/// null if the line number is invalid.
const char *
SourceMgr::SrcBuffer::getPointerForLineNumber(unsigned lineNo) const {
  size_t sz = buffer->getBufferSize();
  if (sz <= std::numeric_limits<uint8_t>::max()) {
    return getPointerForLineNumberSpecialized<uint8_t>(lineNo);
  }

  if (sz <= std::numeric_limits<uint16_t>::max()) {
    return getPointerForLineNumberSpecialized<uint16_t>(lineNo);
  }

  if (sz <= std::numeric_limits<uint32_t>::max()) {
    return getPointerForLineNumberSpecialized<uint32_t>(lineNo);
  }

  return getPointerForLineNumberSpecialized<uint64_t>(lineNo);
}

SourceMgr::SrcBuffer::SrcBuffer(SourceMgr::SrcBuffer &&other) noexcept
    : buffer(std::move(other.buffer)), offsetCache(other.offsetCache),
      importLoc(other.importLoc) {
  other.offsetCache = nullptr;
}

SourceMgr::SrcBuffer::~SrcBuffer() {
  if (offsetCache != nullptr) {
    size_t sz = buffer->getBufferSize();
    if (sz <= std::numeric_limits<uint8_t>::max()) {
      delete static_cast<std::vector<uint8_t> *>(offsetCache);
    } else if (sz <= std::numeric_limits<uint16_t>::max()) {
      delete static_cast<std::vector<uint16_t> *>(offsetCache);
    } else if (sz <= std::numeric_limits<uint32_t>::max()) {
      delete static_cast<std::vector<uint32_t> *>(offsetCache);
    } else {
      delete static_cast<std::vector<uint64_t> *>(offsetCache);
    }
    offsetCache = nullptr;
  }
}

}; // namespace serene
