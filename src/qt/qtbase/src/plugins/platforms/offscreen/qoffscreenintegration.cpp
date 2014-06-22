/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qoffscreenintegration.h"
#include "qoffscreenwindow.h"
#include "qoffscreencommon.h"

#if defined(Q_OS_UNIX)
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#if defined(Q_OS_MAC)
#include <qpa/qplatformfontdatabase.h>
#else
#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#endif
#elif defined(Q_OS_WIN)
#include <QtPlatformSupport/private/qbasicfontdatabase_p.h>
#ifndef Q_OS_WINRT
#include <QtCore/private/qeventdispatcher_win_p.h>
#else
#include <QtCore/private/qeventdispatcher_winrt_p.h>
#endif
#endif

#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformservices.h>

QT_BEGIN_NAMESPACE

template <typename BaseEventDispatcher>
class QOffscreenEventDispatcher : public BaseEventDispatcher
{
public:
    explicit QOffscreenEventDispatcher(QObject *parent = 0)
        : BaseEventDispatcher(parent)
    {
    }

    bool processEvents(QEventLoop::ProcessEventsFlags flags)
    {
        bool didSendEvents = BaseEventDispatcher::processEvents(flags);

        return QWindowSystemInterface::sendWindowSystemEvents(flags) || didSendEvents;
    }

    bool hasPendingEvents()
    {
        return BaseEventDispatcher::hasPendingEvents()
            || QWindowSystemInterface::windowSystemEventsQueued();
    }

    void flush()
    {
        if (qApp)
            qApp->sendPostedEvents();
        BaseEventDispatcher::flush();
    }
};

QOffscreenIntegration::QOffscreenIntegration()
{
#if defined(Q_OS_UNIX)
#if defined(Q_OS_MAC)
    m_fontDatabase.reset(new QPlatformFontDatabase());
#else
    m_fontDatabase.reset(new QGenericUnixFontDatabase());
#endif
#elif defined(Q_OS_WIN)
    m_fontDatabase.reset(new QBasicFontDatabase());
#endif

#ifndef QT_NO_DRAGANDDROP
    m_drag.reset(new QOffscreenDrag);
#endif
    m_services.reset(new QPlatformServices);

    screenAdded(new QOffscreenScreen);
}

QOffscreenIntegration::~QOffscreenIntegration()
{
}

bool QOffscreenIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case MultipleWindows: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QOffscreenIntegration::createPlatformWindow(QWindow *window) const
{
    Q_UNUSED(window);
    QPlatformWindow *w = new QOffscreenWindow(window);
    w->requestActivateWindow();
    return w;
}

QPlatformBackingStore *QOffscreenIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QOffscreenBackingStore(window);
}

QAbstractEventDispatcher *QOffscreenIntegration::createEventDispatcher() const
{
#if defined(Q_OS_UNIX)
    return createUnixEventDispatcher();
#elif defined(Q_OS_WIN)
#ifndef Q_OS_WINRT
    return new QOffscreenEventDispatcher<QEventDispatcherWin32>();
#else // !Q_OS_WINRT
    return new QOffscreenEventDispatcher<QEventDispatcherWinRT>();
#endif // Q_OS_WINRT
#else
    return 0;
#endif
}

QPlatformFontDatabase *QOffscreenIntegration::fontDatabase() const
{
    return m_fontDatabase.data();
}

#ifndef QT_NO_DRAGANDDROP
QPlatformDrag *QOffscreenIntegration::drag() const
{
    return m_drag.data();
}
#endif

QPlatformServices *QOffscreenIntegration::services() const
{
    return m_services.data();
}

QT_END_NAMESPACE
