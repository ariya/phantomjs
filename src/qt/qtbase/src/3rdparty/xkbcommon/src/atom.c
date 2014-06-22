/***********************************************************
 * Copyright 1987, 1998  The Open Group
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of The Open Group shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from The Open Group.
 *
 *
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 *
 *                      All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Digital not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 ******************************************************************/

/************************************************************
 * Copyright 1994 by Silicon Graphics Computer Systems, Inc.
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

#include "utils.h"
#include "atom.h"

struct atom_node {
    xkb_atom_t left, right;
    xkb_atom_t atom;
    unsigned int fingerprint;
    char *string;
};

struct atom_table {
    xkb_atom_t root;
    darray(struct atom_node) table;
};

struct atom_table *
atom_table_new(void)
{
    struct atom_table *table;

    table = calloc(1, sizeof(*table));
    if (!table)
        return NULL;

    darray_init(table->table);
    /* The original throw-away root is here, at the illegal atom 0. */
    darray_resize0(table->table, 1);

    return table;
}

void
atom_table_free(struct atom_table *table)
{
    struct atom_node *node;

    if (!table)
        return;

    darray_foreach(node, table->table)
        free(node->string);
    darray_free(table->table);
    free(table);
}

const char *
atom_text(struct atom_table *table, xkb_atom_t atom)
{
    if (atom == XKB_ATOM_NONE || atom >= darray_size(table->table))
        return NULL;

    return darray_item(table->table, atom).string;
}

static bool
find_atom_pointer(struct atom_table *table, const char *string, size_t len,
                  xkb_atom_t **atomp_out, unsigned int *fingerprint_out)
{
    xkb_atom_t *atomp = &table->root;
    unsigned int fingerprint = 0;
    bool found = false;

    for (size_t i = 0; i < (len + 1) / 2; i++) {
        fingerprint = fingerprint * 27 + string[i];
        fingerprint = fingerprint * 27 + string[len - 1 - i];
    }

    while (*atomp != XKB_ATOM_NONE) {
        struct atom_node *node = &darray_item(table->table, *atomp);

        if (fingerprint < node->fingerprint) {
            atomp = &node->left;
        }
        else if (fingerprint > node->fingerprint) {
            atomp = &node->right;
        }
        else {
            /* Now start testing the strings. */
            const int cmp = strncmp(string, node->string, len);
            if (cmp < 0 || (cmp == 0 && len < strlen(node->string))) {
                atomp = &node->left;
            }
            else if (cmp > 0) {
                atomp = &node->right;
            }
            else {
                found = true;
                break;
            }
        }
    }

    if (fingerprint_out)
        *fingerprint_out = fingerprint;
    if (atomp_out)
        *atomp_out = atomp;
    return found;
}

xkb_atom_t
atom_lookup(struct atom_table *table, const char *string, size_t len)
{
    xkb_atom_t *atomp;

    if (!string)
        return XKB_ATOM_NONE;

    if (!find_atom_pointer(table, string, len, &atomp, NULL))
        return XKB_ATOM_NONE;

    return *atomp;
}

/*
 * If steal is true, we do not strdup @string; therefore it must be
 * dynamically allocated, NUL-terminated, not be free'd by the caller
 * and not be used afterwards. Use to avoid some redundant allocations.
 */
xkb_atom_t
atom_intern(struct atom_table *table, const char *string, size_t len,
            bool steal)
{
    xkb_atom_t *atomp;
    struct atom_node node;
    unsigned int fingerprint;

    if (!string)
        return XKB_ATOM_NONE;

    if (find_atom_pointer(table, string, len, &atomp, &fingerprint)) {
        if (steal)
            free(UNCONSTIFY(string));
        return *atomp;
    }

    if (steal) {
        node.string = UNCONSTIFY(string);
    }
    else {
        node.string = strndup(string, len);
        if (!node.string)
            return XKB_ATOM_NONE;
    }

    node.left = node.right = XKB_ATOM_NONE;
    node.fingerprint = fingerprint;
    node.atom = darray_size(table->table);
    /* Do this before the append, as it may realloc and change the offsets. */
    *atomp = node.atom;
    darray_append(table->table, node);

    return node.atom;
}
