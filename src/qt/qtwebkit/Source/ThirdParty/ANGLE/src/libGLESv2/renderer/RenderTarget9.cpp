#include "precompiled.h"
//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderTarget9.cpp: Implements a D3D9-specific wrapper for IDirect3DSurface9
// pointers retained by renderbuffers.

#include "libGLESv2/renderer/RenderTarget9.h"
#include "libGLESv2/renderer/Renderer9.h"

#include "libGLESv2/renderer/renderer9_utils.h"
#include "libGLESv2/main.h"

namespace rx
{

RenderTarget9::RenderTarget9(Renderer *renderer, IDirect3DSurface9 *surface)
{
    mRenderer = Renderer9::makeRenderer9(renderer);
    mRenderTarget = surface;

    if (mRenderTarget)
    {
        D3DSURFACE_DESC description;
        mRenderTarget->GetDesc(&description);

        mWidth = description.Width;
        mHeight = description.Height;
        
        mInternalFormat = d3d9_gl::GetEquivalentFormat(description.Format);
        mActualFormat = d3d9_gl::GetEquivalentFormat(description.Format);
        mSamples = d3d9_gl::GetSamplesFromMultisampleType(description.MultiSampleType);
    }
}

RenderTarget9::RenderTarget9(Renderer *renderer, GLsizei width, GLsizei height, GLenum format, GLsizei samples)
{
    mRenderer = Renderer9::makeRenderer9(renderer);
    mRenderTarget = NULL;

    D3DFORMAT requestedFormat = gl_d3d9::ConvertRenderbufferFormat(format);
    int supportedSamples = mRenderer->getNearestSupportedSamples(requestedFormat, samples);

    if (supportedSamples == -1)
    {
        gl::error(GL_OUT_OF_MEMORY);

        return;
    }

    HRESULT result = D3DERR_INVALIDCALL;
    
    if (width > 0 && height > 0)
    {
        if (requestedFormat == D3DFMT_D24S8)
        {
            result = mRenderer->getDevice()->CreateDepthStencilSurface(width, height, requestedFormat,
                                                                       gl_d3d9::GetMultisampleTypeFromSamples(supportedSamples),
                                                                       0, FALSE, &mRenderTarget, NULL);
        }
        else
        {
            result = mRenderer->getDevice()->CreateRenderTarget(width, height, requestedFormat, 
                                                                gl_d3d9::GetMultisampleTypeFromSamples(supportedSamples),
                                                                0, FALSE, &mRenderTarget, NULL);
        }

        if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
        {
            gl::error(GL_OUT_OF_MEMORY);

            return;
        }

        ASSERT(SUCCEEDED(result));
    }

    mWidth = width;
    mHeight = height;
    mInternalFormat = format;
    mSamples = supportedSamples;
    mActualFormat = d3d9_gl::GetEquivalentFormat(requestedFormat);
}

RenderTarget9::~RenderTarget9()
{
    if (mRenderTarget)
    {
        mRenderTarget->Release();
    }
}

RenderTarget9 *RenderTarget9::makeRenderTarget9(RenderTarget *target)
{
    ASSERT(HAS_DYNAMIC_TYPE(rx::RenderTarget9*, target));
    return static_cast<rx::RenderTarget9*>(target);
}

IDirect3DSurface9 *RenderTarget9::getSurface()
{
    // Caller is responsible for releasing the returned surface reference.
    if (mRenderTarget)
    {
        mRenderTarget->AddRef();
    }

    return mRenderTarget;
}

}