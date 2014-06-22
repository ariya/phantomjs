#include "precompiled.h"
//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderExecutable9.cpp: Implements a D3D9-specific class to contain shader
// executable implementation details.

#include "libGLESv2/renderer/ShaderExecutable9.h"

#include "common/debug.h"

namespace rx
{

ShaderExecutable9::ShaderExecutable9(const void *function, size_t length, IDirect3DPixelShader9 *executable)
    : ShaderExecutable(function, length)
{
    mPixelExecutable = executable;
    mVertexExecutable = NULL;
}

ShaderExecutable9::ShaderExecutable9(const void *function, size_t length, IDirect3DVertexShader9 *executable)
    : ShaderExecutable(function, length)
{
    mVertexExecutable = executable;
    mPixelExecutable = NULL;
}

ShaderExecutable9::~ShaderExecutable9()
{
    if (mVertexExecutable)
    {
        mVertexExecutable->Release();
    }
    if (mPixelExecutable)
    {
        mPixelExecutable->Release();
    }
}

ShaderExecutable9 *ShaderExecutable9::makeShaderExecutable9(ShaderExecutable *executable)
{
    ASSERT(HAS_DYNAMIC_TYPE(ShaderExecutable9*, executable));
    return static_cast<ShaderExecutable9*>(executable);
}

IDirect3DVertexShader9 *ShaderExecutable9::getVertexShader() const
{
    return mVertexExecutable;
}

IDirect3DPixelShader9 *ShaderExecutable9::getPixelShader() const
{
    return mPixelExecutable;
}

}