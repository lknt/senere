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

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "utils.h"

#include <llvm/ADT/StringMap.h>
#include <mlir/Support/LogicalResult.h>

namespace serene {

/// This class represents a classic lisp environment (or scope) that holds the
/// bindings from type `K` to type `V`. For example an environment of symbols
/// to expressions would be `Environment<Symbol, Node>`
template <typename V>
class Environment {

  Environment<V> *parent;

  using StorageType = llvm::StringMap<V>;
  // The actual bindings storage
  StorageType pairs;

public:
  Environment() : parent(nullptr) {}
  explicit Environment(Environment *parent) : parent(parent){};

  /// Look up the given `key` in the environment and return it.
  std::optional<V> lookup(llvm::StringRef key) {
    if (auto value = pairs.lookup(key)) {
      return value;
    }

    if (parent) {
      return parent->lookup(key);
    }

    return std::nullopt;
  };

  /// Insert the given `key` with the given `value` into the storage. This
  /// operation will shadow an aleady exist `key` in the parent environment
  mlir::LogicalResult insert_symbol(llvm::StringRef key, V value) {
    auto result = pairs.insert_or_assign(key, value);
    UNUSED(result);
    return mlir::success();
  };

  inline typename StorageType::iterator begin() { return pairs.begin(); }

  inline typename StorageType::iterator end() { return pairs.end(); }

  inline typename StorageType::const_iterator begin() const {
    return pairs.begin();
  }
  inline typename StorageType::const_iterator end() const {
    return pairs.end();
  }
};

} // namespace serene

#endif
