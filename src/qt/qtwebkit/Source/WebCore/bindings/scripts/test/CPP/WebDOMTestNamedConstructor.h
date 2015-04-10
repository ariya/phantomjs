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

#ifndef WebDOMTestNamedConstructor_h
#define WebDOMTestNamedConstructor_h

#include <WebDOMObject.h>
#include <WebDOMString.h>

namespace WebCore {
class TestNamedConstructor;
};


class WebDOMTestNamedConstructor : public WebDOMObject {
public:
    WebDOMTestNamedConstructor();
    explicit WebDOMTestNamedConstructor(WebCore::TestNamedConstructor*);
    WebDOMTestNamedConstructor(const WebDOMTestNamedConstructor&);
    WebDOMTestNamedConstructor& operator=(const WebDOMTestNamedConstructor&);
    virtual ~WebDOMTestNamedConstructor();


    WebCore::TestNamedConstructor* impl() const;

protected:
    struct WebDOMTestNamedConstructorPrivate;
    WebDOMTestNamedConstructorPrivate* m_impl;
};

WebCore::TestNamedConstructor* toWebCore(const WebDOMTestNamedConstructor&);
WebDOMTestNamedConstructor toWebKit(WebCore::TestNamedConstructor*);

#endif
