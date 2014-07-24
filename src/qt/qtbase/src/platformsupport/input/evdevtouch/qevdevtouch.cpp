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

#include "qevdevtouch_p.h"
#include <QStringList>
#include <QHash>
#include <QSocketNotifier>
#include <QGuiApplication>
#include <QDebug>
#include <QtCore/private/qcore_unix_p.h>
#include <QtPlatformSupport/private/qdevicediscovery_p.h>
#include <linux/input.h>

#if !defined(QT_NO_MTDEV)
extern "C" {
#include <mtdev.h>
}
#endif

QT_BEGIN_NAMESPACE

/* android (and perhaps some other linux-derived stuff) don't define everything
 * in linux/input.h, so we'll need to do that ourselves.
 */
#ifndef ABS_MT_TOUCH_MAJOR
#define ABS_MT_TOUCH_MAJOR      0x30    /* Major axis of touching ellipse */
#endif
#ifndef ABS_MT_POSITION_X
#define ABS_MT_POSITION_X 0x35    /* Center X ellipse position */
#endif
#ifndef ABS_MT_POSITION_Y
#define ABS_MT_POSITION_Y       0x36    /* Center Y ellipse position */
#endif
#ifndef ABS_MT_SLOT
#define ABS_MT_SLOT 0x2f
#endif
#ifndef ABS_CNT
#define ABS_CNT                 (ABS_MAX+1)
#endif
#ifndef ABS_MT_TRACKING_ID
#define ABS_MT_TRACKING_ID      0x39    /* Unique ID of initiated contact */
#endif
#ifndef SYN_MT_REPORT
#define SYN_MT_REPORT           2
#endif

class QEvdevTouchScreenData
{
public:
    QEvdevTouchScreenData(QEvdevTouchScreenHandler *q_ptr, const QStringList &args);

    void processInputEvent(input_event *data);
    void assignIds();

    QEvdevTouchScreenHandler *q;
    int m_lastEventType;
    QList<QWindowSystemInterface::TouchPoint> m_touchPoints;

    struct Contact {
        int trackingId;
        int x;
        int y;
        int maj;
        int pressure;
        Qt::TouchPointState state;
        QTouchEvent::TouchPoint::InfoFlags flags;
        Contact() : trackingId(-1),
            x(0), y(0), maj(-1), pressure(0),
            state(Qt::TouchPointPressed), flags(0) { }
    };
    QHash<int, Contact> m_contacts; // The key is a tracking id for type A, slot number for type B.
    QHash<int, Contact> m_lastContacts;
    Contact m_currentData;
    int m_currentSlot;

    int findClosestContact(const QHash<int, Contact> &contacts, int x, int y, int *dist);
    void reportPoints();
    void registerDevice();
    void addTouchPoint(const Contact &contact, Qt::TouchPointStates *combinedStates);

    int hw_range_x_min;
    int hw_range_x_max;
    int hw_range_y_min;
    int hw_range_y_max;
    int hw_pressure_min;
    int hw_pressure_max;
    QString hw_name;
    bool m_forceToActiveWindow;
    QTouchDevice *m_device;
    bool m_typeB;
    QTransform m_rotate;
    bool m_singleTouch;
};

QEvdevTouchScreenData::QEvdevTouchScreenData(QEvdevTouchScreenHandler *q_ptr, const QStringList &args)
    : q(q_ptr),
      m_lastEventType(-1),
      m_currentSlot(0),
      hw_range_x_min(0), hw_range_x_max(0),
      hw_range_y_min(0), hw_range_y_max(0),
      hw_pressure_min(0), hw_pressure_max(0),
      m_device(0), m_typeB(false), m_singleTouch(false)
{
    m_forceToActiveWindow = args.contains(QLatin1String("force_window"));
}

void QEvdevTouchScreenData::registerDevice()
{
    m_device = new QTouchDevice;
    m_device->setName(hw_name);
    m_device->setType(QTouchDevice::TouchScreen);
    m_device->setCapabilities(QTouchDevice::Position | QTouchDevice::Area);
    if (hw_pressure_max > hw_pressure_min)
        m_device->setCapabilities(m_device->capabilities() | QTouchDevice::Pressure);

    QWindowSystemInterface::registerTouchDevice(m_device);
}

#define LONG_BITS (sizeof(long) << 3)
#define NUM_LONGS(bits) (((bits) + LONG_BITS - 1) / LONG_BITS)

#if defined(QT_NO_MTDEV)
static inline bool testBit(long bit, const long *array)
{
    return (array[bit / LONG_BITS] >> bit % LONG_BITS) & 1;
}
#endif

QEvdevTouchScreenHandler::QEvdevTouchScreenHandler(const QString &specification, QObject *parent)
    : QObject(parent), m_notify(0), m_fd(-1), d(0)
#if !defined(QT_NO_MTDEV)
      , m_mtdev(0)
#endif
{
    setObjectName(QLatin1String("Evdev Touch Handler"));

    bool printDeviceInfo = qgetenv("QT_QPA_EVDEV_DEBUG").toInt();

    // only the first device argument is used for now
    QString spec = QString::fromLocal8Bit(qgetenv("QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS"));

    if (spec.isEmpty())
        spec = specification;

    QStringList args = spec.split(QLatin1Char(':'));

    QString dev;
    int rotationAngle = 0;
    for (int i = 0; i < args.count(); ++i) {
        if (args.at(i).startsWith(QLatin1String("/dev/")) && dev.isEmpty()) {
            dev = args.at(i);
        } else if (args.at(i).startsWith(QLatin1String("rotate"))) {
            QString rotateArg = args.at(i).section(QLatin1Char('='), 1, 1);
            bool ok;
            uint argValue = rotateArg.toUInt(&ok);
            if (ok) {
                switch (argValue) {
                case 90:
                case 180:
                case 270:
                    rotationAngle = argValue;
                default:
                    break;
                }
            }
        }
    }

    if (dev.isEmpty()) {
        // try to let udev scan for already connected devices
        QScopedPointer<QDeviceDiscovery> deviceDiscovery(QDeviceDiscovery::create(QDeviceDiscovery::Device_Touchpad | QDeviceDiscovery::Device_Touchscreen, this));
        if (deviceDiscovery) {
            QStringList devices = deviceDiscovery->scanConnectedDevices();

            // only the first device found is used for now
            if (devices.size() > 0)
                dev = devices[0];
        }
    }

    if (dev.isEmpty()) {
        if (printDeviceInfo)
            qDebug("evdevtouch: No touch devices found");
        return;
    }

    if (printDeviceInfo)
        qDebug("evdevtouch: Using device %s", qPrintable(dev));

    m_fd = QT_OPEN(dev.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0);

    if (m_fd >= 0) {
        m_notify = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        connect(m_notify, SIGNAL(activated(int)), this, SLOT(readData()));
    } else {
        qErrnoWarning(errno, "evdevtouch: Cannot open input device %s", qPrintable(dev));
        return;
    }

#if !defined(QT_NO_MTDEV)
    m_mtdev = static_cast<mtdev *>(calloc(1, sizeof(mtdev)));
    int mtdeverr = mtdev_open(m_mtdev, m_fd);
    if (mtdeverr) {
        qWarning("evdevtouch: mtdev_open failed: %d", mtdeverr);
        QT_CLOSE(m_fd);
        return;
    }
#endif

    d = new QEvdevTouchScreenData(this, args);

#if !defined(QT_NO_MTDEV)
    const char *mtdevStr = "(mtdev)";
    d->m_typeB = true;
#else
    const char *mtdevStr = "";
    long absbits[NUM_LONGS(ABS_CNT)];
    if (ioctl(m_fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits) >= 0) {
        d->m_typeB = testBit(ABS_MT_SLOT, absbits);
        d->m_singleTouch = !testBit(ABS_MT_POSITION_X, absbits);
    }
#endif

    if (printDeviceInfo)
        qDebug("evdevtouch: Protocol type %c %s (%s)", d->m_typeB ? 'B' : 'A',
               mtdevStr, d->m_singleTouch ? "single" : "multi");

    input_absinfo absInfo;
    memset(&absInfo, 0, sizeof(input_absinfo));
    bool has_x_range = false, has_y_range = false;

    if (ioctl(m_fd, EVIOCGABS((d->m_singleTouch ? ABS_X : ABS_MT_POSITION_X)), &absInfo) >= 0) {
        if (printDeviceInfo)
            qDebug("evdevtouch: min X: %d max X: %d", absInfo.minimum, absInfo.maximum);
        d->hw_range_x_min = absInfo.minimum;
        d->hw_range_x_max = absInfo.maximum;
        has_x_range = true;
    }

    if (ioctl(m_fd, EVIOCGABS((d->m_singleTouch ? ABS_Y : ABS_MT_POSITION_Y)), &absInfo) >= 0) {
        if (printDeviceInfo)
            qDebug("evdevtouch: min Y: %d max Y: %d", absInfo.minimum, absInfo.maximum);
        d->hw_range_y_min = absInfo.minimum;
        d->hw_range_y_max = absInfo.maximum;
        has_y_range = true;
    }

    if (!has_x_range || !has_y_range)
        qWarning("evdevtouch: Invalid ABS limits, behavior unspecified");

    if (ioctl(m_fd, EVIOCGABS(ABS_PRESSURE), &absInfo) >= 0) {
        if (printDeviceInfo)
            qDebug("evdevtouch: min pressure: %d max pressure: %d", absInfo.minimum, absInfo.maximum);
        if (absInfo.maximum > absInfo.minimum) {
            d->hw_pressure_min = absInfo.minimum;
            d->hw_pressure_max = absInfo.maximum;
        }
    }

    char name[1024];
    if (ioctl(m_fd, EVIOCGNAME(sizeof(name) - 1), name) >= 0) {
        d->hw_name = QString::fromLocal8Bit(name);
        if (printDeviceInfo)
            qDebug("evdevtouch: device name: %s", name);
    }

    // Fix up the coordinate ranges for am335x in case the kernel driver does not have them fixed.
    if (d->hw_name == QLatin1String("ti-tsc")) {
        if (d->hw_range_x_min == 0 && d->hw_range_x_max == 4095) {
            d->hw_range_x_min = 165;
            d->hw_range_x_max = 4016;
        }
        if (d->hw_range_y_min == 0 && d->hw_range_y_max == 4095) {
            d->hw_range_y_min = 220;
            d->hw_range_y_max = 3907;
        }
        if (printDeviceInfo)
            qDebug("evdevtouch: found ti-tsc, overriding: min X: %d max X: %d min Y: %d max Y: %d",
                   d->hw_range_x_min, d->hw_range_x_max, d->hw_range_y_min, d->hw_range_y_max);
    }

    bool grabSuccess = !ioctl(m_fd, EVIOCGRAB, (void *) 1);
    if (grabSuccess)
        ioctl(m_fd, EVIOCGRAB, (void *) 0);
    else
        qWarning("evdevtouch: The device is grabbed by another process. No events will be read.");

    if (rotationAngle)
        d->m_rotate = QTransform::fromTranslate(0.5, 0.5).rotate(rotationAngle).translate(-0.5, -0.5);

    d->registerDevice();
}

QEvdevTouchScreenHandler::~QEvdevTouchScreenHandler()
{
#if !defined(QT_NO_MTDEV)
    if (m_mtdev) {
        mtdev_close(m_mtdev);
        free(m_mtdev);
    }
#endif

    if (m_fd >= 0)
        QT_CLOSE(m_fd);

    delete d;
}

void QEvdevTouchScreenHandler::readData()
{
    ::input_event buffer[32];
    int events = 0;

#if !defined(QT_NO_MTDEV)
    forever {
        do {
            events = mtdev_get(m_mtdev, m_fd, buffer, sizeof(buffer) / sizeof(::input_event));
            // keep trying mtdev_get if we get interrupted. note that we do not
            // (and should not) handle EAGAIN; EAGAIN means that reading would
            // block and we'll get back here later to try again anyway.
        } while (events == -1 && errno == EINTR);

        // 0 events is EOF, -1 means error, handle both in the same place
        if (events <= 0)
            goto err;

        // process our shiny new events
        for (int i = 0; i < events; ++i)
            d->processInputEvent(&buffer[i]);

        // and try to get more
    }
#else
    int n = 0;
    for (; ;) {
        events = QT_READ(m_fd, reinterpret_cast<char*>(buffer) + n, sizeof(buffer) - n);
        if (events <= 0)
            goto err;
        n += events;
        if (n % sizeof(::input_event) == 0)
            break;
    }

    n /= sizeof(::input_event);

    for (int i = 0; i < n; ++i)
        d->processInputEvent(&buffer[i]);
#endif
    return;

err:
    if (!events) {
        qWarning("evdevtouch: Got EOF from input device");
        return;
    } else if (events < 0) {
        if (errno != EINTR && errno != EAGAIN) {
            qErrnoWarning(errno, "evdevtouch: Could not read from input device");
            if (errno == ENODEV) { // device got disconnected -> stop reading
                delete m_notify;
                m_notify = 0;
                QT_CLOSE(m_fd);
                m_fd = -1;
            }
            return;
        }
    }
}

void QEvdevTouchScreenData::addTouchPoint(const Contact &contact, Qt::TouchPointStates *combinedStates)
{
    QWindowSystemInterface::TouchPoint tp;
    tp.id = contact.trackingId;
    tp.flags = contact.flags;
    tp.state = contact.state;
    *combinedStates |= tp.state;

    // Store the HW coordinates for now, will be updated later.
    tp.area = QRectF(0, 0, contact.maj, contact.maj);
    tp.area.moveCenter(QPoint(contact.x, contact.y));
    tp.pressure = contact.pressure;

    // Get a normalized position in range 0..1.
    tp.normalPosition = QPointF((contact.x - hw_range_x_min) / qreal(hw_range_x_max - hw_range_x_min),
                                (contact.y - hw_range_y_min) / qreal(hw_range_y_max - hw_range_y_min));

    if (!m_rotate.isIdentity())
        tp.normalPosition = m_rotate.map(tp.normalPosition);

    tp.rawPositions.append(QPointF(contact.x, contact.y));

    m_touchPoints.append(tp);
}

void QEvdevTouchScreenData::processInputEvent(input_event *data)
{
    if (data->type == EV_ABS) {

        if (data->code == ABS_MT_POSITION_X || (m_singleTouch && data->code == ABS_X)) {
            m_currentData.x = qBound(hw_range_x_min, data->value, hw_range_x_max);
            if (m_singleTouch)
                m_contacts[m_currentSlot].x = m_currentData.x;
            if (m_typeB) {
                m_contacts[m_currentSlot].x = m_currentData.x;
                if (m_contacts[m_currentSlot].state == Qt::TouchPointStationary)
                    m_contacts[m_currentSlot].state = Qt::TouchPointMoved;
            }
        } else if (data->code == ABS_MT_POSITION_Y || (m_singleTouch && data->code == ABS_Y)) {
            m_currentData.y = qBound(hw_range_y_min, data->value, hw_range_y_max);
            if (m_singleTouch)
                m_contacts[m_currentSlot].y = m_currentData.y;
                if (m_typeB) {
                    m_contacts[m_currentSlot].y = m_currentData.y;
                    if (m_contacts[m_currentSlot].state == Qt::TouchPointStationary)
                        m_contacts[m_currentSlot].state = Qt::TouchPointMoved;
                }
        } else if (data->code == ABS_MT_TRACKING_ID) {
            m_currentData.trackingId = data->value;
            if (m_typeB) {
                if (m_currentData.trackingId == -1) {
                    m_contacts[m_currentSlot].state = Qt::TouchPointReleased;
                } else {
                    m_contacts[m_currentSlot].state = Qt::TouchPointPressed;
                    m_contacts[m_currentSlot].trackingId = m_currentData.trackingId;
                }
            }
        } else if (data->code == ABS_MT_TOUCH_MAJOR) {
            m_currentData.maj = data->value;
            if (data->value == 0)
                m_currentData.state = Qt::TouchPointReleased;
            if (m_typeB)
                m_contacts[m_currentSlot].maj = m_currentData.maj;
        } else if (data->code == ABS_PRESSURE) {
            m_currentData.pressure = qBound(hw_pressure_min, data->value, hw_pressure_max);
            if (m_typeB || m_singleTouch)
                m_contacts[m_currentSlot].pressure = m_currentData.pressure;
        } else if (data->code == ABS_MT_SLOT) {
            m_currentSlot = data->value;
        }

    } else if (data->type == EV_KEY && !m_typeB) {
        if (data->code == BTN_TOUCH && data->value == 0)
            m_contacts[m_currentSlot].state = Qt::TouchPointReleased;
    } else if (data->type == EV_SYN && data->code == SYN_MT_REPORT && m_lastEventType != EV_SYN) {

        // If there is no tracking id, one will be generated later.
        // Until that use a temporary key.
        int key = m_currentData.trackingId;
        if (key == -1)
            key = m_contacts.count();

        m_contacts.insert(key, m_currentData);
        m_currentData = Contact();

    } else if (data->type == EV_SYN && data->code == SYN_REPORT) {

        // Ensure valid IDs even when the driver does not report ABS_MT_TRACKING_ID.
        if (!m_contacts.isEmpty() && m_contacts.constBegin().value().trackingId == -1)
            assignIds();

        m_touchPoints.clear();
        Qt::TouchPointStates combinedStates;

        QMutableHashIterator<int, Contact> it(m_contacts);
        while (it.hasNext()) {
            it.next();
            Contact &contact(it.value());

            if (!contact.state)
                continue;

            int key = m_typeB ? it.key() : contact.trackingId;
            if (!m_typeB && m_lastContacts.contains(key)) {
                const Contact &prev(m_lastContacts.value(key));
                if (contact.state == Qt::TouchPointReleased) {
                    // Copy over the previous values for released points, just in case.
                    contact.x = prev.x;
                    contact.y = prev.y;
                    contact.maj = prev.maj;
                } else {
                    contact.state = (prev.x == contact.x && prev.y == contact.y)
                            ? Qt::TouchPointStationary : Qt::TouchPointMoved;
                }
            }

            // Avoid reporting a contact in released state more than once.
            if (!m_typeB && contact.state == Qt::TouchPointReleased
                    && !m_lastContacts.contains(key)) {
                it.remove();
                continue;
            }

            addTouchPoint(contact, &combinedStates);
        }

        // Now look for contacts that have disappeared since the last sync.
        it = m_lastContacts;
        while (it.hasNext()) {
            it.next();
            Contact &contact(it.value());
            int key = m_typeB ? it.key() : contact.trackingId;
            if (!m_contacts.contains(key)) {
                contact.state = Qt::TouchPointReleased;
                addTouchPoint(contact, &combinedStates);
            }
        }

        // Remove contacts that have just been reported as released.
        it = m_contacts;
        while (it.hasNext()) {
            it.next();
            Contact &contact(it.value());
            if (contact.state == Qt::TouchPointReleased) {
                if (m_typeB)
                    contact.state = static_cast<Qt::TouchPointState>(0);
                else
                    it.remove();
            } else {
                contact.state = Qt::TouchPointStationary;
            }
        }

        m_lastContacts = m_contacts;
        if (!m_typeB && !m_singleTouch)
            m_contacts.clear();

        if (!m_touchPoints.isEmpty() && combinedStates != Qt::TouchPointStationary)
            reportPoints();
    }

    m_lastEventType = data->type;
}

int QEvdevTouchScreenData::findClosestContact(const QHash<int, Contact> &contacts, int x, int y, int *dist)
{
    int minDist = -1, id = -1;
    for (QHash<int, Contact>::const_iterator it = contacts.constBegin(), ite = contacts.constEnd();
         it != ite; ++it) {
        const Contact &contact(it.value());
        int dx = x - contact.x;
        int dy = y - contact.y;
        int dist = dx * dx + dy * dy;
        if (minDist == -1 || dist < minDist) {
            minDist = dist;
            id = contact.trackingId;
        }
    }
    if (dist)
        *dist = minDist;
    return id;
}

void QEvdevTouchScreenData::assignIds()
{
    QHash<int, Contact> candidates = m_lastContacts, pending = m_contacts, newContacts;
    int maxId = -1;
    QHash<int, Contact>::iterator it, ite, bestMatch;
    while (!pending.isEmpty() && !candidates.isEmpty()) {
        int bestDist = -1, bestId = 0;
        for (it = pending.begin(), ite = pending.end(); it != ite; ++it) {
            int dist;
            int id = findClosestContact(candidates, it->x, it->y, &dist);
            if (id >= 0 && (bestDist == -1 || dist < bestDist)) {
                bestDist = dist;
                bestId = id;
                bestMatch = it;
            }
        }
        if (bestDist >= 0) {
            bestMatch->trackingId = bestId;
            newContacts.insert(bestId, *bestMatch);
            candidates.remove(bestId);
            pending.erase(bestMatch);
            if (bestId > maxId)
                maxId = bestId;
        }
    }
    if (candidates.isEmpty()) {
        for (it = pending.begin(), ite = pending.end(); it != ite; ++it) {
            it->trackingId = ++maxId;
            newContacts.insert(it->trackingId, *it);
        }
    }
    m_contacts = newContacts;
}

void QEvdevTouchScreenData::reportPoints()
{
    QRect winRect;
    if (m_forceToActiveWindow) {
        QWindow *win = QGuiApplication::focusWindow();
        if (!win)
            return;
        winRect = win->geometry();
    } else {
        winRect = QGuiApplication::primaryScreen()->geometry();
    }

    const int hw_w = hw_range_x_max - hw_range_x_min;
    const int hw_h = hw_range_y_max - hw_range_y_min;

    // Map the coordinates based on the normalized position. QPA expects 'area'
    // to be in screen coordinates.
    const int pointCount = m_touchPoints.count();
    for (int i = 0; i < pointCount; ++i) {
        QWindowSystemInterface::TouchPoint &tp(m_touchPoints[i]);

        // Generate a screen position that is always inside the active window
        // or the primary screen.  Even though we report this as a QRectF, internally
        // Qt uses QRect/QPoint so we need to bound the size to winRect.size() - QSize(1, 1)
        const qreal wx = winRect.left() + tp.normalPosition.x() * (winRect.width() - 1);
        const qreal wy = winRect.top() + tp.normalPosition.y() * (winRect.height() - 1);
        const qreal sizeRatio = (winRect.width() + winRect.height()) / qreal(hw_w + hw_h);
        if (tp.area.width() == -1) // touch major was not provided
            tp.area = QRectF(0, 0, 8, 8);
        else
            tp.area = QRectF(0, 0, tp.area.width() * sizeRatio, tp.area.height() * sizeRatio);
        tp.area.moveCenter(QPointF(wx, wy));

        // Calculate normalized pressure.
        if (!hw_pressure_min && !hw_pressure_max)
            tp.pressure = tp.state == Qt::TouchPointReleased ? 0 : 1;
        else
            tp.pressure = (tp.pressure - hw_pressure_min) / qreal(hw_pressure_max - hw_pressure_min);
    }

    QWindowSystemInterface::handleTouchEvent(0, m_device, m_touchPoints);
}


QEvdevTouchScreenHandlerThread::QEvdevTouchScreenHandlerThread(const QString &spec, QObject *parent)
    : QThread(parent), m_spec(spec), m_handler(0)
{
    start();
}

QEvdevTouchScreenHandlerThread::~QEvdevTouchScreenHandlerThread()
{
    quit();
    wait();
}

void QEvdevTouchScreenHandlerThread::run()
{
    m_handler = new QEvdevTouchScreenHandler(m_spec);
    exec();
    delete m_handler;
    m_handler = 0;
}


QT_END_NAMESPACE
