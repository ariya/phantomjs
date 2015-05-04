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
QCheckBox *box = ...;
QSignalSpy spy(box, SIGNAL(clicked(bool)));

// do something that triggers the signal
box->animateClick();

QCOMPARE(spy.count(), 1); // make sure the signal was emitted exactly one time
QList<QVariant> arguments = spy.takeFirst(); // take the first signal

QVERIFY(arguments.at(0).toBool() == true); // verify the first argument
//! [0]


//! [1]
QSignalSpy spy(myCustomObject, SIGNAL(mySignal(int,QString,double)));

myCustomObject->doSomething(); // trigger emission of the signal

QList<QVariant> arguments = spy.takeFirst();
QVERIFY(arguments.at(0).type() == QVariant::Int);
QVERIFY(arguments.at(1).type() == QVariant::QString);
QVERIFY(arguments.at(2).type() == QVariant::double);
//! [1]


//! [2]
qRegisterMetaType<SomeStruct>();
QSignalSpy spy(&model, SIGNAL(whatever(SomeStruct)));
//! [2]


//! [3]
// get the first argument from the first received signal:
SomeStruct result = qvariant_cast<SomeStruct>(spy.at(0).at(0));
//! [3]


//! [4]
QSignalSpy spy(myPushButton, SIGNAL(clicked(bool)));
//! [4]

//! [5]
QVERIFY(spy.wait(1000));
//! [5]

//! [6]
QSignalSpy spy(myPushButton, &QPushButton::clicked);
//! [6]

