/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All Rights Reserved.
 *  Copyright (C) 2009 Torch Mobile, Inc.
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
#include "RegExpConstructor.h"

#include "Error.h"
#include "Operations.h"
#include "RegExpMatchesArray.h"
#include "RegExpPrototype.h"

namespace JSC {

static JSValue regExpConstructorInput(ExecState*, JSValue, PropertyName);
static JSValue regExpConstructorMultiline(ExecState*, JSValue, PropertyName);
static JSValue regExpConstructorLastMatch(ExecState*, JSValue, PropertyName);
static JSValue regExpConstructorLastParen(ExecState*, JSValue, PropertyName);
static JSValue regExpConstructorLeftContext(ExecState*, JSValue, PropertyName);
static JSValue regExpConstructorRightContext(ExecState*, JSValue, PropertyName);
static JSValue regExpConstructorDollar1(ExecState*, JSValue, PropertyName);
static JSValue regExpConstructorDollar2(ExecState*, JSValue, PropertyName);
static JSValue regExpConstructorDollar3(ExecState*, JSValue, PropertyName);
static JSValue regExpConstructorDollar4(ExecState*, JSValue, PropertyName);
static JSValue regExpConstructorDollar5(ExecState*, JSValue, PropertyName);
static JSValue regExpConstructorDollar6(ExecState*, JSValue, PropertyName);
static JSValue regExpConstructorDollar7(ExecState*, JSValue, PropertyName);
static JSValue regExpConstructorDollar8(ExecState*, JSValue, PropertyName);
static JSValue regExpConstructorDollar9(ExecState*, JSValue, PropertyName);

static void setRegExpConstructorInput(ExecState*, JSObject*, JSValue);
static void setRegExpConstructorMultiline(ExecState*, JSObject*, JSValue);

} // namespace JSC

#include "RegExpConstructor.lut.h"

namespace JSC {

const ClassInfo RegExpConstructor::s_info = { "Function", &InternalFunction::s_info, 0, ExecState::regExpConstructorTable, CREATE_METHOD_TABLE(RegExpConstructor) };

/* Source for RegExpConstructor.lut.h
@begin regExpConstructorTable
    input           regExpConstructorInput          None
    $_              regExpConstructorInput          DontEnum
    multiline       regExpConstructorMultiline      None
    $*              regExpConstructorMultiline      DontEnum
    lastMatch       regExpConstructorLastMatch      DontDelete|ReadOnly
    $&              regExpConstructorLastMatch      DontDelete|ReadOnly|DontEnum
    lastParen       regExpConstructorLastParen      DontDelete|ReadOnly
    $+              regExpConstructorLastParen      DontDelete|ReadOnly|DontEnum
    leftContext     regExpConstructorLeftContext    DontDelete|ReadOnly
    $`              regExpConstructorLeftContext    DontDelete|ReadOnly|DontEnum
    rightContext    regExpConstructorRightContext   DontDelete|ReadOnly
    $'              regExpConstructorRightContext   DontDelete|ReadOnly|DontEnum
    $1              regExpConstructorDollar1        DontDelete|ReadOnly
    $2              regExpConstructorDollar2        DontDelete|ReadOnly
    $3              regExpConstructorDollar3        DontDelete|ReadOnly
    $4              regExpConstructorDollar4        DontDelete|ReadOnly
    $5              regExpConstructorDollar5        DontDelete|ReadOnly
    $6              regExpConstructorDollar6        DontDelete|ReadOnly
    $7              regExpConstructorDollar7        DontDelete|ReadOnly
    $8              regExpConstructorDollar8        DontDelete|ReadOnly
    $9              regExpConstructorDollar9        DontDelete|ReadOnly
@end
*/

RegExpConstructor::RegExpConstructor(JSGlobalObject* globalObject, Structure* structure, RegExpPrototype* regExpPrototype)
    : InternalFunction(globalObject, structure)
    , m_cachedResult(globalObject->vm(), this, regExpPrototype->regExp())
    , m_multiline(false)
{
}

void RegExpConstructor::finishCreation(ExecState* exec, RegExpPrototype* regExpPrototype)
{
    Base::finishCreation(exec->vm(), Identifier(exec, "RegExp").string());
    ASSERT(inherits(&s_info));

    // ECMA 15.10.5.1 RegExp.prototype
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().prototype, regExpPrototype, DontEnum | DontDelete | ReadOnly);

    // no. of arguments for constructor
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().length, jsNumber(2), ReadOnly | DontDelete | DontEnum);
}

void RegExpConstructor::destroy(JSCell* cell)
{
    static_cast<RegExpConstructor*>(cell)->RegExpConstructor::~RegExpConstructor();
}

void RegExpConstructor::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    RegExpConstructor* thisObject = jsCast<RegExpConstructor*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());

    Base::visitChildren(thisObject, visitor);
    thisObject->m_cachedResult.visitChildren(visitor);
}

JSValue RegExpConstructor::getBackref(ExecState* exec, unsigned i)
{
    RegExpMatchesArray* array = m_cachedResult.lastResult(exec, this);

    if (i < array->length()) {
        JSValue result = JSValue(array).get(exec, i);
        ASSERT(result.isString() || result.isUndefined());
        if (!result.isUndefined())
            return result;
    }
    return jsEmptyString(exec);
}

JSValue RegExpConstructor::getLastParen(ExecState* exec)
{
    RegExpMatchesArray* array = m_cachedResult.lastResult(exec, this);
    unsigned length = array->length();
    if (length > 1) {
        JSValue result = JSValue(array).get(exec, length - 1);
        ASSERT(result.isString() || result.isUndefined());
        if (!result.isUndefined())
            return result;
    }
    return jsEmptyString(exec);
}

JSValue RegExpConstructor::getLeftContext(ExecState* exec)
{
    return m_cachedResult.lastResult(exec, this)->leftContext(exec);
}

JSValue RegExpConstructor::getRightContext(ExecState* exec)
{
    return m_cachedResult.lastResult(exec, this)->rightContext(exec);
}
    
bool RegExpConstructor::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<RegExpConstructor, InternalFunction>(exec, ExecState::regExpConstructorTable(exec), jsCast<RegExpConstructor*>(cell), propertyName, slot);
}

bool RegExpConstructor::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    return getStaticValueDescriptor<RegExpConstructor, InternalFunction>(exec, ExecState::regExpConstructorTable(exec), jsCast<RegExpConstructor*>(object), propertyName, descriptor);
}

JSValue regExpConstructorDollar1(ExecState* exec, JSValue slotBase, PropertyName)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 1);
}

JSValue regExpConstructorDollar2(ExecState* exec, JSValue slotBase, PropertyName)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 2);
}

JSValue regExpConstructorDollar3(ExecState* exec, JSValue slotBase, PropertyName)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 3);
}

JSValue regExpConstructorDollar4(ExecState* exec, JSValue slotBase, PropertyName)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 4);
}

JSValue regExpConstructorDollar5(ExecState* exec, JSValue slotBase, PropertyName)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 5);
}

JSValue regExpConstructorDollar6(ExecState* exec, JSValue slotBase, PropertyName)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 6);
}

JSValue regExpConstructorDollar7(ExecState* exec, JSValue slotBase, PropertyName)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 7);
}

JSValue regExpConstructorDollar8(ExecState* exec, JSValue slotBase, PropertyName)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 8);
}

JSValue regExpConstructorDollar9(ExecState* exec, JSValue slotBase, PropertyName)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 9);
}

JSValue regExpConstructorInput(ExecState*, JSValue slotBase, PropertyName)
{
    return asRegExpConstructor(slotBase)->input();
}

JSValue regExpConstructorMultiline(ExecState*, JSValue slotBase, PropertyName)
{
    return jsBoolean(asRegExpConstructor(slotBase)->multiline());
}

JSValue regExpConstructorLastMatch(ExecState* exec, JSValue slotBase, PropertyName)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 0);
}

JSValue regExpConstructorLastParen(ExecState* exec, JSValue slotBase, PropertyName)
{
    return asRegExpConstructor(slotBase)->getLastParen(exec);
}

JSValue regExpConstructorLeftContext(ExecState* exec, JSValue slotBase, PropertyName)
{
    return asRegExpConstructor(slotBase)->getLeftContext(exec);
}

JSValue regExpConstructorRightContext(ExecState* exec, JSValue slotBase, PropertyName)
{
    return asRegExpConstructor(slotBase)->getRightContext(exec);
}

void RegExpConstructor::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    lookupPut<RegExpConstructor, InternalFunction>(exec, propertyName, value, ExecState::regExpConstructorTable(exec), jsCast<RegExpConstructor*>(cell), slot);
}

void setRegExpConstructorInput(ExecState* exec, JSObject* baseObject, JSValue value)
{
    asRegExpConstructor(baseObject)->setInput(exec, value.toString(exec));
}

void setRegExpConstructorMultiline(ExecState* exec, JSObject* baseObject, JSValue value)
{
    asRegExpConstructor(baseObject)->setMultiline(value.toBoolean(exec));
}

// ECMA 15.10.4
JSObject* constructRegExp(ExecState* exec, JSGlobalObject* globalObject, const ArgList& args, bool callAsConstructor)
{
    JSValue arg0 = args.at(0);
    JSValue arg1 = args.at(1);

    if (arg0.inherits(&RegExpObject::s_info)) {
        if (!arg1.isUndefined())
            return throwError(exec, createTypeError(exec, ASCIILiteral("Cannot supply flags when constructing one RegExp from another.")));
        // If called as a function, this just returns the first argument (see 15.10.3.1).
        if (callAsConstructor) {
            RegExp* regExp = static_cast<RegExpObject*>(asObject(arg0))->regExp();
            return RegExpObject::create(exec, globalObject, globalObject->regExpStructure(), regExp);
        }
        return asObject(arg0);
    }

    String pattern = arg0.isUndefined() ? String("") : arg0.toString(exec)->value(exec);
    if (exec->hadException())
        return 0;

    RegExpFlags flags = NoFlags;
    if (!arg1.isUndefined()) {
        flags = regExpFlags(arg1.toString(exec)->value(exec));
        if (exec->hadException())
            return 0;
        if (flags == InvalidFlags)
            return throwError(exec, createSyntaxError(exec, ASCIILiteral("Invalid flags supplied to RegExp constructor.")));
    }

    RegExp* regExp = RegExp::create(exec->vm(), pattern, flags);
    if (!regExp->isValid())
        return throwError(exec, createSyntaxError(exec, regExp->errorMessage()));
    return RegExpObject::create(exec, exec->lexicalGlobalObject(), globalObject->regExpStructure(), regExp);
}

static EncodedJSValue JSC_HOST_CALL constructWithRegExpConstructor(ExecState* exec)
{
    ArgList args(exec);
    return JSValue::encode(constructRegExp(exec, asInternalFunction(exec->callee())->globalObject(), args, true));
}

ConstructType RegExpConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructWithRegExpConstructor;
    return ConstructTypeHost;
}

// ECMA 15.10.3
static EncodedJSValue JSC_HOST_CALL callRegExpConstructor(ExecState* exec)
{
    ArgList args(exec);
    return JSValue::encode(constructRegExp(exec, asInternalFunction(exec->callee())->globalObject(), args));
}

CallType RegExpConstructor::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callRegExpConstructor;
    return CallTypeHost;
}

} // namespace JSC
