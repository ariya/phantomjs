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
QLinkedList<int> integerList;
QLinkedList<QTime> timeList;
//! [0]


//! [1]
QLinkedList<QString> list;
list << "one" << "two" << "three";
// list: ["one", "two", "three"]
//! [1]


//! [2]
QLinkedList<QWidget *> list;
...
while (!list.isEmpty())
    delete list.takeFirst();
//! [2]


//! [3]
QLinkedList<QString> list;
list.append("one");
list.append("two");
list.append("three");
// list: ["one", "two", "three"]
//! [3]


//! [4]
QLinkedList<QString> list;
list.prepend("one");
list.prepend("two");
list.prepend("three");
// list: ["three", "two", "one"]
//! [4]


//! [5]
QList<QString> list;
list << "sun" << "cloud" << "sun" << "rain";
list.removeAll("sun");
// list: ["cloud", "rain"]
//! [5]


//! [6]
QList<QString> list;
list << "sun" << "cloud" << "sun" << "rain";
list.removeOne("sun");
// list: ["cloud", "sun", "rain"]
//! [6]


//! [7]
QLinkedList<QString> list;
list.append("January");
list.append("February");
...
list.append("December");

QLinkedList<QString>::iterator i;
for (i = list.begin(); i != list.end(); ++i)
    cout << *i << endl;
//! [7]


//! [8]
QLinkedList<QString> list;
...
QLinkedList<QString>::iterator it = qFind(list.begin(),
                                          list.end(), "Joel");
if (it != list.end())
    cout << "Found Joel" << endl;
//! [8]


//! [9]
QLinkedList<int>::iterator i;
for (i = list.begin(); i != list.end(); ++i)
    *i += 2;
//! [9]


//! [10]
QLinkedList<QString> list;
...
QLinkedList<QString>::iterator i = list.begin();
while (i != list.end()) {
    if ((*i).startsWith("_"))
        i = list.erase(i);
    else
        ++i;
}
//! [10]


//! [11]
QLinkedList<QString>::iterator i = list.begin();
while (i != list.end()) {
    QLinkedList<QString>::iterator previous = i;
    ++i;
    if ((*previous).startsWith("_"))
        list.erase(previous);
}
//! [11]


//! [12]
// WRONG
while (i != list.end()) {
    if ((*i).startsWith("_"))
        list.erase(i);
    ++i;
}
//! [12]


//! [13]
if (*it == "Hello")
    *it = "Bonjour";
//! [13]


//! [14]
QLinkedList<QString> list;
list.append("January");
list.append("February");
...
list.append("December");

QLinkedList<QString>::const_iterator i;
for (i = list.constBegin(); i != list.constEnd(); ++i)
    cout << *i << endl;
//! [14]


//! [15]
QLinkedList<QString> list;
...
QLinkedList<QString>::iterator it = qFind(list.constBegin(),
                                          list.constEnd(), "Joel");
if (it != list.constEnd())
    cout << "Found Joel" << endl;
//! [15]


//! [16]
std::list<double> stdlist;
list.push_back(1.2);
list.push_back(0.5);
list.push_back(3.14);

QLinkedList<double> list = QLinkedList<double>::fromStdList(stdlist);
//! [16]


//! [17]
QLinkedList<double> list;
list << 1.2 << 0.5 << 3.14;

std::list<double> stdlist = list.toStdList();
//! [17]
