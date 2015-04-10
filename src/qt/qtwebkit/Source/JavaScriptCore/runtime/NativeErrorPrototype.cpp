/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2008 Apple Inc. All rights reserved.
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
#include "NativeErrorPrototype.h"

#include "JSGlobalObject.h"
#include "JSString.h"
#include "NativeErrorConstructor.h"
#include "Operations.h"

namespace JSC {

NativeErrorPrototype::NativeErrorPrototype(ExecState* exec, Structure* structure)
    : ErrorPrototype(exec, structure)
{
}

void NativeErrorPrototype::finishCreation(ExecState* exec, JSGlobalObject* globalObject, const WTF::String& nameAndMessage, NativeErrorConstructor* constructor)
{
    Base::finishCreation(exec, globalObject);
    putDirect(exec->vm(), exec->propertyNames().name, jsString(exec, nameAndMessage), DontEnum);
    putDirect(exec->vm(), exec->propertyNames().message, jsEmptyString(exec), DontEnum);
    putDirect(exec->vm(), exec->propertyNames().constructor, constructor, DontEnum);
}

} // namespace JSC
