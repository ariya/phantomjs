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

#include "qwinrtwindow.h"
#include "qwinrtscreen.h"

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformscreen.h>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>

#include <qfunctions_winrt.h>
#include <windows.ui.viewmanagement.h>
#include <wrl.h>

using namespace ABI::Windows::UI::ViewManagement;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

QT_BEGIN_NAMESPACE

QWinRTWindow::QWinRTWindow(QWindow *window)
    : QPlatformWindow(window)
    , m_screen(static_cast<QWinRTScreen*>(screen()))
{
    setWindowFlags(window->flags());
    setWindowState(window->windowState());
    setWindowTitle(window->title());
    handleContentOrientationChange(window->contentOrientation());
    setGeometry(window->geometry());
}

QWinRTWindow::~QWinRTWindow()
{
    m_screen->removeWindow(window());
}

QSurfaceFormat QWinRTWindow::format() const
{
    return m_screen->surfaceFormat();
}

bool QWinRTWindow::isActive() const
{
    return m_screen->topWindow() == window();
}

bool QWinRTWindow::isExposed() const
{
    const bool exposed = isActive();
    return exposed;
}

void QWinRTWindow::setGeometry(const QRect &rect)
{
    if (window()->isTopLevel()) {
        QPlatformWindow::setGeometry(m_screen->geometry());
        QWindowSystemInterface::handleGeometryChange(window(), geometry());
    } else {
        QPlatformWindow::setGeometry(rect);
        QWindowSystemInterface::handleGeometryChange(window(), rect);
    }
}

void QWinRTWindow::setVisible(bool visible)
{
    if (!window()->isTopLevel())
        return;
    if (visible)
        m_screen->addWindow(window());
    else
        m_screen->removeWindow(window());
}

void QWinRTWindow::setWindowTitle(const QString &title)
{
    ComPtr<IApplicationViewStatics2> statics;
    HRESULT hr;

    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_ViewManagement_ApplicationView).Get(),
                              IID_PPV_ARGS(&statics));
    RETURN_VOID_IF_FAILED("Could not get ApplicationViewStatics");

    ComPtr<IApplicationView> view;
    hr = statics->GetForCurrentView(&view);
    RETURN_VOID_IF_FAILED("Could not access currentView");

    HStringReference str(reinterpret_cast<LPCWSTR>(title.utf16()), title.length());
    hr = view->put_Title(str.Get());
    RETURN_VOID_IF_FAILED("Unable to set window title");
}

void QWinRTWindow::raise()
{
    if (!window()->isTopLevel())
        return;
    m_screen->raise(window());
}

void QWinRTWindow::lower()
{
    if (!window()->isTopLevel())
        return;
    m_screen->lower(window());
}

qreal QWinRTWindow::devicePixelRatio() const
{
    return screen()->devicePixelRatio();
}

QT_END_NAMESPACE
