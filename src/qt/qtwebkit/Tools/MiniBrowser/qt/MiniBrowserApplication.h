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

#ifndef MiniBrowserApplication_h
#define MiniBrowserApplication_h

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QtQml>
#include <QGuiApplication>
#include <QTouchEvent>
#include <QUrl>
#include <qpa/qwindowsysteminterface.h>

class BrowserWindow;

class WindowOptions : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool printLoadedUrls READ printLoadedUrls)
    Q_PROPERTY(bool startMaximized READ startMaximized)
    Q_PROPERTY(bool touchMockingEnabled READ touchMockingEnabled WRITE setTouchMockingEnabled NOTIFY touchMockingEnabledChanged)

public:
    WindowOptions(QObject* parent = 0)
        : QObject(parent)
        , m_printLoadedUrls(false)
        , m_startMaximized(false)
        , m_touchMockingEnabled(true)
        , m_windowSize(QSize(980, 735))
    {
    }

    void setPrintLoadedUrls(bool enabled) { m_printLoadedUrls = enabled; }
    bool printLoadedUrls() const { return m_printLoadedUrls; }
    void setStartMaximized(bool enabled) { m_startMaximized = enabled; }
    bool startMaximized() const { return m_startMaximized; }
    void setStartFullScreen(bool enabled) { m_startFullScreen = enabled; }
    bool startFullScreen() const { return m_startFullScreen; }
    void setRequestedWindowSize(const QSize& size) { m_windowSize = size; }
    QSize requestedWindowSize() const { return m_windowSize; }
    void setUserAgent(const QString& userAgent) { m_userAgent = userAgent; }
    QString userAgent() const { return m_userAgent; }
    bool touchMockingEnabled() const { return m_touchMockingEnabled; }
    void setTouchMockingEnabled(bool enabled)
    {
        if (enabled != m_touchMockingEnabled) {
            m_touchMockingEnabled = enabled;
            emit touchMockingEnabledChanged();
        }
    }

Q_SIGNALS:
    void touchMockingEnabledChanged();

private:
    bool m_printLoadedUrls;
    bool m_startMaximized;
    bool m_startFullScreen;
    bool m_touchMockingEnabled;
    QSize m_windowSize;
    QString m_userAgent;
};

class MiniBrowserApplication : public QGuiApplication {
    Q_OBJECT

public:
    MiniBrowserApplication(int& argc, char** argv);
    QStringList urls() const { return m_urls; }
    bool isRobotized() const { return m_isRobotized; }
    int robotTimeout() const { return m_robotTimeoutSeconds; }
    int robotExtraTime() const { return m_robotExtraTimeSeconds; }
    WindowOptions* windowOptions() { return &m_windowOptions; }

    virtual bool notify(QObject*, QEvent*);

private:
    void updateTouchPoint(const QMouseEvent*, QTouchEvent::TouchPoint, Qt::MouseButton);
    bool sendTouchEvent(BrowserWindow*, QEvent::Type, ulong timestamp);
    void handleUserOptions();

private:
    bool m_realTouchEventReceived;
    int m_pendingFakeTouchEventCount;
    bool m_isRobotized;
    int m_robotTimeoutSeconds;
    int m_robotExtraTimeSeconds;
    QStringList m_urls;

    QHash<int, QTouchEvent::TouchPoint> m_touchPoints;
    QSet<int> m_heldTouchPoints;

    WindowOptions m_windowOptions;
    bool m_holdingControl;
};

QML_DECLARE_TYPE(WindowOptions);

#endif
