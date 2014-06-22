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
DEFINES += QT_NO_CAST_FROM_ASCII \
           QT_NO_CAST_TO_ASCII
//! [0]


//! [1]
QString url = QLatin1String("http://www.unicode.org/");
//! [1]


//! [2]
double d = 12.34;
QString str = QString("delta: %1").arg(d, 0, 'E', 3);
// str == "delta: 1.234E+01"
//! [2]


//! [3]
if (str == "auto" || str == "extern"
        || str == "static" || str == "register") {
    ...
}
//! [3]


//! [4]
if (str == QString("auto") || str == QString("extern")
        || str == QString("static") || str == QString("register")) {
    ...
}
//! [4]


//! [5]
if (str == QLatin1String("auto")
        || str == QLatin1String("extern")
        || str == QLatin1String("static")
        || str == QLatin1String("register") {
    ...
}
//! [5]


//! [6]
QLabel *label = new QLabel(QLatin1String("MOD"), this);
//! [6]


//! [7]
QString plain = "#include <QtCore>"
QString html = plain.toHtmlEscaped();
// html == "#include &lt;QtCore&gt;"
//! [7]
