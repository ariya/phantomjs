//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// libEGL.cpp: Implements the exported EGL functions.

#undef EGLAPI
#define EGLAPI

#include <exception>

#include "common/debug.h"
#include "common/version.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/main.h"
#include "libGLESv2/renderer/SwapChain.h"
#if defined(ANGLE_ENABLE_D3D11)
#  include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"
#endif

#include "libEGL/main.h"
#include "libEGL/Display.h"
#include "libEGL/Surface.h"

#include "common/NativeWindow.h"

bool validateDisplay(egl::Display *display)
{
    if (display == EGL_NO_DISPLAY)
    {
        recordError(egl::Error(EGL_BAD_DISPLAY));
        return false;
    }

    if (!display->isInitialized())
    {
        recordError(egl::Error(EGL_NOT_INITIALIZED));
        return false;
    }

    return true;
}

bool validateConfig(egl::Display *display, EGLConfig config)
{
    if (!validateDisplay(display))
    {
        return false;
    }

    if (!display->isValidConfig(config))
    {
        recordError(egl::Error(EGL_BAD_CONFIG));
        return false;
    }

    return true;
}

bool validateContext(egl::Display *display, gl::Context *context)
{
    if (!validateDisplay(display))
    {
        return false;
    }

    if (!display->isValidContext(context))
    {
        recordError(egl::Error(EGL_BAD_CONTEXT));
        return false;
    }

    return true;
}

bool validateSurface(egl::Display *display, egl::Surface *surface)
{
    if (!validateDisplay(display))
    {
        return false;
    }

    if (!display->isValidSurface(surface))
    {
        recordError(egl::Error(EGL_BAD_SURFACE));
        return false;
    }

    return true;
}

extern "C"
{
EGLint __stdcall eglGetError(void)
{
    EVENT("()");

    EGLint error = egl::getCurrentError();
    recordError(egl::Error(EGL_SUCCESS));
    return error;
}

EGLDisplay __stdcall eglGetDisplay(EGLNativeDisplayType display_id)
{
    EVENT("(EGLNativeDisplayType display_id = 0x%0.8p)", display_id);

    return egl::Display::getDisplay(display_id, egl::AttributeMap());
}

EGLDisplay __stdcall eglGetPlatformDisplayEXT(EGLenum platform, void *native_display, const EGLint *attrib_list)
{
    EVENT("(EGLenum platform = %d, void* native_display = 0x%0.8p, const EGLint* attrib_list = 0x%0.8p)",
          platform, native_display, attrib_list);

    switch (platform)
    {
      case EGL_PLATFORM_ANGLE_ANGLE:
        break;

      default:
        recordError(egl::Error(EGL_BAD_CONFIG));
        return EGL_NO_DISPLAY;
    }

    EGLNativeDisplayType displayId = static_cast<EGLNativeDisplayType>(native_display);

#if !defined(ANGLE_ENABLE_WINDOWS_STORE)
    // Validate the display device context
    if (WindowFromDC(displayId) == NULL)
    {
        recordError(egl::Error(EGL_SUCCESS));
        return EGL_NO_DISPLAY;
    }
#endif

    EGLint platformType = EGL_PLATFORM_ANGLE_TYPE_DEFAULT_ANGLE;
    bool majorVersionSpecified = false;
    bool minorVersionSpecified = false;
    bool requestedWARP = false;

    if (attrib_list)
    {
        for (const EGLint *curAttrib = attrib_list; curAttrib[0] != EGL_NONE; curAttrib += 2)
        {
            switch (curAttrib[0])
            {
              case EGL_PLATFORM_ANGLE_TYPE_ANGLE:
                switch (curAttrib[1])
                {
                  case EGL_PLATFORM_ANGLE_TYPE_DEFAULT_ANGLE:
                    break;

                  case EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE:
                  case EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE:
                    if (!egl::Display::supportsPlatformD3D())
                    {
                        recordError(egl::Error(EGL_SUCCESS));
                        return EGL_NO_DISPLAY;
                    }
                    break;

                  case EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE:
                  case EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE:
                    if (!egl::Display::supportsPlatformOpenGL())
                    {
                        recordError(egl::Error(EGL_SUCCESS));
                        return EGL_NO_DISPLAY;
                    }
                    break;

                  default:
                    recordError(egl::Error(EGL_SUCCESS));
                    return EGL_NO_DISPLAY;
                }
                platformType = curAttrib[1];
                break;

              case EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE:
                if (curAttrib[1] != EGL_DONT_CARE)
                {
                    majorVersionSpecified = true;
                }
                break;

              case EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE:
                if (curAttrib[1] != EGL_DONT_CARE)
                {
                    minorVersionSpecified = true;
                }
                break;

              case EGL_PLATFORM_ANGLE_USE_WARP_ANGLE:
                if (!egl::Display::supportsPlatformD3D())
                {
                    recordError(egl::Error(EGL_SUCCESS));
                    return EGL_NO_DISPLAY;
                }

                switch (curAttrib[1])
                {
                  case EGL_FALSE:
                  case EGL_TRUE:
                    break;

                  default:
                    recordError(egl::Error(EGL_SUCCESS));
                    return EGL_NO_DISPLAY;
                }

                requestedWARP = (curAttrib[1] == EGL_TRUE);
                break;

              default:
                break;
            }
        }
    }

    if (!majorVersionSpecified && minorVersionSpecified)
    {
        recordError(egl::Error(EGL_BAD_ATTRIBUTE));
        return EGL_NO_DISPLAY;
    }

    if (platformType != EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE && requestedWARP)
    {
        recordError(egl::Error(EGL_BAD_ATTRIBUTE));
        return EGL_NO_DISPLAY;
    }

    recordError(egl::Error(EGL_SUCCESS));
    return egl::Display::getDisplay(displayId, egl::AttributeMap(attrib_list));
}

EGLBoolean __stdcall eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLint *major = 0x%0.8p, EGLint *minor = 0x%0.8p)",
          dpy, major, minor);

    if (dpy == EGL_NO_DISPLAY)
    {
        recordError(egl::Error(EGL_BAD_DISPLAY));
        return EGL_FALSE;
    }

    egl::Display *display = static_cast<egl::Display*>(dpy);

    egl::Error error = display->initialize();
    if (error.isError())
    {
        recordError(error);
        return EGL_FALSE;
    }

    if (major) *major = 1;
    if (minor) *minor = 4;

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean __stdcall eglTerminate(EGLDisplay dpy)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p)", dpy);

    if (dpy == EGL_NO_DISPLAY)
    {
        recordError(egl::Error(EGL_BAD_DISPLAY));
        return EGL_FALSE;
    }

    egl::Display *display = static_cast<egl::Display*>(dpy);

    display->terminate();

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

const char *__stdcall eglQueryString(EGLDisplay dpy, EGLint name)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLint name = %d)", dpy, name);

    egl::Display *display = static_cast<egl::Display*>(dpy);
    if (!(display == EGL_NO_DISPLAY && name == EGL_EXTENSIONS) && !validateDisplay(display))
    {
        return NULL;
    }

    const char *result;
    switch (name)
    {
      case EGL_CLIENT_APIS:
        result = "OpenGL_ES";
        break;
      case EGL_EXTENSIONS:
        result = egl::Display::getExtensionString(display);
        break;
      case EGL_VENDOR:
        result = display->getVendorString();
        break;
      case EGL_VERSION:
        result = "1.4 (ANGLE " ANGLE_VERSION_STRING ")";
        break;
      default:
        recordError(egl::Error(EGL_BAD_PARAMETER));
        return NULL;
    }

    recordError(egl::Error(EGL_SUCCESS));
    return result;
}

EGLBoolean __stdcall eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig *configs = 0x%0.8p, "
          "EGLint config_size = %d, EGLint *num_config = 0x%0.8p)",
          dpy, configs, config_size, num_config);

    egl::Display *display = static_cast<egl::Display*>(dpy);

    if (!validateDisplay(display))
    {
        return EGL_FALSE;
    }

    if (!num_config)
    {
        recordError(egl::Error(EGL_BAD_PARAMETER));
        return EGL_FALSE;
    }

    const EGLint attribList[] =    {EGL_NONE};

    if (!display->getConfigs(configs, attribList, config_size, num_config))
    {
        recordError(egl::Error(EGL_BAD_ATTRIBUTE));
        return EGL_FALSE;
    }

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean __stdcall eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, const EGLint *attrib_list = 0x%0.8p, "
          "EGLConfig *configs = 0x%0.8p, EGLint config_size = %d, EGLint *num_config = 0x%0.8p)",
          dpy, attrib_list, configs, config_size, num_config);

    egl::Display *display = static_cast<egl::Display*>(dpy);

    if (!validateDisplay(display))
    {
        return EGL_FALSE;
    }

    if (!num_config)
    {
        recordError(egl::Error(EGL_BAD_PARAMETER));
        return EGL_FALSE;
    }

    const EGLint attribList[] =    {EGL_NONE};

    if (!attrib_list)
    {
        attrib_list = attribList;
    }

    display->getConfigs(configs, attrib_list, config_size, num_config);

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean __stdcall eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, EGLint attribute = %d, EGLint *value = 0x%0.8p)",
          dpy, config, attribute, value);

    egl::Display *display = static_cast<egl::Display*>(dpy);

    if (!validateConfig(display, config))
    {
        return EGL_FALSE;
    }

    if (!display->getConfigAttrib(config, attribute, value))
    {
        recordError(egl::Error(EGL_BAD_ATTRIBUTE));
        return EGL_FALSE;
    }

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLSurface __stdcall eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, EGLNativeWindowType win = 0x%0.8p, "
          "const EGLint *attrib_list = 0x%0.8p)", dpy, config, win, attrib_list);

    egl::Display *display = static_cast<egl::Display*>(dpy);

    if (!validateConfig(display, config))
    {
        return EGL_NO_SURFACE;
    }

    if (!rx::IsValidEGLNativeWindowType(win))
    {
        recordError(egl::Error(EGL_BAD_NATIVE_WINDOW));
        return EGL_NO_SURFACE;
    }

    EGLSurface surface = EGL_NO_SURFACE;
    egl::Error error = display->createWindowSurface(win, config, attrib_list, &surface);
    if (error.isError())
    {
        recordError(error);
        return EGL_NO_SURFACE;
    }

    return surface;
}

EGLSurface __stdcall eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, const EGLint *attrib_list = 0x%0.8p)",
          dpy, config, attrib_list);

    egl::Display *display = static_cast<egl::Display*>(dpy);

    if (!validateConfig(display, config))
    {
        return EGL_NO_SURFACE;
    }

    EGLSurface surface = EGL_NO_SURFACE;
    egl::Error error = display->createOffscreenSurface(config, NULL, attrib_list, &surface);
    if (error.isError())
    {
        recordError(error);
        return EGL_NO_SURFACE;
    }

    return surface;
}

EGLSurface __stdcall eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, EGLNativePixmapType pixmap = 0x%0.8p, "
          "const EGLint *attrib_list = 0x%0.8p)", dpy, config, pixmap, attrib_list);

    egl::Display *display = static_cast<egl::Display*>(dpy);

    if (!validateConfig(display, config))
    {
        return EGL_NO_SURFACE;
    }

    UNIMPLEMENTED();   // FIXME

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_NO_SURFACE;
}

EGLBoolean __stdcall eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p)", dpy, surface);

    egl::Display *display = static_cast<egl::Display*>(dpy);
    egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

    if (!validateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    if (surface == EGL_NO_SURFACE)
    {
        recordError(egl::Error(EGL_BAD_SURFACE));
        return EGL_FALSE;
    }

    display->destroySurface((egl::Surface*)surface);

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean __stdcall eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint attribute = %d, EGLint *value = 0x%0.8p)",
          dpy, surface, attribute, value);

    egl::Display *display = static_cast<egl::Display*>(dpy);
    egl::Surface *eglSurface = (egl::Surface*)surface;

    if (!validateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    if (surface == EGL_NO_SURFACE)
    {
        recordError(egl::Error(EGL_BAD_SURFACE));
        return EGL_FALSE;
    }

    switch (attribute)
    {
      case EGL_VG_ALPHA_FORMAT:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_VG_COLORSPACE:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_CONFIG_ID:
        *value = eglSurface->getConfigID();
        break;
      case EGL_HEIGHT:
        *value = eglSurface->getHeight();
        break;
      case EGL_HORIZONTAL_RESOLUTION:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_LARGEST_PBUFFER:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_MIPMAP_TEXTURE:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_MIPMAP_LEVEL:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_MULTISAMPLE_RESOLVE:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_PIXEL_ASPECT_RATIO:
        *value = eglSurface->getPixelAspectRatio();
        break;
      case EGL_RENDER_BUFFER:
        *value = eglSurface->getRenderBuffer();
        break;
      case EGL_SWAP_BEHAVIOR:
        *value = eglSurface->getSwapBehavior();
        break;
      case EGL_TEXTURE_FORMAT:
        *value = eglSurface->getTextureFormat();
        break;
      case EGL_TEXTURE_TARGET:
        *value = eglSurface->getTextureTarget();
        break;
      case EGL_VERTICAL_RESOLUTION:
        UNIMPLEMENTED();   // FIXME
        break;
      case EGL_WIDTH:
        *value = eglSurface->getWidth();
        break;
      case EGL_POST_SUB_BUFFER_SUPPORTED_NV:
        *value = eglSurface->isPostSubBufferSupported();
        break;
      case EGL_FIXED_SIZE_ANGLE:
        *value = eglSurface->isFixedSize();
        break;
      default:
        recordError(egl::Error(EGL_BAD_ATTRIBUTE));
        return EGL_FALSE;
    }

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean __stdcall eglQuerySurfacePointerANGLE(EGLDisplay dpy, EGLSurface surface, EGLint attribute, void **value)
{
    TRACE("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint attribute = %d, void **value = 0x%0.8p)",
          dpy, surface, attribute, value);

    egl::Display *display = static_cast<egl::Display*>(dpy);
    egl::Surface *eglSurface = (egl::Surface*)surface;

    switch (attribute)
    {
      case EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE:
        {
            if (!validateSurface(display, eglSurface))
            {
                return EGL_FALSE;
            }

            if (surface == EGL_NO_SURFACE)
            {
                recordError(egl::Error(EGL_BAD_SURFACE));
                return EGL_FALSE;
            }

            rx::SwapChain *swapchain = eglSurface->getSwapChain();
            *value = (void*) (swapchain ? swapchain->getShareHandle() : NULL);
        }
        break;
#if defined(ANGLE_ENABLE_D3D11)
      case EGL_DEVICE_EXT:
        {
            if (!validateDisplay(display))
            {
                return EGL_FALSE;
            }

            rx::Renderer *renderer = display->getRenderer();
            if (!renderer)
            {
                *value = NULL;
                break;
            }

            if (renderer->getMajorShaderModel() < 4)
            {
                recordError(egl::Error(EGL_BAD_CONTEXT));
                return EGL_FALSE;
            }

            *value = static_cast<rx::Renderer11*>(renderer)->getDevice();
        }
        break;
#endif
      default:
        recordError(egl::Error(EGL_BAD_ATTRIBUTE));
        return EGL_FALSE;
    }

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean __stdcall eglBindAPI(EGLenum api)
{
    EVENT("(EGLenum api = 0x%X)", api);

    switch (api)
    {
      case EGL_OPENGL_API:
      case EGL_OPENVG_API:
        recordError(egl::Error(EGL_BAD_PARAMETER));
        return EGL_FALSE;   // Not supported by this implementation
      case EGL_OPENGL_ES_API:
        break;
      default:
        recordError(egl::Error(EGL_BAD_PARAMETER));
        return EGL_FALSE;
    }

    egl::setCurrentAPI(api);

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLenum __stdcall eglQueryAPI(void)
{
    EVENT("()");

    EGLenum API = egl::getCurrentAPI();

    recordError(egl::Error(EGL_SUCCESS));
    return API;
}

EGLBoolean __stdcall eglWaitClient(void)
{
    EVENT("()");

    UNIMPLEMENTED();   // FIXME

    recordError(egl::Error(EGL_SUCCESS));
    return 0;
}

EGLBoolean __stdcall eglReleaseThread(void)
{
    EVENT("()");

    eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_CONTEXT, EGL_NO_SURFACE, EGL_NO_SURFACE);

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLSurface __stdcall eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLenum buftype = 0x%X, EGLClientBuffer buffer = 0x%0.8p, "
          "EGLConfig config = 0x%0.8p, const EGLint *attrib_list = 0x%0.8p)",
          dpy, buftype, buffer, config, attrib_list);

    egl::Display *display = static_cast<egl::Display*>(dpy);

    if (!validateConfig(display, config))
    {
        return EGL_NO_SURFACE;
    }

    if (buftype != EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE || !buffer)
    {
        recordError(egl::Error(EGL_BAD_PARAMETER));
        return EGL_NO_SURFACE;
    }

    EGLSurface surface = EGL_NO_SURFACE;
    egl::Error error = display->createOffscreenSurface(config, (HANDLE)buffer, attrib_list, &surface);
    if (error.isError())
    {
        recordError(error);
        return EGL_NO_SURFACE;
    }

    return surface;
}

EGLBoolean __stdcall eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint attribute = %d, EGLint value = %d)",
          dpy, surface, attribute, value);

    egl::Display *display = static_cast<egl::Display*>(dpy);
    egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

    if (!validateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    switch (attribute)
    {
    case EGL_WIDTH:
        if (!eglSurface->isFixedSize() || !value) {
            recordError(egl::Error(EGL_BAD_PARAMETER));
            return EGL_FALSE;
        }
        eglSurface->setFixedWidth(value);
        return EGL_TRUE;
    case EGL_HEIGHT:
        if (!eglSurface->isFixedSize() || !value) {
            recordError(egl::Error(EGL_BAD_PARAMETER));
            return EGL_FALSE;
        }
        eglSurface->setFixedHeight(value);
        return EGL_TRUE;
    default:
        break;
    }

    UNIMPLEMENTED();   // FIXME

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean __stdcall eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint buffer = %d)", dpy, surface, buffer);

    egl::Display *display = static_cast<egl::Display*>(dpy);
    egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

    if (!validateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    if (buffer != EGL_BACK_BUFFER)
    {
        recordError(egl::Error(EGL_BAD_PARAMETER));
        return EGL_FALSE;
    }

    if (surface == EGL_NO_SURFACE || eglSurface->getWindowHandle())
    {
        recordError(egl::Error(EGL_BAD_SURFACE));
        return EGL_FALSE;
    }

    if (eglSurface->getBoundTexture())
    {
        recordError(egl::Error(EGL_BAD_ACCESS));
        return EGL_FALSE;
    }

    if (eglSurface->getTextureFormat() == EGL_NO_TEXTURE)
    {
        recordError(egl::Error(EGL_BAD_MATCH));
        return EGL_FALSE;
    }

    if (!glBindTexImage(eglSurface))
    {
        recordError(egl::Error(EGL_BAD_MATCH));
        return EGL_FALSE;
    }

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean __stdcall eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint buffer = %d)", dpy, surface, buffer);

    egl::Display *display = static_cast<egl::Display*>(dpy);
    egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

    if (!validateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    if (buffer != EGL_BACK_BUFFER)
    {
        recordError(egl::Error(EGL_BAD_PARAMETER));
        return EGL_FALSE;
    }

    if (surface == EGL_NO_SURFACE || eglSurface->getWindowHandle())
    {
        recordError(egl::Error(EGL_BAD_SURFACE));
        return EGL_FALSE;
    }

    if (eglSurface->getTextureFormat() == EGL_NO_TEXTURE)
    {
        recordError(egl::Error(EGL_BAD_MATCH));
        return EGL_FALSE;
    }

    gl::Texture2D *texture = eglSurface->getBoundTexture();

    if (texture)
    {
        texture->releaseTexImage();
    }

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean __stdcall eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLint interval = %d)", dpy, interval);

    egl::Display *display = static_cast<egl::Display*>(dpy);

    if (!validateDisplay(display))
    {
        return EGL_FALSE;
    }

    egl::Surface *draw_surface = static_cast<egl::Surface*>(egl::getCurrentDrawSurface());

    if (draw_surface == NULL)
    {
        recordError(egl::Error(EGL_BAD_SURFACE));
        return EGL_FALSE;
    }

    draw_surface->setSwapInterval(interval);

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLContext __stdcall eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, EGLContext share_context = 0x%0.8p, "
          "const EGLint *attrib_list = 0x%0.8p)", dpy, config, share_context, attrib_list);

    // Get the requested client version (default is 1) and check it is 2 or 3.
    EGLint client_version = 1;
    bool reset_notification = false;
    bool robust_access = false;

    if (attrib_list)
    {
        for (const EGLint* attribute = attrib_list; attribute[0] != EGL_NONE; attribute += 2)
        {
            switch (attribute[0])
            {
              case EGL_CONTEXT_CLIENT_VERSION:
                client_version = attribute[1];
                break;
              case EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT:
                if (attribute[1] == EGL_TRUE)
                {
                    recordError(egl::Error(EGL_BAD_CONFIG));   // Unimplemented
                    return EGL_NO_CONTEXT;
                    // robust_access = true;
                }
                else if (attribute[1] != EGL_FALSE)
                {
                    recordError(egl::Error(EGL_BAD_ATTRIBUTE));
                    return EGL_NO_CONTEXT;
                }
                break;
              case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT:
                if (attribute[1] == EGL_LOSE_CONTEXT_ON_RESET_EXT)
                {
                    reset_notification = true;
                }
                else if (attribute[1] != EGL_NO_RESET_NOTIFICATION_EXT)
                {
                    recordError(egl::Error(EGL_BAD_ATTRIBUTE));
                    return EGL_NO_CONTEXT;
                }
                break;
              default:
                recordError(egl::Error(EGL_BAD_ATTRIBUTE));
                return EGL_NO_CONTEXT;
            }
        }
    }

    if (client_version != 2 && client_version != 3)
    {
        recordError(egl::Error(EGL_BAD_CONFIG));
        return EGL_NO_CONTEXT;
    }

    egl::Display *display = static_cast<egl::Display*>(dpy);

    if (share_context)
    {
        gl::Context* sharedGLContext = static_cast<gl::Context*>(share_context);

        if (sharedGLContext->isResetNotificationEnabled() != reset_notification)
        {
            recordError(egl::Error(EGL_BAD_MATCH));
            return EGL_NO_CONTEXT;
        }

        if (sharedGLContext->getClientVersion() != client_version)
        {
            recordError(egl::Error(EGL_BAD_CONTEXT));
            return EGL_NO_CONTEXT;
        }

        // Can not share contexts between displays
        if (sharedGLContext->getRenderer() != display->getRenderer())
        {
            recordError(egl::Error(EGL_BAD_MATCH));
            return EGL_NO_CONTEXT;
        }
    }

    if (!validateConfig(display, config))
    {
        return EGL_NO_CONTEXT;
    }

    EGLContext context = EGL_NO_CONTEXT;
    egl::Error error =  display->createContext(config, client_version, static_cast<gl::Context*>(share_context),
                                               reset_notification, robust_access, &context);
    if (error.isError())
    {
        recordError(error);
        return EGL_NO_CONTEXT;
    }

    return context;
}

EGLBoolean __stdcall eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLContext ctx = 0x%0.8p)", dpy, ctx);

    egl::Display *display = static_cast<egl::Display*>(dpy);
    gl::Context *context = static_cast<gl::Context*>(ctx);

    if (!validateContext(display, context))
    {
        return EGL_FALSE;
    }

    if (ctx == EGL_NO_CONTEXT)
    {
        recordError(egl::Error(EGL_BAD_CONTEXT));
        return EGL_FALSE;
    }

    display->destroyContext(context);

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean __stdcall eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface draw = 0x%0.8p, EGLSurface read = 0x%0.8p, EGLContext ctx = 0x%0.8p)",
          dpy, draw, read, ctx);

    egl::Display *display = static_cast<egl::Display*>(dpy);
    gl::Context *context = static_cast<gl::Context*>(ctx);

    bool noContext = (ctx == EGL_NO_CONTEXT);
    bool noSurface = (draw == EGL_NO_SURFACE || read == EGL_NO_SURFACE);
    if (noContext != noSurface)
    {
        recordError(egl::Error(EGL_BAD_MATCH));
        return EGL_FALSE;
    }

    if (ctx != EGL_NO_CONTEXT && !validateContext(display, context))
    {
        return EGL_FALSE;
    }

    if (dpy != EGL_NO_DISPLAY && display->isInitialized())
    {
        rx::Renderer *renderer = display->getRenderer();
        if (renderer->testDeviceLost(true))
        {
            return EGL_FALSE;
        }

        if (renderer->isDeviceLost())
        {
            recordError(egl::Error(EGL_CONTEXT_LOST));
            return EGL_FALSE;
        }
    }

    if ((draw != EGL_NO_SURFACE && !validateSurface(display, static_cast<egl::Surface*>(draw))) ||
        (read != EGL_NO_SURFACE && !validateSurface(display, static_cast<egl::Surface*>(read))))
    {
        return EGL_FALSE;
    }

    if (draw != read)
    {
        UNIMPLEMENTED();   // FIXME
    }

    egl::setCurrentDisplay(dpy);
    egl::setCurrentDrawSurface(draw);
    egl::setCurrentReadSurface(read);

    glMakeCurrent(context, display, static_cast<egl::Surface*>(draw));

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLContext __stdcall eglGetCurrentContext(void)
{
    EVENT("()");

    EGLContext context = glGetCurrentContext();

    recordError(egl::Error(EGL_SUCCESS));
    return context;
}

EGLSurface __stdcall eglGetCurrentSurface(EGLint readdraw)
{
    EVENT("(EGLint readdraw = %d)", readdraw);

    if (readdraw == EGL_READ)
    {
        recordError(egl::Error(EGL_SUCCESS));
        return egl::getCurrentReadSurface();
    }
    else if (readdraw == EGL_DRAW)
    {
        recordError(egl::Error(EGL_SUCCESS));
        return egl::getCurrentDrawSurface();
    }
    else
    {
        recordError(egl::Error(EGL_BAD_PARAMETER));
        return EGL_NO_SURFACE;
    }
}

EGLDisplay __stdcall eglGetCurrentDisplay(void)
{
    EVENT("()");

    EGLDisplay dpy = egl::getCurrentDisplay();

    recordError(egl::Error(EGL_SUCCESS));
    return dpy;
}

EGLBoolean __stdcall eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLContext ctx = 0x%0.8p, EGLint attribute = %d, EGLint *value = 0x%0.8p)",
          dpy, ctx, attribute, value);

    egl::Display *display = static_cast<egl::Display*>(dpy);
    gl::Context *context = static_cast<gl::Context*>(ctx);

    if (!validateContext(display, context))
    {
        return EGL_FALSE;
    }

    UNIMPLEMENTED();   // FIXME

    recordError(egl::Error(EGL_SUCCESS));
    return 0;
}

EGLBoolean __stdcall eglWaitGL(void)
{
    EVENT("()");

    UNIMPLEMENTED();   // FIXME

    recordError(egl::Error(EGL_SUCCESS));
    return 0;
}

EGLBoolean __stdcall eglWaitNative(EGLint engine)
{
    EVENT("(EGLint engine = %d)", engine);

    UNIMPLEMENTED();   // FIXME

    recordError(egl::Error(EGL_SUCCESS));
    return 0;
}

EGLBoolean __stdcall eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p)", dpy, surface);

    egl::Display *display = static_cast<egl::Display*>(dpy);
    egl::Surface *eglSurface = (egl::Surface*)surface;

    if (!validateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    if (display->getRenderer()->isDeviceLost())
    {
        recordError(egl::Error(EGL_CONTEXT_LOST));
        return EGL_FALSE;
    }

    if (surface == EGL_NO_SURFACE)
    {
        recordError(egl::Error(EGL_BAD_SURFACE));
        return EGL_FALSE;
    }

    egl::Error error = eglSurface->swap();
    if (error.isError())
    {
        recordError(error);
        return EGL_FALSE;
    }

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

EGLBoolean __stdcall eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLNativePixmapType target = 0x%0.8p)", dpy, surface, target);

    egl::Display *display = static_cast<egl::Display*>(dpy);
    egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

    if (!validateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    if (display->getRenderer()->isDeviceLost())
    {
        recordError(egl::Error(EGL_CONTEXT_LOST));
        return EGL_FALSE;
    }

    UNIMPLEMENTED();   // FIXME

    recordError(egl::Error(EGL_SUCCESS));
    return 0;
}

EGLBoolean __stdcall eglPostSubBufferNV(EGLDisplay dpy, EGLSurface surface, EGLint x, EGLint y, EGLint width, EGLint height)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint x = %d, EGLint y = %d, EGLint width = %d, EGLint height = %d)", dpy, surface, x, y, width, height);

    if (x < 0 || y < 0 || width < 0 || height < 0)
    {
        recordError(egl::Error(EGL_BAD_PARAMETER));
        return EGL_FALSE;
    }

    egl::Display *display = static_cast<egl::Display*>(dpy);
    egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

    if (!validateSurface(display, eglSurface))
    {
        return EGL_FALSE;
    }

    if (display->getRenderer()->isDeviceLost())
    {
        recordError(egl::Error(EGL_CONTEXT_LOST));
        return EGL_FALSE;
    }

    if (surface == EGL_NO_SURFACE)
    {
        recordError(egl::Error(EGL_BAD_SURFACE));
        return EGL_FALSE;
    }

    egl::Error error = eglSurface->postSubBuffer(x, y, width, height);
    if (error.isError())
    {
        recordError(error);
        return EGL_FALSE;
    }

    recordError(egl::Error(EGL_SUCCESS));
    return EGL_TRUE;
}

__eglMustCastToProperFunctionPointerType __stdcall eglGetProcAddress(const char *procname)
{
    EVENT("(const char *procname = \"%s\")", procname);

    struct Extension
    {
        const char *name;
        __eglMustCastToProperFunctionPointerType address;
    };

    static const Extension eglExtensions[] =
    {
        { "eglQuerySurfacePointerANGLE", (__eglMustCastToProperFunctionPointerType)eglQuerySurfacePointerANGLE },
        { "eglPostSubBufferNV", (__eglMustCastToProperFunctionPointerType)eglPostSubBufferNV },
        { "eglGetPlatformDisplayEXT", (__eglMustCastToProperFunctionPointerType)eglGetPlatformDisplayEXT },
        { "", NULL },
    };

    for (unsigned int ext = 0; ext < ArraySize(eglExtensions); ext++)
    {
        if (strcmp(procname, eglExtensions[ext].name) == 0)
        {
            return (__eglMustCastToProperFunctionPointerType)eglExtensions[ext].address;
        }
    }

    return glGetProcAddress(procname);
}
}
