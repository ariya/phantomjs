/*
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "IconDatabaseClientBlackBerry.h"

#include "BlackBerryPlatformSettings.h"
#include "IconDatabase.h"
#include "WebSettings.h"

namespace WebCore {

SINGLETON_INITIALIZER_THREADUNSAFE(IconDatabaseClientBlackBerry)

bool IconDatabaseClientBlackBerry::initIconDatabase(const BlackBerry::WebKit::WebSettings* settings)
{
    bool enable = !settings->isPrivateBrowsingEnabled() && settings->isDatabasesEnabled();
    iconDatabase().setEnabled(enable);
    if (!enable) {
        m_initState = NotInitialized;
        return false;
    }

    if (m_initState == InitializeFailed)
        return false;

    if (m_initState == InitializeSucceeded)
        return true;

    iconDatabase().setClient(this);

    m_initState = iconDatabase().open(BlackBerry::Platform::Settings::instance()->applicationDataDirectory().c_str(),
        IconDatabase::defaultDatabaseFilename()) ? InitializeSucceeded : InitializeFailed;

    return m_initState == InitializeSucceeded;
}

void IconDatabaseClientBlackBerry::didRemoveAllIcons()
{
}

void IconDatabaseClientBlackBerry::didImportIconURLForPageURL(const String&)
{
}

void IconDatabaseClientBlackBerry::didImportIconDataForPageURL(const String&)
{
}

void IconDatabaseClientBlackBerry::didChangeIconForPageURL(const String&)
{
}

void IconDatabaseClientBlackBerry::didFinishURLImport()
{
}

} // namespace WebCore
