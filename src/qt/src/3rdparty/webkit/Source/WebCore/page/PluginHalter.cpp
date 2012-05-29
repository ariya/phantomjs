/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "PluginHalter.h"

#include "HaltablePlugin.h"
#include "PlatformString.h"
#include <wtf/CurrentTime.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

using namespace std;

namespace WebCore {

PluginHalter::PluginHalter(PassOwnPtr<PluginHalterClient> client)
    : m_client(client)
    , m_timer(this, &PluginHalter::timerFired)
    , m_pluginAllowedRunTime(numeric_limits<unsigned>::max())
{
    ASSERT_ARG(m_client, m_client);
}

void PluginHalter::didStartPlugin(HaltablePlugin* obj)
{
    ASSERT_ARG(obj, obj);
    ASSERT_ARG(obj, !m_plugins.contains(obj));

    if (!m_client->enabled())
        return;

    double currentTime = WTF::currentTime();

    m_plugins.add(obj, currentTime);

    if (m_plugins.size() == 1)
        m_oldestStartTime = currentTime;

    startTimerIfNecessary();
}

void PluginHalter::didStopPlugin(HaltablePlugin* obj)
{
    if (!m_client->enabled())
        return;

    m_plugins.remove(obj);
}

void PluginHalter::timerFired(Timer<PluginHalter>*)
{
    if (m_plugins.isEmpty())
        return;

    Vector<HaltablePlugin*> plugins;
    copyKeysToVector(m_plugins, plugins);

    // Plug-ins older than this are candidates to be halted.
    double pluginCutOffTime = WTF::currentTime() - m_pluginAllowedRunTime;

    m_oldestStartTime = numeric_limits<double>::max();

    for (size_t i = 0; i < plugins.size(); ++i) {
        double thisStartTime = m_plugins.get(plugins[i]);
        if (thisStartTime > pluginCutOffTime) {
            // This plug-in is too young to be halted. We find the oldest
            // plug-in that is not old enough to be halted and use it to set
            // the timer's next fire time.
            if (thisStartTime < m_oldestStartTime)
                m_oldestStartTime = thisStartTime;
            continue;
        }

        if (m_client->shouldHaltPlugin(plugins[i]->node(), plugins[i]->isWindowed(), plugins[i]->pluginName()))
            plugins[i]->halt();

        m_plugins.remove(plugins[i]);
    }

    startTimerIfNecessary();
}

void PluginHalter::startTimerIfNecessary()
{
    if (m_timer.isActive())
        return;

    if (m_plugins.isEmpty())
        return;

    double nextFireInterval = static_cast<double>(m_pluginAllowedRunTime) - (currentTime() - m_oldestStartTime);
    m_timer.startOneShot(nextFireInterval < 0 ? 0 : nextFireInterval);
}

} // namespace WebCore
