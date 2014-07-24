/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"

#include "SharedGraphicsContext3D.h"

#include "Extensions3D.h"
#include <wtf/MainThread.h>

namespace WebCore {

class SharedGraphicsContext3DImpl {
public:
    SharedGraphicsContext3DImpl() : m_context(0) { }
    PassRefPtr<GraphicsContext3D> getOrCreateContext()
    {
        bool wasCreated = false;

        // If we lost the context, or can't make it current, create a new one.
        if (m_context && (!m_context->makeContextCurrent() || (m_context->getExtensions()->getGraphicsResetStatusARB() != GraphicsContext3D::NO_ERROR)))
            m_context.clear();

        if (!m_context) {
            createContext();
            wasCreated = true;
        }

        if (m_context && !m_context->makeContextCurrent())
            m_context.clear();

        if (m_context && wasCreated)
            m_context->getExtensions()->pushGroupMarkerEXT("SharedGraphicsContext");
        return m_context;
    }

    PassRefPtr<GraphicsContext3D> getContext()
    {
        return m_context;
    }

    PassRefPtr<GraphicsContext3D> createContext()
    {
        GraphicsContext3D::Attributes attributes;
        attributes.depth = false;
        attributes.stencil = true;
        attributes.antialias = false;
        attributes.shareResources = true;
        m_context = GraphicsContext3D::create(attributes, 0);
        return m_context;
    }
private:
    RefPtr<GraphicsContext3D> m_context;
};

PassRefPtr<GraphicsContext3D> SharedGraphicsContext3D::get()
{
    DEFINE_STATIC_LOCAL(SharedGraphicsContext3DImpl, impl, ());
    return impl.getOrCreateContext();
}

enum ContextOperation {
    Get, Create
};

static PassRefPtr<GraphicsContext3D> getOrCreateContextForImplThread(ContextOperation op)
{
    DEFINE_STATIC_LOCAL(SharedGraphicsContext3DImpl, impl, ());
    return op == Create ? impl.createContext() : impl.getContext();
}

PassRefPtr<GraphicsContext3D> SharedGraphicsContext3D::getForImplThread()
{
    return getOrCreateContextForImplThread(Get);
}

bool SharedGraphicsContext3D::haveForImplThread()
{
    ASSERT(isMainThread());
    return getOrCreateContextForImplThread(Get);
}

bool SharedGraphicsContext3D::createForImplThread()
{
    ASSERT(isMainThread());
    return getOrCreateContextForImplThread(Create);
}

}

