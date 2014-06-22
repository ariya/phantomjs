/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef WebCustomFilterProgramProxy_h
#define WebCustomFilterProgramProxy_h

#if USE(COORDINATED_GRAPHICS) && ENABLE(CSS_SHADERS)

#include <WebCore/TextureMapperPlatformCompiledProgram.h>
#include <wtf/RefCounted.h>

namespace WebKit {

class WebCustomFilterProgramProxy;

class WebCustomFilterProgramProxyClient {
public:
    virtual void removeCustomFilterProgramProxy(WebCustomFilterProgramProxy*) = 0;
};

// This is a proxy class used to store the ID of the custom filter program serialized to the other process.
// It lives in the WebProcess and is referenced from the CustomFilterValidatedProgram meaning that it will be kept alive as
// long as a layer on the page will render with this program. It will call removeCustomFilterProgramProxy on the m_client
// when the program is no longer needed to render the filter. The client can then send a message to the UI process
// to destroy the associated reference. Note that more layers can share the same program and there's
// no need to implement a caching mechanism in the compositor side.

class WebCustomFilterProgramProxy : public RefCounted<WebCustomFilterProgramProxy>, public WebCore::TextureMapperPlatformCompiledProgramClient {
public:
    using RefCounted<WebCustomFilterProgramProxy>::ref;
    using RefCounted<WebCustomFilterProgramProxy>::deref;

    static PassRefPtr<WebCustomFilterProgramProxy> create()
    {
        return adoptRef(new WebCustomFilterProgramProxy());
    }

    int id() const { return m_id; }

    // Needed to make TextureMapperPlatformCompiledProgramClient look like a RefCounted object.
    virtual void refFromValidatedProgram() { ref(); }
    virtual void derefFromValidatedProgram() { deref(); }

    virtual ~WebCustomFilterProgramProxy();
    
    void setClient(WebCustomFilterProgramProxyClient* client) { m_client = client; }
    WebCustomFilterProgramProxyClient* client() const { return m_client; }

private:
    WebCustomFilterProgramProxy()
        : m_client(0)
        , m_id(s_nextId++)
    {
    }
    
    WebCustomFilterProgramProxyClient* m_client;
    int m_id;
    
    static int s_nextId;
};

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS) && ENABLE(CSS_SHADERS)

#endif // WebCustomFilterProgramProxy_h

