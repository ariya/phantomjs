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
class Employee
{
public:
    Employee() {}
    Employee(const Employee &other);

    Employee &operator=(const Employee &other);

private:
    QString myName;
    QDate myDateOfBirth;
};
//! [0]


//! [1]
QList<QString> list;
list << "A" << "B" << "C" << "D";

QListIterator<QString> i(list);
while (i.hasNext())
    qDebug() << i.next();
//! [1]


//! [2]
QListIterator<QString> i(list);
i.toBack();
while (i.hasPrevious())
    qDebug() << i.previous();
//! [2]


//! [3]
QMutableListIterator<int> i(list);
while (i.hasNext()) {
    if (i.next() % 2 != 0)
        i.remove();
}
//! [3]


//! [4]
QMutableListIterator<int> i(list);
i.toBack();
while (i.hasPrevious()) {
    if (i.previous() % 2 != 0)
        i.remove();
}
//! [4]


//! [5]
QMutableListIterator<int> i(list);
while (i.hasNext()) {
    if (i.next() > 128)
        i.setValue(128);
}
//! [5]


//! [6]
QMutableListIterator<int> i(list);
while (i.hasNext())
    i.next() *= 2;
//! [6]


//! [7]
QMap<QString, QString> map;
map.insert("Paris", "France");
map.insert("Guatemala City", "Guatemala");
map.insert("Mexico City", "Mexico");
map.insert("Moscow", "Russia");
...

QMutableMapIterator<QString, QString> i(map);
while (i.hasNext()) {
    if (i.next().key().endsWith("City"))
        i.remove();
}
//! [7]


//! [8]
QMap<int, QWidget *> map;
QHash<int, QWidget *> hash;

QMapIterator<int, QWidget *> i(map);
while (i.hasNext()) {
    i.next();
    hash.insert(i.key(), i.value());
}
//! [8]


//! [9]
QMutableMapIterator<int, QWidget *> i(map);
while (i.findNext(widget))
    i.remove();
//! [9]


//! [10]
QList<QString> list;
list << "A" << "B" << "C" << "D";

QList<QString>::iterator i;
for (i = list.begin(); i != list.end(); ++i)
    *i = (*i).toLower();
//! [10]


//! [11]
QList<QString> list;
list << "A" << "B" << "C" << "D";

QList<QString>::iterator i = list.end();
while (i != list.begin()) {
    --i;
    *i = (*i).toLower();
}
//! [11]


//! [12]
QList<QString>::const_iterator i;
for (i = list.constBegin(); i != list.constEnd(); ++i)
    qDebug() << *i;
//! [12]


//! [13]
QMap<int, int> map;
...
QMap<int, int>::const_iterator i;
for (i = map.constBegin(); i != map.constEnd(); ++i)
    qDebug() << i.key() << ":" << i.value();
//! [13]


//! [14]
// RIGHT
const QList<int> sizes = splitter->sizes();
QList<int>::const_iterator i;
for (i = sizes.begin(); i != sizes.end(); ++i)
    ...

// WRONG
QList<int>::const_iterator i;
for (i = splitter->sizes().begin();
        i != splitter->sizes().end(); ++i)
    ...
//! [14]


//! [15]
QLinkedList<QString> list;
...
QString str;
foreach (str, list)
    qDebug() << str;
//! [15]


//! [16]
QLinkedList<QString> list;
...
QLinkedListIterator<QString> i(list);
while (i.hasNext())
    qDebug() << i.next();
//! [16]


//! [17]
QLinkedList<QString> list;
...
foreach (const QString &str, list)
    qDebug() << str;
//! [17]


//! [18]
QLinkedList<QString> list;
...
foreach (const QString &str, list) {
    if (str.isEmpty())
        break;
    qDebug() << str;
}
//! [18]


//! [19]
QMap<QString, int> map;
...
foreach (const QString &str, map.keys())
    qDebug() << str << ":" << map.value(str);
//! [19]


//! [20]
QMultiMap<QString, int> map;
...
foreach (const QString &str, map.uniqueKeys()) {
    foreach (int i, map.values(str))
        qDebug() << str << ":" << i;
}
//! [20]


//! [21]
forever {
    ...
}
//! [21]


//! [22]
CONFIG += no_keywords
//! [22]


//! [23]
QString onlyLetters(const QString &in)
{
    QString out;
    for (int j = 0; j < in.size(); ++j) {
        if (in[j].isLetter())
            out += in[j];
    }
    return out;
}
//! [23]

//! [24]
QVector<int> a, b;
a.resize(100000); // make a big vector filled with 0.

QVector<int>::iterator i = a.begin();
// WRONG way of using the iterator i:
b = a;
/*
    Now we should be careful with iterator i since it will point to shared data
    If we do *i = 4 then we would change the shared instance (both vectors)
    The behavior differs from STL containers. Avoid doing such things in Qt.
*/

a[0] = 5;
/*
    Container a is now detached from the shared data,
    and even though i was an iterator from the container a, it now works as an iterator in b.
    Here the situation is that (*i) == 0.
*/

b.clear(); // Now the iterator i is completely invalid.

int j = *i; // Undefined behavior!
/*
    The data from b (which i pointed to) is gone.
    This would be well-defined with STL containers (and (*i) == 5),
    but with QVector this is likely to crash.
*/
//! [24]
