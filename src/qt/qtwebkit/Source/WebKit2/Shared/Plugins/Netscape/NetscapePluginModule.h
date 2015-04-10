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

#ifndef NetscapePluginModule_h
#define NetscapePluginModule_h

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "Module.h"
#include "PluginModuleInfo.h"
#include "PluginQuirks.h"
#include <WebCore/npruntime_internal.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

class RawPluginMetaData;

class NetscapePluginModule : public RefCounted<NetscapePluginModule> {
public:
    static PassRefPtr<NetscapePluginModule> getOrCreate(const String& pluginPath);
    ~NetscapePluginModule();

    const NPPluginFuncs& pluginFuncs() const { return m_pluginFuncs; }
 
    void incrementLoadCount();
    void decrementLoadCount();

    static bool getPluginInfo(const String& pluginPath, PluginModuleInfo&);

    const PluginQuirks& pluginQuirks() const { return m_pluginQuirks; }

    // Return a list of domains for which the plug-in has data stored.
    Vector<String> sitesWithData();

    // Request that the plug-in clear the site data.
    bool clearSiteData(const String& site, uint64_t flags, uint64_t maxAge);

    Module* module() const { return m_module.get(); }

#if PLUGIN_ARCHITECTURE(MAC)
    static bool createPluginMIMETypesPreferences(const String& pluginPath);
#endif

#if PLUGIN_ARCHITECTURE(X11)
    static bool scanPlugin(const String& pluginPath);
#endif

private:
    explicit NetscapePluginModule(const String& pluginPath);

    void determineQuirks();

#if PLUGIN_ARCHITECTURE(X11)
    bool getPluginInfoForLoadedPlugin(RawPluginMetaData&);
#endif

    bool tryGetSitesWithData(Vector<String>&);
    bool tryClearSiteData(const String& site, uint64_t flags, uint64_t maxAge);

    bool tryLoad();
    bool load();
    void unload();

    void shutdown();

    String m_pluginPath;
    bool m_isInitialized;
    unsigned m_loadCount;

    PluginQuirks m_pluginQuirks;

    NPP_ShutdownProcPtr m_shutdownProcPtr;
    NPPluginFuncs m_pluginFuncs;

    OwnPtr<Module> m_module;
};
    
} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)

#endif // NetscapePluginModule_h
