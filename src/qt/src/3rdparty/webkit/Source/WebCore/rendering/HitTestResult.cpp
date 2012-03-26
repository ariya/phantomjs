/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
*/

#include "config.h"
#include "HitTestResult.h"

#include "DocumentMarkerController.h"
#include "Frame.h"
#include "FrameTree.h"
#include "HTMLAnchorElement.h"
#include "HTMLVideoElement.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "RenderImage.h"
#include "RenderInline.h"
#include "Scrollbar.h"
#include "SelectionController.h"

#if ENABLE(SVG)
#include "SVGNames.h"
#include "XLinkNames.h"
#endif

namespace WebCore {

using namespace HTMLNames;

HitTestResult::HitTestResult()
    : m_isOverWidget(false)
    , m_isRectBased(false)
    , m_topPadding(0)
    , m_rightPadding(0)
    , m_bottomPadding(0)
    , m_leftPadding(0)
{
}

HitTestResult::HitTestResult(const IntPoint& point)
    : m_point(point)
    , m_isOverWidget(false)
    , m_isRectBased(false)
    , m_topPadding(0)
    , m_rightPadding(0)
    , m_bottomPadding(0)
    , m_leftPadding(0)
{
}

HitTestResult::HitTestResult(const IntPoint& centerPoint, unsigned topPadding, unsigned rightPadding, unsigned bottomPadding, unsigned leftPadding)
    : m_point(centerPoint)
    , m_isOverWidget(false)
    , m_topPadding(topPadding)
    , m_rightPadding(rightPadding)
    , m_bottomPadding(bottomPadding)
    , m_leftPadding(leftPadding)
{
    // If all padding values passed in are zero then it is not a rect based hit test.
    m_isRectBased = topPadding || rightPadding || bottomPadding || leftPadding;

    // Make sure all padding values are clamped to zero if it is not a rect hit test.
    if (!m_isRectBased)
        m_topPadding = m_rightPadding = m_bottomPadding = m_leftPadding = 0;
}

HitTestResult::HitTestResult(const HitTestResult& other)
    : m_innerNode(other.innerNode())
    , m_innerNonSharedNode(other.innerNonSharedNode())
    , m_point(other.point())
    , m_localPoint(other.localPoint())
    , m_innerURLElement(other.URLElement())
    , m_scrollbar(other.scrollbar())
    , m_isOverWidget(other.isOverWidget())
{
    // Only copy the padding and NodeSet in case of rect hit test.
    // Copying the later is rather expensive.
    if ((m_isRectBased = other.isRectBasedTest())) {
        m_topPadding = other.m_topPadding;
        m_rightPadding = other.m_rightPadding;
        m_bottomPadding = other.m_bottomPadding;
        m_leftPadding = other.m_leftPadding;
    } else
        m_topPadding = m_rightPadding = m_bottomPadding = m_leftPadding = 0;

    m_rectBasedTestResult = adoptPtr(other.m_rectBasedTestResult ? new NodeSet(*other.m_rectBasedTestResult) : 0);
}

HitTestResult::~HitTestResult()
{
}

HitTestResult& HitTestResult::operator=(const HitTestResult& other)
{
    m_innerNode = other.innerNode();
    m_innerNonSharedNode = other.innerNonSharedNode();
    m_point = other.point();
    m_localPoint = other.localPoint();
    m_innerURLElement = other.URLElement();
    m_scrollbar = other.scrollbar();
    m_isOverWidget = other.isOverWidget();
    // Only copy the padding and NodeSet in case of rect hit test.
    // Copying the later is rather expensive.
    if ((m_isRectBased = other.isRectBasedTest())) {
        m_topPadding = other.m_topPadding;
        m_rightPadding = other.m_rightPadding;
        m_bottomPadding = other.m_bottomPadding;
        m_leftPadding = other.m_leftPadding;
    } else
        m_topPadding = m_rightPadding = m_bottomPadding = m_leftPadding = 0;

    m_rectBasedTestResult = adoptPtr(other.m_rectBasedTestResult ? new NodeSet(*other.m_rectBasedTestResult) : 0);
    return *this;
}

void HitTestResult::setToNonShadowAncestor()
{
    Node* node = innerNode();
    if (node)
        node = node->shadowAncestorNode();
    setInnerNode(node);
    node = innerNonSharedNode();
    if (node)
        node = node->shadowAncestorNode();
    setInnerNonSharedNode(node);
}

void HitTestResult::setInnerNode(Node* n)
{
    m_innerNode = n;
}
    
void HitTestResult::setInnerNonSharedNode(Node* n)
{
    m_innerNonSharedNode = n;
}

void HitTestResult::setURLElement(Element* n) 
{ 
    m_innerURLElement = n; 
}

void HitTestResult::setScrollbar(Scrollbar* s)
{
    m_scrollbar = s;
}

Frame* HitTestResult::targetFrame() const
{
    if (!m_innerURLElement)
        return 0;

    Frame* frame = m_innerURLElement->document()->frame();
    if (!frame)
        return 0;

    return frame->tree()->find(m_innerURLElement->target());
}

bool HitTestResult::isSelected() const
{
    if (!m_innerNonSharedNode)
        return false;

    Frame* frame = m_innerNonSharedNode->document()->frame();
    if (!frame)
        return false;

    return frame->selection()->contains(m_point);
}

String HitTestResult::spellingToolTip(TextDirection& dir) const
{
    dir = LTR;
    // Return the tool tip string associated with this point, if any. Only markers associated with bad grammar
    // currently supply strings, but maybe someday markers associated with misspelled words will also.
    if (!m_innerNonSharedNode)
        return String();
    
    DocumentMarker* marker = m_innerNonSharedNode->document()->markers()->markerContainingPoint(m_point, DocumentMarker::Grammar);
    if (!marker)
        return String();

    if (RenderObject* renderer = m_innerNonSharedNode->renderer())
        dir = renderer->style()->direction();
    return marker->description;
}

String HitTestResult::replacedString() const
{
    // Return the replaced string associated with this point, if any. This marker is created when a string is autocorrected, 
    // and is used for generating a contextual menu item that allows it to easily be changed back if desired.
    if (!m_innerNonSharedNode)
        return String();
    
    DocumentMarker* marker = m_innerNonSharedNode->document()->markers()->markerContainingPoint(m_point, DocumentMarker::Replacement);
    if (!marker)
        return String();
    
    return marker->description;
}    
    
String HitTestResult::title(TextDirection& dir) const
{
    dir = LTR;
    // Find the title in the nearest enclosing DOM node.
    // For <area> tags in image maps, walk the tree for the <area>, not the <img> using it.
    for (Node* titleNode = m_innerNode.get(); titleNode; titleNode = titleNode->parentNode()) {
        if (titleNode->isElementNode()) {
            String title = static_cast<Element*>(titleNode)->title();
            if (!title.isEmpty()) {
                if (RenderObject* renderer = titleNode->renderer())
                    dir = renderer->style()->direction();
                return title;
            }
        }
    }
    return String();
}

String displayString(const String& string, const Node* node)
{
    if (!node)
        return string;
    return node->document()->displayStringModifiedByEncoding(string);
}

String HitTestResult::altDisplayString() const
{
    if (!m_innerNonSharedNode)
        return String();
    
    if (m_innerNonSharedNode->hasTagName(imgTag)) {
        HTMLImageElement* image = static_cast<HTMLImageElement*>(m_innerNonSharedNode.get());
        return displayString(image->getAttribute(altAttr), m_innerNonSharedNode.get());
    }
    
    if (m_innerNonSharedNode->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(m_innerNonSharedNode.get());
        return displayString(input->alt(), m_innerNonSharedNode.get());
    }

    return String();
}

Image* HitTestResult::image() const
{
    if (!m_innerNonSharedNode)
        return 0;
    
    RenderObject* renderer = m_innerNonSharedNode->renderer();
    if (renderer && renderer->isImage()) {
        RenderImage* image = static_cast<WebCore::RenderImage*>(renderer);
        if (image->cachedImage() && !image->cachedImage()->errorOccurred())
            return image->cachedImage()->image();
    }

    return 0;
}

IntRect HitTestResult::imageRect() const
{
    if (!image())
        return IntRect();
    return m_innerNonSharedNode->renderBox()->absoluteContentQuad().enclosingBoundingBox();
}

KURL HitTestResult::absoluteImageURL() const
{
    if (!(m_innerNonSharedNode && m_innerNonSharedNode->document()))
        return KURL();

    if (!(m_innerNonSharedNode->renderer() && m_innerNonSharedNode->renderer()->isImage()))
        return KURL();

    AtomicString urlString;
    if (m_innerNonSharedNode->hasTagName(embedTag)
        || m_innerNonSharedNode->hasTagName(imgTag)
        || m_innerNonSharedNode->hasTagName(inputTag)
        || m_innerNonSharedNode->hasTagName(objectTag)    
#if ENABLE(SVG)
        || m_innerNonSharedNode->hasTagName(SVGNames::imageTag)
#endif
       ) {
        Element* element = static_cast<Element*>(m_innerNonSharedNode.get());
        urlString = element->getAttribute(element->imageSourceAttributeName());
    } else
        return KURL();

    return m_innerNonSharedNode->document()->completeURL(stripLeadingAndTrailingHTMLSpaces(urlString));
}

KURL HitTestResult::absoluteMediaURL() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        return m_innerNonSharedNode->document()->completeURL(stripLeadingAndTrailingHTMLSpaces(mediaElt->currentSrc()));
    return KURL();
#else
    return KURL();
#endif
}

bool HitTestResult::mediaSupportsFullscreen() const
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElt(mediaElement());
    return (mediaElt && mediaElt->hasTagName(HTMLNames::videoTag) && mediaElt->supportsFullscreen());
#else
    return false;
#endif
}

#if ENABLE(VIDEO)
HTMLMediaElement* HitTestResult::mediaElement() const
{
    if (!(m_innerNonSharedNode && m_innerNonSharedNode->document()))
        return 0;

    if (!(m_innerNonSharedNode->renderer() && m_innerNonSharedNode->renderer()->isMedia()))
        return 0;

    if (m_innerNonSharedNode->hasTagName(HTMLNames::videoTag) || m_innerNonSharedNode->hasTagName(HTMLNames::audioTag))
        return static_cast<HTMLMediaElement*>(m_innerNonSharedNode.get());
    return 0;
}
#endif

void HitTestResult::toggleMediaControlsDisplay() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        mediaElt->setControls(!mediaElt->controls());
#endif
}

void HitTestResult::toggleMediaLoopPlayback() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        mediaElt->setLoop(!mediaElt->loop());
#endif
}

void HitTestResult::enterFullscreenForVideo() const
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElt(mediaElement());
    if (mediaElt && mediaElt->hasTagName(HTMLNames::videoTag)) {
        HTMLVideoElement* videoElt = static_cast<HTMLVideoElement*>(mediaElt);
        if (!videoElt->isFullscreen() && mediaElt->supportsFullscreen())
            videoElt->enterFullscreen();
    }
#endif
}

bool HitTestResult::mediaControlsEnabled() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        return mediaElt->controls();
#endif
    return false;
}

bool HitTestResult::mediaLoopEnabled() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        return mediaElt->loop();
#endif
    return false;
}

bool HitTestResult::mediaPlaying() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        return !mediaElt->paused();
#endif
    return false;
}

void HitTestResult::toggleMediaPlayState() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        mediaElt->togglePlayState();
#endif
}

bool HitTestResult::mediaHasAudio() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        return mediaElt->hasAudio();
#endif
    return false;
}

bool HitTestResult::mediaIsVideo() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        return mediaElt->hasTagName(HTMLNames::videoTag);
#endif
    return false;
}

bool HitTestResult::mediaMuted() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        return mediaElt->muted();
#endif
    return false;
}

void HitTestResult::toggleMediaMuteState() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        mediaElt->setMuted(!mediaElt->muted());
#endif
}

KURL HitTestResult::absoluteLinkURL() const
{
    if (!(m_innerURLElement && m_innerURLElement->document()))
        return KURL();

    AtomicString urlString;
    if (m_innerURLElement->hasTagName(aTag) || m_innerURLElement->hasTagName(areaTag) || m_innerURLElement->hasTagName(linkTag))
        urlString = m_innerURLElement->getAttribute(hrefAttr);
#if ENABLE(SVG)
    else if (m_innerURLElement->hasTagName(SVGNames::aTag))
        urlString = m_innerURLElement->getAttribute(XLinkNames::hrefAttr);
#endif
    else
        return KURL();

    return m_innerURLElement->document()->completeURL(stripLeadingAndTrailingHTMLSpaces(urlString));
}

bool HitTestResult::isLiveLink() const
{
    if (!(m_innerURLElement && m_innerURLElement->document()))
        return false;

    if (m_innerURLElement->hasTagName(aTag))
        return static_cast<HTMLAnchorElement*>(m_innerURLElement.get())->isLiveLink();
#if ENABLE(SVG)
    if (m_innerURLElement->hasTagName(SVGNames::aTag))
        return m_innerURLElement->isLink();
#endif

    return false;
}

String HitTestResult::titleDisplayString() const
{
    if (!m_innerURLElement)
        return String();
    
    return displayString(m_innerURLElement->title(), m_innerURLElement.get());
}

String HitTestResult::textContent() const
{
    if (!m_innerURLElement)
        return String();
    return m_innerURLElement->textContent();
}

// FIXME: This function needs a better name and may belong in a different class. It's not
// really isContentEditable(); it's more like needsEditingContextMenu(). In many ways, this
// function would make more sense in the ContextMenu class, except that WebElementDictionary 
// hooks into it. Anyway, we should architect this better. 
bool HitTestResult::isContentEditable() const
{
    if (!m_innerNonSharedNode)
        return false;

    if (m_innerNonSharedNode->hasTagName(textareaTag) || m_innerNonSharedNode->hasTagName(isindexTag))
        return true;

    if (m_innerNonSharedNode->hasTagName(inputTag))
        return static_cast<HTMLInputElement*>(m_innerNonSharedNode.get())->isTextField();

    return m_innerNonSharedNode->rendererIsEditable();
}

bool HitTestResult::addNodeToRectBasedTestResult(Node* node, int x, int y, const IntRect& rect)
{
    // If it is not a rect-based hit test, this method has to be no-op.
    // Return false, so the hit test stops.
    if (!isRectBasedTest())
        return false;

    // If node is null, return true so the hit test can continue.
    if (!node)
        return true;

    node = node->shadowAncestorNode();
    mutableRectBasedTestResult().add(node);

    if (node->renderer()->isInline()) {
        for (RenderObject* curr = node->renderer()->parent(); curr; curr = curr->parent()) {
            if (!curr->isRenderInline())
                break;
            
            // We need to make sure the nodes for culled inlines get included.
            RenderInline* currInline = toRenderInline(curr);
            if (currInline->alwaysCreateLineBoxes())
                break;
            
            if (currInline->visibleToHitTesting() && currInline->node())
                mutableRectBasedTestResult().add(currInline->node()->shadowAncestorNode());
        }
    }
    return !rect.contains(rectForPoint(x, y));
}

bool HitTestResult::addNodeToRectBasedTestResult(Node* node, int x, int y, const FloatRect& rect)
{
    // If it is not a rect-based hit test, this method has to be no-op.
    // Return false, so the hit test stops.
    if (!isRectBasedTest())
        return false;

    // If node is null, return true so the hit test can continue.
    if (!node)
        return true;

    node = node->shadowAncestorNode();
    mutableRectBasedTestResult().add(node);

    if (node->renderer()->isInline()) {
        for (RenderObject* curr = node->renderer()->parent(); curr; curr = curr->parent()) {
            if (!curr->isRenderInline())
                break;
            
            // We need to make sure the nodes for culled inlines get included.
            RenderInline* currInline = toRenderInline(curr);
            if (currInline->alwaysCreateLineBoxes())
                break;
            
            if (currInline->visibleToHitTesting() && currInline->node())
                mutableRectBasedTestResult().add(currInline->node()->shadowAncestorNode());
        }
    }
    return !rect.contains(rectForPoint(x, y));
}

void HitTestResult::append(const HitTestResult& other)
{
    ASSERT(isRectBasedTest() && other.isRectBasedTest());

    if (!m_innerNode && other.innerNode()) {
        m_innerNode = other.innerNode();
        m_innerNonSharedNode = other.innerNonSharedNode();
        m_localPoint = other.localPoint();
        m_innerURLElement = other.URLElement();
        m_scrollbar = other.scrollbar();
        m_isOverWidget = other.isOverWidget();
    }

    if (other.m_rectBasedTestResult) {
        NodeSet& set = mutableRectBasedTestResult();
        for (NodeSet::const_iterator it = other.m_rectBasedTestResult->begin(), last = other.m_rectBasedTestResult->end(); it != last; ++it)
            set.add(it->get());
    }
}

IntRect HitTestResult::rectForPoint(const IntPoint& point, unsigned topPadding, unsigned rightPadding, unsigned bottomPadding, unsigned leftPadding)
{
    IntPoint actualPoint(point);
    actualPoint -= IntSize(leftPadding, topPadding);

    IntSize actualPadding(leftPadding + rightPadding, topPadding + bottomPadding);
    // As IntRect is left inclusive and right exclusive (seeing IntRect::contains(x, y)), adding "1".
    actualPadding += IntSize(1, 1);

    return IntRect(actualPoint, actualPadding);
}

const HitTestResult::NodeSet& HitTestResult::rectBasedTestResult() const
{
    if (!m_rectBasedTestResult)
        m_rectBasedTestResult = adoptPtr(new NodeSet);
    return *m_rectBasedTestResult;
}

HitTestResult::NodeSet& HitTestResult::mutableRectBasedTestResult()
{
    if (!m_rectBasedTestResult)
        m_rectBasedTestResult = adoptPtr(new NodeSet);
    return *m_rectBasedTestResult;
}

} // namespace WebCore
