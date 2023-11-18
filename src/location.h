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

#ifndef LOCATION_H
#define LOCATION_H

#include <mlir/IR/Diagnostics.h>
#include <mlir/IR/Location.h>

#include <string>

namespace serene {

/// It represents a location in the input string to the parser via `line`,
struct Location {
  /// Since namespaces are our unit of compilation, we need to have
  /// a namespace in hand
  llvm::StringRef ns;

  std::optional<llvm::StringRef> filename = std::nullopt;
  /// A pointer to the character that this location is pointing to
  /// it the input buffer
  const char *c = nullptr;

  /// At this stage we only support 65535 lines of code in each file
  unsigned short int line = 0;
  /// At this stage we only support 65535 chars in each line
  unsigned short int col = 0;

  bool knownLocation = true;

  ::std::string toString() const;

  Location() = default;
  explicit Location(llvm::StringRef ns,
                    std::optional<llvm::StringRef> fname = std::nullopt,
                    const char *c = nullptr, unsigned short int line = 0,
                    unsigned short int col = 0, bool knownLocation = true)
      : ns(ns), filename(fname), c(c), line(line), col(col),
        knownLocation(knownLocation){};

  Location clone() const;

  // mlir::Location toMLIRLocation(mlir::MLIRContext &ctx);

  /// Returns an unknown location for the given \p ns.
  static Location UnknownLocation(llvm::StringRef ns) {
    return Location(ns, std::nullopt, nullptr, 0, 0, false);
  }

  ~Location() = default;
};

class LocationRange {
public:
  Location start;
  Location end;

  LocationRange() = default;
  explicit LocationRange(Location _start) : start(_start), end(_start){};
  LocationRange(Location _start, Location _end) : start(_start), end(_end){};
  // LocationRange(const LocationRange &);

  LocationRange(LocationRange &lr) : start(lr.start), end(lr.end){};

  LocationRange(const LocationRange &lr) : start(lr.start), end(lr.end){};

  bool isKnownLocation() const { return start.knownLocation; };

  static LocationRange UnknownLocation(llvm::StringRef ns) {
    return LocationRange(Location::UnknownLocation(ns));
  }

  ~LocationRange() = default;
};

void incLocation(Location &, const char *);
void decLocation(Location &, const char *);

} // namespace serene
#endif
