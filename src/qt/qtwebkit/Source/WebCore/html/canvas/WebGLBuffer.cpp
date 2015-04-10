/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(WEBGL)

#include "WebGLBuffer.h"

#include "WebGLContextGroup.h"
#include "WebGLRenderingContext.h"

namespace WebCore {

PassRefPtr<WebGLBuffer> WebGLBuffer::create(WebGLRenderingContext* ctx)
{
    return adoptRef(new WebGLBuffer(ctx));
}

WebGLBuffer::WebGLBuffer(WebGLRenderingContext* ctx)
    : WebGLSharedObject(ctx)
    , m_target(0)
    , m_byteLength(0)
    , m_nextAvailableCacheEntry(0)
{
    setObject(ctx->graphicsContext3D()->createBuffer());
    clearCachedMaxIndices();
}

WebGLBuffer::~WebGLBuffer()
{
    deleteObject(0);
}

void WebGLBuffer::deleteObjectImpl(GraphicsContext3D* context3d, Platform3DObject object)
{
      context3d->deleteBuffer(object);
}

bool WebGLBuffer::associateBufferDataImpl(const void* data, GC3Dsizeiptr byteLength)
{
    if (byteLength < 0)
        return false;

    switch (m_target) {
    case GraphicsContext3D::ELEMENT_ARRAY_BUFFER:
        m_byteLength = byteLength;
        clearCachedMaxIndices();
        if (byteLength) {
            m_elementArrayBuffer = ArrayBuffer::create(byteLength, 1);
            if (!m_elementArrayBuffer) {
                m_byteLength = 0;
                return false;
            }
            if (data) {
                // We must always clone the incoming data because client-side
                // modifications without calling bufferData or bufferSubData
                // must never be able to change the validation results.
                memcpy(m_elementArrayBuffer->data(), data, byteLength);
            }
        } else
            m_elementArrayBuffer = 0;
        return true;
    case GraphicsContext3D::ARRAY_BUFFER:
        m_byteLength = byteLength;
        return true;
    default:
        return false;
    }
}

bool WebGLBuffer::associateBufferData(GC3Dsizeiptr size)
{
    return associateBufferDataImpl(0, size);
}

bool WebGLBuffer::associateBufferData(ArrayBuffer* array)
{
    if (!array)
        return false;
    return associateBufferDataImpl(array ? array->data() : 0, array ? array->byteLength() : 0);
}

bool WebGLBuffer::associateBufferData(ArrayBufferView* array)
{
    if (!array)
        return false;
    return associateBufferDataImpl(array ? array->baseAddress() : 0, array ? array->byteLength() : 0);
}

bool WebGLBuffer::associateBufferSubDataImpl(GC3Dintptr offset, const void* data, GC3Dsizeiptr byteLength)
{
    if (!data || offset < 0 || byteLength < 0)
        return false;

    if (byteLength) {
        Checked<GC3Dintptr, RecordOverflow> checkedBufferOffset(offset);
        Checked<GC3Dsizeiptr, RecordOverflow> checkedDataLength(byteLength);
        Checked<GC3Dintptr, RecordOverflow> checkedBufferMax = checkedBufferOffset + checkedDataLength;
        if (checkedBufferMax.hasOverflowed() || offset > m_byteLength || checkedBufferMax.unsafeGet() > m_byteLength)
            return false;
    }

    switch (m_target) {
    case GraphicsContext3D::ELEMENT_ARRAY_BUFFER:
        clearCachedMaxIndices();
        if (byteLength) {
            if (!m_elementArrayBuffer)
                return false;
            memcpy(static_cast<unsigned char*>(m_elementArrayBuffer->data()) + offset, data, byteLength);
        }
        return true;
    case GraphicsContext3D::ARRAY_BUFFER:
        return true;
    default:
        return false;
    }
}

bool WebGLBuffer::associateBufferSubData(GC3Dintptr offset, ArrayBuffer* array)
{
    if (!array)
        return false;
    return associateBufferSubDataImpl(offset, array->data(), array->byteLength());
}

bool WebGLBuffer::associateBufferSubData(GC3Dintptr offset, ArrayBufferView* array)
{
    if (!array)
        return false;
    return associateBufferSubDataImpl(offset, array->baseAddress(), array->byteLength());
}

GC3Dsizeiptr WebGLBuffer::byteLength() const
{
    return m_byteLength;
}

int WebGLBuffer::getCachedMaxIndex(GC3Denum type)
{
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(m_maxIndexCache); ++i)
        if (m_maxIndexCache[i].type == type)
            return m_maxIndexCache[i].maxIndex;
    return -1;
}

void WebGLBuffer::setCachedMaxIndex(GC3Denum type, int value)
{
    size_t numEntries = WTF_ARRAY_LENGTH(m_maxIndexCache);
    for (size_t i = 0; i < numEntries; ++i)
        if (m_maxIndexCache[i].type == type) {
            m_maxIndexCache[i].maxIndex = value;
            return;
        }
    m_maxIndexCache[m_nextAvailableCacheEntry].type = type;
    m_maxIndexCache[m_nextAvailableCacheEntry].maxIndex = value;
    m_nextAvailableCacheEntry = (m_nextAvailableCacheEntry + 1) % numEntries;
}

void WebGLBuffer::setTarget(GC3Denum target)
{
    // In WebGL, a buffer is bound to one target in its lifetime
    if (m_target)
        return;
    if (target == GraphicsContext3D::ARRAY_BUFFER || target == GraphicsContext3D::ELEMENT_ARRAY_BUFFER)
        m_target = target;
}

void WebGLBuffer::clearCachedMaxIndices()
{
    memset(m_maxIndexCache, 0, sizeof(m_maxIndexCache));
}

}

#endif // ENABLE(WEBGL)
