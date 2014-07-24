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

const ClassInfo JSString::s_info = { "string", 0, 0, 0, CREATE_METHOD_TABLE(JSString) };

void JSRopeString::RopeBuilder::expand()
{
    ASSERT(m_index == JSRopeString::s_maxInternalRopeLength);
    JSString* jsString = m_jsString;
    m_jsString = jsStringBuilder(&m_vm);
    m_index = 0;
    append(jsString);
}

void JSString::destroy(JSCell* cell)
{
    JSString* thisObject = static_cast<JSString*>(cell);
    thisObject->JSString::~JSString();
}

void JSString::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSString* thisObject = jsCast<JSString*>(cell);
    Base::visitChildren(thisObject, visitor);
    
    MARK_LOG_MESSAGE1("[%u]: ", thisObject->length());

#if ENABLE(OBJECT_MARK_LOGGING)
    if (!thisObject->isRope()) {
        WTF::StringImpl* ourImpl = thisObject->m_value.impl();
        if (ourImpl->is8Bit())
            MARK_LOG_MESSAGE1("[8 %p]", ourImpl->characters8());
        else
            MARK_LOG_MESSAGE1("[16 %p]", ourImpl->characters16());
    } else
        MARK_LOG_MESSAGE0("[rope]: ");
#endif

    if (thisObject->isRope())
        static_cast<JSRopeString*>(thisObject)->visitFibers(visitor);
}

void JSRopeString::visitFibers(SlotVisitor& visitor)
{
    for (size_t i = 0; i < s_maxInternalRopeLength && m_fibers[i]; ++i)
        visitor.append(&m_fibers[i]);
}

void JSRopeString::resolveRope(ExecState* exec) const
{
    ASSERT(isRope());

    if (is8Bit()) {
        LChar* buffer;
        if (RefPtr<StringImpl> newImpl = StringImpl::tryCreateUninitialized(m_length, buffer)) {
            Heap::heap(this)->reportExtraMemoryCost(newImpl->cost());
            m_value = newImpl.release();
        } else {
            outOfMemory(exec);
            return;
        }

        for (size_t i = 0; i < s_maxInternalRopeLength && m_fibers[i]; ++i) {
            if (m_fibers[i]->isRope())
                return resolveRopeSlowCase8(buffer);
        }

        LChar* position = buffer;
        for (size_t i = 0; i < s_maxInternalRopeLength && m_fibers[i]; ++i) {
            StringImpl* string = m_fibers[i]->m_value.impl();
            unsigned length = string->length();
            StringImpl::copyChars(position, string->characters8(), length);
            position += length;
            m_fibers[i].clear();
        }
        ASSERT((buffer + m_length) == position);
        ASSERT(!isRope());

        return;
    }

    UChar* buffer;
    if (RefPtr<StringImpl> newImpl = StringImpl::tryCreateUninitialized(m_length, buffer)) {
        Heap::heap(this)->reportExtraMemoryCost(newImpl->cost());
        m_value = newImpl.release();
    } else {
        outOfMemory(exec);
        return;
    }

    for (size_t i = 0; i < s_maxInternalRopeLength && m_fibers[i]; ++i) {
        if (m_fibers[i]->isRope())
            return resolveRopeSlowCase(buffer);
    }

    UChar* position = buffer;
    for (size_t i = 0; i < s_maxInternalRopeLength && m_fibers[i]; ++i) {
        StringImpl* string = m_fibers[i]->m_value.impl();
        unsigned length = string->length();
        if (string->is8Bit())
            StringImpl::copyChars(position, string->characters8(), length);
        else
            StringImpl::copyChars(position, string->characters16(), length);
        position += length;
        m_fibers[i].clear();
    }
    ASSERT((buffer + m_length) == position);
    ASSERT(!isRope());
}

// Overview: These functions convert a JSString from holding a string in rope form
// down to a simple String representation. It does so by building up the string
// backwards, since we want to avoid recursion, we expect that the tree structure
// representing the rope is likely imbalanced with more nodes down the left side
// (since appending to the string is likely more common) - and as such resolving
// in this fashion should minimize work queue size.  (If we built the queue forwards
// we would likely have to place all of the constituent StringImpls into the
// Vector before performing any concatenation, but by working backwards we likely
// only fill the queue with the number of substrings at any given level in a
// rope-of-ropes.)    
void JSRopeString::resolveRopeSlowCase8(LChar* buffer) const
{
    LChar* position = buffer + m_length; // We will be working backwards over the rope.
    Vector<JSString*, 32, UnsafeVectorOverflow> workQueue; // Putting strings into a Vector is only OK because there are no GC points in this method.
    
    for (size_t i = 0; i < s_maxInternalRopeLength && m_fibers[i]; ++i) {
        workQueue.append(m_fibers[i].get());
        // Clearing here works only because there are no GC points in this method.
        m_fibers[i].clear();
    }

    while (!workQueue.isEmpty()) {
        JSString* currentFiber = workQueue.last();
        workQueue.removeLast();

        if (currentFiber->isRope()) {
            JSRopeString* currentFiberAsRope = static_cast<JSRopeString*>(currentFiber);
            for (size_t i = 0; i < s_maxInternalRopeLength && currentFiberAsRope->m_fibers[i]; ++i)
                workQueue.append(currentFiberAsRope->m_fibers[i].get());
            continue;
        }

        StringImpl* string = static_cast<StringImpl*>(currentFiber->m_value.impl());
        unsigned length = string->length();
        position -= length;
        StringImpl::copyChars(position, string->characters8(), length);
    }

    ASSERT(buffer == position);
    ASSERT(!isRope());
}

void JSRopeString::resolveRopeSlowCase(UChar* buffer) const
{
    UChar* position = buffer + m_length; // We will be working backwards over the rope.
    Vector<JSString*, 32, UnsafeVectorOverflow> workQueue; // These strings are kept alive by the parent rope, so using a Vector is OK.

    for (size_t i = 0; i < s_maxInternalRopeLength && m_fibers[i]; ++i)
        workQueue.append(m_fibers[i].get());

    while (!workQueue.isEmpty()) {
        JSString* currentFiber = workQueue.last();
        workQueue.removeLast();

        if (currentFiber->isRope()) {
            JSRopeString* currentFiberAsRope = static_cast<JSRopeString*>(currentFiber);
            for (size_t i = 0; i < s_maxInternalRopeLength && currentFiberAsRope->m_fibers[i]; ++i)
                workQueue.append(currentFiberAsRope->m_fibers[i].get());
            continue;
        }

        StringImpl* string = static_cast<StringImpl*>(currentFiber->m_value.impl());
        unsigned length = string->length();
        position -= length;
        if (string->is8Bit())
            StringImpl::copyChars(position, string->characters8(), length);
        else
            StringImpl::copyChars(position, string->characters16(), length);
    }

    ASSERT(buffer == position);
    ASSERT(!isRope());
}

void JSRopeString::outOfMemory(ExecState* exec) const
{
    for (size_t i = 0; i < s_maxInternalRopeLength && m_fibers[i]; ++i)
        m_fibers[i].clear();
    ASSERT(isRope());
    ASSERT(m_value.isNull());
    if (exec)
        throwOutOfMemoryError(exec);
}

JSString* JSRopeString::getIndexSlowCase(ExecState* exec, unsigned i)
{
    ASSERT(isRope());
    resolveRope(exec);
    // Return a safe no-value result, this should never be used, since the excetion will be thrown.
    if (exec->exception())
        return jsEmptyString(exec);
    ASSERT(!isRope());
    RELEASE_ASSERT(i < m_value.length());
    return jsSingleCharacterSubstring(exec, m_value, i);
}

JSValue JSString::toPrimitive(ExecState*, PreferredPrimitiveType) const
{
    return const_cast<JSString*>(this);
}

bool JSString::getPrimitiveNumber(ExecState* exec, double& number, JSValue& result) const
{
    result = this;
    number = jsToNumber(value(exec));
    return false;
}

bool JSString::toBoolean() const
{
    return m_length;
}

double JSString::toNumber(ExecState* exec) const
{
    return jsToNumber(value(exec));
}

inline StringObject* StringObject::create(ExecState* exec, JSGlobalObject* globalObject, JSString* string)
{
    StringObject* object = new (NotNull, allocateCell<StringObject>(*exec->heap())) StringObject(exec->vm(), globalObject->stringObjectStructure());
    object->finishCreation(exec->vm(), string);
    return object;
}

JSObject* JSString::toObject(ExecState* exec, JSGlobalObject* globalObject) const
{
    return StringObject::create(exec, globalObject, const_cast<JSString*>(this));
}

JSObject* JSString::toThisObject(JSCell* cell, ExecState* exec)
{
    return StringObject::create(exec, exec->lexicalGlobalObject(), jsCast<JSString*>(cell));
}

bool JSString::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSString* thisObject = jsCast<JSString*>(cell);
    // The semantics here are really getPropertySlot, not getOwnPropertySlot.
    // This function should only be called by JSValue::get.
    if (thisObject->getStringPropertySlot(exec, propertyName, slot))
        return true;
    slot.setBase(thisObject);
    JSObject* object;
    for (JSValue prototype = exec->lexicalGlobalObject()->stringPrototype(); !prototype.isNull(); prototype = object->prototype()) {
        object = asObject(prototype);
        if (object->methodTable()->getOwnPropertySlot(object, exec, propertyName, slot))
            return true;
    }
    slot.setUndefined();
    return true;
}

bool JSString::getStringPropertyDescriptor(ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    if (propertyName == exec->propertyNames().length) {
        descriptor.setDescriptor(jsNumber(m_length), DontEnum | DontDelete | ReadOnly);
        return true;
    }
    
    unsigned i = propertyName.asIndex();
    if (i < m_length) {
        ASSERT(i != PropertyName::NotAnIndex); // No need for an explicit check, the above test would always fail!
        descriptor.setDescriptor(getIndex(exec, i), DontDelete | ReadOnly);
        return true;
    }
    
    return false;
}

bool JSString::getOwnPropertySlotByIndex(JSCell* cell, ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    JSString* thisObject = jsCast<JSString*>(cell);
    // The semantics here are really getPropertySlot, not getOwnPropertySlot.
    // This function should only be called by JSValue::get.
    if (thisObject->getStringPropertySlot(exec, propertyName, slot))
        return true;
    return JSString::getOwnPropertySlot(thisObject, exec, Identifier::from(exec, propertyName), slot);
}

} // namespace JSC
