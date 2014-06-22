//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

//
// Implement the top-level of interface to the compiler,
// as defined in ShaderLang.h
//

#include "GLSLANG/ShaderLang.h"

#include "compiler/translator/InitializeDll.h"
#include "compiler/preprocessor/length_limits.h"
#include "compiler/translator/ShHandle.h"
#include "compiler/translator/TranslatorHLSL.h"
#include "compiler/translator/VariablePacker.h"

//
// This is the platform independent interface between an OGL driver
// and the shading language compiler.
//

static bool checkVariableMaxLengths(const ShHandle handle,
                                    size_t expectedValue)
{
    size_t activeUniformLimit = 0;
    ShGetInfo(handle, SH_ACTIVE_UNIFORM_MAX_LENGTH, &activeUniformLimit);
    size_t activeAttribLimit = 0;
    ShGetInfo(handle, SH_ACTIVE_ATTRIBUTE_MAX_LENGTH, &activeAttribLimit);
    size_t varyingLimit = 0;
    ShGetInfo(handle, SH_VARYING_MAX_LENGTH, &varyingLimit);
    return (expectedValue == activeUniformLimit &&
            expectedValue == activeAttribLimit &&
            expectedValue == varyingLimit);
}

static bool checkMappedNameMaxLength(const ShHandle handle, size_t expectedValue)
{
    size_t mappedNameMaxLength = 0;
    ShGetInfo(handle, SH_MAPPED_NAME_MAX_LENGTH, &mappedNameMaxLength);
    return (expectedValue == mappedNameMaxLength);
}

//
// Driver must call this first, once, before doing any other compiler operations.
// Subsequent calls to this function are no-op.
//
int ShInitialize()
{
    static const bool kInitialized = InitProcess();
    return kInitialized ? 1 : 0;
}

//
// Cleanup symbol tables
//
int ShFinalize()
{
    DetachProcess();
    return 1;
}

//
// Initialize built-in resources with minimum expected values.
//
void ShInitBuiltInResources(ShBuiltInResources* resources)
{
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

    // Disable highp precision in fragment shader by default.
    resources->FragmentPrecisionHigh = 0;

    // Disable name hashing by default.
    resources->HashFunction = NULL;

    resources->ArrayIndexClampingStrategy = SH_CLAMP_WITH_CLAMP_INTRINSIC;

    resources->MaxExpressionComplexity = 256;
    resources->MaxCallStackDepth = 256;
}

//
// Driver calls these to create and destroy compiler objects.
//
ShHandle ShConstructCompiler(ShShaderType type, ShShaderSpec spec,
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

//
// Do an actual compile on the given strings.  The result is left 
// in the given compile object.
//
// Return:  The return value of ShCompile is really boolean, indicating
// success or failure.
//
int ShCompile(
    const ShHandle handle,
    const char* const shaderStrings[],
    size_t numStrings,
    int compileOptions)
{
    if (handle == 0)
        return 0;

    TShHandleBase* base = reinterpret_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (compiler == 0)
        return 0;

    bool success = compiler->compile(shaderStrings, numStrings, compileOptions);
    return success ? 1 : 0;
}

void ShGetInfo(const ShHandle handle, ShShaderInfo pname, size_t* params)
{
    if (!handle || !params)
        return;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (!compiler) return;

    switch(pname)
    {
    case SH_INFO_LOG_LENGTH:
        *params = compiler->getInfoSink().info.size() + 1;
        break;
    case SH_OBJECT_CODE_LENGTH:
        *params = compiler->getInfoSink().obj.size() + 1;
        break;
    case SH_ACTIVE_UNIFORMS:
        *params = compiler->getUniforms().size();
        break;
    case SH_ACTIVE_UNIFORM_MAX_LENGTH:
        *params = 1 +  MAX_SYMBOL_NAME_LEN;
        break;
    case SH_ACTIVE_ATTRIBUTES:
        *params = compiler->getAttribs().size();
        break;
    case SH_ACTIVE_ATTRIBUTE_MAX_LENGTH:
        *params = 1 + MAX_SYMBOL_NAME_LEN;
        break;
    case SH_VARYINGS:
        *params = compiler->getVaryings().size();
        break;
    case SH_VARYING_MAX_LENGTH:
        *params = 1 + MAX_SYMBOL_NAME_LEN;
        break;
    case SH_MAPPED_NAME_MAX_LENGTH:
        // Use longer length than MAX_SHORTENED_IDENTIFIER_SIZE to
        // handle array and struct dereferences.
        *params = 1 + MAX_SYMBOL_NAME_LEN;
        break;
    case SH_NAME_MAX_LENGTH:
        *params = 1 + MAX_SYMBOL_NAME_LEN;
        break;
    case SH_HASHED_NAME_MAX_LENGTH:
        if (compiler->getHashFunction() == NULL) {
            *params = 0;
        } else {
            // 64 bits hashing output requires 16 bytes for hex 
            // representation.
            const char HashedNamePrefix[] = HASHED_NAME_PREFIX;
            *params = 16 + sizeof(HashedNamePrefix);
        }
        break;
    case SH_HASHED_NAMES_COUNT:
        *params = compiler->getNameMap().size();
        break;
    default: UNREACHABLE();
    }
}

//
// Return any compiler log of messages for the application.
//
void ShGetInfoLog(const ShHandle handle, char* infoLog)
{
    if (!handle || !infoLog)
        return;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (!compiler) return;

    TInfoSink& infoSink = compiler->getInfoSink();
    strcpy(infoLog, infoSink.info.c_str());
}

//
// Return any object code.
//
void ShGetObjectCode(const ShHandle handle, char* objCode)
{
    if (!handle || !objCode)
        return;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (!compiler) return;

    TInfoSink& infoSink = compiler->getInfoSink();
    strcpy(objCode, infoSink.obj.c_str());
}

void ShGetVariableInfo(const ShHandle handle,
                       ShShaderInfo varType,
                       int index,
                       size_t* length,
                       int* size,
                       ShDataType* type,
                       ShPrecisionType* precision,
                       int* staticUse,
                       char* name,
                       char* mappedName)
{
    if (!handle || !size || !type || !precision || !staticUse || !name)
        return;
    ASSERT((varType == SH_ACTIVE_ATTRIBUTES) ||
           (varType == SH_ACTIVE_UNIFORMS) ||
           (varType == SH_VARYINGS));

    TShHandleBase* base = reinterpret_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (compiler == 0)
        return;

    const TVariableInfoList& varList =
        varType == SH_ACTIVE_ATTRIBUTES ? compiler->getAttribs() :
            (varType == SH_ACTIVE_UNIFORMS ? compiler->getUniforms() :
                compiler->getVaryings());
    if (index < 0 || index >= static_cast<int>(varList.size()))
        return;

    const TVariableInfo& varInfo = varList[index];
    if (length) *length = varInfo.name.size();
    *size = varInfo.size;
    *type = varInfo.type;
    switch (varInfo.precision) {
    case EbpLow:
        *precision = SH_PRECISION_LOWP;
        break;
    case EbpMedium:
        *precision = SH_PRECISION_MEDIUMP;
        break;
    case EbpHigh:
        *precision = SH_PRECISION_HIGHP;
        break;
    default:
        // Some types does not support precision, for example, boolean.
        *precision = SH_PRECISION_UNDEFINED;
        break;
    }
    *staticUse = varInfo.staticUse ? 1 : 0;

    // This size must match that queried by
    // SH_ACTIVE_UNIFORM_MAX_LENGTH, SH_ACTIVE_ATTRIBUTE_MAX_LENGTH, SH_VARYING_MAX_LENGTH
    // in ShGetInfo, below.
    size_t variableLength = 1 + MAX_SYMBOL_NAME_LEN;
    ASSERT(checkVariableMaxLengths(handle, variableLength));
    strncpy(name, varInfo.name.c_str(), variableLength);
    name[variableLength - 1] = 0;
    if (mappedName) {
        // This size must match that queried by
        // SH_MAPPED_NAME_MAX_LENGTH in ShGetInfo, below.
        size_t maxMappedNameLength = 1 + MAX_SYMBOL_NAME_LEN;
        ASSERT(checkMappedNameMaxLength(handle, maxMappedNameLength));
        strncpy(mappedName, varInfo.mappedName.c_str(), maxMappedNameLength);
        mappedName[maxMappedNameLength - 1] = 0;
    }
}

void ShGetNameHashingEntry(const ShHandle handle,
                           int index,
                           char* name,
                           char* hashedName)
{
    if (!handle || !name || !hashedName || index < 0)
        return;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (!compiler) return;

    const NameMap& nameMap = compiler->getNameMap();
    if (index >= static_cast<int>(nameMap.size()))
        return;

    NameMap::const_iterator it = nameMap.begin();
    for (int i = 0; i < index; ++i)
        ++it;

    size_t len = it->first.length() + 1;
    size_t max_len = 0;
    ShGetInfo(handle, SH_NAME_MAX_LENGTH, &max_len);
    if (len > max_len) {
        ASSERT(false);
        len = max_len;
    }
    strncpy(name, it->first.c_str(), len);
    // To be on the safe side in case the source is longer than expected.
    name[len - 1] = '\0';

    len = it->second.length() + 1;
    max_len = 0;
    ShGetInfo(handle, SH_HASHED_NAME_MAX_LENGTH, &max_len);
    if (len > max_len) {
        ASSERT(false);
        len = max_len;
    }
    strncpy(hashedName, it->second.c_str(), len);
    // To be on the safe side in case the source is longer than expected.
    hashedName[len - 1] = '\0';
}

void ShGetInfoPointer(const ShHandle handle, ShShaderInfo pname, void** params)
{
    if (!handle || !params)
        return;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);
    TranslatorHLSL* translator = base->getAsTranslatorHLSL();
    if (!translator) return;

    switch(pname)
    {
    case SH_ACTIVE_UNIFORMS_ARRAY:
        *params = (void*)&translator->getUniforms();
        break;
    default: UNREACHABLE();
    }
}

int ShCheckVariablesWithinPackingLimits(
    int maxVectors, ShVariableInfo* varInfoArray, size_t varInfoArraySize)
{
    if (varInfoArraySize == 0)
        return 1;
    ASSERT(varInfoArray);
    TVariableInfoList variables;
    for (size_t ii = 0; ii < varInfoArraySize; ++ii)
    {
        TVariableInfo var(varInfoArray[ii].type, varInfoArray[ii].size);
        variables.push_back(var);
    }
    VariablePacker packer;
    return packer.CheckVariablesWithinPackingLimits(maxVectors, variables) ? 1 : 0;
}
