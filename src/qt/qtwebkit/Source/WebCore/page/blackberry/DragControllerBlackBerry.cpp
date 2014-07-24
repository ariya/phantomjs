/*
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
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
#include "DragController.h"

#if ENABLE(DRAG_SUPPORT)
#include "DragData.h"
#include "EventHandler.h"

#include "NotImplemented.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

const double EventHandler::TextDragDelay = 0;
const int DragController::LinkDragBorderInset = 2;
const int DragController::MaxOriginalImageArea = 1500 * 1500;
const int DragController::DragIconRightInset = 7;
const int DragController::DragIconBottomInset = 3;
const float DragController::DragImageAlpha = 0.75f;

bool DragController::isCopyKeyDown(DragData*)
{
    notImplemented();
    return false;
}

const IntSize& DragController::maxDragImageSize()
{
    notImplemented();
    // FIXME: Remove this when no stubs use it anymore.
    DEFINE_STATIC_LOCAL(const WebCore::IntSize, nullSize, ());
    return nullSize;
}

void DragController::cleanupAfterSystemDrag()
{
    notImplemented();
}

DragOperation DragController::dragOperation(DragData*)
{
    notImplemented();
    return DragOperationNone;
}
} // namespace WebCore
#endif // ENABLE(DRAG_SUPPORT)
