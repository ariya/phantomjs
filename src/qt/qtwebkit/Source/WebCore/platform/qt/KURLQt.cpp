/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#include "config.h"
#include "KURL.h"

#include "NotImplemented.h"
#include "TextEncoding.h"
#include "qurl.h"
#include <wtf/text/CString.h>

namespace WebCore {

KURL::KURL(const QUrl& url)
{
    *this = KURL(KURL(), url.toEncoded().constData(), UTF8Encoding());
}

KURL::operator QUrl() const
{
    return QUrl(m_string);
}

String KURL::fileSystemPath() const
{
    if (!isValid())
        return String();

    if (protocolIs("file"))
        return static_cast<QUrl>(*this).toLocalFile();

    // A valid qrc resource path begins with a colon.
    if (protocolIs("qrc"))
        return ":" + decodeURLEscapeSequences(path());

    return String();
}

}
