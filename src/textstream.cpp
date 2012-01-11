/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2012 execjosh, http://execjosh.blogspot.com

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

TextStream::TextStream(QTextStream *stream, QObject *parent) :
    QObject(parent),
    m_stream(stream)
{
}

TextStream::~TextStream()
{
    if ((QTextStream *)NULL != m_stream) {
        // "delete" should be performed only by owner
        m_stream = (QTextStream *)NULL;
    }
    deleteLater();
}

// public slots:

QString TextStream::read(qint64 n)
{
    return m_stream->read(n);
}

QString TextStream::readLine()
{
    return m_stream->readLine();
}

bool TextStream::write(const QString &string)
{
    return write(string, false);
}

bool TextStream::writeLine(const QString &string)
{
    return write(string, true);
}

// private:

bool TextStream::write(const QString &string, const bool newline)
{
    (*m_stream) << string;
    if (newline) (*m_stream) << endl;
    return (QTextStream::Ok == m_stream->status());
}
