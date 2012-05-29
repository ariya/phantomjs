/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All Rights Reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "RegExpObject.h"

#include "Error.h"
#include "ExceptionHelpers.h"
#include "JSArray.h"
#include "JSGlobalObject.h"
#include "JSString.h"
#include "Lookup.h"
#include "RegExpConstructor.h"
#include "RegExpPrototype.h"
#include "UStringConcatenate.h"
#include <wtf/PassOwnPtr.h>

namespace JSC {

static JSValue regExpObjectGlobal(ExecState*, JSValue, const Identifier&);
static JSValue regExpObjectIgnoreCase(ExecState*, JSValue, const Identifier&);
static JSValue regExpObjectMultiline(ExecState*, JSValue, const Identifier&);
static JSValue regExpObjectSource(ExecState*, JSValue, const Identifier&);
static JSValue regExpObjectLastIndex(ExecState*, JSValue, const Identifier&);
static void setRegExpObjectLastIndex(ExecState*, JSObject*, JSValue);

} // namespace JSC

#include "RegExpObject.lut.h"

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(RegExpObject);

const ClassInfo RegExpObject::s_info = { "RegExp", &JSObjectWithGlobalObject::s_info, 0, ExecState::regExpTable };

/* Source for RegExpObject.lut.h
@begin regExpTable
    global        regExpObjectGlobal       DontDelete|ReadOnly|DontEnum
    ignoreCase    regExpObjectIgnoreCase   DontDelete|ReadOnly|DontEnum
    multiline     regExpObjectMultiline    DontDelete|ReadOnly|DontEnum
    source        regExpObjectSource       DontDelete|ReadOnly|DontEnum
    lastIndex     regExpObjectLastIndex    DontDelete|DontEnum
@end
*/

RegExpObject::RegExpObject(JSGlobalObject* globalObject, Structure* structure, NonNullPassRefPtr<RegExp> regExp)
    : JSObjectWithGlobalObject(globalObject, structure)
    , d(adoptPtr(new RegExpObjectData(regExp)))
{
    ASSERT(inherits(&s_info));
}

RegExpObject::~RegExpObject()
{
}

void RegExpObject::visitChildren(SlotVisitor& visitor)
{
    Base::visitChildren(visitor);
    if (UNLIKELY(!d->lastIndex.get().isInt32()))
        visitor.append(&d->lastIndex);
}

bool RegExpObject::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<RegExpObject, JSObject>(exec, ExecState::regExpTable(exec), this, propertyName, slot);
}

bool RegExpObject::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticValueDescriptor<RegExpObject, JSObject>(exec, ExecState::regExpTable(exec), this, propertyName, descriptor);
}

JSValue regExpObjectGlobal(ExecState*, JSValue slotBase, const Identifier&)
{
    return jsBoolean(asRegExpObject(slotBase)->regExp()->global());
}

JSValue regExpObjectIgnoreCase(ExecState*, JSValue slotBase, const Identifier&)
{
    return jsBoolean(asRegExpObject(slotBase)->regExp()->ignoreCase());
}
 
JSValue regExpObjectMultiline(ExecState*, JSValue slotBase, const Identifier&)
{            
    return jsBoolean(asRegExpObject(slotBase)->regExp()->multiline());
}

JSValue regExpObjectSource(ExecState* exec, JSValue slotBase, const Identifier&)
{
    return jsString(exec, asRegExpObject(slotBase)->regExp()->pattern());
}

JSValue regExpObjectLastIndex(ExecState*, JSValue slotBase, const Identifier&)
{
    return asRegExpObject(slotBase)->getLastIndex();
}

void RegExpObject::put(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    lookupPut<RegExpObject, JSObject>(exec, propertyName, value, ExecState::regExpTable(exec), this, slot);
}

void setRegExpObjectLastIndex(ExecState* exec, JSObject* baseObject, JSValue value)
{
    asRegExpObject(baseObject)->setLastIndex(exec->globalData(), value);
}

JSValue RegExpObject::test(ExecState* exec)
{
    return jsBoolean(match(exec));
}

JSValue RegExpObject::exec(ExecState* exec)
{
    if (match(exec))
        return exec->lexicalGlobalObject()->regExpConstructor()->arrayOfMatches(exec);
    return jsNull();
}

// Shared implementation used by test and exec.
bool RegExpObject::match(ExecState* exec)
{
    RegExpConstructor* regExpConstructor = exec->lexicalGlobalObject()->regExpConstructor();
    UString input = exec->argument(0).toString(exec);

    if (!regExp()->global()) {
        int position;
        int length;
        regExpConstructor->performMatch(d->regExp.get(), input, 0, position, length);
        return position >= 0;
    }

    JSValue jsLastIndex = getLastIndex();
    unsigned lastIndex;
    if (LIKELY(jsLastIndex.isUInt32())) {
        lastIndex = jsLastIndex.asUInt32();
        if (lastIndex > input.length()) {
            setLastIndex(0);
            return false;
        }
    } else {
        double doubleLastIndex = jsLastIndex.toInteger(exec);
        if (doubleLastIndex < 0 || doubleLastIndex > input.length()) {
            setLastIndex(0);
            return false;
        }
        lastIndex = static_cast<unsigned>(doubleLastIndex);
    }

    int position;
    int length = 0;
    regExpConstructor->performMatch(d->regExp.get(), input, lastIndex, position, length);
    if (position < 0) {
        setLastIndex(0);
        return false;
    }

    setLastIndex(position + length);
    return true;
}

} // namespace JSC
