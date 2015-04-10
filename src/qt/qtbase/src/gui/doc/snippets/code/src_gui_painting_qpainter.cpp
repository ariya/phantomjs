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

//! [0]
void SimpleExampleWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(Qt::blue);
    painter.setFont(QFont("Arial", 30));
    painter.drawText(rect(), Qt::AlignCenter, "Qt");
}
//! [0]


//! [1]
void MyWidget::paintEvent(QPaintEvent *)
{
    QPainter p;
    p.begin(this);
    p.drawLine(...);        // drawing code
    p.end();
}
//! [1]


//! [2]
void MyWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.drawLine(...);        // drawing code
}
//! [2]


//! [3]
painter->begin(0); // impossible - paint device cannot be 0

QPixmap image(0, 0);
painter->begin(&image); // impossible - image.isNull() == true;

painter->begin(myWidget);
painter2->begin(myWidget); // impossible - only one painter at a time
//! [3]


//! [4]
void QPainter::rotate(qreal angle)
{
    QMatrix matrix;
    matrix.rotate(angle);
    setWorldMatrix(matrix, true);
}
//! [4]


//! [5]
QPainterPath path;
path.moveTo(20, 80);
path.lineTo(20, 30);
path.cubicTo(80, 0, 50, 50, 80, 80);

QPainter painter(this);
painter.drawPath(path);
//! [5]


//! [6]
QLineF line(10.0, 80.0, 90.0, 20.0);

QPainter(this);
painter.drawLine(line);
//! [6]


//! [7]
QRectF rectangle(10.0, 20.0, 80.0, 60.0);

QPainter painter(this);
painter.drawRect(rectangle);
//! [7]


//! [8]
QRectF rectangle(10.0, 20.0, 80.0, 60.0);

QPainter painter(this);
painter.drawRoundedRect(rectangle, 20.0, 15.0);
//! [8]


//! [9]
QRectF rectangle(10.0, 20.0, 80.0, 60.0);

QPainter painter(this);
painter.drawEllipse(rectangle);
//! [9]


//! [10]
QRectF rectangle(10.0, 20.0, 80.0, 60.0);
int startAngle = 30 * 16;
int spanAngle = 120 * 16;

QPainter painter(this);
painter.drawArc(rectangle, startAngle, spanAngle);
//! [10]


//! [11]
QRectF rectangle(10.0, 20.0, 80.0, 60.0);
int startAngle = 30 * 16;
int spanAngle = 120 * 16;

QPainter painter(this);
painter.drawPie(rectangle, startAngle, spanAngle);
//! [11]


//! [12]
QRectF rectangle(10.0, 20.0, 80.0, 60.0);
int startAngle = 30 * 16;
int spanAngle = 120 * 16;

QPainter painter(this);
painter.drawChord(rect, startAngle, spanAngle);
//! [12]


//! [13]
static const QPointF points[3] = {
    QPointF(10.0, 80.0),
    QPointF(20.0, 10.0),
    QPointF(80.0, 30.0),
};

QPainter painter(this);
painter.drawPolyline(points, 3);
//! [13]


//! [14]
static const QPointF points[4] = {
    QPointF(10.0, 80.0),
    QPointF(20.0, 10.0),
    QPointF(80.0, 30.0),
    QPointF(90.0, 70.0)
};

QPainter painter(this);
painter.drawPolygon(points, 4);
//! [14]


//! [15]
static const QPointF points[4] = {
    QPointF(10.0, 80.0),
    QPointF(20.0, 10.0),
    QPointF(80.0, 30.0),
    QPointF(90.0, 70.0)
};

QPainter painter(this);
painter.drawConvexPolygon(points, 4);
//! [15]


//! [16]
QRectF target(10.0, 20.0, 80.0, 60.0);
QRectF source(0.0, 0.0, 70.0, 40.0);
QPixmap pixmap(":myPixmap.png");

QPainter(this);
painter.drawPixmap(target, pixmap, source);
//! [16]


//! [17]
QPainter painter(this);
painter.drawText(rect, Qt::AlignCenter, tr("Qt\nProject"));
//! [17]


//! [18]
QPicture picture;
QPointF point(10.0, 20.0)
picture.load("drawing.pic");

QPainter painter(this);
painter.drawPicture(0, 0, picture);
//! [18]


//! [19]
fillRect(rectangle, background()).
//! [19]


//! [20]
QRectF target(10.0, 20.0, 80.0, 60.0);
QRectF source(0.0, 0.0, 70.0, 40.0);
QImage image(":/images/myImage.png");

QPainter painter(this);
painter.drawImage(target, image, source);
//! [20]


//! [21]
QPainter painter(this);
painter.fillRect(0, 0, 128, 128, Qt::green);
painter.beginNativePainting();

glEnable(GL_SCISSOR_TEST);
glScissor(0, 0, 64, 64);

glClearColor(1, 0, 0, 1);
glClear(GL_COLOR_BUFFER_BIT);

glDisable(GL_SCISSOR_TEST);

painter.endNativePainting();
//! [21]
