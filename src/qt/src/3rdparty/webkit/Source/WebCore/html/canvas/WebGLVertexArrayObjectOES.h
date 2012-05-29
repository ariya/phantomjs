/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef WebGLVertexArrayObjectOES_h
#define WebGLVertexArrayObjectOES_h

#include "WebGLBuffer.h"
#include "WebGLObject.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class WebGLVertexArrayObjectOES : public WebGLObject {
public:
    enum VaoType {
        VaoTypeDefault,
        VaoTypeUser,
    };
    
    virtual ~WebGLVertexArrayObjectOES() { deleteObject(); }

    static PassRefPtr<WebGLVertexArrayObjectOES> create(WebGLRenderingContext*, VaoType);
    
    // Cached values for vertex attrib range checks
    struct VertexAttribState {
        VertexAttribState()
            : enabled(false)
            , bytesPerElement(0)
            , size(4)
            , type(GraphicsContext3D::FLOAT)
            , normalized(false)
            , stride(16)
            , originalStride(0)
            , offset(0)
        {
        }
        
        bool enabled;
        RefPtr<WebGLBuffer> bufferBinding;
        GC3Dsizei bytesPerElement;
        GC3Dint size;
        GC3Denum type;
        bool normalized;
        GC3Dsizei stride;
        GC3Dsizei originalStride;
        GC3Dintptr offset;
    };
    
    bool isDefaultObject() const { return m_type == VaoTypeDefault; }
    
    bool hasEverBeenBound() const { return object() && m_hasEverBeenBound; }
    void setHasEverBeenBound() { m_hasEverBeenBound = true; }
    
    PassRefPtr<WebGLBuffer> getElementArrayBuffer() const { return m_boundElementArrayBuffer; }
    void setElementArrayBuffer(PassRefPtr<WebGLBuffer> buffer) { m_boundElementArrayBuffer = buffer; }
    
    VertexAttribState& getVertexAttribState(int index) { return m_vertexAttribState[index]; }

private:
    WebGLVertexArrayObjectOES(WebGLRenderingContext*, VaoType);

    virtual void deleteObjectImpl(Platform3DObject);

    virtual bool isVertexArray() const { return true; }
    
    VaoType m_type;
    bool m_hasEverBeenBound;
    RefPtr<WebGLBuffer> m_boundElementArrayBuffer;
    Vector<VertexAttribState> m_vertexAttribState;
};

} // namespace WebCore

#endif // WebGLVertexArrayObjectOES_h
