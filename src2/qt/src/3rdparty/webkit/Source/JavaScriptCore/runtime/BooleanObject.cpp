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
#include "BooleanObject.h"

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(BooleanObject);

const ClassInfo BooleanObject::s_info = { "Boolean", &JSWrapperObject::s_info, 0, 0 };

BooleanObject::BooleanObject(JSGlobalData& globalData, Structure* structure)
    : JSWrapperObject(globalData, structure)
{
    ASSERT(inherits(&s_info));
}

} // namespace JSC
