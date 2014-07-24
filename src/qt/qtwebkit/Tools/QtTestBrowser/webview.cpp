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

#include "webview.h"

#include <QAction>
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QScrollBar>
#include <QTimer>

#ifndef QT_NO_ANIMATION
#include <QAbstractAnimation>
#include <QAbstractTransition>
#include <QFinalState>
#include <QPropertyAnimation>
#include <QState>
#include <QStateMachine>
#endif

WebViewGraphicsBased::WebViewGraphicsBased(QWidget* parent)
    : QGraphicsView(parent)
    , m_item(new GraphicsWebView)
    , m_numPaintsTotal(0)
    , m_numPaintsSinceLastMeasure(0)
    , m_measureFps(false)
    , m_resizesToContents(false)
    , m_machine(0)
{
    setScene(new QGraphicsScene(this));
    scene()->addItem(m_item);
    scene()->setFocusItem(m_item);

    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(1000);
    connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(updateFrameRate()));
}

void WebViewGraphicsBased::setPage(QWebPage* page)
{
    connect(page->mainFrame(), SIGNAL(contentsSizeChanged(const QSize&)), SLOT(contentsSizeChanged(const QSize&)));
    connect(page, SIGNAL(scrollRequested(int, int, const QRect&)), SLOT(scrollRequested(int, int)));
    graphicsWebView()->setPage(page);
}

void WebViewGraphicsBased::scrollRequested(int x, int y)
{
    if (!m_resizesToContents)
        return;

    // Turn off interactive mode while scrolling, or QGraphicsView will replay the
    // last mouse event which may cause WebKit to initiate a drag operation.
    bool interactive = isInteractive();
    setInteractive(false);

    verticalScrollBar()->setValue(-y);
    horizontalScrollBar()->setValue(-x);

    setInteractive(interactive);
}

void WebViewGraphicsBased::contentsSizeChanged(const QSize& size)
{
    if (m_resizesToContents)
        scene()->setSceneRect(0, 0, size.width(), size.height());
}

void WebViewGraphicsBased::setResizesToContents(bool b)
{
    if (b == m_resizesToContents)
        return;

    m_resizesToContents = b;
    graphicsWebView()->setResizesToContents(m_resizesToContents);

    // When setting resizesToContents ON, our web view widget will always size as big as the
    // web content being displayed, and so will the QWebPage's viewport. It implies that internally
    // WebCore will work as if there was no content rendered offscreen, and then no scrollbars need
    // drawing. In order to keep scrolling working, we:
    //
    // 1) Set QGraphicsView's scrollbars policy back to 'auto'.
    // 2) Set scene's boundaries rect to an invalid size, which automatically makes it to be as big
    //    as it needs to enclose all items onto it. We do that because QGraphicsView also calculates
    //    the size of its scrollable area according to the amount of content in scene that is rendered
    //    offscreen.
    // 3) Set QWebPage's preferredContentsSize according to the size of QGraphicsView's viewport,
    //    so WebCore properly lays pages out.
    //
    // On the other hand, when toggling resizesToContents OFF, we set back the default values, as
    // opposite as described above.
    if (m_resizesToContents) {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        graphicsWebView()->page()->setPreferredContentsSize(size());
        QRectF itemRect(graphicsWebView()->geometry().topLeft(), graphicsWebView()->page()->mainFrame()->contentsSize());
        graphicsWebView()->setGeometry(itemRect);
        scene()->setSceneRect(itemRect);
    } else {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        graphicsWebView()->page()->setPreferredContentsSize(QSize());
        QRect viewportRect(QPoint(0, 0), size());
        graphicsWebView()->setGeometry(viewportRect);
        scene()->setSceneRect(viewportRect);
    }
}

void WebViewGraphicsBased::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);

    QSize size(event->size());

    if (m_resizesToContents) {
        graphicsWebView()->page()->setPreferredContentsSize(size);
        return;
    }

    QRectF rect(QPoint(0, 0), size);
    graphicsWebView()->setGeometry(rect);
    scene()->setSceneRect(rect);
}

void WebViewGraphicsBased::setFrameRateMeasurementEnabled(bool enabled)
{
    m_measureFps = enabled;
    if (m_measureFps) {
        m_lastConsultTime = m_startTime = QTime::currentTime();
        m_fpsTimer.start();
        m_updateTimer->start();
    } else {
        m_fpsTimer.stop();
        m_updateTimer->stop();
    }
}

void WebViewGraphicsBased::updateFrameRate()
{
    const QTime now = QTime::currentTime();
    int interval = m_lastConsultTime.msecsTo(now);
    int frames = m_fpsTimer.numFrames(interval);
    int current = interval ? frames * 1000 / interval : 0;

    emit currentFPSUpdated(current);

    m_lastConsultTime = now;
}

void WebViewGraphicsBased::animatedFlip()
{
#ifndef QT_NO_ANIMATION
    QSizeF center = graphicsWebView()->boundingRect().size() / 2;
    QPointF centerPoint = QPointF(center.width(), center.height());
    graphicsWebView()->setTransformOriginPoint(centerPoint);

    QPropertyAnimation* animation = new QPropertyAnimation(graphicsWebView(), "rotation", this);
    animation->setDuration(1000);

    int rotation = int(graphicsWebView()->rotation());

    animation->setStartValue(rotation);
    animation->setEndValue(rotation + 180 - (rotation % 180));

    animation->start(QAbstractAnimation::DeleteWhenStopped);
#endif
}

void WebViewGraphicsBased::animatedYFlip()
{
#ifndef QT_NO_ANIMATION
    if (!m_machine) {
        m_machine = new QStateMachine(this);

        QState* s0 = new QState(m_machine);
        s0->assignProperty(this, "yRotation", 0);

        QState* s1 = new QState(m_machine);
        s1->assignProperty(this, "yRotation", 90);

        QAbstractTransition* t1 = s0->addTransition(s1);
        QPropertyAnimation* yRotationAnim = new QPropertyAnimation(this, "yRotation", this);
        t1->addAnimation(yRotationAnim);

        QState* s2 = new QState(m_machine);
        s2->assignProperty(this, "yRotation", -90);
        s1->addTransition(s1, SIGNAL(propertiesAssigned()), s2);

        QState* s3 = new QState(m_machine);
        s3->assignProperty(this, "yRotation", 0);

        QAbstractTransition* t2 = s2->addTransition(s3);
        t2->addAnimation(yRotationAnim);

        QFinalState* final = new QFinalState(m_machine);
        s3->addTransition(s3, SIGNAL(propertiesAssigned()), final);

        m_machine->setInitialState(s0);
        yRotationAnim->setDuration(1000);
    }

    m_machine->start();
#endif
}

void WebViewGraphicsBased::paintEvent(QPaintEvent* event)
{
    QGraphicsView::paintEvent(event);
    if (!m_measureFps)
        return;
}

static QMenu* createContextMenu(QWebPage* page, QPoint position)
{
    QMenu* menu = page->createStandardContextMenu();

    QWebHitTestResult r = page->mainFrame()->hitTestContent(position);

    if (!r.linkUrl().isEmpty()) {
#ifndef QT_NO_DESKTOPSERVICES
        WebPage* webPage = qobject_cast<WebPage*>(page);
        QAction* newTabAction = menu->addAction("Open in Default &Browser", webPage, SLOT(openUrlInDefaultBrowser()));
        newTabAction->setData(r.linkUrl());
        menu->insertAction(menu->actions().at(2), newTabAction);
#endif
        if (r.linkTargetFrame() != r.frame())
            menu->insertAction(menu->actions().at(0), page->action(QWebPage::OpenLinkInThisWindow));
    }
    return menu;
}

void GraphicsWebView::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    setProperty("mouseButtons", QVariant::fromValue(int(event->buttons())));
    setProperty("keyboardModifiers", QVariant::fromValue(int(event->modifiers())));

    QGraphicsWebView::mousePressEvent(event);
}

void WebViewTraditional::mousePressEvent(QMouseEvent* event)
{
    setProperty("mouseButtons", QVariant::fromValue(int(event->buttons())));
    setProperty("keyboardModifiers", QVariant::fromValue(int(event->modifiers())));

    QWebView::mousePressEvent(event);
}

void GraphicsWebView::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    QMenu* menu = createContextMenu(page(), event->pos().toPoint());
    menu->exec(event->screenPos());
    delete menu;
}

void WebViewTraditional::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu* menu = createContextMenu(page(), event->pos());
    menu->exec(event->globalPos());
    delete menu;
}

