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

#ifndef WebDOMCString_h
#define WebDOMCString_h

#include <WebDOMObject.h>
#include <wtf/Forward.h>
#include <stddef.h> // For size_t

// UTF-16 character type
#if defined(WIN32)
typedef wchar_t WebUChar;
#else
typedef unsigned short WebUChar;
#endif

class WebDOMCStringPrivate;
class WebDOMString;

// A single-byte string container with unspecified encoding.  It is
// inexpensive to copy a WebDOMCString object.
//
// WARNING: It is not safe to pass a WebDOMCString across threads!!!
//
class WebDOMCString {
public:
    ~WebDOMCString() { reset(); }

    WebDOMCString() : m_private(0) { }

    WebDOMCString(const char* data, size_t len) : m_private(0)
    {
        assign(data, len);
    }

    WebDOMCString(const WebDOMCString& s) : m_private(0) { assign(s); }

    WebDOMCString& operator=(const WebDOMCString& s)
    {
        assign(s);
        return *this;
    }

    void reset();
    void assign(const WebDOMCString&);
    void assign(const char* data, size_t len);

    size_t length() const;
    const char* data() const;

    bool isEmpty() const { return !length(); }
    bool isNull() const { return !m_private; }

    WebDOMString utf16() const;

    static WebDOMCString fromUTF16(const WebUChar* data, size_t length);
    static WebDOMCString fromUTF16(const WebUChar* data);

    WebDOMCString(const WTF::CString&);
    WebDOMCString& operator=(const WTF::CString&);
    operator WTF::CString() const;

private:
    void assign(WebDOMCStringPrivate*);
    WebDOMCStringPrivate* m_private;
};

#endif
