/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */


#include "config.h"

#include "PropertyDescriptor.h"

#include "GetterSetter.h"
#include "JSObject.h"
#include "Operations.h"

namespace JSC {
unsigned PropertyDescriptor::defaultAttributes = (DontDelete << 1) - 1;

bool PropertyDescriptor::writable() const
{
    ASSERT(!isAccessorDescriptor());
    return !(m_attributes & ReadOnly);
}

bool PropertyDescriptor::enumerable() const
{
    return !(m_attributes & DontEnum);
}

bool PropertyDescriptor::configurable() const
{
    return !(m_attributes & DontDelete);
}

bool PropertyDescriptor::isDataDescriptor() const
{
    return m_value || (m_seenAttributes & WritablePresent);
}

bool PropertyDescriptor::isGenericDescriptor() const
{
    return !isAccessorDescriptor() && !isDataDescriptor();
}

bool PropertyDescriptor::isAccessorDescriptor() const
{
    return m_getter || m_setter;
}

void PropertyDescriptor::setUndefined()
{
    m_value = jsUndefined();
    m_attributes = ReadOnly | DontDelete | DontEnum;
}

JSValue PropertyDescriptor::getter() const
{
    ASSERT(isAccessorDescriptor());
    return m_getter;
}

JSValue PropertyDescriptor::setter() const
{
    ASSERT(isAccessorDescriptor());
    return m_setter;
}

void PropertyDescriptor::setDescriptor(JSValue value, unsigned attributes)
{
    ASSERT(value);
    m_attributes = attributes;
    if (attributes & (Getter | Setter)) {
        GetterSetter* accessor = asGetterSetter(value);
        m_getter = accessor->getter();
        m_setter = accessor->setter();
        ASSERT(m_getter || m_setter);
        m_seenAttributes = EnumerablePresent | ConfigurablePresent;
        m_attributes &= ~ReadOnly;
    } else {
        m_value = value;
        m_seenAttributes = EnumerablePresent | ConfigurablePresent | WritablePresent;
    }
}

void PropertyDescriptor::setAccessorDescriptor(JSValue getter, JSValue setter, unsigned attributes)
{
    ASSERT(attributes & (Getter | Setter));
    ASSERT(getter || setter);
    m_attributes = attributes;
    m_getter = getter;
    m_setter = setter;
    m_attributes &= ~ReadOnly;
    m_seenAttributes = EnumerablePresent | ConfigurablePresent;
}

void PropertyDescriptor::setWritable(bool writable)
{
    if (writable)
        m_attributes &= ~ReadOnly;
    else
        m_attributes |= ReadOnly;
    m_seenAttributes |= WritablePresent;
}

void PropertyDescriptor::setEnumerable(bool enumerable)
{
    if (enumerable)
        m_attributes &= ~DontEnum;
    else
        m_attributes |= DontEnum;
    m_seenAttributes |= EnumerablePresent;
}

void PropertyDescriptor::setConfigurable(bool configurable)
{
    if (configurable)
        m_attributes &= ~DontDelete;
    else
        m_attributes |= DontDelete;
    m_seenAttributes |= ConfigurablePresent;
}

void PropertyDescriptor::setSetter(JSValue setter)
{
    m_setter = setter;
    m_attributes |= Setter;
    m_attributes &= ~ReadOnly;
}

void PropertyDescriptor::setGetter(JSValue getter)
{
    m_getter = getter;
    m_attributes |= Getter;
    m_attributes &= ~ReadOnly;
}

bool PropertyDescriptor::equalTo(ExecState* exec, const PropertyDescriptor& other) const
{
    if (!other.m_value == m_value ||
        !other.m_getter == m_getter ||
        !other.m_setter == m_setter)
        return false;
    return (!m_value || JSValue::strictEqual(exec, other.m_value, m_value)) && 
           (!m_getter || JSValue::strictEqual(exec, other.m_getter, m_getter)) && 
           (!m_setter || JSValue::strictEqual(exec, other.m_setter, m_setter)) &&
           attributesEqual(other);
}

bool PropertyDescriptor::attributesEqual(const PropertyDescriptor& other) const
{
    unsigned mismatch = other.m_attributes ^ m_attributes;
    unsigned sharedSeen = other.m_seenAttributes & m_seenAttributes;
    if (sharedSeen & WritablePresent && mismatch & ReadOnly)
        return false;
    if (sharedSeen & ConfigurablePresent && mismatch & DontDelete)
        return false;
    if (sharedSeen & EnumerablePresent && mismatch & DontEnum)
        return false;
    return true;
}

unsigned PropertyDescriptor::attributesWithOverride(const PropertyDescriptor& other) const
{
    unsigned mismatch = other.m_attributes ^ m_attributes;
    unsigned sharedSeen = other.m_seenAttributes & m_seenAttributes;
    unsigned newAttributes = m_attributes & defaultAttributes;
    if (sharedSeen & WritablePresent && mismatch & ReadOnly)
        newAttributes ^= ReadOnly;
    if (sharedSeen & ConfigurablePresent && mismatch & DontDelete)
        newAttributes ^= DontDelete;
    if (sharedSeen & EnumerablePresent && mismatch & DontEnum)
        newAttributes ^= DontEnum;
    return newAttributes;
}

}
