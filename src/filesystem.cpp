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

#include "filesystem.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

// File
// public:
File::File(QFile *openfile, QObject *parent) :
    QObject(parent),
    m_file(openfile)
{
    m_fileStream.setDevice(m_file);
}

File::~File()
{
    this->close();
}

// public slots:
QString File::read()
{
    if ( m_file->isReadable() ) {
        return m_fileStream.readAll();
    }
    qDebug() << "File::read - " << "Couldn't read:" << m_file->fileName();
    return QString();
}

bool File::write(const QString &data)
{
    if ( m_file->isWritable() ) {
        m_fileStream << data;
        return true;
    }
    qDebug() << "File::write - " << "Couldn't write:" << m_file->fileName();
    return false;
}

QString File::readLine()
{
    if ( m_file->isReadable() ) {
        return m_fileStream.readLine();
    }
    qDebug() << "File::readLine - " << "Couldn't read:" << m_file->fileName();
    return QString();
}

bool File::writeLine(const QString &data)
{
    if ( write(data) && write("\n") ) {
        return true;
    }
    qDebug() << "File::writeLine - " << "Couldn't write:" << m_file->fileName();
    return false;
}

bool File::atEnd() const
{
    if ( m_file->isReadable() ) {
        return m_fileStream.atEnd();
    }
    qDebug() << "File::atEnd - " << "Couldn't read:" << m_file->fileName();
    return false;
}

void File::flush()
{
    if ( m_file ) {
        m_fileStream.flush();
    }
}

void File::close()
{
    flush();
    if ( m_file ) {
        m_file->close();
        delete m_file;
        m_file = NULL;
    }
    deleteLater();
}


// FileSystem
// public:
FileSystem::FileSystem(QObject *parent) :
    QObject(parent)
{ }

// public slots:
bool FileSystem::exists(const QString &path) const
{
    return QFile::exists(path);
}

bool FileSystem::isDirectory(const QString &path) const
{
    return QFileInfo(path).isDir();
}

bool FileSystem::isFile(const QString &path) const
{
    return QFileInfo(path).isFile();
}

bool FileSystem::makeDirectory(const QString &path) const
{
    return QDir().mkdir(path);
}

bool FileSystem::makeTree(const QString &path) const
{
    return QDir().mkpath(path);
}

bool FileSystem::remove(const QString &path) const
{
    return QFile::remove(path);
}

bool FileSystem::removeDirectory(const QString &path) const
{
    return QDir().rmdir(path);
}

bool FileSystem::removeTree(const QString &path) const
{
    QDir dir(path);
    bool res = false;

    if (dir.exists()) {
        foreach(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                res = removeTree(info.absoluteFilePath());
            } else {
                res = remove(info.absoluteFilePath());
            }

            if (!res) {
                return res;
            }
        }
        res = removeDirectory(path);
    }

    return res;
}

QStringList FileSystem::list(const QString &path) const
{
    return QDir(path).entryList();
}

QString FileSystem::workingDirectory() const
{
    return QDir::currentPath();
}

bool FileSystem::changeWorkingDirectory(const QString &path) const
{
    return QDir::setCurrent(path);
}

QString FileSystem::separator() const
{
    return QDir::separator();
}

QObject *FileSystem::open(const QString &path, const QString &mode) const
{
    File *f = NULL;
    QFile *_f = new QFile(path);
    QFile::OpenMode modeCode = QFile::NotOpen;

    // Ensure only one "mode character" has been selected
    if ( mode.length() != 1) {
        qDebug() << "FileSystem::open - " << "Wrong Mode string length:" << mode;
        return NULL;
    }

    // Determine the OpenMode
    switch(mode[0].toAscii()) {
    case 'r': case 'R': {
        modeCode |= QFile::ReadOnly;
        // Make sure there is something to read
        if ( !_f->exists() ) {
            qDebug() << "FileSystem::open - " << "Trying to read a file that doesn't exist:" << path;
            return NULL;
        }
        break;
    }
    case 'a': case 'A': case '+': {
        modeCode |= QFile::Append;
        // NOTE: no "break" here! This case will also execute the code for case 'w'.
    }
    case 'w': case 'W': {
        modeCode |= QFile::WriteOnly;
        // Make sure the file exists OR it can be created at the required path
        if ( !_f->exists() && !makeTree(QFileInfo(path).dir().absolutePath()) ) {
            qDebug() << "FileSystem::open - " << "Full path coulnd't be created:" << path;
            return NULL;
        }
        break;
    }
    default: {
        qDebug() << "FileSystem::open - " << "Wrong Mode:" << mode;
        return NULL;
    }
    }

    // Try to Open
    if ( _f->open(modeCode) ) {
        f = new File(_f);
        if ( f ) {
            return f;
        }
    }

    // Return "NULL" if the file couldn't be opened as requested
    qDebug() << "FileSystem::open - " << "Couldn't be opened:" << path;
    return NULL;
}
