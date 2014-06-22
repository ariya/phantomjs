#include "precompiled.h"
//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Shader.cpp: Implements the gl::Shader class and its  derived classes
// VertexShader and FragmentShader. Implements GL shader objects and related
// functionality. [OpenGL ES 2.0.24] section 2.10 page 24 and section 3.8 page 84.

#include "libGLESv2/Shader.h"

#include "GLSLANG/ShaderLang.h"
#include "libGLESv2/utilities.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/Constants.h"
#include "libGLESv2/ResourceManager.h"

namespace gl
{
void *Shader::mFragmentCompiler = NULL;
void *Shader::mVertexCompiler = NULL;

Shader::Shader(ResourceManager *manager, const rx::Renderer *renderer, GLuint handle)
    : mHandle(handle), mRenderer(renderer), mResourceManager(manager)
{
    mSource = NULL;
    mHlsl = NULL;
    mInfoLog = NULL;

    uncompile();
    initializeCompiler();

    mRefCount = 0;
    mDeleteStatus = false;
}

Shader::~Shader()
{
    delete[] mSource;
    delete[] mHlsl;
    delete[] mInfoLog;
}

GLuint Shader::getHandle() const
{
    return mHandle;
}

void Shader::setSource(GLsizei count, const char **string, const GLint *length)
{
    delete[] mSource;
    int totalLength = 0;

    for (int i = 0; i < count; i++)
    {
        if (length && length[i] >= 0)
        {
            totalLength += length[i];
        }
        else
        {
            totalLength += (int)strlen(string[i]);
        }
    }

    mSource = new char[totalLength + 1];
    char *code = mSource;

    for (int i = 0; i < count; i++)
    {
        int stringLength;

        if (length && length[i] >= 0)
        {
            stringLength = length[i];
        }
        else
        {
            stringLength = (int)strlen(string[i]);
        }

        strncpy(code, string[i], stringLength);
        code += stringLength;
    }

    mSource[totalLength] = '\0';
}

int Shader::getInfoLogLength() const
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

void Shader::getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog)
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

int Shader::getSourceLength() const
{
    if (!mSource)
    {
        return 0;
    }
    else
    {
       return strlen(mSource) + 1;
    }
}

int Shader::getTranslatedSourceLength() const
{
    if (!mHlsl)
    {
        return 0;
    }
    else
    {
       return strlen(mHlsl) + 1;
    }
}

void Shader::getSourceImpl(char *source, GLsizei bufSize, GLsizei *length, char *buffer)
{
    int index = 0;

    if (bufSize > 0)
    {
        if (source)
        {
            index = std::min(bufSize - 1, (int)strlen(source));
            memcpy(buffer, source, index);
        }

        buffer[index] = '\0';
    }

    if (length)
    {
        *length = index;
    }
}

void Shader::getSource(GLsizei bufSize, GLsizei *length, char *buffer)
{
    getSourceImpl(mSource, bufSize, length, buffer);
}

void Shader::getTranslatedSource(GLsizei bufSize, GLsizei *length, char *buffer)
{
    getSourceImpl(mHlsl, bufSize, length, buffer);
}

const sh::ActiveUniforms &Shader::getUniforms()
{
    return mActiveUniforms;
}

bool Shader::isCompiled()
{
    return mHlsl != NULL;
}

const char *Shader::getHLSL()
{
    return mHlsl;
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

// Perform a one-time initialization of the shader compiler (or after being destructed by releaseCompiler)
void Shader::initializeCompiler()
{
    if (!mFragmentCompiler)
    {
        int result = ShInitialize();

        if (result)
        {
            ShShaderOutput hlslVersion = (mRenderer->getMajorShaderModel() >= 4) ? SH_HLSL11_OUTPUT : SH_HLSL9_OUTPUT;

            ShBuiltInResources resources;
            ShInitBuiltInResources(&resources);

            resources.MaxVertexAttribs = MAX_VERTEX_ATTRIBS;
            resources.MaxVertexUniformVectors = mRenderer->getMaxVertexUniformVectors();
            resources.MaxVaryingVectors = mRenderer->getMaxVaryingVectors();
            resources.MaxVertexTextureImageUnits = mRenderer->getMaxVertexTextureImageUnits();
            resources.MaxCombinedTextureImageUnits = mRenderer->getMaxCombinedTextureImageUnits();
            resources.MaxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
            resources.MaxFragmentUniformVectors = mRenderer->getMaxFragmentUniformVectors();
            resources.MaxDrawBuffers = mRenderer->getMaxRenderTargets();
            resources.OES_standard_derivatives = mRenderer->getDerivativeInstructionSupport();
            resources.EXT_draw_buffers = mRenderer->getMaxRenderTargets() > 1;
            // resources.OES_EGL_image_external = mRenderer->getShareHandleSupport() ? 1 : 0; // TODO: commented out until the extension is actually supported.
            resources.FragmentPrecisionHigh = 1;   // Shader Model 2+ always supports FP24 (s16e7) which corresponds to highp
            resources.EXT_frag_depth = 1; // Shader Model 2+ always supports explicit depth output

            mFragmentCompiler = ShConstructCompiler(SH_FRAGMENT_SHADER, SH_GLES2_SPEC, hlslVersion, &resources);
            mVertexCompiler = ShConstructCompiler(SH_VERTEX_SHADER, SH_GLES2_SPEC, hlslVersion, &resources);
        }
    }
}

void Shader::releaseCompiler()
{
    ShDestruct(mFragmentCompiler);
    ShDestruct(mVertexCompiler);

    mFragmentCompiler = NULL;
    mVertexCompiler = NULL;

    ShFinalize();
}

void Shader::parseVaryings()
{
    if (mHlsl)
    {
        const char *input = strstr(mHlsl, "// Varyings") + 12;

        while(true)
        {
            char varyingType[256];
            char varyingName[256];

            int matches = sscanf(input, "static %255s %255s", varyingType, varyingName);

            if (matches != 2)
            {
                break;
            }

            char *array = strstr(varyingName, "[");
            int size = 1;

            if (array)
            {
                size = atoi(array + 1);
                *array = '\0';
            }

            mVaryings.push_back(Varying(parseType(varyingType), varyingName, size, array != NULL));

            input = strstr(input, ";") + 2;
        }

        mUsesMultipleRenderTargets = strstr(mHlsl, "GL_USES_MRT") != NULL;
        mUsesFragColor = strstr(mHlsl, "GL_USES_FRAG_COLOR") != NULL;
        mUsesFragData = strstr(mHlsl, "GL_USES_FRAG_DATA") != NULL;
        mUsesFragCoord = strstr(mHlsl, "GL_USES_FRAG_COORD") != NULL;
        mUsesFrontFacing = strstr(mHlsl, "GL_USES_FRONT_FACING") != NULL;
        mUsesPointSize = strstr(mHlsl, "GL_USES_POINT_SIZE") != NULL;
        mUsesPointCoord = strstr(mHlsl, "GL_USES_POINT_COORD") != NULL;
        mUsesDepthRange = strstr(mHlsl, "GL_USES_DEPTH_RANGE") != NULL;
        mUsesFragDepth = strstr(mHlsl, "GL_USES_FRAG_DEPTH") != NULL;
    }
}

void Shader::resetVaryingsRegisterAssignment()
{
    for (VaryingList::iterator var = mVaryings.begin(); var != mVaryings.end(); var++)
    {
        var->reg = -1;
        var->col = -1;
    }
}

// initialize/clean up previous state
void Shader::uncompile()
{
    // set by compileToHLSL
    delete[] mHlsl;
    mHlsl = NULL;
    delete[] mInfoLog;
    mInfoLog = NULL;

    // set by parseVaryings
    mVaryings.clear();

    mUsesMultipleRenderTargets = false;
    mUsesFragColor = false;
    mUsesFragData = false;
    mUsesFragCoord = false;
    mUsesFrontFacing = false;
    mUsesPointSize = false;
    mUsesPointCoord = false;
    mUsesDepthRange = false;
    mUsesFragDepth = false;

    mActiveUniforms.clear();
}

void Shader::compileToHLSL(void *compiler)
{
    // ensure we don't pass a NULL source to the compiler
    const char *source = "\0";
    if (mSource)
    {
        source = mSource;
    }

    // ensure the compiler is loaded
    initializeCompiler();

    int compileOptions = SH_OBJECT_CODE;
    std::string sourcePath;
    if (perfActive())
    {
        sourcePath = getTempPath();
        writeFile(sourcePath.c_str(), source, strlen(source));
        compileOptions |= SH_LINE_DIRECTIVES;
    }

    int result;
    if (sourcePath.empty())
    {
        result = ShCompile(compiler, &source, 1, compileOptions);
    }
    else
    {
        const char* sourceStrings[2] =
        {
            sourcePath.c_str(),
            source
        };

        result = ShCompile(compiler, sourceStrings, 2, compileOptions | SH_SOURCE_PATH);
    }

    if (result)
    {
        size_t objCodeLen = 0;
        ShGetInfo(compiler, SH_OBJECT_CODE_LENGTH, &objCodeLen);
        mHlsl = new char[objCodeLen];
        ShGetObjectCode(compiler, mHlsl);

        void *activeUniforms;
        ShGetInfoPointer(compiler, SH_ACTIVE_UNIFORMS_ARRAY, &activeUniforms);
        mActiveUniforms = *(sh::ActiveUniforms*)activeUniforms;
    }
    else
    {
        size_t infoLogLen = 0;
        ShGetInfo(compiler, SH_INFO_LOG_LENGTH, &infoLogLen);
        mInfoLog = new char[infoLogLen];
        ShGetInfoLog(compiler, mInfoLog);

        TRACE("\n%s", mInfoLog);
    }
}

GLenum Shader::parseType(const std::string &type)
{
    if (type == "float")
    {
        return GL_FLOAT;
    }
    else if (type == "float2")
    {
        return GL_FLOAT_VEC2;
    }
    else if (type == "float3")
    {
        return GL_FLOAT_VEC3;
    }
    else if (type == "float4")
    {
        return GL_FLOAT_VEC4;
    }
    else if (type == "float2x2")
    {
        return GL_FLOAT_MAT2;
    }
    else if (type == "float3x3")
    {
        return GL_FLOAT_MAT3;
    }
    else if (type == "float4x4")
    {
        return GL_FLOAT_MAT4;
    }
    else UNREACHABLE();

    return GL_NONE;
}

// true if varying x has a higher priority in packing than y
bool Shader::compareVarying(const Varying &x, const Varying &y)
{
    if(x.type == y.type)
    {
        return x.size > y.size;
    }

    switch (x.type)
    {
      case GL_FLOAT_MAT4: return true;
      case GL_FLOAT_MAT2:
        switch(y.type)
        {
          case GL_FLOAT_MAT4: return false;
          case GL_FLOAT_MAT2: return true;
          case GL_FLOAT_VEC4: return true;
          case GL_FLOAT_MAT3: return true;
          case GL_FLOAT_VEC3: return true;
          case GL_FLOAT_VEC2: return true;
          case GL_FLOAT:      return true;
          default: UNREACHABLE();
        }
        break;
      case GL_FLOAT_VEC4:
        switch(y.type)
        {
          case GL_FLOAT_MAT4: return false;
          case GL_FLOAT_MAT2: return false;
          case GL_FLOAT_VEC4: return true;
          case GL_FLOAT_MAT3: return true;
          case GL_FLOAT_VEC3: return true;
          case GL_FLOAT_VEC2: return true;
          case GL_FLOAT:      return true;
          default: UNREACHABLE();
        }
        break;
      case GL_FLOAT_MAT3:
        switch(y.type)
        {
          case GL_FLOAT_MAT4: return false;
          case GL_FLOAT_MAT2: return false;
          case GL_FLOAT_VEC4: return false;
          case GL_FLOAT_MAT3: return true;
          case GL_FLOAT_VEC3: return true;
          case GL_FLOAT_VEC2: return true;
          case GL_FLOAT:      return true;
          default: UNREACHABLE();
        }
        break;
      case GL_FLOAT_VEC3:
        switch(y.type)
        {
          case GL_FLOAT_MAT4: return false;
          case GL_FLOAT_MAT2: return false;
          case GL_FLOAT_VEC4: return false;
          case GL_FLOAT_MAT3: return false;
          case GL_FLOAT_VEC3: return true;
          case GL_FLOAT_VEC2: return true;
          case GL_FLOAT:      return true;
          default: UNREACHABLE();
        }
        break;
      case GL_FLOAT_VEC2:
        switch(y.type)
        {
          case GL_FLOAT_MAT4: return false;
          case GL_FLOAT_MAT2: return false;
          case GL_FLOAT_VEC4: return false;
          case GL_FLOAT_MAT3: return false;
          case GL_FLOAT_VEC3: return false;
          case GL_FLOAT_VEC2: return true;
          case GL_FLOAT:      return true;
          default: UNREACHABLE();
        }
        break;
      case GL_FLOAT: return false;
      default: UNREACHABLE();
    }

    return false;
}

VertexShader::VertexShader(ResourceManager *manager, const rx::Renderer *renderer, GLuint handle)
    : Shader(manager, renderer, handle)
{
}

VertexShader::~VertexShader()
{
}

GLenum VertexShader::getType()
{
    return GL_VERTEX_SHADER;
}

void VertexShader::uncompile()
{
    Shader::uncompile();

    // set by ParseAttributes
    mAttributes.clear();
}

void VertexShader::compile()
{
    uncompile();

    compileToHLSL(mVertexCompiler);
    parseAttributes();
    parseVaryings();
}

int VertexShader::getSemanticIndex(const std::string &attributeName)
{
    if (!attributeName.empty())
    {
        int semanticIndex = 0;
        for (AttributeArray::iterator attribute = mAttributes.begin(); attribute != mAttributes.end(); attribute++)
        {
            if (attribute->name == attributeName)
            {
                return semanticIndex;
            }

            semanticIndex += VariableRowCount(attribute->type);
        }
    }

    return -1;
}

void VertexShader::parseAttributes()
{
    const char *hlsl = getHLSL();
    if (hlsl)
    {
        const char *input = strstr(hlsl, "// Attributes") + 14;

        while(true)
        {
            char attributeType[256];
            char attributeName[256];

            int matches = sscanf(input, "static %255s _%255s", attributeType, attributeName);

            if (matches != 2)
            {
                break;
            }

            mAttributes.push_back(Attribute(parseType(attributeType), attributeName));

            input = strstr(input, ";") + 2;
        }
    }
}

FragmentShader::FragmentShader(ResourceManager *manager, const rx::Renderer *renderer, GLuint handle)
    : Shader(manager, renderer, handle)
{
}

FragmentShader::~FragmentShader()
{
}

GLenum FragmentShader::getType()
{
    return GL_FRAGMENT_SHADER;
}

void FragmentShader::compile()
{
    uncompile();

    compileToHLSL(mFragmentCompiler);
    parseVaryings();
    mVaryings.sort(compareVarying);
}
}
