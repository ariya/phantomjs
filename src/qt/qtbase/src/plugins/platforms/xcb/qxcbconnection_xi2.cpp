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

#include "qxcbconnection.h"
#include "qxcbkeyboard.h"
#include "qxcbscreen.h"
#include "qxcbwindow.h"
#include "qtouchdevice.h"
#include <qpa/qwindowsysteminterface.h>
#include <QDebug>

#ifdef XCB_USE_XINPUT2

#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2proto.h>
#define FINGER_MAX_WIDTH_MM 10

struct XInput2DeviceData {
    XInput2DeviceData()
    : xiDeviceInfo(0)
    , qtTouchDevice(0)
    {
    }
    XIDeviceInfo *xiDeviceInfo;
    QTouchDevice *qtTouchDevice;

    // Stuff that is relevant only for touchpads
    QHash<int, QPointF> pointPressedPosition; // in screen coordinates where each point was pressed
    QPointF firstPressedPosition;        // in screen coordinates where the first point was pressed
    QPointF firstPressedNormalPosition;  // device coordinates (0 to 1, 0 to 1) where the first point was pressed
    QSizeF size;                         // device size in mm
};

void QXcbConnection::initializeXInput2()
{
    debug_xinput = qEnvironmentVariableIsSet("QT_XCB_DEBUG_XINPUT");
    debug_xinput_devices = qEnvironmentVariableIsSet("QT_XCB_DEBUG_XINPUT_DEVICES");
#ifndef QT_NO_TABLETEVENT
    m_tabletData.clear();
#endif
    m_scrollingDevices.clear();
    Display *xDisplay = static_cast<Display *>(m_xlib_display);
    if (XQueryExtension(xDisplay, "XInputExtension", &m_xiOpCode, &m_xiEventBase, &m_xiErrorBase)) {
        int xiMajor = 2;
        m_xi2Minor = 2; // try 2.2 first, needed for TouchBegin/Update/End
        if (XIQueryVersion(xDisplay, &xiMajor, &m_xi2Minor) == BadRequest) {
            m_xi2Minor = 1; // for smooth scrolling 2.1 is enough
            if (XIQueryVersion(xDisplay, &xiMajor, &m_xi2Minor) == BadRequest) {
                m_xi2Minor = 0; // for tablet support 2.0 is enough
                m_xi2Enabled = XIQueryVersion(xDisplay, &xiMajor, &m_xi2Minor) != BadRequest;
            } else
                m_xi2Enabled = true;
        } else
            m_xi2Enabled = true;
        if (m_xi2Enabled) {
            if (Q_UNLIKELY(debug_xinput_devices))
#ifdef XCB_USE_XINPUT22
                qDebug("XInput version %d.%d is available and Qt supports 2.2 or greater", xiMajor, m_xi2Minor);
#else
                qDebug("XInput version %d.%d is available and Qt supports 2.0", xiMajor, m_xi2Minor);
#endif
            int deviceCount = 0;
            XIDeviceInfo *devices = XIQueryDevice(xDisplay, XIAllDevices, &deviceCount);
            for (int i = 0; i < deviceCount; ++i) {
                // Only non-master pointing devices are relevant here.
                if (devices[i].use != XISlavePointer)
                    continue;
                if (Q_UNLIKELY(debug_xinput_devices))
                    qDebug() << "input device "<< devices[i].name;
#ifndef QT_NO_TABLETEVENT
                TabletData tabletData;
#endif
                ScrollingDevice scrollingDevice;
                for (int c = 0; c < devices[i].num_classes; ++c) {
                    switch (devices[i].classes[c]->type) {
                    case XIValuatorClass: {
                        XIValuatorClassInfo *vci = reinterpret_cast<XIValuatorClassInfo *>(devices[i].classes[c]);
                        const int valuatorAtom = qatom(vci->label);
                        if (Q_UNLIKELY(debug_xinput_devices))
                            qDebug() << "   has valuator" << atomName(vci->label) << "recognized?" << (valuatorAtom < QXcbAtom::NAtoms);
#ifndef QT_NO_TABLETEVENT
                        if (valuatorAtom < QXcbAtom::NAtoms) {
                            TabletData::ValuatorClassInfo info;
                            info.minVal = vci->min;
                            info.maxVal = vci->max;
                            info.number = vci->number;
                            tabletData.valuatorInfo[valuatorAtom] = info;
                        }
#endif // QT_NO_TABLETEVENT
                        if (valuatorAtom == QXcbAtom::RelHorizScroll || valuatorAtom == QXcbAtom::RelHorizWheel)
                            scrollingDevice.lastScrollPosition.setX(vci->value);
                        else if (valuatorAtom == QXcbAtom::RelVertScroll || valuatorAtom == QXcbAtom::RelVertWheel)
                            scrollingDevice.lastScrollPosition.setY(vci->value);
                        break;
                    }
#ifdef XCB_USE_XINPUT21
                    case XIScrollClass: {
                        XIScrollClassInfo *sci = reinterpret_cast<XIScrollClassInfo *>(devices[i].classes[c]);
                        if (sci->scroll_type == XIScrollTypeVertical) {
                            scrollingDevice.orientations |= Qt::Vertical;
                            scrollingDevice.verticalIndex = sci->number;
                            scrollingDevice.verticalIncrement = sci->increment;
                        }
                        else if (sci->scroll_type == XIScrollTypeHorizontal) {
                            scrollingDevice.orientations |= Qt::Horizontal;
                            scrollingDevice.horizontalIndex = sci->number;
                            scrollingDevice.horizontalIncrement = sci->increment;
                        }
                        break;
                    }
                    case XIButtonClass: {
                        XIButtonClassInfo *bci = reinterpret_cast<XIButtonClassInfo *>(devices[i].classes[c]);
                        if (bci->num_buttons >= 5) {
                            Atom label4 = bci->labels[3];
                            Atom label5 = bci->labels[4];
                            if ((!label4 || qatom(label4) == QXcbAtom::ButtonWheelUp) && (!label5 || qatom(label5) == QXcbAtom::ButtonWheelDown))
                                scrollingDevice.legacyOrientations |= Qt::Vertical;
                        }
                        if (bci->num_buttons >= 7) {
                            Atom label6 = bci->labels[5];
                            Atom label7 = bci->labels[6];
                            if ((!label6 || qatom(label6) == QXcbAtom::ButtonHorizWheelLeft) && (!label7 || qatom(label7) == QXcbAtom::ButtonHorizWheelRight))
                                scrollingDevice.legacyOrientations |= Qt::Horizontal;
                        }
                        break;
                    }
#endif
                    default:
                        break;
                    }
                }
                bool isTablet = false;
#ifndef QT_NO_TABLETEVENT
                // If we have found the valuators which we expect a tablet to have, assume it's a tablet.
                if (tabletData.valuatorInfo.contains(QXcbAtom::AbsX) &&
                        tabletData.valuatorInfo.contains(QXcbAtom::AbsY) &&
                        tabletData.valuatorInfo.contains(QXcbAtom::AbsPressure)) {
                    tabletData.deviceId = devices[i].deviceid;
                    tabletData.pointerType = QTabletEvent::Pen;
                    if (QByteArray(devices[i].name).toLower().contains("eraser"))
                        tabletData.pointerType = QTabletEvent::Eraser;
                    m_tabletData.append(tabletData);
                    isTablet = true;
                    if (Q_UNLIKELY(debug_xinput_devices))
                        qDebug() << "   it's a tablet with pointer type" << tabletData.pointerType;
                }
#endif // QT_NO_TABLETEVENT

#ifdef XCB_USE_XINPUT21
                if (scrollingDevice.orientations || scrollingDevice.legacyOrientations) {
                    scrollingDevice.deviceId = devices[i].deviceid;
                    // Only use legacy wheel button events when we don't have real scroll valuators.
                    scrollingDevice.legacyOrientations &= ~scrollingDevice.orientations;
                    m_scrollingDevices.insert(scrollingDevice.deviceId, scrollingDevice);
                    if (Q_UNLIKELY(debug_xinput_devices))
                        qDebug() << "   it's a scrolling device";
                }
#endif

                if (!isTablet) {
                    XInput2DeviceData *dev = deviceForId(devices[i].deviceid);
                    if (Q_UNLIKELY(debug_xinput_devices)) {
                        if (dev && dev->qtTouchDevice->type() == QTouchDevice::TouchScreen)
                            qDebug("   it's a touchscreen with type %d capabilities 0x%X max touch points %d",
                                   dev->qtTouchDevice->type(), (unsigned int)dev->qtTouchDevice->capabilities(),
                                   dev->qtTouchDevice->maximumTouchPoints());
                        else if (dev && dev->qtTouchDevice->type() == QTouchDevice::TouchPad)
                            qDebug("   it's a touchpad with type %d capabilities 0x%X max touch points %d size %f x %f",
                                   dev->qtTouchDevice->type(), (unsigned int)dev->qtTouchDevice->capabilities(),
                                   dev->qtTouchDevice->maximumTouchPoints(),
                                   dev->size.width(), dev->size.height());
                    }
                }
            }
            XIFreeDeviceInfo(devices);
        }
    }
}

void QXcbConnection::finalizeXInput2()
{
}

void QXcbConnection::xi2Select(xcb_window_t window)
{
    if (!m_xi2Enabled)
        return;

    Display *xDisplay = static_cast<Display *>(m_xlib_display);
    unsigned int bitMask = 0;
    unsigned char *xiBitMask = reinterpret_cast<unsigned char *>(&bitMask);

#ifdef XCB_USE_XINPUT22
    bitMask |= XI_TouchBeginMask;
    bitMask |= XI_TouchUpdateMask;
    bitMask |= XI_TouchEndMask;
    XIEventMask mask;
    mask.mask_len = sizeof(bitMask);
    mask.mask = xiBitMask;
    // Enable each touchscreen
    foreach (XInput2DeviceData *dev, m_touchDevices) {
        mask.deviceid = dev->xiDeviceInfo->deviceid;
        Status result = XISelectEvents(xDisplay, window, &mask, 1);
        // If we have XInput >= 2.2 and successfully enable a touchscreen, then
        // it will provide touch only. In most other cases, there will be
        // emulated mouse events from the driver. If not, then Qt must do its
        // own mouse emulation to enable interaction with mouse-oriented QWidgets.
        if (m_xi2Minor >= 2 && result == Success)
            has_touch_without_mouse_emulation = true;
    }
#endif // XCB_USE_XINPUT22

    QSet<int> tabletDevices;
#ifndef QT_NO_TABLETEVENT
    // For each tablet, select some additional event types.
    // Press, motion, etc. events must never be selected for _all_ devices
    // as that would render the standard XCB_MOTION_NOTIFY and
    // similar handlers useless and we have no intention to infect
    // all the pure xcb code with Xlib-based XI2.
    if (!m_tabletData.isEmpty()) {
        unsigned int tabletBitMask = bitMask;
        unsigned char *xiTabletBitMask = reinterpret_cast<unsigned char *>(&tabletBitMask);
        QVector<XIEventMask> xiEventMask(m_tabletData.count());
        tabletBitMask |= XI_ButtonPressMask;
        tabletBitMask |= XI_ButtonReleaseMask;
        tabletBitMask |= XI_MotionMask;
        tabletBitMask |= XI_PropertyEventMask;
        for (int i = 0; i < m_tabletData.count(); ++i) {
            int deviceId = m_tabletData.at(i).deviceId;
            tabletDevices.insert(deviceId);
            xiEventMask[i].deviceid = deviceId;
            xiEventMask[i].mask_len = sizeof(tabletBitMask);
            xiEventMask[i].mask = xiTabletBitMask;
        }
        XISelectEvents(xDisplay, window, xiEventMask.data(), m_tabletData.count());
    }
#endif // QT_NO_TABLETEVENT

#ifdef XCB_USE_XINPUT21
    // Enable each scroll device
    if (!m_scrollingDevices.isEmpty()) {
        QVector<XIEventMask> xiEventMask(m_scrollingDevices.size());
        unsigned int scrollBitMask = 0;
        unsigned char *xiScrollBitMask = reinterpret_cast<unsigned char *>(&scrollBitMask);
        scrollBitMask = XI_MotionMask;
        scrollBitMask |= XI_ButtonReleaseMask;
        bitMask |= XI_MotionMask;
        bitMask |= XI_ButtonReleaseMask;
        int i=0;
        Q_FOREACH (const ScrollingDevice& scrollingDevice, m_scrollingDevices) {
            if (tabletDevices.contains(scrollingDevice.deviceId))
                continue; // All necessary events are already captured.
            xiEventMask[i].deviceid = scrollingDevice.deviceId;
            if (m_touchDevices.contains(scrollingDevice.deviceId)) {
                xiEventMask[i].mask_len = sizeof(bitMask);
                xiEventMask[i].mask = xiBitMask;
            } else {
                xiEventMask[i].mask_len = sizeof(scrollBitMask);
                xiEventMask[i].mask = xiScrollBitMask;
            }
            i++;
        }
        XISelectEvents(xDisplay, window, xiEventMask.data(), i);
    }
#else
    Q_UNUSED(xiBitMask);
#endif
}

XInput2DeviceData *QXcbConnection::deviceForId(int id)
{
    XInput2DeviceData *dev = m_touchDevices[id];
    if (!dev) {
        int unused = 0;
        QTouchDevice::Capabilities caps = 0;
        dev = new XInput2DeviceData;
        dev->xiDeviceInfo = XIQueryDevice(static_cast<Display *>(m_xlib_display), id, &unused);
        int type = -1;
        int maxTouchPoints = 1;
        bool hasRelativeCoords = false;
        for (int i = 0; i < dev->xiDeviceInfo->num_classes; ++i) {
            XIAnyClassInfo *classinfo = dev->xiDeviceInfo->classes[i];
            switch (classinfo->type) {
#ifdef XCB_USE_XINPUT22
            case XITouchClass: {
                XITouchClassInfo *tci = reinterpret_cast<XITouchClassInfo *>(classinfo);
                maxTouchPoints = tci->num_touches;
                if (Q_UNLIKELY(debug_xinput_devices))
                    qDebug("   has touch class with mode %d", tci->mode);
                switch (tci->mode) {
                case XIModeRelative:
                    type = QTouchDevice::TouchPad;
                    break;
                case XIModeAbsolute:
                    type = QTouchDevice::TouchScreen;
                    break;
                }
                break;
            }
#endif // XCB_USE_XINPUT22
            case XIValuatorClass: {
                XIValuatorClassInfo *vci = reinterpret_cast<XIValuatorClassInfo *>(classinfo);
                if (vci->label == atom(QXcbAtom::AbsMTPositionX))
                    caps |= QTouchDevice::Position | QTouchDevice::NormalizedPosition;
                else if (vci->label == atom(QXcbAtom::AbsMTTouchMajor))
                    caps |= QTouchDevice::Area;
                else if (vci->label == atom(QXcbAtom::AbsMTPressure) || vci->label == atom(QXcbAtom::AbsPressure))
                    caps |= QTouchDevice::Pressure;
                else if (vci->label == atom(QXcbAtom::RelX)) {
                    hasRelativeCoords = true;
                    dev->size.setWidth((vci->max - vci->min) * 1000.0 / vci->resolution);
                } else if (vci->label == atom(QXcbAtom::RelY)) {
                    hasRelativeCoords = true;
                    dev->size.setHeight((vci->max - vci->min) * 1000.0 / vci->resolution);
                }
                break;
            }
            }
        }
        if (type < 0 && caps && hasRelativeCoords) {
            type = QTouchDevice::TouchPad;
            if (dev->size.width() < 10 || dev->size.height() < 10 ||
                    dev->size.width() > 10000 || dev->size.height() > 10000)
                dev->size = QSizeF(130, 110);
        }
        if (type >= QTouchDevice::TouchScreen && type <= QTouchDevice::TouchPad) {
            dev->qtTouchDevice = new QTouchDevice;
            dev->qtTouchDevice->setName(dev->xiDeviceInfo->name);
            dev->qtTouchDevice->setType((QTouchDevice::DeviceType)type);
            dev->qtTouchDevice->setCapabilities(caps);
            dev->qtTouchDevice->setMaximumTouchPoints(maxTouchPoints);
            if (caps != 0)
                QWindowSystemInterface::registerTouchDevice(dev->qtTouchDevice);
            m_touchDevices[id] = dev;
        } else {
            m_touchDevices.remove(id);
            delete dev;
            dev = 0;
        }
    }
    return dev;
}

#if defined(XCB_USE_XINPUT21) || !defined(QT_NO_TABLETEVENT)
static qreal fixed1616ToReal(FP1616 val)
{
    return (qreal(val >> 16)) + (val & 0xFF) / (qreal)0xFF;
}
#endif // defined(XCB_USE_XINPUT21) || !defined(QT_NO_TABLETEVENT)

#if defined(XCB_USE_XINPUT21)
static qreal valuatorNormalized(double value, XIValuatorClassInfo *vci)
{
    if (value > vci->max)
        value = vci->max;
    if (value < vci->min)
        value = vci->min;
    return (value - vci->min) / (vci->max - vci->min);
}
#endif // XCB_USE_XINPUT21

void QXcbConnection::xi2HandleEvent(xcb_ge_event_t *event)
{
    if (xi2PrepareXIGenericDeviceEvent(event, m_xiOpCode)) {
        xXIGenericDeviceEvent *xiEvent = reinterpret_cast<xXIGenericDeviceEvent *>(event);

#ifndef QT_NO_TABLETEVENT
        for (int i = 0; i < m_tabletData.count(); ++i) {
            if (m_tabletData.at(i).deviceId == xiEvent->deviceid) {
                if (xi2HandleTabletEvent(xiEvent, &m_tabletData[i]))
                    return;
            }
        }
#endif // QT_NO_TABLETEVENT

#ifdef XCB_USE_XINPUT21
        QHash<int, ScrollingDevice>::iterator device = m_scrollingDevices.find(xiEvent->deviceid);
        if (device != m_scrollingDevices.end())
            xi2HandleScrollEvent(xiEvent, device.value());
#endif // XCB_USE_XINPUT21

#ifdef XCB_USE_XINPUT22
        if (xiEvent->evtype == XI_TouchBegin || xiEvent->evtype == XI_TouchUpdate || xiEvent->evtype == XI_TouchEnd) {
            xXIDeviceEvent* xiDeviceEvent = reinterpret_cast<xXIDeviceEvent *>(event);
            if (Q_UNLIKELY(debug_xinput))
                qDebug("XI2 touch event type %d seq %d detail %d pos %6.1f, %6.1f root pos %6.1f, %6.1f",
                    event->event_type, xiEvent->sequenceNumber, xiDeviceEvent->detail,
                    fixed1616ToReal(xiDeviceEvent->event_x), fixed1616ToReal(xiDeviceEvent->event_y),
                    fixed1616ToReal(xiDeviceEvent->root_x), fixed1616ToReal(xiDeviceEvent->root_y) );

            if (QXcbWindow *platformWindow = platformWindowFromId(xiDeviceEvent->event)) {
                XInput2DeviceData *dev = deviceForId(xiEvent->deviceid);
                Q_ASSERT(dev);
                const bool firstTouch = m_touchPoints.isEmpty();
                if (xiEvent->evtype == XI_TouchBegin) {
                    QWindowSystemInterface::TouchPoint tp;
                    tp.id = xiDeviceEvent->detail % INT_MAX;
                    tp.state = Qt::TouchPointPressed;
                    tp.pressure = -1.0;
                    m_touchPoints[tp.id] = tp;
                }
                QWindowSystemInterface::TouchPoint &touchPoint = m_touchPoints[xiDeviceEvent->detail];
                qreal x = fixed1616ToReal(xiDeviceEvent->root_x);
                qreal y = fixed1616ToReal(xiDeviceEvent->root_y);
                qreal nx = -1.0, ny = -1.0, w = 0.0, h = 0.0;
                QXcbScreen* screen = m_screens.at(0);
                for (int i = 0; i < dev->xiDeviceInfo->num_classes; ++i) {
                    XIAnyClassInfo *classinfo = dev->xiDeviceInfo->classes[i];
                    if (classinfo->type == XIValuatorClass) {
                        XIValuatorClassInfo *vci = reinterpret_cast<XIValuatorClassInfo *>(classinfo);
                        int n = vci->number;
                        double value;
                        if (!xi2GetValuatorValueIfSet(xiDeviceEvent, n, &value))
                            continue;
                        if (Q_UNLIKELY(debug_xinput))
                            qDebug("   valuator %20s value %lf from range %lf -> %lf",
                                atomName(vci->label).constData(), value, vci->min, vci->max );
                        if (vci->label == atom(QXcbAtom::RelX)) {
                            nx = valuatorNormalized(value, vci);
                        } else if (vci->label == atom(QXcbAtom::RelY)) {
                            ny = valuatorNormalized(value, vci);
                        } else if (vci->label == atom(QXcbAtom::AbsMTPositionX)) {
                            nx = valuatorNormalized(value, vci);
                        } else if (vci->label == atom(QXcbAtom::AbsMTPositionY)) {
                            ny = valuatorNormalized(value, vci);
                        } else if (vci->label == atom(QXcbAtom::AbsMTTouchMajor)) {
                            // Convert the value within its range as a fraction of a finger's max (contact patch)
                            //  width in mm, and from there to pixels depending on screen resolution
                            w = valuatorNormalized(value, vci) * FINGER_MAX_WIDTH_MM *
                                screen->geometry().width() / screen->physicalSize().width();
                        } else if (vci->label == atom(QXcbAtom::AbsMTTouchMinor)) {
                            h = valuatorNormalized(value, vci) * FINGER_MAX_WIDTH_MM *
                                screen->geometry().height() / screen->physicalSize().height();
                        } else if (vci->label == atom(QXcbAtom::AbsMTPressure) ||
                                   vci->label == atom(QXcbAtom::AbsPressure)) {
                            touchPoint.pressure = valuatorNormalized(value, vci);
                        }
                    }
                }
                // If any value was not updated, use the last-known value.
                if (nx == -1.0) {
                    x = touchPoint.area.center().x();
                    nx = x / screen->geometry().width();
                }
                if (ny == -1.0) {
                    y = touchPoint.area.center().y();
                    ny = y / screen->geometry().height();
                }
                if (xiEvent->evtype != XI_TouchEnd) {
                    if (w == 0.0)
                        w = touchPoint.area.width();
                    if (h == 0.0)
                        h = touchPoint.area.height();
                }

                switch (xiEvent->evtype) {
                case XI_TouchBegin:
                    if (firstTouch) {
                        dev->firstPressedPosition = QPointF(x, y);
                        dev->firstPressedNormalPosition = QPointF(nx, ny);
                    }
                    dev->pointPressedPosition.insert(touchPoint.id, QPointF(x, y));
                    break;
                case XI_TouchUpdate:
                    if (dev->qtTouchDevice->type() == QTouchDevice::TouchPad && dev->pointPressedPosition.value(touchPoint.id) == QPointF(x, y)) {
                        qreal dx = (nx - dev->firstPressedNormalPosition.x()) *
                            dev->size.width() * screen->geometry().width() / screen->physicalSize().width();
                        qreal dy = (ny - dev->firstPressedNormalPosition.y()) *
                            dev->size.height() * screen->geometry().height() / screen->physicalSize().height();
                        x = dev->firstPressedPosition.x() + dx;
                        y = dev->firstPressedPosition.y() + dy;
                        touchPoint.state = Qt::TouchPointMoved;
                    } else if (touchPoint.area.center() != QPoint(x, y)) {
                        touchPoint.state = Qt::TouchPointMoved;
                        dev->pointPressedPosition[touchPoint.id] = QPointF(x, y);
                    }
                    break;
                case XI_TouchEnd:
                    touchPoint.state = Qt::TouchPointReleased;
                    if (dev->qtTouchDevice->type() == QTouchDevice::TouchPad && dev->pointPressedPosition.value(touchPoint.id) == QPointF(x, y)) {
                        qreal dx = (nx - dev->firstPressedNormalPosition.x()) *
                            dev->size.width() * screen->geometry().width() / screen->physicalSize().width();
                        qreal dy = (ny - dev->firstPressedNormalPosition.y()) *
                            dev->size.width() * screen->geometry().width() / screen->physicalSize().width();
                        x = dev->firstPressedPosition.x() + dx;
                        y = dev->firstPressedPosition.y() + dy;
                    }
                    dev->pointPressedPosition.remove(touchPoint.id);
                }
                touchPoint.area = QRectF(x - w/2, y - h/2, w, h);
                touchPoint.normalPosition = QPointF(nx, ny);

                if (Q_UNLIKELY(debug_xinput))
                    qDebug() << "   touchpoint "  << touchPoint.id << " state " << touchPoint.state << " pos norm " << touchPoint.normalPosition <<
                        " area " << touchPoint.area << " pressure " << touchPoint.pressure;
                QWindowSystemInterface::handleTouchEvent(platformWindow->window(), xiEvent->time, dev->qtTouchDevice, m_touchPoints.values());
                if (has_touch_without_mouse_emulation) {
                    // We need to grab the touch event to prevent mouse emulation.
                    if (xiEvent->evtype == XI_TouchBegin) {
                        XIEventMask xieventmask;
                        unsigned int bitMask = 0;
                        unsigned char *xiBitMask = reinterpret_cast<unsigned char *>(&bitMask);
                        xieventmask.deviceid = xiEvent->deviceid;
                        xieventmask.mask = xiBitMask;
                        xieventmask.mask_len = sizeof(bitMask);
                        bitMask |= XI_TouchBeginMask;
                        bitMask |= XI_TouchUpdateMask;
                        bitMask |= XI_TouchEndMask;
                        XIGrabDevice(static_cast<Display *>(m_xlib_display), xiEvent->deviceid, platformWindow->winId(), xiEvent->time, None, GrabModeAsync, GrabModeAsync, true, &xieventmask);
                    } else if (xiEvent->evtype == XI_TouchEnd)
                        XIUngrabDevice(static_cast<Display *>(m_xlib_display), xiEvent->deviceid, xiEvent->time);
                }
                if (touchPoint.state == Qt::TouchPointReleased)
                    // If a touchpoint was released, we can forget it, because the ID won't be reused.
                    m_touchPoints.remove(touchPoint.id);
                else
                    // Make sure that we don't send TouchPointPressed/Moved in more than one QTouchEvent
                    // with this touch point if the next XI2 event is about a different touch point.
                    touchPoint.state = Qt::TouchPointStationary;
            }
        }
#endif // XCB_USE_XINPUT22
    }
}

void QXcbConnection::handleEnterEvent(const xcb_enter_notify_event_t *)
{
#ifdef XCB_USE_XINPUT21
    QHash<int, ScrollingDevice>::iterator it = m_scrollingDevices.begin();
    const QHash<int, ScrollingDevice>::iterator end = m_scrollingDevices.end();
    while (it != end) {
        ScrollingDevice& scrollingDevice = it.value();
        int nrDevices = 0;
        XIDeviceInfo* xiDeviceInfo = XIQueryDevice(static_cast<Display *>(m_xlib_display), scrollingDevice.deviceId, &nrDevices);
        if (nrDevices <= 0) {
            it = m_scrollingDevices.erase(it);
            continue;
        }
        for (int c = 0; c < xiDeviceInfo->num_classes; ++c) {
            if (xiDeviceInfo->classes[c]->type == XIValuatorClass) {
                XIValuatorClassInfo *vci = reinterpret_cast<XIValuatorClassInfo *>(xiDeviceInfo->classes[c]);
                const int valuatorAtom = qatom(vci->label);
                if (valuatorAtom == QXcbAtom::RelHorizScroll || valuatorAtom == QXcbAtom::RelHorizWheel)
                    scrollingDevice.lastScrollPosition.setX(vci->value);
                else if (valuatorAtom == QXcbAtom::RelVertScroll || valuatorAtom == QXcbAtom::RelVertWheel)
                    scrollingDevice.lastScrollPosition.setY(vci->value);
            }
        }
        XIFreeDeviceInfo(xiDeviceInfo);
        ++it;
    }
#endif
}

void QXcbConnection::xi2HandleScrollEvent(void *event, ScrollingDevice &scrollingDevice)
{
#ifdef XCB_USE_XINPUT21
    xXIGenericDeviceEvent *xiEvent = reinterpret_cast<xXIGenericDeviceEvent *>(event);

    if (xiEvent->evtype == XI_Motion && scrollingDevice.orientations) {
        xXIDeviceEvent* xiDeviceEvent = reinterpret_cast<xXIDeviceEvent *>(event);
        if (QXcbWindow *platformWindow = platformWindowFromId(xiDeviceEvent->event)) {
            QPoint rawDelta;
            QPoint angleDelta;
            double value;
            if (scrollingDevice.orientations & Qt::Vertical) {
                if (xi2GetValuatorValueIfSet(xiDeviceEvent, scrollingDevice.verticalIndex, &value)) {
                    double delta = scrollingDevice.lastScrollPosition.y() - value;
                    scrollingDevice.lastScrollPosition.setY(value);
                    angleDelta.setY((delta / scrollingDevice.verticalIncrement) * 120);
                    // We do not set "pixel" delta if it is only measured in ticks.
                    if (scrollingDevice.verticalIncrement > 1)
                        rawDelta.setY(delta);
                }
            }
            if (scrollingDevice.orientations & Qt::Horizontal) {
                if (xi2GetValuatorValueIfSet(xiDeviceEvent, scrollingDevice.horizontalIndex, &value)) {
                    double delta = scrollingDevice.lastScrollPosition.x() - value;
                    scrollingDevice.lastScrollPosition.setX(value);
                    angleDelta.setX((delta / scrollingDevice.horizontalIncrement) * 120);
                    // We do not set "pixel" delta if it is only measured in ticks.
                    if (scrollingDevice.horizontalIncrement > 1)
                        rawDelta.setX(delta);
                }
            }
            if (!angleDelta.isNull()) {
                QPoint local(fixed1616ToReal(xiDeviceEvent->event_x), fixed1616ToReal(xiDeviceEvent->event_y));
                QPoint global(fixed1616ToReal(xiDeviceEvent->root_x), fixed1616ToReal(xiDeviceEvent->root_y));
                Qt::KeyboardModifiers modifiers = keyboard()->translateModifiers(xiDeviceEvent->mods.effective_mods);
                if (modifiers & Qt::AltModifier) {
                    std::swap(angleDelta.rx(), angleDelta.ry());
                    std::swap(rawDelta.rx(), rawDelta.ry());
                }
                QWindowSystemInterface::handleWheelEvent(platformWindow->window(), xiEvent->time, local, global, rawDelta, angleDelta, modifiers);
            }
        }
    } else if (xiEvent->evtype == XI_ButtonRelease && scrollingDevice.legacyOrientations) {
        xXIDeviceEvent* xiDeviceEvent = reinterpret_cast<xXIDeviceEvent *>(event);
        if (QXcbWindow *platformWindow = platformWindowFromId(xiDeviceEvent->event)) {
            QPoint angleDelta;
            if (scrollingDevice.legacyOrientations & Qt::Vertical) {
                if (xiDeviceEvent->detail == 4)
                    angleDelta.setY(120);
                else if (xiDeviceEvent->detail == 5)
                    angleDelta.setY(-120);
            }
            if (scrollingDevice.legacyOrientations & Qt::Horizontal) {
                if (xiDeviceEvent->detail == 6)
                    angleDelta.setX(120);
                else if (xiDeviceEvent->detail == 7)
                    angleDelta.setX(-120);
            }
            if (!angleDelta.isNull()) {
                QPoint local(fixed1616ToReal(xiDeviceEvent->event_x), fixed1616ToReal(xiDeviceEvent->event_y));
                QPoint global(fixed1616ToReal(xiDeviceEvent->root_x), fixed1616ToReal(xiDeviceEvent->root_y));
                Qt::KeyboardModifiers modifiers = keyboard()->translateModifiers(xiDeviceEvent->mods.effective_mods);
                if (modifiers & Qt::AltModifier)
                    std::swap(angleDelta.rx(), angleDelta.ry());
                QWindowSystemInterface::handleWheelEvent(platformWindow->window(), xiEvent->time, local, global, QPoint(), angleDelta, modifiers);
            }
        }
    }
#else
    Q_UNUSED(event);
    Q_UNUSED(scrollingDevice);
#endif // XCB_USE_XINPUT21
}

#ifndef QT_NO_TABLETEVENT
bool QXcbConnection::xi2HandleTabletEvent(void *event, TabletData *tabletData)
{
    bool handled = true;
    Display *xDisplay = static_cast<Display *>(m_xlib_display);
    xXIGenericDeviceEvent *xiEvent = static_cast<xXIGenericDeviceEvent *>(event);
    switch (xiEvent->evtype) {
    case XI_ButtonPress: // stylus down
        if (reinterpret_cast<xXIDeviceEvent *>(event)->detail == 1) { // ignore the physical buttons on the stylus
            tabletData->down = true;
            xi2ReportTabletEvent(*tabletData, xiEvent);
        } else
            handled = false;
        break;
    case XI_ButtonRelease: // stylus up
        if (reinterpret_cast<xXIDeviceEvent *>(event)->detail == 1) {
            tabletData->down = false;
            xi2ReportTabletEvent(*tabletData, xiEvent);
        } else
            handled = false;
        break;
    case XI_Motion:
        // Report TabletMove only when the stylus is touching the tablet.
        // No possibility to report proximity motion (no suitable Qt event exists yet).
        if (tabletData->down)
            xi2ReportTabletEvent(*tabletData, xiEvent);
        break;
    case XI_PropertyEvent: {
        xXIPropertyEvent *ev = reinterpret_cast<xXIPropertyEvent *>(event);
        if (ev->what == XIPropertyModified) {
            if (ev->property == atom(QXcbAtom::WacomSerialIDs)) {
                Atom propType;
                int propFormat;
                unsigned long numItems, bytesAfter;
                unsigned char *data;
                if (XIGetProperty(xDisplay, tabletData->deviceId, ev->property, 0, 100,
                                  0, AnyPropertyType, &propType, &propFormat,
                                  &numItems, &bytesAfter, &data) == Success) {
                    if (propType == atom(QXcbAtom::INTEGER) && propFormat == 32) {
                        int *ptr = reinterpret_cast<int *>(data);
                        for (unsigned long i = 0; i < numItems; ++i)
                            tabletData->serialId |= qint64(ptr[i]) << (i * 32);
                    }
                    XFree(data);
                }
                // With recent-enough X drivers this property change event seems to come always
                // when entering and leaving proximity. Due to the lack of other options hook up
                // the enter/leave events to it.
                if (tabletData->inProximity) {
                    tabletData->inProximity = false;
                    QWindowSystemInterface::handleTabletLeaveProximityEvent(QTabletEvent::Stylus,
                                                                            tabletData->pointerType,
                                                                            tabletData->serialId);
                } else {
                    tabletData->inProximity = true;
                    QWindowSystemInterface::handleTabletEnterProximityEvent(QTabletEvent::Stylus,
                                                                            tabletData->pointerType,
                                                                            tabletData->serialId);
                }
            }
        }
        break;
    }
    default:
        handled = false;
        break;
    }
    return handled;
}

void QXcbConnection::xi2ReportTabletEvent(const TabletData &tabletData, void *event)
{
    xXIDeviceEvent *ev = reinterpret_cast<xXIDeviceEvent *>(event);
    QXcbWindow *xcbWindow = platformWindowFromId(ev->event);
    if (!xcbWindow)
        return;
    QWindow *window = xcbWindow->window();
    const double scale = 65536.0;
    QPointF local(ev->event_x / scale, ev->event_y / scale);
    QPointF global(ev->root_x / scale, ev->root_y / scale);
    double pressure = 0, rotation = 0;
    int xTilt = 0, yTilt = 0;

    for (QHash<int, TabletData::ValuatorClassInfo>::const_iterator it = tabletData.valuatorInfo.constBegin(),
            ite = tabletData.valuatorInfo.constEnd(); it != ite; ++it) {
        int valuator = it.key();
        const TabletData::ValuatorClassInfo &classInfo(it.value());
        double value;
        if (xi2GetValuatorValueIfSet(event, classInfo.number, &value)) {
            double normalizedValue = (value - classInfo.minVal) / double(classInfo.maxVal - classInfo.minVal);
            switch (valuator) {
            case QXcbAtom::AbsPressure:
                pressure = normalizedValue;
                break;
            case QXcbAtom::AbsTiltX:
                xTilt = value;
                break;
            case QXcbAtom::AbsTiltY:
                yTilt = value;
                break;
            case QXcbAtom::AbsWheel:
                rotation = value / 64.0;
                break;
            default:
                break;
            }
        }
    }

    if (Q_UNLIKELY(debug_xinput))
        qDebug("XI2 tablet event type %d seq %d detail %d pos %6.1f, %6.1f root pos %6.1f, %6.1f pressure %4.2lf tilt %d, %d rotation %6.2lf",
            ev->type, ev->sequenceNumber, ev->detail,
            fixed1616ToReal(ev->event_x), fixed1616ToReal(ev->event_y),
            fixed1616ToReal(ev->root_x), fixed1616ToReal(ev->root_y),
            pressure, xTilt, yTilt, rotation);

    QWindowSystemInterface::handleTabletEvent(window, tabletData.down, local, global,
                                              QTabletEvent::Stylus, tabletData.pointerType,
                                              pressure, xTilt, yTilt, 0,
                                              rotation, 0, tabletData.serialId);
}
#endif // QT_NO_TABLETEVENT

#endif // XCB_USE_XINPUT2
