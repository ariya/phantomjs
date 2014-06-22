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

/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 *         Ran Benita <ran234@gmail.com>
 */

#include "xkbcomp-priv.h"
#include "ast-build.h"
#include "include.h"

ParseCommon *
AppendStmt(ParseCommon *to, ParseCommon *append)
{
    ParseCommon *iter;

    if (!to)
        return append;

    for (iter = to; iter->next; iter = iter->next);

    iter->next = append;
    return to;
}

static ExprDef *
ExprCreate(enum expr_op_type op, enum expr_value_type type, size_t size)
{
    ExprDef *expr = malloc(size);
    if (!expr)
        return NULL;

    expr->common.type = STMT_EXPR;
    expr->common.next = NULL;
    expr->expr.op = op;
    expr->expr.value_type = type;

    return expr;
}

#define EXPR_CREATE(type_, name_, op_, value_type_) \
    ExprDef *name_ = ExprCreate(op_, value_type_, sizeof(type_)); \
    if (!name_) \
        return NULL;

ExprDef *
ExprCreateString(xkb_atom_t str)
{
    EXPR_CREATE(ExprString, expr, EXPR_VALUE, EXPR_TYPE_STRING);
    expr->string.str = str;
    return expr;
}

ExprDef *
ExprCreateInteger(int ival)
{
    EXPR_CREATE(ExprInteger, expr, EXPR_VALUE, EXPR_TYPE_INT);
    expr->integer.ival = ival;
    return expr;
}

ExprDef *
ExprCreateBoolean(bool set)
{
    EXPR_CREATE(ExprBoolean, expr, EXPR_VALUE, EXPR_TYPE_BOOLEAN);
    expr->boolean.set = set;
    return expr;
}

ExprDef *
ExprCreateKeyName(xkb_atom_t key_name)
{
    EXPR_CREATE(ExprKeyName, expr, EXPR_VALUE, EXPR_TYPE_KEYNAME);
    expr->key_name.key_name = key_name;
    return expr;
}

ExprDef *
ExprCreateIdent(xkb_atom_t ident)
{
    EXPR_CREATE(ExprIdent, expr, EXPR_IDENT, EXPR_TYPE_UNKNOWN);
    expr->ident.ident = ident;
    return expr;
}

ExprDef *
ExprCreateUnary(enum expr_op_type op, enum expr_value_type type,
                ExprDef *child)
{
    EXPR_CREATE(ExprUnary, expr, op, type);
    expr->unary.child = child;
    return expr;
}

ExprDef *
ExprCreateBinary(enum expr_op_type op, ExprDef *left, ExprDef *right)
{
    EXPR_CREATE(ExprBinary, expr, op, EXPR_TYPE_UNKNOWN);

    if (op == EXPR_ASSIGN || left->expr.value_type == EXPR_TYPE_UNKNOWN)
        expr->expr.value_type = right->expr.value_type;
    else if (left->expr.value_type == right->expr.value_type ||
             right->expr.value_type == EXPR_TYPE_UNKNOWN)
        expr->expr.value_type = left->expr.value_type;
    expr->binary.left = left;
    expr->binary.right = right;

    return expr;
}

ExprDef *
ExprCreateFieldRef(xkb_atom_t element, xkb_atom_t field)
{
    EXPR_CREATE(ExprFieldRef, expr, EXPR_FIELD_REF, EXPR_TYPE_UNKNOWN);
    expr->field_ref.element = element;
    expr->field_ref.field = field;
    return expr;
}

ExprDef *
ExprCreateArrayRef(xkb_atom_t element, xkb_atom_t field, ExprDef *entry)
{
    EXPR_CREATE(ExprArrayRef, expr, EXPR_ARRAY_REF, EXPR_TYPE_UNKNOWN);
    expr->array_ref.element = element;
    expr->array_ref.field = field;
    expr->array_ref.entry = entry;
    return expr;
}

ExprDef *
ExprCreateAction(xkb_atom_t name, ExprDef *args)
{
    EXPR_CREATE(ExprAction, expr, EXPR_ACTION_DECL, EXPR_TYPE_UNKNOWN);
    expr->action.name = name;
    expr->action.args = args;
    return expr;
}

ExprDef *
ExprCreateKeysymList(xkb_keysym_t sym)
{
    EXPR_CREATE(ExprKeysymList, expr, EXPR_KEYSYM_LIST, EXPR_TYPE_SYMBOLS);

    darray_init(expr->keysym_list.syms);
    darray_init(expr->keysym_list.symsMapIndex);
    darray_init(expr->keysym_list.symsNumEntries);

    darray_append(expr->keysym_list.syms, sym);
    darray_append(expr->keysym_list.symsMapIndex, 0);
    darray_append(expr->keysym_list.symsNumEntries, 1);

    return expr;
}

ExprDef *
ExprCreateMultiKeysymList(ExprDef *expr)
{
    unsigned nLevels = darray_size(expr->keysym_list.symsMapIndex);

    darray_resize(expr->keysym_list.symsMapIndex, 1);
    darray_resize(expr->keysym_list.symsNumEntries, 1);
    darray_item(expr->keysym_list.symsMapIndex, 0) = 0;
    darray_item(expr->keysym_list.symsNumEntries, 0) = nLevels;

    return expr;
}

ExprDef *
ExprAppendKeysymList(ExprDef *expr, xkb_keysym_t sym)
{
    unsigned nSyms = darray_size(expr->keysym_list.syms);

    darray_append(expr->keysym_list.symsMapIndex, nSyms);
    darray_append(expr->keysym_list.symsNumEntries, 1);
    darray_append(expr->keysym_list.syms, sym);

    return expr;
}

ExprDef *
ExprAppendMultiKeysymList(ExprDef *expr, ExprDef *append)
{
    unsigned nSyms = darray_size(expr->keysym_list.syms);
    unsigned numEntries = darray_size(append->keysym_list.syms);

    darray_append(expr->keysym_list.symsMapIndex, nSyms);
    darray_append(expr->keysym_list.symsNumEntries, numEntries);
    darray_append_items(expr->keysym_list.syms,
                        darray_mem(append->keysym_list.syms, 0), numEntries);

    darray_resize(append->keysym_list.syms, 0);
    FreeStmt(&append->common);

    return expr;
}

KeycodeDef *
KeycodeCreate(xkb_atom_t name, int64_t value)
{
    KeycodeDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_KEYCODE;
    def->common.next = NULL;
    def->name = name;
    def->value = value;

    return def;
}

KeyAliasDef *
KeyAliasCreate(xkb_atom_t alias, xkb_atom_t real)
{
    KeyAliasDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_ALIAS;
    def->common.next = NULL;
    def->alias = alias;
    def->real = real;

    return def;
}

VModDef *
VModCreate(xkb_atom_t name, ExprDef *value)
{
    VModDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_VMOD;
    def->common.next = NULL;
    def->name = name;
    def->value = value;

    return def;
}

VarDef *
VarCreate(ExprDef *name, ExprDef *value)
{
    VarDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_VAR;
    def->common.next = NULL;
    def->name = name;
    def->value = value;

    return def;
}

VarDef *
BoolVarCreate(xkb_atom_t ident, bool set)
{
    return VarCreate((ExprDef *) ExprCreateIdent(ident),
                     (ExprDef *) ExprCreateBoolean(set));
}

InterpDef *
InterpCreate(xkb_keysym_t sym, ExprDef *match)
{
    InterpDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_INTERP;
    def->common.next = NULL;
    def->sym = sym;
    def->match = match;

    return def;
}

KeyTypeDef *
KeyTypeCreate(xkb_atom_t name, VarDef *body)
{
    KeyTypeDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_TYPE;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->name = name;
    def->body = body;

    return def;
}

SymbolsDef *
SymbolsCreate(xkb_atom_t keyName, VarDef *symbols)
{
    SymbolsDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_SYMBOLS;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->keyName = keyName;
    def->symbols = symbols;

    return def;
}

GroupCompatDef *
GroupCompatCreate(unsigned group, ExprDef *val)
{
    GroupCompatDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_GROUP_COMPAT;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->group = group;
    def->def = val;

    return def;
}

ModMapDef *
ModMapCreate(xkb_atom_t modifier, ExprDef *keys)
{
    ModMapDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_MODMAP;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->modifier = modifier;
    def->keys = keys;

    return def;
}

LedMapDef *
LedMapCreate(xkb_atom_t name, VarDef *body)
{
    LedMapDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_LED_MAP;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->name = name;
    def->body = body;

    return def;
}

LedNameDef *
LedNameCreate(unsigned ndx, ExprDef *name, bool virtual)
{
    LedNameDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_LED_NAME;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->ndx = ndx;
    def->name = name;
    def->virtual = virtual;

    return def;
}

static void
FreeInclude(IncludeStmt *incl);

IncludeStmt *
IncludeCreate(struct xkb_context *ctx, char *str, enum merge_mode merge)
{
    IncludeStmt *incl, *first;
    char *file, *map, *stmt, *tmp, *extra_data;
    char nextop;

    incl = first = NULL;
    file = map = NULL;
    tmp = str;
    stmt = strdup_safe(str);
    while (tmp && *tmp)
    {
        if (!ParseIncludeMap(&tmp, &file, &map, &nextop, &extra_data))
            goto err;

        /*
         * Given an RMLVO (here layout) like 'us,,fr', the rules parser
         * will give out something like 'pc+us+:2+fr:3+inet(evdev)'.
         * We should just skip the ':2' in this case and leave it to the
         * appropriate section to deal with the empty group.
         */
        if (isempty(file)) {
            free(file);
            free(map);
            free(extra_data);
            continue;
        }

        if (first == NULL) {
            first = incl = malloc(sizeof(*first));
        } else {
            incl->next_incl = malloc(sizeof(*first));
            incl = incl->next_incl;
        }

        if (!incl) {
            log_wsgo(ctx,
                     "Allocation failure in IncludeCreate; "
                     "Using only part of the include\n");
            break;
        }

        incl->common.type = STMT_INCLUDE;
        incl->common.next = NULL;
        incl->merge = merge;
        incl->stmt = NULL;
        incl->file = file;
        incl->map = map;
        incl->modifier = extra_data;
        incl->next_incl = NULL;

        if (nextop == '|')
            merge = MERGE_AUGMENT;
        else
            merge = MERGE_OVERRIDE;
    }

    if (first)
        first->stmt = stmt;
    else
        free(stmt);

    return first;

err:
    log_err(ctx, "Illegal include statement \"%s\"; Ignored\n", stmt);
    FreeInclude(first);
    free(stmt);
    return NULL;
}

XkbFile *
XkbFileCreate(enum xkb_file_type type, char *name, ParseCommon *defs,
              enum xkb_map_flags flags)
{
    XkbFile *file;

    file = calloc(1, sizeof(*file));
    if (!file)
        return NULL;

    XkbEscapeMapName(name);
    file->file_type = type;
    file->topName = strdup_safe(name);
    file->name = name;
    file->defs = defs;
    file->flags = flags;

    return file;
}

XkbFile *
XkbFileFromComponents(struct xkb_context *ctx,
                      const struct xkb_component_names *kkctgs)
{
    char *const components[] = {
        kkctgs->keycodes, kkctgs->types,
        kkctgs->compat, kkctgs->symbols,
    };
    enum xkb_file_type type;
    IncludeStmt *include = NULL;
    XkbFile *file = NULL;
    ParseCommon *defs = NULL;

    for (type = FIRST_KEYMAP_FILE_TYPE; type <= LAST_KEYMAP_FILE_TYPE; type++) {
        include = IncludeCreate(ctx, components[type], MERGE_DEFAULT);
        if (!include)
            goto err;

        file = XkbFileCreate(type, NULL, (ParseCommon *) include, 0);
        if (!file) {
            FreeInclude(include);
            goto err;
        }

        defs = AppendStmt(defs, &file->common);
    }

    file = XkbFileCreate(FILE_TYPE_KEYMAP, NULL, defs, 0);
    if (!file)
        goto err;

    return file;

err:
    FreeXkbFile((XkbFile *) defs);
    return NULL;
}

static void
FreeExpr(ExprDef *expr)
{
    if (!expr)
        return;

    switch (expr->expr.op) {
    case EXPR_ACTION_LIST:
    case EXPR_NEGATE:
    case EXPR_UNARY_PLUS:
    case EXPR_NOT:
    case EXPR_INVERT:
        FreeStmt((ParseCommon *) expr->unary.child);
        break;

    case EXPR_DIVIDE:
    case EXPR_ADD:
    case EXPR_SUBTRACT:
    case EXPR_MULTIPLY:
    case EXPR_ASSIGN:
        FreeStmt((ParseCommon *) expr->binary.left);
        FreeStmt((ParseCommon *) expr->binary.right);
        break;

    case EXPR_ACTION_DECL:
        FreeStmt((ParseCommon *) expr->action.args);
        break;

    case EXPR_ARRAY_REF:
        FreeStmt((ParseCommon *) expr->array_ref.entry);
        break;

    case EXPR_KEYSYM_LIST:
        darray_free(expr->keysym_list.syms);
        darray_free(expr->keysym_list.symsMapIndex);
        darray_free(expr->keysym_list.symsNumEntries);
        break;

    default:
        break;
    }
}

static void
FreeInclude(IncludeStmt *incl)
{
    IncludeStmt *next;

    while (incl)
    {
        next = incl->next_incl;

        free(incl->file);
        free(incl->map);
        free(incl->modifier);
        free(incl->stmt);

        free(incl);
        incl = next;
    }
}

void
FreeStmt(ParseCommon *stmt)
{
    ParseCommon *next;

    while (stmt)
    {
        next = stmt->next;

        switch (stmt->type) {
        case STMT_INCLUDE:
            FreeInclude((IncludeStmt *) stmt);
            /* stmt is already free'd here. */
            stmt = NULL;
            break;
        case STMT_EXPR:
            FreeExpr((ExprDef *) stmt);
            break;
        case STMT_VAR:
            FreeStmt((ParseCommon *) ((VarDef *) stmt)->name);
            FreeStmt((ParseCommon *) ((VarDef *) stmt)->value);
            break;
        case STMT_TYPE:
            FreeStmt((ParseCommon *) ((KeyTypeDef *) stmt)->body);
            break;
        case STMT_INTERP:
            FreeStmt((ParseCommon *) ((InterpDef *) stmt)->match);
            FreeStmt((ParseCommon *) ((InterpDef *) stmt)->def);
            break;
        case STMT_VMOD:
            FreeStmt((ParseCommon *) ((VModDef *) stmt)->value);
            break;
        case STMT_SYMBOLS:
            FreeStmt((ParseCommon *) ((SymbolsDef *) stmt)->symbols);
            break;
        case STMT_MODMAP:
            FreeStmt((ParseCommon *) ((ModMapDef *) stmt)->keys);
            break;
        case STMT_GROUP_COMPAT:
            FreeStmt((ParseCommon *) ((GroupCompatDef *) stmt)->def);
            break;
        case STMT_LED_MAP:
            FreeStmt((ParseCommon *) ((LedMapDef *) stmt)->body);
            break;
        case STMT_LED_NAME:
            FreeStmt((ParseCommon *) ((LedNameDef *) stmt)->name);
            break;
        default:
            break;
        }

        free(stmt);
        stmt = next;
    }
}

void
FreeXkbFile(XkbFile *file)
{
    XkbFile *next;

    while (file)
    {
        next = (XkbFile *) file->common.next;

        switch (file->file_type) {
        case FILE_TYPE_KEYMAP:
            FreeXkbFile((XkbFile *) file->defs);
            break;

        case FILE_TYPE_TYPES:
        case FILE_TYPE_COMPAT:
        case FILE_TYPE_SYMBOLS:
        case FILE_TYPE_KEYCODES:
        case FILE_TYPE_GEOMETRY:
            FreeStmt(file->defs);
            break;

        default:
            break;
        }

        free(file->name);
        free(file->topName);
        free(file);
        file = next;
    }
}

static const char *xkb_file_type_strings[_FILE_TYPE_NUM_ENTRIES] = {
    [FILE_TYPE_KEYCODES] = "xkb_keycodes",
    [FILE_TYPE_TYPES] = "xkb_types",
    [FILE_TYPE_COMPAT] = "xkb_compatibility",
    [FILE_TYPE_SYMBOLS] = "xkb_symbols",
    [FILE_TYPE_GEOMETRY] = "xkb_geometry",
    [FILE_TYPE_KEYMAP] = "xkb_keymap",
    [FILE_TYPE_RULES] = "rules",
};

const char *
xkb_file_type_to_string(enum xkb_file_type type)
{
    if (type > _FILE_TYPE_NUM_ENTRIES)
        return "unknown";
    return xkb_file_type_strings[type];
}

static const char *stmt_type_strings[_STMT_NUM_VALUES] = {
    [STMT_UNKNOWN] = "unknown statement",
    [STMT_INCLUDE] = "include statement",
    [STMT_KEYCODE] = "key name definition",
    [STMT_ALIAS] = "key alias definition",
    [STMT_EXPR] = "expression",
    [STMT_VAR] = "variable definition",
    [STMT_TYPE] = "key type definition",
    [STMT_INTERP] = "symbol interpretation definition",
    [STMT_VMOD] = "virtual modifiers definition",
    [STMT_SYMBOLS] = "key symbols definition",
    [STMT_MODMAP] = "modifier map declaration",
    [STMT_GROUP_COMPAT] = "group declaration",
    [STMT_LED_MAP] = "indicator map declaration",
    [STMT_LED_NAME] = "indicator name declaration",
};

const char *
stmt_type_to_string(enum stmt_type type)
{
    if (type >= _STMT_NUM_VALUES)
        return NULL;
    return stmt_type_strings[type];
}

static const char *expr_op_type_strings[_EXPR_NUM_VALUES] = {
    [EXPR_VALUE] = "literal",
    [EXPR_IDENT] = "identifier",
    [EXPR_ACTION_DECL] = "action declaration",
    [EXPR_FIELD_REF] = "field reference",
    [EXPR_ARRAY_REF] = "array reference",
    [EXPR_KEYSYM_LIST] = "list of keysyms",
    [EXPR_ACTION_LIST] = "list of actions",
    [EXPR_ADD] = "addition",
    [EXPR_SUBTRACT] = "subtraction",
    [EXPR_MULTIPLY] = "multiplication",
    [EXPR_DIVIDE] = "division",
    [EXPR_ASSIGN] = "assignment",
    [EXPR_NOT] = "logical negation",
    [EXPR_NEGATE] = "arithmetic negation",
    [EXPR_INVERT] = "bitwise inversion",
    [EXPR_UNARY_PLUS] = "unary plus",
};

const char *
expr_op_type_to_string(enum expr_op_type type)
{
    if (type >= _EXPR_NUM_VALUES)
        return NULL;
    return expr_op_type_strings[type];
}

static const char *expr_value_type_strings[_EXPR_TYPE_NUM_VALUES] = {
    [EXPR_TYPE_UNKNOWN] = "unknown",
    [EXPR_TYPE_BOOLEAN] = "boolean",
    [EXPR_TYPE_INT] = "int",
    [EXPR_TYPE_STRING] = "string",
    [EXPR_TYPE_ACTION] = "action",
    [EXPR_TYPE_KEYNAME] = "keyname",
    [EXPR_TYPE_SYMBOLS] = "symbols",
};

const char *
expr_value_type_to_string(enum expr_value_type type)
{
    if (type >= _EXPR_TYPE_NUM_VALUES)
        return NULL;
    return expr_value_type_strings[type];
}
