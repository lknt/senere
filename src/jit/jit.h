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

/**
 * Commentary:
  - It operates in lazy (for REPL) and non-lazy mode and wraps LLJIT
    and LLLazyJIT
  - It uses an object cache layer to cache module (not NSs) objects.
 */

#ifndef JIT_JIT_H
#define JIT_JIT_H

#include "options.h"

#include <__memory/unique_ptr.h>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ExecutionEngine/ObjectCache.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/MemoryBufferRef.h>
#include <llvm/Support/raw_ostream.h>

#include <optional>
#include <stddef.h>
#include <variant>
#include <vector>

namespace llvm {
class JITEventListener;
} // namespace llvm
namespace llvm {
class Module;
} // namespace llvm
namespace llvm::orc {
class JITDylib;
class LLJIT;
class LLLazyJIT;
} // namespace llvm::orc

namespace serene {
class Namespace;
} // namespace serene

#define MAIN_PROCESS_JD_NAME "*main*"
#define JIT_LOG(...) \
  DEBUG_WITH_TYPE("JIT", llvm::dbgs() << "[JIT]: " << __VA_ARGS__ << "\n");

namespace orc = llvm::orc;

namespace serene::jit {
class JIT;
using JITPtr            = std::unique_ptr<JIT>;
using MaybeJIT          = llvm::Expected<JITPtr>;
using JitWrappedAddress = void (*)(void **);
using MaybeJitAddress   = llvm::Expected<JitWrappedAddress>;

/// A simple object cache following Lang's LLJITWithObjectCache example and
/// MLIR's SimpelObjectCache.
class ObjectCache : public llvm::ObjectCache {
public:
  /// Cache the given `objBuffer` for the given module `m`. The buffer contains
  /// the combiled objects of the module
  void notifyObjectCompiled(const llvm::Module *m,
                            llvm::MemoryBufferRef objBuffer) override;

  // Lookup the cache for the given module `m` or returen a nullptr.
  std::unique_ptr<llvm::MemoryBuffer> getObject(const llvm::Module *m) override;

  /// Dump cached object to output file `filename`.
  void dumpToObjectFile(llvm::StringRef filename);

private:
  llvm::StringMap<std::unique_ptr<llvm::MemoryBuffer>> cachedObjects;
};

class JIT {
  std::unique_ptr<const Options> options;

  std::unique_ptr<orc::LLJIT> engine;
  std::unique_ptr<ObjectCache> cache;

  llvm::JITEventListener *gdbListener;
  /// Perf notification listener.
  llvm::JITEventListener *perfListener;

  llvm::orc::JITTargetMachineBuilder jtmb;

  std::vector<const char *> loadPaths;

  // We keep the jibDylibs for each name space in a mapping from the ns
  // name to a vector of jitdylibs, the last element is always the newest
  // jitDylib
  //
  // Questions:
  // Is using string as the key good enough, what about an ID for NSs
  // or even a pointer to the ns?
  llvm::StringMap<llvm::SmallVector<llvm::orc::JITDylib *, 1>> jitDylibs;

  void pushJITDylib(const llvm::StringRef &nsName, llvm::orc::JITDylib *l);
  size_t getNumberOfJITDylibs(const llvm::StringRef &nsName);

  llvm::Error createCurrentProcessJD();

  // Anonymous function counter. We need to assing a unique name to each
  // anonymous function and we use this counter to generate those names
  std::atomic<uint> fn_counter = 0;

  // Since indexing namespaces by the name would be inefficient, We use
  // unsigned integer and assign a number to all the namespaces at the
  // creation time. Namespace IDs have to be unique.
  std::atomic<uint> ns_counter = 0;

public:
  JIT(llvm::orc::JITTargetMachineBuilder &&jtmb, std::unique_ptr<Options> opts);
  static MaybeJIT make(llvm::orc::JITTargetMachineBuilder &&jtmb,
                       std::unique_ptr<Options> opts);

  // Return an integer indicating the level of optimization that is currently
  // set. 0 == No optimizaion -> it includes compling to IR and AST
  int getOptimizatioLevel() const;

  /// Return a pointer to the most registered JITDylib of the given \p ns
  ////name
  llvm::orc::JITDylib *getLatestJITDylib(const llvm::StringRef &nsName);

  /// Looks up a packed-argument function with the given sym name and returns a
  /// pointer to it. Propagates errors in case of failure.
  MaybeJitAddress lookup(const llvm::StringRef &nsName,
                         const llvm::StringRef &sym) const;

  /// Invokes the function with the given name passing it the list of opaque
  /// pointers containing the actual arguments.
  llvm::Error
  invokePacked(const llvm::StringRef &symbolName,
               llvm::MutableArrayRef<void *> args = std::nullopt) const;

  llvm::Error loadModule(const llvm::StringRef &nsName,
                         const llvm::StringRef &file);
  void dumpToObjectFile(const llvm::StringRef &filename);

  /// Setup the load path for namespace lookups
  void setLoadPaths(std::vector<const char *> &dirs) { loadPaths.swap(dirs); };
  /// Return the load paths for namespaces
  llvm::ArrayRef<const char *> getLoadPaths() { return loadPaths; };

  const Options &getOptions() const { return *options; };
};

MaybeJIT makeJIT(std::unique_ptr<Options> opts);
} // namespace serene::jit
#endif
