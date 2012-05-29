/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "DateInstance.h"

#include "JSGlobalObject.h"

#include <math.h>
#include <wtf/DateMath.h>
#include <wtf/MathExtras.h>

using namespace WTF;

namespace JSC {

const ClassInfo DateInstance::s_info = {"Date", &JSWrapperObject::s_info, 0, 0};

DateInstance::DateInstance(ExecState* exec, Structure* structure)
    : JSWrapperObject(exec->globalData(), structure)
{
    ASSERT(inherits(&s_info));
    setInternalValue(exec->globalData(), jsNaN());
}

DateInstance::DateInstance(ExecState* exec, Structure* structure, double time)
    : JSWrapperObject(exec->globalData(), structure)
{
    ASSERT(inherits(&s_info));
    setInternalValue(exec->globalData(), jsNumber(timeClip(time)));
}

const GregorianDateTime* DateInstance::calculateGregorianDateTime(ExecState* exec) const
{
    double milli = internalNumber();
    if (isnan(milli))
        return 0;

    if (!m_data)
        m_data = exec->globalData().dateInstanceCache.add(milli);

    if (m_data->m_gregorianDateTimeCachedForMS != milli) {
        msToGregorianDateTime(exec, milli, false, m_data->m_cachedGregorianDateTime);
        m_data->m_gregorianDateTimeCachedForMS = milli;
    }
    return &m_data->m_cachedGregorianDateTime;
}

const GregorianDateTime* DateInstance::calculateGregorianDateTimeUTC(ExecState* exec) const
{
    double milli = internalNumber();
    if (isnan(milli))
        return 0;

    if (!m_data)
        m_data = exec->globalData().dateInstanceCache.add(milli);

    if (m_data->m_gregorianDateTimeUTCCachedForMS != milli) {
        msToGregorianDateTime(exec, milli, true, m_data->m_cachedGregorianDateTimeUTC);
        m_data->m_gregorianDateTimeUTCCachedForMS = milli;
    }
    return &m_data->m_cachedGregorianDateTimeUTC;
}

} // namespace JSC
