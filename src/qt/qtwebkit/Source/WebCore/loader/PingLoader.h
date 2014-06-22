/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef PingLoader_h
#define PingLoader_h

#include "ResourceHandleClient.h"
#include "Timer.h"
#include <wtf/Noncopyable.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class FormData;
class Frame;
class KURL;
class ResourceError;
class ResourceHandle;
class ResourceResponse;

// This class triggers asynchronous loads independent of Frame staying alive (i.e., auditing pingbacks).
// Since nothing depends on resources loaded through this class, we just want
// to allow the load to live long enough to ensure the message was actually sent.
// Therefore, as soon as a callback is received from the ResourceHandle, this class 
// will cancel the load and delete itself.
class PingLoader : private ResourceHandleClient {
    WTF_MAKE_NONCOPYABLE(PingLoader); WTF_MAKE_FAST_ALLOCATED;
public:
    static void loadImage(Frame*, const KURL& url);
    static void sendPing(Frame*, const KURL& pingURL, const KURL& destinationURL);
    static void sendViolationReport(Frame*, const KURL& reportURL, PassRefPtr<FormData> report);

    virtual ~PingLoader();

private:
    PingLoader(Frame*, ResourceRequest&);

    virtual void didReceiveResponse(ResourceHandle*, const ResourceResponse&) OVERRIDE { delete this; }
    virtual void didReceiveData(ResourceHandle*, const char*, int, int) OVERRIDE { delete this; }
    virtual void didFinishLoading(ResourceHandle*, double) OVERRIDE { delete this; }
    virtual void didFail(ResourceHandle*, const ResourceError&) OVERRIDE { delete this; }
    virtual bool shouldUseCredentialStorage(ResourceHandle*)  OVERRIDE { return m_shouldUseCredentialStorage; }
    void timeout(Timer<PingLoader>*) { delete this; }

    RefPtr<ResourceHandle> m_handle;
    Timer<PingLoader> m_timeout;
    bool m_shouldUseCredentialStorage;
};

}

#endif
