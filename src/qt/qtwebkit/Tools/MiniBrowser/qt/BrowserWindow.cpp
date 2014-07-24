/*
 * Copyright (C) 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010 University of Szeged
 *
 * All rights reserved.
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

#include "BrowserWindow.h"

#include "private/qquickwebpage_p.h"
#include "private/qquickwebview_p.h"
#include "utils.h"

#include <QQmlEngine>
#include <QDir>
#include <QPointF>

BrowserWindow::BrowserWindow(WindowOptions* options)
    : m_windowOptions(options)
{
    setTitle("MiniBrowser");
    setFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setResizeMode(QQuickView::SizeRootObjectToView);

    // This allows starting MiniBrowser from the build directory without previously defining QML_IMPORT_PATH.
    QDir qmlImportDir = QDir(QCoreApplication::applicationDirPath());
    qmlImportDir.cd("../imports");
    engine()->addImportPath(qmlImportDir.canonicalPath());

    Utils* utils = new Utils(this);
    engine()->rootContext()->setContextProperty("utils", utils);
    engine()->rootContext()->setContextProperty("options", options);
    setSource(QUrl("qrc:///qml/BrowserWindow.qml"));
    connect(rootObject(), SIGNAL(pageTitleChanged(QString)), this, SLOT(onTitleChanged(QString)));
    connect(rootObject(), SIGNAL(newWindow(QString)), this, SLOT(newWindow(QString)));
    if (options->startFullScreen())
        showFullScreen();
    else {
        if (options->startMaximized())
            setWindowState(Qt::WindowMaximized);
        else
            resize(options->requestedWindowSize());
        show();
    }

    QQmlContext* context = rootContext();
    context->setContextProperty("Window", this);

    if (!options->userAgent().isNull())
        webViewExperimental()->setUserAgent(options->userAgent());

    // Zoom values are chosen to be like in Mozilla Firefox 3.
    m_zoomLevels << 0.3 << 0.5 << 0.67 << 0.8 << 0.9
                 << 1
                 << 1.11 << 1.22 << 1.33 << 1.5 << 1.7 << 2 << 2.4 << 3;

    m_currentZoomLevel = m_zoomLevels.indexOf(1);
}

QQuickWebView* BrowserWindow::webView() const
{
    return rootObject()->property("webview").value<QQuickWebView*>();
}

QQuickWebViewExperimental* BrowserWindow::webViewExperimental() const
{
    return webView()->property("experimental").value<QQuickWebViewExperimental*>();
}

void BrowserWindow::load(const QString& url)
{
    QUrl completedUrl = Utils::urlFromUserInput(url);
    QMetaObject::invokeMethod(rootObject(), "load", Qt::DirectConnection, Q_ARG(QVariant, completedUrl));
}

void BrowserWindow::reload()
{
    QMetaObject::invokeMethod(rootObject(), "reload", Qt::DirectConnection);
}

void BrowserWindow::focusAddressBar()
{
    QMetaObject::invokeMethod(rootObject(), "focusAddressBar", Qt::DirectConnection);
}

void BrowserWindow::toggleFind()
{
    QMetaObject::invokeMethod(rootObject(), "toggleFind", Qt::DirectConnection);
}

BrowserWindow* BrowserWindow::newWindow(const QString& url)
{
    BrowserWindow* window = new BrowserWindow(m_windowOptions);
    window->load(url);
    return window;
}

void BrowserWindow::updateVisualMockTouchPoints(const QList<QTouchEvent::TouchPoint>& touchPoints)
{
    if (touchPoints.isEmpty()) {
        // Hide all touch indicator items.
        foreach (QQuickItem* item, m_activeMockComponents.values())
            item->setProperty("pressed", false);

        return;
    }

    foreach (const QTouchEvent::TouchPoint& touchPoint, touchPoints) {
        QQuickItem* mockTouchPointItem = m_activeMockComponents.value(touchPoint.id());

        if (!mockTouchPointItem) {
            QQmlComponent touchMockPointComponent(engine(), QUrl("qrc:///qml/MockTouchPoint.qml"));
            mockTouchPointItem = qobject_cast<QQuickItem*>(touchMockPointComponent.create());
            Q_ASSERT(mockTouchPointItem);
            m_activeMockComponents.insert(touchPoint.id(), mockTouchPointItem);
            mockTouchPointItem->setProperty("pointId", QVariant(touchPoint.id()));
            mockTouchPointItem->setParent(rootObject());
            mockTouchPointItem->setParentItem(rootObject());
        }

        QRectF touchRect = touchPoint.rect();
        mockTouchPointItem->setX(touchRect.center().x());
        mockTouchPointItem->setY(touchRect.center().y());
        mockTouchPointItem->setWidth(touchRect.width());
        mockTouchPointItem->setHeight(touchRect.height());
        mockTouchPointItem->setProperty("pressed", QVariant(touchPoint.state() != Qt::TouchPointReleased));
    }
}

void BrowserWindow::screenshot()
{
}

BrowserWindow::~BrowserWindow()
{
}

void BrowserWindow::onTitleChanged(QString title)
{
    if (title.isEmpty())
        title = QLatin1String("MiniBrowser");
    setTitle(title);
}

void BrowserWindow::zoomIn()
{
    if (m_currentZoomLevel < m_zoomLevels.size() - 1)
        ++m_currentZoomLevel;
    webView()->setZoomFactor(m_zoomLevels[m_currentZoomLevel]);
}

void BrowserWindow::zoomOut()
{
    if (m_currentZoomLevel > 0)
        --m_currentZoomLevel;
    webView()->setZoomFactor(m_zoomLevels[m_currentZoomLevel]);
}

void BrowserWindow::keyPressEvent(QKeyEvent* event)
{

    if (event->modifiers() & Qt::ControlModifier) {
        bool handled;
        if ((handled = event->key() == Qt::Key_Plus))
            zoomIn();
        else if ((handled = event->key() == Qt::Key_Minus))
            zoomOut();

        if (handled) {
            event->accept();
            return;
        }
    }
    QQuickView::keyPressEvent(event);
}

void BrowserWindow::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier && event->orientation() == Qt::Vertical) {
        if (event->delta() > 0)
            zoomIn();
        else
            zoomOut();

        event->accept();
        return;
    }
    QQuickView::wheelEvent(event);
}
