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
QSize t1(10, 12);
t1.scale(60, 60, Qt::IgnoreAspectRatio);
// t1 is (60, 60)

QSize t2(10, 12);
t2.scale(60, 60, Qt::KeepAspectRatio);
// t2 is (50, 60)

QSize t3(10, 12);
t3.scale(60, 60, Qt::KeepAspectRatioByExpanding);
// t3 is (60, 72)
//! [0]


//! [1]
QSize size(100, 10);
size.rwidth() += 20;

// size becomes (120,10)
//! [1]


//! [2]
QSize size(100, 10);
size.rheight() += 5;

// size becomes (100,15)
//! [2]


//! [3]
QSize s( 3, 7);
QSize r(-1, 4);
s += r;

// s becomes (2,11)
//! [3]


//! [4]
QSize s( 3, 7);
QSize r(-1, 4);
s -= r;

// s becomes (4,3)
//! [4]


//! [5]
QSizeF t1(10, 12);
t1.scale(60, 60, Qt::IgnoreAspectRatio);
// t1 is (60, 60)

QSizeF t2(10, 12);
t2.scale(60, 60, Qt::KeepAspectRatio);
// t2 is (50, 60)

QSizeF t3(10, 12);
t3.scale(60, 60, Qt::KeepAspectRatioByExpanding);
// t3 is (60, 72)
//! [5]


//! [6]
QSizeF size(100.3, 10);
size.rwidth() += 20.5;

 // size becomes (120.8,10)
//! [6]


//! [7]
QSizeF size(100, 10.2);
size.rheight() += 5.5;

// size becomes (100,15.7)
//! [7]


//! [8]
QSizeF s( 3, 7);
QSizeF r(-1, 4);
s += r;

// s becomes (2,11)
//! [8]


//! [9]
QSizeF s( 3, 7);
QSizeF r(-1, 4);
s -= r;

// s becomes (4,3)
//! [9]
