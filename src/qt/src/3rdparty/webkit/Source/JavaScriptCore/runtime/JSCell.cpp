/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All rights reserved.
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
#include "JSCell.h"

#include "JSFunction.h"
#include "JSString.h"
#include "JSObject.h"
#include <wtf/MathExtras.h>

namespace JSC {

#if defined NAN && defined INFINITY

extern const double NaN = NAN;
extern const double Inf = INFINITY;

#else // !(defined NAN && defined INFINITY)

// The trick is to define the NaN and Inf globals with a different type than the declaration.
// This trick works because the mangled name of the globals does not include the type, although
// I'm not sure that's guaranteed. There could be alignment issues with this, since arrays of
// characters don't necessarily need the same alignment doubles do, but for now it seems to work.
// It would be good to figure out a 100% clean way that still avoids code that runs at init time.

// Note, we have to use union to ensure alignment. Otherwise, NaN_Bytes can start anywhere,
// while NaN_double has to be 4-byte aligned for 32-bits.
// With -fstrict-aliasing enabled, unions are the only safe way to do type masquerading.

static const union {
    struct {
        unsigned char NaN_Bytes[8];
        unsigned char Inf_Bytes[8];
    } bytes;
    
    struct {
        double NaN_Double;
        double Inf_Double;
    } doubles;
    
} NaNInf = { {
#if CPU(BIG_ENDIAN)
    { 0x7f, 0xf8, 0, 0, 0, 0, 0, 0 },
    { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 }
#elif CPU(MIDDLE_ENDIAN)
    { 0, 0, 0xf8, 0x7f, 0, 0, 0, 0 },
    { 0, 0, 0xf0, 0x7f, 0, 0, 0, 0 }
#else
    { 0, 0, 0, 0, 0, 0, 0xf8, 0x7f },
    { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f }
#endif
} } ;

extern const double NaN = NaNInf.doubles.NaN_Double;
extern const double Inf = NaNInf.doubles.Inf_Double;
 
#endif // !(defined NAN && defined INFINITY)

const ClassInfo JSCell::s_dummyCellInfo = { "DummyCell", 0, 0, 0 };

bool JSCell::getUInt32(uint32_t&) const
{
    return false;
}

bool JSCell::getString(ExecState* exec, UString&stringValue) const
{
    if (!isString())
        return false;
    stringValue = static_cast<const JSString*>(this)->value(exec);
    return true;
}

UString JSCell::getString(ExecState* exec) const
{
    return isString() ? static_cast<const JSString*>(this)->value(exec) : UString();
}

JSObject* JSCell::getObject()
{
    return isObject() ? asObject(this) : 0;
}

const JSObject* JSCell::getObject() const
{
    return isObject() ? static_cast<const JSObject*>(this) : 0;
}

CallType JSCell::getCallData(CallData&)
{
    return CallTypeNone;
}

ConstructType JSCell::getConstructData(ConstructData&)
{
    return ConstructTypeNone;
}

bool JSCell::getOwnPropertySlot(ExecState* exec, const Identifier& identifier, PropertySlot& slot)
{
    // This is not a general purpose implementation of getOwnPropertySlot.
    // It should only be called by JSValue::get.
    // It calls getPropertySlot, not getOwnPropertySlot.
    JSObject* object = toObject(exec, exec->lexicalGlobalObject());
    slot.setBase(object);
    if (!object->getPropertySlot(exec, identifier, slot))
        slot.setUndefined();
    return true;
}

bool JSCell::getOwnPropertySlot(ExecState* exec, unsigned identifier, PropertySlot& slot)
{
    // This is not a general purpose implementation of getOwnPropertySlot.
    // It should only be called by JSValue::get.
    // It calls getPropertySlot, not getOwnPropertySlot.
    JSObject* object = toObject(exec, exec->lexicalGlobalObject());
    slot.setBase(object);
    if (!object->getPropertySlot(exec, identifier, slot))
        slot.setUndefined();
    return true;
}

void JSCell::put(ExecState* exec, const Identifier& identifier, JSValue value, PutPropertySlot& slot)
{
    toObject(exec, exec->lexicalGlobalObject())->put(exec, identifier, value, slot);
}

void JSCell::put(ExecState* exec, unsigned identifier, JSValue value)
{
    toObject(exec, exec->lexicalGlobalObject())->put(exec, identifier, value);
}

bool JSCell::deleteProperty(ExecState* exec, const Identifier& identifier)
{
    return toObject(exec, exec->lexicalGlobalObject())->deleteProperty(exec, identifier);
}

bool JSCell::deleteProperty(ExecState* exec, unsigned identifier)
{
    return toObject(exec, exec->lexicalGlobalObject())->deleteProperty(exec, identifier);
}

JSObject* JSCell::toThisObject(ExecState* exec) const
{
    return toObject(exec, exec->lexicalGlobalObject());
}

JSValue JSCell::getJSNumber()
{
    return JSValue();
}

bool JSCell::isGetterSetter() const
{
    return false;
}

JSValue JSCell::toPrimitive(ExecState*, PreferredPrimitiveType) const
{
    ASSERT_NOT_REACHED();
    return JSValue();
}

bool JSCell::getPrimitiveNumber(ExecState*, double&, JSValue&)
{
    ASSERT_NOT_REACHED();
    return false;
}

bool JSCell::toBoolean(ExecState*) const
{
    ASSERT_NOT_REACHED();
    return false;
}

double JSCell::toNumber(ExecState*) const
{
    ASSERT_NOT_REACHED();
    return 0;
}

UString JSCell::toString(ExecState*) const
{
    ASSERT_NOT_REACHED();
    return UString();
}

JSObject* JSCell::toObject(ExecState*, JSGlobalObject*) const
{
    ASSERT_NOT_REACHED();
    return 0;
}

bool isZombie(const JSCell* cell)
{
#if ENABLE(JSC_ZOMBIES)
    return cell && cell->isZombie();
#else
    UNUSED_PARAM(cell);
    return false;
#endif
}

} // namespace JSC
