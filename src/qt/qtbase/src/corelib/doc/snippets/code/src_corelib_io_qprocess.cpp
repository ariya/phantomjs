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


void wrapInFunction()
{

//! [0]
QProcess builder;
builder.setProcessChannelMode(QProcess::MergedChannels);
builder.start("make", QStringList() << "-j2");

if (!builder.waitForFinished())
    qDebug() << "Make failed:" << builder.errorString();
else
    qDebug() << "Make output:" << builder.readAll();
//! [0]


//! [1]
QProcess more;
more.start("more");
more.write("Text to display");
more.closeWriteChannel();
// QProcess will emit readyRead() once "more" starts printing
//! [1]


//! [2]
command1 | command2
//! [2]


//! [3]
QProcess process1;
QProcess process2;

process1.setStandardOutputProcess(&process2);

process1.start("command1");
process2.start("command2");
//! [3]


//! [4]
class SandboxProcess : public QProcess
{
    ...
 protected:
     void setupChildProcess();
    ...
};

void SandboxProcess::setupChildProcess()
{
    // Drop all privileges in the child process, and enter
    // a chroot jail.
#if defined Q_OS_UNIX
    ::setgroups(0, 0);
    ::chroot("/etc/safe");
    ::chdir("/");
    ::setgid(safeGid);
    ::setuid(safeUid);
    ::umask(0);
#endif
}

//! [4]


//! [5]
QProcess process;
process.start("del /s *.txt");
// same as process.start("del", QStringList() << "/s" << "*.txt");
...
//! [5]


//! [6]
QProcess process;
process.start("dir \"My Documents\"");
//! [6]


//! [7]
QProcess process;
process.start("dir \"Epic 12\"\"\" Singles\"");
//! [7]


//! [8]
QStringList environment = QProcess::systemEnvironment();
// environment = {"PATH=/usr/bin:/usr/local/bin",
//                "USER=greg", "HOME=/home/greg"}
//! [8]

}
