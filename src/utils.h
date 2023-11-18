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

#ifndef UTILS_H
#define UTILS_H

#include <llvm/Support/Error.h>

#include <variant>

// Sometimes we need this to make both analyzer happy
// and the fn signature right.
#define UNUSED(x) (void)(x)

// We use this value with llvm::SmallString<MAX_PATH_SLOTS>
#define MAX_PATH_SLOTS 256
// C++17 required. We can't go back to 14 any more :))

namespace serene {

/// A similar type to Rust's Result data structure. It either holds a value of
/// type `T` successfully or holds a value of type `E` errorfully. It is
/// designed to be used in situations which the return value of a function might
/// contains some errors. The official way to use this type is to use the
/// factory functions `Success` and `Error`. For example:
///
/// \code
/// auto successfulResult = Result<int>::success(3);
/// auto notOkResult = Result<int>::error(SomeLLVMError());
// \endcode
///
/// In order check for a value being errorful or successful checkout the `ok`
/// method or simply use the value as a conditiona.
///
/// This class is setup in a way tha you can us a value of type `T` in places
/// that the compiler expects a `Result<T>`. So for example:
///
/// \code
/// Result<int> fn() {return 2;}
/// \endcode
///
/// works perfectly.
template <typename T, typename E = llvm::Error>
class Result {

  // The actual data container
  std::variant<T, E> contents;

  /// The main constructor which we made private to avoid ambiguousness in
  /// input type. `Success` and `Error` call this ctor.
  template <typename InPlace, typename Content>
  Result(InPlace i, Content &&c) : contents(i, std::forward<Content>(c)){};

public:
  explicit constexpr Result(const T &v)
      : Result(std::in_place_index_t<0>(), std::move(v)){};

  /// Return a pointer to the success case value of the result. It is
  /// important to check for the success case before calling this function.
  constexpr const T *getPointer() const { return &getValue(); }

  /// Return a pointer to the success case value of the result. It is
  /// important to check for the success case before calling this function.
  T *getPointer() { return &getValue(); }

  /// Return a pointer to the success case value of the result. It is
  /// important to check for the success case before calling this function.
  T *operator->() { return getPointer(); }

  /// Return a pointer to the success case value of the result. It is
  /// important to check for the success case before calling this function.
  constexpr const T *operator->() const { return getPointer(); }

  /// Dereference the success case and returns the value. It is
  /// important to check for the success case before calling this function.
  constexpr const T &operator*() const & { return getValue(); }

  /// Dereference the success case and returns the value. It is
  /// important to check for the success case before calling this function.
  T &operator*() & { return getValue(); }

  /// Create a succesfull result with the given value of type `T`.
  static Result success(T v) {
    return Result(std::in_place_index_t<0>(), std::move(v));
  }

  /// Create an errorful result with the given value of type `E` (default
  /// `llvm::Error`).
  static Result error(E e) {
    return Result(std::in_place_index_t<1>(), std::move(e));
  }

  /// Return the value if it's successful otherwise throw an error
  T &&getValue() && { return std::move(std::get<0>(contents)); };

  /// Return the error value if it's errorful otherwise throw an error
  E &&getError() && { return std::move(std::get<1>(contents)); };

  // using std::get, it'll throw if contents doesn't contain what you ask for

  /// Return the value if it's successful otherwise throw an error
  T &getValue() & { return std::get<0>(contents); };

  /// Return the error value if it's errorful otherwise throw an error
  E &getError() & { return std::get<1>(contents); };

  const T &getValue() const & { return std::get<0>(contents); }
  const E &getError() const & { return std::get<1>(contents); }

  /// Return the a boolean value indicating whether the value is succesful
  /// or errorful.
  bool ok() const { return std::holds_alternative<T>(contents); };

  operator bool() const { return ok(); }
};

inline void makeFQSymbolName(const llvm::StringRef &ns,
                             const llvm::StringRef &sym, std::string &result) {
  result = (ns + "/" + sym).str();
};

} // namespace serene
#endif
