/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

// Note: this file is only for UNIX. On other platforms we can reuse the native implementation.

#include "config.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "PluginInfoStore.h"

#include "NetscapePluginModule.h"
#include "PluginDatabase.h"
#include <WebCore/FileSystem.h>

using namespace WebCore;

namespace WebKit {

Vector<String> PluginInfoStore::pluginsDirectories()
{
    Vector<String> result;

    result.append(homeDirectoryPath() + "/.mozilla/plugins");
    result.append(homeDirectoryPath() + "/.netscape/plugins");
    result.append("/usr/lib/browser/plugins");
    result.append("/usr/local/lib/mozilla/plugins");
    result.append("/usr/lib/firefox/plugins");
    result.append("/usr/lib64/browser-plugins");
    result.append("/usr/lib/browser-plugins");
    result.append("/usr/lib/mozilla/plugins");
    result.append("/usr/local/netscape/plugins");
    result.append("/opt/mozilla/plugins");
    result.append("/opt/mozilla/lib/plugins");
    result.append("/opt/netscape/plugins");
    result.append("/opt/netscape/communicator/plugins");
    result.append("/usr/lib/netscape/plugins");
    result.append("/usr/lib/netscape/plugins-libc5");
    result.append("/usr/lib/netscape/plugins-libc6");
    result.append("/usr/lib64/netscape/plugins");
    result.append("/usr/lib64/mozilla/plugins");
    result.append("/usr/lib/nsbrowser/plugins");
    result.append("/usr/lib64/nsbrowser/plugins");

    String mozillaHome(getenv("MOZILLA_HOME"));
    if (!mozillaHome.isEmpty())
        result.append(mozillaHome + "/plugins");

    String mozillaPaths(getenv("MOZ_PLUGIN_PATH"));
    if (!mozillaPaths.isEmpty()) {
        Vector<String> paths;
        mozillaPaths.split(UChar(':'), /* allowEmptyEntries */ false, paths);
        result.appendVector(paths);
    }

    return result;
}

Vector<String> PluginInfoStore::pluginPathsInDirectory(const String& directory)
{
    Vector<String> result;
    Vector<String> pluginPaths = listDirectory(directory, String("*.so"));
    Vector<String>::const_iterator end = pluginPaths.end();
    for (Vector<String>::const_iterator it = pluginPaths.begin(); it != end; ++it) {
        if (fileExists(*it))
            result.append(*it);
    }

    return result;
}

Vector<String> PluginInfoStore::individualPluginPaths()
{
    return Vector<String>();
}

bool PluginInfoStore::getPluginInfo(const String& pluginPath, PluginModuleInfo& plugin)
{
    return NetscapePluginModule::getPluginInfo(pluginPath, plugin);
}

bool PluginInfoStore::shouldUsePlugin(Vector<PluginModuleInfo>& /*alreadyLoadedPlugins*/, const PluginModuleInfo& /*plugin*/)
{
    // We do not do any black-listing presently.
    return true;
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
