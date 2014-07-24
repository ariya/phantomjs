/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FindController.h"

#include "PluginView.h"
#include "ShareableBitmap.h"
#include "WKPage.h"
#include "WebCoreArgumentCoders.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include <WebCore/DocumentMarkerController.h>
#include <WebCore/FloatQuad.h>
#include <WebCore/FocusController.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/Page.h>
#include <WebCore/PluginDocument.h>

using namespace WebCore;

namespace WebKit {

static WebCore::FindOptions core(FindOptions options)
{
    return (options & FindOptionsCaseInsensitive ? CaseInsensitive : 0)
        | (options & FindOptionsAtWordStarts ? AtWordStarts : 0)
        | (options & FindOptionsTreatMedialCapitalAsWordStart ? TreatMedialCapitalAsWordStart : 0)
        | (options & FindOptionsBackwards ? Backwards : 0)
        | (options & FindOptionsWrapAround ? WrapAround : 0);
}

FindController::FindController(WebPage* webPage)
    : m_webPage(webPage)
    , m_findPageOverlay(0)
    , m_isShowingFindIndicator(false)
{
}

FindController::~FindController()
{
}

static PluginView* pluginViewForFrame(Frame* frame)
{
    if (!frame->document()->isPluginDocument())
        return 0;

    PluginDocument* pluginDocument = static_cast<PluginDocument*>(frame->document());
    return static_cast<PluginView*>(pluginDocument->pluginWidget());
}

void FindController::countStringMatches(const String& string, FindOptions options, unsigned maxMatchCount)
{
    if (maxMatchCount == std::numeric_limits<unsigned>::max())
        --maxMatchCount;
    
    PluginView* pluginView = pluginViewForFrame(m_webPage->mainFrame());
    
    unsigned matchCount;

    if (pluginView)
        matchCount = pluginView->countFindMatches(string, core(options), maxMatchCount + 1);
    else {
        matchCount = m_webPage->corePage()->countFindMatches(string, core(options), maxMatchCount + 1);
        m_webPage->corePage()->unmarkAllTextMatches();
    }

    if (matchCount > maxMatchCount)
        matchCount = static_cast<unsigned>(kWKMoreThanMaximumMatchCount);
    
    m_webPage->send(Messages::WebPageProxy::DidCountStringMatches(string, matchCount));
}

static Frame* frameWithSelection(Page* page)
{
    for (Frame* frame = page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (frame->selection()->isRange())
            return frame;
    }

    return 0;
}

void FindController::updateFindUIAfterPageScroll(bool found, const String& string, FindOptions options, unsigned maxMatchCount)
{
    Frame* selectedFrame = frameWithSelection(m_webPage->corePage());
    
    PluginView* pluginView = pluginViewForFrame(m_webPage->mainFrame());

    bool shouldShowOverlay = false;

    if (!found) {
        if (!pluginView)
            m_webPage->corePage()->unmarkAllTextMatches();

        if (selectedFrame)
            selectedFrame->selection()->clear();

        hideFindIndicator();

        m_webPage->send(Messages::WebPageProxy::DidFailToFindString(string));
    } else {
        shouldShowOverlay = options & FindOptionsShowOverlay;
        bool shouldShowHighlight = options & FindOptionsShowHighlight;
        unsigned matchCount = 1;

        if (shouldShowOverlay || shouldShowHighlight) {
            if (maxMatchCount == std::numeric_limits<unsigned>::max())
                --maxMatchCount;

            if (pluginView) {
                matchCount = pluginView->countFindMatches(string, core(options), maxMatchCount + 1);
                shouldShowOverlay = false;
            } else {
                m_webPage->corePage()->unmarkAllTextMatches();
                matchCount = m_webPage->corePage()->markAllMatchesForText(string, core(options), shouldShowHighlight, maxMatchCount + 1);
            }

            // If we have a large number of matches, we don't want to take the time to paint the overlay.
            if (matchCount > maxMatchCount) {
                shouldShowOverlay = false;
                matchCount = static_cast<unsigned>(kWKMoreThanMaximumMatchCount);
            }
        }

        m_webPage->send(Messages::WebPageProxy::DidFindString(string, matchCount));

        if (!(options & FindOptionsShowFindIndicator) || !updateFindIndicator(selectedFrame, shouldShowOverlay))
            hideFindIndicator();
    }

    if (!shouldShowOverlay) {
        if (m_findPageOverlay)
            m_webPage->uninstallPageOverlay(m_findPageOverlay, true);
    } else {
        if (!m_findPageOverlay) {
            RefPtr<PageOverlay> findPageOverlay = PageOverlay::create(this);
            m_findPageOverlay = findPageOverlay.get();
            m_webPage->installPageOverlay(findPageOverlay.release(), true);
            m_findPageOverlay->setNeedsDisplay();
        } else
            m_findPageOverlay->setNeedsDisplay();
    }
}

void FindController::findString(const String& string, FindOptions options, unsigned maxMatchCount)
{
    PluginView* pluginView = pluginViewForFrame(m_webPage->mainFrame());
    
    bool found;
    
    if (pluginView)
        found = pluginView->findString(string, core(options), maxMatchCount);
    else
        found = m_webPage->corePage()->findString(string, core(options));

    m_webPage->drawingArea()->dispatchAfterEnsuringUpdatedScrollPosition(WTF::bind(&FindController::updateFindUIAfterPageScroll, this, found, string, options, maxMatchCount));
}

void FindController::findStringMatches(const String& string, FindOptions options, unsigned maxMatchCount)
{
    m_findMatches.clear();
    int indexForSelection;

    m_webPage->corePage()->findStringMatchingRanges(string, core(options), maxMatchCount, &m_findMatches, indexForSelection);

    Vector<Vector<IntRect> > matchRects;
    for (size_t i = 0; i < m_findMatches.size(); ++i) {
        Vector<IntRect> rects;
        m_findMatches[i]->textRects(rects);
        matchRects.append(rects);
    }

    m_webPage->send(Messages::WebPageProxy::DidFindStringMatches(string, matchRects, indexForSelection));
}

bool FindController::getFindIndicatorBitmapAndRect(Frame* frame, ShareableBitmap::Handle& handle, IntRect& selectionRect)
{
    selectionRect = enclosingIntRect(frame->selection()->bounds());

    // Selection rect can be empty for matches that are currently obscured from view.
    if (selectionRect.isEmpty())
        return false;

    IntSize backingStoreSize = selectionRect.size();
    backingStoreSize.scale(m_webPage->corePage()->deviceScaleFactor());

    // Create a backing store and paint the find indicator text into it.
    RefPtr<ShareableBitmap> findIndicatorTextBackingStore = ShareableBitmap::createShareable(backingStoreSize, ShareableBitmap::SupportsAlpha);
    if (!findIndicatorTextBackingStore)
        return false;

    OwnPtr<GraphicsContext> graphicsContext = findIndicatorTextBackingStore->createGraphicsContext();
    graphicsContext->scale(FloatSize(m_webPage->corePage()->deviceScaleFactor(), m_webPage->corePage()->deviceScaleFactor()));

    IntRect paintRect = selectionRect;
    paintRect.move(frame->view()->frameRect().x(), frame->view()->frameRect().y());
    paintRect.move(-frame->view()->scrollOffset());

    graphicsContext->translate(-paintRect.x(), -paintRect.y());
    frame->view()->setPaintBehavior(PaintBehaviorSelectionOnly | PaintBehaviorForceBlackText | PaintBehaviorFlattenCompositingLayers);
    frame->document()->updateLayout();

    frame->view()->paint(graphicsContext.get(), paintRect);
    frame->view()->setPaintBehavior(PaintBehaviorNormal);

    if (!findIndicatorTextBackingStore->createHandle(handle))
        return false;
    return true;
}

void FindController::getImageForFindMatch(uint32_t matchIndex)
{
    if (matchIndex >= m_findMatches.size())
        return;
    Frame* frame = m_findMatches[matchIndex]->startContainer()->document()->frame();
    if (!frame)
        return;

    VisibleSelection oldSelection = frame->selection()->selection();
    frame->selection()->setSelection(VisibleSelection(m_findMatches[matchIndex].get()));

    IntRect selectionRect;
    ShareableBitmap::Handle handle;
    getFindIndicatorBitmapAndRect(frame, handle, selectionRect);

    frame->selection()->setSelection(oldSelection);

    if (handle.isNull())
        return;

    m_webPage->send(Messages::WebPageProxy::DidGetImageForFindMatch(handle, matchIndex));
}

void FindController::selectFindMatch(uint32_t matchIndex)
{
    if (matchIndex >= m_findMatches.size())
        return;
    Frame* frame = m_findMatches[matchIndex]->startContainer()->document()->frame();
    if (!frame)
        return;
    frame->selection()->setSelection(VisibleSelection(m_findMatches[matchIndex].get()));
}

void FindController::hideFindUI()
{
    m_findMatches.clear();
    if (m_findPageOverlay)
        m_webPage->uninstallPageOverlay(m_findPageOverlay, true);

    PluginView* pluginView = pluginViewForFrame(m_webPage->mainFrame());
    
    if (pluginView)
        pluginView->findString(emptyString(), 0, 0);
    else
        m_webPage->corePage()->unmarkAllTextMatches();
    
    hideFindIndicator();
}

bool FindController::updateFindIndicator(Frame* selectedFrame, bool isShowingOverlay, bool shouldAnimate)
{
    if (!selectedFrame)
        return false;

    IntRect selectionRect;
    ShareableBitmap::Handle handle;
    if (!getFindIndicatorBitmapAndRect(selectedFrame, handle, selectionRect))
        return false;

    // We want the selection rect in window coordinates.
    IntRect selectionRectInWindowCoordinates = selectedFrame->view()->contentsToWindow(selectionRect);

    Vector<FloatRect> textRects;
    selectedFrame->selection()->getClippedVisibleTextRectangles(textRects);

    // We want the text rects in selection rect coordinates.
    Vector<FloatRect> textRectsInSelectionRectCoordinates;
    
    for (size_t i = 0; i < textRects.size(); ++i) {
        IntRect textRectInSelectionRectCoordinates = selectedFrame->view()->contentsToWindow(enclosingIntRect(textRects[i]));
        textRectInSelectionRectCoordinates.move(-selectionRectInWindowCoordinates.x(), -selectionRectInWindowCoordinates.y());

        textRectsInSelectionRectCoordinates.append(textRectInSelectionRectCoordinates);
    }            

    m_webPage->send(Messages::WebPageProxy::SetFindIndicator(selectionRectInWindowCoordinates, textRectsInSelectionRectCoordinates, m_webPage->corePage()->deviceScaleFactor(), handle, !isShowingOverlay, shouldAnimate));
    m_findIndicatorRect = selectionRectInWindowCoordinates;
    m_isShowingFindIndicator = true;

    return true;
}

void FindController::hideFindIndicator()
{
    if (!m_isShowingFindIndicator)
        return;

    ShareableBitmap::Handle handle;
    m_webPage->send(Messages::WebPageProxy::SetFindIndicator(FloatRect(), Vector<FloatRect>(), m_webPage->corePage()->deviceScaleFactor(), handle, false, true));
    m_isShowingFindIndicator = false;
}

void FindController::showFindIndicatorInSelection()
{
    Frame* selectedFrame = m_webPage->corePage()->focusController()->focusedOrMainFrame();
    if (!selectedFrame)
        return;
    
    updateFindIndicator(selectedFrame, false);
}

void FindController::deviceScaleFactorDidChange()
{
    ASSERT(isShowingOverlay());

    Frame* selectedFrame = frameWithSelection(m_webPage->corePage());
    if (!selectedFrame)
        return;

    updateFindIndicator(selectedFrame, true, false);
}

Vector<IntRect> FindController::rectsForTextMatches()
{
    Vector<IntRect> rects;

    for (Frame* frame = m_webPage->corePage()->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        Document* document = frame->document();
        if (!document)
            continue;

        IntRect visibleRect = frame->view()->visibleContentRect();
        Vector<IntRect> frameRects = document->markers()->renderedRectsForMarkers(DocumentMarker::TextMatch);
        IntPoint frameOffset(-frame->view()->scrollOffsetRelativeToDocument().width(), -frame->view()->scrollOffsetRelativeToDocument().height());
        frameOffset = frame->view()->convertToContainingWindow(frameOffset);

        for (Vector<IntRect>::iterator it = frameRects.begin(), end = frameRects.end(); it != end; ++it) {
            it->intersect(visibleRect);
            it->move(frameOffset.x(), frameOffset.y());
            rects.append(*it);
        }
    }

    return rects;
}

void FindController::pageOverlayDestroyed(PageOverlay*)
{
}

void FindController::willMoveToWebPage(PageOverlay*, WebPage* webPage)
{
    if (webPage)
        return;

    ASSERT(m_findPageOverlay);
    m_findPageOverlay = 0;
}
    
void FindController::didMoveToWebPage(PageOverlay*, WebPage*)
{
}

static const float shadowOffsetX = 0.0;
static const float shadowOffsetY = 1.0;
static const float shadowBlurRadius = 2.0;
static const float whiteFrameThickness = 1.0;

static const float overlayBackgroundRed = 0.1;
static const float overlayBackgroundGreen = 0.1;
static const float overlayBackgroundBlue = 0.1;
static const float overlayBackgroundAlpha = 0.25;

static Color overlayBackgroundColor(float fractionFadedIn)
{
    return Color(overlayBackgroundRed, overlayBackgroundGreen, overlayBackgroundBlue, overlayBackgroundAlpha * fractionFadedIn);
}

static Color holeShadowColor(float fractionFadedIn)
{
    return Color(0.0f, 0.0f, 0.0f, fractionFadedIn);
}

static Color holeFillColor(float fractionFadedIn)
{
    return Color(1.0f, 1.0f, 1.0f, fractionFadedIn);
}

void FindController::drawRect(PageOverlay* pageOverlay, GraphicsContext& graphicsContext, const IntRect& dirtyRect)
{
    float fractionFadedIn = pageOverlay->fractionFadedIn();

    Vector<IntRect> rects = rectsForTextMatches();

    // Draw the background.
    graphicsContext.fillRect(dirtyRect, overlayBackgroundColor(fractionFadedIn), ColorSpaceSRGB);

    {
        GraphicsContextStateSaver stateSaver(graphicsContext);

        graphicsContext.setShadow(FloatSize(shadowOffsetX, shadowOffsetY), shadowBlurRadius, holeShadowColor(fractionFadedIn), ColorSpaceSRGB);
        graphicsContext.setFillColor(holeFillColor(fractionFadedIn), ColorSpaceSRGB);

        // Draw white frames around the holes.
        for (size_t i = 0; i < rects.size(); ++i) {
            IntRect whiteFrameRect = rects[i];
            whiteFrameRect.inflate(1);

            graphicsContext.fillRect(whiteFrameRect);
        }
    }

    graphicsContext.setFillColor(Color::transparent, ColorSpaceSRGB);

    // Clear out the holes.
    for (size_t i = 0; i < rects.size(); ++i)
        graphicsContext.fillRect(rects[i]);

    if (!m_isShowingFindIndicator)
        return;

    if (Frame* selectedFrame = frameWithSelection(m_webPage->corePage())) {
        IntRect findIndicatorRect = selectedFrame->view()->contentsToWindow(enclosingIntRect(selectedFrame->selection()->bounds()));

        if (findIndicatorRect != m_findIndicatorRect)
            hideFindIndicator();
    }
}

bool FindController::mouseEvent(PageOverlay*, const WebMouseEvent& mouseEvent)
{
    if (mouseEvent.type() == WebEvent::MouseDown)
        hideFindUI();

    return false;
}

} // namespace WebKit
