/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 execjosh, http://execjosh.blogspot.com

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

#include "encoding.h"

Encoding::Encoding()
{
    QTextCodec *codec = QTextCodec::codecForName(DEFAULT_CODEC_NAME);

    // Fall back to locale codec
    if ((QTextCodec *)NULL == codec) {
        codec = QTextCodec::codecForLocale();
    }

    m_codec = codec;
}

Encoding::Encoding(const QString &encoding)
{
    setEncoding(encoding);
}

Encoding::~Encoding()
{
    m_codec = (QTextCodec *)NULL;
}

QString Encoding::decode(const QByteArray &bytes) const
{
    return getCodec()->toUnicode(bytes);
}

QByteArray Encoding::encode(const QString &string) const
{
    return getCodec()->fromUnicode(string);
}

QString Encoding::getName() const
{
    // TODO Is it safe to assume UTF-8 here?
    return QString::fromUtf8(getCodec()->name());
}

void Encoding::setEncoding(const QString &encoding)
{
    if (!encoding.isEmpty()) {
        QTextCodec *codec = QTextCodec::codecForName(qPrintable(encoding));

        if ((QTextCodec *)NULL != codec) {
            m_codec = codec;
        }
    }
}

const Encoding Encoding::UTF8 = Encoding("UTF-8");

// private:
QTextCodec *Encoding::getCodec() const
{
    QTextCodec *codec = m_codec;

    if ((QTextCodec *)NULL == codec) {
        codec = QTextCodec::codecForLocale();
    }

    return codec;
}

const QByteArray Encoding::DEFAULT_CODEC_NAME = "UTF-8";
