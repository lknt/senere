/* -*- C -*-
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

#ifndef TYPES_H
#define TYPES_H

#include "serene/config.h"

#include <cstdint>

typedef struct {
  const TypeID id;
  const char *name;

} Type;

typedef struct {
  const Type type;
  const void *data;
} Object;

static const Type type          = {.id = TYPE, .name = "type"};
static const Type nil_type      = {.id = NIL, .name = "nil"};
static const Type function_type = {.id = FN, .name = "function"};
static const Type protocol_type = {.id = PROTOCOL, .name = "protocol"};
static const Type int_type      = {.id = INT, .name = "int"};
static const Type list_type     = {.id = LIST, .name = "list"};

typedef struct {
  const Type type;
  const Type **args;
  const Type *returnType;
} FunctionType;

typedef struct {
  const Type type;
  const char *name;
  const FunctionType **functions;
} ProtocolType;

typedef struct {
  const Type type;
  const Type first;
  const Type second;
} PairType;

typedef struct {
  const PairType type;
  void *first;
  void *second;
} Pair;

typedef struct {
  const Pair *head;
  const unsigned int len;
} List;

typedef struct {
  const char *name;
} Symbol;

typedef struct {
  const char *data;
  const unsigned int len;
} String;

typedef struct {
  const long data;
} Number;

#endif
