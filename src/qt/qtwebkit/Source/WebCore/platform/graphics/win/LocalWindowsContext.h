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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef LocalWindowsContext_h
#define LocalWindowsContext_h

#include "config.h"
#include "GraphicsContext.h"

namespace WebCore {

class LocalWindowsContext {
    WTF_MAKE_NONCOPYABLE(LocalWindowsContext);
public:
    LocalWindowsContext(GraphicsContext* graphicsContext, const IntRect& rect, bool supportAlphaBlend = true, bool mayCreateBitmap = true)
        : m_graphicsContext(graphicsContext)
        , m_rect(rect)
        , m_supportAlphaBlend(supportAlphaBlend)
        , m_mayCreateBitmap(mayCreateBitmap)
    {
        m_hdc = m_graphicsContext->getWindowsContext(m_rect, m_supportAlphaBlend, m_mayCreateBitmap);
    }

    ~LocalWindowsContext()
    {
        m_graphicsContext->releaseWindowsContext(m_hdc, m_rect, m_supportAlphaBlend, m_mayCreateBitmap);
    }

    HDC hdc() const { return m_hdc; }

private:
    GraphicsContext* m_graphicsContext;
    HDC m_hdc;
    IntRect m_rect;
    bool m_supportAlphaBlend;
    bool m_mayCreateBitmap;
};

} // namespace WebCore
#endif // LocalWindowsContext_h
