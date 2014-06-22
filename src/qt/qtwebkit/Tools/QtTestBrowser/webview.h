/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef webview_h
#define webview_h

#include "fpstimer.h"
#include "webpage.h"
#include <qwebview.h>
#include <qgraphicswebview.h>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QTime>

QT_BEGIN_NAMESPACE
class QStateMachine;
QT_END_NAMESPACE

class WebViewTraditional : public QWebView {
    Q_OBJECT

public:
    WebViewTraditional(QWidget* parent) : QWebView(parent) {}

protected:
    virtual void contextMenuEvent(QContextMenuEvent*);
    virtual void mousePressEvent(QMouseEvent*);
};


class GraphicsWebView : public QGraphicsWebView {
    Q_OBJECT

public:
    GraphicsWebView(QGraphicsItem* parent = 0) : QGraphicsWebView(parent) {};

protected:
    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent*);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent*);
};


class WebViewGraphicsBased : public QGraphicsView {
    Q_OBJECT
    Q_PROPERTY(qreal yRotation READ yRotation WRITE setYRotation)

public:
    WebViewGraphicsBased(QWidget* parent);
    void setPage(QWebPage* page);

    void setItemCacheMode(QGraphicsItem::CacheMode mode) { graphicsWebView()->setCacheMode(mode); }
    QGraphicsItem::CacheMode itemCacheMode() { return graphicsWebView()->cacheMode(); }

    void setFrameRateMeasurementEnabled(bool enabled);
    bool frameRateMeasurementEnabled() const { return m_measureFps; }

    virtual void resizeEvent(QResizeEvent*);
    virtual void paintEvent(QPaintEvent* event);

    void setResizesToContents(bool b);
    bool resizesToContents() const { return m_resizesToContents; }

    void setYRotation(qreal angle);
    qreal yRotation() const { return m_yRotation; }

    GraphicsWebView* graphicsWebView() const { return m_item; }

public Q_SLOTS:
    void updateFrameRate();
    void animatedFlip();
    void animatedYFlip();
    void contentsSizeChanged(const QSize&);
    void scrollRequested(int, int);

Q_SIGNALS:
    void currentFPSUpdated(int fps);

private:
    GraphicsWebView* m_item;
    int m_numPaintsTotal;
    int m_numPaintsSinceLastMeasure;
    QTime m_startTime;
    QTime m_lastConsultTime;
    QTimer* m_updateTimer;
    bool m_measureFps;
    qreal m_yRotation;
    bool m_resizesToContents;
    QStateMachine* m_machine;
    FpsTimer m_fpsTimer;
};

inline void WebViewGraphicsBased::setYRotation(qreal angle)
{
    QRectF r = graphicsWebView()->boundingRect();
    graphicsWebView()->setTransform(QTransform()
        .translate(r.width() / 2, r.height() / 2)
        .rotate(angle, Qt::YAxis)
        .translate(-r.width() / 2, -r.height() / 2));
    m_yRotation = angle;
}

#endif
