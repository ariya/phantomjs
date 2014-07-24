/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins module of the Qt Toolkit.
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

#include "qevdevtablet_p.h"
#include <qpa/qwindowsysteminterface.h>
#include <QStringList>
#include <QSocketNotifier>
#include <QGuiApplication>
#include <QDebug>
#include <QtCore/private/qcore_unix_p.h>
#include <QtPlatformSupport/private/qdevicediscovery_p.h>
#include <linux/input.h>

QT_BEGIN_NAMESPACE

class QEvdevTabletData
{
public:
    QEvdevTabletData(QEvdevTabletHandler *q_ptr);
    bool queryLimits();
    void testGrab();
    void processInputEvent(input_event *ev);
    void reportProximityEnter();
    void reportProximityLeave();
    void report();

    QEvdevTabletHandler *q;
    QSocketNotifier *notifier;
    int fd;
    int lastEventType;
    QString devName;
    struct {
        int x, y, p, d;
    } minValues, maxValues;
    struct {
        int x, y, p, d;
        bool down, lastReportDown;
        int tool, lastReportTool;
        QPointF lastReportPos;
    } state;
};

QEvdevTabletData::QEvdevTabletData(QEvdevTabletHandler *q_ptr)
    : q(q_ptr), notifier(0), fd(-1), lastEventType(0)
{
    memset(&minValues, 0, sizeof(minValues));
    memset(&maxValues, 0, sizeof(maxValues));
    memset(&state, 0, sizeof(state));
}

bool QEvdevTabletData::queryLimits()
{
    bool ok = true;
    input_absinfo absInfo;
    memset(&absInfo, 0, sizeof(input_absinfo));
    ok &= ioctl(fd, EVIOCGABS(ABS_X), &absInfo) >= 0;
    if (ok) {
        minValues.x = absInfo.minimum;
        maxValues.x = absInfo.maximum;
        qDebug("evdevtablet: min X: %d max X: %d", minValues.x, maxValues.x);
    }
    ok &= ioctl(fd, EVIOCGABS(ABS_Y), &absInfo) >= 0;
    if (ok) {
        minValues.y = absInfo.minimum;
        maxValues.y = absInfo.maximum;
        qDebug("evdevtablet: min Y: %d max Y: %d", minValues.y, maxValues.y);
    }
    if (ioctl(fd, EVIOCGABS(ABS_PRESSURE), &absInfo) >= 0) {
        minValues.p = absInfo.minimum;
        maxValues.p = absInfo.maximum;
        qDebug("evdevtablet: min pressure: %d max pressure: %d", minValues.p, maxValues.p);
    }
    if (ioctl(fd, EVIOCGABS(ABS_DISTANCE), &absInfo) >= 0) {
        minValues.d = absInfo.minimum;
        maxValues.d = absInfo.maximum;
        qDebug("evdevtablet: min distance: %d max distance: %d", minValues.d, maxValues.d);
    }
    char name[128];
    if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name) >= 0) {
        devName = QString::fromLocal8Bit(name);
        qDebug("evdevtablet: device name: %s", name);
    }
    return ok;
}

void QEvdevTabletData::testGrab()
{
    bool grabSuccess = !ioctl(fd, EVIOCGRAB, (void *) 1);
    if (grabSuccess)
        ioctl(fd, EVIOCGRAB, (void *) 0);
    else
        qWarning("evdevtablet: ERROR: The device is grabbed by another process. No events will be read.");
}

void QEvdevTabletData::processInputEvent(input_event *ev)
{
    if (ev->type == EV_ABS) {
        switch (ev->code) {
        case ABS_X:
            state.x = ev->value;
            break;
        case ABS_Y:
            state.y = ev->value;
            break;
        case ABS_PRESSURE:
            state.p = ev->value;
            break;
        case ABS_DISTANCE:
            state.d = ev->value;
            break;
        default:
            break;
        }
    } else if (ev->type == EV_KEY) {
        // code BTN_TOOL_* value 1 -> proximity enter
        // code BTN_TOOL_* value 0 -> proximity leave
        // code BTN_TOUCH value 1 -> contact with screen
        // code BTN_TOUCH value 0 -> no contact
        switch (ev->code) {
        case BTN_TOUCH:
            state.down = ev->value != 0;
            break;
        case BTN_TOOL_PEN:
            state.tool = ev->value ? QTabletEvent::Pen : 0;
            break;
        case BTN_TOOL_RUBBER:
            state.tool = ev->value ? QTabletEvent::Eraser : 0;
            break;
        default:
            break;
        }
    } else if (ev->type == EV_SYN && ev->code == SYN_REPORT && lastEventType != ev->type) {
        report();
    }
    lastEventType = ev->type;
}

void QEvdevTabletData::reportProximityEnter()
{
    QWindowSystemInterface::handleTabletEnterProximityEvent(QTabletEvent::Stylus, state.tool, 1);
}

void QEvdevTabletData::reportProximityLeave()
{
    QWindowSystemInterface::handleTabletLeaveProximityEvent(QTabletEvent::Stylus, state.tool, 1);
}

void QEvdevTabletData::report()
{
    if (!state.lastReportTool && state.tool)
        reportProximityEnter();

    qreal nx = (state.x - minValues.x) / qreal(maxValues.x - minValues.x);
    qreal ny = (state.y - minValues.y) / qreal(maxValues.y - minValues.y);

    QRect winRect = QGuiApplication::primaryScreen()->geometry();
    QPointF globalPos(nx * winRect.width(), ny * winRect.height());
    int pointer = state.tool;
    // Prevent sending confusing values of 0 when moving the pen outside the active area.
    if (!state.down && state.lastReportDown) {
        globalPos = state.lastReportPos;
        pointer = state.lastReportTool;
    }

    qreal pressure = (state.p - minValues.p) / qreal(maxValues.p - minValues.p);

    if (state.down || state.lastReportDown) {
        QWindowSystemInterface::handleTabletEvent(0, state.down, QPointF(), globalPos,
                                                  QTabletEvent::Stylus, pointer,
                                                  pressure, 0, 0, 0, 0, 0, 1, qGuiApp->keyboardModifiers());
    }

    if (state.lastReportTool && !state.tool)
        reportProximityLeave();

    state.lastReportDown = state.down;
    state.lastReportTool = state.tool;
    state.lastReportPos = globalPos;
}


QEvdevTabletHandler::QEvdevTabletHandler(const QString &spec, QObject *parent)
    : QObject(parent), d(0)
{
    setObjectName(QLatin1String("Evdev Tablet Handler"));
    d = new QEvdevTabletData(this);
    QString dev;
    QStringList args = spec.split(QLatin1Char(':'));
    for (int i = 0; i < args.count(); ++i) {
        if (args.at(i).startsWith(QLatin1String("/dev/"))) {
            dev = args.at(i);
            break;
        }
    }
    if (dev.isEmpty()) {
        QScopedPointer<QDeviceDiscovery> deviceDiscovery(
                    QDeviceDiscovery::create(QDeviceDiscovery::Device_Tablet, this));
        if (deviceDiscovery) {
            QStringList devices = deviceDiscovery->scanConnectedDevices();
            if (!devices.isEmpty())
                dev = devices.at(0);
        }
    }
    if (!dev.isEmpty()) {
        qDebug("evdevtablet: using %s", qPrintable(dev));
        d->fd = QT_OPEN(dev.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0);
        if (d->fd >= 0) {
            d->testGrab();
            if (d->queryLimits()) {
                d->notifier = new QSocketNotifier(d->fd, QSocketNotifier::Read, this);
                connect(d->notifier, SIGNAL(activated(int)), this, SLOT(readData()));
            }
        } else {
            qErrnoWarning(errno, "evdevtablet: Cannot open input device");
        }
    }
}

QEvdevTabletHandler::~QEvdevTabletHandler()
{
    delete d->notifier;
    if (d->fd >= 0)
        QT_CLOSE(d->fd);

    delete d;
}

void QEvdevTabletHandler::readData()
{
    static input_event buffer[32];
    int n = 0;
    for (; ;) {
        int result = QT_READ(d->fd, reinterpret_cast<char*>(buffer) + n, sizeof(buffer) - n);
        if (!result) {
            qWarning("evdevtablet: Got EOF from input device");
            return;
        } else if (result < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                qErrnoWarning(errno, "evdevtablet: Could not read from input device");
                if (errno == ENODEV) { // device got disconnected -> stop reading
                    delete d->notifier;
                    d->notifier = 0;
                    QT_CLOSE(d->fd);
                    d->fd = -1;
                }
                return;
            }
        } else {
            n += result;
            if (n % sizeof(input_event) == 0)
                break;
        }
    }

    n /= sizeof(input_event);

    for (int i = 0; i < n; ++i)
        d->processInputEvent(&buffer[i]);
}


QEvdevTabletHandlerThread::QEvdevTabletHandlerThread(const QString &spec, QObject *parent)
    : QThread(parent), m_spec(spec), m_handler(0)
{
    start();
}

QEvdevTabletHandlerThread::~QEvdevTabletHandlerThread()
{
    quit();
    wait();
}

void QEvdevTabletHandlerThread::run()
{
    m_handler = new QEvdevTabletHandler(m_spec);
    exec();
    delete m_handler;
    m_handler = 0;
}


QT_END_NAMESPACE
