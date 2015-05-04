//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Shader.cpp: Implements the gl::Shader class and its  derived classes
// VertexShader and FragmentShader. Implements GL shader objects and related
// functionality. [OpenGL ES 2.0.24] section 2.10 page 24 and section 3.8 page 84.

#include "libGLESv2/Shader.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/ShaderImpl.h"
#include "libGLESv2/Constants.h"
#include "libGLESv2/ResourceManager.h"

#include "common/utilities.h"

#include "GLSLANG/ShaderLang.h"

#include <sstream>

namespace gl
{

Shader::Shader(ResourceManager *manager, rx::ShaderImpl *impl, GLenum type, GLuint handle)
    : mShader(impl),
      mType(type),
      mHandle(handle),
      mResourceManager(manager),
      mRefCount(0),
      mDeleteStatus(false),
      mCompiled(false)
{
    ASSERT(impl);
}

Shader::~Shader()
{
    SafeDelete(mShader);
}

GLuint Shader::getHandle() const
{
    return mHandle;
}

void Shader::setSource(GLsizei count, const char *const *string, const GLint *length)
{
    std::ostringstream stream;

    for (int i = 0; i < count; i++)
    {
        if (length == nullptr || length[i] < 0)
        {
            stream.write(string[i], strlen(string[i]));
        }
        else
        {
            stream.write(string[i], length[i]);
        }
    }

    mSource = stream.str();
}

int Shader::getInfoLogLength() const
{
    return  mShader->getInfoLog().empty() ? 0 : (mShader->getInfoLog().length() + 1);
}

void Shader::getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog) const
{
    int index = 0;

    if (bufSize > 0)
    {
        index = std::min(bufSize - 1, static_cast<GLsizei>(mShader->getInfoLog().length()));
        memcpy(infoLog, mShader->getInfoLog().c_str(), index);

        infoLog[index] = '\0';
    }

    if (length)
    {
        *length = index;
    }
}

int Shader::getSourceLength() const
{
    return mSource.empty() ? 0 : (mSource.length() + 1);
}

int Shader::getTranslatedSourceLength() const
{
    return mShader->getTranslatedSource().empty() ? 0 : (mShader->getTranslatedSource().length() + 1);
}

void Shader::getSourceImpl(const std::string &source, GLsizei bufSize, GLsizei *length, char *buffer)
{
    int index = 0;

    if (bufSize > 0)
    {
        index = std::min(bufSize - 1, static_cast<GLsizei>(source.length()));
        memcpy(buffer, source.c_str(), index);

        buffer[index] = '\0';
    }

    if (length)
    {
        *length = index;
    }
}

void Shader::getSource(GLsizei bufSize, GLsizei *length, char *buffer) const
{
    getSourceImpl(mSource, bufSize, length, buffer);
}

void Shader::getTranslatedSource(GLsizei bufSize, GLsizei *length, char *buffer) const
{
    getSourceImpl(mShader->getTranslatedSource(), bufSize, length, buffer);
}

void Shader::getTranslatedSourceWithDebugInfo(GLsizei bufSize, GLsizei *length, char *buffer) const
{
    std::string debugInfo(mShader->getDebugInfo());
    getSourceImpl(debugInfo, bufSize, length, buffer);
}

void Shader::compile(const gl::Data &data)
{
    mCompiled = mShader->compile(data, mSource);
}

void Shader::addRef()
{
    mRefCount++;
}

void Shader::release()
{
    mRefCount--;

    if (mRefCount == 0 && mDeleteStatus)
    {
        mResourceManager->deleteShader(mHandle);
    }
}

unsigned int Shader::getRefCount() const
{
    return mRefCount;
}

bool Shader::isFlaggedForDeletion() const
{
    return mDeleteStatus;
}

void Shader::flagForDeletion()
{
    mDeleteStatus = true;
}

const std::vector<gl::PackedVarying> &Shader::getVaryings() const
{
    return mShader->getVaryings();
}

const std::vector<sh::Uniform> &Shader::getUniforms() const
{
    return mShader->getUniforms();
}

const std::vector<sh::InterfaceBlock> &Shader::getInterfaceBlocks() const
{
    return mShader->getInterfaceBlocks();
}

const std::vector<sh::Attribute> &Shader::getActiveAttributes() const
{
    return mShader->getActiveAttributes();
}

const std::vector<sh::Attribute> &Shader::getActiveOutputVariables() const
{
    return mShader->getActiveOutputVariables();
}

std::vector<gl::PackedVarying> &Shader::getVaryings()
{
    return mShader->getVaryings();
}

std::vector<sh::Uniform> &Shader::getUniforms()
{
    return mShader->getUniforms();
}

std::vector<sh::InterfaceBlock> &Shader::getInterfaceBlocks()
{
    return mShader->getInterfaceBlocks();
}

std::vector<sh::Attribute> &Shader::getActiveAttributes()
{
    return mShader->getActiveAttributes();
}

std::vector<sh::Attribute> &Shader::getActiveOutputVariables()
{
    return mShader->getActiveOutputVariables();
}

}
