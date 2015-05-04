//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// CoreWindowNativeWindow.h: NativeWindow for managing ICoreWindow native window types.

#ifndef COMMON_WINRT_COREWINDOWNATIVEWINDOW_H_
#define COMMON_WINRT_COREWINDOWNATIVEWINDOW_H_

#include "common/winrt/InspectableNativeWindow.h"
#include <memory>
#include <windows.graphics.display.h>

namespace rx
{

class CoreWindowNativeWindow : public InspectableNativeWindow, public std::enable_shared_from_this<CoreWindowNativeWindow>
{
  public:
    ~CoreWindowNativeWindow();

    bool initialize(EGLNativeWindowType window, EGLNativeDisplayType display, IPropertySet *propertySet);
    bool registerForSizeChangeEvents();
    void unregisterForSizeChangeEvents();
    HRESULT createSwapChain(ID3D11Device *device, DXGIFactory *factory, DXGI_FORMAT format, unsigned int width, unsigned int height, DXGISwapChain **swapChain);

  private:
    HRESULT onSizeChanged(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IWindowSizeChangedEventArgs *);

    ComPtr<ABI::Windows::UI::Core::ICoreWindow> mCoreWindow;
    ComPtr<ABI::Windows::Graphics::Display::IDisplayInformation> mDisplayInformation;
    ComPtr<IMap<HSTRING, IInspectable*>> mPropertyMap;
};

}

#endif // COMMON_WINRT_COREWINDOWNATIVEWINDOW_H_
