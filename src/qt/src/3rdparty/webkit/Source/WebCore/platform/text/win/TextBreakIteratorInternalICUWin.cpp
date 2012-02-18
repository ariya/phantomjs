/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "TextBreakIteratorInternalICU.h"

namespace WebCore {

const char* currentSearchLocaleID()
{
    // FIXME: Should use system locale.
    return "";
}

const char* currentTextBreakLocaleID()
{
    // Using en_US_POSIX now so word selection in address field works as expected as before (double-clicking
    // in a URL selects a word delimited by periods rather than selecting the entire URL).
    // However, this is not entirely correct - we should honor the system locale in the normal case.
    // FIXME: <rdar://problem/6786703> Should use system locale for text breaking
    return "en_US_POSIX";
}

}
