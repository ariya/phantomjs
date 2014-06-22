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
#include "vmod.h"

bool
HandleVModDef(struct xkb_keymap *keymap, VModDef *stmt,
              enum merge_mode merge)
{
    xkb_mod_index_t i;
    struct xkb_mod *mod;
    xkb_mod_mask_t mapping;
    struct xkb_mod new;

    merge = (merge == MERGE_DEFAULT ? stmt->merge : merge);

    if (stmt->value) {
        /*
         * This is a statement such as 'virtualModifiers NumLock = Mod1';
         * it sets the vmod-to-real-mod[s] mapping directly instead of going
         * through modifier_map or some such.
         */
        if (!ExprResolveModMask(keymap, stmt->value, MOD_REAL, &mapping)) {
            log_err(keymap->ctx,
                    "Declaration of %s ignored\n",
                    xkb_atom_text(keymap->ctx, stmt->name));
            return false;
        }
    }
    else {
        mapping = 0;
    }

    darray_enumerate(i, mod, keymap->mods) {
        if (mod->name == stmt->name) {
            if (mod->type != MOD_VIRT) {
                log_err(keymap->ctx,
                        "Can't add a virtual modifier named \"%s\"; "
                        "there is already a non-virtual modifier with this name! Ignored\n",
                        xkb_atom_text(keymap->ctx, mod->name));
                return false;
            }

            if (mod->mapping == mapping)
                return true;

            if (mod->mapping != 0) {
                xkb_mod_mask_t use, ignore;

                use = (merge == MERGE_OVERRIDE ? mapping : mod->mapping);
                ignore = (merge == MERGE_OVERRIDE ? mod->mapping : mapping);

                log_warn(keymap->ctx,
                         "Virtual modifier %s defined multiple times; "
                         "Using %s, ignoring %s\n",
                         xkb_atom_text(keymap->ctx, stmt->name),
                         ModMaskText(keymap, use),
                         ModMaskText(keymap, ignore));

                mapping = use;
            }

            mod->mapping = mapping;
            return true;
        }
    }

    if (darray_size(keymap->mods) >= XKB_MAX_MODS) {
        log_err(keymap->ctx,
                "Too many modifiers defined (maximum %d)\n",
                XKB_MAX_MODS);
        return false;
    }

    new.name = stmt->name;
    new.mapping = mapping;
    new.type = MOD_VIRT;
    darray_append(keymap->mods, new);
    return true;
}
