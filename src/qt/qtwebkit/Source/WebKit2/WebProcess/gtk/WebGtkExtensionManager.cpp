/*
 * Copyright (C) 2012 Igalia S.L.
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
#include "WebGtkExtensionManager.h"

#include "InjectedBundle.h"
#include "WKBundleAPICast.h"
#include "WKString.h"
#include "WKType.h"
#include "WebKitWebExtensionPrivate.h"
#include <WebCore/FileSystem.h>
#include <wtf/OwnPtr.h>

namespace WebKit {

WebGtkExtensionManager& WebGtkExtensionManager::shared()
{
    DEFINE_STATIC_LOCAL(WebGtkExtensionManager, extensionManager, ());
    return extensionManager;
}

WebGtkExtensionManager::WebGtkExtensionManager()
{
}

void WebGtkExtensionManager::scanModules(const String& webExtensionsDirectory, Vector<String>& modules)
{
    Vector<String> modulePaths = WebCore::listDirectory(webExtensionsDirectory, String("*.so"));
    for (size_t i = 0; i < modulePaths.size(); ++i) {
        if (WebCore::fileExists(modulePaths[i]))
            modules.append(modulePaths[i]);
    }
}

void WebGtkExtensionManager::initialize(WKBundleRef bundle, WKTypeRef userData)
{
    m_extension = adoptGRef(webkitWebExtensionCreate(toImpl(bundle)));

    String webExtensionsDirectory;
    if (userData) {
        ASSERT(WKGetTypeID(userData) == WKStringGetTypeID());
        webExtensionsDirectory = toImpl(static_cast<WKStringRef>(userData))->string();
    }

    if (webExtensionsDirectory.isNull())
        return;

    Vector<String> modulePaths;
    scanModules(webExtensionsDirectory, modulePaths);

    for (size_t i = 0; i < modulePaths.size(); ++i) {
        OwnPtr<Module> module = adoptPtr(new Module(modulePaths[i]));
        if (!module->load())
            continue;

        WebKitWebExtensionInitializeFunction initializeFunction =
            module->functionPointer<WebKitWebExtensionInitializeFunction>("webkit_web_extension_initialize");
        if (!initializeFunction)
            continue;

        m_extensionModules.append(module.leakPtr());
        initializeFunction(m_extension.get());
    }
}

} // namespace WebKit
