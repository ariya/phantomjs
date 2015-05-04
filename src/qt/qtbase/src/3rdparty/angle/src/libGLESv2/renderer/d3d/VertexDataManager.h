//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexDataManager.h: Defines the VertexDataManager, a class that
// runs the Buffer translation process.

#ifndef LIBGLESV2_RENDERER_VERTEXDATAMANAGER_H_
#define LIBGLESV2_RENDERER_VERTEXDATAMANAGER_H_

#include "libGLESv2/Constants.h"
#include "libGLESv2/VertexAttribute.h"
#include "common/angleutils.h"

namespace gl
{
class ProgramBinary;
class State;
struct VertexAttribute;
struct VertexAttribCurrentValueData;
}

namespace rx
{
class BufferD3D;
class StreamingVertexBufferInterface;
class VertexBuffer;
class RendererD3D;

struct TranslatedAttribute
{
    TranslatedAttribute() : active(false), attribute(NULL), currentValueType(GL_NONE),
                            offset(0), stride(0), vertexBuffer(NULL), storage(NULL),
                            serial(0), divisor(0) {};
    bool active;

    const gl::VertexAttribute *attribute;
    GLenum currentValueType;
    unsigned int offset;
    unsigned int stride;   // 0 means not to advance the read pointer at all

    VertexBuffer *vertexBuffer;
    BufferD3D *storage;
    unsigned int serial;
    unsigned int divisor;
};

class VertexDataManager
{
  public:
    VertexDataManager(RendererD3D *renderer);
    virtual ~VertexDataManager();

    gl::Error prepareVertexData(const gl::State &state, GLint start, GLsizei count,
                                TranslatedAttribute *outAttribs, GLsizei instances);

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexDataManager);

    gl::Error reserveSpaceForAttrib(const gl::VertexAttribute &attrib,
                                    const gl::VertexAttribCurrentValueData &currentValue,
                                    GLsizei count,
                                    GLsizei instances) const;

    void invalidateMatchingStaticData(const gl::VertexAttribute &attrib,
                                      const gl::VertexAttribCurrentValueData &currentValue) const;

    gl::Error storeAttribute(const gl::VertexAttribute &attrib,
                             const gl::VertexAttribCurrentValueData &currentValue,
                             TranslatedAttribute *translated,
                             GLint start,
                             GLsizei count,
                             GLsizei instances);

    gl::Error storeCurrentValue(const gl::VertexAttribute &attrib,
                                const gl::VertexAttribCurrentValueData &currentValue,
                                TranslatedAttribute *translated,
                                gl::VertexAttribCurrentValueData *cachedValue,
                                size_t *cachedOffset,
                                StreamingVertexBufferInterface *buffer);

    RendererD3D *const mRenderer;

    StreamingVertexBufferInterface *mStreamingBuffer;

    gl::VertexAttribCurrentValueData mCurrentValue[gl::MAX_VERTEX_ATTRIBS];

    StreamingVertexBufferInterface *mCurrentValueBuffer[gl::MAX_VERTEX_ATTRIBS];
    std::size_t mCurrentValueOffsets[gl::MAX_VERTEX_ATTRIBS];
};

}

#endif   // LIBGLESV2_RENDERER_VERTEXDATAMANAGER_H_
