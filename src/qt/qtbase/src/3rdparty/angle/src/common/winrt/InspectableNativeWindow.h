//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// InspectableNativeWindow.h: Host specific implementation interface for
// managing IInspectable native window types.

#ifndef COMMON_WINRT_INSPECTABLENATIVEWINDOW_H_
#define COMMON_WINRT_INSPECTABLENATIVEWINDOW_H_

#include "common/platform.h"
#include "common/NativeWindow.h"
#include "angle_windowsstore.h"

#include <windows.ui.xaml.h>
#include <windows.ui.xaml.media.dxinterop.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;

namespace rx
{
class InspectableNativeWindow
{
  public:
    InspectableNativeWindow() :
        mSupportsSwapChainResize(true),
        mRequiresSwapChainScaling(false),
        mClientRectChanged(false),
        mClientRect({0,0,0,0}),
        mNewClientRect({0,0,0,0}),
        mScaleFactor(1.0)
    {
        mSizeChangedEventToken.value = 0;
    }
    virtual ~InspectableNativeWindow(){}

    virtual bool initialize(EGLNativeWindowType window, EGLNativeDisplayType display, IPropertySet *propertySet) = 0;
    virtual HRESULT createSwapChain(ID3D11Device *device, DXGIFactory *factory, DXGI_FORMAT format, unsigned int width, unsigned int height, DXGISwapChain **swapChain) = 0;
    virtual bool registerForSizeChangeEvents() = 0;
    virtual void unregisterForSizeChangeEvents() = 0;
    virtual HRESULT scaleSwapChain(const SIZE& newSize) { return S_OK; }

    bool getClientRect(RECT *rect)
    {
        if (mClientRectChanged && mSupportsSwapChainResize)
        {
            mClientRect = mNewClientRect;
            mClientRectChanged = false;
        }

        *rect = mClientRect;

        return true;
    }

    void setNewClientSize(const SIZE &newSize)
    {
        if (mSupportsSwapChainResize && !mRequiresSwapChainScaling)
        {
            mNewClientRect = { 0, 0, newSize.cx, newSize.cy };
            mClientRectChanged = true;
        }

        if (mRequiresSwapChainScaling)
        {
            scaleSwapChain(newSize);
        }
    }

protected:
    bool mSupportsSwapChainResize;
    bool mRequiresSwapChainScaling;
    RECT mClientRect;
    RECT mNewClientRect;
    bool mClientRectChanged;
    DOUBLE mScaleFactor;

    EventRegistrationToken mSizeChangedEventToken;
};

bool IsCoreWindow(EGLNativeWindowType window, ComPtr<ABI::Windows::UI::Core::ICoreWindow> *coreWindow = nullptr);
bool IsSwapChainPanel(EGLNativeWindowType window, ComPtr<ABI::Windows::UI::Xaml::Controls::ISwapChainPanel> *swapChainPanel = nullptr);
bool IsEGLConfiguredPropertySet(EGLNativeWindowType window, ABI::Windows::Foundation::Collections::IPropertySet **propertySet = nullptr, IInspectable **inspectable = nullptr);
HRESULT GetOptionalSizePropertyValue(const ComPtr<ABI::Windows::Foundation::Collections::IMap<HSTRING, IInspectable*>>& propertyMap, const wchar_t *propertyName, SIZE *value, bool *valueExists);
}
#endif // COMMON_WINRT_INSPECTABLENATIVEWINDOW_H_
