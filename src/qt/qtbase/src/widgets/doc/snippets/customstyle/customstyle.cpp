/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
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

#include "customstyle.h"

CustomStyle::CustomStyle()
{
//! [0]
    QSpinBox *spinBox = qobject_cast<QSpinBox *>(widget);
    if (spinBox) {
//! [0] //! [1]
    }
//! [1]
}

//! [2]
void CustomStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                QPainter *painter, const QWidget *widget) const
{
    if (element == PE_IndicatorSpinUp || element == PE_IndicatorSpinDown) {
        QPolygon points(3);
        int x = option->rect.x();
        int y = option->rect.y();
        int w = option->rect.width() / 2;
        int h = option->rect.height() / 2;
        x += (option->rect.width() - w) / 2;
        y += (option->rect.height() - h) / 2;

        if (element == PE_IndicatorSpinUp) {
            points[0] = QPoint(x, y + h);
            points[1] = QPoint(x + w, y + h);
            points[2] = QPoint(x + w / 2, y);
        } else { // PE_SpinBoxDown
            points[0] = QPoint(x, y);
            points[1] = QPoint(x + w, y);
            points[2] = QPoint(x + w / 2, y + h);
        }

        if (option->state & State_Enabled) {
            painter->setPen(option->palette.mid().color());
            painter->setBrush(option->palette.buttonText());
        } else {
            painter->setPen(option->palette.buttonText().color());
            painter->setBrush(option->palette.mid());
        }
        painter->drawPolygon(points);
    } else {
    QProxyStyle::drawPrimitive(element, option, painter, widget);
//! [2] //! [3]
    }
//! [3] //! [4]
}
//! [4]
