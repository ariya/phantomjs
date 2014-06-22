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
QMap<QString, int> map;
//! [0]


//! [1]
map["one"] = 1;
map["three"] = 3;
map["seven"] = 7;
//! [1]


//! [2]
map.insert("twelve", 12);
//! [2]


//! [3]
int num1 = map["thirteen"];
int num2 = map.value("thirteen");
//! [3]


//! [4]
int timeout = 30;
if (map.contains("TIMEOUT"))
    timeout = map.value("TIMEOUT");
//! [4]


//! [5]
int timeout = map.value("TIMEOUT", 30);
//! [5]


//! [6]
// WRONG
QMap<int, QWidget *> map;
...
for (int i = 0; i < 1000; ++i) {
    if (map[i] == okButton)
        cout << "Found button at index " << i << endl;
}
//! [6]


//! [7]
QMapIterator<QString, int> i(map);
while (i.hasNext()) {
    i.next();
    cout << i.key() << ": " << i.value() << endl;
}
//! [7]


//! [8]
QMap<QString, int>::const_iterator i = map.constBegin();
while (i != map.constEnd()) {
    cout << i.key() << ": " << i.value() << endl;
    ++i;
}
//! [8]


//! [9]
map.insert("plenty", 100);
map.insert("plenty", 2000);
// map.value("plenty") == 2000
//! [9]


//! [10]
QList<int> values = map.values("plenty");
for (int i = 0; i < values.size(); ++i)
    cout << values.at(i) << endl;
//! [10]


//! [11]
QMap<QString, int>::iterator i = map.find("plenty");
while (i != map.end() && i.key() == "plenty") {
    cout << i.value() << endl;
    ++i;
}
//! [11]


//! [12]
QMap<QString, int> map;
...
foreach (int value, map)
    cout << value << endl;
//! [12]


//! [13]
#ifndef EMPLOYEE_H
#define EMPLOYEE_H

class Employee
{
public:
    Employee() {}
    Employee(const QString &name, const QDate &dateOfBirth);
    ...

private:
    QString myName;
    QDate myDateOfBirth;
};

inline bool operator<(const Employee &e1, const Employee &e2)
{
    if (e1.name() != e2.name())
        return e1.name() < e2.name();
    return e1.dateOfBirth() < e2.dateOfBirth();
}

#endif // EMPLOYEE_H
//! [13]


//! [14]
QMap<QString, int> map;
...
QMap<QString, int>::const_iterator i = map.find("HDR");
while (i != map.end() && i.key() == "HDR") {
    cout << i.value() << endl;
    ++i;
}
//! [14]


//! [15]
QMap<int, QString> map;
map.insert(1, "one");
map.insert(5, "five");
map.insert(10, "ten");

map.lowerBound(0);      // returns iterator to (1, "one")
map.lowerBound(1);      // returns iterator to (1, "one")
map.lowerBound(2);      // returns iterator to (5, "five")
map.lowerBound(10);     // returns iterator to (10, "ten")
map.lowerBound(999);    // returns end()
//! [15]


//! [16]
QMap<QString, int> map;
...
QMap<QString, int>::const_iterator i = map.lowerBound("HDR");
QMap<QString, int>::const_iterator upperBound = map.upperBound("HDR");
while (i != upperBound) {
    cout << i.value() << endl;
    ++i;
}
//! [16]


//! [17]
QMap<int, QString> map;
map.insert(1, "one");
map.insert(5, "five");
map.insert(10, "ten");

map.upperBound(0);      // returns iterator to (1, "one")
map.upperBound(1);      // returns iterator to (5, "five")
map.upperBound(2);      // returns iterator to (5, "five")
map.upperBound(10);     // returns end()
map.upperBound(999);    // returns end()
//! [17]


//! [18]
QMap<QString, int> map;
map.insert("January", 1);
map.insert("February", 2);
...
map.insert("December", 12);

QMap<QString, int>::iterator i;
for (i = map.begin(); i != map.end(); ++i)
    cout << i.key() << ": " << i.value() << endl;
//! [18]


//! [19]
QMap<QString, int>::iterator i;
for (i = map.begin(); i != map.end(); ++i)
    i.value() += 2;
//! [19]


//! [20]
QMap<QString, int>::iterator i = map.begin();
while (i != map.end()) {
    if (i.key().startsWith("_"))
        i = map.erase(i);
    else
        ++i;
}
//! [20]


//! [21]
QMap<QString, int>::iterator i = map.begin();
while (i != map.end()) {
    QMap<QString, int>::iterator prev = i;
    ++i;
    if (prev.key().startsWith("_"))
        map.erase(prev);
}
//! [21]


//! [22]
// WRONG
while (i != map.end()) {
    if (i.key().startsWith("_"))
        map.erase(i);
    ++i;
}
//! [22]


//! [23]
if (i.key() == "Hello")
    i.value() = "Bonjour";
//! [23]


//! [24]
QMap<QString, int> map;
map.insert("January", 1);
map.insert("February", 2);
...
map.insert("December", 12);

QMap<QString, int>::const_iterator i;
for (i = map.constBegin(); i != map.constEnd(); ++i)
    cout << i.key() << ": " << i.value() << endl;
//! [24]


//! [25]
QMultiMap<QString, int> map1, map2, map3;

map1.insert("plenty", 100);
map1.insert("plenty", 2000);
// map1.size() == 2

map2.insert("plenty", 5000);
// map2.size() == 1

map3 = map1 + map2;
// map3.size() == 3
//! [25]


//! [26]
QList<int> values = map.values("plenty");
for (int i = 0; i < values.size(); ++i)
    cout << values.at(i) << endl;
//! [26]


//! [27]
QMultiMap<QString, int>::iterator i = map.find("plenty");
while (i != map.end() && i.key() == "plenty") {
    cout << i.value() << endl;
    ++i;
}
//! [27]
