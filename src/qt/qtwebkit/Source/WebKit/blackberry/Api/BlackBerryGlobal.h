/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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

#ifndef BlackBerryGlobal_h
#define BlackBerryGlobal_h

#if defined(__QNXNTO__) && defined(BUILD_WEBKIT)
#define BLACKBERRY_EXPORT __attribute__ ((visibility("default")))
#else
#define BLACKBERRY_EXPORT
#endif

namespace BlackBerry {
namespace Platform {
class String;
}

namespace WebKit {

BLACKBERRY_EXPORT void globalInitialize();
void collectJavascriptGarbageNow();
void clearCookieCache();
BLACKBERRY_EXPORT void clearMemoryCaches();
void clearAppCache(const BlackBerry::Platform::String& pageGroupName);
void reopenAllAppCaches();
void closeAllAppCaches();
void clearDatabase(const BlackBerry::Platform::String& pageGroupName);
void reopenAllTrackerDatabases();
void closeAllTrackerDatabases();
void updateOnlineStatus(bool online);
bool isRunningDrt();
}
}

#endif // BlackBerryGlobal_h
