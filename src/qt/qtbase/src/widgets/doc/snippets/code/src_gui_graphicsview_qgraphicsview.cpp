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
scene.addText("Hello, world!");

QGraphicsView view(&scene);
view.show();
//! [0]


//! [1]
QGraphicsScene scene;
scene.addRect(QRectF(-10, -10, 20, 20));

QGraphicsView view(&scene);
view.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
view.show();
//! [1]


//! [2]
QGraphicsView view;
view.setBackgroundBrush(QImage(":/images/backgroundtile.png"));
view.setCacheMode(QGraphicsView::CacheBackground);
//! [2]


//! [3]
QGraphicsScene scene;
scene.addText("GraphicsView rotated clockwise");

QGraphicsView view(&scene);
view.rotate(90); // the text is rendered with a 90 degree clockwise rotation
view.show();
//! [3]


//! [4]
QGraphicsScene scene;
scene.addItem(...
...

QGraphicsView view(&scene);
view.show();
...

QPrinter printer(QPrinter::HighResolution);
printer.setPageSize(QPrinter::A4);
QPainter painter(&printer);

// print, fitting the viewport contents into a full page
view.render(&painter);

// print the upper half of the viewport into the lower.
// half of the page.
QRect viewport = view.viewport()->rect();
view.render(&painter,
            QRectF(0, printer.height() / 2,
                   printer.width(), printer.height() / 2),
            viewport.adjusted(0, 0, 0, -viewport.height() / 2));

//! [4]


//! [5]
void CustomView::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "There are" << items(event->pos()).size()
             << "items at position" << mapToScene(event->pos());
}
//! [5]


//! [6]
void CustomView::mousePressEvent(QMouseEvent *event)
{
    if (QGraphicsItem *item = itemAt(event->pos())) {
        qDebug() << "You clicked on item" << item;
    } else {
        qDebug() << "You didn't click on an item.";
    }
}
//! [6]


//! [7]
QGraphicsScene scene;
scene.addText("GraphicsView rotated clockwise");

QGraphicsView view(&scene);
view.rotate(90); // the text is rendered with a 90 degree clockwise rotation
view.show();
//! [7]
