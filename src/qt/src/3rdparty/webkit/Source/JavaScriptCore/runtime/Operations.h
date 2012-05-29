/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2002, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef Operations_h
#define Operations_h

#include "ExceptionHelpers.h"
#include "Interpreter.h"
#include "JSString.h"
#include "JSValueInlineMethods.h"

namespace JSC {

    NEVER_INLINE JSValue jsAddSlowCase(CallFrame*, JSValue, JSValue);
    JSValue jsTypeStringForValue(CallFrame*, JSValue);
    bool jsIsObjectType(JSValue);
    bool jsIsFunctionType(JSValue);

    ALWAYS_INLINE JSValue jsString(ExecState* exec, JSString* s1, JSString* s2)
    {
        unsigned length1 = s1->length();
        if (!length1)
            return s2;
        unsigned length2 = s2->length();
        if (!length2)
            return s1;
        if ((length1 + length2) < length1)
            return throwOutOfMemoryError(exec);

        unsigned fiberCount = s1->fiberCount() + s2->fiberCount();
        JSGlobalData* globalData = &exec->globalData();

        if (fiberCount <= JSString::s_maxInternalRopeLength)
            return new (globalData) JSString(globalData, fiberCount, s1, s2);

        JSString::RopeBuilder ropeBuilder(fiberCount);
        if (UNLIKELY(ropeBuilder.isOutOfMemory()))
            return throwOutOfMemoryError(exec);
        ropeBuilder.append(s1);
        ropeBuilder.append(s2);
        return new (globalData) JSString(globalData, ropeBuilder.release());
    }

    ALWAYS_INLINE JSValue jsString(ExecState* exec, const UString& u1, JSString* s2)
    {
        unsigned length1 = u1.length();
        if (!length1)
            return s2;
        unsigned length2 = s2->length();
        if (!length2)
            return jsString(exec, u1);
        if ((length1 + length2) < length1)
            return throwOutOfMemoryError(exec);

        unsigned fiberCount = 1 + s2->fiberCount();
        JSGlobalData* globalData = &exec->globalData();

        if (fiberCount <= JSString::s_maxInternalRopeLength)
            return new (globalData) JSString(globalData, fiberCount, u1, s2);

        JSString::RopeBuilder ropeBuilder(fiberCount);
        if (UNLIKELY(ropeBuilder.isOutOfMemory()))
            return throwOutOfMemoryError(exec);
        ropeBuilder.append(u1);
        ropeBuilder.append(s2);
        return new (globalData) JSString(globalData, ropeBuilder.release());
    }

    ALWAYS_INLINE JSValue jsString(ExecState* exec, JSString* s1, const UString& u2)
    {
        unsigned length1 = s1->length();
        if (!length1)
            return jsString(exec, u2);
        unsigned length2 = u2.length();
        if (!length2)
            return s1;
        if ((length1 + length2) < length1)
            return throwOutOfMemoryError(exec);

        unsigned fiberCount = s1->fiberCount() + 1;
        JSGlobalData* globalData = &exec->globalData();

        if (fiberCount <= JSString::s_maxInternalRopeLength)
            return new (globalData) JSString(globalData, fiberCount, s1, u2);

        JSString::RopeBuilder ropeBuilder(fiberCount);
        if (UNLIKELY(ropeBuilder.isOutOfMemory()))
            return throwOutOfMemoryError(exec);
        ropeBuilder.append(s1);
        ropeBuilder.append(u2);
        return new (globalData) JSString(globalData, ropeBuilder.release());
    }

    ALWAYS_INLINE JSValue jsString(ExecState* exec, const UString& u1, const UString& u2)
    {
        unsigned length1 = u1.length();
        if (!length1)
            return jsString(exec, u2);
        unsigned length2 = u2.length();
        if (!length2)
            return jsString(exec, u1);
        if ((length1 + length2) < length1)
            return throwOutOfMemoryError(exec);

        JSGlobalData* globalData = &exec->globalData();
        return new (globalData) JSString(globalData, u1, u2);
    }

    ALWAYS_INLINE JSValue jsString(ExecState* exec, const UString& u1, const UString& u2, const UString& u3)
    {
        unsigned length1 = u1.length();
        unsigned length2 = u2.length();
        unsigned length3 = u3.length();
        if (!length1)
            return jsString(exec, u2, u3);
        if (!length2)
            return jsString(exec, u1, u3);
        if (!length3)
            return jsString(exec, u1, u2);

        if ((length1 + length2) < length1)
            return throwOutOfMemoryError(exec);
        if ((length1 + length2 + length3) < length3)
            return throwOutOfMemoryError(exec);

        JSGlobalData* globalData = &exec->globalData();
        return new (globalData) JSString(globalData, u1, u2, u3);
    }

    ALWAYS_INLINE JSValue jsString(ExecState* exec, Register* strings, unsigned count)
    {
        ASSERT(count >= 3);

        unsigned fiberCount = 0;
        for (unsigned i = 0; i < count; ++i) {
            JSValue v = strings[i].jsValue();
            if (LIKELY(v.isString()))
                fiberCount += asString(v)->fiberCount();
            else
                ++fiberCount;
        }

        JSGlobalData* globalData = &exec->globalData();
        if (fiberCount == 3)
            return new (globalData) JSString(exec, strings[0].jsValue(), strings[1].jsValue(), strings[2].jsValue());

        JSString::RopeBuilder ropeBuilder(fiberCount);
        if (UNLIKELY(ropeBuilder.isOutOfMemory()))
            return throwOutOfMemoryError(exec);

        unsigned length = 0;
        bool overflow = false;

        for (unsigned i = 0; i < count; ++i) {
            JSValue v = strings[i].jsValue();
            if (LIKELY(v.isString()))
                ropeBuilder.append(asString(v));
            else
                ropeBuilder.append(v.toString(exec));

            unsigned newLength = ropeBuilder.length();
            if (newLength < length)
                overflow = true;
            length = newLength;
        }

        if (overflow)
            return throwOutOfMemoryError(exec);

        return new (globalData) JSString(globalData, ropeBuilder.release());
    }

    ALWAYS_INLINE JSValue jsString(ExecState* exec, JSValue thisValue)
    {
        unsigned fiberCount = 0;
        if (LIKELY(thisValue.isString()))
            fiberCount += asString(thisValue)->fiberCount();
        else
            ++fiberCount;
        for (unsigned i = 0; i < exec->argumentCount(); ++i) {
            JSValue v = exec->argument(i);
            if (LIKELY(v.isString()))
                fiberCount += asString(v)->fiberCount();
            else
                ++fiberCount;
        }

        JSString::RopeBuilder ropeBuilder(fiberCount);
        if (UNLIKELY(ropeBuilder.isOutOfMemory()))
            return throwOutOfMemoryError(exec);

        if (LIKELY(thisValue.isString()))
            ropeBuilder.append(asString(thisValue));
        else
            ropeBuilder.append(thisValue.toString(exec));

        unsigned length = 0;
        bool overflow = false;

        for (unsigned i = 0; i < exec->argumentCount(); ++i) {
            JSValue v = exec->argument(i);
            if (LIKELY(v.isString()))
                ropeBuilder.append(asString(v));
            else
                ropeBuilder.append(v.toString(exec));

            unsigned newLength = ropeBuilder.length();
            if (newLength < length)
                overflow = true;
            length = newLength;
        }

        if (overflow)
            return throwOutOfMemoryError(exec);

        JSGlobalData* globalData = &exec->globalData();
        return new (globalData) JSString(globalData, ropeBuilder.release());
    }

    // ECMA 11.9.3
    inline bool JSValue::equal(ExecState* exec, JSValue v1, JSValue v2)
    {
        if (v1.isInt32() && v2.isInt32())
            return v1 == v2;

        return equalSlowCase(exec, v1, v2);
    }

    ALWAYS_INLINE bool JSValue::equalSlowCaseInline(ExecState* exec, JSValue v1, JSValue v2)
    {
        do {
            if (v1.isNumber() && v2.isNumber())
                return v1.uncheckedGetNumber() == v2.uncheckedGetNumber();

            bool s1 = v1.isString();
            bool s2 = v2.isString();
            if (s1 && s2)
                return asString(v1)->value(exec) == asString(v2)->value(exec);

            if (v1.isUndefinedOrNull()) {
                if (v2.isUndefinedOrNull())
                    return true;
                if (!v2.isCell())
                    return false;
                return v2.asCell()->structure()->typeInfo().masqueradesAsUndefined();
            }

            if (v2.isUndefinedOrNull()) {
                if (!v1.isCell())
                    return false;
                return v1.asCell()->structure()->typeInfo().masqueradesAsUndefined();
            }

            if (v1.isObject()) {
                if (v2.isObject())
                    return v1 == v2;
                JSValue p1 = v1.toPrimitive(exec);
                if (exec->hadException())
                    return false;
                v1 = p1;
                if (v1.isInt32() && v2.isInt32())
                    return v1 == v2;
                continue;
            }

            if (v2.isObject()) {
                JSValue p2 = v2.toPrimitive(exec);
                if (exec->hadException())
                    return false;
                v2 = p2;
                if (v1.isInt32() && v2.isInt32())
                    return v1 == v2;
                continue;
            }

            if (s1 || s2) {
                double d1 = v1.toNumber(exec);
                double d2 = v2.toNumber(exec);
                return d1 == d2;
            }

            if (v1.isBoolean()) {
                if (v2.isNumber())
                    return static_cast<double>(v1.getBoolean()) == v2.uncheckedGetNumber();
            } else if (v2.isBoolean()) {
                if (v1.isNumber())
                    return v1.uncheckedGetNumber() == static_cast<double>(v2.getBoolean());
            }

            return v1 == v2;
        } while (true);
    }

    // ECMA 11.9.3
    ALWAYS_INLINE bool JSValue::strictEqualSlowCaseInline(ExecState* exec, JSValue v1, JSValue v2)
    {
        ASSERT(v1.isCell() && v2.isCell());

        if (v1.asCell()->isString() && v2.asCell()->isString())
            return asString(v1)->value(exec) == asString(v2)->value(exec);

        return v1 == v2;
    }

    inline bool JSValue::strictEqual(ExecState* exec, JSValue v1, JSValue v2)
    {
        if (v1.isInt32() && v2.isInt32())
            return v1 == v2;

        if (v1.isNumber() && v2.isNumber())
            return v1.uncheckedGetNumber() == v2.uncheckedGetNumber();

        if (!v1.isCell() || !v2.isCell())
            return v1 == v2;

        return strictEqualSlowCaseInline(exec, v1, v2);
    }

    ALWAYS_INLINE bool jsLess(CallFrame* callFrame, JSValue v1, JSValue v2)
    {
        if (v1.isInt32() && v2.isInt32())
            return v1.asInt32() < v2.asInt32();

        double n1;
        double n2;
        if (v1.getNumber(n1) && v2.getNumber(n2))
            return n1 < n2;

        JSGlobalData* globalData = &callFrame->globalData();
        if (isJSString(globalData, v1) && isJSString(globalData, v2))
            return asString(v1)->value(callFrame) < asString(v2)->value(callFrame);

        JSValue p1;
        JSValue p2;
        bool wasNotString1 = v1.getPrimitiveNumber(callFrame, n1, p1);
        bool wasNotString2 = v2.getPrimitiveNumber(callFrame, n2, p2);

        if (wasNotString1 | wasNotString2)
            return n1 < n2;

        return asString(p1)->value(callFrame) < asString(p2)->value(callFrame);
    }

    inline bool jsLessEq(CallFrame* callFrame, JSValue v1, JSValue v2)
    {
        if (v1.isInt32() && v2.isInt32())
            return v1.asInt32() <= v2.asInt32();

        double n1;
        double n2;
        if (v1.getNumber(n1) && v2.getNumber(n2))
            return n1 <= n2;

        JSGlobalData* globalData = &callFrame->globalData();
        if (isJSString(globalData, v1) && isJSString(globalData, v2))
            return !(asString(v2)->value(callFrame) < asString(v1)->value(callFrame));

        JSValue p1;
        JSValue p2;
        bool wasNotString1 = v1.getPrimitiveNumber(callFrame, n1, p1);
        bool wasNotString2 = v2.getPrimitiveNumber(callFrame, n2, p2);

        if (wasNotString1 | wasNotString2)
            return n1 <= n2;

        return !(asString(p2)->value(callFrame) < asString(p1)->value(callFrame));
    }

    // Fast-path choices here are based on frequency data from SunSpider:
    //    <times> Add case: <t1> <t2>
    //    ---------------------------
    //    5626160 Add case: 3 3 (of these, 3637690 are for immediate values)
    //    247412  Add case: 5 5
    //    20900   Add case: 5 6
    //    13962   Add case: 5 3
    //    4000    Add case: 3 5

    ALWAYS_INLINE JSValue jsAdd(CallFrame* callFrame, JSValue v1, JSValue v2)
    {
        double left = 0.0, right;
        if (v1.getNumber(left) && v2.getNumber(right))
            return jsNumber(left + right);
        
        if (v1.isString()) {
            return v2.isString()
                ? jsString(callFrame, asString(v1), asString(v2))
                : jsString(callFrame, asString(v1), v2.toPrimitiveString(callFrame));
        }

        // All other cases are pretty uncommon
        return jsAddSlowCase(callFrame, v1, v2);
    }

    inline size_t normalizePrototypeChain(CallFrame* callFrame, JSValue base, JSValue slotBase, const Identifier& propertyName, size_t& slotOffset)
    {
        JSCell* cell = base.asCell();
        size_t count = 0;

        while (slotBase != cell) {
            JSValue v = cell->structure()->prototypeForLookup(callFrame);

            // If we didn't find slotBase in base's prototype chain, then base
            // must be a proxy for another object.

            if (v.isNull())
                return 0;

            cell = v.asCell();

            // Since we're accessing a prototype in a loop, it's a good bet that it
            // should not be treated as a dictionary.
            if (cell->structure()->isDictionary()) {
                asObject(cell)->flattenDictionaryObject(callFrame->globalData());
                if (slotBase == cell)
                    slotOffset = cell->structure()->get(callFrame->globalData(), propertyName); 
            }

            ++count;
        }
        
        ASSERT(count);
        return count;
    }

    inline size_t normalizePrototypeChain(CallFrame* callFrame, JSCell* base)
    {
        size_t count = 0;
        while (1) {
            JSValue v = base->structure()->prototypeForLookup(callFrame);
            if (v.isNull())
                return count;

            base = v.asCell();

            // Since we're accessing a prototype in a loop, it's a good bet that it
            // should not be treated as a dictionary.
            if (base->structure()->isDictionary())
                asObject(base)->flattenDictionaryObject(callFrame->globalData());

            ++count;
        }
    }

    ALWAYS_INLINE JSValue resolveBase(CallFrame* callFrame, Identifier& property, ScopeChainNode* scopeChain, bool isStrictPut)
    {
        ScopeChainIterator iter = scopeChain->begin();
        ScopeChainIterator next = iter;
        ++next;
        ScopeChainIterator end = scopeChain->end();
        ASSERT(iter != end);

        PropertySlot slot;
        JSObject* base;
        while (true) {
            base = iter->get();
            if (next == end) {
                if (isStrictPut && !base->getPropertySlot(callFrame, property, slot))
                    return JSValue();
                return base;
            }
            if (base->getPropertySlot(callFrame, property, slot))
                return base;

            iter = next;
            ++next;
        }

        ASSERT_NOT_REACHED();
        return JSValue();
    }
} // namespace JSC

#endif // Operations_h
