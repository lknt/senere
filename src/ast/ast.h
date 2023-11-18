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

#ifndef AST_AST_H
#define AST_AST_H

#include "environment.h"
#include "location.h"
#include "serene/config.h"

#include <llvm/Support/Error.h>

#include <memory>

namespace serene::ast {

struct Expression;

using Node      = std::unique_ptr<Expression>;
using MaybeNode = llvm::Expected<Node>;

using Ast      = std::vector<Node>;
using MaybeAst = llvm::Expected<Ast>;

constexpr static auto EmptyNode = nullptr;

// ============================================================================
// Expression
// The abstract class that all the AST nodes derived from. It provides the
// common interface for the expressions to implement.
// ============================================================================
struct Expression {
  /// The location range provide information regarding to where in the input
  /// string the current expression is used.
  LocationRange location;

  Expression(const LocationRange &loc) : location(loc){};
  virtual ~Expression() = default;

  /// Returns the type of the expression. We need this funciton to perform
  /// dynamic casting of expression object to implementations such as lisp or
  /// symbol.
  virtual TypeID getType() const = 0;

  /// The AST representa htion of an expression
  virtual std::string toString() const = 0;

  /// Analyzes the semantics of current node and return a new node in case
  /// that we need to semantically rewrite the current node and replace it with
  /// another node. For example to change from a List containing `(def a b)`
  /// to a `Def` node that represents defining a new binding.
  ///
  /// \param state is the analysis state object of the semantic analyzer.
  // virtual MaybeNode analyze(semantics::AnalysisState &state) = 0;

  /// Genenates the correspondig SLIR of the expressoin and attach it to the
  /// given module.
  ///
  /// \param ns The namespace that current expression is in it.
  /// \param m  The target MLIR moduleOp to attach the operations to
  // virtual void generateIR(serene::Namespace &ns, mlir::ModuleOp &m) = 0;
};

// ============================================================================
// Symbol
// It represent a lisp symbol (don't mix it up with ELF symbols).
// ============================================================================
struct Symbol : public Expression {
  std::string name;
  std::string nsName;

  Symbol(const LocationRange &loc, llvm::StringRef name,
         llvm::StringRef currentNS);
  Symbol(Symbol &s);

  TypeID getType() const override;
  std::string toString() const override;

  ~Symbol() = default;

  static bool classof(const Expression *e);
};

// ============================================================================
// Number
// ============================================================================
struct Number : public Expression {
  // TODO: [ast] Split the number type into their own types
  std::string value;
  // /TODO

  bool isNeg;
  bool isFloat;

  Number(const LocationRange &loc, const llvm::StringRef &n, bool neg, bool fl);
  Number(Number &n);

  TypeID getType() const override;
  std::string toString() const override;

  ~Number() = default;

  static bool classof(const Expression *e);
};

// ============================================================================
// List
// ============================================================================
struct List : public Expression {
  Ast elements;

  explicit List(const LocationRange &loc);
  List(const LocationRange &loc, Ast &v);
  List(const List &l) = delete;
  List(List &&l) noexcept;

  TypeID getType() const override;
  std::string toString() const override;

  ~List() = default;
  void append(Node &n);

  static bool classof(const Expression *e);
};

// ============================================================================
// String
// ============================================================================
struct String : public Expression {
  std::string data;

  String(const LocationRange &loc, llvm::StringRef v);
  String(String &s);

  TypeID getType() const override;
  std::string toString() const override;

  ~String() = default;

  static bool classof(const Expression *e);
};

// ============================================================================
// Keyword
// ============================================================================
struct Keyword : public Expression {
  std::string name;

  Keyword(const LocationRange &loc, llvm::StringRef name);
  Keyword(Keyword &s);

  TypeID getType() const override;
  std::string toString() const override;

  ~Keyword() = default;

  static bool classof(const Expression *e);
};

// ============================================================================
// Error
// One way of representing errors is to just treat them as another type of node
// in the AST and the parser can generate them in case of any error or semantic
// analizer can do the same. At the time of processing the AST by the JIT
// or even anytime earlier we can just stop the execution and deal with the
// issue
// ============================================================================
struct Error : public Expression {
  std::string msg;
  std::unique_ptr<Keyword> tag;

  Error(const LocationRange &loc, std::unique_ptr<Keyword> tag,
        llvm::StringRef msg);
  Error(Error &e);

  TypeID getType() const override;
  std::string toString() const override;

  ~Error() = default;

  static bool classof(const Expression *e);
};

// ============================================================================
// Namespace
// ============================================================================
struct Namespace : public Expression {
  using SemanticEnv          = Environment<Node>;
  using SemanticEnvPtr       = std::unique_ptr<SemanticEnv>;
  using SemanticEnvironments = std::vector<SemanticEnvPtr>;

  std::string name;
  std::optional<std::string> filename;

  Ast tree;

  SemanticEnvironments environments;

  Namespace(const LocationRange &loc, llvm::StringRef name);
  Namespace(const LocationRange &loc, llvm::StringRef name,
            std::optional<llvm::StringRef> filename);
  Namespace(Namespace &s) = delete;

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
  mlir::LogicalResult define(std::string &name, Node &node);

  /// Add the given \p ast to the namespace and return any possible error.
  /// The given \p ast will be added to a vector of ASTs by expanding
  /// the tree vector to contain \p ast.
  ///
  /// This function runs the semantic analyzer on the \p ast as well.
  llvm::Error ExpandTree(Ast &ast);

  Ast &getTree();

  TypeID getType() const override;
  std::string toString() const override;

  ~Namespace() = default;

  static bool classof(const Expression *e);
};

using MaybeNS = llvm::Expected<std::unique_ptr<Namespace>>;

/// Create a new `node` of type `T` and forwards any given parameter
/// to the constructor of type `T`. This is the **official way** to create
/// a new `Expression`. Here is an example:
/// \code
/// auto list = make<List>();
/// \endcode
///
/// \param[args] Any argument with any type passed to this function will be
///              passed to the constructor of type T.
/// \return A unique pointer to an Expression
template <typename T, typename... Args>
Node make(Args &&...args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
};
/// Create a new `node` of type `T` and forwards any given parameter
/// to the constructor of type `T`. This is the **official way** to create
/// a new `Expression`. Here is an example:
/// \code
/// auto list = makeAndCast<List>();
/// \endcode
///
/// \param[args] Any argument with any type passed to this function will be
///              passed to the constructor of type T.
/// \return A unique pointer to a value of type T.
template <typename T, typename... Args>
std::unique_ptr<T> makeAndCast(Args &&...args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
};

/// The helper function to create a new `Node` and returnsit. It should be useds
/// where every we want to return a `MaybeNode` successfully.
template <typename T, typename... Args>
MaybeNode makeSuccessfulNode(Args &&...args) {
  return make<T>(std::forward<Args>(args)...);
};

/// The hlper function to creates an Error (`llvm::Error`) by passing all
/// the given arguments to the constructor of the template param `E`.
template <typename E, typename T = Node, typename... Args>
llvm::Expected<T> makeErrorful(Args &&...args) {
  return llvm::make_error<E>(std::forward<Args>(args)...);
};

/// The hlper function to creates an Error (`llvm::Error`) by passing all
/// the given arguments to the constructor of the template param `E`.
template <typename E, typename... Args>
MaybeNode makeErrorNode(Args &&...args) {
  return makeErrorful<E, Node>(std::forward<Args>(args)...);
};

/// Converts the given AST to string and prints it out
void dump(Ast &);

} // namespace serene::ast

#endif
