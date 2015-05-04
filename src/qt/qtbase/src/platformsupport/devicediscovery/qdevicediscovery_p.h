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

#ifndef QDEVICEDISCOVERY_H
#define QDEVICEDISCOVERY_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QObject>
#include <QSocketNotifier>
#include <QStringList>

#define QT_EVDEV_DEVICE_PATH "/dev/input/"
#define QT_EVDEV_DEVICE_PREFIX "event"
#define QT_EVDEV_DEVICE QT_EVDEV_DEVICE_PATH QT_EVDEV_DEVICE_PREFIX

#define QT_DRM_DEVICE_PATH "/dev/dri/"
#define QT_DRM_DEVICE_PREFIX "card"
#define QT_DRM_DEVICE QT_DRM_DEVICE_PATH QT_DRM_DEVICE_PREFIX

QT_BEGIN_NAMESPACE

class QDeviceDiscovery : public QObject
{
    Q_OBJECT
    Q_ENUMS(QDeviceType)

public:
    enum QDeviceType {
        Device_Unknown = 0x00,
        Device_Mouse = 0x01,
        Device_Touchpad = 0x02,
        Device_Touchscreen = 0x04,
        Device_Keyboard = 0x08,
        Device_DRM = 0x10,
        Device_DRM_PrimaryGPU = 0x20,
        Device_Tablet = 0x40,
        Device_InputMask = Device_Mouse | Device_Touchpad | Device_Touchscreen | Device_Keyboard | Device_Tablet,
        Device_VideoMask = Device_DRM
    };
    Q_DECLARE_FLAGS(QDeviceTypes, QDeviceType)

    static QDeviceDiscovery *create(QDeviceTypes type, QObject *parent = 0);

    virtual QStringList scanConnectedDevices() = 0;

signals:
    void deviceDetected(const QString &deviceNode);
    void deviceRemoved(const QString &deviceNode);

protected:
    QDeviceDiscovery(QDeviceTypes types, QObject *parent) : QObject(parent), m_types(types) { }
    Q_DISABLE_COPY(QDeviceDiscovery)

    QDeviceTypes m_types;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QDeviceDiscovery::QDeviceTypes)

QT_END_NAMESPACE

#endif // QDEVICEDISCOVERY_H
