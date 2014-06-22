/*
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
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
#include "Language.h"

#include <wtf/gobject/GOwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#include <glib.h>
#include <locale.h>

namespace WebCore {

// Using pango_language_get_default() here is not an option, because
// it doesn't support changing the locale in runtime, so it returns
// always the same value.
static String platformLanguage()
{
    char* localeDefault = setlocale(LC_CTYPE, NULL);

    if (!localeDefault)
        return String("c");

    GOwnPtr<gchar> normalizedDefault(g_ascii_strdown(localeDefault, -1));
    char* ptr = strchr(normalizedDefault.get(), '_');

    if (ptr)
        *ptr = '-';

    ptr = strchr(normalizedDefault.get(), '.');

    if (ptr)
        *ptr = '\0';

    return String(normalizedDefault.get());
}

Vector<String> platformUserPreferredLanguages()
{
    Vector<String> userPreferredLanguages;
    userPreferredLanguages.append(platformLanguage());
    return userPreferredLanguages;
}
    
}
