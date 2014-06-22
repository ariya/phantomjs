/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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

#ifndef FatFingers_h
#define FatFingers_h

#include "HitTestResult.h"
#include "RenderLayer.h"

#include <BlackBerryPlatformIntRectRegion.h>

#include <utility>

#include <wtf/HashSet.h>
#include <wtf/ListHashSet.h>
#include <wtf/Vector.h>

namespace WebCore {
class Document;
class Element;
class IntPoint;
class IntRect;
class IntSize;
class Node;
}

#define DEBUG_FAT_FINGERS 0

namespace BlackBerry {
namespace WebKit {

class WebPagePrivate;
class FatFingersResult;
class TouchEventHandler;


class FatFingers {
public:
    enum TargetType { ClickableElement, Text };

    FatFingers(WebPagePrivate* webpage, const WebCore::IntPoint& contentPos, TargetType);
    ~FatFingers();

    const FatFingersResult findBestPoint();

#if DEBUG_FAT_FINGERS
    // These debug vars are all in content coordinates. They are public so
    // they can be read from BackingStore, which will draw a visible rect
    // around the fat fingers area.
    static WebCore::IntRect m_debugFatFingerRect;
    static WebCore::IntPoint m_debugFatFingerClickPosition;
    static WebCore::IntPoint m_debugFatFingerAdjustedPosition;
#endif

private:
    enum MatchingApproachForClickable { ClickableByDefault = 0, MadeClickableByTheWebpage, Done };

    typedef std::pair<WebCore::Node*, Platform::IntRectRegion> IntersectingRegion;

    bool checkFingerIntersection(const Platform::IntRectRegion&,
        const Platform::IntRectRegion& remainingFingerRegion,
        WebCore::Node*,
        Vector<IntersectingRegion>& intersectingRegions);

    bool findIntersectingRegions(WebCore::Document*,
        Vector<IntersectingRegion>& intersectingRegions,
        Platform::IntRectRegion& remainingFingerRegion);

    bool checkForClickableElement(WebCore::Element*,
        Vector<IntersectingRegion>& intersectingRegions,
        Platform::IntRectRegion& remainingFingerRegion,
        WebCore::RenderLayer*& lowestPositionedEnclosingLayerSoFar);

    bool checkForText(WebCore::Node*,
        Vector<IntersectingRegion>& intersectingRegions,
        Platform::IntRectRegion& fingerRegion);

    void setSuccessfulFatFingersResult(FatFingersResult&, WebCore::Node*, const WebCore::IntPoint&);

    void getNodesFromRect(WebCore::Document*, const WebCore::IntPoint&, ListHashSet<RefPtr<WebCore::Node> >&);

    bool isElementClickable(WebCore::Element*) const;

    inline WebCore::IntRect fingerRectForPoint(const WebCore::IntPoint&) const;
    void getAdjustedPaddings(const WebCore::IntPoint&, unsigned& top, unsigned& right, unsigned& bottom, unsigned& left) const;

    WebPagePrivate* m_webPage;
    WebCore::IntPoint m_contentPos;
    TargetType m_targetType;
};

class FatFingersResult {
public:

    FatFingersResult(const WebCore::IntPoint& p = WebCore::IntPoint::zero(), const FatFingers::TargetType targetType = FatFingers::ClickableElement)
        : m_originalPosition(p)
        , m_adjustedPosition(p)
        , m_targetType(targetType)
        , m_positionWasAdjusted(false)
        , m_isTextInput(false)
        , m_isValid(false)
        , m_nodeUnderFatFinger(0)
    {
    }

    void reset()
    {
        m_originalPosition = m_adjustedPosition = WebCore::IntPoint::zero();
        m_positionWasAdjusted = false;
        m_isTextInput = false;
        m_isValid = false;
        m_nodeUnderFatFinger = 0;
    }

    bool resultMatches(const WebCore::IntPoint& p, const FatFingers::TargetType targetType) const
    {
        return m_isValid && p == m_originalPosition && targetType == m_targetType;
    }

    WebCore::IntPoint originPosition() const { return m_originalPosition; }
    WebCore::IntPoint adjustedPosition() const { return m_adjustedPosition; }
    bool positionWasAdjusted() const { return m_isValid && m_positionWasAdjusted; }
    bool isTextInput() const { return m_isValid && !!m_nodeUnderFatFinger && m_isTextInput; }
    bool isValid() const { return m_isValid; }

    enum ContentType { ShadowContentAllowed, ShadowContentNotAllowed };

    WebCore::Node* node(ContentType type = ShadowContentAllowed, bool shouldUseRootEditableElement = false) const
    {
        if (!m_nodeUnderFatFinger || !m_nodeUnderFatFinger->inDocument())
            return 0;

        WebCore::Node* result = m_nodeUnderFatFinger.get();

        if (type == ShadowContentAllowed)
            return result;

        // Shadow trees can be nested.
        while (result->isInShadowTree())
            result = toElement(result->deprecatedShadowAncestorNode());

        if (!shouldUseRootEditableElement || !result->isElementNode())
            return result;

        // Retrieve the top level editable node
        while (!result->isRootEditableElement()) {
            WebCore::Element* parentElement = result->parentElement();
            if (!parentElement)
                break;
            result = parentElement;
        }

        return result;
    }

    WebCore::Element* nodeAsElementIfApplicable(ContentType type = ShadowContentAllowed, bool shouldUseRootEditableElement = false) const
    {
        WebCore::Node* result = node(type, shouldUseRootEditableElement);
        if (!result || !result->isElementNode())
            return 0;

        return static_cast<WebCore::Element*>(result);
    }

private:
    friend class WebKit::FatFingers;
    friend class WebKit::TouchEventHandler;

    WebCore::IntPoint m_originalPosition; // Main frame contents coordinates.
    WebCore::IntPoint m_adjustedPosition; // Main frame contents coordinates.
    FatFingers::TargetType m_targetType;
    bool m_positionWasAdjusted;
    bool m_isTextInput; // Check if the element under the touch point will require a VKB be displayed so that the touch down can be suppressed.
    bool m_isValid;
    RefPtr<WebCore::Node> m_nodeUnderFatFinger;
};

}
}

#endif // FatFingers_h

