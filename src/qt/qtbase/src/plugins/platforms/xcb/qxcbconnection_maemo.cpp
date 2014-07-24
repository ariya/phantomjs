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

#ifdef XCB_USE_XINPUT2_MAEMO

#include "qxcbwindow.h"
#include <qpa/qwindowsysteminterface.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2proto.h>
#include <X11/Xatom.h>

QT_BEGIN_NAMESPACE

// Define it here to work around XLib defining Bool and stuff.
// We can't declare those variables in the header without facing include order headaches.
struct XInput2MaemoData {
    XInput2MaemoData()
    : use_xinput(false)
    , xinput_opcode(0)
    , xinput_eventbase(0)
    , xinput_errorbase(0)
    , xideviceinfo(0)
    , xibuttonclassinfo(0)
    , xiMaxContacts(0)
    , qtTouchDevice(0)
    {
    }
    // true if Qt is compiled w/ XInput2 or Tablet support and we have a tablet.
    bool use_xinput;
    int xinput_opcode;
    int xinput_eventbase;
    int xinput_errorbase;
    // device info for the master pointer Qt is using
    XIDeviceInfo *xideviceinfo;
    XIButtonClassInfo *xibuttonclassinfo;
    int xiMaxContacts;
    QList<QWindowSystemInterface::TouchPoint> allTouchPoints;
    QTouchDevice *qtTouchDevice;
};

bool QXcbConnection::isUsingXInput2Maemo()
{
    return m_xinputData && m_xinputData->use_xinput && m_xinputData->xiMaxContacts != 0;
}

void QXcbConnection::initializeXInput2Maemo()
{
    Q_ASSERT(!m_xinputData);
    m_xinputData = new XInput2MaemoData;
    m_xinputData->use_xinput = XQueryExtension((Display *)m_xlib_display, "XInputExtension", &m_xinputData->xinput_opcode,
                                      &m_xinputData->xinput_eventbase, &m_xinputData->xinput_errorbase);
    if (m_xinputData->use_xinput) {
        // we want XInput2
        int ximajor = 2, ximinor = 0;
        if (XIQueryVersion((Display *)m_xlib_display, &ximajor, &ximinor) == BadRequest) {
            // XInput2 not available
            m_xinputData->use_xinput = false;
        } else {
            // find the first master pointer and use this throughout Qt
            // when making XI2 calls that need a device id (rationale is that
            // for the time being, most setups will only have one master
            // pointer (despite having multiple slaves)
            int deviceCount = 0;
            XIDeviceInfo *devices = XIQueryDevice((Display *)m_xlib_display, XIAllMasterDevices, &deviceCount);
            if (devices) {
                for (int i = 0; i < deviceCount; ++i) {
                    if (devices[i].use == XIMasterPointer) {
                        int unused = 0;
                        m_xinputData->xideviceinfo = XIQueryDevice((Display *)m_xlib_display, devices[i].deviceid, &unused);
                        break;
                    }
                }
                XIFreeDeviceInfo(devices);
            }
            if (!m_xinputData->xideviceinfo)
                qFatal("Qt: Internal error, no XI2 master pointer found.");

            // find the button info
            m_xinputData->xibuttonclassinfo = 0;
            for (int i = 0; i < m_xinputData->xideviceinfo->num_classes; ++i) {
                if (m_xinputData->xideviceinfo->classes[i]->type == XIButtonClass) {
                    m_xinputData->xibuttonclassinfo = (XIButtonClassInfo *) m_xinputData->xideviceinfo->classes[i];
                    break;
                }
            }

            // find the "Max Contacts" property on the device
            Atom typeReturn;
            int formatReturn;
            ulong countReturn, bytesReturn;
            uchar *data = 0;
            if (XIGetProperty((Display *)m_xlib_display,
                              m_xinputData->xibuttonclassinfo->sourceid,
                              atom(QXcbAtom::MaxContacts),
                              0, 1,
                              False,
                              XA_INTEGER,
                              &typeReturn,
                              &formatReturn,
                              &countReturn,
                              &bytesReturn,
                              &data) == Success
                && data != 0
                && typeReturn == XA_INTEGER
                && formatReturn == 8
                && countReturn == 1) {
                // touch driver reported the max number of touch-points
                m_xinputData->xiMaxContacts = data[0];
            } else {
                m_xinputData->xiMaxContacts = 0;
            }
            if (data)
                XFree(data);
            XFlush((Display *)m_xlib_display);
        }
    }
}

void QXcbConnection::finalizeXInput2Maemo()
{
    if (m_xinputData && m_xinputData->xideviceinfo) {
        XIFreeDeviceInfo(m_xinputData->xideviceinfo);
    }
    delete m_xinputData;
}

void QXcbConnection::handleGenericEventMaemo(xcb_ge_event_t *event)
{
    if (m_xinputData->use_xinput && xi2PrepareXIGenericDeviceEvent(event, m_xinputData->xinput_opcode)) {
        xXIGenericDeviceEvent* xievent = (xXIGenericDeviceEvent*)event;

        // On Harmattan XInput2 is hacked to give touch points updates into standard mouse button press/motion events.
        if (m_xinputData->xiMaxContacts != 0
            && (xievent->evtype == XI_ButtonPress
                || xievent->evtype == XI_ButtonRelease
                || xievent->evtype == XI_Motion)) {
            xXIDeviceEvent *xideviceevent = (xXIDeviceEvent *)xievent;
            QList<QWindowSystemInterface::TouchPoint> touchPoints = m_xinputData->allTouchPoints;
            if (touchPoints.count() != m_xinputData->xiMaxContacts) {
                // initial event, allocate space for all (potential) touch points
                touchPoints.reserve(m_xinputData->xiMaxContacts);
                for (int i = 0; i < m_xinputData->xiMaxContacts; ++i) {
                    QWindowSystemInterface::TouchPoint tp;
                    tp.id = i;
                    tp.state = Qt::TouchPointReleased;
                    touchPoints << tp;
                }
            }
            qreal x, y, nx, ny, w = 0.0, h = 0.0, p = -1.0;
            int id;
            uint active = 0;
            for (int i = 0; i < m_xinputData->xideviceinfo->num_classes; ++i) {
                XIAnyClassInfo *classinfo = m_xinputData->xideviceinfo->classes[i];
                if (classinfo->type == XIValuatorClass) {
                    XIValuatorClassInfo *valuatorclassinfo = reinterpret_cast<XIValuatorClassInfo *>(classinfo);
                    int n = valuatorclassinfo->number;
                    double value;
                    if (!xi2GetValuatorValueIfSet(xideviceevent, n, &value))
                        continue;

                    if (valuatorclassinfo->label == atom(QXcbAtom::AbsMTPositionX)) {
                        x = value;
                        nx = (x - valuatorclassinfo->min) / (valuatorclassinfo->max - valuatorclassinfo->min);
                    } else if (valuatorclassinfo->label == atom(QXcbAtom::AbsMTPositionY)) {
                        y = value;
                        ny = (y - valuatorclassinfo->min) / (valuatorclassinfo->max - valuatorclassinfo->min);
                    } else if (valuatorclassinfo->label == atom(QXcbAtom::AbsMTTouchMajor)) {
                        w = value;
                    } else if (valuatorclassinfo->label == atom(QXcbAtom::AbsMTTouchMinor)) {
                        h = value;
                    } else if (valuatorclassinfo->label == atom(QXcbAtom::AbsMTPressure)) {
                        p = (value - valuatorclassinfo->min) / (valuatorclassinfo->max - valuatorclassinfo->min);
                    } else if (valuatorclassinfo->label == atom(QXcbAtom::AbsMTTrackingID)) {
                        id = value;
                        active |= 1 << id;
                        QWindowSystemInterface::TouchPoint &touchPoint = touchPoints[id];

                        Qt::TouchPointState newstate;
                        if (touchPoint.state == Qt::TouchPointReleased) {
                            newstate = Qt::TouchPointPressed;
                        } else {
                            if (touchPoint.area.center() != QPoint(x, y))
                                newstate = Qt::TouchPointMoved;
                            else
                                newstate = Qt::TouchPointStationary;
                        }

                        touchPoint.state = newstate;
                        touchPoint.area = QRectF(x - w/2, y - h/2, w, h);
                        touchPoint.normalPosition = QPointF(nx, ny);
                        touchPoint.pressure = p;
                    }
                }
            }

            // mark previously-active-but-now-inactive touch points as released
            for (int i = 0; i < touchPoints.count(); ++i)
                if (!(active & (1 << i)) && touchPoints.at(i).state != Qt::TouchPointReleased)
                    touchPoints[i].state = Qt::TouchPointReleased;

            if (QXcbWindow *platformWindow = platformWindowFromId(xideviceevent->event)) {
                QTouchDevice *dev = m_xinputData->qtTouchDevice;
                if (!dev) {
                    dev = new QTouchDevice;
                    dev->setType(QTouchDevice::TouchScreen);
                    dev->setCapabilities(QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::Pressure | QTouchDevice::NormalizedPosition);
                    QWindowSystemInterface::registerTouchDevice(dev);
                    m_xinputData->qtTouchDevice = dev;
                }
                QWindowSystemInterface::handleTouchEvent(platformWindow->window(), xideviceevent->time, dev, touchPoints);
            }

            if (xideviceevent->evtype == XI_ButtonRelease) {
                // final event, forget touch state
                m_xinputData->allTouchPoints.clear();
            } else {
                // save current state so that we have something to reuse later
                m_xinputData->allTouchPoints = touchPoints;
            }

        }
    }
}

QT_END_NAMESPACE

#endif // XCB_USE_XINPUT2_MAEMO

