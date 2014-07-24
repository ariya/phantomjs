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
QLocale egyptian(QLocale::Arabic, QLocale::Egypt);
QString s1 = egyptian.toString(1.571429E+07, 'e');
QString s2 = egyptian.toString(10);

double d = egyptian.toDouble(s1);
int i = egyptian.toInt(s2);
//! [0]


//! [1]
QLocale::setDefault(QLocale(QLocale::Hebrew, QLocale::Israel));
QLocale hebrew; // Constructs a default QLocale
QString s1 = hebrew.toString(15714.3, 'e');

bool ok;
double d;

QLocale::setDefault(QLocale::C);
d = QString("1234,56").toDouble(&ok);   // ok == false
d = QString("1234.56").toDouble(&ok);   // ok == true, d == 1234.56

QLocale::setDefault(QLocale::German);
d = QString("1234,56").toDouble(&ok);   // ok == true, d == 1234.56
d = QString("1234.56").toDouble(&ok);   // ok == true, d == 1234.56

QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
str = QString("%1 %L2 %L3")
      .arg(12345).arg(12345).arg(12345, 0, 16);
// str == "12345 12,345 3039"
//! [1]


//! [2]
QLocale korean("ko");
QLocale swiss("de_CH");
//! [2]


//! [3]
bool ok;
double d;

QLocale c(QLocale::C);
d = c.toDouble( "1234.56", &ok );  // ok == true, d == 1234.56
d = c.toDouble( "1,234.56", &ok ); // ok == true, d == 1234.56
d = c.toDouble( "1234,56", &ok );  // ok == false

QLocale german(QLocale::German);
d = german.toDouble( "1234,56", &ok );  // ok == true, d == 1234.56
d = german.toDouble( "1.234,56", &ok ); // ok == true, d == 1234.56
d = german.toDouble( "1234.56", &ok );  // ok == false

d = german.toDouble( "1.234", &ok );    // ok == true, d == 1234.0
//! [3]
