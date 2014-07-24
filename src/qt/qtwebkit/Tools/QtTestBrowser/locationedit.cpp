/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 Andreas Kling <kling@webkit.org>
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

#include "locationedit.h"

#ifndef QT_NO_INPUTDIALOG

static const QSize gPageIconSize(16, 16);

static QPixmap defaultPageIcon()
{
    static QPixmap icon;
    if (icon.isNull())
        icon.load(":/favicon.png");

    return icon;
}

LocationEdit::LocationEdit(QWidget* parent)
    : QLineEdit(parent)
    , m_progress(0)
{
    m_clearTimer.setSingleShot(true);
    connect(&m_clearTimer, SIGNAL(timeout()), this, SLOT(reset()));

    m_pageIconLabel = new QLabel(this);
    m_pageIconLabel->setFixedSize(gPageIconSize);
    m_pageIconLabel->setPixmap(defaultPageIcon());
}

void LocationEdit::setPageIcon(const QIcon& icon)
{
    if (icon.isNull())
        m_pageIconLabel->setPixmap(defaultPageIcon());
    else
        m_pageIconLabel->setPixmap(icon.pixmap(gPageIconSize));
}

void LocationEdit::setProgress(int progress)
{
    m_clearTimer.stop();
    m_progress = progress;
    update();
}

void LocationEdit::reset()
{
    setProgress(0);
}

void LocationEdit::resizeEvent(QResizeEvent*)
{
    updateInternalGeometry();
}

void LocationEdit::updateInternalGeometry()
{
    QStyleOptionFrameV3 styleOption;
    initStyleOption(&styleOption);

    QRect textRect = style()->subElementRect(QStyle::SE_LineEditContents, &styleOption, this);

    const int spacing = 2;

    int x = textRect.x() + spacing;
    int y = (textRect.center().y() + 1) - gPageIconSize.height() / 2;

    m_pageIconLabel->move(x, y);

    QMargins margins = textMargins();
    margins.setLeft(m_pageIconLabel->sizeHint().width() + spacing);
    setTextMargins(margins);
}

void LocationEdit::paintEvent(QPaintEvent* ev)
{
    QColor backgroundColor = QApplication::palette().color(QPalette::Base);
    QColor progressColor = QColor(120, 180, 240);
    QPalette p = palette();

    if (!m_progress)
        p.setBrush(QPalette::Base, backgroundColor);
    else {
        QLinearGradient gradient(0, 0, width(), 0);
        gradient.setColorAt(0, progressColor);
        gradient.setColorAt(((double) m_progress) / 100, progressColor);
        if (m_progress != 100)
            gradient.setColorAt((double) m_progress / 100 + 0.001, backgroundColor);
        p.setBrush(QPalette::Base, gradient);
    }
    setPalette(p);

    QLineEdit::paintEvent(ev);

    if (m_progress == 100)
        m_clearTimer.start(100);
}

#endif
