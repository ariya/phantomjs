//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DynamicHLSL.h: Interface for link and run-time HLSL generation
//

#ifndef LIBGLESV2_RENDERER_DYNAMIC_HLSL_H_
#define LIBGLESV2_RENDERER_DYNAMIC_HLSL_H_

#include "common/angleutils.h"
#include "libGLESv2/Constants.h"

#include "angle_gl.h"

#include <vector>
#include <map>

namespace sh
{
struct Attribute;
struct ShaderVariable;
}

namespace gl
{
class InfoLog;
struct VariableLocation;
struct LinkedVarying;
struct VertexAttribute;
struct VertexFormat;
struct PackedVarying;
struct Data;
}

namespace rx
{
class RendererD3D;
class ShaderD3D;

typedef const gl::PackedVarying *VaryingPacking[gl::IMPLEMENTATION_MAX_VARYING_VECTORS][4];

struct PixelShaderOutputVariable
{
    GLenum type;
    std::string name;
    std::string source;
    size_t outputIndex;
};

class DynamicHLSL
{
  public:
    explicit DynamicHLSL(RendererD3D *const renderer);

    int packVaryings(gl::InfoLog &infoLog, VaryingPacking packing, ShaderD3D *fragmentShader,
                     ShaderD3D *vertexShader, const std::vector<std::string>& transformFeedbackVaryings);
    std::string generateVertexShaderForInputLayout(const std::string &sourceShader, const gl::VertexFormat inputLayout[],
                                                   const sh::Attribute shaderAttributes[]) const;
    std::string generatePixelShaderForOutputSignature(const std::string &sourceShader, const std::vector<PixelShaderOutputVariable> &outputVariables,
                                                      bool usesFragDepth, const std::vector<GLenum> &outputLayout) const;
    bool generateShaderLinkHLSL(const gl::Data &data, gl::InfoLog &infoLog, int registers,
                                const VaryingPacking packing,
                                std::string &pixelHLSL, std::string &vertexHLSL,
                                ShaderD3D *fragmentShader, ShaderD3D *vertexShader,
                                const std::vector<std::string> &transformFeedbackVaryings,
                                std::vector<gl::LinkedVarying> *linkedVaryings,
                                std::map<int, gl::VariableLocation> *programOutputVars,
                                std::vector<PixelShaderOutputVariable> *outPixelShaderKey,
                                bool *outUsesFragDepth) const;

    std::string generateGeometryShaderHLSL(int registers, ShaderD3D *fragmentShader, ShaderD3D *vertexShader) const;
    void getInputLayoutSignature(const gl::VertexFormat inputLayout[], GLenum signature[]) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(DynamicHLSL);

    RendererD3D *const mRenderer;

    struct SemanticInfo;

    std::string getVaryingSemantic(bool pointSize) const;
    SemanticInfo getSemanticInfo(int startRegisters, bool fragCoord, bool pointCoord, bool pointSize,
                                        bool pixelShader) const;
    std::string generateVaryingLinkHLSL(const SemanticInfo &info, const std::string &varyingHLSL) const;
    std::string generateVaryingHLSL(const ShaderD3D *shader) const;
    void storeUserLinkedVaryings(const ShaderD3D *vertexShader, std::vector<gl::LinkedVarying> *linkedVaryings) const;
    void storeBuiltinLinkedVaryings(const SemanticInfo &info, std::vector<gl::LinkedVarying> *linkedVaryings) const;
    void defineOutputVariables(ShaderD3D *fragmentShader, std::map<int, gl::VariableLocation> *programOutputVars) const;
    std::string generatePointSpriteHLSL(int registers, ShaderD3D *fragmentShader, ShaderD3D *vertexShader) const;

    // Prepend an underscore
    static std::string decorateVariable(const std::string &name);

    std::string generateAttributeConversionHLSL(const gl::VertexFormat &vertexFormat, const sh::ShaderVariable &shaderAttrib) const;
};

}

#endif // LIBGLESV2_RENDERER_DYNAMIC_HLSL_H_
