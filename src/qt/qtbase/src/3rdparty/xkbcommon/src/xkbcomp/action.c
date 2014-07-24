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
#include "text.h"
#include "expr.h"
#include "action.h"

static const ExprBoolean constTrue = {
    .expr = {
        .common = { .type = STMT_EXPR, .next = NULL },
        .op = EXPR_VALUE,
        .value_type = EXPR_TYPE_BOOLEAN,
    },
    .set = true,
};

static const ExprBoolean constFalse = {
    .expr = {
        .common = { .type = STMT_EXPR, .next = NULL },
        .op = EXPR_VALUE,
        .value_type = EXPR_TYPE_BOOLEAN,
    },
    .set = false,
};

enum action_field {
    ACTION_FIELD_CLEAR_LOCKS,
    ACTION_FIELD_LATCH_TO_LOCK,
    ACTION_FIELD_GEN_KEY_EVENT,
    ACTION_FIELD_REPORT,
    ACTION_FIELD_DEFAULT,
    ACTION_FIELD_AFFECT,
    ACTION_FIELD_INCREMENT,
    ACTION_FIELD_MODIFIERS,
    ACTION_FIELD_GROUP,
    ACTION_FIELD_X,
    ACTION_FIELD_Y,
    ACTION_FIELD_ACCEL,
    ACTION_FIELD_BUTTON,
    ACTION_FIELD_VALUE,
    ACTION_FIELD_CONTROLS,
    ACTION_FIELD_TYPE,
    ACTION_FIELD_COUNT,
    ACTION_FIELD_SCREEN,
    ACTION_FIELD_SAME,
    ACTION_FIELD_DATA,
    ACTION_FIELD_DEVICE,
    ACTION_FIELD_KEYCODE,
    ACTION_FIELD_MODS_TO_CLEAR,
};

ActionsInfo *
NewActionsInfo(void)
{
    unsigned type;
    ActionsInfo *info;

    info = calloc(1, sizeof(*info));
    if (!info)
        return NULL;

    for (type = 0; type < _ACTION_TYPE_NUM_ENTRIES; type++)
        info->actions[type].type = type;

    /* Apply some "factory defaults". */

    /* Increment default button. */
    info->actions[ACTION_TYPE_PTR_DEFAULT].dflt.flags = 0;
    info->actions[ACTION_TYPE_PTR_DEFAULT].dflt.value = 1;
    info->actions[ACTION_TYPE_PTR_MOVE].ptr.flags = ACTION_ACCEL;
    info->actions[ACTION_TYPE_SWITCH_VT].screen.flags = ACTION_SAME_SCREEN;

    return info;
}

void
FreeActionsInfo(ActionsInfo *info)
{
    free(info);
}

static const LookupEntry fieldStrings[] = {
    { "clearLocks",       ACTION_FIELD_CLEAR_LOCKS   },
    { "latchToLock",      ACTION_FIELD_LATCH_TO_LOCK },
    { "genKeyEvent",      ACTION_FIELD_GEN_KEY_EVENT },
    { "generateKeyEvent", ACTION_FIELD_GEN_KEY_EVENT },
    { "report",           ACTION_FIELD_REPORT        },
    { "default",          ACTION_FIELD_DEFAULT       },
    { "affect",           ACTION_FIELD_AFFECT        },
    { "increment",        ACTION_FIELD_INCREMENT     },
    { "modifiers",        ACTION_FIELD_MODIFIERS     },
    { "mods",             ACTION_FIELD_MODIFIERS     },
    { "group",            ACTION_FIELD_GROUP         },
    { "x",                ACTION_FIELD_X             },
    { "y",                ACTION_FIELD_Y             },
    { "accel",            ACTION_FIELD_ACCEL         },
    { "accelerate",       ACTION_FIELD_ACCEL         },
    { "repeat",           ACTION_FIELD_ACCEL         },
    { "button",           ACTION_FIELD_BUTTON        },
    { "value",            ACTION_FIELD_VALUE         },
    { "controls",         ACTION_FIELD_CONTROLS      },
    { "ctrls",            ACTION_FIELD_CONTROLS      },
    { "type",             ACTION_FIELD_TYPE          },
    { "count",            ACTION_FIELD_COUNT         },
    { "screen",           ACTION_FIELD_SCREEN        },
    { "same",             ACTION_FIELD_SAME          },
    { "sameServer",       ACTION_FIELD_SAME          },
    { "data",             ACTION_FIELD_DATA          },
    { "device",           ACTION_FIELD_DEVICE        },
    { "dev",              ACTION_FIELD_DEVICE        },
    { "key",              ACTION_FIELD_KEYCODE       },
    { "keycode",          ACTION_FIELD_KEYCODE       },
    { "kc",               ACTION_FIELD_KEYCODE       },
    { "clearmods",        ACTION_FIELD_MODS_TO_CLEAR },
    { "clearmodifiers",   ACTION_FIELD_MODS_TO_CLEAR },
    { NULL,               0                          }
};

static bool
stringToAction(const char *str, unsigned *type_rtrn)
{
    return LookupString(actionTypeNames, str, type_rtrn);
}

static bool
stringToField(const char *str, enum action_field *field_rtrn)
{
    return LookupString(fieldStrings, str, field_rtrn);
}

static const char *
fieldText(enum action_field field)
{
    return LookupValue(fieldStrings, field);
}

/***====================================================================***/

static inline bool
ReportMismatch(struct xkb_context *ctx, enum xkb_action_type action,
               enum action_field field, const char *type)
{
    log_err(ctx,
            "Value of %s field must be of type %s; "
            "Action %s definition ignored\n",
            fieldText(field), type, ActionTypeText(action));
    return false;
}

static inline bool
ReportIllegal(struct xkb_context *ctx, enum xkb_action_type action,
              enum action_field field)
{
    log_err(ctx,
            "Field %s is not defined for an action of type %s; "
            "Action definition ignored\n",
            fieldText(field), ActionTypeText(action));
    return false;
}

static inline bool
ReportActionNotArray(struct xkb_context *ctx, enum xkb_action_type action,
                     enum action_field field)
{
    log_err(ctx,
            "The %s field in the %s action is not an array; "
            "Action definition ignored\n",
            fieldText(field), ActionTypeText(action));
    return false;
}

static bool
HandleNoAction(struct xkb_keymap *keymap, union xkb_action *action,
               enum action_field field, const ExprDef *array_ndx,
               const ExprDef *value)

{
    return true;
}

static bool
CheckBooleanFlag(struct xkb_context *ctx, enum xkb_action_type action,
                 enum action_field field, enum xkb_action_flags flag,
                 const ExprDef *array_ndx, const ExprDef *value,
                 enum xkb_action_flags *flags_inout)
{
    bool set;

    if (array_ndx)
        return ReportActionNotArray(ctx, action, field);

    if (!ExprResolveBoolean(ctx, value, &set))
        return ReportMismatch(ctx, action, field, "boolean");

    if (set)
        *flags_inout |= flag;
    else
        *flags_inout &= ~flag;

    return true;
}

static bool
CheckModifierField(struct xkb_keymap *keymap, enum xkb_action_type action,
                   const ExprDef *array_ndx, const ExprDef *value,
                   enum xkb_action_flags *flags_inout, xkb_mod_mask_t *mods_rtrn)
{
    if (array_ndx)
        return ReportActionNotArray(keymap->ctx, action, ACTION_FIELD_MODIFIERS);

    if (value->expr.op == EXPR_IDENT) {
        const char *valStr;
        valStr = xkb_atom_text(keymap->ctx, value->ident.ident);
        if (valStr && (istreq(valStr, "usemodmapmods") ||
                       istreq(valStr, "modmapmods"))) {
            *mods_rtrn = 0;
            *flags_inout |= ACTION_MODS_LOOKUP_MODMAP;
            return true;
        }
    }

    if (!ExprResolveModMask(keymap, value, MOD_BOTH, mods_rtrn))
        return ReportMismatch(keymap->ctx, action,
                              ACTION_FIELD_MODIFIERS, "modifier mask");

    *flags_inout &= ~ACTION_MODS_LOOKUP_MODMAP;
    return true;
}

static const LookupEntry lockWhich[] = {
    { "both", 0 },
    { "lock", ACTION_LOCK_NO_UNLOCK },
    { "neither", (ACTION_LOCK_NO_LOCK | ACTION_LOCK_NO_UNLOCK) },
    { "unlock", ACTION_LOCK_NO_LOCK },
    { NULL, 0 }
};

static bool
CheckAffectField(struct xkb_context *ctx, enum xkb_action_type action,
                 const ExprDef *array_ndx, const ExprDef *value,
                 enum xkb_action_flags *flags_inout)
{
    enum xkb_action_flags flags;

    if (array_ndx)
        return ReportActionNotArray(ctx, action, ACTION_FIELD_AFFECT);

    if (!ExprResolveEnum(ctx, value, &flags, lockWhich))
        return ReportMismatch(ctx, action, ACTION_FIELD_AFFECT,
                              "lock, unlock, both, neither");

    *flags_inout &= ~(ACTION_LOCK_NO_LOCK | ACTION_LOCK_NO_UNLOCK);
    *flags_inout |= flags;
    return true;
}

static bool
HandleSetLatchLockMods(struct xkb_keymap *keymap, union xkb_action *action,
                       enum action_field field, const ExprDef *array_ndx,
                       const ExprDef *value)
{
    struct xkb_mod_action *act = &action->mods;
    const enum xkb_action_type type = action->type;

    if (field == ACTION_FIELD_MODIFIERS)
        return CheckModifierField(keymap, action->type, array_ndx, value,
                                  &act->flags, &act->mods.mods);
    if ((type == ACTION_TYPE_MOD_SET || type == ACTION_TYPE_MOD_LATCH) &&
        field == ACTION_FIELD_CLEAR_LOCKS)
        return CheckBooleanFlag(keymap->ctx, action->type, field,
                                ACTION_LOCK_CLEAR, array_ndx, value,
                                &act->flags);
    if (type == ACTION_TYPE_MOD_LATCH &&
        field == ACTION_FIELD_LATCH_TO_LOCK)
        return CheckBooleanFlag(keymap->ctx, action->type, field,
                                ACTION_LATCH_TO_LOCK, array_ndx, value,
                                &act->flags);
    if (type == ACTION_TYPE_MOD_LOCK &&
        field == ACTION_FIELD_AFFECT)
        return CheckAffectField(keymap->ctx, action->type, array_ndx, value,
                                &act->flags);

    return ReportIllegal(keymap->ctx, action->type, field);
}

static bool
CheckGroupField(struct xkb_context *ctx, unsigned action,
                const ExprDef *array_ndx, const ExprDef *value,
                enum xkb_action_flags *flags_inout, int32_t *group_rtrn)
{
    const ExprDef *spec;
    xkb_layout_index_t idx;
    enum xkb_action_flags flags = *flags_inout;

    if (array_ndx)
        return ReportActionNotArray(ctx, action, ACTION_FIELD_GROUP);

    if (value->expr.op == EXPR_NEGATE || value->expr.op == EXPR_UNARY_PLUS) {
        flags &= ~ACTION_ABSOLUTE_SWITCH;
        spec = value->unary.child;
    }
    else {
        flags |= ACTION_ABSOLUTE_SWITCH;
        spec = value;
    }

    if (!ExprResolveGroup(ctx, spec, &idx))
        return ReportMismatch(ctx, action, ACTION_FIELD_GROUP,
                              "integer (range 1..8)");

    /* +n, -n are relative, n is absolute. */
    if (value->expr.op == EXPR_NEGATE || value->expr.op == EXPR_UNARY_PLUS) {
        *group_rtrn = (int32_t) idx;
        if (value->expr.op == EXPR_NEGATE)
            *group_rtrn = -*group_rtrn;
    }
    else {
        *group_rtrn = (int32_t) (idx - 1);
    }
    *flags_inout = flags;
    return true;
}

static bool
HandleSetLatchLockGroup(struct xkb_keymap *keymap, union xkb_action *action,
                        enum action_field field, const ExprDef *array_ndx,
                        const ExprDef *value)
{
    struct xkb_group_action *act = &action->group;
    const enum xkb_action_type type = action->type;

    if (field == ACTION_FIELD_GROUP)
        return CheckGroupField(keymap->ctx, action->type, array_ndx, value,
                               &act->flags, &act->group);
    if ((type == ACTION_TYPE_GROUP_SET || type == ACTION_TYPE_GROUP_LATCH) &&
        field == ACTION_FIELD_CLEAR_LOCKS)
        return CheckBooleanFlag(keymap->ctx, action->type, field,
                                ACTION_LOCK_CLEAR, array_ndx, value,
                                &act->flags);
    if (type == ACTION_TYPE_GROUP_LATCH &&
        field == ACTION_FIELD_LATCH_TO_LOCK)
        return CheckBooleanFlag(keymap->ctx, action->type, field,
                                ACTION_LATCH_TO_LOCK, array_ndx, value,
                                &act->flags);

    return ReportIllegal(keymap->ctx, action->type, field);
}

static bool
HandleMovePtr(struct xkb_keymap *keymap, union xkb_action *action,
              enum action_field field, const ExprDef *array_ndx,
              const ExprDef *value)
{
    struct xkb_pointer_action *act = &action->ptr;

    if (field == ACTION_FIELD_X || field == ACTION_FIELD_Y) {
        int val;
        const bool absolute = (value->expr.op != EXPR_NEGATE &&
                               value->expr.op != EXPR_UNARY_PLUS);

        if (array_ndx)
            return ReportActionNotArray(keymap->ctx, action->type, field);

        if (!ExprResolveInteger(keymap->ctx, value, &val))
            return ReportMismatch(keymap->ctx, action->type, field, "integer");

        if (val < INT16_MIN || val > INT16_MAX) {
            log_err(keymap->ctx,
                    "The %s field in the %s action must be in range %d..%d; "
                    "Action definition ignored\n",
                    fieldText(field), ActionTypeText(action->type),
                    INT16_MIN, INT16_MAX);
            return false;
        }

        if (field == ACTION_FIELD_X) {
            if (absolute)
                act->flags |= ACTION_ABSOLUTE_X;
            act->x = (int16_t) val;
        }
        else {
            if (absolute)
                act->flags |= ACTION_ABSOLUTE_Y;
            act->y = (int16_t) val;
        }

        return true;
    }
    else if (field == ACTION_FIELD_ACCEL) {
        return CheckBooleanFlag(keymap->ctx, action->type, field,
                                ACTION_ACCEL, array_ndx, value, &act->flags);
    }

    return ReportIllegal(keymap->ctx, action->type, field);
}

static bool
HandlePtrBtn(struct xkb_keymap *keymap, union xkb_action *action,
             enum action_field field, const ExprDef *array_ndx,
             const ExprDef *value)
{
    struct xkb_pointer_button_action *act = &action->btn;

    if (field == ACTION_FIELD_BUTTON) {
        int btn;

        if (array_ndx)
            return ReportActionNotArray(keymap->ctx, action->type, field);

        if (!ExprResolveButton(keymap->ctx, value, &btn))
            return ReportMismatch(keymap->ctx, action->type, field,
                                  "integer (range 1..5)");

        if (btn < 0 || btn > 5) {
            log_err(keymap->ctx,
                    "Button must specify default or be in the range 1..5; "
                    "Illegal button value %d ignored\n", btn);
            return false;
        }

        act->button = btn;
        return true;
    }
    else if (action->type == ACTION_TYPE_PTR_LOCK &&
             field == ACTION_FIELD_AFFECT) {
        return CheckAffectField(keymap->ctx, action->type, array_ndx, value,
                                &act->flags);
    }
    else if (field == ACTION_FIELD_COUNT) {
        int val;

        if (array_ndx)
            return ReportActionNotArray(keymap->ctx, action->type, field);

        if (!ExprResolveInteger(keymap->ctx, value, &val))
            return ReportMismatch(keymap->ctx, action->type, field, "integer");

        if (val < 0 || val > 255) {
            log_err(keymap->ctx,
                    "The count field must have a value in the range 0..255; "
                    "Illegal count %d ignored\n", val);
            return false;
        }

        act->count = (uint8_t) val;
        return true;
    }

    return ReportIllegal(keymap->ctx, action->type, field);
}

static const LookupEntry ptrDflts[] = {
    { "dfltbtn", 1 },
    { "defaultbutton", 1 },
    { "button", 1 },
    { NULL, 0 }
};

static bool
HandleSetPtrDflt(struct xkb_keymap *keymap, union xkb_action *action,
                 enum action_field field, const ExprDef *array_ndx,
                 const ExprDef *value)
{
    struct xkb_pointer_default_action *act = &action->dflt;

    if (field == ACTION_FIELD_AFFECT) {
        unsigned int val;

        if (array_ndx)
            return ReportActionNotArray(keymap->ctx, action->type, field);

        if (!ExprResolveEnum(keymap->ctx, value, &val, ptrDflts))
            return ReportMismatch(keymap->ctx, action->type, field,
                                  "pointer component");
        return true;
    }
    else if (field == ACTION_FIELD_BUTTON || field == ACTION_FIELD_VALUE) {
        const ExprDef *button;
        int btn;

        if (array_ndx)
            return ReportActionNotArray(keymap->ctx, action->type, field);

        if (value->expr.op == EXPR_NEGATE ||
            value->expr.op == EXPR_UNARY_PLUS) {
            act->flags &= ~ACTION_ABSOLUTE_SWITCH;
            button = value->unary.child;
        }
        else {
            act->flags |= ACTION_ABSOLUTE_SWITCH;
            button = value;
        }

        if (!ExprResolveButton(keymap->ctx, button, &btn))
            return ReportMismatch(keymap->ctx, action->type, field,
                                  "integer (range 1..5)");

        if (btn < 0 || btn > 5) {
            log_err(keymap->ctx,
                    "New default button value must be in the range 1..5; "
                    "Illegal default button value %d ignored\n", btn);
            return false;
        }
        if (btn == 0) {
            log_err(keymap->ctx,
                    "Cannot set default pointer button to \"default\"; "
                    "Illegal default button setting ignored\n");
            return false;
        }

        act->value = (value->expr.op == EXPR_NEGATE ? -btn: btn);
        return true;
    }

    return ReportIllegal(keymap->ctx, action->type, field);
}

static bool
HandleSwitchScreen(struct xkb_keymap *keymap, union xkb_action *action,
                   enum action_field field, const ExprDef *array_ndx,
                   const ExprDef *value)
{
    struct xkb_switch_screen_action *act = &action->screen;

    if (field == ACTION_FIELD_SCREEN) {
        const ExprDef *scrn;
        int val;

        if (array_ndx)
            return ReportActionNotArray(keymap->ctx, action->type, field);

        if (value->expr.op == EXPR_NEGATE ||
            value->expr.op == EXPR_UNARY_PLUS) {
            act->flags &= ~ACTION_ABSOLUTE_SWITCH;
            scrn = value->unary.child;
        }
        else {
            act->flags |= ACTION_ABSOLUTE_SWITCH;
            scrn = value;
        }

        if (!ExprResolveInteger(keymap->ctx, scrn, &val))
            return ReportMismatch(keymap->ctx, action->type, field,
                                  "integer (0..255)");

        if (val < 0 || val > 255) {
            log_err(keymap->ctx,
                    "Screen index must be in the range 1..255; "
                    "Illegal screen value %d ignored\n", val);
            return false;
        }

        act->screen = (value->expr.op == EXPR_NEGATE ? -val : val);
        return true;
    }
    else if (field == ACTION_FIELD_SAME) {
        return CheckBooleanFlag(keymap->ctx, action->type, field,
                                ACTION_SAME_SCREEN, array_ndx, value,
                                &act->flags);
    }

    return ReportIllegal(keymap->ctx, action->type, field);
}

static bool
HandleSetLockControls(struct xkb_keymap *keymap, union xkb_action *action,
                      enum action_field field, const ExprDef *array_ndx,
                      const ExprDef *value)
{
    struct xkb_controls_action *act = &action->ctrls;

    if (field == ACTION_FIELD_CONTROLS) {
        unsigned int mask;

        if (array_ndx)
            return ReportActionNotArray(keymap->ctx, action->type, field);

        if (!ExprResolveMask(keymap->ctx, value, &mask, ctrlMaskNames))
            return ReportMismatch(keymap->ctx, action->type, field,
                                  "controls mask");

        act->ctrls = mask;
        return true;
    }
    else if (field == ACTION_FIELD_AFFECT) {
        return CheckAffectField(keymap->ctx, action->type, array_ndx, value,
                                &act->flags);
    }

    return ReportIllegal(keymap->ctx, action->type, field);
}

static bool
HandlePrivate(struct xkb_keymap *keymap, union xkb_action *action,
              enum action_field field, const ExprDef *array_ndx,
              const ExprDef *value)
{
    struct xkb_private_action *act = &action->priv;

    if (field == ACTION_FIELD_TYPE) {
        int type;

        if (array_ndx)
            return ReportActionNotArray(keymap->ctx, action->type, field);

        if (!ExprResolveInteger(keymap->ctx, value, &type))
            return ReportMismatch(keymap->ctx, ACTION_TYPE_PRIVATE, field, "integer");

        if (type < 0 || type > 255) {
            log_err(keymap->ctx,
                    "Private action type must be in the range 0..255; "
                    "Illegal type %d ignored\n", type);
            return false;
        }

        /*
         * It's possible for someone to write something like this:
         *      actions = [ Private(type=3,data[0]=1,data[1]=3,data[2]=3) ]
         * where the type refers to some existing action type, e.g. LockMods.
         * This assumes that this action's struct is laid out in memory
         * exactly as described in the XKB specification and libraries.
         * We, however, have changed these structs in various ways, so this
         * assumption is no longer true. Since this is a lousy "feature", we
         * make actions like these no-ops for now.
         */
        if (type < ACTION_TYPE_PRIVATE) {
            log_info(keymap->ctx,
                     "Private actions of type %s are not supported; Ignored\n",
                     ActionTypeText(type));
            act->type = ACTION_TYPE_NONE;
        }
        else {
            act->type = (enum xkb_action_type) type;
        }

        return true;
    }
    else if (field == ACTION_FIELD_DATA) {
        if (array_ndx == NULL) {
            xkb_atom_t val;
            const char *str;
            size_t len;

            if (!ExprResolveString(keymap->ctx, value, &val))
                return ReportMismatch(keymap->ctx, action->type, field, "string");

            str = xkb_atom_text(keymap->ctx, val);
            len = strlen(str);
            if (len < 1 || len > 7) {
                log_warn(keymap->ctx,
                         "A private action has 7 data bytes; "
                         "Illegal data ignored\n");
                return false;
            }

            strncpy((char *) act->data, str, sizeof(act->data));
            return true;
        }
        else {
            int ndx, datum;

            if (!ExprResolveInteger(keymap->ctx, array_ndx, &ndx)) {
                log_err(keymap->ctx,
                        "Array subscript must be integer; "
                        "Illegal subscript ignored\n");
                return false;
            }

            if (ndx < 0 || (size_t) ndx >= sizeof(act->data)) {
                log_err(keymap->ctx,
                        "The data for a private action is %lu bytes long; "
                        "Attempt to use data[%d] ignored\n",
                        (unsigned long) sizeof(act->data), ndx);
                return false;
            }

            if (!ExprResolveInteger(keymap->ctx, value, &datum))
                return ReportMismatch(keymap->ctx, act->type, field, "integer");

            if (datum < 0 || datum > 255) {
                log_err(keymap->ctx,
                        "All data for a private action must be 0..255; "
                        "Illegal datum %d ignored\n", datum);
                return false;
            }

            act->data[ndx] = (uint8_t) datum;
            return true;
        }
    }

    return ReportIllegal(keymap->ctx, ACTION_TYPE_NONE, field);
}

typedef bool (*actionHandler)(struct xkb_keymap *keymap,
                              union xkb_action *action,
                              enum action_field field,
                              const ExprDef *array_ndx,
                              const ExprDef *value);

static const actionHandler handleAction[_ACTION_TYPE_NUM_ENTRIES] = {
    [ACTION_TYPE_NONE] = HandleNoAction,
    [ACTION_TYPE_MOD_SET] = HandleSetLatchLockMods,
    [ACTION_TYPE_MOD_LATCH] = HandleSetLatchLockMods,
    [ACTION_TYPE_MOD_LOCK] = HandleSetLatchLockMods,
    [ACTION_TYPE_GROUP_SET] = HandleSetLatchLockGroup,
    [ACTION_TYPE_GROUP_LATCH] = HandleSetLatchLockGroup,
    [ACTION_TYPE_GROUP_LOCK] = HandleSetLatchLockGroup,
    [ACTION_TYPE_PTR_MOVE] = HandleMovePtr,
    [ACTION_TYPE_PTR_BUTTON] = HandlePtrBtn,
    [ACTION_TYPE_PTR_LOCK] = HandlePtrBtn,
    [ACTION_TYPE_PTR_DEFAULT] = HandleSetPtrDflt,
    [ACTION_TYPE_TERMINATE] = HandleNoAction,
    [ACTION_TYPE_SWITCH_VT] = HandleSwitchScreen,
    [ACTION_TYPE_CTRL_SET] = HandleSetLockControls,
    [ACTION_TYPE_CTRL_LOCK] = HandleSetLockControls,
    [ACTION_TYPE_PRIVATE] = HandlePrivate,
};

/***====================================================================***/

bool
HandleActionDef(ExprDef *def, struct xkb_keymap *keymap,
                union xkb_action *action, ActionsInfo *info)
{
    ExprDef *arg;
    const char *str;
    unsigned handler_type;

    if (def->expr.op != EXPR_ACTION_DECL) {
        log_err(keymap->ctx, "Expected an action definition, found %s\n",
                expr_op_type_to_string(def->expr.op));
        return false;
    }

    str = xkb_atom_text(keymap->ctx, def->action.name);
    if (!stringToAction(str, &handler_type)) {
        log_err(keymap->ctx, "Unknown action %s\n", str);
        return false;
    }

    /*
     * Get the default values for this action type, as modified by
     * statements such as:
     *     latchMods.clearLocks = True;
     */
    *action = info->actions[handler_type];

    /*
     * Now change the action properties as specified for this
     * particular instance, e.g. "modifiers" and "clearLocks" in:
     *     SetMods(modifiers=Alt,clearLocks);
     */
    for (arg = def->action.args; arg != NULL;
         arg = (ExprDef *) arg->common.next) {
        const ExprDef *value;
        ExprDef *field, *arrayRtrn;
        const char *elemRtrn, *fieldRtrn;
        enum action_field fieldNdx;

        if (arg->expr.op == EXPR_ASSIGN) {
            field = arg->binary.left;
            value = arg->binary.right;
        }
        else if (arg->expr.op == EXPR_NOT || arg->expr.op == EXPR_INVERT) {
            field = arg->unary.child;
            value = (const ExprDef *) &constFalse;
        }
        else {
            field = arg;
            value = (const ExprDef *) &constTrue;
        }

        if (!ExprResolveLhs(keymap->ctx, field, &elemRtrn, &fieldRtrn,
                            &arrayRtrn))
            return false;

        if (elemRtrn) {
            log_err(keymap->ctx,
                    "Cannot change defaults in an action definition; "
                    "Ignoring attempt to change %s.%s\n",
                    elemRtrn, fieldRtrn);
            return false;
        }

        if (!stringToField(fieldRtrn, &fieldNdx)) {
            log_err(keymap->ctx, "Unknown field name %s\n", fieldRtrn);
            return false;
        }

        if (!handleAction[handler_type](keymap, action, fieldNdx, arrayRtrn,
                                        value))
            return false;
    }

    return true;
}

bool
SetActionField(struct xkb_keymap *keymap, const char *elem, const char *field,
               ExprDef *array_ndx, ExprDef *value, ActionsInfo *info)
{
    unsigned action;
    enum action_field action_field;

    if (!stringToAction(elem, &action))
        return false;

    if (!stringToField(field, &action_field)) {
        log_err(keymap->ctx, "\"%s\" is not a legal field name\n", field);
        return false;
    }

    return handleAction[action](keymap, &info->actions[action],
                                action_field, array_ndx, value);
}
