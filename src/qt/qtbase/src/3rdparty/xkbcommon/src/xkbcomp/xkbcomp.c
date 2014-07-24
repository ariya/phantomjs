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
 * Authors: Dan Nicholson <dbn.lists@gmail.com>
 *          Ran Benita <ran234@gmail.com>
 *          Daniel Stone <daniel@fooishbar.org>
 */

#include "xkbcomp-priv.h"
#include "rules.h"

static bool
compile_keymap_file(struct xkb_keymap *keymap, XkbFile *file)
{
    if (file->file_type != FILE_TYPE_KEYMAP) {
        log_err(keymap->ctx,
                "Cannot compile a %s file alone into a keymap\n",
                xkb_file_type_to_string(file->file_type));
        return false;
    }

    if (!CompileKeymap(file, keymap, MERGE_OVERRIDE)) {
        log_err(keymap->ctx,
                "Failed to compile keymap\n");
        return false;
    }

    return true;
}

static bool
text_v1_keymap_new_from_names(struct xkb_keymap *keymap,
                              const struct xkb_rule_names *rmlvo)
{
    bool ok;
    struct xkb_component_names kccgst;
    XkbFile *file;

    log_dbg(keymap->ctx,
            "Compiling from RMLVO: rules '%s', model '%s', layout '%s', "
            "variant '%s', options '%s'\n",
            rmlvo->rules, rmlvo->model, rmlvo->layout, rmlvo->variant,
            rmlvo->options);

    ok = xkb_components_from_rules(keymap->ctx, rmlvo, &kccgst);
    if (!ok) {
        log_err(keymap->ctx,
                "Couldn't look up rules '%s', model '%s', layout '%s', "
                "variant '%s', options '%s'\n",
                rmlvo->rules, rmlvo->model, rmlvo->layout, rmlvo->variant,
                rmlvo->options);
        return false;
    }

    log_dbg(keymap->ctx,
            "Compiling from KcCGST: keycodes '%s', types '%s', "
            "compat '%s', symbols '%s'\n",
            kccgst.keycodes, kccgst.types, kccgst.compat, kccgst.symbols);

    file = XkbFileFromComponents(keymap->ctx, &kccgst);

    free(kccgst.keycodes);
    free(kccgst.types);
    free(kccgst.compat);
    free(kccgst.symbols);

    if (!file) {
        log_err(keymap->ctx,
                "Failed to generate parsed XKB file from components\n");
        return false;
    }

    ok = compile_keymap_file(keymap, file);
    FreeXkbFile(file);
    return ok;
}

static bool
text_v1_keymap_new_from_string(struct xkb_keymap *keymap,
                               const char *string, size_t len)
{
    bool ok;
    XkbFile *xkb_file;

    xkb_file = XkbParseString(keymap->ctx, string, len, "(input string)", NULL);
    if (!xkb_file) {
        log_err(keymap->ctx, "Failed to parse input xkb string\n");
        return NULL;
    }

    ok = compile_keymap_file(keymap, xkb_file);
    FreeXkbFile(xkb_file);
    return ok;
}

static bool
text_v1_keymap_new_from_file(struct xkb_keymap *keymap, FILE *file)
{
    bool ok;
    XkbFile *xkb_file;

    xkb_file = XkbParseFile(keymap->ctx, file, "(unknown file)", NULL);
    if (!xkb_file) {
        log_err(keymap->ctx, "Failed to parse input xkb file\n");
        return false;
    }

    ok = compile_keymap_file(keymap, xkb_file);
    FreeXkbFile(xkb_file);
    return ok;
}

const struct xkb_keymap_format_ops text_v1_keymap_format_ops = {
    .keymap_new_from_names = text_v1_keymap_new_from_names,
    .keymap_new_from_string = text_v1_keymap_new_from_string,
    .keymap_new_from_file = text_v1_keymap_new_from_file,
    .keymap_get_as_string = text_v1_keymap_get_as_string,
};
