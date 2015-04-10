/*
 * Copyright © 2009 Dan Nicholson
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
 * Author: Dan Nicholson <dbn.lists@gmail.com>
 *         Daniel Stone <daniel@fooishbar.org>
 *         Ran Benita <ran234@gmail.com>
 */

#include "xkbcomp-priv.h"

static void
ComputeEffectiveMask(struct xkb_keymap *keymap, struct xkb_mods *mods)
{
    const struct xkb_mod *mod;
    xkb_mod_index_t i;

    /* The effective mask is only real mods for now. */
    mods->mask = mods->mods & MOD_REAL_MASK_ALL;

    darray_enumerate(i, mod, keymap->mods)
        if (mods->mods & (1u << i))
            mods->mask |= mod->mapping;
}

static void
UpdateActionMods(struct xkb_keymap *keymap, union xkb_action *act,
                 xkb_mod_mask_t modmap)
{
    switch (act->type) {
    case ACTION_TYPE_MOD_SET:
    case ACTION_TYPE_MOD_LATCH:
    case ACTION_TYPE_MOD_LOCK:
        if (act->mods.flags & ACTION_MODS_LOOKUP_MODMAP)
            act->mods.mods.mods = modmap;
        ComputeEffectiveMask(keymap, &act->mods.mods);
        break;
    default:
        break;
    }
}

static const struct xkb_sym_interpret default_interpret = {
    .sym = XKB_KEY_NoSymbol,
    .repeat = true,
    .match = MATCH_ANY_OR_NONE,
    .mods = 0,
    .virtual_mod = XKB_MOD_INVALID,
    .action = { .type = ACTION_TYPE_NONE },
};

/**
 * Find an interpretation which applies to this particular level, either by
 * finding an exact match for the symbol and modifier combination, or a
 * generic XKB_KEY_NoSymbol match.
 */
static const struct xkb_sym_interpret *
FindInterpForKey(struct xkb_keymap *keymap, const struct xkb_key *key,
                 xkb_layout_index_t group, xkb_level_index_t level)
{
    const xkb_keysym_t *syms;
    int num_syms;

    num_syms = xkb_keymap_key_get_syms_by_level(keymap, key->keycode, group,
                                                level, &syms);
    if (num_syms == 0)
        return NULL;

    /*
     * There may be multiple matchings interprets; we should always return
     * the most specific. Here we rely on compat.c to set up the
     * sym_interprets array from the most specific to the least specific,
     * such that when we find a match we return immediately.
     */
    for (unsigned i = 0; i < keymap->num_sym_interprets; i++) {
        const struct xkb_sym_interpret *interp = &keymap->sym_interprets[i];

        xkb_mod_mask_t mods;
        bool found = false;

        if ((num_syms > 1 || interp->sym != syms[0]) &&
            interp->sym != XKB_KEY_NoSymbol)
            continue;

        if (interp->level_one_only && level != 0)
            mods = 0;
        else
            mods = key->modmap;

        switch (interp->match) {
        case MATCH_NONE:
            found = !(interp->mods & mods);
            break;
        case MATCH_ANY_OR_NONE:
            found = (!mods || (interp->mods & mods));
            break;
        case MATCH_ANY:
            found = !!(interp->mods & mods);
            break;
        case MATCH_ALL:
            found = ((interp->mods & mods) == interp->mods);
            break;
        case MATCH_EXACTLY:
            found = (interp->mods == mods);
            break;
        }

        if (found)
            return interp;
    }

    return &default_interpret;
}

static bool
ApplyInterpsToKey(struct xkb_keymap *keymap, struct xkb_key *key)
{
    xkb_mod_mask_t vmodmap = 0;
    xkb_layout_index_t group;
    xkb_level_index_t level;

    /* If we've been told not to bind interps to this key, then don't. */
    if (key->explicit & EXPLICIT_INTERP)
        return true;

    for (group = 0; group < key->num_groups; group++) {
        for (level = 0; level < XkbKeyGroupWidth(key, group); level++) {
            const struct xkb_sym_interpret *interp;

            interp = FindInterpForKey(keymap, key, group, level);
            if (!interp)
                continue;

            /* Infer default key behaviours from the base level. */
            if (group == 0 && level == 0)
                if (!(key->explicit & EXPLICIT_REPEAT) && interp->repeat)
                    key->repeats = true;

            if ((group == 0 && level == 0) || !interp->level_one_only)
                if (interp->virtual_mod != XKB_MOD_INVALID)
                    vmodmap |= (1u << interp->virtual_mod);

            if (interp->action.type != ACTION_TYPE_NONE)
                key->groups[group].levels[level].action = interp->action;
        }
    }

    if (!(key->explicit & EXPLICIT_VMODMAP))
        key->vmodmap = vmodmap;

    return true;
}

/**
 * This collects a bunch of disparate functions which was done in the server
 * at various points that really should've been done within xkbcomp.  Turns out
 * your actions and types are a lot more useful when any of your modifiers
 * other than Shift actually do something ...
 */
static bool
UpdateDerivedKeymapFields(struct xkb_keymap *keymap)
{
    struct xkb_mod *mod;
    struct xkb_led *led;
    unsigned int i, j;
    struct xkb_key *key;

    /* Find all the interprets for the key and bind them to actions,
     * which will also update the vmodmap. */
    xkb_foreach_key(key, keymap)
        if (!ApplyInterpsToKey(keymap, key))
            return false;

    /* Update keymap->mods, the virtual -> real mod mapping. */
    xkb_foreach_key(key, keymap)
        darray_enumerate(i, mod, keymap->mods)
            if (key->vmodmap & (1u << i))
                mod->mapping |= key->modmap;

    /* Now update the level masks for all the types to reflect the vmods. */
    for (i = 0; i < keymap->num_types; i++) {
        ComputeEffectiveMask(keymap, &keymap->types[i].mods);

        for (j = 0; j < keymap->types[i].num_entries; j++) {
            ComputeEffectiveMask(keymap, &keymap->types[i].entries[j].mods);
            ComputeEffectiveMask(keymap, &keymap->types[i].entries[j].preserve);
        }
    }

    /* Update action modifiers. */
    xkb_foreach_key(key, keymap)
        for (i = 0; i < key->num_groups; i++)
            for (j = 0; j < XkbKeyGroupWidth(key, i); j++)
                UpdateActionMods(keymap, &key->groups[i].levels[j].action,
                                 key->modmap);

    /* Update vmod -> led maps. */
    darray_foreach(led, keymap->leds)
        ComputeEffectiveMask(keymap, &led->mods);

    /* Find maximum number of groups out of all keys in the keymap. */
    xkb_foreach_key(key, keymap)
        keymap->num_groups = MAX(keymap->num_groups, key->num_groups);

    return true;
}

typedef bool (*compile_file_fn)(XkbFile *file,
                                struct xkb_keymap *keymap,
                                enum merge_mode merge);

static const compile_file_fn compile_file_fns[LAST_KEYMAP_FILE_TYPE + 1] = {
    [FILE_TYPE_KEYCODES] = CompileKeycodes,
    [FILE_TYPE_TYPES] = CompileKeyTypes,
    [FILE_TYPE_COMPAT] = CompileCompatMap,
    [FILE_TYPE_SYMBOLS] = CompileSymbols,
};

bool
CompileKeymap(XkbFile *file, struct xkb_keymap *keymap, enum merge_mode merge)
{
    bool ok;
    const char *main_name;
    XkbFile *files[LAST_KEYMAP_FILE_TYPE + 1] = { NULL };
    enum xkb_file_type type;
    struct xkb_context *ctx = keymap->ctx;

    main_name = file->name ? file->name : "(unnamed)";

    /* Collect section files and check for duplicates. */
    for (file = (XkbFile *) file->defs; file;
         file = (XkbFile *) file->common.next) {
        if (file->file_type < FIRST_KEYMAP_FILE_TYPE ||
            file->file_type > LAST_KEYMAP_FILE_TYPE) {
            log_err(ctx, "Cannot define %s in a keymap file\n",
                    xkb_file_type_to_string(file->file_type));
            continue;
        }

        if (files[file->file_type]) {
            log_err(ctx,
                    "More than one %s section in keymap file; "
                    "All sections after the first ignored\n",
                    xkb_file_type_to_string(file->file_type));
            continue;
        }

        if (!file->topName) {
            free(file->topName);
            file->topName = strdup(main_name);
        }

        files[file->file_type] = file;
    }

    /*
     * Check that all required section were provided.
     * Report everything before failing.
     */
    ok = true;
    for (type = FIRST_KEYMAP_FILE_TYPE;
         type <= LAST_KEYMAP_FILE_TYPE;
         type++) {
        if (files[type] == NULL) {
            log_err(ctx, "Required section %s missing from keymap\n",
                    xkb_file_type_to_string(type));
            ok = false;
        }
    }
    if (!ok)
        return false;

    /* Compile sections. */
    for (type = FIRST_KEYMAP_FILE_TYPE;
         type <= LAST_KEYMAP_FILE_TYPE;
         type++) {
        log_dbg(ctx, "Compiling %s \"%s\"\n",
                xkb_file_type_to_string(type), files[type]->topName);

        ok = compile_file_fns[type](files[type], keymap, merge);
        if (!ok) {
            log_err(ctx, "Failed to compile %s\n",
                    xkb_file_type_to_string(type));
            return false;
        }
    }

    return UpdateDerivedKeymapFields(keymap);
}
