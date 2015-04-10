/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef BrowserWindow_h
#define BrowserWindow_h

#include "MiniBrowserApplication.h"
#include <QStringList>
#include <QtQuick/QQuickView>

class QQuickWebView;
class QQuickWebViewExperimental;

class BrowserWindow : public QQuickView {
    Q_OBJECT

public:
    BrowserWindow(WindowOptions*);
    ~BrowserWindow();
    void load(const QString& url);
    void reload();
    void focusAddressBar();
    void toggleFind();
    QQuickWebView* webView() const;
    QQuickWebViewExperimental* webViewExperimental() const;

    void updateVisualMockTouchPoints(const QList<QTouchEvent::TouchPoint>& touchPoints);

public Q_SLOTS:
    BrowserWindow* newWindow(const QString& url = "about:blank");

protected Q_SLOTS:
    void screenshot();

private Q_SLOTS:
    void onTitleChanged(QString);

private:
    void zoomIn();
    void zoomOut();

    virtual void keyPressEvent(QKeyEvent*);
    virtual void wheelEvent(QWheelEvent*);

    WindowOptions* m_windowOptions;
    QHash<int, QQuickItem*> m_activeMockComponents;
    QVector<qreal> m_zoomLevels;
    unsigned m_currentZoomLevel;
};

#endif
