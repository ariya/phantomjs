//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer9.cpp: Implements a back-end specific class for the D3D9 renderer.

#include "libGLESv2/renderer/d3d/d3d9/Renderer9.h"
#include "libGLESv2/renderer/d3d/d3d9/renderer9_utils.h"
#include "libGLESv2/renderer/d3d/d3d9/formatutils9.h"
#include "libGLESv2/renderer/d3d/d3d9/ShaderExecutable9.h"
#include "libGLESv2/renderer/d3d/d3d9/SwapChain9.h"
#include "libGLESv2/renderer/d3d/d3d9/TextureStorage9.h"
#include "libGLESv2/renderer/d3d/d3d9/Image9.h"
#include "libGLESv2/renderer/d3d/d3d9/Blit9.h"
#include "libGLESv2/renderer/d3d/d3d9/RenderTarget9.h"
#include "libGLESv2/renderer/d3d/d3d9/VertexBuffer9.h"
#include "libGLESv2/renderer/d3d/d3d9/IndexBuffer9.h"
#include "libGLESv2/renderer/d3d/d3d9/Buffer9.h"
#include "libGLESv2/renderer/d3d/d3d9/Query9.h"
#include "libGLESv2/renderer/d3d/d3d9/Fence9.h"
#include "libGLESv2/renderer/d3d/d3d9/VertexArray9.h"
#include "libGLESv2/renderer/d3d/IndexDataManager.h"
#include "libGLESv2/renderer/d3d/ProgramD3D.h"
#include "libGLESv2/renderer/d3d/ShaderD3D.h"
#include "libGLESv2/renderer/d3d/TextureD3D.h"
#include "libGLESv2/renderer/d3d/TransformFeedbackD3D.h"
#include "libGLESv2/renderer/d3d/RenderbufferD3D.h"
#include "libGLESv2/main.h"
#include "libGLESv2/Buffer.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/FramebufferAttachment.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/State.h"
#include "libGLESv2/angletypes.h"

#include "libEGL/Display.h"

#include "common/features.h"
#include "common/utilities.h"

#include <sstream>

// Can also be enabled by defining FORCE_REF_RAST in the project's predefined macros
#define REF_RAST 0

#if !defined(ANGLE_COMPILE_OPTIMIZATION_LEVEL)
#define ANGLE_COMPILE_OPTIMIZATION_LEVEL D3DCOMPILE_OPTIMIZATION_LEVEL3
#endif

const D3DFORMAT D3DFMT_INTZ = ((D3DFORMAT)(MAKEFOURCC('I','N','T','Z')));
const D3DFORMAT D3DFMT_NULL = ((D3DFORMAT)(MAKEFOURCC('N','U','L','L')));

namespace rx
{
static const D3DFORMAT RenderTargetFormats[] =
    {
        D3DFMT_A1R5G5B5,
    //  D3DFMT_A2R10G10B10,   // The color_ramp conformance test uses ReadPixels with UNSIGNED_BYTE causing it to think that rendering skipped a colour value.
        D3DFMT_A8R8G8B8,
        D3DFMT_R5G6B5,
    //  D3DFMT_X1R5G5B5,      // Has no compatible OpenGL ES renderbuffer format
        D3DFMT_X8R8G8B8
    };

static const D3DFORMAT DepthStencilFormats[] =
    {
        D3DFMT_UNKNOWN,
    //  D3DFMT_D16_LOCKABLE,
        D3DFMT_D32,
    //  D3DFMT_D15S1,
        D3DFMT_D24S8,
        D3DFMT_D24X8,
    //  D3DFMT_D24X4S4,
        D3DFMT_D16,
    //  D3DFMT_D32F_LOCKABLE,
    //  D3DFMT_D24FS8
    };

enum
{
    MAX_VERTEX_CONSTANT_VECTORS_D3D9 = 256,
    MAX_PIXEL_CONSTANT_VECTORS_SM2 = 32,
    MAX_PIXEL_CONSTANT_VECTORS_SM3 = 224,
    MAX_VARYING_VECTORS_SM2 = 8,
    MAX_VARYING_VECTORS_SM3 = 10,

    MAX_TEXTURE_IMAGE_UNITS_VTF_SM3 = 4
};

Renderer9::Renderer9(egl::Display *display, EGLNativeDisplayType hDc, const egl::AttributeMap &attributes)
    : RendererD3D(display),
      mDc(hDc)
{
    mD3d9Module = NULL;

    mD3d9 = NULL;
    mD3d9Ex = NULL;
    mDevice = NULL;
    mDeviceEx = NULL;
    mDeviceWindow = NULL;
    mBlit = NULL;

    mAdapter = D3DADAPTER_DEFAULT;

    #if REF_RAST == 1 || defined(FORCE_REF_RAST)
        mDeviceType = D3DDEVTYPE_REF;
    #else
        mDeviceType = D3DDEVTYPE_HAL;
    #endif

    mDeviceLost = false;

    mMaskedClearSavedState = NULL;

    mVertexDataManager = NULL;
    mIndexDataManager = NULL;
    mLineLoopIB = NULL;
    mCountingIB = NULL;

    mMaxNullColorbufferLRU = 0;
    for (int i = 0; i < NUM_NULL_COLORBUFFER_CACHE_ENTRIES; i++)
    {
        mNullColorbufferCache[i].lruCount = 0;
        mNullColorbufferCache[i].width = 0;
        mNullColorbufferCache[i].height = 0;
        mNullColorbufferCache[i].buffer = NULL;
    }

    mAppliedVertexShader = NULL;
    mAppliedPixelShader = NULL;
    mAppliedProgramSerial = 0;
}

Renderer9::~Renderer9()
{
    if (mDevice)
    {
        // If the device is lost, reset it first to prevent leaving the driver in an unstable state
        if (testDeviceLost(false))
        {
            resetDevice();
        }
    }

    release();
}

void Renderer9::release()
{
    RendererD3D::cleanup();

    releaseShaderCompiler();
    releaseDeviceResources();

    SafeRelease(mDevice);
    SafeRelease(mDeviceEx);
    SafeRelease(mD3d9);
    SafeRelease(mD3d9Ex);

    mCompiler.release();

    if (mDeviceWindow)
    {
        DestroyWindow(mDeviceWindow);
        mDeviceWindow = NULL;
    }

    mD3d9Module = NULL;
}

Renderer9 *Renderer9::makeRenderer9(Renderer *renderer)
{
    ASSERT(HAS_DYNAMIC_TYPE(Renderer9*, renderer));
    return static_cast<Renderer9*>(renderer);
}

EGLint Renderer9::initialize()
{
    if (!mCompiler.initialize())
    {
        return EGL_NOT_INITIALIZED;
    }

    mD3d9Module = GetModuleHandle(TEXT("d3d9.dll"));

    if (mD3d9Module == NULL)
    {
        ERR("No D3D9 module found - aborting!\n");
        return EGL_NOT_INITIALIZED;
    }

    typedef HRESULT (WINAPI *Direct3DCreate9ExFunc)(UINT, IDirect3D9Ex**);
    Direct3DCreate9ExFunc Direct3DCreate9ExPtr = reinterpret_cast<Direct3DCreate9ExFunc>(GetProcAddress(mD3d9Module, "Direct3DCreate9Ex"));

    // Use Direct3D9Ex if available. Among other things, this version is less
    // inclined to report a lost context, for example when the user switches
    // desktop. Direct3D9Ex is available in Windows Vista and later if suitable drivers are available.
    if (ANGLE_D3D9EX == ANGLE_ENABLED && Direct3DCreate9ExPtr && SUCCEEDED(Direct3DCreate9ExPtr(D3D_SDK_VERSION, &mD3d9Ex)))
    {
        ASSERT(mD3d9Ex);
        mD3d9Ex->QueryInterface(IID_IDirect3D9, reinterpret_cast<void**>(&mD3d9));
        ASSERT(mD3d9);
    }
    else
    {
        mD3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    }

    if (!mD3d9)
    {
        ERR("Could not create D3D9 device - aborting!\n");
        return EGL_NOT_INITIALIZED;
    }

    if (mDc != NULL)
    {
    //  UNIMPLEMENTED();   // FIXME: Determine which adapter index the device context corresponds to
    }

    HRESULT result;

    // Give up on getting device caps after about one second.
    {
        for (int i = 0; i < 10; ++i)
        {
            result = mD3d9->GetDeviceCaps(mAdapter, mDeviceType, &mDeviceCaps);
            if (SUCCEEDED(result))
            {
                break;
            }
            else if (result == D3DERR_NOTAVAILABLE)
            {
                Sleep(100);   // Give the driver some time to initialize/recover
            }
            else if (FAILED(result))   // D3DERR_OUTOFVIDEOMEMORY, E_OUTOFMEMORY, D3DERR_INVALIDDEVICE, or another error we can't recover from
            {
                ERR("failed to get device caps (0x%x)\n", result);
                return EGL_NOT_INITIALIZED;
            }
        }
    }

    if (mDeviceCaps.PixelShaderVersion < D3DPS_VERSION(2, 0))
    {
        ERR("Renderer does not support PS 2.0. aborting!\n");
        return EGL_NOT_INITIALIZED;
    }

    // When DirectX9 is running with an older DirectX8 driver, a StretchRect from a regular texture to a render target texture is not supported.
    // This is required by Texture2D::ensureRenderTarget.
    if ((mDeviceCaps.DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES) == 0)
    {
        ERR("Renderer does not support stretctrect from textures!\n");
        return EGL_NOT_INITIALIZED;
    }

    {
        mD3d9->GetAdapterIdentifier(mAdapter, 0, &mAdapterIdentifier);
    }

    mMinSwapInterval = 4;
    mMaxSwapInterval = 0;

    if (mDeviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE)
    {
        mMinSwapInterval = std::min(mMinSwapInterval, 0);
        mMaxSwapInterval = std::max(mMaxSwapInterval, 0);
    }
    if (mDeviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE)
    {
        mMinSwapInterval = std::min(mMinSwapInterval, 1);
        mMaxSwapInterval = std::max(mMaxSwapInterval, 1);
    }
    if (mDeviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_TWO)
    {
        mMinSwapInterval = std::min(mMinSwapInterval, 2);
        mMaxSwapInterval = std::max(mMaxSwapInterval, 2);
    }
    if (mDeviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_THREE)
    {
        mMinSwapInterval = std::min(mMinSwapInterval, 3);
        mMaxSwapInterval = std::max(mMaxSwapInterval, 3);
    }
    if (mDeviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_FOUR)
    {
        mMinSwapInterval = std::min(mMinSwapInterval, 4);
        mMaxSwapInterval = std::max(mMaxSwapInterval, 4);
    }

    static const TCHAR windowName[] = TEXT("AngleHiddenWindow");
    static const TCHAR className[] = TEXT("STATIC");

    {
        mDeviceWindow = CreateWindowEx(WS_EX_NOACTIVATE, className, windowName, WS_DISABLED | WS_POPUP, 0, 0, 1, 1, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
    }

    D3DPRESENT_PARAMETERS presentParameters = getDefaultPresentParameters();
    DWORD behaviorFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_NOWINDOWCHANGES;

    static wchar_t *qt_d3dcreate_multihreaded_var = _wgetenv(L"QT_D3DCREATE_MULTITHREADED");
    if (qt_d3dcreate_multihreaded_var && wcsstr(qt_d3dcreate_multihreaded_var, L"1"))
        behaviorFlags |= D3DCREATE_MULTITHREADED;

    {
        result = mD3d9->CreateDevice(mAdapter, mDeviceType, mDeviceWindow, behaviorFlags | D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE, &presentParameters, &mDevice);
    }
    if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY || result == D3DERR_DEVICELOST)
    {
        return EGL_BAD_ALLOC;
    }

    if (FAILED(result))
    {
        result = mD3d9->CreateDevice(mAdapter, mDeviceType, mDeviceWindow, behaviorFlags | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParameters, &mDevice);

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY || result == D3DERR_NOTAVAILABLE || result == D3DERR_DEVICELOST);
            return EGL_BAD_ALLOC;
        }
    }

    if (mD3d9Ex)
    {
        result = mDevice->QueryInterface(IID_IDirect3DDevice9Ex, (void**)&mDeviceEx);
        ASSERT(SUCCEEDED(result));
    }

    {
        mVertexShaderCache.initialize(mDevice);
        mPixelShaderCache.initialize(mDevice);
    }

    D3DDISPLAYMODE currentDisplayMode;
    mD3d9->GetAdapterDisplayMode(mAdapter, &currentDisplayMode);

    // Check vertex texture support
    // Only Direct3D 10 ready devices support all the necessary vertex texture formats.
    // We test this using D3D9 by checking support for the R16F format.
    mVertexTextureSupport = mDeviceCaps.PixelShaderVersion >= D3DPS_VERSION(3, 0) &&
                            SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format,
                                                               D3DUSAGE_QUERY_VERTEXTEXTURE, D3DRTYPE_TEXTURE, D3DFMT_R16F));

    initializeDevice();

    return EGL_SUCCESS;
}

// do any one-time device initialization
// NOTE: this is also needed after a device lost/reset
// to reset the scene status and ensure the default states are reset.
void Renderer9::initializeDevice()
{
    // Permanent non-default states
    mDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, TRUE);
    mDevice->SetRenderState(D3DRS_LASTPIXEL, FALSE);

    if (mDeviceCaps.PixelShaderVersion >= D3DPS_VERSION(3, 0))
    {
        mDevice->SetRenderState(D3DRS_POINTSIZE_MAX, (DWORD&)mDeviceCaps.MaxPointSize);
    }
    else
    {
        mDevice->SetRenderState(D3DRS_POINTSIZE_MAX, 0x3F800000);   // 1.0f
    }

    const gl::Caps &rendererCaps = getRendererCaps();

    mForceSetVertexSamplerStates.resize(rendererCaps.maxVertexTextureImageUnits);
    mCurVertexSamplerStates.resize(rendererCaps.maxVertexTextureImageUnits);

    mForceSetPixelSamplerStates.resize(rendererCaps.maxTextureImageUnits);
    mCurPixelSamplerStates.resize(rendererCaps.maxTextureImageUnits);

    mCurVertexTextureSerials.resize(rendererCaps.maxVertexTextureImageUnits);
    mCurPixelTextureSerials.resize(rendererCaps.maxTextureImageUnits);

    markAllStateDirty();

    mSceneStarted = false;

    ASSERT(!mBlit);
    mBlit = new Blit9(this);
    mBlit->initialize();

    ASSERT(!mVertexDataManager && !mIndexDataManager);
    mVertexDataManager = new VertexDataManager(this);
    mIndexDataManager = new IndexDataManager(this);
}

D3DPRESENT_PARAMETERS Renderer9::getDefaultPresentParameters()
{
    D3DPRESENT_PARAMETERS presentParameters = {0};

    // The default swap chain is never actually used. Surface will create a new swap chain with the proper parameters.
    presentParameters.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
    presentParameters.BackBufferCount = 1;
    presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
    presentParameters.BackBufferWidth = 1;
    presentParameters.BackBufferHeight = 1;
    presentParameters.EnableAutoDepthStencil = FALSE;
    presentParameters.Flags = 0;
    presentParameters.hDeviceWindow = mDeviceWindow;
    presentParameters.MultiSampleQuality = 0;
    presentParameters.MultiSampleType = D3DMULTISAMPLE_NONE;
    presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    presentParameters.Windowed = TRUE;

    return presentParameters;
}

int Renderer9::generateConfigs(ConfigDesc **configDescList)
{
    D3DDISPLAYMODE currentDisplayMode;
    mD3d9->GetAdapterDisplayMode(mAdapter, &currentDisplayMode);

    unsigned int numRenderFormats = ArraySize(RenderTargetFormats);
    unsigned int numDepthFormats = ArraySize(DepthStencilFormats);
    (*configDescList) = new ConfigDesc[numRenderFormats * numDepthFormats];
    int numConfigs = 0;

    for (unsigned int formatIndex = 0; formatIndex < numRenderFormats; formatIndex++)
    {
        const d3d9::D3DFormat &renderTargetFormatInfo = d3d9::GetD3DFormatInfo(RenderTargetFormats[formatIndex]);
        const gl::TextureCaps &renderTargetFormatCaps = getRendererTextureCaps().get(renderTargetFormatInfo.internalFormat);
        if (renderTargetFormatCaps.renderable)
        {
            for (unsigned int depthStencilIndex = 0; depthStencilIndex < numDepthFormats; depthStencilIndex++)
            {
                const d3d9::D3DFormat &depthStencilFormatInfo = d3d9::GetD3DFormatInfo(DepthStencilFormats[depthStencilIndex]);
                const gl::TextureCaps &depthStencilFormatCaps = getRendererTextureCaps().get(depthStencilFormatInfo.internalFormat);
                if (depthStencilFormatCaps.renderable || DepthStencilFormats[depthStencilIndex] == D3DFMT_UNKNOWN)
                {
                    ConfigDesc newConfig;
                    newConfig.renderTargetFormat = renderTargetFormatInfo.internalFormat;
                    newConfig.depthStencilFormat = depthStencilFormatInfo.internalFormat;
                    newConfig.multiSample = 0; // FIXME: enumerate multi-sampling
                    newConfig.fastConfig = (currentDisplayMode.Format == RenderTargetFormats[formatIndex]);
                    newConfig.es3Capable = false;

                    (*configDescList)[numConfigs++] = newConfig;
                }
            }
        }
    }

    return numConfigs;
}

void Renderer9::deleteConfigs(ConfigDesc *configDescList)
{
    delete [] (configDescList);
}

void Renderer9::startScene()
{
    if (!mSceneStarted)
    {
        long result = mDevice->BeginScene();
        if (SUCCEEDED(result)) {
            // This is defensive checking against the device being
            // lost at unexpected times.
            mSceneStarted = true;
        }
    }
}

void Renderer9::endScene()
{
    if (mSceneStarted)
    {
        // EndScene can fail if the device was lost, for example due
        // to a TDR during a draw call.
        mDevice->EndScene();
        mSceneStarted = false;
    }
}

gl::Error Renderer9::sync(bool block)
{
    IDirect3DQuery9* query = NULL;
    gl::Error error = allocateEventQuery(&query);
    if (error.isError())
    {
        return error;
    }

    HRESULT result = query->Issue(D3DISSUE_END);
    if (FAILED(result))
    {
        ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to issue event query, result: 0x%X.", result);
    }

    // Grab the query data once in blocking and non-blocking case
    result = query->GetData(NULL, 0, D3DGETDATA_FLUSH);
    if (FAILED(result))
    {
        if (d3d9::isDeviceLostError(result))
        {
            notifyDeviceLost();
        }

        freeEventQuery(query);
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to get event query data, result: 0x%X.", result);
    }

    // If blocking, loop until the query completes
    while (block && result == S_FALSE)
    {
        // Keep polling, but allow other threads to do something useful first
        Sleep(0);

        result = query->GetData(NULL, 0, D3DGETDATA_FLUSH);

        // explicitly check for device loss
        // some drivers seem to return S_FALSE even if the device is lost
        // instead of D3DERR_DEVICELOST like they should
        if (result == S_FALSE && testDeviceLost(false))
        {
            result = D3DERR_DEVICELOST;
        }

        if (FAILED(result))
        {
            if (d3d9::isDeviceLostError(result))
            {
                notifyDeviceLost();
            }

            freeEventQuery(query);
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to get event query data, result: 0x%X.", result);
        }

    }

    freeEventQuery(query);

    return gl::Error(GL_NO_ERROR);
}

SwapChain *Renderer9::createSwapChain(NativeWindow nativeWindow, HANDLE shareHandle, GLenum backBufferFormat, GLenum depthBufferFormat)
{
    return new SwapChain9(this, nativeWindow, shareHandle, backBufferFormat, depthBufferFormat);
}

gl::Error Renderer9::allocateEventQuery(IDirect3DQuery9 **outQuery)
{
    if (mEventQueryPool.empty())
    {
        HRESULT result = mDevice->CreateQuery(D3DQUERYTYPE_EVENT, outQuery);
        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to allocate event query, result: 0x%X.", result);
        }
    }
    else
    {
        *outQuery = mEventQueryPool.back();
        mEventQueryPool.pop_back();
    }

    return gl::Error(GL_NO_ERROR);
}

void Renderer9::freeEventQuery(IDirect3DQuery9* query)
{
    if (mEventQueryPool.size() > 1000)
    {
        SafeRelease(query);
    }
    else
    {
        mEventQueryPool.push_back(query);
    }
}

gl::Error Renderer9::createVertexShader(const DWORD *function, size_t length, IDirect3DVertexShader9 **outShader)
{
    return mVertexShaderCache.create(function, length, outShader);
}

gl::Error Renderer9::createPixelShader(const DWORD *function, size_t length, IDirect3DPixelShader9 **outShader)
{
    return mPixelShaderCache.create(function, length, outShader);
}

HRESULT Renderer9::createVertexBuffer(UINT Length, DWORD Usage, IDirect3DVertexBuffer9 **ppVertexBuffer)
{
    D3DPOOL Pool = getBufferPool(Usage);
    return mDevice->CreateVertexBuffer(Length, Usage, 0, Pool, ppVertexBuffer, NULL);
}

VertexBuffer *Renderer9::createVertexBuffer()
{
    return new VertexBuffer9(this);
}

HRESULT Renderer9::createIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, IDirect3DIndexBuffer9 **ppIndexBuffer)
{
    D3DPOOL Pool = getBufferPool(Usage);
    return mDevice->CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer, NULL);
}

IndexBuffer *Renderer9::createIndexBuffer()
{
    return new IndexBuffer9(this);
}

BufferImpl *Renderer9::createBuffer()
{
    return new Buffer9(this);
}

VertexArrayImpl *Renderer9::createVertexArray()
{
    return new VertexArray9(this);
}

QueryImpl *Renderer9::createQuery(GLenum type)
{
    return new Query9(this, type);
}

FenceNVImpl *Renderer9::createFenceNV()
{
    return new FenceNV9(this);
}

FenceSyncImpl *Renderer9::createFenceSync()
{
    // Renderer9 doesn't support ES 3.0 and its sync objects.
    UNREACHABLE();
    return NULL;
}

TransformFeedbackImpl* Renderer9::createTransformFeedback()
{
    return new TransformFeedbackD3D();
}

bool Renderer9::supportsFastCopyBufferToTexture(GLenum internalFormat) const
{
    // Pixel buffer objects are not supported in D3D9, since D3D9 is ES2-only and PBOs are ES3.
    return false;
}

gl::Error Renderer9::fastCopyBufferToTexture(const gl::PixelUnpackState &unpack, unsigned int offset, RenderTarget *destRenderTarget,
                                             GLenum destinationFormat, GLenum sourcePixelsType, const gl::Box &destArea)
{
    // Pixel buffer objects are not supported in D3D9, since D3D9 is ES2-only and PBOs are ES3.
    UNREACHABLE();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error Renderer9::generateSwizzle(gl::Texture *texture)
{
    // Swizzled textures are not available in ES2 or D3D9
    UNREACHABLE();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error Renderer9::setSamplerState(gl::SamplerType type, int index, gl::Texture *texture, const gl::SamplerState &samplerState)
{
    std::vector<bool> &forceSetSamplers = (type == gl::SAMPLER_PIXEL) ? mForceSetPixelSamplerStates : mForceSetVertexSamplerStates;
    std::vector<gl::SamplerState> &appliedSamplers = (type == gl::SAMPLER_PIXEL) ? mCurPixelSamplerStates: mCurVertexSamplerStates;

    if (forceSetSamplers[index] || memcmp(&samplerState, &appliedSamplers[index], sizeof(gl::SamplerState)) != 0)
    {
        int d3dSamplerOffset = (type == gl::SAMPLER_PIXEL) ? 0 : D3DVERTEXTEXTURESAMPLER0;
        int d3dSampler = index + d3dSamplerOffset;

        // Make sure to add the level offset for our tiny compressed texture workaround
        TextureD3D *textureD3D = TextureD3D::makeTextureD3D(texture->getImplementation());
        DWORD baseLevel = samplerState.baseLevel + textureD3D->getNativeTexture()->getTopLevel();

        mDevice->SetSamplerState(d3dSampler, D3DSAMP_ADDRESSU, gl_d3d9::ConvertTextureWrap(samplerState.wrapS));
        mDevice->SetSamplerState(d3dSampler, D3DSAMP_ADDRESSV, gl_d3d9::ConvertTextureWrap(samplerState.wrapT));

        mDevice->SetSamplerState(d3dSampler, D3DSAMP_MAGFILTER, gl_d3d9::ConvertMagFilter(samplerState.magFilter, samplerState.maxAnisotropy));
        D3DTEXTUREFILTERTYPE d3dMinFilter, d3dMipFilter;
        gl_d3d9::ConvertMinFilter(samplerState.minFilter, &d3dMinFilter, &d3dMipFilter, samplerState.maxAnisotropy);
        mDevice->SetSamplerState(d3dSampler, D3DSAMP_MINFILTER, d3dMinFilter);
        mDevice->SetSamplerState(d3dSampler, D3DSAMP_MIPFILTER, d3dMipFilter);
        mDevice->SetSamplerState(d3dSampler, D3DSAMP_MAXMIPLEVEL, baseLevel);
        if (getRendererExtensions().textureFilterAnisotropic)
        {
            mDevice->SetSamplerState(d3dSampler, D3DSAMP_MAXANISOTROPY, (DWORD)samplerState.maxAnisotropy);
        }
    }

    forceSetSamplers[index] = false;
    appliedSamplers[index] = samplerState;

    return gl::Error(GL_NO_ERROR);
}

gl::Error Renderer9::setTexture(gl::SamplerType type, int index, gl::Texture *texture)
{
    int d3dSamplerOffset = (type == gl::SAMPLER_PIXEL) ? 0 : D3DVERTEXTEXTURESAMPLER0;
    int d3dSampler = index + d3dSamplerOffset;
    IDirect3DBaseTexture9 *d3dTexture = NULL;
    unsigned int serial = 0;
    bool forceSetTexture = false;

    std::vector<unsigned int> &appliedSerials = (type == gl::SAMPLER_PIXEL) ? mCurPixelTextureSerials : mCurVertexTextureSerials;

    if (texture)
    {
        TextureD3D* textureImpl = TextureD3D::makeTextureD3D(texture->getImplementation());

        TextureStorage *texStorage = textureImpl->getNativeTexture();
        if (texStorage)
        {
            TextureStorage9 *storage9 = TextureStorage9::makeTextureStorage9(texStorage);

            gl::Error error = storage9->getBaseTexture(&d3dTexture);
            if (error.isError())
            {
                return error;
            }
        }
        // If we get NULL back from getBaseTexture here, something went wrong
        // in the texture class and we're unexpectedly missing the d3d texture
        ASSERT(d3dTexture != NULL);

        serial = texture->getTextureSerial();
        forceSetTexture = textureImpl->hasDirtyImages();
        textureImpl->resetDirty();
    }

    if (forceSetTexture || appliedSerials[index] != serial)
    {
        mDevice->SetTexture(d3dSampler, d3dTexture);
    }

    appliedSerials[index] = serial;

    return gl::Error(GL_NO_ERROR);
}

gl::Error Renderer9::setUniformBuffers(const gl::Buffer* /*vertexUniformBuffers*/[], const gl::Buffer* /*fragmentUniformBuffers*/[])
{
    // No effect in ES2/D3D9
    return gl::Error(GL_NO_ERROR);
}

gl::Error Renderer9::setRasterizerState(const gl::RasterizerState &rasterState)
{
    bool rasterStateChanged = mForceSetRasterState || memcmp(&rasterState, &mCurRasterState, sizeof(gl::RasterizerState)) != 0;

    if (rasterStateChanged)
    {
        // Set the cull mode
        if (rasterState.cullFace)
        {
            mDevice->SetRenderState(D3DRS_CULLMODE, gl_d3d9::ConvertCullMode(rasterState.cullMode, rasterState.frontFace));
        }
        else
        {
            mDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        }

        if (rasterState.polygonOffsetFill)
        {
            if (mCurDepthSize > 0)
            {
                mDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&rasterState.polygonOffsetFactor);

                float depthBias = ldexp(rasterState.polygonOffsetUnits, -static_cast<int>(mCurDepthSize));
                mDevice->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&depthBias);
            }
        }
        else
        {
            mDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, 0);
            mDevice->SetRenderState(D3DRS_DEPTHBIAS, 0);
        }

        mCurRasterState = rasterState;
    }

    mForceSetRasterState = false;

    return gl::Error(GL_NO_ERROR);
}

gl::Error Renderer9::setBlendState(const gl::Framebuffer *framebuffer, const gl::BlendState &blendState, const gl::ColorF &blendColor,
                                   unsigned int sampleMask)
{
    bool blendStateChanged = mForceSetBlendState || memcmp(&blendState, &mCurBlendState, sizeof(gl::BlendState)) != 0;
    bool blendColorChanged = mForceSetBlendState || memcmp(&blendColor, &mCurBlendColor, sizeof(gl::ColorF)) != 0;
    bool sampleMaskChanged = mForceSetBlendState || sampleMask != mCurSampleMask;

    if (blendStateChanged || blendColorChanged)
    {
        if (blendState.blend)
        {
            mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

            if (blendState.sourceBlendRGB != GL_CONSTANT_ALPHA && blendState.sourceBlendRGB != GL_ONE_MINUS_CONSTANT_ALPHA &&
                blendState.destBlendRGB != GL_CONSTANT_ALPHA && blendState.destBlendRGB != GL_ONE_MINUS_CONSTANT_ALPHA)
            {
                mDevice->SetRenderState(D3DRS_BLENDFACTOR, gl_d3d9::ConvertColor(blendColor));
            }
            else
            {
                mDevice->SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_RGBA(gl::unorm<8>(blendColor.alpha),
                                                                         gl::unorm<8>(blendColor.alpha),
                                                                         gl::unorm<8>(blendColor.alpha),
                                                                         gl::unorm<8>(blendColor.alpha)));
            }

            mDevice->SetRenderState(D3DRS_SRCBLEND, gl_d3d9::ConvertBlendFunc(blendState.sourceBlendRGB));
            mDevice->SetRenderState(D3DRS_DESTBLEND, gl_d3d9::ConvertBlendFunc(blendState.destBlendRGB));
            mDevice->SetRenderState(D3DRS_BLENDOP, gl_d3d9::ConvertBlendOp(blendState.blendEquationRGB));

            if (blendState.sourceBlendRGB != blendState.sourceBlendAlpha ||
                blendState.destBlendRGB != blendState.destBlendAlpha ||
                blendState.blendEquationRGB != blendState.blendEquationAlpha)
            {
                mDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);

                mDevice->SetRenderState(D3DRS_SRCBLENDALPHA, gl_d3d9::ConvertBlendFunc(blendState.sourceBlendAlpha));
                mDevice->SetRenderState(D3DRS_DESTBLENDALPHA, gl_d3d9::ConvertBlendFunc(blendState.destBlendAlpha));
                mDevice->SetRenderState(D3DRS_BLENDOPALPHA, gl_d3d9::ConvertBlendOp(blendState.blendEquationAlpha));
            }
            else
            {
                mDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
            }
        }
        else
        {
            mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        }

        if (blendState.sampleAlphaToCoverage)
        {
            FIXME("Sample alpha to coverage is unimplemented.");
        }

        gl::FramebufferAttachment *attachment = framebuffer->getFirstColorbuffer();
        GLenum internalFormat = attachment ? attachment->getInternalFormat() : GL_NONE;

        // Set the color mask
        bool zeroColorMaskAllowed = getAdapterVendor() != VENDOR_ID_AMD;
        // Apparently some ATI cards have a bug where a draw with a zero color
        // write mask can cause later draws to have incorrect results. Instead,
        // set a nonzero color write mask but modify the blend state so that no
        // drawing is done.
        // http://code.google.com/p/angleproject/issues/detail?id=169

        const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalFormat);
        DWORD colorMask = gl_d3d9::ConvertColorMask(formatInfo.redBits   > 0 && blendState.colorMaskRed,
                                                    formatInfo.greenBits > 0 && blendState.colorMaskGreen,
                                                    formatInfo.blueBits  > 0 && blendState.colorMaskBlue,
                                                    formatInfo.alphaBits > 0 && blendState.colorMaskAlpha);
        if (colorMask == 0 && !zeroColorMaskAllowed)
        {
            // Enable green channel, but set blending so nothing will be drawn.
            mDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_GREEN);
            mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

            mDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
            mDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            mDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        }
        else
        {
            mDevice->SetRenderState(D3DRS_COLORWRITEENABLE, colorMask);
        }

        mDevice->SetRenderState(D3DRS_DITHERENABLE, blendState.dither ? TRUE : FALSE);

        mCurBlendState = blendState;
        mCurBlendColor = blendColor;
    }

    if (sampleMaskChanged)
    {
        // Set the multisample mask
        mDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
        mDevice->SetRenderState(D3DRS_MULTISAMPLEMASK, static_cast<DWORD>(sampleMask));

        mCurSampleMask = sampleMask;
    }

    mForceSetBlendState = false;

    return gl::Error(GL_NO_ERROR);
}

gl::Error Renderer9::setDepthStencilState(const gl::DepthStencilState &depthStencilState, int stencilRef,
                                          int stencilBackRef, bool frontFaceCCW)
{
    bool depthStencilStateChanged = mForceSetDepthStencilState ||
                                    memcmp(&depthStencilState, &mCurDepthStencilState, sizeof(gl::DepthStencilState)) != 0;
    bool stencilRefChanged = mForceSetDepthStencilState || stencilRef != mCurStencilRef ||
                             stencilBackRef != mCurStencilBackRef;
    bool frontFaceCCWChanged = mForceSetDepthStencilState || frontFaceCCW != mCurFrontFaceCCW;

    if (depthStencilStateChanged)
    {
        if (depthStencilState.depthTest)
        {
            mDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
            mDevice->SetRenderState(D3DRS_ZFUNC, gl_d3d9::ConvertComparison(depthStencilState.depthFunc));
        }
        else
        {
            mDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
        }

        mCurDepthStencilState = depthStencilState;
    }

    if (depthStencilStateChanged || stencilRefChanged || frontFaceCCWChanged)
    {
        if (depthStencilState.stencilTest && mCurStencilSize > 0)
        {
            mDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
            mDevice->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, TRUE);

            // FIXME: Unsupported by D3D9
            const D3DRENDERSTATETYPE D3DRS_CCW_STENCILREF = D3DRS_STENCILREF;
            const D3DRENDERSTATETYPE D3DRS_CCW_STENCILMASK = D3DRS_STENCILMASK;
            const D3DRENDERSTATETYPE D3DRS_CCW_STENCILWRITEMASK = D3DRS_STENCILWRITEMASK;

            ASSERT(depthStencilState.stencilWritemask == depthStencilState.stencilBackWritemask);
            ASSERT(stencilRef == stencilBackRef);
            ASSERT(depthStencilState.stencilMask == depthStencilState.stencilBackMask);

            // get the maximum size of the stencil ref
            unsigned int maxStencil = (1 << mCurStencilSize) - 1;

            mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILWRITEMASK : D3DRS_CCW_STENCILWRITEMASK,
                                    depthStencilState.stencilWritemask);
            mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILFUNC : D3DRS_CCW_STENCILFUNC,
                                    gl_d3d9::ConvertComparison(depthStencilState.stencilFunc));

            mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILREF : D3DRS_CCW_STENCILREF,
                                    (stencilRef < (int)maxStencil) ? stencilRef : maxStencil);
            mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILMASK : D3DRS_CCW_STENCILMASK,
                                    depthStencilState.stencilMask);

            mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILFAIL : D3DRS_CCW_STENCILFAIL,
                                    gl_d3d9::ConvertStencilOp(depthStencilState.stencilFail));
            mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILZFAIL : D3DRS_CCW_STENCILZFAIL,
                                    gl_d3d9::ConvertStencilOp(depthStencilState.stencilPassDepthFail));
            mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILPASS : D3DRS_CCW_STENCILPASS,
                                    gl_d3d9::ConvertStencilOp(depthStencilState.stencilPassDepthPass));

            mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILWRITEMASK : D3DRS_CCW_STENCILWRITEMASK,
                                    depthStencilState.stencilBackWritemask);
            mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILFUNC : D3DRS_CCW_STENCILFUNC,
                                    gl_d3d9::ConvertComparison(depthStencilState.stencilBackFunc));

            mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILREF : D3DRS_CCW_STENCILREF,
                                    (stencilBackRef < (int)maxStencil) ? stencilBackRef : maxStencil);
            mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILMASK : D3DRS_CCW_STENCILMASK,
                                    depthStencilState.stencilBackMask);

            mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILFAIL : D3DRS_CCW_STENCILFAIL,
                                    gl_d3d9::ConvertStencilOp(depthStencilState.stencilBackFail));
            mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILZFAIL : D3DRS_CCW_STENCILZFAIL,
                                    gl_d3d9::ConvertStencilOp(depthStencilState.stencilBackPassDepthFail));
            mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILPASS : D3DRS_CCW_STENCILPASS,
                                    gl_d3d9::ConvertStencilOp(depthStencilState.stencilBackPassDepthPass));
        }
        else
        {
            mDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        }

        mDevice->SetRenderState(D3DRS_ZWRITEENABLE, depthStencilState.depthMask ? TRUE : FALSE);

        mCurStencilRef = stencilRef;
        mCurStencilBackRef = stencilBackRef;
        mCurFrontFaceCCW = frontFaceCCW;
    }

    mForceSetDepthStencilState = false;

    return gl::Error(GL_NO_ERROR);
}

void Renderer9::setScissorRectangle(const gl::Rectangle &scissor, bool enabled)
{
    bool scissorChanged = mForceSetScissor ||
                          memcmp(&scissor, &mCurScissor, sizeof(gl::Rectangle)) != 0 ||
                          enabled != mScissorEnabled;

    if (scissorChanged)
    {
        if (enabled)
        {
            RECT rect;
            rect.left = gl::clamp(scissor.x, 0, static_cast<int>(mRenderTargetDesc.width));
            rect.top = gl::clamp(scissor.y, 0, static_cast<int>(mRenderTargetDesc.height));
            rect.right = gl::clamp(scissor.x + scissor.width, 0, static_cast<int>(mRenderTargetDesc.width));
            rect.bottom = gl::clamp(scissor.y + scissor.height, 0, static_cast<int>(mRenderTargetDesc.height));
            mDevice->SetScissorRect(&rect);
        }

        mDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, enabled ? TRUE : FALSE);

        mScissorEnabled = enabled;
        mCurScissor = scissor;
    }

    mForceSetScissor = false;
}

void Renderer9::setViewport(const gl::Rectangle &viewport, float zNear, float zFar, GLenum drawMode, GLenum frontFace,
                            bool ignoreViewport)
{
    gl::Rectangle actualViewport = viewport;
    float actualZNear = gl::clamp01(zNear);
    float actualZFar = gl::clamp01(zFar);
    if (ignoreViewport)
    {
        actualViewport.x = 0;
        actualViewport.y = 0;
        actualViewport.width = mRenderTargetDesc.width;
        actualViewport.height = mRenderTargetDesc.height;
        actualZNear = 0.0f;
        actualZFar = 1.0f;
    }

    D3DVIEWPORT9 dxViewport;
    dxViewport.X = gl::clamp(actualViewport.x, 0, static_cast<int>(mRenderTargetDesc.width));
    dxViewport.Y = gl::clamp(actualViewport.y, 0, static_cast<int>(mRenderTargetDesc.height));
    dxViewport.Width = gl::clamp(actualViewport.width, 0, static_cast<int>(mRenderTargetDesc.width) - static_cast<int>(dxViewport.X));
    dxViewport.Height = gl::clamp(actualViewport.height, 0, static_cast<int>(mRenderTargetDesc.height) - static_cast<int>(dxViewport.Y));
    dxViewport.MinZ = actualZNear;
    dxViewport.MaxZ = actualZFar;

    float depthFront = !gl::IsTriangleMode(drawMode) ? 0.0f : (frontFace == GL_CCW ? 1.0f : -1.0f);

    bool viewportChanged = mForceSetViewport || memcmp(&actualViewport, &mCurViewport, sizeof(gl::Rectangle)) != 0 ||
                           actualZNear != mCurNear || actualZFar != mCurFar || mCurDepthFront != depthFront;
    if (viewportChanged)
    {
        mDevice->SetViewport(&dxViewport);

        mCurViewport = actualViewport;
        mCurNear = actualZNear;
        mCurFar = actualZFar;
        mCurDepthFront = depthFront;

        dx_VertexConstants vc = {0};
        dx_PixelConstants pc = {0};

        vc.viewAdjust[0] = (float)((actualViewport.width - (int)dxViewport.Width) + 2 * (actualViewport.x - (int)dxViewport.X) - 1) / dxViewport.Width;
        vc.viewAdjust[1] = (float)((actualViewport.height - (int)dxViewport.Height) + 2 * (actualViewport.y - (int)dxViewport.Y) - 1) / dxViewport.Height;
        vc.viewAdjust[2] = (float)actualViewport.width / dxViewport.Width;
        vc.viewAdjust[3] = (float)actualViewport.height / dxViewport.Height;

        pc.viewCoords[0] = actualViewport.width  * 0.5f;
        pc.viewCoords[1] = actualViewport.height * 0.5f;
        pc.viewCoords[2] = actualViewport.x + (actualViewport.width  * 0.5f);
        pc.viewCoords[3] = actualViewport.y + (actualViewport.height * 0.5f);

        pc.depthFront[0] = (actualZFar - actualZNear) * 0.5f;
        pc.depthFront[1] = (actualZNear + actualZFar) * 0.5f;
        pc.depthFront[2] = depthFront;

        vc.depthRange[0] = actualZNear;
        vc.depthRange[1] = actualZFar;
        vc.depthRange[2] = actualZFar - actualZNear;

        pc.depthRange[0] = actualZNear;
        pc.depthRange[1] = actualZFar;
        pc.depthRange[2] = actualZFar - actualZNear;

        if (memcmp(&vc, &mVertexConstants, sizeof(dx_VertexConstants)) != 0)
        {
            mVertexConstants = vc;
            mDxUniformsDirty = true;
        }

        if (memcmp(&pc, &mPixelConstants, sizeof(dx_PixelConstants)) != 0)
        {
            mPixelConstants = pc;
            mDxUniformsDirty = true;
        }
    }

    mForceSetViewport = false;
}

bool Renderer9::applyPrimitiveType(GLenum mode, GLsizei count)
{
    switch (mode)
    {
      case GL_POINTS:
        mPrimitiveType = D3DPT_POINTLIST;
        mPrimitiveCount = count;
        break;
      case GL_LINES:
        mPrimitiveType = D3DPT_LINELIST;
        mPrimitiveCount = count / 2;
        break;
      case GL_LINE_LOOP:
        mPrimitiveType = D3DPT_LINESTRIP;
        mPrimitiveCount = count - 1;   // D3D doesn't support line loops, so we draw the last line separately
        break;
      case GL_LINE_STRIP:
        mPrimitiveType = D3DPT_LINESTRIP;
        mPrimitiveCount = count - 1;
        break;
      case GL_TRIANGLES:
        mPrimitiveType = D3DPT_TRIANGLELIST;
        mPrimitiveCount = count / 3;
        break;
      case GL_TRIANGLE_STRIP:
        mPrimitiveType = D3DPT_TRIANGLESTRIP;
        mPrimitiveCount = count - 2;
        break;
      case GL_TRIANGLE_FAN:
        mPrimitiveType = D3DPT_TRIANGLEFAN;
        mPrimitiveCount = count - 2;
        break;
      default:
        UNREACHABLE();
        return false;
    }

    return mPrimitiveCount > 0;
}


gl::Error Renderer9::getNullColorbuffer(gl::FramebufferAttachment *depthbuffer, gl::FramebufferAttachment **outColorBuffer)
{
    ASSERT(depthbuffer);

    GLsizei width  = depthbuffer->getWidth();
    GLsizei height = depthbuffer->getHeight();

    // search cached nullcolorbuffers
    for (int i = 0; i < NUM_NULL_COLORBUFFER_CACHE_ENTRIES; i++)
    {
        if (mNullColorbufferCache[i].buffer != NULL &&
            mNullColorbufferCache[i].width == width &&
            mNullColorbufferCache[i].height == height)
        {
            mNullColorbufferCache[i].lruCount = ++mMaxNullColorbufferLRU;
            *outColorBuffer = mNullColorbufferCache[i].buffer;
            return gl::Error(GL_NO_ERROR);
        }
    }

    gl::Renderbuffer *nullRenderbuffer = new gl::Renderbuffer(createRenderbuffer(), 0);
    gl::Error error = nullRenderbuffer->setStorage(width, height, GL_NONE, 0);
    if (error.isError())
    {
        SafeDelete(nullRenderbuffer);
        return error;
    }

    gl::RenderbufferAttachment *nullbuffer = new gl::RenderbufferAttachment(GL_NONE, nullRenderbuffer);

    // add nullbuffer to the cache
    NullColorbufferCacheEntry *oldest = &mNullColorbufferCache[0];
    for (int i = 1; i < NUM_NULL_COLORBUFFER_CACHE_ENTRIES; i++)
    {
        if (mNullColorbufferCache[i].lruCount < oldest->lruCount)
        {
            oldest = &mNullColorbufferCache[i];
        }
    }

    delete oldest->buffer;
    oldest->buffer = nullbuffer;
    oldest->lruCount = ++mMaxNullColorbufferLRU;
    oldest->width = width;
    oldest->height = height;

    *outColorBuffer = nullbuffer;
    return gl::Error(GL_NO_ERROR);
}

gl::Error Renderer9::applyRenderTarget(const gl::Framebuffer *framebuffer)
{
    // if there is no color attachment we must synthesize a NULL colorattachment
    // to keep the D3D runtime happy.  This should only be possible if depth texturing.
    gl::FramebufferAttachment *attachment = framebuffer->getColorbuffer(0);
    if (!attachment)
    {
        gl::Error error = getNullColorbuffer(framebuffer->getDepthbuffer(), &attachment);
        if (error.isError())
        {
            return error;
        }
    }
    ASSERT(attachment);

    bool renderTargetChanged = false;
    unsigned int renderTargetSerial = GetAttachmentSerial(attachment);
    if (renderTargetSerial != mAppliedRenderTargetSerial)
    {
        // Apply the render target on the device
        RenderTarget9 *renderTarget = NULL;
        gl::Error error = d3d9::GetAttachmentRenderTarget(attachment, &renderTarget);
        if (error.isError())
        {
            return error;
        }
        ASSERT(renderTarget);

        IDirect3DSurface9 *renderTargetSurface = renderTarget->getSurface();
        ASSERT(renderTargetSurface);

        mDevice->SetRenderTarget(0, renderTargetSurface);
        SafeRelease(renderTargetSurface);

        mAppliedRenderTargetSerial = renderTargetSerial;
        renderTargetChanged = true;
    }

    gl::FramebufferAttachment *depthStencil = framebuffer->getDepthbuffer();
    unsigned int depthbufferSerial = 0;
    unsigned int stencilbufferSerial = 0;
    if (depthStencil)
    {
        depthbufferSerial = GetAttachmentSerial(depthStencil);
    }
    else if (framebuffer->getStencilbuffer())
    {
        depthStencil = framebuffer->getStencilbuffer();
        stencilbufferSerial = GetAttachmentSerial(depthStencil);
    }

    if (depthbufferSerial != mAppliedDepthbufferSerial ||
        stencilbufferSerial != mAppliedStencilbufferSerial ||
        !mDepthStencilInitialized)
    {
        unsigned int depthSize = 0;
        unsigned int stencilSize = 0;

        // Apply the depth stencil on the device
        if (depthStencil)
        {
            RenderTarget9 *depthStencilRenderTarget = NULL;
            gl::Error error = d3d9::GetAttachmentRenderTarget(depthStencil, &depthStencilRenderTarget);
            if (error.isError())
            {
                return error;
            }
            ASSERT(depthStencilRenderTarget);

            IDirect3DSurface9 *depthStencilSurface = depthStencilRenderTarget->getSurface();
            ASSERT(depthStencilSurface);

            mDevice->SetDepthStencilSurface(depthStencilSurface);
            SafeRelease(depthStencilSurface);

            depthSize = depthStencil->getDepthSize();
            stencilSize = depthStencil->getStencilSize();
        }
        else
        {
            mDevice->SetDepthStencilSurface(NULL);
        }

        if (!mDepthStencilInitialized || depthSize != mCurDepthSize)
        {
            mCurDepthSize = depthSize;
            mForceSetRasterState = true;
        }

        if (!mDepthStencilInitialized || stencilSize != mCurStencilSize)
        {
            mCurStencilSize = stencilSize;
            mForceSetDepthStencilState = true;
        }

        mAppliedDepthbufferSerial = depthbufferSerial;
        mAppliedStencilbufferSerial = stencilbufferSerial;
        mDepthStencilInitialized = true;
    }

    if (renderTargetChanged || !mRenderTargetDescInitialized)
    {
        mForceSetScissor = true;
        mForceSetViewport = true;
        mForceSetBlendState = true;

        mRenderTargetDesc.width = attachment->getWidth();
        mRenderTargetDesc.height = attachment->getHeight();
        mRenderTargetDesc.format = attachment->getActualFormat();
        mRenderTargetDescInitialized = true;
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error Renderer9::applyVertexBuffer(const gl::State &state, GLint first, GLsizei count, GLsizei instances)
{
    TranslatedAttribute attributes[gl::MAX_VERTEX_ATTRIBS];
    gl::Error error = mVertexDataManager->prepareVertexData(state, first, count, attributes, instances);
    if (error.isError())
    {
        return error;
    }

    return mVertexDeclarationCache.applyDeclaration(mDevice, attributes, state.getCurrentProgramBinary(), instances, &mRepeatDraw);
}

// Applies the indices and element array bindings to the Direct3D 9 device
gl::Error Renderer9::applyIndexBuffer(const GLvoid *indices, gl::Buffer *elementArrayBuffer, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo)
{
    gl::Error error = mIndexDataManager->prepareIndexData(type, count, elementArrayBuffer, indices, indexInfo);
    if (error.isError())
    {
        return error;
    }

    // Directly binding the storage buffer is not supported for d3d9
    ASSERT(indexInfo->storage == NULL);

    if (indexInfo->serial != mAppliedIBSerial)
    {
        IndexBuffer9* indexBuffer = IndexBuffer9::makeIndexBuffer9(indexInfo->indexBuffer);

        mDevice->SetIndices(indexBuffer->getBuffer());
        mAppliedIBSerial = indexInfo->serial;
    }

    return gl::Error(GL_NO_ERROR);
}

void Renderer9::applyTransformFeedbackBuffers(const gl::State& state)
{
    UNREACHABLE();
}

gl::Error Renderer9::drawArrays(GLenum mode, GLsizei count, GLsizei instances, bool transformFeedbackActive)
{
    ASSERT(!transformFeedbackActive);

    startScene();

    if (mode == GL_LINE_LOOP)
    {
        return drawLineLoop(count, GL_NONE, NULL, 0, NULL);
    }
    else if (instances > 0)
    {
        StaticIndexBufferInterface *countingIB = NULL;
        gl::Error error = getCountingIB(count, &countingIB);
        if (error.isError())
        {
            return error;
        }

        if (mAppliedIBSerial != countingIB->getSerial())
        {
            IndexBuffer9 *indexBuffer = IndexBuffer9::makeIndexBuffer9(countingIB->getIndexBuffer());

            mDevice->SetIndices(indexBuffer->getBuffer());
            mAppliedIBSerial = countingIB->getSerial();
        }

        for (int i = 0; i < mRepeatDraw; i++)
        {
            mDevice->DrawIndexedPrimitive(mPrimitiveType, 0, 0, count, 0, mPrimitiveCount);
        }

        return gl::Error(GL_NO_ERROR);
    }
    else   // Regular case
    {
        mDevice->DrawPrimitive(mPrimitiveType, 0, mPrimitiveCount);
        return gl::Error(GL_NO_ERROR);
    }
}

gl::Error Renderer9::drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices,
                                  gl::Buffer *elementArrayBuffer, const TranslatedIndexData &indexInfo, GLsizei /*instances*/)
{
    startScene();

    int minIndex = static_cast<int>(indexInfo.indexRange.start);

    if (mode == GL_POINTS)
    {
        return drawIndexedPoints(count, type, indices, minIndex, elementArrayBuffer);
    }
    else if (mode == GL_LINE_LOOP)
    {
        return drawLineLoop(count, type, indices, minIndex, elementArrayBuffer);
    }
    else
    {
        for (int i = 0; i < mRepeatDraw; i++)
        {
            GLsizei vertexCount = static_cast<int>(indexInfo.indexRange.length()) + 1;
            mDevice->DrawIndexedPrimitive(mPrimitiveType, -minIndex, minIndex, vertexCount, indexInfo.startIndex, mPrimitiveCount);
        }
        return gl::Error(GL_NO_ERROR);
    }
}

gl::Error Renderer9::drawLineLoop(GLsizei count, GLenum type, const GLvoid *indices, int minIndex, gl::Buffer *elementArrayBuffer)
{
    // Get the raw indices for an indexed draw
    if (type != GL_NONE && elementArrayBuffer)
    {
        BufferD3D *storage = BufferD3D::makeFromBuffer(elementArrayBuffer);
        intptr_t offset = reinterpret_cast<intptr_t>(indices);
        const uint8_t *bufferData = NULL;
        gl::Error error = storage->getData(&bufferData);
        if (error.isError())
        {
            return error;
        }
        indices = bufferData + offset;
    }

    unsigned int startIndex = 0;

    if (getRendererExtensions().elementIndexUint)
    {
        if (!mLineLoopIB)
        {
            mLineLoopIB = new StreamingIndexBufferInterface(this);
            gl::Error error = mLineLoopIB->reserveBufferSpace(INITIAL_INDEX_BUFFER_SIZE, GL_UNSIGNED_INT);
            if (error.isError())
            {
                SafeDelete(mLineLoopIB);
                return error;
            }
        }

        // Checked by Renderer9::applyPrimitiveType
        ASSERT(count >= 0);

        if (static_cast<unsigned int>(count) + 1 > (std::numeric_limits<unsigned int>::max() / sizeof(unsigned int)))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create a 32-bit looping index buffer for GL_LINE_LOOP, too many indices required.");
        }

        const unsigned int spaceNeeded = (static_cast<unsigned int>(count)+1) * sizeof(unsigned int);
        gl::Error error = mLineLoopIB->reserveBufferSpace(spaceNeeded, GL_UNSIGNED_INT);
        if (error.isError())
        {
            return error;
        }

        void* mappedMemory = NULL;
        unsigned int offset = 0;
        error = mLineLoopIB->mapBuffer(spaceNeeded, &mappedMemory, &offset);
        if (error.isError())
        {
            return error;
        }

        startIndex = static_cast<unsigned int>(offset) / 4;
        unsigned int *data = reinterpret_cast<unsigned int*>(mappedMemory);

        switch (type)
        {
          case GL_NONE:   // Non-indexed draw
            for (int i = 0; i < count; i++)
            {
                data[i] = i;
            }
            data[count] = 0;
            break;
          case GL_UNSIGNED_BYTE:
            for (int i = 0; i < count; i++)
            {
                data[i] = static_cast<const GLubyte*>(indices)[i];
            }
            data[count] = static_cast<const GLubyte*>(indices)[0];
            break;
          case GL_UNSIGNED_SHORT:
            for (int i = 0; i < count; i++)
            {
                data[i] = static_cast<const GLushort*>(indices)[i];
            }
            data[count] = static_cast<const GLushort*>(indices)[0];
            break;
          case GL_UNSIGNED_INT:
            for (int i = 0; i < count; i++)
            {
                data[i] = static_cast<const GLuint*>(indices)[i];
            }
            data[count] = static_cast<const GLuint*>(indices)[0];
            break;
          default: UNREACHABLE();
        }

        error = mLineLoopIB->unmapBuffer();
        if (error.isError())
        {
            return error;
        }
    }
    else
    {
        if (!mLineLoopIB)
        {
            mLineLoopIB = new StreamingIndexBufferInterface(this);
            gl::Error error = mLineLoopIB->reserveBufferSpace(INITIAL_INDEX_BUFFER_SIZE, GL_UNSIGNED_SHORT);
            if (error.isError())
            {
                SafeDelete(mLineLoopIB);
                return error;
            }
        }

        // Checked by Renderer9::applyPrimitiveType
        ASSERT(count >= 0);

        if (static_cast<unsigned int>(count) + 1 > (std::numeric_limits<unsigned short>::max() / sizeof(unsigned short)))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create a 16-bit looping index buffer for GL_LINE_LOOP, too many indices required.");
        }

        const unsigned int spaceNeeded = (static_cast<unsigned int>(count) + 1) * sizeof(unsigned short);
        gl::Error error = mLineLoopIB->reserveBufferSpace(spaceNeeded, GL_UNSIGNED_SHORT);
        if (error.isError())
        {
            return error;
        }

        void* mappedMemory = NULL;
        unsigned int offset;
        error = mLineLoopIB->mapBuffer(spaceNeeded, &mappedMemory, &offset);
        if (error.isError())
        {
            return error;
        }

        startIndex = static_cast<unsigned int>(offset) / 2;
        unsigned short *data = reinterpret_cast<unsigned short*>(mappedMemory);

        switch (type)
        {
          case GL_NONE:   // Non-indexed draw
            for (int i = 0; i < count; i++)
            {
                data[i] = i;
            }
            data[count] = 0;
            break;
          case GL_UNSIGNED_BYTE:
            for (int i = 0; i < count; i++)
            {
                data[i] = static_cast<const GLubyte*>(indices)[i];
            }
            data[count] = static_cast<const GLubyte*>(indices)[0];
            break;
          case GL_UNSIGNED_SHORT:
            for (int i = 0; i < count; i++)
            {
                data[i] = static_cast<const GLushort*>(indices)[i];
            }
            data[count] = static_cast<const GLushort*>(indices)[0];
            break;
          case GL_UNSIGNED_INT:
            for (int i = 0; i < count; i++)
            {
                data[i] = static_cast<const GLuint*>(indices)[i];
            }
            data[count] = static_cast<const GLuint*>(indices)[0];
            break;
          default: UNREACHABLE();
        }

        error = mLineLoopIB->unmapBuffer();
        if (error.isError())
        {
            return error;
        }
    }

    if (mAppliedIBSerial != mLineLoopIB->getSerial())
    {
        IndexBuffer9 *indexBuffer = IndexBuffer9::makeIndexBuffer9(mLineLoopIB->getIndexBuffer());

        mDevice->SetIndices(indexBuffer->getBuffer());
        mAppliedIBSerial = mLineLoopIB->getSerial();
    }

    mDevice->DrawIndexedPrimitive(D3DPT_LINESTRIP, -minIndex, minIndex, count, startIndex, count);

    return gl::Error(GL_NO_ERROR);
}

template <typename T>
static gl::Error drawPoints(IDirect3DDevice9* device, GLsizei count, const GLvoid *indices, int minIndex)
{
    for (int i = 0; i < count; i++)
    {
        unsigned int indexValue = static_cast<unsigned int>(static_cast<const T*>(indices)[i]) - minIndex;
        device->DrawPrimitive(D3DPT_POINTLIST, indexValue, 1);
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error Renderer9::drawIndexedPoints(GLsizei count, GLenum type, const GLvoid *indices, int minIndex, gl::Buffer *elementArrayBuffer)
{
    // Drawing index point lists is unsupported in d3d9, fall back to a regular DrawPrimitive call
    // for each individual point. This call is not expected to happen often.

    if (elementArrayBuffer)
    {
        BufferD3D *storage = BufferD3D::makeFromBuffer(elementArrayBuffer);
        intptr_t offset = reinterpret_cast<intptr_t>(indices);

        const uint8_t *bufferData = NULL;
        gl::Error error = storage->getData(&bufferData);
        if (error.isError())
        {
            return error;
        }

        indices = bufferData + offset;
    }

    switch (type)
    {
        case GL_UNSIGNED_BYTE:  return drawPoints<GLubyte>(mDevice, count, indices, minIndex);
        case GL_UNSIGNED_SHORT: return drawPoints<GLushort>(mDevice, count, indices, minIndex);
        case GL_UNSIGNED_INT:   return drawPoints<GLuint>(mDevice, count, indices, minIndex);
        default: UNREACHABLE(); return gl::Error(GL_INVALID_OPERATION);
    }
}

gl::Error Renderer9::getCountingIB(size_t count, StaticIndexBufferInterface **outIB)
{
    // Update the counting index buffer if it is not large enough or has not been created yet.
    if (count <= 65536)   // 16-bit indices
    {
        const unsigned int spaceNeeded = count * sizeof(unsigned short);

        if (!mCountingIB || mCountingIB->getBufferSize() < spaceNeeded)
        {
            SafeDelete(mCountingIB);
            mCountingIB = new StaticIndexBufferInterface(this);
            mCountingIB->reserveBufferSpace(spaceNeeded, GL_UNSIGNED_SHORT);

            void *mappedMemory = NULL;
            gl::Error error = mCountingIB->mapBuffer(spaceNeeded, &mappedMemory, NULL);
            if (error.isError())
            {
                return error;
            }

            unsigned short *data = reinterpret_cast<unsigned short*>(mappedMemory);
            for (size_t i = 0; i < count; i++)
            {
                data[i] = i;
            }

            error = mCountingIB->unmapBuffer();
            if (error.isError())
            {
                return error;
            }
        }
    }
    else if (getRendererExtensions().elementIndexUint)
    {
        const unsigned int spaceNeeded = count * sizeof(unsigned int);

        if (!mCountingIB || mCountingIB->getBufferSize() < spaceNeeded)
        {
            SafeDelete(mCountingIB);
            mCountingIB = new StaticIndexBufferInterface(this);
            mCountingIB->reserveBufferSpace(spaceNeeded, GL_UNSIGNED_INT);

            void *mappedMemory = NULL;
            gl::Error error = mCountingIB->mapBuffer(spaceNeeded, &mappedMemory, NULL);
            if (error.isError())
            {
                return error;
            }

            unsigned int *data = reinterpret_cast<unsigned int*>(mappedMemory);
            for (size_t i = 0; i < count; i++)
            {
                data[i] = i;
            }

            error = mCountingIB->unmapBuffer();
            if (error.isError())
            {
                return error;
            }
        }
    }
    else
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Could not create a counting index buffer for glDrawArraysInstanced.");
    }

    *outIB = mCountingIB;
    return gl::Error(GL_NO_ERROR);
}

gl::Error Renderer9::applyShaders(gl::ProgramBinary *programBinary, const gl::VertexFormat inputLayout[], const gl::Framebuffer *framebuffer,
                                  bool rasterizerDiscard, bool transformFeedbackActive)
{
    ASSERT(!transformFeedbackActive);
    ASSERT(!rasterizerDiscard);

    ProgramD3D *programD3D = ProgramD3D::makeProgramD3D(programBinary->getImplementation());

    ShaderExecutable *vertexExe = NULL;
    gl::Error error = programD3D->getVertexExecutableForInputLayout(inputLayout, &vertexExe);
    if (error.isError())
    {
        return error;
    }

    ShaderExecutable *pixelExe = NULL;
    error = programD3D->getPixelExecutableForFramebuffer(framebuffer, &pixelExe);
    if (error.isError())
    {
        return error;
    }

    IDirect3DVertexShader9 *vertexShader = (vertexExe ? ShaderExecutable9::makeShaderExecutable9(vertexExe)->getVertexShader() : NULL);
    IDirect3DPixelShader9 *pixelShader = (pixelExe ? ShaderExecutable9::makeShaderExecutable9(pixelExe)->getPixelShader() : NULL);

    if (vertexShader != mAppliedVertexShader)
    {
        mDevice->SetVertexShader(vertexShader);
        mAppliedVertexShader = vertexShader;
    }

    if (pixelShader != mAppliedPixelShader)
    {
        mDevice->SetPixelShader(pixelShader);
        mAppliedPixelShader = pixelShader;
    }

    // D3D9 has a quirk where creating multiple shaders with the same content
    // can return the same shader pointer. Because GL programs store different data
    // per-program, checking the program serial guarantees we upload fresh
    // uniform data even if our shader pointers are the same.
    // https://code.google.com/p/angleproject/issues/detail?id=661
    unsigned int programSerial = programBinary->getSerial();
    if (programSerial != mAppliedProgramSerial)
    {
        programD3D->dirtyAllUniforms();
        mDxUniformsDirty = true;
        mAppliedProgramSerial = programSerial;
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error Renderer9::applyUniforms(const ProgramImpl &program, const std::vector<gl::LinkedUniform*> &uniformArray)
{
    for (size_t uniformIndex = 0; uniformIndex < uniformArray.size(); uniformIndex++)
    {
        gl::LinkedUniform *targetUniform = uniformArray[uniformIndex];

        if (targetUniform->dirty)
        {
            GLfloat *f = (GLfloat*)targetUniform->data;
            GLint *i = (GLint*)targetUniform->data;

            switch (targetUniform->type)
            {
              case GL_SAMPLER_2D:
              case GL_SAMPLER_CUBE:
                break;
              case GL_BOOL:
              case GL_BOOL_VEC2:
              case GL_BOOL_VEC3:
              case GL_BOOL_VEC4:
                applyUniformnbv(targetUniform, i);
                break;
              case GL_FLOAT:
              case GL_FLOAT_VEC2:
              case GL_FLOAT_VEC3:
              case GL_FLOAT_VEC4:
              case GL_FLOAT_MAT2:
              case GL_FLOAT_MAT3:
              case GL_FLOAT_MAT4:
                applyUniformnfv(targetUniform, f);
                break;
              case GL_INT:
              case GL_INT_VEC2:
              case GL_INT_VEC3:
              case GL_INT_VEC4:
                applyUniformniv(targetUniform, i);
                break;
              default:
                UNREACHABLE();
            }
        }
    }

    // Driver uniforms
    if (mDxUniformsDirty)
    {
        mDevice->SetVertexShaderConstantF(0, (float*)&mVertexConstants, sizeof(dx_VertexConstants) / sizeof(float[4]));
        mDevice->SetPixelShaderConstantF(0, (float*)&mPixelConstants, sizeof(dx_PixelConstants) / sizeof(float[4]));
        mDxUniformsDirty = false;
    }

    return gl::Error(GL_NO_ERROR);
}

void Renderer9::applyUniformnfv(gl::LinkedUniform *targetUniform, const GLfloat *v)
{
    if (targetUniform->isReferencedByFragmentShader())
    {
        mDevice->SetPixelShaderConstantF(targetUniform->psRegisterIndex, v, targetUniform->registerCount);
    }

    if (targetUniform->isReferencedByVertexShader())
    {
        mDevice->SetVertexShaderConstantF(targetUniform->vsRegisterIndex, v, targetUniform->registerCount);
    }
}

void Renderer9::applyUniformniv(gl::LinkedUniform *targetUniform, const GLint *v)
{
    ASSERT(targetUniform->registerCount <= MAX_VERTEX_CONSTANT_VECTORS_D3D9);
    GLfloat vector[MAX_VERTEX_CONSTANT_VECTORS_D3D9][4];

    for (unsigned int i = 0; i < targetUniform->registerCount; i++)
    {
        vector[i][0] = (GLfloat)v[4 * i + 0];
        vector[i][1] = (GLfloat)v[4 * i + 1];
        vector[i][2] = (GLfloat)v[4 * i + 2];
        vector[i][3] = (GLfloat)v[4 * i + 3];
    }

    applyUniformnfv(targetUniform, (GLfloat*)vector);
}

void Renderer9::applyUniformnbv(gl::LinkedUniform *targetUniform, const GLint *v)
{
    ASSERT(targetUniform->registerCount <= MAX_VERTEX_CONSTANT_VECTORS_D3D9);
    GLfloat vector[MAX_VERTEX_CONSTANT_VECTORS_D3D9][4];

    for (unsigned int i = 0; i < targetUniform->registerCount; i++)
    {
        vector[i][0] = (v[4 * i + 0] == GL_FALSE) ? 0.0f : 1.0f;
        vector[i][1] = (v[4 * i + 1] == GL_FALSE) ? 0.0f : 1.0f;
        vector[i][2] = (v[4 * i + 2] == GL_FALSE) ? 0.0f : 1.0f;
        vector[i][3] = (v[4 * i + 3] == GL_FALSE) ? 0.0f : 1.0f;
    }

    applyUniformnfv(targetUniform, (GLfloat*)vector);
}

gl::Error Renderer9::clear(const gl::ClearParameters &clearParams, const gl::Framebuffer *frameBuffer)
{
    if (clearParams.colorClearType != GL_FLOAT)
    {
        // Clearing buffers with non-float values is not supported by Renderer9 and ES 2.0
        UNREACHABLE();
        return gl::Error(GL_INVALID_OPERATION);
    }

    bool clearColor = clearParams.clearColor[0];
    for (unsigned int i = 0; i < ArraySize(clearParams.clearColor); i++)
    {
        if (clearParams.clearColor[i] != clearColor)
        {
            // Clearing individual buffers other than buffer zero is not supported by Renderer9 and ES 2.0
            UNREACHABLE();
            return gl::Error(GL_INVALID_OPERATION);
        }
    }

    float depth = gl::clamp01(clearParams.depthClearValue);
    DWORD stencil = clearParams.stencilClearValue & 0x000000FF;

    unsigned int stencilUnmasked = 0x0;
    if (clearParams.clearStencil && frameBuffer->hasStencil())
    {
        unsigned int stencilSize = gl::GetInternalFormatInfo((frameBuffer->getStencilbuffer()->getActualFormat())).stencilBits;
        stencilUnmasked = (0x1 << stencilSize) - 1;
    }

    const bool needMaskedStencilClear = clearParams.clearStencil &&
                                        (clearParams.stencilWriteMask & stencilUnmasked) != stencilUnmasked;

    bool needMaskedColorClear = false;
    D3DCOLOR color = D3DCOLOR_ARGB(255, 0, 0, 0);
    if (clearColor)
    {
        const gl::FramebufferAttachment *attachment = frameBuffer->getFirstColorbuffer();
        const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(attachment->getInternalFormat());
        const gl::InternalFormat &actualFormatInfo = gl::GetInternalFormatInfo(attachment->getActualFormat());

        color = D3DCOLOR_ARGB(gl::unorm<8>((formatInfo.alphaBits == 0 && actualFormatInfo.alphaBits > 0) ? 1.0f : clearParams.colorFClearValue.alpha),
                              gl::unorm<8>((formatInfo.redBits   == 0 && actualFormatInfo.redBits   > 0) ? 0.0f : clearParams.colorFClearValue.red),
                              gl::unorm<8>((formatInfo.greenBits == 0 && actualFormatInfo.greenBits > 0) ? 0.0f : clearParams.colorFClearValue.green),
                              gl::unorm<8>((formatInfo.blueBits  == 0 && actualFormatInfo.blueBits  > 0) ? 0.0f : clearParams.colorFClearValue.blue));

        if ((formatInfo.redBits   > 0 && !clearParams.colorMaskRed) ||
            (formatInfo.greenBits > 0 && !clearParams.colorMaskGreen) ||
            (formatInfo.blueBits  > 0 && !clearParams.colorMaskBlue) ||
            (formatInfo.alphaBits > 0 && !clearParams.colorMaskAlpha))
        {
            needMaskedColorClear = true;
        }
    }

    if (needMaskedColorClear || needMaskedStencilClear)
    {
        // State which is altered in all paths from this point to the clear call is saved.
        // State which is altered in only some paths will be flagged dirty in the case that
        //  that path is taken.
        HRESULT hr;
        if (mMaskedClearSavedState == NULL)
        {
            hr = mDevice->BeginStateBlock();
            ASSERT(SUCCEEDED(hr) || hr == D3DERR_OUTOFVIDEOMEMORY || hr == E_OUTOFMEMORY);

            mDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            mDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
            mDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
            mDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            mDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
            mDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            mDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
            mDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
            mDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
            mDevice->SetPixelShader(NULL);
            mDevice->SetVertexShader(NULL);
            mDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
            mDevice->SetStreamSource(0, NULL, 0, 0);
            mDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
            mDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
            mDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
            mDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            mDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
            mDevice->SetRenderState(D3DRS_TEXTUREFACTOR, color);
            mDevice->SetRenderState(D3DRS_MULTISAMPLEMASK, 0xFFFFFFFF);

            for(int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
            {
                mDevice->SetStreamSourceFreq(i, 1);
            }

            hr = mDevice->EndStateBlock(&mMaskedClearSavedState);
            ASSERT(SUCCEEDED(hr) || hr == D3DERR_OUTOFVIDEOMEMORY || hr == E_OUTOFMEMORY);
        }

        ASSERT(mMaskedClearSavedState != NULL);

        if (mMaskedClearSavedState != NULL)
        {
            hr = mMaskedClearSavedState->Capture();
            ASSERT(SUCCEEDED(hr));
        }

        mDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        mDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
        mDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        mDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        mDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
        mDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        mDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);

        if (clearColor)
        {
            mDevice->SetRenderState(D3DRS_COLORWRITEENABLE,
                                    gl_d3d9::ConvertColorMask(clearParams.colorMaskRed,
                                                              clearParams.colorMaskGreen,
                                                              clearParams.colorMaskBlue,
                                                              clearParams.colorMaskAlpha));
        }
        else
        {
            mDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
        }

        if (stencilUnmasked != 0x0 && clearParams.clearStencil)
        {
            mDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
            mDevice->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, FALSE);
            mDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
            mDevice->SetRenderState(D3DRS_STENCILREF, stencil);
            mDevice->SetRenderState(D3DRS_STENCILWRITEMASK, clearParams.stencilWriteMask);
            mDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_REPLACE);
            mDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_REPLACE);
            mDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
        }
        else
        {
            mDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        }

        mDevice->SetPixelShader(NULL);
        mDevice->SetVertexShader(NULL);
        mDevice->SetFVF(D3DFVF_XYZRHW);
        mDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
        mDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        mDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
        mDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        mDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
        mDevice->SetRenderState(D3DRS_TEXTUREFACTOR, color);
        mDevice->SetRenderState(D3DRS_MULTISAMPLEMASK, 0xFFFFFFFF);

        for(int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
        {
            mDevice->SetStreamSourceFreq(i, 1);
        }

        float quad[4][4];   // A quadrilateral covering the target, aligned to match the edges
        quad[0][0] = -0.5f;
        quad[0][1] = mRenderTargetDesc.height - 0.5f;
        quad[0][2] = 0.0f;
        quad[0][3] = 1.0f;

        quad[1][0] = mRenderTargetDesc.width - 0.5f;
        quad[1][1] = mRenderTargetDesc.height - 0.5f;
        quad[1][2] = 0.0f;
        quad[1][3] = 1.0f;

        quad[2][0] = -0.5f;
        quad[2][1] = -0.5f;
        quad[2][2] = 0.0f;
        quad[2][3] = 1.0f;

        quad[3][0] = mRenderTargetDesc.width - 0.5f;
        quad[3][1] = -0.5f;
        quad[3][2] = 0.0f;
        quad[3][3] = 1.0f;

        startScene();
        mDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(float[4]));

        if (clearParams.clearDepth)
        {
            mDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
            mDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
            mDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, color, depth, stencil);
        }

        if (mMaskedClearSavedState != NULL)
        {
            mMaskedClearSavedState->Apply();
        }
    }
    else if (clearColor || clearParams.clearDepth || clearParams.clearStencil)
    {
        DWORD dxClearFlags = 0;
        if (clearColor)
        {
            dxClearFlags |= D3DCLEAR_TARGET;
        }
        if (clearParams.clearDepth)
        {
            dxClearFlags |= D3DCLEAR_ZBUFFER;
        }
        if (clearParams.clearStencil)
        {
            dxClearFlags |= D3DCLEAR_STENCIL;
        }

        mDevice->Clear(0, NULL, dxClearFlags, color, depth, stencil);
    }

    return gl::Error(GL_NO_ERROR);
}

void Renderer9::markAllStateDirty()
{
    mAppliedRenderTargetSerial = 0;
    mAppliedDepthbufferSerial = 0;
    mAppliedStencilbufferSerial = 0;
    mDepthStencilInitialized = false;
    mRenderTargetDescInitialized = false;

    mForceSetDepthStencilState = true;
    mForceSetRasterState = true;
    mForceSetScissor = true;
    mForceSetViewport = true;
    mForceSetBlendState = true;

    ASSERT(mForceSetVertexSamplerStates.size() == mCurVertexTextureSerials.size());
    for (unsigned int i = 0; i < mForceSetVertexSamplerStates.size(); i++)
    {
        mForceSetVertexSamplerStates[i] = true;
        mCurVertexTextureSerials[i] = 0;
    }

    ASSERT(mForceSetPixelSamplerStates.size() == mCurPixelTextureSerials.size());
    for (unsigned int i = 0; i < mForceSetPixelSamplerStates.size(); i++)
    {
        mForceSetPixelSamplerStates[i] = true;
        mCurPixelTextureSerials[i] = 0;
    }

    mAppliedIBSerial = 0;
    mAppliedVertexShader = NULL;
    mAppliedPixelShader = NULL;
    mAppliedProgramSerial = 0;
    mDxUniformsDirty = true;

    mVertexDeclarationCache.markStateDirty();
}

void Renderer9::releaseDeviceResources()
{
    for (size_t i = 0; i < mEventQueryPool.size(); i++)
    {
        SafeRelease(mEventQueryPool[i]);
    }
    mEventQueryPool.clear();

    SafeRelease(mMaskedClearSavedState);

    mVertexShaderCache.clear();
    mPixelShaderCache.clear();

    SafeDelete(mBlit);
    SafeDelete(mVertexDataManager);
    SafeDelete(mIndexDataManager);
    SafeDelete(mLineLoopIB);
    SafeDelete(mCountingIB);

    for (int i = 0; i < NUM_NULL_COLORBUFFER_CACHE_ENTRIES; i++)
    {
        SafeDelete(mNullColorbufferCache[i].buffer);
    }
}

void Renderer9::notifyDeviceLost()
{
    mDeviceLost = true;
    mDisplay->notifyDeviceLost();
}

bool Renderer9::isDeviceLost()
{
    return mDeviceLost;
}

// set notify to true to broadcast a message to all contexts of the device loss
bool Renderer9::testDeviceLost(bool notify)
{
    HRESULT status = getDeviceStatusCode();
    bool isLost = FAILED(status);

    if (isLost)
    {
        // ensure we note the device loss --
        // we'll probably get this done again by notifyDeviceLost
        // but best to remember it!
        // Note that we don't want to clear the device loss status here
        // -- this needs to be done by resetDevice
        mDeviceLost = true;
        if (notify)
        {
            notifyDeviceLost();
        }
    }

    return isLost;
}

HRESULT Renderer9::getDeviceStatusCode()
{
    HRESULT status = D3D_OK;

    if (mDeviceEx)
    {
        status = mDeviceEx->CheckDeviceState(NULL);
    }
    else if (mDevice)
    {
        status = mDevice->TestCooperativeLevel();
    }

    return status;
}

bool Renderer9::testDeviceResettable()
{
    // On D3D9Ex, DEVICELOST represents a hung device that needs to be restarted
    // DEVICEREMOVED indicates the device has been stopped and must be recreated
    switch (getDeviceStatusCode())
    {
      case D3DERR_DEVICENOTRESET:
      case D3DERR_DEVICEHUNG:
        return true;
      case D3DERR_DEVICELOST:
        return (mDeviceEx != NULL);
      case D3DERR_DEVICEREMOVED:
        ASSERT(mDeviceEx != NULL);
        return isRemovedDeviceResettable();
      default:
        return false;
    }
}

bool Renderer9::resetDevice()
{
    releaseDeviceResources();

    D3DPRESENT_PARAMETERS presentParameters = getDefaultPresentParameters();

    HRESULT result = D3D_OK;
    bool lost = testDeviceLost(false);
    bool removedDevice = (getDeviceStatusCode() == D3DERR_DEVICEREMOVED);

    // Device Removed is a feature which is only present with D3D9Ex
    ASSERT(mDeviceEx != NULL || !removedDevice);

    for (int attempts = 3; lost && attempts > 0; attempts--)
    {
        if (removedDevice)
        {
            // Device removed, which may trigger on driver reinstallation,
            // may cause a longer wait other reset attempts before the
            // system is ready to handle creating a new device.
            Sleep(800);
            lost = !resetRemovedDevice();
        }
        else if (mDeviceEx)
        {
            Sleep(500);   // Give the graphics driver some CPU time
            result = mDeviceEx->ResetEx(&presentParameters, NULL);
            lost = testDeviceLost(false);
        }
        else
        {
            result = mDevice->TestCooperativeLevel();
            while (result == D3DERR_DEVICELOST)
            {
                Sleep(100);   // Give the graphics driver some CPU time
                result = mDevice->TestCooperativeLevel();
            }

            if (result == D3DERR_DEVICENOTRESET)
            {
                result = mDevice->Reset(&presentParameters);
            }
            lost = testDeviceLost(false);
        }
    }

    if (FAILED(result))
    {
        ERR("Reset/ResetEx failed multiple times: 0x%08X", result);
        return false;
    }

    if (removedDevice && lost)
    {
        ERR("Device lost reset failed multiple times");
        return false;
    }

    // If the device was removed, we already finished re-initialization in resetRemovedDevice
    if (!removedDevice)
    {
        // reset device defaults
        initializeDevice();
    }

    mDeviceLost = false;

    return true;
}

bool Renderer9::isRemovedDeviceResettable() const
{
    bool success = false;

#if ANGLE_D3D9EX == ANGLE_ENABLED
    IDirect3D9Ex *d3d9Ex = NULL;
    typedef HRESULT (WINAPI *Direct3DCreate9ExFunc)(UINT, IDirect3D9Ex**);
    Direct3DCreate9ExFunc Direct3DCreate9ExPtr = reinterpret_cast<Direct3DCreate9ExFunc>(GetProcAddress(mD3d9Module, "Direct3DCreate9Ex"));

    if (Direct3DCreate9ExPtr && SUCCEEDED(Direct3DCreate9ExPtr(D3D_SDK_VERSION, &d3d9Ex)))
    {
        D3DCAPS9 deviceCaps;
        HRESULT result = d3d9Ex->GetDeviceCaps(mAdapter, mDeviceType, &deviceCaps);
        success = SUCCEEDED(result);
    }

    SafeRelease(d3d9Ex);
#else
    ASSERT(UNREACHABLE());
#endif

    return success;
}

bool Renderer9::resetRemovedDevice()
{
    // From http://msdn.microsoft.com/en-us/library/windows/desktop/bb172554(v=vs.85).aspx:
    // The hardware adapter has been removed. Application must destroy the device, do enumeration of
    // adapters and create another Direct3D device. If application continues rendering without
    // calling Reset, the rendering calls will succeed. Applies to Direct3D 9Ex only.
    release();
    return (initialize() == EGL_SUCCESS);
}

DWORD Renderer9::getAdapterVendor() const
{
    return mAdapterIdentifier.VendorId;
}

std::string Renderer9::getRendererDescription() const
{
    std::ostringstream rendererString;

    rendererString << mAdapterIdentifier.Description;
    if (getShareHandleSupport())
    {
        rendererString << " Direct3D9Ex";
    }
    else
    {
        rendererString << " Direct3D9";
    }

    rendererString << " vs_" << D3DSHADER_VERSION_MAJOR(mDeviceCaps.VertexShaderVersion) << "_" << D3DSHADER_VERSION_MINOR(mDeviceCaps.VertexShaderVersion);
    rendererString << " ps_" << D3DSHADER_VERSION_MAJOR(mDeviceCaps.PixelShaderVersion) << "_" << D3DSHADER_VERSION_MINOR(mDeviceCaps.PixelShaderVersion);

    return rendererString.str();
}

GUID Renderer9::getAdapterIdentifier() const
{
    return mAdapterIdentifier.DeviceIdentifier;
}

unsigned int Renderer9::getReservedVertexUniformVectors() const
{
    return 2;   // dx_ViewAdjust and dx_DepthRange.
}

unsigned int Renderer9::getReservedFragmentUniformVectors() const
{
    return 3;   // dx_ViewCoords, dx_DepthFront and dx_DepthRange.
}

unsigned int Renderer9::getReservedVertexUniformBuffers() const
{
    return 0;
}

unsigned int Renderer9::getReservedFragmentUniformBuffers() const
{
    return 0;
}

bool Renderer9::getShareHandleSupport() const
{
    // PIX doesn't seem to support using share handles, so disable them.
    return (mD3d9Ex != NULL) && !gl::perfActive();
}

bool Renderer9::getPostSubBufferSupport() const
{
    return true;
}

int Renderer9::getMajorShaderModel() const
{
    return D3DSHADER_VERSION_MAJOR(mDeviceCaps.PixelShaderVersion);
}

DWORD Renderer9::getCapsDeclTypes() const
{
    return mDeviceCaps.DeclTypes;
}

int Renderer9::getMinSwapInterval() const
{
    return mMinSwapInterval;
}

int Renderer9::getMaxSwapInterval() const
{
    return mMaxSwapInterval;
}

D3DPOOL Renderer9::getBufferPool(DWORD usage) const
{
    if (mD3d9Ex != NULL)
    {
        return D3DPOOL_DEFAULT;
    }
    else
    {
        if (!(usage & D3DUSAGE_DYNAMIC))
        {
            return D3DPOOL_MANAGED;
        }
    }

    return D3DPOOL_DEFAULT;
}

gl::Error Renderer9::copyImage2D(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                                 GLint xoffset, GLint yoffset, TextureStorage *storage, GLint level)
{
    RECT rect;
    rect.left = sourceRect.x;
    rect.top = sourceRect.y;
    rect.right = sourceRect.x + sourceRect.width;
    rect.bottom = sourceRect.y + sourceRect.height;

    return mBlit->copy2D(framebuffer, rect, destFormat, xoffset, yoffset, storage, level);
}

gl::Error Renderer9::copyImageCube(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                                   GLint xoffset, GLint yoffset, TextureStorage *storage, GLenum target, GLint level)
{
    RECT rect;
    rect.left = sourceRect.x;
    rect.top = sourceRect.y;
    rect.right = sourceRect.x + sourceRect.width;
    rect.bottom = sourceRect.y + sourceRect.height;

    return mBlit->copyCube(framebuffer, rect, destFormat, xoffset, yoffset, storage, target, level);
}

gl::Error Renderer9::copyImage3D(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                                 GLint xoffset, GLint yoffset, GLint zOffset, TextureStorage *storage, GLint level)
{
    // 3D textures are not available in the D3D9 backend.
    UNREACHABLE();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error Renderer9::copyImage2DArray(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                                      GLint xoffset, GLint yoffset, GLint zOffset, TextureStorage *storage, GLint level)
{
    // 2D array textures are not available in the D3D9 backend.
    UNREACHABLE();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error Renderer9::blitRect(const gl::Framebuffer *readFramebuffer, const gl::Rectangle &readRect,
                              const gl::Framebuffer *drawFramebuffer, const gl::Rectangle &drawRect,
                              const gl::Rectangle *scissor, bool blitRenderTarget,
                              bool blitDepth, bool blitStencil, GLenum filter)
{
    ASSERT(filter == GL_NEAREST);

    endScene();

    if (blitRenderTarget)
    {
        gl::FramebufferAttachment *readBuffer = readFramebuffer->getColorbuffer(0);
        ASSERT(readBuffer);

        RenderTarget9 *readRenderTarget = NULL;
        gl::Error error = d3d9::GetAttachmentRenderTarget(readBuffer, &readRenderTarget);
        if (error.isError())
        {
            return error;
        }
        ASSERT(readRenderTarget);

        gl::FramebufferAttachment *drawBuffer = drawFramebuffer->getColorbuffer(0);
        ASSERT(drawBuffer);

        RenderTarget9 *drawRenderTarget = NULL;
        error = d3d9::GetAttachmentRenderTarget(drawBuffer, &drawRenderTarget);
        if (error.isError())
        {
            return error;
        }
        ASSERT(drawRenderTarget);

        // The getSurface calls do an AddRef so save them until after no errors are possible
        IDirect3DSurface9* readSurface = readRenderTarget->getSurface();
        ASSERT(readSurface);

        IDirect3DSurface9* drawSurface = drawRenderTarget->getSurface();
        ASSERT(drawSurface);

        gl::Extents srcSize(readRenderTarget->getWidth(), readRenderTarget->getHeight(), 1);
        gl::Extents dstSize(drawRenderTarget->getWidth(), drawRenderTarget->getHeight(), 1);

        RECT srcRect;
        srcRect.left = readRect.x;
        srcRect.right = readRect.x + readRect.width;
        srcRect.top = readRect.y;
        srcRect.bottom = readRect.y + readRect.height;

        RECT dstRect;
        dstRect.left = drawRect.x;
        dstRect.right = drawRect.x + drawRect.width;
        dstRect.top = drawRect.y;
        dstRect.bottom = drawRect.y + drawRect.height;

        // Clip the rectangles to the scissor rectangle
        if (scissor)
        {
            if (dstRect.left < scissor->x)
            {
                srcRect.left += (scissor->x - dstRect.left);
                dstRect.left = scissor->x;
            }
            if (dstRect.top < scissor->y)
            {
                srcRect.top += (scissor->y - dstRect.top);
                dstRect.top = scissor->y;
            }
            if (dstRect.right > scissor->x + scissor->width)
            {
                srcRect.right -= (dstRect.right - (scissor->x + scissor->width));
                dstRect.right = scissor->x + scissor->width;
            }
            if (dstRect.bottom > scissor->y + scissor->height)
            {
                srcRect.bottom -= (dstRect.bottom - (scissor->y + scissor->height));
                dstRect.bottom = scissor->y + scissor->height;
            }
        }

        // Clip the rectangles to the destination size
        if (dstRect.left < 0)
        {
            srcRect.left += -dstRect.left;
            dstRect.left = 0;
        }
        if (dstRect.right > dstSize.width)
        {
            srcRect.right -= (dstRect.right - dstSize.width);
            dstRect.right = dstSize.width;
        }
        if (dstRect.top < 0)
        {
            srcRect.top += -dstRect.top;
            dstRect.top = 0;
        }
        if (dstRect.bottom > dstSize.height)
        {
            srcRect.bottom -= (dstRect.bottom - dstSize.height);
            dstRect.bottom = dstSize.height;
        }

        // Clip the rectangles to the source size
        if (srcRect.left < 0)
        {
            dstRect.left += -srcRect.left;
            srcRect.left = 0;
        }
        if (srcRect.right > srcSize.width)
        {
            dstRect.right -= (srcRect.right - srcSize.width);
            srcRect.right = srcSize.width;
        }
        if (srcRect.top < 0)
        {
            dstRect.top += -srcRect.top;
            srcRect.top = 0;
        }
        if (srcRect.bottom > srcSize.height)
        {
            dstRect.bottom -= (srcRect.bottom - srcSize.height);
            srcRect.bottom = srcSize.height;
        }

        HRESULT result = mDevice->StretchRect(readSurface, &srcRect, drawSurface, &dstRect, D3DTEXF_NONE);

        SafeRelease(readSurface);
        SafeRelease(drawSurface);

        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Internal blit failed, StretchRect returned 0x%X.", result);
        }
    }

    if (blitDepth || blitStencil)
    {
        gl::FramebufferAttachment *readBuffer = readFramebuffer->getDepthOrStencilbuffer();
        ASSERT(readBuffer);

        RenderTarget9 *readDepthStencil = NULL;
        gl::Error error = d3d9::GetAttachmentRenderTarget(readBuffer, &readDepthStencil);
        if (error.isError())
        {
            return error;
        }
        ASSERT(readDepthStencil);

        gl::FramebufferAttachment *drawBuffer = drawFramebuffer->getDepthOrStencilbuffer();
        ASSERT(drawBuffer);

        RenderTarget9 *drawDepthStencil = NULL;
        error = d3d9::GetAttachmentRenderTarget(drawBuffer, &drawDepthStencil);
        if (error.isError())
        {
            return error;
        }
        ASSERT(drawDepthStencil);

        // The getSurface calls do an AddRef so save them until after no errors are possible
        IDirect3DSurface9* readSurface = readDepthStencil->getSurface();
        ASSERT(readDepthStencil);

        IDirect3DSurface9* drawSurface = drawDepthStencil->getSurface();
        ASSERT(drawDepthStencil);

        HRESULT result = mDevice->StretchRect(readSurface, NULL, drawSurface, NULL, D3DTEXF_NONE);

        SafeRelease(readSurface);
        SafeRelease(drawSurface);

        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Internal blit failed, StretchRect returned 0x%X.", result);
        }
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error Renderer9::readPixels(const gl::Framebuffer *framebuffer, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,
                                GLenum type, GLuint outputPitch, const gl::PixelPackState &pack, uint8_t *pixels)
{
    ASSERT(pack.pixelBuffer.get() == NULL);

    gl::FramebufferAttachment *colorbuffer = framebuffer->getColorbuffer(0);
    ASSERT(colorbuffer);

    RenderTarget9 *renderTarget = NULL;
    gl::Error error = d3d9::GetAttachmentRenderTarget(colorbuffer, &renderTarget);
    if (error.isError())
    {
        return error;
    }
    ASSERT(renderTarget);

    IDirect3DSurface9 *surface = renderTarget->getSurface();
    ASSERT(surface);

    D3DSURFACE_DESC desc;
    surface->GetDesc(&desc);

    if (desc.MultiSampleType != D3DMULTISAMPLE_NONE)
    {
        UNIMPLEMENTED();   // FIXME: Requires resolve using StretchRect into non-multisampled render target
        SafeRelease(surface);
        return gl::Error(GL_OUT_OF_MEMORY, "ReadPixels is unimplemented for multisampled framebuffer attachments.");
    }

    HRESULT result;
    IDirect3DSurface9 *systemSurface = NULL;
    bool directToPixels = !pack.reverseRowOrder && pack.alignment <= 4 && getShareHandleSupport() &&
                          x == 0 && y == 0 && UINT(width) == desc.Width && UINT(height) == desc.Height &&
                          desc.Format == D3DFMT_A8R8G8B8 && format == GL_BGRA_EXT && type == GL_UNSIGNED_BYTE;
    if (directToPixels)
    {
        // Use the pixels ptr as a shared handle to write directly into client's memory
        result = mDevice->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format,
                                                      D3DPOOL_SYSTEMMEM, &systemSurface, reinterpret_cast<void**>(&pixels));
        if (FAILED(result))
        {
            // Try again without the shared handle
            directToPixels = false;
        }
    }

    if (!directToPixels)
    {
        result = mDevice->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format,
                                                      D3DPOOL_SYSTEMMEM, &systemSurface, NULL);
        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            SafeRelease(surface);
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to allocate internal texture for ReadPixels.");
        }
    }

    result = mDevice->GetRenderTargetData(surface, systemSurface);
    SafeRelease(surface);

    if (FAILED(result))
    {
        SafeRelease(systemSurface);

        // It turns out that D3D will sometimes produce more error
        // codes than those documented.
        if (d3d9::isDeviceLostError(result))
        {
            notifyDeviceLost();
        }
        else
        {
            UNREACHABLE();
        }

        return gl::Error(GL_OUT_OF_MEMORY, "Failed to read internal render target data.");
    }

    if (directToPixels)
    {
        SafeRelease(systemSurface);
        return gl::Error(GL_NO_ERROR);
    }

    RECT rect;
    rect.left = gl::clamp(x, 0L, static_cast<LONG>(desc.Width));
    rect.top = gl::clamp(y, 0L, static_cast<LONG>(desc.Height));
    rect.right = gl::clamp(x + width, 0L, static_cast<LONG>(desc.Width));
    rect.bottom = gl::clamp(y + height, 0L, static_cast<LONG>(desc.Height));

    D3DLOCKED_RECT lock;
    result = systemSurface->LockRect(&lock, &rect, D3DLOCK_READONLY);

    if (FAILED(result))
    {
        UNREACHABLE();
        SafeRelease(systemSurface);

        return gl::Error(GL_OUT_OF_MEMORY, "Failed to lock internal render target.");
    }

    uint8_t *source;
    int inputPitch;
    if (pack.reverseRowOrder)
    {
        source = reinterpret_cast<uint8_t*>(lock.pBits) + lock.Pitch * (rect.bottom - rect.top - 1);
        inputPitch = -lock.Pitch;
    }
    else
    {
        source = reinterpret_cast<uint8_t*>(lock.pBits);
        inputPitch = lock.Pitch;
    }

    const d3d9::D3DFormat &d3dFormatInfo = d3d9::GetD3DFormatInfo(desc.Format);
    const gl::InternalFormat &sourceFormatInfo = gl::GetInternalFormatInfo(d3dFormatInfo.internalFormat);
    if (sourceFormatInfo.format == format && sourceFormatInfo.type == type)
    {
        // Direct copy possible
        for (int y = 0; y < rect.bottom - rect.top; y++)
        {
            memcpy(pixels + y * outputPitch, source + y * inputPitch, (rect.right - rect.left) * sourceFormatInfo.pixelBytes);
        }
    }
    else
    {
        const d3d9::D3DFormat &sourceD3DFormatInfo = d3d9::GetD3DFormatInfo(desc.Format);
        ColorCopyFunction fastCopyFunc = sourceD3DFormatInfo.getFastCopyFunction(format, type);

        const gl::FormatType &destFormatTypeInfo = gl::GetFormatTypeInfo(format, type);
        const gl::InternalFormat &destFormatInfo = gl::GetInternalFormatInfo(destFormatTypeInfo.internalFormat);

        if (fastCopyFunc)
        {
            // Fast copy is possible through some special function
            for (int y = 0; y < rect.bottom - rect.top; y++)
            {
                for (int x = 0; x < rect.right - rect.left; x++)
                {
                    uint8_t *dest = pixels + y * outputPitch + x * destFormatInfo.pixelBytes;
                    const uint8_t *src = source + y * inputPitch + x * sourceFormatInfo.pixelBytes;

                    fastCopyFunc(src, dest);
                }
            }
        }
        else
        {
            uint8_t temp[sizeof(gl::ColorF)];
            for (int y = 0; y < rect.bottom - rect.top; y++)
            {
                for (int x = 0; x < rect.right - rect.left; x++)
                {
                    uint8_t *dest = pixels + y * outputPitch + x * destFormatInfo.pixelBytes;
                    const uint8_t *src = source + y * inputPitch + x * sourceFormatInfo.pixelBytes;

                    // readFunc and writeFunc will be using the same type of color, CopyTexImage
                    // will not allow the copy otherwise.
                    sourceD3DFormatInfo.colorReadFunction(src, temp);
                    destFormatTypeInfo.colorWriteFunction(temp, dest);
                }
            }
        }
    }

    systemSurface->UnlockRect();
    SafeRelease(systemSurface);

    return gl::Error(GL_NO_ERROR);
}

gl::Error Renderer9::createRenderTarget(SwapChain *swapChain, bool depth, RenderTarget **outRT)
{
    SwapChain9 *swapChain9 = SwapChain9::makeSwapChain9(swapChain);
    *outRT = new SurfaceRenderTarget9(swapChain9, depth);
    return gl::Error(GL_NO_ERROR);
}

gl::Error Renderer9::createRenderTarget(int width, int height, GLenum format, GLsizei samples, RenderTarget **outRT)
{
    const d3d9::TextureFormat &d3d9FormatInfo = d3d9::GetTextureFormatInfo(format);

    const gl::TextureCaps &textureCaps = getRendererTextureCaps().get(format);
    GLuint supportedSamples = textureCaps.getNearestSamples(samples);

    IDirect3DSurface9 *renderTarget = NULL;
    if (width > 0 && height > 0)
    {
        bool requiresInitialization = false;
        HRESULT result = D3DERR_INVALIDCALL;

        const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(format);
        if (formatInfo.depthBits > 0 || formatInfo.stencilBits > 0)
        {
            result = mDevice->CreateDepthStencilSurface(width, height, d3d9FormatInfo.renderFormat,
                                                        gl_d3d9::GetMultisampleType(supportedSamples),
                                                        0, FALSE, &renderTarget, NULL);
        }
        else
        {
            requiresInitialization = (d3d9FormatInfo.dataInitializerFunction != NULL);
            result = mDevice->CreateRenderTarget(width, height, d3d9FormatInfo.renderFormat,
                                                 gl_d3d9::GetMultisampleType(supportedSamples),
                                                 0, FALSE, &renderTarget, NULL);
        }

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create render target, result: 0x%X.", result);
        }

        if (requiresInitialization)
        {
            // This format requires that the data be initialized before the render target can be used
            // Unfortunately this requires a Get call on the d3d device but it is far better than having
            // to mark the render target as lockable and copy data to the gpu.
            IDirect3DSurface9 *prevRenderTarget = NULL;
            mDevice->GetRenderTarget(0, &prevRenderTarget);
            mDevice->SetRenderTarget(0, renderTarget);
            mDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 255), 0.0f, 0);
            mDevice->SetRenderTarget(0, prevRenderTarget);
        }
    }

    *outRT = new TextureRenderTarget9(renderTarget, format, width, height, 1, supportedSamples);
    return gl::Error(GL_NO_ERROR);
}

ShaderImpl *Renderer9::createShader(const gl::Data &data, GLenum type)
{
    return new ShaderD3D(data, type, this);
}

ProgramImpl *Renderer9::createProgram()
{
    return new ProgramD3D(this);
}

void Renderer9::releaseShaderCompiler()
{
    ShaderD3D::releaseCompiler();
}

gl::Error Renderer9::loadExecutable(const void *function, size_t length, ShaderType type,
                                    const std::vector<gl::LinkedVarying> &transformFeedbackVaryings,
                                    bool separatedOutputBuffers, ShaderExecutable **outExecutable)
{
    // Transform feedback is not supported in ES2 or D3D9
    ASSERT(transformFeedbackVaryings.size() == 0);

    switch (type)
    {
      case SHADER_VERTEX:
        {
            IDirect3DVertexShader9 *vshader = NULL;
            gl::Error error = createVertexShader((DWORD*)function, length, &vshader);
            if (error.isError())
            {
                return error;
            }
            *outExecutable = new ShaderExecutable9(function, length, vshader);
        }
        break;
      case SHADER_PIXEL:
        {
            IDirect3DPixelShader9 *pshader = NULL;
            gl::Error error = createPixelShader((DWORD*)function, length, &pshader);
            if (error.isError())
            {
                return error;
            }
            *outExecutable = new ShaderExecutable9(function, length, pshader);
        }
        break;
      default:
        UNREACHABLE();
        return gl::Error(GL_INVALID_OPERATION);
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error Renderer9::compileToExecutable(gl::InfoLog &infoLog, const std::string &shaderHLSL, ShaderType type,
                                         const std::vector<gl::LinkedVarying> &transformFeedbackVaryings,
                                         bool separatedOutputBuffers, D3DWorkaroundType workaround,
                                         ShaderExecutable **outExectuable)
{
    // Transform feedback is not supported in ES2 or D3D9
    ASSERT(transformFeedbackVaryings.size() == 0);

    const char *profileType = NULL;
    switch (type)
    {
      case SHADER_VERTEX:
        profileType = "vs";
        break;
      case SHADER_PIXEL:
        profileType = "ps";
        break;
      default:
        UNREACHABLE();
        return gl::Error(GL_INVALID_OPERATION);
    }
    unsigned int profileMajorVersion = (getMajorShaderModel() >= 3) ? 3 : 2;
    unsigned int profileMinorVersion = 0;
    std::string profile = FormatString("%s_%u_%u", profileType, profileMajorVersion, profileMinorVersion);

    UINT flags = ANGLE_COMPILE_OPTIMIZATION_LEVEL;

    if (workaround == ANGLE_D3D_WORKAROUND_SKIP_OPTIMIZATION)
    {
        flags = D3DCOMPILE_SKIP_OPTIMIZATION;
    }
    else if (workaround == ANGLE_D3D_WORKAROUND_MAX_OPTIMIZATION)
    {
        flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
    }
    else ASSERT(workaround == ANGLE_D3D_WORKAROUND_NONE);

    if (gl::perfActive())
    {
#ifndef NDEBUG
        flags = D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        flags |= D3DCOMPILE_DEBUG;
    }

    // Sometimes D3DCompile will fail with the default compilation flags for complicated shaders when it would otherwise pass with alternative options.
    // Try the default flags first and if compilation fails, try some alternatives.
    std::vector<CompileConfig> configs;
    configs.push_back(CompileConfig(flags,                                  "default"            ));
    configs.push_back(CompileConfig(flags | D3DCOMPILE_AVOID_FLOW_CONTROL,  "avoid flow control" ));
    configs.push_back(CompileConfig(flags | D3DCOMPILE_PREFER_FLOW_CONTROL, "prefer flow control"));

    ID3DBlob *binary = NULL;
    std::string debugInfo;
    gl::Error error = mCompiler.compileToBinary(infoLog, shaderHLSL, profile, configs, NULL, &binary, &debugInfo);
    if (error.isError())
    {
        return error;
    }

    // It's possible that binary is NULL if the compiler failed in all configurations.  Set the executable to NULL
    // and return GL_NO_ERROR to signify that there was a link error but the internal state is still OK.
    if (!binary)
    {
        *outExectuable = NULL;
        return gl::Error(GL_NO_ERROR);
    }

    error = loadExecutable(binary->GetBufferPointer(), binary->GetBufferSize(), type,
                           transformFeedbackVaryings, separatedOutputBuffers, outExectuable);

    SafeRelease(binary);
    if (error.isError())
    {
        return error;
    }

    if (!debugInfo.empty())
    {
        (*outExectuable)->appendDebugInfo(debugInfo);
    }

    return gl::Error(GL_NO_ERROR);
}

UniformStorage *Renderer9::createUniformStorage(size_t storageSize)
{
    return new UniformStorage(storageSize);
}

gl::Error Renderer9::boxFilter(IDirect3DSurface9 *source, IDirect3DSurface9 *dest)
{
    return mBlit->boxFilter(source, dest);
}

D3DPOOL Renderer9::getTexturePool(DWORD usage) const
{
    if (mD3d9Ex != NULL)
    {
        return D3DPOOL_DEFAULT;
    }
    else
    {
        if (!(usage & (D3DUSAGE_DEPTHSTENCIL | D3DUSAGE_RENDERTARGET)))
        {
            return D3DPOOL_MANAGED;
        }
    }

    return D3DPOOL_DEFAULT;
}

gl::Error Renderer9::copyToRenderTarget(IDirect3DSurface9 *dest, IDirect3DSurface9 *source, bool fromManaged)
{
    ASSERT(source && dest);

    HRESULT result = D3DERR_OUTOFVIDEOMEMORY;

    if (fromManaged)
    {
        D3DSURFACE_DESC desc;
        source->GetDesc(&desc);

        IDirect3DSurface9 *surf = 0;
        result = mDevice->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &surf, NULL);

        if (SUCCEEDED(result))
        {
            Image9::copyLockableSurfaces(surf, source);
            result = mDevice->UpdateSurface(surf, NULL, dest, NULL);
            SafeRelease(surf);
        }
    }
    else
    {
        endScene();
        result = mDevice->StretchRect(source, NULL, dest, NULL, D3DTEXF_NONE);
    }

    if (FAILED(result))
    {
        ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to blit internal texture, result: 0x%X.", result);
    }

    return gl::Error(GL_NO_ERROR);
}

Image *Renderer9::createImage()
{
    return new Image9();
}

gl::Error Renderer9::generateMipmap(Image *dest, Image *src)
{
    Image9 *src9 = Image9::makeImage9(src);
    Image9 *dst9 = Image9::makeImage9(dest);
    return Image9::generateMipmap(dst9, src9);
}

TextureStorage *Renderer9::createTextureStorage2D(SwapChain *swapChain)
{
    SwapChain9 *swapChain9 = SwapChain9::makeSwapChain9(swapChain);
    return new TextureStorage9_2D(this, swapChain9);
}

TextureStorage *Renderer9::createTextureStorage2D(GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, int levels)
{
    return new TextureStorage9_2D(this, internalformat, renderTarget, width, height, levels);
}

TextureStorage *Renderer9::createTextureStorageCube(GLenum internalformat, bool renderTarget, int size, int levels)
{
    return new TextureStorage9_Cube(this, internalformat, renderTarget, size, levels);
}

TextureStorage *Renderer9::createTextureStorage3D(GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, GLsizei depth, int levels)
{
    // 3D textures are not supported by the D3D9 backend.
    UNREACHABLE();

    return NULL;
}

TextureStorage *Renderer9::createTextureStorage2DArray(GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, GLsizei depth, int levels)
{
    // 2D array textures are not supported by the D3D9 backend.
    UNREACHABLE();

    return NULL;
}

TextureImpl *Renderer9::createTexture(GLenum target)
{
    switch(target)
    {
      case GL_TEXTURE_2D:       return new TextureD3D_2D(this);
      case GL_TEXTURE_CUBE_MAP: return new TextureD3D_Cube(this);
      default:                  UNREACHABLE();
    }

    return NULL;
}

RenderbufferImpl *Renderer9::createRenderbuffer()
{
    RenderbufferD3D *renderbuffer = new RenderbufferD3D(this);
    return renderbuffer;
}

RenderbufferImpl *Renderer9::createRenderbuffer(SwapChain *swapChain, bool depth)
{
    RenderbufferD3D *renderbuffer = new RenderbufferD3D(this);
    renderbuffer->setStorage(swapChain, depth);
    return renderbuffer;
}

bool Renderer9::getLUID(LUID *adapterLuid) const
{
    adapterLuid->HighPart = 0;
    adapterLuid->LowPart = 0;

    if (mD3d9Ex)
    {
        mD3d9Ex->GetAdapterLUID(mAdapter, adapterLuid);
        return true;
    }

    return false;
}

VertexConversionType Renderer9::getVertexConversionType(const gl::VertexFormat &vertexFormat) const
{
    return d3d9::GetVertexFormatInfo(getCapsDeclTypes(), vertexFormat).conversionType;
}

GLenum Renderer9::getVertexComponentType(const gl::VertexFormat &vertexFormat) const
{
    return d3d9::GetVertexFormatInfo(getCapsDeclTypes(), vertexFormat).componentType;
}

void Renderer9::generateCaps(gl::Caps *outCaps, gl::TextureCapsMap *outTextureCaps, gl::Extensions *outExtensions) const
{
    d3d9_gl::GenerateCaps(mD3d9, mDevice, mDeviceType, mAdapter, outCaps, outTextureCaps, outExtensions);
}

Workarounds Renderer9::generateWorkarounds() const
{
    return d3d9::GenerateWorkarounds();
}

}
