//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// CoreWindowNativeWindow.cpp: NativeWindow for managing ICoreWindow native window types.

#include <algorithm>
#include "common/winrt/CoreWindowNativeWindow.h"
using namespace ABI::Windows::Foundation::Collections;

namespace rx
{

typedef ITypedEventHandler<ABI::Windows::UI::Core::CoreWindow *, ABI::Windows::UI::Core::WindowSizeChangedEventArgs *> SizeChangedHandler;

CoreWindowNativeWindow::~CoreWindowNativeWindow()
{
    unregisterForSizeChangeEvents();
}

bool CoreWindowNativeWindow::initialize(EGLNativeWindowType window, EGLNativeDisplayType display, IPropertySet *propertySet)
{
    ComPtr<IPropertySet> props = propertySet;
    ComPtr<IInspectable> win = window;
    ComPtr<IInspectable> displayInformation = display;
    SIZE swapChainSize = {};
    bool swapChainSizeSpecified = false;
    HRESULT result = S_OK;

    // IPropertySet is an optional parameter and can be null.
    // If one is specified, cache as an IMap and read the properties
    // used for initial host initialization.
    if (propertySet)
    {
        result = props.As(&mPropertyMap);
        if (SUCCEEDED(result))
        {
            // The EGLRenderSurfaceSizeProperty is optional and may be missing.  The IPropertySet
            // was prevalidated to contain the EGLNativeWindowType before being passed to
            // this host.
            result = GetOptionalSizePropertyValue(mPropertyMap, EGLRenderSurfaceSizeProperty, &swapChainSize, &swapChainSizeSpecified);
        }
    }

    if (SUCCEEDED(result))
    {
        result = win.As(&mCoreWindow);
    }

    if (SUCCEEDED(result))
    {
        result = displayInformation.As(&mDisplayInformation);
    }

    if (SUCCEEDED(result))
    {
#if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
        ComPtr<ABI::Windows::Graphics::Display::IDisplayInformation2> displayInformation2;
        result = mDisplayInformation.As(&displayInformation2);
        ASSERT(SUCCEEDED(result));

        result = displayInformation2->get_RawPixelsPerViewPixel(&mScaleFactor);
        ASSERT(SUCCEEDED(result));
#else
        ABI::Windows::Graphics::Display::ResolutionScale resolutionScale;
        result = mDisplayInformation->get_ResolutionScale(&resolutionScale);
        ASSERT(SUCCEEDED(result));

        mScaleFactor = DOUBLE(resolutionScale) / 100.0;
#endif
    }

    if (SUCCEEDED(result))
    {
        // If a swapchain size is specfied, then the automatic resize
        // behaviors implemented by the host should be disabled.  The swapchain
        // will be still be scaled when being rendered to fit the bounds
        // of the host.
        // Scaling of the swapchain output occurs automatically because if
        // the scaling mode setting DXGI_SCALING_STRETCH on the swapchain.
        if (swapChainSizeSpecified)
        {
            mClientRect = { 0, 0, swapChainSize.cx, swapChainSize.cy };
            mSupportsSwapChainResize = false;
        }
        else
        {
            ABI::Windows::Foundation::Rect rect;
            HRESULT result = mCoreWindow->get_Bounds(&rect);
            if (SUCCEEDED(result))
            {
                LONG width = std::floor(rect.Width * mScaleFactor + 0.5);
                LONG height = std::floor(rect.Height * mScaleFactor + 0.5);
                mClientRect = { 0, 0, width, height };
            }
        }
    }

    if (SUCCEEDED(result))
    {
        mNewClientRect = mClientRect;
        mClientRectChanged = false;
        return registerForSizeChangeEvents();
    }

    return false;
}

bool CoreWindowNativeWindow::registerForSizeChangeEvents()
{
    HRESULT result = mCoreWindow->add_SizeChanged(Callback<SizeChangedHandler>(this, &CoreWindowNativeWindow::onSizeChanged).Get(),
                                                  &mSizeChangedEventToken);

    if (SUCCEEDED(result))
    {
        return true;
    }

    return false;
}

void CoreWindowNativeWindow::unregisterForSizeChangeEvents()
{
    if (mCoreWindow)
    {
        (void)mCoreWindow->remove_SizeChanged(mSizeChangedEventToken);
    }
    mSizeChangedEventToken.value = 0;
}

HRESULT CoreWindowNativeWindow::createSwapChain(ID3D11Device *device, DXGIFactory *factory, DXGI_FORMAT format, unsigned int width, unsigned int height, DXGISwapChain **swapChain)
{
    if (device == NULL || factory == NULL || swapChain == NULL || width == 0 || height == 0)
    {
        return E_INVALIDARG;
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = format;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

    *swapChain = nullptr;

    ComPtr<IDXGISwapChain1> newSwapChain;
    HRESULT result = factory->CreateSwapChainForCoreWindow(device, mCoreWindow.Get(), &swapChainDesc, nullptr, newSwapChain.ReleaseAndGetAddressOf());
    if (SUCCEEDED(result))
    {

#if (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP) // This block is disabled for Qt applications, as the resize events are expected
        // Test if swapchain supports resize.  On Windows Phone devices, this will return DXGI_ERROR_UNSUPPORTED.  On
        // other devices DXGI_ERROR_INVALID_CALL should be returned because the combination of flags passed
        // (DXGI_SWAP_CHAIN_FLAG_NONPREROTATED | DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE) are invalid flag combinations.
        if (newSwapChain->ResizeBuffers(swapChainDesc.BufferCount, swapChainDesc.Width, swapChainDesc.Height, swapChainDesc.Format, DXGI_SWAP_CHAIN_FLAG_NONPREROTATED | DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE) == DXGI_ERROR_UNSUPPORTED)
        {
            mSupportsSwapChainResize = false;
        }
#endif // (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)

        result = newSwapChain.CopyTo(swapChain);
    }

    if (SUCCEEDED(result))
    {
        // If automatic swapchain resize behaviors have been disabled, then
        // unregister for the resize change events.
        if (mSupportsSwapChainResize == false)
        {
            unregisterForSizeChangeEvents();
        }
    }

    return result;
}

// Basically, this shouldn't be used on Phone
HRESULT CoreWindowNativeWindow::onSizeChanged(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IWindowSizeChangedEventArgs *e)
{
    ABI::Windows::Foundation::Size size;
    if (SUCCEEDED(e->get_Size(&size)))
    {
        SIZE windowSizeInPixels = {
            std::floor(size.Width * mScaleFactor + 0.5),
            std::floor(size.Height * mScaleFactor + 0.5)
        };
        setNewClientSize(windowSizeInPixels);
    }

    return S_OK;
}
}
