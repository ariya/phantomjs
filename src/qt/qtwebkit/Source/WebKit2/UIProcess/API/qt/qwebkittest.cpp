/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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
#include "qwebkittest_p.h"

#include "PageViewportControllerClientQt.h"
#include "qquickwebview_p_p.h"
#include <QMutableListIterator>
#include <QTouchEvent>
#include <QWheelEvent>
#include <qpa/qwindowsysteminterface.h>

using namespace WebKit;

QWebKitTest::QWebKitTest(QQuickWebViewPrivate* webViewPrivate, QObject* parent)
    : QObject(parent)
    , m_webViewPrivate(webViewPrivate)
{
}

QWebKitTest::~QWebKitTest()
{
}

static QTouchEvent::TouchPoint touchPoint(qreal x, qreal y)
{
    QPointF localPos(x, y);

    QTouchEvent::TouchPoint point;
    point.setId(1);
    point.setLastPos(localPos);
    QRectF touchRect(0, 0, 40, 40);
    touchRect.moveCenter(localPos);
    point.setRect(touchRect);
    point.setPressure(1);

    return point;
}

bool QWebKitTest::sendTouchEvent(QQuickWebView* window, QEvent::Type type, const QList<QTouchEvent::TouchPoint>& points, ulong timestamp)
{
    ASSERT(window);

    static QTouchDevice* device = 0;
    if (!device) {
        device = new QTouchDevice;
        device->setType(QTouchDevice::TouchScreen);
        QWindowSystemInterface::registerTouchDevice(device);
    }

    Qt::TouchPointStates touchPointStates = 0;
    foreach (const QTouchEvent::TouchPoint& touchPoint, points)
        touchPointStates |= touchPoint.state();

    QTouchEvent event(type, device, Qt::NoModifier, touchPointStates, points);
    event.setTimestamp(timestamp);
    event.setAccepted(false);

    window->touchEvent(&event);

    return event.isAccepted();
}

bool QWebKitTest::touchTap(QObject* item, qreal x, qreal y, int delay)
{
    QQuickWebView* window = qobject_cast<QQuickWebView*>(item);

    if (!window) {
        qWarning("Touch event \"TouchBegin\" not accepted by receiving item");
        return false;
    }

    // FIXME: implement delay using QTest::qWait() or similar.
    Q_UNUSED(delay);

    QList<QTouchEvent::TouchPoint> points;
    points.append(touchPoint(x, y));

    points[0].setState(Qt::TouchPointPressed);
    sendTouchEvent(window, QEvent::TouchBegin, points, QDateTime::currentMSecsSinceEpoch());

    points[0].setState(Qt::TouchPointReleased);
    sendTouchEvent(window, QEvent::TouchEnd, points, QDateTime::currentMSecsSinceEpoch());

    return true;
}

bool QWebKitTest::touchDoubleTap(QObject* item, qreal x, qreal y, int delay)
{
    if (!touchTap(item, x, y, delay))
        return false;

    if (!touchTap(item, x, y, delay))
        return false;

    return true;
}

bool QWebKitTest::wheelEvent(QObject* item, qreal x, qreal y, int delta, Qt::Orientation orient)
{
    QQuickWebView* window = qobject_cast<QQuickWebView*>(item);

    if (!window) {
        qWarning("Wheel event not accepted by receiving item");
        return false;
    }

    QWheelEvent event(QPointF(x, y), delta, Qt::NoButton, Qt::NoModifier, orient);
    event.setTimestamp(QDateTime::currentMSecsSinceEpoch());
    event.setAccepted(false);

    window->wheelEvent(&event);

    return event.isAccepted();
}

QSize QWebKitTest::contentsSize() const
{
    return QSize(m_webViewPrivate->pageView->contentsSize().toSize());
}

static inline QJsonObject toJsonObject(const QSizeF& sizeF)
{
    QJsonObject result;
    result.insert(QLatin1String("width"), sizeF.width());
    result.insert(QLatin1String("height"), sizeF.height());
    return result;
}

QJsonObject QWebKitTest::viewport() const
{
    QJsonObject viewportData;
    if (const PageViewportController* const viewportHandler = m_webViewPrivate->viewportController()) {
        viewportData.insert(QLatin1String("layoutSize"), toJsonObject(viewportHandler->contentsLayoutSize()));
        viewportData.insert(QLatin1String("isScalable"), viewportHandler->allowsUserScaling());
        viewportData.insert(QLatin1String("minimumScale"), viewportHandler->minimumScale());
        viewportData.insert(QLatin1String("maximumScale"), viewportHandler->maximumScale());
    } else {
        viewportData.insert(QLatin1String("initialScale"), 1.0);
        viewportData.insert(QLatin1String("layoutSize"), toJsonObject(QSizeF()));
        viewportData.insert(QLatin1String("isScalable"), false);
        viewportData.insert(QLatin1String("minimumScale"), 1.0);
        viewportData.insert(QLatin1String("maximumScale"), 1.0);
    }
    return viewportData;
}

QVariant QWebKitTest::devicePixelRatio() const
{
    if (const PageViewportController* const viewport = m_webViewPrivate->viewportController())
        return viewport->deviceScaleFactor();
    return 1.0;
}

QVariant QWebKitTest::contentsScale() const
{
    if (const PageViewportController* const viewport = m_webViewPrivate->viewportController())
        return viewport->currentScale();
    return 1.0;
}
