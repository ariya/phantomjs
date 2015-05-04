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

#ifndef QWINRTSCREEN_H
#define QWINRTSCREEN_H

#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

#include <EGL/egl.h>

namespace ABI {
    namespace Windows {
        namespace ApplicationModel {
            struct ISuspendingEventArgs;
        }
        namespace UI {
            namespace Core {
                struct IAutomationProviderRequestedEventArgs;
                struct ICharacterReceivedEventArgs;
                struct ICoreWindow;
                struct ICoreWindowEventArgs;
                struct IKeyEventArgs;
                struct IPointerEventArgs;
                struct IVisibilityChangedEventArgs;
                struct IWindowActivatedEventArgs;
                struct IWindowSizeChangedEventArgs;
            }
        }
        namespace Graphics {
            namespace Display {
                struct IDisplayInformation;
            }
        }
#ifdef Q_OS_WINPHONE
        namespace Phone {
            namespace UI {
                namespace Input {
                    struct IBackPressedEventArgs;
                }
            }
        }
#endif
    }
}
struct IInspectable;

QT_BEGIN_NAMESPACE

class QTouchDevice;
class QWinRTEGLContext;
class QWinRTCursor;
class QWinRTInputContext;
class QWinRTScreenPrivate;
class QWinRTScreen : public QPlatformScreen
{
public:
    explicit QWinRTScreen();
    ~QWinRTScreen();
    QRect geometry() const;
    int depth() const;
    QImage::Format format() const;
    QSurfaceFormat surfaceFormat() const;
    QSizeF physicalSize() const Q_DECL_OVERRIDE;
    QDpi logicalDpi() const Q_DECL_OVERRIDE;
    QWinRTInputContext *inputContext() const;
    QPlatformCursor *cursor() const;
    Qt::KeyboardModifiers keyboardModifiers() const;

    Qt::ScreenOrientation nativeOrientation() const;
    Qt::ScreenOrientation orientation() const;

    QWindow *topWindow() const;
    void addWindow(QWindow *window);
    void removeWindow(QWindow *window);
    void raise(QWindow *window);
    void lower(QWindow *window);

    ABI::Windows::UI::Core::ICoreWindow *coreWindow() const;
    EGLDisplay eglDisplay() const; // To opengl context
    EGLSurface eglSurface() const; // To window
    EGLConfig eglConfig() const;

private:
    void handleExpose();

    HRESULT onKeyDown(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IKeyEventArgs *);
    HRESULT onKeyUp(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IKeyEventArgs *);
    HRESULT onCharacterReceived(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::ICharacterReceivedEventArgs *);
    HRESULT onPointerEntered(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IPointerEventArgs *);
    HRESULT onPointerExited(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IPointerEventArgs *);
    HRESULT onPointerUpdated(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IPointerEventArgs *);
    HRESULT onSizeChanged(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IWindowSizeChangedEventArgs *);

    HRESULT onActivated(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IWindowActivatedEventArgs *);
    HRESULT onSuspended(IInspectable *, ABI::Windows::ApplicationModel::ISuspendingEventArgs *);
    HRESULT onResume(IInspectable *, IInspectable *);

    HRESULT onClosed(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::ICoreWindowEventArgs *);
    HRESULT onVisibilityChanged(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IVisibilityChangedEventArgs *);
    HRESULT onAutomationProviderRequested(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IAutomationProviderRequestedEventArgs *);

    HRESULT onOrientationChanged(ABI::Windows::Graphics::Display::IDisplayInformation *, IInspectable *);
    HRESULT onDpiChanged(ABI::Windows::Graphics::Display::IDisplayInformation *, IInspectable *);

#ifdef Q_OS_WINPHONE
    HRESULT onBackButtonPressed(IInspectable *, ABI::Windows::Phone::UI::Input::IBackPressedEventArgs *args);
#endif

    QScopedPointer<QWinRTScreenPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTScreen)
};

QT_END_NAMESPACE

#endif // QWINRTSCREEN_H
