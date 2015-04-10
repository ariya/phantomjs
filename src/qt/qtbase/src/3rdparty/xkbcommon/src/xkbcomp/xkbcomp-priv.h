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

#ifndef XKBCOMP_PRIV_H
#define XKBCOMP_PRIV_H

#include "keymap.h"
#include "ast.h"

struct xkb_component_names {
    char *keycodes;
    char *types;
    char *compat;
    char *symbols;
};

char *
text_v1_keymap_get_as_string(struct xkb_keymap *keymap);

XkbFile *
XkbParseFile(struct xkb_context *ctx, FILE *file,
             const char *file_name, const char *map);

XkbFile *
XkbParseString(struct xkb_context *ctx,
               const char *string, size_t len,
               const char *file_name, const char *map);

void
FreeXkbFile(XkbFile *file);

XkbFile *
XkbFileFromComponents(struct xkb_context *ctx,
                      const struct xkb_component_names *kkctgs);

bool
CompileKeycodes(XkbFile *file, struct xkb_keymap *keymap,
                enum merge_mode merge);

bool
CompileKeyTypes(XkbFile *file, struct xkb_keymap *keymap,
                enum merge_mode merge);

bool
CompileCompatMap(XkbFile *file, struct xkb_keymap *keymap,
                 enum merge_mode merge);

bool
CompileSymbols(XkbFile *file, struct xkb_keymap *keymap,
               enum merge_mode merge);

bool
CompileKeymap(XkbFile *file, struct xkb_keymap *keymap,
              enum merge_mode merge);

/***====================================================================***/

static inline bool
ReportNotArray(struct xkb_context *ctx, const char *type, const char *field,
               const char *name)
{
    log_err(ctx,
            "The %s %s field is not an array; "
            "Ignoring illegal assignment in %s\n",
            type, field, name);
    return false;
}

static inline bool
ReportShouldBeArray(struct xkb_context *ctx, const char *type,
                    const char *field, const char *name)
{
    log_err(ctx,
            "Missing subscript for %s %s; "
            "Ignoring illegal assignment in %s\n",
            type, field, name);
    return false;
}

static inline bool
ReportBadType(struct xkb_context *ctx, const char *type, const char *field,
              const char *name, const char *wanted)
{
    log_err(ctx, "The %s %s field must be a %s; "
            "Ignoring illegal assignment in %s\n",
            type, field, wanted, name);
    return false;
}

static inline bool
ReportBadField(struct xkb_context *ctx, const char *type, const char *field,
               const char *name)
{
    log_err(ctx,
            "Unknown %s field %s in %s; "
            "Ignoring assignment to unknown field in %s\n",
            type, field, name, name);
    return false;
}

#endif
