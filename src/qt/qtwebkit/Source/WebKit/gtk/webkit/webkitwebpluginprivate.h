/*
 *  Copyright (C) 2010 Igalia S.L.
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

#ifndef webkitwebpluginprivate_h
#define webkitwebpluginprivate_h

#include "webkitwebplugin.h"
#include <glib-object.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

namespace WebCore {
class PluginPackage;
}

namespace WebKit {
WebKitWebPlugin* kitNew(WebCore::PluginPackage* package);
}

extern "C" {

typedef struct _WebKitWebPluginPrivate WebKitWebPluginPrivate;
struct _WebKitWebPluginPrivate {
    RefPtr<WebCore::PluginPackage> corePlugin;
    CString name;
    CString description;
    GOwnPtr<char> path;
    GSList* mimeTypes;
};

}

#endif
