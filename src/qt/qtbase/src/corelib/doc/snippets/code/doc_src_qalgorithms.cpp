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
QStringList list;
list << "one" << "two" << "three";

qFill(list.begin(), list.end(), "eleven");
// list: [ "eleven", "eleven", "eleven" ]
//! [0]


//! [1]
qFill(list.begin() + 1, list.end(), "six");
// list: [ "eleven", "six", "six" ]
//! [1]


//! [2]
QChar resolveEntity(const QString &entity)
{
    static const QLatin1String name_table[] = {
        "AElig", "Aacute", ..., "zwnj"
    };
    static const ushort value_table[] = {
        0x0061, 0x00c1, ..., 0x200c
    };
    int N = sizeof(name_table) / sizeof(name_table[0]);

    const QLatin1String *name = qBinaryFind(name_table, name_table + N,
                                            entity);
    int index = name - name_table;
    if (index == N)
        return QChar();

    return QChar(value_table[index]);
}
//! [2]


//! [3]
QChar resolveEntity(const QString &entity)
{
    static QMap<QString, int> entityMap;

    if (!entityMap) {
        entityMap.insert("AElig", 0x0061);
        entityMap.insert("Aacute", 0x00c1);
        ...
        entityMap.insert("zwnj", 0x200c);
    }
    return QChar(entityMap.value(entity));
}
//! [3]


//! [4]
QStringList list;
list << "one" << "two" << "three";

QVector<QString> vect1(3);
qCopy(list.begin(), list.end(), vect1.begin());
// vect: [ "one", "two", "three" ]

QVector<QString> vect2(8);
qCopy(list.begin(), list.end(), vect2.begin() + 2);
// vect: [ "", "", "one", "two", "three", "", "", "" ]
//! [4]


//! [5]
QStringList list;
list << "one" << "two" << "three";

QVector<QString> vect(5);
qCopyBackward(list.begin(), list.end(), vect.end());
// vect: [ "", "", "one", "two", "three" ]
//! [5]


//! [6]
QStringList list;
list << "one" << "two" << "three";

QVector<QString> vect(3);
vect[0] = "one";
vect[1] = "two";
vect[2] = "three";

bool ret1 = qEqual(list.begin(), list.end(), vect.begin());
// ret1 == true

vect[2] = "seven";
bool ret2 = qEqual(list.begin(), list.end(), vect.begin());
// ret2 == false
//! [6]


//! [7]
QStringList list;
list << "one" << "two" << "three";

qFill(list.begin(), list.end(), "eleven");
// list: [ "eleven", "eleven", "eleven" ]

qFill(list.begin() + 1, list.end(), "six");
// list: [ "eleven", "six", "six" ]
//! [7]


//! [8]
QStringList list;
list << "one" << "two" << "three";

QStringList::iterator i1 = qFind(list.begin(), list.end(), "two");
// i1 == list.begin() + 1

QStringList::iterator i2 = qFind(list.begin(), list.end(), "seventy");
// i2 == list.end()
//! [8]


//! [9]
QList<int> list;
list << 3 << 3 << 6 << 6 << 6 << 8;

int countOf6 = 0;
qCount(list.begin(), list.end(), 6, countOf6);
// countOf6 == 3

int countOf7 = 0;
qCount(list.begin(), list.end(), 7, countOf7);
// countOf7 == 0
//! [9]


//! [10]
double pi = 3.14;
double e = 2.71;

qSwap(pi, e);
// pi == 2.71, e == 3.14
//! [10]


//! [11]
QList<int> list;
list << 33 << 12 << 68 << 6 << 12;
qSort(list.begin(), list.end());
// list: [ 6, 12, 12, 33, 68 ]
//! [11]


//! [12]
bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
{
    return s1.toLower() < s2.toLower();
}

int doSomething()
{
    QStringList list;
    list << "AlPha" << "beTA" << "gamma" << "DELTA";
    qSort(list.begin(), list.end(), caseInsensitiveLessThan);
    // list: [ "AlPha", "beTA", "DELTA", "gamma" ]
}
//! [12]


//! [13]
QList<int> list;
list << 33 << 12 << 68 << 6 << 12;
qSort(list.begin(), list.end(), qGreater<int>());
// list: [ 68, 33, 12, 12, 6 ]
//! [13]


//! [14]
QStringList list;
list << "AlPha" << "beTA" << "gamma" << "DELTA";

QMap<QString, QString> map;
foreach (const QString &str, list)
    map.insert(str.toLower(), str);

list = map.values();
//! [14]


//! [15]
QList<int> list;
list << 33 << 12 << 68 << 6 << 12;
qStableSort(list.begin(), list.end());
// list: [ 6, 12, 12, 33, 68 ]
//! [15]


//! [16]
bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
{
    return s1.toLower() < s2.toLower();
}

int doSomething()
{
    QStringList list;
    list << "AlPha" << "beTA" << "gamma" << "DELTA";
    qStableSort(list.begin(), list.end(), caseInsensitiveLessThan);
    // list: [ "AlPha", "beTA", "DELTA", "gamma" ]
}
//! [16]


//! [17]
QList<int> list;
list << 33 << 12 << 68 << 6 << 12;
qStableSort(list.begin(), list.end(), qGreater<int>());
// list: [ 68, 33, 12, 12, 6 ]
//! [17]


//! [18]
QList<int> list;
list << 3 << 3 << 6 << 6 << 6 << 8;

QList<int>::iterator i = qLowerBound(list.begin(), list.end(), 5);
list.insert(i, 5);
// list: [ 3, 3, 5, 6, 6, 6, 8 ]

i = qLowerBound(list.begin(), list.end(), 12);
list.insert(i, 12);
// list: [ 3, 3, 5, 6, 6, 6, 8, 12 ]
//! [18]


//! [19]
QVector<int> vect;
vect << 3 << 3 << 6 << 6 << 6 << 8;
QVector<int>::iterator begin6 =
        qLowerBound(vect.begin(), vect.end(), 6);
QVector<int>::iterator end6 =
        qUpperBound(begin6, vect.end(), 6);

QVector<int>::iterator i = begin6;
while (i != end6) {
    *i = 7;
    ++i;
}
// vect: [ 3, 3, 7, 7, 7, 8 ]
//! [19]


//! [20]
QList<int> list;
list << 3 << 3 << 6 << 6 << 6 << 8;

QList<int>::iterator i = qUpperBound(list.begin(), list.end(), 5);
list.insert(i, 5);
// list: [ 3, 3, 5, 6, 6, 6, 8 ]

i = qUpperBound(list.begin(), list.end(), 12);
list.insert(i, 12);
// list: [ 3, 3, 5, 6, 6, 6, 8, 12 ]
//! [20]


//! [21]
QVector<int> vect;
vect << 3 << 3 << 6 << 6 << 6 << 8;
QVector<int>::iterator begin6 =
        qLowerBound(vect.begin(), vect.end(), 6);
QVector<int>::iterator end6 =
        qUpperBound(vect.begin(), vect.end(), 6);

QVector<int>::iterator i = begin6;
while (i != end6) {
    *i = 7;
    ++i;
}
// vect: [ 3, 3, 7, 7, 7, 8 ]
//! [21]


//! [22]
QVector<int> vect;
vect << 3 << 3 << 6 << 6 << 6 << 8;

QVector<int>::iterator i =
        qBinaryFind(vect.begin(), vect.end(), 6);
// i == vect.begin() + 2 (or 3 or 4)
//! [22]


//! [23]
QList<Employee *> list;
list.append(new Employee("Blackpool", "Stephen"));
list.append(new Employee("Twist", "Oliver"));

qDeleteAll(list.begin(), list.end());
list.clear();
//! [23]


//! [24]
QList<int> list;
list << 33 << 12 << 68 << 6 << 12;
qSort(list.begin(), list.end(), qLess<int>());
// list: [ 6, 12, 12, 33, 68 ]
//! [24]


//! [25]
QList<int> list;
list << 33 << 12 << 68 << 6 << 12;
qSort(list.begin(), list.end(), qGreater<int>());
// list: [ 68, 33, 12, 12, 6 ]
//! [25]
