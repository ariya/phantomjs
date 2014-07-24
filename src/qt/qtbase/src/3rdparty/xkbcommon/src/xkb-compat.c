/*
 * Copyright © 2012 Daniel Stone
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
 */

#define _XKBCOMMON_COMPAT_H /* don't mangle our legacy names! */

#include "xkbcommon/xkbcommon.h"
#include "utils.h"

/* We don't carry any prototypes for these functions, as we #define them
 * to their newer versions so people link against those. */
#pragma GCC diagnostic ignored "-Wmissing-prototypes"

XKB_EXPORT struct xkb_keymap *
xkb_map_new_from_names(struct xkb_context *context,
                       const struct xkb_rule_names *names,
                       enum xkb_keymap_compile_flags flags)
{
    return xkb_keymap_new_from_names(context, names, flags);
}

XKB_EXPORT struct xkb_keymap *
xkb_map_new_from_file(struct xkb_context *context, FILE *file,
                      enum xkb_keymap_format format,
                      enum xkb_keymap_compile_flags flags)
{
    return xkb_keymap_new_from_file(context, file, format, flags);
}

XKB_EXPORT struct xkb_keymap *
xkb_map_new_from_string(struct xkb_context *context, const char *string,
                        enum xkb_keymap_format format,
                        enum xkb_keymap_compile_flags flags)
{
    return xkb_keymap_new_from_string(context, string, format, flags);
}

XKB_EXPORT char *
xkb_map_get_as_string(struct xkb_keymap *keymap)
{
    return xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
}

XKB_EXPORT struct xkb_keymap *
xkb_map_ref(struct xkb_keymap *keymap)
{
    return xkb_keymap_ref(keymap);
}

XKB_EXPORT void
xkb_map_unref(struct xkb_keymap *keymap)
{
    xkb_keymap_unref(keymap);
}

XKB_EXPORT xkb_mod_index_t
xkb_map_num_mods(struct xkb_keymap *keymap)
{
    return xkb_keymap_num_mods(keymap);
}

XKB_EXPORT const char *
xkb_map_mod_get_name(struct xkb_keymap *keymap, xkb_mod_index_t idx)
{
    return xkb_keymap_mod_get_name(keymap, idx);
}

XKB_EXPORT xkb_mod_index_t
xkb_map_mod_get_index(struct xkb_keymap *keymap, const char *name)
{
    return xkb_keymap_mod_get_index(keymap, name);
}

XKB_EXPORT bool
xkb_key_mod_index_is_consumed(struct xkb_state *state, xkb_keycode_t kc,
                              xkb_mod_index_t idx)
{
    return xkb_state_mod_index_is_consumed(state, kc, idx);
}

XKB_EXPORT xkb_mod_mask_t
xkb_key_mod_mask_remove_consumed(struct xkb_state *state, xkb_keycode_t kc,
                                 xkb_mod_mask_t mask)
{
    return xkb_state_mod_mask_remove_consumed(state, kc, mask);
}

XKB_EXPORT xkb_layout_index_t
xkb_map_num_groups(struct xkb_keymap *keymap)
{
    return xkb_keymap_num_layouts(keymap);
}

XKB_EXPORT xkb_layout_index_t
xkb_key_num_groups(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    return xkb_keymap_num_layouts_for_key(keymap, kc);
}

XKB_EXPORT const char *
xkb_map_group_get_name(struct xkb_keymap *keymap, xkb_layout_index_t idx)
{
    return xkb_keymap_layout_get_name(keymap, idx);
}

XKB_EXPORT xkb_layout_index_t
xkb_map_group_get_index(struct xkb_keymap *keymap, const char *name)
{
    return xkb_keymap_layout_get_index(keymap, name);
}

XKB_EXPORT xkb_led_index_t
xkb_map_num_leds(struct xkb_keymap *keymap)
{
    return xkb_keymap_num_leds(keymap);
}

XKB_EXPORT const char *
xkb_map_led_get_name(struct xkb_keymap *keymap, xkb_led_index_t idx)
{
    return xkb_keymap_led_get_name(keymap, idx);
}

XKB_EXPORT xkb_led_index_t
xkb_map_led_get_index(struct xkb_keymap *keymap, const char *name)
{
    return xkb_keymap_led_get_index(keymap, name);
}

XKB_EXPORT bool
xkb_key_repeats(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    return xkb_keymap_key_repeats(keymap, kc);
}

XKB_EXPORT int
xkb_key_get_syms(struct xkb_state *state, xkb_keycode_t kc,
                 const xkb_keysym_t **syms_out)
{
    return xkb_state_key_get_syms(state, kc, syms_out);
}

XKB_EXPORT bool
xkb_state_group_name_is_active(struct xkb_state *state, const char *name,
                               enum xkb_state_component type)
{
    return xkb_state_layout_name_is_active(state, name, type);
}

XKB_EXPORT bool
xkb_state_group_index_is_active(struct xkb_state *state, xkb_layout_index_t idx,
                                enum xkb_state_component type)
{
    return xkb_state_layout_index_is_active(state, idx, type);
}

XKB_EXPORT xkb_layout_index_t
xkb_state_serialize_group(struct xkb_state *state,
                          enum xkb_state_component type)
{
    return xkb_state_serialize_layout(state, type);
}

XKB_EXPORT struct xkb_keymap *
xkb_state_get_map(struct xkb_state *state)
{
    return xkb_state_get_keymap(state);
}
