/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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

#ifndef Extensions3DOpenGLCommon_h
#define Extensions3DOpenGLCommon_h

#include "Extensions3D.h"

#include "GraphicsContext3D.h"
#include <wtf/HashSet.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class Extensions3DOpenGLCommon : public Extensions3D {
public:
    virtual ~Extensions3DOpenGLCommon();

    // Extensions3D methods.
    virtual bool supports(const String&);
    virtual void ensureEnabled(const String&);
    virtual bool isEnabled(const String&);
    virtual int getGraphicsResetStatusARB();

    virtual Platform3DObject createVertexArrayOES() = 0;
    virtual void deleteVertexArrayOES(Platform3DObject) = 0;
    virtual GC3Dboolean isVertexArrayOES(Platform3DObject) = 0;
    virtual void bindVertexArrayOES(Platform3DObject) = 0;

    virtual void drawBuffersEXT(GC3Dsizei, const GC3Denum*) = 0;

    virtual String getTranslatedShaderSourceANGLE(Platform3DObject);

    // EXT Robustness - uses getGraphicsResetStatusARB()
    virtual void readnPixelsEXT(int x, int y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, GC3Dsizei bufSize, void *data);
    virtual void getnUniformfvEXT(GC3Duint program, int location, GC3Dsizei bufSize, float *params);
    virtual void getnUniformivEXT(GC3Duint program, int location, GC3Dsizei bufSize, int *params);

    virtual bool isNVIDIA() { return m_isNVIDIA; }
    virtual bool isAMD() { return m_isAMD; }
    virtual bool isIntel() { return m_isIntel; }
    virtual String vendor() { return m_vendor; }

    virtual bool maySupportMultisampling() { return m_maySupportMultisampling; }
    virtual bool requiresBuiltInFunctionEmulation() { return m_requiresBuiltInFunctionEmulation; }

protected:
    friend class Extensions3DOpenGLES;
    Extensions3DOpenGLCommon(GraphicsContext3D*);

    virtual bool supportsExtension(const String&) = 0;
    virtual String getExtensions() = 0;

    virtual void initializeAvailableExtensions();
    bool m_initializedAvailableExtensions;
    HashSet<String> m_availableExtensions;

    // Weak pointer back to GraphicsContext3D
    GraphicsContext3D* m_context;

    bool m_isNVIDIA;
    bool m_isAMD;
    bool m_isIntel;
    bool m_maySupportMultisampling;
    bool m_requiresBuiltInFunctionEmulation;

    String m_vendor;
};

} // namespace WebCore

#endif // Extensions3DOpenGLCommon_h
