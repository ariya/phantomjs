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

#include "xkbcomp-priv.h"
#include "text.h"
#include "expr.h"

typedef bool (*IdentLookupFunc)(struct xkb_context *ctx, const void *priv,
                                xkb_atom_t field, enum expr_value_type type,
                                unsigned int *val_rtrn);

bool
ExprResolveLhs(struct xkb_context *ctx, const ExprDef *expr,
               const char **elem_rtrn, const char **field_rtrn,
               ExprDef **index_rtrn)
{
    switch (expr->expr.op) {
    case EXPR_IDENT:
        *elem_rtrn = NULL;
        *field_rtrn = xkb_atom_text(ctx, expr->ident.ident);
        *index_rtrn = NULL;
        return true;
    case EXPR_FIELD_REF:
        *elem_rtrn = xkb_atom_text(ctx, expr->field_ref.element);
        *field_rtrn = xkb_atom_text(ctx, expr->field_ref.field);
        *index_rtrn = NULL;
        return true;
    case EXPR_ARRAY_REF:
        *elem_rtrn = xkb_atom_text(ctx, expr->array_ref.element);
        *field_rtrn = xkb_atom_text(ctx, expr->array_ref.field);
        *index_rtrn = expr->array_ref.entry;
        return true;
    default:
        break;
    }
    log_wsgo(ctx, "Unexpected operator %d in ResolveLhs\n", expr->expr.op);
    return false;
}

static bool
SimpleLookup(struct xkb_context *ctx, const void *priv, xkb_atom_t field,
             enum expr_value_type type, unsigned int *val_rtrn)
{
    const LookupEntry *entry;
    const char *str;

    if (!priv || field == XKB_ATOM_NONE || type != EXPR_TYPE_INT)
        return false;

    str = xkb_atom_text(ctx, field);
    for (entry = priv; entry && entry->name; entry++) {
        if (istreq(str, entry->name)) {
            *val_rtrn = entry->value;
            return true;
        }
    }

    return false;
}

/* Data passed in the *priv argument for LookupModMask. */
typedef struct {
    const struct xkb_keymap *keymap;
    enum mod_type mod_type;
} LookupModMaskPriv;

static bool
LookupModMask(struct xkb_context *ctx, const void *priv, xkb_atom_t field,
              enum expr_value_type type, xkb_mod_mask_t *val_rtrn)
{
    const char *str;
    xkb_mod_index_t ndx;
    const LookupModMaskPriv *arg = priv;
    const struct xkb_keymap *keymap = arg->keymap;
    enum mod_type mod_type = arg->mod_type;

    if (type != EXPR_TYPE_INT)
        return false;

    str = xkb_atom_text(ctx, field);

    if (istreq(str, "all")) {
        *val_rtrn  = MOD_REAL_MASK_ALL;
        return true;
    }

    if (istreq(str, "none")) {
        *val_rtrn = 0;
        return true;
    }

    ndx = ModNameToIndex(keymap, field, mod_type);
    if (ndx == XKB_MOD_INVALID)
        return false;

    *val_rtrn = (1u << ndx);
    return true;
}

bool
ExprResolveBoolean(struct xkb_context *ctx, const ExprDef *expr,
                   bool *set_rtrn)
{
    bool ok = false;
    const char *ident;

    switch (expr->expr.op) {
    case EXPR_VALUE:
        if (expr->expr.value_type != EXPR_TYPE_BOOLEAN) {
            log_err(ctx,
                    "Found constant of type %s where boolean was expected\n",
                    expr_value_type_to_string(expr->expr.value_type));
            return false;
        }
        *set_rtrn = expr->boolean.set;
        return true;

    case EXPR_IDENT:
        ident = xkb_atom_text(ctx, expr->ident.ident);
        if (ident) {
            if (istreq(ident, "true") ||
                istreq(ident, "yes") ||
                istreq(ident, "on")) {
                *set_rtrn = true;
                return true;
            }
            else if (istreq(ident, "false") ||
                     istreq(ident, "no") ||
                     istreq(ident, "off")) {
                *set_rtrn = false;
                return true;
            }
        }
        log_err(ctx, "Identifier \"%s\" of type boolean is unknown\n", ident);
        return false;

    case EXPR_FIELD_REF:
        log_err(ctx, "Default \"%s.%s\" of type boolean is unknown\n",
                xkb_atom_text(ctx, expr->field_ref.element),
                xkb_atom_text(ctx, expr->field_ref.field));
        return false;

    case EXPR_INVERT:
    case EXPR_NOT:
        ok = ExprResolveBoolean(ctx, expr, set_rtrn);
        if (ok)
            *set_rtrn = !*set_rtrn;
        return ok;
    case EXPR_ADD:
    case EXPR_SUBTRACT:
    case EXPR_MULTIPLY:
    case EXPR_DIVIDE:
    case EXPR_ASSIGN:
    case EXPR_NEGATE:
    case EXPR_UNARY_PLUS:
        log_err(ctx, "%s of boolean values not permitted\n",
                expr_op_type_to_string(expr->expr.op));
        break;

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveBoolean\n",
                 expr->expr.op);
        break;
    }

    return false;
}

bool
ExprResolveKeyCode(struct xkb_context *ctx, const ExprDef *expr,
                   xkb_keycode_t *kc)
{
    xkb_keycode_t leftRtrn, rightRtrn;

    switch (expr->expr.op) {
    case EXPR_VALUE:
        if (expr->expr.value_type != EXPR_TYPE_INT) {
            log_err(ctx,
                    "Found constant of type %s where an int was expected\n",
                    expr_value_type_to_string(expr->expr.value_type));
            return false;
        }

        *kc = (xkb_keycode_t) expr->integer.ival;
        return true;

    case EXPR_ADD:
    case EXPR_SUBTRACT:
    case EXPR_MULTIPLY:
    case EXPR_DIVIDE:
        if (!ExprResolveKeyCode(ctx, expr->binary.left, &leftRtrn) ||
            !ExprResolveKeyCode(ctx, expr->binary.right, &rightRtrn))
            return false;

        switch (expr->expr.op) {
        case EXPR_ADD:
            *kc = leftRtrn + rightRtrn;
            break;
        case EXPR_SUBTRACT:
            *kc = leftRtrn - rightRtrn;
            break;
        case EXPR_MULTIPLY:
            *kc = leftRtrn * rightRtrn;
            break;
        case EXPR_DIVIDE:
            if (rightRtrn == 0) {
                log_err(ctx, "Cannot divide by zero: %d / %d\n",
                        leftRtrn, rightRtrn);
                return false;
            }

            *kc = leftRtrn / rightRtrn;
            break;
        default:
            break;
        }

        return true;

    case EXPR_NEGATE:
        if (!ExprResolveKeyCode(ctx, expr->unary.child, &leftRtrn))
            return false;

        *kc = ~leftRtrn;
        return true;

    case EXPR_UNARY_PLUS:
        return ExprResolveKeyCode(ctx, expr->unary.child, kc);

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveKeyCode\n",
                 expr->expr.op);
        break;
    }

    return false;
}

/**
 * This function returns ... something.  It's a bit of a guess, really.
 *
 * If an integer is given in value ctx, it will be returned in ival.
 * If an ident or field reference is given, the lookup function (if given)
 * will be called.  At the moment, only SimpleLookup use this, and they both
 * return the results in uval.  And don't support field references.
 *
 * Cool.
 */
static bool
ExprResolveIntegerLookup(struct xkb_context *ctx, const ExprDef *expr,
                         int *val_rtrn, IdentLookupFunc lookup,
                         const void *lookupPriv)
{
    bool ok = false;
    int l, r;
    unsigned u;
    ExprDef *left, *right;

    switch (expr->expr.op) {
    case EXPR_VALUE:
        if (expr->expr.value_type != EXPR_TYPE_INT) {
            log_err(ctx,
                    "Found constant of type %s where an int was expected\n",
                    expr_value_type_to_string(expr->expr.value_type));
            return false;
        }

        *val_rtrn = expr->integer.ival;
        return true;

    case EXPR_IDENT:
        if (lookup)
            ok = lookup(ctx, lookupPriv, expr->ident.ident, EXPR_TYPE_INT, &u);

        if (!ok)
            log_err(ctx, "Identifier \"%s\" of type int is unknown\n",
                    xkb_atom_text(ctx, expr->ident.ident));
        else
            *val_rtrn = (int) u;

        return ok;

    case EXPR_FIELD_REF:
        log_err(ctx, "Default \"%s.%s\" of type int is unknown\n",
                xkb_atom_text(ctx, expr->field_ref.element),
                xkb_atom_text(ctx, expr->field_ref.field));
        return false;

    case EXPR_ADD:
    case EXPR_SUBTRACT:
    case EXPR_MULTIPLY:
    case EXPR_DIVIDE:
        left = expr->binary.left;
        right = expr->binary.right;
        if (!ExprResolveIntegerLookup(ctx, left, &l, lookup, lookupPriv) ||
            !ExprResolveIntegerLookup(ctx, right, &r, lookup, lookupPriv))
            return false;

        switch (expr->expr.op) {
        case EXPR_ADD:
            *val_rtrn = l + r;
            break;
        case EXPR_SUBTRACT:
            *val_rtrn = l - r;
            break;
        case EXPR_MULTIPLY:
            *val_rtrn = l * r;
            break;
        case EXPR_DIVIDE:
            if (r == 0) {
                log_err(ctx, "Cannot divide by zero: %d / %d\n", l, r);
                return false;
            }
            *val_rtrn = l / r;
            break;
        default:
            break;
        }

        return true;

    case EXPR_ASSIGN:
        log_wsgo(ctx, "Assignment operator not implemented yet\n");
        break;

    case EXPR_NOT:
        log_err(ctx, "The ! operator cannot be applied to an integer\n");
        return false;

    case EXPR_INVERT:
    case EXPR_NEGATE:
        left = expr->unary.child;
        if (!ExprResolveIntegerLookup(ctx, left, &l, lookup, lookupPriv))
            return false;

        *val_rtrn = (expr->expr.op == EXPR_NEGATE ? -l : ~l);
        return true;

    case EXPR_UNARY_PLUS:
        left = expr->unary.child;
        return ExprResolveIntegerLookup(ctx, left, val_rtrn, lookup,
                                        lookupPriv);

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveInteger\n",
                 expr->expr.op);
        break;
    }

    return false;
}

bool
ExprResolveInteger(struct xkb_context *ctx, const ExprDef *expr,
                   int *val_rtrn)
{
    return ExprResolveIntegerLookup(ctx, expr, val_rtrn, NULL, NULL);
}

bool
ExprResolveGroup(struct xkb_context *ctx, const ExprDef *expr,
                 xkb_layout_index_t *group_rtrn)
{
    bool ok;
    int result;

    ok = ExprResolveIntegerLookup(ctx, expr, &result, SimpleLookup,
                                  groupNames);
    if (!ok)
        return false;

    if (result <= 0 || result > XKB_MAX_GROUPS) {
        log_err(ctx, "Group index %u is out of range (1..%d)\n",
                result, XKB_MAX_GROUPS);
        return false;
    }

    *group_rtrn = (xkb_layout_index_t) result;
    return true;
}

bool
ExprResolveLevel(struct xkb_context *ctx, const ExprDef *expr,
                 xkb_level_index_t *level_rtrn)
{
    bool ok;
    int result;

    ok = ExprResolveIntegerLookup(ctx, expr, &result, SimpleLookup,
                                  levelNames);
    if (!ok)
        return false;

    if (result < 1) {
        log_err(ctx, "Shift level %d is out of range\n", result);
        return false;
    }

    /* Level is zero-indexed from now on. */
    *level_rtrn = (unsigned int) (result - 1);
    return true;
}

bool
ExprResolveButton(struct xkb_context *ctx, const ExprDef *expr, int *btn_rtrn)
{
    return ExprResolveIntegerLookup(ctx, expr, btn_rtrn, SimpleLookup,
                                    buttonNames);
}

bool
ExprResolveString(struct xkb_context *ctx, const ExprDef *expr,
                  xkb_atom_t *val_rtrn)
{
    switch (expr->expr.op) {
    case EXPR_VALUE:
        if (expr->expr.value_type != EXPR_TYPE_STRING) {
            log_err(ctx, "Found constant of type %s, expected a string\n",
                    expr_value_type_to_string(expr->expr.value_type));
            return false;
        }

        *val_rtrn = expr->string.str;
        return true;

    case EXPR_IDENT:
        log_err(ctx, "Identifier \"%s\" of type string not found\n",
                xkb_atom_text(ctx, expr->ident.ident));
        return false;

    case EXPR_FIELD_REF:
        log_err(ctx, "Default \"%s.%s\" of type string not found\n",
                xkb_atom_text(ctx, expr->field_ref.element),
                xkb_atom_text(ctx, expr->field_ref.field));
        return false;

    case EXPR_ADD:
    case EXPR_SUBTRACT:
    case EXPR_MULTIPLY:
    case EXPR_DIVIDE:
    case EXPR_ASSIGN:
    case EXPR_NEGATE:
    case EXPR_INVERT:
    case EXPR_NOT:
    case EXPR_UNARY_PLUS:
        log_err(ctx, "%s of strings not permitted\n",
                expr_op_type_to_string(expr->expr.op));
        return false;

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveString\n",
                 expr->expr.op);
        break;
    }
    return false;
}

bool
ExprResolveEnum(struct xkb_context *ctx, const ExprDef *expr,
                unsigned int *val_rtrn, const LookupEntry *values)
{
    if (expr->expr.op != EXPR_IDENT) {
        log_err(ctx, "Found a %s where an enumerated value was expected\n",
                expr_op_type_to_string(expr->expr.op));
        return false;
    }

    if (!SimpleLookup(ctx, values, expr->ident.ident, EXPR_TYPE_INT,
                      val_rtrn)) {
        log_err(ctx, "Illegal identifier %s; expected one of:\n",
                xkb_atom_text(ctx, expr->ident.ident));
        while (values && values->name)
        {
            log_err(ctx, "\t%s\n", values->name);
            values++;
        }
        return false;
    }

    return true;
}

static bool
ExprResolveMaskLookup(struct xkb_context *ctx, const ExprDef *expr,
                      unsigned int *val_rtrn, IdentLookupFunc lookup,
                      const void *lookupPriv)
{
    bool ok = 0;
    unsigned int l, r;
    int v;
    ExprDef *left, *right;
    const char *bogus = NULL;

    switch (expr->expr.op) {
    case EXPR_VALUE:
        if (expr->expr.value_type != EXPR_TYPE_INT) {
            log_err(ctx,
                    "Found constant of type %s where a mask was expected\n",
                    expr_value_type_to_string(expr->expr.value_type));
            return false;
        }
        *val_rtrn = (unsigned int) expr->integer.ival;
        return true;

    case EXPR_IDENT:
        ok = lookup(ctx, lookupPriv, expr->ident.ident, EXPR_TYPE_INT,
                    val_rtrn);
        if (!ok)
            log_err(ctx, "Identifier \"%s\" of type int is unknown\n",
                    xkb_atom_text(ctx, expr->ident.ident));
        return ok;

    case EXPR_FIELD_REF:
        log_err(ctx, "Default \"%s.%s\" of type int is unknown\n",
                xkb_atom_text(ctx, expr->field_ref.element),
                xkb_atom_text(ctx, expr->field_ref.field));
        return false;

    case EXPR_ARRAY_REF:
        bogus = "array reference";

    case EXPR_ACTION_DECL:
        if (bogus == NULL)
            bogus = "function use";
        log_err(ctx,
                "Unexpected %s in mask expression; Expression Ignored\n",
                bogus);
        return false;

    case EXPR_ADD:
    case EXPR_SUBTRACT:
    case EXPR_MULTIPLY:
    case EXPR_DIVIDE:
        left = expr->binary.left;
        right = expr->binary.right;
        if (!ExprResolveMaskLookup(ctx, left, &l, lookup, lookupPriv) ||
            !ExprResolveMaskLookup(ctx, right, &r, lookup, lookupPriv))
            return false;

        switch (expr->expr.op) {
        case EXPR_ADD:
            *val_rtrn = l | r;
            break;
        case EXPR_SUBTRACT:
            *val_rtrn = l & (~r);
            break;
        case EXPR_MULTIPLY:
        case EXPR_DIVIDE:
            log_err(ctx, "Cannot %s masks; Illegal operation ignored\n",
                    (expr->expr.op == EXPR_DIVIDE ? "divide" : "multiply"));
            return false;
        default:
            break;
        }

        return true;

    case EXPR_ASSIGN:
        log_wsgo(ctx, "Assignment operator not implemented yet\n");
        break;

    case EXPR_INVERT:
        left = expr->unary.child;
        if (!ExprResolveIntegerLookup(ctx, left, &v, lookup, lookupPriv))
            return false;

        *val_rtrn = ~v;
        return true;

    case EXPR_UNARY_PLUS:
    case EXPR_NEGATE:
    case EXPR_NOT:
        left = expr->unary.child;
        if (!ExprResolveIntegerLookup(ctx, left, &v, lookup, lookupPriv))
            log_err(ctx, "The %s operator cannot be used with a mask\n",
                    (expr->expr.op == EXPR_NEGATE ? "-" : "!"));
        return false;

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveMask\n",
                 expr->expr.op);
        break;
    }

    return false;
}

bool
ExprResolveMask(struct xkb_context *ctx, const ExprDef *expr,
                unsigned int *mask_rtrn, const LookupEntry *values)
{
    return ExprResolveMaskLookup(ctx, expr, mask_rtrn, SimpleLookup, values);
}

bool
ExprResolveModMask(struct xkb_keymap *keymap, const ExprDef *expr,
                   enum mod_type mod_type, xkb_mod_mask_t *mask_rtrn)
{
    LookupModMaskPriv priv = { .keymap = keymap, .mod_type = mod_type };
    return ExprResolveMaskLookup(keymap->ctx, expr, mask_rtrn, LookupModMask,
                                 &priv);
}

bool
ExprResolveKeySym(struct xkb_context *ctx, const ExprDef *expr,
                  xkb_keysym_t *sym_rtrn)
{
    int val;

    if (expr->expr.op == EXPR_IDENT) {
        const char *str = xkb_atom_text(ctx, expr->ident.ident);
        *sym_rtrn = xkb_keysym_from_name(str, 0);
        if (*sym_rtrn != XKB_KEY_NoSymbol)
            return true;
    }

    if (!ExprResolveInteger(ctx, expr, &val))
        return false;

    if (val < 0 || val >= 10)
        return false;

    *sym_rtrn = XKB_KEY_0 + (xkb_keysym_t) val;
    return true;
}

bool
ExprResolveMod(struct xkb_keymap *keymap, const ExprDef *def,
               enum mod_type mod_type, xkb_mod_index_t *ndx_rtrn)
{
    xkb_mod_index_t ndx;
    xkb_atom_t name;

    if (def->expr.op != EXPR_IDENT) {
        log_err(keymap->ctx,
                "Cannot resolve virtual modifier: "
                "found %s where a virtual modifier name was expected\n",
                expr_op_type_to_string(def->expr.op));
        return false;
    }

    name = def->ident.ident;
    ndx = ModNameToIndex(keymap, name, mod_type);
    if (ndx == XKB_MOD_INVALID) {
        log_err(keymap->ctx,
                "Cannot resolve virtual modifier: "
                "\"%s\" was not previously declared\n",
                xkb_atom_text(keymap->ctx, name));
        return false;
    }

    *ndx_rtrn = ndx;
    return true;
}
