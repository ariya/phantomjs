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
#include "WebDOMString.h"

#include "WebDOMCString.h"
#include <wtf/text/AtomicString.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

class WebDOMStringPrivate : public WTF::StringImpl {
};

void WebDOMString::reset()
{
    if (m_private) {
        m_private->deref();
        m_private = 0;
    }
}

void WebDOMString::assign(const WebDOMString& other)
{
    assign(const_cast<WebDOMStringPrivate*>(other.m_private));
}

void WebDOMString::assign(const WebUChar* data, size_t length)
{
    assign(static_cast<WebDOMStringPrivate*>(
        WTF::StringImpl::create(data, length).get()));
}

size_t WebDOMString::length() const
{
    return m_private ? const_cast<WebDOMStringPrivate*>(m_private)->length() : 0;
}

const WebUChar* WebDOMString::data() const
{
    return m_private ? const_cast<WebDOMStringPrivate*>(m_private)->characters() : 0;
}

WebDOMCString WebDOMString::utf8() const
{
    return WTF::String(m_private).utf8();
}

WebDOMString WebDOMString::fromUTF8(const char* data, size_t length)
{
    return WTF::String::fromUTF8(data, length);
}

WebDOMString WebDOMString::fromUTF8(const char* data)
{
    return WTF::String::fromUTF8(data);
}

WebDOMString::WebDOMString(const WTF::String& s)
    : m_private(static_cast<WebDOMStringPrivate*>(s.impl()))
{
    if (m_private)
        m_private->ref();
}

WebDOMString& WebDOMString::operator=(const WTF::String& s)
{
    assign(static_cast<WebDOMStringPrivate*>(s.impl()));
    return *this;
}

WebDOMString::operator WTF::String() const
{
    return m_private;
}

WebDOMString::WebDOMString(const WTF::AtomicString& s)
    : m_private(0)
{
    assign(s.string());
}

WebDOMString& WebDOMString::operator=(const WTF::AtomicString& s)
{
    assign(s.string());
    return *this;
}

WebDOMString::operator WTF::AtomicString() const
{
    return WTF::AtomicString(static_cast<WTF::StringImpl *>(m_private));
}

bool WebDOMString::equals(const char* string) const
{
    return WTF::equal(m_private, string);
}

void WebDOMString::assign(WebDOMStringPrivate* p)
{
    // Take care to handle the case where m_private == p
    if (p)
        p->ref();
    if (m_private)
        m_private->deref();
    m_private = p;
}
