/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef CustomProtocolManagerProxy_h
#define CustomProtocolManagerProxy_h

#if ENABLE(CUSTOM_PROTOCOLS)

#include "MessageReceiver.h"

#if PLATFORM(MAC)
#include <wtf/HashMap.h>
#include <wtf/RetainPtr.h>
OBJC_CLASS WKCustomProtocolLoader;
#endif

namespace WebCore {
class ResourceRequest;
} // namespace WebCore

namespace WebKit {

class ChildProcessProxy;

class CustomProtocolManagerProxy : public CoreIPC::MessageReceiver {
public:
    explicit CustomProtocolManagerProxy(ChildProcessProxy*);

    void startLoading(uint64_t customProtocolID, const WebCore::ResourceRequest&);
    void stopLoading(uint64_t customProtocolID);

private:
    // CoreIPC::MessageReceiver
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;

    ChildProcessProxy* m_childProcessProxy;

#if PLATFORM(MAC)
    typedef HashMap<uint64_t, RetainPtr<WKCustomProtocolLoader>> LoaderMap;
    LoaderMap m_loaderMap;
#endif
};

} // namespace WebKit

#endif // ENABLE(CUSTOM_PROTOCOLS)

#endif // CustomProtocolManagerProxy_h
