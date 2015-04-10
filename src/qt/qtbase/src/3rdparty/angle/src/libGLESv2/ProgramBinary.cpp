#include "precompiled.h"
//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Program.cpp: Implements the gl::Program class. Implements GL program objects
// and related functionality. [OpenGL ES 2.0.24] section 2.10.3 page 28.

#include "libGLESv2/BinaryStream.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/renderer/ShaderExecutable.h"

#include "common/debug.h"
#include "common/version.h"
#include "utilities.h"

#include "libGLESv2/main.h"
#include "libGLESv2/Shader.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/VertexDataManager.h"

#undef near
#undef far

namespace gl
{
std::string str(int i)
{
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%d", i);
    return buffer;
}

static rx::D3DWorkaroundType DiscardWorkaround(bool usesDiscard)
{
    return (usesDiscard ? rx::ANGLE_D3D_WORKAROUND_SM3_OPTIMIZER : rx::ANGLE_D3D_WORKAROUND_NONE);
}

UniformLocation::UniformLocation(const std::string &name, unsigned int element, unsigned int index) 
    : name(name), element(element), index(index)
{
}

unsigned int ProgramBinary::mCurrentSerial = 1;

ProgramBinary::ProgramBinary(rx::Renderer *renderer) : mRenderer(renderer), RefCountObject(0), mSerial(issueSerial())
{
    mPixelExecutable = NULL;
    mVertexExecutable = NULL;
    mGeometryExecutable = NULL;

    mValidated = false;

    for (int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
    {
        mSemanticIndex[index] = -1;
    }

    for (int index = 0; index < MAX_TEXTURE_IMAGE_UNITS; index++)
    {
        mSamplersPS[index].active = false;
    }

    for (int index = 0; index < IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS; index++)
    {
        mSamplersVS[index].active = false;
    }

    mUsedVertexSamplerRange = 0;
    mUsedPixelSamplerRange = 0;
    mUsesPointSize = false;
}

ProgramBinary::~ProgramBinary()
{
    delete mPixelExecutable;
    mPixelExecutable = NULL;

    delete mVertexExecutable;
    mVertexExecutable = NULL;

    delete mGeometryExecutable;
    mGeometryExecutable = NULL;

    while (!mUniforms.empty())
    {
        delete mUniforms.back();
        mUniforms.pop_back();
    }
}

unsigned int ProgramBinary::getSerial() const
{
    return mSerial;
}

unsigned int ProgramBinary::issueSerial()
{
    return mCurrentSerial++;
}

rx::ShaderExecutable *ProgramBinary::getPixelExecutable()
{
    return mPixelExecutable;
}

rx::ShaderExecutable *ProgramBinary::getVertexExecutable()
{
    return mVertexExecutable;
}

rx::ShaderExecutable *ProgramBinary::getGeometryExecutable()
{
    return mGeometryExecutable;
}

GLuint ProgramBinary::getAttributeLocation(const char *name)
{
    if (name)
    {
        for (int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
        {
            if (mLinkedAttribute[index].name == std::string(name))
            {
                return index;
            }
        }
    }

    return -1;
}

int ProgramBinary::getSemanticIndex(int attributeIndex)
{
    ASSERT(attributeIndex >= 0 && attributeIndex < MAX_VERTEX_ATTRIBS);
    
    return mSemanticIndex[attributeIndex];
}

// Returns one more than the highest sampler index used.
GLint ProgramBinary::getUsedSamplerRange(SamplerType type)
{
    switch (type)
    {
      case SAMPLER_PIXEL:
        return mUsedPixelSamplerRange;
      case SAMPLER_VERTEX:
        return mUsedVertexSamplerRange;
      default:
        UNREACHABLE();
        return 0;
    }
}

bool ProgramBinary::usesPointSize() const
{
    return mUsesPointSize;
}

bool ProgramBinary::usesPointSpriteEmulation() const
{
    return mUsesPointSize && mRenderer->getMajorShaderModel() >= 4;
}

bool ProgramBinary::usesGeometryShader() const
{
    return usesPointSpriteEmulation();
}

// Returns the index of the texture image unit (0-19) corresponding to a Direct3D 9 sampler
// index (0-15 for the pixel shader and 0-3 for the vertex shader).
GLint ProgramBinary::getSamplerMapping(SamplerType type, unsigned int samplerIndex)
{
    GLint logicalTextureUnit = -1;

    switch (type)
    {
      case SAMPLER_PIXEL:
        ASSERT(samplerIndex < sizeof(mSamplersPS)/sizeof(mSamplersPS[0]));

        if (mSamplersPS[samplerIndex].active)
        {
            logicalTextureUnit = mSamplersPS[samplerIndex].logicalTextureUnit;
        }
        break;
      case SAMPLER_VERTEX:
        ASSERT(samplerIndex < sizeof(mSamplersVS)/sizeof(mSamplersVS[0]));

        if (mSamplersVS[samplerIndex].active)
        {
            logicalTextureUnit = mSamplersVS[samplerIndex].logicalTextureUnit;
        }
        break;
      default: UNREACHABLE();
    }

    if (logicalTextureUnit >= 0 && logicalTextureUnit < (GLint)mRenderer->getMaxCombinedTextureImageUnits())
    {
        return logicalTextureUnit;
    }

    return -1;
}

// Returns the texture type for a given Direct3D 9 sampler type and
// index (0-15 for the pixel shader and 0-3 for the vertex shader).
TextureType ProgramBinary::getSamplerTextureType(SamplerType type, unsigned int samplerIndex)
{
    switch (type)
    {
      case SAMPLER_PIXEL:
        ASSERT(samplerIndex < sizeof(mSamplersPS)/sizeof(mSamplersPS[0]));
        ASSERT(mSamplersPS[samplerIndex].active);
        return mSamplersPS[samplerIndex].textureType;
      case SAMPLER_VERTEX:
        ASSERT(samplerIndex < sizeof(mSamplersVS)/sizeof(mSamplersVS[0]));
        ASSERT(mSamplersVS[samplerIndex].active);
        return mSamplersVS[samplerIndex].textureType;
      default: UNREACHABLE();
    }

    return TEXTURE_2D;
}

GLint ProgramBinary::getUniformLocation(std::string name)
{
    unsigned int subscript = 0;

    // Strip any trailing array operator and retrieve the subscript
    size_t open = name.find_last_of('[');
    size_t close = name.find_last_of(']');
    if (open != std::string::npos && close == name.length() - 1)
    {
        subscript = atoi(name.substr(open + 1).c_str());
        name.erase(open);
    }

    unsigned int numUniforms = mUniformIndex.size();
    for (unsigned int location = 0; location < numUniforms; location++)
    {
        if (mUniformIndex[location].name == name &&
            mUniformIndex[location].element == subscript)
        {
            return location;
        }
    }

    return -1;
}

bool ProgramBinary::setUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];
    targetUniform->dirty = true;

    int elementCount = targetUniform->elementCount();

    if (elementCount == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);

    if (targetUniform->type == GL_FLOAT)
    {
        GLfloat *target = (GLfloat*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            target[0] = v[0];
            target[1] = 0;
            target[2] = 0;
            target[3] = 0;
            target += 4;
            v += 1;
        }
    }
    else if (targetUniform->type == GL_BOOL)
    {
        GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            boolParams[0] = (v[0] == 0.0f) ? GL_FALSE : GL_TRUE;
            boolParams[1] = GL_FALSE;
            boolParams[2] = GL_FALSE;
            boolParams[3] = GL_FALSE;
            boolParams += 4;
            v += 1;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool ProgramBinary::setUniform2fv(GLint location, GLsizei count, const GLfloat *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];
    targetUniform->dirty = true;

    int elementCount = targetUniform->elementCount();

    if (elementCount == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);

    if (targetUniform->type == GL_FLOAT_VEC2)
    {
        GLfloat *target = (GLfloat*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            target[0] = v[0];
            target[1] = v[1];
            target[2] = 0;
            target[3] = 0;
            target += 4;
            v += 2;
        }
    }
    else if (targetUniform->type == GL_BOOL_VEC2)
    {
        GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            boolParams[0] = (v[0] == 0.0f) ? GL_FALSE : GL_TRUE;
            boolParams[1] = (v[1] == 0.0f) ? GL_FALSE : GL_TRUE;
            boolParams[2] = GL_FALSE;
            boolParams[3] = GL_FALSE;
            boolParams += 4;
            v += 2;
        }
    }
    else 
    {
        return false;
    }

    return true;
}

bool ProgramBinary::setUniform3fv(GLint location, GLsizei count, const GLfloat *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];
    targetUniform->dirty = true;

    int elementCount = targetUniform->elementCount();

    if (elementCount == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);

    if (targetUniform->type == GL_FLOAT_VEC3)
    {
        GLfloat *target = (GLfloat*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            target[0] = v[0];
            target[1] = v[1];
            target[2] = v[2];
            target[3] = 0;
            target += 4;
            v += 3;
        }
    }
    else if (targetUniform->type == GL_BOOL_VEC3)
    {
        GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            boolParams[0] = (v[0] == 0.0f) ? GL_FALSE : GL_TRUE;
            boolParams[1] = (v[1] == 0.0f) ? GL_FALSE : GL_TRUE;
            boolParams[2] = (v[2] == 0.0f) ? GL_FALSE : GL_TRUE;
            boolParams[3] = GL_FALSE;
            boolParams += 4;
            v += 3;
        }
    }
    else 
    {
        return false;
    }

    return true;
}

bool ProgramBinary::setUniform4fv(GLint location, GLsizei count, const GLfloat *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];
    targetUniform->dirty = true;

    int elementCount = targetUniform->elementCount();

    if (elementCount == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);

    if (targetUniform->type == GL_FLOAT_VEC4)
    {
        GLfloat *target = (GLfloat*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            target[0] = v[0];
            target[1] = v[1];
            target[2] = v[2];
            target[3] = v[3];
            target += 4;
            v += 4;
        }
    }
    else if (targetUniform->type == GL_BOOL_VEC4)
    {
        GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            boolParams[0] = (v[0] == 0.0f) ? GL_FALSE : GL_TRUE;
            boolParams[1] = (v[1] == 0.0f) ? GL_FALSE : GL_TRUE;
            boolParams[2] = (v[2] == 0.0f) ? GL_FALSE : GL_TRUE;
            boolParams[3] = (v[3] == 0.0f) ? GL_FALSE : GL_TRUE;
            boolParams += 4;
            v += 4;
        }
    }
    else 
    {
        return false;
    }

    return true;
}

template<typename T, int targetWidth, int targetHeight, int srcWidth, int srcHeight>
void transposeMatrix(T *target, const GLfloat *value)
{
    int copyWidth = std::min(targetWidth, srcWidth);
    int copyHeight = std::min(targetHeight, srcHeight);

    for (int x = 0; x < copyWidth; x++)
    {
        for (int y = 0; y < copyHeight; y++)
        {
            target[x * targetWidth + y] = (T)value[y * srcWidth + x];
        }
    }
    // clear unfilled right side
    for (int y = 0; y < copyHeight; y++)
    {
        for (int x = srcWidth; x < targetWidth; x++)
        {
            target[y * targetWidth + x] = (T)0;
        }
    }
    // clear unfilled bottom.
    for (int y = srcHeight; y < targetHeight; y++)
    {
        for (int x = 0; x < targetWidth; x++)
        {
            target[y * targetWidth + x] = (T)0;
        }
    }
}

bool ProgramBinary::setUniformMatrix2fv(GLint location, GLsizei count, const GLfloat *value)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];
    targetUniform->dirty = true;

    if (targetUniform->type != GL_FLOAT_MAT2)
    {
        return false;
    }

    int elementCount = targetUniform->elementCount();

    if (elementCount == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);
    GLfloat *target = (GLfloat*)targetUniform->data + mUniformIndex[location].element * 8;

    for (int i = 0; i < count; i++)
    {
        transposeMatrix<GLfloat,4,2,2,2>(target, value);
        target += 8;
        value += 4;
    }

    return true;
}

bool ProgramBinary::setUniformMatrix3fv(GLint location, GLsizei count, const GLfloat *value)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];
    targetUniform->dirty = true;

    if (targetUniform->type != GL_FLOAT_MAT3)
    {
        return false;
    }

    int elementCount = targetUniform->elementCount();

    if (elementCount == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);
    GLfloat *target = (GLfloat*)targetUniform->data + mUniformIndex[location].element * 12;

    for (int i = 0; i < count; i++)
    {
        transposeMatrix<GLfloat,4,3,3,3>(target, value);
        target += 12;
        value += 9;
    }

    return true;
}


bool ProgramBinary::setUniformMatrix4fv(GLint location, GLsizei count, const GLfloat *value)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];
    targetUniform->dirty = true;

    if (targetUniform->type != GL_FLOAT_MAT4)
    {
        return false;
    }

    int elementCount = targetUniform->elementCount();

    if (elementCount == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);
    GLfloat *target = (GLfloat*)(targetUniform->data + mUniformIndex[location].element * sizeof(GLfloat) * 16);

    for (int i = 0; i < count; i++)
    {
        transposeMatrix<GLfloat,4,4,4,4>(target, value);
        target += 16;
        value += 16;
    }

    return true;
}

bool ProgramBinary::setUniform1iv(GLint location, GLsizei count, const GLint *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];
    targetUniform->dirty = true;

    int elementCount = targetUniform->elementCount();

    if (elementCount == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);

    if (targetUniform->type == GL_INT ||
        targetUniform->type == GL_SAMPLER_2D ||
        targetUniform->type == GL_SAMPLER_CUBE)
    {
        GLint *target = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            target[0] = v[0];
            target[1] = 0;
            target[2] = 0;
            target[3] = 0;
            target += 4;
            v += 1;
        }
    }
    else if (targetUniform->type == GL_BOOL)
    {
        GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            boolParams[0] = (v[0] == 0) ? GL_FALSE : GL_TRUE;
            boolParams[1] = GL_FALSE;
            boolParams[2] = GL_FALSE;
            boolParams[3] = GL_FALSE;
            boolParams += 4;
            v += 1;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool ProgramBinary::setUniform2iv(GLint location, GLsizei count, const GLint *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];
    targetUniform->dirty = true;

    int elementCount = targetUniform->elementCount();

    if (elementCount == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);

    if (targetUniform->type == GL_INT_VEC2)
    {
        GLint *target = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            target[0] = v[0];
            target[1] = v[1];
            target[2] = 0;
            target[3] = 0;
            target += 4;
            v += 2;
        }
    }
    else if (targetUniform->type == GL_BOOL_VEC2)
    {
        GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            boolParams[0] = (v[0] == 0) ? GL_FALSE : GL_TRUE;
            boolParams[1] = (v[1] == 0) ? GL_FALSE : GL_TRUE;
            boolParams[2] = GL_FALSE;
            boolParams[3] = GL_FALSE;
            boolParams += 4;
            v += 2;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool ProgramBinary::setUniform3iv(GLint location, GLsizei count, const GLint *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];
    targetUniform->dirty = true;

    int elementCount = targetUniform->elementCount();

    if (elementCount == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);

    if (targetUniform->type == GL_INT_VEC3)
    {
        GLint *target = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            target[0] = v[0];
            target[1] = v[1];
            target[2] = v[2];
            target[3] = 0;
            target += 4;
            v += 3;
        }
    }
    else if (targetUniform->type == GL_BOOL_VEC3)
    {
        GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            boolParams[0] = (v[0] == 0) ? GL_FALSE : GL_TRUE;
            boolParams[1] = (v[1] == 0) ? GL_FALSE : GL_TRUE;
            boolParams[2] = (v[2] == 0) ? GL_FALSE : GL_TRUE;
            boolParams[3] = GL_FALSE;
            boolParams += 4;
            v += 3;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool ProgramBinary::setUniform4iv(GLint location, GLsizei count, const GLint *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];
    targetUniform->dirty = true;

    int elementCount = targetUniform->elementCount();

    if (elementCount == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);

    if (targetUniform->type == GL_INT_VEC4)
    {
        GLint *target = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            target[0] = v[0];
            target[1] = v[1];
            target[2] = v[2];
            target[3] = v[3];
            target += 4;
            v += 4;
        }
    }
    else if (targetUniform->type == GL_BOOL_VEC4)
    {
        GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            boolParams[0] = (v[0] == 0) ? GL_FALSE : GL_TRUE;
            boolParams[1] = (v[1] == 0) ? GL_FALSE : GL_TRUE;
            boolParams[2] = (v[2] == 0) ? GL_FALSE : GL_TRUE;
            boolParams[3] = (v[3] == 0) ? GL_FALSE : GL_TRUE;
            boolParams += 4;
            v += 4;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool ProgramBinary::getUniformfv(GLint location, GLsizei *bufSize, GLfloat *params)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    // sized queries -- ensure the provided buffer is large enough
    if (bufSize)
    {
        int requiredBytes = UniformExternalSize(targetUniform->type);
        if (*bufSize < requiredBytes)
        {
            return false;
        }
    }

    switch (targetUniform->type)
    {
      case GL_FLOAT_MAT2:
        transposeMatrix<GLfloat,2,2,4,2>(params, (GLfloat*)targetUniform->data + mUniformIndex[location].element * 8);
        break;
      case GL_FLOAT_MAT3:
        transposeMatrix<GLfloat,3,3,4,3>(params, (GLfloat*)targetUniform->data + mUniformIndex[location].element * 12);
        break;
      case GL_FLOAT_MAT4:
        transposeMatrix<GLfloat,4,4,4,4>(params, (GLfloat*)targetUniform->data + mUniformIndex[location].element * 16);
        break;
      default:
        {
            unsigned int size = UniformComponentCount(targetUniform->type);

            switch (UniformComponentType(targetUniform->type))
            {
              case GL_BOOL:
                {
                    GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

                    for (unsigned int i = 0; i < size; i++)
                    {
                        params[i] = (boolParams[i] == GL_FALSE) ? 0.0f : 1.0f;
                    }
                }
                break;
              case GL_FLOAT:
                memcpy(params, targetUniform->data + mUniformIndex[location].element * 4 * sizeof(GLfloat),
                       size * sizeof(GLfloat));
                break;
              case GL_INT:
                {
                    GLint *intParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

                    for (unsigned int i = 0; i < size; i++)
                    {
                        params[i] = (float)intParams[i];
                    }
                }
                break;
              default: UNREACHABLE();
            }
        }
    }

    return true;
}

bool ProgramBinary::getUniformiv(GLint location, GLsizei *bufSize, GLint *params)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    // sized queries -- ensure the provided buffer is large enough
    if (bufSize)
    {
        int requiredBytes = UniformExternalSize(targetUniform->type);
        if (*bufSize < requiredBytes)
        {
            return false;
        }
    }

    switch (targetUniform->type)
    {
      case GL_FLOAT_MAT2:
        transposeMatrix<GLint,2,2,4,2>(params, (GLfloat*)targetUniform->data + mUniformIndex[location].element * 8);
        break;
      case GL_FLOAT_MAT3:
        transposeMatrix<GLint,3,3,4,3>(params, (GLfloat*)targetUniform->data + mUniformIndex[location].element * 12);
        break;
      case GL_FLOAT_MAT4:
        transposeMatrix<GLint,4,4,4,4>(params, (GLfloat*)targetUniform->data + mUniformIndex[location].element * 16);
        break;
      default:
        {
            unsigned int size = VariableColumnCount(targetUniform->type);

            switch (UniformComponentType(targetUniform->type))
            {
              case GL_BOOL:
                {
                    GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

                    for (unsigned int i = 0; i < size; i++)
                    {
                        params[i] = boolParams[i];
                    }
                }
                break;
              case GL_FLOAT:
                {
                    GLfloat *floatParams = (GLfloat*)targetUniform->data + mUniformIndex[location].element * 4;

                    for (unsigned int i = 0; i < size; i++)
                    {
                        params[i] = (GLint)floatParams[i];
                    }
                }
                break;
              case GL_INT:
                memcpy(params, targetUniform->data + mUniformIndex[location].element * 4 * sizeof(GLint),
                    size * sizeof(GLint));
                break;
              default: UNREACHABLE();
            }
        }
    }

    return true;
}

void ProgramBinary::dirtyAllUniforms()
{
    unsigned int numUniforms = mUniforms.size();
    for (unsigned int index = 0; index < numUniforms; index++)
    {
        mUniforms[index]->dirty = true;
    }
}

// Applies all the uniforms set for this program object to the renderer
void ProgramBinary::applyUniforms()
{
    // Retrieve sampler uniform values
    for (std::vector<Uniform*>::iterator ub = mUniforms.begin(), ue = mUniforms.end(); ub != ue; ++ub)
    {
        Uniform *targetUniform = *ub;

        if (targetUniform->dirty)
        {
            if (targetUniform->type == GL_SAMPLER_2D || 
                targetUniform->type == GL_SAMPLER_CUBE)
            {
                int count = targetUniform->elementCount();
                GLint (*v)[4] = (GLint(*)[4])targetUniform->data;

                if (targetUniform->psRegisterIndex >= 0)
                {
                    unsigned int firstIndex = targetUniform->psRegisterIndex;

                    for (int i = 0; i < count; i++)
                    {
                        unsigned int samplerIndex = firstIndex + i;

                        if (samplerIndex < MAX_TEXTURE_IMAGE_UNITS)
                        {
                            ASSERT(mSamplersPS[samplerIndex].active);
                            mSamplersPS[samplerIndex].logicalTextureUnit = v[i][0];
                        }
                    }
                }

                if (targetUniform->vsRegisterIndex >= 0)
                {
                    unsigned int firstIndex = targetUniform->vsRegisterIndex;

                    for (int i = 0; i < count; i++)
                    {
                        unsigned int samplerIndex = firstIndex + i;

                        if (samplerIndex < IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS)
                        {
                            ASSERT(mSamplersVS[samplerIndex].active);
                            mSamplersVS[samplerIndex].logicalTextureUnit = v[i][0];
                        }
                    }
                }
            }
        }
    }

    mRenderer->applyUniforms(this, &mUniforms);
}

// Packs varyings into generic varying registers, using the algorithm from [OpenGL ES Shading Language 1.00 rev. 17] appendix A section 7 page 111
// Returns the number of used varying registers, or -1 if unsuccesful
int ProgramBinary::packVaryings(InfoLog &infoLog, const Varying *packing[][4], FragmentShader *fragmentShader)
{
    const int maxVaryingVectors = mRenderer->getMaxVaryingVectors();

    fragmentShader->resetVaryingsRegisterAssignment();

    for (VaryingList::iterator varying = fragmentShader->mVaryings.begin(); varying != fragmentShader->mVaryings.end(); varying++)
    {
        int n = VariableRowCount(varying->type) * varying->size;
        int m = VariableColumnCount(varying->type);
        bool success = false;

        if (m == 2 || m == 3 || m == 4)
        {
            for (int r = 0; r <= maxVaryingVectors - n && !success; r++)
            {
                bool available = true;

                for (int y = 0; y < n && available; y++)
                {
                    for (int x = 0; x < m && available; x++)
                    {
                        if (packing[r + y][x])
                        {
                            available = false;
                        }
                    }
                }

                if (available)
                {
                    varying->reg = r;
                    varying->col = 0;

                    for (int y = 0; y < n; y++)
                    {
                        for (int x = 0; x < m; x++)
                        {
                            packing[r + y][x] = &*varying;
                        }
                    }

                    success = true;
                }
            }

            if (!success && m == 2)
            {
                for (int r = maxVaryingVectors - n; r >= 0 && !success; r--)
                {
                    bool available = true;

                    for (int y = 0; y < n && available; y++)
                    {
                        for (int x = 2; x < 4 && available; x++)
                        {
                            if (packing[r + y][x])
                            {
                                available = false;
                            }
                        }
                    }

                    if (available)
                    {
                        varying->reg = r;
                        varying->col = 2;

                        for (int y = 0; y < n; y++)
                        {
                            for (int x = 2; x < 4; x++)
                            {
                                packing[r + y][x] = &*varying;
                            }
                        }

                        success = true;
                    }
                }
            }
        }
        else if (m == 1)
        {
            int space[4] = {0};

            for (int y = 0; y < maxVaryingVectors; y++)
            {
                for (int x = 0; x < 4; x++)
                {
                    space[x] += packing[y][x] ? 0 : 1;
                }
            }

            int column = 0;

            for (int x = 0; x < 4; x++)
            {
                if (space[x] >= n && space[x] < space[column])
                {
                    column = x;
                }
            }

            if (space[column] >= n)
            {
                for (int r = 0; r < maxVaryingVectors; r++)
                {
                    if (!packing[r][column])
                    {
                        varying->reg = r;

                        for (int y = r; y < r + n; y++)
                        {
                            packing[y][column] = &*varying;
                        }

                        break;
                    }
                }

                varying->col = column;

                success = true;
            }
        }
        else UNREACHABLE();

        if (!success)
        {
            infoLog.append("Could not pack varying %s", varying->name.c_str());

            return -1;
        }
    }

    // Return the number of used registers
    int registers = 0;

    for (int r = 0; r < maxVaryingVectors; r++)
    {
        if (packing[r][0] || packing[r][1] || packing[r][2] || packing[r][3])
        {
            registers++;
        }
    }

    return registers;
}

bool ProgramBinary::linkVaryings(InfoLog &infoLog, int registers, const Varying *packing[][4],
                                 std::string& pixelHLSL, std::string& vertexHLSL,
                                 FragmentShader *fragmentShader, VertexShader *vertexShader)
{
    if (pixelHLSL.empty() || vertexHLSL.empty())
    {
        return false;
    }

    bool usesMRT = fragmentShader->mUsesMultipleRenderTargets;
    bool usesFragColor = fragmentShader->mUsesFragColor;
    bool usesFragData = fragmentShader->mUsesFragData;
    if (usesFragColor && usesFragData)
    {
        infoLog.append("Cannot use both gl_FragColor and gl_FragData in the same fragment shader.");
        return false;
    }

    // Write the HLSL input/output declarations
    const int shaderModel = mRenderer->getMajorShaderModel();
    const int maxVaryingVectors = mRenderer->getMaxVaryingVectors();

    const int registersNeeded = registers + (fragmentShader->mUsesFragCoord ? 1 : 0) + (fragmentShader->mUsesPointCoord ? 1 : 0);

    // The output color is broadcast to all enabled draw buffers when writing to gl_FragColor
    const bool broadcast = fragmentShader->mUsesFragColor;
    const unsigned int numRenderTargets = (broadcast || usesMRT ? mRenderer->getMaxRenderTargets() : 1);

    if (registersNeeded > maxVaryingVectors)
    {
        infoLog.append("No varying registers left to support gl_FragCoord/gl_PointCoord");

        return false;
    }

    vertexShader->resetVaryingsRegisterAssignment();

    for (VaryingList::iterator input = fragmentShader->mVaryings.begin(); input != fragmentShader->mVaryings.end(); input++)
    {
        bool matched = false;

        for (VaryingList::iterator output = vertexShader->mVaryings.begin(); output != vertexShader->mVaryings.end(); output++)
        {
            if (output->name == input->name)
            {
                if (output->type != input->type || output->size != input->size)
                {
                    infoLog.append("Type of vertex varying %s does not match that of the fragment varying", output->name.c_str());

                    return false;
                }

                output->reg = input->reg;
                output->col = input->col;

                matched = true;
                break;
            }
        }

        if (!matched)
        {
            infoLog.append("Fragment varying %s does not match any vertex varying", input->name.c_str());

            return false;
        }
    }

    mUsesPointSize = vertexShader->mUsesPointSize;
    std::string varyingSemantic = (mUsesPointSize && shaderModel == 3) ? "COLOR" : "TEXCOORD";
    std::string targetSemantic = (shaderModel >= 4) ? "SV_Target" : "COLOR";
    std::string positionSemantic = (shaderModel >= 4) ? "SV_Position" : "POSITION";
    std::string depthSemantic = (shaderModel >= 4) ? "SV_Depth" : "DEPTH";

    // special varyings that use reserved registers
    int reservedRegisterIndex = registers;
    std::string fragCoordSemantic;
    std::string pointCoordSemantic;

    if (fragmentShader->mUsesFragCoord)
    {
        fragCoordSemantic = varyingSemantic + str(reservedRegisterIndex++);
    }

    if (fragmentShader->mUsesPointCoord)
    {
        // Shader model 3 uses a special TEXCOORD semantic for point sprite texcoords.
        // In DX11 we compute this in the GS.
        if (shaderModel == 3)
        {
            pointCoordSemantic = "TEXCOORD0";
        }
        else if (shaderModel >= 4)
        {
            pointCoordSemantic = varyingSemantic + str(reservedRegisterIndex++); 
        }
    }

    vertexHLSL += "struct VS_INPUT\n"
                  "{\n";

    int semanticIndex = 0;
    for (AttributeArray::iterator attribute = vertexShader->mAttributes.begin(); attribute != vertexShader->mAttributes.end(); attribute++)
    {
        switch (attribute->type)
        {
          case GL_FLOAT:      vertexHLSL += "    float ";    break;
          case GL_FLOAT_VEC2: vertexHLSL += "    float2 ";   break;
          case GL_FLOAT_VEC3: vertexHLSL += "    float3 ";   break;
          case GL_FLOAT_VEC4: vertexHLSL += "    float4 ";   break;
          case GL_FLOAT_MAT2: vertexHLSL += "    float2x2 "; break;
          case GL_FLOAT_MAT3: vertexHLSL += "    float3x3 "; break;
          case GL_FLOAT_MAT4: vertexHLSL += "    float4x4 "; break;
          default:  UNREACHABLE();
        }

        vertexHLSL += decorateAttribute(attribute->name) + " : TEXCOORD" + str(semanticIndex) + ";\n";

        semanticIndex += VariableRowCount(attribute->type);
    }

    vertexHLSL += "};\n"
                  "\n"
                  "struct VS_OUTPUT\n"
                  "{\n";

    if (shaderModel < 4)
    {
        vertexHLSL += "    float4 gl_Position : " + positionSemantic + ";\n";
    }

    for (int r = 0; r < registers; r++)
    {
        int registerSize = packing[r][3] ? 4 : (packing[r][2] ? 3 : (packing[r][1] ? 2 : 1));

        vertexHLSL += "    float" + str(registerSize) + " v" + str(r) + " : " + varyingSemantic + str(r) + ";\n";
    }

    if (fragmentShader->mUsesFragCoord)
    {
        vertexHLSL += "    float4 gl_FragCoord : " + fragCoordSemantic + ";\n";
    }

    if (vertexShader->mUsesPointSize && shaderModel >= 3)
    {
        vertexHLSL += "    float gl_PointSize : PSIZE;\n";
    }

    if (shaderModel >= 4)
    {
        vertexHLSL += "    float4 gl_Position : " + positionSemantic + ";\n";
    }

    vertexHLSL += "};\n"
                  "\n"
                  "VS_OUTPUT main(VS_INPUT input)\n"
                  "{\n";

    for (AttributeArray::iterator attribute = vertexShader->mAttributes.begin(); attribute != vertexShader->mAttributes.end(); attribute++)
    {
        vertexHLSL += "    " + decorateAttribute(attribute->name) + " = ";

        if (VariableRowCount(attribute->type) > 1)   // Matrix
        {
            vertexHLSL += "transpose";
        }

        vertexHLSL += "(input." + decorateAttribute(attribute->name) + ");\n";
    }

    if (shaderModel >= 4)
    {
        vertexHLSL += "\n"
                      "    gl_main();\n"
                      "\n"
                      "    VS_OUTPUT output;\n"
                      "    output.gl_Position.x = gl_Position.x;\n"
                      "    output.gl_Position.y = -gl_Position.y;\n"
                      "    output.gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;\n"
                      "    output.gl_Position.w = gl_Position.w;\n";
    }
    else
    {
        vertexHLSL += "\n"
                      "    gl_main();\n"
                      "\n"
                      "    VS_OUTPUT output;\n"
                      "    output.gl_Position.x = gl_Position.x * dx_ViewAdjust.z + dx_ViewAdjust.x * gl_Position.w;\n"
                      "    output.gl_Position.y = -(gl_Position.y * dx_ViewAdjust.w + dx_ViewAdjust.y * gl_Position.w);\n"
                      "    output.gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;\n"
                      "    output.gl_Position.w = gl_Position.w;\n";
    }

    if (vertexShader->mUsesPointSize && shaderModel >= 3)
    {
        vertexHLSL += "    output.gl_PointSize = gl_PointSize;\n";
    }

    if (fragmentShader->mUsesFragCoord)
    {
        vertexHLSL += "    output.gl_FragCoord = gl_Position;\n";
    }

    for (VaryingList::iterator varying = vertexShader->mVaryings.begin(); varying != vertexShader->mVaryings.end(); varying++)
    {
        if (varying->reg >= 0)
        {
            for (int i = 0; i < varying->size; i++)
            {
                int rows = VariableRowCount(varying->type);

                for (int j = 0; j < rows; j++)
                {
                    int r = varying->reg + i * rows + j;
                    vertexHLSL += "    output.v" + str(r);

                    bool sharedRegister = false;   // Register used by multiple varyings
                    
                    for (int x = 0; x < 4; x++)
                    {
                        if (packing[r][x] && packing[r][x] != packing[r][0])
                        {
                            sharedRegister = true;
                            break;
                        }
                    }

                    if(sharedRegister)
                    {
                        vertexHLSL += ".";

                        for (int x = 0; x < 4; x++)
                        {
                            if (packing[r][x] == &*varying)
                            {
                                switch(x)
                                {
                                  case 0: vertexHLSL += "x"; break;
                                  case 1: vertexHLSL += "y"; break;
                                  case 2: vertexHLSL += "z"; break;
                                  case 3: vertexHLSL += "w"; break;
                                }
                            }
                        }
                    }

                    vertexHLSL += " = " + varying->name;
                    
                    if (varying->array)
                    {
                        vertexHLSL += "[" + str(i) + "]";
                    }

                    if (rows > 1)
                    {
                        vertexHLSL += "[" + str(j) + "]";
                    }
                    
                    vertexHLSL += ";\n";
                }
            }
        }
    }

    vertexHLSL += "\n"
                  "    return output;\n"
                  "}\n";

    pixelHLSL += "struct PS_INPUT\n"
                 "{\n";
    
    for (VaryingList::iterator varying = fragmentShader->mVaryings.begin(); varying != fragmentShader->mVaryings.end(); varying++)
    {
        if (varying->reg >= 0)
        {
            for (int i = 0; i < varying->size; i++)
            {
                int rows = VariableRowCount(varying->type);
                for (int j = 0; j < rows; j++)
                {
                    std::string n = str(varying->reg + i * rows + j);
                    pixelHLSL += "    float" + str(VariableColumnCount(varying->type)) + " v" + n + " : " + varyingSemantic + n + ";\n";
                }
            }
        }
        else UNREACHABLE();
    }

    if (fragmentShader->mUsesFragCoord)
    {
        pixelHLSL += "    float4 gl_FragCoord : " + fragCoordSemantic + ";\n";
    }
        
    if (fragmentShader->mUsesPointCoord && shaderModel >= 3)
    {
        pixelHLSL += "    float2 gl_PointCoord : " + pointCoordSemantic + ";\n";
    }

    // Must consume the PSIZE element if the geometry shader is not active
    // We won't know if we use a GS until we draw
    if (vertexShader->mUsesPointSize && shaderModel >= 4)
    {
        pixelHLSL += "    float gl_PointSize : PSIZE;\n";
    }

    if (fragmentShader->mUsesFragCoord)
    {
        if (shaderModel >= 4)
        {
            pixelHLSL += "    float4 dx_VPos : SV_Position;\n";
        }
        else if (shaderModel >= 3)
        {
            pixelHLSL += "    float2 dx_VPos : VPOS;\n";
        }
    }

    pixelHLSL += "};\n"
                 "\n"
                 "struct PS_OUTPUT\n"
                 "{\n";

    for (unsigned int renderTargetIndex = 0; renderTargetIndex < numRenderTargets; renderTargetIndex++)
    {
        pixelHLSL += "    float4 gl_Color" + str(renderTargetIndex) + " : " + targetSemantic + str(renderTargetIndex) + ";\n";
    }

    if (fragmentShader->mUsesFragDepth)
    {
        pixelHLSL += "    float gl_Depth : " + depthSemantic + ";\n";
    }

    pixelHLSL += "};\n"
                 "\n";

    if (fragmentShader->mUsesFrontFacing)
    {
        if (shaderModel >= 4)
        {
            pixelHLSL += "PS_OUTPUT main(PS_INPUT input, bool isFrontFace : SV_IsFrontFace)\n"
                         "{\n";
        }
        else
        {
            pixelHLSL += "PS_OUTPUT main(PS_INPUT input, float vFace : VFACE)\n"
                         "{\n";
        }
    }
    else
    {
        pixelHLSL += "PS_OUTPUT main(PS_INPUT input)\n"
                     "{\n";
    }

    if (fragmentShader->mUsesFragCoord)
    {
        pixelHLSL += "    float rhw = 1.0 / input.gl_FragCoord.w;\n";
        
        if (shaderModel >= 4)
        {
            pixelHLSL += "    gl_FragCoord.x = input.dx_VPos.x;\n"
                         "    gl_FragCoord.y = input.dx_VPos.y;\n";
        }
        else if (shaderModel >= 3)
        {
            pixelHLSL += "    gl_FragCoord.x = input.dx_VPos.x + 0.5;\n"
                         "    gl_FragCoord.y = input.dx_VPos.y + 0.5;\n";
        }
        else
        {
            // dx_ViewCoords contains the viewport width/2, height/2, center.x and center.y. See Renderer::setViewport()
            pixelHLSL += "    gl_FragCoord.x = (input.gl_FragCoord.x * rhw) * dx_ViewCoords.x + dx_ViewCoords.z;\n"
                         "    gl_FragCoord.y = (input.gl_FragCoord.y * rhw) * dx_ViewCoords.y + dx_ViewCoords.w;\n";
        }
        
        pixelHLSL += "    gl_FragCoord.z = (input.gl_FragCoord.z * rhw) * dx_DepthFront.x + dx_DepthFront.y;\n"
                     "    gl_FragCoord.w = rhw;\n";
    }

    if (fragmentShader->mUsesPointCoord && shaderModel >= 3)
    {
        pixelHLSL += "    gl_PointCoord.x = input.gl_PointCoord.x;\n";
        pixelHLSL += "    gl_PointCoord.y = 1.0 - input.gl_PointCoord.y;\n";
    }

    if (fragmentShader->mUsesFrontFacing)
    {
        if (shaderModel <= 3)
        {
            pixelHLSL += "    gl_FrontFacing = (vFace * dx_DepthFront.z >= 0.0);\n";
        }
        else
        {
            pixelHLSL += "    gl_FrontFacing = isFrontFace;\n";
        }
    }

    for (VaryingList::iterator varying = fragmentShader->mVaryings.begin(); varying != fragmentShader->mVaryings.end(); varying++)
    {
        if (varying->reg >= 0)
        {
            for (int i = 0; i < varying->size; i++)
            {
                int rows = VariableRowCount(varying->type);
                for (int j = 0; j < rows; j++)
                {
                    std::string n = str(varying->reg + i * rows + j);
                    pixelHLSL += "    " + varying->name;

                    if (varying->array)
                    {
                        pixelHLSL += "[" + str(i) + "]";
                    }

                    if (rows > 1)
                    {
                        pixelHLSL += "[" + str(j) + "]";
                    }

                    switch (VariableColumnCount(varying->type))
                    {
                      case 1: pixelHLSL += " = input.v" + n + ".x;\n";   break;
                      case 2: pixelHLSL += " = input.v" + n + ".xy;\n";  break;
                      case 3: pixelHLSL += " = input.v" + n + ".xyz;\n"; break;
                      case 4: pixelHLSL += " = input.v" + n + ";\n";     break;
                      default: UNREACHABLE();
                    }
                }
            }
        }
        else UNREACHABLE();
    }

    pixelHLSL += "\n"
                 "    gl_main();\n"
                 "\n"
                 "    PS_OUTPUT output;\n";

    for (unsigned int renderTargetIndex = 0; renderTargetIndex < numRenderTargets; renderTargetIndex++)
    {
        unsigned int sourceColorIndex = broadcast ? 0 : renderTargetIndex;

        pixelHLSL += "    output.gl_Color" + str(renderTargetIndex) + " = gl_Color[" + str(sourceColorIndex) + "];\n";
    }

    if (fragmentShader->mUsesFragDepth)
    {
        pixelHLSL += "    output.gl_Depth = gl_Depth;\n";
    }

    pixelHLSL += "\n"
                 "    return output;\n"
                 "}\n";

    return true;
}

bool ProgramBinary::load(InfoLog &infoLog, const void *binary, GLsizei length)
{
    BinaryInputStream stream(binary, length);

    int format = 0;
    stream.read(&format);
    if (format != GL_PROGRAM_BINARY_ANGLE)
    {
        infoLog.append("Invalid program binary format.");
        return false;
    }

    int majorVersion = 0;
    int minorVersion = 0;
    stream.read(&majorVersion);
    stream.read(&minorVersion);
    if (majorVersion != ANGLE_MAJOR_VERSION || minorVersion != ANGLE_MINOR_VERSION)
    {
        infoLog.append("Invalid program binary version.");
        return false;
    }

#if !defined(ANGLE_ENABLE_UNIVERSAL_BINARY)
    unsigned char commitString[ANGLE_COMMIT_HASH_SIZE];
    stream.read(commitString, ANGLE_COMMIT_HASH_SIZE);
    if (memcmp(commitString, ANGLE_COMMIT_HASH, sizeof(unsigned char) * ANGLE_COMMIT_HASH_SIZE) != 0)
    {
        infoLog.append("Invalid program binary version.");
        return false;
    }

    int compileFlags = 0;
    stream.read(&compileFlags);
    if (compileFlags != ANGLE_COMPILE_OPTIMIZATION_LEVEL)
    {
        infoLog.append("Mismatched compilation flags.");
        return false;
    }
#endif

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; ++i)
    {
        stream.read(&mLinkedAttribute[i].type);
        std::string name;
        stream.read(&name);
        mLinkedAttribute[i].name = name;
        stream.read(&mSemanticIndex[i]);
    }

    initAttributesByLayout();

    for (unsigned int i = 0; i < MAX_TEXTURE_IMAGE_UNITS; ++i)
    {
        stream.read(&mSamplersPS[i].active);
        stream.read(&mSamplersPS[i].logicalTextureUnit);
        
        int textureType;
        stream.read(&textureType);
        mSamplersPS[i].textureType = (TextureType) textureType;
    }

    for (unsigned int i = 0; i < IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS; ++i)
    {
        stream.read(&mSamplersVS[i].active);
        stream.read(&mSamplersVS[i].logicalTextureUnit);
        
        int textureType;
        stream.read(&textureType);
        mSamplersVS[i].textureType = (TextureType) textureType;
    }

    stream.read(&mUsedVertexSamplerRange);
    stream.read(&mUsedPixelSamplerRange);
    stream.read(&mUsesPointSize);

    size_t size;
    stream.read(&size);
    if (stream.error())
    {
        infoLog.append("Invalid program binary.");
        return false;
    }

    mUniforms.resize(size);
    for (unsigned int i = 0; i < size; ++i)
    {
        GLenum type;
        GLenum precision;
        std::string name;
        unsigned int arraySize;

        stream.read(&type);
        stream.read(&precision);
        stream.read(&name);
        stream.read(&arraySize);

        mUniforms[i] = new Uniform(type, precision, name, arraySize);
        
        stream.read(&mUniforms[i]->psRegisterIndex);
        stream.read(&mUniforms[i]->vsRegisterIndex);
        stream.read(&mUniforms[i]->registerCount);
    }

    stream.read(&size);
    if (stream.error())
    {
        infoLog.append("Invalid program binary.");
        return false;
    }

    mUniformIndex.resize(size);
    for (unsigned int i = 0; i < size; ++i)
    {
        stream.read(&mUniformIndex[i].name);
        stream.read(&mUniformIndex[i].element);
        stream.read(&mUniformIndex[i].index);
    }

    unsigned int pixelShaderSize;
    stream.read(&pixelShaderSize);

    unsigned int vertexShaderSize;
    stream.read(&vertexShaderSize);

    unsigned int geometryShaderSize;
    stream.read(&geometryShaderSize);

    const char *ptr = (const char*) binary + stream.offset();

#if !defined(ANGLE_ENABLE_UNIVERSAL_BINARY)
    const GUID *binaryIdentifier = (const GUID *) ptr;
    ptr += sizeof(GUID);

    GUID identifier = mRenderer->getAdapterIdentifier();
    if (memcmp(&identifier, binaryIdentifier, sizeof(GUID)) != 0)
    {
        infoLog.append("Invalid program binary.");
        return false;
    }
#endif

    const char *pixelShaderFunction = ptr;
    ptr += pixelShaderSize;

    const char *vertexShaderFunction = ptr;
    ptr += vertexShaderSize;

    const char *geometryShaderFunction = geometryShaderSize > 0 ? ptr : NULL;
    ptr += geometryShaderSize;

    mPixelExecutable = mRenderer->loadExecutable(reinterpret_cast<const DWORD*>(pixelShaderFunction),
                                                 pixelShaderSize, rx::SHADER_PIXEL);
    if (!mPixelExecutable)
    {
        infoLog.append("Could not create pixel shader.");
        return false;
    }

    mVertexExecutable = mRenderer->loadExecutable(reinterpret_cast<const DWORD*>(vertexShaderFunction),
                                                  vertexShaderSize, rx::SHADER_VERTEX);
    if (!mVertexExecutable)
    {
        infoLog.append("Could not create vertex shader.");
        delete mPixelExecutable;
        mPixelExecutable = NULL;
        return false;
    }

    if (geometryShaderFunction != NULL && geometryShaderSize > 0)
    {
        mGeometryExecutable = mRenderer->loadExecutable(reinterpret_cast<const DWORD*>(geometryShaderFunction),
                                                        geometryShaderSize, rx::SHADER_GEOMETRY);
        if (!mGeometryExecutable)
        {
            infoLog.append("Could not create geometry shader.");
            delete mPixelExecutable;
            mPixelExecutable = NULL;
            delete mVertexExecutable;
            mVertexExecutable = NULL;
            return false;
        }
    }
    else
    {
        mGeometryExecutable = NULL;
    }

    return true;
}

bool ProgramBinary::save(void* binary, GLsizei bufSize, GLsizei *length)
{
    BinaryOutputStream stream;

    stream.write(GL_PROGRAM_BINARY_ANGLE);
    stream.write(ANGLE_MAJOR_VERSION);
    stream.write(ANGLE_MINOR_VERSION);
#if !defined(ANGLE_ENABLE_UNIVERSAL_BINARY)
    stream.write(ANGLE_COMMIT_HASH, ANGLE_COMMIT_HASH_SIZE);
    stream.write(ANGLE_COMPILE_OPTIMIZATION_LEVEL);
#endif
    for (unsigned int i = 0; i < MAX_VERTEX_ATTRIBS; ++i)
    {
        stream.write(mLinkedAttribute[i].type);
        stream.write(mLinkedAttribute[i].name);
        stream.write(mSemanticIndex[i]);
    }

    for (unsigned int i = 0; i < MAX_TEXTURE_IMAGE_UNITS; ++i)
    {
        stream.write(mSamplersPS[i].active);
        stream.write(mSamplersPS[i].logicalTextureUnit);
        stream.write((int) mSamplersPS[i].textureType);
    }

    for (unsigned int i = 0; i < IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS; ++i)
    {
        stream.write(mSamplersVS[i].active);
        stream.write(mSamplersVS[i].logicalTextureUnit);
        stream.write((int) mSamplersVS[i].textureType);
    }

    stream.write(mUsedVertexSamplerRange);
    stream.write(mUsedPixelSamplerRange);
    stream.write(mUsesPointSize);

    stream.write(mUniforms.size());
    for (unsigned int i = 0; i < mUniforms.size(); ++i)
    {
        stream.write(mUniforms[i]->type);
        stream.write(mUniforms[i]->precision);
        stream.write(mUniforms[i]->name);
        stream.write(mUniforms[i]->arraySize);

        stream.write(mUniforms[i]->psRegisterIndex);
        stream.write(mUniforms[i]->vsRegisterIndex);
        stream.write(mUniforms[i]->registerCount);
    }

    stream.write(mUniformIndex.size());
    for (unsigned int i = 0; i < mUniformIndex.size(); ++i)
    {
        stream.write(mUniformIndex[i].name);
        stream.write(mUniformIndex[i].element);
        stream.write(mUniformIndex[i].index);
    }

    UINT pixelShaderSize = mPixelExecutable->getLength();
    stream.write(pixelShaderSize);

    UINT vertexShaderSize = mVertexExecutable->getLength();
    stream.write(vertexShaderSize);

    UINT geometryShaderSize = (mGeometryExecutable != NULL) ? mGeometryExecutable->getLength() : 0;
    stream.write(geometryShaderSize);

#if !defined(ANGLE_ENABLE_UNIVERSAL_BINARY)
    GUID identifier = mRenderer->getAdapterIdentifier();
#endif

    GLsizei streamLength = stream.length();
    const void *streamData = stream.data();

    GLsizei totalLength = streamLength + sizeof(GUID) + pixelShaderSize + vertexShaderSize + geometryShaderSize;
    if (totalLength > bufSize)
    {
        if (length)
        {
            *length = 0;
        }

        return false;
    }

    if (binary)
    {
        char *ptr = (char*) binary;

        memcpy(ptr, streamData, streamLength);
        ptr += streamLength;

#if !defined(ANGLE_ENABLE_UNIVERSAL_BINARY)
        memcpy(ptr, &identifier, sizeof(GUID));
        ptr += sizeof(GUID);
#endif

        memcpy(ptr, mPixelExecutable->getFunction(), pixelShaderSize);
        ptr += pixelShaderSize;

        memcpy(ptr, mVertexExecutable->getFunction(), vertexShaderSize);
        ptr += vertexShaderSize;

        if (mGeometryExecutable != NULL && geometryShaderSize > 0)
        {
            memcpy(ptr, mGeometryExecutable->getFunction(), geometryShaderSize);
            ptr += geometryShaderSize;
        }

        ASSERT(ptr - totalLength == binary);
    }

    if (length)
    {
        *length = totalLength;
    }

    return true;
}

GLint ProgramBinary::getLength()
{
    GLint length;
    if (save(NULL, INT_MAX, &length))
    {
        return length;
    }
    else
    {
        return 0;
    }
}

bool ProgramBinary::link(InfoLog &infoLog, const AttributeBindings &attributeBindings, FragmentShader *fragmentShader, VertexShader *vertexShader)
{
    if (!fragmentShader || !fragmentShader->isCompiled())
    {
        return false;
    }

    if (!vertexShader || !vertexShader->isCompiled())
    {
        return false;
    }

    std::string pixelHLSL = fragmentShader->getHLSL();
    std::string vertexHLSL = vertexShader->getHLSL();

    // Map the varyings to the register file
    const Varying *packing[IMPLEMENTATION_MAX_VARYING_VECTORS][4] = {NULL};
    int registers = packVaryings(infoLog, packing, fragmentShader);

    if (registers < 0)
    {
        return false;
    }

    if (!linkVaryings(infoLog, registers, packing, pixelHLSL, vertexHLSL, fragmentShader, vertexShader))
    {
        return false;
    }

    bool success = true;

    if (!linkAttributes(infoLog, attributeBindings, fragmentShader, vertexShader))
    {
        success = false;
    }

    if (!linkUniforms(infoLog, vertexShader->getUniforms(), fragmentShader->getUniforms()))
    {
        success = false;
    }

    // special case for gl_DepthRange, the only built-in uniform (also a struct)
    if (vertexShader->mUsesDepthRange || fragmentShader->mUsesDepthRange)
    {
        mUniforms.push_back(new Uniform(GL_FLOAT, GL_HIGH_FLOAT, "gl_DepthRange.near", 0));
        mUniforms.push_back(new Uniform(GL_FLOAT, GL_HIGH_FLOAT, "gl_DepthRange.far", 0));
        mUniforms.push_back(new Uniform(GL_FLOAT, GL_HIGH_FLOAT, "gl_DepthRange.diff", 0));
    }

    if (success)
    {
        mVertexExecutable = mRenderer->compileToExecutable(infoLog, vertexHLSL.c_str(), rx::SHADER_VERTEX, DiscardWorkaround(vertexShader->mUsesDiscardRewriting));
        mPixelExecutable = mRenderer->compileToExecutable(infoLog, pixelHLSL.c_str(), rx::SHADER_PIXEL, DiscardWorkaround(fragmentShader->mUsesDiscardRewriting));

        if (usesGeometryShader())
        {
            std::string geometryHLSL = generateGeometryShaderHLSL(registers, packing, fragmentShader, vertexShader);
            mGeometryExecutable = mRenderer->compileToExecutable(infoLog, geometryHLSL.c_str(), rx::SHADER_GEOMETRY, rx::ANGLE_D3D_WORKAROUND_NONE);
        }

        if (!mVertexExecutable || !mPixelExecutable || (usesGeometryShader() && !mGeometryExecutable))
        {
            infoLog.append("Failed to create D3D shaders.");
            success = false;

            delete mVertexExecutable;
            mVertexExecutable = NULL;
            delete mPixelExecutable;
            mPixelExecutable = NULL;
            delete mGeometryExecutable;
            mGeometryExecutable = NULL;
        }
    }

    return success;
}

// Determines the mapping between GL attributes and Direct3D 9 vertex stream usage indices
bool ProgramBinary::linkAttributes(InfoLog &infoLog, const AttributeBindings &attributeBindings, FragmentShader *fragmentShader, VertexShader *vertexShader)
{
    unsigned int usedLocations = 0;

    // Link attributes that have a binding location
    for (AttributeArray::iterator attribute = vertexShader->mAttributes.begin(); attribute != vertexShader->mAttributes.end(); attribute++)
    {
        int location = attributeBindings.getAttributeBinding(attribute->name);

        if (location != -1)   // Set by glBindAttribLocation
        {
            if (!mLinkedAttribute[location].name.empty())
            {
                // Multiple active attributes bound to the same location; not an error
            }

            mLinkedAttribute[location] = *attribute;

            int rows = VariableRowCount(attribute->type);

            if (rows + location > MAX_VERTEX_ATTRIBS)
            {
                infoLog.append("Active attribute (%s) at location %d is too big to fit", attribute->name.c_str(), location);

                return false;
            }

            for (int i = 0; i < rows; i++)
            {
                usedLocations |= 1 << (location + i);
            }
        }
    }

    // Link attributes that don't have a binding location
    for (AttributeArray::iterator attribute = vertexShader->mAttributes.begin(); attribute != vertexShader->mAttributes.end(); attribute++)
    {
        int location = attributeBindings.getAttributeBinding(attribute->name);

        if (location == -1)   // Not set by glBindAttribLocation
        {
            int rows = VariableRowCount(attribute->type);
            int availableIndex = AllocateFirstFreeBits(&usedLocations, rows, MAX_VERTEX_ATTRIBS);

            if (availableIndex == -1 || availableIndex + rows > MAX_VERTEX_ATTRIBS)
            {
                infoLog.append("Too many active attributes (%s)", attribute->name.c_str());

                return false;   // Fail to link
            }

            mLinkedAttribute[availableIndex] = *attribute;
        }
    }

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; )
    {
        int index = vertexShader->getSemanticIndex(mLinkedAttribute[attributeIndex].name);
        int rows = std::max(VariableRowCount(mLinkedAttribute[attributeIndex].type), 1);

        for (int r = 0; r < rows; r++)
        {
            mSemanticIndex[attributeIndex++] = index++;
        }
    }

    initAttributesByLayout();

    return true;
}

bool ProgramBinary::linkUniforms(InfoLog &infoLog, const sh::ActiveUniforms &vertexUniforms, const sh::ActiveUniforms &fragmentUniforms)
{
    for (sh::ActiveUniforms::const_iterator uniform = vertexUniforms.begin(); uniform != vertexUniforms.end(); uniform++)
    {
        if (!defineUniform(GL_VERTEX_SHADER, *uniform, infoLog))
        {
            return false;
        }
    }

    for (sh::ActiveUniforms::const_iterator uniform = fragmentUniforms.begin(); uniform != fragmentUniforms.end(); uniform++)
    {
        if (!defineUniform(GL_FRAGMENT_SHADER, *uniform, infoLog))
        {
            return false;
        }
    }

    return true;
}

bool ProgramBinary::defineUniform(GLenum shader, const sh::Uniform &constant, InfoLog &infoLog)
{
    if (constant.type == GL_SAMPLER_2D ||
        constant.type == GL_SAMPLER_CUBE)
    {
        unsigned int samplerIndex = constant.registerIndex;
            
        do
        {
            if (shader == GL_VERTEX_SHADER)
            {
                if (samplerIndex < mRenderer->getMaxVertexTextureImageUnits())
                {
                    mSamplersVS[samplerIndex].active = true;
                    mSamplersVS[samplerIndex].textureType = (constant.type == GL_SAMPLER_CUBE) ? TEXTURE_CUBE : TEXTURE_2D;
                    mSamplersVS[samplerIndex].logicalTextureUnit = 0;
                    mUsedVertexSamplerRange = std::max(samplerIndex + 1, mUsedVertexSamplerRange);
                }
                else
                {
                    infoLog.append("Vertex shader sampler count exceeds the maximum vertex texture units (%d).", mRenderer->getMaxVertexTextureImageUnits());
                    return false;
                }
            }
            else if (shader == GL_FRAGMENT_SHADER)
            {
                if (samplerIndex < MAX_TEXTURE_IMAGE_UNITS)
                {
                    mSamplersPS[samplerIndex].active = true;
                    mSamplersPS[samplerIndex].textureType = (constant.type == GL_SAMPLER_CUBE) ? TEXTURE_CUBE : TEXTURE_2D;
                    mSamplersPS[samplerIndex].logicalTextureUnit = 0;
                    mUsedPixelSamplerRange = std::max(samplerIndex + 1, mUsedPixelSamplerRange);
                }
                else
                {
                    infoLog.append("Pixel shader sampler count exceeds MAX_TEXTURE_IMAGE_UNITS (%d).", MAX_TEXTURE_IMAGE_UNITS);
                    return false;
                }
            }
            else UNREACHABLE();

            samplerIndex++;
        }
        while (samplerIndex < constant.registerIndex + constant.arraySize);
    }

    Uniform *uniform = NULL;
    GLint location = getUniformLocation(constant.name);

    if (location >= 0)   // Previously defined, type and precision must match
    {
        uniform = mUniforms[mUniformIndex[location].index];

        if (uniform->type != constant.type)
        {
            infoLog.append("Types for uniform %s do not match between the vertex and fragment shader", uniform->name.c_str());
            return false;
        }

        if (uniform->precision != constant.precision)
        {
            infoLog.append("Precisions for uniform %s do not match between the vertex and fragment shader", uniform->name.c_str());
            return false;
        }
    }
    else
    {
        uniform = new Uniform(constant.type, constant.precision, constant.name, constant.arraySize);
    }

    if (!uniform)
    {
        return false;
    }

    if (shader == GL_FRAGMENT_SHADER)
    {
        uniform->psRegisterIndex = constant.registerIndex;
    }
    else if (shader == GL_VERTEX_SHADER)
    {
        uniform->vsRegisterIndex = constant.registerIndex;
    }
    else UNREACHABLE();

    if (location >= 0)
    {
        return uniform->type == constant.type;
    }

    mUniforms.push_back(uniform);
    unsigned int uniformIndex = mUniforms.size() - 1;

    for (unsigned int i = 0; i < uniform->elementCount(); i++)
    {
        mUniformIndex.push_back(UniformLocation(constant.name, i, uniformIndex));
    }

    if (shader == GL_VERTEX_SHADER)
    {
        if (constant.registerIndex + uniform->registerCount > mRenderer->getReservedVertexUniformVectors() + mRenderer->getMaxVertexUniformVectors())
        {
            infoLog.append("Vertex shader active uniforms exceed GL_MAX_VERTEX_UNIFORM_VECTORS (%u)", mRenderer->getMaxVertexUniformVectors());
            return false;
        }
    }
    else if (shader == GL_FRAGMENT_SHADER)
    {
        if (constant.registerIndex + uniform->registerCount > mRenderer->getReservedFragmentUniformVectors() + mRenderer->getMaxFragmentUniformVectors())
        {
            infoLog.append("Fragment shader active uniforms exceed GL_MAX_FRAGMENT_UNIFORM_VECTORS (%u)", mRenderer->getMaxFragmentUniformVectors());
            return false;
        }
    }
    else UNREACHABLE();

    return true;
}

std::string ProgramBinary::generateGeometryShaderHLSL(int registers, const Varying *packing[][4], FragmentShader *fragmentShader, VertexShader *vertexShader) const
{
    // for now we only handle point sprite emulation
    ASSERT(usesPointSpriteEmulation());
    return generatePointSpriteHLSL(registers, packing, fragmentShader, vertexShader);
}

std::string ProgramBinary::generatePointSpriteHLSL(int registers, const Varying *packing[][4], FragmentShader *fragmentShader, VertexShader *vertexShader) const
{
    ASSERT(registers >= 0);
    ASSERT(vertexShader->mUsesPointSize);
    ASSERT(mRenderer->getMajorShaderModel() >= 4);

    std::string geomHLSL;

    std::string varyingSemantic = "TEXCOORD";

    std::string fragCoordSemantic;
    std::string pointCoordSemantic;

    int reservedRegisterIndex = registers;

    if (fragmentShader->mUsesFragCoord)
    {
        fragCoordSemantic = varyingSemantic + str(reservedRegisterIndex++);
    }

    if (fragmentShader->mUsesPointCoord)
    {
        pointCoordSemantic = varyingSemantic + str(reservedRegisterIndex++);
    }

    geomHLSL += "uniform float4 dx_ViewCoords : register(c1);\n"
                "\n"
                "struct GS_INPUT\n"
                "{\n";

    for (int r = 0; r < registers; r++)
    {
        int registerSize = packing[r][3] ? 4 : (packing[r][2] ? 3 : (packing[r][1] ? 2 : 1));

        geomHLSL += "    float" + str(registerSize) + " v" + str(r) + " : " + varyingSemantic + str(r) + ";\n";
    }

    if (fragmentShader->mUsesFragCoord)
    {
        geomHLSL += "    float4 gl_FragCoord : " + fragCoordSemantic + ";\n";
    }

    geomHLSL += "    float gl_PointSize : PSIZE;\n"
                "    float4 gl_Position : SV_Position;\n"
                "};\n"
                "\n"
                "struct GS_OUTPUT\n"
                "{\n";

    for (int r = 0; r < registers; r++)
    {
        int registerSize = packing[r][3] ? 4 : (packing[r][2] ? 3 : (packing[r][1] ? 2 : 1));

        geomHLSL += "    float" + str(registerSize) + " v" + str(r) + " : " + varyingSemantic + str(r) + ";\n";
    }

    if (fragmentShader->mUsesFragCoord)
    {
        geomHLSL += "    float4 gl_FragCoord : " + fragCoordSemantic + ";\n";
    }

    if (fragmentShader->mUsesPointCoord)
    {
        geomHLSL += "    float2 gl_PointCoord : " + pointCoordSemantic + ";\n";
    }

    geomHLSL +=   "    float gl_PointSize : PSIZE;\n"
                  "    float4 gl_Position : SV_Position;\n"
                  "};\n"
                  "\n"
                  "static float2 pointSpriteCorners[] = \n"
                  "{\n"
                  "    float2( 0.5f, -0.5f),\n"
                  "    float2( 0.5f,  0.5f),\n"
                  "    float2(-0.5f, -0.5f),\n"
                  "    float2(-0.5f,  0.5f)\n"
                  "};\n"
                  "\n"
                  "static float2 pointSpriteTexcoords[] = \n"
                  "{\n"
                  "    float2(1.0f, 1.0f),\n"
                  "    float2(1.0f, 0.0f),\n"
                  "    float2(0.0f, 1.0f),\n"
                  "    float2(0.0f, 0.0f)\n"
                  "};\n"
                  "\n"
                  "static float minPointSize = " + str(ALIASED_POINT_SIZE_RANGE_MIN) + ".0f;\n"
                  "static float maxPointSize = " + str(mRenderer->getMaxPointSize()) + ".0f;\n"
                  "\n"
                  "[maxvertexcount(4)]\n"
                  "void main(point GS_INPUT input[1], inout TriangleStream<GS_OUTPUT> outStream)\n"
                  "{\n"
                  "    GS_OUTPUT output = (GS_OUTPUT)0;\n"
                  "    output.gl_PointSize = input[0].gl_PointSize;\n";

    for (int r = 0; r < registers; r++)
    {
        geomHLSL += "    output.v" + str(r) + " = input[0].v" + str(r) + ";\n";
    }

    if (fragmentShader->mUsesFragCoord)
    {
        geomHLSL += "    output.gl_FragCoord = input[0].gl_FragCoord;\n";
    }

    geomHLSL += "    \n"
                "    float gl_PointSize = clamp(input[0].gl_PointSize, minPointSize, maxPointSize);\n"
                "    float4 gl_Position = input[0].gl_Position;\n"
                "    float2 viewportScale = float2(1.0f / dx_ViewCoords.x, 1.0f / dx_ViewCoords.y) * gl_Position.w;\n";

    for (int corner = 0; corner < 4; corner++)
    {
        geomHLSL += "    \n"
                    "    output.gl_Position = gl_Position + float4(pointSpriteCorners[" + str(corner) + "] * viewportScale * gl_PointSize, 0.0f, 0.0f);\n";

        if (fragmentShader->mUsesPointCoord)
        {
            geomHLSL += "    output.gl_PointCoord = pointSpriteTexcoords[" + str(corner) + "];\n";
        }

        geomHLSL += "    outStream.Append(output);\n";
    }

    geomHLSL += "    \n"
                "    outStream.RestartStrip();\n"
                "}\n";

    return geomHLSL;
}

// This method needs to match OutputHLSL::decorate
std::string ProgramBinary::decorateAttribute(const std::string &name)
{
    if (name.compare(0, 3, "gl_") != 0 && name.compare(0, 3, "dx_") != 0)
    {
        return "_" + name;
    }
    
    return name;
}

bool ProgramBinary::isValidated() const 
{
    return mValidated;
}

void ProgramBinary::getActiveAttribute(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const
{
    // Skip over inactive attributes
    unsigned int activeAttribute = 0;
    unsigned int attribute;
    for (attribute = 0; attribute < MAX_VERTEX_ATTRIBS; attribute++)
    {
        if (mLinkedAttribute[attribute].name.empty())
        {
            continue;
        }

        if (activeAttribute == index)
        {
            break;
        }

        activeAttribute++;
    }

    if (bufsize > 0)
    {
        const char *string = mLinkedAttribute[attribute].name.c_str();

        strncpy(name, string, bufsize);
        name[bufsize - 1] = '\0';

        if (length)
        {
            *length = strlen(name);
        }
    }

    *size = 1;   // Always a single 'type' instance

    *type = mLinkedAttribute[attribute].type;
}

GLint ProgramBinary::getActiveAttributeCount() const
{
    int count = 0;

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        if (!mLinkedAttribute[attributeIndex].name.empty())
        {
            count++;
        }
    }

    return count;
}

GLint ProgramBinary::getActiveAttributeMaxLength() const
{
    int maxLength = 0;

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        if (!mLinkedAttribute[attributeIndex].name.empty())
        {
            maxLength = std::max((int)(mLinkedAttribute[attributeIndex].name.length() + 1), maxLength);
        }
    }

    return maxLength;
}

void ProgramBinary::getActiveUniform(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const
{
    ASSERT(index < mUniforms.size());   // index must be smaller than getActiveUniformCount()

    if (bufsize > 0)
    {
        std::string string = mUniforms[index]->name;

        if (mUniforms[index]->isArray())
        {
            string += "[0]";
        }

        strncpy(name, string.c_str(), bufsize);
        name[bufsize - 1] = '\0';

        if (length)
        {
            *length = strlen(name);
        }
    }

    *size = mUniforms[index]->elementCount();

    *type = mUniforms[index]->type;
}

GLint ProgramBinary::getActiveUniformCount() const
{
    return mUniforms.size();
}

GLint ProgramBinary::getActiveUniformMaxLength() const
{
    int maxLength = 0;

    unsigned int numUniforms = mUniforms.size();
    for (unsigned int uniformIndex = 0; uniformIndex < numUniforms; uniformIndex++)
    {
        if (!mUniforms[uniformIndex]->name.empty())
        {
            int length = (int)(mUniforms[uniformIndex]->name.length() + 1);
            if (mUniforms[uniformIndex]->isArray())
            {
                length += 3;  // Counting in "[0]".
            }
            maxLength = std::max(length, maxLength);
        }
    }

    return maxLength;
}

void ProgramBinary::validate(InfoLog &infoLog)
{
    applyUniforms();
    if (!validateSamplers(&infoLog))
    {
        mValidated = false;
    }
    else
    {
        mValidated = true;
    }
}

bool ProgramBinary::validateSamplers(InfoLog *infoLog)
{
    // if any two active samplers in a program are of different types, but refer to the same
    // texture image unit, and this is the current program, then ValidateProgram will fail, and
    // DrawArrays and DrawElements will issue the INVALID_OPERATION error.

    const unsigned int maxCombinedTextureImageUnits = mRenderer->getMaxCombinedTextureImageUnits();
    TextureType textureUnitType[IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS];

    for (unsigned int i = 0; i < IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS; ++i)
    {
        textureUnitType[i] = TEXTURE_UNKNOWN;
    }

    for (unsigned int i = 0; i < mUsedPixelSamplerRange; ++i)
    {
        if (mSamplersPS[i].active)
        {
            unsigned int unit = mSamplersPS[i].logicalTextureUnit;
            
            if (unit >= maxCombinedTextureImageUnits)
            {
                if (infoLog)
                {
                    infoLog->append("Sampler uniform (%d) exceeds IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS (%d)", unit, maxCombinedTextureImageUnits);
                }

                return false;
            }

            if (textureUnitType[unit] != TEXTURE_UNKNOWN)
            {
                if (mSamplersPS[i].textureType != textureUnitType[unit])
                {
                    if (infoLog)
                    {
                        infoLog->append("Samplers of conflicting types refer to the same texture image unit (%d).", unit);
                    }

                    return false;
                }
            }
            else
            {
                textureUnitType[unit] = mSamplersPS[i].textureType;
            }
        }
    }

    for (unsigned int i = 0; i < mUsedVertexSamplerRange; ++i)
    {
        if (mSamplersVS[i].active)
        {
            unsigned int unit = mSamplersVS[i].logicalTextureUnit;
            
            if (unit >= maxCombinedTextureImageUnits)
            {
                if (infoLog)
                {
                    infoLog->append("Sampler uniform (%d) exceeds IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS (%d)", unit, maxCombinedTextureImageUnits);
                }

                return false;
            }

            if (textureUnitType[unit] != TEXTURE_UNKNOWN)
            {
                if (mSamplersVS[i].textureType != textureUnitType[unit])
                {
                    if (infoLog)
                    {
                        infoLog->append("Samplers of conflicting types refer to the same texture image unit (%d).", unit);
                    }

                    return false;
                }
            }
            else
            {
                textureUnitType[unit] = mSamplersVS[i].textureType;
            }
        }
    }

    return true;
}

ProgramBinary::Sampler::Sampler() : active(false), logicalTextureUnit(0), textureType(TEXTURE_2D)
{
}

struct AttributeSorter
{
    AttributeSorter(const int (&semanticIndices)[MAX_VERTEX_ATTRIBS])
        : originalIndices(semanticIndices)
    {
    }

    bool operator()(int a, int b)
    {
        if (originalIndices[a] == -1) return false;
        if (originalIndices[b] == -1) return true;
        return (originalIndices[a] < originalIndices[b]);
    }

    const int (&originalIndices)[MAX_VERTEX_ATTRIBS];
};

void ProgramBinary::initAttributesByLayout()
{
    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        mAttributesByLayout[i] = i;
    }

    std::sort(&mAttributesByLayout[0], &mAttributesByLayout[MAX_VERTEX_ATTRIBS], AttributeSorter(mSemanticIndex));
}

void ProgramBinary::sortAttributesByLayout(rx::TranslatedAttribute attributes[MAX_VERTEX_ATTRIBS], int sortedSemanticIndices[MAX_VERTEX_ATTRIBS]) const
{
    rx::TranslatedAttribute oldTranslatedAttributes[MAX_VERTEX_ATTRIBS];

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        oldTranslatedAttributes[i] = attributes[i];
    }

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        int oldIndex = mAttributesByLayout[i];
        sortedSemanticIndices[i] = oldIndex;
        attributes[i] = oldTranslatedAttributes[oldIndex];
    }
}

}
