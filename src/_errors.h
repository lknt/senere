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

#ifndef SERENE__ERRORS_H
#define SERENE__ERRORS_H

#include <string>

// TODO: Autogenerate this file

namespace serene::errors {
enum class Type {
  NSLoadError = 0,
  NSAddToSMError,
  InvalidDigitForNumber,
  TwoFloatPoints,
  InvalidCharacterForSymbol,
  EOFWhileScaningAList,
  // This error has to be the final error at all time. DO NOT CHANGE IT!
  FINALERROR,
};

static std::string errorMessages[static_cast<int>(Type::FINALERROR) + 1] = {
    "Faild to load the namespace",                      // NSLoadError
    "Faild to add the namespace to the source manager", // NSAddToSMError
    "Invalid number format",                            // InvalidDigitForNumber
    "Invalid float number format",                      // TwoFloatPoints,
    "Invalid symbol format", // InvalidCharacterForSymbol
    "Reached the end of the file while scanning for a list", // EOFWhileScaningAList
};
} // namespace serene::errors
#endif
