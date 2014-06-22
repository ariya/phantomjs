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
QVector<int> integerVector;
QVector<QString> stringVector;
//! [0]


//! [1]
QVector<QString> vector(200);
//! [1]


//! [2]
QVector<QString> vector(200, "Pass");
//! [2]


//! [3]
if (vector[0] == "Liz")
    vector[0] = "Elizabeth";
//! [3]


//! [4]
for (int i = 0; i < vector.size(); ++i) {
    if (vector.at(i) == "Alfonso")
        cout << "Found Alfonso at position " << i << endl;
}
//! [4]


//! [5]
int i = vector.indexOf("Harumi");
if (i != -1)
    cout << "First occurrence of Harumi is at position " << i << endl;
//! [5]


//! [6]
QVector<int> vector(10);
int *data = vector.data();
for (int i = 0; i < 10; ++i)
    data[i] = 2 * i;
//! [6]


//! [7]
QVector<QString> vector(0);
vector.append("one");
vector.append("two");
vector.append("three");
// vector: ["one", "two", "three"]
//! [7]


//! [8]
QVector<QString> vector;
vector.prepend("one");
vector.prepend("two");
vector.prepend("three");
// vector: ["three", "two", "one"]
//! [8]


//! [9]
QVector<QString> vector;
vector << "alpha" << "beta" << "delta";
vector.insert(2, "gamma");
// vector: ["alpha", "beta", "gamma", "delta"]
//! [9]


//! [10]
QVector<double> vector;
vector << 2.718 << 1.442 << 0.4342;
vector.insert(1, 3, 9.9);
// vector: [2.718, 9.9, 9.9, 9.9, 1.442, 0.4342]
//! [10]


//! [11]
QVector<QString> vector(3);
vector.fill("Yes");
// vector: ["Yes", "Yes", "Yes"]

vector.fill("oh", 5);
// vector: ["oh", "oh", "oh", "oh", "oh"]
//! [11]


//! [12]
QVector<QString> vector;
vector << "A" << "B" << "C" << "B" << "A";
vector.indexOf("B");            // returns 1
vector.indexOf("B", 1);         // returns 1
vector.indexOf("B", 2);         // returns 3
vector.indexOf("X");            // returns -1
//! [12]


//! [13]
QList<QString> vector;
vector << "A" << "B" << "C" << "B" << "A";
vector.lastIndexOf("B");        // returns 3
vector.lastIndexOf("B", 3);     // returns 3
vector.lastIndexOf("B", 2);     // returns 1
vector.lastIndexOf("X");        // returns -1
//! [13]


//! [14]
QVector<double> vect;
vect << "red" << "green" << "blue" << "black";

QList<double> list = vect.toList();
// list: ["red", "green", "blue", "black"]
//! [14]


//! [15]
QStringList list;
list << "Sven" << "Kim" << "Ola";

QVector<QString> vect = QVector<QString>::fromList(list);
// vect: ["Sven", "Kim", "Ola"]
//! [15]


//! [16]
std::vector<double> stdvector;
vector.push_back(1.2);
vector.push_back(0.5);
vector.push_back(3.14);

QVector<double> vector = QVector<double>::fromStdVector(stdvector);
//! [16]


//! [17]
QVector<double> vector;
vector << 1.2 << 0.5 << 3.14;

std::vector<double> stdvector = vector.toStdVector();
//! [17]
