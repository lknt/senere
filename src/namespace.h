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
 * Rules of a namespace:
 * - A namespace has have a name and it has to own it.
 * - A namespace may or may not be associated with a file
 * - The internal AST of a namespace is an evergrowing tree which may expand at
 *   any given time. For example via iteration of a REPL
 * - `environments` vector is the owner of all the semantic envs
 * - The first env in the `environments` is the root env.
 *
 * How to create a namespace ?
 * The official way to create a namespace object is to use the `SereneContext`
 * object and call `readNamespace`, `importNamespace` or `makeNamespace`.
 */

// TODO: Add a mechanism to figure out whether a namespace has changed or not
//       either on memory or disk

#ifndef NAMESPACE_H
#define NAMESPACE_H

#include "ast/ast.h"
#include "environment.h"
#include "utils.h"

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>
#include <mlir/IR/Builders.h>
#include <mlir/IR/BuiltinOps.h>
#include <mlir/IR/OwningOpRef.h>
#include <mlir/IR/Value.h>
#include <mlir/Support/LogicalResult.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>

#define NAMESPACE_LOG(...) \
  DEBUG_WITH_TYPE("NAMESPACE", llvm::dbgs() << __VA_ARGS__ << "\n");

namespace serene {
namespace jit {
class JIT;
} // namespace jit

class Namespace;

using NSPtr                = std::unique_ptr<Namespace>;
using MaybeNS              = llvm::Expected<NSPtr>;
using SemanticEnv          = Environment<ast::Node>;
using SemanticEnvPtr       = std::unique_ptr<SemanticEnv>;
using SemanticEnvironments = std::vector<SemanticEnvPtr>;

/// Serene's namespaces are the unit of compilation. Any code that needs to be
/// compiled has to be in a namespace. The official way to create a new
/// namespace is to use the `readNamespace`, `importNamespace` and
/// `makeNamespace` member functions of `SereneContext`.
class Namespace {
  jit::JIT &engine;
  /// The content of the namespace. It should alway hold a semantically
  /// correct AST. It means thet the AST that we want to store here has
  /// to pass the semantic analyzer checks.
  ast::Ast tree;

  SemanticEnvironments environments;

  std::vector<llvm::StringRef> symbolList;

public:
  std::string name;
  std::optional<std::string> filename;

  /// Create a naw namespace with the given `name` and optional `filename` and
  /// return a unique pointer to it in the given Serene context.
  static NSPtr make(jit::JIT &engine, llvm::StringRef name,
                    std::optional<llvm::StringRef> filename);

  Namespace(jit::JIT &engine, llvm::StringRef ns_name,
            std::optional<llvm::StringRef> filename);

  /// Create a new environment with the give \p parent as the parent,
  /// push the environment to the internal environment storage and
  /// return a reference to it. The namespace itself is the owner of
  /// environments.
  SemanticEnv &createEnv(SemanticEnv *parent);

  /// Return a referenece to the top level (root) environment of ns.
  SemanticEnv &getRootEnv();

  /// Define a new binding in the root environment with the given \p name
  /// and the given \p node. Defining a new binding with a name that
  /// already exists in legal and will overwrite the previous binding and
  /// the given name will point to a new value from now on.
  mlir::LogicalResult define(std::string &name, ast::Node &node);

  /// Add the given \p ast to the namespace and return any possible error.
  /// The given \p ast will be added to a vector of ASTs by expanding
  /// the tree vector to contain \p ast.
  ///
  /// This function runs the semantic analyzer on the \p ast as well.
  llvm::Error ExpandTree(ast::Ast &ast);

  ast::Ast &getTree();

  const std::vector<llvm::StringRef> &getSymList() { return symbolList; };

  /// Dumps the namespace with respect to the compilation phase
  // void dump();

  ~Namespace();
};

} // namespace serene

#endif
