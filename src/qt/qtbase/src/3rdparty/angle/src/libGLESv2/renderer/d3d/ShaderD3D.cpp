//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderD3D.cpp: Defines the rx::ShaderD3D class which implements rx::ShaderImpl.

#include "libGLESv2/Shader.h"
#include "libGLESv2/main.h"
#include "libGLESv2/renderer/d3d/RendererD3D.h"
#include "libGLESv2/renderer/d3d/ShaderD3D.h"

#include "common/features.h"
#include "common/utilities.h"

// Definitions local to the translation unit
namespace
{

const char *GetShaderTypeString(GLenum type)
{
    switch (type)
    {
      case GL_VERTEX_SHADER:
        return "VERTEX";

      case GL_FRAGMENT_SHADER:
        return "FRAGMENT";

      default:
        UNREACHABLE();
        return "";
    }
}

}

namespace rx
{

template <typename VarT>
void FilterInactiveVariables(std::vector<VarT> *variableList)
{
    ASSERT(variableList);

    for (size_t varIndex = 0; varIndex < variableList->size();)
    {
        if (!(*variableList)[varIndex].staticUse)
        {
            variableList->erase(variableList->begin() + varIndex);
        }
        else
        {
            varIndex++;
        }
    }
}

void *ShaderD3D::mFragmentCompiler = NULL;
void *ShaderD3D::mVertexCompiler = NULL;

template <typename VarT>
const std::vector<VarT> *GetShaderVariables(const std::vector<VarT> *variableList)
{
    ASSERT(variableList);
    return variableList;
}

ShaderD3D::ShaderD3D(const gl::Data &data, GLenum type, RendererD3D *renderer)
    : mType(type),
      mRenderer(renderer),
      mShaderVersion(100)
{
    uncompile();
    initializeCompiler(data);
}

ShaderD3D::~ShaderD3D()
{
}

ShaderD3D *ShaderD3D::makeShaderD3D(ShaderImpl *impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(ShaderD3D*, impl));
    return static_cast<ShaderD3D*>(impl);
}

const ShaderD3D *ShaderD3D::makeShaderD3D(const ShaderImpl *impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(const ShaderD3D*, impl));
    return static_cast<const ShaderD3D*>(impl);
}

std::string ShaderD3D::getDebugInfo() const
{
    return mDebugInfo + std::string("\n// ") + GetShaderTypeString(mType) + " SHADER END\n";
}

// Perform a one-time initialization of the shader compiler (or after being destructed by releaseCompiler)
void ShaderD3D::initializeCompiler(const gl::Data &data)
{
    if (!mFragmentCompiler)
    {
        bool result = ShInitialize();

        if (result)
        {
            ShShaderSpec specVersion = (data.clientVersion >= 3) ? SH_GLES3_SPEC : SH_GLES2_SPEC;
            ShShaderOutput hlslVersion = (mRenderer->getMajorShaderModel() >= 4) ? SH_HLSL11_OUTPUT : SH_HLSL9_OUTPUT;

            ShBuiltInResources resources;
            ShInitBuiltInResources(&resources);

            const gl::Caps &caps = *data.caps;
            const gl::Extensions &extensions = *data.extensions;

            resources.MaxVertexAttribs = caps.maxVertexAttributes;
            resources.MaxVertexUniformVectors = caps.maxVertexUniformVectors;
            resources.MaxVaryingVectors = caps.maxVaryingVectors;
            resources.MaxVertexTextureImageUnits = caps.maxVertexTextureImageUnits;
            resources.MaxCombinedTextureImageUnits = caps.maxCombinedTextureImageUnits;
            resources.MaxTextureImageUnits = caps.maxTextureImageUnits;
            resources.MaxFragmentUniformVectors = caps.maxFragmentUniformVectors;
            resources.MaxDrawBuffers = caps.maxDrawBuffers;
            resources.OES_standard_derivatives = extensions.standardDerivatives;
            resources.EXT_draw_buffers = extensions.drawBuffers;
            resources.EXT_shader_texture_lod = 1;
            // resources.OES_EGL_image_external = mRenderer->getShareHandleSupport() ? 1 : 0; // TODO: commented out until the extension is actually supported.
            resources.FragmentPrecisionHigh = 1;   // Shader Model 2+ always supports FP24 (s16e7) which corresponds to highp
            resources.EXT_frag_depth = 1; // Shader Model 2+ always supports explicit depth output
            // GLSL ES 3.0 constants
            resources.MaxVertexOutputVectors = caps.maxVertexOutputComponents / 4;
            resources.MaxFragmentInputVectors = caps.maxFragmentInputComponents / 4;
            resources.MinProgramTexelOffset = caps.minProgramTexelOffset;
            resources.MaxProgramTexelOffset = caps.maxProgramTexelOffset;

            mFragmentCompiler = ShConstructCompiler(GL_FRAGMENT_SHADER, specVersion, hlslVersion, &resources);
            mVertexCompiler = ShConstructCompiler(GL_VERTEX_SHADER, specVersion, hlslVersion, &resources);
        }
    }
}

void ShaderD3D::releaseCompiler()
{
    ShDestruct(mFragmentCompiler);
    ShDestruct(mVertexCompiler);

    mFragmentCompiler = NULL;
    mVertexCompiler = NULL;

    ShFinalize();
}

void ShaderD3D::parseVaryings(void *compiler)
{
    if (!mHlsl.empty())
    {
        const std::vector<sh::Varying> *varyings = ShGetVaryings(compiler);
        ASSERT(varyings);

        for (size_t varyingIndex = 0; varyingIndex < varyings->size(); varyingIndex++)
        {
            mVaryings.push_back(gl::PackedVarying((*varyings)[varyingIndex]));
        }

        mUsesMultipleRenderTargets = mHlsl.find("GL_USES_MRT")          != std::string::npos;
        mUsesFragColor             = mHlsl.find("GL_USES_FRAG_COLOR")   != std::string::npos;
        mUsesFragData              = mHlsl.find("GL_USES_FRAG_DATA")    != std::string::npos;
        mUsesFragCoord             = mHlsl.find("GL_USES_FRAG_COORD")   != std::string::npos;
        mUsesFrontFacing           = mHlsl.find("GL_USES_FRONT_FACING") != std::string::npos;
        mUsesPointSize             = mHlsl.find("GL_USES_POINT_SIZE")   != std::string::npos;
        mUsesPointCoord            = mHlsl.find("GL_USES_POINT_COORD")  != std::string::npos;
        mUsesDepthRange            = mHlsl.find("GL_USES_DEPTH_RANGE")  != std::string::npos;
        mUsesFragDepth             = mHlsl.find("GL_USES_FRAG_DEPTH")   != std::string::npos;
        mUsesDiscardRewriting      = mHlsl.find("ANGLE_USES_DISCARD_REWRITING") != std::string::npos;
        mUsesNestedBreak           = mHlsl.find("ANGLE_USES_NESTED_BREAK") != std::string::npos;
    }
}

void ShaderD3D::resetVaryingsRegisterAssignment()
{
    for (size_t varyingIndex = 0; varyingIndex < mVaryings.size(); varyingIndex++)
    {
        mVaryings[varyingIndex].resetRegisterAssignment();
    }
}

// initialize/clean up previous state
void ShaderD3D::uncompile()
{
    // set by compileToHLSL
    mHlsl.clear();
    mInfoLog.clear();

    mUsesMultipleRenderTargets = false;
    mUsesFragColor = false;
    mUsesFragData = false;
    mUsesFragCoord = false;
    mUsesFrontFacing = false;
    mUsesPointSize = false;
    mUsesPointCoord = false;
    mUsesDepthRange = false;
    mUsesFragDepth = false;
    mShaderVersion = 100;
    mUsesDiscardRewriting = false;
    mUsesNestedBreak = false;

    mVaryings.clear();
    mUniforms.clear();
    mInterfaceBlocks.clear();
    mActiveAttributes.clear();
    mActiveOutputVariables.clear();
    mDebugInfo.clear();
}

void ShaderD3D::compileToHLSL(const gl::Data &data, void *compiler, const std::string &source)
{
    // ensure the compiler is loaded
    initializeCompiler(data);

    int compileOptions = (SH_OBJECT_CODE | SH_VARIABLES);
    std::string sourcePath;

#if !defined (ANGLE_ENABLE_WINDOWS_STORE)
    if (gl::perfActive())
    {
        sourcePath = getTempPath();
        writeFile(sourcePath.c_str(), source.c_str(), source.length());
        compileOptions |= SH_LINE_DIRECTIVES;
    }
#endif

    int result;
    if (sourcePath.empty())
    {
        const char* sourceStrings[] =
        {
            source.c_str(),
        };

        result = ShCompile(compiler, sourceStrings, ArraySize(sourceStrings), compileOptions);
    }
    else
    {
        const char* sourceStrings[] =
        {
            sourcePath.c_str(),
            source.c_str(),
        };

        result = ShCompile(compiler, sourceStrings, ArraySize(sourceStrings), compileOptions | SH_SOURCE_PATH);
    }

    mShaderVersion = ShGetShaderVersion(compiler);

    if (mShaderVersion == 300 && data.clientVersion < 3)
    {
        mInfoLog = "GLSL ES 3.00 is not supported by OpenGL ES 2.0 contexts";
        TRACE("\n%s", mInfoLog.c_str());
    }
    else if (result)
    {
        mHlsl = ShGetObjectCode(compiler);

#ifdef _DEBUG
        // Prefix hlsl shader with commented out glsl shader
        // Useful in diagnostics tools like pix which capture the hlsl shaders
        std::ostringstream hlslStream;
        hlslStream << "// GLSL\n";
        hlslStream << "//\n";

        size_t curPos = 0;
        while (curPos != std::string::npos)
        {
            size_t nextLine = source.find("\n", curPos);
            size_t len = (nextLine == std::string::npos) ? std::string::npos : (nextLine - curPos + 1);

            hlslStream << "// " << source.substr(curPos, len);

            curPos = (nextLine == std::string::npos) ? std::string::npos : (nextLine + 1);
        }
        hlslStream << "\n\n";
        hlslStream << mHlsl;
        mHlsl = hlslStream.str();
#endif

        mUniforms = *GetShaderVariables(ShGetUniforms(compiler));

        for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
        {
            const sh::Uniform &uniform = mUniforms[uniformIndex];

            if (uniform.staticUse)
            {
                unsigned int index = -1;
                bool result = ShGetUniformRegister(compiler, uniform.name, &index);
                UNUSED_ASSERTION_VARIABLE(result);
                ASSERT(result);

                mUniformRegisterMap[uniform.name] = index;
            }
        }

        mInterfaceBlocks = *GetShaderVariables(ShGetInterfaceBlocks(compiler));

        for (size_t blockIndex = 0; blockIndex < mInterfaceBlocks.size(); blockIndex++)
        {
            const sh::InterfaceBlock &interfaceBlock = mInterfaceBlocks[blockIndex];

            if (interfaceBlock.staticUse)
            {
                unsigned int index = -1;
                bool result = ShGetInterfaceBlockRegister(compiler, interfaceBlock.name, &index);
                UNUSED_ASSERTION_VARIABLE(result);
                ASSERT(result);

                mInterfaceBlockRegisterMap[interfaceBlock.name] = index;
            }
        }
    }
    else
    {
        mInfoLog = ShGetInfoLog(compiler);

        TRACE("\n%s", mInfoLog.c_str());
    }
}

D3DWorkaroundType ShaderD3D::getD3DWorkarounds() const
{
    if (mUsesDiscardRewriting)
    {
        // ANGLE issue 486:
        // Work-around a D3D9 compiler bug that presents itself when using conditional discard, by disabling optimization
        return ANGLE_D3D_WORKAROUND_SKIP_OPTIMIZATION;
    }

    if (mUsesNestedBreak)
    {
        // ANGLE issue 603:
        // Work-around a D3D9 compiler bug that presents itself when using break in a nested loop, by maximizing optimization
        // We want to keep the use of ANGLE_D3D_WORKAROUND_MAX_OPTIMIZATION minimal to prevent hangs, so usesDiscard takes precedence
        return ANGLE_D3D_WORKAROUND_MAX_OPTIMIZATION;
    }

    return ANGLE_D3D_WORKAROUND_NONE;
}

// true if varying x has a higher priority in packing than y
bool ShaderD3D::compareVarying(const gl::PackedVarying &x, const gl::PackedVarying &y)
{
    if (x.type == y.type)
    {
        return x.arraySize > y.arraySize;
    }

    // Special case for handling structs: we sort these to the end of the list
    if (x.type == GL_STRUCT_ANGLEX)
    {
        return false;
    }

    if (y.type == GL_STRUCT_ANGLEX)
    {
        return true;
    }

    return gl::VariableSortOrder(x.type) < gl::VariableSortOrder(y.type);
}

unsigned int ShaderD3D::getUniformRegister(const std::string &uniformName) const
{
    ASSERT(mUniformRegisterMap.count(uniformName) > 0);
    return mUniformRegisterMap.find(uniformName)->second;
}

unsigned int ShaderD3D::getInterfaceBlockRegister(const std::string &blockName) const
{
    ASSERT(mInterfaceBlockRegisterMap.count(blockName) > 0);
    return mInterfaceBlockRegisterMap.find(blockName)->second;
}

void *ShaderD3D::getCompiler()
{
    if (mType == GL_VERTEX_SHADER)
    {
        return mVertexCompiler;
    }
    else
    {
        ASSERT(mType == GL_FRAGMENT_SHADER);
        return mFragmentCompiler;
    }
}

ShShaderOutput ShaderD3D::getCompilerOutputType(GLenum shader)
{
    void *compiler = NULL;

    switch (shader)
    {
      case GL_VERTEX_SHADER:   compiler = mVertexCompiler;   break;
      case GL_FRAGMENT_SHADER: compiler = mFragmentCompiler; break;
      default: UNREACHABLE();  return SH_HLSL9_OUTPUT;
    }

    return ShGetShaderOutputType(compiler);
}

bool ShaderD3D::compile(const gl::Data &data, const std::string &source)
{
    uncompile();

    void *compiler = getCompiler();

    compileToHLSL(data, compiler, source);

    if (mType == GL_VERTEX_SHADER)
    {
        parseAttributes(compiler);
    }

    parseVaryings(compiler);

    if (mType == GL_FRAGMENT_SHADER)
    {
        std::sort(mVaryings.begin(), mVaryings.end(), compareVarying);

        const std::string &hlsl = getTranslatedSource();
        if (!hlsl.empty())
        {
            mActiveOutputVariables = *GetShaderVariables(ShGetOutputVariables(compiler));
            FilterInactiveVariables(&mActiveOutputVariables);
        }
    }

#if ANGLE_SHADER_DEBUG_INFO == ANGLE_ENABLED
    mDebugInfo += std::string("// ") + GetShaderTypeString(mType) + " SHADER BEGIN\n";
    mDebugInfo += "\n// GLSL BEGIN\n\n" + source + "\n\n// GLSL END\n\n\n";
    mDebugInfo += "// INITIAL HLSL BEGIN\n\n" + getTranslatedSource() + "\n// INITIAL HLSL END\n\n\n";
    // Successive steps will append more info
#else
    mDebugInfo += getTranslatedSource();
#endif

    return !getTranslatedSource().empty();
}

void ShaderD3D::parseAttributes(void *compiler)
{
    const std::string &hlsl = getTranslatedSource();
    if (!hlsl.empty())
    {
        mActiveAttributes = *GetShaderVariables(ShGetAttributes(compiler));
        FilterInactiveVariables(&mActiveAttributes);
    }
}

int ShaderD3D::getSemanticIndex(const std::string &attributeName) const
{
    if (!attributeName.empty())
    {
        int semanticIndex = 0;
        for (size_t attributeIndex = 0; attributeIndex < mActiveAttributes.size(); attributeIndex++)
        {
            const sh::ShaderVariable &attribute = mActiveAttributes[attributeIndex];

            if (attribute.name == attributeName)
            {
                return semanticIndex;
            }

            semanticIndex += gl::VariableRegisterCount(attribute.type);
        }
    }

    return -1;
}

}
