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

void mainWindowExample()
{
    QMdiArea *mdiArea = new QMdiArea;
//! [0]
    QMainWindow *mainWindow = new QMainWindow;
    mainWindow->setCentralWidget(mdiArea);
//! [0]

    mdiArea->addSubWindow(new QPushButton("Push Me Now!"));

    mainWindow->show();
}

void addingSubWindowsExample()
{
    QWidget *internalWidget1 = new QWidget;
    QWidget *internalWidget2 = new QWidget;

//! [1]
    QMdiArea mdiArea;
    QMdiSubWindow *subWindow1 = new QMdiSubWindow;
    subWindow1->setWidget(internalWidget1);
    subWindow1->setAttribute(Qt::WA_DeleteOnClose);
    mdiArea.addSubWindow(subWindow1);

    QMdiSubWindow *subWindow2 =
        mdiArea.addSubWindow(internalWidget2);

//! [1]
    subWindow1->show();
    subWindow2->show();

    mdiArea.show();
}

int main(int argv, char **args)
{
    QApplication app(argv, args);

    mainWindowExample();
    //addingSubWindowsExample();

   QAction *act = new QAction(qApp);
   act->setShortcut(Qt::ALT + Qt::Key_S);
   act->setShortcutContext( Qt::ApplicationShortcut );
   QObject::connect(act, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    QWidget widget5;
    widget5.show();
    widget5.addAction(act);

    return app.exec();
}


