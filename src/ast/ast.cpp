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

#include "ast/ast.h"

#include <llvm/Support/FormatVariadic.h>

namespace serene::ast {

// ============================================================================
// Symbol
// ============================================================================
Symbol::Symbol(const LocationRange &loc, llvm::StringRef name,
               llvm::StringRef currentNS)
    : Expression(loc) {
  // IMPORTANT NOTE: the `name` and `currentNS` should be valid string and
  //                 already validated.
  auto partDelimiter = name.find('/');
  if (partDelimiter == std::string::npos) {
    nsName     = currentNS.str();
    this->name = name.str();

  } else {
    this->name = name.substr(partDelimiter + 1, name.size()).str();
    nsName     = name.substr(0, partDelimiter).str();
  }
};

Symbol::Symbol(Symbol &s) : Expression(s.location) {
  this->name   = s.name;
  this->nsName = s.nsName;
};

TypeID Symbol::getType() const { return TypeID::SYMBOL; };

std::string Symbol::toString() const {
  return llvm::formatv("<Symbol {0}/{1}>", nsName, name);
}

bool Symbol::classof(const Expression *e) {
  return e->getType() == TypeID::SYMBOL;
};

// ============================================================================
// Number
// ============================================================================
Number::Number(const LocationRange &loc, const llvm::StringRef &n, bool neg,
               bool fl)
    : Expression(loc), value(n), isNeg(neg), isFloat(fl){};

Number::Number(Number &n) : Expression(n.location) { this->value = n.value; };

TypeID Number::getType() const { return TypeID::NUMBER; };

std::string Number::toString() const {
  return llvm::formatv("<Number {0}{1}>", isNeg ? "-" : "", value);
}

bool Number::classof(const Expression *e) {
  return e->getType() == TypeID::NUMBER;
};

// ============================================================================
// List
// ============================================================================
List::List(const LocationRange &loc) : Expression(loc){};

List::List(const LocationRange &loc, Ast &v) : Expression(loc) {
  this->elements.swap(v);
  v.clear();
};

List::List(List &&l) noexcept : Expression(l.location) {
  this->elements.swap(l.elements);
  l.elements.clear();
};

TypeID List::getType() const { return TypeID::LIST; };

std::string List::toString() const {
  std::string s{this->elements.empty() ? "-" : ""};

  for (const auto &n : this->elements) {
    s = llvm::formatv("{0}, {1}", s, n->toString());
  }

  return llvm::formatv("<List {0}>", s);
}

bool List::classof(const Expression *e) {
  return e->getType() == TypeID::LIST;
};

void List::append(Node &n) { elements.push_back(std::move(n)); }
// ============================================================================
// String
// ============================================================================
String::String(const LocationRange &loc, llvm::StringRef v)
    : Expression(loc), data(v.str()){};

String::String(String &s) : Expression(s.location), data(s.data){};

TypeID String::getType() const { return TypeID::STRING; };

std::string String::toString() const {
  const short truncateSize = 10;
  return llvm::formatv(
      "<String '{0}'>",
      data.substr(0, data.size() >= truncateSize ? truncateSize : data.size()));
}

bool String::classof(const Expression *e) {
  return e->getType() == TypeID::STRING;
};
// ============================================================================
// Keyword
// ============================================================================
Keyword::Keyword(const LocationRange &loc, llvm::StringRef name)
    : Expression(loc), name(name.str()){};

Keyword::Keyword(Keyword &s) : Expression(s.location) { this->name = s.name; };

TypeID Keyword::getType() const { return TypeID::KEYWORD; };

std::string Keyword::toString() const {
  return llvm::formatv("<Keyword {0}>", name);
}

bool Keyword::classof(const Expression *e) {
  return e->getType() == TypeID::KEYWORD;
};

// ============================================================================
// Error
// ============================================================================
Error::Error(const LocationRange &loc, std::unique_ptr<Keyword> tag,
             llvm::StringRef msg)
    : Expression(loc), msg(msg.str()), tag(std::move(tag)){};

Error::Error(Error &e) : Expression(e.location) {
  this->msg = e.msg;
  this->tag = std::move(e.tag);
};

TypeID Error::getType() const { return TypeID::KEYWORD; };

std::string Error::toString() const {
  return llvm::formatv("<Error {0}>", msg);
}

bool Error::classof(const Expression *e) {
  return e->getType() == TypeID::KEYWORD;
};

// ============================================================================
// Namespace
// ============================================================================
Namespace::Namespace(const LocationRange &loc, llvm::StringRef name)
    : Namespace(loc, name, std::nullopt){};
Namespace::Namespace(const LocationRange &loc, llvm::StringRef name,
                     std::optional<llvm::StringRef> filename)
    : Expression(loc), name(name), filename(filename) {
  createEnv(nullptr);
};

Namespace::SemanticEnv &Namespace::createEnv(SemanticEnv *parent) {
  auto env = std::make_unique<SemanticEnv>(parent);
  environments.push_back(std::move(env));

  return *environments.back();
};

TypeID Namespace::getType() const { return TypeID::NS; };

std::string Namespace::toString() const {
  return llvm::formatv("<NS {0}>", name);
}

bool Namespace::classof(const Expression *e) {
  return e->getType() == TypeID::NS;
};

} // namespace serene::ast
