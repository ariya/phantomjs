/************************************************************
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of Silicon Graphics not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific prior written permission.
 * Silicon Graphics makes no representation about the suitability
 * of this software for any purpose. It is provided "as is"
 * without any express or implied warranty.
 *
 * SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 * GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 * THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ********************************************************/

#ifndef XKBCOMP_AST_BUILD_H
#define XKBCOMP_AST_BUILD_H

ParseCommon *
AppendStmt(ParseCommon *to, ParseCommon *append);

ExprDef *
ExprCreateString(xkb_atom_t str);

ExprDef *
ExprCreateInteger(int ival);

ExprDef *
ExprCreateBoolean(bool set);

ExprDef *
ExprCreateKeyName(xkb_atom_t key_name);

ExprDef *
ExprCreateIdent(xkb_atom_t ident);

ExprDef *
ExprCreateUnary(enum expr_op_type op, enum expr_value_type type,
                ExprDef *child);

ExprDef *
ExprCreateBinary(enum expr_op_type op, ExprDef *left, ExprDef *right);

ExprDef *
ExprCreateFieldRef(xkb_atom_t element, xkb_atom_t field);

ExprDef *
ExprCreateArrayRef(xkb_atom_t element, xkb_atom_t field, ExprDef *entry);

ExprDef *
ExprCreateAction(xkb_atom_t name, ExprDef *args);

ExprDef *
ExprCreateMultiKeysymList(ExprDef *list);

ExprDef *
ExprCreateKeysymList(xkb_keysym_t sym);

ExprDef *
ExprAppendMultiKeysymList(ExprDef *list, ExprDef *append);

ExprDef *
ExprAppendKeysymList(ExprDef *list, xkb_keysym_t sym);

KeycodeDef *
KeycodeCreate(xkb_atom_t name, int64_t value);

KeyAliasDef *
KeyAliasCreate(xkb_atom_t alias, xkb_atom_t real);

VModDef *
VModCreate(xkb_atom_t name, ExprDef *value);

VarDef *
VarCreate(ExprDef *name, ExprDef *value);

VarDef *
BoolVarCreate(xkb_atom_t ident, bool set);

InterpDef *
InterpCreate(xkb_keysym_t sym, ExprDef *match);

KeyTypeDef *
KeyTypeCreate(xkb_atom_t name, VarDef *body);

SymbolsDef *
SymbolsCreate(xkb_atom_t keyName, VarDef *symbols);

GroupCompatDef *
GroupCompatCreate(unsigned group, ExprDef *def);

ModMapDef *
ModMapCreate(xkb_atom_t modifier, ExprDef *keys);

LedMapDef *
LedMapCreate(xkb_atom_t name, VarDef *body);

LedNameDef *
LedNameCreate(unsigned ndx, ExprDef *name, bool virtual);

IncludeStmt *
IncludeCreate(struct xkb_context *ctx, char *str, enum merge_mode merge);

XkbFile *
XkbFileCreate(enum xkb_file_type type, char *name, ParseCommon *defs,
              enum xkb_map_flags flags);

void
FreeStmt(ParseCommon *stmt);

#endif
