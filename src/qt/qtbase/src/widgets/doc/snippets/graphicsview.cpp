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
QGraphicsScene scene;
QGraphicsRectItem *rect = scene.addRect(QRectF(0, 0, 100, 100));

QGraphicsItem *item = scene.itemAt(50, 50);
// item == rect
//! [0]


//! [1]
QGraphicsScene scene;
myPopulateScene(&scene);

QGraphicsView view(&scene);
view.show();
//! [1]


//! [2]
class View : public QGraphicsView
{
Q_OBJECT
    ...
public slots:
    void zoomIn() { scale(1.2, 1.2); }
    void zoomOut() { scale(1 / 1.2, 1 / 1.2); }
    void rotateLeft() { rotate(-10); }
    void rotateRight() { rotate(10); }
    ...
};
//! [2]


//! [3]
QGraphicsScene scene;
scene.addRect(QRectF(0, 0, 100, 200), QPen(Qt::black), QBrush(Qt::green));

QPrinter printer;
if (QPrintDialog(&printer).exec() == QDialog::Accepted) {
    QPainter painter(&printer);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);
}
//! [3]


//! [4]
QGraphicsScene scene;
scene.addRect(QRectF(0, 0, 100, 200), QPen(Qt::black), QBrush(Qt::green));

QPixmap pixmap;
QPainter painter(&pixmap);
painter.setRenderHint(QPainter::Antialiasing);
scene.render(&painter);
painter.end();

pixmap.save("scene.png");
//! [4]


//! [5]
void CustomItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QMimeData *data = new QMimeData;
    data->setColor(Qt::green);

    QDrag *drag = new QDrag(event->widget());
    drag->setMimeData(data);
    drag->start();
}
//! [5]


//! [6]
QGraphicsView view(&scene);
view.setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
//! [6]
