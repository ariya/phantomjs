/*
 * Copyright (C) 2010, Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "ClipboardUtilitiesGtk.h"

namespace WebCore {

GdkDragAction dragOperationToGdkDragActions(DragOperation coreAction)
{
    GdkDragAction gdkAction = static_cast<GdkDragAction>(0);
    if (coreAction == DragOperationNone)
        return gdkAction;

    if (coreAction & DragOperationCopy)
        gdkAction = static_cast<GdkDragAction>(GDK_ACTION_COPY | gdkAction);
    if (coreAction & DragOperationMove)
        gdkAction = static_cast<GdkDragAction>(GDK_ACTION_MOVE | gdkAction);
    if (coreAction & DragOperationLink)
        gdkAction = static_cast<GdkDragAction>(GDK_ACTION_LINK | gdkAction);
    if (coreAction & DragOperationPrivate)
        gdkAction = static_cast<GdkDragAction>(GDK_ACTION_PRIVATE | gdkAction);

    return gdkAction;
}

GdkDragAction dragOperationToSingleGdkDragAction(DragOperation coreAction)
{
    if (coreAction == DragOperationEvery || coreAction & DragOperationCopy)
        return GDK_ACTION_COPY;
    if (coreAction & DragOperationMove)
        return GDK_ACTION_MOVE;
    if (coreAction & DragOperationLink)
        return GDK_ACTION_LINK;
    if (coreAction & DragOperationPrivate)
        return GDK_ACTION_PRIVATE;
    return static_cast<GdkDragAction>(0);
}

DragOperation gdkDragActionToDragOperation(GdkDragAction gdkAction)
{
    // We have no good way to detect DragOperationEvery other than
    // to use it when all applicable flags are on.
    if (gdkAction & GDK_ACTION_COPY && gdkAction & GDK_ACTION_MOVE
        && gdkAction & GDK_ACTION_LINK && gdkAction & GDK_ACTION_PRIVATE)
        return DragOperationEvery;

    unsigned int action = DragOperationNone;
    if (gdkAction & GDK_ACTION_COPY)
        action |= DragOperationCopy;
    if (gdkAction & GDK_ACTION_MOVE)
        action |= DragOperationMove;
    if (gdkAction & GDK_ACTION_LINK)
        action |= DragOperationLink;
    if (gdkAction & GDK_ACTION_PRIVATE)
        action |= DragOperationPrivate;
    return static_cast<DragOperation>(action);
}

}
