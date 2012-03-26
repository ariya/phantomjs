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

#include "ArrayBufferView.h"
#include "CheckedInt.h"
#include "WebGLRenderingContext.h"

namespace WebCore {

PassRefPtr<WebGLBuffer> WebGLBuffer::create(WebGLRenderingContext* ctx)
{
    return adoptRef(new WebGLBuffer(ctx));
}

WebGLBuffer::WebGLBuffer(WebGLRenderingContext* ctx)
    : WebGLObject(ctx)
    , m_target(0)
    , m_byteLength(0)
    , m_nextAvailableCacheEntry(0)
{
    setObject(context()->graphicsContext3D()->createBuffer());
    clearCachedMaxIndices();
}

void WebGLBuffer::deleteObjectImpl(Platform3DObject object)
{
    context()->graphicsContext3D()->deleteBuffer(object);
}

bool WebGLBuffer::associateBufferDataImpl(ArrayBuffer* array, GC3Dintptr byteOffset, GC3Dsizeiptr byteLength)
{
    if (byteLength < 0 || byteOffset < 0)
        return false;

    if (array && byteLength) {
        CheckedInt<GC3Dintptr> checkedOffset(byteOffset);
        CheckedInt<GC3Dsizeiptr> checkedLength(byteLength);
        CheckedInt<GC3Dintptr> checkedMax = checkedOffset + checkedLength;
        if (!checkedMax.valid() || checkedMax.value() > static_cast<int32_t>(array->byteLength()))
            return false;
    }

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
            if (array) {
                // We must always clone the incoming data because client-side
                // modifications without calling bufferData or bufferSubData
                // must never be able to change the validation results.
                memcpy(static_cast<unsigned char*>(m_elementArrayBuffer->data()),
                       static_cast<unsigned char*>(array->data()) + byteOffset,
                       byteLength);
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
    if (size < 0)
        return false;
    return associateBufferDataImpl(0, 0, size);
}

bool WebGLBuffer::associateBufferData(ArrayBuffer* array)
{
    if (!array)
        return false;
    return associateBufferDataImpl(array, 0, array->byteLength());
}

bool WebGLBuffer::associateBufferData(ArrayBufferView* array)
{
    if (!array)
        return false;
    return associateBufferDataImpl(array->buffer().get(), array->byteOffset(), array->byteLength());
}

bool WebGLBuffer::associateBufferSubDataImpl(GC3Dintptr offset, ArrayBuffer* array, GC3Dintptr arrayByteOffset, GC3Dsizeiptr byteLength)
{
    if (!array || offset < 0 || arrayByteOffset < 0 || byteLength < 0)
        return false;

    if (byteLength) {
        CheckedInt<GC3Dintptr> checkedBufferOffset(offset);
        CheckedInt<GC3Dintptr> checkedArrayOffset(arrayByteOffset);
        CheckedInt<GC3Dsizeiptr> checkedLength(byteLength);
        CheckedInt<GC3Dintptr> checkedArrayMax = checkedArrayOffset + checkedLength;
        CheckedInt<GC3Dintptr> checkedBufferMax = checkedBufferOffset + checkedLength;
        if (!checkedArrayMax.valid() || checkedArrayMax.value() > static_cast<int32_t>(array->byteLength()) || !checkedBufferMax.valid() || checkedBufferMax.value() > m_byteLength)
            return false;
    }

    switch (m_target) {
    case GraphicsContext3D::ELEMENT_ARRAY_BUFFER:
        clearCachedMaxIndices();
        if (byteLength) {
            if (!m_elementArrayBuffer)
                return false;
            memcpy(static_cast<unsigned char*>(m_elementArrayBuffer->data()) + offset,
                   static_cast<unsigned char*>(array->data()) + arrayByteOffset,
                   byteLength);
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
    return associateBufferSubDataImpl(offset, array, 0, array->byteLength());
}

bool WebGLBuffer::associateBufferSubData(GC3Dintptr offset, ArrayBufferView* array)
{
    if (!array)
        return false;
    return associateBufferSubDataImpl(offset, array->buffer().get(), array->byteOffset(), array->byteLength());
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
