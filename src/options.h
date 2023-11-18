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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <llvm/TargetParser/Triple.h> // for Triple

namespace serene {
/// This enum describes the different operational phases for the compiler
/// in order. Anything below `NoOptimization` is considered only for debugging
enum class CompilationPhase {
  Parse,
  Analysis,
  SLIR,
  MLIR, // Lowered slir to other dialects
  LIR,  // Lowered to the llvm ir dialect
  IR,   // Lowered to the LLVMIR itself
  NoOptimization,
  O1,
  O2,
  O3,
};

/// Options describes the compiler options that can be passed to the
/// compiler via command line. Anything that user should be able to
/// tweak about the compiler has to end up here regardless of the
/// different subsystem that might use it.
struct Options {

  bool verbose = false;

  /// Whether to use colors for the output or not
  bool withColors = true;

  // JIT related flags
  bool JITenableObjectCache              = true;
  bool JITenableGDBNotificationListener  = true;
  bool JITenablePerfNotificationListener = true;
  bool JITLazy                           = false;

  // We will use this triple to generate code that will endup in the binary
  // for the target platform. If we're not cross compiling, `targetTriple`
  // will be the same as `hostTriple`.
  const llvm::Triple targetTriple;

  // This triple will be used in code generation for the host platform in
  // complie time. For example any function that will be called during
  // the compile time has to run on the host. So we need to generate
  // appropriate code for the host. If the same function has to be part
  // of the runtime, then we use `targetTriple` again to generate the code
  // for the target platform. So, we might end up with two version of the
  // same function.
  const llvm::Triple hostTriple;

  CompilationPhase compilationPhase = CompilationPhase::NoOptimization;
};

} // namespace serene
#endif
