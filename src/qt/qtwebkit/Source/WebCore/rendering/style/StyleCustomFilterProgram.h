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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
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

#ifndef StyleCustomFilterProgram_h
#define StyleCustomFilterProgram_h

#if ENABLE(CSS_SHADERS)
#include "CachedResourceClient.h"
#include "CachedResourceHandle.h"
#include "CachedShader.h"
#include "CustomFilterProgram.h"
#include "KURL.h"
#include "StyleShader.h"
#include <wtf/FastAllocBase.h>

namespace WebCore {

// CSS Shaders

class StyleCustomFilterProgramCache;

class StyleCustomFilterProgram : public CustomFilterProgram, public CachedResourceClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<StyleCustomFilterProgram> create(KURL vertexShaderURL, PassRefPtr<StyleShader> vertexShader, 
        KURL fragmentShaderURL, PassRefPtr<StyleShader> fragmentShader, CustomFilterProgramType programType,
        const CustomFilterProgramMixSettings& mixSettings, CustomFilterMeshType meshType)
    {
        return adoptRef(new StyleCustomFilterProgram(vertexShaderURL, vertexShader, fragmentShaderURL, fragmentShader, programType, mixSettings, meshType));
    }
    
    void setVertexShader(PassRefPtr<StyleShader> shader)
    {
        // The shader is immutable while in the cache.
        ASSERT(!m_cache);
        m_vertexShader = shader; 
    }
    StyleShader* vertexShader() const { return m_vertexShader.get(); }
    
    void setFragmentShader(PassRefPtr<StyleShader> shader)
    {
        // The shader is immutable while in the cache.
        ASSERT(!m_cache);
        m_fragmentShader = shader; 
    }
    StyleShader* fragmentShader() const { return m_fragmentShader.get(); }
    
    virtual String vertexShaderString() const
    {
        ASSERT(isLoaded());
        return m_cachedVertexShader.get() ? m_cachedVertexShader->shaderString() : String();
    }
    
    virtual String fragmentShaderString() const
    {
        ASSERT(isLoaded());
        return m_cachedFragmentShader.get() ? m_cachedFragmentShader->shaderString() : String();
    }

    virtual bool isLoaded() const
    {
        // Do not use the CachedResource:isLoaded method here, because it actually means !isLoading(),
        // so missing and canceled resources will have isLoaded set to true, even if they are not loaded yet.
        ASSERT(!m_vertexShader || m_vertexShader->isCachedShader());
        ASSERT(!m_fragmentShader || m_fragmentShader->isCachedShader());
        ASSERT(m_cachedVertexShader.get() || m_cachedFragmentShader.get());
        return (!m_cachedVertexShader.get() || m_isVertexShaderLoaded)
            && (!m_cachedFragmentShader.get() || m_isFragmentShaderLoaded);
    }

    virtual void willHaveClients()
    {
        if (m_vertexShader) {
            m_cachedVertexShader = m_vertexShader->cachedShader();
            m_cachedVertexShader->addClient(this);
        }
        if (m_fragmentShader) {
            m_cachedFragmentShader = m_fragmentShader->cachedShader();
            m_cachedFragmentShader->addClient(this);
        }
    }
    
    virtual void didRemoveLastClient()
    {
        if (m_cachedVertexShader.get()) {
            m_cachedVertexShader->removeClient(this);
            m_cachedVertexShader = 0;
            m_isVertexShaderLoaded = false;
        }
        if (m_cachedFragmentShader.get()) {
            m_cachedFragmentShader->removeClient(this);
            m_cachedFragmentShader = 0;
            m_isFragmentShaderLoaded = false;
        }
    }
    
    virtual void notifyFinished(CachedResource* resource)
    {
        if (resource->errorOccurred())
            return;
        // Note that m_cachedVertexShader might be equal to m_cachedFragmentShader and it would only get one event in that case.
        if (resource == m_cachedVertexShader.get())
            m_isVertexShaderLoaded = true;
        if (resource == m_cachedFragmentShader.get())
            m_isFragmentShaderLoaded = true;
        if (isLoaded())
            notifyClients();
    }

    bool hasPendingShaders() const
    {
        return (m_vertexShader && m_vertexShader->isPendingShader()) 
            || (m_fragmentShader && m_fragmentShader->isPendingShader());
    }

    // StyleCustomFilterProgramCache is responsible with updating the reference to the cache.
    void setCache(StyleCustomFilterProgramCache* cache) { m_cache = cache; }
    bool inCache() const { return m_cache; }
    
    KURL vertexShaderURL() const { return m_vertexShaderURL; }
    KURL fragmentShaderURL() const { return m_fragmentShaderURL; }

private:
    StyleCustomFilterProgram(KURL vertexShaderURL, PassRefPtr<StyleShader> vertexShader, KURL fragmentShaderURL, PassRefPtr<StyleShader> fragmentShader, 
        CustomFilterProgramType programType, const CustomFilterProgramMixSettings& mixSettings, CustomFilterMeshType meshType)
        : CustomFilterProgram(programType, mixSettings, meshType)
        , m_vertexShader(vertexShader)
        , m_fragmentShader(fragmentShader)
        , m_vertexShaderURL(vertexShaderURL)
        , m_fragmentShaderURL(fragmentShaderURL)
        , m_cache(0)
        , m_isVertexShaderLoaded(false)
        , m_isFragmentShaderLoaded(false)
    {
    }

    ~StyleCustomFilterProgram();
    
    RefPtr<StyleShader> m_vertexShader;
    RefPtr<StyleShader> m_fragmentShader;

    CachedResourceHandle<CachedShader> m_cachedVertexShader;
    CachedResourceHandle<CachedShader> m_cachedFragmentShader;

    // The URLs form the key of the StyleCustomFilterProgram in the cache and are used
    // to lookup the StyleCustomFilterProgram when it's removed from the cache.
    KURL m_vertexShaderURL;
    KURL m_fragmentShaderURL;

    // The Cache is responsible of invalidating this reference.
    StyleCustomFilterProgramCache* m_cache;
    
    bool m_isVertexShaderLoaded;
    bool m_isFragmentShaderLoaded;
};

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)

#endif // StyleCustomFilterProgram_h
