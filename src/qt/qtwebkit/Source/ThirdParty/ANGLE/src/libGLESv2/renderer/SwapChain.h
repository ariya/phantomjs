//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SwapChain.h: Defines a back-end specific class that hides the details of the
// implementation-specific swapchain.

#ifndef LIBGLESV2_RENDERER_SWAPCHAIN_H_
#define LIBGLESV2_RENDERER_SWAPCHAIN_H_

#include "common/angleutils.h"

namespace rx
{

class SwapChain
{
  public:
    SwapChain(HWND window, HANDLE shareHandle, GLenum backBufferFormat, GLenum depthBufferFormat)
        : mWindow(window), mShareHandle(shareHandle), mBackBufferFormat(backBufferFormat), mDepthBufferFormat(depthBufferFormat)
    {
    }

    virtual ~SwapChain() {};

    virtual EGLint resize(EGLint backbufferWidth, EGLint backbufferSize) = 0;
    virtual EGLint reset(EGLint backbufferWidth, EGLint backbufferHeight, EGLint swapInterval) = 0;
    virtual EGLint swapRect(EGLint x, EGLint y, EGLint width, EGLint height) = 0;
    virtual void recreate() = 0;

    virtual HANDLE getShareHandle() {return mShareHandle;};

  protected:
    const HWND mWindow;            // Window that the surface is created for.
    const GLenum mBackBufferFormat;
    const GLenum mDepthBufferFormat;

    HANDLE mShareHandle;
};

}
#endif // LIBGLESV2_RENDERER_SWAPCHAIN_H_
