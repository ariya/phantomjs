//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Program.cpp: Implements the gl::Program class. Implements GL program objects
// and related functionality. [OpenGL ES 2.0.24] section 2.10.3 page 28.

#include "libGLESv2/Program.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/ResourceManager.h"
#include "libGLESv2/renderer/Renderer.h"

namespace gl
{
const char * const g_fakepath = "C:\\fakepath";

AttributeBindings::AttributeBindings()
{
}

AttributeBindings::~AttributeBindings()
{
}

InfoLog::InfoLog() : mInfoLog(NULL)
{
}

InfoLog::~InfoLog()
{
    delete[] mInfoLog;
}


int InfoLog::getLength() const
{
    if (!mInfoLog)
    {
        return 0;
    }
    else
    {
       return strlen(mInfoLog) + 1;
    }
}

void InfoLog::getLog(GLsizei bufSize, GLsizei *length, char *infoLog)
{
    int index = 0;

    if (bufSize > 0)
    {
        if (mInfoLog)
        {
            index = std::min(bufSize - 1, (int)strlen(mInfoLog));
            memcpy(infoLog, mInfoLog, index);
        }

        infoLog[index] = '\0';
    }

    if (length)
    {
        *length = index;
    }
}

// append a santized message to the program info log.
// The D3D compiler includes a fake file path in some of the warning or error 
// messages, so lets remove all occurrences of this fake file path from the log.
void InfoLog::appendSanitized(const char *message)
{
    std::string msg(message);

    size_t found;
    do
    {
        found = msg.find(g_fakepath);
        if (found != std::string::npos)
        {
            msg.erase(found, strlen(g_fakepath));
        }
    }
    while (found != std::string::npos);

    append("%s", msg.c_str());
}

void InfoLog::append(const char *format, ...)
{
    if (!format)
    {
        return;
    }

    va_list vararg;
    va_start(vararg, format);
    size_t infoLength = vsnprintf(NULL, 0, format, vararg);
    va_end(vararg);

    char *logPointer = NULL;

    if (!mInfoLog)
    {
        mInfoLog = new char[infoLength + 2];
        logPointer = mInfoLog;
    }
    else
    {
        size_t currentlogLength = strlen(mInfoLog);
        char *newLog = new char[currentlogLength + infoLength + 2];
        strcpy(newLog, mInfoLog);

        delete[] mInfoLog;
        mInfoLog = newLog;

        logPointer = mInfoLog + currentlogLength;
    }

    va_start(vararg, format);
    vsnprintf(logPointer, infoLength, format, vararg);
    va_end(vararg);

    logPointer[infoLength] = 0;
    strcpy(logPointer + infoLength, "\n");
}

void InfoLog::reset()
{
    if (mInfoLog)
    {
        delete [] mInfoLog;
        mInfoLog = NULL;
    }
}

Program::Program(rx::Renderer *renderer, ResourceManager *manager, GLuint handle) : mResourceManager(manager), mHandle(handle)
{
    mFragmentShader = NULL;
    mVertexShader = NULL;
    mProgramBinary.set(NULL);
    mDeleteStatus = false;
    mLinked = false;
    mRefCount = 0;
    mRenderer = renderer;

    resetUniformBlockBindings();
}

Program::~Program()
{
    unlink(true);

    if (mVertexShader != NULL)
    {
        mVertexShader->release();
    }

    if (mFragmentShader != NULL)
    {
        mFragmentShader->release();
    }
}

bool Program::attachShader(Shader *shader)
{
    if (shader->getType() == GL_VERTEX_SHADER)
    {
        if (mVertexShader)
        {
            return false;
        }

        mVertexShader = shader;
        mVertexShader->addRef();
    }
    else if (shader->getType() == GL_FRAGMENT_SHADER)
    {
        if (mFragmentShader)
        {
            return false;
        }

        mFragmentShader = shader;
        mFragmentShader->addRef();
    }
    else UNREACHABLE();

    return true;
}

bool Program::detachShader(Shader *shader)
{
    if (shader->getType() == GL_VERTEX_SHADER)
    {
        if (mVertexShader != shader)
        {
            return false;
        }

        mVertexShader->release();
        mVertexShader = NULL;
    }
    else if (shader->getType() == GL_FRAGMENT_SHADER)
    {
        if (mFragmentShader != shader)
        {
            return false;
        }

        mFragmentShader->release();
        mFragmentShader = NULL;
    }
    else UNREACHABLE();

    return true;
}

int Program::getAttachedShadersCount() const
{
    return (mVertexShader ? 1 : 0) + (mFragmentShader ? 1 : 0);
}

void AttributeBindings::bindAttributeLocation(GLuint index, const char *name)
{
    if (index < MAX_VERTEX_ATTRIBS)
    {
        for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
        {
            mAttributeBinding[i].erase(name);
        }

        mAttributeBinding[index].insert(name);
    }
}

void Program::bindAttributeLocation(GLuint index, const char *name)
{
    mAttributeBindings.bindAttributeLocation(index, name);
}

// Links the HLSL code of the vertex and pixel shader by matching up their varyings,
// compiling them into binaries, determining the attribute mappings, and collecting
// a list of uniforms
Error Program::link(const Data &data)
{
    unlink(false);

    mInfoLog.reset();
    resetUniformBlockBindings();

    mProgramBinary.set(new ProgramBinary(mRenderer->createProgram()));
    LinkResult result = mProgramBinary->link(data, mInfoLog, mAttributeBindings, mFragmentShader, mVertexShader,
                                             mTransformFeedbackVaryings, mTransformFeedbackBufferMode);
    if (result.error.isError())
    {
        return result.error;
    }

    mLinked = result.linkSuccess;
    return gl::Error(GL_NO_ERROR);
}

int AttributeBindings::getAttributeBinding(const std::string &name) const
{
    for (int location = 0; location < MAX_VERTEX_ATTRIBS; location++)
    {
        if (mAttributeBinding[location].find(name) != mAttributeBinding[location].end())
        {
            return location;
        }
    }

    return -1;
}

// Returns the program object to an unlinked state, before re-linking, or at destruction
void Program::unlink(bool destroy)
{
    if (destroy)   // Object being destructed
    {
        if (mFragmentShader)
        {
            mFragmentShader->release();
            mFragmentShader = NULL;
        }

        if (mVertexShader)
        {
            mVertexShader->release();
            mVertexShader = NULL;
        }
    }

    mProgramBinary.set(NULL);
    mLinked = false;
}

bool Program::isLinked()
{
    return mLinked;
}

ProgramBinary* Program::getProgramBinary() const
{
    return mProgramBinary.get();
}

Error Program::setProgramBinary(GLenum binaryFormat, const void *binary, GLsizei length)
{
    unlink(false);

    mInfoLog.reset();

    mProgramBinary.set(new ProgramBinary(mRenderer->createProgram()));
    LinkResult result = mProgramBinary->load(mInfoLog, binaryFormat, binary, length);
    if (result.error.isError())
    {
        mProgramBinary.set(NULL);
        return result.error;
    }

    mLinked = result.linkSuccess;
    return Error(GL_NO_ERROR);
}

void Program::release()
{
    mRefCount--;

    if (mRefCount == 0 && mDeleteStatus)
    {
        mResourceManager->deleteProgram(mHandle);
    }
}

void Program::addRef()
{
    mRefCount++;
}

unsigned int Program::getRefCount() const
{
    return mRefCount;
}

GLint Program::getProgramBinaryLength() const
{
    ProgramBinary *programBinary = mProgramBinary.get();
    if (programBinary)
    {
        return programBinary->getLength();
    }
    else
    {
        return 0;
    }
}

int Program::getInfoLogLength() const
{
    return mInfoLog.getLength();
}

void Program::getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog)
{
    return mInfoLog.getLog(bufSize, length, infoLog);
}

void Program::getAttachedShaders(GLsizei maxCount, GLsizei *count, GLuint *shaders)
{
    int total = 0;

    if (mVertexShader)
    {
        if (total < maxCount)
        {
            shaders[total] = mVertexShader->getHandle();
        }

        total++;
    }

    if (mFragmentShader)
    {
        if (total < maxCount)
        {
            shaders[total] = mFragmentShader->getHandle();
        }

        total++;
    }

    if (count)
    {
        *count = total;
    }
}

void Program::getActiveAttribute(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
    ProgramBinary *programBinary = getProgramBinary();
    if (programBinary)
    {
        programBinary->getActiveAttribute(index, bufsize, length, size, type, name);
    }
    else
    {
        if (bufsize > 0)
        {
            name[0] = '\0';
        }
        
        if (length)
        {
            *length = 0;
        }

        *type = GL_NONE;
        *size = 1;
    }
}

GLint Program::getActiveAttributeCount()
{
    ProgramBinary *programBinary = getProgramBinary();
    if (programBinary)
    {
        return programBinary->getActiveAttributeCount();
    }
    else
    {
        return 0;
    }
}

GLint Program::getActiveAttributeMaxLength()
{
    ProgramBinary *programBinary = getProgramBinary();
    if (programBinary)
    {
        return programBinary->getActiveAttributeMaxLength();
    }
    else
    {
        return 0;
    }
}

void Program::getActiveUniform(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
    ProgramBinary *programBinary = getProgramBinary();
    if (programBinary)
    {
        return programBinary->getActiveUniform(index, bufsize, length, size, type, name);
    }
    else
    {
        if (bufsize > 0)
        {
            name[0] = '\0';
        }

        if (length)
        {
            *length = 0;
        }

        *size = 0;
        *type = GL_NONE;
    }
}

GLint Program::getActiveUniformCount()
{
    ProgramBinary *programBinary = getProgramBinary();
    if (programBinary)
    {
        return programBinary->getActiveUniformCount();
    }
    else
    {
        return 0;
    }
}

GLint Program::getActiveUniformMaxLength()
{
    ProgramBinary *programBinary = getProgramBinary();
    if (programBinary)
    {
        return programBinary->getActiveUniformMaxLength();
    }
    else
    {
        return 0;
    }
}

void Program::flagForDeletion()
{
    mDeleteStatus = true;
}

bool Program::isFlaggedForDeletion() const
{
    return mDeleteStatus;
}

void Program::validate(const Caps &caps)
{
    mInfoLog.reset();

    ProgramBinary *programBinary = getProgramBinary();
    if (isLinked() && programBinary)
    {
        programBinary->validate(mInfoLog, caps);
    }
    else
    {
        mInfoLog.append("Program has not been successfully linked.");
    }
}

bool Program::isValidated() const
{
    ProgramBinary *programBinary = mProgramBinary.get();
    if (programBinary)
    {
        return programBinary->isValidated();
    }
    else
    {
        return false;
    }
}

GLint Program::getActiveUniformBlockCount()
{
    ProgramBinary *programBinary = getProgramBinary();
    if (programBinary)
    {
        return static_cast<GLint>(programBinary->getActiveUniformBlockCount());
    }
    else
    {
        return 0;
    }
}

GLint Program::getActiveUniformBlockMaxLength()
{
    ProgramBinary *programBinary = getProgramBinary();
    if (programBinary)
    {
        return static_cast<GLint>(programBinary->getActiveUniformBlockMaxLength());
    }
    else
    {
        return 0;
    }
}

void Program::bindUniformBlock(GLuint uniformBlockIndex, GLuint uniformBlockBinding)
{
    mUniformBlockBindings[uniformBlockIndex] = uniformBlockBinding;
}

GLuint Program::getUniformBlockBinding(GLuint uniformBlockIndex) const
{
    return mUniformBlockBindings[uniformBlockIndex];
}

void Program::resetUniformBlockBindings()
{
    for (unsigned int blockId = 0; blockId < IMPLEMENTATION_MAX_COMBINED_SHADER_UNIFORM_BUFFERS; blockId++)
    {
        mUniformBlockBindings[blockId] = 0;
    }
}

void Program::setTransformFeedbackVaryings(GLsizei count, const GLchar *const *varyings, GLenum bufferMode)
{
    mTransformFeedbackVaryings.resize(count);
    for (GLsizei i = 0; i < count; i++)
    {
        mTransformFeedbackVaryings[i] = varyings[i];
    }

    mTransformFeedbackBufferMode = bufferMode;
}

void Program::getTransformFeedbackVarying(GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name) const
{
    ProgramBinary *programBinary = getProgramBinary();
    if (programBinary && index < programBinary->getTransformFeedbackVaryingCount())
    {
        const LinkedVarying &varying = programBinary->getTransformFeedbackVarying(index);
        GLsizei lastNameIdx = std::min(bufSize - 1, static_cast<GLsizei>(varying.name.length()));
        if (length)
        {
            *length = lastNameIdx;
        }
        if (size)
        {
            *size = varying.size;
        }
        if (type)
        {
            *type = varying.type;
        }
        if (name)
        {
            memcpy(name, varying.name.c_str(), lastNameIdx);
            name[lastNameIdx] = '\0';
        }
    }
}

GLsizei Program::getTransformFeedbackVaryingCount() const
{
    ProgramBinary *programBinary = getProgramBinary();
    if (programBinary)
    {
        return static_cast<GLsizei>(programBinary->getTransformFeedbackVaryingCount());
    }
    else
    {
        return 0;
    }
}

GLsizei Program::getTransformFeedbackVaryingMaxLength() const
{
    ProgramBinary *programBinary = getProgramBinary();
    if (programBinary)
    {
        GLsizei maxSize = 0;
        for (size_t i = 0; i < programBinary->getTransformFeedbackVaryingCount(); i++)
        {
            const LinkedVarying &varying = programBinary->getTransformFeedbackVarying(i);
            maxSize = std::max(maxSize, static_cast<GLsizei>(varying.name.length() + 1));
        }

        return maxSize;
    }
    else
    {
        return 0;
    }
}

GLenum Program::getTransformFeedbackBufferMode() const
{
    ProgramBinary *programBinary = getProgramBinary();
    if (programBinary)
    {
        return programBinary->getTransformFeedbackBufferMode();
    }
    else
    {
        return mTransformFeedbackBufferMode;
    }
}

}
