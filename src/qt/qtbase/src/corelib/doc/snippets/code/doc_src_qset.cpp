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
QSet<QString> set;
//! [0]


//! [1]
set.insert("one");
set.insert("three");
set.insert("seven");
//! [1]


//! [2]
set << "twelve" << "fifteen" << "nineteen";
//! [2]


//! [3]
if (!set.contains("ninety-nine"))
    ...
//! [3]


//! [4]
QSetIterator<QWidget *> i(set);
while (i.hasNext())
    qDebug() << i.next();
//! [4]


//! [5]
QSet<QWidget *>::const_iterator i = set.constBegin();
while (i != set.constEnd()) {
    qDebug() << *i;
    ++i;
}
//! [5]


//! [6]
QSet<QString> set;
...
foreach (const QString &value, set)
    qDebug() << value;
//! [6]


//! [7]
QSet<QString> set;
set.reserve(20000);
for (int i = 0; i < 20000; ++i)
    set.insert(values[i]);
//! [7]


//! [8]
QSet<QString> set;
set << "January" << "February" << ... << "December";

QSet<QString>::iterator i;
for (i = set.begin(); i != set.end(); ++i)
    qDebug() << *i;
//! [8]


//! [9]
QSet<QString> set;
set << "January" << "February" << ... << "December";

QSet<QString>::iterator i = set.begin();
while (i != set.end()) {
    if ((*i).startsWith('J')) {
        i = set.erase(i);
    } else {
        ++i;
    }
}
//! [9]


//! [10]
QSet<QString> set;
...
QSet<QString>::iterator it = qFind(set.begin(), set.end(), "Jeanette");
if (it != set.end())
    cout << "Found Jeanette" << endl;
//! [10]


//! [11]
QSet<QString> set;
set << "January" << "February" << ... << "December";

QSet<QString>::const_iterator i;
for (i = set.begin(); i != set.end(); ++i)
    qDebug() << *i;
//! [11]


//! [12]
QSet<QString> set;
...
QSet<QString>::iterator it = qFind(set.begin(), set.end(), "Jeanette");
if (it != set.constEnd())
    cout << "Found Jeanette" << endl;
//! [12]


//! [13]
QSet<QString> set;
set << "red" << "green" << "blue" << ... << "black";

QList<QString> list = set.toList();
qSort(list);
//! [13]


//! [14]
QStringList list;
list << "Julia" << "Mike" << "Mike" << "Julia" << "Julia";

QSet<QString> set = QSet<QString>::fromList(list);
set.contains("Julia");  // returns true
set.contains("Mike");   // returns true
set.size();             // returns 2
//! [14]
