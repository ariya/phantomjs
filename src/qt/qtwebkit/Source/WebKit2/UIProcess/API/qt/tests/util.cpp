/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

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

#include "util.h"

#include <QtTest/QtTest>
#include <stdio.h>

#if defined(HAVE_QTQUICK) && HAVE_QTQUICK
#include "private/qquickwebview_p.h"
#include "private/qwebloadrequest_p.h"
#endif

void addQtWebProcessToPath()
{
    // Since tests won't find ./QtWebProcess, add it to PATH (at the end to prevent surprises).
    // QWP_PATH should be defined by qmake.
    qputenv("PATH", qgetenv("PATH") + ":" + QWP_PATH);
}

/**
 * Starts an event loop that runs until the given signal is received.
 * Optionally the event loop
 * can return earlier on a timeout.
 *
 * \return \p true if the requested signal was received
 *         \p false on timeout
 */
bool waitForSignal(QObject* obj, const char* signal, int timeout)
{
    QEventLoop loop;
    QObject::connect(obj, signal, &loop, SLOT(quit()));
    QTimer timer;
    QSignalSpy timeoutSpy(&timer, SIGNAL(timeout()));
    if (timeout > 0) {
        QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        timer.setSingleShot(true);
        timer.start(timeout);
    }
    loop.exec();
    return timeoutSpy.isEmpty();
}

static void messageHandler(QtMsgType type, const QMessageLogContext&, const QString& message)
{
    if (type == QtCriticalMsg) {
        fprintf(stderr, "%s\n", qPrintable(message));
        return;
    }
    // Do nothing
}

void suppressDebugOutput()
{
    qInstallMessageHandler(messageHandler); \
    if (qgetenv("QT_WEBKIT_SUPPRESS_WEB_PROCESS_OUTPUT").isEmpty()) \
        qputenv("QT_WEBKIT_SUPPRESS_WEB_PROCESS_OUTPUT", "1");
}

#if defined(HAVE_QTQUICK) && HAVE_QTQUICK
bool waitForLoadSucceeded(QQuickWebView* webView, int timeout)
{
    QEventLoop loop;
    LoadSpy loadSpy(webView);
    QObject::connect(&loadSpy, SIGNAL(loadSucceeded()), &loop, SLOT(quit()));
    QTimer timer;
    QSignalSpy timeoutSpy(&timer, SIGNAL(timeout()));
    if (timeout > 0) {
        QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        timer.setSingleShot(true);
        timer.start(timeout);
    }
    loop.exec();
    return timeoutSpy.isEmpty();
}

bool waitForLoadFailed(QQuickWebView* webView, int timeout)
{
    QEventLoop loop;
    LoadSpy loadSpy(webView);
    QObject::connect(&loadSpy, SIGNAL(loadFailed()), &loop, SLOT(quit()));
    QTimer timer;
    QSignalSpy timeoutSpy(&timer, SIGNAL(timeout()));
    if (timeout > 0) {
        QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        timer.setSingleShot(true);
        timer.start(timeout);
    }
    loop.exec();
    return timeoutSpy.isEmpty();
}

bool waitForViewportReady(QQuickWebView* webView, int timeout)
{
    // The viewport is locked until the first frame of a page load is rendered.
    // The QQuickView needs to be shown for this to succeed.
    return waitForSignal(webView->experimental(), SIGNAL(loadVisuallyCommitted()), timeout);
}

LoadSpy::LoadSpy(QQuickWebView* webView)
{
    connect(webView, SIGNAL(loadingChanged(QWebLoadRequest*)), SLOT(onLoadingChanged(QWebLoadRequest*)));
}

void LoadSpy::onLoadingChanged(QWebLoadRequest* loadRequest)
{
    if (loadRequest->status() == QQuickWebView::LoadSucceededStatus)
        emit loadSucceeded();
    else if (loadRequest->status() == QQuickWebView::LoadFailedStatus)
        emit loadFailed();
}

LoadStartedCatcher::LoadStartedCatcher(QQuickWebView* webView)
    : m_webView(webView)
{
    connect(m_webView, SIGNAL(loadingChanged(QWebLoadRequest*)), this, SLOT(onLoadingChanged(QWebLoadRequest*)));
}

void LoadStartedCatcher::onLoadingChanged(QWebLoadRequest* loadRequest)
{
    if (loadRequest->status() == QQuickWebView::LoadStartedStatus) {
        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);

        QCOMPARE(m_webView->loading(), true);
    }
}
#endif
