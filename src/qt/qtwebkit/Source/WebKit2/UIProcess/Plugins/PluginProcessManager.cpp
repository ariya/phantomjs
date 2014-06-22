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

#include "config.h"
#include "PluginProcessManager.h"

#if ENABLE(PLUGIN_PROCESS)

#include "PluginProcessProxy.h"
#include "WebContext.h"
#include <wtf/CryptographicallyRandomNumber.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

PluginProcessManager& PluginProcessManager::shared()
{
    DEFINE_STATIC_LOCAL(PluginProcessManager, pluginProcessManager, ());
    return pluginProcessManager;
}

PluginProcessManager::PluginProcessManager()
{
}

uint64_t PluginProcessManager::pluginProcessToken(const PluginModuleInfo& pluginModuleInfo, PluginProcessType pluginProcessType, PluginProcessSandboxPolicy pluginProcessSandboxPolicy)
{
    // See if we know this token already.
    for (size_t i = 0; i < m_pluginProcessTokens.size(); ++i) {
        const PluginProcessAttributes& attributes = m_pluginProcessTokens[i].first;

        if (attributes.moduleInfo.path == pluginModuleInfo.path
            && attributes.processType == pluginProcessType
            && attributes.sandboxPolicy == pluginProcessSandboxPolicy)
            return m_pluginProcessTokens[i].second;
    }

    uint64_t token;
    while (true) {
        cryptographicallyRandomValues(&token, sizeof(token));

        if (m_knownTokens.isValidValue(token) && !m_knownTokens.contains(token))
            break;
    }

    PluginProcessAttributes attributes;
    attributes.moduleInfo = pluginModuleInfo;
    attributes.processType = pluginProcessType;
    attributes.sandboxPolicy = pluginProcessSandboxPolicy;

#if COMPILER_SUPPORTS(CXX_RVALUE_REFERENCES)
    m_pluginProcessTokens.append(std::make_pair(std::move(attributes), token));
#else
    m_pluginProcessTokens.append(std::make_pair(attributes, token));
#endif
    m_knownTokens.add(token);

    return token;
}

void PluginProcessManager::getPluginProcessConnection(uint64_t pluginProcessToken, PassRefPtr<Messages::WebProcessProxy::GetPluginProcessConnection::DelayedReply> reply)
{
    ASSERT(pluginProcessToken);

    PluginProcessProxy* pluginProcess = getOrCreatePluginProcess(pluginProcessToken);
    pluginProcess->getPluginProcessConnection(reply);
}

void PluginProcessManager::removePluginProcessProxy(PluginProcessProxy* pluginProcessProxy)
{
    size_t vectorIndex = m_pluginProcesses.find(pluginProcessProxy);
    ASSERT(vectorIndex != notFound);

    m_pluginProcesses.remove(vectorIndex);
}

void PluginProcessManager::getSitesWithData(const PluginModuleInfo& plugin, WebPluginSiteDataManager* webPluginSiteDataManager, uint64_t callbackID)
{
    PluginProcessProxy* pluginProcess = getOrCreatePluginProcess(pluginProcessToken(plugin, PluginProcessTypeNormal, PluginProcessSandboxPolicyNormal));
    pluginProcess->getSitesWithData(webPluginSiteDataManager, callbackID);
}

void PluginProcessManager::clearSiteData(const PluginModuleInfo& plugin, WebPluginSiteDataManager* webPluginSiteDataManager, const Vector<String>& sites, uint64_t flags, uint64_t maxAgeInSeconds, uint64_t callbackID)
{
    PluginProcessProxy* pluginProcess = getOrCreatePluginProcess(pluginProcessToken(plugin, PluginProcessTypeNormal, PluginProcessSandboxPolicyNormal));
    pluginProcess->clearSiteData(webPluginSiteDataManager, sites, flags, maxAgeInSeconds, callbackID);
}

PluginProcessProxy* PluginProcessManager::getOrCreatePluginProcess(uint64_t pluginProcessToken)
{
    for (size_t i = 0; i < m_pluginProcesses.size(); ++i) {
        if (m_pluginProcesses[i]->pluginProcessToken() == pluginProcessToken)
            return m_pluginProcesses[i].get();
    }

    for (size_t i = 0; i < m_pluginProcessTokens.size(); ++i) {
        std::pair<PluginProcessAttributes, uint64_t>& attributesAndToken = m_pluginProcessTokens[i];
        if (attributesAndToken.second == pluginProcessToken) {
            RefPtr<PluginProcessProxy> pluginProcess = PluginProcessProxy::create(this, attributesAndToken.first, attributesAndToken.second);
            PluginProcessProxy* pluginProcessPtr = pluginProcess.get();

            m_pluginProcesses.append(pluginProcess.release());
            return pluginProcessPtr;
        }
    }

    return 0;
}

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
