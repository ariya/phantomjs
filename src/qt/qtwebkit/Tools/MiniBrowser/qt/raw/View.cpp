/*
    Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "View.h"

#include <QDebug>
#include <QExposeEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMatrix4x4>
#include <QResizeEvent>
#include <QUrl>

#include <WebKit2/WKContext.h>
#include <WebKit2/WKPageGroup.h>
#include <WebKit2/WKPreferences.h>
#include <WebKit2/WKPreferencesPrivate.h>
#include <WebKit2/WKStringQt.h>
#include <WebKit2/WKURL.h>


static WKContextRef createWebContext()
{
    return WKContextCreate();
}

static WKPageGroupRef createWebPageGroup(const QString& name)
{
    WKPageGroupRef pageGroup =WKPageGroupCreateWithIdentifier(WKStringCreateWithQString(name));
    WKPreferencesRef preferences = WKPageGroupGetPreferences(pageGroup);
    WKPreferencesSetAcceleratedCompositingEnabled(preferences, true);
    WKPreferencesSetFrameFlatteningEnabled(preferences, true);

    return pageGroup;
}

View::~View()
{
    delete m_context;
    delete m_webView;
}

View::View(const QString& url)
    : m_url(url)
    , m_active(false)
{
    setSurfaceType(OpenGLSurface);
    setGeometry(50, 50, 980, 600);
    setFlags(Qt::Window | Qt::WindowTitleHint);
    create();

    m_context = new QOpenGLContext;
    m_context->create();

    m_webView = new QRawWebView(createWebContext(), createWebPageGroup(QString()), this);
    m_webView->create();
    WKPageSetUseFixedLayout(m_webView->pageRef(), true);
}


void View::exposeEvent(QExposeEvent* event)
{
    if (!m_active) {
        m_active = true;
        WKPageLoadURL(m_webView->pageRef(), WKURLCreateWithUTF8CString(m_url.toLocal8Bit().data()));

        m_webView->setFocused(true);
        m_webView->setVisible(true);
        m_webView->setActive(true);
    }
}

void View::resizeEvent(QResizeEvent* event)
{
    m_webView->setSize(event->size());
}

void View::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == m_paintTimer.timerId()) {
        m_paintTimer.stop();
        m_context->makeCurrent(this);
        m_webView->paint(QMatrix4x4(), 1, 0);
        m_context->swapBuffers(this);
    }
}

void View::viewNeedsDisplay(const QRect&)
{
    if (!m_paintTimer.isActive())
        m_paintTimer.start(0, this);
}

void View::viewRequestedCursorOverride(const QCursor& cursor)
{
    QGuiApplication::setOverrideCursor(cursor);
}

void View::doneWithKeyEvent(const QKeyEvent* event, bool wasHandled)
{
    if (wasHandled || event->isAccepted())
        return;

    switch (event->key()) {
    case Qt::Key_Backspace: {
        WKPageRef page = m_webView->pageRef();
        if (WKPageCanGoBack(page))
            WKPageGoBack(page);
        break;
    }
    }
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);

    View view(app.arguments().size() > 1 ? app.arguments().at(1) : QStringLiteral("http://www.google.com"));
    view.show();
    return app.exec();
}
