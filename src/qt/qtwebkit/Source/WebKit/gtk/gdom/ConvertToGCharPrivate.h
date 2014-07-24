/*
 * Copyright (C) 2008, 2009 Luke Kenneth Casson Leighton <lkcl@lkcl.net>
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
 */

#ifndef ConvertToGCharPrivate_h
#define ConvertToGCharPrivate_h

#include "KURL.h"
#include <wtf/text/CString.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/WTFString.h>

inline gchar* copyAsGchar(WTF::String const& s)
{
    return g_strdup(s.utf8().data());
}

inline gchar* copyAsGchar(WebCore::KURL const& s)
{
    return copyAsGchar(s.string());
}

inline gchar* copyAsGchar(const String& s)
{
    return g_strdup(s.UTF8String().c_str());
}

inline gchar* copyAsGchar(WTF::AtomicString const& s)
{
    return g_strdup(s.string().utf8().data());
}

#endif /* ConvertToGCharPrivate_h*/
