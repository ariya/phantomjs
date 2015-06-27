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

#include "terminal.h"

#include <QCoreApplication>

#include <iostream>

static Terminal* terminal_instance = 0;

Terminal* Terminal::instance()
{
    if (!terminal_instance) {
        terminal_instance = new Terminal();
    }

    return terminal_instance;
}

Terminal::Terminal()
    : QObject(QCoreApplication::instance())
{
}

QString Terminal::getEncoding() const
{
    return m_encoding.getName();
}

bool Terminal::setEncoding(const QString& encoding)
{
    // Since there can be multiple names for the same codec (i.e., "utf8" and
    // "utf-8"), we need to get the codec in the system first and use its
    // canonical name
    QTextCodec* codec = QTextCodec::codecForName(encoding.toLatin1());
    if ((QTextCodec*)NULL == codec) {
        return false;
    }

    // Check whether encoding actually needs to be changed
    const QString encodingBeforeUpdate(m_encoding.getName());
    if (0 == encodingBeforeUpdate.compare(QString(codec->name()), Qt::CaseInsensitive)) {
        return false;
    }

    m_encoding.setEncoding(encoding);

    // Emit the signal only if the encoding actually was changed
    const QString encodingAfterUpdate(m_encoding.getName());
    if (0 == encodingBeforeUpdate.compare(encodingAfterUpdate, Qt::CaseInsensitive)) {
        return false;
    }

    emit encodingChanged(encoding);

    return true;
}

void Terminal::cout(const QString& string, const bool newline) const
{
    output(std::cout, string, newline);
}

void Terminal::cerr(const QString& string, const bool newline) const
{
    output(std::cerr, string, newline);
}

// private
void Terminal::output(std::ostream& out, const QString& string, const bool newline) const
{
    out << m_encoding.encode(string).constData();
    if (newline) {
        out << std::endl;
    }
    out << std::flush;
}
