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
    return NULL;
}

bool File::write(const QString &data)
{
    if ( m_file->isWritable() ) {
        m_fileStream << data;
        return true;
    }
    return false;
}

QString File::readLine()
{
    if ( m_file->isReadable() ) {
        return m_fileStream.readLine();
    }
    return NULL;
}

bool File::writeLine(const QString &data)
{
    if ( write(data) && write("\n") ) {
        return true;
    }
    return false;
}

bool File::atEnd() const
{
    if ( m_file->isReadable() ) {
        return m_fileStream.atEnd();
    }
    return false;
}

void File::flush()
{
    if ( m_file->isWritable() ) {
        m_fileStream.flush();
    }
}

void File::close()
{
    if ( m_file->isOpen() ) {
        flush();
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

bool FileSystem::isDir(const QString &path) const
{
    return QFileInfo(path).isDir();
}

bool FileSystem::isFile(const QString &path) const
{
    return QFileInfo(path).isFile();
}

bool FileSystem::mkDir(const QString &path) const
{
    return QDir().mkpath(path);
}

QStringList FileSystem::list(const QString &path) const
{
    return QDir(path).entryList();
}

QString FileSystem::workDir() const
{
    return QDir::currentPath();
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

    // Determine the OpenMode
    if ( mode.contains('r', Qt::CaseInsensitive) ) { modeCode |= QFile::ReadOnly; }
    if ( mode.contains('w', Qt::CaseInsensitive) ) { modeCode |= QFile::WriteOnly; }
    if ( mode.contains('a', Qt::CaseInsensitive) ) { modeCode |= QFile::WriteOnly | QFile::Append; }

    // Try to Open
    if ( _f->open(modeCode) ) {
        f = new File(_f);
    }

    // Return 'false/undefined' if the file couldn't be opened as requested
    return (f) ? f : false;
}

bool FileSystem::remove(const QString &path) const
{
    return QFile::remove(path);
}
