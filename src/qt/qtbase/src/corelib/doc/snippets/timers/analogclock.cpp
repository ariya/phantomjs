/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>

#include "analogclock.h"

//! [0] //! [1]
AnalogClock::AnalogClock(QWidget *parent)
//! [0] //! [2]
    : QWidget(parent)
//! [2] //! [3]
{
//! [3] //! [4]
    QTimer *timer = new QTimer(this);
//! [4] //! [5]
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
//! [5] //! [6]
    timer->start(1000);
//! [6]

    setWindowTitle(tr("Analog Clock"));
    resize(200, 200);
//! [7]
}
//! [1] //! [7]

//! [8] //! [9]
void AnalogClock::paintEvent(QPaintEvent *)
//! [8] //! [10]
{
    static const QPoint hourHand[3] = {
        QPoint(7, 8),
        QPoint(-7, 8),
        QPoint(0, -40)
    };
    static const QPoint minuteHand[3] = {
        QPoint(7, 8),
        QPoint(-7, 8),
        QPoint(0, -70)
    };

    QColor hourColor(127, 0, 127);
    QColor minuteColor(0, 127, 127, 191);

    int side = qMin(width(), height());
    QTime time = QTime::currentTime();
//! [10]

//! [11]
    QPainter painter(this);
//! [11] //! [12]
    painter.setRenderHint(QPainter::Antialiasing);
//! [12] //! [13]
    painter.translate(width() / 2, height() / 2);
//! [13] //! [14]
    painter.scale(side / 200.0, side / 200.0);
//! [9] //! [14]

//! [15]
    painter.setPen(Qt::NoPen);
//! [15] //! [16]
    painter.setBrush(hourColor);
//! [16]

//! [17] //! [18]
    painter.save();
//! [17] //! [19]
    painter.rotate(30.0 * ((time.hour() + time.minute() / 60.0)));
    painter.drawConvexPolygon(hourHand, 3);
    painter.restore();
//! [18] //! [19]

//! [20]
    painter.setPen(hourColor);
//! [20] //! [21]

    for (int i = 0; i < 12; ++i) {
        painter.drawLine(88, 0, 96, 0);
        painter.rotate(30.0);
    }
//! [21]

//! [22]
    painter.setPen(Qt::NoPen);
//! [22] //! [23]
    painter.setBrush(minuteColor);

//! [24]
    painter.save();
    painter.rotate(6.0 * (time.minute() + time.second() / 60.0));
    painter.drawConvexPolygon(minuteHand, 3);
    painter.restore();
//! [23] //! [24]

//! [25]
    painter.setPen(minuteColor);
//! [25] //! [26]

//! [27]
    for (int j = 0; j < 60; ++j) {
        if ((j % 5) != 0)
            painter.drawLine(92, 0, 96, 0);
        painter.rotate(6.0);
    }
//! [27]
}
//! [26]
