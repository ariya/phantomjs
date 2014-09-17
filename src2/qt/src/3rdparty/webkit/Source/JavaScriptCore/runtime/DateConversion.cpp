/*
 * Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

#include "config.h"
#include "DateConversion.h"

#include "CallFrame.h"
#include "JSObject.h"
#include "ScopeChain.h"
#include "UString.h"
#include <wtf/DateMath.h>
#include <wtf/StringExtras.h>
#include <wtf/text/CString.h>

using namespace WTF;

namespace JSC {

void formatDate(const GregorianDateTime &t, DateConversionBuffer& buffer)
{
    snprintf(buffer, DateConversionBufferSize, "%s %s %02d %04d",
        weekdayName[(t.weekDay + 6) % 7],
        monthName[t.month], t.monthDay, t.year + 1900);
}

void formatDateUTCVariant(const GregorianDateTime &t, DateConversionBuffer& buffer)
{
    snprintf(buffer, DateConversionBufferSize, "%s, %02d %s %04d",
        weekdayName[(t.weekDay + 6) % 7],
        t.monthDay, monthName[t.month], t.year + 1900);
}

void formatTime(const GregorianDateTime &t, DateConversionBuffer& buffer)
{
    int offset = abs(gmtoffset(t));
    char timeZoneName[70];
    struct tm gtm = t;
    strftime(timeZoneName, sizeof(timeZoneName), "%Z", &gtm);

    if (timeZoneName[0]) {
        snprintf(buffer, DateConversionBufferSize, "%02d:%02d:%02d GMT%c%02d%02d (%s)",
            t.hour, t.minute, t.second,
            gmtoffset(t) < 0 ? '-' : '+', offset / (60*60), (offset / 60) % 60, timeZoneName);
    } else {
        snprintf(buffer, DateConversionBufferSize, "%02d:%02d:%02d GMT%c%02d%02d",
            t.hour, t.minute, t.second,
            gmtoffset(t) < 0 ? '-' : '+', offset / (60*60), (offset / 60) % 60);
    }
}

void formatTimeUTC(const GregorianDateTime &t, DateConversionBuffer& buffer)
{
    snprintf(buffer, DateConversionBufferSize, "%02d:%02d:%02d GMT", t.hour, t.minute, t.second);
}

} // namespace JSC
