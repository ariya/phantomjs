/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebGLGetInfo_h
#define WebGLGetInfo_h

#include "Float32Array.h"
#include "Int32Array.h"
#include "PlatformString.h"
#include "Uint8Array.h"
#include "WebGLBuffer.h"
#include "WebGLFramebuffer.h"
#include "WebGLProgram.h"
#include "WebGLRenderbuffer.h"
#include "WebGLTexture.h"
#include "WebGLVertexArrayObjectOES.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

// A tagged union representing the result of get queries like
// getParameter (encompassing getBooleanv, getIntegerv, getFloatv) and
// similar variants. For reference counted types, increments and
// decrements the reference count of the target object.

class WebGLGetInfo {
public:
    enum Type {
        kTypeBool,
        kTypeBoolArray,
        kTypeFloat,
        kTypeInt,
        kTypeNull,
        kTypeString,
        kTypeUnsignedInt,
        kTypeWebGLBuffer,
        kTypeWebGLFloatArray,
        kTypeWebGLFramebuffer,
        kTypeWebGLIntArray,
        kTypeWebGLObjectArray,
        kTypeWebGLProgram,
        kTypeWebGLRenderbuffer,
        kTypeWebGLTexture,
        kTypeWebGLUnsignedByteArray,
        kTypeWebGLVertexArrayObjectOES,
    };

    WebGLGetInfo(bool value);
    WebGLGetInfo(const bool* value, int size);
    WebGLGetInfo(float value);
    WebGLGetInfo(int value);
    // Represents the null value and type.
    WebGLGetInfo();
    WebGLGetInfo(const String& value);
    WebGLGetInfo(unsigned int value);
    WebGLGetInfo(PassRefPtr<WebGLBuffer> value);
    WebGLGetInfo(PassRefPtr<Float32Array> value);
    WebGLGetInfo(PassRefPtr<WebGLFramebuffer> value);
    WebGLGetInfo(PassRefPtr<Int32Array> value);
    // FIXME: implement WebGLObjectArray
    // WebGLGetInfo(PassRefPtr<WebGLObjectArray> value);
    WebGLGetInfo(PassRefPtr<WebGLProgram> value);
    WebGLGetInfo(PassRefPtr<WebGLRenderbuffer> value);
    WebGLGetInfo(PassRefPtr<WebGLTexture> value);
    WebGLGetInfo(PassRefPtr<Uint8Array> value);
    WebGLGetInfo(PassRefPtr<WebGLVertexArrayObjectOES> value);

    virtual ~WebGLGetInfo();

    Type getType() const;

    bool getBool() const;
    const Vector<bool>& getBoolArray() const;
    float getFloat() const;
    int getInt() const;
    const String& getString() const;
    unsigned int getUnsignedInt() const;
    PassRefPtr<WebGLBuffer> getWebGLBuffer() const;
    PassRefPtr<Float32Array> getWebGLFloatArray() const;
    PassRefPtr<WebGLFramebuffer> getWebGLFramebuffer() const;
    PassRefPtr<Int32Array> getWebGLIntArray() const;
    // FIXME: implement WebGLObjectArray
    // PassRefPtr<WebGLObjectArray> getWebGLObjectArray() const;
    PassRefPtr<WebGLProgram> getWebGLProgram() const;
    PassRefPtr<WebGLRenderbuffer> getWebGLRenderbuffer() const;
    PassRefPtr<WebGLTexture> getWebGLTexture() const;
    PassRefPtr<Uint8Array> getWebGLUnsignedByteArray() const;
    PassRefPtr<WebGLVertexArrayObjectOES> getWebGLVertexArrayObjectOES() const;

private:
    Type m_type;
    bool m_bool;
    Vector<bool> m_boolArray;
    float m_float;
    int m_int;
    String m_string;
    unsigned int m_unsignedInt;
    RefPtr<WebGLBuffer> m_webglBuffer;
    RefPtr<Float32Array> m_webglFloatArray;
    RefPtr<WebGLFramebuffer> m_webglFramebuffer;
    RefPtr<Int32Array> m_webglIntArray;
    // FIXME: implement WebGLObjectArray
    // RefPtr<WebGLObjectArray> m_webglObjectArray;
    RefPtr<WebGLProgram> m_webglProgram;
    RefPtr<WebGLRenderbuffer> m_webglRenderbuffer;
    RefPtr<WebGLTexture> m_webglTexture;
    RefPtr<Uint8Array> m_webglUnsignedByteArray;
    RefPtr<WebGLVertexArrayObjectOES> m_webglVertexArrayObject;
};

} // namespace WebCore

#endif // WebGLGetInfo_h
