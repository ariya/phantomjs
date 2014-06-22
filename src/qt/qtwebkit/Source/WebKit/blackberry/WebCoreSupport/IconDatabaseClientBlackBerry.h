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

#ifndef IconDatabaseClientBlackBerry_h
#define IconDatabaseClientBlackBerry_h

#include "BlackBerryPlatformSingleton.h"
#include "IconDatabaseClient.h"

namespace BlackBerry {
namespace WebKit {
class WebSettings;
}
}

namespace WebCore {

class IconDatabaseClientBlackBerry : public BlackBerry::Platform::ThreadUnsafeSingleton<IconDatabaseClientBlackBerry>
    , public IconDatabaseClient {

    SINGLETON_DEFINITION_THREADUNSAFE(IconDatabaseClientBlackBerry)
public:
    bool initIconDatabase(const BlackBerry::WebKit::WebSettings*);

    virtual void didRemoveAllIcons();
    virtual void didImportIconURLForPageURL(const String&);
    virtual void didImportIconDataForPageURL(const String&);
    virtual void didChangeIconForPageURL(const String&);
    virtual void didFinishURLImport();

private:
    IconDatabaseClientBlackBerry()
        : m_initState(NotInitialized)
    {
    }
    enum { NotInitialized, InitializeSucceeded, InitializeFailed } m_initState;
};

}

#endif
