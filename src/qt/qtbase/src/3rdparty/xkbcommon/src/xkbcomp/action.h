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

#ifndef XKBCOMP_ACTION_H
#define XKBCOMP_ACTION_H

/*
 * This struct contains the default values which every new action
 * (e.g. in an interpret statement) starts off with. It can be
 * modified within the files (see calls to SetActionField).
 */
typedef struct {
    union xkb_action actions[_ACTION_TYPE_NUM_ENTRIES];
} ActionsInfo;

ActionsInfo *
NewActionsInfo(void);

void
FreeActionsInfo(ActionsInfo *info);

bool
HandleActionDef(ExprDef *def, struct xkb_keymap *keymap,
                union xkb_action *action, ActionsInfo *info);

bool
SetActionField(struct xkb_keymap *keymap, const char *elem, const char *field,
               ExprDef *array_ndx, ExprDef *value, ActionsInfo *info);

#endif
