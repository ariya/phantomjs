//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Display.h: Defines the egl::Display class, representing the abstract
// display on which graphics are drawn. Implements EGLDisplay.
// [EGL 1.4] section 2.1.2 page 3.

#ifndef LIBEGL_DISPLAY_H_
#define LIBEGL_DISPLAY_H_

#include <set>
#include <vector>

#include "libEGL/Config.h"

namespace gl
{
class Context;
}

namespace egl
{
class Surface;

class Display
{
  public:
    ~Display();

    bool initialize();
    void terminate();

    static egl::Display *getDisplay(EGLNativeDisplayType displayId);

    bool getConfigs(EGLConfig *configs, const EGLint *attribList, EGLint configSize, EGLint *numConfig);
    bool getConfigAttrib(EGLConfig config, EGLint attribute, EGLint *value);

    EGLSurface createWindowSurface(EGLNativeWindowType window, EGLConfig config, const EGLint *attribList);
    EGLSurface createOffscreenSurface(EGLConfig config, HANDLE shareHandle, const EGLint *attribList);
    EGLContext createContext(EGLConfig configHandle, const gl::Context *shareContext, bool notifyResets, bool robustAccess);

    void destroySurface(egl::Surface *surface);
    void destroyContext(gl::Context *context);

    bool isInitialized() const;
    bool isValidConfig(EGLConfig config);
    bool isValidContext(gl::Context *context);
    bool isValidSurface(egl::Surface *surface);
    bool hasExistingWindowSurface(EGLNativeWindowType window);

    rx::Renderer *getRenderer() { return mRenderer; };

    // exported methods must be virtual
    virtual void notifyDeviceLost();
    virtual void recreateSwapChains();

    const char *getExtensionString() const;
    const char *getVendorString() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Display);

    Display(EGLNativeDisplayType displayId);

    bool restoreLostDevice();

    EGLNativeDisplayType mDisplayId;

    bool mSoftwareDevice;
    
    typedef std::set<Surface*> SurfaceSet;
    SurfaceSet mSurfaceSet;

    ConfigSet mConfigSet;

    typedef std::set<gl::Context*> ContextSet;
    ContextSet mContextSet;

    rx::Renderer *mRenderer;

    void initExtensionString();
    void initVendorString();
    std::string mExtensionString;
    std::string mVendorString;
};
}

#endif   // LIBEGL_DISPLAY_H_
