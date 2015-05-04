//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// NativeWindow.cpp: Handler for managing HWND native window types.

#include "common/NativeWindow.h"
#include "common/debug.h"

namespace rx
{
bool IsValidEGLNativeWindowType(EGLNativeWindowType window)
{
    return (IsWindow(window) == TRUE);
}

NativeWindow::NativeWindow(EGLNativeWindowType window, EGLNativeDisplayType display) : mWindow(window), mDisplay(display)
{
}

bool NativeWindow::initialize()
{ 
    return true; 
}

bool NativeWindow::getClientRect(LPRECT rect)
{
    return GetClientRect(mWindow, rect) == TRUE;
}

bool NativeWindow::isIconic()
{
    return IsIconic(mWindow) == TRUE;
}

HRESULT NativeWindow::createSwapChain(NativeWindow::Device* device, DXGIFactory* factory,
                                      DXGI_FORMAT format, unsigned int width, unsigned int height,
                                      DXGISwapChain** swapChain)
{
    if (device == NULL || factory == NULL || swapChain == NULL || width == 0 || height == 0)
    {
        return E_INVALIDARG;
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Format = format;
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
    swapChainDesc.Flags = 0;
    swapChainDesc.OutputWindow = mWindow;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    return factory->CreateSwapChain(device, &swapChainDesc, swapChain);
}
}
