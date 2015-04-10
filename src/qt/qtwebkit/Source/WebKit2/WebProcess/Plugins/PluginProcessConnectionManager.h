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

#ifndef PluginProcessConnectionManager_h
#define PluginProcessConnectionManager_h

#if ENABLE(PLUGIN_PROCESS)

#include "PluginProcess.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>

// Manages plug-in process connections for the given web process.

namespace WebKit {

class PluginProcessConnection;
        
class PluginProcessConnectionManager : public CoreIPC::Connection::WorkQueueMessageReceiver {
public:
    static PassRefPtr<PluginProcessConnectionManager> create();
    ~PluginProcessConnectionManager();

    void initializeConnection(CoreIPC::Connection*);

    PluginProcessConnection* getPluginProcessConnection(uint64_t pluginProcessToken);
    void removePluginProcessConnection(PluginProcessConnection*);

    void didReceivePluginProcessConnectionManagerMessageOnConnectionWorkQueue(CoreIPC::Connection*, OwnPtr<CoreIPC::MessageDecoder>&);

private:
    PluginProcessConnectionManager();

    // CoreIPC::Connection::WorkQueueMessageReceiver.
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;

    void pluginProcessCrashed(uint64_t pluginProcessToken);

    RefPtr<WorkQueue> m_queue;

    Vector<RefPtr<PluginProcessConnection> > m_pluginProcessConnections;

    Mutex m_tokensAndConnectionsMutex;
    HashMap<uint64_t, RefPtr<CoreIPC::Connection> > m_tokensAndConnections;
};

}

#endif // ENABLE(PLUGIN_PROCESS)

#endif // PluginProcessConnectionManager_h
