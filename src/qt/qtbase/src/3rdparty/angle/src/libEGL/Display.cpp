//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Display.cpp: Implements the egl::Display class, representing the abstract
// display on which graphics are drawn. Implements EGLDisplay.
// [EGL 1.4] section 2.1.2 page 3.

#include "libEGL/Display.h"

#include <algorithm>
#include <map>
#include <vector>
#include <sstream>
#include <iterator>

#include "common/debug.h"
#include "common/mathutil.h"
#include "libGLESv2/main.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/renderer/SwapChain.h"

#include "libEGL/main.h"
#include "libEGL/Surface.h"

namespace egl
{

typedef std::map<EGLNativeDisplayType, Display*> DisplayMap;
static DisplayMap *GetDisplayMap()
{
    static DisplayMap displays;
    return &displays;
}

egl::Display *Display::getDisplay(EGLNativeDisplayType displayId, const AttributeMap &attribMap)
{
    Display *display = NULL;

    DisplayMap *displays = GetDisplayMap();
    DisplayMap::const_iterator iter = displays->find(displayId);
    if (iter != displays->end())
    {
        display = iter->second;
    }
    else
    {
        display = new egl::Display(displayId);
        displays->insert(std::make_pair(displayId, display));
    }

    // Apply new attributes if the display is not initialized yet.
    if (!display->isInitialized())
    {
        display->setAttributes(attribMap);
    }

    return display;
}

Display::Display(EGLNativeDisplayType displayId)
    : mDisplayId(displayId),
      mAttributeMap(),
      mRenderer(NULL)
{
}

Display::~Display()
{
    terminate();

    DisplayMap *displays = GetDisplayMap();
    DisplayMap::iterator iter = displays->find(mDisplayId);
    if (iter != displays->end())
    {
        displays->erase(iter);
    }
}

void Display::setAttributes(const AttributeMap &attribMap)
{
    mAttributeMap = attribMap;
}

Error Display::initialize()
{
    if (isInitialized())
    {
        return Error(EGL_SUCCESS);
    }

    mRenderer = glCreateRenderer(this, mDisplayId, mAttributeMap);

    if (!mRenderer)
    {
        terminate();
        return Error(EGL_NOT_INITIALIZED);
    }

    //TODO(jmadill): should be part of caps?
    EGLint minSwapInterval = mRenderer->getMinSwapInterval();
    EGLint maxSwapInterval = mRenderer->getMaxSwapInterval();
    EGLint maxTextureSize = mRenderer->getRendererCaps().max2DTextureSize;

    rx::ConfigDesc *descList;
    int numConfigs = mRenderer->generateConfigs(&descList);
    ConfigSet configSet;

    for (int i = 0; i < numConfigs; ++i)
    {
        configSet.add(descList[i], minSwapInterval, maxSwapInterval, maxTextureSize, maxTextureSize);
    }

    // Give the sorted configs a unique ID and store them internally
    EGLint index = 1;
    for (ConfigSet::Iterator config = configSet.mSet.begin(); config != configSet.mSet.end(); config++)
    {
        Config configuration = *config;
        configuration.mConfigID = index;
        index++;

        mConfigSet.mSet.insert(configuration);
    }

    mRenderer->deleteConfigs(descList);
    descList = NULL;

    if (!isInitialized())
    {
        terminate();
        return Error(EGL_NOT_INITIALIZED);
    }

    initDisplayExtensionString();
    initVendorString();

    return Error(EGL_SUCCESS);
}

void Display::terminate()
{
    while (!mSurfaceSet.empty())
    {
        destroySurface(*mSurfaceSet.begin());
    }

    while (!mContextSet.empty())
    {
        destroyContext(*mContextSet.begin());
    }

    glDestroyRenderer(mRenderer);
    mRenderer = NULL;

    mConfigSet.mSet.clear();
}

bool Display::getConfigs(EGLConfig *configs, const EGLint *attribList, EGLint configSize, EGLint *numConfig)
{
    return mConfigSet.getConfigs(configs, attribList, configSize, numConfig);
}

bool Display::getConfigAttrib(EGLConfig config, EGLint attribute, EGLint *value)
{
    const egl::Config *configuration = mConfigSet.get(config);

    switch (attribute)
    {
      case EGL_BUFFER_SIZE:               *value = configuration->mBufferSize;             break;
      case EGL_ALPHA_SIZE:                *value = configuration->mAlphaSize;              break;
      case EGL_BLUE_SIZE:                 *value = configuration->mBlueSize;               break;
      case EGL_GREEN_SIZE:                *value = configuration->mGreenSize;              break;
      case EGL_RED_SIZE:                  *value = configuration->mRedSize;                break;
      case EGL_DEPTH_SIZE:                *value = configuration->mDepthSize;              break;
      case EGL_STENCIL_SIZE:              *value = configuration->mStencilSize;            break;
      case EGL_CONFIG_CAVEAT:             *value = configuration->mConfigCaveat;           break;
      case EGL_CONFIG_ID:                 *value = configuration->mConfigID;               break;
      case EGL_LEVEL:                     *value = configuration->mLevel;                  break;
      case EGL_NATIVE_RENDERABLE:         *value = configuration->mNativeRenderable;       break;
      case EGL_NATIVE_VISUAL_TYPE:        *value = configuration->mNativeVisualType;       break;
      case EGL_SAMPLES:                   *value = configuration->mSamples;                break;
      case EGL_SAMPLE_BUFFERS:            *value = configuration->mSampleBuffers;          break;
      case EGL_SURFACE_TYPE:              *value = configuration->mSurfaceType;            break;
      case EGL_TRANSPARENT_TYPE:          *value = configuration->mTransparentType;        break;
      case EGL_TRANSPARENT_BLUE_VALUE:    *value = configuration->mTransparentBlueValue;   break;
      case EGL_TRANSPARENT_GREEN_VALUE:   *value = configuration->mTransparentGreenValue;  break;
      case EGL_TRANSPARENT_RED_VALUE:     *value = configuration->mTransparentRedValue;    break;
      case EGL_BIND_TO_TEXTURE_RGB:       *value = configuration->mBindToTextureRGB;       break;
      case EGL_BIND_TO_TEXTURE_RGBA:      *value = configuration->mBindToTextureRGBA;      break;
      case EGL_MIN_SWAP_INTERVAL:         *value = configuration->mMinSwapInterval;        break;
      case EGL_MAX_SWAP_INTERVAL:         *value = configuration->mMaxSwapInterval;        break;
      case EGL_LUMINANCE_SIZE:            *value = configuration->mLuminanceSize;          break;
      case EGL_ALPHA_MASK_SIZE:           *value = configuration->mAlphaMaskSize;          break;
      case EGL_COLOR_BUFFER_TYPE:         *value = configuration->mColorBufferType;        break;
      case EGL_RENDERABLE_TYPE:           *value = configuration->mRenderableType;         break;
      case EGL_MATCH_NATIVE_PIXMAP:       *value = false; UNIMPLEMENTED();                 break;
      case EGL_CONFORMANT:                *value = configuration->mConformant;             break;
      case EGL_MAX_PBUFFER_WIDTH:         *value = configuration->mMaxPBufferWidth;        break;
      case EGL_MAX_PBUFFER_HEIGHT:        *value = configuration->mMaxPBufferHeight;       break;
      case EGL_MAX_PBUFFER_PIXELS:        *value = configuration->mMaxPBufferPixels;       break;
      default:
        return false;
    }

    return true;
}



Error Display::createWindowSurface(EGLNativeWindowType window, EGLConfig config, const EGLint *attribList, EGLSurface *outSurface)
{
    const Config *configuration = mConfigSet.get(config);
    EGLint postSubBufferSupported = EGL_FALSE;

    EGLint width = 0;
    EGLint height = 0;
    EGLint fixedSize = EGL_FALSE;

    if (attribList)
    {
        while (*attribList != EGL_NONE)
        {
            switch (attribList[0])
            {
              case EGL_RENDER_BUFFER:
                switch (attribList[1])
                {
                  case EGL_BACK_BUFFER:
                    break;
                  case EGL_SINGLE_BUFFER:
                    return Error(EGL_BAD_MATCH);   // Rendering directly to front buffer not supported
                  default:
                    return Error(EGL_BAD_ATTRIBUTE);
                }
                break;
              case EGL_POST_SUB_BUFFER_SUPPORTED_NV:
                postSubBufferSupported = attribList[1];
                break;
              case EGL_WIDTH:
                width = attribList[1];
                break;
              case EGL_HEIGHT:
                height = attribList[1];
                break;
              case EGL_FIXED_SIZE_ANGLE:
                fixedSize = attribList[1];
                break;
              case EGL_VG_COLORSPACE:
                return Error(EGL_BAD_MATCH);
              case EGL_VG_ALPHA_FORMAT:
                return Error(EGL_BAD_MATCH);
              default:
                return Error(EGL_BAD_ATTRIBUTE);
            }

            attribList += 2;
        }
    }

    if (width < 0 || height < 0)
    {
        return Error(EGL_BAD_PARAMETER);
    }

    if (!fixedSize)
    {
        width = -1;
        height = -1;
    }

    if (hasExistingWindowSurface(window))
    {
        return Error(EGL_BAD_ALLOC);
    }

    if (mRenderer->testDeviceLost(false))
    {
        Error error = restoreLostDevice();
        if (error.isError())
        {
            return error;
        }
    }

    Surface *surface = new Surface(this, configuration, window, fixedSize, width, height, postSubBufferSupported);
    Error error = surface->initialize();
    if (error.isError())
    {
        SafeDelete(surface);
        return error;
    }

    mSurfaceSet.insert(surface);

    *outSurface = surface;
    return Error(EGL_SUCCESS);
}

Error Display::createOffscreenSurface(EGLConfig config, HANDLE shareHandle, const EGLint *attribList, EGLSurface *outSurface)
{
    EGLint width = 0, height = 0;
    EGLenum textureFormat = EGL_NO_TEXTURE;
    EGLenum textureTarget = EGL_NO_TEXTURE;
    const Config *configuration = mConfigSet.get(config);

    if (attribList)
    {
        while (*attribList != EGL_NONE)
        {
            switch (attribList[0])
            {
              case EGL_WIDTH:
                width = attribList[1];
                break;
              case EGL_HEIGHT:
                height = attribList[1];
                break;
              case EGL_LARGEST_PBUFFER:
                if (attribList[1] != EGL_FALSE)
                  UNIMPLEMENTED(); // FIXME
                break;
              case EGL_TEXTURE_FORMAT:
                switch (attribList[1])
                {
                  case EGL_NO_TEXTURE:
                  case EGL_TEXTURE_RGB:
                  case EGL_TEXTURE_RGBA:
                    textureFormat = attribList[1];
                    break;
                  default:
                    return Error(EGL_BAD_ATTRIBUTE);
                }
                break;
              case EGL_TEXTURE_TARGET:
                switch (attribList[1])
                {
                  case EGL_NO_TEXTURE:
                  case EGL_TEXTURE_2D:
                    textureTarget = attribList[1];
                    break;
                  default:
                    return Error(EGL_BAD_ATTRIBUTE);
                }
                break;
              case EGL_MIPMAP_TEXTURE:
                if (attribList[1] != EGL_FALSE)
                  return Error(EGL_BAD_ATTRIBUTE);
                break;
              case EGL_VG_COLORSPACE:
                return Error(EGL_BAD_MATCH);
              case EGL_VG_ALPHA_FORMAT:
                return Error(EGL_BAD_MATCH);
              default:
                return Error(EGL_BAD_ATTRIBUTE);
            }

            attribList += 2;
        }
    }

    if (width < 0 || height < 0)
    {
        return Error(EGL_BAD_PARAMETER);
    }

    if (width == 0 || height == 0)
    {
        return Error(EGL_BAD_ATTRIBUTE);
    }

    if (textureFormat != EGL_NO_TEXTURE && !mRenderer->getRendererExtensions().textureNPOT && (!gl::isPow2(width) || !gl::isPow2(height)))
    {
        return Error(EGL_BAD_MATCH);
    }

    if ((textureFormat != EGL_NO_TEXTURE && textureTarget == EGL_NO_TEXTURE) ||
        (textureFormat == EGL_NO_TEXTURE && textureTarget != EGL_NO_TEXTURE))
    {
        return Error(EGL_BAD_MATCH);
    }

    if (!(configuration->mSurfaceType & EGL_PBUFFER_BIT))
    {
        return Error(EGL_BAD_MATCH);
    }

    if ((textureFormat == EGL_TEXTURE_RGB && configuration->mBindToTextureRGB != EGL_TRUE) ||
        (textureFormat == EGL_TEXTURE_RGBA && configuration->mBindToTextureRGBA != EGL_TRUE))
    {
        return Error(EGL_BAD_ATTRIBUTE);
    }

    if (mRenderer->testDeviceLost(false))
    {
        Error error = restoreLostDevice();
        if (error.isError())
        {
            return error;
        }
    }

    Surface *surface = new Surface(this, configuration, shareHandle, width, height, textureFormat, textureTarget);
    Error error = surface->initialize();
    if (error.isError())
    {
        SafeDelete(surface);
        return error;
    }

    mSurfaceSet.insert(surface);

    *outSurface = surface;
    return Error(EGL_SUCCESS);
}

Error Display::createContext(EGLConfig configHandle, EGLint clientVersion, const gl::Context *shareContext, bool notifyResets,
                             bool robustAccess, EGLContext *outContext)
{
    if (!mRenderer)
    {
        *outContext = EGL_NO_CONTEXT;
        return Error(EGL_SUCCESS);
    }
    else if (mRenderer->testDeviceLost(false))   // Lost device
    {
        Error error = restoreLostDevice();
        if (error.isError())
        {
            return error;
        }
    }

    //TODO(jmadill): shader model is not cross-platform
    if (clientVersion > 2 && mRenderer->getMajorShaderModel() < 4)
    {
        return Error(EGL_BAD_CONFIG);
    }

    gl::Context *context = glCreateContext(clientVersion, shareContext, mRenderer, notifyResets, robustAccess);
    mContextSet.insert(context);

    *outContext = context;
    return Error(EGL_SUCCESS);
}

Error Display::restoreLostDevice()
{
    for (ContextSet::iterator ctx = mContextSet.begin(); ctx != mContextSet.end(); ctx++)
    {
        if ((*ctx)->isResetNotificationEnabled())
        {
            // If reset notifications have been requested, application must delete all contexts first
            return Error(EGL_CONTEXT_LOST);
        }
    }

    // Release surface resources to make the Reset() succeed
    for (SurfaceSet::iterator surface = mSurfaceSet.begin(); surface != mSurfaceSet.end(); surface++)
    {
        (*surface)->release();
    }

    if (!mRenderer->resetDevice())
    {
        return Error(EGL_BAD_ALLOC);
    }

    // Restore any surfaces that may have been lost
    for (SurfaceSet::iterator surface = mSurfaceSet.begin(); surface != mSurfaceSet.end(); surface++)
    {
        Error error = (*surface)->resetSwapChain();
        if (error.isError())
        {
            return error;
        }
    }

    return Error(EGL_SUCCESS);
}


void Display::destroySurface(egl::Surface *surface)
{
    delete surface;
    mSurfaceSet.erase(surface);
}

void Display::destroyContext(gl::Context *context)
{
    glDestroyContext(context);
    mContextSet.erase(context);
}

void Display::notifyDeviceLost()
{
    for (ContextSet::iterator context = mContextSet.begin(); context != mContextSet.end(); context++)
    {
        (*context)->markContextLost();
    }
}

void Display::recreateSwapChains()
{
    for (SurfaceSet::iterator surface = mSurfaceSet.begin(); surface != mSurfaceSet.end(); surface++)
    {
        (*surface)->getSwapChain()->recreate();
    }
}

bool Display::isInitialized() const
{
    return mRenderer != NULL && mConfigSet.size() > 0;
}

bool Display::isValidConfig(EGLConfig config)
{
    return mConfigSet.get(config) != NULL;
}

bool Display::isValidContext(gl::Context *context)
{
    return mContextSet.find(context) != mContextSet.end();
}

bool Display::isValidSurface(egl::Surface *surface)
{
    return mSurfaceSet.find(surface) != mSurfaceSet.end();
}

bool Display::hasExistingWindowSurface(EGLNativeWindowType window)
{
    for (SurfaceSet::iterator surface = mSurfaceSet.begin(); surface != mSurfaceSet.end(); surface++)
    {
        if ((*surface)->getWindowHandle() == window)
        {
            return true;
        }
    }

    return false;
}

std::string Display::generateClientExtensionString()
{
    std::vector<std::string> extensions;

    extensions.push_back("EGL_EXT_client_extensions");

    extensions.push_back("ANGLE_platform_angle");

    if (supportsPlatformD3D())
    {
        extensions.push_back("ANGLE_platform_angle_d3d");
    }

    if (supportsPlatformOpenGL())
    {
        extensions.push_back("ANGLE_platform_angle_opengl");
    }

    std::ostringstream stream;
    std::copy(extensions.begin(), extensions.end(), std::ostream_iterator<std::string>(stream, " "));
    return stream.str();
}

void Display::initDisplayExtensionString()
{
    std::vector<std::string> extensions;

    // Multi-vendor (EXT) extensions
    extensions.push_back("EGL_EXT_create_context_robustness");

    // ANGLE-specific extensions
    if (mRenderer->getShareHandleSupport())
    {
        extensions.push_back("EGL_ANGLE_d3d_share_handle_client_buffer");
        extensions.push_back("EGL_ANGLE_surface_d3d_texture_2d_share_handle");
    }

    extensions.push_back("EGL_ANGLE_query_surface_pointer");
    extensions.push_back("EGL_ANGLE_window_fixed_size");

    if (mRenderer->getPostSubBufferSupport())
    {
        extensions.push_back("EGL_NV_post_sub_buffer");
    }

#if defined (ANGLE_TEST_CONFIG)
    // TODO: complete support for the EGL_KHR_create_context extension
    extensions.push_back("EGL_KHR_create_context");
#endif

    std::ostringstream stream;
    std::copy(extensions.begin(), extensions.end(), std::ostream_iterator<std::string>(stream, " "));
    mDisplayExtensionString = stream.str();
}

const char *Display::getExtensionString(egl::Display *display)
{
    if (display != EGL_NO_DISPLAY)
    {
        return display->mDisplayExtensionString.c_str();
    }
    else
    {
        static std::string clientExtensions = generateClientExtensionString();
        return clientExtensions.c_str();
    }
}

bool Display::supportsPlatformD3D()
{
#if defined(ANGLE_ENABLE_D3D9) || defined(ANGLE_ENABLE_D3D11)
    return true;
#else
    return false;
#endif
}

bool Display::supportsPlatformOpenGL()
{
    return false;
}

void Display::initVendorString()
{
    mVendorString = "Google Inc.";

    LUID adapterLuid = {0};

    //TODO(jmadill): LUID is not cross-platform
    if (mRenderer && mRenderer->getLUID(&adapterLuid))
    {
        char adapterLuidString[64];
        sprintf_s(adapterLuidString, sizeof(adapterLuidString), " (adapter LUID: %08x%08x)", adapterLuid.HighPart, adapterLuid.LowPart);

        mVendorString += adapterLuidString;
    }
}

const char *Display::getVendorString() const
{
    return mVendorString.c_str();
}

}
