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

#include "qwinrtscreen.h"

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#ifndef Q_OS_WINPHONE
#include <dxgi1_3.h>
#endif

#include "qwinrtbackingstore.h"
#include "qwinrtinputcontext.h"
#include "qwinrtcursor.h"
#include "qwinrteglcontext.h"

#include <QtGui/QSurfaceFormat>
#include <QtGui/QGuiApplication>
#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qt_windows.h>
#include <QtCore/qfunctions_winrt.h>

#include <wrl.h>
#include <windows.system.h>
#include <Windows.Applicationmodel.h>
#include <Windows.ApplicationModel.core.h>
#include <windows.devices.input.h>
#include <windows.ui.h>
#include <windows.ui.core.h>
#include <windows.ui.input.h>
#include <windows.ui.viewmanagement.h>
#include <windows.graphics.display.h>
#include <windows.foundation.h>
#ifdef Q_OS_WINPHONE
#include <windows.phone.ui.input.h>
#endif

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::ApplicationModel;
using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::System;
using namespace ABI::Windows::UI::Core;
using namespace ABI::Windows::UI::Input;
using namespace ABI::Windows::UI::ViewManagement;
using namespace ABI::Windows::Devices::Input;
using namespace ABI::Windows::Graphics::Display;
#ifdef Q_OS_WINPHONE
using namespace ABI::Windows::Phone::UI::Input;
#endif

typedef IEventHandler<IInspectable*> ResumeHandler;
typedef IEventHandler<SuspendingEventArgs*> SuspendHandler;
typedef ITypedEventHandler<CoreWindow*, WindowActivatedEventArgs*> ActivatedHandler;
typedef ITypedEventHandler<CoreWindow*, CoreWindowEventArgs*> ClosedHandler;
typedef ITypedEventHandler<CoreWindow*, CharacterReceivedEventArgs*> CharacterReceivedHandler;
typedef ITypedEventHandler<CoreWindow*, InputEnabledEventArgs*> InputEnabledHandler;
typedef ITypedEventHandler<CoreWindow*, KeyEventArgs*> KeyHandler;
typedef ITypedEventHandler<CoreWindow*, PointerEventArgs*> PointerHandler;
typedef ITypedEventHandler<CoreWindow*, WindowSizeChangedEventArgs*> SizeChangedHandler;
typedef ITypedEventHandler<CoreWindow*, VisibilityChangedEventArgs*> VisibilityChangedHandler;
typedef ITypedEventHandler<CoreWindow*, AutomationProviderRequestedEventArgs*> AutomationProviderRequestedHandler;
typedef ITypedEventHandler<DisplayInformation*, IInspectable*> DisplayInformationHandler;
#ifdef Q_OS_WINPHONE
typedef IEventHandler<BackPressedEventArgs*> BackPressedHandler;
#endif

QT_BEGIN_NAMESPACE

static inline Qt::ScreenOrientations qtOrientationsFromNative(DisplayOrientations native)
{
    Qt::ScreenOrientations orientations = Qt::PrimaryOrientation;
    if (native & DisplayOrientations_Portrait)
        orientations |= Qt::PortraitOrientation;
    if (native & DisplayOrientations_PortraitFlipped)
        orientations |= Qt::InvertedPortraitOrientation;
    if (native & DisplayOrientations_Landscape)
        orientations |= Qt::LandscapeOrientation;
    if (native & DisplayOrientations_LandscapeFlipped)
        orientations |= Qt::InvertedLandscapeOrientation;
    return orientations;
}

static inline DisplayOrientations nativeOrientationsFromQt(Qt::ScreenOrientations orientation)
{
    DisplayOrientations native = DisplayOrientations_None;
    if (orientation & Qt::PortraitOrientation)
        native |= DisplayOrientations_Portrait;
    if (orientation & Qt::InvertedPortraitOrientation)
        native |= DisplayOrientations_PortraitFlipped;
    if (orientation & Qt::LandscapeOrientation)
        native |= DisplayOrientations_Landscape;
    if (orientation & Qt::InvertedLandscapeOrientation)
        native |= DisplayOrientations_LandscapeFlipped;
    return native;
}

static inline bool qIsNonPrintable(quint32 keyCode)
{
    switch (keyCode) {
    case '\b':
    case '\n':
    case '\t':
    case '\r':
    case '\v':
    case '\f':
        return true;
    default:
        return false;
    }
}

// Return Qt meta key from VirtualKey
static inline Qt::Key qKeyFromVirtual(VirtualKey key)
{
    switch (key) {

    default:
        return Qt::Key_unknown;

    // Non-printable characters
    case VirtualKey_Enter:
        return Qt::Key_Enter;
    case VirtualKey_Tab:
        return Qt::Key_Tab;
    case VirtualKey_Back:
        return Qt::Key_Backspace;

    // Modifiers
    case VirtualKey_Shift:
    case VirtualKey_LeftShift:
    case VirtualKey_RightShift:
        return Qt::Key_Shift;
    case VirtualKey_Control:
    case VirtualKey_LeftControl:
    case VirtualKey_RightControl:
        return Qt::Key_Control;
    case VirtualKey_Menu:
    case VirtualKey_LeftMenu:
    case VirtualKey_RightMenu:
        return Qt::Key_Alt;
    case VirtualKey_LeftWindows:
    case VirtualKey_RightWindows:
        return Qt::Key_Meta;

    // Toggle keys
    case VirtualKey_CapitalLock:
        return Qt::Key_CapsLock;
    case VirtualKey_NumberKeyLock:
        return Qt::Key_NumLock;
    case VirtualKey_Scroll:
        return Qt::Key_ScrollLock;

    // East-Asian language keys
    case VirtualKey_Kana:
    //case VirtualKey_Hangul: // Same enum as Kana
        return Qt::Key_Kana_Shift;
    case VirtualKey_Junja:
        return Qt::Key_Hangul_Jeonja;
    case VirtualKey_Kanji:
    //case VirtualKey_Hanja: // Same enum as Kanji
        return Qt::Key_Kanji;
    case VirtualKey_ModeChange:
        return Qt::Key_Mode_switch;
    case VirtualKey_Convert:
        return Qt::Key_Henkan;
    case VirtualKey_NonConvert:
        return Qt::Key_Muhenkan;

    // Misc. keys
    case VirtualKey_Cancel:
        return Qt::Key_Cancel;
    case VirtualKey_Clear:
        return Qt::Key_Clear;
    case VirtualKey_Application:
        return Qt::Key_ApplicationLeft;
    case VirtualKey_Sleep:
        return Qt::Key_Sleep;
    case VirtualKey_Pause:
        return Qt::Key_Pause;
    case VirtualKey_PageUp:
        return Qt::Key_PageUp;
    case VirtualKey_PageDown:
        return Qt::Key_PageDown;
    case VirtualKey_End:
        return Qt::Key_End;
    case VirtualKey_Home:
        return Qt::Key_Home;
    case VirtualKey_Left:
        return Qt::Key_Left;
    case VirtualKey_Up:
        return Qt::Key_Up;
    case VirtualKey_Right:
        return Qt::Key_Right;
    case VirtualKey_Down:
        return Qt::Key_Down;
    case VirtualKey_Select:
        return Qt::Key_Select;
    case VirtualKey_Print:
        return Qt::Key_Print;
    case VirtualKey_Execute:
        return Qt::Key_Execute;
    case VirtualKey_Insert:
        return Qt::Key_Insert;
    case VirtualKey_Delete:
        return Qt::Key_Delete;
    case VirtualKey_Help:
        return Qt::Key_Help;
    case VirtualKey_Snapshot:
        return Qt::Key_Camera;
    case VirtualKey_Escape:
        return Qt::Key_Escape;

    // Function Keys
    case VirtualKey_F1:
        return Qt::Key_F1;
    case VirtualKey_F2:
        return Qt::Key_F2;
    case VirtualKey_F3:
        return Qt::Key_F3;
    case VirtualKey_F4:
        return Qt::Key_F4;
    case VirtualKey_F5:
        return Qt::Key_F5;
    case VirtualKey_F6:
        return Qt::Key_F6;
    case VirtualKey_F7:
        return Qt::Key_F7;
    case VirtualKey_F8:
        return Qt::Key_F8;
    case VirtualKey_F9:
        return Qt::Key_F9;
    case VirtualKey_F10:
        return Qt::Key_F10;
    case VirtualKey_F11:
        return Qt::Key_F11;
    case VirtualKey_F12:
        return Qt::Key_F12;
    case VirtualKey_F13:
        return Qt::Key_F13;
    case VirtualKey_F14:
        return Qt::Key_F14;
    case VirtualKey_F15:
        return Qt::Key_F15;
    case VirtualKey_F16:
        return Qt::Key_F16;
    case VirtualKey_F17:
        return Qt::Key_F17;
    case VirtualKey_F18:
        return Qt::Key_F18;
    case VirtualKey_F19:
        return Qt::Key_F19;
    case VirtualKey_F20:
        return Qt::Key_F20;
    case VirtualKey_F21:
        return Qt::Key_F21;
    case VirtualKey_F22:
        return Qt::Key_F22;
    case VirtualKey_F23:
        return Qt::Key_F23;
    case VirtualKey_F24:
        return Qt::Key_F24;

    // Character keys
    case VirtualKey_Space:
        return Qt::Key_Space;
    case VirtualKey_Number0:
    case VirtualKey_NumberPad0:
        return Qt::Key_0;
    case VirtualKey_Number1:
    case VirtualKey_NumberPad1:
        return Qt::Key_1;
    case VirtualKey_Number2:
    case VirtualKey_NumberPad2:
        return Qt::Key_2;
    case VirtualKey_Number3:
    case VirtualKey_NumberPad3:
        return Qt::Key_3;
    case VirtualKey_Number4:
    case VirtualKey_NumberPad4:
        return Qt::Key_4;
    case VirtualKey_Number5:
    case VirtualKey_NumberPad5:
        return Qt::Key_5;
    case VirtualKey_Number6:
    case VirtualKey_NumberPad6:
        return Qt::Key_6;
    case VirtualKey_Number7:
    case VirtualKey_NumberPad7:
        return Qt::Key_7;
    case VirtualKey_Number8:
    case VirtualKey_NumberPad8:
        return Qt::Key_8;
    case VirtualKey_Number9:
    case VirtualKey_NumberPad9:
        return Qt::Key_9;
    case VirtualKey_A:
        return Qt::Key_A;
    case VirtualKey_B:
        return Qt::Key_B;
    case VirtualKey_C:
        return Qt::Key_C;
    case VirtualKey_D:
        return Qt::Key_D;
    case VirtualKey_E:
        return Qt::Key_E;
    case VirtualKey_F:
        return Qt::Key_F;
    case VirtualKey_G:
        return Qt::Key_G;
    case VirtualKey_H:
        return Qt::Key_H;
    case VirtualKey_I:
        return Qt::Key_I;
    case VirtualKey_J:
        return Qt::Key_J;
    case VirtualKey_K:
        return Qt::Key_K;
    case VirtualKey_L:
        return Qt::Key_L;
    case VirtualKey_M:
        return Qt::Key_M;
    case VirtualKey_N:
        return Qt::Key_N;
    case VirtualKey_O:
        return Qt::Key_O;
    case VirtualKey_P:
        return Qt::Key_P;
    case VirtualKey_Q:
        return Qt::Key_Q;
    case VirtualKey_R:
        return Qt::Key_R;
    case VirtualKey_S:
        return Qt::Key_S;
    case VirtualKey_T:
        return Qt::Key_T;
    case VirtualKey_U:
        return Qt::Key_U;
    case VirtualKey_V:
        return Qt::Key_V;
    case VirtualKey_W:
        return Qt::Key_W;
    case VirtualKey_X:
        return Qt::Key_X;
    case VirtualKey_Y:
        return Qt::Key_Y;
    case VirtualKey_Z:
        return Qt::Key_Z;
    case VirtualKey_Multiply:
        return Qt::Key_9;
    case VirtualKey_Add:
        return Qt::Key_9;
    case VirtualKey_Separator:
        return Qt::Key_9;
    case VirtualKey_Subtract:
        return Qt::Key_9;
    case VirtualKey_Decimal:
        return Qt::Key_9;
    case VirtualKey_Divide:
        return Qt::Key_9;

    /* Keys with no matching Qt enum (?)
    case VirtualKey_None:
    case VirtualKey_LeftButton:
    case VirtualKey_RightButton:
    case VirtualKey_MiddleButton:
    case VirtualKey_XButton1:
    case VirtualKey_XButton2:
    case VirtualKey_Final:
    case VirtualKey_Accept:*/
    }
}

static inline Qt::Key qKeyFromCode(quint32 code, int mods)
{
    if (code >= 'a' && code <= 'z')
        code = toupper(code);
    if ((mods & Qt::ControlModifier) != 0) {
        if (code >= 0 && code <= 31)              // Ctrl+@..Ctrl+A..CTRL+Z..Ctrl+_
            code += '@';                       // to @..A..Z.._
    }
    return static_cast<Qt::Key>(code & 0xff);
}

typedef HRESULT (__stdcall ICoreApplication::*CoreApplicationCallbackRemover)(EventRegistrationToken);
uint qHash(CoreApplicationCallbackRemover key) { void *ptr = *(void **)(&key); return qHash(ptr); }
typedef HRESULT (__stdcall ICoreWindow::*CoreWindowCallbackRemover)(EventRegistrationToken);
uint qHash(CoreWindowCallbackRemover key) { void *ptr = *(void **)(&key); return qHash(ptr); }
typedef HRESULT (__stdcall IDisplayInformation::*DisplayCallbackRemover)(EventRegistrationToken);
uint qHash(DisplayCallbackRemover key) { void *ptr = *(void **)(&key); return qHash(ptr); }
#ifdef Q_OS_WINPHONE
typedef HRESULT (__stdcall IHardwareButtonsStatics::*HardwareButtonsCallbackRemover)(EventRegistrationToken);
uint qHash(HardwareButtonsCallbackRemover key) { void *ptr = *(void **)(&key); return qHash(ptr); }
#endif

class QWinRTScreenPrivate
{
public:
    ComPtr<ICoreApplication> application;
    ComPtr<ICoreWindow> coreWindow;
    ComPtr<IDisplayInformation> displayInformation;
#ifdef Q_OS_WINPHONE
    ComPtr<IHardwareButtonsStatics> hardwareButtons;
#endif

    QScopedPointer<QWinRTCursor> cursor;
#ifdef Q_OS_WINPHONE
    QScopedPointer<QWinRTInputContext> inputContext;
#else
    ComPtr<QWinRTInputContext> inputContext;
#endif

    QSizeF logicalSize;
    QSurfaceFormat surfaceFormat;
    qreal logicalDpi;
    QDpi physicalDpi;
    qreal scaleFactor;
    Qt::ScreenOrientation nativeOrientation;
    Qt::ScreenOrientation orientation;
    QList<QWindow *> visibleWindows;
#ifndef Q_OS_WINPHONE
    QHash<quint32, QPair<Qt::Key, QString>> activeKeys;
#endif
    QTouchDevice *touchDevice;
    QHash<quint32, QWindowSystemInterface::TouchPoint> touchPoints;

    EGLDisplay eglDisplay;
    EGLSurface eglSurface;
    EGLConfig eglConfig;

    QHash<CoreApplicationCallbackRemover, EventRegistrationToken> applicationTokens;
    QHash<CoreWindowCallbackRemover, EventRegistrationToken> windowTokens;
    QHash<DisplayCallbackRemover, EventRegistrationToken> displayTokens;
#ifdef Q_OS_WINPHONE
    QHash<HardwareButtonsCallbackRemover, EventRegistrationToken> buttonsTokens;
#endif
};

QWinRTScreen::QWinRTScreen()
    : d_ptr(new QWinRTScreenPrivate)
{
    Q_D(QWinRTScreen);
    d->orientation = Qt::PrimaryOrientation;
    d->touchDevice = Q_NULLPTR;
    d->eglDisplay = EGL_NO_DISPLAY;

    // Obtain the WinRT Application, view, and window
    HRESULT hr;
    hr = RoGetActivationFactory(Wrappers::HString::MakeReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(),
                                IID_PPV_ARGS(&d->application));
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->application->add_Suspending(Callback<SuspendHandler>(this, &QWinRTScreen::onSuspended).Get(), &d->applicationTokens[&ICoreApplication::remove_Resuming]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->application->add_Resuming(Callback<ResumeHandler>(this, &QWinRTScreen::onResume).Get(), &d->applicationTokens[&ICoreApplication::remove_Resuming]);
    Q_ASSERT_SUCCEEDED(hr);

    ComPtr<ICoreApplicationView> view;
    hr = d->application->GetCurrentView(&view);
    Q_ASSERT_SUCCEEDED(hr);
    hr = view->get_CoreWindow(&d->coreWindow);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->Activate();
    Q_ASSERT_SUCCEEDED(hr);

#ifdef Q_OS_WINPHONE
    d->inputContext.reset(new QWinRTInputContext(d->coreWindow.Get()));
#else
    d->inputContext = Make<QWinRTInputContext>(d->coreWindow.Get());
#endif

    Rect rect;
    hr = d->coreWindow->get_Bounds(&rect);
    Q_ASSERT_SUCCEEDED(hr);
    d->logicalSize = QSizeF(rect.Width, rect.Height);

    d->surfaceFormat.setAlphaBufferSize(0);
    d->surfaceFormat.setRedBufferSize(8);
    d->surfaceFormat.setGreenBufferSize(8);
    d->surfaceFormat.setBlueBufferSize(8);
    d->surfaceFormat.setDepthBufferSize(24);
    d->surfaceFormat.setStencilBufferSize(8);
    d->surfaceFormat.setRenderableType(QSurfaceFormat::OpenGLES);
    d->surfaceFormat.setSamples(1);
    d->surfaceFormat.setSwapBehavior(QSurfaceFormat::DoubleBuffer);

    hr = d->coreWindow->add_KeyDown(Callback<KeyHandler>(this, &QWinRTScreen::onKeyDown).Get(), &d->windowTokens[&ICoreWindow::remove_KeyDown]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_KeyUp(Callback<KeyHandler>(this, &QWinRTScreen::onKeyUp).Get(), &d->windowTokens[&ICoreWindow::remove_KeyUp]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_CharacterReceived(Callback<CharacterReceivedHandler>(this, &QWinRTScreen::onCharacterReceived).Get(), &d->windowTokens[&ICoreWindow::remove_CharacterReceived]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_PointerEntered(Callback<PointerHandler>(this, &QWinRTScreen::onPointerEntered).Get(), &d->windowTokens[&ICoreWindow::remove_PointerEntered]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_PointerExited(Callback<PointerHandler>(this, &QWinRTScreen::onPointerExited).Get(), &d->windowTokens[&ICoreWindow::remove_PointerExited]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_PointerMoved(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &d->windowTokens[&ICoreWindow::remove_PointerMoved]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_PointerPressed(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &d->windowTokens[&ICoreWindow::remove_PointerPressed]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_PointerReleased(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &d->windowTokens[&ICoreWindow::remove_PointerReleased]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_PointerWheelChanged(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &d->windowTokens[&ICoreWindow::remove_PointerWheelChanged]);
    Q_ASSERT_SUCCEEDED(hr);
#ifndef Q_OS_WINPHONE
    hr = d->coreWindow->add_SizeChanged(Callback<SizeChangedHandler>(this, &QWinRTScreen::onSizeChanged).Get(), &d->windowTokens[&ICoreWindow::remove_SizeChanged]);
    Q_ASSERT_SUCCEEDED(hr);
#endif
    hr = d->coreWindow->add_Activated(Callback<ActivatedHandler>(this, &QWinRTScreen::onActivated).Get(), &d->windowTokens[&ICoreWindow::remove_Activated]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_Closed(Callback<ClosedHandler>(this, &QWinRTScreen::onClosed).Get(), &d->windowTokens[&ICoreWindow::remove_Closed]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_VisibilityChanged(Callback<VisibilityChangedHandler>(this, &QWinRTScreen::onVisibilityChanged).Get(), &d->windowTokens[&ICoreWindow::remove_VisibilityChanged]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_AutomationProviderRequested(Callback<AutomationProviderRequestedHandler>(this, &QWinRTScreen::onAutomationProviderRequested).Get(), &d->windowTokens[&ICoreWindow::remove_AutomationProviderRequested]);
    Q_ASSERT_SUCCEEDED(hr);
#ifdef Q_OS_WINPHONE
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Phone_UI_Input_HardwareButtons).Get(), IID_PPV_ARGS(&d->hardwareButtons));
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->hardwareButtons->add_BackPressed(Callback<BackPressedHandler>(this, &QWinRTScreen::onBackButtonPressed).Get(), &d->buttonsTokens[&IHardwareButtonsStatics::remove_BackPressed]);
    Q_ASSERT_SUCCEEDED(hr);
#endif // Q_OS_WINPHONE

    // Orientation handling
    ComPtr<IDisplayInformationStatics> displayInformationStatics;
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(),
                                IID_PPV_ARGS(&displayInformationStatics));
    Q_ASSERT_SUCCEEDED(hr);

    hr = displayInformationStatics->GetForCurrentView(&d->displayInformation);
    Q_ASSERT_SUCCEEDED(hr);

    // Set native orientation
    DisplayOrientations displayOrientation;
    hr = d->displayInformation->get_NativeOrientation(&displayOrientation);
    Q_ASSERT_SUCCEEDED(hr);
    d->nativeOrientation = static_cast<Qt::ScreenOrientation>(static_cast<int>(qtOrientationsFromNative(displayOrientation)));

    hr = d->displayInformation->add_OrientationChanged(Callback<DisplayInformationHandler>(this, &QWinRTScreen::onOrientationChanged).Get(), &d->displayTokens[&IDisplayInformation::remove_OrientationChanged]);
    Q_ASSERT_SUCCEEDED(hr);

    hr = d->displayInformation->add_DpiChanged(Callback<DisplayInformationHandler>(this, &QWinRTScreen::onDpiChanged).Get(), &d->displayTokens[&IDisplayInformation::remove_DpiChanged]);
    Q_ASSERT_SUCCEEDED(hr);

    // Set initial orientation & pixel density
    onDpiChanged(Q_NULLPTR, Q_NULLPTR);
    d->orientation = d->nativeOrientation;
    onOrientationChanged(Q_NULLPTR, Q_NULLPTR);

    d->eglDisplay = eglGetDisplay(d->displayInformation.Get());
    if (d->eglDisplay == EGL_NO_DISPLAY)
        qCritical("Failed to initialize EGL display: 0x%x", eglGetError());

    if (!eglInitialize(d->eglDisplay, NULL, NULL))
        qCritical("Failed to initialize EGL: 0x%x", eglGetError());

    // Check that the device properly supports depth/stencil rendering, and disable them if not
    ComPtr<ID3D11Device> d3dDevice;
    const EGLBoolean ok = eglQuerySurfacePointerANGLE(d->eglDisplay, EGL_NO_SURFACE, EGL_DEVICE_EXT, (void **)d3dDevice.GetAddressOf());
    if (ok && d3dDevice) {
        ComPtr<IDXGIDevice> dxgiDevice;
        hr = d3dDevice.As(&dxgiDevice);
        if (SUCCEEDED(hr)) {
            ComPtr<IDXGIAdapter> dxgiAdapter;
            hr = dxgiDevice->GetAdapter(&dxgiAdapter);
            if (SUCCEEDED(hr)) {
                ComPtr<IDXGIAdapter2> dxgiAdapter2;
                hr = dxgiAdapter.As(&dxgiAdapter2);
                if (SUCCEEDED(hr)) {
                    DXGI_ADAPTER_DESC2 desc;
                    hr = dxgiAdapter2->GetDesc2(&desc);
                    if (SUCCEEDED(hr)) {
                        // The following GPUs do not render properly with depth/stencil
                        if ((desc.VendorId == 0x4d4f4351 && desc.DeviceId == 0x32303032)) { // Qualcomm Adreno 225
                            d->surfaceFormat.setDepthBufferSize(-1);
                            d->surfaceFormat.setStencilBufferSize(-1);
                        }
                    }
                }
            }
        }
    }

    d->eglConfig = q_configFromGLFormat(d->eglDisplay, d->surfaceFormat);
    d->surfaceFormat = q_glFormatFromConfig(d->eglDisplay, d->eglConfig, d->surfaceFormat);
    const QRect bounds = geometry();
    EGLint windowAttributes[] = {
        EGL_FIXED_SIZE_ANGLE, EGL_TRUE,
        EGL_WIDTH, bounds.width(),
        EGL_HEIGHT, bounds.height(),
        EGL_NONE
    };
    d->eglSurface = eglCreateWindowSurface(d->eglDisplay, d->eglConfig, d->coreWindow.Get(), windowAttributes);
    if (d->eglSurface == EGL_NO_SURFACE)
        qCritical("Failed to create EGL window surface: 0x%x", eglGetError());
}

QWinRTScreen::~QWinRTScreen()
{
    Q_D(QWinRTScreen);

    // Unregister callbacks
    for (QHash<CoreApplicationCallbackRemover, EventRegistrationToken>::const_iterator i = d->applicationTokens.begin(); i != d->applicationTokens.end(); ++i)
        (d->application.Get()->*i.key())(i.value());
    for (QHash<CoreWindowCallbackRemover, EventRegistrationToken>::const_iterator i = d->windowTokens.begin(); i != d->windowTokens.end(); ++i)
        (d->coreWindow.Get()->*i.key())(i.value());
    for (QHash<DisplayCallbackRemover, EventRegistrationToken>::const_iterator i = d->displayTokens.begin(); i != d->displayTokens.end(); ++i)
        (d->displayInformation.Get()->*i.key())(i.value());
#ifdef Q_OS_WINPHONE
    for (QHash<HardwareButtonsCallbackRemover, EventRegistrationToken>::const_iterator i = d->buttonsTokens.begin(); i != d->buttonsTokens.end(); ++i)
        (d->hardwareButtons.Get()->*i.key())(i.value());
#endif
}

QRect QWinRTScreen::geometry() const
{
    Q_D(const QWinRTScreen);
    return QRect(QPoint(), (d->logicalSize * d->scaleFactor).toSize());
}

int QWinRTScreen::depth() const
{
    return 32;
}

QImage::Format QWinRTScreen::format() const
{
    return QImage::Format_ARGB32_Premultiplied;
}

QSurfaceFormat QWinRTScreen::surfaceFormat() const
{
    Q_D(const QWinRTScreen);
    return d->surfaceFormat;
}

QSizeF QWinRTScreen::physicalSize() const
{
    Q_D(const QWinRTScreen);
    return QSizeF(d->logicalSize.width() * d->scaleFactor / d->physicalDpi.first * qreal(25.4),
                  d->logicalSize.height() * d->scaleFactor / d->physicalDpi.second * qreal(25.4));
}

QDpi QWinRTScreen::logicalDpi() const
{
    Q_D(const QWinRTScreen);
    return QDpi(d->logicalDpi, d->logicalDpi);
}

QWinRTInputContext *QWinRTScreen::inputContext() const
{
    Q_D(const QWinRTScreen);
#ifdef Q_OS_WINPHONE
    return d->inputContext.data();
#else
    return d->inputContext.Get();
#endif
}

QPlatformCursor *QWinRTScreen::cursor() const
{
    Q_D(const QWinRTScreen);
    if (!d->cursor)
        const_cast<QWinRTScreenPrivate *>(d)->cursor.reset(new QWinRTCursor);
    return d->cursor.data();
}

Qt::KeyboardModifiers QWinRTScreen::keyboardModifiers() const
{
    Q_D(const QWinRTScreen);

    Qt::KeyboardModifiers mods;
    CoreVirtualKeyStates mod;
    d->coreWindow->GetAsyncKeyState(VirtualKey_Shift, &mod);
    if (mod == CoreVirtualKeyStates_Down)
        mods |= Qt::ShiftModifier;
    d->coreWindow->GetAsyncKeyState(VirtualKey_Menu, &mod);
    if (mod == CoreVirtualKeyStates_Down)
        mods |= Qt::AltModifier;
    d->coreWindow->GetAsyncKeyState(VirtualKey_Control, &mod);
    if (mod == CoreVirtualKeyStates_Down)
        mods |= Qt::ControlModifier;
    d->coreWindow->GetAsyncKeyState(VirtualKey_LeftWindows, &mod);
    if (mod == CoreVirtualKeyStates_Down) {
        mods |= Qt::MetaModifier;
    } else {
        d->coreWindow->GetAsyncKeyState(VirtualKey_RightWindows, &mod);
        if (mod == CoreVirtualKeyStates_Down)
            mods |= Qt::MetaModifier;
    }
    return mods;
}

Qt::ScreenOrientation QWinRTScreen::nativeOrientation() const
{
    Q_D(const QWinRTScreen);
    return d->nativeOrientation;
}

Qt::ScreenOrientation QWinRTScreen::orientation() const
{
    Q_D(const QWinRTScreen);
    return d->orientation;
}

ICoreWindow *QWinRTScreen::coreWindow() const
{
    Q_D(const QWinRTScreen);
    return d->coreWindow.Get();
}

EGLDisplay QWinRTScreen::eglDisplay() const
{
    Q_D(const QWinRTScreen);
    return d->eglDisplay;
}

EGLSurface QWinRTScreen::eglSurface() const
{
    Q_D(const QWinRTScreen);
    return d->eglSurface;
}

EGLConfig QWinRTScreen::eglConfig() const
{
    Q_D(const QWinRTScreen);
    return d->eglConfig;
}

QWindow *QWinRTScreen::topWindow() const
{
    Q_D(const QWinRTScreen);
    return d->visibleWindows.isEmpty() ? 0 : d->visibleWindows.first();
}

void QWinRTScreen::addWindow(QWindow *window)
{
    Q_D(QWinRTScreen);
    if (window == topWindow())
        return;
    d->visibleWindows.prepend(window);
    QWindowSystemInterface::handleWindowActivated(window, Qt::OtherFocusReason);
    handleExpose();
}

void QWinRTScreen::removeWindow(QWindow *window)
{
    Q_D(QWinRTScreen);
    const bool wasTopWindow = window == topWindow();
    if (!d->visibleWindows.removeAll(window))
        return;
    if (wasTopWindow)
        QWindowSystemInterface::handleWindowActivated(window, Qt::OtherFocusReason);
    handleExpose();
}

void QWinRTScreen::raise(QWindow *window)
{
    Q_D(QWinRTScreen);
    d->visibleWindows.removeAll(window);
    addWindow(window);
}

void QWinRTScreen::lower(QWindow *window)
{
    Q_D(QWinRTScreen);
    const bool wasTopWindow = window == topWindow();
    if (wasTopWindow && d->visibleWindows.size() == 1)
        return;
    d->visibleWindows.removeAll(window);
    d->visibleWindows.append(window);
    if (wasTopWindow)
        QWindowSystemInterface::handleWindowActivated(window, Qt::OtherFocusReason);
    handleExpose();
}

void QWinRTScreen::handleExpose()
{
    Q_D(QWinRTScreen);
    if (d->visibleWindows.isEmpty())
        return;
    QList<QWindow *>::const_iterator it = d->visibleWindows.constBegin();
    QWindowSystemInterface::handleExposeEvent(*it, geometry());
    while (++it != d->visibleWindows.constEnd())
        QWindowSystemInterface::handleExposeEvent(*it, QRegion());
}

HRESULT QWinRTScreen::onKeyDown(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IKeyEventArgs *args)
{
    VirtualKey virtualKey;
    args->get_VirtualKey(&virtualKey);
    Qt::Key key = qKeyFromVirtual(virtualKey);
    // Defer character key presses to onCharacterReceived
    if (key == Qt::Key_unknown || (key >= Qt::Key_Space && key <= Qt::Key_ydiaeresis))
        return S_OK;
    QWindowSystemInterface::handleKeyEvent(topWindow(), QEvent::KeyPress, key, keyboardModifiers());
    return S_OK;
}

HRESULT QWinRTScreen::onKeyUp(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IKeyEventArgs *args)
{
    Qt::KeyboardModifiers mods = keyboardModifiers();
#ifndef Q_OS_WINPHONE
    Q_D(QWinRTScreen);
    CorePhysicalKeyStatus status; // Look for a pressed character key
    if (SUCCEEDED(args->get_KeyStatus(&status)) && d->activeKeys.contains(status.ScanCode)) {
        QPair<Qt::Key, QString> keyStatus = d->activeKeys.take(status.ScanCode);
        QWindowSystemInterface::handleKeyEvent(topWindow(), QEvent::KeyRelease,
                                               keyStatus.first, mods, keyStatus.second);
        return S_OK;
    }
#endif // !Q_OS_WINPHONE
    VirtualKey virtualKey;
    args->get_VirtualKey(&virtualKey);
    QWindowSystemInterface::handleKeyEvent(topWindow(), QEvent::KeyRelease,
                                           qKeyFromVirtual(virtualKey), mods);
    return S_OK;
}

HRESULT QWinRTScreen::onCharacterReceived(ICoreWindow *, ICharacterReceivedEventArgs *args)
{
    quint32 keyCode;
    args->get_KeyCode(&keyCode);
    // Don't generate character events for non-printables; the meta key stage is enough
    if (qIsNonPrintable(keyCode))
        return S_OK;

    Qt::KeyboardModifiers mods = keyboardModifiers();
    Qt::Key key = qKeyFromCode(keyCode, mods);
    QString text = QChar(keyCode);
    QWindowSystemInterface::handleKeyEvent(topWindow(), QEvent::KeyPress, key, mods, text);
#ifndef Q_OS_WINPHONE
    Q_D(QWinRTScreen);
    CorePhysicalKeyStatus status; // Defer release to onKeyUp for physical keys
    if (SUCCEEDED(args->get_KeyStatus(&status)) && !status.IsKeyReleased) {
        d->activeKeys.insert(status.ScanCode, qMakePair(key, text));
        return S_OK;
    }
#endif // !Q_OS_WINPHONE
    QWindowSystemInterface::handleKeyEvent(topWindow(), QEvent::KeyRelease, key, mods, text);
    return S_OK;
}

HRESULT QWinRTScreen::onPointerEntered(ICoreWindow *, IPointerEventArgs *args)
{
    Q_D(QWinRTScreen);

    ComPtr<IPointerPoint> pointerPoint;
    if (SUCCEEDED(args->get_CurrentPoint(&pointerPoint))) {
        // Assumes full-screen window
        Point point;
        pointerPoint->get_Position(&point);
        QPoint pos(point.X * d->scaleFactor, point.Y * d->scaleFactor);

        QWindowSystemInterface::handleEnterEvent(topWindow(), pos, pos);
    }
    return S_OK;
}

HRESULT QWinRTScreen::onPointerExited(ICoreWindow *, IPointerEventArgs *)
{
    QWindowSystemInterface::handleLeaveEvent(0);
    return S_OK;
}

HRESULT QWinRTScreen::onPointerUpdated(ICoreWindow *, IPointerEventArgs *args)
{
    Q_D(QWinRTScreen);
    ComPtr<IPointerPoint> pointerPoint;
    if (FAILED(args->get_CurrentPoint(&pointerPoint)))
        return E_INVALIDARG;

    // Common traits - point, modifiers, properties
    Point point;
    pointerPoint->get_Position(&point);
    QPointF pos(point.X * d->scaleFactor, point.Y * d->scaleFactor);

    VirtualKeyModifiers modifiers;
    args->get_KeyModifiers(&modifiers);
    Qt::KeyboardModifiers mods;
    if (modifiers & VirtualKeyModifiers_Control)
        mods |= Qt::ControlModifier;
    if (modifiers & VirtualKeyModifiers_Menu)
        mods |= Qt::AltModifier;
    if (modifiers & VirtualKeyModifiers_Shift)
        mods |= Qt::ShiftModifier;
    if (modifiers & VirtualKeyModifiers_Windows)
        mods |= Qt::MetaModifier;

    ComPtr<IPointerPointProperties> properties;
    if (FAILED(pointerPoint->get_Properties(&properties)))
        return E_INVALIDARG;

    ComPtr<IPointerDevice> pointerDevice;
    HRESULT hr = pointerPoint->get_PointerDevice(&pointerDevice);
    RETURN_OK_IF_FAILED("Failed to get pointer device.");

    PointerDeviceType pointerDeviceType;
    hr = pointerDevice->get_PointerDeviceType(&pointerDeviceType);
    RETURN_OK_IF_FAILED("Failed to get pointer device type.");

    switch (pointerDeviceType) {
    case PointerDeviceType_Mouse: {
        qint32 delta;
        properties->get_MouseWheelDelta(&delta);
        if (delta) {
            boolean isHorizontal;
            properties->get_IsHorizontalMouseWheel(&isHorizontal);
            QPoint angleDelta(isHorizontal ? delta : 0, isHorizontal ? 0 : delta);
            QWindowSystemInterface::handleWheelEvent(topWindow(), pos, pos, QPoint(), angleDelta, mods);
            break;
        }

        boolean isPressed;
        Qt::MouseButtons buttons = Qt::NoButton;
        properties->get_IsLeftButtonPressed(&isPressed);
        if (isPressed)
            buttons |= Qt::LeftButton;

        properties->get_IsMiddleButtonPressed(&isPressed);
        if (isPressed)
            buttons |= Qt::MiddleButton;

        properties->get_IsRightButtonPressed(&isPressed);
        if (isPressed)
            buttons |= Qt::RightButton;

        properties->get_IsXButton1Pressed(&isPressed);
        if (isPressed)
            buttons |= Qt::XButton1;

        properties->get_IsXButton2Pressed(&isPressed);
        if (isPressed)
            buttons |= Qt::XButton2;

        QWindowSystemInterface::handleMouseEvent(topWindow(), pos, pos, buttons, mods);

        break;
    }
    case PointerDeviceType_Touch: {
        if (!d->touchDevice) {
            d->touchDevice = new QTouchDevice;
            d->touchDevice->setName(QStringLiteral("WinRTTouchScreen"));
            d->touchDevice->setType(QTouchDevice::TouchScreen);
            d->touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::Pressure | QTouchDevice::NormalizedPosition);
            QWindowSystemInterface::registerTouchDevice(d->touchDevice);
        }

        quint32 id;
        pointerPoint->get_PointerId(&id);

        Rect area;
        properties->get_ContactRect(&area);

        float pressure;
        properties->get_Pressure(&pressure);

        QHash<quint32, QWindowSystemInterface::TouchPoint>::iterator it = d->touchPoints.find(id);
        if (it != d->touchPoints.end()) {
            boolean isPressed;
#ifndef Q_OS_WINPHONE
            pointerPoint->get_IsInContact(&isPressed);
#else
            properties->get_IsLeftButtonPressed(&isPressed); // IsInContact not reliable on phone
#endif
            it.value().state = isPressed ? Qt::TouchPointMoved : Qt::TouchPointReleased;
        } else {
            it = d->touchPoints.insert(id, QWindowSystemInterface::TouchPoint());
            it.value().state = Qt::TouchPointPressed;
            it.value().id = id;
        }
        it.value().area = QRectF(area.X * d->scaleFactor, area.Y * d->scaleFactor,
                                 area.Width * d->scaleFactor, area.Height * d->scaleFactor);
        it.value().normalPosition = QPointF(point.X/d->logicalSize.width(), point.Y/d->logicalSize.height());
        it.value().pressure = pressure;

        QWindowSystemInterface::handleTouchEvent(topWindow(), d->touchDevice, d->touchPoints.values(), mods);

        // Remove released points, station others
        for (QHash<quint32, QWindowSystemInterface::TouchPoint>::iterator i = d->touchPoints.begin(); i != d->touchPoints.end();) {
            if (i.value().state == Qt::TouchPointReleased)
                i = d->touchPoints.erase(i);
            else
                (i++).value().state = Qt::TouchPointStationary;
        }

        break;
    }
    case PointerDeviceType_Pen: {
        quint32 id;
        pointerPoint->get_PointerId(&id);

        boolean isPressed;
        pointerPoint->get_IsInContact(&isPressed);

        boolean isEraser;
        properties->get_IsEraser(&isEraser);
        int pointerType = isEraser ? 3 : 1;

        float pressure;
        properties->get_Pressure(&pressure);

        float xTilt;
        properties->get_XTilt(&xTilt);

        float yTilt;
        properties->get_YTilt(&yTilt);

        float rotation;
        properties->get_Twist(&rotation);

        QWindowSystemInterface::handleTabletEvent(topWindow(), isPressed, pos, pos, 0,
                                                  pointerType, pressure, xTilt, yTilt,
                                                  0, rotation, 0, id, mods);

        break;
    }
    }

    return S_OK;
}

HRESULT QWinRTScreen::onAutomationProviderRequested(ICoreWindow *, IAutomationProviderRequestedEventArgs *args)
{
#ifndef Q_OS_WINPHONE
    Q_D(const QWinRTScreen);
    args->put_AutomationProvider(d->inputContext.Get());
#else
    Q_UNUSED(args)
#endif
    return S_OK;
}

HRESULT QWinRTScreen::onSizeChanged(ICoreWindow *, IWindowSizeChangedEventArgs *)
{
    Q_D(QWinRTScreen);

    Rect size;
    HRESULT hr;
    hr = d->coreWindow->get_Bounds(&size);
    RETURN_OK_IF_FAILED("Failed to get window bounds");
    QSizeF logicalSize = QSizeF(size.Width, size.Height);
#ifndef Q_OS_WINPHONE // This handler is called from orientation changed, in which case we should always update the size
    if (d->logicalSize == logicalSize)
        return S_OK;
#endif

    d->logicalSize = logicalSize;
    if (d->eglDisplay) {
        const QRect newGeometry = geometry();
        int width = newGeometry.width();
        int height = newGeometry.height();
#ifdef Q_OS_WINPHONE // Windows Phone can pass in a negative size to provide orientation information
        width *= (d->orientation == Qt::InvertedPortraitOrientation || d->orientation == Qt::LandscapeOrientation) ? -1 : 1;
        height *= (d->orientation == Qt::InvertedPortraitOrientation || d->orientation == Qt::InvertedLandscapeOrientation) ? -1 : 1;
#endif
        eglSurfaceAttrib(d->eglDisplay, d->eglSurface, EGL_WIDTH, width);
        eglSurfaceAttrib(d->eglDisplay, d->eglSurface, EGL_HEIGHT, height);
        QWindowSystemInterface::handleScreenGeometryChange(screen(), newGeometry, newGeometry);
        QPlatformScreen::resizeMaximizedWindows();
        handleExpose();
    }
    return S_OK;
}

HRESULT QWinRTScreen::onActivated(ICoreWindow *, IWindowActivatedEventArgs *args)
{
    Q_D(QWinRTScreen);

    CoreWindowActivationState activationState;
    args->get_WindowActivationState(&activationState);
    if (activationState == CoreWindowActivationState_Deactivated) {
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationInactive);
        return S_OK;
    }

    // Activate topWindow
    if (!d->visibleWindows.isEmpty()) {
        Qt::FocusReason focusReason = activationState == CoreWindowActivationState_PointerActivated
                ? Qt::MouseFocusReason : Qt::ActiveWindowFocusReason;
        QWindowSystemInterface::handleWindowActivated(topWindow(), focusReason);
    }
    return S_OK;
}

HRESULT QWinRTScreen::onSuspended(IInspectable *, ISuspendingEventArgs *)
{
#ifndef Q_OS_WINPHONE
    Q_D(QWinRTScreen);
    ComPtr<ID3D11Device> d3dDevice;
    const EGLBoolean ok = eglQuerySurfacePointerANGLE(d->eglDisplay, EGL_NO_SURFACE, EGL_DEVICE_EXT, (void **)d3dDevice.GetAddressOf());
    if (ok && d3dDevice) {
        ComPtr<IDXGIDevice3> dxgiDevice;
        if (SUCCEEDED(d3dDevice.As(&dxgiDevice)))
            dxgiDevice->Trim();
    }
#endif
    QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationSuspended);
    QWindowSystemInterface::flushWindowSystemEvents();
    return S_OK;
}

HRESULT QWinRTScreen::onResume(IInspectable *, IInspectable *)
{
    // First the system invokes onResume and then changes
    // the visibility of the screen to be active.
    QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationHidden);
    return S_OK;
}

HRESULT QWinRTScreen::onClosed(ICoreWindow *, ICoreWindowEventArgs *)
{
    foreach (QWindow *w, QGuiApplication::topLevelWindows())
        QWindowSystemInterface::handleCloseEvent(w);
    return S_OK;
}

HRESULT QWinRTScreen::onVisibilityChanged(ICoreWindow *, IVisibilityChangedEventArgs *args)
{
    boolean visible;
    args->get_Visible(&visible);
    QWindowSystemInterface::handleApplicationStateChanged(visible ? Qt::ApplicationActive : Qt::ApplicationHidden);
    if (visible)
        handleExpose();
    return S_OK;
}

HRESULT QWinRTScreen::onOrientationChanged(IDisplayInformation *, IInspectable *)
{
    Q_D(QWinRTScreen);

    DisplayOrientations displayOrientation;
    HRESULT hr = d->displayInformation->get_CurrentOrientation(&displayOrientation);
    RETURN_OK_IF_FAILED("Failed to get current orientations.");

    Qt::ScreenOrientation newOrientation = static_cast<Qt::ScreenOrientation>(static_cast<int>(qtOrientationsFromNative(displayOrientation)));
    if (d->orientation != newOrientation) {
        d->orientation = newOrientation;
        QWindowSystemInterface::handleScreenOrientationChange(screen(), d->orientation);
    }

#ifdef Q_OS_WINPHONE // The size changed handler is ignored in favor of this callback
    onSizeChanged(Q_NULLPTR, Q_NULLPTR);
#endif
    return S_OK;
}

HRESULT QWinRTScreen::onDpiChanged(IDisplayInformation *, IInspectable *)
{
    Q_D(QWinRTScreen);

    HRESULT hr;
#ifdef Q_OS_WINPHONE
    ComPtr<IDisplayInformation2> displayInformation;
    hr = d->displayInformation.As(&displayInformation);
    RETURN_OK_IF_FAILED("Failed to cast display information.");
    hr = displayInformation->get_RawPixelsPerViewPixel(&d->scaleFactor);
#else
    ResolutionScale resolutionScale;
    hr = d->displayInformation->get_ResolutionScale(&resolutionScale);
    d->scaleFactor = qreal(resolutionScale) / 100;
#endif
    RETURN_OK_IF_FAILED("Failed to get scale factor");

    FLOAT dpi;
    hr = d->displayInformation->get_LogicalDpi(&dpi);
    RETURN_OK_IF_FAILED("Failed to get logical DPI.");
    d->logicalDpi = dpi;

    hr = d->displayInformation->get_RawDpiX(&dpi);
    RETURN_OK_IF_FAILED("Failed to get x raw DPI.");
    d->physicalDpi.first = dpi ? dpi : 96.0;

    hr = d->displayInformation->get_RawDpiY(&dpi);
    RETURN_OK_IF_FAILED("Failed to get y raw DPI.");
    d->physicalDpi.second = dpi ? dpi : 96.0;

    return S_OK;
}

#ifdef Q_OS_WINPHONE
HRESULT QWinRTScreen::onBackButtonPressed(IInspectable *, IBackPressedEventArgs *args)
{
    Q_D(QWinRTScreen);

    QKeyEvent backPress(QEvent::KeyPress, Qt::Key_Back, Qt::NoModifier);
    QKeyEvent backRelease(QEvent::KeyRelease, Qt::Key_Back, Qt::NoModifier);
    backPress.setAccepted(false);
    backRelease.setAccepted(false);

    QObject *receiver = d->visibleWindows.isEmpty()
            ? static_cast<QObject *>(QGuiApplication::instance())
            : static_cast<QObject *>(d->visibleWindows.first());

    // If the event is ignored, the app will suspend
    QGuiApplication::sendEvent(receiver, &backPress);
    QGuiApplication::sendEvent(receiver, &backRelease);
    args->put_Handled(backPress.isAccepted() || backRelease.isAccepted());

    return S_OK;
}
#endif // Q_OS_WINPHONE

QT_END_NAMESPACE
