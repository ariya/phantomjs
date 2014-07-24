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
    QWidget *button;

  {
//![0]
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QStateMachine::RestoreProperties);

    QState *s1 = new QState();
    s1->assignProperty(object, "fooBar", 1.0);
    machine.addState(s1);
    machine.setInitialState(s1);

    QState *s2 = new QState();
    machine.addState(s2);
//![0]
  }

  {

//![2]
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QStateMachine::RestoreProperties);

    QState *s1 = new QState();
    s1->assignProperty(object, "fooBar", 1.0);
    machine.addState(s1);
    machine.setInitialState(s1);

    QState *s2 = new QState(s1);
    s2->assignProperty(object, "fooBar", 2.0);
    s1->setInitialState(s2);

    QState *s3 = new QState(s1);
//![2]

  }

  {
//![3]
    QState *s1 = new QState();
    QState *s2 = new QState();

    s1->assignProperty(button, "geometry", QRectF(0, 0, 50, 50));
    s2->assignProperty(button, "geometry", QRectF(0, 0, 100, 100));

    s1->addTransition(button, SIGNAL(clicked()), s2);
//![3]

  }

  {
//![4]
    QState *s1 = new QState();
    QState *s2 = new QState();

    s1->assignProperty(button, "geometry", QRectF(0, 0, 50, 50));
    s2->assignProperty(button, "geometry", QRectF(0, 0, 100, 100));

    QSignalTransition *transition = s1->addTransition(button, SIGNAL(clicked()), s2);
    transition->addAnimation(new QPropertyAnimation(button, "geometry"));
//![4]

  }

  {
    QMainWindow *mainWindow = 0;

//![5]
    QMessageBox *messageBox = new QMessageBox(mainWindow);
    messageBox->addButton(QMessageBox::Ok);
    messageBox->setText("Button geometry has been set!");
    messageBox->setIcon(QMessageBox::Information);

    QState *s1 = new QState();

    QState *s2 = new QState();
    s2->assignProperty(button, "geometry", QRectF(0, 0, 50, 50));
    connect(s2, SIGNAL(entered()), messageBox, SLOT(exec()));

    s1->addTransition(button, SIGNAL(clicked()), s2);
//![5]
  }

  {
    QMainWindow *mainWindow = 0;

//![6]
    QMessageBox *messageBox = new QMessageBox(mainWindow);
    messageBox->addButton(QMessageBox::Ok);
    messageBox->setText("Button geometry has been set!");
    messageBox->setIcon(QMessageBox::Information);

    QState *s1 = new QState();

    QState *s2 = new QState();
    s2->assignProperty(button, "geometry", QRectF(0, 0, 50, 50));

    QState *s3 = new QState();
    connect(s3, SIGNAL(entered()), messageBox, SLOT(exec()));

    s1->addTransition(button, SIGNAL(clicked()), s2);
    s2->addTransition(s2, SIGNAL(propertiesAssigned()), s3);
//![6]

  }

  {

//![7]
    QState *s1 = new QState();
    QState *s2 = new QState();

    s2->assignProperty(object, "fooBar", 2.0);
    s1->addTransition(s2);

    QStateMachine machine;
    machine.setInitialState(s1);
    machine.addDefaultAnimation(new QPropertyAnimation(object, "fooBar"));
//![7]

  }



    return app.exec();
}

