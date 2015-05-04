#ifndef LIBGLESV2_RENDERER_HLSL_D3DCOMPILER_H_
#define LIBGLESV2_RENDERER_HLSL_D3DCOMPILER_H_

#include "libGLESv2/Error.h"

#include "common/angleutils.h"
#include "common/platform.h"

#include <vector>
#include <string>

namespace gl
{
class InfoLog;
}

namespace rx
{

struct CompileConfig
{
    UINT flags;
    std::string name;

    CompileConfig();
    CompileConfig(UINT flags, const std::string &name);
};

class HLSLCompiler
{
  public:
    HLSLCompiler();
    ~HLSLCompiler();

    bool initialize();
    void release();

    // Attempt to compile a HLSL shader using the supplied configurations, may output a NULL compiled blob
    // even if no GL errors are returned.
    gl::Error compileToBinary(gl::InfoLog &infoLog, const std::string &hlsl, const std::string &profile,
                              const std::vector<CompileConfig> &configs, const D3D_SHADER_MACRO *overrideMacros,
                              ID3DBlob **outCompiledBlob, std::string *outDebugInfo) const;

    std::string disassembleBinary(ID3DBlob* shaderBinary) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(HLSLCompiler);

    HMODULE mD3DCompilerModule;
    pD3DCompile mD3DCompileFunc;
    pD3DDisassemble mD3DDisassembleFunc;
};

}

#endif // LIBGLESV2_RENDERER_HLSL_D3DCOMPILER_H_
