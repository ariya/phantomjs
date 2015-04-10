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

int main(int argv, char **args)
{
  QApplication app(argv, args);

    QStateMachine machine;

//![0]
    QState *s1 = new QState();
    QState *s11 = new QState(s1);
    QState *s12 = new QState(s1);
    QState *s13 = new QState(s1);
    s1->setInitialState(s11);
    machine.addState(s1);
//![0]

//![2]
    s12->addTransition(quitButton, SIGNAL(clicked()), s12);
//![2]

//![1]
    QFinalState *s2 = new QFinalState();
    s1->addTransition(quitButton, SIGNAL(clicked()), s2);
    machine.addState(s2);
    machine.setInitialState(s1);

    QObject::connect(&machine, SIGNAL(finished()), QApplication::instance(), SLOT(quit()));
//![1]

  QButton *interruptButton = new QPushButton("Interrupt Button");
  QWidget *mainWindow = new QWidget();

//![3]
    QHistoryState *s1h = new QHistoryState(s1);

    QState *s3 = new QState();
    s3->assignProperty(label, "text", "In s3");
    QMessageBox *mbox = new QMessageBox(mainWindow);
    mbox->addButton(QMessageBox::Ok);
    mbox->setText("Interrupted!");
    mbox->setIcon(QMessageBox::Information);
    QObject::connect(s3, SIGNAL(entered()), mbox, SLOT(exec()));
    s3->addTransition(s1h);
    machine.addState(s3);

    s1->addTransition(interruptButton, SIGNAL(clicked()), s3);
//![3]

  return app.exec();
}

