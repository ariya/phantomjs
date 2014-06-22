/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "JavaScriptVariant.h"

#include "JavaScriptVariant_p.h"
#include "WebPage.h"
#include <JSStringRef.h>
#include <JSValueRef.h>
#include <stdlib.h>
#include <wtf/Vector.h>

namespace BlackBerry {
namespace WebKit {

JavaScriptVariant JSValueRefToBlackBerryJavaScriptVariant(const JSGlobalContextRef& ctx, const JSValueRef& value)
{
    JavaScriptVariant returnValue;

    switch (JSValueGetType(ctx, value)) {
    case kJSTypeNull:
        returnValue.setType(JavaScriptVariant::Null);
        break;
    case kJSTypeBoolean:
        returnValue.setBoolean(JSValueToBoolean(ctx, value));
        break;
    case kJSTypeNumber:
        returnValue.setDouble(JSValueToNumber(ctx, value, 0));
        break;
    case kJSTypeString: {
        JSStringRef stringRef = JSValueToStringCopy(ctx, value, 0);
        if (!stringRef) {
            returnValue.setString(BlackBerry::Platform::String::emptyString());
            break;
        }
        size_t bufferSize = JSStringGetMaximumUTF8CStringSize(stringRef);
        WTF::Vector<char> buffer(bufferSize);
        size_t rc = JSStringGetUTF8CString(stringRef, buffer.data(), bufferSize);
        returnValue.setString(BlackBerry::Platform::String::fromUtf8(buffer.data(), rc - 1));
        JSStringRelease(stringRef);
        break;
    }
    case kJSTypeObject:
        returnValue.setType(JavaScriptVariant::Object);
        break;
    case kJSTypeUndefined:
        returnValue.setType(JavaScriptVariant::Undefined);
        break;
    }
    return returnValue;
}

JSValueRef BlackBerryJavaScriptVariantToJSValueRef(const JSGlobalContextRef& ctx, const JavaScriptVariant& variant)
{
    JSValueRef ref = 0;
    switch (variant.type()) {
    case JavaScriptVariant::Undefined:
        ref = JSValueMakeUndefined(ctx);
        break;
    case JavaScriptVariant::Null:
        ref = JSValueMakeNull(ctx);
        break;
    case JavaScriptVariant::Boolean:
        ref = JSValueMakeBoolean(ctx, variant.booleanValue());
        break;
    case JavaScriptVariant::Number:
        ref = JSValueMakeNumber(ctx, variant.doubleValue());
        break;
    case JavaScriptVariant::String: {
        JSStringRef str = JSStringCreateWithUTF8CString(variant.stringValue().c_str());
        ref = JSValueMakeString(ctx, str);
        JSStringRelease(str);
        break;
    }
    case JavaScriptVariant::Exception:
    case JavaScriptVariant::Object:
        ASSERT_NOT_REACHED();
        break;
    }
    return ref;
}

JavaScriptVariant::JavaScriptVariant()
    : m_type(Undefined)
{
}

JavaScriptVariant::JavaScriptVariant(double value)
    : m_type(Undefined)
{
    setDouble(value);
}

JavaScriptVariant::JavaScriptVariant(int value)
    : m_type(Undefined)
{
    setDouble(value);
}

JavaScriptVariant::JavaScriptVariant(const BlackBerry::Platform::String& value)
    : m_type(Undefined)
{
    setString(value);
}

JavaScriptVariant::JavaScriptVariant(bool value)
    : m_type(Undefined)
{
    setBoolean(value);
}

JavaScriptVariant::JavaScriptVariant(const JavaScriptVariant &v)
    : m_type(Undefined)
{
    this->operator=(v);
}

JavaScriptVariant::~JavaScriptVariant()
{
}

JavaScriptVariant& JavaScriptVariant::operator=(const JavaScriptVariant& v)
{
    if (&v == this)
        return *this;

    switch (v.type()) {
    case Boolean:
        setBoolean(v.booleanValue());
        break;
    case Number:
        setDouble(v.doubleValue());
        break;
    case String:
        setString(v.stringValue());
        break;
    default:
        setType(v.type());
        break;
    }

    return *this;
}

void JavaScriptVariant::setType(const DataType& type)
{
    if (m_type == String)
        m_stringValue = BlackBerry::Platform::String::emptyString();
    m_type = type;
}

JavaScriptVariant::DataType JavaScriptVariant::type() const
{
    return m_type;
}

void JavaScriptVariant::setDouble(double value)
{
    setType(Number);
    m_doubleValue = value;
}

double JavaScriptVariant::doubleValue() const
{
    return m_doubleValue;
}

void JavaScriptVariant::setString(const BlackBerry::Platform::String& value)
{
    setType(String);
    m_stringValue = value;
}

const BlackBerry::Platform::String& JavaScriptVariant::stringValue() const
{
    return m_stringValue;
}

void JavaScriptVariant::setBoolean(bool value)
{
    setType(Boolean);
    m_booleanValue = value;
}

bool JavaScriptVariant::booleanValue() const
{
    return m_booleanValue;
}

}
}
