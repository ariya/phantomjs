/*
 *  Copyright (C) 2006 Maks Orlovich
 *  Copyright (C) 2006, 2009 Apple, Inc.
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

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(JSWrapperObject);

void JSWrapperObject::visitChildren(SlotVisitor& visitor) 
{
    JSObject::visitChildren(visitor);
    if (m_internalValue)
        visitor.append(&m_internalValue);
}

} // namespace JSC
