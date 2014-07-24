/*
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "DragClientBlackBerry.h"

#include "NotImplemented.h"

namespace WebCore {

void DragClientBlackBerry::willPerformDragDestinationAction(DragDestinationAction, DragData*)
{
    notImplemented();
}

void DragClientBlackBerry::willPerformDragSourceAction(DragSourceAction, const IntPoint&, Clipboard*)
{
    notImplemented();
}

DragDestinationAction DragClientBlackBerry::actionMaskForDrag(DragData*)
{
    notImplemented();
    return DragDestinationActionNone;
}

DragSourceAction DragClientBlackBerry::dragSourceActionMaskForPoint(const IntPoint&)
{
    notImplemented();
    return DragSourceActionNone;
}

void DragClientBlackBerry::startDrag(void*, const IntPoint&, const IntPoint&, Clipboard*, Frame*, bool)
{
    notImplemented();
}

void DragClientBlackBerry::dragControllerDestroyed()
{
    delete this;
}

} // namespace WebCore
