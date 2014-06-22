/*
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
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

#ifndef StyleShader_h
#define StyleShader_h

#if ENABLE(CSS_SHADERS)

#include <wtf/RefCounted.h>

namespace WebCore {

class CachedShader;
class CSSValue;

class StyleShader : public RefCounted<StyleShader> {
public:
    virtual ~StyleShader() { }

    ALWAYS_INLINE bool isCachedShader() const { return m_isCachedShader; }
    ALWAYS_INLINE bool isPendingShader() const { return m_isPendingShader; }
    
    virtual PassRefPtr<CSSValue> cssValue() const = 0;
    
    virtual CachedShader* cachedShader() const { return 0; }
    
protected:
    StyleShader()
        : m_isCachedShader(false)
        , m_isPendingShader(false)
    {
    }
    bool m_isCachedShader : 1;
    bool m_isPendingShader : 1;
};

}

#endif // ENABLE(CSS_SHADERS)

#endif // StyleShader_h
