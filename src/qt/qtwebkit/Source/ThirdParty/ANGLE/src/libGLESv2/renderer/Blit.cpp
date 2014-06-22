#include "precompiled.h"
//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Blit.cpp: Surface copy utility class.

#include "libGLESv2/renderer/Blit.h"

#include "libGLESv2/main.h"
#include "libGLESv2/renderer/renderer9_utils.h"
#include "libGLESv2/renderer/TextureStorage9.h"
#include "libGLESv2/renderer/RenderTarget9.h"
#include "libGLESv2/renderer/Renderer9.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/Renderbuffer.h"

namespace
{
#include "libGLESv2/renderer/shaders/compiled/standardvs.h"
#include "libGLESv2/renderer/shaders/compiled/flipyvs.h"
#include "libGLESv2/renderer/shaders/compiled/passthroughps.h"
#include "libGLESv2/renderer/shaders/compiled/luminanceps.h"
#include "libGLESv2/renderer/shaders/compiled/componentmaskps.h"

const BYTE* const g_shaderCode[] =
{
    g_vs20_standardvs,
    g_vs20_flipyvs,
    g_ps20_passthroughps,
    g_ps20_luminanceps,
    g_ps20_componentmaskps
};

const size_t g_shaderSize[] =
{
    sizeof(g_vs20_standardvs),
    sizeof(g_vs20_flipyvs),
    sizeof(g_ps20_passthroughps),
    sizeof(g_ps20_luminanceps),
    sizeof(g_ps20_componentmaskps)
};
}

namespace rx
{
Blit::Blit(rx::Renderer9 *renderer)
  : mRenderer(renderer), mQuadVertexBuffer(NULL), mQuadVertexDeclaration(NULL), mSavedStateBlock(NULL), mSavedRenderTarget(NULL), mSavedDepthStencil(NULL)
{
    initGeometry();
    memset(mCompiledShaders, 0, sizeof(mCompiledShaders));
}

Blit::~Blit()
{
    if (mSavedStateBlock) mSavedStateBlock->Release();
    if (mQuadVertexBuffer) mQuadVertexBuffer->Release();
    if (mQuadVertexDeclaration) mQuadVertexDeclaration->Release();

    for (int i = 0; i < SHADER_COUNT; i++)
    {
        if (mCompiledShaders[i])
        {
            mCompiledShaders[i]->Release();
        }
    }
}

void Blit::initGeometry()
{
    static const float quad[] =
    {
        -1, -1,
        -1,  1,
         1, -1,
         1,  1
    };

    IDirect3DDevice9 *device = mRenderer->getDevice();

    HRESULT result = device->CreateVertexBuffer(sizeof(quad), D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &mQuadVertexBuffer, NULL);

    if (FAILED(result))
    {
        ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
        return gl::error(GL_OUT_OF_MEMORY);
    }

    void *lockPtr = NULL;
    result = mQuadVertexBuffer->Lock(0, 0, &lockPtr, 0);

    if (FAILED(result) || lockPtr == NULL)
    {
        ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
        return gl::error(GL_OUT_OF_MEMORY);
    }

    memcpy(lockPtr, quad, sizeof(quad));
    mQuadVertexBuffer->Unlock();

    static const D3DVERTEXELEMENT9 elements[] =
    {
        { 0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        D3DDECL_END()
    };

    result = device->CreateVertexDeclaration(elements, &mQuadVertexDeclaration);

    if (FAILED(result))
    {
        ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
        return gl::error(GL_OUT_OF_MEMORY);
    }
}

template <class D3DShaderType>
bool Blit::setShader(ShaderId source, const char *profile,
                     D3DShaderType *(rx::Renderer9::*createShader)(const DWORD *, size_t length),
                     HRESULT (WINAPI IDirect3DDevice9::*setShader)(D3DShaderType*))
{
    IDirect3DDevice9 *device = mRenderer->getDevice();

    D3DShaderType *shader;

    if (mCompiledShaders[source] != NULL)
    {
        shader = static_cast<D3DShaderType*>(mCompiledShaders[source]);
    }
    else
    {
        const BYTE* shaderCode = g_shaderCode[source];
        size_t shaderSize = g_shaderSize[source];

        shader = (mRenderer->*createShader)(reinterpret_cast<const DWORD*>(shaderCode), shaderSize);
        if (!shader)
        {
            ERR("Failed to create shader for blit operation");
            return false;
        }

        mCompiledShaders[source] = shader;
    }

    HRESULT hr = (device->*setShader)(shader);

    if (FAILED(hr))
    {
        ERR("Failed to set shader for blit operation");
        return false;
    }

    return true;
}

bool Blit::setVertexShader(ShaderId shader)
{
    return setShader<IDirect3DVertexShader9>(shader, "vs_2_0", &rx::Renderer9::createVertexShader, &IDirect3DDevice9::SetVertexShader);
}

bool Blit::setPixelShader(ShaderId shader)
{
    return setShader<IDirect3DPixelShader9>(shader, "ps_2_0", &rx::Renderer9::createPixelShader, &IDirect3DDevice9::SetPixelShader);
}

RECT Blit::getSurfaceRect(IDirect3DSurface9 *surface) const
{
    D3DSURFACE_DESC desc;
    surface->GetDesc(&desc);

    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = desc.Width;
    rect.bottom = desc.Height;

    return rect;
}

bool Blit::boxFilter(IDirect3DSurface9 *source, IDirect3DSurface9 *dest)
{
    IDirect3DTexture9 *texture = copySurfaceToTexture(source, getSurfaceRect(source));
    if (!texture)
    {
        return false;
    }

    IDirect3DDevice9 *device = mRenderer->getDevice();

    saveState();

    device->SetTexture(0, texture);
    device->SetRenderTarget(0, dest);

    setVertexShader(SHADER_VS_STANDARD);
    setPixelShader(SHADER_PS_PASSTHROUGH);

    setCommonBlitState();
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    setViewport(getSurfaceRect(dest), 0, 0);

    render();

    texture->Release();

    restoreState();

    return true;
}

bool Blit::copy(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, TextureStorageInterface2D *storage, GLint level)
{
    RenderTarget9 *renderTarget = NULL;
    IDirect3DSurface9 *source = NULL;
    gl::Renderbuffer *colorbuffer = framebuffer->getColorbuffer(0);

    if (colorbuffer)
    {
        renderTarget = RenderTarget9::makeRenderTarget9(colorbuffer->getRenderTarget());
    }
    
    if (renderTarget)
    {
        source = renderTarget->getSurface();
    }

    if (!source)
    {
        ERR("Failed to retrieve the render target.");
        return gl::error(GL_OUT_OF_MEMORY, false);
    }

    TextureStorage9_2D *storage9 = TextureStorage9_2D::makeTextureStorage9_2D(storage->getStorageInstance());
    IDirect3DSurface9 *destSurface = storage9->getSurfaceLevel(level, true);
    bool result = false;
        
    if (destSurface)
    {
        result = copy(source, sourceRect, destFormat, xoffset, yoffset, destSurface);
        destSurface->Release();
    }

    source->Release();
    return result;
}

bool Blit::copy(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, TextureStorageInterfaceCube *storage, GLenum target, GLint level)
{
    RenderTarget9 *renderTarget = NULL;
    IDirect3DSurface9 *source = NULL;
    gl::Renderbuffer *colorbuffer = framebuffer->getColorbuffer(0);

    if (colorbuffer)
    {
        renderTarget = RenderTarget9::makeRenderTarget9(colorbuffer->getRenderTarget());
    }
    
    if (renderTarget)
    {
        source = renderTarget->getSurface();
    }

    if (!source)
    {
        ERR("Failed to retrieve the render target.");
        return gl::error(GL_OUT_OF_MEMORY, false);
    }

    TextureStorage9_Cube *storage9 = TextureStorage9_Cube::makeTextureStorage9_Cube(storage->getStorageInstance());
    IDirect3DSurface9 *destSurface = storage9->getCubeMapSurface(target, level, true);
    bool result = false;

    if (destSurface)
    {
        result = copy(source, sourceRect, destFormat, xoffset, yoffset, destSurface);
        destSurface->Release();
    }

    source->Release();
    return result;
}

bool Blit::copy(IDirect3DSurface9 *source, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, IDirect3DSurface9 *dest)
{
    if (!dest)
    {
        return false;
    }

    IDirect3DDevice9 *device = mRenderer->getDevice();

    D3DSURFACE_DESC sourceDesc;
    D3DSURFACE_DESC destDesc;
    source->GetDesc(&sourceDesc);
    dest->GetDesc(&destDesc);

    if (sourceDesc.Format == destDesc.Format && destDesc.Usage & D3DUSAGE_RENDERTARGET &&
        d3d9_gl::IsFormatChannelEquivalent(destDesc.Format, destFormat))   // Can use StretchRect
    {
        RECT destRect = {xoffset, yoffset, xoffset + (sourceRect.right - sourceRect.left), yoffset + (sourceRect.bottom - sourceRect.top)};
        HRESULT result = device->StretchRect(source, &sourceRect, dest, &destRect, D3DTEXF_POINT);

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            return gl::error(GL_OUT_OF_MEMORY, false);
        }
    }
    else
    {
        return formatConvert(source, sourceRect, destFormat, xoffset, yoffset, dest);
    }
    return true;
}

bool Blit::formatConvert(IDirect3DSurface9 *source, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, IDirect3DSurface9 *dest)
{
    IDirect3DTexture9 *texture = copySurfaceToTexture(source, sourceRect);
    if (!texture)
    {
        return false;
    }

    IDirect3DDevice9 *device = mRenderer->getDevice();

    saveState();

    device->SetTexture(0, texture);
    device->SetRenderTarget(0, dest);

    setViewport(sourceRect, xoffset, yoffset);

    setCommonBlitState();
    if (setFormatConvertShaders(destFormat))
    {
        render();
    }

    texture->Release();

    restoreState();

    return true;
}

bool Blit::setFormatConvertShaders(GLenum destFormat)
{
    bool okay = setVertexShader(SHADER_VS_STANDARD);

    switch (destFormat)
    {
      default: UNREACHABLE();
      case GL_RGBA:
      case GL_BGRA_EXT:
      case GL_RGB:
      case GL_ALPHA:
        okay = okay && setPixelShader(SHADER_PS_COMPONENTMASK);
        break;

      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
        okay = okay && setPixelShader(SHADER_PS_LUMINANCE);
        break;
    }

    if (!okay)
    {
        return false;
    }

    enum { X = 0, Y = 1, Z = 2, W = 3 };

    // The meaning of this constant depends on the shader that was selected.
    // See the shader assembly code above for details.
    float psConst0[4] = { 0, 0, 0, 0 };

    switch (destFormat)
    {
      default: UNREACHABLE();
      case GL_RGBA:
      case GL_BGRA_EXT:
        psConst0[X] = 1;
        psConst0[Z] = 1;
        break;

      case GL_RGB:
        psConst0[X] = 1;
        psConst0[W] = 1;
        break;

      case GL_ALPHA:
        psConst0[Z] = 1;
        break;

      case GL_LUMINANCE:
        psConst0[Y] = 1;
        break;

      case GL_LUMINANCE_ALPHA:
        psConst0[X] = 1;
        break;
    }

    mRenderer->getDevice()->SetPixelShaderConstantF(0, psConst0, 1);

    return true;
}

IDirect3DTexture9 *Blit::copySurfaceToTexture(IDirect3DSurface9 *surface, const RECT &sourceRect)
{
    if (!surface)
    {
        return NULL;
    }

    IDirect3DDevice9 *device = mRenderer->getDevice();

    D3DSURFACE_DESC sourceDesc;
    surface->GetDesc(&sourceDesc);

    // Copy the render target into a texture
    IDirect3DTexture9 *texture;
    HRESULT result = device->CreateTexture(sourceRect.right - sourceRect.left, sourceRect.bottom - sourceRect.top, 1, D3DUSAGE_RENDERTARGET, sourceDesc.Format, D3DPOOL_DEFAULT, &texture, NULL);

    if (FAILED(result))
    {
        ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
        return gl::error(GL_OUT_OF_MEMORY, (IDirect3DTexture9*)NULL);
    }

    IDirect3DSurface9 *textureSurface;
    result = texture->GetSurfaceLevel(0, &textureSurface);

    if (FAILED(result))
    {
        ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
        texture->Release();
        return gl::error(GL_OUT_OF_MEMORY, (IDirect3DTexture9*)NULL);
    }

    mRenderer->endScene();
    result = device->StretchRect(surface, &sourceRect, textureSurface, NULL, D3DTEXF_NONE);

    textureSurface->Release();

    if (FAILED(result))
    {
        ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
        texture->Release();
        return gl::error(GL_OUT_OF_MEMORY, (IDirect3DTexture9*)NULL);
    }

    return texture;
}

void Blit::setViewport(const RECT &sourceRect, GLint xoffset, GLint yoffset)
{
    IDirect3DDevice9 *device = mRenderer->getDevice();

    D3DVIEWPORT9 vp;
    vp.X      = xoffset;
    vp.Y      = yoffset;
    vp.Width  = sourceRect.right - sourceRect.left;
    vp.Height = sourceRect.bottom - sourceRect.top;
    vp.MinZ   = 0.0f;
    vp.MaxZ   = 1.0f;
    device->SetViewport(&vp);

    float halfPixelAdjust[4] = { -1.0f/vp.Width, 1.0f/vp.Height, 0, 0 };
    device->SetVertexShaderConstantF(0, halfPixelAdjust, 1);
}

void Blit::setCommonBlitState()
{
    IDirect3DDevice9 *device = mRenderer->getDevice();

    device->SetDepthStencilSurface(NULL);

    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
    device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);
    device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    RECT scissorRect = {0};   // Scissoring is disabled for flipping, but we need this to capture and restore the old rectangle
    device->SetScissorRect(&scissorRect);

    for(int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        device->SetStreamSourceFreq(i, 1);
    }
}

void Blit::render()
{
    IDirect3DDevice9 *device = mRenderer->getDevice();

    HRESULT hr = device->SetStreamSource(0, mQuadVertexBuffer, 0, 2 * sizeof(float));
    hr = device->SetVertexDeclaration(mQuadVertexDeclaration);

    mRenderer->startScene();
    hr = device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
}

void Blit::saveState()
{
    IDirect3DDevice9 *device = mRenderer->getDevice();

    HRESULT hr;

    device->GetDepthStencilSurface(&mSavedDepthStencil);
    device->GetRenderTarget(0, &mSavedRenderTarget);

    if (mSavedStateBlock == NULL)
    {
        hr = device->BeginStateBlock();
        ASSERT(SUCCEEDED(hr) || hr == D3DERR_OUTOFVIDEOMEMORY || hr == E_OUTOFMEMORY);

        setCommonBlitState();

        static const float dummyConst[4] = { 0, 0, 0, 0 };

        device->SetVertexShader(NULL);
        device->SetVertexShaderConstantF(0, dummyConst, 1);
        device->SetPixelShader(NULL);
        device->SetPixelShaderConstantF(0, dummyConst, 1);

        D3DVIEWPORT9 dummyVp;
        dummyVp.X = 0;
        dummyVp.Y = 0;
        dummyVp.Width = 1;
        dummyVp.Height = 1;
        dummyVp.MinZ = 0;
        dummyVp.MaxZ = 1;

        device->SetViewport(&dummyVp);

        device->SetTexture(0, NULL);

        device->SetStreamSource(0, mQuadVertexBuffer, 0, 0);

        device->SetVertexDeclaration(mQuadVertexDeclaration);

        hr = device->EndStateBlock(&mSavedStateBlock);
        ASSERT(SUCCEEDED(hr) || hr == D3DERR_OUTOFVIDEOMEMORY || hr == E_OUTOFMEMORY);
    }

    ASSERT(mSavedStateBlock != NULL);

    if (mSavedStateBlock != NULL)
    {
        hr = mSavedStateBlock->Capture();
        ASSERT(SUCCEEDED(hr));
    }
}

void Blit::restoreState()
{
    IDirect3DDevice9 *device = mRenderer->getDevice();

    device->SetDepthStencilSurface(mSavedDepthStencil);
    if (mSavedDepthStencil != NULL)
    {
        mSavedDepthStencil->Release();
        mSavedDepthStencil = NULL;
    }

    device->SetRenderTarget(0, mSavedRenderTarget);
    if (mSavedRenderTarget != NULL)
    {
        mSavedRenderTarget->Release();
        mSavedRenderTarget = NULL;
    }

    ASSERT(mSavedStateBlock != NULL);

    if (mSavedStateBlock != NULL)
    {
        mSavedStateBlock->Apply();
    }
}

}
