/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwinrtscreen.h"

#include "qwinrtbackingstore.h"
#include "qwinrtinputcontext.h"
#include "qwinrtcursor.h"
#include "qwinrteglcontext.h"

#include <QtGui/QSurfaceFormat>
#include <QtGui/QGuiApplication>
#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qt_windows.h>

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

QWinRTScreen::QWinRTScreen(ICoreWindow *window)
    : m_coreWindow(window)
    , m_depth(32)
    , m_format(QImage::Format_ARGB32_Premultiplied)
#ifdef Q_OS_WINPHONE
    , m_inputContext(new QWinRTInputContext(m_coreWindow))
#else
    , m_inputContext(Make<QWinRTInputContext>(m_coreWindow).Detach())
#endif
    , m_cursor(new QWinRTCursor(window))
    , m_orientation(Qt::PrimaryOrientation)
    , m_touchDevice(Q_NULLPTR)
{
    Rect rect;
    window->get_Bounds(&rect);
    m_geometry = QRect(0, 0, rect.Width, rect.Height);

    m_surfaceFormat.setAlphaBufferSize(0);
    m_surfaceFormat.setRedBufferSize(8);
    m_surfaceFormat.setGreenBufferSize(8);
    m_surfaceFormat.setBlueBufferSize(8);

    m_surfaceFormat.setRenderableType(QSurfaceFormat::OpenGLES);
    m_surfaceFormat.setSamples(1);
    m_surfaceFormat.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    m_surfaceFormat.setDepthBufferSize(24);
    m_surfaceFormat.setStencilBufferSize(8);

    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_eglDisplay == EGL_NO_DISPLAY)
        qFatal("Qt WinRT platform plugin: failed to initialize EGL display.");

    if (!eglInitialize(m_eglDisplay, NULL, NULL))
        qFatal("Qt WinRT platform plugin: failed to initialize EGL. This can happen if you haven't included the D3D compiler DLL in your application package.");

    // TODO: move this to Window
    m_eglSurface = eglCreateWindowSurface(m_eglDisplay, q_configFromGLFormat(m_eglDisplay, m_surfaceFormat), window, NULL);
    if (m_eglSurface == EGL_NO_SURFACE)
        qFatal("Could not create EGL surface, error 0x%X", eglGetError());

    // Event handlers mapped to QEvents
    m_coreWindow->add_KeyDown(Callback<KeyHandler>(this, &QWinRTScreen::onKeyDown).Get(), &m_tokens[QEvent::KeyPress]);
    m_coreWindow->add_KeyUp(Callback<KeyHandler>(this, &QWinRTScreen::onKeyUp).Get(), &m_tokens[QEvent::KeyRelease]);
    m_coreWindow->add_CharacterReceived(Callback<CharacterReceivedHandler>(this, &QWinRTScreen::onCharacterReceived).Get(), &m_tokens[QEvent::User]);
    m_coreWindow->add_PointerEntered(Callback<PointerHandler>(this, &QWinRTScreen::onPointerEntered).Get(), &m_tokens[QEvent::Enter]);
    m_coreWindow->add_PointerExited(Callback<PointerHandler>(this, &QWinRTScreen::onPointerExited).Get(), &m_tokens[QEvent::Leave]);
    m_coreWindow->add_PointerMoved(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &m_tokens[QEvent::MouseMove]);
    m_coreWindow->add_PointerPressed(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &m_tokens[QEvent::MouseButtonPress]);
    m_coreWindow->add_PointerReleased(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &m_tokens[QEvent::MouseButtonRelease]);
    m_coreWindow->add_PointerWheelChanged(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &m_tokens[QEvent::Wheel]);
    m_coreWindow->add_SizeChanged(Callback<SizeChangedHandler>(this, &QWinRTScreen::onSizeChanged).Get(), &m_tokens[QEvent::Resize]);
#ifdef Q_OS_WINPHONE
    ComPtr<IHardwareButtonsStatics> hardwareButtons;
    if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Phone_UI_Input_HardwareButtons).Get(), &hardwareButtons)))
        hardwareButtons->add_BackPressed(Callback<BackPressedHandler>(this, &QWinRTScreen::onBackButtonPressed).Get(), &m_tokens[QEvent::User]);
#endif // Q_OS_WINPHONE

    // Window event handlers
    m_coreWindow->add_Activated(Callback<ActivatedHandler>(this, &QWinRTScreen::onActivated).Get(), &m_tokens[QEvent::WindowActivate]);
    m_coreWindow->add_Closed(Callback<ClosedHandler>(this, &QWinRTScreen::onClosed).Get(), &m_tokens[QEvent::WindowDeactivate]);
    m_coreWindow->add_VisibilityChanged(Callback<VisibilityChangedHandler>(this, &QWinRTScreen::onVisibilityChanged).Get(), &m_tokens[QEvent::Show]);
    m_coreWindow->add_AutomationProviderRequested(Callback<AutomationProviderRequestedHandler>(this, &QWinRTScreen::onAutomationProviderRequested).Get(), &m_tokens[QEvent::InputMethodQuery]);

    // Orientation handling
    if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Graphics_Display_DisplayProperties).Get(),
                                       &m_displayProperties))) {
        // Set native orientation
        DisplayOrientations displayOrientation;
        m_displayProperties->get_NativeOrientation(&displayOrientation);
        m_nativeOrientation = static_cast<Qt::ScreenOrientation>(static_cast<int>(qtOrientationsFromNative(displayOrientation)));

        // Set initial orientation
        onOrientationChanged(0);

        m_displayProperties->add_OrientationChanged(Callback<IDisplayPropertiesEventHandler>(this, &QWinRTScreen::onOrientationChanged).Get(),
                                                    &m_tokens[QEvent::OrientationChange]);
    }

#ifndef Q_OS_WINPHONE
    GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_ViewManagement_ApplicationView).Get(),
                         &m_applicationView);
#endif


    if (SUCCEEDED(RoGetActivationFactory(Wrappers::HString::MakeReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(),
                                         IID_PPV_ARGS(&m_application)))) {
        m_application->add_Suspending(Callback<SuspendHandler>(this, &QWinRTScreen::onSuspended).Get(), &m_suspendTokens[Qt::ApplicationSuspended]);
        m_application->add_Resuming(Callback<ResumeHandler>(this, &QWinRTScreen::onResume).Get(), &m_suspendTokens[Qt::ApplicationHidden]);
    }
}

QRect QWinRTScreen::geometry() const
{
    return m_geometry;
}

int QWinRTScreen::depth() const
{
    return m_depth;
}

QImage::Format QWinRTScreen::format() const
{
    return m_format;
}

QSurfaceFormat QWinRTScreen::surfaceFormat() const
{
    return m_surfaceFormat;
}

QWinRTInputContext *QWinRTScreen::inputContext() const
{
    return m_inputContext;
}

QPlatformCursor *QWinRTScreen::cursor() const
{
    return m_cursor;
}

Qt::KeyboardModifiers QWinRTScreen::keyboardModifiers() const
{
    Qt::KeyboardModifiers mods;
    CoreVirtualKeyStates mod;
    m_coreWindow->GetAsyncKeyState(VirtualKey_Shift, &mod);
    if (mod == CoreVirtualKeyStates_Down)
        mods |= Qt::ShiftModifier;
    m_coreWindow->GetAsyncKeyState(VirtualKey_Menu, &mod);
    if (mod == CoreVirtualKeyStates_Down)
        mods |= Qt::AltModifier;
    m_coreWindow->GetAsyncKeyState(VirtualKey_Control, &mod);
    if (mod == CoreVirtualKeyStates_Down)
        mods |= Qt::ControlModifier;
    m_coreWindow->GetAsyncKeyState(VirtualKey_LeftWindows, &mod);
    if (mod == CoreVirtualKeyStates_Down) {
        mods |= Qt::MetaModifier;
    } else {
        m_coreWindow->GetAsyncKeyState(VirtualKey_RightWindows, &mod);
        if (mod == CoreVirtualKeyStates_Down)
            mods |= Qt::MetaModifier;
    }
    return mods;
}

Qt::ScreenOrientation QWinRTScreen::nativeOrientation() const
{
    return m_nativeOrientation;
}

Qt::ScreenOrientation QWinRTScreen::orientation() const
{
    return m_orientation;
}

void QWinRTScreen::setOrientationUpdateMask(Qt::ScreenOrientations mask)
{
    m_displayProperties->put_AutoRotationPreferences(nativeOrientationsFromQt(mask));
}

ICoreWindow *QWinRTScreen::coreWindow() const
{
    return m_coreWindow;
}

EGLDisplay QWinRTScreen::eglDisplay() const
{
    return m_eglDisplay;
}

EGLSurface QWinRTScreen::eglSurface() const
{
    return m_eglSurface;
}

QWindow *QWinRTScreen::topWindow() const
{
    return m_visibleWindows.isEmpty() ? 0 : m_visibleWindows.first();
}

void QWinRTScreen::addWindow(QWindow *window)
{
    if (window == topWindow())
        return;
    m_visibleWindows.prepend(window);
    QWindowSystemInterface::handleWindowActivated(window, Qt::OtherFocusReason);
    handleExpose();
}

void QWinRTScreen::removeWindow(QWindow *window)
{
    const bool wasTopWindow = window == topWindow();
    if (!m_visibleWindows.removeAll(window))
        return;
    if (wasTopWindow)
        QWindowSystemInterface::handleWindowActivated(window, Qt::OtherFocusReason);
    handleExpose();
}

void QWinRTScreen::raise(QWindow *window)
{
    m_visibleWindows.removeAll(window);
    addWindow(window);
}

void QWinRTScreen::lower(QWindow *window)
{
    const bool wasTopWindow = window == topWindow();
    if (wasTopWindow && m_visibleWindows.size() == 1)
        return;
    m_visibleWindows.removeAll(window);
    m_visibleWindows.append(window);
    if (wasTopWindow)
        QWindowSystemInterface::handleWindowActivated(window, Qt::OtherFocusReason);
    handleExpose();
}

void QWinRTScreen::handleExpose()
{
    if (m_visibleWindows.isEmpty())
        return;
    QList<QWindow *>::const_iterator it = m_visibleWindows.constBegin();
    QWindowSystemInterface::handleExposeEvent(*it, m_geometry);
    while (++it != m_visibleWindows.constEnd())
        QWindowSystemInterface::handleExposeEvent(*it, QRegion());
    QWindowSystemInterface::flushWindowSystemEvents();
}

HRESULT QWinRTScreen::onKeyDown(ABI::Windows::UI::Core::ICoreWindow *window, ABI::Windows::UI::Core::IKeyEventArgs *args)
{
    Q_UNUSED(window);
    VirtualKey virtualKey;
    args->get_VirtualKey(&virtualKey);
    Qt::Key key = qKeyFromVirtual(virtualKey);
    // Defer character key presses to onCharacterReceived
    if (key == Qt::Key_unknown || (key >= Qt::Key_Space && key <= Qt::Key_ydiaeresis))
        return S_OK;
    QWindowSystemInterface::handleKeyEvent(topWindow(), QEvent::KeyPress, key, keyboardModifiers());
    return S_OK;
}

HRESULT QWinRTScreen::onKeyUp(ABI::Windows::UI::Core::ICoreWindow *window, ABI::Windows::UI::Core::IKeyEventArgs *args)
{
    Q_UNUSED(window);
    Qt::KeyboardModifiers mods = keyboardModifiers();
#ifndef Q_OS_WINPHONE
    CorePhysicalKeyStatus status; // Look for a pressed character key
    if (SUCCEEDED(args->get_KeyStatus(&status)) && m_activeKeys.contains(status.ScanCode)) {
        QPair<Qt::Key, QString> keyStatus = m_activeKeys.take(status.ScanCode);
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

HRESULT QWinRTScreen::onCharacterReceived(ICoreWindow *window, ICharacterReceivedEventArgs *args)
{
    Q_UNUSED(window);

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
    CorePhysicalKeyStatus status; // Defer release to onKeyUp for physical keys
    if (SUCCEEDED(args->get_KeyStatus(&status)) && !status.IsKeyReleased) {
        m_activeKeys.insert(status.ScanCode, qMakePair(key, text));
        return S_OK;
    }
#endif // !Q_OS_WINPHONE
    QWindowSystemInterface::handleKeyEvent(topWindow(), QEvent::KeyRelease, key, mods, text);
    return S_OK;
}

HRESULT QWinRTScreen::onPointerEntered(ICoreWindow *window, IPointerEventArgs *args)
{
    Q_UNUSED(window);
    IPointerPoint *pointerPoint;
    if (SUCCEEDED(args->get_CurrentPoint(&pointerPoint))) {
        // Assumes full-screen window
        Point point;
        pointerPoint->get_Position(&point);
        QPoint pos(point.X, point.Y);

        QWindowSystemInterface::handleEnterEvent(topWindow(), pos, pos);
        pointerPoint->Release();
    }
    return S_OK;
}

HRESULT QWinRTScreen::onPointerExited(ICoreWindow *window, IPointerEventArgs *args)
{
    Q_UNUSED(window);
    Q_UNUSED(args);
    QWindowSystemInterface::handleLeaveEvent(0);
    return S_OK;
}

HRESULT QWinRTScreen::onPointerUpdated(ICoreWindow *window, IPointerEventArgs *args)
{
    Q_UNUSED(window);

    IPointerPoint *pointerPoint;
    if (FAILED(args->get_CurrentPoint(&pointerPoint)))
        return E_INVALIDARG;

    // Common traits - point, modifiers, properties
    Point point;
    pointerPoint->get_Position(&point);
    QPointF pos(point.X, point.Y);

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

    IPointerPointProperties *properties;
    if (FAILED(pointerPoint->get_Properties(&properties)))
        return E_INVALIDARG;

    PointerDeviceType pointerDeviceType;
#if defined(Q_OS_WINPHONE) && _MSC_VER <= 1700
    pointerDeviceType = PointerDeviceType_Touch;
#else
    ComPtr<IPointerDevice> pointerDevice;
    HRESULT hr = pointerPoint->get_PointerDevice(&pointerDevice);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to get pointer device.");
        return S_OK;
    }

    hr = pointerDevice->get_PointerDeviceType(&pointerDeviceType);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to get pointer device type.");
        return S_OK;
    }
#endif
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
        if (!m_touchDevice) {
            m_touchDevice = new QTouchDevice;
            m_touchDevice->setName(QStringLiteral("WinRTTouchScreen"));
            m_touchDevice->setType(QTouchDevice::TouchScreen);
            m_touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::Pressure | QTouchDevice::NormalizedPosition);
            QWindowSystemInterface::registerTouchDevice(m_touchDevice);
        }

        quint32 id;
        pointerPoint->get_PointerId(&id);

        Rect area;
        properties->get_ContactRect(&area);

        float pressure;
        properties->get_Pressure(&pressure);

        QHash<quint32, QWindowSystemInterface::TouchPoint>::iterator it = m_touchPoints.find(id);
        if (it != m_touchPoints.end()) {
            boolean isPressed;
#ifndef Q_OS_WINPHONE
            pointerPoint->get_IsInContact(&isPressed);
#else
            properties->get_IsLeftButtonPressed(&isPressed); // IsInContact not reliable on phone
#endif
            it.value().state = isPressed ? Qt::TouchPointMoved : Qt::TouchPointReleased;
        } else {
            it = m_touchPoints.insert(id, QWindowSystemInterface::TouchPoint());
            it.value().state = Qt::TouchPointPressed;
            it.value().id = id;
        }
        it.value().area = QRectF(area.X, area.Y, area.Width, area.Height);
        it.value().normalPosition = QPointF(pos.x()/m_geometry.width(), pos.y()/m_geometry.height());
        it.value().pressure = pressure;

        QWindowSystemInterface::handleTouchEvent(topWindow(), m_touchDevice, m_touchPoints.values(), mods);

        // Remove released points, station others
        for (QHash<quint32, QWindowSystemInterface::TouchPoint>::iterator i = m_touchPoints.begin(); i != m_touchPoints.end();) {
            if (i.value().state == Qt::TouchPointReleased)
                i = m_touchPoints.erase(i);
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

    properties->Release();
    pointerPoint->Release();

    return S_OK;
}

HRESULT QWinRTScreen::onAutomationProviderRequested(ICoreWindow *, IAutomationProviderRequestedEventArgs *args)
{
#ifndef Q_OS_WINPHONE
    args->put_AutomationProvider(m_inputContext);
#endif
    return S_OK;
}

HRESULT QWinRTScreen::onSizeChanged(ICoreWindow *window, IWindowSizeChangedEventArgs *args)
{
    Q_UNUSED(window);

    Size size;
    if (FAILED(args->get_Size(&size))) {
        qWarning(Q_FUNC_INFO ": failed to get size");
        return S_OK;
    }

    // Regardless of state, all top-level windows are viewport-sized - this might change if
    // a more advanced compositor is written.
    m_geometry.setSize(QSize(size.Width, size.Height));
    QWindowSystemInterface::handleScreenGeometryChange(screen(), m_geometry);
    QWindowSystemInterface::handleScreenAvailableGeometryChange(screen(), m_geometry);
    QPlatformScreen::resizeMaximizedWindows();
    handleExpose();

    return S_OK;
}

HRESULT QWinRTScreen::onActivated(ICoreWindow *window, IWindowActivatedEventArgs *args)
{
    Q_UNUSED(window);

    CoreWindowActivationState activationState;
    args->get_WindowActivationState(&activationState);
    if (activationState == CoreWindowActivationState_Deactivated) {
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationInactive);
        return S_OK;
    }

    // Activate topWindow
    if (!m_visibleWindows.isEmpty()) {
        Qt::FocusReason focusReason = activationState == CoreWindowActivationState_PointerActivated
                ? Qt::MouseFocusReason : Qt::ActiveWindowFocusReason;
        QWindowSystemInterface::handleWindowActivated(topWindow(), focusReason);
    }
    return S_OK;
}

HRESULT QWinRTScreen::onSuspended(IInspectable *, ISuspendingEventArgs *)
{
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

HRESULT QWinRTScreen::onClosed(ICoreWindow *window, ICoreWindowEventArgs *args)
{
    Q_UNUSED(window);
    Q_UNUSED(args);

    foreach (QWindow *w, QGuiApplication::topLevelWindows())
        QWindowSystemInterface::handleCloseEvent(w);
    return S_OK;
}

HRESULT QWinRTScreen::onVisibilityChanged(ICoreWindow *window, IVisibilityChangedEventArgs *args)
{
    Q_UNUSED(window);
    Q_UNUSED(args);

    boolean visible;
    args->get_Visible(&visible);
    QWindowSystemInterface::handleApplicationStateChanged(visible ? Qt::ApplicationActive : Qt::ApplicationHidden);
    if (visible)
        handleExpose();
    return S_OK;
}

HRESULT QWinRTScreen::onOrientationChanged(IInspectable *)
{
    DisplayOrientations displayOrientation;
    m_displayProperties->get_CurrentOrientation(&displayOrientation);
    Qt::ScreenOrientation newOrientation = static_cast<Qt::ScreenOrientation>(static_cast<int>(qtOrientationsFromNative(displayOrientation)));
    if (m_orientation != newOrientation) {
        m_orientation = newOrientation;
        QWindowSystemInterface::handleScreenOrientationChange(screen(), m_orientation);
    }

    return S_OK;
}

#ifdef Q_OS_WINPHONE
HRESULT QWinRTScreen::onBackButtonPressed(IInspectable *, IBackPressedEventArgs *args)
{
    QKeyEvent backPress(QEvent::KeyPress, Qt::Key_Back, Qt::NoModifier);
    QKeyEvent backRelease(QEvent::KeyRelease, Qt::Key_Back, Qt::NoModifier);
    backPress.setAccepted(false);
    backRelease.setAccepted(false);

    QObject *receiver = m_visibleWindows.isEmpty()
            ? static_cast<QObject *>(QGuiApplication::instance())
            : static_cast<QObject *>(m_visibleWindows.first());

    // If the event is ignored, the app will suspend
    QGuiApplication::sendEvent(receiver, &backPress);
    QGuiApplication::sendEvent(receiver, &backRelease);
    args->put_Handled(backPress.isAccepted() || backRelease.isAccepted());

    return S_OK;
}
#endif // Q_OS_WINPHONE

QT_END_NAMESPACE
