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
QDir("/home/user/Documents")
QDir("C:/Documents and Settings")
//! [0]


//! [1]
QDir("images/landscape.png")
//! [1]


//! [2]
QDir("Documents/Letters/Applications").dirName() // "Applications"
QDir().dirName()                                 // "."
//! [2]


//! [3]
QDir directory("Documents/Letters");
QString path = directory.filePath("contents.txt");
QString absolutePath = directory.absoluteFilePath("contents.txt");
//! [3]


//! [4]
QDir dir("example");
if (!dir.exists())
    qWarning("Cannot find the example directory");
//! [4]


//! [5]
QDir dir = QDir::root();                 // "/"
if (!dir.cd("tmp")) {                    // "/tmp"
    qWarning("Cannot find the \"/tmp\" directory");
} else {
    QFile file(dir.filePath("ex1.txt")); // "/tmp/ex1.txt"
    if (!file.open(QIODevice::ReadWrite))
        qWarning("Cannot create the file %s", file.name());
}
//! [5]


//! [6]
QString bin = "/local/bin";         // where /local/bin is a symlink to /usr/bin
QDir binDir(bin);
QString canonicalBin = binDir.canonicalPath();
// canonicalBin now equals "/usr/bin"

QString ls = "/local/bin/ls";       // where ls is the executable "ls"
QDir lsDir(ls);
QString canonicalLs = lsDir.canonicalPath();
// canonicalLS now equals "/usr/bin/ls".
//! [6]


//! [7]
QDir dir("/home/bob");
QString s;

s = dir.relativeFilePath("images/file.jpg");     // s is "images/file.jpg"
s = dir.relativeFilePath("/home/mary/file.txt"); // s is "../mary/file.txt"
//! [7]


//! [8]
QDir::setSearchPaths("icons", QStringList(QDir::homePath() + "/images"));
QDir::setSearchPaths("docs", QStringList(":/embeddedDocuments"));
...
QPixmap pixmap("icons:undo.png"); // will look for undo.png in QDir::homePath() + "/images"
QFile file("docs:design.odf"); // will look in the :/embeddedDocuments resource path
//! [8]


//! [9]
QDir dir("/tmp/root_link");
dir = dir.canonicalPath();
if (dir.isRoot())
    qWarning("It is a root link");
//! [9]


//! [10]
// The current directory is "/usr/local"
QDir d1("/usr/local/bin");
QDir d2("bin");
if (d1 == d2)
    qDebug("They're the same");
//! [10]


//! [11]
// The current directory is "/usr/local"
QDir d1("/usr/local/bin");
d1.setFilter(QDir::Executable);
QDir d2("bin");
if (d1 != d2)
    qDebug("They differ");
//! [11]


//! [12]
C:/Documents and Settings/Username
//! [12]


//! [13]
Q_INIT_RESOURCE(myapp);
//! [13]


//! [14]
inline void initMyResource() { Q_INIT_RESOURCE(myapp); }

namespace MyNamespace
{
    ...

    void myFunction()
    {
        initMyResource();
    }
}
//! [14]


//! [15]
Q_CLEANUP_RESOURCE(myapp);
//! [15]

}
