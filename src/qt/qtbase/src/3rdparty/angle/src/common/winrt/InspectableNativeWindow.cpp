//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// InspectableNativeWindow.cpp: NativeWindow base class for managing IInspectable native window types.

#include "common/winrt/CoreWindowNativeWindow.h"
#include "common/winrt/SwapChainPanelNativeWindow.h"

namespace rx
{
NativeWindow::NativeWindow(EGLNativeWindowType window, EGLNativeDisplayType display)
    : mWindow(window), mDisplay(display)
{
}

bool NativeWindow::initialize()
{
    // If the native window type is a IPropertySet, extract the
    // EGLNativeWindowType (IInspectable) and initialize the
    // proper host with this IPropertySet.
    ComPtr<ABI::Windows::Foundation::Collections::IPropertySet> propertySet;
    ComPtr<IInspectable> eglNativeWindow;
    if (IsEGLConfiguredPropertySet(mWindow, &propertySet, &eglNativeWindow))
    {
        // A property set was found and the EGLNativeWindowType was
        // retrieved. The mWindow member of the host to must be updated
        // to use the EGLNativeWindowType specified in the property set.
        // mWindow is treated as a raw pointer not an AddRef'd interface, so
        // the old mWindow does not need a Release() before this assignment.
        mWindow = eglNativeWindow.Get();
    }

    ComPtr<ABI::Windows::UI::Core::ICoreWindow> coreWindow;
    ComPtr<ABI::Windows::UI::Xaml::Controls::ISwapChainPanel> swapChainPanel;
    if (IsCoreWindow(mWindow, &coreWindow))
    {
        mImpl = std::make_shared<CoreWindowNativeWindow>();
        if (mImpl)
        {
            return mImpl->initialize(mWindow, mDisplay, propertySet.Get());
        }
    }
    else if (IsSwapChainPanel(mWindow, &swapChainPanel))
    {
        mImpl = std::make_shared<SwapChainPanelNativeWindow>();
        if (mImpl)
        {
            return mImpl->initialize(mWindow, mDisplay, propertySet.Get());
        }
    }
    else
    {
        ERR("Invalid IInspectable EGLNativeWindowType detected. Valid IInspectables include ICoreWindow, ISwapChainPanel and IPropertySet");
    }

    return false;
}

bool NativeWindow::getClientRect(RECT *rect)
{
    if (mImpl)
    {
        return mImpl->getClientRect(rect);
    }

    return false;
}

bool NativeWindow::isIconic()
{
    return false;
}

HRESULT NativeWindow::createSwapChain(ID3D11Device *device, DXGIFactory *factory, DXGI_FORMAT format, unsigned int width, unsigned int height, DXGISwapChain **swapChain)
{
    if (mImpl)
    {
        return mImpl->createSwapChain(device, factory, format, width, height, swapChain);
    }

    return E_UNEXPECTED;
}

bool IsCoreWindow(EGLNativeWindowType window, ComPtr<ABI::Windows::UI::Core::ICoreWindow> *coreWindow)
{
    if (!window)
    {
        return false;
    }

    ComPtr<IInspectable> win = window;
    ComPtr<ABI::Windows::UI::Core::ICoreWindow> coreWin;
    if (SUCCEEDED(win.As(&coreWin)))
    {
        if (coreWindow != nullptr)
        {
            *coreWindow = coreWin.Detach();
        }
        return true;
    }

    return false;
}

bool IsSwapChainPanel(EGLNativeWindowType window, ComPtr<ABI::Windows::UI::Xaml::Controls::ISwapChainPanel> *swapChainPanel)
{
    if (!window)
    {
        return false;
    }

    ComPtr<IInspectable> win = window;
    ComPtr<ABI::Windows::UI::Xaml::Controls::ISwapChainPanel> panel;
    if (SUCCEEDED(win.As(&panel)))
    {
        if (swapChainPanel != nullptr)
        {
            *swapChainPanel = panel.Detach();
        }
        return true;
    }

    return false;
}

bool IsEGLConfiguredPropertySet(EGLNativeWindowType window, ABI::Windows::Foundation::Collections::IPropertySet **propertySet, IInspectable **eglNativeWindow)
{
    if (!window)
    {
        return false;
    }

    ComPtr<IInspectable> props = window;
    ComPtr<IPropertySet> propSet;
    ComPtr<IInspectable> nativeWindow;
    ComPtr<ABI::Windows::Foundation::Collections::IMap<HSTRING, IInspectable*>> propMap;
    boolean hasEglNativeWindowPropertyKey = false;

    HRESULT result = props.As(&propSet);
    if (SUCCEEDED(result))
    {
        result = propSet.As(&propMap);
    }

    // Look for the presence of the EGLNativeWindowType in the property set
    if (SUCCEEDED(result))
    {
        result = propMap->HasKey(HStringReference(EGLNativeWindowTypeProperty).Get(), &hasEglNativeWindowPropertyKey);
    }

    // If the IPropertySet does not contain the required EglNativeWindowType key, the property set is
    // considered invalid.
    if (SUCCEEDED(result) && !hasEglNativeWindowPropertyKey)
    {
        ERR("Could not find EGLNativeWindowTypeProperty in IPropertySet. Valid EGLNativeWindowTypeProperty values include ICoreWindow");
        return false;
    }

    // The EglNativeWindowType property exists, so retreive the IInspectable that represents the EGLNativeWindowType
    if (SUCCEEDED(result) && hasEglNativeWindowPropertyKey)
    {
        result = propMap->Lookup(HStringReference(EGLNativeWindowTypeProperty).Get(), &nativeWindow);
    }

    if (SUCCEEDED(result))
    {
        if (propertySet != nullptr)
        {
            result = propSet.CopyTo(propertySet);
        }
    }

    if (SUCCEEDED(result))
    {
        if (eglNativeWindow != nullptr)
        {
            result = nativeWindow.CopyTo(eglNativeWindow);
        }
    }

    if (SUCCEEDED(result))
    {
        return true;
    }

    return false;
}

// A Valid EGLNativeWindowType IInspectable can only be:
//
// ICoreWindow
// IPropertySet
// 
// Anything else will be rejected as an invalid IInspectable.
bool IsValidEGLNativeWindowType(EGLNativeWindowType window)
{
    return IsCoreWindow(window) || IsSwapChainPanel(window) || IsEGLConfiguredPropertySet(window);
}

// Attempts to read an optional SIZE property value that is assumed to be in the form of
// an ABI::Windows::Foundation::Size.  This function validates the Size value before returning 
// it to the caller.
//
// Possible return values are:
// S_OK, valueExists == true - optional SIZE value was successfully retrieved and validated
// S_OK, valueExists == false - optional SIZE value was not found
// E_INVALIDARG, valueExists = false - optional SIZE value was malformed in the property set.
//    * Incorrect property type ( must be PropertyType_Size)
//    * Invalid property value (width/height must be > 0)
// Additional errors may be returned from IMap or IPropertyValue
//
HRESULT GetOptionalSizePropertyValue(const ComPtr<ABI::Windows::Foundation::Collections::IMap<HSTRING, IInspectable*>>& propertyMap, const wchar_t *propertyName, SIZE *value, bool *valueExists)
{
    if (!propertyMap || !propertyName || !value || !valueExists)
    {
        return false;
    }

    // Assume that the value does not exist
    *valueExists = false;
    *value = { 0, 0 };

    ComPtr<ABI::Windows::Foundation::IPropertyValue> propertyValue;
    ABI::Windows::Foundation::PropertyType propertyType = ABI::Windows::Foundation::PropertyType::PropertyType_Empty;
    Size sizeValue = { 0, 0 };
    boolean hasKey = false;

    HRESULT result = propertyMap->HasKey(HStringReference(propertyName).Get(), &hasKey);
    if (SUCCEEDED(result) && !hasKey)
    {
        // Value does not exist, so return S_OK and set the exists parameter to false to indicate
        // that a the optional property does not exist.
        *valueExists = false;
        return S_OK;
    }

    if (SUCCEEDED(result))
    {
        result = propertyMap->Lookup(HStringReference(propertyName).Get(), &propertyValue);
    }

    if (SUCCEEDED(result))
    {
        result = propertyValue->get_Type(&propertyType);
    }

    // Check if the expected Size property is of PropertyType_Size type.
    if (SUCCEEDED(result) && propertyType == ABI::Windows::Foundation::PropertyType::PropertyType_Size)
    {
        if (SUCCEEDED(propertyValue->GetSize(&sizeValue)) && (sizeValue.Width > 0 && sizeValue.Height > 0))
        {
            // A valid property value exists
            *value = { static_cast<long>(sizeValue.Width), static_cast<long>(sizeValue.Height) };
            *valueExists = true;
            result = S_OK;
        }
        else
        {
            // An invalid Size property was detected. Width/Height values must > 0
            result = E_INVALIDARG;
        }
    }
    else
    {
        // An invalid property type was detected. Size property must be of PropertyType_Size
        result = E_INVALIDARG;
    }

    return result;
}
}
