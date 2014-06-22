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

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    {
//! [0]
    QWidget *window = new QWidget;
//! [0] //! [1]
    QPushButton *button1 = new QPushButton("One");
//! [1] //! [2]
    QPushButton *button2 = new QPushButton("Two");
    QPushButton *button3 = new QPushButton("Three");
    QPushButton *button4 = new QPushButton("Four");
    QPushButton *button5 = new QPushButton("Five");
//! [2]

//! [3]
    QHBoxLayout *layout = new QHBoxLayout;
//! [3] //! [4]
    layout->addWidget(button1);
    layout->addWidget(button2);
    layout->addWidget(button3);
    layout->addWidget(button4);
    layout->addWidget(button5);

    window->setLayout(layout);
//! [4]
    window->setWindowTitle("QHBoxLayout");
//! [5]
    window->show();
//! [5]
    }

    {
//! [6]
    QWidget *window = new QWidget;
//! [6] //! [7]
    QPushButton *button1 = new QPushButton("One");
//! [7] //! [8]
    QPushButton *button2 = new QPushButton("Two");
    QPushButton *button3 = new QPushButton("Three");
    QPushButton *button4 = new QPushButton("Four");
    QPushButton *button5 = new QPushButton("Five");
//! [8]

//! [9]
    QVBoxLayout *layout = new QVBoxLayout;
//! [9] //! [10]
    layout->addWidget(button1);
    layout->addWidget(button2);
    layout->addWidget(button3);
    layout->addWidget(button4);
    layout->addWidget(button5);

    window->setLayout(layout);
//! [10]
    window->setWindowTitle("QVBoxLayout");
//! [11]
    window->show();
//! [11]
    }

    {
//! [12]
    QWidget *window = new QWidget;
//! [12] //! [13]
    QPushButton *button1 = new QPushButton("One");
//! [13] //! [14]
    QPushButton *button2 = new QPushButton("Two");
    QPushButton *button3 = new QPushButton("Three");
    QPushButton *button4 = new QPushButton("Four");
    QPushButton *button5 = new QPushButton("Five");
//! [14]

//! [15]
    QGridLayout *layout = new QGridLayout;
//! [15] //! [16]
    layout->addWidget(button1, 0, 0);
    layout->addWidget(button2, 0, 1);
    layout->addWidget(button3, 1, 0, 1, 2);
    layout->addWidget(button4, 2, 0);
    layout->addWidget(button5, 2, 1);

    window->setLayout(layout);
//! [16]
    window->setWindowTitle("QGridLayout");
//! [17]
    window->show();
//! [17]
    }

    {
//! [18]
    QWidget *window = new QWidget;
//! [18]
//! [19]
    QPushButton *button1 = new QPushButton("One");
    QLineEdit *lineEdit1 = new QLineEdit();
//! [19]
//! [20]
    QPushButton *button2 = new QPushButton("Two");
    QLineEdit *lineEdit2 = new QLineEdit();
    QPushButton *button3 = new QPushButton("Three");
    QLineEdit *lineEdit3 = new QLineEdit();
//! [20]
//! [21]
    QFormLayout *layout = new QFormLayout;
//! [21]
//! [22]
    layout->addRow(button1, lineEdit1);
    layout->addRow(button2, lineEdit2);
    layout->addRow(button3, lineEdit3);

    window->setLayout(layout);
//! [22]
    window->setWindowTitle("QFormLayout");
//! [23]
    window->show();
//! [23]
    }

    {
//! [24]
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(formWidget);
    setLayout(layout);
//! [24]
    }
    return app.exec();
}
