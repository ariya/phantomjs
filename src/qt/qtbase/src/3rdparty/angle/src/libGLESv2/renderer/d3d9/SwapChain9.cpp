#include "precompiled.h"
//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SwapChain9.cpp: Implements a back-end specific class for the D3D9 swap chain.

#include "libGLESv2/renderer/d3d9/SwapChain9.h"
#include "libGLESv2/renderer/d3d9/renderer9_utils.h"
#include "libGLESv2/renderer/d3d9/Renderer9.h"

namespace rx
{

SwapChain9::SwapChain9(Renderer9 *renderer, HWND window, HANDLE shareHandle,
                       GLenum backBufferFormat, GLenum depthBufferFormat)
    : mRenderer(renderer), SwapChain(window, shareHandle, backBufferFormat, depthBufferFormat)
{
    mSwapChain = NULL;
    mBackBuffer = NULL;
    mDepthStencil = NULL;
    mRenderTarget = NULL;
    mOffscreenTexture = NULL;
    mWidth = -1;
    mHeight = -1;
    mSwapInterval = -1;
}

SwapChain9::~SwapChain9()
{
    release();
}

void SwapChain9::release()
{
    if (mSwapChain)
    {
        mSwapChain->Release();
        mSwapChain = NULL;
    }

    if (mBackBuffer)
    {
        mBackBuffer->Release();
        mBackBuffer = NULL;
    }

    if (mDepthStencil)
    {
        mDepthStencil->Release();
        mDepthStencil = NULL;
    }

    if (mRenderTarget)
    {
        mRenderTarget->Release();
        mRenderTarget = NULL;
    }

    if (mOffscreenTexture)
    {
        mOffscreenTexture->Release();
        mOffscreenTexture = NULL;
    }

    if (mWindow)
        mShareHandle = NULL;
}

static DWORD convertInterval(EGLint interval)
{
#if ANGLE_FORCE_VSYNC_OFF
    return D3DPRESENT_INTERVAL_IMMEDIATE;
#else
    switch(interval)
    {
      case 0: return D3DPRESENT_INTERVAL_IMMEDIATE;
      case 1: return D3DPRESENT_INTERVAL_ONE;
      case 2: return D3DPRESENT_INTERVAL_TWO;
      case 3: return D3DPRESENT_INTERVAL_THREE;
      case 4: return D3DPRESENT_INTERVAL_FOUR;
      default: UNREACHABLE();
    }

    return D3DPRESENT_INTERVAL_DEFAULT;
#endif
}

EGLint SwapChain9::resize(int backbufferWidth, int backbufferHeight)
{
    // D3D9 does not support resizing swap chains without recreating them
    return reset(backbufferWidth, backbufferHeight, mSwapInterval);
}

EGLint SwapChain9::reset(int backbufferWidth, int backbufferHeight, EGLint swapInterval)
{
    IDirect3DDevice9 *device = mRenderer->getDevice();

    if (device == NULL)
    {
        return EGL_BAD_ACCESS;
    }

    // Evict all non-render target textures to system memory and release all resources
    // before reallocating them to free up as much video memory as possible.
    device->EvictManagedResources();

    HRESULT result;

    // Release specific resources to free up memory for the new render target, while the
    // old render target still exists for the purpose of preserving its contents.
    if (mSwapChain)
    {
        mSwapChain->Release();
        mSwapChain = NULL;
    }

    if (mBackBuffer)
    {
        mBackBuffer->Release();
        mBackBuffer = NULL;
    }

    if (mOffscreenTexture)
    {
        mOffscreenTexture->Release();
        mOffscreenTexture = NULL;
    }

    if (mDepthStencil)
    {
        mDepthStencil->Release();
        mDepthStencil = NULL;
    }

    HANDLE *pShareHandle = NULL;
    if (!mWindow && mRenderer->getShareHandleSupport())
    {
        pShareHandle = &mShareHandle;
    }

    result = device->CreateTexture(backbufferWidth, backbufferHeight, 1, D3DUSAGE_RENDERTARGET,
                                   gl_d3d9::ConvertRenderbufferFormat(mBackBufferFormat), D3DPOOL_DEFAULT,
                                   &mOffscreenTexture, pShareHandle);
    if (FAILED(result))
    {
        ERR("Could not create offscreen texture: %08lX", result);
        release();

        if (d3d9::isDeviceLostError(result))
        {
            return EGL_CONTEXT_LOST;
        }
        else
        {
            return EGL_BAD_ALLOC;
        }
    }

    IDirect3DSurface9 *oldRenderTarget = mRenderTarget;

    result = mOffscreenTexture->GetSurfaceLevel(0, &mRenderTarget);
    ASSERT(SUCCEEDED(result));

    if (oldRenderTarget)
    {
        RECT rect =
        {
            0, 0,
            mWidth, mHeight
        };

        if (rect.right > static_cast<LONG>(backbufferWidth))
        {
            rect.right = backbufferWidth;
        }

        if (rect.bottom > static_cast<LONG>(backbufferHeight))
        {
            rect.bottom = backbufferHeight;
        }

        mRenderer->endScene();

        result = device->StretchRect(oldRenderTarget, &rect, mRenderTarget, &rect, D3DTEXF_NONE);
        ASSERT(SUCCEEDED(result));

        oldRenderTarget->Release();
    }

    if (mWindow)
    {
        D3DPRESENT_PARAMETERS presentParameters = {0};
        presentParameters.AutoDepthStencilFormat = gl_d3d9::ConvertRenderbufferFormat(mDepthBufferFormat);
        presentParameters.BackBufferCount = 1;
        presentParameters.BackBufferFormat = gl_d3d9::ConvertRenderbufferFormat(mBackBufferFormat);
        presentParameters.EnableAutoDepthStencil = FALSE;
        presentParameters.Flags = 0;
        presentParameters.hDeviceWindow = mWindow;
        presentParameters.MultiSampleQuality = 0;                  // FIXME: Unimplemented
        presentParameters.MultiSampleType = D3DMULTISAMPLE_NONE;   // FIXME: Unimplemented
        presentParameters.PresentationInterval = convertInterval(swapInterval);
        presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
        presentParameters.Windowed = TRUE;
        presentParameters.BackBufferWidth = backbufferWidth;
        presentParameters.BackBufferHeight = backbufferHeight;

        // http://crbug.com/140239
        // http://crbug.com/143434
        //
        // Some AMD/Intel switchable systems / drivers appear to round swap chain surfaces to a multiple of 64 pixels in width
        // when using the integrated Intel. This rounds the width up rather than down.
        //
        // Some non-switchable AMD GPUs / drivers do not respect the source rectangle to Present. Therefore, when the vendor ID
        // is not Intel, the back buffer width must be exactly the same width as the window or horizontal scaling will occur.
        if (mRenderer->getAdapterVendor() == VENDOR_ID_INTEL)
        {
            presentParameters.BackBufferWidth = (presentParameters.BackBufferWidth + 63) / 64 * 64;
        }

        result = device->CreateAdditionalSwapChain(&presentParameters, &mSwapChain);

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY || result == D3DERR_INVALIDCALL || result == D3DERR_DEVICELOST);

            ERR("Could not create additional swap chains or offscreen surfaces: %08lX", result);
            release();

            if (d3d9::isDeviceLostError(result))
            {
                return EGL_CONTEXT_LOST;
            }
            else
            {
                return EGL_BAD_ALLOC;
            }
        }

        result = mSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &mBackBuffer);
        ASSERT(SUCCEEDED(result));
        InvalidateRect(mWindow, NULL, FALSE);
    }

    if (mDepthBufferFormat != GL_NONE)
    {
        result = device->CreateDepthStencilSurface(backbufferWidth, backbufferHeight,
                                                   gl_d3d9::ConvertRenderbufferFormat(mDepthBufferFormat),
                                                   D3DMULTISAMPLE_NONE, 0, FALSE, &mDepthStencil, NULL);

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY || result == D3DERR_INVALIDCALL);

            ERR("Could not create depthstencil surface for new swap chain: 0x%08X", result);
            release();

            if (d3d9::isDeviceLostError(result))
            {
                return EGL_CONTEXT_LOST;
            }
            else
            {
                return EGL_BAD_ALLOC;
            }
        }
    }

    mWidth = backbufferWidth;
    mHeight = backbufferHeight;
    mSwapInterval = swapInterval;

    return EGL_SUCCESS;
}

// parameters should be validated/clamped by caller
EGLint SwapChain9::swapRect(EGLint x, EGLint y, EGLint width, EGLint height)
{
    if (!mSwapChain)
    {
        return EGL_SUCCESS;
    }

    IDirect3DDevice9 *device = mRenderer->getDevice();

    // Disable all pipeline operations
    device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
    device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);
    device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    device->SetPixelShader(NULL);
    device->SetVertexShader(NULL);

    device->SetRenderTarget(0, mBackBuffer);
    device->SetDepthStencilSurface(NULL);

    device->SetTexture(0, mOffscreenTexture);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

    for (UINT streamIndex = 0; streamIndex < gl::MAX_VERTEX_ATTRIBS; streamIndex++)
    {
        device->SetStreamSourceFreq(streamIndex, 1);
    }

    D3DVIEWPORT9 viewport = {0, 0, mWidth, mHeight, 0.0f, 1.0f};
    device->SetViewport(&viewport);

    float x1 = x - 0.5f;
    float y1 = (mHeight - y - height) - 0.5f;
    float x2 = (x + width) - 0.5f;
    float y2 = (mHeight - y) - 0.5f;

    float u1 = x / float(mWidth);
    float v1 = y / float(mHeight);
    float u2 = (x + width) / float(mWidth);
    float v2 = (y + height) / float(mHeight);

    float quad[4][6] = {{x1, y1, 0.0f, 1.0f, u1, v2},
                        {x2, y1, 0.0f, 1.0f, u2, v2},
                        {x2, y2, 0.0f, 1.0f, u2, v1},
                        {x1, y2, 0.0f, 1.0f, u1, v1}};   // x, y, z, rhw, u, v

    mRenderer->startScene();
    device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, 6 * sizeof(float));
    mRenderer->endScene();

    device->SetTexture(0, NULL);

    RECT rect =
    {
        x, mHeight - y - height,
        x + width, mHeight - y
    };

    HRESULT result = mSwapChain->Present(&rect, &rect, NULL, NULL, 0);

    mRenderer->markAllStateDirty();

    if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY || result == D3DERR_DRIVERINTERNALERROR)
    {
        return EGL_BAD_ALLOC;
    }

    // http://crbug.com/313210
    // If our swap failed, trigger a device lost event. Resetting will work around an AMD-specific
    // device removed bug with lost contexts when reinstalling drivers.
    if (FAILED(result))
    {
        mRenderer->notifyDeviceLost();
        return EGL_CONTEXT_LOST;
    }

    return EGL_SUCCESS;
}

// Increments refcount on surface.
// caller must Release() the returned surface
IDirect3DSurface9 *SwapChain9::getRenderTarget()
{
    if (mRenderTarget)
    {
        mRenderTarget->AddRef();
    }

    return mRenderTarget;
}

// Increments refcount on surface.
// caller must Release() the returned surface
IDirect3DSurface9 *SwapChain9::getDepthStencil()
{
    if (mDepthStencil)
    {
        mDepthStencil->AddRef();
    }

    return mDepthStencil;
}

// Increments refcount on texture.
// caller must Release() the returned texture
IDirect3DTexture9 *SwapChain9::getOffscreenTexture()
{
    if (mOffscreenTexture)
    {
        mOffscreenTexture->AddRef();
    }

    return mOffscreenTexture;
}

SwapChain9 *SwapChain9::makeSwapChain9(SwapChain *swapChain)
{
    ASSERT(HAS_DYNAMIC_TYPE(rx::SwapChain9*, swapChain));
    return static_cast<rx::SwapChain9*>(swapChain);
}

void SwapChain9::recreate()
{
    if (!mSwapChain)
    {
        return;
    }

    IDirect3DDevice9 *device = mRenderer->getDevice();
    if (device == NULL)
    {
        return;
    }

    D3DPRESENT_PARAMETERS presentParameters;
    HRESULT result = mSwapChain->GetPresentParameters(&presentParameters);
    ASSERT(SUCCEEDED(result));

    IDirect3DSwapChain9* newSwapChain = NULL;
    result = device->CreateAdditionalSwapChain(&presentParameters, &newSwapChain);
    if (FAILED(result))
    {
        return;
    }

    mSwapChain->Release();
    mSwapChain = newSwapChain;

    mBackBuffer->Release();
    result = mSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &mBackBuffer);
    ASSERT(SUCCEEDED(result));
}

}
