//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SwapChainPanelNativeWindow.cpp: NativeWindow for managing ISwapChainPanel native window types.

#include "common/winrt/SwapChainPanelNativeWindow.h"
#include <algorithm>
#include <math.h>
using namespace ABI::Windows::Foundation::Collections;

namespace rx
{
SwapChainPanelNativeWindow::~SwapChainPanelNativeWindow()
{
    unregisterForSizeChangeEvents();
}

bool SwapChainPanelNativeWindow::initialize(EGLNativeWindowType window, EGLNativeDisplayType display, IPropertySet *propertySet)
{
    ComPtr<IPropertySet> props = propertySet;
    ComPtr<IInspectable> win = window;
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
        result = win.As(&mSwapChainPanel);
    }

    if (SUCCEEDED(result))
    {
        // If a swapchain size is specfied, then the automatic resize
        // behaviors implemented by the host should be disabled.  The swapchain
        // will be still be scaled when being rendered to fit the bounds
        // of the host.
        // Scaling of the swapchain output needs to be handled by the
        // host for swapchain panels even though the scaling mode setting
        // DXGI_SCALING_STRETCH is configured on the swapchain.
        if (swapChainSizeSpecified)
        {
            mClientRect = { 0, 0, swapChainSize.cx, swapChainSize.cy };

            // Enable host swapchain scaling
            mRequiresSwapChainScaling = true;
        }
        else
        {
            result = GetSwapChainPanelSize(mSwapChainPanel, &mClientRect);
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

bool SwapChainPanelNativeWindow::registerForSizeChangeEvents()
{
    ComPtr<ABI::Windows::UI::Xaml::ISizeChangedEventHandler> sizeChangedHandler;
    ComPtr<ABI::Windows::UI::Xaml::IFrameworkElement> frameworkElement;
    HRESULT result = Microsoft::WRL::MakeAndInitialize<SwapChainPanelSizeChangedHandler>(sizeChangedHandler.ReleaseAndGetAddressOf(), this->shared_from_this());

    if (SUCCEEDED(result))
    {
        result = mSwapChainPanel.As(&frameworkElement);
    }

    if (SUCCEEDED(result))
    {
        result = frameworkElement->add_SizeChanged(sizeChangedHandler.Get(), &mSizeChangedEventToken);
    }

    if (SUCCEEDED(result))
    {
        return true;
    }

    return false;
}

void SwapChainPanelNativeWindow::unregisterForSizeChangeEvents()
{
    ComPtr<ABI::Windows::UI::Xaml::IFrameworkElement> frameworkElement;
    if (SUCCEEDED(mSwapChainPanel.As(&frameworkElement)))
    {
        (void)frameworkElement->remove_SizeChanged(mSizeChangedEventToken);
    }

    mSizeChangedEventToken.value = 0;
}

HRESULT SwapChainPanelNativeWindow::createSwapChain(ID3D11Device *device, DXGIFactory *factory, DXGI_FORMAT format, unsigned int width, unsigned int height, DXGISwapChain **swapChain)
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
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    *swapChain = nullptr;

    ComPtr<IDXGISwapChain1> newSwapChain;
    ComPtr<ISwapChainPanelNative> swapChainPanelNative;
    RECT currentPanelSize = {};

    HRESULT result = factory->CreateSwapChainForComposition(device, &swapChainDesc, nullptr, newSwapChain.ReleaseAndGetAddressOf());

    if (SUCCEEDED(result))
    {
        result = mSwapChainPanel.As(&swapChainPanelNative);
    }

    if (SUCCEEDED(result))
    {
        result = swapChainPanelNative->SetSwapChain(newSwapChain.Get());
    }

    if (SUCCEEDED(result))
    {
        // The swapchain panel host requires an instance of the swapchain set on the SwapChainPanel
        // to perform the runtime-scale behavior.  This swapchain is cached here because there are
        // no methods for retreiving the currently configured on from ISwapChainPanelNative.
        mSwapChain = newSwapChain;
        result = newSwapChain.CopyTo(swapChain);
    }

    // If the host is responsible for scaling the output of the swapchain, then
    // scale it now before returning an instance to the caller.  This is done by
    // first reading the current size of the swapchain panel, then scaling
    if (SUCCEEDED(result) && mRequiresSwapChainScaling)
    {
        result = GetSwapChainPanelSize(mSwapChainPanel, &currentPanelSize);
    }

    // Scale the swapchain to fit inside the contents of the panel.
    if (SUCCEEDED(result) && mRequiresSwapChainScaling)
    {
        SIZE currentSize = { currentPanelSize.right, currentPanelSize.bottom };
        result = scaleSwapChain(currentSize);
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

HRESULT SwapChainPanelNativeWindow::scaleSwapChain(const SIZE &newSize)
{
    ABI::Windows::Foundation::Size renderScale = { (float)newSize.cx/(float)mClientRect.right, (float)newSize.cy/(float)mClientRect.bottom };
    // Setup a scale matrix for the swap chain
    DXGI_MATRIX_3X2_F scaleMatrix = {};
    scaleMatrix._11 = renderScale.Width;
    scaleMatrix._22 = renderScale.Height;

    ComPtr<IDXGISwapChain2> swapChain2;
    HRESULT result = mSwapChain.As(&swapChain2);
    if (SUCCEEDED(result))
    {
        result = swapChain2->SetMatrixTransform(&scaleMatrix);
    }

    return result;
}

HRESULT GetSwapChainPanelSize(const ComPtr<ABI::Windows::UI::Xaml::Controls::ISwapChainPanel> &swapChainPanel, RECT *windowSize)
{
    ComPtr<ABI::Windows::UI::Xaml::IUIElement> uiElement;
    ABI::Windows::Foundation::Size renderSize = { 0, 0 };
    HRESULT result = swapChainPanel.As(&uiElement);
    if (SUCCEEDED(result))
    {
        result = uiElement->get_RenderSize(&renderSize);
    }

    if (SUCCEEDED(result))
    {
        *windowSize = { 0, 0, lround(renderSize.Width), lround(renderSize.Height) };
    }

    return result;
}
}
