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

#ifndef SERENE_ERRORS_H
#define SERENE_ERRORS_H

#include "_errors.h"
#include "location.h"

#include <system_error>

#include <llvm/Support/Error.h>
#include <llvm/Support/FormatVariadic.h>

namespace serene::errors {

class Error : public llvm::ErrorInfo<Error> {
public:
  // We need this to make Error class a llvm::Error friendy implementation
  static char ID;

  Type type;
  const LocationRange location;
  std::string msg;

  void log(llvm::raw_ostream &os) const override { os << msg; }

  std::error_code convertToErrorCode() const override {
    // TODO: Fix this by creating a mapping from ErrorType to standard
    // errc or return the ErrorType number instead
    return std::make_error_code(std::errc::io_error);
  }

  Error(Type errtype, const LocationRange &loc)
      : type(errtype), location(loc){};

  Error(Type errtype, const LocationRange &loc, llvm::StringRef msg)
      : type(errtype), location(loc), msg(msg.str()){};

  const LocationRange &where() { return location; };
};

llvm::Error make(Type t, const LocationRange &loc, llvm::StringRef msg);

llvm::Error make(Type t, const LocationRange &loc);

}; // namespace serene::errors
#endif
