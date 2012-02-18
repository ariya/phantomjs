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

#ifndef PluginHalter_h
#define PluginHalter_h

#include "PluginHalterClient.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

class HaltablePlugin;

class PluginHalter {
    WTF_MAKE_NONCOPYABLE(PluginHalter); WTF_MAKE_FAST_ALLOCATED;
public:
    PluginHalter(PassOwnPtr<PluginHalterClient>);

    void didStartPlugin(HaltablePlugin*);
    void didStopPlugin(HaltablePlugin*);

    void setPluginAllowedRunTime(unsigned runTime) { m_pluginAllowedRunTime = runTime; }

private:
    void timerFired(Timer<PluginHalter>*);
    void startTimerIfNecessary();

    OwnPtr<PluginHalterClient> m_client;
    Timer<PluginHalter> m_timer;
    unsigned m_pluginAllowedRunTime;
    double m_oldestStartTime;
    HashMap<HaltablePlugin*, double> m_plugins;
};

} // namespace WebCore

#endif // PluginHalter_h
