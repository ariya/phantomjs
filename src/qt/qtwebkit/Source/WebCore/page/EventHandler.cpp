/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2012 Digia Plc. and/or its subsidiary(-ies)
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

#include "AXObjectCache.h"
#include "AutoscrollController.h"
#include "CachedImage.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Cursor.h"
#include "CursorList.h"
#include "Document.h"
#include "DocumentEventQueue.h"
#include "DragController.h"
#include "DragState.h"
#include "Editor.h"
#include "EditorClient.h"
#include "EventNames.h"
#include "EventPathWalker.h"
#include "ExceptionCodePlaceholder.h"
#include "FloatPoint.h"
#include "FloatRect.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameSelection.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "htmlediting.h"
#include "HTMLFrameElementBase.h"
#include "HTMLFrameSetElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "Image.h"
#include "InspectorInstrumentation.h"
#include "KeyboardEvent.h"
#include "MouseEvent.h"
#include "MouseEventWithHitTestResults.h"
#include "Page.h"
#include "PlatformEvent.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformWheelEvent.h"
#include "PluginDocument.h"
#include "RenderFrameSet.h"
#include "RenderLayer.h"
#include "RenderTextControlSingleLine.h"
#include "RenderView.h"
#include "RenderWidget.h"
#include "ScrollAnimator.h"
#include "Scrollbar.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "SpatialNavigation.h"
#include "StyleCachedImage.h"
#include "TextEvent.h"
#include "TextIterator.h"
#include "UserGestureIndicator.h"
#include "UserTypingGestureIndicator.h"
#include "WheelEvent.h"
#include "WindowsKeyboardCodes.h"
#include <wtf/Assertions.h>
#include <wtf/CurrentTime.h>
#include <wtf/StdLibExtras.h>
#include <wtf/TemporaryChange.h>

#if ENABLE(GESTURE_EVENTS)
#include "PlatformGestureEvent.h"
#endif

#if ENABLE(TOUCH_ADJUSTMENT)
#include "TouchAdjustment.h"
#endif

#if ENABLE(SVG)
#include "SVGDocument.h"
#include "SVGElementInstance.h"
#include "SVGNames.h"
#include "SVGUseElement.h"
#endif

#if ENABLE(TOUCH_EVENTS)
#include "PlatformTouchEvent.h"
#include "TouchEvent.h"
#include "TouchList.h"
#endif

#if ENABLE(CSS_IMAGE_SET)
#include "StyleCachedImageSet.h"
#endif

namespace WebCore {

using namespace HTMLNames;

#if ENABLE(DRAG_SUPPORT)
// The link drag hysteresis is much larger than the others because there
// needs to be enough space to cancel the link press without starting a link drag,
// and because dragging links is rare.
const int LinkDragHysteresis = 40;
const int ImageDragHysteresis = 5;
const int TextDragHysteresis = 3;
const int GeneralDragHysteresis = 3;
#endif // ENABLE(DRAG_SUPPORT)

// Match key code of composition keydown event on windows.
// IE sends VK_PROCESSKEY which has value 229;
const int CompositionEventKeyCode = 229;

#if ENABLE(SVG)
using namespace SVGNames;
#endif

// The amount of time to wait before sending a fake mouse event, triggered
// during a scroll. The short interval is used if the content responds to the mouse events quickly enough,
// otherwise the long interval is used.
const double fakeMouseMoveShortInterval = 0.1;
const double fakeMouseMoveLongInterval = 0.250;

// The amount of time to wait for a cursor update on style and layout changes
// Set to 50Hz, no need to be faster than common screen refresh rate
const double cursorUpdateInterval = 0.02;

const int maximumCursorSize = 128;
#if ENABLE(MOUSE_CURSOR_SCALE)
// It's pretty unlikely that a scale of less than one would ever be used. But all we really
// need to ensure here is that the scale isn't so small that integer overflow can occur when
// dividing cursor sizes (limited above) by the scale.
const double minimumCursorScale = 0.001;
#endif

enum NoCursorChangeType { NoCursorChange };

class OptionalCursor {
public:
    OptionalCursor(NoCursorChangeType) : m_isCursorChange(false) { }
    OptionalCursor(const Cursor& cursor) : m_isCursorChange(true), m_cursor(cursor) { }

    bool isCursorChange() const { return m_isCursorChange; }
    const Cursor& cursor() const { ASSERT(m_isCursorChange); return m_cursor; }

private:
    bool m_isCursorChange;
    Cursor m_cursor;
};

class MaximumDurationTracker {
public:
    explicit MaximumDurationTracker(double *maxDuration)
        : m_maxDuration(maxDuration)
        , m_start(monotonicallyIncreasingTime())
    {
    }

    ~MaximumDurationTracker()
    {
        *m_maxDuration = max(*m_maxDuration, monotonicallyIncreasingTime() - m_start);
    }

private:
    double* m_maxDuration;
    double m_start;
};

#if ENABLE(TOUCH_EVENTS)
class SyntheticTouchPoint : public PlatformTouchPoint {
public:

    // The default values are based on http://dvcs.w3.org/hg/webevents/raw-file/tip/touchevents.html
    explicit SyntheticTouchPoint(const PlatformMouseEvent& event)
    {
        const static int idDefaultValue = 0;
        const static int radiusYDefaultValue = 1;
        const static int radiusXDefaultValue = 1;
        const static float rotationAngleDefaultValue = 0.0f;
        const static float forceDefaultValue = 1.0f;

        m_id = idDefaultValue; // There is only one active TouchPoint.
        m_screenPos = event.globalPosition();
        m_pos = event.position();
        m_radiusY = radiusYDefaultValue;
        m_radiusX = radiusXDefaultValue;
        m_rotationAngle = rotationAngleDefaultValue;
        m_force = forceDefaultValue;

        PlatformEvent::Type type = event.type();
        ASSERT(type == PlatformEvent::MouseMoved || type == PlatformEvent::MousePressed || type == PlatformEvent::MouseReleased);

        switch (type) {
        case PlatformEvent::MouseMoved:
            m_state = TouchMoved;
            break;
        case PlatformEvent::MousePressed:
            m_state = TouchPressed;
            break;
        case PlatformEvent::MouseReleased:
            m_state = TouchReleased;
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }
};

class SyntheticSingleTouchEvent : public PlatformTouchEvent {
public:
    explicit SyntheticSingleTouchEvent(const PlatformMouseEvent& event)
    {
        switch (event.type()) {
        case PlatformEvent::MouseMoved:
            m_type = TouchMove;
            break;
        case PlatformEvent::MousePressed:
            m_type = TouchStart;
            break;
        case PlatformEvent::MouseReleased:
            m_type = TouchEnd;
            break;
        default:
            ASSERT_NOT_REACHED();
            m_type = NoType;
            break;
        }
        m_timestamp = event.timestamp();
        m_modifiers = event.modifiers();
        m_touchPoints.append(SyntheticTouchPoint(event));
    }
};
#endif

static inline ScrollGranularity wheelGranularityToScrollGranularity(unsigned deltaMode)
{
    switch (deltaMode) {
    case WheelEvent::DOM_DELTA_PAGE:
        return ScrollByPage;
    case WheelEvent::DOM_DELTA_LINE:
        return ScrollByLine;
    case WheelEvent::DOM_DELTA_PIXEL:
        return ScrollByPixel;
    default:
        return ScrollByPixel;
    }
}

static inline bool scrollNode(float delta, ScrollGranularity granularity, ScrollDirection positiveDirection, ScrollDirection negativeDirection, Node* node, Node** stopNode)
{
    if (!delta)
        return false;
    if (!node->renderer())
        return false;
    RenderBox* enclosingBox = node->renderer()->enclosingBox();
    float absDelta = delta > 0 ? delta : -delta;
    return enclosingBox->scroll(delta < 0 ? negativeDirection : positiveDirection, granularity, absDelta, stopNode);
}

static inline bool shouldGesturesTriggerActive()
{
    // If the platform we're on supports GestureTapDown and GestureTapCancel then we'll
    // rely on them to set the active state. Unfortunately there's no generic way to
    // know in advance what event types are supported.
    return false;
}

#if !PLATFORM(MAC)

inline bool EventHandler::eventLoopHandleMouseUp(const MouseEventWithHitTestResults&)
{
    return false;
}

#if ENABLE(DRAG_SUPPORT)
inline bool EventHandler::eventLoopHandleMouseDragged(const MouseEventWithHitTestResults&)
{
    return false;
}
#endif

#endif

EventHandler::EventHandler(Frame* frame)
    : m_frame(frame)
    , m_mousePressed(false)
    , m_capturesDragging(false)
    , m_mouseDownMayStartSelect(false)
#if ENABLE(DRAG_SUPPORT)
    , m_mouseDownMayStartDrag(false)
    , m_dragMayStartSelectionInstead(false)
#endif
    , m_mouseDownWasSingleClickInSelection(false)
    , m_selectionInitiationState(HaveNotStartedSelection)
    , m_hoverTimer(this, &EventHandler::hoverTimerFired)
    , m_cursorUpdateTimer(this, &EventHandler::cursorUpdateTimerFired)
    , m_autoscrollController(adoptPtr(new AutoscrollController))
    , m_mouseDownMayStartAutoscroll(false)
    , m_mouseDownWasInSubframe(false)
    , m_fakeMouseMoveEventTimer(this, &EventHandler::fakeMouseMoveEventTimerFired)
#if ENABLE(SVG)
    , m_svgPan(false)
#endif
    , m_resizeLayer(0)
    , m_eventHandlerWillResetCapturingMouseEventsNode(0)
    , m_clickCount(0)
    , m_mousePositionIsUnknown(true)
    , m_mouseDownTimestamp(0)
    , m_widgetIsLatched(false)
#if PLATFORM(MAC)
    , m_mouseDownView(nil)
    , m_sendingEventToSubview(false)
    , m_activationEventNumber(-1)
#endif
#if ENABLE(TOUCH_EVENTS)
    , m_originatingTouchPointTargetKey(0)
    , m_touchPressed(false)
#endif
#if ENABLE(GESTURE_EVENTS)
    , m_scrollGestureHandlingNode(0)
    , m_lastHitTestResultOverWidget(false)
#endif
    , m_maxMouseMovedDuration(0)
    , m_baseEventType(PlatformEvent::NoType)
    , m_didStartDrag(false)
    , m_didLongPressInvokeContextMenu(false)
    , m_isHandlingWheelEvent(false)
#if ENABLE(CURSOR_VISIBILITY)
    , m_autoHideCursorTimer(this, &EventHandler::autoHideCursorTimerFired)
#endif
{
}

EventHandler::~EventHandler()
{
    ASSERT(!m_fakeMouseMoveEventTimer.isActive());
#if ENABLE(CURSOR_VISIBILITY)
    ASSERT(!m_autoHideCursorTimer.isActive());
#endif
}
    
#if ENABLE(DRAG_SUPPORT)
DragState& EventHandler::dragState()
{
    DEFINE_STATIC_LOCAL(DragState, state, ());
    return state;
}
#endif // ENABLE(DRAG_SUPPORT)
    
void EventHandler::clear()
{
    m_hoverTimer.stop();
    m_cursorUpdateTimer.stop();
    m_fakeMouseMoveEventTimer.stop();
#if ENABLE(CURSOR_VISIBILITY)
    cancelAutoHideCursorTimer();
#endif
    m_resizeLayer = 0;
    m_nodeUnderMouse = 0;
    m_lastNodeUnderMouse = 0;
#if ENABLE(SVG)
    m_instanceUnderMouse = 0;
    m_lastInstanceUnderMouse = 0;
#endif
    m_lastMouseMoveEventSubframe = 0;
    m_lastScrollbarUnderMouse = 0;
    m_clickCount = 0;
    m_clickNode = 0;
    m_frameSetBeingResized = 0;
#if ENABLE(DRAG_SUPPORT)
    m_dragTarget = 0;
    m_shouldOnlyFireDragOverEvent = false;
#endif
    m_mousePositionIsUnknown = true;
    m_lastKnownMousePosition = IntPoint();
    m_lastKnownMouseGlobalPosition = IntPoint();
    m_mousePressNode = 0;
    m_mousePressed = false;
    m_capturesDragging = false;
    m_capturingMouseEventsNode = 0;
    m_latchedWheelEventNode = 0;
    m_previousWheelScrolledNode = 0;
#if ENABLE(TOUCH_EVENTS)
    m_originatingTouchPointTargets.clear();
    m_originatingTouchPointDocument.clear();
    m_originatingTouchPointTargetKey = 0;
#endif
#if ENABLE(GESTURE_EVENTS)
    m_scrollGestureHandlingNode = 0;
    m_lastHitTestResultOverWidget = false;
    m_previousGestureScrolledNode = 0;
    m_scrollbarHandlingScrollGesture = 0;
#endif
    m_maxMouseMovedDuration = 0;
    m_baseEventType = PlatformEvent::NoType;
    m_didStartDrag = false;
    m_didLongPressInvokeContextMenu = false;
}

void EventHandler::nodeWillBeRemoved(Node* nodeToBeRemoved)
{
    if (nodeToBeRemoved->contains(m_clickNode.get()))
        m_clickNode = 0;
}

static void setSelectionIfNeeded(FrameSelection* selection, const VisibleSelection& newSelection)
{
    ASSERT(selection);
    if (selection->selection() != newSelection && selection->shouldChangeSelection(newSelection))
        selection->setSelection(newSelection);
}

static inline bool dispatchSelectStart(Node* node)
{
    if (!node || !node->renderer())
        return true;

    return node->dispatchEvent(Event::create(eventNames().selectstartEvent, true, true));
}

static VisibleSelection expandSelectionToRespectUserSelectAll(Node* targetNode, const VisibleSelection& selection)
{
#if ENABLE(USERSELECT_ALL)
    Node* rootUserSelectAll = Position::rootUserSelectAllForNode(targetNode);
    if (!rootUserSelectAll)
        return selection;

    VisibleSelection newSelection(selection);
    newSelection.setBase(positionBeforeNode(rootUserSelectAll).upstream(CanCrossEditingBoundary));
    newSelection.setExtent(positionAfterNode(rootUserSelectAll).downstream(CanCrossEditingBoundary));

    return newSelection;
#else
    UNUSED_PARAM(targetNode);
    return selection;
#endif
}

bool EventHandler::updateSelectionForMouseDownDispatchingSelectStart(Node* targetNode, const VisibleSelection& selection, TextGranularity granularity)
{
    if (Position::nodeIsUserSelectNone(targetNode))
        return false;

    if (!dispatchSelectStart(targetNode))
        return false;

    if (selection.isRange())
        m_selectionInitiationState = ExtendedSelection;
    else {
        granularity = CharacterGranularity;
        m_selectionInitiationState = PlacedCaret;
    }

    m_frame->selection()->setNonDirectionalSelectionIfNeeded(selection, granularity);

    return true;
}

void EventHandler::selectClosestWordFromHitTestResult(const HitTestResult& result, AppendTrailingWhitespace appendTrailingWhitespace)
{
    Node* innerNode = result.targetNode();
    VisibleSelection newSelection;

    if (innerNode && innerNode->renderer()) {
        VisiblePosition pos(innerNode->renderer()->positionForPoint(result.localPoint()));
        if (pos.isNotNull()) {
            newSelection = VisibleSelection(pos);
            newSelection.expandUsingGranularity(WordGranularity);
        }

        if (appendTrailingWhitespace == ShouldAppendTrailingWhitespace && newSelection.isRange())
            newSelection.appendTrailingWhitespace();

        updateSelectionForMouseDownDispatchingSelectStart(innerNode, expandSelectionToRespectUserSelectAll(innerNode, newSelection), WordGranularity);
    }
}

void EventHandler::selectClosestWordFromMouseEvent(const MouseEventWithHitTestResults& result)
{
    if (m_mouseDownMayStartSelect) {
        selectClosestWordFromHitTestResult(result.hitTestResult(),
            (result.event().clickCount() == 2 && m_frame->editor().isSelectTrailingWhitespaceEnabled()) ? ShouldAppendTrailingWhitespace : DontAppendTrailingWhitespace);
    }
}

void EventHandler::selectClosestWordOrLinkFromMouseEvent(const MouseEventWithHitTestResults& result)
{
    if (!result.hitTestResult().isLiveLink())
        return selectClosestWordFromMouseEvent(result);

    Node* innerNode = result.targetNode();

    if (innerNode && innerNode->renderer() && m_mouseDownMayStartSelect) {
        VisibleSelection newSelection;
        Element* URLElement = result.hitTestResult().URLElement();
        VisiblePosition pos(innerNode->renderer()->positionForPoint(result.localPoint()));
        if (pos.isNotNull() && pos.deepEquivalent().deprecatedNode()->isDescendantOf(URLElement))
            newSelection = VisibleSelection::selectionFromContentsOfNode(URLElement);

        updateSelectionForMouseDownDispatchingSelectStart(innerNode, expandSelectionToRespectUserSelectAll(innerNode, newSelection), WordGranularity);
    }
}

bool EventHandler::handleMousePressEventDoubleClick(const MouseEventWithHitTestResults& event)
{
    if (event.event().button() != LeftButton)
        return false;

    if (m_frame->selection()->isRange())
        // A double-click when range is already selected
        // should not change the selection.  So, do not call
        // selectClosestWordFromMouseEvent, but do set
        // m_beganSelectingText to prevent handleMouseReleaseEvent
        // from setting caret selection.
        m_selectionInitiationState = ExtendedSelection;
    else
        selectClosestWordFromMouseEvent(event);

    return true;
}

bool EventHandler::handleMousePressEventTripleClick(const MouseEventWithHitTestResults& event)
{
    if (event.event().button() != LeftButton)
        return false;
    
    Node* innerNode = event.targetNode();
    if (!(innerNode && innerNode->renderer() && m_mouseDownMayStartSelect))
        return false;

    VisibleSelection newSelection;
    VisiblePosition pos(innerNode->renderer()->positionForPoint(event.localPoint()));
    if (pos.isNotNull()) {
        newSelection = VisibleSelection(pos);
        newSelection.expandUsingGranularity(ParagraphGranularity);
    }

    return updateSelectionForMouseDownDispatchingSelectStart(innerNode, expandSelectionToRespectUserSelectAll(innerNode, newSelection), ParagraphGranularity);
}

static int textDistance(const Position& start, const Position& end)
{
     RefPtr<Range> range = Range::create(start.anchorNode()->document(), start, end);
     return TextIterator::rangeLength(range.get(), true);
}

bool EventHandler::handleMousePressEventSingleClick(const MouseEventWithHitTestResults& event)
{
    m_frame->document()->updateLayoutIgnorePendingStylesheets();
    Node* innerNode = event.targetNode();
    if (!(innerNode && innerNode->renderer() && m_mouseDownMayStartSelect))
        return false;

    // Extend the selection if the Shift key is down, unless the click is in a link.
    bool extendSelection = event.event().shiftKey() && !event.isOverLink();

    // Don't restart the selection when the mouse is pressed on an
    // existing selection so we can allow for text dragging.
    if (FrameView* view = m_frame->view()) {
        LayoutPoint vPoint = view->windowToContents(event.event().position());
        if (!extendSelection && m_frame->selection()->contains(vPoint)) {
            m_mouseDownWasSingleClickInSelection = true;
            return false;
        }
    }

    VisiblePosition visiblePos(innerNode->renderer()->positionForPoint(event.localPoint()));
    if (visiblePos.isNull())
        visiblePos = VisiblePosition(firstPositionInOrBeforeNode(innerNode), DOWNSTREAM);
    Position pos = visiblePos.deepEquivalent();

    VisibleSelection newSelection = m_frame->selection()->selection();
    TextGranularity granularity = CharacterGranularity;

    if (extendSelection && newSelection.isCaretOrRange()) {
        VisibleSelection selectionInUserSelectAll = expandSelectionToRespectUserSelectAll(innerNode, VisibleSelection(pos));
        if (selectionInUserSelectAll.isRange()) {
            if (comparePositions(selectionInUserSelectAll.start(), newSelection.start()) < 0)
                pos = selectionInUserSelectAll.start();
            else if (comparePositions(newSelection.end(), selectionInUserSelectAll.end()) < 0)
                pos = selectionInUserSelectAll.end();
        }

        if (!m_frame->editor().behavior().shouldConsiderSelectionAsDirectional() && pos.isNotNull()) {
            // See <rdar://problem/3668157> REGRESSION (Mail): shift-click deselects when selection
            // was created right-to-left
            Position start = newSelection.start();
            Position end = newSelection.end();
            int distanceToStart = textDistance(start, pos);
            int distanceToEnd = textDistance(pos, end);
            if (distanceToStart <= distanceToEnd)
                newSelection = VisibleSelection(end, pos);
            else
                newSelection = VisibleSelection(start, pos);
        } else
            newSelection.setExtent(pos);

        if (m_frame->selection()->granularity() != CharacterGranularity) {
            granularity = m_frame->selection()->granularity();
            newSelection.expandUsingGranularity(m_frame->selection()->granularity());
        }
    } else
        newSelection = expandSelectionToRespectUserSelectAll(innerNode, visiblePos);

    bool handled = updateSelectionForMouseDownDispatchingSelectStart(innerNode, newSelection, granularity);

    if (event.event().button() == MiddleButton) {
        // Ignore handled, since we want to paste to where the caret was placed anyway.
        handled = handlePasteGlobalSelection(event.event()) || handled;
    }
    return handled;
}

static inline bool canMouseDownStartSelect(Node* node)
{
    if (!node || !node->renderer())
        return true;

    return node->canStartSelection() || Position::nodeIsUserSelectAll(node);
}

bool EventHandler::handleMousePressEvent(const MouseEventWithHitTestResults& event)
{
#if ENABLE(DRAG_SUPPORT)
    // Reset drag state.
    dragState().source = 0;
#endif

    cancelFakeMouseMoveEvent();

    m_frame->document()->updateLayoutIgnorePendingStylesheets();

    if (ScrollView* scrollView = m_frame->view()) {
        if (scrollView->isPointInScrollbarCorner(event.event().position()))
            return false;
    }

    bool singleClick = event.event().clickCount() <= 1;

    // If we got the event back, that must mean it wasn't prevented,
    // so it's allowed to start a drag or selection if it wasn't in a scrollbar.
    m_mouseDownMayStartSelect = canMouseDownStartSelect(event.targetNode()) && !event.scrollbar();
    
#if ENABLE(DRAG_SUPPORT)
    // Careful that the drag starting logic stays in sync with eventMayStartDrag()
    m_mouseDownMayStartDrag = singleClick;
#endif

    m_mouseDownWasSingleClickInSelection = false;

    m_mouseDown = event.event();

    if (event.isOverWidget() && passWidgetMouseDownEventToWidget(event))
        return true;

#if ENABLE(SVG)
    if (m_frame->document()->isSVGDocument()
        && toSVGDocument(m_frame->document())->zoomAndPanEnabled()) {
        if (event.event().shiftKey() && singleClick) {
            m_svgPan = true;
            toSVGDocument(m_frame->document())->startPan(m_frame->view()->windowToContents(event.event().position()));
            return true;
        }
    }
#endif

    // We don't do this at the start of mouse down handling,
    // because we don't want to do it until we know we didn't hit a widget.
    if (singleClick)
        focusDocumentView();

    Node* innerNode = event.targetNode();

    m_mousePressNode = innerNode;
#if ENABLE(DRAG_SUPPORT)
    m_dragStartPos = event.event().position();
#endif

    bool swallowEvent = false;
    m_mousePressed = true;
    m_selectionInitiationState = HaveNotStartedSelection;

    if (event.event().clickCount() == 2)
        swallowEvent = handleMousePressEventDoubleClick(event);
    else if (event.event().clickCount() >= 3)
        swallowEvent = handleMousePressEventTripleClick(event);
    else
        swallowEvent = handleMousePressEventSingleClick(event);
    
    m_mouseDownMayStartAutoscroll = m_mouseDownMayStartSelect
        || (m_mousePressNode && m_mousePressNode->renderBox() && m_mousePressNode->renderBox()->canBeProgramaticallyScrolled());

    return swallowEvent;
}

#if ENABLE(DRAG_SUPPORT)
bool EventHandler::handleMouseDraggedEvent(const MouseEventWithHitTestResults& event)
{
    if (!m_mousePressed)
        return false;

    if (handleDrag(event, ShouldCheckDragHysteresis))
        return true;

    Node* targetNode = event.targetNode();
    if (event.event().button() != LeftButton || !targetNode)
        return false;

    RenderObject* renderer = targetNode->renderer();
    if (!renderer) {
        Node* parent = EventPathWalker::parent(targetNode);
        if (!parent)
            return false;

        renderer = parent->renderer();
        if (!renderer || !renderer->isListBox())
            return false;
    }

#if PLATFORM(MAC) // FIXME: Why does this assertion fire on other platforms?
    ASSERT(m_mouseDownMayStartSelect || m_mouseDownMayStartAutoscroll);
#endif

    m_mouseDownMayStartDrag = false;

    if (m_mouseDownMayStartAutoscroll && !panScrollInProgress()) {
        m_autoscrollController->startAutoscrollForSelection(renderer);
        m_mouseDownMayStartAutoscroll = false;
    }

    if (m_selectionInitiationState != ExtendedSelection) {
        HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::DisallowShadowContent);
        HitTestResult result(m_mouseDownPos);
        m_frame->document()->renderView()->hitTest(request, result);

        updateSelectionForMouseDrag(result);
    }
    updateSelectionForMouseDrag(event.hitTestResult());
    return true;
}
    
bool EventHandler::eventMayStartDrag(const PlatformMouseEvent& event) const
{
    // This is a pre-flight check of whether the event might lead to a drag being started.  Be careful
    // that its logic needs to stay in sync with handleMouseMoveEvent() and the way we setMouseDownMayStartDrag
    // in handleMousePressEvent
    
    if (!m_frame->contentRenderer() || !m_frame->contentRenderer()->hasLayer())
        return false;

    if (event.button() != LeftButton || event.clickCount() != 1)
        return false;
    
    FrameView* view = m_frame->view();
    if (!view)
        return false;

    Page* page = m_frame->page();
    if (!page)
        return false;

    updateDragSourceActionsAllowed();
    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::DisallowShadowContent);
    HitTestResult result(view->windowToContents(event.position()));
    m_frame->contentRenderer()->hitTest(request, result);
    DragState state;
    return result.innerElement() && page->dragController()->draggableElement(m_frame, result.innerElement(), result.roundedPointInInnerNodeFrame(), state);
}

void EventHandler::updateSelectionForMouseDrag()
{
    FrameView* view = m_frame->view();
    if (!view)
        return;
    RenderView* renderer = m_frame->contentRenderer();
    if (!renderer)
        return;

    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::Move | HitTestRequest::DisallowShadowContent);
    HitTestResult result(view->windowToContents(m_lastKnownMousePosition));
    renderer->hitTest(request, result);
    updateSelectionForMouseDrag(result);
}

static VisiblePosition selectionExtentRespectingEditingBoundary(const VisibleSelection& selection, const LayoutPoint& localPoint, Node* targetNode)
{
    LayoutPoint selectionEndPoint = localPoint;
    Element* editableElement = selection.rootEditableElement();

    if (!targetNode->renderer())
        return VisiblePosition();

    if (editableElement && !editableElement->contains(targetNode)) {
        if (!editableElement->renderer())
            return VisiblePosition();

        FloatPoint absolutePoint = targetNode->renderer()->localToAbsolute(FloatPoint(selectionEndPoint));
        selectionEndPoint = roundedLayoutPoint(editableElement->renderer()->absoluteToLocal(absolutePoint));
        targetNode = editableElement;
    }

    return targetNode->renderer()->positionForPoint(selectionEndPoint);
}

void EventHandler::updateSelectionForMouseDrag(const HitTestResult& hitTestResult)
{
    if (!m_mouseDownMayStartSelect)
        return;

    Node* target = hitTestResult.targetNode();
    if (!target)
        return;

    VisiblePosition targetPosition = selectionExtentRespectingEditingBoundary(m_frame->selection()->selection(), hitTestResult.localPoint(), target);

    // Don't modify the selection if we're not on a node.
    if (targetPosition.isNull())
        return;

    // Restart the selection if this is the first mouse move. This work is usually
    // done in handleMousePressEvent, but not if the mouse press was on an existing selection.
    VisibleSelection newSelection = m_frame->selection()->selection();

#if ENABLE(SVG)
    // Special case to limit selection to the containing block for SVG text.
    // FIXME: Isn't there a better non-SVG-specific way to do this?
    if (Node* selectionBaseNode = newSelection.base().deprecatedNode())
        if (RenderObject* selectionBaseRenderer = selectionBaseNode->renderer())
            if (selectionBaseRenderer->isSVGText())
                if (target->renderer()->containingBlock() != selectionBaseRenderer->containingBlock())
                    return;
#endif

    if (m_selectionInitiationState == HaveNotStartedSelection && !dispatchSelectStart(target))
        return;

    if (m_selectionInitiationState != ExtendedSelection) {
        // Always extend selection here because it's caused by a mouse drag
        m_selectionInitiationState = ExtendedSelection;
        newSelection = VisibleSelection(targetPosition);
    }

#if ENABLE(USERSELECT_ALL)
    Node* rootUserSelectAllForMousePressNode = Position::rootUserSelectAllForNode(m_mousePressNode.get());
    if (rootUserSelectAllForMousePressNode && rootUserSelectAllForMousePressNode == Position::rootUserSelectAllForNode(target)) {
        newSelection.setBase(positionBeforeNode(rootUserSelectAllForMousePressNode).upstream(CanCrossEditingBoundary));
        newSelection.setExtent(positionAfterNode(rootUserSelectAllForMousePressNode).downstream(CanCrossEditingBoundary));
    } else {
        // Reset base for user select all when base is inside user-select-all area and extent < base.
        if (rootUserSelectAllForMousePressNode && comparePositions(target->renderer()->positionForPoint(hitTestResult.localPoint()), m_mousePressNode->renderer()->positionForPoint(m_dragStartPos)) < 0)
            newSelection.setBase(positionAfterNode(rootUserSelectAllForMousePressNode).downstream(CanCrossEditingBoundary));
        
        Node* rootUserSelectAllForTarget = Position::rootUserSelectAllForNode(target);
        if (rootUserSelectAllForTarget && m_mousePressNode->renderer() && comparePositions(target->renderer()->positionForPoint(hitTestResult.localPoint()), m_mousePressNode->renderer()->positionForPoint(m_dragStartPos)) < 0)
            newSelection.setExtent(positionBeforeNode(rootUserSelectAllForTarget).upstream(CanCrossEditingBoundary));
        else if (rootUserSelectAllForTarget && m_mousePressNode->renderer())
            newSelection.setExtent(positionAfterNode(rootUserSelectAllForTarget).downstream(CanCrossEditingBoundary));
        else
            newSelection.setExtent(targetPosition);
    }
#else
    newSelection.setExtent(targetPosition);
#endif

    if (m_frame->selection()->granularity() != CharacterGranularity)
        newSelection.expandUsingGranularity(m_frame->selection()->granularity());

    m_frame->selection()->setNonDirectionalSelectionIfNeeded(newSelection, m_frame->selection()->granularity(),
        FrameSelection::AdjustEndpointsAtBidiBoundary);
}
#endif // ENABLE(DRAG_SUPPORT)

void EventHandler::lostMouseCapture()
{
    m_frame->selection()->setCaretBlinkingSuspended(false);
}

bool EventHandler::handleMouseUp(const MouseEventWithHitTestResults& event)
{
    if (eventLoopHandleMouseUp(event))
        return true;
    
    // If this was the first click in the window, we don't even want to clear the selection.
    // This case occurs when the user clicks on a draggable element, since we have to process
    // the mouse down and drag events to see if we might start a drag.  For other first clicks
    // in a window, we just don't acceptFirstMouse, and the whole down-drag-up sequence gets
    // ignored upstream of this layer.
    return eventActivatedView(event.event());
}    

bool EventHandler::handleMouseReleaseEvent(const MouseEventWithHitTestResults& event)
{
    if (autoscrollInProgress())
        stopAutoscrollTimer();

    if (handleMouseUp(event))
        return true;

    // Used to prevent mouseMoveEvent from initiating a drag before
    // the mouse is pressed again.
    m_mousePressed = false;
    m_capturesDragging = false;
#if ENABLE(DRAG_SUPPORT)
    m_mouseDownMayStartDrag = false;
#endif
    m_mouseDownMayStartSelect = false;
    m_mouseDownMayStartAutoscroll = false;
    m_mouseDownWasInSubframe = false;
  
    bool handled = false;

    // Clear the selection if the mouse didn't move after the last mouse
    // press and it's not a context menu click.  We do this so when clicking
    // on the selection, the selection goes away.  However, if we are
    // editing, place the caret.
    if (m_mouseDownWasSingleClickInSelection && m_selectionInitiationState != ExtendedSelection
#if ENABLE(DRAG_SUPPORT)
            && m_dragStartPos == event.event().position()
#endif
            && m_frame->selection()->isRange()
            && event.event().button() != RightButton) {
        VisibleSelection newSelection;
        Node* node = event.targetNode();
        bool caretBrowsing = m_frame->settings() && m_frame->settings()->caretBrowsingEnabled();
        if (node && (caretBrowsing || node->rendererIsEditable()) && node->renderer()) {
            VisiblePosition pos = node->renderer()->positionForPoint(event.localPoint());
            newSelection = VisibleSelection(pos);
        }

        setSelectionIfNeeded(m_frame->selection(), newSelection);

        handled = true;
    }

    m_frame->selection()->notifyRendererOfSelectionChange(UserTriggered);

    m_frame->selection()->selectFrameElementInParentIfFullySelected();

    if (event.event().button() == MiddleButton) {
        // Ignore handled, since we want to paste to where the caret was placed anyway.
        handled = handlePasteGlobalSelection(event.event()) || handled;
    }

    return handled;
}

#if ENABLE(PAN_SCROLLING)

void EventHandler::didPanScrollStart()
{
    m_autoscrollController->didPanScrollStart();
}

void EventHandler::didPanScrollStop()
{
    m_autoscrollController->didPanScrollStop();
}

void EventHandler::startPanScrolling(RenderObject* renderer)
{
    if (!renderer->isBox())
        return;
    m_autoscrollController->startPanScrolling(toRenderBox(renderer), lastKnownMousePosition());
    invalidateClick();
}

#endif // ENABLE(PAN_SCROLLING)

RenderObject* EventHandler::autoscrollRenderer() const
{
    return m_autoscrollController->autoscrollRenderer();
}

void EventHandler::updateAutoscrollRenderer()
{
    m_autoscrollController->updateAutoscrollRenderer();
}

bool EventHandler::autoscrollInProgress() const
{
    return m_autoscrollController->autoscrollInProgress();
}

bool EventHandler::panScrollInProgress() const
{
    return m_autoscrollController->panScrollInProgress();
}

#if ENABLE(DRAG_SUPPORT)
DragSourceAction EventHandler::updateDragSourceActionsAllowed() const
{
    if (!m_frame)
        return DragSourceActionNone;

    Page* page = m_frame->page();
    if (!page)
        return DragSourceActionNone;

    FrameView* view = m_frame->view();
    if (!view)
        return DragSourceActionNone;

    return page->dragController()->delegateDragSourceAction(view->contentsToRootView(m_mouseDownPos));
}
#endif // ENABLE(DRAG_SUPPORT)

HitTestResult EventHandler::hitTestResultAtPoint(const LayoutPoint& point, HitTestRequest::HitTestRequestType hitType, const LayoutSize& padding)
{
    // We always send hitTestResultAtPoint to the main frame if we have one,
    // otherwise we might hit areas that are obscured by higher frames.
    if (Page* page = m_frame->page()) {
        Frame* mainFrame = page->mainFrame();
        if (m_frame != mainFrame) {
            FrameView* frameView = m_frame->view();
            FrameView* mainView = mainFrame->view();
            if (frameView && mainView) {
                IntPoint mainFramePoint = mainView->rootViewToContents(frameView->contentsToRootView(roundedIntPoint(point)));
                return mainFrame->eventHandler()->hitTestResultAtPoint(mainFramePoint, hitType, padding);
            }
        }
    }

    HitTestResult result(point, padding.height(), padding.width(), padding.height(), padding.width());

    if (!m_frame->contentRenderer())
        return result;

    // hitTestResultAtPoint is specifically used to hitTest into all frames, thus it always allows child frame content.
    HitTestRequest request(hitType | HitTestRequest::AllowChildFrameContent);
    m_frame->contentRenderer()->hitTest(request, result);
    if (!request.readOnly())
        m_frame->document()->updateHoverActiveState(request, result.innerElement());

    if (request.disallowsShadowContent())
        result.setToNonShadowAncestor();

    return result;
}

void EventHandler::stopAutoscrollTimer(bool rendererIsBeingDestroyed)
{
    m_autoscrollController->stopAutoscrollTimer(rendererIsBeingDestroyed);
}

Node* EventHandler::mousePressNode() const
{
    return m_mousePressNode.get();
}

void EventHandler::setMousePressNode(PassRefPtr<Node> node)
{
    m_mousePressNode = node;
}

bool EventHandler::scrollOverflow(ScrollDirection direction, ScrollGranularity granularity, Node* startingNode)
{
    Node* node = startingNode;

    if (!node)
        node = m_frame->document()->focusedElement();

    if (!node)
        node = m_mousePressNode.get();
    
    if (node) {
        RenderObject* r = node->renderer();
        if (r && !r->isListBox() && r->enclosingBox()->scroll(direction, granularity)) {
            setFrameWasScrolledByUser();
            return true;
        }
    }

    return false;
}

bool EventHandler::logicalScrollOverflow(ScrollLogicalDirection direction, ScrollGranularity granularity, Node* startingNode)
{
    Node* node = startingNode;

    if (!node)
        node = m_frame->document()->focusedElement();

    if (!node)
        node = m_mousePressNode.get();
    
    if (node) {
        RenderObject* r = node->renderer();
        if (r && !r->isListBox() && r->enclosingBox()->logicalScroll(direction, granularity)) {
            setFrameWasScrolledByUser();
            return true;
        }
    }

    return false;
}

bool EventHandler::scrollRecursively(ScrollDirection direction, ScrollGranularity granularity, Node* startingNode)
{
    // The layout needs to be up to date to determine if we can scroll. We may be
    // here because of an onLoad event, in which case the final layout hasn't been performed yet.
    m_frame->document()->updateLayoutIgnorePendingStylesheets();
    if (scrollOverflow(direction, granularity, startingNode))
        return true;    
    Frame* frame = m_frame;
    FrameView* view = frame->view();
    if (view && view->scroll(direction, granularity))
        return true;
    frame = frame->tree()->parent();
    if (!frame)
        return false;
    return frame->eventHandler()->scrollRecursively(direction, granularity, m_frame->ownerElement());
}

bool EventHandler::logicalScrollRecursively(ScrollLogicalDirection direction, ScrollGranularity granularity, Node* startingNode)
{
    // The layout needs to be up to date to determine if we can scroll. We may be
    // here because of an onLoad event, in which case the final layout hasn't been performed yet.
    m_frame->document()->updateLayoutIgnorePendingStylesheets();
    if (logicalScrollOverflow(direction, granularity, startingNode))
        return true;    
    Frame* frame = m_frame;
    FrameView* view = frame->view();
    
    bool scrolled = false;
#if PLATFORM(MAC)
    // Mac also resets the scroll position in the inline direction.
    if (granularity == ScrollByDocument && view && view->logicalScroll(ScrollInlineDirectionBackward, ScrollByDocument))
        scrolled = true;
#endif
    if (view && view->logicalScroll(direction, granularity))
        scrolled = true;
    
    if (scrolled)
        return true;
    
    frame = frame->tree()->parent();
    if (!frame)
        return false;

    return frame->eventHandler()->logicalScrollRecursively(direction, granularity, m_frame->ownerElement());
}

IntPoint EventHandler::lastKnownMousePosition() const
{
    return m_lastKnownMousePosition;
}

Frame* EventHandler::subframeForHitTestResult(const MouseEventWithHitTestResults& hitTestResult)
{
    if (!hitTestResult.isOverWidget())
        return 0;
    return subframeForTargetNode(hitTestResult.targetNode());
}

Frame* EventHandler::subframeForTargetNode(Node* node)
{
    if (!node)
        return 0;

    RenderObject* renderer = node->renderer();
    if (!renderer || !renderer->isWidget())
        return 0;

    Widget* widget = toRenderWidget(renderer)->widget();
    if (!widget || !widget->isFrameView())
        return 0;

    return toFrameView(widget)->frame();
}

static bool isSubmitImage(Node* node)
{
    return node && isHTMLInputElement(node) && toHTMLInputElement(node)->isImageButton();
}

// Returns true if the node's editable block is not current focused for editing
static bool nodeIsNotBeingEdited(Node* node, Frame* frame)
{
    return frame->selection()->rootEditableElement() != node->rootEditableElement();
}

bool EventHandler::useHandCursor(Node* node, bool isOverLink, bool shiftKey)
{
    if (!node)
        return false;

    bool editable = node->rendererIsEditable();
    bool editableLinkEnabled = false;

    // If the link is editable, then we need to check the settings to see whether or not the link should be followed
    if (editable) {
        ASSERT(m_frame->settings());
        switch (m_frame->settings()->editableLinkBehavior()) {
        default:
        case EditableLinkDefaultBehavior:
        case EditableLinkAlwaysLive:
            editableLinkEnabled = true;
            break;

        case EditableLinkNeverLive:
            editableLinkEnabled = false;
            break;

        case EditableLinkLiveWhenNotFocused:
            editableLinkEnabled = nodeIsNotBeingEdited(node, m_frame) || shiftKey;
            break;

        case EditableLinkOnlyLiveWithShiftKey:
            editableLinkEnabled = shiftKey;
            break;
        }
    }

    return ((isOverLink || isSubmitImage(node)) && (!editable || editableLinkEnabled));
}

void EventHandler::cursorUpdateTimerFired(Timer<EventHandler>*)
{
    ASSERT(m_frame);
    ASSERT(m_frame->document());

    updateCursor();
}

void EventHandler::updateCursor()
{
    if (m_mousePositionIsUnknown)
        return;

    FrameView* view = m_frame->view();
    if (!view)
        return;

    RenderView* renderView = view->renderView();
    if (!renderView)
        return;

    if (!view->shouldSetCursor())
        return;

    bool shiftKey;
    bool ctrlKey;
    bool altKey;
    bool metaKey;
    PlatformKeyboardEvent::getCurrentModifierState(shiftKey, ctrlKey, altKey, metaKey);

    m_frame->document()->updateLayout();

    HitTestRequest request(HitTestRequest::ReadOnly);
    HitTestResult result(view->windowToContents(m_lastKnownMousePosition));
    renderView->hitTest(request, result);

    OptionalCursor optionalCursor = selectCursor(result, shiftKey);
    if (optionalCursor.isCursorChange()) {
        m_currentMouseCursor = optionalCursor.cursor();
        view->setCursor(m_currentMouseCursor);
    }
}

OptionalCursor EventHandler::selectCursor(const HitTestResult& result, bool shiftKey)
{
    if (m_resizeLayer && m_resizeLayer->inResizeMode())
        return NoCursorChange;

    Page* page = m_frame->page();
    if (!page)
        return NoCursorChange;
#if ENABLE(PAN_SCROLLING)
    if (page->mainFrame()->eventHandler()->panScrollInProgress())
        return NoCursorChange;
#endif

    Node* node = result.targetNode();
    if (!node)
        return NoCursorChange;

    RenderObject* renderer = node->renderer();
    RenderStyle* style = renderer ? renderer->style() : 0;
    bool horizontalText = !style || style->isHorizontalWritingMode();
    const Cursor& iBeam = horizontalText ? iBeamCursor() : verticalTextCursor();

#if ENABLE(CURSOR_VISIBILITY)
    if (style && style->cursorVisibility() == CursorVisibilityAutoHide) {
        FeatureObserver::observe(m_frame->document(), FeatureObserver::CursorVisibility);
        startAutoHideCursorTimer();
    } else
        cancelAutoHideCursorTimer();
#endif

    // During selection, use an I-beam no matter what we're over.
    // If a drag may be starting or we're capturing mouse events for a particular node, don't treat this as a selection.
    if (m_mousePressed && m_mouseDownMayStartSelect
#if ENABLE(DRAG_SUPPORT)
        && !m_mouseDownMayStartDrag
#endif
        && m_frame->selection()->isCaretOrRange() && !m_capturingMouseEventsNode)
        return iBeam;

    if (renderer) {
        Cursor overrideCursor;
        switch (renderer->getCursor(roundedIntPoint(result.localPoint()), overrideCursor)) {
        case SetCursorBasedOnStyle:
            break;
        case SetCursor:
            return overrideCursor;
        case DoNotSetCursor:
            return NoCursorChange;
        }
    }

    if (style && style->cursors()) {
        const CursorList* cursors = style->cursors();
        for (unsigned i = 0; i < cursors->size(); ++i) {
            StyleImage* styleImage = (*cursors)[i].image();
            if (!styleImage)
                continue;
            CachedImage* cachedImage = styleImage->cachedImage();
            if (!cachedImage)
                continue;
            float scale = styleImage->imageScaleFactor();
            // Get hotspot and convert from logical pixels to physical pixels.
            IntPoint hotSpot = (*cursors)[i].hotSpot();
            hotSpot.scale(scale, scale);
            IntSize size = cachedImage->imageForRenderer(renderer)->size();
            if (cachedImage->errorOccurred())
                continue;
            // Limit the size of cursors (in UI pixels) so that they cannot be
            // used to cover UI elements in chrome.
            size.scale(1 / scale);
            if (size.width() > maximumCursorSize || size.height() > maximumCursorSize)
                continue;

            Image* image = cachedImage->imageForRenderer(renderer);
#if ENABLE(MOUSE_CURSOR_SCALE)
            // Ensure no overflow possible in calculations above.
            if (scale < minimumCursorScale)
                continue;
            return Cursor(image, hotSpot, scale);
#else
            ASSERT(scale == 1);
            return Cursor(image, hotSpot);
#endif // ENABLE(MOUSE_CURSOR_SCALE)
        }
    }

    switch (style ? style->cursor() : CURSOR_AUTO) {
    case CURSOR_AUTO: {
        bool editable = node->rendererIsEditable();

        if (useHandCursor(node, result.isOverLink(), shiftKey))
            return handCursor();

        bool inResizer = false;
        if (renderer) {
            if (RenderLayer* layer = renderer->enclosingLayer()) {
                if (FrameView* view = m_frame->view())
                    inResizer = layer->isPointInResizeControl(view->windowToContents(roundedIntPoint(result.localPoint())));
            }
        }
        if ((editable || (renderer && renderer->isText() && node->canStartSelection())) && !inResizer && !result.scrollbar())
            return iBeam;
        return pointerCursor();
    }
    case CURSOR_CROSS:
        return crossCursor();
    case CURSOR_POINTER:
        return handCursor();
    case CURSOR_MOVE:
        return moveCursor();
    case CURSOR_ALL_SCROLL:
        return moveCursor();
    case CURSOR_E_RESIZE:
        return eastResizeCursor();
    case CURSOR_W_RESIZE:
        return westResizeCursor();
    case CURSOR_N_RESIZE:
        return northResizeCursor();
    case CURSOR_S_RESIZE:
        return southResizeCursor();
    case CURSOR_NE_RESIZE:
        return northEastResizeCursor();
    case CURSOR_SW_RESIZE:
        return southWestResizeCursor();
    case CURSOR_NW_RESIZE:
        return northWestResizeCursor();
    case CURSOR_SE_RESIZE:
        return southEastResizeCursor();
    case CURSOR_NS_RESIZE:
        return northSouthResizeCursor();
    case CURSOR_EW_RESIZE:
        return eastWestResizeCursor();
    case CURSOR_NESW_RESIZE:
        return northEastSouthWestResizeCursor();
    case CURSOR_NWSE_RESIZE:
        return northWestSouthEastResizeCursor();
    case CURSOR_COL_RESIZE:
        return columnResizeCursor();
    case CURSOR_ROW_RESIZE:
        return rowResizeCursor();
    case CURSOR_TEXT:
        return iBeamCursor();
    case CURSOR_WAIT:
        return waitCursor();
    case CURSOR_HELP:
        return helpCursor();
    case CURSOR_VERTICAL_TEXT:
        return verticalTextCursor();
    case CURSOR_CELL:
        return cellCursor();
    case CURSOR_CONTEXT_MENU:
        return contextMenuCursor();
    case CURSOR_PROGRESS:
        return progressCursor();
    case CURSOR_NO_DROP:
        return noDropCursor();
    case CURSOR_ALIAS:
        return aliasCursor();
    case CURSOR_COPY:
        return copyCursor();
    case CURSOR_NONE:
        return noneCursor();
    case CURSOR_NOT_ALLOWED:
        return notAllowedCursor();
    case CURSOR_DEFAULT:
        return pointerCursor();
    case CURSOR_WEBKIT_ZOOM_IN:
        return zoomInCursor();
    case CURSOR_WEBKIT_ZOOM_OUT:
        return zoomOutCursor();
    case CURSOR_WEBKIT_GRAB:
        return grabCursor();
    case CURSOR_WEBKIT_GRABBING:
        return grabbingCursor();
    }
    return pointerCursor();
}

#if ENABLE(CURSOR_VISIBILITY)
void EventHandler::startAutoHideCursorTimer()
{
    Page* page = m_frame->page();
    if (!page)
        return;

    m_autoHideCursorTimer.startOneShot(page->settings()->timeWithoutMouseMovementBeforeHidingControls());

    // The fake mouse move event screws up the auto-hide feature (by resetting the auto-hide timer)
    // so cancel any pending fake mouse moves.
    if (m_fakeMouseMoveEventTimer.isActive())
        m_fakeMouseMoveEventTimer.stop();
}

void EventHandler::cancelAutoHideCursorTimer()
{
    if (m_autoHideCursorTimer.isActive())
        m_autoHideCursorTimer.stop();
}

void EventHandler::autoHideCursorTimerFired(Timer<EventHandler>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_autoHideCursorTimer);
    m_currentMouseCursor = noneCursor();
    FrameView* view = m_frame->view();
    if (view && view->isActive())
        view->setCursor(m_currentMouseCursor);
}
#endif

static LayoutPoint documentPointForWindowPoint(Frame* frame, const IntPoint& windowPoint)
{
    FrameView* view = frame->view();
    // FIXME: Is it really OK to use the wrong coordinates here when view is 0?
    // Historically the code would just crash; this is clearly no worse than that.
    return view ? view->windowToContents(windowPoint) : windowPoint;
}

bool EventHandler::handleMousePressEvent(const PlatformMouseEvent& mouseEvent)
{
    RefPtr<FrameView> protector(m_frame->view());

    if (InspectorInstrumentation::handleMousePress(m_frame->page())) {
        invalidateClick();
        return true;
    }

#if ENABLE(TOUCH_EVENTS)
    bool defaultPrevented = dispatchSyntheticTouchEventIfEnabled(mouseEvent);
    if (defaultPrevented)
        return true;
#endif

    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);

    // FIXME (bug 68185): this call should be made at another abstraction layer
    m_frame->loader()->resetMultipleFormSubmissionProtection();
    
    cancelFakeMouseMoveEvent();
    m_mousePressed = true;
    m_capturesDragging = true;
    setLastKnownMousePosition(mouseEvent);
    m_mouseDownTimestamp = mouseEvent.timestamp();
#if ENABLE(DRAG_SUPPORT)
    m_mouseDownMayStartDrag = false;
#endif
    m_mouseDownMayStartSelect = false;
    m_mouseDownMayStartAutoscroll = false;
    if (FrameView* view = m_frame->view())
        m_mouseDownPos = view->windowToContents(mouseEvent.position());
    else {
        invalidateClick();
        return false;
    }
    m_mouseDownWasInSubframe = false;

    HitTestRequest request(HitTestRequest::Active | HitTestRequest::DisallowShadowContent);
    // Save the document point we generate in case the window coordinate is invalidated by what happens
    // when we dispatch the event.
    LayoutPoint documentPoint = documentPointForWindowPoint(m_frame, mouseEvent.position());
    MouseEventWithHitTestResults mev = m_frame->document()->prepareMouseEvent(request, documentPoint, mouseEvent);

    if (!mev.targetNode()) {
        invalidateClick();
        return false;
    }

    m_mousePressNode = mev.targetNode();

    RefPtr<Frame> subframe = subframeForHitTestResult(mev);
    if (subframe && passMousePressEventToSubframe(mev, subframe.get())) {
        // Start capturing future events for this frame.  We only do this if we didn't clear
        // the m_mousePressed flag, which may happen if an AppKit widget entered a modal event loop.
        m_capturesDragging = subframe->eventHandler()->capturesDragging();
        if (m_mousePressed && m_capturesDragging) {
            m_capturingMouseEventsNode = mev.targetNode();
            m_eventHandlerWillResetCapturingMouseEventsNode = true;
        }
        invalidateClick();
        return true;
    }

#if ENABLE(PAN_SCROLLING)
    // We store whether pan scrolling is in progress before calling stopAutoscrollTimer()
    // because it will set m_autoscrollType to NoAutoscroll on return.
    bool isPanScrollInProgress = m_frame->page() && m_frame->page()->mainFrame()->eventHandler()->panScrollInProgress();
    stopAutoscrollTimer();
    if (isPanScrollInProgress) {
        // We invalidate the click when exiting pan scrolling so that we don't inadvertently navigate
        // away from the current page (e.g. the click was on a hyperlink). See <rdar://problem/6095023>.
        invalidateClick();
        return true;
    }
#endif

    m_clickCount = mouseEvent.clickCount();
    m_clickNode = mev.targetNode();

    if (FrameView* view = m_frame->view()) {
        RenderLayer* layer = m_clickNode->renderer() ? m_clickNode->renderer()->enclosingLayer() : 0;
        IntPoint p = view->windowToContents(mouseEvent.position());
        if (layer && layer->isPointInResizeControl(p)) {
            layer->setInResizeMode(true);
            m_resizeLayer = layer;
            m_offsetFromResizeCorner = layer->offsetFromResizeCorner(p);
            invalidateClick();
            return true;
        }
    }

    m_frame->selection()->setCaretBlinkingSuspended(true);

    bool swallowEvent = !dispatchMouseEvent(eventNames().mousedownEvent, mev.targetNode(), true, m_clickCount, mouseEvent, true);
    m_capturesDragging = !swallowEvent || mev.scrollbar();

    // If the hit testing originally determined the event was in a scrollbar, refetch the MouseEventWithHitTestResults
    // in case the scrollbar widget was destroyed when the mouse event was handled.
    if (mev.scrollbar()) {
        const bool wasLastScrollBar = mev.scrollbar() == m_lastScrollbarUnderMouse.get();
        HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::DisallowShadowContent);
        mev = m_frame->document()->prepareMouseEvent(request, documentPoint, mouseEvent);
        if (wasLastScrollBar && mev.scrollbar() != m_lastScrollbarUnderMouse.get())
            m_lastScrollbarUnderMouse = 0;
    }

    if (swallowEvent) {
        // scrollbars should get events anyway, even disabled controls might be scrollable
        Scrollbar* scrollbar = mev.scrollbar();

        updateLastScrollbarUnderMouse(scrollbar, true);

        if (scrollbar)
            passMousePressEventToScrollbar(mev, scrollbar);
    } else {
        // Refetch the event target node if it currently is the shadow node inside an <input> element.
        // If a mouse event handler changes the input element type to one that has a widget associated,
        // we'd like to EventHandler::handleMousePressEvent to pass the event to the widget and thus the
        // event target node can't still be the shadow node.
        if (mev.targetNode()->isShadowRoot() && isHTMLInputElement(toShadowRoot(mev.targetNode())->host())) {
            HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::DisallowShadowContent);
            mev = m_frame->document()->prepareMouseEvent(request, documentPoint, mouseEvent);
        }

        FrameView* view = m_frame->view();
        Scrollbar* scrollbar = view ? view->scrollbarAtPoint(mouseEvent.position()) : 0;
        if (!scrollbar)
            scrollbar = mev.scrollbar();

        updateLastScrollbarUnderMouse(scrollbar, true);

        if (scrollbar && passMousePressEventToScrollbar(mev, scrollbar))
            swallowEvent = true;
        else
            swallowEvent = handleMousePressEvent(mev);
    }

    return swallowEvent;
}

// This method only exists for platforms that don't know how to deliver 
bool EventHandler::handleMouseDoubleClickEvent(const PlatformMouseEvent& mouseEvent)
{
    RefPtr<FrameView> protector(m_frame->view());

    m_frame->selection()->setCaretBlinkingSuspended(false);

    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);

    // We get this instead of a second mouse-up 
    m_mousePressed = false;
    setLastKnownMousePosition(mouseEvent);

    HitTestRequest request(HitTestRequest::Active | HitTestRequest::DisallowShadowContent);
    MouseEventWithHitTestResults mev = prepareMouseEvent(request, mouseEvent);
    Frame* subframe = subframeForHitTestResult(mev);
    if (m_eventHandlerWillResetCapturingMouseEventsNode)
        m_capturingMouseEventsNode = 0;
    if (subframe && passMousePressEventToSubframe(mev, subframe))
        return true;

    m_clickCount = mouseEvent.clickCount();
    bool swallowMouseUpEvent = !dispatchMouseEvent(eventNames().mouseupEvent, mev.targetNode(), true, m_clickCount, mouseEvent, false);

    bool swallowClickEvent = mouseEvent.button() != RightButton && mev.targetNode() == m_clickNode && !dispatchMouseEvent(eventNames().clickEvent, mev.targetNode(), true, m_clickCount, mouseEvent, true);

    if (m_lastScrollbarUnderMouse)
        swallowMouseUpEvent = m_lastScrollbarUnderMouse->mouseUp(mouseEvent);

    bool swallowMouseReleaseEvent = !swallowMouseUpEvent && handleMouseReleaseEvent(mev);

    invalidateClick();

    return swallowMouseUpEvent || swallowClickEvent || swallowMouseReleaseEvent;
}

static RenderLayer* layerForNode(Node* node)
{
    if (!node)
        return 0;

    RenderObject* renderer = node->renderer();
    if (!renderer)
        return 0;

    RenderLayer* layer = renderer->enclosingLayer();
    if (!layer)
        return 0;

    return layer;
}

bool EventHandler::mouseMoved(const PlatformMouseEvent& event)
{
    RefPtr<FrameView> protector(m_frame->view());
    MaximumDurationTracker maxDurationTracker(&m_maxMouseMovedDuration);

    HitTestResult hoveredNode = HitTestResult(LayoutPoint());
    bool result = handleMouseMoveEvent(event, &hoveredNode);

    Page* page = m_frame->page();
    if (!page)
        return result;

    if (RenderLayer* layer = layerForNode(hoveredNode.innerNode())) {
        if (FrameView* frameView = m_frame->view()) {
            if (frameView->containsScrollableArea(layer))
                layer->mouseMovedInContentArea();
        }
    }

    if (FrameView* frameView = m_frame->view())
        frameView->mouseMovedInContentArea();  

    hoveredNode.setToNonShadowAncestor();
    page->chrome().mouseDidMoveOverElement(hoveredNode, event.modifierFlags());
    page->chrome().setToolTip(hoveredNode);
    return result;
}

bool EventHandler::passMouseMovedEventToScrollbars(const PlatformMouseEvent& event)
{
    HitTestResult hoveredNode;
    return handleMouseMoveEvent(event, &hoveredNode, true);
}

bool EventHandler::handleMouseMoveEvent(const PlatformMouseEvent& mouseEvent, HitTestResult* hoveredNode, bool onlyUpdateScrollbars)
{
    // in Radar 3703768 we saw frequent crashes apparently due to the
    // part being null here, which seems impossible, so check for nil
    // but also assert so that we can try to figure this out in debug
    // builds, if it happens.
    ASSERT(m_frame);
    if (!m_frame)
        return false;

#if ENABLE(TOUCH_EVENTS)
    bool defaultPrevented = dispatchSyntheticTouchEventIfEnabled(mouseEvent);
    if (defaultPrevented)
        return true;
#endif

    RefPtr<FrameView> protector(m_frame->view());
    
    setLastKnownMousePosition(mouseEvent);

    if (m_hoverTimer.isActive())
        m_hoverTimer.stop();

    m_cursorUpdateTimer.stop();

    cancelFakeMouseMoveEvent();

#if ENABLE(SVG)
    if (m_svgPan) {
        toSVGDocument(m_frame->document())->updatePan(m_frame->view()->windowToContents(m_lastKnownMousePosition));
        return true;
    }
#endif

    if (m_frameSetBeingResized)
        return !dispatchMouseEvent(eventNames().mousemoveEvent, m_frameSetBeingResized.get(), false, 0, mouseEvent, false);

    // Send events right to a scrollbar if the mouse is pressed.
    if (m_lastScrollbarUnderMouse && m_mousePressed)
        return m_lastScrollbarUnderMouse->mouseMoved(mouseEvent);

    HitTestRequest::HitTestRequestType hitType = HitTestRequest::Move | HitTestRequest::DisallowShadowContent | HitTestRequest::AllowFrameScrollbars;
    if (m_mousePressed)
        hitType |= HitTestRequest::Active;
    else if (onlyUpdateScrollbars) {
        // Mouse events should be treated as "read-only" if we're updating only scrollbars. This  
        // means that :hover and :active freeze in the state they were in, rather than updating  
        // for nodes the mouse moves while the window is not key (which will be the case if 
        // onlyUpdateScrollbars is true). 
        hitType |= HitTestRequest::ReadOnly;
    }

#if ENABLE(TOUCH_EVENTS)
    // Treat any mouse move events as readonly if the user is currently touching the screen.
    if (m_touchPressed)
        hitType |= HitTestRequest::Active | HitTestRequest::ReadOnly;
#endif
    HitTestRequest request(hitType);
    MouseEventWithHitTestResults mev = prepareMouseEvent(request, mouseEvent);
    if (hoveredNode)
        *hoveredNode = mev.hitTestResult();

    if (m_resizeLayer && m_resizeLayer->inResizeMode())
        m_resizeLayer->resize(mouseEvent, m_offsetFromResizeCorner);
    else {
        Scrollbar* scrollbar = mev.scrollbar();
        updateLastScrollbarUnderMouse(scrollbar, !m_mousePressed);
        if (!m_mousePressed && scrollbar)
            scrollbar->mouseMoved(mouseEvent); // Handle hover effects on platforms that support visual feedback on scrollbar hovering.
        if (onlyUpdateScrollbars)
            return true;
    }

    bool swallowEvent = false;
    RefPtr<Frame> newSubframe = m_capturingMouseEventsNode.get() ? subframeForTargetNode(m_capturingMouseEventsNode.get()) : subframeForHitTestResult(mev);
 
    // We want mouseouts to happen first, from the inside out.  First send a move event to the last subframe so that it will fire mouseouts.
    if (m_lastMouseMoveEventSubframe && m_lastMouseMoveEventSubframe->tree()->isDescendantOf(m_frame) && m_lastMouseMoveEventSubframe != newSubframe)
        passMouseMoveEventToSubframe(mev, m_lastMouseMoveEventSubframe.get());

    if (newSubframe) {
        // Update over/out state before passing the event to the subframe.
        updateMouseEventTargetNode(mev.targetNode(), mouseEvent, true);
        
        // Event dispatch in updateMouseEventTargetNode may have caused the subframe of the target
        // node to be detached from its FrameView, in which case the event should not be passed.
        if (newSubframe->view())
            swallowEvent |= passMouseMoveEventToSubframe(mev, newSubframe.get(), hoveredNode);
    } else {
        if (FrameView* view = m_frame->view()) {
            OptionalCursor optionalCursor = selectCursor(mev.hitTestResult(), mouseEvent.shiftKey());
            if (optionalCursor.isCursorChange()) {
                m_currentMouseCursor = optionalCursor.cursor();
                view->setCursor(m_currentMouseCursor);
            }
        }
    }
    
    m_lastMouseMoveEventSubframe = newSubframe;

    if (swallowEvent)
        return true;
    
    swallowEvent = !dispatchMouseEvent(eventNames().mousemoveEvent, mev.targetNode(), false, 0, mouseEvent, true);
#if ENABLE(DRAG_SUPPORT)
    if (!swallowEvent)
        swallowEvent = handleMouseDraggedEvent(mev);
#endif // ENABLE(DRAG_SUPPORT)

    return swallowEvent;
}

void EventHandler::invalidateClick()
{
    m_clickCount = 0;
    m_clickNode = 0;
}

inline static bool mouseIsReleasedOnPressedElement(Node* targetNode, Node* clickNode)
{
    if (targetNode == clickNode)
        return true;

    if (!targetNode)
        return false;

    ShadowRoot* containingShadowRoot = targetNode->containingShadowRoot();
    if (!containingShadowRoot)
        return false;

    // FIXME: When an element in UA ShadowDOM (e.g. inner element in <input>) is clicked,
    // we assume that the host element is clicked. This is necessary for implementing <input type="range"> etc.
    // However, we should not check ShadowRoot type basically.
    // https://bugs.webkit.org/show_bug.cgi?id=108047
    if (containingShadowRoot->type() != ShadowRoot::UserAgentShadowRoot)
        return false;

    Node* adjustedTargetNode = targetNode->shadowHost();
    Node* adjustedClickNode = clickNode ? clickNode->shadowHost() : 0;
    return adjustedTargetNode == adjustedClickNode;
}

bool EventHandler::handleMouseReleaseEvent(const PlatformMouseEvent& mouseEvent)
{
    RefPtr<FrameView> protector(m_frame->view());

    m_frame->selection()->setCaretBlinkingSuspended(false);

#if ENABLE(TOUCH_EVENTS)
    bool defaultPrevented = dispatchSyntheticTouchEventIfEnabled(mouseEvent);
    if (defaultPrevented)
        return true;
#endif

    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);

#if ENABLE(PAN_SCROLLING)
    m_autoscrollController->handleMouseReleaseEvent(mouseEvent);
#endif

    m_mousePressed = false;
    setLastKnownMousePosition(mouseEvent);

#if ENABLE(SVG)
    if (m_svgPan) {
        m_svgPan = false;
        toSVGDocument(m_frame->document())->updatePan(m_frame->view()->windowToContents(m_lastKnownMousePosition));
        return true;
    }
#endif

    if (m_frameSetBeingResized)
        return !dispatchMouseEvent(eventNames().mouseupEvent, m_frameSetBeingResized.get(), true, m_clickCount, mouseEvent, false);

    if (m_lastScrollbarUnderMouse) {
        invalidateClick();
        m_lastScrollbarUnderMouse->mouseUp(mouseEvent);
        bool cancelable = true;
        bool setUnder = false;
        return !dispatchMouseEvent(eventNames().mouseupEvent, m_lastNodeUnderMouse.get(), cancelable, m_clickCount, mouseEvent, setUnder);
    }

    HitTestRequest request(HitTestRequest::Release | HitTestRequest::DisallowShadowContent);
    MouseEventWithHitTestResults mev = prepareMouseEvent(request, mouseEvent);
    Frame* subframe = m_capturingMouseEventsNode.get() ? subframeForTargetNode(m_capturingMouseEventsNode.get()) : subframeForHitTestResult(mev);
    if (m_eventHandlerWillResetCapturingMouseEventsNode)
        m_capturingMouseEventsNode = 0;
    if (subframe && passMouseReleaseEventToSubframe(mev, subframe))
        return true;

    bool swallowMouseUpEvent = !dispatchMouseEvent(eventNames().mouseupEvent, mev.targetNode(), true, m_clickCount, mouseEvent, false);

    bool contextMenuEvent = mouseEvent.button() == RightButton;

    bool swallowClickEvent = m_clickCount > 0 && !contextMenuEvent && mouseIsReleasedOnPressedElement(mev.targetNode(), m_clickNode.get()) && !dispatchMouseEvent(eventNames().clickEvent, mev.targetNode(), true, m_clickCount, mouseEvent, true);

    if (m_resizeLayer) {
        m_resizeLayer->setInResizeMode(false);
        m_resizeLayer = 0;
    }

    bool swallowMouseReleaseEvent = false;
    if (!swallowMouseUpEvent)
        swallowMouseReleaseEvent = handleMouseReleaseEvent(mev);

    invalidateClick();

    return swallowMouseUpEvent || swallowClickEvent || swallowMouseReleaseEvent;
}

bool EventHandler::handlePasteGlobalSelection(const PlatformMouseEvent& mouseEvent)
{
    // If the event was a middle click, attempt to copy global selection in after
    // the newly set caret position.
    //
    // This code is called from either the mouse up or mouse down handling. There
    // is some debate about when the global selection is pasted:
    //   xterm: pastes on up.
    //   GTK: pastes on down.
    //   Qt: pastes on up.
    //   Firefox: pastes on up.
    //   Chromium: pastes on up.
    //
    // There is something of a webcompat angle to this well, as highlighted by
    // crbug.com/14608. Pages can clear text boxes 'onclick' and, if we paste on
    // down then the text is pasted just before the onclick handler runs and
    // clears the text box. So it's important this happens after the event
    // handlers have been fired.
#if PLATFORM(GTK)
    if (mouseEvent.type() != PlatformEvent::MousePressed)
        return false;
#else
    if (mouseEvent.type() != PlatformEvent::MouseReleased)
        return false;
#endif

    if (!m_frame->page())
        return false;
    Frame* focusFrame = m_frame->page()->focusController()->focusedOrMainFrame();
    // Do not paste here if the focus was moved somewhere else.
    if (m_frame == focusFrame && m_frame->editor().client()->supportsGlobalSelection())
        return m_frame->editor().command(ASCIILiteral("PasteGlobalSelection")).execute();

    return false;
}


#if ENABLE(DRAG_SUPPORT)
bool EventHandler::dispatchDragEvent(const AtomicString& eventType, Node* dragTarget, const PlatformMouseEvent& event, Clipboard* clipboard)
{
    FrameView* view = m_frame->view();

    // FIXME: We might want to dispatch a dragleave even if the view is gone.
    if (!view)
        return false;

    view->resetDeferredRepaintDelay();
    RefPtr<MouseEvent> me = MouseEvent::create(eventType,
        true, true, event.timestamp(), m_frame->document()->defaultView(),
        0, event.globalPosition().x(), event.globalPosition().y(), event.position().x(), event.position().y(),
#if ENABLE(POINTER_LOCK)
        event.movementDelta().x(), event.movementDelta().y(),
#endif
        event.ctrlKey(), event.altKey(), event.shiftKey(), event.metaKey(),
        0, 0, clipboard);

    dragTarget->dispatchEvent(me.get(), IGNORE_EXCEPTION);
    return me->defaultPrevented();
}

static bool targetIsFrame(Node* target, Frame*& frame)
{
    if (!target)
        return false;

    if (!target->hasTagName(frameTag) && !target->hasTagName(iframeTag))
        return false;

    frame = toHTMLFrameElementBase(target)->contentFrame();

    return true;
}

static bool findDropZone(Node* target, Clipboard* clipboard)
{
    Element* element = target->isElementNode() ? toElement(target) : target->parentElement();
    for (; element; element = element->parentElement()) {
        bool matched = false;
        String dropZoneStr = element->fastGetAttribute(webkitdropzoneAttr);

        if (dropZoneStr.isEmpty())
            continue;
        
        dropZoneStr.makeLower();
        
        SpaceSplitString keywords(dropZoneStr, false);
        if (keywords.isNull())
            continue;
        
        DragOperation dragOperation = DragOperationNone;
        for (unsigned int i = 0; i < keywords.size(); i++) {
            DragOperation op = convertDropZoneOperationToDragOperation(keywords[i]);
            if (op != DragOperationNone) {
                if (dragOperation == DragOperationNone)
                    dragOperation = op;
            } else
                matched = matched || clipboard->hasDropZoneType(keywords[i].string());

            if (matched && dragOperation != DragOperationNone)
                break;
        }
        if (matched) {
            clipboard->setDropEffect(convertDragOperationToDropZoneOperation(dragOperation));
            return true;
        }
    }
    return false;
}
    
bool EventHandler::updateDragAndDrop(const PlatformMouseEvent& event, Clipboard* clipboard)
{
    bool accept = false;

    if (!m_frame->view())
        return false;

    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::DisallowShadowContent);
    MouseEventWithHitTestResults mev = prepareMouseEvent(request, event);

    // Drag events should never go to text nodes (following IE, and proper mouseover/out dispatch)
    RefPtr<Node> newTarget = mev.targetNode();
    if (newTarget && newTarget->isTextNode())
        newTarget = EventPathWalker::parent(newTarget.get());

    m_autoscrollController->updateDragAndDrop(newTarget.get(), event.position(), event.timestamp());

    if (m_dragTarget != newTarget) {
        // FIXME: this ordering was explicitly chosen to match WinIE. However,
        // it is sometimes incorrect when dragging within subframes, as seen with
        // LayoutTests/fast/events/drag-in-frames.html.
        //
        // Moreover, this ordering conforms to section 7.9.4 of the HTML 5 spec. <http://dev.w3.org/html5/spec/Overview.html#drag-and-drop-processing-model>.
        Frame* targetFrame;
        if (targetIsFrame(newTarget.get(), targetFrame)) {
            if (targetFrame)
                accept = targetFrame->eventHandler()->updateDragAndDrop(event, clipboard);
        } else if (newTarget) {
            // As per section 7.9.4 of the HTML 5 spec., we must always fire a drag event before firing a dragenter, dragleave, or dragover event.
            if (dragState().source && dragState().shouldDispatchEvents) {
                // for now we don't care if event handler cancels default behavior, since there is none
                dispatchDragSrcEvent(eventNames().dragEvent, event);
            }
            accept = dispatchDragEvent(eventNames().dragenterEvent, newTarget.get(), event, clipboard);
            if (!accept)
                accept = findDropZone(newTarget.get(), clipboard);
        }

        if (targetIsFrame(m_dragTarget.get(), targetFrame)) {
            if (targetFrame)
                accept = targetFrame->eventHandler()->updateDragAndDrop(event, clipboard);
        } else if (m_dragTarget)
            dispatchDragEvent(eventNames().dragleaveEvent, m_dragTarget.get(), event, clipboard);

        if (newTarget) {
            // We do not explicitly call dispatchDragEvent here because it could ultimately result in the appearance that
            // two dragover events fired. So, we mark that we should only fire a dragover event on the next call to this function.
            m_shouldOnlyFireDragOverEvent = true;
        }
    } else {
        Frame* targetFrame;
        if (targetIsFrame(newTarget.get(), targetFrame)) {
            if (targetFrame)
                accept = targetFrame->eventHandler()->updateDragAndDrop(event, clipboard);
        } else if (newTarget) {
            // Note, when dealing with sub-frames, we may need to fire only a dragover event as a drag event may have been fired earlier.
            if (!m_shouldOnlyFireDragOverEvent && dragState().source && dragState().shouldDispatchEvents) {
                // for now we don't care if event handler cancels default behavior, since there is none
                dispatchDragSrcEvent(eventNames().dragEvent, event);
            }
            accept = dispatchDragEvent(eventNames().dragoverEvent, newTarget.get(), event, clipboard);
            if (!accept)
                accept = findDropZone(newTarget.get(), clipboard);
            m_shouldOnlyFireDragOverEvent = false;
        }
    }
    m_dragTarget = newTarget;

    return accept;
}

void EventHandler::cancelDragAndDrop(const PlatformMouseEvent& event, Clipboard* clipboard)
{
    Frame* targetFrame;
    if (targetIsFrame(m_dragTarget.get(), targetFrame)) {
        if (targetFrame)
            targetFrame->eventHandler()->cancelDragAndDrop(event, clipboard);
    } else if (m_dragTarget.get()) {
        if (dragState().source && dragState().shouldDispatchEvents)
            dispatchDragSrcEvent(eventNames().dragEvent, event);
        dispatchDragEvent(eventNames().dragleaveEvent, m_dragTarget.get(), event, clipboard);
    }
    clearDragState();
}

bool EventHandler::performDragAndDrop(const PlatformMouseEvent& event, Clipboard* clipboard)
{
    Frame* targetFrame;
    bool preventedDefault = false;
    if (targetIsFrame(m_dragTarget.get(), targetFrame)) {
        if (targetFrame)
            preventedDefault = targetFrame->eventHandler()->performDragAndDrop(event, clipboard);
    } else if (m_dragTarget.get())
        preventedDefault = dispatchDragEvent(eventNames().dropEvent, m_dragTarget.get(), event, clipboard);
    clearDragState();
    return preventedDefault;
}

void EventHandler::clearDragState()
{
    stopAutoscrollTimer();
    m_dragTarget = 0;
    m_capturingMouseEventsNode = 0;
    m_shouldOnlyFireDragOverEvent = false;
#if PLATFORM(MAC)
    m_sendingEventToSubview = false;
#endif
}
#endif // ENABLE(DRAG_SUPPORT)

void EventHandler::setCapturingMouseEventsNode(PassRefPtr<Node> n)
{
    m_capturingMouseEventsNode = n;
    m_eventHandlerWillResetCapturingMouseEventsNode = false;
}

MouseEventWithHitTestResults EventHandler::prepareMouseEvent(const HitTestRequest& request, const PlatformMouseEvent& mev)
{
    ASSERT(m_frame);
    ASSERT(m_frame->document());
    
    return m_frame->document()->prepareMouseEvent(request, documentPointForWindowPoint(m_frame, mev.position()), mev);
}

#if ENABLE(SVG)
static inline SVGElementInstance* instanceAssociatedWithShadowTreeElement(Node* referenceNode)
{
    if (!referenceNode || !referenceNode->isSVGElement())
        return 0;

    ShadowRoot* shadowRoot = referenceNode->containingShadowRoot();
    if (!shadowRoot)
        return 0;

    Element* shadowTreeParentElement = shadowRoot->host();
    if (!shadowTreeParentElement || !shadowTreeParentElement->hasTagName(useTag))
        return 0;

    return toSVGUseElement(shadowTreeParentElement)->instanceForShadowTreeElement(referenceNode);
}
#endif

void EventHandler::updateMouseEventTargetNode(Node* targetNode, const PlatformMouseEvent& mouseEvent, bool fireMouseOverOut)
{
    Node* result = targetNode;
    
    // If we're capturing, we always go right to that node.
    if (m_capturingMouseEventsNode)
        result = m_capturingMouseEventsNode.get();
    else {
        // If the target node is a text node, dispatch on the parent node - rdar://4196646
        if (result && result->isTextNode())
            result = EventPathWalker::parent(result);
    }
    m_nodeUnderMouse = result;
#if ENABLE(SVG)
    m_instanceUnderMouse = instanceAssociatedWithShadowTreeElement(result);

    // <use> shadow tree elements may have been recloned, update node under mouse in any case
    if (m_lastInstanceUnderMouse) {
        SVGElement* lastCorrespondingElement = m_lastInstanceUnderMouse->correspondingElement();
        SVGElement* lastCorrespondingUseElement = m_lastInstanceUnderMouse->correspondingUseElement();

        if (lastCorrespondingElement && lastCorrespondingUseElement) {
            HashSet<SVGElementInstance*> instances = lastCorrespondingElement->instancesForElement();

            // Locate the recloned shadow tree element for our corresponding instance
            HashSet<SVGElementInstance*>::iterator end = instances.end();
            for (HashSet<SVGElementInstance*>::iterator it = instances.begin(); it != end; ++it) {
                SVGElementInstance* instance = (*it);
                ASSERT(instance->correspondingElement() == lastCorrespondingElement);

                if (instance == m_lastInstanceUnderMouse)
                    continue;

                if (instance->correspondingUseElement() != lastCorrespondingUseElement)
                    continue;

                SVGElement* shadowTreeElement = instance->shadowTreeElement();
                if (!shadowTreeElement->inDocument() || m_lastNodeUnderMouse == shadowTreeElement)
                    continue;

                m_lastNodeUnderMouse = shadowTreeElement;
                m_lastInstanceUnderMouse = instance;
                break;
            }
        }
    }
#endif

    // Fire mouseout/mouseover if the mouse has shifted to a different node.
    if (fireMouseOverOut) {
        RenderLayer* layerForLastNode = layerForNode(m_lastNodeUnderMouse.get());
        RenderLayer* layerForNodeUnderMouse = layerForNode(m_nodeUnderMouse.get());
        Page* page = m_frame->page();

        if (m_lastNodeUnderMouse && (!m_nodeUnderMouse || m_nodeUnderMouse->document() != m_frame->document())) {
            // The mouse has moved between frames.
            if (Frame* frame = m_lastNodeUnderMouse->document()->frame()) {
                if (FrameView* frameView = frame->view())
                    frameView->mouseExitedContentArea();
            }
        } else if (page && (layerForLastNode && (!layerForNodeUnderMouse || layerForNodeUnderMouse != layerForLastNode))) {
            // The mouse has moved between layers.
            if (Frame* frame = m_lastNodeUnderMouse->document()->frame()) {
                if (FrameView* frameView = frame->view()) {
                    if (frameView->containsScrollableArea(layerForLastNode))
                        layerForLastNode->mouseExitedContentArea();
                }
            }
        }

        if (m_nodeUnderMouse && (!m_lastNodeUnderMouse || m_lastNodeUnderMouse->document() != m_frame->document())) {
            // The mouse has moved between frames.
            if (Frame* frame = m_nodeUnderMouse->document()->frame()) {
                if (FrameView* frameView = frame->view())
                    frameView->mouseEnteredContentArea();
            }
        } else if (page && (layerForNodeUnderMouse && (!layerForLastNode || layerForNodeUnderMouse != layerForLastNode))) {
            // The mouse has moved between layers.
            if (Frame* frame = m_nodeUnderMouse->document()->frame()) {
                if (FrameView* frameView = frame->view()) {
                    if (frameView->containsScrollableArea(layerForNodeUnderMouse))
                        layerForNodeUnderMouse->mouseEnteredContentArea();
                }
            }
        }

        if (m_lastNodeUnderMouse && m_lastNodeUnderMouse->document() != m_frame->document()) {
            m_lastNodeUnderMouse = 0;
            m_lastScrollbarUnderMouse = 0;
#if ENABLE(SVG)
            m_lastInstanceUnderMouse = 0;
#endif
        }

        if (m_lastNodeUnderMouse != m_nodeUnderMouse) {
            // send mouseout event to the old node
            if (m_lastNodeUnderMouse)
                m_lastNodeUnderMouse->dispatchMouseEvent(mouseEvent, eventNames().mouseoutEvent, 0, m_nodeUnderMouse.get());
            // send mouseover event to the new node
            if (m_nodeUnderMouse)
                m_nodeUnderMouse->dispatchMouseEvent(mouseEvent, eventNames().mouseoverEvent, 0, m_lastNodeUnderMouse.get());
        }
        m_lastNodeUnderMouse = m_nodeUnderMouse;
#if ENABLE(SVG)
        m_lastInstanceUnderMouse = instanceAssociatedWithShadowTreeElement(m_nodeUnderMouse.get());
#endif
    }
}

bool EventHandler::dispatchMouseEvent(const AtomicString& eventType, Node* targetNode, bool /*cancelable*/, int clickCount, const PlatformMouseEvent& mouseEvent, bool setUnder)
{
    if (FrameView* view = m_frame->view())
        view->resetDeferredRepaintDelay();

    updateMouseEventTargetNode(targetNode, mouseEvent, setUnder);

    bool swallowEvent = false;

    if (m_nodeUnderMouse)
        swallowEvent = !(m_nodeUnderMouse->dispatchMouseEvent(mouseEvent, eventType, clickCount));

    if (!swallowEvent && eventType == eventNames().mousedownEvent) {

        // If clicking on a frame scrollbar, do not mess up with content focus.
        if (FrameView* view = m_frame->view()) {
            if (view->scrollbarAtPoint(mouseEvent.position()))
                return true;
        }

        // The layout needs to be up to date to determine if an element is focusable.
        m_frame->document()->updateLayoutIgnorePendingStylesheets();

        // Blur current focus node when a link/button is clicked; this
        // is expected by some sites that rely on onChange handlers running
        // from form fields before the button click is processed.

        Element* element;
        if (m_nodeUnderMouse)
            element = m_nodeUnderMouse->isElementNode() ? toElement(m_nodeUnderMouse.get()) : m_nodeUnderMouse->parentOrShadowHostElement();
        else
            element = 0;

        // Walk up the DOM tree to search for an element to focus.
        while (element) {
            if (element->isMouseFocusable()) {
                // To fix <rdar://problem/4895428> Can't drag selected ToDo, we don't focus a
                // node on mouse down if it's selected and inside a focused node. It will be
                // focused if the user does a mouseup over it, however, because the mouseup
                // will set a selection inside it, which will call setFocuseNodeIfNeeded.
                if (m_frame->selection()->isRange()
                    && m_frame->selection()->toNormalizedRange()->compareNode(element, IGNORE_EXCEPTION) == Range::NODE_INSIDE
                    && element->isDescendantOf(m_frame->document()->focusedElement()))
                    return true;
                    
                break;
            }
            element = element->parentOrShadowHostElement();
        }

        // Only change the focus when clicking scrollbars if it can transfered to a mouse focusable node.
        if ((!element || !element->isMouseFocusable()) && isInsideScrollbar(mouseEvent.position()))
            return false;

        // If focus shift is blocked, we eat the event.  Note we should never clear swallowEvent
        // if the page already set it (e.g., by canceling default behavior).
        if (Page* page = m_frame->page()) {
            if (element && element->isMouseFocusable()) {
                if (!page->focusController()->setFocusedElement(element, m_frame))
                    swallowEvent = true;
            } else if (!element || !element->focused()) {
                if (!page->focusController()->setFocusedElement(0, m_frame))
                    swallowEvent = true;
            }
        }
    }

    return !swallowEvent;
}

bool EventHandler::isInsideScrollbar(const IntPoint& windowPoint) const
{
    if (RenderView* renderView = m_frame->contentRenderer()) {
        HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::DisallowShadowContent);
        HitTestResult result(windowPoint);
        renderView->hitTest(request, result);
        return result.scrollbar();
    }

    return false;
}

#if !PLATFORM(GTK)
bool EventHandler::shouldTurnVerticalTicksIntoHorizontal(const HitTestResult&, const PlatformWheelEvent&) const
{
    return false;
}
#endif

bool EventHandler::handleWheelEvent(const PlatformWheelEvent& e)
{
    Document* doc = m_frame->document();

    RenderObject* docRenderer = doc->renderer();
    if (!docRenderer)
        return false;
    
    RefPtr<FrameView> protector(m_frame->view());

    FrameView* view = m_frame->view();
    if (!view)
        return false;

    m_isHandlingWheelEvent = true;
    setFrameWasScrolledByUser();
    LayoutPoint vPoint = view->windowToContents(e.position());

    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::DisallowShadowContent);
    HitTestResult result(vPoint);
    doc->renderView()->hitTest(request, result);

    bool useLatchedWheelEventNode = e.useLatchedEventNode();

    // FIXME: Is the following code different from just calling innerElement?
    Node* node = result.innerNode();
    // Wheel events should not dispatch to text nodes.
    if (node && node->isTextNode())
        node = EventPathWalker::parent(node);

    bool isOverWidget;
    if (useLatchedWheelEventNode) {
        if (!m_latchedWheelEventNode) {
            m_latchedWheelEventNode = node;
            m_widgetIsLatched = result.isOverWidget();
        } else
            node = m_latchedWheelEventNode.get();

        isOverWidget = m_widgetIsLatched;
    } else {
        if (m_latchedWheelEventNode)
            m_latchedWheelEventNode = 0;
        if (m_previousWheelScrolledNode)
            m_previousWheelScrolledNode = 0;

        isOverWidget = result.isOverWidget();
    }

    // FIXME: It should not be necessary to do this mutation here.
    // Instead, the handlers should know convert vertical scrolls
    // appropriately.
    PlatformWheelEvent event = e;
    if (m_baseEventType == PlatformEvent::NoType && shouldTurnVerticalTicksIntoHorizontal(result, e))
        event = event.copyTurningVerticalTicksIntoHorizontalTicks();

    if (node) {
        // Figure out which view to send the event to.
        RenderObject* target = node->renderer();
        
        if (isOverWidget && target && target->isWidget()) {
            Widget* widget = toRenderWidget(target)->widget();
            if (widget && passWheelEventToWidget(e, widget)) {
                m_isHandlingWheelEvent = false;
                return true;
            }
        }

        if (node && !node->dispatchWheelEvent(event)) {
            m_isHandlingWheelEvent = false;
            return true;
        }
    }


    // We do another check on the frame view because the event handler can run JS which results in the frame getting destroyed.
    view = m_frame->view();
    bool didHandleEvent = view ? view->wheelEvent(event) : false;
    m_isHandlingWheelEvent = false;
    return didHandleEvent;
}

void EventHandler::defaultWheelEventHandler(Node* startNode, WheelEvent* wheelEvent)
{
    if (!startNode || !wheelEvent)
        return;
    
    Node* stopNode = m_previousWheelScrolledNode.get();
    ScrollGranularity granularity = wheelGranularityToScrollGranularity(wheelEvent->deltaMode());
    
    // Break up into two scrolls if we need to.  Diagonal movement on 
    // a MacBook pro is an example of a 2-dimensional mouse wheel event (where both deltaX and deltaY can be set).
    if (scrollNode(wheelEvent->rawDeltaX(), granularity, ScrollLeft, ScrollRight, startNode, &stopNode))
        wheelEvent->setDefaultHandled();
    
    if (scrollNode(wheelEvent->rawDeltaY(), granularity, ScrollUp, ScrollDown, startNode, &stopNode))
        wheelEvent->setDefaultHandled();
    
    if (!m_latchedWheelEventNode)
        m_previousWheelScrolledNode = stopNode;
}

#if ENABLE(GESTURE_EVENTS)
bool EventHandler::handleGestureTapDown()
{
    FrameView* view = m_frame->view();
    if (!view)
        return false;
    if (ScrollAnimator* scrollAnimator = view->existingScrollAnimator())
        scrollAnimator->cancelAnimations();
    const FrameView::ScrollableAreaSet* areas = view->scrollableAreas();
    if (!areas)
        return false;
    for (FrameView::ScrollableAreaSet::const_iterator it = areas->begin(); it != areas->end(); ++it) {
        ScrollableArea* sa = *it;
        ScrollAnimator* animator = sa->scrollAnimator();
        if (animator)
            animator->cancelAnimations();
    }
    return false;
}

bool EventHandler::handleGestureEvent(const PlatformGestureEvent& gestureEvent)
{
    Node* eventTarget = 0;
    Scrollbar* scrollbar = 0;
    if (gestureEvent.type() == PlatformEvent::GestureScrollEnd
        || gestureEvent.type() == PlatformEvent::GestureScrollUpdate
        || gestureEvent.type() == PlatformEvent::GestureScrollUpdateWithoutPropagation) {
        scrollbar = m_scrollbarHandlingScrollGesture.get();
        eventTarget = m_scrollGestureHandlingNode.get();
    }

    IntPoint adjustedPoint = gestureEvent.position();
    HitTestRequest::HitTestRequestType hitType = HitTestRequest::TouchEvent;
    if (gestureEvent.type() == PlatformEvent::GestureTapDown) {
#if ENABLE(TOUCH_ADJUSTMENT)
        adjustGesturePosition(gestureEvent, adjustedPoint);
#endif
        hitType |= HitTestRequest::Active;
    } else if (gestureEvent.type() == PlatformEvent::GestureTapDownCancel)
        hitType |= HitTestRequest::Release;
    else if (gestureEvent.type() == PlatformEvent::GestureTap) {
        // The mouseup event synthesized for this gesture will clear the active state of the
        // targeted node, so performing a ReadOnly hit test here is fine.
        hitType |= HitTestRequest::ReadOnly;
    }
    else
        hitType |= HitTestRequest::Active | HitTestRequest::ReadOnly;

    if (!shouldGesturesTriggerActive())
        hitType |= HitTestRequest::ReadOnly;

    if ((!scrollbar && !eventTarget) || !(hitType & HitTestRequest::ReadOnly)) {
        IntPoint hitTestPoint = m_frame->view()->windowToContents(adjustedPoint);
        HitTestResult result = hitTestResultAtPoint(hitTestPoint, hitType | HitTestRequest::AllowFrameScrollbars);
        eventTarget = result.targetNode();
        if (!scrollbar)
            scrollbar = result.scrollbar();
    }

    if (scrollbar) {
        bool eventSwallowed = scrollbar->gestureEvent(gestureEvent);
        if (gestureEvent.type() == PlatformEvent::GestureScrollBegin && eventSwallowed)
            m_scrollbarHandlingScrollGesture = scrollbar;
        else if (gestureEvent.type() == PlatformEvent::GestureScrollEnd || !eventSwallowed)
            m_scrollbarHandlingScrollGesture = 0;

        if (eventSwallowed)
            return true;
    }

    if (eventTarget) {
        bool eventSwallowed = eventTarget->dispatchGestureEvent(gestureEvent);
        if (gestureEvent.type() == PlatformEvent::GestureScrollBegin || gestureEvent.type() == PlatformEvent::GestureScrollEnd) {
            if (eventSwallowed)
                m_scrollGestureHandlingNode = eventTarget;
        }

        if (eventSwallowed)
            return true;
    }

    // FIXME: A more general scroll system (https://bugs.webkit.org/show_bug.cgi?id=80596) will
    // eliminate the need for this.
    TemporaryChange<PlatformEvent::Type> baseEventType(m_baseEventType, gestureEvent.type());

    switch (gestureEvent.type()) {
    case PlatformEvent::GestureScrollBegin:
        return handleGestureScrollBegin(gestureEvent);
    case PlatformEvent::GestureScrollUpdate:
    case PlatformEvent::GestureScrollUpdateWithoutPropagation:
        return handleGestureScrollUpdate(gestureEvent);
    case PlatformEvent::GestureScrollEnd:
        clearGestureScrollNodes(); 
        return true;
    case PlatformEvent::GestureTap:
        return handleGestureTap(gestureEvent);
    case PlatformEvent::GestureTapDown:
        return handleGestureTapDown();
    case PlatformEvent::GestureLongPress:
        return handleGestureLongPress(gestureEvent);
    case PlatformEvent::GestureLongTap:
        return handleGestureLongTap(gestureEvent);
    case PlatformEvent::GestureTwoFingerTap:
        return handleGestureTwoFingerTap(gestureEvent);
    case PlatformEvent::GestureTapDownCancel:
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    return false;
}

bool EventHandler::handleGestureTap(const PlatformGestureEvent& gestureEvent)
{
    // FIXME: Refactor this code to not hit test multiple times. We use the adjusted position to ensure that the correct node is targeted by the later redundant hit tests.
    IntPoint adjustedPoint = gestureEvent.position();
#if ENABLE(TOUCH_ADJUSTMENT)
    adjustGesturePosition(gestureEvent, adjustedPoint);
#endif

    PlatformMouseEvent fakeMouseMove(adjustedPoint, gestureEvent.globalPosition(),
        NoButton, PlatformEvent::MouseMoved, /* clickCount */ 0,
        gestureEvent.shiftKey(), gestureEvent.ctrlKey(), gestureEvent.altKey(), gestureEvent.metaKey(), gestureEvent.timestamp());
    mouseMoved(fakeMouseMove);

    int tapCount = 1;
    // FIXME: deletaX is overloaded to mean different things for different gestures.
    // http://wkb.ug/93123
    if (gestureEvent.deltaX() > 0)
        tapCount = static_cast<int>(gestureEvent.deltaX());

    bool defaultPrevented = false;
    PlatformMouseEvent fakeMouseDown(adjustedPoint, gestureEvent.globalPosition(),
        LeftButton, PlatformEvent::MousePressed, tapCount,
        gestureEvent.shiftKey(), gestureEvent.ctrlKey(), gestureEvent.altKey(), gestureEvent.metaKey(), gestureEvent.timestamp());
    defaultPrevented |= handleMousePressEvent(fakeMouseDown);

    PlatformMouseEvent fakeMouseUp(adjustedPoint, gestureEvent.globalPosition(),
        LeftButton, PlatformEvent::MouseReleased, tapCount,
        gestureEvent.shiftKey(), gestureEvent.ctrlKey(), gestureEvent.altKey(), gestureEvent.metaKey(), gestureEvent.timestamp());
    defaultPrevented |= handleMouseReleaseEvent(fakeMouseUp);

    return defaultPrevented;
}

bool EventHandler::handleGestureLongPress(const PlatformGestureEvent& gestureEvent)
{
#if ENABLE(DRAG_SUPPORT)
    if (m_frame->settings() && m_frame->settings()->touchDragDropEnabled()) {
        IntPoint adjustedPoint = gestureEvent.position();
#if ENABLE(TOUCH_ADJUSTMENT)
        adjustGesturePosition(gestureEvent, adjustedPoint);
#endif
        PlatformMouseEvent mouseDownEvent(adjustedPoint, gestureEvent.globalPosition(), LeftButton, PlatformEvent::MousePressed, 0, false, false, false, false, WTF::currentTime());
        handleMousePressEvent(mouseDownEvent);
        PlatformMouseEvent mouseDragEvent(adjustedPoint, gestureEvent.globalPosition(), LeftButton, PlatformEvent::MouseMoved, 0, false, false, false, false, WTF::currentTime());
        HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::DisallowShadowContent);
        MouseEventWithHitTestResults mev = prepareMouseEvent(request, mouseDragEvent);
        m_didStartDrag = false;
        RefPtr<Frame> subframe = subframeForHitTestResult(mev);
        if (subframe && !m_mouseDownMayStartDrag) {
            if (subframe->eventHandler()->handleGestureLongPress(gestureEvent))
                return true;
        }
        handleDrag(mev, DontCheckDragHysteresis);
        if (m_didStartDrag)
            return true;
    }
#endif
    return handleGestureForTextSelectionOrContextMenu(gestureEvent);
}

bool EventHandler::handleGestureLongTap(const PlatformGestureEvent& gestureEvent)
{
#if ENABLE(CONTEXT_MENUS)
    if (!m_didLongPressInvokeContextMenu)
        return sendContextMenuEventForGesture(gestureEvent);
#endif // ENABLE(CONTEXT_MENUS)
    return false;
}

bool EventHandler::handleGestureForTextSelectionOrContextMenu(const PlatformGestureEvent& gestureEvent)
{
#if ENABLE(CONTEXT_MENUS)
    m_didLongPressInvokeContextMenu = (gestureEvent.type() == PlatformEvent::GestureLongPress);
    return sendContextMenuEventForGesture(gestureEvent);
#else
    return false;
#endif
}

bool EventHandler::handleGestureTwoFingerTap(const PlatformGestureEvent& gestureEvent)
{
    return handleGestureForTextSelectionOrContextMenu(gestureEvent);
}

bool EventHandler::passGestureEventToWidget(const PlatformGestureEvent& gestureEvent, Widget* widget)
{
    if (!widget)
        return false;

    if (!widget->isFrameView())
        return false;

    return toFrameView(widget)->frame()->eventHandler()->handleGestureEvent(gestureEvent);
}

bool EventHandler::passGestureEventToWidgetIfPossible(const PlatformGestureEvent& gestureEvent, RenderObject* renderer)
{
    if (m_lastHitTestResultOverWidget && renderer && renderer->isWidget()) {
        Widget* widget = toRenderWidget(renderer)->widget();
        return widget && passGestureEventToWidget(gestureEvent, widget);
    }
    return false;
}

bool EventHandler::handleGestureScrollBegin(const PlatformGestureEvent& gestureEvent)
{
    Document* document = m_frame->document();
    RenderObject* documentRenderer = document->renderer();
    if (!documentRenderer)
        return false;

    FrameView* view = m_frame->view();
    if (!view)
        return false;

    LayoutPoint viewPoint = view->windowToContents(gestureEvent.position());
    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::DisallowShadowContent);
    HitTestResult result(viewPoint);
    document->renderView()->hitTest(request, result);

    m_lastHitTestResultOverWidget = result.isOverWidget(); 
    m_scrollGestureHandlingNode = result.innerNode();
    m_previousGestureScrolledNode = 0;

    Node* node = m_scrollGestureHandlingNode.get();
    if (node)
        passGestureEventToWidgetIfPossible(gestureEvent, node->renderer());
    
    return node && node->renderer();
}

bool EventHandler::handleGestureScrollUpdate(const PlatformGestureEvent& gestureEvent)
{
    FloatSize delta(gestureEvent.deltaX(), gestureEvent.deltaY());
    if (delta.isZero())
        return false;

    const float scaleFactor = m_frame->pageZoomFactor() * m_frame->frameScaleFactor();
    delta.scale(1 / scaleFactor, 1 / scaleFactor);

    Node* node = m_scrollGestureHandlingNode.get();
    if (!node)
        return sendScrollEventToView(gestureEvent, delta);

    // Ignore this event if the targeted node does not have a valid renderer.
    RenderObject* renderer = node->renderer();
    if (!renderer)
        return false;

    RefPtr<FrameView> protector(m_frame->view());

    // Try to send the event to the correct view.
    if (passGestureEventToWidgetIfPossible(gestureEvent, renderer))
        return true;

    Node* stopNode = 0;
    bool scrollShouldNotPropagate = gestureEvent.type() == PlatformEvent::GestureScrollUpdateWithoutPropagation;
    if (scrollShouldNotPropagate)
        stopNode = m_previousGestureScrolledNode.get();

    // First try to scroll the closest scrollable RenderBox ancestor of |node|.
    ScrollGranularity granularity = ScrollByPixel; 
    bool horizontalScroll = scrollNode(delta.width(), granularity, ScrollLeft, ScrollRight, node, &stopNode);
    bool verticalScroll = scrollNode(delta.height(), granularity, ScrollUp, ScrollDown, node, &stopNode);

    if (scrollShouldNotPropagate)
        m_previousGestureScrolledNode = stopNode;

    if (horizontalScroll || verticalScroll) {
        setFrameWasScrolledByUser();
        return true;
    }

    // Otherwise try to scroll the view.
    return sendScrollEventToView(gestureEvent, delta);
}

bool EventHandler::sendScrollEventToView(const PlatformGestureEvent& gestureEvent, const FloatSize& scaledDelta)
{
    FrameView* view = m_frame->view();
    if (!view)
        return false;

    const float tickDivisor = static_cast<float>(WheelEvent::TickMultiplier);
    IntPoint point(gestureEvent.position().x(), gestureEvent.position().y());
    IntPoint globalPoint(gestureEvent.globalPosition().x(), gestureEvent.globalPosition().y());
    PlatformWheelEvent syntheticWheelEvent(point, globalPoint,
        scaledDelta.width(), scaledDelta.height(), 
        scaledDelta.width() / tickDivisor, scaledDelta.height() / tickDivisor,
        ScrollByPixelWheelEvent,
        gestureEvent.shiftKey(), gestureEvent.ctrlKey(), gestureEvent.altKey(), gestureEvent.metaKey());
#if PLATFORM(MAC)
    syntheticWheelEvent.setHasPreciseScrollingDeltas(true);
#endif

    bool scrolledFrame = view->wheelEvent(syntheticWheelEvent);
    if (scrolledFrame)
        setFrameWasScrolledByUser();

    return scrolledFrame;
}

void EventHandler::clearGestureScrollNodes()
{
    m_scrollGestureHandlingNode = 0;
    m_previousGestureScrolledNode = 0;
}

bool EventHandler::isScrollbarHandlingGestures() const
{
    return m_scrollbarHandlingScrollGesture.get();
}
#endif // ENABLE(GESTURE_EVENTS)

#if ENABLE(TOUCH_ADJUSTMENT)
bool EventHandler::shouldApplyTouchAdjustment(const PlatformGestureEvent& event) const
{
    if (m_frame->settings() && !m_frame->settings()->touchAdjustmentEnabled())
        return false;
    return !event.area().isEmpty();
}


bool EventHandler::bestClickableNodeForTouchPoint(const IntPoint& touchCenter, const IntSize& touchRadius, IntPoint& targetPoint, Node*& targetNode)
{
    IntPoint hitTestPoint = m_frame->view()->windowToContents(touchCenter);
    HitTestResult result = hitTestResultAtPoint(hitTestPoint, HitTestRequest::ReadOnly | HitTestRequest::Active, touchRadius);

    IntRect touchRect(touchCenter - touchRadius, touchRadius + touchRadius);

    // FIXME: Should be able to handle targetNode being a shadow DOM node to avoid performing uncessary hit tests
    // in the case where further processing on the node is required. Returning the shadow ancestor prevents a
    // regression in touchadjustment/html-label.html. Some refinement is required to testing/internals to
    // handle targetNode being a shadow DOM node. 
    bool success = findBestClickableCandidate(targetNode, targetPoint, touchCenter, touchRect, result.rectBasedTestResult());
    if (success && targetNode)
        targetNode = targetNode->deprecatedShadowAncestorNode();
    return success;
}

bool EventHandler::bestContextMenuNodeForTouchPoint(const IntPoint& touchCenter, const IntSize& touchRadius, IntPoint& targetPoint, Node*& targetNode)
{
    IntPoint hitTestPoint = m_frame->view()->windowToContents(touchCenter);
    HitTestResult result = hitTestResultAtPoint(hitTestPoint, HitTestRequest::ReadOnly | HitTestRequest::Active, touchRadius);

    IntRect touchRect(touchCenter - touchRadius, touchRadius + touchRadius);
    return findBestContextMenuCandidate(targetNode, targetPoint, touchCenter, touchRect, result.rectBasedTestResult());
}

bool EventHandler::bestZoomableAreaForTouchPoint(const IntPoint& touchCenter, const IntSize& touchRadius, IntRect& targetArea, Node*& targetNode)
{
    IntPoint hitTestPoint = m_frame->view()->windowToContents(touchCenter);
    HitTestResult result = hitTestResultAtPoint(hitTestPoint, HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::DisallowShadowContent, touchRadius);

    IntRect touchRect(touchCenter - touchRadius, touchRadius + touchRadius);
    return findBestZoomableArea(targetNode, targetArea, touchCenter, touchRect, result.rectBasedTestResult());
}

bool EventHandler::adjustGesturePosition(const PlatformGestureEvent& gestureEvent, IntPoint& adjustedPoint)
{
    if (!shouldApplyTouchAdjustment(gestureEvent))
        return false;

    Node* targetNode = 0;
    switch (gestureEvent.type()) {
    case PlatformEvent::GestureTap:
    case PlatformEvent::GestureTapDown:
        bestClickableNodeForTouchPoint(gestureEvent.position(), IntSize(gestureEvent.area().width() / 2, gestureEvent.area().height() / 2), adjustedPoint, targetNode);
        break;
    case PlatformEvent::GestureLongPress:
    case PlatformEvent::GestureLongTap:
    case PlatformEvent::GestureTwoFingerTap:
        bestContextMenuNodeForTouchPoint(gestureEvent.position(), IntSize(gestureEvent.area().width() / 2, gestureEvent.area().height() / 2), adjustedPoint, targetNode);
        break;
    default:
        // FIXME: Implement handling for other types as needed.
        ASSERT_NOT_REACHED();
    }
    return targetNode;
}
#endif

#if ENABLE(CONTEXT_MENUS)
bool EventHandler::sendContextMenuEvent(const PlatformMouseEvent& event)
{
    Document* doc = m_frame->document();
    FrameView* v = m_frame->view();
    if (!v)
        return false;
    
    // Clear mouse press state to avoid initiating a drag while context menu is up.
    m_mousePressed = false;
    bool swallowEvent;
    LayoutPoint viewportPos = v->windowToContents(event.position());
    HitTestRequest request(HitTestRequest::Active | HitTestRequest::DisallowShadowContent);
    MouseEventWithHitTestResults mev = doc->prepareMouseEvent(request, viewportPos, event);

    if (m_frame->editor().behavior().shouldSelectOnContextualMenuClick()
        && !m_frame->selection()->contains(viewportPos)
        && !mev.scrollbar()
        // FIXME: In the editable case, word selection sometimes selects content that isn't underneath the mouse.
        // If the selection is non-editable, we do word selection to make it easier to use the contextual menu items
        // available for text selections.  But only if we're above text.
        && (m_frame->selection()->isContentEditable() || (mev.targetNode() && mev.targetNode()->isTextNode()))) {
        m_mouseDownMayStartSelect = true; // context menu events are always allowed to perform a selection
        selectClosestWordOrLinkFromMouseEvent(mev);
    }

    swallowEvent = !dispatchMouseEvent(eventNames().contextmenuEvent, mev.targetNode(), true, 0, event, false);
    
    return swallowEvent;
}

bool EventHandler::sendContextMenuEventForKey()
{
    FrameView* view = m_frame->view();
    if (!view)
        return false;

    Document* doc = m_frame->document();
    if (!doc)
        return false;

    // Clear mouse press state to avoid initiating a drag while context menu is up.
    m_mousePressed = false;

    static const int kContextMenuMargin = 1;

#if OS(WINDOWS) && !OS(WINCE)
    int rightAligned = ::GetSystemMetrics(SM_MENUDROPALIGNMENT);
#else
    int rightAligned = 0;
#endif
    IntPoint location;

    Element* focusedElement = doc->focusedElement();
    FrameSelection* selection = m_frame->selection();
    Position start = selection->selection().start();

    if (start.deprecatedNode() && (selection->rootEditableElement() || selection->isRange())) {
        RefPtr<Range> selectionRange = selection->toNormalizedRange();
        IntRect firstRect = m_frame->editor().firstRectForRange(selectionRange.get());

        int x = rightAligned ? firstRect.maxX() : firstRect.x();
        // In a multiline edit, firstRect.maxY() would endup on the next line, so -1.
        int y = firstRect.maxY() ? firstRect.maxY() - 1 : 0;
        location = IntPoint(x, y);
    } else if (focusedElement) {
        RenderBoxModelObject* box = focusedElement->renderBoxModelObject();
        if (!box)
            return false;
        IntRect clippedRect = box->pixelSnappedAbsoluteClippedOverflowRect();
        location = IntPoint(clippedRect.x(), clippedRect.maxY() - 1);
    } else {
        location = IntPoint(
            rightAligned ? view->contentsWidth() - kContextMenuMargin : kContextMenuMargin,
            kContextMenuMargin);
    }

    m_frame->view()->setCursor(pointerCursor());

    IntPoint position = view->contentsToRootView(location);
    IntPoint globalPosition = view->hostWindow()->rootViewToScreen(IntRect(position, IntSize())).location();

    Node* targetNode = doc->focusedElement();
    if (!targetNode)
        targetNode = doc;

    // Use the focused node as the target for hover and active.
    HitTestResult result(position);
    result.setInnerNode(targetNode);
    doc->updateHoverActiveState(HitTestRequest::Active | HitTestRequest::DisallowShadowContent, result.innerElement());

    // The contextmenu event is a mouse event even when invoked using the keyboard.
    // This is required for web compatibility.

#if OS(WINDOWS)
    PlatformEvent::Type eventType = PlatformEvent::MouseReleased;
#else
    PlatformEvent::Type eventType = PlatformEvent::MousePressed;
#endif

    PlatformMouseEvent mouseEvent(position, globalPosition, RightButton, eventType, 1, false, false, false, false, WTF::currentTime());

    return !dispatchMouseEvent(eventNames().contextmenuEvent, targetNode, true, 0, mouseEvent, false);
}

#if ENABLE(GESTURE_EVENTS)
bool EventHandler::sendContextMenuEventForGesture(const PlatformGestureEvent& event)
{
#if OS(WINDOWS)
    PlatformEvent::Type eventType = PlatformEvent::MouseReleased;
#else
    PlatformEvent::Type eventType = PlatformEvent::MousePressed;
#endif

    IntPoint adjustedPoint = event.position();
#if ENABLE(TOUCH_ADJUSTMENT)
    adjustGesturePosition(event, adjustedPoint);
#endif
    PlatformMouseEvent mouseEvent(adjustedPoint, event.globalPosition(), RightButton, eventType, 1, false, false, false, false, WTF::currentTime());
    // To simulate right-click behavior, we send a right mouse down and then
    // context menu event.
    handleMousePressEvent(mouseEvent);
    return sendContextMenuEvent(mouseEvent);
    // We do not need to send a corresponding mouse release because in case of
    // right-click, the context menu takes capture and consumes all events.
}
#endif // ENABLE(GESTURE_EVENTS)
#endif // ENABLE(CONTEXT_MENUS)

void EventHandler::scheduleHoverStateUpdate()
{
    if (!m_hoverTimer.isActive())
        m_hoverTimer.startOneShot(0);
}

void EventHandler::scheduleCursorUpdate()
{
    if (!m_cursorUpdateTimer.isActive())
        m_cursorUpdateTimer.startOneShot(cursorUpdateInterval);
}

void EventHandler::dispatchFakeMouseMoveEventSoon()
{
    if (m_mousePressed)
        return;

    if (m_mousePositionIsUnknown)
        return;

    Settings* settings = m_frame->settings();
    if (settings && !settings->deviceSupportsMouse())
        return;

    // If the content has ever taken longer than fakeMouseMoveShortInterval we
    // reschedule the timer and use a longer time. This will cause the content
    // to receive these moves only after the user is done scrolling, reducing
    // pauses during the scroll.
    if (m_maxMouseMovedDuration > fakeMouseMoveShortInterval) {
        if (m_fakeMouseMoveEventTimer.isActive())
            m_fakeMouseMoveEventTimer.stop();
        m_fakeMouseMoveEventTimer.startOneShot(fakeMouseMoveLongInterval);
    } else {
        if (!m_fakeMouseMoveEventTimer.isActive())
            m_fakeMouseMoveEventTimer.startOneShot(fakeMouseMoveShortInterval);
    }
}

void EventHandler::dispatchFakeMouseMoveEventSoonInQuad(const FloatQuad& quad)
{
    FrameView* view = m_frame->view();
    if (!view)
        return;

    if (!quad.containsPoint(view->windowToContents(m_lastKnownMousePosition)))
        return;

    dispatchFakeMouseMoveEventSoon();
}

void EventHandler::cancelFakeMouseMoveEvent()
{
    m_fakeMouseMoveEventTimer.stop();
}

void EventHandler::fakeMouseMoveEventTimerFired(Timer<EventHandler>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_fakeMouseMoveEventTimer);
    ASSERT(!m_mousePressed);

    Settings* settings = m_frame->settings();
    if (settings && !settings->deviceSupportsMouse())
        return;

    FrameView* view = m_frame->view();
    if (!view)
        return;

    if (!m_frame->page() || !m_frame->page()->isOnscreen() || !m_frame->page()->focusController()->isActive())
        return;

    bool shiftKey;
    bool ctrlKey;
    bool altKey;
    bool metaKey;
    PlatformKeyboardEvent::getCurrentModifierState(shiftKey, ctrlKey, altKey, metaKey);
    PlatformMouseEvent fakeMouseMoveEvent(m_lastKnownMousePosition, m_lastKnownMouseGlobalPosition, NoButton, PlatformEvent::MouseMoved, 0, shiftKey, ctrlKey, altKey, metaKey, currentTime());
    mouseMoved(fakeMouseMoveEvent);
}

void EventHandler::setResizingFrameSet(HTMLFrameSetElement* frameSet)
{
    m_frameSetBeingResized = frameSet;
}

void EventHandler::resizeLayerDestroyed()
{
    ASSERT(m_resizeLayer);
    m_resizeLayer = 0;
}

void EventHandler::hoverTimerFired(Timer<EventHandler>*)
{
    m_hoverTimer.stop();

    ASSERT(m_frame);
    ASSERT(m_frame->document());

    if (RenderView* renderer = m_frame->contentRenderer()) {
        if (FrameView* view = m_frame->view()) {
            HitTestRequest request(HitTestRequest::Move | HitTestRequest::DisallowShadowContent);
            HitTestResult result(view->windowToContents(m_lastKnownMousePosition));
            renderer->hitTest(request, result);
            m_frame->document()->updateHoverActiveState(request, result.innerElement());
        }
    }
}

bool EventHandler::handleAccessKey(const PlatformKeyboardEvent& evt)
{
    // FIXME: Ignoring the state of Shift key is what neither IE nor Firefox do.
    // IE matches lower and upper case access keys regardless of Shift key state - but if both upper and
    // lower case variants are present in a document, the correct element is matched based on Shift key state.
    // Firefox only matches an access key if Shift is not pressed, and does that case-insensitively.
    ASSERT(!(accessKeyModifiers() & PlatformEvent::ShiftKey));
    if ((evt.modifiers() & ~PlatformEvent::ShiftKey) != accessKeyModifiers())
        return false;
    String key = evt.unmodifiedText();
    Element* elem = m_frame->document()->getElementByAccessKey(key.lower());
    if (!elem)
        return false;
    elem->accessKeyAction(false);
    return true;
}

#if !PLATFORM(MAC)
bool EventHandler::needsKeyboardEventDisambiguationQuirks() const
{
    return false;
}
#endif

#if ENABLE(FULLSCREEN_API)
bool EventHandler::isKeyEventAllowedInFullScreen(const PlatformKeyboardEvent& keyEvent) const
{
    Document* document = m_frame->document();
    if (document->webkitFullScreenKeyboardInputAllowed())
        return true;

    if (keyEvent.type() == PlatformKeyboardEvent::Char) {
        if (keyEvent.text().length() != 1)
            return false;
        UChar character = keyEvent.text()[0];
        return character == ' ';
    }

    int keyCode = keyEvent.windowsVirtualKeyCode();
    return (keyCode >= VK_BACK && keyCode <= VK_CAPITAL)
        || (keyCode >= VK_SPACE && keyCode <= VK_DELETE)
        || (keyCode >= VK_OEM_1 && keyCode <= VK_OEM_PLUS)
        || (keyCode >= VK_MULTIPLY && keyCode <= VK_OEM_8);
}
#endif

bool EventHandler::keyEvent(const PlatformKeyboardEvent& initialKeyEvent)
{
    RefPtr<FrameView> protector(m_frame->view()); 

#if ENABLE(FULLSCREEN_API)
    if (m_frame->document()->webkitIsFullScreen() && !isKeyEventAllowedInFullScreen(initialKeyEvent))
        return false;
#endif

    if (initialKeyEvent.windowsVirtualKeyCode() == VK_CAPITAL)
        capsLockStateMayHaveChanged();

#if ENABLE(PAN_SCROLLING)
    if (Page* page = m_frame->page()) {
        if (page->mainFrame()->eventHandler()->panScrollInProgress()) {
            // If a key is pressed while the panScroll is in progress then we want to stop
            if (initialKeyEvent.type() == PlatformEvent::KeyDown || initialKeyEvent.type() == PlatformEvent::RawKeyDown) 
                stopAutoscrollTimer();

            // If we were in panscroll mode, we swallow the key event
            return true;
        }
    }
#endif

    // Check for cases where we are too early for events -- possible unmatched key up
    // from pressing return in the location bar.
    RefPtr<Node> node = eventTargetNodeForDocument(m_frame->document());
    if (!node)
        return false;

    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);
    UserTypingGestureIndicator typingGestureIndicator(m_frame);

    if (FrameView* view = m_frame->view())
        view->resetDeferredRepaintDelay();

    // FIXME (bug 68185): this call should be made at another abstraction layer
    m_frame->loader()->resetMultipleFormSubmissionProtection();

    // In IE, access keys are special, they are handled after default keydown processing, but cannot be canceled - this is hard to match.
    // On Mac OS X, we process them before dispatching keydown, as the default keydown handler implements Emacs key bindings, which may conflict
    // with access keys. Then we dispatch keydown, but suppress its default handling.
    // On Windows, WebKit explicitly calls handleAccessKey() instead of dispatching a keypress event for WM_SYSCHAR messages.
    // Other platforms currently match either Mac or Windows behavior, depending on whether they send combined KeyDown events.
    bool matchedAnAccessKey = false;
    if (initialKeyEvent.type() == PlatformEvent::KeyDown)
        matchedAnAccessKey = handleAccessKey(initialKeyEvent);

    // FIXME: it would be fair to let an input method handle KeyUp events before DOM dispatch.
    if (initialKeyEvent.type() == PlatformEvent::KeyUp || initialKeyEvent.type() == PlatformEvent::Char)
        return !node->dispatchKeyEvent(initialKeyEvent);

    bool backwardCompatibilityMode = needsKeyboardEventDisambiguationQuirks();

    PlatformKeyboardEvent keyDownEvent = initialKeyEvent;    
    if (keyDownEvent.type() != PlatformEvent::RawKeyDown)
        keyDownEvent.disambiguateKeyDownEvent(PlatformEvent::RawKeyDown, backwardCompatibilityMode);
    RefPtr<KeyboardEvent> keydown = KeyboardEvent::create(keyDownEvent, m_frame->document()->defaultView());
    if (matchedAnAccessKey)
        keydown->setDefaultPrevented(true);
    keydown->setTarget(node);

    if (initialKeyEvent.type() == PlatformEvent::RawKeyDown) {
        node->dispatchEvent(keydown, IGNORE_EXCEPTION);
        // If frame changed as a result of keydown dispatch, then return true to avoid sending a subsequent keypress message to the new frame.
        bool changedFocusedFrame = m_frame->page() && m_frame != m_frame->page()->focusController()->focusedOrMainFrame();
        return keydown->defaultHandled() || keydown->defaultPrevented() || changedFocusedFrame;
    }

    // Run input method in advance of DOM event handling.  This may result in the IM
    // modifying the page prior the keydown event, but this behaviour is necessary
    // in order to match IE:
    // 1. preventing default handling of keydown and keypress events has no effect on IM input;
    // 2. if an input method handles the event, its keyCode is set to 229 in keydown event.
    m_frame->editor().handleInputMethodKeydown(keydown.get());
    
    bool handledByInputMethod = keydown->defaultHandled();
    
    if (handledByInputMethod) {
        keyDownEvent.setWindowsVirtualKeyCode(CompositionEventKeyCode);
        keydown = KeyboardEvent::create(keyDownEvent, m_frame->document()->defaultView());
        keydown->setTarget(node);
        keydown->setDefaultHandled();
    }

    node->dispatchEvent(keydown, IGNORE_EXCEPTION);
    // If frame changed as a result of keydown dispatch, then return early to avoid sending a subsequent keypress message to the new frame.
    bool changedFocusedFrame = m_frame->page() && m_frame != m_frame->page()->focusController()->focusedOrMainFrame();
    bool keydownResult = keydown->defaultHandled() || keydown->defaultPrevented() || changedFocusedFrame;
    if (handledByInputMethod || (keydownResult && !backwardCompatibilityMode))
        return keydownResult;
    
    // Focus may have changed during keydown handling, so refetch node.
    // But if we are dispatching a fake backward compatibility keypress, then we pretend that the keypress happened on the original node.
    if (!keydownResult) {
        node = eventTargetNodeForDocument(m_frame->document());
        if (!node)
            return false;
    }

    PlatformKeyboardEvent keyPressEvent = initialKeyEvent;
    keyPressEvent.disambiguateKeyDownEvent(PlatformEvent::Char, backwardCompatibilityMode);
    if (keyPressEvent.text().isEmpty())
        return keydownResult;
    RefPtr<KeyboardEvent> keypress = KeyboardEvent::create(keyPressEvent, m_frame->document()->defaultView());
    keypress->setTarget(node);
    if (keydownResult)
        keypress->setDefaultPrevented(true);
#if PLATFORM(MAC)
    keypress->keypressCommands() = keydown->keypressCommands();
#endif
    node->dispatchEvent(keypress, IGNORE_EXCEPTION);

    return keydownResult || keypress->defaultPrevented() || keypress->defaultHandled();
}

static FocusDirection focusDirectionForKey(const AtomicString& keyIdentifier)
{
    DEFINE_STATIC_LOCAL(AtomicString, Down, ("Down", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, Up, ("Up", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, Left, ("Left", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, Right, ("Right", AtomicString::ConstructFromLiteral));

    FocusDirection retVal = FocusDirectionNone;

    if (keyIdentifier == Down)
        retVal = FocusDirectionDown;
    else if (keyIdentifier == Up)
        retVal = FocusDirectionUp;
    else if (keyIdentifier == Left)
        retVal = FocusDirectionLeft;
    else if (keyIdentifier == Right)
        retVal = FocusDirectionRight;

    return retVal;
}

static void handleKeyboardSelectionMovement(FrameSelection* selection, KeyboardEvent* event)
{
    if (!event)
        return;

    bool isOptioned = event->getModifierState("Alt");
    bool isCommanded = event->getModifierState("Meta");

    SelectionDirection direction = DirectionForward;
    TextGranularity granularity = CharacterGranularity;

    switch (focusDirectionForKey(event->keyIdentifier())) {
    case FocusDirectionNone:
        return;
    case FocusDirectionForward:
    case FocusDirectionBackward:
        ASSERT_NOT_REACHED();
        return;
    case FocusDirectionUp:
        direction = DirectionBackward;
        granularity = isCommanded ? DocumentBoundary : LineGranularity;
        break;
    case FocusDirectionDown:
        direction = DirectionForward;
        granularity = isCommanded ? DocumentBoundary : LineGranularity;
        break;
    case FocusDirectionLeft:
        direction = DirectionLeft;
        granularity = (isCommanded) ? LineBoundary : (isOptioned) ? WordGranularity : CharacterGranularity;
        break;
    case FocusDirectionRight:
        direction = DirectionRight;
        granularity = (isCommanded) ? LineBoundary : (isOptioned) ? WordGranularity : CharacterGranularity;
        break;
    }

    FrameSelection::EAlteration alternation = event->getModifierState("Shift") ? FrameSelection::AlterationExtend : FrameSelection::AlterationMove;
    selection->modify(alternation, direction, granularity, UserTriggered);
    event->setDefaultHandled();
}
    
void EventHandler::defaultKeyboardEventHandler(KeyboardEvent* event)
{
    if (event->type() == eventNames().keydownEvent) {
        m_frame->editor().handleKeyboardEvent(event);
        if (event->defaultHandled())
            return;
        if (event->keyIdentifier() == "U+0009")
            defaultTabEventHandler(event);
        else if (event->keyIdentifier() == "U+0008")
            defaultBackspaceEventHandler(event);
        else {
            FocusDirection direction = focusDirectionForKey(event->keyIdentifier());
            if (direction != FocusDirectionNone)
                defaultArrowEventHandler(direction, event);
        }

        // provides KB navigation and selection for enhanced accessibility users
        if (AXObjectCache::accessibilityEnhancedUserInterfaceEnabled())
            handleKeyboardSelectionMovement(m_frame->selection(), event);
    }
    if (event->type() == eventNames().keypressEvent) {
        m_frame->editor().handleKeyboardEvent(event);
        if (event->defaultHandled())
            return;
        if (event->charCode() == ' ')
            defaultSpaceEventHandler(event);
    }
}

#if ENABLE(DRAG_SUPPORT)
bool EventHandler::dragHysteresisExceeded(const IntPoint& floatDragViewportLocation) const
{
    FloatPoint dragViewportLocation(floatDragViewportLocation.x(), floatDragViewportLocation.y());
    return dragHysteresisExceeded(dragViewportLocation);
}

bool EventHandler::dragHysteresisExceeded(const FloatPoint& dragViewportLocation) const
{
    FrameView* view = m_frame->view();
    if (!view)
        return false;
    IntPoint dragLocation = view->windowToContents(flooredIntPoint(dragViewportLocation));
    IntSize delta = dragLocation - m_mouseDownPos;
    
    int threshold = GeneralDragHysteresis;
    switch (dragState().type) {
    case DragSourceActionSelection:
        threshold = TextDragHysteresis;
        break;
    case DragSourceActionImage:
        threshold = ImageDragHysteresis;
        break;
    case DragSourceActionLink:
        threshold = LinkDragHysteresis;
        break;
    case DragSourceActionDHTML:
        break;
    case DragSourceActionNone:
    case DragSourceActionAny:
        ASSERT_NOT_REACHED();
    }
    
    return abs(delta.width()) >= threshold || abs(delta.height()) >= threshold;
}
    
void EventHandler::freeClipboard()
{
    if (!dragState().clipboard)
        return;
    dragState().clipboard->setAccessPolicy(ClipboardNumb);
    dragState().clipboard = 0;
}

void EventHandler::dragSourceEndedAt(const PlatformMouseEvent& event, DragOperation operation)
{
    // Send a hit test request so that RenderLayer gets a chance to update the :hover and :active pseudoclasses.
    HitTestRequest request(HitTestRequest::Release | HitTestRequest::DisallowShadowContent);
    prepareMouseEvent(request, event);

    if (dragState().source && dragState().shouldDispatchEvents) {
        dragState().clipboard->setDestinationOperation(operation);
        // For now we don't care if event handler cancels default behavior, since there is no default behavior.
        dispatchDragSrcEvent(eventNames().dragendEvent, event);
    }
    freeClipboard();
    dragState().source = 0;
    // In case the drag was ended due to an escape key press we need to ensure
    // that consecutive mousemove events don't reinitiate the drag and drop.
    m_mouseDownMayStartDrag = false;
}

void EventHandler::updateDragStateAfterEditDragIfNeeded(Element* rootEditableElement)
{
    // If inserting the dragged contents removed the drag source, we still want to fire dragend at the root editable element.
    if (dragState().source && !dragState().source->inDocument())
        dragState().source = rootEditableElement;
}

// Return value indicates if we should continue "default processing", i.e., whether event handler canceled.
bool EventHandler::dispatchDragSrcEvent(const AtomicString& eventType, const PlatformMouseEvent& event)
{
    return !dispatchDragEvent(eventType, dragState().source.get(), event, dragState().clipboard.get());
}
    
static bool ExactlyOneBitSet(DragSourceAction n)
{
    return n && !(n & (n - 1));
}

bool EventHandler::handleDrag(const MouseEventWithHitTestResults& event, CheckDragHysteresis checkDragHysteresis)
{
    if (event.event().button() != LeftButton || event.event().type() != PlatformEvent::MouseMoved) {
        // If we allowed the other side of the bridge to handle a drag
        // last time, then m_mousePressed might still be set. So we
        // clear it now to make sure the next move after a drag
        // doesn't look like a drag.
        m_mousePressed = false;
        return false;
    }
    
    if (eventLoopHandleMouseDragged(event))
        return true;
    
    // Careful that the drag starting logic stays in sync with eventMayStartDrag()
    
    if (m_mouseDownMayStartDrag && !dragState().source) {
        dragState().shouldDispatchEvents = (updateDragSourceActionsAllowed() & DragSourceActionDHTML);

        // try to find an element that wants to be dragged
        HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::DisallowShadowContent);
        HitTestResult result(m_mouseDownPos);
        m_frame->contentRenderer()->hitTest(request, result);
        if (m_frame->page())
            dragState().source = m_frame->page()->dragController()->draggableElement(m_frame, result.innerElement(), m_mouseDownPos, dragState());
        
        if (!dragState().source)
            m_mouseDownMayStartDrag = false; // no element is draggable
        else
            m_dragMayStartSelectionInstead = (dragState().type & DragSourceActionSelection);
    }
    
    // For drags starting in the selection, the user must wait between the mousedown and mousedrag,
    // or else we bail on the dragging stuff and allow selection to occur
    if (m_mouseDownMayStartDrag && m_dragMayStartSelectionInstead && (dragState().type & DragSourceActionSelection) && event.event().timestamp() - m_mouseDownTimestamp < TextDragDelay) {
        ASSERT(event.event().type() == PlatformEvent::MouseMoved);
        if ((dragState().type & DragSourceActionImage)) {
            // ... unless the mouse is over an image, then we start dragging just the image
            dragState().type = DragSourceActionImage;
        } else if (!(dragState().type & (DragSourceActionDHTML | DragSourceActionLink))) {
            // ... but only bail if we're not over an unselectable element.
            m_mouseDownMayStartDrag = false;
            dragState().source = 0;
            // ... but if this was the first click in the window, we don't even want to start selection
            if (eventActivatedView(event.event()))
                m_mouseDownMayStartSelect = false;
        } else {
            // Prevent the following case from occuring:
            // 1. User starts a drag immediately after mouse down over an unselectable element.
            // 2. We enter this block and decided that since we're over an unselectable element, don't cancel the drag.
            // 3. The drag gets resolved as a potential selection drag below /but/ we haven't exceeded the drag hysteresis yet.
            // 4. We enter this block again, and since it's now marked as a selection drag, we cancel the drag.
            m_dragMayStartSelectionInstead = false;
        }
    }
    
    if (!m_mouseDownMayStartDrag)
        return !mouseDownMayStartSelect() && !m_mouseDownMayStartAutoscroll;
    
    if (!ExactlyOneBitSet(dragState().type)) {
        ASSERT((dragState().type & DragSourceActionSelection));
        ASSERT((dragState().type & ~DragSourceActionSelection) == DragSourceActionDHTML
            || (dragState().type & ~DragSourceActionSelection) == DragSourceActionImage
            || (dragState().type & ~DragSourceActionSelection) == DragSourceActionLink);
        dragState().type = DragSourceActionSelection;
    }

    // We are starting a text/image/url drag, so the cursor should be an arrow
    if (FrameView* view = m_frame->view()) {
        // FIXME <rdar://7577595>: Custom cursors aren't supported during drag and drop (default to pointer).
        view->setCursor(pointerCursor());
    }

    if (checkDragHysteresis == ShouldCheckDragHysteresis && !dragHysteresisExceeded(event.event().position()))
        return true;
    
    // Once we're past the hysteresis point, we don't want to treat this gesture as a click
    invalidateClick();
    
    DragOperation srcOp = DragOperationNone;      
    
    // This does work only if we missed a dragEnd. Do it anyway, just to make sure the old clipboard gets numbed.
    freeClipboard();

    dragState().clipboard = createDraggingClipboard();
    
    if (dragState().shouldDispatchEvents) {
        // Check to see if the is a DOM based drag. If it is, get the DOM specified drag image and offset.
        if (dragState().type == DragSourceActionDHTML) {
            if (RenderObject* renderer = dragState().source->renderer()) {
                // FIXME: This doesn't work correctly with transforms.
                FloatPoint absPos = renderer->localToAbsolute();
                IntSize delta = m_mouseDownPos - roundedIntPoint(absPos);
                dragState().clipboard->setDragImage(dragState().source.get(), delta.width(), delta.height());
            } else {
                // The renderer has disappeared, this can happen if the onStartDrag handler has hidden
                // the element in some way.  In this case we just kill the drag.
                m_mouseDownMayStartDrag = false;
                goto cleanupDrag;
            }
        } 
        
        m_mouseDownMayStartDrag = dispatchDragSrcEvent(eventNames().dragstartEvent, m_mouseDown)
            && !m_frame->selection()->isInPasswordField();
        
        // Invalidate clipboard here against anymore pasteboard writing for security.  The drag
        // image can still be changed as we drag, but not the pasteboard data.
        dragState().clipboard->setAccessPolicy(ClipboardImageWritable);
        
        if (m_mouseDownMayStartDrag) {
            // Gather values from DHTML element, if it set any.
            srcOp = dragState().clipboard->sourceOperation();
            
            // Yuck, a draggedImage:moveTo: message can be fired as a result of kicking off the
            // drag with dragImage! Because of that dumb reentrancy, we may think we've not
            // started the drag when that happens. So we have to assume it's started before we kick it off.
            dragState().clipboard->setDragHasStarted();
        }
    }
    
    if (m_mouseDownMayStartDrag) {
        Page* page = m_frame->page();
        DragController* dragController = page ? page->dragController() : 0;
        m_didStartDrag = dragController && dragController->startDrag(m_frame, dragState(), srcOp, event.event(), m_mouseDownPos);
        // In WebKit2 we could re-enter this code and start another drag.
        // On OS X this causes problems with the ownership of the pasteboard and the promised types.
        if (m_didStartDrag) {
            m_mouseDownMayStartDrag = false;
            return true;
        }
        if (dragState().source && dragState().shouldDispatchEvents) {
            // Drag was canned at the last minute. We owe dragSource a dragend event.
            dispatchDragSrcEvent(eventNames().dragendEvent, event.event());
            m_mouseDownMayStartDrag = false;
        }
    } 

cleanupDrag:
    if (!m_mouseDownMayStartDrag) {
        // Something failed to start the drag, clean up.
        freeClipboard();
        dragState().source = 0;
    }
    
    // No more default handling (like selection), whether we're past the hysteresis bounds or not
    return true;
}
#endif // ENABLE(DRAG_SUPPORT)
  
bool EventHandler::handleTextInputEvent(const String& text, Event* underlyingEvent, TextEventInputType inputType)
{
    // Platforms should differentiate real commands like selectAll from text input in disguise (like insertNewline),
    // and avoid dispatching text input events from keydown default handlers.
    ASSERT(!underlyingEvent || !underlyingEvent->isKeyboardEvent() || static_cast<KeyboardEvent*>(underlyingEvent)->type() == eventNames().keypressEvent);

    if (!m_frame)
        return false;

    EventTarget* target;
    if (underlyingEvent)
        target = underlyingEvent->target();
    else
        target = eventTargetNodeForDocument(m_frame->document());
    if (!target)
        return false;
    
    if (FrameView* view = m_frame->view())
        view->resetDeferredRepaintDelay();

    RefPtr<TextEvent> event = TextEvent::create(m_frame->document()->domWindow(), text, inputType);
    event->setUnderlyingEvent(underlyingEvent);

    target->dispatchEvent(event, IGNORE_EXCEPTION);
    return event->defaultHandled();
}
    
bool EventHandler::isKeyboardOptionTab(KeyboardEvent* event)
{
    return event
        && (event->type() == eventNames().keydownEvent || event->type() == eventNames().keypressEvent)
        && event->altKey()
        && event->keyIdentifier() == "U+0009";    
}

bool EventHandler::eventInvertsTabsToLinksClientCallResult(KeyboardEvent* event)
{
#if PLATFORM(MAC) || PLATFORM(QT) || PLATFORM(EFL)
    return EventHandler::isKeyboardOptionTab(event);
#else
    return false;
#endif
}

bool EventHandler::tabsToLinks(KeyboardEvent* event) const
{
    // FIXME: This function needs a better name. It can be called for keypresses other than Tab when spatial navigation is enabled.

    Page* page = m_frame->page();
    if (!page)
        return false;

    bool tabsToLinksClientCallResult = page->chrome().client()->keyboardUIMode() & KeyboardAccessTabsToLinks;
    return eventInvertsTabsToLinksClientCallResult(event) ? !tabsToLinksClientCallResult : tabsToLinksClientCallResult;
}

void EventHandler::defaultTextInputEventHandler(TextEvent* event)
{
    if (m_frame->editor().handleTextEvent(event))
        event->setDefaultHandled();
}


void EventHandler::defaultSpaceEventHandler(KeyboardEvent* event)
{
    ASSERT(event->type() == eventNames().keypressEvent);

    if (event->ctrlKey() || event->metaKey() || event->altKey() || event->altGraphKey())
        return;

    ScrollLogicalDirection direction = event->shiftKey() ? ScrollBlockDirectionBackward : ScrollBlockDirectionForward;
    if (logicalScrollOverflow(direction, ScrollByPage)) {
        event->setDefaultHandled();
        return;
    }

    FrameView* view = m_frame->view();
    if (!view)
        return;

    if (view->logicalScroll(direction, ScrollByPage))
        event->setDefaultHandled();
}

void EventHandler::defaultBackspaceEventHandler(KeyboardEvent* event)
{
    ASSERT(event->type() == eventNames().keydownEvent);

    if (event->ctrlKey() || event->metaKey() || event->altKey() || event->altGraphKey())
        return;

    if (!m_frame->editor().behavior().shouldNavigateBackOnBackspace())
        return;
    
    Page* page = m_frame->page();
    if (!page)
        return;

    if (!m_frame->settings()->backspaceKeyNavigationEnabled())
        return;
    
    bool handledEvent = false;

    if (event->shiftKey())
        handledEvent = page->goForward();
    else
        handledEvent = page->goBack();

    if (handledEvent)
        event->setDefaultHandled();
}


void EventHandler::defaultArrowEventHandler(FocusDirection focusDirection, KeyboardEvent* event)
{
    ASSERT(event->type() == eventNames().keydownEvent);

    if (event->ctrlKey() || event->metaKey() || event->altGraphKey() || event->shiftKey())
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (!isSpatialNavigationEnabled(m_frame))
        return;

    // Arrows and other possible directional navigation keys can be used in design
    // mode editing.
    if (m_frame->document()->inDesignMode())
        return;

    if (page->focusController()->advanceFocus(focusDirection, event))
        event->setDefaultHandled();
}

void EventHandler::defaultTabEventHandler(KeyboardEvent* event)
{
    ASSERT(event->type() == eventNames().keydownEvent);

    // We should only advance focus on tabs if no special modifier keys are held down.
    if (event->ctrlKey() || event->metaKey() || event->altGraphKey())
        return;

    Page* page = m_frame->page();
    if (!page)
        return;
    if (!page->tabKeyCyclesThroughElements())
        return;

    FocusDirection focusDirection = event->shiftKey() ? FocusDirectionBackward : FocusDirectionForward;

    // Tabs can be used in design mode editing.
    if (m_frame->document()->inDesignMode())
        return;

    if (page->focusController()->advanceFocus(focusDirection, event))
        event->setDefaultHandled();
}

void EventHandler::capsLockStateMayHaveChanged()
{
    Document* d = m_frame->document();
    if (Element* element = d->focusedElement()) {
        if (RenderObject* r = element->renderer()) {
            if (r->isTextField())
                toRenderTextControlSingleLine(r)->capsLockStateMayHaveChanged();
        }
    }
}

void EventHandler::sendScrollEvent()
{
    setFrameWasScrolledByUser();
    if (m_frame->view() && m_frame->document())
        m_frame->document()->eventQueue()->enqueueOrDispatchScrollEvent(m_frame->document(), DocumentEventQueue::ScrollEventDocumentTarget);
}

void EventHandler::setFrameWasScrolledByUser()
{
    FrameView* v = m_frame->view();
    if (v)
        v->setWasScrolledByUser(true);
}

bool EventHandler::passMousePressEventToScrollbar(MouseEventWithHitTestResults& mev, Scrollbar* scrollbar)
{
    if (!scrollbar || !scrollbar->enabled())
        return false;
    setFrameWasScrolledByUser();
    return scrollbar->mouseDown(mev.event());
}

// If scrollbar (under mouse) is different from last, send a mouse exited. Set
// last to scrollbar if setLast is true; else set last to 0.
void EventHandler::updateLastScrollbarUnderMouse(Scrollbar* scrollbar, bool setLast)
{
    if (m_lastScrollbarUnderMouse != scrollbar) {
        // Send mouse exited to the old scrollbar.
        if (m_lastScrollbarUnderMouse)
            m_lastScrollbarUnderMouse->mouseExited();

        // Send mouse entered if we're setting a new scrollbar.
        if (scrollbar && setLast)
            scrollbar->mouseEntered();

        m_lastScrollbarUnderMouse = setLast ? scrollbar : 0;
    }
}

#if ENABLE(TOUCH_EVENTS)

static const AtomicString& eventNameForTouchPointState(PlatformTouchPoint::State state)
{
    switch (state) {
    case PlatformTouchPoint::TouchReleased:
        return eventNames().touchendEvent;
    case PlatformTouchPoint::TouchCancelled:
        return eventNames().touchcancelEvent;
    case PlatformTouchPoint::TouchPressed:
        return eventNames().touchstartEvent;
    case PlatformTouchPoint::TouchMoved:
        return eventNames().touchmoveEvent;
    case PlatformTouchPoint::TouchStationary:
        // TouchStationary state is not converted to touch events, so fall through to assert.
    default:
        ASSERT_NOT_REACHED();
        return emptyAtom;
    }
}

HitTestResult EventHandler::hitTestResultInFrame(Frame* frame, const LayoutPoint& point, HitTestRequest::HitTestRequestType hitType)
{
    HitTestResult result(point);

    if (!frame || !frame->contentRenderer())
        return result;
    if (frame->view()) {
        IntRect rect = frame->view()->visibleContentRect();
        if (!rect.contains(roundedIntPoint(point)))
            return result;
    }
    frame->contentRenderer()->hitTest(HitTestRequest(hitType), result);
    return result;
}

bool EventHandler::handleTouchEvent(const PlatformTouchEvent& event)
{
    // First build up the lists to use for the 'touches', 'targetTouches' and 'changedTouches' attributes
    // in the JS event. See http://www.sitepen.com/blog/2008/07/10/touching-and-gesturing-on-the-iphone/
    // for an overview of how these lists fit together.

    // Holds the complete set of touches on the screen and will be used as the 'touches' list in the JS event.
    RefPtr<TouchList> touches = TouchList::create();

    // A different view on the 'touches' list above, filtered and grouped by event target. Used for the
    // 'targetTouches' list in the JS event.
    typedef HashMap<EventTarget*, RefPtr<TouchList> > TargetTouchesMap;
    TargetTouchesMap touchesByTarget;

    // Array of touches per state, used to assemble the 'changedTouches' list in the JS event.
    typedef HashSet<RefPtr<EventTarget> > EventTargetSet;
    struct {
        // The touches corresponding to the particular change state this struct instance represents.
        RefPtr<TouchList> m_touches;
        // Set of targets involved in m_touches.
        EventTargetSet m_targets;
    } changedTouches[PlatformTouchPoint::TouchStateEnd];

    const Vector<PlatformTouchPoint>& points = event.touchPoints();

    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);

    unsigned i;
    bool freshTouchEvents = true;
    bool allTouchReleased = true;
    for (i = 0; i < points.size(); ++i) {
        const PlatformTouchPoint& point = points[i];
        if (point.state() != PlatformTouchPoint::TouchPressed)
            freshTouchEvents = false;
        if (point.state() != PlatformTouchPoint::TouchReleased && point.state() != PlatformTouchPoint::TouchCancelled)
            allTouchReleased = false;
    }

    for (i = 0; i < points.size(); ++i) {
        const PlatformTouchPoint& point = points[i];
        PlatformTouchPoint::State pointState = point.state();
        LayoutPoint pagePoint = documentPointForWindowPoint(m_frame, point.pos());

        HitTestRequest::HitTestRequestType hitType = HitTestRequest::TouchEvent;
        // The HitTestRequest types used for mouse events map quite adequately
        // to touch events. Note that in addition to meaning that the hit test
        // should affect the active state of the current node if necessary,
        // HitTestRequest::Active signifies that the hit test is taking place
        // with the mouse (or finger in this case) being pressed.
        switch (pointState) {
        case PlatformTouchPoint::TouchPressed:
            hitType |= HitTestRequest::Active;
            break;
        case PlatformTouchPoint::TouchMoved:
            hitType |= HitTestRequest::Active | HitTestRequest::Move | HitTestRequest::ReadOnly;
            break;
        case PlatformTouchPoint::TouchReleased:
        case PlatformTouchPoint::TouchCancelled:
            hitType |= HitTestRequest::Release;
            break;
        case PlatformTouchPoint::TouchStationary:
            hitType |= HitTestRequest::Active | HitTestRequest::ReadOnly;
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }

        if (shouldGesturesTriggerActive())
            hitType |= HitTestRequest::ReadOnly;

        // Increment the platform touch id by 1 to avoid storing a key of 0 in the hashmap.
        unsigned touchPointTargetKey = point.id() + 1;
        RefPtr<EventTarget> touchTarget;
        if (pointState == PlatformTouchPoint::TouchPressed) {
            HitTestResult result;
            if (freshTouchEvents) {
                result = hitTestResultAtPoint(pagePoint, hitType);
                m_originatingTouchPointTargetKey = touchPointTargetKey;
            } else if (m_originatingTouchPointDocument.get() && m_originatingTouchPointDocument->frame()) {
                LayoutPoint pagePointInOriginatingDocument = documentPointForWindowPoint(m_originatingTouchPointDocument->frame(), point.pos());
                result = hitTestResultInFrame(m_originatingTouchPointDocument->frame(), pagePointInOriginatingDocument, hitType);
                if (!result.innerNode())
                    continue;
            } else
                continue;

            // FIXME: Is the following code different from just calling innerElement?
            Node* node = result.innerNode();
            ASSERT(node);
            // Touch events should not go to text nodes
            if (node->isTextNode())
                node = EventPathWalker::parent(node);

            if (InspectorInstrumentation::handleTouchEvent(m_frame->page(), node))
                return true;

            Document* doc = node->document();
            // Record the originating touch document even if it does not have a touch listener.
            if (freshTouchEvents) {
                m_originatingTouchPointDocument = doc;
                freshTouchEvents = false;
            }
            if (!doc)
                continue;
            if (!doc->hasTouchEventHandlers())
                continue;
            m_originatingTouchPointTargets.set(touchPointTargetKey, node);
            touchTarget = node;
        } else if (pointState == PlatformTouchPoint::TouchReleased || pointState == PlatformTouchPoint::TouchCancelled) {
            // No need to perform a hit-test since we only need to unset :hover and :active states.
            if (!shouldGesturesTriggerActive() && allTouchReleased)
                m_frame->document()->updateHoverActiveState(hitType, 0);
            if (touchPointTargetKey == m_originatingTouchPointTargetKey)
                m_originatingTouchPointTargetKey = 0;

            // The target should be the original target for this touch, so get it from the hashmap. As it's a release or cancel
            // we also remove it from the map.
            touchTarget = m_originatingTouchPointTargets.take(touchPointTargetKey);
        } else
            // No hittest is performed on move or stationary, since the target is not allowed to change anyway.
            touchTarget = m_originatingTouchPointTargets.get(touchPointTargetKey);

        if (!touchTarget.get())
            continue;
        Document* doc = touchTarget->toNode()->document();
        if (!doc)
            continue;
        if (!doc->hasTouchEventHandlers())
            continue;
        Frame* targetFrame = doc->frame();
        if (!targetFrame)
            continue;

        if (m_frame != targetFrame) {
            // pagePoint should always be relative to the target elements containing frame.
            pagePoint = documentPointForWindowPoint(targetFrame, point.pos());
        }

        float scaleFactor = targetFrame->pageZoomFactor() * targetFrame->frameScaleFactor();

        int adjustedPageX = lroundf(pagePoint.x() / scaleFactor);
        int adjustedPageY = lroundf(pagePoint.y() / scaleFactor);

        RefPtr<Touch> touch = Touch::create(targetFrame, touchTarget.get(), point.id(),
                                            point.screenPos().x(), point.screenPos().y(),
                                            adjustedPageX, adjustedPageY,
                                            point.radiusX(), point.radiusY(), point.rotationAngle(), point.force());

        // Ensure this target's touch list exists, even if it ends up empty, so it can always be passed to TouchEvent::Create below.
        TargetTouchesMap::iterator targetTouchesIterator = touchesByTarget.find(touchTarget.get());
        if (targetTouchesIterator == touchesByTarget.end())
            targetTouchesIterator = touchesByTarget.set(touchTarget.get(), TouchList::create()).iterator;

        // touches and targetTouches should only contain information about touches still on the screen, so if this point is
        // released or cancelled it will only appear in the changedTouches list.
        if (pointState != PlatformTouchPoint::TouchReleased && pointState != PlatformTouchPoint::TouchCancelled) {
            touches->append(touch);
            targetTouchesIterator->value->append(touch);
        }

        // Now build up the correct list for changedTouches.
        // Note that  any touches that are in the TouchStationary state (e.g. if
        // the user had several points touched but did not move them all) should
        // never be in the changedTouches list so we do not handle them explicitly here.
        // See https://bugs.webkit.org/show_bug.cgi?id=37609 for further discussion
        // about the TouchStationary state.
        if (pointState != PlatformTouchPoint::TouchStationary) {
            ASSERT(pointState < PlatformTouchPoint::TouchStateEnd);
            if (!changedTouches[pointState].m_touches)
                changedTouches[pointState].m_touches = TouchList::create();
            changedTouches[pointState].m_touches->append(touch);
            changedTouches[pointState].m_targets.add(touchTarget);
        }
    }
    m_touchPressed = touches->length() > 0;
    if (allTouchReleased)
        m_originatingTouchPointDocument.clear();

    // Now iterate the changedTouches list and m_targets within it, sending events to the targets as required.
    bool swallowedEvent = false;
    RefPtr<TouchList> emptyList = TouchList::create();
    for (unsigned state = 0; state != PlatformTouchPoint::TouchStateEnd; ++state) {
        if (!changedTouches[state].m_touches)
            continue;

        // When sending a touch cancel event, use empty touches and targetTouches lists.
        bool isTouchCancelEvent = (state == PlatformTouchPoint::TouchCancelled);
        RefPtr<TouchList>& effectiveTouches(isTouchCancelEvent ? emptyList : touches);
        const AtomicString& stateName(eventNameForTouchPointState(static_cast<PlatformTouchPoint::State>(state)));
        const EventTargetSet& targetsForState = changedTouches[state].m_targets;

        for (EventTargetSet::const_iterator it = targetsForState.begin(); it != targetsForState.end(); ++it) {
            EventTarget* touchEventTarget = it->get();
            RefPtr<TouchList> targetTouches(isTouchCancelEvent ? emptyList : touchesByTarget.get(touchEventTarget));
            ASSERT(targetTouches);

            RefPtr<TouchEvent> touchEvent =
                TouchEvent::create(effectiveTouches.get(), targetTouches.get(), changedTouches[state].m_touches.get(),
                                   stateName, touchEventTarget->toNode()->document()->defaultView(),
                                   0, 0, 0, 0, event.ctrlKey(), event.altKey(), event.shiftKey(), event.metaKey());
            touchEventTarget->toNode()->dispatchTouchEvent(touchEvent.get());
            swallowedEvent = swallowedEvent || touchEvent->defaultPrevented() || touchEvent->defaultHandled();
        }
    }

    return swallowedEvent;
}

bool EventHandler::dispatchSyntheticTouchEventIfEnabled(const PlatformMouseEvent& event)
{
    if (!m_frame || !m_frame->settings() || !m_frame->settings()->isTouchEventEmulationEnabled())
        return false;

    PlatformEvent::Type eventType = event.type();
    if (eventType != PlatformEvent::MouseMoved && eventType != PlatformEvent::MousePressed && eventType != PlatformEvent::MouseReleased)
        return false;

    HitTestRequest request(HitTestRequest::Active | HitTestRequest::DisallowShadowContent);
    MouseEventWithHitTestResults mev = prepareMouseEvent(request, event);
    if (mev.scrollbar() || subframeForHitTestResult(mev))
        return false;

    // The order is important. This check should follow the subframe test: http://webkit.org/b/111292.
    if (eventType == PlatformEvent::MouseMoved && !m_touchPressed)
        return true;

    SyntheticSingleTouchEvent touchEvent(event);
    return handleTouchEvent(touchEvent);
}

#endif

void EventHandler::setLastKnownMousePosition(const PlatformMouseEvent& event)
{
    m_mousePositionIsUnknown = false;
    m_lastKnownMousePosition = event.position();
    m_lastKnownMouseGlobalPosition = event.globalPosition();
}

} // namespace WebCore
