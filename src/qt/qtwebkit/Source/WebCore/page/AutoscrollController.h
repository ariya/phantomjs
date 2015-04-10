/*
 * Copyright (C) 2006, 2007, 2009, 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef AutoscrollController_h
#define AutoscrollController_h

#include "IntPoint.h"
#include "Timer.h"

namespace WebCore {

class EventHandler;
class Frame;
class FrameView;
class Node;
class PlatformMouseEvent;
class RenderBox;
class RenderObject;

enum AutoscrollType {
    NoAutoscroll,
    AutoscrollForDragAndDrop,
    AutoscrollForSelection,
#if ENABLE(PAN_SCROLLING)
    AutoscrollForPanCanStop,
    AutoscrollForPan,
#endif
};

// AutscrollController handels autoscroll and pan scroll for EventHandler.
class AutoscrollController {
public:
    AutoscrollController();
    RenderBox* autoscrollRenderer() const;
    bool autoscrollInProgress() const;
    bool panScrollInProgress() const;
    void startAutoscrollForSelection(RenderObject*);
    void stopAutoscrollTimer(bool rendererIsBeingDestroyed = false);
    void updateAutoscrollRenderer();
    void updateDragAndDrop(Node* targetNode, const IntPoint& eventPosition, double eventTime);
#if ENABLE(PAN_SCROLLING)
    void didPanScrollStart();
    void didPanScrollStop();
    void handleMouseReleaseEvent(const PlatformMouseEvent&);
    void setPanScrollInProgress(bool);
    void startPanScrolling(RenderBox*, const IntPoint&);
#endif

private:
    void autoscrollTimerFired(Timer<AutoscrollController>*);
    void startAutoscrollTimer();
#if ENABLE(PAN_SCROLLING)
    void updatePanScrollState(FrameView*, const IntPoint&);
#endif

    Timer<AutoscrollController> m_autoscrollTimer;
    RenderBox* m_autoscrollRenderer;
    AutoscrollType m_autoscrollType;
    IntPoint m_dragAndDropAutoscrollReferencePosition;
    double m_dragAndDropAutoscrollStartTime;
#if ENABLE(PAN_SCROLLING)
    IntPoint m_panScrollStartPos;
#endif
};

} // namespace WebCore

#endif // AutoscrollController_h
