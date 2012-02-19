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

#include "textstream.h"

#include <QDebug>

namespace commonjs {

// public:
TextStream::TextStream(QFile *openfile, QObject *parent) :
    QObject(parent),
    m_file(openfile)
{
    m_fileStream.setDevice(m_file);
}

TextStream::~TextStream()
{
    this->close();
}

// public slots:
QString TextStream::read()
{
    if ( m_file->isReadable() ) {
        return m_fileStream.readAll();
    }
    qDebug() << "TextStream::read - " << "Couldn't read:" << m_file->fileName();
    return QString();
}

bool TextStream::write(const QString &data)
{
    if ( m_file->isWritable() ) {
        m_fileStream << data;
        return true;
    }
    qDebug() << "TextStream::write - " << "Couldn't write:" << m_file->fileName();
    return false;
}

QString TextStream::readLine()
{
    if ( m_file->isReadable() ) {
        return m_fileStream.readLine();
    }
    qDebug() << "TextStream::readLine - " << "Couldn't read:" << m_file->fileName();
    return QString();
}

bool TextStream::writeLine(const QString &data)
{
    if ( write(data) && write("\n") ) {
        return true;
    }
    qDebug() << "TextStream::writeLine - " << "Couldn't write:" << m_file->fileName();
    return false;
}

bool TextStream::atEnd() const
{
    if ( m_file->isReadable() ) {
        return m_fileStream.atEnd();
    }
    qDebug() << "TextStream::atEnd - " << "Couldn't read:" << m_file->fileName();
    return false;
}

void TextStream::flush()
{
    if ( m_file ) {
        m_fileStream.flush();
    }
}

void TextStream::close()
{
    flush();
    if ( m_file ) {
        m_file->close();
        delete m_file;
        m_file = NULL;
    }
    deleteLater();
}

} // namespace commonjs
