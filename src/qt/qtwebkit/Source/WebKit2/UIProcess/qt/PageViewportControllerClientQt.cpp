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
#include "PageViewportControllerClientQt.h"

#include "WebPageProxy.h"
#include "qquickwebpage_p.h"
#include "qquickwebview_p.h"
#include "qwebkittest_p.h"
#include <QPointF>
#include <QTransform>
#include <QtQuick/qquickitem.h>
#include <WKAPICast.h>
#include <WebCore/FloatRect.h>
#include <WebCore/FloatSize.h>

using namespace WebCore;

namespace WebKit {

static const int kScaleAnimationDurationMillis = 250;

PageViewportControllerClientQt::PageViewportControllerClientQt(QQuickWebView* viewportItem, QQuickWebPage* pageItem)
    : m_viewportItem(viewportItem)
    , m_pageItem(pageItem)
    , m_scaleChange(this)
    , m_scrollChange(this)
    , m_touchInteraction(this, false /* shouldSuspend */)
    , m_scaleAnimation(new ScaleAnimation(this))
    , m_activeInteractionCount(0)
    , m_pinchStartScale(-1)
    , m_lastCommittedScale(-1)
    , m_zoomOutScale(0)
{
    m_scaleAnimation->setDuration(kScaleAnimationDurationMillis);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_viewportItem, SIGNAL(movementStarted()), SLOT(flickMoveStarted()), Qt::DirectConnection);
    connect(m_viewportItem, SIGNAL(movementEnded()), SLOT(flickMoveEnded()), Qt::DirectConnection);
    connect(m_viewportItem, SIGNAL(contentXChanged()), SLOT(pageItemPositionChanged()));
    connect(m_viewportItem, SIGNAL(contentYChanged()), SLOT(pageItemPositionChanged()));


    connect(m_scaleAnimation, SIGNAL(stateChanged(QAbstractAnimation::State, QAbstractAnimation::State)),
            SLOT(scaleAnimationStateChanged(QAbstractAnimation::State, QAbstractAnimation::State)));
}

void PageViewportControllerClientQt::ScaleAnimation::updateCurrentValue(const QVariant& value)
{
    // Resetting the end value, the easing curve or the duration of the scale animation
    // triggers a recalculation of the animation interval. This might change the current
    // value of the animated property.
    // Make sure we only act on animation value changes if the animation is active.
    if (!m_controllerClient->scaleAnimationActive())
        return;

    QRectF itemRect = value.toRectF();
    float itemScale = m_controllerClient->viewportScaleForRect(itemRect);

    m_controllerClient->setContentRectVisiblePositionAtScale(itemRect.topLeft(), itemScale);
}

void PageViewportControllerClientQt::ViewportInteractionTracker::begin()
{
    if (m_inProgress)
        return;

    m_inProgress = true;

    if (m_shouldSuspend)
        toImpl(m_controllerClient->m_viewportItem->pageRef())->suspendActiveDOMObjectsAndAnimations();

    ++(m_controllerClient->m_activeInteractionCount);
}

void PageViewportControllerClientQt::ViewportInteractionTracker::end()
{
    if (!m_inProgress)
        return;

    m_inProgress = false;

    ASSERT(m_controllerClient->m_activeInteractionCount > 0);

    if (!(--(m_controllerClient->m_activeInteractionCount)))
        toImpl(m_controllerClient->m_viewportItem->pageRef())->resumeActiveDOMObjectsAndAnimations();
}

PageViewportControllerClientQt::~PageViewportControllerClientQt()
{
}

void PageViewportControllerClientQt::setContentRectVisiblePositionAtScale(const QPointF& location, qreal itemScale)
{
    ASSERT(itemScale >= 0);

    scaleContent(itemScale);

    // To animate the position together with the scale we multiply the position with the current scale
    // and add it to the page position (displacement on the flickable contentItem because of additional items).
    QPointF newPosition(m_pageItem->position() + location * itemScale);

    m_viewportItem->setContentPos(newPosition);
}

void PageViewportControllerClientQt::animateContentRectVisible(const QRectF& contentRect)
{
    ASSERT(!scaleAnimationActive());
    ASSERT(!scrollAnimationActive());

    QRectF viewportRectInContentCoords = m_viewportItem->mapRectToWebContent(m_viewportItem->boundingRect());
    if (contentRect == viewportRectInContentCoords) {
        m_scaleChange.end();
        updateViewportController();
        return;
    }

    // Inform the web process about the requested visible content rect immediately so that new tiles
    // are rendered at the final destination during the animation.
    m_controller->didChangeContentsVisibility(contentRect.topLeft(), viewportScaleForRect(contentRect));

    // Since we have to animate scale and position at the same time the scale animation interpolates
    // from the current viewport rect in content coordinates to a visible rect of the content.
    m_scaleAnimation->setStartValue(viewportRectInContentCoords);
    m_scaleAnimation->setEndValue(contentRect);

    m_scaleAnimation->start();
}

void PageViewportControllerClientQt::flickMoveStarted()
{
    m_scrollChange.begin();
    m_lastScrollPosition = m_viewportItem->contentPos();
}

void PageViewportControllerClientQt::flickMoveEnded()
{
    // This method is called on the end of the pan or pan kinetic animation.
    m_scrollChange.end();
    updateViewportController();
}

void PageViewportControllerClientQt::pageItemPositionChanged()
{
    if (m_scaleChange.inProgress())
        return;

    QPointF newPosition = m_viewportItem->contentPos();

    updateViewportController(m_lastScrollPosition - newPosition);

    m_lastScrollPosition = newPosition;
}

void PageViewportControllerClientQt::scaleAnimationStateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State /*oldState*/)
{
    switch (newState) {
    case QAbstractAnimation::Running:
        m_scaleChange.begin();
        break;
    case QAbstractAnimation::Stopped:
        m_scaleChange.end();
        updateViewportController();
        break;
    default:
        break;
    }
}

void PageViewportControllerClientQt::touchBegin()
{
    // Check for sane event delivery. At this point neither a pan gesture nor a pinch gesture should be active.
    ASSERT(!m_viewportItem->isDragging());
    ASSERT(!(m_pinchStartScale > 0));

    m_controller->setHadUserInteraction(true);

    // Prevent resuming the page during transition between gestures while the user is interacting.
    // The content is suspended as soon as a pan or pinch gesture or an animation is started.
    m_touchInteraction.begin();
}

void PageViewportControllerClientQt::touchEnd()
{
    m_touchInteraction.end();
}

void PageViewportControllerClientQt::focusEditableArea(const QRectF& caretArea, const QRectF& targetArea)
{
    // This can only happen as a result of a user interaction.
    ASSERT(m_controller->hadUserInteraction());

    const float editingFixedScale = 2;
    float targetScale = m_controller->innerBoundedViewportScale(editingFixedScale);
    const QRectF viewportRect = m_viewportItem->boundingRect();

    qreal x;
    const qreal borderOffset = 10;
    if ((targetArea.width() + borderOffset) * targetScale <= viewportRect.width()) {
        // Center the input field in the middle of the view, if it is smaller than
        // the view at the scale target.
        x = viewportRect.center().x() - targetArea.width() * targetScale / 2.0;
    } else {
        // Ensure that the caret always has borderOffset contents pixels to the right
        // of it, and secondarily (if possible), that the area has borderOffset
        // contents pixels to the left of it.
        qreal caretOffset = caretArea.x() - targetArea.x();
        x = qMin(viewportRect.width() - (caretOffset + borderOffset) * targetScale, borderOffset * targetScale);
    }

    const QPointF hotspot = QPointF(targetArea.x(), targetArea.center().y());
    const QPointF viewportHotspot = QPointF(x, /* FIXME: visibleCenter */ viewportRect.center().y());

    QPointF endPosition = hotspot - viewportHotspot / targetScale;
    endPosition = m_controller->boundContentsPositionAtScale(endPosition, targetScale);
    QRectF endVisibleContentRect(endPosition, viewportRect.size() / targetScale);

    animateContentRectVisible(endVisibleContentRect);
}

void PageViewportControllerClientQt::zoomToAreaGestureEnded(const QPointF& touchPoint, const QRectF& targetArea)
{
    // This can only happen as a result of a user interaction.
    ASSERT(m_controller->hadUserInteraction());

    if (!targetArea.isValid())
        return;

    if (m_scrollChange.inProgress() || m_scaleChange.inProgress())
        return;

    const float margin = 10; // We want at least a little bit of margin.
    QRectF endArea = targetArea.adjusted(-margin, -margin, margin, margin);

    const QRectF viewportRect = m_viewportItem->boundingRect();

    const qreal minViewportScale = qreal(2.5);
    qreal targetScale = viewportRect.size().width() / endArea.size().width();
    targetScale = m_controller->innerBoundedViewportScale(qMin(minViewportScale, targetScale));
    qreal currentScale = m_pageItem->contentsScale();

    // We want to end up with the target area filling the whole width of the viewport (if possible),
    // and centralized vertically where the user requested zoom. Thus our hotspot is the center of
    // the targetArea x-wise and the requested zoom position, y-wise.
    const QPointF hotspot = QPointF(endArea.center().x(), touchPoint.y());
    const QPointF viewportHotspot = viewportRect.center();

    QPointF endPosition = hotspot - viewportHotspot / targetScale;
    endPosition = m_controller->boundContentsPositionAtScale(endPosition, targetScale);
    QRectF endVisibleContentRect(endPosition, viewportRect.size() / targetScale);

    enum { ZoomIn, ZoomBack, ZoomOut, NoZoom } zoomAction = ZoomIn;

    // Zoom back out if attempting to scale to the same current scale, or
    // attempting to continue scaling out from the inner most level.
    // Use fuzzy compare with a fixed error to be able to deal with largish differences due to pixel rounding.
    if (!m_scaleStack.isEmpty() && fuzzyCompare(targetScale, currentScale, 0.01)) {
        // If moving the viewport would expose more of the targetRect and move at least 40 pixels, update position but do not scale out.
        QRectF currentContentRect(m_viewportItem->mapRectToWebContent(viewportRect));
        QRectF targetIntersection = endVisibleContentRect.intersected(targetArea);
        if (!currentContentRect.contains(targetIntersection)
            && (qAbs(endVisibleContentRect.top() - currentContentRect.top()) >= 40
            || qAbs(endVisibleContentRect.left() - currentContentRect.left()) >= 40))
            zoomAction = NoZoom;
        else
            zoomAction = ZoomBack;
    } else if (fuzzyCompare(targetScale, m_zoomOutScale, 0.01))
        zoomAction = ZoomBack;
    else if (targetScale < currentScale)
        zoomAction = ZoomOut;

    switch (zoomAction) {
    case ZoomIn:
        m_scaleStack.append(ScaleStackItem(currentScale, m_viewportItem->contentPos().x() / currentScale));
        m_zoomOutScale = targetScale;
        break;
    case ZoomBack: {
        if (m_scaleStack.isEmpty()) {
            targetScale = m_controller->minimumScale();
            endPosition.setY(hotspot.y() - viewportHotspot.y() / targetScale);
            endPosition.setX(0);
            m_zoomOutScale = 0;
        } else {
            ScaleStackItem lastScale = m_scaleStack.takeLast();
            targetScale = lastScale.scale;
            // Recalculate endPosition and clamp it according to the new scale.
            endPosition.setY(hotspot.y() - viewportHotspot.y() / targetScale);
            endPosition.setX(lastScale.xPosition);
        }
        endPosition = m_controller->boundContentsPositionAtScale(endPosition, targetScale);
        endVisibleContentRect = QRectF(endPosition, viewportRect.size() / targetScale);
        break;
    }
    case ZoomOut:
        // Unstack all scale-levels deeper than the new level, so a zoom-back won't end up zooming in.
        while (!m_scaleStack.isEmpty() && m_scaleStack.last().scale >= targetScale)
            m_scaleStack.removeLast();
        m_zoomOutScale = targetScale;
        break;
    case NoZoom:
        break;
    }

    animateContentRectVisible(endVisibleContentRect);
}

void PageViewportControllerClientQt::clearRelativeZoomState()
{
    m_zoomOutScale = 0;
    m_scaleStack.clear();
}

QRectF PageViewportControllerClientQt::nearestValidVisibleContentsRect() const
{
    float targetScale = m_controller->innerBoundedViewportScale(m_pageItem->contentsScale());

    const QRectF viewportRect = m_viewportItem->boundingRect();
    QPointF viewportHotspot = viewportRect.center();
    // Keep the center at the position of the old center, and substract viewportHotspot / targetScale to get the top left position.
    QPointF endPosition = m_viewportItem->mapToWebContent(viewportHotspot) - viewportHotspot / targetScale;

    endPosition = m_controller->boundContentsPositionAtScale(endPosition, targetScale);
    return QRectF(endPosition, viewportRect.size() / targetScale);
}

void PageViewportControllerClientQt::setViewportPosition(const FloatPoint& contentsPoint)
{
    QPointF newPosition((m_pageItem->position() + QPointF(contentsPoint)) * m_pageItem->contentsScale());
    // The contentX and contentY property changes trigger a visible rect update.
    m_viewportItem->setContentPos(newPosition);
}

void PageViewportControllerClientQt::setPageScaleFactor(float localScale)
{
    scaleContent(localScale);
}

void PageViewportControllerClientQt::setContentsRectToNearestValidBounds()
{
    float targetScale = m_controller->innerBoundedViewportScale(m_pageItem->contentsScale());
    setContentRectVisiblePositionAtScale(nearestValidVisibleContentsRect().topLeft(), targetScale);
    updateViewportController();
}

bool PageViewportControllerClientQt::scrollAnimationActive() const
{
    return m_viewportItem->isFlicking();
}

void PageViewportControllerClientQt::panGestureStarted(const QPointF& position, qint64 eventTimestampMillis)
{
    // This can only happen as a result of a user interaction.
    ASSERT(m_touchInteraction.inProgress());

    m_viewportItem->handleFlickableMousePress(position, eventTimestampMillis);
    m_lastPinchCenterInViewportCoordinates = position;
}

void PageViewportControllerClientQt::panGestureRequestUpdate(const QPointF& position, qint64 eventTimestampMillis)
{
    m_viewportItem->handleFlickableMouseMove(position, eventTimestampMillis);
    m_lastPinchCenterInViewportCoordinates = position;
}

void PageViewportControllerClientQt::panGestureEnded(const QPointF& position, qint64 eventTimestampMillis)
{
    m_viewportItem->handleFlickableMouseRelease(position, eventTimestampMillis);
    m_lastPinchCenterInViewportCoordinates = position;
}

void PageViewportControllerClientQt::panGestureCancelled()
{
    // Reset the velocity samples of the flickable.
    // This should only be called by the recognizer if we have a recognized
    // pan gesture and receive a touch event with multiple touch points
    // (ie. transition to a pinch gesture) as it does not move the content
    // back inside valid bounds.
    // When the pinch gesture ends, the content is positioned and scaled
    // back to valid boundaries.
    m_viewportItem->cancelFlick();
}

bool PageViewportControllerClientQt::scaleAnimationActive() const
{
    return m_scaleAnimation->state() == QAbstractAnimation::Running;
}

void PageViewportControllerClientQt::cancelScrollAnimation()
{
    if (!scrollAnimationActive())
        return;

    // If the pan gesture recognizer receives a touch begin event
    // during an ongoing kinetic scroll animation of a previous
    // pan gesture, the animation is stopped and the content is
    // immediately positioned back to valid boundaries.

    m_viewportItem->cancelFlick();
    setContentsRectToNearestValidBounds();
}

void PageViewportControllerClientQt::interruptScaleAnimation()
{
    // This interrupts the scale animation exactly where it is, even if it is out of bounds.
    m_scaleAnimation->stop();
}

void PageViewportControllerClientQt::pinchGestureStarted(const QPointF& pinchCenterInViewportCoordinates)
{
    // This can only happen as a result of a user interaction.
    ASSERT(m_touchInteraction.inProgress());

    if (!m_controller->allowsUserScaling() || !m_viewportItem->isInteractive())
        return;

    clearRelativeZoomState();
    m_scaleChange.begin();

    m_lastPinchCenterInViewportCoordinates = pinchCenterInViewportCoordinates;
    m_pinchStartScale = m_pageItem->contentsScale();
}

void PageViewportControllerClientQt::pinchGestureRequestUpdate(const QPointF& pinchCenterInViewportCoordinates, qreal totalScaleFactor)
{
    if (!m_controller->allowsUserScaling() || !m_viewportItem->isInteractive())
        return;

    ASSERT(m_scaleChange.inProgress());
    ASSERT(m_pinchStartScale > 0);
    //  Changes of the center position should move the page even if the zoom factor does not change.
    const qreal pinchScale = m_pinchStartScale * totalScaleFactor;

    // Allow zooming out beyond mimimum scale on pages that do not explicitly disallow it.
    const qreal targetScale = m_controller->outerBoundedViewportScale(pinchScale);

    scaleContent(targetScale, m_viewportItem->mapToWebContent(pinchCenterInViewportCoordinates));

    const QPointF positionDiff = pinchCenterInViewportCoordinates - m_lastPinchCenterInViewportCoordinates;
    m_lastPinchCenterInViewportCoordinates = pinchCenterInViewportCoordinates;

    m_viewportItem->setContentPos(m_viewportItem->contentPos() - positionDiff);
}

void PageViewportControllerClientQt::pinchGestureEnded()
{
    if (m_pinchStartScale < 0)
        return;

    ASSERT(m_scaleChange.inProgress());
    m_pinchStartScale = -1;

    // This will take care of resuming the content, even if no animation was performed.
    animateContentRectVisible(nearestValidVisibleContentsRect());
}

void PageViewportControllerClientQt::pinchGestureCancelled()
{
    m_pinchStartScale = -1;
    m_scaleChange.end();
    updateViewportController();
}

void PageViewportControllerClientQt::didChangeContentsSize(const IntSize& newSize)
{
    m_pageItem->setContentsSize(QSizeF(newSize));

    // Emit for testing purposes, so that it can be verified that
    // we didn't do scale adjustment.
    emit m_viewportItem->experimental()->test()->contentsScaleCommitted();

    if (!m_scaleChange.inProgress() && !m_scrollChange.inProgress())
        setContentsRectToNearestValidBounds();
}

void PageViewportControllerClientQt::didChangeVisibleContents()
{
    qreal scale = m_pageItem->contentsScale();

    if (scale != m_lastCommittedScale)
        emit m_viewportItem->experimental()->test()->contentsScaleCommitted();
    m_lastCommittedScale = scale;

    // Ensure that updatePaintNode is always called before painting.
    m_pageItem->update();
}

void PageViewportControllerClientQt::didChangeViewportAttributes()
{
    clearRelativeZoomState();
    emit m_viewportItem->experimental()->test()->viewportChanged();
}

void PageViewportControllerClientQt::updateViewportController(const QPointF& trajectory)
{
    FloatPoint viewportPos = m_viewportItem->mapToWebContent(QPointF());
    m_controller->didChangeContentsVisibility(viewportPos, m_pageItem->contentsScale(), trajectory);
}

void PageViewportControllerClientQt::scaleContent(qreal itemScale, const QPointF& centerInCSSCoordinates)
{
    QPointF oldPinchCenterOnViewport = m_viewportItem->mapFromWebContent(centerInCSSCoordinates);
    m_pageItem->setContentsScale(itemScale);
    QPointF newPinchCenterOnViewport = m_viewportItem->mapFromWebContent(centerInCSSCoordinates);
    m_viewportItem->setContentPos(m_viewportItem->contentPos() + (newPinchCenterOnViewport - oldPinchCenterOnViewport));
}

float PageViewportControllerClientQt::viewportScaleForRect(const QRectF& rect) const
{
    return static_cast<float>(m_viewportItem->width()) / static_cast<float>(rect.width());
}

} // namespace WebKit

#include "moc_PageViewportControllerClientQt.cpp"
