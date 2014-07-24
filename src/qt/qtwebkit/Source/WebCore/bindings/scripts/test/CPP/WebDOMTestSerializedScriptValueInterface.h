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

#ifndef WebDOMTestSerializedScriptValueInterface_h
#define WebDOMTestSerializedScriptValueInterface_h

#if ENABLE(Condition1) || ENABLE(Condition2)

#include <WebDOMObject.h>
#include <WebDOMString.h>

namespace WebCore {
class TestSerializedScriptValueInterface;
};

class WebDOMString;

class WebDOMTestSerializedScriptValueInterface : public WebDOMObject {
public:
    WebDOMTestSerializedScriptValueInterface();
    explicit WebDOMTestSerializedScriptValueInterface(WebCore::TestSerializedScriptValueInterface*);
    WebDOMTestSerializedScriptValueInterface(const WebDOMTestSerializedScriptValueInterface&);
    WebDOMTestSerializedScriptValueInterface& operator=(const WebDOMTestSerializedScriptValueInterface&);
    virtual ~WebDOMTestSerializedScriptValueInterface();

    WebDOMString value() const;
    void setValue(const WebDOMString&);
    WebDOMString readonlyValue() const;
    WebDOMString cachedValue() const;
    void setCachedValue(const WebDOMString&);
    WebDOMString cachedReadonlyValue() const;

    WebCore::TestSerializedScriptValueInterface* impl() const;

protected:
    struct WebDOMTestSerializedScriptValueInterfacePrivate;
    WebDOMTestSerializedScriptValueInterfacePrivate* m_impl;
};

WebCore::TestSerializedScriptValueInterface* toWebCore(const WebDOMTestSerializedScriptValueInterface&);
WebDOMTestSerializedScriptValueInterface toWebKit(WebCore::TestSerializedScriptValueInterface*);

#endif
#endif // ENABLE(Condition1) || ENABLE(Condition2)

