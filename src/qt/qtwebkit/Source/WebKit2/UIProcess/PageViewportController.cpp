/*
 * Copyright (C) 2011, 2012 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 Benjamin Poulain <benjamin@webkit.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)
#include "PageViewportController.h"

#include "PageViewportControllerClient.h"
#include "WebPageProxy.h"
#include <WebCore/FloatRect.h>
#include <WebCore/FloatSize.h>
#include <wtf/MathExtras.h>

using namespace WebCore;

namespace WebKit {

bool fuzzyCompare(float a, float b, float epsilon)
{
    return std::abs(a - b) < epsilon;
}

PageViewportController::PageViewportController(WebKit::WebPageProxy* proxy, PageViewportControllerClient* client)
    : m_webPageProxy(proxy)
    , m_client(client)
    , m_allowsUserScaling(false)
    , m_minimumScaleToFit(1)
    , m_initiallyFitToViewport(true)
    , m_hadUserInteraction(false)
    , m_pageScaleFactor(1)
    , m_pendingPositionChange(false)
    , m_pendingScaleChange(false)
{
    // Initializing Viewport Raw Attributes to avoid random negative or infinity scale factors
    // if there is a race condition between the first layout and setting the viewport attributes for the first time.
    m_rawAttributes.minimumScale = 1;
    m_rawAttributes.maximumScale = 1;
    m_rawAttributes.userScalable = m_allowsUserScaling;

    // The initial scale might be implicit and set to -1, in this case we have to infer it
    // using the viewport size and the final layout size.
    // To be able to assert for valid scale we initialize it to -1.
    m_rawAttributes.initialScale = -1;

    ASSERT(m_client);
    m_client->setController(this);
}

float PageViewportController::innerBoundedViewportScale(float viewportScale) const
{
    return clampTo(viewportScale, m_minimumScaleToFit, m_rawAttributes.maximumScale);
}

float PageViewportController::outerBoundedViewportScale(float viewportScale) const
{
    if (m_allowsUserScaling) {
        // Bounded by [0.1, 10.0] like the viewport meta code in WebCore.
        float hardMin = std::max<float>(0.1, 0.5 * m_minimumScaleToFit);
        float hardMax = std::min<float>(10, 2 * m_rawAttributes.maximumScale);
        return clampTo(viewportScale, hardMin, hardMax);
    }
    return innerBoundedViewportScale(viewportScale);
}

float PageViewportController::deviceScaleFactor() const
{
    return m_webPageProxy->deviceScaleFactor();
}

static inline bool isIntegral(float value)
{
    return static_cast<int>(value) == value;
}

FloatPoint PageViewportController::pixelAlignedFloatPoint(const FloatPoint& framePosition)
{
#if PLATFORM(EFL)
    float effectiveScale = m_pageScaleFactor * deviceScaleFactor();
    if (!isIntegral(effectiveScale)) {
        // To avoid blurryness, modify the position so that it maps into a discrete device position.
        FloatPoint scaledPos(framePosition);

        // Scale by the effective scale factor to compute the screen-relative position.
        scaledPos.scale(effectiveScale, effectiveScale);

        // Round to integer boundaries.
        FloatPoint alignedPos = roundedIntPoint(scaledPos);

        // Convert back to CSS coordinates.
        alignedPos.scale(1 / effectiveScale, 1 / effectiveScale);

        return alignedPos;
    }
#endif

    return framePosition;
}

FloatPoint PageViewportController::boundContentsPositionAtScale(const WebCore::FloatPoint& framePosition, float scale)
{
    // We need to floor the viewport here as to allow aligning the content in device units. If not,
    // it might not be possible to scroll the last pixel and that affects fixed position elements.
    FloatRect bounds;
    bounds.setWidth(std::max(0.f, m_contentsSize.width() - floorf(m_viewportSize.width() / scale)));
    bounds.setHeight(std::max(0.f, m_contentsSize.height() - floorf(m_viewportSize.height() / scale)));

    FloatPoint position;
    position.setX(clampTo(framePosition.x(), bounds.x(), bounds.width()));
    position.setY(clampTo(framePosition.y(), bounds.y(), bounds.height()));

    return position;
}

FloatPoint PageViewportController::boundContentsPosition(const WebCore::FloatPoint& framePosition)
{
    return boundContentsPositionAtScale(framePosition, m_pageScaleFactor);
}

void PageViewportController::didCommitLoad()
{
    // Do not count the previous committed page contents as covered.
    m_lastFrameCoveredRect = FloatRect();

    // Do not continue to use the content size of the previous page.
    m_contentsSize = IntSize();

    // Reset the position to the top, page/history scroll requests may override this before we re-enable rendering.
    applyPositionAfterRenderingContents(FloatPoint());
}

void PageViewportController::didChangeContentsSize(const IntSize& newSize)
{
    m_contentsSize = newSize;

    bool minimumScaleUpdated = updateMinimumScaleToFit(false);

    if (m_initiallyFitToViewport) {
        // Restrict scale factors to m_minimumScaleToFit.
        ASSERT(m_minimumScaleToFit > 0);
        m_rawAttributes.initialScale = m_minimumScaleToFit;
        WebCore::restrictScaleFactorToInitialScaleIfNotUserScalable(m_rawAttributes);
    }

    if (minimumScaleUpdated)
        m_client->didChangeViewportAttributes();

    // We might have pending position change which is now possible.
    syncVisibleContents();
}

void PageViewportController::didRenderFrame(const IntSize& contentsSize, const IntRect& coveredRect)
{
    if (m_clientContentsSize != contentsSize) {
        m_clientContentsSize = contentsSize;
        // Only update the viewport's contents dimensions along with its render if the
        // size actually changed since animations on the page trigger DidRenderFrame
        // messages without causing dimension changes.
        m_client->didChangeContentsSize(contentsSize);
    }

    m_lastFrameCoveredRect = coveredRect;

    // Apply any scale or scroll position we locked to be set on the viewport
    // only when there is something to display there. The scale goes first to
    // avoid offsetting our deferred position by scaling at the viewport center.
    // All position and scale changes resulting from a web process event should
    // go through here to be applied on the viewport to avoid showing incomplete
    // tiles to the user during a few milliseconds.

    if (m_pendingScaleChange) {
        m_pendingScaleChange = false;
        m_client->setPageScaleFactor(m_pageScaleFactor);

        // The scale changed, we have to re-pixel align.
        m_pendingPositionChange = true;
        FloatPoint currentDiscretePos = roundedIntPoint(m_contentsPosition);
        FloatPoint pixelAlignedPos = pixelAlignedFloatPoint(currentDiscretePos);
        m_contentsPosition = boundContentsPosition(pixelAlignedPos);

        m_webPageProxy->scalePage(m_pageScaleFactor, roundedIntPoint(m_contentsPosition));
    }

    // There might be rendered frames not covering our requested position yet, wait for it.
    FloatRect endVisibleContentRect(m_contentsPosition, visibleContentsSize());
    if (m_pendingPositionChange && endVisibleContentRect.intersects(coveredRect)) {
        m_client->setViewportPosition(m_contentsPosition);
        m_pendingPositionChange = false;
    }
}

void PageViewportController::pageTransitionViewportReady()
{
    if (!m_rawAttributes.layoutSize.isEmpty()) {
        m_hadUserInteraction = false;
        float initialScale = m_initiallyFitToViewport ? m_minimumScaleToFit : m_rawAttributes.initialScale;
        applyScaleAfterRenderingContents(innerBoundedViewportScale(initialScale));
    }

    // At this point we should already have received the first viewport arguments and the requested scroll
    // position for the newly loaded page and sent our reactions to the web process. It's now safe to tell
    // the web process to start rendering the new page contents and possibly re-use the current tiles.
    // This assumes that all messages have been handled in order and that nothing has been pushed back on the event loop.
    m_webPageProxy->commitPageTransitionViewport();
}

void PageViewportController::pageDidRequestScroll(const IntPoint& cssPosition)
{
    // Ignore the request if suspended. Can only happen due to delay in event delivery.
    if (m_webPageProxy->areActiveDOMObjectsAndAnimationsSuspended())
        return;

    FloatPoint boundPosition = boundContentsPosition(FloatPoint(cssPosition));
    FloatPoint alignedPosition = pixelAlignedFloatPoint(boundPosition);
    FloatRect endVisibleContentRect(alignedPosition, visibleContentsSize());

    if (m_lastFrameCoveredRect.intersects(endVisibleContentRect))
        m_client->setViewportPosition(alignedPosition);
    else {
        // Keep the unbound position in case the contents size is changed later on.
        FloatPoint position = pixelAlignedFloatPoint(FloatPoint(cssPosition));
        applyPositionAfterRenderingContents(position);
    }
}

void PageViewportController::didChangeViewportSize(const FloatSize& newSize)
{
    if (newSize.isEmpty())
        return;

    m_viewportSize = newSize;

    // Let the WebProcess know about the new viewport size, so that
    // it can resize the content accordingly.
    m_webPageProxy->drawingArea()->setSize(roundedIntSize(newSize), IntSize(), IntSize());
}

void PageViewportController::didChangeContentsVisibility(const FloatPoint& position, float scale, const FloatPoint& trajectoryVector)
{
    if (!m_pendingPositionChange)
        m_contentsPosition = position;
    if (!m_pendingScaleChange)
        applyScaleAfterRenderingContents(scale);

    syncVisibleContents(trajectoryVector);
}

void PageViewportController::syncVisibleContents(const FloatPoint& trajectoryVector)
{
    DrawingAreaProxy* drawingArea = m_webPageProxy->drawingArea();
    if (!drawingArea || m_viewportSize.isEmpty() || m_contentsSize.isEmpty())
        return;

    FloatRect visibleContentsRect(boundContentsPosition(m_contentsPosition), visibleContentsSize());
    visibleContentsRect.intersect(FloatRect(FloatPoint::zero(), m_contentsSize));
    drawingArea->setVisibleContentsRect(visibleContentsRect, trajectoryVector);

    m_client->didChangeVisibleContents();
}

void PageViewportController::didChangeViewportAttributes(const WebCore::ViewportAttributes& newAttributes)
{
    if (newAttributes.layoutSize.isEmpty())
        return;

    m_rawAttributes = newAttributes;
    m_allowsUserScaling = !!m_rawAttributes.userScalable;
    m_initiallyFitToViewport = (m_rawAttributes.initialScale < 0);

    if (!m_initiallyFitToViewport)
        WebCore::restrictScaleFactorToInitialScaleIfNotUserScalable(m_rawAttributes);

    updateMinimumScaleToFit(true);

    // As the viewport attributes are calculated when loading pages, after load, or after
    // viewport resize, it is important that we inform the client of the new scale and
    // position, so that the content can be positioned correctly and pixel aligned.
    m_pendingPositionChange = true;
    m_pendingScaleChange = true;

    m_client->didChangeViewportAttributes();
}

FloatSize PageViewportController::visibleContentsSize() const
{
    return FloatSize(m_viewportSize.width() / m_pageScaleFactor, m_viewportSize.height() / m_pageScaleFactor);
}

void PageViewportController::applyScaleAfterRenderingContents(float scale)
{
    if (m_pageScaleFactor == scale)
        return;

    m_pageScaleFactor = scale;
    m_pendingScaleChange = true;
    syncVisibleContents();
}

void PageViewportController::applyPositionAfterRenderingContents(const FloatPoint& pos)
{
    if (m_contentsPosition == pos)
        return;

    m_contentsPosition = pos;
    m_pendingPositionChange = true;
    syncVisibleContents();
}

bool PageViewportController::updateMinimumScaleToFit(bool userInitiatedUpdate)
{
    if (m_viewportSize.isEmpty() || m_contentsSize.isEmpty())
        return false;

    bool currentlyScaledToFit = fuzzyCompare(m_pageScaleFactor, m_minimumScaleToFit, 0.0001);

    float minimumScale = WebCore::computeMinimumScaleFactorForContentContained(m_rawAttributes, WebCore::roundedIntSize(m_viewportSize), WebCore::roundedIntSize(m_contentsSize));

    if (minimumScale <= 0)
        return false;

    if (!fuzzyCompare(minimumScale, m_minimumScaleToFit, 0.0001)) {
        m_minimumScaleToFit = minimumScale;

        if (!m_webPageProxy->areActiveDOMObjectsAndAnimationsSuspended()) {
            if (!m_hadUserInteraction || (userInitiatedUpdate && currentlyScaledToFit))
                applyScaleAfterRenderingContents(m_minimumScaleToFit);
            else {
                // Ensure the effective scale stays within bounds.
                float boundedScale = innerBoundedViewportScale(m_pageScaleFactor);
                if (!fuzzyCompare(boundedScale, m_pageScaleFactor, 0.0001))
                    applyScaleAfterRenderingContents(boundedScale);
            }
        }

        return true;
    }
    return false;
}

} // namespace WebKit

#endif
