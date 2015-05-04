//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libGLESv2/renderer/d3d/HLSLCompiler.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/main.h"

#include "common/features.h"
#include "common/utilities.h"

#ifndef QT_D3DCOMPILER_DLL
#define QT_D3DCOMPILER_DLL D3DCOMPILER_DLL
#endif
#ifndef D3DCOMPILE_RESERVED16
#define D3DCOMPILE_RESERVED16 (1 << 16)
#endif
#ifndef D3DCOMPILE_RESERVED17
#define D3DCOMPILE_RESERVED17 (1 << 17)
#endif

// Definitions local to the translation unit
namespace
{

#ifdef CREATE_COMPILER_FLAG_INFO
    #undef CREATE_COMPILER_FLAG_INFO
#endif

#define CREATE_COMPILER_FLAG_INFO(flag) { flag, #flag }

struct CompilerFlagInfo
{
    UINT mFlag;
    const char *mName;
};

CompilerFlagInfo CompilerFlagInfos[] =
{
    // NOTE: The data below is copied from d3dcompiler.h
    // If something changes there it should be changed here as well
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_DEBUG),                          // (1 << 0)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_SKIP_VALIDATION),                // (1 << 1)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_SKIP_OPTIMIZATION),              // (1 << 2)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_PACK_MATRIX_ROW_MAJOR),          // (1 << 3)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR),       // (1 << 4)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_PARTIAL_PRECISION),              // (1 << 5)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_FORCE_VS_SOFTWARE_NO_OPT),       // (1 << 6)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_FORCE_PS_SOFTWARE_NO_OPT),       // (1 << 7)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_NO_PRESHADER),                   // (1 << 8)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_AVOID_FLOW_CONTROL),             // (1 << 9)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_PREFER_FLOW_CONTROL),            // (1 << 10)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_ENABLE_STRICTNESS),              // (1 << 11)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY), // (1 << 12)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_IEEE_STRICTNESS),                // (1 << 13)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_OPTIMIZATION_LEVEL0),            // (1 << 14)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_OPTIMIZATION_LEVEL1),            // 0
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_OPTIMIZATION_LEVEL2),            // ((1 << 14) | (1 << 15))
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_OPTIMIZATION_LEVEL3),            // (1 << 15)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_RESERVED16),                     // (1 << 16)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_RESERVED17),                     // (1 << 17)
    CREATE_COMPILER_FLAG_INFO(D3DCOMPILE_WARNINGS_ARE_ERRORS)             // (1 << 18)
};

#undef CREATE_COMPILER_FLAG_INFO

bool IsCompilerFlagSet(UINT mask, UINT flag)
{
    bool isFlagSet = IsMaskFlagSet(mask, flag);

    switch(flag)
    {
      case D3DCOMPILE_OPTIMIZATION_LEVEL0:
        return isFlagSet && !IsMaskFlagSet(mask, UINT(D3DCOMPILE_OPTIMIZATION_LEVEL3));

      case D3DCOMPILE_OPTIMIZATION_LEVEL1:
        return (mask & D3DCOMPILE_OPTIMIZATION_LEVEL2) == UINT(0);

      case D3DCOMPILE_OPTIMIZATION_LEVEL3:
        return isFlagSet && !IsMaskFlagSet(mask, UINT(D3DCOMPILE_OPTIMIZATION_LEVEL0));

      default:
        return isFlagSet;
    }
}

const char *GetCompilerFlagName(UINT mask, size_t flagIx)
{
    const CompilerFlagInfo &flagInfo = CompilerFlagInfos[flagIx];
    if (IsCompilerFlagSet(mask, flagInfo.mFlag))
    {
        return flagInfo.mName;
    }

    return nullptr;
}

}

namespace rx
{

CompileConfig::CompileConfig()
    : flags(0),
      name()
{
}

CompileConfig::CompileConfig(UINT flags, const std::string &name)
    : flags(flags),
      name(name)
{
}

HLSLCompiler::HLSLCompiler()
    : mD3DCompilerModule(NULL),
      mD3DCompileFunc(NULL),
      mD3DDisassembleFunc(NULL)
{
}

HLSLCompiler::~HLSLCompiler()
{
    release();
}

bool HLSLCompiler::initialize()
{
#if !defined(ANGLE_ENABLE_WINDOWS_STORE)
#if defined(ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES)
    // Find a D3DCompiler module that had already been loaded based on a predefined list of versions.
    static const char *d3dCompilerNames[] = ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES;

    for (size_t i = 0; i < ArraySize(d3dCompilerNames); ++i)
    {
        if (GetModuleHandleExA(0, d3dCompilerNames[i], &mD3DCompilerModule))
        {
            break;
        }
    }
#endif  // ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES

    // Load the compiler DLL specified by the environment, or default to QT_D3DCOMPILER_DLL
    const wchar_t *defaultCompiler = _wgetenv(L"QT_D3DCOMPILER_DLL");
    if (!defaultCompiler)
        defaultCompiler = QT_D3DCOMPILER_DLL;

    const wchar_t *compilerDlls[] = {
        defaultCompiler,
        L"d3dcompiler_47.dll",
        L"d3dcompiler_46.dll",
        L"d3dcompiler_45.dll",
        L"d3dcompiler_44.dll",
        L"d3dcompiler_43.dll",
        0
    };

    // Load the first available known compiler DLL
    for (int i = 0; compilerDlls[i]; ++i)
    {
        mD3DCompilerModule = LoadLibrary(compilerDlls[i]);
        if (mD3DCompilerModule)
            break;
    }

    if (!mD3DCompilerModule)
    {
        // Load the version of the D3DCompiler DLL associated with the Direct3D version ANGLE was built with.
        mD3DCompilerModule = LoadLibrary(D3DCOMPILER_DLL);
    }

    if (!mD3DCompilerModule)
    {
        ERR("No D3D compiler module found - aborting!\n");
        return false;
    }

    mD3DCompileFunc = reinterpret_cast<pD3DCompile>(GetProcAddress(mD3DCompilerModule, "D3DCompile"));
    ASSERT(mD3DCompileFunc);

    mD3DDisassembleFunc = reinterpret_cast<pD3DDisassemble>(GetProcAddress(mD3DCompilerModule, "D3DDisassemble"));
    ASSERT(mD3DDisassembleFunc);

#else
    // D3D Shader compiler is linked already into this module, so the export
    // can be directly assigned.
    mD3DCompilerModule = NULL;
    mD3DCompileFunc = reinterpret_cast<pD3DCompile>(D3DCompile);
    mD3DDisassembleFunc = reinterpret_cast<pD3DDisassemble>(D3DDisassemble);
#endif

    return mD3DCompileFunc != NULL;
}

void HLSLCompiler::release()
{
    if (mD3DCompilerModule)
    {
        FreeLibrary(mD3DCompilerModule);
        mD3DCompilerModule = NULL;
        mD3DCompileFunc = NULL;
        mD3DDisassembleFunc = NULL;
    }
}

gl::Error HLSLCompiler::compileToBinary(gl::InfoLog &infoLog, const std::string &hlsl, const std::string &profile,
                                        const std::vector<CompileConfig> &configs, const D3D_SHADER_MACRO *overrideMacros,
                                        ID3DBlob **outCompiledBlob, std::string *outDebugInfo) const
{
#if !defined(ANGLE_ENABLE_WINDOWS_STORE)
    ASSERT(mD3DCompilerModule);
#endif
    ASSERT(mD3DCompileFunc);

#if !defined(ANGLE_ENABLE_WINDOWS_STORE)
    if (gl::perfActive())
    {
        std::string sourcePath = getTempPath();
        std::string sourceText = FormatString("#line 2 \"%s\"\n\n%s", sourcePath.c_str(), hlsl.c_str());
        writeFile(sourcePath.c_str(), sourceText.c_str(), sourceText.size());
    }
#endif

    const D3D_SHADER_MACRO *macros = overrideMacros ? overrideMacros : NULL;

    for (size_t i = 0; i < configs.size(); ++i)
    {
        ID3DBlob *errorMessage = NULL;
        ID3DBlob *binary = NULL;

        HRESULT result = mD3DCompileFunc(hlsl.c_str(), hlsl.length(), gl::g_fakepath, macros, NULL, "main", profile.c_str(),
                                         configs[i].flags, 0, &binary, &errorMessage);

        if (errorMessage)
        {
            std::string message = reinterpret_cast<const char*>(errorMessage->GetBufferPointer());
            SafeRelease(errorMessage);

            infoLog.appendSanitized(message.c_str());
            TRACE("\n%s", hlsl.c_str());
            TRACE("\n%s", message.c_str());

            if (message.find("error X3531:") != std::string::npos)   // "can't unroll loops marked with loop attribute"
            {
                macros = NULL;   // Disable [loop] and [flatten]

                // Retry without changing compiler flags
                i--;
                continue;
            }
        }

        if (SUCCEEDED(result))
        {
            *outCompiledBlob = binary;

#if ANGLE_SHADER_DEBUG_INFO == ANGLE_ENABLED
            (*outDebugInfo) += "// COMPILER INPUT HLSL BEGIN\n\n" + hlsl + "\n// COMPILER INPUT HLSL END\n";
            (*outDebugInfo) += "\n\n// ASSEMBLY BEGIN\n\n";
            (*outDebugInfo) += "// Compiler configuration: " + configs[i].name + "\n// Flags:\n";
            for (size_t fIx = 0; fIx < ArraySize(CompilerFlagInfos); ++fIx)
            {
                const char *flagName = GetCompilerFlagName(configs[i].flags, fIx);
                if (flagName != nullptr)
                {
                    (*outDebugInfo) += std::string("// ") + flagName + "\n";
                }
            }

            (*outDebugInfo) += "// Macros:\n";
            if (macros == nullptr)
            {
                (*outDebugInfo) += "// - : -\n";
            }
            else
            {
                for (const D3D_SHADER_MACRO *mIt = macros; mIt->Name != nullptr; ++mIt)
                {
                    (*outDebugInfo) += std::string("// ") + mIt->Name + " : " + mIt->Definition + "\n";
                }
            }

            (*outDebugInfo) += "\n" + disassembleBinary(binary) + "\n// ASSEMBLY END\n";
#endif

            return gl::Error(GL_NO_ERROR);
        }
        else
        {
            if (result == E_OUTOFMEMORY)
            {
                *outCompiledBlob = NULL;
                return gl::Error(GL_OUT_OF_MEMORY, "HLSL compiler had an unexpected failure, result: 0x%X.", result);
            }

            infoLog.append("Warning: D3D shader compilation failed with %s flags.", configs[i].name.c_str());

            if (i + 1 < configs.size())
            {
                infoLog.append(" Retrying with %s.\n", configs[i + 1].name.c_str());
            }
        }
    }

    // None of the configurations succeeded in compiling this shader but the compiler is still intact
    *outCompiledBlob = NULL;
    return gl::Error(GL_NO_ERROR);
}

std::string HLSLCompiler::disassembleBinary(ID3DBlob *shaderBinary) const
{
    // Retrieve disassembly
    UINT flags = D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS | D3D_DISASM_ENABLE_INSTRUCTION_NUMBERING;
    ID3DBlob *disassembly = NULL;
    pD3DDisassemble disassembleFunc = reinterpret_cast<pD3DDisassemble>(mD3DDisassembleFunc);
    LPCVOID buffer = shaderBinary->GetBufferPointer();
    SIZE_T bufSize = shaderBinary->GetBufferSize();
    HRESULT result = disassembleFunc(buffer, bufSize, flags, "", &disassembly);

    std::string asmSrc;
    if (SUCCEEDED(result))
    {
        asmSrc = reinterpret_cast<const char*>(disassembly->GetBufferPointer());
    }

    SafeRelease(disassembly);

    return asmSrc;
}

}
