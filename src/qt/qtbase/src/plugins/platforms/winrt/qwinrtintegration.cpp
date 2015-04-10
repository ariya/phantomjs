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

#include "qwinrtintegration.h"
#include "qwinrtwindow.h"
#include "qwinrteventdispatcher.h"
#include "qwinrtbackingstore.h"
#include "qwinrtscreen.h"
#include "qwinrtinputcontext.h"
#include "qwinrtservices.h"
#include "qwinrteglcontext.h"
#include "qwinrtfontdatabase.h"
#include "qwinrtplatformtheme.h"

#include <QtGui/QOpenGLContext>

#include <wrl.h>
#include <windows.ui.core.h>
#include <windows.ui.viewmanagement.h>
#include <Windows.ApplicationModel.core.h>

using namespace Microsoft::WRL;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::UI::Core;
using namespace ABI::Windows::UI::ViewManagement;
using namespace ABI::Windows::ApplicationModel::Core;

static IUISettings *getSettings()
{
    static IUISettings *settings = 0;
    if (!settings) {
        if (FAILED(RoActivateInstance(Wrappers::HString::MakeReference(RuntimeClass_Windows_UI_ViewManagement_UISettings).Get(),
                                      reinterpret_cast<IInspectable **>(&settings)))) {
            qWarning("Could not activate UISettings.");
        }
    }
    return settings;
}

QT_BEGIN_NAMESPACE

QWinRTIntegration::QWinRTIntegration()
    : m_success(false)
    , m_fontDatabase(new QWinRTFontDatabase)
    , m_services(new QWinRTServices)
{
    // Obtain the WinRT Application, view, and window
    ICoreApplication *application;
    if (FAILED(RoGetActivationFactory(Wrappers::HString::MakeReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(),
                                      IID_PPV_ARGS(&application))))
        qCritical("Could not attach to the application factory.");

    ICoreApplicationView *view;
    if (FAILED(application->GetCurrentView(&view))) {
        qCritical("Could not obtain the application view - have you started outside of WinRT?");
        return;
    }

    // Get core window (will act as our screen)
    ICoreWindow *window;
    if (FAILED(view->get_CoreWindow(&window))) {
        qCritical("Could not obtain the application window - have you started outside of WinRT?");
        return;
    }
    window->Activate();
    m_screen = new QWinRTScreen(window);
    screenAdded(m_screen);

    m_success = true;
}

QWinRTIntegration::~QWinRTIntegration()
{
    Windows::Foundation::Uninitialize();
}

QAbstractEventDispatcher *QWinRTIntegration::createEventDispatcher() const
{
    return new QWinRTEventDispatcher;
}

bool QWinRTIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
    case OpenGL:
    case ApplicationState:
        return true;
    case NonFullScreenWindows:
        return false;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QVariant QWinRTIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
    case CursorFlashTime:
        if (IUISettings *settings = getSettings()) {
            quint32 blinkRate;
            settings->get_CaretBlinkRate(&blinkRate);
            return blinkRate;
        }
        break;
    case MouseDoubleClickInterval:
        if (IUISettings *settings = getSettings()) {
            quint32 doubleClickTime;
            settings->get_DoubleClickTime(&doubleClickTime);
            return doubleClickTime;
        }
    case ShowIsFullScreen:
        return true;
    default:
        break;
    }
    return QPlatformIntegration::styleHint(hint);
}

QPlatformWindow *QWinRTIntegration::createPlatformWindow(QWindow *window) const
{
    return new QWinRTWindow(window);
}

QPlatformBackingStore *QWinRTIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QWinRTBackingStore(window);
}

QPlatformOpenGLContext *QWinRTIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QWinRTScreen *screen = static_cast<QWinRTScreen *>(context->screen()->handle());
    return new QWinRTEGLContext(context->format(), context->handle(), screen->eglDisplay(), screen->eglSurface());
}

QPlatformFontDatabase *QWinRTIntegration::fontDatabase() const
{
    return m_fontDatabase;
}

QPlatformInputContext *QWinRTIntegration::inputContext() const
{
    return m_screen->inputContext();
}

QPlatformServices *QWinRTIntegration::services() const
{
    return m_services;
}

Qt::KeyboardModifiers QWinRTIntegration::queryKeyboardModifiers() const
{
    return m_screen->keyboardModifiers();
}

QStringList QWinRTIntegration::themeNames() const
{
    return QStringList(QLatin1String("winrt"));
}

QPlatformTheme *QWinRTIntegration::createPlatformTheme(const QString &
name) const
{
    if (name == QLatin1String("winrt"))
        return new QWinRTPlatformTheme();

    return 0;
}
QT_END_NAMESPACE
