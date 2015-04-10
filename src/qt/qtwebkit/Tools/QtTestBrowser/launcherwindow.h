/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Girish Ramakrishnan <girish@forwardbias.in>
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
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

#ifndef launcherwindow_h
#define launcherwindow_h

#include <QtNetwork/QNetworkRequest>

#ifndef QT_NO_OPENGL
#include <QtOpenGL/QGLWidget>
#endif

#include <QDebug>

#include <cstdio>
#include <qevent.h>
#include <qwebelement.h>
#include <qwebframe.h>
#include <qwebinspector.h>
#include <qwebsettings.h>

#include "mainwindow.h"
#include "urlloader.h"
#include "utils.h"
#include "webinspector.h"
#include "webpage.h"
#include "webview.h"

QT_BEGIN_NAMESPACE
class QPropertyAnimation;
class QLineEdit;
QT_END_NAMESPACE

struct HighlightedElement;

class WindowOptions {
public:
    WindowOptions()
        : useGraphicsView(false)
        , useDiskCache(false)
        , useCompositing(true)
        , useTiledBackingStore(false)
        , useWebGL(false)
        , useWebAudio(false)
        , useFrameFlattening(false)
        , cacheWebView(false)
        , showFrameRate(false)
        , resizesToContents(false)
        , viewportUpdateMode(QGraphicsView::MinimalViewportUpdate)
        , useLocalStorage(false)
        , useOfflineStorageDatabase(false)
        , useOfflineWebApplicationCache(false)
        , useDiskCookies(true)
        , enableScrollAnimator(false)
        , offlineStorageDefaultQuotaSize(0)
#ifndef QT_NO_OPENGL
        , useQGLWidgetViewport(false)
#endif
        , printLoadedUrls(false)
        , startMaximized(false)
    {
    }

    bool useGraphicsView;
    bool useDiskCache;
    bool useCompositing;
    bool useTiledBackingStore;
    bool useWebGL;
    bool useWebAudio;
    bool useFrameFlattening;
    bool cacheWebView;
    bool showFrameRate;
    bool resizesToContents;
    QGraphicsView::ViewportUpdateMode viewportUpdateMode;
    bool useLocalStorage;
    bool useOfflineStorageDatabase;
    bool useOfflineWebApplicationCache;
    bool useDiskCookies;
    bool enableScrollAnimator;
    quint64 offlineStorageDefaultQuotaSize;
#ifndef QT_NO_OPENGL
    bool useQGLWidgetViewport;
#endif
    bool printLoadedUrls;
    QUrl inspectorUrl;
    quint16 remoteInspectorPort;
    bool startMaximized;
};

class LauncherWindow : public MainWindow {
    Q_OBJECT

public:
    LauncherWindow(WindowOptions* data = 0, QGraphicsScene* sharedScene = 0);
    virtual ~LauncherWindow();

    void sendTouchEvent();

    bool eventFilter(QObject* obj, QEvent* event);

protected Q_SLOTS:
    void loadStarted();
    void loadFinished();

    void showLinkHover(const QString &link, const QString &toolTip);

    void zoomIn();
    void zoomOut();
    void resetZoom();
    void toggleZoomTextOnly(bool on);
    void zoomAnimationFinished();

    void print();
    void screenshot();

    void setEditable(bool on);

    /* void dumpPlugins() */
    void dumpHtml();

    void loadURLListFromFile();

    void setDiskCache(bool enable);
    void setTouchMocking(bool on);
    void toggleWebView(bool graphicsBased);
    void toggleAcceleratedCompositing(bool toggle);
    void toggleTiledBackingStore(bool toggle);
    void toggleResizesToContents(bool toggle);
    void toggleWebGL(bool toggle);
    void toggleWebAudio(bool toggle);
    void toggleSpatialNavigation(bool b);
    void toggleFullScreenMode(bool enable);
    void toggleFrameFlattening(bool toggle);
    void toggleJavaScriptEnabled(bool enable);
    void toggleInterruptingJavaScriptEnabled(bool enable);
    void toggleJavascriptCanOpenWindows(bool enable);
    void toggleAutoLoadImages(bool enable);
    void setUseDiskCookies(bool enable);
    void clearCookies();
    void togglePlugins(bool enable);
    void toggleLocalStorage(bool toggle);
    void toggleOfflineStorageDatabase(bool toggle);
    void toggleOfflineWebApplicationCache(bool toggle);
    void toggleScrollAnimator(bool toggle);
    void setOfflineStorageDefaultQuota();
#ifndef QT_NO_LINEEDIT
    void showFindBar();
    void find(int mode);
#endif
#ifndef QT_NO_OPENGL
    void toggleQGLWidgetViewport(bool enable);
#endif

    void changeViewportUpdateMode(int mode);
    void animatedFlip();
    void animatedYFlip();
    void selectElements();
    void clearSelection();
    void showFPS(bool enable);
    void showUserAgentDialog();

    void printURL(const QUrl&);
#if !defined(QT_NO_FILEDIALOG) && !defined(QT_NO_MESSAGEBOX)
    void downloadRequest(const QNetworkRequest&);
    void fileDownloadFinished();
#endif

public Q_SLOTS:
    LauncherWindow* newWindow();
    LauncherWindow* cloneWindow();
    void updateFPS(int fps);

Q_SIGNALS:
    void enteredFullScreenMode(bool on);

private:
    void init();
    void initializeView();
    void createChrome();
    void applyPrefs();
    void applyZoom();

    bool isGraphicsBased() const;

private:
    static QVector<int> m_zoomLevels;
    int m_currentZoom;

    UrlLoader* m_urlLoader;

    QWidget* m_view;
    WebInspector* m_inspector;

    WindowOptions m_windowOptions;

    QAction* m_formatMenuAction;

    QPropertyAnimation* m_zoomAnimation;
#if !defined(QT_NO_FILEDIALOG) && !defined(QT_NO_MESSAGEBOX)
    QNetworkReply* m_reply;
#endif
    QList<QTouchEvent::TouchPoint> m_touchPoints;
    QList<HighlightedElement> m_highlightedElements;
    bool m_touchMocking;

    QString m_inputUrl;
#ifndef QT_NO_LINEEDIT
    QToolBar* m_findBar;
    QLineEdit* m_lineEdit;
    int m_findFlag;
    static const int s_findNormalFlag = 0;
#endif
};

#endif
