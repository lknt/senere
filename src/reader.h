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
 * `Reader` is the base parser class and accepts a buffer like object (usually
 * `llvm::StringRef`) as the input and parses it to create an AST (look at the
 * `serene::exprs::Expression` class).
 *
 * The parsing algorithm is quite simple and it is a LL(2). It means that, we
 * start parsing the input from the very first character and parse the input
 * one char at a time till we reach the end of the input. Please note that
 * when we call the `advance` function to move forward in the buffer, we
 * can't go back. In order to look ahead in the buffer without moving in the
 * buffer we use the `nextChar` method.
 *
 * We have dedicated methods to read different forms like `list`, `symbol`
 * `number` and etc. Each of them return a `MaybeNode` that in the success
 * case contains the node and an `Error` on the failure case.
 */

#ifndef READER_H
#define READER_H

#include "ast/ast.h"
#include "location.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/MemoryBufferRef.h>

#define READER_LOG(...)                  \
  DEBUG_WITH_TYPE("READER", llvm::dbgs() \
                                << "[READER]: " << __VA_ARGS__ << "\n");

namespace serene {
namespace jit {
class JIT;
} // namespace jit

/// Base reader class which reads from a string directly.
class Reader {
private:
  llvm::StringRef ns;
  std::optional<llvm::StringRef> filename;

  const char *currentChar = nullptr;

  llvm::StringRef buf;

  /// The position tracker that we will use to determine the end of the
  /// buffer since the buffer might not be null terminated
  size_t currentPos = static_cast<size_t>(-1);

  Location currentLocation;

  bool readEOL = false;

  /// Returns a clone of the current location
  Location getCurrentLocation();
  /// Returns the next character from the stream.
  /// @param skip_whitespace An indicator to whether skip white space like chars
  /// or not
  void advance(bool skipWhitespace = false);
  void advanceByOne();

  const char *nextChar(bool skipWhitespace = false, unsigned count = 1);

  /// Returns a boolean indicating whether the given input character is valid
  /// for an identifier or not.
  static bool isValidForIdentifier(char c);

  // The property to store the ast tree
  ast::Ast ast;

  ast::MaybeNode readSymbol();
  ast::MaybeNode readNumber(bool);
  ast::MaybeNode readList();
  ast::MaybeNode readExpr();

  bool isEndOfBuffer(const char *);

public:
  Reader(llvm::StringRef buf, llvm::StringRef ns,
         std::optional<llvm::StringRef> filename);
  Reader(llvm::MemoryBufferRef buf, llvm::StringRef ns,
         std::optional<llvm::StringRef> filename);

  // void setInput(const llvm::StringRef string);

  /// Parses the the input and creates a possible AST out of it or errors
  /// otherwise.
  ast::MaybeAst read();

  ~Reader();
};

/// Parses the given `input` string and returns a `Result<ast>`
/// which may contains an AST or an `llvm::Error`
ast::MaybeAst read(llvm::StringRef input, llvm::StringRef ns,
                   std::optional<llvm::StringRef> filename);
ast::MaybeAst read(llvm::MemoryBufferRef input, llvm::StringRef ns,
                   std::optional<llvm::StringRef> filename);

} // namespace serene
#endif
