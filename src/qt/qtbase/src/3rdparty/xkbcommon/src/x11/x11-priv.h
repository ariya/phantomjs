/*
 * Copyright © 2013 Ran Benita
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
 */

#ifndef _XKBCOMMON_X11_PRIV_H
#define _XKBCOMMON_X11_PRIV_H

#include <xcb/xkb.h>

#include "keymap.h"
#include "xkbcommon/xkbcommon-x11.h"

/* Get a strdup'd name of an X atom. */
bool
get_atom_name(xcb_connection_t *conn, xcb_atom_t atom, char **out);

/*
 * Make a xkb_atom_t's from X atoms (prefer to send as many as possible
 * at once, to avoid many roundtrips).
 *
 * TODO: We can make this more flexible, such that @to doesn't have to
 *       be sequential. Then we can convert most adopt_atom() calls to
 *       adopt_atoms().
 *       Atom caching would also likely be useful for avoiding quite a
 *       few requests.
 */
bool
adopt_atoms(struct xkb_context *ctx, xcb_connection_t *conn,
            const xcb_atom_t *from, xkb_atom_t *to, size_t count);

bool
adopt_atom(struct xkb_context *ctx, xcb_connection_t *conn, xcb_atom_t atom,
           xkb_atom_t *out);

#endif
