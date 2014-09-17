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

#ifndef WebDOMTestCallback_h
#define WebDOMTestCallback_h

#if ENABLE(DATABASE)

#include <WebDOMObject.h>
#include <WebDOMString.h>

namespace WebCore {
class TestCallback;
};

class WebDOMClass1;
class WebDOMClass2;
class WebDOMClass3;
class WebDOMClass5;
class WebDOMClass6;
class WebDOMDOMStringList;

class WebDOMTestCallback : public WebDOMObject {
public:
    WebDOMTestCallback();
    explicit WebDOMTestCallback(WebCore::TestCallback*);
    WebDOMTestCallback(const WebDOMTestCallback&);
    WebDOMTestCallback& operator=(const WebDOMTestCallback&);
    virtual ~WebDOMTestCallback();

    bool callbackWithNoParam();
    bool callbackWithClass1Param(const WebDOMClass1& class1Param);
    bool callbackWithClass2Param(const WebDOMClass2& class2Param, const WebDOMString& strArg);
    int callbackWithNonBoolReturnType(const WebDOMClass3& class3Param);
    int customCallback(const WebDOMClass5& class5Param, const WebDOMClass6& class6Param);
    bool callbackWithStringList(const WebDOMDOMStringList& listParam);

    WebCore::TestCallback* impl() const;

protected:
    struct WebDOMTestCallbackPrivate;
    WebDOMTestCallbackPrivate* m_impl;
};

WebCore::TestCallback* toWebCore(const WebDOMTestCallback&);
WebDOMTestCallback toWebKit(WebCore::TestCallback*);

#endif
#endif // ENABLE(DATABASE)

