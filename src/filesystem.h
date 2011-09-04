/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QObject>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QVariant>

class File : public QObject
{
    Q_OBJECT

public:
    File(QFile *openfile, QObject *parent = 0);
    virtual ~File();

public slots:
    QString read();
    bool write(const QString &data);

    QString readLine();
    bool writeLine(const QString &data);

    bool atEnd() const;
    void flush();
    void close();

private:
    QFile *m_file;
    QTextStream m_fileStream;
};


class FileSystem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString workingDirectory READ workingDirectory)
    Q_PROPERTY(QString separator READ separator)

public:
    FileSystem(QObject *parent = 0);

public slots:
    // Attributes
    // 'size(path)' implemented in "filesystem-shim.js" using '_size(path)'
    int _size(const QString &path) const;
    QVariant lastModified(const QString &path) const;

    // Directory
    // 'copyTree(source, destination)' implemented in "filesystem-shim.js" using '_copyTree(source, destination)'
    bool _copyTree(const QString &source, const QString &destination) const;
    bool makeDirectory(const QString &path) const;
    bool makeTree(const QString &path) const;
    // 'removeDirectory(path)' implemented in "filesystem-shim.js" using '_removeDirectory(path)'
    bool _removeDirectory(const QString &path) const;
    // 'removeTree(path)' implemented in "filesystem-shim.js" using '_removeTree(path)'
    bool _removeTree(const QString &path) const;

    // Files
    // 'open(path, mode)' implemented in "filesystem-shim.js" using '_open(path, mode)'
    QObject *_open(const QString &path, const QString &mode) const;
    // 'read(path)' implemented in "filesystem-shim.js"
    // 'write(path, mode)' implemented in the "filesystem-shim.js"
    // 'remove(path)' implemented in "filesystem-shim.js" using '_remove(path)'
    bool _remove(const QString &path) const;
    // 'copy(source, destination)' implemented in "filesystem-shim.js" using '_copy(source, destination)'
    bool _copy(const QString &source, const QString &destination) const;
    // 'move(source, destination)' implemented in "filesystem-shim.js"
    // 'touch(path)' implemented in "filesystem-shim.js"

    // Listing
    QStringList list(const QString &path) const;

    // Paths
    QString separator() const;
    QString workingDirectory() const;
    bool changeWorkingDirectory(const QString &path) const;
    QString absolute(const QString &relativePath) const;

    // Tests
    bool exists(const QString &path) const;
    bool isDirectory(const QString &path) const;
    bool isFile(const QString &path) const;
    bool isAbsolute(const QString &path) const;
    bool isExecutable(const QString &path) const;
    bool isReadable(const QString &path) const;
    bool isWritable(const QString &path) const;
    bool isLink(const QString &path) const;
};

#endif // FILESYSTEM_H
