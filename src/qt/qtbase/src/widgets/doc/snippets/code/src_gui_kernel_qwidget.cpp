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
w->setWindowState(w->windowState() ^ Qt::WindowFullScreen);
//! [0]


//! [1]
w->setWindowState((w->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
//! [1]


//! [2]
width = baseSize().width() + i * sizeIncrement().width();
height = baseSize().height() + j * sizeIncrement().height();
//! [2]


//! [3]
aWidget->window()->setWindowTitle("New Window Title");
//! [3]


//! [4]
QFont font("Helvetica", 12, QFont::Bold);
setFont(font);
//! [4]


//! [5]
QFont font;
font.setBold(false);
setFont(font);
//! [5]


//! [6]
setCursor(Qt::IBeamCursor);
//! [6]


//! [7]
QPixmap pixmap(widget->size());
widget->render(&pixmap);
//! [7]


//! [8]
QPainter painter(this);
...
painter.end();
myWidget->render(this);
//! [8]


//! [9]
setTabOrder(a, b); // a to b
setTabOrder(b, c); // a to b to c
setTabOrder(c, d); // a to b to c to d
//! [9]


//! [10]
// WRONG
setTabOrder(c, d); // c to d
setTabOrder(a, b); // a to b AND c to d
setTabOrder(b, c); // a to b to c, but not c to d
//! [10]


//! [11]
void MyWidget::closeEvent(QCloseEvent *event)
{
    QSettings settings("MyCompany", "MyApp");
    settings.setValue("geometry", saveGeometry());
    QWidget::closeEvent(event);
}
//! [11]


//! [12]
QSettings settings("MyCompany", "MyApp");
myWidget->restoreGeometry(settings.value("myWidget/geometry").toByteArray());
//! [12]


//! [13]
setUpdatesEnabled(false);
bigVisualChanges();
setUpdatesEnabled(true);
//! [13]


//! [14]
...
extern void qt_x11_set_global_double_buffer(bool);
qt_x11_set_global_double_buffer(false);
...
//! [14]
