/*
 * Copyright (C) 2010, 2012 Apple Inc. All rights reserved.
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

#ifndef SharedWorkerProcessManager_h
#define SharedWorkerProcessManager_h

#if ENABLE(SHARED_WORKER_PROCESS)

#include "PluginModuleInfo.h"
#include "WebProcessProxyMessages.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

namespace CoreIPC {
    class ArgumentEncoder;
}

namespace WebKit {

class PluginInfoStore;
class SharedWorkerProcessProxy;
class WebProcessProxy;
class WebPluginSiteDataManager;

class SharedWorkerProcessManager {
    WTF_MAKE_NONCOPYABLE(SharedWorkerProcessManager);
public:
    static SharedWorkerProcessManager& shared();

    void getSharedWorkerProcessConnection(const String& url, const String& name, PassRefPtr<Messages::WebProcessProxy::GetSharedWorkerProcessConnection::DelayedReply>);
    void removeSharedWorkerProcessProxy(SharedWorkerProcessProxy*);

#if PLATFORM(MAC)
    void setProcessSuppressionEnabled(bool);
#endif

private:
    SharedWorkerProcessManager();

    SharedWorkerProcessProxy* getOrCreateSharedWorkerProcess(const String& url, const String& name);

    Vector<RefPtr<SharedWorkerProcessProxy> > m_sharedWorkerProcesses;
};

} // namespace WebKit

#endif // ENABLE(SHARED_WORKER_PROCESS)

#endif // SharedWorkerProcessManager_h
