/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "EventHandler.h"

#include "COMPtr.h"
#include "Clipboard.h"
#include "Cursor.h"
#include "FloatPoint.h"
#include "FocusController.h"
#include "FrameView.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "MouseEventWithHitTestResults.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformWheelEvent.h"
#include "Scrollbar.h"
#include "WCDataObject.h"
#include "NotImplemented.h"

namespace WebCore {

#if ENABLE(DRAG_SUPPORT)
const double EventHandler::TextDragDelay = 0.0;
#endif

bool EventHandler::passMousePressEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe)
{
    subframe->eventHandler()->handleMousePressEvent(mev.event());
    return true;
}

bool EventHandler::passMouseMoveEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe, HitTestResult* hoveredNode)
{
#if ENABLE(DRAG_SUPPORT)
    if (m_mouseDownMayStartDrag && !m_mouseDownWasInSubframe)
        return false;
#endif
    subframe->eventHandler()->handleMouseMoveEvent(mev.event(), hoveredNode);
    return true;
}

bool EventHandler::passMouseReleaseEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe)
{
    subframe->eventHandler()->handleMouseReleaseEvent(mev.event());
    return true;
}

bool EventHandler::passWheelEventToWidget(const PlatformWheelEvent& wheelEvent, Widget* widget)
{
    if (!widget->isFrameView())
        return false;

    return toFrameView(widget)->frame()->eventHandler()->handleWheelEvent(wheelEvent);
}

bool EventHandler::tabsToAllFormControls(KeyboardEvent*) const
{
    return true;
}

bool EventHandler::eventActivatedView(const PlatformMouseEvent& event) const
{
    return event.didActivateWebView();
}

#if ENABLE(DRAG_SUPPORT)
PassRefPtr<Clipboard> EventHandler::createDraggingClipboard() const
{
#if OS(WINCE)
    return 0;
#else
    return Clipboard::createForDragAndDrop();
#endif
}
#endif

void EventHandler::focusDocumentView()
{
    Page* page = m_frame->page();
    if (!page)
        return;
    page->focusController()->setFocusedFrame(m_frame);
}

bool EventHandler::passWidgetMouseDownEventToWidget(const MouseEventWithHitTestResults&)
{
    notImplemented();
    return false;
}

unsigned EventHandler::accessKeyModifiers()
{
    return PlatformEvent::AltKey;
}

}
