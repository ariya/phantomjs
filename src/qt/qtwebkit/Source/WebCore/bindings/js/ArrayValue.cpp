/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ArrayValue.h"

#include "Dictionary.h"
#include <runtime/JSArray.h>

using namespace JSC;

namespace WebCore {

ArrayValue::ArrayValue()
    : m_exec(0)
{
}

ArrayValue::ArrayValue(JSC::ExecState* exec, JSC::JSValue value)
    : m_exec(exec)
{
    if (!value.isUndefinedOrNull() && isJSArray(value))
        m_value = value;
}

ArrayValue& ArrayValue::operator=(const ArrayValue& other)
{
    m_exec = other.m_exec;
    m_value = other.m_value;
    return *this;
}

bool ArrayValue::isUndefinedOrNull() const
{
    return m_value.isEmpty() || m_value.isUndefinedOrNull();
}

bool ArrayValue::length(size_t& length) const
{
    if (isUndefinedOrNull())
        return false;

    JSArray* array = asArray(m_value);
    length = array->length();
    return true;
}

bool ArrayValue::get(size_t index, Dictionary& value) const
{
    if (isUndefinedOrNull())
        return false;

    JSValue indexedValue = asArray(m_value)->getIndex(m_exec, index);
    if (indexedValue.isUndefinedOrNull() || !indexedValue.isObject())
        return false;

    value = Dictionary(m_exec, indexedValue);
    return true;
}

} // namespace WebCore
