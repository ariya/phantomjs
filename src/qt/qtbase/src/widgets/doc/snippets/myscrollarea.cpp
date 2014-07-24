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

class MyScrollArea : public QAbstractScrollArea
{
public:
    MyScrollArea(QWidget *w);
    void setWidget(QWidget *w);

protected:
    void scrollContentsBy(int dx, int dy);
    void resizeEvent(QResizeEvent *event);

private:
    void updateWidgetPosition();
    void updateArea();

    QWidget *widget;
};

MyScrollArea::MyScrollArea(QWidget *widget)
    : QAbstractScrollArea()
{
    setWidget(widget);
}

void MyScrollArea::setWidget(QWidget *w)
{
    widget = w;
    widget->setParent(viewport());
    if (!widget->testAttribute(Qt::WA_Resized))
        widget->resize(widget->sizeHint());

    verticalScrollBar()->setValue(0);
    verticalScrollBar()->setValue(0);

    updateArea();
}

void MyScrollArea::updateWidgetPosition()
{
//! [0]
    int hvalue = horizontalScrollBar()->value();
    int vvalue = verticalScrollBar()->value();
    QPoint topLeft = viewport()->rect().topLeft();

    widget->move(topLeft.x() - hvalue, topLeft.y() - vvalue);
//! [0]
}

void MyScrollArea::scrollContentsBy(int /*dx*/, int /*dy*/)
{
    updateWidgetPosition();
}

void MyScrollArea::updateArea()
{
//! [1]
    QSize areaSize = viewport()->size();
    QSize  widgetSize = widget->size();

    verticalScrollBar()->setPageStep(areaSize.height());
    horizontalScrollBar()->setPageStep(areaSize.width());
    verticalScrollBar()->setRange(0, widgetSize.height() - areaSize.height());
    horizontalScrollBar()->setRange(0, widgetSize.width() - areaSize.width());
    updateWidgetPosition();
//! [1]
}

void MyScrollArea::resizeEvent(QResizeEvent *event)
{
    updateArea();
}

int main(int argv, char **args)
{
    QApplication app(argv, args);

    QPixmap pixmap("mypixmap.png");
    QLabel label;
    label.setPixmap(pixmap);
    MyScrollArea area(&label);
    area.resize(300, 300);
    area.show();

    area.setWidget(&label);

    return app.exec();
}
