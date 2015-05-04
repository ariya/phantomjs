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

#include "qdevicediscovery_static_p.h"

#include <QStringList>
#include <QCoreApplication>
#include <QObject>
#include <QHash>
#include <QDir>
#include <QtCore/private/qcore_unix_p.h>

#include <linux/input.h>
#include <fcntl.h>

/* android (and perhaps some other linux-derived stuff) don't define everything
 * in linux/input.h, so we'll need to do that ourselves.
 */
#ifndef KEY_CNT
#define KEY_CNT                 (KEY_MAX+1)
#endif
#ifndef REL_CNT
#define REL_CNT                 (REL_MAX+1)
#endif
#ifndef ABS_CNT
#define ABS_CNT                 (ABS_MAX+1)
#endif


//#define QT_QPA_DEVICE_DISCOVERY_DEBUG

#ifdef QT_QPA_DEVICE_DISCOVERY_DEBUG
#include <QtDebug>
#endif

#define LONG_BITS (sizeof(long) * 8 )
#define LONG_FIELD_SIZE(bits) ((bits / LONG_BITS) + 1)

static bool testBit(long bit, const long *field)
{
    return (field[bit / LONG_BITS] >> bit % LONG_BITS) & 1;
}

QT_BEGIN_NAMESPACE

QDeviceDiscovery *QDeviceDiscovery::create(QDeviceTypes types, QObject *parent)
{
    return new QDeviceDiscoveryStatic(types, parent);
}

QDeviceDiscoveryStatic::QDeviceDiscoveryStatic(QDeviceTypes types, QObject *parent)
    : QDeviceDiscovery(types, parent)
{
#ifdef QT_QPA_DEVICE_DISCOVERY_DEBUG
    qWarning() << "New DeviceDiscovery created for type" << types;
#endif
}

QStringList QDeviceDiscoveryStatic::scanConnectedDevices()
{
    QStringList devices;
    QDir dir;
    dir.setFilter(QDir::System);

    // check for input devices
    if (m_types & Device_InputMask) {
        dir.setPath(QString::fromLatin1(QT_EVDEV_DEVICE_PATH));
        foreach (const QString &deviceFile, dir.entryList()) {
            QString absoluteFilePath = dir.absolutePath() + QString::fromLatin1("/") + deviceFile;
            if (checkDeviceType(absoluteFilePath))
                devices << absoluteFilePath;
        }
    }

    // check for drm devices
    if (m_types & Device_VideoMask) {
        dir.setPath(QString::fromLatin1(QT_DRM_DEVICE_PATH));
        foreach (const QString &deviceFile, dir.entryList()) {
            QString absoluteFilePath = dir.absolutePath() + QString::fromLatin1("/") + deviceFile;
            if (checkDeviceType(absoluteFilePath))
                devices << absoluteFilePath;
        }
    }

#ifdef QT_QPA_DEVICE_DISCOVERY_DEBUG
    qWarning() << "DeviceDiscovery found matching devices" << devices;
#endif

    return devices;
}

bool QDeviceDiscoveryStatic::checkDeviceType(const QString &device)
{
    bool ret = false;
    int fd = QT_OPEN(device.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0);
    if (!fd) {
#ifdef QT_QPA_DEVICE_DISCOVERY_DEBUG
        qWarning() << "DeviceDiscovery cannot open device" << device;
#endif
        return false;
    }

    long bitsKey[LONG_FIELD_SIZE(KEY_CNT)];
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bitsKey)), bitsKey) >= 0 ) {
        if (!ret && (m_types & Device_Keyboard)) {
            if (testBit(KEY_Q, bitsKey)) {
#ifdef QT_QPA_DEVICE_DISCOVERY_DEBUG
                qWarning() << "DeviceDiscovery found keyboard at" << device;
#endif
                ret = true;
            }
        }

        if (!ret && (m_types & Device_Mouse)) {
            long bitsRel[LONG_FIELD_SIZE(REL_CNT)];
            if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(bitsRel)), bitsRel) >= 0 ) {
                if (testBit(REL_X, bitsRel) && testBit(REL_Y, bitsRel) && testBit(BTN_MOUSE, bitsKey)) {
#ifdef QT_QPA_DEVICE_DISCOVERY_DEBUG
                    qWarning() << "DeviceDiscovery found mouse at" << device;
#endif
                    ret = true;
                }
            }
        }

        if (!ret && (m_types & (Device_Touchpad | Device_Touchscreen))) {
            long bitsAbs[LONG_FIELD_SIZE(ABS_CNT)];
            if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(bitsAbs)), bitsAbs) >= 0 ) {
                if (testBit(ABS_X, bitsAbs) && testBit(ABS_Y, bitsAbs)) {
                    if ((m_types & Device_Touchpad) && testBit(BTN_TOOL_FINGER, bitsKey)) {
#ifdef QT_QPA_DEVICE_DISCOVERY_DEBUG
                        qWarning() << "DeviceDiscovery found touchpad at" << device;
#endif
                        ret = true;
                    } else if ((m_types & Device_Touchscreen) && testBit(BTN_TOUCH, bitsKey)) {
#ifdef QT_QPA_DEVICE_DISCOVERY_DEBUG
                        qWarning() << "DeviceDiscovery found touchscreen at" << device;
#endif
                        ret = true;
                    } else if ((m_types & Device_Tablet) && (testBit(BTN_STYLUS, bitsKey) || testBit(BTN_TOOL_PEN, bitsKey))) {
#ifdef QT_QPA_DEVICE_DISCOVERY_DEBUG
                        qWarning() << "DeviceDiscovery found tablet at" << device;
#endif
                        ret = true;
                    }
                }
            }
        }
    }

    if (!ret && (m_types & Device_DRM) && device.contains(QString::fromLatin1(QT_DRM_DEVICE_PREFIX)))
        ret = true;

    QT_CLOSE(fd);
    return ret;
}

QT_END_NAMESPACE
