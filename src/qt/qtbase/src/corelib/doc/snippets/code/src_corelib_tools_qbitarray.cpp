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
QBitArray ba(200);
//! [0]


//! [1]
QBitArray ba;
ba.resize(3);
ba[0] = true;
ba[1] = false;
ba[2] = true;
//! [1]


//! [2]
QBitArray ba(3);
ba.setBit(0, true);
ba.setBit(1, false);
ba.setBit(2, true);
//! [2]


//! [3]
QBitArray x(5);
x.setBit(3, true);
// x: [ 0, 0, 0, 1, 0 ]

QBitArray y(5);
y.setBit(4, true);
// y: [ 0, 0, 0, 0, 1 ]

x |= y;
// x: [ 0, 0, 0, 1, 1 ]
//! [3]


//! [4]
QBitArray().isNull();           // returns true
QBitArray().isEmpty();          // returns true

QBitArray(0).isNull();          // returns false
QBitArray(0).isEmpty();         // returns true

QBitArray(3).isNull();          // returns false
QBitArray(3).isEmpty();         // returns false
//! [4]


//! [5]
QBitArray().isNull();           // returns true
QBitArray(0).isNull();          // returns false
QBitArray(3).isNull();          // returns false
//! [5]


//! [6]
QBitArray ba(8);
ba.fill(true);
// ba: [ 1, 1, 1, 1, 1, 1, 1, 1 ]

ba.fill(false, 2);
// ba: [ 0, 0 ]
//! [6]


//! [7]
QBitArray a(3);
a[0] = false;
a[1] = true;
a[2] = a[0] ^ a[1];
//! [7]


//! [8]
QBitArray a(3);
QBitArray b(2);
a[0] = 1; a[1] = 0; a[2] = 1;   // a: [ 1, 0, 1 ]
b[0] = 1; b[1] = 1;             // b: [ 1, 1 ]
a &= b;                         // a: [ 1, 0, 0 ]
//! [8]


//! [9]
QBitArray a(3);
QBitArray b(2);
a[0] = 1; a[1] = 0; a[2] = 1;   // a: [ 1, 0, 1 ]
b[0] = 1; b[1] = 1;             // b: [ 1, 1 ]
a |= b;                         // a: [ 1, 1, 1 ]
//! [9]


//! [10]
QBitArray a(3);
QBitArray b(2);
a[0] = 1; a[1] = 0; a[2] = 1;   // a: [ 1, 0, 1 ]
b[0] = 1; b[1] = 1;             // b: [ 1, 1 ]
a ^= b;                         // a: [ 0, 1, 1 ]
//! [10]


//! [11]
QBitArray a(3);
QBitArray b;
a[0] = 1; a[1] = 0; a[2] = 1;   // a: [ 1, 0, 1 ]
b = ~a;                         // b: [ 0, 1, 0 ]
//! [11]


//! [12]
QBitArray a(3);
QBitArray b(2);
QBitArray c;
a[0] = 1; a[1] = 0; a[2] = 1;   // a: [ 1, 0, 1 ]
b[0] = 1; b[1] = 1;             // b: [ 1, 1 ]
c = a & b;                      // c: [ 1, 0, 0 ]
//! [12]


//! [13]
QBitArray a(3);
QBitArray b(2);
QBitArray c;
a[0] = 1; a[1] = 0; a[2] = 1;   // a: [ 1, 0, 1 ]
b[0] = 1; b[1] = 1;             // b: [ 1, 1 ]
c = a | b;                      // c: [ 1, 1, 1 ]
//! [13]


//! [14]
QBitArray a(3);
QBitArray b(2);
QBitArray c;
a[0] = 1; a[1] = 0; a[2] = 1;   // a: [ 1, 0, 1 ]
b[0] = 1; b[1] = 1;             // b: [ 1, 1 ]
c = a ^ b;                      // c: [ 0, 1, 1 ]
//! [14]
