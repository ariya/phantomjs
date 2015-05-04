//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Implementation of the state class for mananging GLES 3 Vertex Array Objects.
//

#include "libGLESv2/VertexAttribute.h"

namespace gl
{

VertexAttribute::VertexAttribute()
    : enabled(false),
      type(GL_FLOAT),
      size(4),
      normalized(false),
      pureInteger(false),
      stride(0),
      pointer(NULL),
      divisor(0)
{
}

size_t ComputeVertexAttributeTypeSize(const VertexAttribute& attrib)
{
    GLuint size = attrib.size;
    switch (attrib.type)
    {
      case GL_BYTE:                        return size * sizeof(GLbyte);
      case GL_UNSIGNED_BYTE:               return size * sizeof(GLubyte);
      case GL_SHORT:                       return size * sizeof(GLshort);
      case GL_UNSIGNED_SHORT:              return size * sizeof(GLushort);
      case GL_INT:                         return size * sizeof(GLint);
      case GL_UNSIGNED_INT:                return size * sizeof(GLuint);
      case GL_INT_2_10_10_10_REV:          return 4;
      case GL_UNSIGNED_INT_2_10_10_10_REV: return 4;
      case GL_FIXED:                       return size * sizeof(GLfixed);
      case GL_HALF_FLOAT:                  return size * sizeof(GLhalf);
      case GL_FLOAT:                       return size * sizeof(GLfloat);
      default: UNREACHABLE();              return size * sizeof(GLfloat);
    }
}

size_t ComputeVertexAttributeStride(const VertexAttribute& attrib)
{
    if (!attrib.enabled)
    {
        return 16;
    }
    return attrib.stride ? attrib.stride : ComputeVertexAttributeTypeSize(attrib);
}

}