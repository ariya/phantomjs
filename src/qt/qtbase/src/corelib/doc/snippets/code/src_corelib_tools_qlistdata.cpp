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
QList<int> integerList;
QList<QDate> dateList;
//! [0]


//! [1]
QList<QString> list;
list << "one" << "two" << "three";
// list: ["one", "two", "three"]
//! [1]


//! [2]
if (list[0] == "Bob")
    list[0] = "Robert";
//! [2]


//! [3]
for (int i = 0; i < list.size(); ++i) {
    if (list.at(i) == "Jane")
        cout << "Found Jane at position " << i << endl;
}
//! [3]


//! [4]
QList<QWidget *> list;
...
while (!list.isEmpty())
    delete list.takeFirst();
//! [4]


//! [5]
int i = list.indexOf("Jane");
if (i != -1)
    cout << "First occurrence of Jane is at position " << i << endl;
//! [5]


//! [6]
QList<QString> list;
list.append("one");
list.append("two");
list.append("three");
// list: ["one", "two", "three"]
//! [6]


//! [7]
QList<QString> list;
list.prepend("one");
list.prepend("two");
list.prepend("three");
// list: ["three", "two", "one"]
//! [7]


//! [8]
QList<QString> list;
list << "alpha" << "beta" << "delta";
list.insert(2, "gamma");
// list: ["alpha", "beta", "gamma", "delta"]
//! [8]


//! [9]
QList<QString> list;
list << "sun" << "cloud" << "sun" << "rain";
list.removeAll("sun");
// list: ["cloud", "rain"]
//! [9]


//! [10]
QList<QString> list;
list << "sun" << "cloud" << "sun" << "rain";
list.removeOne("sun");
// list: ["cloud", ,"sun", "rain"]
//! [10]


//! [11]
QList<QString> list;
list << "A" << "B" << "C" << "D" << "E" << "F";
list.move(1, 4);
// list: ["A", "C", "D", "E", "B", "F"]
//! [11]


//! [12]
QList<QString> list;
list << "A" << "B" << "C" << "D" << "E" << "F";
list.swap(1, 4);
// list: ["A", "E", "C", "D", "B", "F"]
//! [12]


//! [13]
QList<QString> list;
list << "A" << "B" << "C" << "B" << "A";
list.indexOf("B");          // returns 1
list.indexOf("B", 1);       // returns 1
list.indexOf("B", 2);       // returns 3
list.indexOf("X");          // returns -1
//! [13]


//! [14]
QList<QString> list;
list << "A" << "B" << "C" << "B" << "A";
list.lastIndexOf("B");      // returns 3
list.lastIndexOf("B", 3);   // returns 3
list.lastIndexOf("B", 2);   // returns 1
list.lastIndexOf("X");      // returns -1
//! [14]


//! [15]
QList<QString> list;
list.append("January");
list.append("February");
...
list.append("December");

QList<QString>::iterator i;
for (i = list.begin(); i != list.end(); ++i)
    cout << *i << endl;
//! [15]


//! [16]
QList<int>::iterator i;
for (i = list.begin(); i != list.end(); ++i)
    *i += 2;
//! [16]


//! [17]
QList<QWidget *> list;
...
qDeleteAll(list.begin(), list.end());
//! [17]


//! [18]
if (*it == "Hello")
    *it = "Bonjour";
//! [18]


//! [19]
QList<QString> list;
list.append("January");
list.append("February");
...
list.append("December");

QList<QString>::const_iterator i;
for (i = list.constBegin(); i != list.constEnd(); ++i)
    cout << *i << endl;
//! [19]


//! [20]
QList<QWidget *> list;
...
qDeleteAll(list.constBegin(), list.constEnd());
//! [20]


//! [21]
QVector<double> vect;
vect << 20.0 << 30.0 << 40.0 << 50.0;

QList<double> list = QVector<T>::fromVector(vect);
// list: [20.0, 30.0, 40.0, 50.0]
//! [21]


//! [22]
QStringList list;
list << "Sven" << "Kim" << "Ola";

QVector<QString> vect = list.toVector();
// vect: ["Sven", "Kim", "Ola"]
//! [22]


//! [23]
QSet<int> set;
set << 20 << 30 << 40 << ... << 70;

QList<int> list = QList<int>::fromSet(set);
qSort(list);
//! [23]


//! [24]
QStringList list;
list << "Julia" << "Mike" << "Mike" << "Julia" << "Julia";

QSet<QString> set = list.toSet();
set.contains("Julia");  // returns true
set.contains("Mike");   // returns true
set.size();             // returns 2
//! [24]


//! [25]
std::list<double> stdlist;
list.push_back(1.2);
list.push_back(0.5);
list.push_back(3.14);

QList<double> list = QList<double>::fromStdList(stdlist);
//! [25]


//! [26]
QList<double> list;
list << 1.2 << 0.5 << 3.14;

std::list<double> stdlist = list.toStdList();
//! [26]
