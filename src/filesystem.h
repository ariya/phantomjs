/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>

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
    // int size(const QString &path) const; //< TODO - in bytes, or throw excep if doesn't exists
    // QDateTime lastModified(const QString &path) const; //< TODO - returns the time that a file was last modified as a Date object.

    // Tests
    bool exists(const QString &path) const;
    bool isDirectory(const QString &path) const;
    bool isFile(const QString &path) const;
    // bool isLink(const QString &path) const; //< TODO
    // bool isReadable(const QString &path) const; //< TODO
    // bool isWritable(const QString &path) const; //< TODO is it writable or creatable
    // isExecutable //< TODO
    // isMount //< TODO
    // linkExists //< TODO
    // isAbsolute //< TODO
    // same //< TODO

    // Files / Directory
    bool makeDirectory(const QString &path) const;
    bool makeTree(const QString &path) const;
    // copy //< TODO
    // move(source, target) //< TODO - throw exception if it fails
    // touch(path, date) //< TODO
    bool remove(const QString &path) const;
    bool removeDirectory(const QString &path) const;
    bool removeTree(const QString &path) const;
    // rename //< TODO
    // copyTree(source, target) //< TODO - copies files from a source path to a target path,
                                // copying the files of the source tree to the corresponding locations
                                // relative to the target, copying but not traversing into symbolic links to directories.

    // Listing
    QStringList list(const QString &path) const;

    // Links
    // symbolicLink(source, target) //< TODO
    // hardLink(source, target) //< TODO
    // readLink(path) String //< TODO

    // Paths
    // canonical //< TODO
    QString workingDirectory() const;
    bool changeWorkingDirectory(const QString &path) const;
    // absolute //< TODO
    // base //< TODO
    // directory //< TODO
    // extension //< TODO
    // join //< TODO
    // normal //< TODO
    // relative //< TODO
    QString separator() const;
    // split //< TODO

    // Files
    QObject *_open(const QString &path, const QString &mode) const;
    // read //< TODO
    // write //< TODO
};

#endif // FILESYSTEM_H
