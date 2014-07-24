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

#include "qevdevmousemanager_p.h"

#include <QStringList>
#include <QGuiApplication>
#include <QScreen>
#include <qpa/qwindowsysteminterface.h>

//#define QT_QPA_MOUSEMANAGER_DEBUG

#ifdef QT_QPA_MOUSEMANAGER_DEBUG
#include <QDebug>
#endif

QT_BEGIN_NAMESPACE

QEvdevMouseManager::QEvdevMouseManager(const QString &key, const QString &specification, QObject *parent)
    : QObject(parent), m_x(0), m_y(0), m_xoffset(0), m_yoffset(0)
{
    Q_UNUSED(key);

    QString spec = QString::fromLocal8Bit(qgetenv("QT_QPA_EVDEV_MOUSE_PARAMETERS"));

    if (spec.isEmpty())
        spec = specification;

    QStringList args = spec.split(QLatin1Char(':'));
    QStringList devices;

    foreach (const QString &arg, args) {
        if (arg.startsWith(QLatin1String("/dev/"))) {
            // if device is specified try to use it
            devices.append(arg);
            args.removeAll(arg);
        } else if (arg.startsWith(QLatin1String("xoffset="))) {
            m_xoffset = arg.mid(8).toInt();
        } else if (arg.startsWith(QLatin1String("yoffset="))) {
            m_yoffset = arg.mid(8).toInt();
        }
    }

    // build new specification without /dev/ elements
    m_spec = args.join(QLatin1Char(':'));

    // add all mice for devices specified in the argument list
    foreach (const QString &device, devices)
        addMouse(device);

    if (devices.isEmpty()) {
#ifdef QT_QPA_MOUSEMANAGER_DEBUG
        qWarning() << "Use device discovery";
#endif

        m_deviceDiscovery = QDeviceDiscovery::create(QDeviceDiscovery::Device_Mouse | QDeviceDiscovery::Device_Touchpad, this);
        if (m_deviceDiscovery) {
            // scan and add already connected keyboards
            QStringList devices = m_deviceDiscovery->scanConnectedDevices();
            foreach (const QString &device, devices) {
                addMouse(device);
            }

            connect(m_deviceDiscovery, SIGNAL(deviceDetected(QString)), this, SLOT(addMouse(QString)));
            connect(m_deviceDiscovery, SIGNAL(deviceRemoved(QString)), this, SLOT(removeMouse(QString)));
        }
    }
}

QEvdevMouseManager::~QEvdevMouseManager()
{
    qDeleteAll(m_mice);
    m_mice.clear();
}

void QEvdevMouseManager::handleMouseEvent(int x, int y, Qt::MouseButtons buttons)
{
    // update current absolute coordinates
    m_x += x;
    m_y += y;

    // clamp to screen geometry
    QRect g = QGuiApplication::primaryScreen()->virtualGeometry();
    if (m_x + m_xoffset < g.left())
        m_x = g.left() - m_xoffset;
    else if (m_x + m_xoffset > g.right())
        m_x = g.right() - m_xoffset;

    if (m_y + m_yoffset < g.top())
        m_y = g.top() - m_yoffset;
    else if (m_y + m_yoffset > g.bottom())
        m_y = g.bottom() - m_yoffset;

    QPoint pos(m_x + m_xoffset, m_y + m_yoffset);
    QWindowSystemInterface::handleMouseEvent(0, pos, pos, buttons);

#ifdef QT_QPA_MOUSEMANAGER_DEBUG
    qDebug("mouse event %d %d %d", pos.x(), pos.y(), int(buttons));
#endif
}

void QEvdevMouseManager::handleWheelEvent(int delta, Qt::Orientation orientation)
{
    QPoint pos(m_x + m_xoffset, m_y + m_yoffset);
    QWindowSystemInterface::handleWheelEvent(0, pos, pos, delta, orientation);

#ifdef QT_QPA_MOUSEMANAGER_DEBUG
    qDebug("mouse wheel event %dx%d %d %d", pos.x(), pos.y(), delta, int(orientation));
#endif
}

void QEvdevMouseManager::addMouse(const QString &deviceNode)
{
#ifdef QT_QPA_MOUSEMANAGER_DEBUG
    qWarning() << "Adding mouse at" << deviceNode;
#endif

    QEvdevMouseHandler *handler;
    handler = QEvdevMouseHandler::create(deviceNode, m_spec);
    if (handler) {
        connect(handler, SIGNAL(handleMouseEvent(int,int,Qt::MouseButtons)), this, SLOT(handleMouseEvent(int,int,Qt::MouseButtons)));
        connect(handler, SIGNAL(handleWheelEvent(int,Qt::Orientation)), this, SLOT(handleWheelEvent(int,Qt::Orientation)));
        m_mice.insert(deviceNode, handler);
    } else {
        qWarning("Failed to open mouse");
    }
}

void QEvdevMouseManager::removeMouse(const QString &deviceNode)
{
    if (m_mice.contains(deviceNode)) {
#ifdef QT_QPA_MOUSEMANAGER_DEBUG
        qWarning() << "Removing mouse at" << deviceNode;
#endif
        QEvdevMouseHandler *handler = m_mice.value(deviceNode);
        m_mice.remove(deviceNode);
        delete handler;
    }
}

QT_END_NAMESPACE
