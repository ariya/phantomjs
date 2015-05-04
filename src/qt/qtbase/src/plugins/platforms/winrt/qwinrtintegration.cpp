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

#include "qwinrtintegration.h"
#include "qwinrtwindow.h"
#include "qwinrteventdispatcher.h"
#include "qwinrtbackingstore.h"
#include "qwinrtscreen.h"
#include "qwinrtinputcontext.h"
#include "qwinrtservices.h"
#include "qwinrteglcontext.h"
#include "qwinrtfontdatabase.h"
#include "qwinrttheme.h"

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

QT_BEGIN_NAMESPACE

QWinRTIntegration::QWinRTIntegration()
    : m_success(false)
    , m_fontDatabase(new QWinRTFontDatabase)
    , m_services(new QWinRTServices)
{
    m_screen = new QWinRTScreen;
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
    return QWinRTTheme::styleHint(hint);
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
    return new QWinRTEGLContext(context->format(), context->handle(), screen->eglDisplay(), screen->eglSurface(), screen->eglConfig());
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
        return new QWinRTTheme();

    return 0;
}
QT_END_NAMESPACE
