/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwinrtinputcontext.h"
#include <QtGui/QWindow>

#include <wrl.h>
#include <roapi.h>
#include <windows.ui.viewmanagement.h>
#include <windows.ui.core.h>
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::UI::ViewManagement;
using namespace ABI::Windows::UI::Core;

#if defined(Q_OS_WINPHONE) && _MSC_VER==1700
#include <windows.phone.ui.core.h>
using namespace ABI::Windows::Phone::UI::Core;
#endif

typedef ITypedEventHandler<InputPane*, InputPaneVisibilityEventArgs*> InputPaneVisibilityHandler;

QT_BEGIN_NAMESPACE

/*!
    \class QWinRTInputContext
    \brief Manages Input Method visibility
    \internal
    \ingroup qt-qpa-winrt

    Listens to the native virtual keyboard for hide/show events and provides
    hints to the OS for showing/hiding. On WinRT, showInputPanel()/hideInputPanel()
    have no effect because WinRT dictates that keyboard presence is user-driven:
    (http://msdn.microsoft.com/en-us/library/windows/apps/hh465404.aspx)
    Windows Phone, however, supports direct hiding/showing of the keyboard.
*/

QWinRTInputContext::QWinRTInputContext(ICoreWindow *window)
    : m_window(window)
{
    IInputPaneStatics *statics;
    if (FAILED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_ViewManagement_InputPane).Get(),
                                    &statics))) {
        qWarning(Q_FUNC_INFO ": failed to retrieve input pane statics.");
        return;
    }

    IInputPane *inputPane;
    statics->GetForCurrentView(&inputPane);
    statics->Release();
    if (inputPane) {
        EventRegistrationToken showToken, hideToken;
        inputPane->add_Showing(Callback<InputPaneVisibilityHandler>(
                                   this, &QWinRTInputContext::onShowing).Get(), &showToken);
        inputPane->add_Hiding(Callback<InputPaneVisibilityHandler>(
                                  this, &QWinRTInputContext::onHiding).Get(), &hideToken);

        Rect rect;
        inputPane->get_OccludedRect(&rect);
        m_keyboardRect = QRectF(rect.X, rect.Y, rect.Width, rect.Height);
        m_isInputPanelVisible = !m_keyboardRect.isEmpty();
    } else {
        qWarning(Q_FUNC_INFO ": failed to retrieve InputPane.");
    }
}

QRectF QWinRTInputContext::keyboardRect() const
{
    return m_keyboardRect;
}

bool QWinRTInputContext::isInputPanelVisible() const
{
    return m_isInputPanelVisible;
}

HRESULT QWinRTInputContext::onShowing(IInputPane *pane, IInputPaneVisibilityEventArgs *)
{
    m_isInputPanelVisible = true;
    emitInputPanelVisibleChanged();

    Rect rect;
    pane->get_OccludedRect(&rect);
    setKeyboardRect(QRectF(rect.X, rect.Y, rect.Width, rect.Height));

    return S_OK;
}

HRESULT QWinRTInputContext::onHiding(IInputPane *pane, IInputPaneVisibilityEventArgs *)
{
    m_isInputPanelVisible = false;
    emitInputPanelVisibleChanged();

    Rect rect;
    pane->get_OccludedRect(&rect);
    setKeyboardRect(QRectF(rect.X, rect.Y, rect.Width, rect.Height));

    return S_OK;
}

void QWinRTInputContext::setKeyboardRect(const QRectF rect)
{
    if (m_keyboardRect == rect)
        return;

    m_keyboardRect = rect;
    emitKeyboardRectChanged();
}

#ifdef Q_OS_WINPHONE

#if _MSC_VER>1700 // Windows Phone 8.1+
static HRESULT getInputPane(ComPtr<IInputPane2> *inputPane2)
{
    ComPtr<IInputPaneStatics> factory;
    HRESULT hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_ViewManagement_InputPane).Get(),
                                      &factory);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to get input pane factory.");
        return hr;
    }

    ComPtr<IInputPane> inputPane;
    hr = factory->GetForCurrentView(&inputPane);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to get input pane.");
        return hr;
    }

    hr = inputPane.As(inputPane2);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to get extended input pane.");
        return hr;
    }
    return hr;
}
#endif // _MSC_VER>1700

void QWinRTInputContext::showInputPanel()
{
#if _MSC_VER<=1700 // Windows Phone 8.0
    ICoreWindowKeyboardInput *input;
    if (SUCCEEDED(m_window->QueryInterface(IID_PPV_ARGS(&input)))) {
        input->put_IsKeyboardInputEnabled(true);
        input->Release();
    }
#else // _MSC_VER<=1700
    ComPtr<IInputPane2> inputPane;
    HRESULT hr = getInputPane(&inputPane);
    if (FAILED(hr))
        return;

    boolean success;
    hr = inputPane->TryShow(&success);
    if (FAILED(hr))
        qErrnoWarning(hr, "Failed to show input panel.");
#endif // _MSC_VER>1700
}

void QWinRTInputContext::hideInputPanel()
{
#if _MSC_VER<=1700 // Windows Phone 8.0
    ICoreWindowKeyboardInput *input;
    if (SUCCEEDED(m_window->QueryInterface(IID_PPV_ARGS(&input)))) {
        input->put_IsKeyboardInputEnabled(false);
        input->Release();
    }
#else // _MSC_VER<=1700
    ComPtr<IInputPane2> inputPane;
    HRESULT hr = getInputPane(&inputPane);
    if (FAILED(hr))
        return;

    boolean success;
    hr = inputPane->TryHide(&success);
    if (FAILED(hr))
        qErrnoWarning(hr, "Failed to hide input panel.");
#endif // _MSC_VER>1700
}

#else // Q_OS_WINPHONE

// IRawElementProviderSimple
HRESULT QWinRTInputContext::get_ProviderOptions(ProviderOptions *retVal)
{
    *retVal = ProviderOptions_ServerSideProvider|ProviderOptions_UseComThreading;
    return S_OK;
}

HRESULT QWinRTInputContext::GetPatternProvider(PATTERNID id, IUnknown **retVal)
{
    switch (id) {
    case 10002: //UIA_ValuePatternId
        return QueryInterface(__uuidof(IValueProvider), (void**)retVal);
        break;
    case 10014: //UIA_TextPatternId:
        return QueryInterface(__uuidof(ITextProvider), (void**)retVal);
    case 10029: //UIA_TextChildPatternId:
        *retVal = nullptr;
        break;
    default:
        qWarning("Unhandled pattern ID: %d", id);
        break;
    }
    return S_OK;
}

HRESULT QWinRTInputContext::GetPropertyValue(PROPERTYID idProp, VARIANT *retVal)
{
    switch (idProp) {
    case 30003: //UIA_ControlTypePropertyId
        retVal->vt = VT_I4;
        retVal->lVal = 50025; //UIA_CustomControlTypeId
        break;
    case 30008: //UIA_IsKeyboardFocusablePropertyId
    case 30009: //UIA_HasKeyboardFocusPropertyId
        // These are probably never actually called
    case 30016: //UIA_IsControlElementPropertyId
    case 30017: //UIA_IsContentElementPropertyId
        retVal->vt = VT_BOOL;
        retVal->boolVal = VARIANT_TRUE;
        break;
    case 30019: //UIA_IsPasswordPropertyId
        retVal->vt = VT_BOOL;
        retVal->boolVal = VARIANT_FALSE;
        break;
    case 30020: //UIA_NativeWindowHandlePropertyId
        retVal->vt = VT_PTR;
        retVal->punkVal = m_window;
        break;
    }
    return S_OK;
}

HRESULT QWinRTInputContext::get_HostRawElementProvider(IRawElementProviderSimple **retVal)
{
    // Return the window's element provider
    IInspectable *hostProvider;
    HRESULT hr = m_window->get_AutomationHostProvider(&hostProvider);
    if (SUCCEEDED(hr)) {
        hr = hostProvider->QueryInterface(IID_PPV_ARGS(retVal));
        hostProvider->Release();
    }
    return hr;
}

// ITextProvider
HRESULT QWinRTInputContext::GetSelection(SAFEARRAY **)
{
    // To be useful, requires listening to the focus object for a selection change and raising an event
    return S_OK;
}

HRESULT QWinRTInputContext::GetVisibleRanges(SAFEARRAY **)
{
    // To be useful, requires listening to the focus object for a selection change and raising an event
    return S_OK;
}

HRESULT QWinRTInputContext::RangeFromChild(IRawElementProviderSimple *,ITextRangeProvider **)
{
    // To be useful, requires listening to the focus object for a selection change and raising an event
    return S_OK;
}

HRESULT QWinRTInputContext::RangeFromPoint(UiaPoint, ITextRangeProvider **)
{
    // To be useful, requires listening to the focus object for a selection change and raising an event
    return S_OK;
}

HRESULT QWinRTInputContext::get_DocumentRange(ITextRangeProvider **)
{
    // To be useful, requires listening to the focus object for a selection change and raising an event
    return S_OK;
}

HRESULT QWinRTInputContext::get_SupportedTextSelection(SupportedTextSelection *)
{
    // To be useful, requires listening to the focus object for a selection change and raising an event
    return S_OK;
}

// IValueProvider
HRESULT QWinRTInputContext::SetValue(LPCWSTR)
{
    // To be useful, requires listening to the focus object for a value change and raising an event
    // May be useful for inputPanel autocomplete, etc.
    return S_OK;
}

HRESULT QWinRTInputContext::get_Value(BSTR *)
{
    // To be useful, requires listening to the focus object for a value change and raising an event
    // May be useful for inputPanel autocomplete, etc.
    return S_OK;
}

HRESULT QWinRTInputContext::get_IsReadOnly(BOOL *isReadOnly)
{
    // isReadOnly dictates keyboard opening behavior when view is tapped.
    // We need to decide if the user tapped within a control which is about to receive focus...
    // Since this isn't possible (this function gets called before we receive the touch event),
    // the most platform-aligned option is to show the keyboard if an editable item has focus,
    // and close the keyboard if it is already open.
    *isReadOnly = m_isInputPanelVisible || !inputMethodAccepted();
    return S_OK;
}

#endif // !Q_OS_WINPHONE

QT_END_NAMESPACE
