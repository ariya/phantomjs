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

#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dintegration.h"
#include "qwindowsdirect2dbackingstore.h"
#include "qwindowsdirect2dplatformpixmap.h"
#include "qwindowsdirect2dnativeinterface.h"
#include "qwindowsdirect2dwindow.h"

#include "qwindowscontext.h"
#include "qwindowsguieventdispatcher.h"

#include <qplatformdefs.h>
#include <QtCore/QCoreApplication>
#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

class QWindowsDirect2DEventDispatcher : public QWindowsGuiEventDispatcher
{
public:
    QWindowsDirect2DEventDispatcher(QObject *parent = 0)
        : QWindowsGuiEventDispatcher(parent)
    {
        uninstallMessageHook(); // ### Workaround for QTBUG-42428
    }
};

class QWindowsDirect2DIntegrationPrivate
{
public:
    QWindowsDirect2DNativeInterface m_nativeInterface;
    QWindowsDirect2DContext m_d2dContext;
};

class Direct2DVersion
{
private:
    Direct2DVersion()
        : partOne(0)
        , partTwo(0)
        , partThree(0)
        , partFour(0)
    {}

    Direct2DVersion(int one, int two, int three, int four)
        : partOne(one)
        , partTwo(two)
        , partThree(three)
        , partFour(four)
    {}

public:
    // 6.2.9200.16492 corresponds to Direct2D 1.1 on Windows 7 SP1 with Platform Update
    enum {
        D2DMinVersionPart1 = 6,
        D2DMinVersionPart2 = 2,
        D2DMinVersionPart3 = 9200,
        D2DMinVersionPart4 = 16492
    };

    static Direct2DVersion systemVersion() {
        static const int bufSize = 512;
        TCHAR filename[bufSize];

        UINT i = GetSystemDirectory(filename, bufSize);
        if (i > 0 && i < bufSize) {
            if (_tcscat_s(filename, bufSize, __TEXT("\\d2d1.dll")) == 0) {
                DWORD versionInfoSize = GetFileVersionInfoSize(filename, NULL);
                if (versionInfoSize) {
                    QVector<BYTE> info(versionInfoSize);
                    if (GetFileVersionInfo(filename, NULL, versionInfoSize, info.data())) {
                        UINT size;
                        DWORD *fi;

                        if (VerQueryValue(info.constData(), __TEXT("\\"), (LPVOID *) &fi, &size) && size) {
                            VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *) fi;
                            return Direct2DVersion(HIWORD(verInfo->dwFileVersionMS),
                                                   LOWORD(verInfo->dwFileVersionMS),
                                                   HIWORD(verInfo->dwFileVersionLS),
                                                   LOWORD(verInfo->dwFileVersionLS));
                        }
                    }
                }
            }
        }

        return Direct2DVersion();
    }

    static Direct2DVersion minimumVersion() {
        return Direct2DVersion(D2DMinVersionPart1,
                               D2DMinVersionPart2,
                               D2DMinVersionPart3,
                               D2DMinVersionPart4);
    }

    bool isValid() const {
        return partOne || partTwo || partThree || partFour;
    }

    bool operator<(const Direct2DVersion &other) {
        int c = cmp(partOne, other.partOne);
        if (c > 0)
            return false;
        if (c < 0)
            return true;

        c = cmp(partTwo, other.partTwo);
        if (c > 0)
            return false;
        if (c < 0)
            return true;

        c = cmp(partThree, other.partThree);
        if (c > 0)
            return false;
        if (c < 0)
            return true;

        c = cmp(partFour, other.partFour);
        if (c > 0)
            return false;
        if (c < 0)
            return true;

        return false;
    }

    static Q_DECL_CONSTEXPR int cmp(int a, int b) {
        return a - b;
    }

    int partOne, partTwo, partThree, partFour;
};

QWindowsDirect2DIntegration *QWindowsDirect2DIntegration::create(const QStringList &paramList)
{
    Direct2DVersion systemVersion = Direct2DVersion::systemVersion();

    if (systemVersion.isValid() && systemVersion < Direct2DVersion::minimumVersion()) {
        QString msg = QCoreApplication::translate("QWindowsDirect2DIntegration",
            "Qt cannot load the direct2d platform plugin because " \
            "the Direct2D version on this system is too old. The " \
            "minimum system requirement for this platform plugin " \
            "is Windows 7 SP1 with Platform Update.\n\n" \
            "The minimum Direct2D version required is %1.%2.%3.%4. " \
            "The Direct2D version on this system is %5.%6.%7.%8.");

        msg = msg.arg(Direct2DVersion::D2DMinVersionPart1)
                 .arg(Direct2DVersion::D2DMinVersionPart2)
                 .arg(Direct2DVersion::D2DMinVersionPart3)
                 .arg(Direct2DVersion::D2DMinVersionPart4)
                 .arg(systemVersion.partOne)
                 .arg(systemVersion.partTwo)
                 .arg(systemVersion.partThree)
                 .arg(systemVersion.partFour);

        QString caption = QCoreApplication::translate("QWindowsDirect2DIntegration",
            "Cannot load direct2d platform plugin");

        MessageBoxW(NULL,
                    msg.toStdWString().c_str(),
                    caption.toStdWString().c_str(),
                    MB_OK | MB_ICONERROR);

        return Q_NULLPTR;
    }

    QWindowsDirect2DIntegration *integration = new QWindowsDirect2DIntegration(paramList);

    if (!integration->init()) {
        delete integration;
        integration = 0;
    }

    return integration;
}

QWindowsDirect2DIntegration::~QWindowsDirect2DIntegration()
{

}

 QWindowsDirect2DIntegration *QWindowsDirect2DIntegration::instance()
 {
     return static_cast<QWindowsDirect2DIntegration *>(QWindowsIntegration::instance());
 }

 QPlatformWindow *QWindowsDirect2DIntegration::createPlatformWindow(QWindow *window) const
 {
     QWindowsWindowData data = createWindowData(window);
     return data.hwnd ? new QWindowsDirect2DWindow(window, data)
                      : Q_NULLPTR;
 }

 QPlatformNativeInterface *QWindowsDirect2DIntegration::nativeInterface() const
 {
     return &d->m_nativeInterface;
 }

QPlatformPixmap *QWindowsDirect2DIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
    switch (type) {
    case QPlatformPixmap::BitmapType:
        return new QRasterPlatformPixmap(type);
        break;
    default:
        return new QWindowsDirect2DPlatformPixmap(type);
        break;
    }
}

QPlatformBackingStore *QWindowsDirect2DIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QWindowsDirect2DBackingStore(window);
}

QAbstractEventDispatcher *QWindowsDirect2DIntegration::createEventDispatcher() const
{
    return new QWindowsDirect2DEventDispatcher;
}

QWindowsDirect2DContext *QWindowsDirect2DIntegration::direct2DContext() const
{
    return &d->m_d2dContext;
}

QWindowsDirect2DIntegration::QWindowsDirect2DIntegration(const QStringList &paramList)
    : QWindowsIntegration(paramList)
    , d(new QWindowsDirect2DIntegrationPrivate)
{
}

bool QWindowsDirect2DIntegration::init()
{
    return d->m_d2dContext.init();
}

QT_END_NAMESPACE
