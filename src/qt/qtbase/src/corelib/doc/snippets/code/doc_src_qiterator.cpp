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
QList<float> list;
...
QListIterator<float> i(list);
while (i.hasNext())
    qDebug() << i.next();
//! [0]


//! [1]
QListIterator<float> i(list);
i.toBack();
while (i.hasPrevious())
    qDebug() << i.previous();
//! [1]


//! [2]
QLinkedList<float> list;
...
QLinkedListIterator<float> i(list);
while (i.hasNext())
    qDebug() << i.next();
//! [2]


//! [3]
QLinkedListIterator<float> i(list);
i.toBack();
while (i.hasPrevious())
    qDebug() << i.previous();
//! [3]


//! [4]
QVector<float> vector;
...
QVectorIterator<float> i(vector);
while (i.hasNext())
    qDebug() << i.next();
//! [4]


//! [5]
QVectorIterator<float> i(vector);
i.toBack();
while (i.hasPrevious())
    qDebug() << i.previous();
//! [5]


//! [6]
QSet<QString> set;
...
QSetIterator<QString> i(set);
while (i.hasNext())
    qDebug() << i.next();
//! [6]


//! [7]
QSetIterator<QString> i(set);
i.toBack();
while (i.hasPrevious())
    qDebug() << i.previous();
//! [7]


//! [8]
QList<float> list;
...
QMutableListIterator<float> i(list);
while (i.hasNext())
    qDebug() << i.next();
//! [8]


//! [9]
QMutableListIterator<float> i(list);
i.toBack();
while (i.hasPrevious())
    qDebug() << i.previous();
//! [9]


//! [10]
QMutableListIterator<int> i(list);
while (i.hasNext()) {
    int val = i.next();
    if (val < 0) {
        i.setValue(-val);
    } else if (val == 0) {
        i.remove();
    }
}
//! [10]


//! [11]
QLinkedList<float> list;
...
QMutableLinkedListIterator<float> i(list);
while (i.hasNext())
    qDebug() << i.next();
//! [11]


//! [12]
QMutableLinkedListIterator<float> i(list);
i.toBack();
while (i.hasPrevious())
    qDebug() << i.previous();
//! [12]


//! [13]
QMutableLinkedListIterator<int> i(list);
while (i.hasNext()) {
    int val = i.next();
    if (val < 0) {
        i.setValue(-val);
    } else if (val == 0) {
        i.remove();
    }
}
//! [13]


//! [14]
QVector<float> vector;
...
QMutableVectorIterator<float> i(vector);
while (i.hasNext())
    qDebug() << i.next();
//! [14]


//! [15]
QMutableVectorIterator<float> i(vector);
i.toBack();
while (i.hasPrevious())
    qDebug() << i.previous();
//! [15]


//! [16]
QMutableVectorIterator<int> i(vector);
while (i.hasNext()) {
    int val = i.next();
    if (val < 0) {
        i.setValue(-val);
    } else if (val == 0) {
        i.remove();
    }
}
//! [16]


//! [17]
QSet<float> set;
...
QMutableSetIterator<float> i(set);
while (i.hasNext())
    qDebug() << i.next();
//! [17]


//! [18]
QMutableSetIterator<float> i(set);
i.toBack();
while (i.hasPrevious())
    qDebug() << i.previous();
//! [18]


//! [19]
QMutableListIterator<int> i(list);
while (i.hasNext()) {
    int val = i.next();
    if (val < -32768 || val > 32767)
        i.remove();
}
//! [19]


//! [20]
QMutableLinkedListIterator<int> i(list);
while (i.hasNext()) {
    int val = i.next();
    if (val < -32768 || val > 32767)
        i.remove();
}
//! [20]


//! [21]
QMutableVectorIterator<int> i(vector);
while (i.hasNext()) {
    int val = i.next();
    if (val < -32768 || val > 32767)
        i.remove();
}
//! [21]


//! [22]
QMutableSetIterator<int> i(set);
while (i.hasNext()) {
    int val = i.next();
    if (val < -32768 || val > 32767)
        i.remove();
}
//! [22]


//! [23]
QMutableListIterator<double> i(list);
while (i.hasNext()) {
    double val = i.next();
    i.setValue(sqrt(val));
}
//! [23]


//! [24]
QMutableLinkedListIterator<double> i(list);
while (i.hasNext()) {
    double val = i.next();
    i.setValue(sqrt(val));
}
//! [24]


//! [25]
QMutableVectorIterator<double> i(list);
while (i.hasNext()) {
    double val = i.next();
    i.setValue(sqrt(val));
}
//! [25]


//! [26]
QMap<int, QWidget *> map;
...
QMapIterator<int, QWidget *> i(map);
while (i.hasNext()) {
    i.next();
    qDebug() << i.key() << ": " << i.value();
}
//! [26]


//! [27]
QMapIterator<int, QWidget *> i(map);
i.toBack();
while (i.hasPrevious()) {
    i.previous();
    qDebug() << i.key() << ": " << i.value();
}
//! [27]


//! [28]
QMapIterator<int, QWidget *> i(map);
while (i.findNext(widget)) {
    qDebug() << "Found widget " << widget << " under key "
             << i.key();
}
//! [28]


//! [29]
QHash<int, QWidget *> hash;
...
QHashIterator<int, QWidget *> i(hash);
while (i.hasNext()) {
    i.next();
    qDebug() << i.key() << ": " << i.value();
}
//! [29]


//! [30]
QHashIterator<int, QWidget *> i(hash);
i.toBack();
while (i.hasPrevious()) {
    i.previous();
    qDebug() << i.key() << ": " << i.value();
}
//! [30]


//! [31]
QHashIterator<int, QWidget *> i(hash);
while (i.findNext(widget)) {
    qDebug() << "Found widget " << widget << " under key "
             << i.key();
}
//! [31]


//! [32]
QMap<int, QWidget *> map;
...
QMutableMapIterator<int, QWidget *> i(map);
while (i.hasNext()) {
    i.next();
    qDebug() << i.key() << ": " << i.value();
}
//! [32]


//! [33]
QMutableMapIterator<int, QWidget *> i(map);
i.toBack();
while (i.hasPrevious()) {
    i.previous();
    qDebug() << i.key() << ": " << i.value();
}
//! [33]


//! [34]
QMutableMapIterator<int, QWidget *> i(map);
while (i.findNext(widget)) {
    qDebug() << "Found widget " << widget << " under key "
             << i.key();
}
//! [34]


//! [35]
QMutableMapIterator<QString, QString> i(map);
while (i.hasNext()) {
    i.next();
    if (i.key() == i.value())
        i.remove();
}
//! [35]


//! [36]
QHash<int, QWidget *> hash;
...
QMutableHashIterator<QString, QWidget *> i(hash);
while (i.hasNext()) {
    i.next();
    qDebug() << i.key() << ": " << i.value();
}
//! [36]


//! [37]
QMutableHashIterator<int, QWidget *> i(hash);
i.toBack();
while (i.hasPrevious()) {
    i.previous();
    qDebug() << i.key() << ": " << i.value();
}
//! [37]


//! [38]
QMutableHashIterator<int, QWidget *> i(hash);
while (i.findNext(widget)) {
    qDebug() << "Found widget " << widget << " under key "
             << i.key();
}
//! [38]


//! [39]
QMutableHashIterator<QString, QString> i(hash);
while (i.hasNext()) {
    i.next();
    if (i.key() == i.value())
        i.remove();
}
//! [39]
