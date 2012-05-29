/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
 */

#include "config.h"
#include "WebDOMCString.h"

#include "TextEncoding.h"
#include "WebDOMString.h"
#include <wtf/text/CString.h>

class WebDOMCStringPrivate : public WTF::CStringBuffer {
};

void WebDOMCString::reset()
{
    if (m_private) {
        m_private->deref();
        m_private = 0;
    }
}

void WebDOMCString::assign(const WebDOMCString& other)
{
    assign(const_cast<WebDOMCStringPrivate*>(other.m_private));
}

void WebDOMCString::assign(const char* data, size_t length)
{
    char* newData;
    RefPtr<WTF::CStringBuffer> buffer =
        WTF::CString::newUninitialized(length, newData).buffer();
    memcpy(newData, data, length);
    assign(static_cast<WebDOMCStringPrivate*>(buffer.get()));
}

size_t WebDOMCString::length() const
{
    if (!m_private)
        return 0;
    // NOTE: The buffer's length includes the null byte.
    return const_cast<WebDOMCStringPrivate*>(m_private)->length() - 1;
}

const char* WebDOMCString::data() const
{
    if (!m_private)
        return 0;
    return const_cast<WebDOMCStringPrivate*>(m_private)->data();
}

WebDOMString WebDOMCString::utf16() const
{
    return WebCore::UTF8Encoding().decode(data(), length());
}

WebDOMCString WebDOMCString::fromUTF16(const WebUChar* data, size_t length)
{
    return WebCore::UTF8Encoding().encode(
        data, length, WebCore::QuestionMarksForUnencodables);
}

WebDOMCString WebDOMCString::fromUTF16(const WebUChar* data)
{
    size_t len = 0;
    while (data[len] != WebUChar(0))
        len++;
    return fromUTF16(data, len);
}

WebDOMCString::WebDOMCString(const WTF::CString& s)
    : m_private(static_cast<WebDOMCStringPrivate*>(s.buffer()))
{
    if (m_private)
        m_private->ref();
}

WebDOMCString& WebDOMCString::operator=(const WTF::CString& s)
{
    assign(static_cast<WebDOMCStringPrivate*>(s.buffer()));
    return *this;
}

WebDOMCString::operator WTF::CString() const
{
    return m_private;
}

void WebDOMCString::assign(WebDOMCStringPrivate* p)
{
    // Take care to handle the case where m_private == p
    if (p)
        p->ref();
    if (m_private)
        m_private->deref();
    m_private = p;
}
