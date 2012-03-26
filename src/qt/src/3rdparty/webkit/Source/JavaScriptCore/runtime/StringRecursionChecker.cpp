/*
 *  Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "StringRecursionChecker.h"

#include "Error.h"
#include "ExceptionHelpers.h"

namespace JSC {

EncodedJSValue StringRecursionChecker::throwStackOverflowError()
{
    return throwVMError(m_exec, createStackOverflowError(m_exec));
}

EncodedJSValue StringRecursionChecker::emptyString()
{
    return JSValue::encode(jsEmptyString(m_exec));
}

}
