/*
 *  Copyright (C) 2012 Igalia S.L.
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
 */

#include "config.h"
#include "CookieStorage.h"

#include "CookieJarSoup.h"
#include "NotImplemented.h"

#include <stdio.h>

namespace WebCore {

static CookieChangeCallbackPtr cookieChangeCallback;

static void soupCookiesChanged(SoupCookieJar* jar, SoupCookie*, SoupCookie*, gpointer)
{
    if (jar != soupCookieJar())
        return;
    cookieChangeCallback();
}

void startObservingCookieChanges(CookieChangeCallbackPtr callback)
{
    ASSERT(!cookieChangeCallback);
    cookieChangeCallback = callback;

    g_signal_connect(soupCookieJar(), "changed", G_CALLBACK(soupCookiesChanged), 0);
}

void stopObservingCookieChanges()
{
    g_signal_handlers_disconnect_by_func(soupCookieJar(), reinterpret_cast<void*>(soupCookiesChanged), 0);
    cookieChangeCallback = 0;
}

}
