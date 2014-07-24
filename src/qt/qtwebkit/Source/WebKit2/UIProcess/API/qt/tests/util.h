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
// Functions and macros that really need to be in QTestLib

#include <QEventLoop>
#include <QSignalSpy>
#include <QTimer>

class QQuickWebView;
class QWebLoadRequest;

#if !defined(TESTS_SOURCE_DIR)
#define TESTS_SOURCE_DIR ""
#endif

void addQtWebProcessToPath();
bool waitForSignal(QObject*, const char* signal, int timeout = 10000);
void suppressDebugOutput();

#if defined(HAVE_QTQUICK) && HAVE_QTQUICK
bool waitForLoadSucceeded(QQuickWebView* webView, int timeout = 10000);
bool waitForLoadFailed(QQuickWebView* webView, int timeout = 10000);
bool waitForViewportReady(QQuickWebView* webView, int timeout = 10000);

class LoadSpy : public QEventLoop {
    Q_OBJECT
public:
    LoadSpy(QQuickWebView* webView);
Q_SIGNALS:
    void loadSucceeded();
    void loadFailed();
private Q_SLOTS:
    void onLoadingChanged(QWebLoadRequest* loadRequest);
};

class LoadStartedCatcher : public QObject {
    Q_OBJECT
public:
    LoadStartedCatcher(QQuickWebView* webView);
    virtual ~LoadStartedCatcher() { }
public Q_SLOTS:
    void onLoadingChanged(QWebLoadRequest* loadRequest);
Q_SIGNALS:
    void finished();
private:
    QQuickWebView* m_webView;
};
#endif
