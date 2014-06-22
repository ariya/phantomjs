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

#ifndef PluginProcessManager_h
#define PluginProcessManager_h

#if ENABLE(PLUGIN_PROCESS)

#include "PluginModuleInfo.h"
#include "PluginProcess.h"
#include "PluginProcessAttributes.h"
#include "WebProcessProxyMessages.h"
#include <wtf/Forward.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

namespace CoreIPC {
    class ArgumentEncoder;
}

namespace WebKit {

class PluginInfoStore;
class PluginProcessProxy;
class WebProcessProxy;
class WebPluginSiteDataManager;

class PluginProcessManager {
    WTF_MAKE_NONCOPYABLE(PluginProcessManager);
public:
    static PluginProcessManager& shared();

    uint64_t pluginProcessToken(const PluginModuleInfo&, PluginProcessType, PluginProcessSandboxPolicy);

    void getPluginProcessConnection(uint64_t pluginProcessToken, PassRefPtr<Messages::WebProcessProxy::GetPluginProcessConnection::DelayedReply>);
    void removePluginProcessProxy(PluginProcessProxy*);

    void getSitesWithData(const PluginModuleInfo&, WebPluginSiteDataManager*, uint64_t callbackID);
    void clearSiteData(const PluginModuleInfo&, WebPluginSiteDataManager*, const Vector<String>& sites, uint64_t flags, uint64_t maxAgeInSeconds, uint64_t callbackID);

#if PLATFORM(MAC)
    void setProcessSuppressionEnabled(bool);
#endif

private:
    PluginProcessManager();

    PluginProcessProxy* getOrCreatePluginProcess(uint64_t pluginProcessToken);

    Vector<std::pair<PluginProcessAttributes, uint64_t> > m_pluginProcessTokens;
    HashSet<uint64_t> m_knownTokens;

    Vector<RefPtr<PluginProcessProxy> > m_pluginProcesses;
};

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)

#endif // PluginProcessManager_h
