/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#ifndef WebDOMTestTypedefs_h
#define WebDOMTestTypedefs_h

#include <WebDOMObject.h>
#include <WebDOMString.h>

namespace WebCore {
class TestTypedefs;
};

class WebDOMDOMString[];
class WebDOMSVGPoint;
class WebDOMString;
class WebDOMlong[];

class WebDOMTestTypedefs : public WebDOMObject {
public:
    WebDOMTestTypedefs();
    explicit WebDOMTestTypedefs(WebCore::TestTypedefs*);
    WebDOMTestTypedefs(const WebDOMTestTypedefs&);
    WebDOMTestTypedefs& operator=(const WebDOMTestTypedefs&);
    virtual ~WebDOMTestTypedefs();

    unsigned long long unsignedLongLongAttr() const;
    void setUnsignedLongLongAttr(unsigned long long);
    WebDOMString immutableSerializedScriptValue() const;
    void setImmutableSerializedScriptValue(const WebDOMString&);
    int attrWithGetterException() const;
    void setAttrWithGetterException(int);
    int attrWithSetterException() const;
    void setAttrWithSetterException(int);
    WebDOMString stringAttrWithGetterException() const;
    void setStringAttrWithGetterException(const WebDOMString&);
    WebDOMString stringAttrWithSetterException() const;
    void setStringAttrWithSetterException(const WebDOMString&);

    void func(const WebDOMlong[]& x);
    void setShadow(float width, float height, float blur, const WebDOMString& color, float alpha);
    void nullableArrayArg(const WebDOMDOMString[]& arrayArg);
    WebDOMSVGPoint immutablePointFunction();
    void methodWithException();

    WebCore::TestTypedefs* impl() const;

protected:
    struct WebDOMTestTypedefsPrivate;
    WebDOMTestTypedefsPrivate* m_impl;
};

WebCore::TestTypedefs* toWebCore(const WebDOMTestTypedefs&);
WebDOMTestTypedefs toWebKit(WebCore::TestTypedefs*);

#endif
