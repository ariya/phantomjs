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

#include <QtGui>
#include <cmath>

class SimpleTransformation : public QWidget
{
    void paintEvent(QPaintEvent *);
};

//! [0]
void SimpleTransformation::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(QPen(Qt::blue, 1, Qt::DashLine));
    painter.drawRect(0, 0, 100, 100);

    painter.rotate(45);

    painter.setFont(QFont("Helvetica", 24));
    painter.setPen(QPen(Qt::black, 1));
    painter.drawText(20, 10, "QTransform");
}
//! [0]

class CombinedTransformation : public QWidget
{
    void paintEvent(QPaintEvent *);
};

//! [1]
void CombinedTransformation::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(QPen(Qt::blue, 1, Qt::DashLine));
    painter.drawRect(0, 0, 100, 100);

    QTransform transform;
    transform.translate(50, 50);
    transform.rotate(45);
    transform.scale(0.5, 1.0);
    painter.setTransform(transform);

    painter.setFont(QFont("Helvetica", 24));
    painter.setPen(QPen(Qt::black, 1));
    painter.drawText(20, 10, "QTransform");
}
//! [1]

class BasicOperations : public QWidget
{
    void paintEvent(QPaintEvent *);
};

//! [2]
void BasicOperations::paintEvent(QPaintEvent *)
{
    double pi = 3.14;

    double a    = pi/180 * 45.0;
    double sina = sin(a);
    double cosa = cos(a);

    QTransform translationTransform(1, 0, 0, 1, 50.0, 50.0);
    QTransform rotationTransform(cosa, sina, -sina, cosa, 0, 0);
    QTransform scalingTransform(0.5, 0, 0, 1.0, 0, 0);

    QTransform transform;
    transform = scalingTransform * rotationTransform * translationTransform;

    QPainter painter(this);
    painter.setPen(QPen(Qt::blue, 1, Qt::DashLine));
    painter.drawRect(0, 0, 100, 100);

    painter.setTransform(transform);

    painter.setFont(QFont("Helvetica", 24));
    painter.setPen(QPen(Qt::black, 1));
    painter.drawText(20, 10, "QTransform");
}
//! [2]

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QWidget widget;

    SimpleTransformation *simpleWidget = new SimpleTransformation;
    CombinedTransformation *combinedWidget = new CombinedTransformation;
    BasicOperations *basicWidget = new BasicOperations;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(simpleWidget);
    layout->addWidget(combinedWidget);
    layout->addWidget(basicWidget);
    widget.setLayout(layout);

    widget.show();
    widget.resize(130, 350);
    return app.exec();
}
