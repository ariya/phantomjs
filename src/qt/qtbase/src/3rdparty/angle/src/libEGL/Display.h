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

#include "libEGL/Error.h"
#include "libEGL/Config.h"
#include "libEGL/AttributeMap.h"

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

    Error initialize();
    void terminate();

    static egl::Display *getDisplay(EGLNativeDisplayType displayId, const AttributeMap &attribMap);

    static const char *getExtensionString(egl::Display *display);

    static bool supportsPlatformD3D();
    static bool supportsPlatformOpenGL();

    bool getConfigs(EGLConfig *configs, const EGLint *attribList, EGLint configSize, EGLint *numConfig);
    bool getConfigAttrib(EGLConfig config, EGLint attribute, EGLint *value);

    Error createWindowSurface(EGLNativeWindowType window, EGLConfig config, const EGLint *attribList, EGLSurface *outSurface);
    Error createOffscreenSurface(EGLConfig config, HANDLE shareHandle, const EGLint *attribList, EGLSurface *outSurface);
    Error createContext(EGLConfig configHandle, EGLint clientVersion, const gl::Context *shareContext, bool notifyResets,
                        bool robustAccess, EGLContext *outContext);

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
    EGLNativeDisplayType getDisplayId() const { return mDisplayId; }

  private:
    DISALLOW_COPY_AND_ASSIGN(Display);

    Display(EGLNativeDisplayType displayId);

    void setAttributes(const AttributeMap &attribMap);

    Error restoreLostDevice();

    EGLNativeDisplayType mDisplayId;
    AttributeMap mAttributeMap;

    typedef std::set<Surface*> SurfaceSet;
    SurfaceSet mSurfaceSet;

    ConfigSet mConfigSet;

    typedef std::set<gl::Context*> ContextSet;
    ContextSet mContextSet;

    rx::Renderer *mRenderer;

    static std::string generateClientExtensionString();

    void initDisplayExtensionString();
    std::string mDisplayExtensionString;

    void initVendorString();
    std::string mVendorString;
};
}

#endif   // LIBEGL_DISPLAY_H_
