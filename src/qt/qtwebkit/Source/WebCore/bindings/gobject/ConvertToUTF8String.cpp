/*
 * Copyright (C) 2008 Luke Kenneth Casson Leighton <lkcl@lkcl.net>
 * Copyright (C) 2010 Igalia S.L.
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

#include "config.h"
#include "ConvertToUTF8String.h"

#include "KURL.h"
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#include <glib.h>

gchar* convertToUTF8String(WTF::String const& s)
{
    return g_strdup(s.utf8().data());
}

gchar* convertToUTF8String(WebCore::KURL const& s)
{
    return g_strdup(s.string().utf8().data());
}

