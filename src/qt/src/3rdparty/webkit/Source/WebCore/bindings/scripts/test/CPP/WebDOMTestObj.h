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

#ifndef WebDOMTestObj_h
#define WebDOMTestObj_h

#include <WebDOMObject.h>
#include <WebDOMString.h>

namespace WebCore {
class TestObj;
};

class WebDOMEventListener;
class WebDOMIDBKey;
class WebDOMOptionsObject;
class WebDOMString;
class WebDOMTestObj;

class WebDOMTestObj : public WebDOMObject {
public:
    WebDOMTestObj();
    explicit WebDOMTestObj(WebCore::TestObj*);
    WebDOMTestObj(const WebDOMTestObj&);
    WebDOMTestObj& operator=(const WebDOMTestObj&);
    virtual ~WebDOMTestObj();

    enum {
        WEBDOM_CONST_VALUE_0 = 0,
        WEBDOM_CONST_VALUE_1 = 1,
        WEBDOM_CONST_VALUE_2 = 2,
        WEBDOM_CONST_VALUE_4 = 4,
        WEBDOM_CONST_VALUE_8 = 8,
        WEBDOM_CONST_VALUE_9 = -1,
        WEBDOM_CONST_VALUE_10 = "my constant string",
        WEBDOM_CONST_VALUE_11 = 0xffffffff,
        WEBDOM_CONST_VALUE_12 = 0x01,
        WEBDOM_CONST_VALUE_13 = 0X20,
        WEBDOM_CONST_VALUE_14 = 0x1abc
    };

    int readOnlyIntAttr() const;
    WebDOMString readOnlyStringAttr() const;
    WebDOMTestObj readOnlyTestObjAttr() const;
    short shortAttr() const;
    void setShortAttr(short);
    unsigned short unsignedShortAttr() const;
    void setUnsignedShortAttr(unsigned short);
    int intAttr() const;
    void setIntAttr(int);
    long long longLongAttr() const;
    void setLongLongAttr(long long);
    unsigned long long unsignedLongLongAttr() const;
    void setUnsignedLongLongAttr(unsigned long long);
    WebDOMString stringAttr() const;
    void setStringAttr(const WebDOMString&);
    WebDOMTestObj testObjAttr() const;
    void setTestObjAttr(const WebDOMTestObj&);
    WebDOMTestObj XMLObjAttr() const;
    void setXMLObjAttr(const WebDOMTestObj&);
    bool create() const;
    void setCreate(bool);
    WebDOMString reflectedStringAttr() const;
    void setReflectedStringAttr(const WebDOMString&);
    int reflectedIntegralAttr() const;
    void setReflectedIntegralAttr(int);
    unsigned reflectedUnsignedIntegralAttr() const;
    void setReflectedUnsignedIntegralAttr(unsigned);
    bool reflectedBooleanAttr() const;
    void setReflectedBooleanAttr(bool);
    WebDOMString reflectedURLAttr() const;
    void setReflectedURLAttr(const WebDOMString&);
    WebDOMString reflectedNonEmptyURLAttr() const;
    void setReflectedNonEmptyURLAttr(const WebDOMString&);
    WebDOMString reflectedStringAttr() const;
    void setReflectedStringAttr(const WebDOMString&);
    int reflectedCustomIntegralAttr() const;
    void setReflectedCustomIntegralAttr(int);
    bool reflectedCustomBooleanAttr() const;
    void setReflectedCustomBooleanAttr(bool);
    WebDOMString reflectedCustomURLAttr() const;
    void setReflectedCustomURLAttr(const WebDOMString&);
    WebDOMString reflectedCustomNonEmptyURLAttr() const;
    void setReflectedCustomNonEmptyURLAttr(const WebDOMString&);
    int attrWithGetterException() const;
    void setAttrWithGetterException(int);
    int attrWithSetterException() const;
    void setAttrWithSetterException(int);
    WebDOMString stringAttrWithGetterException() const;
    void setStringAttrWithGetterException(const WebDOMString&);
    WebDOMString stringAttrWithSetterException() const;
    void setStringAttrWithSetterException(const WebDOMString&);
    int customAttr() const;
    void setCustomAttr(int);
    WebDOMString scriptStringAttr() const;
#if ENABLE(Condition1)
    int conditionalAttr1() const;
    void setConditionalAttr1(int);
#endif
#if ENABLE(Condition1) && ENABLE(Condition2)
    int conditionalAttr2() const;
    void setConditionalAttr2(int);
#endif
#if ENABLE(Condition1) || ENABLE(Condition2)
    int conditionalAttr3() const;
    void setConditionalAttr3(int);
#endif
    int description() const;
    int id() const;
    void setId(int);
    WebDOMString hash() const;

    void voidMethod();
    void voidMethodWithArgs(int intArg, const WebDOMString& strArg, const WebDOMTestObj& objArg);
    int intMethod();
    int intMethodWithArgs(int intArg, const WebDOMString& strArg, const WebDOMTestObj& objArg);
    WebDOMTestObj objMethod();
    WebDOMTestObj objMethodWithArgs(int intArg, const WebDOMString& strArg, const WebDOMTestObj& objArg);
    WebDOMTestObj methodThatRequiresAllArgs(const WebDOMString& strArg, const WebDOMTestObj& objArg);
    WebDOMTestObj methodThatRequiresAllArgsAndThrows(const WebDOMString& strArg, const WebDOMTestObj& objArg);
    void serializedValue(const WebDOMString& serializedArg);
    void idbKey(const WebDOMIDBKey& key);
    void optionsObject(const WebDOMOptionsObject& oo, const WebDOMOptionsObject& ooo);
    void methodWithException();
    void customMethod();
    void customMethodWithArgs(int intArg, const WebDOMString& strArg, const WebDOMTestObj& objArg);
    void addEventListener(const WebDOMString& type, const WebDOMEventListener& listener, bool useCapture);
    void removeEventListener(const WebDOMString& type, const WebDOMEventListener& listener, bool useCapture);
    void withDynamicFrame();
    void withDynamicFrameAndArg(int intArg);
    void withDynamicFrameAndOptionalArg(int intArg, int optionalArg);
    void withDynamicFrameAndUserGesture(int intArg);
    void withDynamicFrameAndUserGestureASAD(int intArg, int optionalArg);
    void withScriptStateVoid();
    WebDOMTestObj withScriptStateObj();
    void withScriptStateVoidException();
    WebDOMTestObj withScriptStateObjException();
    void withScriptExecutionContext();
    void methodWithOptionalArg(int opt);
    void methodWithNonOptionalArgAndOptionalArg(int nonOpt, int opt);
    void methodWithNonOptionalArgAndTwoOptionalArgs(int nonOpt, int opt1, int opt2);
    void classMethod();
    int classMethodWithOptional(int arg);

    WebCore::TestObj* impl() const;

protected:
    struct WebDOMTestObjPrivate;
    WebDOMTestObjPrivate* m_impl;
};

WebCore::TestObj* toWebCore(const WebDOMTestObj&);
WebDOMTestObj toWebKit(WebCore::TestObj*);

#endif
