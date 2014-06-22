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

#include "qminimalintegration.h"
#include "qminimalbackingstore.h"

#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformfontdatabase.h>

#if !defined(Q_OS_WIN)
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#elif defined(Q_OS_WINRT)
#include <QtCore/private/qeventdispatcher_winrt_p.h>
#else
#include <QtCore/private/qeventdispatcher_win_p.h>
#endif

QT_BEGIN_NAMESPACE

static const char debugBackingStoreEnvironmentVariable[] = "QT_DEBUG_BACKINGSTORE";

static inline unsigned parseOptions(const QStringList &paramList)
{
    unsigned options = 0;
    foreach (const QString &param, paramList) {
        if (param == QLatin1String("enable_fonts"))
            options |= QMinimalIntegration::EnableFonts;
    }
    return options;
}

QMinimalIntegration::QMinimalIntegration(const QStringList &parameters)
    : m_dummyFontDatabase(0)
    , m_options(parseOptions(parameters))
{
    if (qEnvironmentVariableIsSet(debugBackingStoreEnvironmentVariable)
        && qgetenv(debugBackingStoreEnvironmentVariable).toInt() > 0) {
        m_options |= DebugBackingStore | EnableFonts;
    }

    QMinimalScreen *mPrimaryScreen = new QMinimalScreen();

    mPrimaryScreen->mGeometry = QRect(0, 0, 240, 320);
    mPrimaryScreen->mDepth = 32;
    mPrimaryScreen->mFormat = QImage::Format_ARGB32_Premultiplied;

    screenAdded(mPrimaryScreen);
}

QMinimalIntegration::~QMinimalIntegration()
{
    delete m_dummyFontDatabase;
}

bool QMinimalIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case MultipleWindows: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

// Dummy font database that does not scan the fonts directory to be
// used for command line tools like qmlplugindump that do not create windows
// unless DebugBackingStore is activated.
class DummyFontDatabase : public QPlatformFontDatabase
{
public:
    virtual void populateFontDatabase() {}
};

QPlatformFontDatabase *QMinimalIntegration::fontDatabase() const
{
    if (m_options & EnableFonts)
        return QPlatformIntegration::fontDatabase();
    if (!m_dummyFontDatabase)
        m_dummyFontDatabase = new DummyFontDatabase;
    return m_dummyFontDatabase;
}

QPlatformWindow *QMinimalIntegration::createPlatformWindow(QWindow *window) const
{
    Q_UNUSED(window);
    QPlatformWindow *w = new QPlatformWindow(window);
    w->requestActivateWindow();
    return w;
}

QPlatformBackingStore *QMinimalIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QMinimalBackingStore(window);
}

QAbstractEventDispatcher *QMinimalIntegration::createEventDispatcher() const
{
#ifdef Q_OS_WIN
#ifndef Q_OS_WINRT
    return new QEventDispatcherWin32;
#else // !Q_OS_WINRT
    return new QEventDispatcherWinRT;
#endif // Q_OS_WINRT
#else
    return createUnixEventDispatcher();
#endif
}

QMinimalIntegration *QMinimalIntegration::instance()
{
    return static_cast<QMinimalIntegration *>(QGuiApplicationPrivate::platformIntegration());
}

QT_END_NAMESPACE
