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

#ifndef WebDOMString_h
#define WebDOMString_h

#include <WebDOMCString.h>
#include <wtf/Forward.h>

class WebDOMStringPrivate;

// A UTF-16 string container.  It is inexpensive to copy a WebDOMString
// object.
//
// WARNING: It is not safe to pass a WebDOMString across threads!!!
//
class WebDOMString {
public:
    ~WebDOMString() { reset(); }

    WebDOMString() : m_private(0) { }

    WebDOMString(const WebUChar* data, size_t len) : m_private(0)
    {
        assign(data, len);
    }

    WebDOMString(const WebDOMString& s) : m_private(0) { assign(s); }

    WebDOMString& operator=(const WebDOMString& s)
    {
        assign(s);
        return *this;
    }

    void reset();
    void assign(const WebDOMString&);
    void assign(const WebUChar* data, size_t len);

    size_t length() const;
    const WebUChar* data() const;

    bool isEmpty() const { return !length(); }
    bool isNull() const { return !m_private; }

    WebDOMCString utf8() const;

    static WebDOMString fromUTF8(const char* data, size_t length);
    static WebDOMString fromUTF8(const char* data);

    template <int N> WebDOMString(const char (&data)[N])
        : m_private(0)
    {
        assign(fromUTF8(data, N - 1));
    }

    template <int N> WebDOMString& operator=(const char (&data)[N])
    {
        assign(fromUTF8(data, N - 1));
        return *this;
    }

    WebDOMString(const WTF::String&);
    WebDOMString& operator=(const WTF::String&);
    operator WTF::String() const;

    WebDOMString(const WTF::AtomicString&);
    WebDOMString& operator=(const WTF::AtomicString&);
    operator WTF::AtomicString() const;

    bool equals(const char* string) const;

private:
    void assign(WebDOMStringPrivate*);
    WebDOMStringPrivate* m_private;
};

#endif
