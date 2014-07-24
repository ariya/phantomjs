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
#include "InternalFunction.h"

#include "FunctionPrototype.h"
#include "JSGlobalObject.h"
#include "JSString.h"
#include "Operations.h"

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(InternalFunction);

const ClassInfo InternalFunction::s_info = { "Function", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(InternalFunction) };

InternalFunction::InternalFunction(JSGlobalObject* globalObject, Structure* structure)
    : JSDestructibleObject(globalObject->vm(), structure)
{
}

void InternalFunction::finishCreation(VM& vm, const String& name)
{
    Base::finishCreation(vm);
    ASSERT(inherits(&s_info));
    ASSERT(methodTable()->getCallData != InternalFunction::s_info.methodTable.getCallData);
    putDirect(vm, vm.propertyNames->name, jsString(&vm, name), DontDelete | ReadOnly | DontEnum);
}

const String& InternalFunction::name(ExecState* exec)
{
    return asString(getDirect(exec->vm(), exec->vm().propertyNames->name))->tryGetValue();
}

const String InternalFunction::displayName(ExecState* exec)
{
    JSValue displayName = getDirect(exec->vm(), exec->vm().propertyNames->displayName);
    
    if (displayName && isJSString(displayName))
        return asString(displayName)->tryGetValue();
    
    return String();
}

CallType InternalFunction::getCallData(JSCell*, CallData&)
{
    RELEASE_ASSERT_NOT_REACHED();
    return CallTypeNone;
}

const String InternalFunction::calculatedDisplayName(ExecState* exec)
{
    const String explicitName = displayName(exec);
    
    if (!explicitName.isEmpty())
        return explicitName;
    
    return name(exec);
}

} // namespace JSC
