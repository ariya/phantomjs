//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// queryconversions.cpp: Implementation of state query cast conversions

#include "libGLESv2/Context.h"
#include "common/utilities.h"

namespace gl
{

// Helper class for converting a GL type to a GLenum:
// We can't use CastStateValueEnum generally, because of GLboolean + GLubyte overlap.
// We restrict our use to CastStateValue, where it eliminates duplicate parameters.

template <typename GLType>
struct CastStateValueEnum { static GLenum mEnumForType; };

template <> GLenum CastStateValueEnum<GLint>::mEnumForType      = GL_INT;
template <> GLenum CastStateValueEnum<GLuint>::mEnumForType     = GL_UNSIGNED_INT;
template <> GLenum CastStateValueEnum<GLboolean>::mEnumForType  = GL_BOOL;
template <> GLenum CastStateValueEnum<GLint64>::mEnumForType    = GL_INT_64_ANGLEX;
template <> GLenum CastStateValueEnum<GLfloat>::mEnumForType    = GL_FLOAT;

template <typename QueryT, typename NativeT>
QueryT CastStateValueToInt(GLenum pname, NativeT value)
{
    GLenum queryType = CastStateValueEnum<QueryT>::mEnumForType;
    GLenum nativeType = CastStateValueEnum<NativeT>::mEnumForType;

    if (nativeType == GL_FLOAT)
    {
        // RGBA color values and DepthRangeF values are converted to integer using Equation 2.4 from Table 4.5
        if (pname == GL_DEPTH_RANGE || pname == GL_COLOR_CLEAR_VALUE || pname == GL_DEPTH_CLEAR_VALUE || pname == GL_BLEND_COLOR)
        {
            return static_cast<QueryT>((static_cast<GLfloat>(0xFFFFFFFF) * value - 1.0f) / 2.0f);
        }
        else
        {
            return gl::iround<QueryT>(value);
        }
    }

    // Clamp 64-bit int values when casting to int
    if (nativeType == GL_INT_64_ANGLEX && queryType == GL_INT)
    {
        GLint64 minIntValue = static_cast<GLint64>(std::numeric_limits<GLint>::min());
        GLint64 maxIntValue = static_cast<GLint64>(std::numeric_limits<GLint>::max());
        GLint64 clampedValue = std::max(std::min(static_cast<GLint64>(value), maxIntValue), minIntValue);
        return static_cast<QueryT>(clampedValue);
    }

    return static_cast<QueryT>(value);
}

template <typename QueryT, typename NativeT>
QueryT CastStateValue(GLenum pname, NativeT value)
{
    GLenum queryType = CastStateValueEnum<QueryT>::mEnumForType;

    switch (queryType)
    {
      case GL_INT:              return CastStateValueToInt<QueryT, NativeT>(pname, value);
      case GL_INT_64_ANGLEX:    return CastStateValueToInt<QueryT, NativeT>(pname, value);
      case GL_FLOAT:            return static_cast<QueryT>(value);
      case GL_BOOL:             return (value == static_cast<NativeT>(0) ? GL_FALSE : GL_TRUE);
      default: UNREACHABLE();   return 0;
    }
}

template <typename QueryT>
void CastStateValues(Context *context, GLenum nativeType, GLenum pname,
                     unsigned int numParams, QueryT *outParams)
{
    if (nativeType == GL_INT)
    {
        GLint *intParams = NULL;
        intParams = new GLint[numParams];

        context->getIntegerv(pname, intParams);

        for (unsigned int i = 0; i < numParams; ++i)
        {
            outParams[i] = CastStateValue<QueryT>(pname, intParams[i]);
        }

        delete [] intParams;
    }
    else if (nativeType == GL_BOOL)
    {
        GLboolean *boolParams = NULL;
        boolParams = new GLboolean[numParams];

        context->getBooleanv(pname, boolParams);

        for (unsigned int i = 0; i < numParams; ++i)
        {
            outParams[i] = (boolParams[i] == GL_FALSE ? static_cast<QueryT>(0) : static_cast<QueryT>(1));
        }

        delete [] boolParams;
    }
    else if (nativeType == GL_FLOAT)
    {
        GLfloat *floatParams = NULL;
        floatParams = new GLfloat[numParams];

        context->getFloatv(pname, floatParams);

        for (unsigned int i = 0; i < numParams; ++i)
        {
            outParams[i] = CastStateValue<QueryT>(pname, floatParams[i]);
        }

        delete [] floatParams;
    }
    else if (nativeType == GL_INT_64_ANGLEX)
    {
        GLint64 *int64Params = NULL;
        int64Params = new GLint64[numParams];

        context->getInteger64v(pname, int64Params);

        for (unsigned int i = 0; i < numParams; ++i)
        {
            outParams[i] = CastStateValue<QueryT>(pname, int64Params[i]);
        }

        delete [] int64Params;
    }
    else UNREACHABLE();
}

// Explicit template instantiation (how we export template functions in different files)
// The calls below will make CastStateValues successfully link with the GL state query types
// The GL state query API types are: bool, int, uint, float, int64

template void CastStateValues<GLboolean>(Context *, GLenum, GLenum, unsigned int, GLboolean *);
template void CastStateValues<GLint>(Context *, GLenum, GLenum, unsigned int, GLint *);
template void CastStateValues<GLuint>(Context *, GLenum, GLenum, unsigned int, GLuint *);
template void CastStateValues<GLfloat>(Context *, GLenum, GLenum, unsigned int, GLfloat *);
template void CastStateValues<GLint64>(Context *, GLenum, GLenum, unsigned int, GLint64 *);

}
