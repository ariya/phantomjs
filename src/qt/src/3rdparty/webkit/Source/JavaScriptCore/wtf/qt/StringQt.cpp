/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#include <wtf/StdLibExtras.h>
#include <wtf/text/WTFString.h>

#include <QString>

namespace WTF {

// String conversions
String::String(const QString& qstr)
{
    if (qstr.isNull())
        return;
    m_impl = StringImpl::create(reinterpret_cast_ptr<const UChar*>(qstr.constData()), qstr.length());
}

String::String(const QStringRef& ref)
{
    if (!ref.string())
        return;
    m_impl = StringImpl::create(reinterpret_cast_ptr<const UChar*>(ref.unicode()), ref.length());
}

String::operator QString() const
{
    return QString(reinterpret_cast<const QChar*>(characters()), length());
}

QDataStream& operator<<(QDataStream& stream, const String& str)
{
    // could be faster
    stream << QString(str);
    return stream;
}

QDataStream& operator>>(QDataStream& stream, String& str)
{
    // mabe not the fastest way, but really easy
    QString tmp;
    stream >> tmp;
    str = tmp;
    return stream;
}

}

// vim: ts=4 sw=4 et
