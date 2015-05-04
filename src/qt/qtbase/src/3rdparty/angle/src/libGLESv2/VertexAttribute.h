//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Helper structure describing a single vertex attribute
//

#ifndef LIBGLESV2_VERTEXATTRIBUTE_H_
#define LIBGLESV2_VERTEXATTRIBUTE_H_

#include "libGLESv2/Buffer.h"

namespace gl
{

struct VertexAttribute
{
    bool enabled; // From glEnable/DisableVertexAttribArray

    GLenum type;
    GLuint size;
    bool normalized;
    bool pureInteger;
    GLuint stride; // 0 means natural stride

    union
    {
        const GLvoid *pointer;
        GLintptr offset;
    };
    BindingPointer<Buffer> buffer; // Captured when glVertexAttribPointer is called.

    GLuint divisor;

    VertexAttribute();
};

template <typename T>
T QuerySingleVertexAttributeParameter(const VertexAttribute& attrib, GLenum pname)
{
  switch (pname)
  {
    case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
      return static_cast<T>(attrib.enabled ? GL_TRUE : GL_FALSE);
    case GL_VERTEX_ATTRIB_ARRAY_SIZE:
      return static_cast<T>(attrib.size);
    case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
      return static_cast<T>(attrib.stride);
    case GL_VERTEX_ATTRIB_ARRAY_TYPE:
      return static_cast<T>(attrib.type);
    case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
      return static_cast<T>(attrib.normalized ? GL_TRUE : GL_FALSE);
    case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
      return static_cast<T>(attrib.buffer.id());
    case GL_VERTEX_ATTRIB_ARRAY_DIVISOR:
      return static_cast<T>(attrib.divisor);
    case GL_VERTEX_ATTRIB_ARRAY_INTEGER:
      return static_cast<T>(attrib.pureInteger ? GL_TRUE : GL_FALSE);
    default:
      UNREACHABLE();
      return static_cast<T>(0);
  }
}

size_t ComputeVertexAttributeTypeSize(const VertexAttribute& attrib);
size_t ComputeVertexAttributeStride(const VertexAttribute& attrib);

struct VertexAttribCurrentValueData
{
    union
    {
        GLfloat FloatValues[4];
        GLint IntValues[4];
        GLuint UnsignedIntValues[4];
    };
    GLenum Type;

    void setFloatValues(const GLfloat floatValues[4])
    {
        for (unsigned int valueIndex = 0; valueIndex < 4; valueIndex++)
        {
            FloatValues[valueIndex] = floatValues[valueIndex];
        }
        Type = GL_FLOAT;
    }

    void setIntValues(const GLint intValues[4])
    {
        for (unsigned int valueIndex = 0; valueIndex < 4; valueIndex++)
        {
            IntValues[valueIndex] = intValues[valueIndex];
        }
        Type = GL_INT;
    }

    void setUnsignedIntValues(const GLuint unsignedIntValues[4])
    {
        for (unsigned int valueIndex = 0; valueIndex < 4; valueIndex++)
        {
            UnsignedIntValues[valueIndex] = unsignedIntValues[valueIndex];
        }
        Type = GL_UNSIGNED_INT;
    }

    bool operator==(const VertexAttribCurrentValueData &other)
    {
        return (Type == other.Type && memcmp(FloatValues, other.FloatValues, sizeof(float) * 4) == 0);
    }

    bool operator!=(const VertexAttribCurrentValueData &other)
    {
        return !(*this == other);
    }
};

}

#endif // LIBGLESV2_VERTEXATTRIBUTE_H_
