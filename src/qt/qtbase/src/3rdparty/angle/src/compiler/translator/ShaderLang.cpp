//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

//
// Implement the top-level of interface to the compiler,
// as defined in ShaderLang.h
//

#include "GLSLANG/ShaderLang.h"

#include "compiler/translator/Compiler.h"
#include "compiler/translator/InitializeDll.h"
#include "compiler/translator/length_limits.h"
#include "compiler/translator/TranslatorHLSL.h"
#include "compiler/translator/VariablePacker.h"
#include "angle_gl.h"

namespace
{

enum ShaderVariableType
{
    SHADERVAR_UNIFORM,
    SHADERVAR_VARYING,
    SHADERVAR_ATTRIBUTE,
    SHADERVAR_OUTPUTVARIABLE,
    SHADERVAR_INTERFACEBLOCK
};
    
bool isInitialized = false;

//
// This is the platform independent interface between an OGL driver
// and the shading language compiler.
//

template <typename VarT>
const std::vector<VarT> *GetVariableList(const TCompiler *compiler, ShaderVariableType variableType);

template <>
const std::vector<sh::Uniform> *GetVariableList(const TCompiler *compiler, ShaderVariableType)
{
    return &compiler->getUniforms();
}

template <>
const std::vector<sh::Varying> *GetVariableList(const TCompiler *compiler, ShaderVariableType)
{
    return &compiler->getVaryings();
}

template <>
const std::vector<sh::Attribute> *GetVariableList(const TCompiler *compiler, ShaderVariableType variableType)
{
    return (variableType == SHADERVAR_ATTRIBUTE ?
        &compiler->getAttributes() :
        &compiler->getOutputVariables());
}

template <>
const std::vector<sh::InterfaceBlock> *GetVariableList(const TCompiler *compiler, ShaderVariableType)
{
    return &compiler->getInterfaceBlocks();
}

template <typename VarT>
const std::vector<VarT> *GetShaderVariables(const ShHandle handle, ShaderVariableType variableType)
{
    if (!handle)
    {
        return NULL;
    }

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (!compiler)
    {
        return NULL;
    }

    return GetVariableList<VarT>(compiler, variableType);
}

TCompiler *GetCompilerFromHandle(ShHandle handle)
{
    if (!handle)
        return NULL;
    TShHandleBase *base = static_cast<TShHandleBase *>(handle);
    return base->getAsCompiler();
}

TranslatorHLSL *GetTranslatorHLSLFromHandle(ShHandle handle)
{
    if (!handle)
        return NULL;
    TShHandleBase *base = static_cast<TShHandleBase *>(handle);
    return base->getAsTranslatorHLSL();
}

}  // namespace anonymous

//
// Driver must call this first, once, before doing any other compiler operations.
// Subsequent calls to this function are no-op.
//
bool ShInitialize()
{
    if (!isInitialized)
    {
        isInitialized = InitProcess();
    }
    return isInitialized;
}

//
// Cleanup symbol tables
//
bool ShFinalize()
{
    if (isInitialized)
    {
        DetachProcess();
        isInitialized = false;
    }
    return true;
}

//
// Initialize built-in resources with minimum expected values.
//
void ShInitBuiltInResources(ShBuiltInResources* resources)
{
    // Make comparable.
    memset(resources, 0, sizeof(*resources));

    // Constants.
    resources->MaxVertexAttribs = 8;
    resources->MaxVertexUniformVectors = 128;
    resources->MaxVaryingVectors = 8;
    resources->MaxVertexTextureImageUnits = 0;
    resources->MaxCombinedTextureImageUnits = 8;
    resources->MaxTextureImageUnits = 8;
    resources->MaxFragmentUniformVectors = 16;
    resources->MaxDrawBuffers = 1;

    // Extensions.
    resources->OES_standard_derivatives = 0;
    resources->OES_EGL_image_external = 0;
    resources->ARB_texture_rectangle = 0;
    resources->EXT_draw_buffers = 0;
    resources->EXT_frag_depth = 0;
    resources->EXT_shader_texture_lod = 0;

    resources->NV_draw_buffers = 0;

    // Disable highp precision in fragment shader by default.
    resources->FragmentPrecisionHigh = 0;

    // GLSL ES 3.0 constants.
    resources->MaxVertexOutputVectors = 16;
    resources->MaxFragmentInputVectors = 15;
    resources->MinProgramTexelOffset = -8;
    resources->MaxProgramTexelOffset = 7;

    // Disable name hashing by default.
    resources->HashFunction = NULL;

    resources->ArrayIndexClampingStrategy = SH_CLAMP_WITH_CLAMP_INTRINSIC;

    resources->MaxExpressionComplexity = 256;
    resources->MaxCallStackDepth = 256;
}

//
// Driver calls these to create and destroy compiler objects.
//
ShHandle ShConstructCompiler(sh::GLenum type, ShShaderSpec spec,
                             ShShaderOutput output,
                             const ShBuiltInResources* resources)
{
    TShHandleBase* base = static_cast<TShHandleBase*>(ConstructCompiler(type, spec, output));
    TCompiler* compiler = base->getAsCompiler();
    if (compiler == 0)
        return 0;

    // Generate built-in symbol table.
    if (!compiler->Init(*resources)) {
        ShDestruct(base);
        return 0;
    }

    return reinterpret_cast<void*>(base);
}

void ShDestruct(ShHandle handle)
{
    if (handle == 0)
        return;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);

    if (base->getAsCompiler())
        DeleteCompiler(base->getAsCompiler());
}

const std::string &ShGetBuiltInResourcesString(const ShHandle handle)
{
    TCompiler *compiler = GetCompilerFromHandle(handle);
    ASSERT(compiler);
    return compiler->getBuiltInResourcesString();
}

//
// Do an actual compile on the given strings.  The result is left
// in the given compile object.
//
// Return:  The return value of ShCompile is really boolean, indicating
// success or failure.
//
bool ShCompile(
    const ShHandle handle,
    const char *const shaderStrings[],
    size_t numStrings,
    int compileOptions)
{
    TCompiler *compiler = GetCompilerFromHandle(handle);
    ASSERT(compiler);

    return compiler->compile(shaderStrings, numStrings, compileOptions);
}

int ShGetShaderVersion(const ShHandle handle)
{
    TCompiler* compiler = GetCompilerFromHandle(handle);
    ASSERT(compiler);
    return compiler->getShaderVersion();
}

ShShaderOutput ShGetShaderOutputType(const ShHandle handle)
{
    TCompiler* compiler = GetCompilerFromHandle(handle);
    ASSERT(compiler);
    return compiler->getOutputType();
}

//
// Return any compiler log of messages for the application.
//
const std::string &ShGetInfoLog(const ShHandle handle)
{
    TCompiler *compiler = GetCompilerFromHandle(handle);
    ASSERT(compiler);

    TInfoSink &infoSink = compiler->getInfoSink();
    return infoSink.info.str();
}

//
// Return any object code.
//
const std::string &ShGetObjectCode(const ShHandle handle)
{
    TCompiler *compiler = GetCompilerFromHandle(handle);
    ASSERT(compiler);

    TInfoSink &infoSink = compiler->getInfoSink();
    return infoSink.obj.str();
}

const std::map<std::string, std::string> *ShGetNameHashingMap(
    const ShHandle handle)
{
    TCompiler *compiler = GetCompilerFromHandle(handle);
    ASSERT(compiler);
    return &(compiler->getNameMap());
}

const std::vector<sh::Uniform> *ShGetUniforms(const ShHandle handle)
{
    return GetShaderVariables<sh::Uniform>(handle, SHADERVAR_UNIFORM);
}

const std::vector<sh::Varying> *ShGetVaryings(const ShHandle handle)
{
    return GetShaderVariables<sh::Varying>(handle, SHADERVAR_VARYING);
}

const std::vector<sh::Attribute> *ShGetAttributes(const ShHandle handle)
{
    return GetShaderVariables<sh::Attribute>(handle, SHADERVAR_ATTRIBUTE);
}

const std::vector<sh::Attribute> *ShGetOutputVariables(const ShHandle handle)
{
    return GetShaderVariables<sh::Attribute>(handle, SHADERVAR_OUTPUTVARIABLE);
}

const std::vector<sh::InterfaceBlock> *ShGetInterfaceBlocks(const ShHandle handle)
{
    return GetShaderVariables<sh::InterfaceBlock>(handle, SHADERVAR_INTERFACEBLOCK);
}

bool ShCheckVariablesWithinPackingLimits(
    int maxVectors, ShVariableInfo *varInfoArray, size_t varInfoArraySize)
{
    if (varInfoArraySize == 0)
        return true;
    ASSERT(varInfoArray);
    std::vector<sh::ShaderVariable> variables;
    for (size_t ii = 0; ii < varInfoArraySize; ++ii)
    {
        sh::ShaderVariable var(varInfoArray[ii].type, varInfoArray[ii].size);
        variables.push_back(var);
    }
    VariablePacker packer;
    return packer.CheckVariablesWithinPackingLimits(maxVectors, variables);
}

bool ShGetInterfaceBlockRegister(const ShHandle handle,
                                 const std::string &interfaceBlockName,
                                 unsigned int *indexOut)
{
    ASSERT(indexOut);

    TranslatorHLSL *translator = GetTranslatorHLSLFromHandle(handle);
    ASSERT(translator);

    if (!translator->hasInterfaceBlock(interfaceBlockName))
    {
        return false;
    }

    *indexOut = translator->getInterfaceBlockRegister(interfaceBlockName);
    return true;
}

bool ShGetUniformRegister(const ShHandle handle,
                          const std::string &uniformName,
                          unsigned int *indexOut)
{
    ASSERT(indexOut);
    TranslatorHLSL *translator = GetTranslatorHLSLFromHandle(handle);
    ASSERT(translator);

    if (!translator->hasUniform(uniformName))
    {
        return false;
    }

    *indexOut = translator->getUniformRegister(uniformName);
    return true;
}
