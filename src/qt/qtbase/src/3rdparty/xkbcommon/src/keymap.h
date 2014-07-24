/*
 * Copyright 1985, 1987, 1990, 1998  The Open Group
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors or their
 * institutions shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the authors.
 */

/************************************************************
 * Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.
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
 * Copyright © 2009 Dan Nicholson
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita
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
 *         Dan Nicholson <dbn.lists@gmail.com>
 */

#ifndef KEYMAP_H
#define KEYMAP_H

 /* Don't use compat names in internal code. */
#define _XKBCOMMON_COMPAT_H
#include "xkbcommon/xkbcommon.h"

#include "utils.h"
#include "context.h"

/* This limit is artificially enforced, we do not depend on it any where.
 * The reason it's still here is that the rules file format does not
 * support multiple groups very well, and the rules shipped with
 * xkeyboard-config (see rules/evdev) depend on this limit extensively.
 * So just lifting this limit would cause problems for people who will use
 * more than 4 layouts.
 * TODO: Fix the group index syntax in the rules format, preferably in a
 *       backwards compatible way.
 *       See e.g. https://bugs.freedesktop.org/show_bug.cgi?id=14372
 * Note: A limit on the number of groups we *do* depend on is imposed by
 * the size of the xkb_layout_mask_t type (32). This is more than enough
 * though.
 */
#define XKB_MAX_GROUPS 4

/* Don't allow more modifiers than we can hold in xkb_mod_mask_t. */
#define XKB_MAX_MODS ((xkb_mod_index_t) (sizeof(xkb_mod_mask_t) * 8))

/* Don't allow more leds than we can hold in xkb_led_mask_t. */
#define XKB_MAX_LEDS ((xkb_led_index_t) (sizeof(xkb_led_mask_t) * 8))

/* These should all go away. */
enum mod_type {
    MOD_REAL = (1 << 0),
    MOD_VIRT = (1 << 1),
    MOD_BOTH = (MOD_REAL | MOD_VIRT),
};
#define MOD_REAL_MASK_ALL ((xkb_mod_mask_t) 0x000000ff)

enum xkb_action_type {
    ACTION_TYPE_NONE = 0,
    ACTION_TYPE_MOD_SET,
    ACTION_TYPE_MOD_LATCH,
    ACTION_TYPE_MOD_LOCK,
    ACTION_TYPE_GROUP_SET,
    ACTION_TYPE_GROUP_LATCH,
    ACTION_TYPE_GROUP_LOCK,
    ACTION_TYPE_PTR_MOVE,
    ACTION_TYPE_PTR_BUTTON,
    ACTION_TYPE_PTR_LOCK,
    ACTION_TYPE_PTR_DEFAULT,
    ACTION_TYPE_TERMINATE,
    ACTION_TYPE_SWITCH_VT,
    ACTION_TYPE_CTRL_SET,
    ACTION_TYPE_CTRL_LOCK,
    ACTION_TYPE_PRIVATE,
    _ACTION_TYPE_NUM_ENTRIES
};

enum xkb_action_flags {
    ACTION_LOCK_CLEAR = (1 << 0),
    ACTION_LATCH_TO_LOCK = (1 << 1),
    ACTION_LOCK_NO_LOCK = (1 << 2),
    ACTION_LOCK_NO_UNLOCK = (1 << 3),
    ACTION_MODS_LOOKUP_MODMAP = (1 << 4),
    ACTION_ABSOLUTE_SWITCH = (1 << 5),
    ACTION_ABSOLUTE_X = (1 << 6),
    ACTION_ABSOLUTE_Y = (1 << 7),
    ACTION_ACCEL = (1 << 8),
    ACTION_SAME_SCREEN = (1 << 9),
};

enum xkb_action_controls {
    CONTROL_REPEAT = (1 << 0),
    CONTROL_SLOW = (1 << 1),
    CONTROL_DEBOUNCE = (1 << 2),
    CONTROL_STICKY = (1 << 3),
    CONTROL_MOUSEKEYS = (1 << 4),
    CONTROL_MOUSEKEYS_ACCEL = (1 << 5),
    CONTROL_AX = (1 << 6),
    CONTROL_AX_TIMEOUT = (1 << 7),
    CONTROL_AX_FEEDBACK = (1 << 8),
    CONTROL_BELL = (1 << 9),
    CONTROL_IGNORE_GROUP_LOCK = (1 << 10),
    CONTROL_ALL = \
        (CONTROL_REPEAT | CONTROL_SLOW | CONTROL_DEBOUNCE | CONTROL_STICKY | \
         CONTROL_MOUSEKEYS | CONTROL_MOUSEKEYS_ACCEL | CONTROL_AX | \
         CONTROL_AX_TIMEOUT | CONTROL_AX_FEEDBACK | CONTROL_BELL | \
         CONTROL_IGNORE_GROUP_LOCK)
};

enum xkb_match_operation {
    MATCH_NONE,
    MATCH_ANY_OR_NONE,
    MATCH_ANY,
    MATCH_ALL,
    MATCH_EXACTLY,
};

struct xkb_mods {
    xkb_mod_mask_t mods;       /* original real+virtual mods in definition */
    xkb_mod_mask_t mask;       /* computed effective mask */
};

struct xkb_mod_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    struct xkb_mods mods;
};

struct xkb_group_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    int32_t group;
};

struct xkb_controls_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    enum xkb_action_controls ctrls;
};

struct xkb_pointer_default_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    int8_t value;
};

struct xkb_switch_screen_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    int8_t screen;
};

struct xkb_pointer_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    int16_t x;
    int16_t y;
};

struct xkb_pointer_button_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    uint8_t count;
    uint8_t button;
};

struct xkb_private_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    uint8_t data[7];
};

union xkb_action {
    enum xkb_action_type type;
    struct xkb_mod_action mods;
    struct xkb_group_action group;
    struct xkb_controls_action ctrls;
    struct xkb_pointer_default_action dflt;
    struct xkb_switch_screen_action screen;
    struct xkb_pointer_action ptr;
    struct xkb_pointer_button_action btn;
    struct xkb_private_action priv;
};

struct xkb_key_type_entry {
    xkb_level_index_t level;
    struct xkb_mods mods;
    struct xkb_mods preserve;
};

struct xkb_key_type {
    xkb_atom_t name;
    struct xkb_mods mods;
    xkb_level_index_t num_levels;
    xkb_atom_t *level_names;
    unsigned int num_entries;
    struct xkb_key_type_entry *entries;
};

struct xkb_sym_interpret {
    xkb_keysym_t sym;
    enum xkb_match_operation match;
    xkb_mod_mask_t mods;
    xkb_mod_index_t virtual_mod;
    union xkb_action action;
    bool level_one_only;
    bool repeat;
};

struct xkb_led {
    xkb_atom_t name;
    enum xkb_state_component which_groups;
    xkb_layout_mask_t groups;
    enum xkb_state_component which_mods;
    struct xkb_mods mods;
    enum xkb_action_controls ctrls;
};

struct xkb_key_alias {
    xkb_atom_t real;
    xkb_atom_t alias;
};

struct xkb_controls {
    unsigned char groups_wrap;
    struct xkb_mods internal;
    struct xkb_mods ignore_lock;
    unsigned short repeat_delay;
    unsigned short repeat_interval;
    unsigned short slow_keys_delay;
    unsigned short debounce_delay;
    unsigned short ax_options;
    unsigned short ax_timeout;
    unsigned short axt_opts_mask;
    unsigned short axt_opts_values;
    unsigned int axt_ctrls_mask;
    unsigned int axt_ctrls_values;
};

/* Such an awkward name.  Oh well. */
enum xkb_range_exceed_type {
    RANGE_WRAP = 0,
    RANGE_SATURATE,
    RANGE_REDIRECT,
};

enum xkb_explicit_components {
    EXPLICIT_INTERP = (1 << 0),
    EXPLICIT_VMODMAP = (1 << 1),
    EXPLICIT_REPEAT = (1 << 2),
};

struct xkb_level {
    union xkb_action action;
    unsigned int num_syms;
    union {
        xkb_keysym_t sym;       /* num_syms == 1 */
        xkb_keysym_t *syms;     /* num_syms > 1  */
    } u;
};

struct xkb_group {
    bool explicit_type;
    /* Points to a type in keymap->types. */
    const struct xkb_key_type *type;
    /* Use XkbKeyGroupWidth for the number of levels. */
    struct xkb_level *levels;
};

struct xkb_key {
    xkb_keycode_t keycode;
    xkb_atom_t name;

    enum xkb_explicit_components explicit;

    xkb_mod_mask_t modmap;
    xkb_mod_mask_t vmodmap;

    bool repeats;

    enum xkb_range_exceed_type out_of_range_group_action;
    xkb_layout_index_t out_of_range_group_number;

    xkb_layout_index_t num_groups;
    struct xkb_group *groups;
};

struct xkb_mod {
    xkb_atom_t name;
    enum mod_type type;
    xkb_mod_mask_t mapping; /* vmod -> real mod mapping */
};

/* Common keyboard description structure */
struct xkb_keymap {
    struct xkb_context *ctx;

    int refcnt;
    enum xkb_keymap_compile_flags flags;
    enum xkb_keymap_format format;

    enum xkb_action_controls enabled_ctrls;

    xkb_keycode_t min_key_code;
    xkb_keycode_t max_key_code;
    struct xkb_key *keys;

    /* aliases in no particular order */
    unsigned int num_key_aliases;
    struct xkb_key_alias *key_aliases;

    struct xkb_key_type *types;
    unsigned int num_types;

    unsigned int num_sym_interprets;
    struct xkb_sym_interpret *sym_interprets;

    darray(struct xkb_mod) mods;

    /* Number of groups in the key with the most groups. */
    xkb_layout_index_t num_groups;
    /* Not all groups must have names. */
    xkb_layout_index_t num_group_names;
    xkb_atom_t *group_names;

    darray(struct xkb_led) leds;

    char *keycodes_section_name;
    char *symbols_section_name;
    char *types_section_name;
    char *compat_section_name;
};

#define xkb_foreach_key(iter, keymap) \
    for (iter = keymap->keys + keymap->min_key_code; \
         iter <= keymap->keys + keymap->max_key_code; \
         iter++)

static inline const struct xkb_key *
XkbKey(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    if (kc < keymap->min_key_code || kc > keymap->max_key_code)
        return NULL;
    return &keymap->keys[kc];
}

static inline xkb_level_index_t
XkbKeyGroupWidth(const struct xkb_key *key, xkb_layout_index_t layout)
{
    return key->groups[layout].type->num_levels;
}

struct xkb_keymap *
xkb_keymap_new(struct xkb_context *ctx,
               enum xkb_keymap_format format,
               enum xkb_keymap_compile_flags flags);

struct xkb_key *
XkbKeyByName(struct xkb_keymap *keymap, xkb_atom_t name, bool use_aliases);

xkb_atom_t
XkbResolveKeyAlias(struct xkb_keymap *keymap, xkb_atom_t name);

void
XkbEscapeMapName(char *name);

xkb_layout_index_t
wrap_group_into_range(int32_t group,
                      xkb_layout_index_t num_groups,
                      enum xkb_range_exceed_type out_of_range_group_action,
                      xkb_layout_index_t out_of_range_group_number);

struct xkb_keymap_format_ops {
    bool (*keymap_new_from_names)(struct xkb_keymap *keymap,
                                  const struct xkb_rule_names *names);
    bool (*keymap_new_from_string)(struct xkb_keymap *keymap,
                                   const char *string, size_t length);
    bool (*keymap_new_from_file)(struct xkb_keymap *keymap, FILE *file);
    char *(*keymap_get_as_string)(struct xkb_keymap *keymap);
};

extern const struct xkb_keymap_format_ops text_v1_keymap_format_ops;

#endif
