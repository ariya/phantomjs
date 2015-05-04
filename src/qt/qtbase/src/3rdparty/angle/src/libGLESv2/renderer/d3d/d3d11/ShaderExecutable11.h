//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderExecutable11.h: Defines a D3D11-specific class to contain shader
// executable implementation details.

#ifndef LIBGLESV2_RENDERER_SHADEREXECUTABLE11_H_
#define LIBGLESV2_RENDERER_SHADEREXECUTABLE11_H_

#include "libGLESv2/renderer/ShaderExecutable.h"

namespace rx
{
class Renderer11;
class UniformStorage11;

class ShaderExecutable11 : public ShaderExecutable
{
  public:
    ShaderExecutable11(const void *function, size_t length, ID3D11PixelShader *executable);
    ShaderExecutable11(const void *function, size_t length, ID3D11VertexShader *executable, ID3D11GeometryShader *streamOut);
    ShaderExecutable11(const void *function, size_t length, ID3D11GeometryShader *executable);

    virtual ~ShaderExecutable11();

    static ShaderExecutable11 *makeShaderExecutable11(ShaderExecutable *executable);

    ID3D11PixelShader *getPixelShader() const;
    ID3D11VertexShader *getVertexShader() const;
    ID3D11GeometryShader *getGeometryShader() const;
    ID3D11GeometryShader *getStreamOutShader() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(ShaderExecutable11);

    ID3D11PixelShader *mPixelExecutable;
    ID3D11VertexShader *mVertexExecutable;
    ID3D11GeometryShader *mGeometryExecutable;
    ID3D11GeometryShader *mStreamOutExecutable;
};

class UniformStorage11 : public UniformStorage
{
  public:
    UniformStorage11(Renderer11 *renderer, size_t initialSize);
    virtual ~UniformStorage11();

    static const UniformStorage11 *makeUniformStorage11(const UniformStorage *uniformStorage);

    ID3D11Buffer *getConstantBuffer() const { return mConstantBuffer; }

  private:
    ID3D11Buffer *mConstantBuffer;
};

}

#endif // LIBGLESV2_RENDERER_SHADEREXECUTABLE11_H_
