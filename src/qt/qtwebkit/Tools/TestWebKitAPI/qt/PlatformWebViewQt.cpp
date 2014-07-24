/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 University of Szeged. All rights reserved.
 * Copyright (C) 2013 Digia Plc. and/or its subsidiary(-ies).
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "PlatformWebView.h"
#include "qquickwebpage_p.h"
#include "qquickwebview_p.h"

#include <WebKit2/WKRetainPtr.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QQmlProperty>
#include <QtQuick/QQuickView>
#include <qpa/qwindowsysteminterface.h>

namespace TestWebKitAPI {

class WrapperWindow : public QQuickView {
    Q_OBJECT
public:
    WrapperWindow(QQuickWebView* view)
        : QQuickView(QUrl(QStringLiteral("data:text/plain,import QtQuick 2.0\nItem { objectName: 'root' }")))
        , m_view(view)
    {
        if (status() == QQuickView::Ready)
            handleStatusChanged(QQuickView::Ready);
        else
            connect(this, SIGNAL(statusChanged(QQuickView::Status)), SLOT(handleStatusChanged(QQuickView::Status)));
    }

private Q_SLOTS:
    void handleStatusChanged(QQuickView::Status status)
    {
        if (status != QQuickView::Ready)
            return;

        setGeometry(0, 0, 800, 600);

        setResizeMode(QQuickView::SizeRootObjectToView);
        m_view->setParentItem(rootObject());
        QQmlProperty::write(m_view, QStringLiteral("anchors.fill"), qVariantFromValue(rootObject()));

        m_view->experimental()->setRenderToOffscreenBuffer(true);

        QWindowSystemInterface::handleWindowActivated(this);
        m_view->page()->setFocus(true);
    }

private:
    QQuickWebView* m_view;
};

PlatformWebView::PlatformWebView(WKContextRef contextRef, WKPageGroupRef pageGroupRef)
{
    m_view = new QQuickWebView(contextRef, pageGroupRef);
    m_view->setAllowAnyHTTPSCertificateForLocalHost(true);
    m_view->componentComplete();

    m_window = new WrapperWindow(m_view);
}

PlatformWebView::~PlatformWebView()
{
    delete m_window;
}

void PlatformWebView::resizeTo(unsigned width, unsigned height)
{
    // If we do not have a platform window we will never get the necessary
    // resize event, so simulate it in that case to make sure the quickview is
    // resized to what the api tests expects.
    if (!m_window->handle()) {
        QRect newGeometry(m_window->x(), m_window->y(), width, height);
        QWindowSystemInterface::handleGeometryChange(m_window, newGeometry);
        QWindowSystemInterface::flushWindowSystemEvents();
    }

    m_window->resize(width, height);
}

WKPageRef PlatformWebView::page() const
{
    return m_view->pageRef();
}

void PlatformWebView::focus()
{
    m_view->setFocus(true);
}

void PlatformWebView::simulateSpacebarKeyPress()
{
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
    QCoreApplication::sendEvent(m_window, &event);
    QKeyEvent event2(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier);
    QCoreApplication::sendEvent(m_window, &event2);
}

void PlatformWebView::simulateAltKeyPress()
{
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Alt, Qt::NoModifier);
    QCoreApplication::sendEvent(m_window, &event);
    QKeyEvent event2(QEvent::KeyRelease, Qt::Key_Alt, Qt::NoModifier);
    QCoreApplication::sendEvent(m_window, &event2);
}

void PlatformWebView::simulateMouseMove(unsigned x, unsigned y)
{
    QPointF mousePos(x, y);
    QMouseEvent event(QEvent::MouseMove, mousePos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(m_window, &event);
}

void PlatformWebView::simulateRightClick(unsigned x, unsigned y)
{
    QPointF mousePos(x, y);
    QMouseEvent event2(QEvent::MouseButtonPress, mousePos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    QCoreApplication::sendEvent(m_window, &event2);
    QMouseEvent event3(QEvent::MouseButtonRelease, mousePos,  Qt::RightButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(m_window, &event3);
}

} // namespace TestWebKitAPI

#include "PlatformWebViewQt.moc"
