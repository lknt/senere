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

#include "errors.h"

#include <llvm/Support/Casting.h>
#include <llvm/Support/Error.h>

namespace serene::errors {

// We need this to make Error class a llvm::Error friendy implementation
char Error::ID;

std::string getMessage(const llvm::Error &e) {
  std::string msg;
  llvm::raw_string_ostream os(msg);
  os << e;
  return os.str();
};

llvm::Error make(Type t, const LocationRange &loc, llvm::StringRef msg) {
  return llvm::make_error<Error>(t, loc, msg);
}

llvm::Error make(Type t, const LocationRange &loc) {
  return llvm::make_error<Error>(t, loc);
}

} // namespace serene::errors
