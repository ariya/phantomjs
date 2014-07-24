/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qtouchdevice.h"
#include "qtouchdevice_p.h"
#include <QList>
#include <QMutex>
#include <QCoreApplication>

QT_BEGIN_NAMESPACE

/*!
    \class QTouchDevice
    \brief The QTouchDevice class describes the device from which touch events originate.
    \since 5.0
    \ingroup touch
    \inmodule QtGui

    Each QTouchEvent contains a QTouchDevice pointer to allow accessing
    device-specific properties like type and capabilities. It is the
    responsibility of the platform or generic plug-ins to register the
    available touch devices via QWindowSystemInterface before generating any
    touch events. Applications do not need to instantiate this class, they
    should just access the global instances pointed to by QTouchEvent::device().
*/

/*! \enum QTouchDevice::DeviceType

    This enum represents the type of device that generated a QTouchEvent.

    \value TouchScreen In this type of device, the touch surface and display are integrated. This
                       means the surface and display typically have the same size, such that there
                       is a direct relationship between the touch points' physical positions and the
                       coordinate reported by QTouchEvent::TouchPoint. As a result, Qt allows the
                       user to interact directly with multiple QWidgets and QGraphicsItems at the
                       same time.

    \value TouchPad In this type of device, the touch surface is separate from the display. There
                    is not a direct relationship between the physical touch location and the
                    on-screen coordinates. Instead, they are calculated relative to the current
                    mouse position, and the user must use the touch-pad to move this reference
                    point. Unlike touch-screens, Qt allows users to only interact with a single
                    QWidget or QGraphicsItem at a time.
*/

/*! \enum QTouchDevice::CapabilityFlag

    This enum is used with QTouchDevice::capabilities() to indicate what kind of information the
    touch device or its driver can provide.

    \value Position Indicates that position information is available, meaning
                    that the pos() family of functions in the touch points return valid points.

    \value Area Indicates that touch area information is available, meaning that the rect() family
                of functions in the touch points return valid rectangles.

    \value Pressure Indicates that pressure information is available, meaning that pressure()
                    returns a valid value.

    \value Velocity Indicates that velocity information is available, meaning that velocity()
                    returns a valid vector.

    \value RawPositions Indicates that the list returned by QTouchEvent::TouchPoint::rawScreenPositions()
                        may contain one or more positions for each touch point. This is relevant when
                        the touch input gets filtered or corrected on driver level.

    \value NormalizedPosition Indicates that the normalized position is available, meaning that normalizedPos()
                              returns a valid value.
*/

/*!
  Creates a new touch device instance.
  By default the name is empty, the only capability is Position and type is TouchScreen.
  */
QTouchDevice::QTouchDevice()
    : d(new QTouchDevicePrivate)
{
}

/*!
  Destroys a touch device instance.
  */
QTouchDevice::~QTouchDevice()
{
    delete d;
}

/*!
    Returns the touch device type.
*/
QTouchDevice::DeviceType QTouchDevice::type() const
{
    return d->type;
}

/*!
    Returns the touch device capabilities.
  */
QTouchDevice::Capabilities QTouchDevice::capabilities() const
{
    return d->caps;
}

/*!
    Returns the maximum number of simultaneous touch points (fingers) that
    can be detected.
    \since 5.2
  */
int QTouchDevice::maximumTouchPoints() const
{
    return d->maxTouchPoints;
}

/*!
    Returns the touch device name.

    This string may often be empty. It is however useful for systems that have
    more than one touch input device because there it can be used to
    differentiate between the devices (i.e. to tell from which device a
    QTouchEvent originates from).
*/
QString QTouchDevice::name() const
{
    return d->name;
}

/*!
  Sets the device type \a devType.
  */
void QTouchDevice::setType(DeviceType devType)
{
    d->type = devType;
}

/*!
  Sets the capabilities \a caps supported by the device and its driver.
  */
void QTouchDevice::setCapabilities(Capabilities caps)
{
    d->caps = caps;
}

/*!
  Sets the maximum number of simultaneous touchpoints \a max
  supported by the device and its driver.
  */
void QTouchDevice::setMaximumTouchPoints(int max)
{
    d->maxTouchPoints = max;
}

/*!
  Sets the \a name (a unique identifier) for the device. In most systems it is
  enough to leave this unset and keep the default empty name. This identifier
  becomes important when having multiple touch devices and a need to
  differentiate between them.
  */
void QTouchDevice::setName(const QString &name)
{
    d->name = name;
}

typedef QList<QTouchDevice *> TouchDevices;
Q_GLOBAL_STATIC(TouchDevices, deviceList)
static QBasicMutex devicesMutex;

static void cleanupDevicesList()
{
    QMutexLocker lock(&devicesMutex);
    qDeleteAll(*deviceList());
    deviceList()->clear();
}

/*!
  Returns a list of all registered devices.

  \note The returned list cannot be used to add new devices. Use QWindowSystemInterface::registerTouchDevice() instead.
  */
QList<const QTouchDevice *> QTouchDevice::devices()
{
    QMutexLocker lock(&devicesMutex);
    QList<QTouchDevice *> *devList = deviceList();
    QList<const QTouchDevice *> constDevList;
    for (int i = 0, count = devList->count(); i != count; ++i)
        constDevList.append(devList->at(i));
    return constDevList;
}

/*!
  \internal
  */
bool QTouchDevicePrivate::isRegistered(QTouchDevice *dev)
{
    QMutexLocker lock(&devicesMutex);
    return deviceList()->contains(dev);
}

/*!
  \internal
  */
void QTouchDevicePrivate::registerDevice(QTouchDevice *dev)
{
    QMutexLocker lock(&devicesMutex);
    if (deviceList()->isEmpty())
        qAddPostRoutine(cleanupDevicesList);
    deviceList()->append(dev);
}

QT_END_NAMESPACE
