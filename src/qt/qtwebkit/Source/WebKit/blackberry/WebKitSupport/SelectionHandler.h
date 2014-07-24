/*
 * Copyright (C) 2010, 2011, 2012, 2013 Research In Motion Limited. All rights reserved.
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

#ifndef SelectionHandler_h
#define SelectionHandler_h

#include "BlackBerryPlatformIntRectRegion.h"
#include "BlackBerryPlatformPrimitives.h"
#include "BlackBerryPlatformStopWatch.h"
#include "Color.h"
#include "TextGranularity.h"

#include <wtf/Vector.h>

namespace WTF {
class String;
}

namespace WebCore {
class FloatQuad;
class IntPoint;
class IntRect;
class Node;
class VisiblePosition;
class VisibleSelection;
}

namespace BlackBerry {
namespace Platform {
class String;
}

namespace WebKit {

class FatFingersResult;
class WebPagePrivate;

class SelectionHandler {
public:
    SelectionHandler(WebPagePrivate*);
    ~SelectionHandler();

    bool isSelectionActive() { return m_selectionActive; }
    void setSelectionActive(bool active) { m_selectionActive = active; }

    void cancelSelection();
    BlackBerry::Platform::String selectedText() const;

    bool selectionContains(const WebCore::IntPoint&);

    void setSelection(WebCore::IntPoint start, WebCore::IntPoint end);
    void selectAtPoint(const WebCore::IntPoint&, SelectionExpansionType);
    void selectObject(const WebCore::IntPoint&, WebCore::TextGranularity);
    void selectObject(WebCore::TextGranularity);
    void selectObject(WebCore::Node*);

    void selectionPositionChanged(bool forceUpdateWithoutChange = false);

    void setCaretPosition(const WebCore::IntPoint&);

    bool lastUpdatedEndPointIsValid() const { return m_lastUpdatedEndPointIsValid; }

    void inputHandlerDidFinishProcessingChange();

    void expandSelection(bool isScrollStarted);
    void setOverlayExpansionHeight(int dy) { m_overlayExpansionHeight = dy; }
    void setParagraphExpansionScrollMargin(const WebCore::IntSize&);
    void setSelectionViewportSize(const WebCore::IntSize& selectionViewportSize) { m_selectionViewportSize = selectionViewportSize; }
    void setSelectionSubframeViewportRect(const WebCore::IntRect& selectionSubframeViewportRect) { m_selectionSubframeViewportRect = selectionSubframeViewportRect; }
    WebCore::IntRect selectionViewportRect() const;

private:
    void notifyCaretPositionChangedIfNeeded(bool userTouchTriggeredOnTextField);
    void caretPositionChanged(bool userTouchTriggered);
    void regionForTextQuads(WTF::Vector<WebCore::FloatQuad>&, BlackBerry::Platform::IntRectRegion&, bool shouldClipToVisibleContent = true) const;
    WebCore::IntRect clippingRectForVisibleContent() const;
    bool updateOrHandleInputSelection(WebCore::VisibleSelection& newSelection, const WebCore::IntPoint& relativeStart, const WebCore::IntPoint& relativeEnd);
    WebCore::Node* DOMContainerNodeForVisiblePosition(const WebCore::VisiblePosition&) const;
    bool shouldUpdateSelectionOrCaretForPoint(const WebCore::IntPoint&, const WebCore::IntRect&, bool startCaret = true) const;
    unsigned extendSelectionToFieldBoundary(bool isStartHandle, const WebCore::IntPoint& selectionPoint, WebCore::VisibleSelection& newSelection);
    WebCore::IntPoint clipPointToVisibleContainer(const WebCore::IntPoint&) const;

    void selectNextParagraph();
    void drawAnimationOverlay(BlackBerry::Platform::IntRectRegion, bool isExpandingOverlayAtConstantRate, bool isStartOfSelection = false);
    Platform::IntRectRegion regionForSelectionQuads(WebCore::VisibleSelection);
    bool findNextAnimationOverlayRegion();
    bool ensureSelectedTextVisible(const WebCore::IntPoint&, bool scrollIfNeeded);
    bool expandSelectionToGranularity(WebCore::Frame*, WebCore::VisibleSelection, WebCore::TextGranularity, bool isInputMode);

    bool inputNodeOverridesTouch() const;
    BlackBerry::Platform::RequestedHandlePosition requestedSelectionHandlePosition(const WebCore::VisibleSelection&) const;

    bool selectNodeIfFatFingersResultIsLink(FatFingersResult);

    WebCore::IntRect startCaretViewportRect(const WebCore::IntPoint& frameOffset) const;

    WebPagePrivate* m_webPage;

    bool m_selectionActive;
    bool m_caretActive;
    bool m_lastUpdatedEndPointIsValid;
    bool m_didSuppressCaretPositionChangedNotification;
    BlackBerry::Platform::IntRectRegion m_lastSelectionRegion;
    WebCore::VisiblePosition m_animationOverlayStartPos;
    WebCore::VisiblePosition m_animationOverlayEndPos;
    BlackBerry::Platform::IntRectRegion m_currentAnimationOverlayRegion;
    BlackBerry::Platform::IntRectRegion m_nextAnimationOverlayRegion;
    int m_overlayExpansionHeight;
    WebCore::Color m_animationHighlightColor;

    BlackBerry::Platform::StopWatch m_timer;

    WebCore::IntSize m_scrollMargin;
    WebCore::IntSize m_selectionViewportSize;
    WebCore::IntRect m_selectionSubframeViewportRect;
    WebCore::VisibleSelection m_lastSelection;
};

}
}

#endif // SelectionHandler_h
