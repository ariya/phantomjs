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

#include "keymap.h"
#include "text.h"

bool
LookupString(const LookupEntry tab[], const char *string,
              unsigned int *value_rtrn)
{
    if (!string)
        return false;

    for (const LookupEntry *entry = tab; entry->name; entry++) {
        if (istreq(entry->name, string)) {
            *value_rtrn = entry->value;
            return true;
        }
    }

    return false;
}

const char *
LookupValue(const LookupEntry tab[], unsigned int value)
{
    for (const LookupEntry *entry = tab; entry->name; entry++)
        if (entry->value == value)
            return entry->name;

    return NULL;
}

const LookupEntry ctrlMaskNames[] = {
    { "RepeatKeys", CONTROL_REPEAT },
    { "Repeat", CONTROL_REPEAT },
    { "AutoRepeat", CONTROL_REPEAT },
    { "SlowKeys", CONTROL_SLOW },
    { "BounceKeys", CONTROL_DEBOUNCE },
    { "StickyKeys", CONTROL_STICKY },
    { "MouseKeys", CONTROL_MOUSEKEYS },
    { "MouseKeysAccel", CONTROL_MOUSEKEYS_ACCEL },
    { "AccessXKeys", CONTROL_AX },
    { "AccessXTimeout", CONTROL_AX_TIMEOUT },
    { "AccessXFeedback", CONTROL_AX_FEEDBACK },
    { "AudibleBell", CONTROL_BELL },
    { "IgnoreGroupLock", CONTROL_IGNORE_GROUP_LOCK },
    { "all", CONTROL_ALL },
    { "none", 0 },
    { "Overlay1", 0 },
    { "Overlay2", 0 },
    { NULL, 0 }
};

const LookupEntry modComponentMaskNames[] = {
    { "base", XKB_STATE_MODS_DEPRESSED },
    { "latched", XKB_STATE_MODS_LATCHED },
    { "locked", XKB_STATE_MODS_LOCKED },
    { "effective", XKB_STATE_MODS_EFFECTIVE },
    { "compat", XKB_STATE_MODS_EFFECTIVE },
    { "any", XKB_STATE_MODS_EFFECTIVE },
    { "none", 0 },
    { NULL, 0 }
};

const LookupEntry groupComponentMaskNames[] = {
    { "base", XKB_STATE_LAYOUT_DEPRESSED },
    { "latched", XKB_STATE_LAYOUT_LATCHED },
    { "locked", XKB_STATE_LAYOUT_LOCKED },
    { "effective", XKB_STATE_LAYOUT_EFFECTIVE },
    { "any", XKB_STATE_LAYOUT_EFFECTIVE },
    { "none", 0 },
    { NULL, 0 }
};

const LookupEntry groupMaskNames[] = {
    { "Group1", 0x01 },
    { "Group2", 0x02 },
    { "Group3", 0x04 },
    { "Group4", 0x08 },
    { "Group5", 0x10 },
    { "Group6", 0x20 },
    { "Group7", 0x40 },
    { "Group8", 0x80 },
    { "none", 0x00 },
    { "all", 0xff },
    { NULL, 0 }
};

const LookupEntry groupNames[] = {
    { "Group1", 1 },
    { "Group2", 2 },
    { "Group3", 3 },
    { "Group4", 4 },
    { "Group5", 5 },
    { "Group6", 6 },
    { "Group7", 7 },
    { "Group8", 8 },
    { NULL, 0 }
};

const LookupEntry levelNames[] = {
    { "Level1", 1 },
    { "Level2", 2 },
    { "Level3", 3 },
    { "Level4", 4 },
    { "Level5", 5 },
    { "Level6", 6 },
    { "Level7", 7 },
    { "Level8", 8 },
    { NULL, 0 }
};

const LookupEntry buttonNames[] = {
    { "Button1", 1 },
    { "Button2", 2 },
    { "Button3", 3 },
    { "Button4", 4 },
    { "Button5", 5 },
    { "default", 0 },
    { NULL, 0 }
};

const LookupEntry useModMapValueNames[] = {
    { "LevelOne", 1 },
    { "Level1", 1 },
    { "AnyLevel", 0 },
    { "any", 0 },
    { NULL, 0 }
};

const LookupEntry actionTypeNames[] = {
    { "NoAction", ACTION_TYPE_NONE },
    { "SetMods", ACTION_TYPE_MOD_SET },
    { "LatchMods", ACTION_TYPE_MOD_LATCH },
    { "LockMods", ACTION_TYPE_MOD_LOCK },
    { "SetGroup", ACTION_TYPE_GROUP_SET },
    { "LatchGroup", ACTION_TYPE_GROUP_LATCH },
    { "LockGroup", ACTION_TYPE_GROUP_LOCK },
    { "MovePtr", ACTION_TYPE_PTR_MOVE },
    { "MovePointer", ACTION_TYPE_PTR_MOVE },
    { "PtrBtn", ACTION_TYPE_PTR_BUTTON },
    { "PointerButton", ACTION_TYPE_PTR_BUTTON },
    { "LockPtrBtn", ACTION_TYPE_PTR_LOCK },
    { "LockPtrButton", ACTION_TYPE_PTR_LOCK },
    { "LockPointerButton", ACTION_TYPE_PTR_LOCK },
    { "LockPointerBtn", ACTION_TYPE_PTR_LOCK },
    { "SetPtrDflt", ACTION_TYPE_PTR_DEFAULT },
    { "SetPointerDefault", ACTION_TYPE_PTR_DEFAULT },
    { "Terminate", ACTION_TYPE_TERMINATE },
    { "TerminateServer", ACTION_TYPE_TERMINATE },
    { "SwitchScreen", ACTION_TYPE_SWITCH_VT },
    { "SetControls", ACTION_TYPE_CTRL_SET },
    { "LockControls", ACTION_TYPE_CTRL_LOCK },
    { "Private", ACTION_TYPE_PRIVATE },
    /* deprecated actions below here - unused */
    { "RedirectKey", ACTION_TYPE_NONE },
    { "Redirect", ACTION_TYPE_NONE },
    { "ISOLock", ACTION_TYPE_NONE },
    { "ActionMessage", ACTION_TYPE_NONE },
    { "MessageAction", ACTION_TYPE_NONE },
    { "Message", ACTION_TYPE_NONE },
    { "DeviceBtn", ACTION_TYPE_NONE },
    { "DevBtn", ACTION_TYPE_NONE },
    { "DevButton", ACTION_TYPE_NONE },
    { "DeviceButton", ACTION_TYPE_NONE },
    { "LockDeviceBtn", ACTION_TYPE_NONE },
    { "LockDevBtn", ACTION_TYPE_NONE },
    { "LockDevButton", ACTION_TYPE_NONE },
    { "LockDeviceButton", ACTION_TYPE_NONE },
    { "DeviceValuator", ACTION_TYPE_NONE },
    { "DevVal", ACTION_TYPE_NONE },
    { "DeviceVal", ACTION_TYPE_NONE },
    { "DevValuator", ACTION_TYPE_NONE },
    { NULL, 0 },
};

const LookupEntry symInterpretMatchMaskNames[] = {
    { "NoneOf", MATCH_NONE },
    { "AnyOfOrNone", MATCH_ANY_OR_NONE },
    { "AnyOf", MATCH_ANY },
    { "AllOf", MATCH_ALL },
    { "Exactly", MATCH_EXACTLY },
};

const char *
ModIndexText(const struct xkb_keymap *keymap, xkb_mod_index_t ndx)
{
    if (ndx == XKB_MOD_INVALID)
        return "none";

    if (ndx >= darray_size(keymap->mods))
        return NULL;

    return xkb_atom_text(keymap->ctx, darray_item(keymap->mods, ndx).name);
}

xkb_mod_index_t
ModNameToIndex(const struct xkb_keymap *keymap, xkb_atom_t name,
               enum mod_type type)
{
    xkb_mod_index_t i;
    const struct xkb_mod *mod;

    darray_enumerate(i, mod, keymap->mods)
        if ((mod->type & type) && name == mod->name)
            return i;

    return XKB_MOD_INVALID;
}

const char *
ActionTypeText(enum xkb_action_type type)
{
    const char *name = LookupValue(actionTypeNames, type);
    return name ? name : "Private";
}

const char *
KeysymText(struct xkb_context *ctx, xkb_keysym_t sym)
{
    char *buffer = xkb_context_get_buffer(ctx, 64);
    xkb_keysym_get_name(sym, buffer, 64);
    return buffer;
}

const char *
KeyNameText(struct xkb_context *ctx, xkb_atom_t name)
{
    const char *sname = xkb_atom_text(ctx, name);
    size_t len = strlen_safe(sname) + 3;
    char *buf = xkb_context_get_buffer(ctx, len);
    snprintf(buf, len, "<%s>", strempty(sname));
    return buf;
}

const char *
SIMatchText(enum xkb_match_operation type)
{
    return LookupValue(symInterpretMatchMaskNames, type);
}

const char *
ModMaskText(const struct xkb_keymap *keymap, xkb_mod_mask_t mask)
{
    char buf[1024];
    size_t pos = 0;
    xkb_mod_index_t i;
    const struct xkb_mod *mod;

    if (mask == 0)
        return "none";

    if (mask == MOD_REAL_MASK_ALL)
        return "all";

    darray_enumerate(i, mod, keymap->mods) {
        int ret;

        if (!(mask & (1u << i)))
            continue;

        ret = snprintf(buf + pos, sizeof(buf) - pos, "%s%s",
                       pos == 0 ? "" : "+",
                       xkb_atom_text(keymap->ctx, mod->name));
        if (ret <= 0 || pos + ret >= sizeof(buf))
            break;
        else
            pos += ret;
    }

    return strcpy(xkb_context_get_buffer(keymap->ctx, pos + 1), buf);
}

const char *
LedStateMaskText(struct xkb_context *ctx, enum xkb_state_component mask)
{
    char buf[1024];
    size_t pos = 0;

    if (mask == 0)
        return "0";

    for (unsigned i = 0; mask; i++) {
        int ret;

        if (!(mask & (1u << i)))
            continue;

        mask &= ~(1u << i);

        ret = snprintf(buf + pos, sizeof(buf) - pos, "%s%s",
                       pos == 0 ? "" : "+",
                       LookupValue(modComponentMaskNames, 1u << i));
        if (ret <= 0 || pos + ret >= sizeof(buf))
            break;
        else
            pos += ret;
    }

    return strcpy(xkb_context_get_buffer(ctx, pos + 1), buf);
}

const char *
ControlMaskText(struct xkb_context *ctx, enum xkb_action_controls mask)
{
    char buf[1024];
    size_t pos = 0;

    if (mask == 0)
        return "none";

    if (mask == CONTROL_ALL)
        return "all";

    for (unsigned i = 0; mask; i++) {
        int ret;

        if (!(mask & (1u << i)))
            continue;

        mask &= ~(1u << i);

        ret = snprintf(buf + pos, sizeof(buf) - pos, "%s%s",
                       pos == 0 ? "" : "+",
                       LookupValue(ctrlMaskNames, 1u << i));
        if (ret <= 0 || pos + ret >= sizeof(buf))
            break;
        else
            pos += ret;
    }

    return strcpy(xkb_context_get_buffer(ctx, pos + 1), buf);
}
