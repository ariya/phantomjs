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
#include "ErrorInstance.h"

namespace JSC {

const ClassInfo ErrorInstance::s_info = { "Error", &JSNonFinalObject::s_info, 0, 0 };

ErrorInstance::ErrorInstance(JSGlobalData* globalData, Structure* structure)
    : JSNonFinalObject(*globalData, structure)
    , m_appendSourceToMessage(false)
{
    ASSERT(inherits(&s_info));
    putDirect(*globalData, globalData->propertyNames->message, jsString(globalData, ""));
}

ErrorInstance::ErrorInstance(JSGlobalData* globalData, Structure* structure, const UString& message)
    : JSNonFinalObject(*globalData, structure)
    , m_appendSourceToMessage(false)
{
    ASSERT(inherits(&s_info));
    putDirect(*globalData, globalData->propertyNames->message, jsString(globalData, message));
}

ErrorInstance* ErrorInstance::create(JSGlobalData* globalData, Structure* structure, const UString& message)
{
    return new (globalData) ErrorInstance(globalData, structure, message);
}

ErrorInstance* ErrorInstance::create(ExecState* exec, Structure* structure, JSValue message)
{
    if (message.isUndefined())
        return new (exec) ErrorInstance(&exec->globalData(), structure);
    return new (exec) ErrorInstance(&exec->globalData(), structure, message.toString(exec));
}

} // namespace JSC
