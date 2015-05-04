//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Blit11.cpp: Texture copy utility class.

#ifndef LIBGLESV2_BLIT11_H_
#define LIBGLESV2_BLIT11_H_

#include "common/angleutils.h"
#include "libGLESv2/angletypes.h"
#include "libGLESv2/Error.h"

#include <map>

namespace rx
{
class Renderer11;

class Blit11
{
  public:
    explicit Blit11(Renderer11 *renderer);
    ~Blit11();

    gl::Error swizzleTexture(ID3D11ShaderResourceView *source, ID3D11RenderTargetView *dest, const gl::Extents &size,
                             GLenum swizzleRed, GLenum swizzleGreen, GLenum swizzleBlue, GLenum swizzleAlpha);

    gl::Error copyTexture(ID3D11ShaderResourceView *source, const gl::Box &sourceArea, const gl::Extents &sourceSize,
                          ID3D11RenderTargetView *dest, const gl::Box &destArea, const gl::Extents &destSize,
                          const gl::Rectangle *scissor, GLenum destFormat, GLenum filter);

    gl::Error copyStencil(ID3D11Resource *source, unsigned int sourceSubresource, const gl::Box &sourceArea, const gl::Extents &sourceSize,
                          ID3D11Resource *dest, unsigned int destSubresource, const gl::Box &destArea, const gl::Extents &destSize,
                          const gl::Rectangle *scissor);

    gl::Error copyDepth(ID3D11ShaderResourceView *source, const gl::Box &sourceArea, const gl::Extents &sourceSize,
                        ID3D11DepthStencilView *dest, const gl::Box &destArea, const gl::Extents &destSize,
                        const gl::Rectangle *scissor);

    gl::Error copyDepthStencil(ID3D11Resource *source, unsigned int sourceSubresource, const gl::Box &sourceArea, const gl::Extents &sourceSize,
                               ID3D11Resource *dest, unsigned int destSubresource, const gl::Box &destArea, const gl::Extents &destSize,
                               const gl::Rectangle *scissor);

  private:
    Renderer11 *mRenderer;

    struct BlitParameters
    {
        GLenum mDestinationFormat;
        bool mSignedInteger;
        bool m3DBlit;
    };

    gl::Error copyDepthStencil(ID3D11Resource *source, unsigned int sourceSubresource, const gl::Box &sourceArea, const gl::Extents &sourceSize,
                               ID3D11Resource *dest, unsigned int destSubresource, const gl::Box &destArea, const gl::Extents &destSize,
                               const gl::Rectangle *scissor, bool stencilOnly);

    static bool compareBlitParameters(const BlitParameters &a, const BlitParameters &b);

    typedef void (*WriteVertexFunction)(const gl::Box &sourceArea, const gl::Extents &sourceSize,
                                        const gl::Box &destArea, const gl::Extents &destSize,
                                        void *outVertices, unsigned int *outStride, unsigned int *outVertexCount,
                                        D3D11_PRIMITIVE_TOPOLOGY *outTopology);

    struct Shader
    {
        WriteVertexFunction mVertexWriteFunction;
        ID3D11InputLayout *mInputLayout;
        ID3D11VertexShader *mVertexShader;
        ID3D11GeometryShader *mGeometryShader;
        ID3D11PixelShader *mPixelShader;
    };

    typedef bool (*BlitParametersComparisonFunction)(const BlitParameters&, const BlitParameters &);
    typedef std::map<BlitParameters, Shader, BlitParametersComparisonFunction> BlitShaderMap;
    BlitShaderMap mBlitShaderMap;

    void add2DBlitShaderToMap(GLenum destFormat, bool signedInteger, ID3D11PixelShader *ps);
    void add3DBlitShaderToMap(GLenum destFormat, bool signedInteger, ID3D11PixelShader *ps);

    struct SwizzleParameters
    {
        GLenum mDestinationType;
        D3D11_SRV_DIMENSION mViewDimension;
    };

    static bool compareSwizzleParameters(const SwizzleParameters &a, const SwizzleParameters &b);

    typedef bool (*SwizzleParametersComparisonFunction)(const SwizzleParameters&, const SwizzleParameters &);
    typedef std::map<SwizzleParameters, Shader, SwizzleParametersComparisonFunction> SwizzleShaderMap;
    SwizzleShaderMap mSwizzleShaderMap;

    void addSwizzleShaderToMap(GLenum destType, D3D11_SRV_DIMENSION viewDimension, ID3D11PixelShader *ps);

    void buildShaderMap();
    void clearShaderMap();

    ID3D11Buffer *mVertexBuffer;
    ID3D11SamplerState *mPointSampler;
    ID3D11SamplerState *mLinearSampler;
    ID3D11RasterizerState *mScissorEnabledRasterizerState;
    ID3D11RasterizerState *mScissorDisabledRasterizerState;
    ID3D11DepthStencilState *mDepthStencilState;

    ID3D11InputLayout *mQuad2DIL;
    ID3D11VertexShader *mQuad2DVS;
    ID3D11PixelShader *mDepthPS;

    ID3D11InputLayout *mQuad3DIL;
    ID3D11VertexShader *mQuad3DVS;
    ID3D11GeometryShader *mQuad3DGS;

    ID3D11Buffer *mSwizzleCB;

    DISALLOW_COPY_AND_ASSIGN(Blit11);
};

}

#endif   // LIBGLESV2_BLIT11_H_
