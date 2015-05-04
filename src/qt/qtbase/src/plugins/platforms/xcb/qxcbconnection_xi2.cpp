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

struct XInput2TouchDeviceData {
    XInput2TouchDeviceData()
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
    // TODO Qt 6 (or perhaps earlier): remove these redundant env variables
    if (qEnvironmentVariableIsSet("QT_XCB_DEBUG_XINPUT"))
        const_cast<QLoggingCategory&>(lcQpaXInput()).setEnabled(QtDebugMsg, true);
    if (qEnvironmentVariableIsSet("QT_XCB_DEBUG_XINPUT_DEVICES"))
        const_cast<QLoggingCategory&>(lcQpaXInputDevices()).setEnabled(QtDebugMsg, true);
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
#ifdef XCB_USE_XINPUT22
            qCDebug(lcQpaXInputDevices, "XInput version %d.%d is available and Qt supports 2.2 or greater", xiMajor, m_xi2Minor);
#else
            qCDebug(lcQpaXInputDevices, "XInput version %d.%d is available and Qt supports 2.0", xiMajor, m_xi2Minor);
#endif
        }

        xi2SetupDevices();
    }
}

void QXcbConnection::xi2SetupDevices()
{
#ifndef QT_NO_TABLETEVENT
    m_tabletData.clear();
#endif
    m_scrollingDevices.clear();

    if (!m_xi2Enabled)
        return;

    Display *xDisplay = static_cast<Display *>(m_xlib_display);
    int deviceCount = 0;
    XIDeviceInfo *devices = XIQueryDevice(xDisplay, XIAllDevices, &deviceCount);
    for (int i = 0; i < deviceCount; ++i) {
        // Only non-master pointing devices are relevant here.
        if (devices[i].use != XISlavePointer)
            continue;
        qCDebug(lcQpaXInputDevices) << "input device "<< devices[i].name;
#ifndef QT_NO_TABLETEVENT
        TabletData tabletData;
#endif
        ScrollingDevice scrollingDevice;
        for (int c = 0; c < devices[i].num_classes; ++c) {
            switch (devices[i].classes[c]->type) {
            case XIValuatorClass: {
                XIValuatorClassInfo *vci = reinterpret_cast<XIValuatorClassInfo *>(devices[i].classes[c]);
                const int valuatorAtom = qatom(vci->label);
                qCDebug(lcQpaXInputDevices) << "   has valuator" << atomName(vci->label) << "recognized?" << (valuatorAtom < QXcbAtom::NAtoms);
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
                    // Some drivers have no labels on the wheel buttons, some have no label on just one and some have no label on
                    // button 4 and the wrong one on button 5. So we just check that they are not labelled with unrelated buttons.
                    if ((!label4 || qatom(label4) == QXcbAtom::ButtonWheelUp || qatom(label4) == QXcbAtom::ButtonWheelDown) &&
                        (!label5 || qatom(label5) == QXcbAtom::ButtonWheelUp || qatom(label5) == QXcbAtom::ButtonWheelDown))
                        scrollingDevice.legacyOrientations |= Qt::Vertical;
                }
                if (bci->num_buttons >= 7) {
                    Atom label6 = bci->labels[5];
                    Atom label7 = bci->labels[6];
                    if ((!label6 || qatom(label6) == QXcbAtom::ButtonHorizWheelLeft) && (!label7 || qatom(label7) == QXcbAtom::ButtonHorizWheelRight))
                        scrollingDevice.legacyOrientations |= Qt::Horizontal;
                }
                qCDebug(lcQpaXInputDevices, "   has %d buttons", bci->num_buttons);
                break;
            }
#endif
            case XIKeyClass:
                qCDebug(lcQpaXInputDevices) << "   it's a keyboard";
                break;
            case XITouchClass:
                // will be handled in deviceForId()
                break;
            default:
                qCDebug(lcQpaXInputDevices) << "   has class" << devices[i].classes[c]->type;
                break;
            }
        }
        bool isTablet = false;
#ifndef QT_NO_TABLETEVENT
        // If we have found the valuators which we expect a tablet to have, it might be a tablet.
        if (tabletData.valuatorInfo.contains(QXcbAtom::AbsX) &&
                tabletData.valuatorInfo.contains(QXcbAtom::AbsY) &&
                tabletData.valuatorInfo.contains(QXcbAtom::AbsPressure))
            isTablet = true;

        // But we need to be careful not to take the touch and tablet-button devices as tablets.
        QByteArray name = QByteArray(devices[i].name).toLower();
        QString dbgType = QLatin1String("UNKNOWN");
        if (name.contains("eraser")) {
            isTablet = true;
            tabletData.pointerType = QTabletEvent::Eraser;
            dbgType = QLatin1String("eraser");
        } else if (name.contains("cursor")) {
            isTablet = true;
            tabletData.pointerType = QTabletEvent::Cursor;
            dbgType = QLatin1String("cursor");
        } else if ((name.contains("pen") || name.contains("stylus")) && isTablet) {
            tabletData.pointerType = QTabletEvent::Pen;
            dbgType = QLatin1String("pen");
        } else if (name.contains("wacom") && isTablet && !name.contains("touch")) {
            // combined device (evdev) rather than separate pen/eraser (wacom driver)
            tabletData.pointerType = QTabletEvent::Pen;
            dbgType = QLatin1String("pen");
        } else if (name.contains("aiptek") /* && device == QXcbAtom::KEYBOARD */) {
            // some "Genius" tablets
            isTablet = true;
            tabletData.pointerType = QTabletEvent::Pen;
            dbgType = QLatin1String("pen");
        } else {
            isTablet = false;
        }

        if (isTablet) {
            tabletData.deviceId = devices[i].deviceid;
            m_tabletData.append(tabletData);
            qCDebug(lcQpaXInputDevices) << "   it's a tablet with pointer type" << dbgType;
        }
#endif // QT_NO_TABLETEVENT

#ifdef XCB_USE_XINPUT21
        if (scrollingDevice.orientations || scrollingDevice.legacyOrientations) {
            scrollingDevice.deviceId = devices[i].deviceid;
            // Only use legacy wheel button events when we don't have real scroll valuators.
            scrollingDevice.legacyOrientations &= ~scrollingDevice.orientations;
            m_scrollingDevices.insert(scrollingDevice.deviceId, scrollingDevice);
            qCDebug(lcQpaXInputDevices) << "   it's a scrolling device";
        }
#endif

        if (!isTablet) {
            // touchDeviceForId populates XInput2DeviceData the first time it is called
            // with a new deviceId. On subsequent calls it will return the cached object.
            XInput2TouchDeviceData *dev = touchDeviceForId(devices[i].deviceid);
            if (dev && lcQpaXInputDevices().isDebugEnabled()) {
                if (dev->qtTouchDevice->type() == QTouchDevice::TouchScreen)
                    qCDebug(lcQpaXInputDevices, "   it's a touchscreen with type %d capabilities 0x%X max touch points %d",
                            dev->qtTouchDevice->type(), (unsigned int)dev->qtTouchDevice->capabilities(),
                            dev->qtTouchDevice->maximumTouchPoints());
                else if (dev->qtTouchDevice->type() == QTouchDevice::TouchPad)
                    qCDebug(lcQpaXInputDevices, "   it's a touchpad with type %d capabilities 0x%X max touch points %d size %f x %f",
                            dev->qtTouchDevice->type(), (unsigned int)dev->qtTouchDevice->capabilities(),
                            dev->qtTouchDevice->maximumTouchPoints(),
                            dev->size.width(), dev->size.height());
            }
        }
    }
    XIFreeDeviceInfo(devices);
}

void QXcbConnection::finalizeXInput2()
{
    foreach (XInput2TouchDeviceData *dev, m_touchDevices) {
        if (dev->xiDeviceInfo)
            XIFreeDeviceInfo(dev->xiDeviceInfo);
        delete dev;
    }
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
    if (!m_touchDevices.isEmpty()) {
        mask.deviceid = XIAllMasterDevices;
        Status result = XISelectEvents(xDisplay, window, &mask, 1);
        // If we select for touch events on the master pointer, XInput2
        // will not synthesize mouse events. This means Qt must do it,
        // which is also preferable, since Qt can control better when
        // to do so.
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
        unsigned int tabletBitMask;
        unsigned char *xiTabletBitMask = reinterpret_cast<unsigned char *>(&tabletBitMask);
        QVector<XIEventMask> xiEventMask(m_tabletData.count());
        tabletBitMask = XI_ButtonPressMask;
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
        unsigned int scrollBitMask;
        unsigned char *xiScrollBitMask = reinterpret_cast<unsigned char *>(&scrollBitMask);

        scrollBitMask = XI_MotionMask;
        scrollBitMask |= XI_ButtonReleaseMask;
        int i=0;
        Q_FOREACH (const ScrollingDevice& scrollingDevice, m_scrollingDevices) {
            if (tabletDevices.contains(scrollingDevice.deviceId))
                continue; // All necessary events are already captured.
            xiEventMask[i].deviceid = scrollingDevice.deviceId;
            xiEventMask[i].mask_len = sizeof(scrollBitMask);
            xiEventMask[i].mask = xiScrollBitMask;
            i++;
        }
        XISelectEvents(xDisplay, window, xiEventMask.data(), i);
    }
#else
    Q_UNUSED(xiBitMask);
#endif

    {
        // Listen for hotplug events
        XIEventMask xiEventMask;
        bitMask = XI_HierarchyChangedMask;
        xiEventMask.deviceid = XIAllDevices;
        xiEventMask.mask_len = sizeof(bitMask);
        xiEventMask.mask = xiBitMask;
        XISelectEvents(xDisplay, window, &xiEventMask, 1);
    }
}

XInput2TouchDeviceData *QXcbConnection::touchDeviceForId(int id)
{
    XInput2TouchDeviceData *dev = m_touchDevices[id];
    if (!dev) {
        int nrDevices = 0;
        QTouchDevice::Capabilities caps = 0;
        dev = new XInput2TouchDeviceData;
        dev->xiDeviceInfo = XIQueryDevice(static_cast<Display *>(m_xlib_display), id, &nrDevices);
        if (nrDevices <= 0) {
            delete dev;
            return 0;
        }
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
                qCDebug(lcQpaXInputDevices, "   has touch class with mode %d", tci->mode);
                switch (tci->mode) {
                case XIDependentTouch:
                    type = QTouchDevice::TouchPad;
                    break;
                case XIDirectTouch:
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
                } else if (vci->label == atom(QXcbAtom::AbsX)) {
                    dev->size.setHeight((vci->max - vci->min) * 1000.0 / vci->resolution);
                } else if (vci->label == atom(QXcbAtom::AbsY)) {
                    dev->size.setWidth((vci->max - vci->min) * 1000.0 / vci->resolution);
                }
                break;
            }
            default:
                break;
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
            XIFreeDeviceInfo(dev->xiDeviceInfo);
            delete dev;
            dev = 0;
        }
    }
    return dev;
}

#if defined(XCB_USE_XINPUT21) || !defined(QT_NO_TABLETEVENT)
static qreal fixed1616ToReal(FP1616 val)
{
    return (qreal(val >> 16)) + (val & 0xFFFF) / (qreal)0xFFFF;
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

        if (xiEvent->evtype == XI_HierarchyChanged) {
            xi2HandleHierachyEvent(xiEvent);
            return;
        }
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
            if (Q_UNLIKELY(lcQpaXInput().isDebugEnabled()))
                qCDebug(lcQpaXInput, "XI2 touch event type %d seq %d detail %d pos %6.1f, %6.1f root pos %6.1f, %6.1f",
                        event->event_type, xiEvent->sequenceNumber, xiDeviceEvent->detail,
                        fixed1616ToReal(xiDeviceEvent->event_x), fixed1616ToReal(xiDeviceEvent->event_y),
                        fixed1616ToReal(xiDeviceEvent->root_x), fixed1616ToReal(xiDeviceEvent->root_y) );

            if (QXcbWindow *platformWindow = platformWindowFromId(xiDeviceEvent->event)) {
                XInput2TouchDeviceData *dev = touchDeviceForId(xiDeviceEvent->sourceid);
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
                        if (Q_UNLIKELY(lcQpaXInput().isDebugEnabled()))
                            qCDebug(lcQpaXInput, "   valuator %20s value %lf from range %lf -> %lf",
                                    atomName(vci->label).constData(), value, vci->min, vci->max );
                        if (vci->label == atom(QXcbAtom::RelX)) {
                            nx = valuatorNormalized(value, vci);
                        } else if (vci->label == atom(QXcbAtom::RelY)) {
                            ny = valuatorNormalized(value, vci);
                        } else if (vci->label == atom(QXcbAtom::AbsX)) {
                            nx = valuatorNormalized(value, vci);
                        } else if (vci->label == atom(QXcbAtom::AbsY)) {
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

                if (Q_UNLIKELY(lcQpaXInput().isDebugEnabled()))
                    qCDebug(lcQpaXInput) << "   touchpoint "  << touchPoint.id << " state " << touchPoint.state << " pos norm " << touchPoint.normalPosition <<
                            " area " << touchPoint.area << " pressure " << touchPoint.pressure;
                QWindowSystemInterface::handleTouchEvent(platformWindow->window(), xiEvent->time, dev->qtTouchDevice, m_touchPoints.values());
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

void QXcbConnection::xi2HandleHierachyEvent(void *event)
{
    xXIHierarchyEvent *xiEvent = reinterpret_cast<xXIHierarchyEvent *>(event);
    // We only care about hotplugged devices
    if (!(xiEvent->flags & (XISlaveRemoved | XISlaveAdded)))
        return;
    xi2SetupDevices();
    // Reselect events for all event-listening windows.
    Q_FOREACH (xcb_window_t window, m_mapper.keys()) {
        xi2Select(window);
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
                const int dpr = int(platformWindow->devicePixelRatio());
                QPoint local(fixed1616ToReal(xiDeviceEvent->event_x)/dpr, fixed1616ToReal(xiDeviceEvent->event_y)/dpr);
                QPoint global(fixed1616ToReal(xiDeviceEvent->root_x)/dpr, fixed1616ToReal(xiDeviceEvent->root_y)/dpr);
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
                const int dpr = int(platformWindow->devicePixelRatio());
                QPoint local(fixed1616ToReal(xiDeviceEvent->event_x)/dpr, fixed1616ToReal(xiDeviceEvent->event_y)/dpr);
                QPoint global(fixed1616ToReal(xiDeviceEvent->root_x)/dpr, fixed1616ToReal(xiDeviceEvent->root_y)/dpr);
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

static Qt::MouseButton xiToQtMouseButton(uint32_t b) {
    switch (b) {
    case 1: return Qt::LeftButton;
    case 2: return Qt::MiddleButton;
    case 3: return Qt::RightButton;
    // 4-7 are for scrolling
    default: break;
    }
    if (b >= 8 && b <= Qt::MaxMouseButton)
        return static_cast<Qt::MouseButton>(Qt::BackButton << (b - 8));
    return Qt::NoButton;
}

static QTabletEvent::TabletDevice toolIdToTabletDevice(quint32 toolId) {
    // keep in sync with wacom_intuos_inout() in Linux kernel driver wacom_wac.c
    switch (toolId) {
    case 0xd12:
    case 0x912:
    case 0x112:
    case 0x913: /* Intuos3 Airbrush */
    case 0x91b: /* Intuos3 Airbrush Eraser */
    case 0x902: /* Intuos4/5 13HD/24HD Airbrush */
    case 0x90a: /* Intuos4/5 13HD/24HD Airbrush Eraser */
    case 0x100902: /* Intuos4/5 13HD/24HD Airbrush */
    case 0x10090a: /* Intuos4/5 13HD/24HD Airbrush Eraser */
        return QTabletEvent::Airbrush;
    case 0x007: /* Mouse 4D and 2D */
    case 0x09c:
    case 0x094:
        return QTabletEvent::FourDMouse;
    case 0x017: /* Intuos3 2D Mouse */
    case 0x806: /* Intuos4 Mouse */
    case 0x096: /* Lens cursor */
    case 0x097: /* Intuos3 Lens cursor */
    case 0x006: /* Intuos4 Lens cursor */
        return QTabletEvent::Puck;
    case 0x885:    /* Intuos3 Art Pen (Marker Pen) */
    case 0x100804: /* Intuos4/5 13HD/24HD Art Pen */
    case 0x10080c: /* Intuos4/5 13HD/24HD Art Pen Eraser */
        return QTabletEvent::RotationStylus;
    case 0:
        return QTabletEvent::NoDevice;
    }
    return QTabletEvent::Stylus;  // Safe default assumption if nonzero
}

#ifndef QT_NO_TABLETEVENT
bool QXcbConnection::xi2HandleTabletEvent(void *event, TabletData *tabletData)
{
    bool handled = true;
    Display *xDisplay = static_cast<Display *>(m_xlib_display);
    xXIGenericDeviceEvent *xiEvent = static_cast<xXIGenericDeviceEvent *>(event);
    switch (xiEvent->evtype) {
    case XI_ButtonPress: {
        Qt::MouseButton b = xiToQtMouseButton(reinterpret_cast<xXIDeviceEvent *>(event)->detail);
        tabletData->buttons |= b;
        xi2ReportTabletEvent(*tabletData, xiEvent);
        break;
    }
    case XI_ButtonRelease: {
        Qt::MouseButton b = xiToQtMouseButton(reinterpret_cast<xXIDeviceEvent *>(event)->detail);
        tabletData->buttons ^= b;
        xi2ReportTabletEvent(*tabletData, xiEvent);
        break;
    }
    case XI_Motion:
        // Report TabletMove only when the stylus is touching the tablet or any button is pressed.
        // TODO: report proximity (hover) motion (no suitable Qt event exists yet).
        if (tabletData->buttons != Qt::NoButton)
            xi2ReportTabletEvent(*tabletData, xiEvent);
        break;
    case XI_PropertyEvent: {
        // This is the wacom driver's way of reporting tool proximity.
        // The evdev driver doesn't do it this way.
        xXIPropertyEvent *ev = reinterpret_cast<xXIPropertyEvent *>(event);
        if (ev->what == XIPropertyModified) {
            if (ev->property == atom(QXcbAtom::WacomSerialIDs)) {
                enum WacomSerialIndex {
                    _WACSER_USB_ID = 0,
                    _WACSER_LAST_TOOL_SERIAL,
                    _WACSER_LAST_TOOL_ID,
                    _WACSER_TOOL_SERIAL,
                    _WACSER_TOOL_ID,
                    _WACSER_COUNT
                };
                Atom propType;
                int propFormat;
                unsigned long numItems, bytesAfter;
                unsigned char *data;
                if (XIGetProperty(xDisplay, tabletData->deviceId, ev->property, 0, 100,
                                  0, AnyPropertyType, &propType, &propFormat,
                                  &numItems, &bytesAfter, &data) == Success) {
                    if (propType == atom(QXcbAtom::INTEGER) && propFormat == 32 && numItems == _WACSER_COUNT) {
                        quint32 *ptr = reinterpret_cast<quint32 *>(data);
                        quint32 tool = ptr[_WACSER_TOOL_ID];
                        // Workaround for http://sourceforge.net/p/linuxwacom/bugs/246/
                        // e.g. on Thinkpad Helix, tool ID will be 0 and serial will be 1
                        if (!tool && ptr[_WACSER_TOOL_SERIAL])
                            tool = ptr[_WACSER_TOOL_SERIAL];

                        // The property change event informs us which tool is in proximity or which one left proximity.
                        if (tool) {
                            tabletData->inProximity = true;
                            tabletData->tool = toolIdToTabletDevice(tool);
                            tabletData->serialId = qint64(ptr[_WACSER_USB_ID]) << 32 | qint64(ptr[_WACSER_TOOL_SERIAL]);
                            QWindowSystemInterface::handleTabletEnterProximityEvent(tabletData->tool,
                                                                                    tabletData->pointerType,
                                                                                    tabletData->serialId);
                        } else {
                            tabletData->inProximity = false;
                            tabletData->tool = toolIdToTabletDevice(ptr[_WACSER_LAST_TOOL_ID]);
                            // Workaround for http://sourceforge.net/p/linuxwacom/bugs/246/
                            // e.g. on Thinkpad Helix, tool ID will be 0 and serial will be 1
                            if (!tabletData->tool)
                                tabletData->tool = toolIdToTabletDevice(ptr[_WACSER_LAST_TOOL_SERIAL]);
                            tabletData->serialId = qint64(ptr[_WACSER_USB_ID]) << 32 | qint64(ptr[_WACSER_LAST_TOOL_SERIAL]);
                            QWindowSystemInterface::handleTabletLeaveProximityEvent(tabletData->tool,
                                                                                    tabletData->pointerType,
                                                                                    tabletData->serialId);
                        }
                        // TODO maybe have a hash of tabletData->deviceId to device data so we can
                        // look up the tablet name here, and distinguish multiple tablets
                        qCDebug(lcQpaXInput, "XI2 proximity change on tablet %d (USB %x): last tool: %x id %x current tool: %x id %x TabletDevice %d",
                            ev->deviceid, ptr[_WACSER_USB_ID], ptr[_WACSER_LAST_TOOL_SERIAL], ptr[_WACSER_LAST_TOOL_ID],
                            ptr[_WACSER_TOOL_SERIAL], ptr[_WACSER_TOOL_ID], tabletData->tool);
                    }
                    XFree(data);
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

void QXcbConnection::xi2ReportTabletEvent(TabletData &tabletData, void *event)
{
    xXIDeviceEvent *ev = reinterpret_cast<xXIDeviceEvent *>(event);
    QXcbWindow *xcbWindow = platformWindowFromId(ev->event);
    if (!xcbWindow)
        return;
    QWindow *window = xcbWindow->window();
    const double scale = 65536.0;
    QPointF local(ev->event_x / scale, ev->event_y / scale);
    QPointF global(ev->root_x / scale, ev->root_y / scale);
    double pressure = 0, rotation = 0, tangentialPressure = 0;
    int xTilt = 0, yTilt = 0;

    for (QHash<int, TabletData::ValuatorClassInfo>::iterator it = tabletData.valuatorInfo.begin(),
            ite = tabletData.valuatorInfo.end(); it != ite; ++it) {
        int valuator = it.key();
        TabletData::ValuatorClassInfo &classInfo(it.value());
        xi2GetValuatorValueIfSet(event, classInfo.number, &classInfo.curVal);
        double normalizedValue = (classInfo.curVal - classInfo.minVal) / (classInfo.maxVal - classInfo.minVal);
        switch (valuator) {
        case QXcbAtom::AbsPressure:
            pressure = normalizedValue;
            break;
        case QXcbAtom::AbsTiltX:
            xTilt = classInfo.curVal;
            break;
        case QXcbAtom::AbsTiltY:
            yTilt = classInfo.curVal;
            break;
        case QXcbAtom::AbsWheel:
            switch (tabletData.tool) {
            case QTabletEvent::Airbrush:
                tangentialPressure = normalizedValue * 2.0 - 1.0; // Convert 0..1 range to -1..+1 range
                break;
            case QTabletEvent::RotationStylus:
                rotation = normalizedValue * 360.0 - 180.0; // Convert 0..1 range to -180..+180 degrees
                break;
            default:    // Other types of styli do not use this valuator
                break;
            }
            break;
        default:
            break;
        }
    }

    if (Q_UNLIKELY(lcQpaXInput().isDebugEnabled()))
        qCDebug(lcQpaXInput, "XI2 event on tablet %d with tool %d type %d seq %d detail %d pos %6.1f, %6.1f root pos %6.1f, %6.1f buttons 0x%x pressure %4.2lf tilt %d, %d rotation %6.2lf",
            ev->deviceid, tabletData.tool, ev->evtype, ev->sequenceNumber, ev->detail,
            fixed1616ToReal(ev->event_x), fixed1616ToReal(ev->event_y),
            fixed1616ToReal(ev->root_x), fixed1616ToReal(ev->root_y),
            (int)tabletData.buttons, pressure, xTilt, yTilt, rotation);

    QWindowSystemInterface::handleTabletEvent(window, local, global,
                                              tabletData.tool, tabletData.pointerType,
                                              tabletData.buttons, pressure,
                                              xTilt, yTilt, tangentialPressure,
                                              rotation, 0, tabletData.serialId);
}
#endif // QT_NO_TABLETEVENT

#endif // XCB_USE_XINPUT2
