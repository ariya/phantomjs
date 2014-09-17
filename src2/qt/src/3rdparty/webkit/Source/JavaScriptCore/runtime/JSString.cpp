/*
 *  Copyright (C) 1999-2002 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2004, 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "JSString.h"

#include "JSGlobalObject.h"
#include "JSGlobalObjectFunctions.h"
#include "JSObject.h"
#include "Operations.h"
#include "StringObject.h"
#include "StringPrototype.h"

namespace JSC {
    
static const unsigned substringFromRopeCutoff = 4;

const ClassInfo JSString::s_info = { "string", 0, 0, 0 };

void JSString::resolveRope(ExecState* exec) const
{
    ASSERT(isRope());

    UChar* buffer;
    if (PassRefPtr<StringImpl> newImpl = StringImpl::tryCreateUninitialized(m_length, buffer)) {
        Heap::heap(this)->reportExtraMemoryCost(newImpl->cost());
        m_value = newImpl;
    } else {
        outOfMemory(exec);
        return;
    }

    RopeImpl::Fiber currentFiber = m_fibers[0];

    if ((m_fiberCount > 2) || (RopeImpl::isRope(currentFiber)) 
        || ((m_fiberCount == 2) && (RopeImpl::isRope(m_fibers[1])))) {
        resolveRopeSlowCase(exec, buffer);
        return;
    }

    UChar* position = buffer;
    StringImpl* string = static_cast<StringImpl*>(currentFiber);
    unsigned length = string->length();
    StringImpl::copyChars(position, string->characters(), length);

    if (m_fiberCount > 1) {
        position += length;
        currentFiber = m_fibers[1];
        string = static_cast<StringImpl*>(currentFiber);
        length = string->length();
        StringImpl::copyChars(position, string->characters(), length);
        position += length;
    }

    ASSERT((buffer + m_length) == position);
    for (unsigned i = 0; i < m_fiberCount; ++i) {
        RopeImpl::deref(m_fibers[i]);
        m_fibers[i] = 0;
    }
    m_fiberCount = 0;

    ASSERT(!isRope());
}

// Overview: this methods converts a JSString from holding a string in rope form
// down to a simple UString representation.  It does so by building up the string
// backwards, since we want to avoid recursion, we expect that the tree structure
// representing the rope is likely imbalanced with more nodes down the left side
// (since appending to the string is likely more common) - and as such resolving
// in this fashion should minimize work queue size.  (If we built the queue forwards
// we would likely have to place all of the constituent StringImpls into the
// Vector before performing any concatenation, but by working backwards we likely
// only fill the queue with the number of substrings at any given level in a
// rope-of-ropes.)    
void JSString::resolveRopeSlowCase(ExecState* exec, UChar* buffer) const
{
    UNUSED_PARAM(exec);

    UChar* position = buffer + m_length;

    // Start with the current RopeImpl.
    Vector<RopeImpl::Fiber, 32> workQueue;
    RopeImpl::Fiber currentFiber;
    for (unsigned i = 0; i < (m_fiberCount - 1); ++i)
        workQueue.append(m_fibers[i]);
    currentFiber = m_fibers[m_fiberCount - 1];
    while (true) {
        if (RopeImpl::isRope(currentFiber)) {
            RopeImpl* rope = static_cast<RopeImpl*>(currentFiber);
            // Copy the contents of the current rope into the workQueue, with the last item in 'currentFiber'
            // (we will be working backwards over the rope).
            unsigned fiberCountMinusOne = rope->fiberCount() - 1;
            for (unsigned i = 0; i < fiberCountMinusOne; ++i)
                workQueue.append(rope->fibers()[i]);
            currentFiber = rope->fibers()[fiberCountMinusOne];
        } else {
            StringImpl* string = static_cast<StringImpl*>(currentFiber);
            unsigned length = string->length();
            position -= length;
            StringImpl::copyChars(position, string->characters(), length);

            // Was this the last item in the work queue?
            if (workQueue.isEmpty()) {
                // Create a string from the UChar buffer, clear the rope RefPtr.
                ASSERT(buffer == position);
                for (unsigned i = 0; i < m_fiberCount; ++i) {
                    RopeImpl::deref(m_fibers[i]);
                    m_fibers[i] = 0;
                }
                m_fiberCount = 0;
                
                ASSERT(!isRope());
                return;
            }

            // No! - set the next item up to process.
            currentFiber = workQueue.last();
            workQueue.removeLast();
        }
    }
}

void JSString::outOfMemory(ExecState* exec) const
{
    for (unsigned i = 0; i < m_fiberCount; ++i) {
        RopeImpl::deref(m_fibers[i]);
        m_fibers[i] = 0;
    }
    m_fiberCount = 0;
    ASSERT(!isRope());
    ASSERT(m_value == UString());
    if (exec)
        throwOutOfMemoryError(exec);
}

// This function construsts a substring out of a rope without flattening by reusing the existing fibers.
// This can reduce memory usage substantially. Since traversing ropes is slow the function will revert 
// back to flattening if the rope turns out to be long.
JSString* JSString::substringFromRope(ExecState* exec, unsigned substringStart, unsigned substringLength)
{
    ASSERT(isRope());
    ASSERT(substringLength);
    
    JSGlobalData* globalData = &exec->globalData();

    UString substringFibers[3];
    
    unsigned fiberCount = 0;
    unsigned substringFiberCount = 0;
    unsigned substringEnd = substringStart + substringLength;
    unsigned fiberEnd = 0;

    RopeIterator end;
    for (RopeIterator it(m_fibers.data(), m_fiberCount); it != end; ++it) {
        ++fiberCount;
        StringImpl* fiberString = *it;
        unsigned fiberStart = fiberEnd;
        fiberEnd = fiberStart + fiberString->length();
        if (fiberEnd <= substringStart)
            continue;
        unsigned copyStart = std::max(substringStart, fiberStart);
        unsigned copyEnd = std::min(substringEnd, fiberEnd);
        if (copyStart == fiberStart && copyEnd == fiberEnd)
            substringFibers[substringFiberCount++] = UString(fiberString);
        else
            substringFibers[substringFiberCount++] = UString(StringImpl::create(fiberString, copyStart - fiberStart, copyEnd - copyStart));
        if (fiberEnd >= substringEnd)
            break;
        if (fiberCount > substringFromRopeCutoff || substringFiberCount >= 3) {
            // This turned out to be a really inefficient rope. Just flatten it.
            resolveRope(exec);
            return jsSubstring(&exec->globalData(), m_value, substringStart, substringLength);
        }
    }
    ASSERT(substringFiberCount && substringFiberCount <= 3);

    if (substringLength == 1) {
        ASSERT(substringFiberCount == 1);
        UChar c = substringFibers[0].characters()[0];
        if (c <= maxSingleCharacterString)
            return globalData->smallStrings.singleCharacterString(globalData, c);
    }
    if (substringFiberCount == 1)
        return new (globalData) JSString(globalData, substringFibers[0]);
    if (substringFiberCount == 2)
        return new (globalData) JSString(globalData, substringFibers[0], substringFibers[1]);
    return new (globalData) JSString(globalData, substringFibers[0], substringFibers[1], substringFibers[2]);
}

JSValue JSString::replaceCharacter(ExecState* exec, UChar character, const UString& replacement)
{
    if (!isRope()) {
        size_t matchPosition = m_value.find(character);
        if (matchPosition == notFound)
            return JSValue(this);
        return jsString(exec, m_value.substringSharingImpl(0, matchPosition), replacement, m_value.substringSharingImpl(matchPosition + 1));
    }

    RopeIterator end;
    
    // Count total fibers and find matching string.
    size_t fiberCount = 0;
    StringImpl* matchString = 0;
    size_t matchPosition = notFound;
    for (RopeIterator it(m_fibers.data(), m_fiberCount); it != end; ++it) {
        ++fiberCount;
        if (matchString)
            continue;

        StringImpl* string = *it;
        matchPosition = string->find(character);
        if (matchPosition == notFound)
            continue;
        matchString = string;
    }

    if (!matchString)
        return this;

    RopeBuilder builder(replacement.length() ? fiberCount + 2 : fiberCount + 1);
    if (UNLIKELY(builder.isOutOfMemory()))
        return throwOutOfMemoryError(exec);

    for (RopeIterator it(m_fibers.data(), m_fiberCount); it != end; ++it) {
        StringImpl* string = *it;
        if (string != matchString) {
            builder.append(UString(string));
            continue;
        }

        builder.append(UString(string).substringSharingImpl(0, matchPosition));
        if (replacement.length())
            builder.append(replacement);
        builder.append(UString(string).substringSharingImpl(matchPosition + 1));
        matchString = 0;
    }

    JSGlobalData* globalData = &exec->globalData();
    return JSValue(new (globalData) JSString(globalData, builder.release()));
}

JSString* JSString::getIndexSlowCase(ExecState* exec, unsigned i)
{
    ASSERT(isRope());
    resolveRope(exec);
    // Return a safe no-value result, this should never be used, since the excetion will be thrown.
    if (exec->exception())
        return jsString(exec, "");
    ASSERT(!isRope());
    ASSERT(i < m_value.length());
    return jsSingleCharacterSubstring(exec, m_value, i);
}

JSValue JSString::toPrimitive(ExecState*, PreferredPrimitiveType) const
{
    return const_cast<JSString*>(this);
}

bool JSString::getPrimitiveNumber(ExecState* exec, double& number, JSValue& result)
{
    result = this;
    number = jsToNumber(value(exec));
    return false;
}

bool JSString::toBoolean(ExecState*) const
{
    return m_length;
}

double JSString::toNumber(ExecState* exec) const
{
    return jsToNumber(value(exec));
}

UString JSString::toString(ExecState* exec) const
{
    return value(exec);
}

inline StringObject* StringObject::create(ExecState* exec, JSGlobalObject* globalObject, JSString* string)
{
    return new (exec) StringObject(exec->globalData(), globalObject->stringObjectStructure(), string);
}

JSObject* JSString::toObject(ExecState* exec, JSGlobalObject* globalObject) const
{
    return StringObject::create(exec, globalObject, const_cast<JSString*>(this));
}

JSObject* JSString::toThisObject(ExecState* exec) const
{
    return StringObject::create(exec, exec->lexicalGlobalObject(), const_cast<JSString*>(this));
}

bool JSString::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    // The semantics here are really getPropertySlot, not getOwnPropertySlot.
    // This function should only be called by JSValue::get.
    if (getStringPropertySlot(exec, propertyName, slot))
        return true;
    if (propertyName == exec->propertyNames().underscoreProto) {
        slot.setValue(exec->lexicalGlobalObject()->stringPrototype());
        return true;
    }
    slot.setBase(this);
    JSObject* object;
    for (JSValue prototype = exec->lexicalGlobalObject()->stringPrototype(); !prototype.isNull(); prototype = object->prototype()) {
        object = asObject(prototype);
        if (object->getOwnPropertySlot(exec, propertyName, slot))
            return true;
    }
    slot.setUndefined();
    return true;
}

bool JSString::getStringPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    if (propertyName == exec->propertyNames().length) {
        descriptor.setDescriptor(jsNumber(m_length), DontEnum | DontDelete | ReadOnly);
        return true;
    }
    
    bool isStrictUInt32;
    unsigned i = propertyName.toUInt32(isStrictUInt32);
    if (isStrictUInt32 && i < m_length) {
        descriptor.setDescriptor(getIndex(exec, i), DontDelete | ReadOnly);
        return true;
    }
    
    return false;
}

bool JSString::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    if (getStringPropertyDescriptor(exec, propertyName, descriptor))
        return true;
    if (propertyName != exec->propertyNames().underscoreProto)
        return false;
    descriptor.setDescriptor(exec->lexicalGlobalObject()->stringPrototype(), DontEnum);
    return true;
}

bool JSString::getOwnPropertySlot(ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    // The semantics here are really getPropertySlot, not getOwnPropertySlot.
    // This function should only be called by JSValue::get.
    if (getStringPropertySlot(exec, propertyName, slot))
        return true;
    return JSString::getOwnPropertySlot(exec, Identifier::from(exec, propertyName), slot);
}

} // namespace JSC
