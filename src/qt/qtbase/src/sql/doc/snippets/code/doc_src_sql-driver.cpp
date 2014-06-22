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

//! [2]
QSqlQuery q;
q.exec("call qtestproc (@outval1, @outval2)");
q.exec("select @outval1, @outval2");
q.next();
qDebug() << q.value(0) << q.value(1); // outputs "42" and "43"
//! [2]


//! [10]
// STORED_PROC uses the return statement or returns multiple result sets
QSqlQuery query;
query.setForwardOnly(true);
query.exec("{call STORED_PROC}");
//! [10]


//! [24]
db.setHostName("MyServer");
db.setDatabaseName("C:\\test.gdb");
//! [24]


//! [25]
// connect to database using the Latin-1 character set
db.setConnectOptions("ISC_DPB_LC_CTYPE=Latin1");
db.open();
//! [25]


//! [26]
QSqlQuery q;
q.exec("execute procedure my_procedure");
q.next();
qDebug() << q.value(0); // outputs the first RETURN/OUT value
//! [26]


//! [31]
QSqlDatabase: QMYSQL driver not loaded
QSqlDatabase: available drivers: QMYSQL
//! [31]
