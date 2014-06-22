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

#include "xkbcomp-priv.h"
#include "text.h"

#define BUF_CHUNK_SIZE 4096

struct buf {
    char *buf;
    size_t size;
    size_t alloc;
};

static bool
do_realloc(struct buf *buf, size_t at_least)
{
    char *new;

    buf->alloc += BUF_CHUNK_SIZE;
    if (at_least >= BUF_CHUNK_SIZE)
        buf->alloc += at_least;

    new = realloc(buf->buf, buf->alloc);
    if (!new)
        return false;

    buf->buf = new;
    return true;
}

ATTR_PRINTF(2, 3) static bool
check_write_buf(struct buf *buf, const char *fmt, ...)
{
    va_list args;
    int printed;
    size_t available;

    available = buf->alloc - buf->size;
    va_start(args, fmt);
    printed = vsnprintf(buf->buf + buf->size, available, fmt, args);
    va_end(args);

    if (printed < 0)
        goto err;

    if ((size_t) printed >= available)
        if (!do_realloc(buf, printed))
            goto err;

    /* The buffer has enough space now. */

    available = buf->alloc - buf->size;
    va_start(args, fmt);
    printed = vsnprintf(buf->buf + buf->size, available, fmt, args);
    va_end(args);

    if (printed < 0 || (size_t) printed >= available)
        goto err;

    buf->size += printed;
    return true;

err:
    free(buf->buf);
    buf->buf = NULL;
    return false;
}

#define write_buf(buf, ...) do { \
    if (!check_write_buf(buf, __VA_ARGS__)) \
        return false; \
} while (0)

static bool
write_vmods(struct xkb_keymap *keymap, struct buf *buf)
{
    const struct xkb_mod *mod;
    xkb_mod_index_t num_vmods = 0;

    darray_foreach(mod, keymap->mods) {
        if (mod->type != MOD_VIRT)
            continue;

        if (num_vmods == 0)
            write_buf(buf, "\tvirtual_modifiers ");
        else
            write_buf(buf, ",");
        write_buf(buf, "%s", xkb_atom_text(keymap->ctx, mod->name));
        num_vmods++;
    }

    if (num_vmods > 0)
        write_buf(buf, ";\n\n");

    return true;
}

static bool
write_keycodes(struct xkb_keymap *keymap, struct buf *buf)
{
    const struct xkb_key *key;
    xkb_led_index_t idx;
    const struct xkb_led *led;

    if (keymap->keycodes_section_name)
        write_buf(buf, "xkb_keycodes \"%s\" {\n",
                  keymap->keycodes_section_name);
    else
        write_buf(buf, "xkb_keycodes {\n");

    /* xkbcomp and X11 really want to see keymaps with a minimum of 8, and
     * a maximum of at least 255, else XWayland really starts hating life.
     * If this is a problem and people really need strictly bounded keymaps,
     * we should probably control this with a flag. */
    write_buf(buf, "\tminimum = %u;\n", min(keymap->min_key_code, 8));
    write_buf(buf, "\tmaximum = %u;\n", max(keymap->max_key_code, 255));

    xkb_foreach_key(key, keymap) {
        if (key->name == XKB_ATOM_NONE)
            continue;

        write_buf(buf, "\t%-20s = %u;\n",
                  KeyNameText(keymap->ctx, key->name), key->keycode);
    }

    darray_enumerate(idx, led, keymap->leds)
        if (led->name != XKB_ATOM_NONE)
            write_buf(buf, "\tindicator %u = \"%s\";\n",
                      idx + 1, xkb_atom_text(keymap->ctx, led->name));


    for (unsigned i = 0; i < keymap->num_key_aliases; i++)
        write_buf(buf, "\talias %-14s = %s;\n",
                  KeyNameText(keymap->ctx, keymap->key_aliases[i].alias),
                  KeyNameText(keymap->ctx, keymap->key_aliases[i].real));

    write_buf(buf, "};\n\n");
    return true;
}

static bool
write_types(struct xkb_keymap *keymap, struct buf *buf)
{
    if (keymap->types_section_name)
        write_buf(buf, "xkb_types \"%s\" {\n",
                  keymap->types_section_name);
    else
        write_buf(buf, "xkb_types {\n");

    write_vmods(keymap, buf);

    for (unsigned i = 0; i < keymap->num_types; i++) {
        const struct xkb_key_type *type = &keymap->types[i];

        write_buf(buf, "\ttype \"%s\" {\n",
                  xkb_atom_text(keymap->ctx, type->name));

        write_buf(buf, "\t\tmodifiers= %s;\n",
                  ModMaskText(keymap, type->mods.mods));

        for (unsigned j = 0; j < type->num_entries; j++) {
            const char *str;
            const struct xkb_key_type_entry *entry = &type->entries[j];

            /*
             * Printing level 1 entries is redundant, it's the default,
             * unless there's preserve info.
             */
            if (entry->level == 0 && entry->preserve.mods == 0)
                continue;

            str = ModMaskText(keymap, entry->mods.mods);
            write_buf(buf, "\t\tmap[%s]= Level%u;\n",
                      str, entry->level + 1);

            if (entry->preserve.mods)
                write_buf(buf, "\t\tpreserve[%s]= %s;\n",
                          str, ModMaskText(keymap, entry->preserve.mods));
        }

        for (xkb_level_index_t n = 0; n < type->num_levels; n++)
            if (type->level_names[n])
                write_buf(buf, "\t\tlevel_name[Level%u]= \"%s\";\n", n + 1,
                          xkb_atom_text(keymap->ctx, type->level_names[n]));

        write_buf(buf, "\t};\n");
    }

    write_buf(buf, "};\n\n");
    return true;
}

static bool
write_led_map(struct xkb_keymap *keymap, struct buf *buf,
              const struct xkb_led *led)
{
    write_buf(buf, "\tindicator \"%s\" {\n",
              xkb_atom_text(keymap->ctx, led->name));

    if (led->which_groups) {
        if (led->which_groups != XKB_STATE_LAYOUT_EFFECTIVE) {
            write_buf(buf, "\t\twhichGroupState= %s;\n",
                      LedStateMaskText(keymap->ctx, led->which_groups));
        }
        write_buf(buf, "\t\tgroups= 0x%02x;\n",
                  led->groups);
    }

    if (led->which_mods) {
        if (led->which_mods != XKB_STATE_MODS_EFFECTIVE) {
            write_buf(buf, "\t\twhichModState= %s;\n",
                      LedStateMaskText(keymap->ctx, led->which_mods));
        }
        write_buf(buf, "\t\tmodifiers= %s;\n",
                  ModMaskText(keymap, led->mods.mods));
    }

    if (led->ctrls) {
        write_buf(buf, "\t\tcontrols= %s;\n",
                  ControlMaskText(keymap->ctx, led->ctrls));
    }

    write_buf(buf, "\t};\n");
    return true;
}

static const char *
affect_lock_text(enum xkb_action_flags flags)
{
    switch (flags & (ACTION_LOCK_NO_LOCK | ACTION_LOCK_NO_UNLOCK)) {
    case ACTION_LOCK_NO_UNLOCK:
        return ",affect=lock";
    case ACTION_LOCK_NO_LOCK:
        return ",affect=unlock";
    case ACTION_LOCK_NO_LOCK | ACTION_LOCK_NO_UNLOCK:
        return ",affect=neither";
    }
    return "";
}

static bool
write_action(struct xkb_keymap *keymap, struct buf *buf,
             const union xkb_action *action,
             const char *prefix, const char *suffix)
{
    const char *type;
    const char *args = NULL;

    if (!prefix)
        prefix = "";
    if (!suffix)
        suffix = "";

    type = ActionTypeText(action->type);

    switch (action->type) {
    case ACTION_TYPE_MOD_LOCK:
    case ACTION_TYPE_MOD_SET:
    case ACTION_TYPE_MOD_LATCH:
        if (action->mods.flags & ACTION_MODS_LOOKUP_MODMAP)
            args = "modMapMods";
        else
            args = ModMaskText(keymap, action->mods.mods.mods);
        write_buf(buf, "%s%s(modifiers=%s%s%s%s)%s", prefix, type, args,
                  (action->type != ACTION_TYPE_MOD_LOCK && (action->mods.flags & ACTION_LOCK_CLEAR)) ? ",clearLocks" : "",
                  (action->type != ACTION_TYPE_MOD_LOCK && (action->mods.flags & ACTION_LATCH_TO_LOCK)) ? ",latchToLock" : "",
                  (action->type == ACTION_TYPE_MOD_LOCK) ? affect_lock_text(action->mods.flags) : "",
                  suffix);
        break;

    case ACTION_TYPE_GROUP_SET:
    case ACTION_TYPE_GROUP_LATCH:
    case ACTION_TYPE_GROUP_LOCK:
        write_buf(buf, "%s%s(group=%s%d%s%s)%s", prefix, type,
                  (!(action->group.flags & ACTION_ABSOLUTE_SWITCH) && action->group.group > 0) ? "+" : "",
                  (action->group.flags & ACTION_ABSOLUTE_SWITCH) ? action->group.group + 1 : action->group.group,
                  (action->type != ACTION_TYPE_GROUP_LOCK && (action->group.flags & ACTION_LOCK_CLEAR)) ? ",clearLocks" : "",
                  (action->type != ACTION_TYPE_GROUP_LOCK && (action->group.flags & ACTION_LATCH_TO_LOCK)) ? ",latchToLock" : "",
                  suffix);
        break;

    case ACTION_TYPE_TERMINATE:
        write_buf(buf, "%s%s()%s", prefix, type, suffix);
        break;

    case ACTION_TYPE_PTR_MOVE:
        write_buf(buf, "%s%s(x=%s%d,y=%s%d%s)%s", prefix, type,
                  (!(action->ptr.flags & ACTION_ABSOLUTE_X) && action->ptr.x >= 0) ? "+" : "",
                  action->ptr.x,
                  (!(action->ptr.flags & ACTION_ABSOLUTE_Y) && action->ptr.y >= 0) ? "+" : "",
                  action->ptr.y,
                  (action->ptr.flags & ACTION_ACCEL) ? "" : ",!accel",
                  suffix);
        break;

    case ACTION_TYPE_PTR_LOCK:
        args = affect_lock_text(action->btn.flags);
        /* fallthrough */
    case ACTION_TYPE_PTR_BUTTON:
        write_buf(buf, "%s%s(button=", prefix, type);
        if (action->btn.button > 0 && action->btn.button <= 5)
            write_buf(buf, "%d", action->btn.button);
        else
            write_buf(buf, "default");
        if (action->btn.count)
            write_buf(buf, ",count=%d", action->btn.count);
        if (args)
            write_buf(buf, "%s", args);
        write_buf(buf, ")%s", suffix);
        break;

    case ACTION_TYPE_PTR_DEFAULT:
        write_buf(buf, "%s%s(", prefix, type);
        write_buf(buf, "affect=button,button=%s%d",
                  (!(action->dflt.flags & ACTION_ABSOLUTE_SWITCH) && action->dflt.value >= 0) ? "+" : "",
                  action->dflt.value);
        write_buf(buf, ")%s", suffix);
        break;

    case ACTION_TYPE_SWITCH_VT:
        write_buf(buf, "%s%s(screen=%s%d,%ssame)%s", prefix, type,
                  (!(action->screen.flags & ACTION_ABSOLUTE_SWITCH) && action->screen.screen >= 0) ? "+" : "",
                  action->screen.screen,
                  (action->screen.flags & ACTION_SAME_SCREEN) ? "" : "!",
                  suffix);
        break;

    case ACTION_TYPE_CTRL_SET:
    case ACTION_TYPE_CTRL_LOCK:
        write_buf(buf, "%s%s(controls=%s%s)%s", prefix, type,
                  ControlMaskText(keymap->ctx, action->ctrls.ctrls),
                  (action->type == ACTION_TYPE_CTRL_LOCK) ? affect_lock_text(action->ctrls.flags) : "",
                  suffix);
        break;

    case ACTION_TYPE_NONE:
        write_buf(buf, "%sNoAction()%s", prefix, suffix);
        break;

    default:
        write_buf(buf,
                  "%s%s(type=0x%02x,data[0]=0x%02x,data[1]=0x%02x,data[2]=0x%02x,data[3]=0x%02x,data[4]=0x%02x,data[5]=0x%02x,data[6]=0x%02x)%s",
                  prefix, type, action->type, action->priv.data[0],
                  action->priv.data[1], action->priv.data[2],
                  action->priv.data[3], action->priv.data[4],
                  action->priv.data[5], action->priv.data[6],
                  suffix);
        break;
    }

    return true;
}

static bool
write_compat(struct xkb_keymap *keymap, struct buf *buf)
{
    const struct xkb_led *led;

    if (keymap->compat_section_name)
        write_buf(buf, "xkb_compatibility \"%s\" {\n",
                  keymap->compat_section_name);
    else
        write_buf(buf, "xkb_compatibility {\n");

    write_vmods(keymap, buf);

    write_buf(buf, "\tinterpret.useModMapMods= AnyLevel;\n");
    write_buf(buf, "\tinterpret.repeat= False;\n");

    for (unsigned i = 0; i < keymap->num_sym_interprets; i++) {
        const struct xkb_sym_interpret *si = &keymap->sym_interprets[i];

        write_buf(buf, "\tinterpret %s+%s(%s) {\n",
                  si->sym ? KeysymText(keymap->ctx, si->sym) : "Any",
                  SIMatchText(si->match),
                  ModMaskText(keymap, si->mods));

        if (si->virtual_mod != XKB_MOD_INVALID)
            write_buf(buf, "\t\tvirtualModifier= %s;\n",
                      ModIndexText(keymap, si->virtual_mod));

        if (si->level_one_only)
            write_buf(buf, "\t\tuseModMapMods=level1;\n");

        if (si->repeat)
            write_buf(buf, "\t\trepeat= True;\n");

        write_action(keymap, buf, &si->action, "\t\taction= ", ";\n");
        write_buf(buf, "\t};\n");
    }

    darray_foreach(led, keymap->leds)
        if (led->which_groups || led->groups || led->which_mods ||
            led->mods.mods || led->ctrls)
            write_led_map(keymap, buf, led);

    write_buf(buf, "};\n\n");

    return true;
}

static bool
write_keysyms(struct xkb_keymap *keymap, struct buf *buf,
              const struct xkb_key *key, xkb_layout_index_t group)
{
    for (xkb_level_index_t level = 0; level < XkbKeyGroupWidth(key, group);
         level++) {
        const xkb_keysym_t *syms;
        int num_syms;

        if (level != 0)
            write_buf(buf, ", ");

        num_syms = xkb_keymap_key_get_syms_by_level(keymap, key->keycode,
                                                    group, level, &syms);
        if (num_syms == 0) {
            write_buf(buf, "%15s", "NoSymbol");
        }
        else if (num_syms == 1) {
            write_buf(buf, "%15s", KeysymText(keymap->ctx, syms[0]));
        }
        else {
            write_buf(buf, "{ ");
            for (int s = 0; s < num_syms; s++) {
                if (s != 0)
                    write_buf(buf, ", ");
                write_buf(buf, "%s", KeysymText(keymap->ctx, syms[s]));
            }
            write_buf(buf, " }");
        }
    }

    return true;
}

static bool
write_key(struct xkb_keymap *keymap, struct buf *buf,
          const struct xkb_key *key)
{
    xkb_layout_index_t group;
    bool simple = true;
    bool explicit_types = false;
    bool multi_type = false;
    bool show_actions;

    write_buf(buf, "\tkey %-20s {", KeyNameText(keymap->ctx, key->name));

    for (group = 0; group < key->num_groups; group++) {
        if (key->groups[group].explicit_type)
            explicit_types = true;

        if (group != 0 && key->groups[group].type != key->groups[0].type)
            multi_type = true;
    }

    if (explicit_types) {
        const struct xkb_key_type *type;
        simple = false;

        if (multi_type) {
            for (group = 0; group < key->num_groups; group++) {
                if (!key->groups[group].explicit_type)
                    continue;

                type = key->groups[group].type;
                write_buf(buf, "\n\t\ttype[group%u]= \"%s\",",
                            group + 1,
                            xkb_atom_text(keymap->ctx, type->name));
            }
        }
        else {
            type = key->groups[0].type;
            write_buf(buf, "\n\t\ttype= \"%s\",",
                        xkb_atom_text(keymap->ctx, type->name));
        }
    }

    if (key->explicit & EXPLICIT_REPEAT) {
        if (key->repeats)
            write_buf(buf, "\n\t\trepeat= Yes,");
        else
            write_buf(buf, "\n\t\trepeat= No,");
        simple = false;
    }

    if (key->vmodmap && (key->explicit & EXPLICIT_VMODMAP))
        write_buf(buf, "\n\t\tvirtualMods= %s,",
                    ModMaskText(keymap, key->vmodmap));

    switch (key->out_of_range_group_action) {
    case RANGE_SATURATE:
        write_buf(buf, "\n\t\tgroupsClamp,");
        break;

    case RANGE_REDIRECT:
        write_buf(buf, "\n\t\tgroupsRedirect= Group%u,",
                    key->out_of_range_group_number + 1);
        break;

    default:
        break;
    }

    show_actions = !!(key->explicit & EXPLICIT_INTERP);

    if (key->num_groups > 1 || show_actions)
        simple = false;

    if (simple) {
        write_buf(buf, "\t[ ");
        if (!write_keysyms(keymap, buf, key, 0))
            return false;
        write_buf(buf, " ] };\n");
    }
    else {
        xkb_level_index_t level;

        for (group = 0; group < key->num_groups; group++) {
            if (group != 0)
                write_buf(buf, ",");
            write_buf(buf, "\n\t\tsymbols[Group%u]= [ ", group + 1);
            if (!write_keysyms(keymap, buf, key, group))
                return false;
            write_buf(buf, " ]");
            if (show_actions) {
                write_buf(buf, ",\n\t\tactions[Group%u]= [ ", group + 1);
                for (level = 0;
                        level < XkbKeyGroupWidth(key, group); level++) {
                    if (level != 0)
                        write_buf(buf, ", ");
                    write_action(keymap, buf,
                                    &key->groups[group].levels[level].action,
                                    NULL, NULL);
                }
                write_buf(buf, " ]");
            }
        }
        write_buf(buf, "\n\t};\n");
    }

    return true;
}

static bool
write_symbols(struct xkb_keymap *keymap, struct buf *buf)
{
    const struct xkb_key *key;
    xkb_layout_index_t group;

    if (keymap->symbols_section_name)
        write_buf(buf, "xkb_symbols \"%s\" {\n",
                  keymap->symbols_section_name);
    else
        write_buf(buf, "xkb_symbols {\n");

    for (group = 0; group < keymap->num_group_names; group++)
        if (keymap->group_names[group])
            write_buf(buf,
                      "\tname[group%u]=\"%s\";\n", group + 1,
                      xkb_atom_text(keymap->ctx, keymap->group_names[group]));
    if (group > 0)
        write_buf(buf, "\n");

    xkb_foreach_key(key, keymap)
        if (key->num_groups > 0)
            write_key(keymap, buf, key);

    xkb_foreach_key(key, keymap) {
        xkb_mod_index_t i;
        const struct xkb_mod *mod;

        if (key->modmap == 0)
            continue;

        darray_enumerate(i, mod, keymap->mods)
            if (key->modmap & (1u << i))
                write_buf(buf, "\tmodifier_map %s { %s };\n",
                          xkb_atom_text(keymap->ctx, mod->name),
                          KeyNameText(keymap->ctx, key->name));
    }

    write_buf(buf, "};\n\n");
    return true;
}

static bool
write_keymap(struct xkb_keymap *keymap, struct buf *buf)
{
    return (check_write_buf(buf, "xkb_keymap {\n") &&
            write_keycodes(keymap, buf) &&
            write_types(keymap, buf) &&
            write_compat(keymap, buf) &&
            write_symbols(keymap, buf) &&
            check_write_buf(buf, "};\n"));
}

char *
text_v1_keymap_get_as_string(struct xkb_keymap *keymap)
{
    struct buf buf = { NULL, 0, 0 };

    if (!write_keymap(keymap, &buf)) {
        free(buf.buf);
        return NULL;
    }

    return buf.buf;
}
