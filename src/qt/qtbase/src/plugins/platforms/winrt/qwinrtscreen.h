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

#ifndef QWINRTSCREEN_H
#define QWINRTSCREEN_H

#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtCore/QHash>
#include <QtGui/QSurfaceFormat>
#include <EGL/egl.h>

#include <EventToken.h>

namespace ABI {
    namespace Windows {
        namespace ApplicationModel {
            struct ISuspendingEventArgs;
            namespace Core {
                struct ICoreApplication;
            }
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
            namespace ViewManagement {
                struct IApplicationViewStatics;
            }
        }
        namespace Graphics {
            namespace Display {
                struct IDisplayPropertiesStatics;
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
class QWinRTPageFlipper;
class QWinRTCursor;
class QWinRTInputContext;

class QWinRTScreen : public QPlatformScreen
{
public:
    explicit QWinRTScreen(ABI::Windows::UI::Core::ICoreWindow *window);
    QRect geometry() const;
    int depth() const;
    QImage::Format format() const;
    QSurfaceFormat surfaceFormat() const;
    QWinRTInputContext *inputContext() const;
    QPlatformCursor *cursor() const;
    Qt::KeyboardModifiers keyboardModifiers() const;

    Qt::ScreenOrientation nativeOrientation() const;
    Qt::ScreenOrientation orientation() const;
    void setOrientationUpdateMask(Qt::ScreenOrientations mask);

    QWindow *topWindow() const;
    void addWindow(QWindow *window);
    void removeWindow(QWindow *window);
    void raise(QWindow *window);
    void lower(QWindow *window);

    ABI::Windows::UI::Core::ICoreWindow *coreWindow() const;
    EGLDisplay eglDisplay() const; // To opengl context
    EGLSurface eglSurface() const; // To window

private:
    void handleExpose();

    // Event handlers
    QHash<QEvent::Type, EventRegistrationToken> m_tokens;
    QHash<Qt::ApplicationState, EventRegistrationToken> m_suspendTokens;

    HRESULT onKeyDown(ABI::Windows::UI::Core::ICoreWindow *window, ABI::Windows::UI::Core::IKeyEventArgs *args);
    HRESULT onKeyUp(ABI::Windows::UI::Core::ICoreWindow *window, ABI::Windows::UI::Core::IKeyEventArgs *args);
    HRESULT onCharacterReceived(ABI::Windows::UI::Core::ICoreWindow *window, ABI::Windows::UI::Core::ICharacterReceivedEventArgs *args);
    HRESULT onPointerEntered(ABI::Windows::UI::Core::ICoreWindow *window, ABI::Windows::UI::Core::IPointerEventArgs *args);
    HRESULT onPointerExited(ABI::Windows::UI::Core::ICoreWindow *window, ABI::Windows::UI::Core::IPointerEventArgs *args);
    HRESULT onPointerUpdated(ABI::Windows::UI::Core::ICoreWindow *window, ABI::Windows::UI::Core::IPointerEventArgs *args);
    HRESULT onSizeChanged(ABI::Windows::UI::Core::ICoreWindow *window, ABI::Windows::UI::Core::IWindowSizeChangedEventArgs *args);

    HRESULT onActivated(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IWindowActivatedEventArgs *args);
    HRESULT onSuspended(IInspectable *, ABI::Windows::ApplicationModel::ISuspendingEventArgs *);
    HRESULT onResume(IInspectable *, IInspectable *);

    HRESULT onClosed(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::ICoreWindowEventArgs *args);
    HRESULT onVisibilityChanged(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IVisibilityChangedEventArgs *args);
    HRESULT onAutomationProviderRequested(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IAutomationProviderRequestedEventArgs *args);

    HRESULT onOrientationChanged(IInspectable *);

#ifdef Q_OS_WINPHONE
    HRESULT onBackButtonPressed(IInspectable *, ABI::Windows::Phone::UI::Input::IBackPressedEventArgs *args);
#endif

    ABI::Windows::UI::Core::ICoreWindow *m_coreWindow;
    ABI::Windows::UI::ViewManagement::IApplicationViewStatics *m_applicationView;
    ABI::Windows::ApplicationModel::Core::ICoreApplication *m_application;

    QRect m_geometry;
    QImage::Format m_format;
    QSurfaceFormat m_surfaceFormat;
    int m_depth;
    QWinRTInputContext *m_inputContext;
    QWinRTCursor *m_cursor;
    QList<QWindow *> m_visibleWindows;

    EGLDisplay m_eglDisplay;
    EGLSurface m_eglSurface;

    ABI::Windows::Graphics::Display::IDisplayPropertiesStatics *m_displayProperties;
    Qt::ScreenOrientation m_nativeOrientation;
    Qt::ScreenOrientation m_orientation;

#ifndef Q_OS_WINPHONE
    QHash<quint32, QPair<Qt::Key, QString> > m_activeKeys;
#endif
    QTouchDevice *m_touchDevice;
    QHash<quint32, QWindowSystemInterface::TouchPoint> m_touchPoints;
};

QT_END_NAMESPACE

#endif // QWINRTSCREEN_H
