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

#include "config.h"
#include "FatFingers.h"

#include "BlackBerryPlatformLog.h"
#include "BlackBerryPlatformScreen.h"
#include "BlackBerryPlatformSettings.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSParser.h"
#include "DOMSupport.h"
#include "Document.h"
#include "Element.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "FloatQuad.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLTextAreaElement.h"
#include "Range.h"
#include "RenderObject.h"
#include "RenderView.h"
#include "Text.h"
#include "TextBreakIterator.h"
#include "WebKitThreadViewportAccessor.h"
#include "WebPage_p.h"

#if DEBUG_FAT_FINGERS
#include "BackingStore.h"
#endif

using BlackBerry::Platform::IntRectRegion;

using namespace WebCore;

// Lets make the top padding bigger than other directions, since it gets us more
// accurate clicking results.

namespace BlackBerry {
namespace WebKit {

#if DEBUG_FAT_FINGERS
IntRect FatFingers::m_debugFatFingerRect;
IntPoint FatFingers::m_debugFatFingerClickPosition;
IntPoint FatFingers::m_debugFatFingerAdjustedPosition;
#endif

IntRect FatFingers::fingerRectForPoint(const IntPoint& point) const
{
    const Platform::ViewportAccessor* viewportAccessor = m_webPage->m_webkitThreadViewportAccessor;

    unsigned topPadding, rightPadding, bottomPadding, leftPadding;
    IntPoint contentViewportPos = viewportAccessor->documentViewportFromContents(point);
    getAdjustedPaddings(contentViewportPos, topPadding, rightPadding, bottomPadding, leftPadding);

    return HitTestLocation::rectForPoint(point, topPadding, rightPadding, bottomPadding, leftPadding);
}

static bool hasMousePressListener(Element* element)
{
    ASSERT(element);
    return element->hasEventListeners(eventNames().clickEvent)
        || element->hasEventListeners(eventNames().mousedownEvent)
        || element->hasEventListeners(eventNames().mouseupEvent);
}

bool FatFingers::isElementClickable(Element* element) const
{
    ASSERT(element);
    ASSERT(m_targetType == ClickableElement);

    ExceptionCode ec = 0;

    if (element->webkitMatchesSelector("a[href],*:link,*:visited,*[role=button],button,input,select,label[for],area[href],textarea,embed,object", ec)
        || element->isMediaControlElement()
        || element->isContentEditable()
        || (isHTMLImageElement(element) && element->parentNode() && isHTMLAnchorElement(element->parentNode())))
        return true;

    return hasMousePressListener(element)
        || CSSComputedStyleDeclaration::create(element)->getPropertyValue(cssPropertyID("cursor")) == "pointer";
}

// FIXME: Handle content editable nodes here too.
static inline bool isFieldWithText(Node* node)
{
    ASSERT(node);
    if (!node || !node->isElementNode())
        return false;

    Element* element = toElement(node);
    return !DOMSupport::inputElementText(element).isEmpty();
}

static inline int distanceBetweenPoints(const IntPoint& p1, const IntPoint& p2)
{
    int dx = p1.x() - p2.x();
    int dy = p1.y() - p2.y();
    return sqrt((double)((dx * dx) + (dy * dy)));
}

static bool compareDistanceBetweenPoints(const Platform::IntPoint& p, const IntRectRegion& r1, const IntRectRegion& r2)
{
    return distanceBetweenPoints(p, r1.extents().center()) > distanceBetweenPoints(p, r2.extents().center());
}

static bool isValidFrameOwner(WebCore::Element* element)
{
    ASSERT(element);
    return element->isFrameOwnerElement() && static_cast<HTMLFrameOwnerElement*>(element)->contentFrame();
}

// NOTE: 'contentPos' is in main frame contents coordinates.
FatFingers::FatFingers(WebPagePrivate* webPage, const WebCore::IntPoint& contentPos, TargetType targetType)
    : m_webPage(webPage)
    , m_contentPos(contentPos)
    , m_targetType(targetType)
{
    ASSERT(webPage);

#if DEBUG_FAT_FINGERS
    const Platform::ViewportAccessor* viewportAccessor = m_webPage->m_webkitThreadViewportAccessor;

    m_debugFatFingerRect = IntRect(0, 0, 0, 0);
    m_debugFatFingerClickPosition = viewportAccessor->pixelViewportFromContents(viewportAccessor->roundToPixelFromDocumentContents(WebCore::FloatPoint(contentPos)));
    m_debugFatFingerAdjustedPosition = m_debugFatFingerClickPosition;
#endif
}

FatFingers::~FatFingers()
{
}

const FatFingersResult FatFingers::findBestPoint()
{
    ASSERT(m_webPage);
    ASSERT(m_webPage->m_mainFrame);

    // Even though we have clamped the point in libwebview to viewport, but there might be a rounding difference for viewport rect.
    // Clamp position to viewport to ensure we are inside viewport.
    IntRect viewportRect = m_webPage->mainFrame()->view()->visibleContentRect();
    m_contentPos = Platform::pointClampedToRect(m_contentPos, viewportRect);

    FatFingersResult result(m_contentPos);

    // Lets set nodeUnderFatFinger to the result of a point based hit test here. If something
    // targable is actually found by ::findIntersectingRegions, then we might replace what we just set below later on.
    const HitTestResult& hitResult = m_webPage->hitTestResult(m_contentPos);
    Node* node = hitResult.innerNode();
    while (node && !node->isElementNode())
        node = node->parentNode();

    Element* elementUnderPoint = toElement(node);

    if (elementUnderPoint) {
        result.m_nodeUnderFatFinger = elementUnderPoint;

        // If we are looking for a Clickable Element and we found one, we can quit early.
        if (m_targetType == ClickableElement) {
            if (isElementClickable(elementUnderPoint)) {
                setSuccessfulFatFingersResult(result, elementUnderPoint, m_contentPos /*adjustedPosition*/);
                return result;
            }
            if (hitResult.URLElement()) {
                setSuccessfulFatFingersResult(result, hitResult.URLElement(), m_contentPos /*adjustedPosition*/);
                return result;
            }
        }
    }

#if DEBUG_FAT_FINGERS
    // Force blit to make the fat fingers rects show up.
    if (!m_debugFatFingerRect.isEmpty())
        m_webPage->m_backingStore->repaint(0, 0, m_webPage->transformedViewportSize().width(), m_webPage->transformedViewportSize().height(), true, true);
#endif

    Vector<IntersectingRegion> intersectingRegions;
    IntRectRegion remainingFingerRegion = IntRectRegion(fingerRectForPoint(m_contentPos));

    bool foundOne = findIntersectingRegions(m_webPage->m_mainFrame->document(), intersectingRegions, remainingFingerRegion);

    if (!foundOne)
        return result;

    Node* bestNode = 0;
    IntRectRegion largestIntersectionRegion;
    int largestIntersectionRegionArea = 0;

    Vector<IntersectingRegion>::const_iterator endIt = intersectingRegions.end();
    for (Vector<IntersectingRegion>::const_iterator it = intersectingRegions.begin(); it != endIt; ++it) {
        Node* currentNode = it->first;
        IntRectRegion currentIntersectionRegion = it->second;

        int currentIntersectionRegionArea = currentIntersectionRegion.area();
        if (currentIntersectionRegionArea > largestIntersectionRegionArea
            || (currentIntersectionRegionArea == largestIntersectionRegionArea
            && compareDistanceBetweenPoints(m_contentPos, currentIntersectionRegion, largestIntersectionRegion))) {
            bestNode = currentNode;
            largestIntersectionRegion = currentIntersectionRegion;
            largestIntersectionRegionArea = currentIntersectionRegionArea;
        }
    }

    if (!bestNode || largestIntersectionRegion.isEmpty())
        return result;

#if DEBUG_FAT_FINGERS
    const Platform::ViewportAccessor* viewportAccessor = m_webPage->m_webkitThreadViewportAccessor;

    m_debugFatFingerAdjustedPosition = viewportAccessor->pixelViewportFromContents(
        viewportAccessor->roundToPixelFromDocumentContents(largestIntersectionRegion.rects()[0].center()));
#endif

    setSuccessfulFatFingersResult(result, bestNode, largestIntersectionRegion.rects()[0].center() /*adjustedPosition*/);

    return result;
}

// 'region' is in contents coordinates relative to the frame containing 'node'
// 'remainingFingerRegion' and 'intersectingRegions' will always be in main frame contents
// coordinates.
// Thus, before comparing, we need to map the former to main frame contents coordinates.
bool FatFingers::checkFingerIntersection(const IntRectRegion& region, const IntRectRegion& remainingFingerRegion, Node* node, Vector<IntersectingRegion>& intersectingRegions)
{
    ASSERT(node);

    IntRectRegion regionCopy(region);
    WebCore::IntPoint framePos(m_webPage->frameOffset(node->document()->frame()));
    regionCopy.move(framePos.x(), framePos.y());

    IntRectRegion intersection = intersectRegions(regionCopy, remainingFingerRegion);
    if (intersection.isEmpty())
        return false;

#if DEBUG_FAT_FINGERS
    String nodeName;
    if (node->isTextNode())
        nodeName = "text node";
    else if (node->isElementNode())
        nodeName = String::format("%s node", toElement(node)->tagName().latin1().data());
    else
        nodeName = "unknown node";
    if (node->isInShadowTree()) {
        nodeName = nodeName + "(in shadow tree";
        if (node->isElementNode() && !toElement(node)->shadowPseudoId().isEmpty())
            nodeName = nodeName + ", pseudo id " + toElement(node)->shadowPseudoId();
        nodeName = nodeName + ")";
    }
    Platform::logAlways(Platform::LogLevelInfo,
        "%s has region %s, intersecting at %s (area %d)", nodeName.latin1().data(),
        regionCopy.toString().c_str(), intersection.toString().c_str(), intersection.area());
#endif

    intersectingRegions.append(std::make_pair(node, intersection));
    return true;
}


// intersectingRegions and remainingFingerRegion are all in main frame contents coordinates,
// even on recursive calls of ::findIntersectingRegions.
bool FatFingers::findIntersectingRegions(Document* document, Vector<IntersectingRegion>& intersectingRegions, IntRectRegion& remainingFingerRegion)
{
    if (!document || !document->frame()->view())
        return false;

    // The layout needs to be up-to-date to determine if a node is focusable.
    document->updateLayoutIgnorePendingStylesheets();

    // Create fingerRect.
    IntPoint frameContentPos(document->frame()->view()->windowToContents(m_webPage->m_mainFrame->view()->contentsToWindow(m_contentPos)));
    IntRect viewportRect = m_webPage->mainFrame()->view()->visibleContentRect();

    // Ensure the frameContentPos is inside the viewport.
    frameContentPos = Platform::pointClampedToRect(frameContentPos, viewportRect);

#if DEBUG_FAT_FINGERS
    const Platform::ViewportAccessor* viewportAccessor = m_webPage->m_webkitThreadViewportAccessor;

    Platform::IntRect fingerRect(fingerRectForPoint(frameContentPos));
    Platform::IntRect screenFingerRect = viewportAccessor->roundToPixelFromDocumentContents(fingerRect);
    Platform::logAlways(Platform::LogLevelInfo, "fat finger rect now %s", screenFingerRect.toString().c_str());

    // only record the first finger rect
    if (document == m_webPage->m_mainFrame->document())
        m_debugFatFingerRect = viewportAccessor->pixelViewportFromContents(screenFingerRect);
#endif

    bool foundOne = false;

    RenderLayer* lowestPositionedEnclosingLayerSoFar = 0;

    // Iterate over the list of nodes (and subrects of nodes where possible), for each saving the
    // intersection of the bounding box with the finger rect.
    ListHashSet<RefPtr<Node> > intersectedNodes;

    if (m_webPage->m_cachedRectHitTestResults.contains(document))
        intersectedNodes = m_webPage->m_cachedRectHitTestResults.get(document);
    else
        getNodesFromRect(document, frameContentPos, intersectedNodes);

    ListHashSet<RefPtr<Node> >::const_iterator it = intersectedNodes.begin();
    ListHashSet<RefPtr<Node> >::const_iterator end = intersectedNodes.end();
    for ( ; it != end; ++it) {
        Node* curNode = (*it).get();
        if (!curNode || !curNode->renderer())
            continue;

        if (remainingFingerRegion.isEmpty())
            break;

        bool isElement = curNode->isElementNode();
        if (isElement && isValidFrameOwner(toElement(curNode))) {

            HTMLFrameOwnerElement* owner = static_cast<HTMLFrameOwnerElement*>(curNode);
            Document* childDocument = owner && owner->contentFrame() ? owner->contentFrame()->document() : 0;
            if (!childDocument)
                continue;

            ASSERT(childDocument->frame()->view());

            foundOne |= findIntersectingRegions(childDocument, intersectingRegions, remainingFingerRegion);
        } else if (isElement && m_targetType == ClickableElement) {
            foundOne |= checkForClickableElement(toElement(curNode), intersectingRegions, remainingFingerRegion, lowestPositionedEnclosingLayerSoFar);
        } else if (m_targetType == Text)
            foundOne |= checkForText(curNode, intersectingRegions, remainingFingerRegion);
    }

    return foundOne;
}

bool FatFingers::checkForClickableElement(Element* curElement, Vector<IntersectingRegion>& intersectingRegions, IntRectRegion& remainingFingerRegion, RenderLayer*& lowestPositionedEnclosingLayerSoFar)
{
    ASSERT(curElement);

    bool intersects = false;
    IntRectRegion elementRegion;

    bool isClickableElement = isElementClickable(curElement);
    if (isClickableElement) {
        if (curElement->isLink()) {
            // Links can wrap lines, and in such cases Node::boundingBox() can give us
            // not accurate rects, since it unites all InlineBox's rects. In these
            // cases, we can process each line of the link separately with our
            // intersection rect, getting a more accurate clicking.
            Vector<FloatQuad> quads;
            curElement->renderer()->absoluteFocusRingQuads(quads);

            size_t n = quads.size();
            ASSERT(n);

            for (size_t i = 0; i < n; ++i)
                elementRegion = unionRegions(elementRegion, Platform::IntRect(quads[i].enclosingBoundingBox()));
        } else
            elementRegion = IntRectRegion(curElement->renderer()->absoluteBoundingBoxRect(true /*use transforms*/));

    } else
        elementRegion = IntRectRegion(curElement->renderer()->absoluteBoundingBoxRect(true /*use transforms*/));

    if (lowestPositionedEnclosingLayerSoFar) {
        RenderLayer* curElementRenderLayer = m_webPage->enclosingPositionedAncestorOrSelfIfPositioned(curElement->renderer()->enclosingLayer());
        if (curElementRenderLayer != lowestPositionedEnclosingLayerSoFar) {

            // elementRegion will always be in contents coordinates of its container frame. It needs to be
            // mapped to main frame contents coordinates in order to intersect the fingerRegion, then.
            WebCore::IntPoint framePos(m_webPage->frameOffset(curElement->document()->frame()));
            IntRectRegion layerRegion(Platform::IntRect(lowestPositionedEnclosingLayerSoFar->renderer()->absoluteBoundingBoxRect(true/*use transforms*/)));
            layerRegion.move(framePos.x(), framePos.y());

            remainingFingerRegion = intersectRegions(remainingFingerRegion, layerRegion);

            lowestPositionedEnclosingLayerSoFar = curElementRenderLayer;
        }
    } else
        lowestPositionedEnclosingLayerSoFar = m_webPage->enclosingPositionedAncestorOrSelfIfPositioned(curElement->renderer()->enclosingLayer());

    if (isClickableElement)
        intersects = checkFingerIntersection(elementRegion, remainingFingerRegion, curElement, intersectingRegions);

    return intersects;
}

bool FatFingers::checkForText(Node* curNode, Vector<IntersectingRegion>& intersectingRegions, IntRectRegion& fingerRegion)
{
    ASSERT(curNode);
    if (isFieldWithText(curNode)) {
        // FIXME: Find all text in the field and find the best word.
        // For now, we will just select the whole field.
        IntRect boundingRect = curNode->renderer()->absoluteBoundingBoxRect(true /*use transforms*/);
        IntRectRegion nodeRegion(boundingRect);
        return checkFingerIntersection(nodeRegion, fingerRegion, curNode, intersectingRegions);
    }

    if (curNode->isTextNode()) {
        WebCore::Text* curText = static_cast<WebCore::Text*>(curNode);
        String allText = curText->wholeText();

        // Iterate through all words, breaking at whitespace, to find the bounding box of each word.
        TextBreakIterator* wordIterator = wordBreakIterator(allText.characters(), allText.length());

        int lastOffset = textBreakFirst(wordIterator);
        if (lastOffset == -1)
            return false;

        bool foundOne = false;
        int offset;
        Document* document = curNode->document();

        while ((offset = textBreakNext(wordIterator)) != -1) {
            RefPtr<Range> range = Range::create(document, curText, lastOffset, curText, offset);
            if (!range->text().stripWhiteSpace().isEmpty()) {
#if DEBUG_FAT_FINGERS
                Platform::logAlways(Platform::LogLevelInfo, "Checking word '%s'", range->text().latin1().data());
#endif
                IntRectRegion rangeRegion(DOMSupport::transformedBoundingBoxForRange(*range));
                foundOne |= checkFingerIntersection(rangeRegion, fingerRegion, curNode, intersectingRegions);
            }
            lastOffset = offset;
        }
        return foundOne;
    }
    return false;
}

void FatFingers::getAdjustedPaddings(const IntPoint& contentViewportPos, unsigned& top, unsigned& right, unsigned& bottom, unsigned& left) const
{
    static unsigned topPadding = Platform::Settings::instance()->topFatFingerPadding();
    static unsigned rightPadding = Platform::Settings::instance()->rightFatFingerPadding();
    static unsigned bottomPadding = Platform::Settings::instance()->bottomFatFingerPadding();
    static unsigned leftPadding = Platform::Settings::instance()->leftFatFingerPadding();

    double currentScale = m_webPage->currentScale();
    top = topPadding / currentScale;
    right = rightPadding / currentScale;
    bottom = bottomPadding / currentScale;
    left = leftPadding / currentScale;

    IntRect viewportRect = m_webPage->mainFrame()->view()->visibleContentRect();
    // We clamp the event position inside the viewport. We should not expand the fat finger rect to the edge again.
    top = std::min(unsigned(std::max(contentViewportPos.y() - 1, 0)), top);
    left = std::min(unsigned(std::max(contentViewportPos.x() - 1, 0)), left);
    bottom = std::min(unsigned(std::max(viewportRect.height() - contentViewportPos.y() - 1, 0)), bottom);
    right = std::min(unsigned(std::max(viewportRect.width() - contentViewportPos.x() - 1, 0)), right);
}

void FatFingers::getNodesFromRect(Document* document, const IntPoint& contentPos, ListHashSet<RefPtr<Node> >& intersectedNodes)
{
    const Platform::ViewportAccessor* viewportAccessor = m_webPage->m_webkitThreadViewportAccessor;

    unsigned topPadding, rightPadding, bottomPadding, leftPadding;
    IntPoint contentViewportPos = viewportAccessor->documentViewportFromContents(m_contentPos);
    // Do not allow fat fingers detect anything not visible(ie outside of the viewport)
    getAdjustedPaddings(contentViewportPos, topPadding, rightPadding, bottomPadding, leftPadding);

    // The user functions checkForText() and findIntersectingRegions() uses the Node.wholeText() to checkFingerIntersection()
    // not the text in its shadow tree.
    HitTestRequest::HitTestRequestType requestType = HitTestRequest::ReadOnly | HitTestRequest::Active;
    if (m_targetType != Text)
        requestType |= HitTestRequest::DisallowShadowContent;
    HitTestResult result(contentPos, topPadding, rightPadding, bottomPadding, leftPadding);

    document->renderView()->layer()->hitTest(requestType, result);
    intersectedNodes = result.rectBasedTestResult();
    m_webPage->m_cachedRectHitTestResults.add(document, intersectedNodes);
}

void FatFingers::setSuccessfulFatFingersResult(FatFingersResult& result, Node* bestNode, const WebCore::IntPoint& adjustedPoint)
{
    result.m_nodeUnderFatFinger = bestNode;
    result.m_adjustedPosition = adjustedPoint;
    result.m_positionWasAdjusted = true;
    result.m_isValid = true;

    bool isTextInputElement = false;
    if (m_targetType == ClickableElement) {
        ASSERT_WITH_SECURITY_IMPLICATION(bestNode->isElementNode());
        Element* bestElement = toElement(bestNode);
        isTextInputElement = DOMSupport::isTextInputElement(bestElement);
    }
    result.m_isTextInput = isTextInputElement;
}

}
}
