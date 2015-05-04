//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// PixelTransfer11.h:
//   Buffer-to-Texture and Texture-to-Buffer data transfers.
//   Used to implement pixel unpack and pixel pack buffers in ES3.

#ifndef LIBGLESV2_PIXELTRANSFER11_H_
#define LIBGLESV2_PIXELTRANSFER11_H_

#include "libGLESv2/Error.h"

#include "common/platform.h"

#include <GLES2/gl2.h>

#include <map>

namespace gl
{

class Buffer;
struct Box;
struct Extents;
struct PixelUnpackState;

}

namespace rx
{
class Renderer11;
class RenderTarget;

class PixelTransfer11
{
  public:
    explicit PixelTransfer11(Renderer11 *renderer);
    ~PixelTransfer11();

    // unpack: the source buffer is stored in the unpack state, and buffer strides
    // offset: the start of the data within the unpack buffer
    // destRenderTarget: individual slice/layer of a target texture
    // destinationFormat/sourcePixelsType: determines shaders + shader parameters
    // destArea: the sub-section of destRenderTarget to copy to
    gl::Error copyBufferToTexture(const gl::PixelUnpackState &unpack, unsigned int offset, RenderTarget *destRenderTarget,
                                  GLenum destinationFormat, GLenum sourcePixelsType, const gl::Box &destArea);

  private:

    struct CopyShaderParams
    {
        unsigned int FirstPixelOffset;
        unsigned int PixelsPerRow;
        unsigned int RowStride;
        unsigned int RowsPerSlice;
        float PositionOffset[2];
        float PositionScale[2];
        int TexLocationOffset[2];
        int TexLocationScale[2];
    };

    static void setBufferToTextureCopyParams(const gl::Box &destArea, const gl::Extents &destSize, GLenum internalFormat,
                                             const gl::PixelUnpackState &unpack, unsigned int offset, CopyShaderParams *parametersOut);

    gl::Error loadResources();
    gl::Error buildShaderMap();
    ID3D11PixelShader *findBufferToTexturePS(GLenum internalFormat) const;

    Renderer11 *mRenderer;

    bool mResourcesLoaded;
    std::map<GLenum, ID3D11PixelShader *> mBufferToTexturePSMap;
    ID3D11VertexShader *mBufferToTextureVS;
    ID3D11GeometryShader *mBufferToTextureGS;
    ID3D11Buffer *mParamsConstantBuffer;
    CopyShaderParams mParamsData;

    ID3D11RasterizerState *mCopyRasterizerState;
    ID3D11DepthStencilState *mCopyDepthStencilState;

};

}

#endif // LIBGLESV2_PIXELTRANSFER11_H_
