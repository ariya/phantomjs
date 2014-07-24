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
#include "include.h"

typedef struct {
    enum merge_mode merge;

    xkb_atom_t alias;
    xkb_atom_t real;
} AliasInfo;

typedef struct {
    enum merge_mode merge;

    xkb_atom_t name;
} LedNameInfo;

typedef struct {
    char *name;
    int errorCount;

    xkb_keycode_t min_key_code;
    xkb_keycode_t max_key_code;
    darray(xkb_atom_t) key_names;
    darray(LedNameInfo) led_names;
    darray(AliasInfo) aliases;

    struct xkb_context *ctx;
} KeyNamesInfo;

/***====================================================================***/

static void
InitAliasInfo(AliasInfo *info, enum merge_mode merge,
              xkb_atom_t alias, xkb_atom_t real)
{
    memset(info, 0, sizeof(*info));
    info->merge = merge;
    info->alias = alias;
    info->real = real;
}

static LedNameInfo *
FindLedByName(KeyNamesInfo *info, xkb_atom_t name,
              xkb_led_index_t *idx_out)
{
    LedNameInfo *ledi;
    xkb_led_index_t idx;

    darray_enumerate(idx, ledi, info->led_names) {
        if (ledi->name == name) {
            *idx_out = idx;
            return ledi;
        }
    }

    return NULL;
}

static bool
AddLedName(KeyNamesInfo *info, enum merge_mode merge, bool same_file,
           LedNameInfo *new, xkb_led_index_t new_idx)
{
    xkb_led_index_t old_idx;
    LedNameInfo *old;
    const int verbosity = xkb_context_get_log_verbosity(info->ctx);
    const bool report = (same_file && verbosity > 0) || verbosity > 9;
    const bool replace = (merge == MERGE_REPLACE || merge == MERGE_OVERRIDE);

    /* LED with the same name already exists. */
    old = FindLedByName(info, new->name, &old_idx);
    if (old) {
        if (old_idx == new_idx) {
            log_warn(info->ctx,
                     "Multiple indicators named \"%s\"; "
                     "Identical definitions ignored\n",
                     xkb_atom_text(info->ctx, new->name));
            return true;
        }

        if (report) {
            xkb_led_index_t use = (replace ? new_idx + 1 : old_idx + 1);
            xkb_led_index_t ignore = (replace ? old_idx + 1 : new_idx + 1);
            log_warn(info->ctx,
                     "Multiple indicators named %s; Using %d, ignoring %d\n",
                     xkb_atom_text(info->ctx, new->name), use, ignore);
        }

        if (replace)
            *old = *new;

        return true;
    }

    if (new_idx >= darray_size(info->led_names))
        darray_resize0(info->led_names, new_idx + 1);

    /* LED with the same index already exists. */
    old = &darray_item(info->led_names, new_idx);
    if (old->name != XKB_ATOM_NONE) {
        if (report) {
            const xkb_atom_t use = (replace ? new->name : old->name);
            const xkb_atom_t ignore = (replace ? old->name : new->name);
            log_warn(info->ctx, "Multiple names for indicator %d; "
                     "Using %s, ignoring %s\n", new_idx + 1,
                     xkb_atom_text(info->ctx, use),
                     xkb_atom_text(info->ctx, ignore));
        }

        if (replace)
            *old = *new;

        return true;
    }

    darray_item(info->led_names, new_idx) = *new;
    return true;
}

static void
ClearKeyNamesInfo(KeyNamesInfo *info)
{
    free(info->name);
    darray_free(info->key_names);
    darray_free(info->aliases);
    darray_free(info->led_names);
}

static void
InitKeyNamesInfo(KeyNamesInfo *info, struct xkb_context *ctx)
{
    memset(info, 0, sizeof(*info));
    info->ctx = ctx;
    info->min_key_code = XKB_KEYCODE_INVALID;
#if XKB_KEYCODE_INVALID < XKB_KEYCODE_MAX
#error "Hey, you can't be changing stuff like that."
#endif
}

static xkb_keycode_t
FindKeyByName(KeyNamesInfo *info, xkb_atom_t name)
{
    xkb_keycode_t i;

    for (i = info->min_key_code; i <= info->max_key_code; i++)
        if (darray_item(info->key_names, i) == name)
            return i;

    return XKB_KEYCODE_INVALID;
}

static bool
AddKeyName(KeyNamesInfo *info, xkb_keycode_t kc, xkb_atom_t name,
           enum merge_mode merge, bool same_file, bool report)
{
    xkb_atom_t old_name;
    xkb_keycode_t old_kc;
    const int verbosity = xkb_context_get_log_verbosity(info->ctx);

    report = report && ((same_file && verbosity > 0) || verbosity > 7);

    if (kc >= darray_size(info->key_names))
        darray_resize0(info->key_names, kc + 1);

    info->min_key_code = MIN(info->min_key_code, kc);
    info->max_key_code = MAX(info->max_key_code, kc);

    /* There's already a key with this keycode. */
    old_name = darray_item(info->key_names, kc);
    if (old_name != XKB_ATOM_NONE) {
        const char *lname = KeyNameText(info->ctx, old_name);
        const char *kname = KeyNameText(info->ctx, name);

        if (old_name == name) {
            if (report)
                log_warn(info->ctx,
                         "Multiple identical key name definitions; "
                         "Later occurrences of \"%s = %d\" ignored\n",
                         lname, kc);
            return true;
        }
        else if (merge == MERGE_AUGMENT) {
            if (report)
                log_warn(info->ctx,
                         "Multiple names for keycode %d; "
                         "Using %s, ignoring %s\n", kc, lname, kname);
            return true;
        }
        else {
            if (report)
                log_warn(info->ctx,
                         "Multiple names for keycode %d; "
                         "Using %s, ignoring %s\n", kc, kname, lname);
            darray_item(info->key_names, kc) = XKB_ATOM_NONE;
        }
    }

    /* There's already a key with this name. */
    old_kc = FindKeyByName(info, name);
    if (old_kc != XKB_KEYCODE_INVALID && old_kc != kc) {
        const char *kname = KeyNameText(info->ctx, name);

        if (merge == MERGE_OVERRIDE) {
            darray_item(info->key_names, old_kc) = XKB_ATOM_NONE;
            if (report)
                log_warn(info->ctx,
                         "Key name %s assigned to multiple keys; "
                         "Using %d, ignoring %d\n", kname, kc, old_kc);
        }
        else {
            if (report)
                log_vrb(info->ctx, 3,
                        "Key name %s assigned to multiple keys; "
                        "Using %d, ignoring %d\n", kname, old_kc, kc);
            return true;
        }
    }

    darray_item(info->key_names, kc) = name;
    return true;
}

/***====================================================================***/

static bool
HandleAliasDef(KeyNamesInfo *info, KeyAliasDef *def, enum merge_mode merge);

static void
MergeIncludedKeycodes(KeyNamesInfo *into, KeyNamesInfo *from,
                      enum merge_mode merge)
{
    if (from->errorCount > 0) {
        into->errorCount += from->errorCount;
        return;
    }

    if (into->name == NULL) {
        into->name = from->name;
        from->name = NULL;
    }

    /* Merge key names. */
    if (darray_empty(into->key_names)) {
        into->key_names = from->key_names;
        darray_init(from->key_names);
        into->min_key_code = from->min_key_code;
        into->max_key_code = from->max_key_code;
    }
    else {
        if (darray_size(into->key_names) < darray_size(from->key_names))
            darray_resize0(into->key_names, darray_size(from->key_names));

        for (unsigned i = from->min_key_code; i <= from->max_key_code; i++) {
            xkb_atom_t name = darray_item(from->key_names, i);
            if (name == XKB_ATOM_NONE)
                continue;

            if (!AddKeyName(into, i, name, merge, true, false))
                into->errorCount++;
        }
    }

    /* Merge key aliases. */
    if (darray_empty(into->aliases)) {
        into->aliases = from->aliases;
        darray_init(from->aliases);
    }
    else {
        AliasInfo *alias;

        darray_foreach(alias, from->aliases) {
            KeyAliasDef def;

            def.merge = (merge == MERGE_DEFAULT ? alias->merge : merge);
            def.alias = alias->alias;
            def.real = alias->real;

            if (!HandleAliasDef(into, &def, def.merge))
                into->errorCount++;
        }
    }

    /* Merge LED names. */
    if (darray_empty(into->led_names)) {
        into->led_names = from->led_names;
        darray_init(from->led_names);
    }
    else {
        xkb_led_index_t idx;
        LedNameInfo *ledi;

        darray_enumerate(idx, ledi, from->led_names) {
            if (ledi->name == XKB_ATOM_NONE)
                continue;

            ledi->merge = (merge == MERGE_DEFAULT ? ledi->merge : merge);
            if (!AddLedName(into, ledi->merge, false, ledi, idx))
                into->errorCount++;
        }
    }
}

static void
HandleKeycodesFile(KeyNamesInfo *info, XkbFile *file, enum merge_mode merge);

static bool
HandleIncludeKeycodes(KeyNamesInfo *info, IncludeStmt *include)
{
    KeyNamesInfo included;

    InitKeyNamesInfo(&included, info->ctx);
    included.name = include->stmt;
    include->stmt = NULL;

    for (IncludeStmt *stmt = include; stmt; stmt = stmt->next_incl) {
        KeyNamesInfo next_incl;
        XkbFile *file;

        file = ProcessIncludeFile(info->ctx, stmt, FILE_TYPE_KEYCODES);
        if (!file) {
            info->errorCount += 10;
            ClearKeyNamesInfo(&included);
            return false;
        }

        InitKeyNamesInfo(&next_incl, info->ctx);

        HandleKeycodesFile(&next_incl, file, MERGE_OVERRIDE);

        MergeIncludedKeycodes(&included, &next_incl, stmt->merge);

        ClearKeyNamesInfo(&next_incl);
        FreeXkbFile(file);
    }

    MergeIncludedKeycodes(info, &included, include->merge);
    ClearKeyNamesInfo(&included);

    return (info->errorCount == 0);
}

static bool
HandleKeycodeDef(KeyNamesInfo *info, KeycodeDef *stmt, enum merge_mode merge)
{
    if (stmt->merge != MERGE_DEFAULT) {
        if (stmt->merge == MERGE_REPLACE)
            merge = MERGE_OVERRIDE;
        else
            merge = stmt->merge;
    }

    if (stmt->value < 0 || stmt->value > XKB_KEYCODE_MAX) {
        log_err(info->ctx,
                "Illegal keycode %lld: must be between 0..%u; "
                "Key ignored\n", (long long) stmt->value, XKB_KEYCODE_MAX);
        return false;
    }

    return AddKeyName(info, stmt->value, stmt->name, merge, false, true);
}

static bool
HandleAliasDef(KeyNamesInfo *info, KeyAliasDef *def, enum merge_mode merge)
{
    AliasInfo *old, new;

    darray_foreach(old, info->aliases) {
        if (old->alias == def->alias) {
            if (def->real == old->real) {
                log_vrb(info->ctx, 1,
                        "Alias of %s for %s declared more than once; "
                        "First definition ignored\n",
                        KeyNameText(info->ctx, def->alias),
                        KeyNameText(info->ctx, def->real));
            }
            else {
                xkb_atom_t use, ignore;

                use = (merge == MERGE_AUGMENT ? old->real : def->real);
                ignore = (merge == MERGE_AUGMENT ? def->real : old->real);

                log_warn(info->ctx,
                         "Multiple definitions for alias %s; "
                         "Using %s, ignoring %s\n",
                         KeyNameText(info->ctx, old->alias),
                         KeyNameText(info->ctx, use),
                         KeyNameText(info->ctx, ignore));

                old->real = use;
            }

            old->merge = merge;
            return true;
        }
    }

    InitAliasInfo(&new, merge, def->alias, def->real);
    darray_append(info->aliases, new);
    return true;
}

static bool
HandleKeyNameVar(KeyNamesInfo *info, VarDef *stmt)
{
    const char *elem, *field;
    ExprDef *arrayNdx;

    if (!ExprResolveLhs(info->ctx, stmt->name, &elem, &field, &arrayNdx))
        return false;

    if (elem) {
        log_err(info->ctx, "Unknown element %s encountered; "
                "Default for field %s ignored\n", elem, field);
        return false;
    }

    if (!istreq(field, "minimum") && !istreq(field, "maximum")) {
        log_err(info->ctx, "Unknown field encountered; "
                "Assignment to field %s ignored\n", field);
        return false;
    }

    /* We ignore explicit min/max statements, we always use computed. */
    return true;
}

static bool
HandleLedNameDef(KeyNamesInfo *info, LedNameDef *def,
                 enum merge_mode merge)
{
    LedNameInfo ledi;
    xkb_atom_t name;

    if (def->ndx < 1 || def->ndx > XKB_MAX_LEDS) {
        info->errorCount++;
        log_err(info->ctx,
                "Illegal indicator index (%d) specified; must be between 1 .. %d; "
                "Ignored\n", def->ndx, XKB_MAX_LEDS);
        return false;
    }

    if (!ExprResolveString(info->ctx, def->name, &name)) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", def->ndx);
        info->errorCount++;
        return ReportBadType(info->ctx, "indicator", "name", buf, "string");
    }

    ledi.merge = merge;
    ledi.name = name;
    return AddLedName(info, merge, true, &ledi, def->ndx - 1);
}

static void
HandleKeycodesFile(KeyNamesInfo *info, XkbFile *file, enum merge_mode merge)
{
    bool ok;

    free(info->name);
    info->name = strdup_safe(file->name);

    for (ParseCommon *stmt = file->defs; stmt; stmt = stmt->next) {
        switch (stmt->type) {
        case STMT_INCLUDE:
            ok = HandleIncludeKeycodes(info, (IncludeStmt *) stmt);
            break;
        case STMT_KEYCODE:
            ok = HandleKeycodeDef(info, (KeycodeDef *) stmt, merge);
            break;
        case STMT_ALIAS:
            ok = HandleAliasDef(info, (KeyAliasDef *) stmt, merge);
            break;
        case STMT_VAR:
            ok = HandleKeyNameVar(info, (VarDef *) stmt);
            break;
        case STMT_LED_NAME:
            ok = HandleLedNameDef(info, (LedNameDef *) stmt, merge);
            break;
        default:
            log_err(info->ctx,
                    "Keycode files may define key and indicator names only; "
                    "Ignoring %s\n", stmt_type_to_string(stmt->type));
            ok = false;
            break;
        }

        if (!ok)
            info->errorCount++;

        if (info->errorCount > 10) {
            log_err(info->ctx, "Abandoning keycodes file \"%s\"\n",
                    file->topName);
            break;
        }
    }
}

/***====================================================================***/

static bool
CopyKeyNamesToKeymap(struct xkb_keymap *keymap, KeyNamesInfo *info)
{
    xkb_keycode_t kc;
    xkb_led_index_t idx;
    LedNameInfo *ledi;
    AliasInfo *alias;
    unsigned i;

    keymap->keycodes_section_name = strdup_safe(info->name);
    XkbEscapeMapName(keymap->keycodes_section_name);

    if (info->min_key_code != XKB_KEYCODE_INVALID) {
        keymap->min_key_code = info->min_key_code;
        keymap->max_key_code = info->max_key_code;
    }
    else {
        /*
         * If the keymap has no keys, let's just use the safest pair
         * we know.
         */
        keymap->min_key_code = 8;
        keymap->max_key_code = 255;
    }

    keymap->keys = calloc(keymap->max_key_code + 1, sizeof(*keymap->keys));
    for (kc = keymap->min_key_code; kc <= keymap->max_key_code; kc++)
        keymap->keys[kc].keycode = kc;

    /* Copy key names. */
    for (kc = info->min_key_code; kc <= info->max_key_code; kc++)
        keymap->keys[kc].name = darray_item(info->key_names, kc);

    /*
     * Do some sanity checking on the aliases. We can't do it before
     * because keys and their aliases may be added out-of-order.
     */
    keymap->num_key_aliases = 0;
    darray_foreach(alias, info->aliases) {
        /* Check that ->real is a key. */
        if (!XkbKeyByName(keymap, alias->real, false)) {
            log_vrb(info->ctx, 5,
                    "Attempt to alias %s to non-existent key %s; Ignored\n",
                    KeyNameText(info->ctx, alias->alias),
                    KeyNameText(info->ctx, alias->real));
            alias->real = XKB_ATOM_NONE;
            continue;
        }

        /* Check that ->alias is not a key. */
        if (XkbKeyByName(keymap, alias->alias, false)) {
            log_vrb(info->ctx, 5,
                    "Attempt to create alias with the name of a real key; "
                    "Alias \"%s = %s\" ignored\n",
                    KeyNameText(info->ctx, alias->alias),
                    KeyNameText(info->ctx, alias->real));
            alias->real = XKB_ATOM_NONE;
            continue;
        }

        keymap->num_key_aliases++;
    }

    /* Copy key aliases. */
    keymap->key_aliases = calloc(keymap->num_key_aliases,
                                 sizeof(*keymap->key_aliases));
    i = 0;
    darray_foreach(alias, info->aliases) {
        if (alias->real != XKB_ATOM_NONE) {
            keymap->key_aliases[i].alias = alias->alias;
            keymap->key_aliases[i].real = alias->real;
            i++;
        }
    }

    /* Copy LED names. */
    darray_resize0(keymap->leds, darray_size(info->led_names));
    darray_enumerate(idx, ledi, info->led_names)
        if (ledi->name != XKB_ATOM_NONE)
            darray_item(keymap->leds, idx).name = ledi->name;

    return true;
}

/***====================================================================***/

bool
CompileKeycodes(XkbFile *file, struct xkb_keymap *keymap,
                enum merge_mode merge)
{
    KeyNamesInfo info;

    InitKeyNamesInfo(&info, keymap->ctx);

    HandleKeycodesFile(&info, file, merge);
    if (info.errorCount != 0)
        goto err_info;

    if (!CopyKeyNamesToKeymap(keymap, &info))
        goto err_info;

    ClearKeyNamesInfo(&info);
    return true;

err_info:
    ClearKeyNamesInfo(&info);
    return false;
}
