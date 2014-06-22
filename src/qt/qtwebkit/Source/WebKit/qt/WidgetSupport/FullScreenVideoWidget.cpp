/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "config.h"
#include "FullScreenVideoWidget.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <QMediaPlayer>

static const int gHideMouseCursorDelay = 3000;

namespace WebKit {

FullScreenVideoWidget::FullScreenVideoWidget()
    : QVideoWidget()
    , m_mediaPlayer(0)
{
    QPalette palette(this->palette());
    palette.setColor(QPalette::Base, Qt::black);
    palette.setColor(QPalette::Background, Qt::black);
    setPalette(palette);

    setWindowModality(Qt::ApplicationModal);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);

    setGeometry(QApplication::desktop()->screenGeometry());

    m_cursorTimer.setSingleShot(true);
    connect(&m_cursorTimer, SIGNAL(timeout()), this, SLOT(hideCursor()));
}

FullScreenVideoWidget::~FullScreenVideoWidget()
{
}

void FullScreenVideoWidget::show(QMediaPlayer* player)
{
    m_mediaPlayer = player;
    showFullScreen();
    setMouseTracking(true);
    raise();
    m_mediaPlayer->setVideoOutput(this);
    setFocus();
    grabMouse();
    hideCursor();
}

void FullScreenVideoWidget::closeEvent(QCloseEvent* event)
{
    m_mediaPlayer = 0;
    m_cursorTimer.stop();
    setMouseTracking(false);
    releaseMouse();
    QApplication::restoreOverrideCursor();
    event->ignore();
    hide();
    emit didExitFullScreen();
}

bool FullScreenVideoWidget::event(QEvent* ev)
{
    switch (ev->type()) {
    case QEvent::MouseMove:
        showCursor();
        ev->accept();
        return true;
    case QEvent::MouseButtonDblClick:
        close();
        ev->accept();
        return true;
    default:
        return QVideoWidget::event(ev);
    }
}

void FullScreenVideoWidget::keyPressEvent(QKeyEvent* ev)
{
    if (m_mediaPlayer && ev->key() == Qt::Key_Space) {
        if (m_mediaPlayer->state() == QMediaPlayer::PlayingState)
            m_mediaPlayer->pause();
        else
            m_mediaPlayer->play();
    } else if (ev->key() == Qt::Key_Escape)
        close();
}

void FullScreenVideoWidget::hideCursor()
{
    QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
}

void FullScreenVideoWidget::showCursor()
{
    QApplication::restoreOverrideCursor();
    m_cursorTimer.start(gHideMouseCursorDelay);
}

}
