/*
 *  Copyright (C) 2006 Maks Orlovich
 *  Copyright (C) 2006, 2009, 2012 Apple, Inc.
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
#include "JSWrapperObject.h"

#include "Operations.h"

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(JSWrapperObject);

void JSWrapperObject::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSWrapperObject* thisObject = jsCast<JSWrapperObject*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());

    JSObject::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_internalValue);
}

} // namespace JSC
