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

#include "qwindowstabletsupport.h"

#ifndef QT_NO_TABLETEVENT

#include "qwindowscontext.h"
#include "qwindowskeymapper.h"
#include "qwindowswindow.h"

#include <qpa/qwindowsysteminterface.h>

#include <QtGui/QTabletEvent>
#include <QtGui/QScreen>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtCore/QDebug>
#include <QtCore/QScopedArrayPointer>
#include <QtCore/QtMath>

#include <private/qguiapplication_p.h>
#include <QtCore/private/qsystemlibrary_p.h>

// Note: The definition of the PACKET structure in pktdef.h depends on this define.
#define PACKETDATA (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_TANGENT_PRESSURE | PK_ORIENTATION | PK_CURSOR | PK_Z)
#include <pktdef.h>

QT_BEGIN_NAMESPACE

enum {
    PacketMode = 0,
    TabletPacketQSize = 128,
    DeviceIdMask = 0xFF6, // device type mask && device color mask
    CursorTypeBitMask = 0x0F06 // bitmask to find the specific cursor type (see Wacom FAQ)
};

extern "C" LRESULT QT_WIN_CALLBACK qWindowsTabletSupportWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WT_PROXIMITY:
        if (QWindowsContext::instance()->tabletSupport()->translateTabletProximityEvent(wParam, lParam))
            return 0;
        break;
    case WT_PACKET:
        if (QWindowsContext::instance()->tabletSupport()->translateTabletPacketEvent())
            return 0;
        break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}


// Scale tablet coordinates to screen coordinates.

static inline int sign(int x)
{
    return x >= 0 ? 1 : -1;
}

inline QPointF QWindowsTabletDeviceData::scaleCoordinates(int coordX, int coordY, const QRect &targetArea) const
{
    const int targetX = targetArea.x();
    const int targetY = targetArea.y();
    const int targetWidth = targetArea.width();
    const int targetHeight = targetArea.height();

    const qreal x = sign(targetWidth) == sign(maxX) ?
        ((coordX - minX) * qAbs(targetWidth) / qAbs(qreal(maxX - minX))) + targetX :
        ((qAbs(maxX) - (coordX - minX)) * qAbs(targetWidth) / qAbs(qreal(maxX - minX))) + targetX;

    const qreal y = sign(targetHeight) == sign(maxY) ?
        ((coordY - minY) * qAbs(targetHeight) / qAbs(qreal(maxY - minY))) + targetY :
        ((qAbs(maxY) - (coordY - minY)) * qAbs(targetHeight) / qAbs(qreal(maxY - minY))) + targetY;

    return QPointF(x, y);
}

QWindowsWinTab32DLL QWindowsTabletSupport::m_winTab32DLL;

/*!
    \class QWindowsWinTab32DLL QWindowsTabletSupport
    \brief Functions from wintabl32.dll shipped with WACOM tablets used by QWindowsTabletSupport.

    \internal
    \ingroup qt-lighthouse-win
*/

bool QWindowsWinTab32DLL::init()
{
    if (wTInfo)
        return true;
    QSystemLibrary library(QStringLiteral("wintab32"));
    if (!library.load())
        return false;
    wTOpen = (PtrWTOpen)library.resolve("WTOpenW");
    wTClose = (PtrWTClose)library.resolve("WTClose");
    wTInfo = (PtrWTInfo)library.resolve("WTInfoW");
    wTEnable = (PtrWTEnable)library.resolve("WTEnable");
    wTOverlap = (PtrWTEnable)library.resolve("WTOverlap");
    wTPacketsGet = (PtrWTPacketsGet)library.resolve("WTPacketsGet");
    wTGet = (PtrWTGet)library.resolve("WTGetW");
    wTQueueSizeGet = (PtrWTQueueSizeGet)library.resolve("WTQueueSizeGet");
    wTQueueSizeSet = (PtrWTQueueSizeSet)library.resolve("WTQueueSizeSet");
    return wTOpen && wTClose && wTInfo && wTEnable && wTOverlap && wTPacketsGet && wTQueueSizeGet && wTQueueSizeSet;
}

/*!
    \class QWindowsTabletSupport
    \brief Tablet support for Windows.

    Support for WACOM tablets.

    \sa http://www.wacomeng.com/windows/docs/Wintab_v140.htm

    \internal
    \since 5.2
    \ingroup qt-lighthouse-win
*/

QWindowsTabletSupport::QWindowsTabletSupport(HWND window, HCTX context)
    : m_window(window)
    , m_context(context)
    , m_absoluteRange(20)
    , m_tiltSupport(false)
    , m_currentDevice(-1)
{
     AXIS orientation[3];
    // Some tablets don't support tilt, check if it is possible,
    if (QWindowsTabletSupport::m_winTab32DLL.wTInfo(WTI_DEVICES, DVC_ORIENTATION, &orientation))
        m_tiltSupport = orientation[0].axResolution && orientation[1].axResolution;
}

QWindowsTabletSupport::~QWindowsTabletSupport()
{
    QWindowsTabletSupport::m_winTab32DLL.wTClose(m_context);
    DestroyWindow(m_window);
}

QWindowsTabletSupport *QWindowsTabletSupport::create()
{
    if (!m_winTab32DLL.init())
        return 0;
    const HWND window = QWindowsContext::instance()->createDummyWindow(QStringLiteral("TabletDummyWindow"),
                                                                       L"TabletDummyWindow",
                                                                       qWindowsTabletSupportWndProc);
    if (!window) {
        qCWarning(lcQpaTablet) << __FUNCTION__ << "Unable to create window for tablet.";
        return 0;
    }
    LOGCONTEXT lcMine;
    // build our context from the default context
    QWindowsTabletSupport::m_winTab32DLL.wTInfo(WTI_DEFSYSCTX, 0, &lcMine);
    // Go for the raw coordinates, the tablet event will return good stuff
    lcMine.lcOptions |= CXO_MESSAGES | CXO_CSRMESSAGES;
    lcMine.lcPktData = lcMine.lcMoveMask = PACKETDATA;
    lcMine.lcPktMode = PacketMode;
    lcMine.lcOutOrgX = 0;
    lcMine.lcOutExtX = lcMine.lcInExtX;
    lcMine.lcOutOrgY = 0;
    lcMine.lcOutExtY = -lcMine.lcInExtY;
    const HCTX context = QWindowsTabletSupport::m_winTab32DLL.wTOpen(window, &lcMine, true);
    if (!context) {
        qCDebug(lcQpaTablet) << __FUNCTION__ << "Unable to open tablet.";
        DestroyWindow(window);
        return 0;

    }
    // Set the size of the Packet Queue to the correct size
    const int currentQueueSize = QWindowsTabletSupport::m_winTab32DLL.wTQueueSizeGet(context);
    if (currentQueueSize != TabletPacketQSize) {
        if (!QWindowsTabletSupport::m_winTab32DLL.wTQueueSizeSet(context, TabletPacketQSize)) {
            if (!QWindowsTabletSupport::m_winTab32DLL.wTQueueSizeSet(context, currentQueueSize))  {
                qWarning() << "Unable to set queue size on tablet. The tablet will not work.";
                QWindowsTabletSupport::m_winTab32DLL.wTClose(context);
                DestroyWindow(window);
                return 0;
            } // cannot restore old size
        } // cannot set
    } // mismatch
    qCDebug(lcQpaTablet) << "Opened tablet context " << context << " on window "
        <<  window << "changed packet queue size " << currentQueueSize
        << "->" <<  TabletPacketQSize;
    return new QWindowsTabletSupport(window, context);
}

unsigned QWindowsTabletSupport::options() const
{
    UINT result = 0;
    m_winTab32DLL.wTInfo(WTI_INTERFACE, IFC_CTXOPTIONS, &result);
    return result;
}

QString QWindowsTabletSupport::description() const
{
    const unsigned size = m_winTab32DLL.wTInfo(WTI_INTERFACE, IFC_WINTABID, 0);
    if (!size)
        return QString();
    QScopedPointer<TCHAR> winTabId(new TCHAR[size + 1]);
    m_winTab32DLL.wTInfo(WTI_INTERFACE, IFC_WINTABID, winTabId.data());
    WORD implementationVersion = 0;
    m_winTab32DLL.wTInfo(WTI_INTERFACE, IFC_IMPLVERSION, &implementationVersion);
    WORD specificationVersion = 0;
    m_winTab32DLL.wTInfo(WTI_INTERFACE, IFC_SPECVERSION, &specificationVersion);
    const unsigned opts = options();
    QString result = QString::fromLatin1("%1 specification: v%2.%3 implementation: v%4.%5 options: 0x%6")
        .arg(QString::fromWCharArray(winTabId.data()))
        .arg(specificationVersion >> 8).arg(specificationVersion & 0xFF)
        .arg(implementationVersion >> 8).arg(implementationVersion & 0xFF)
        .arg(opts, 0, 16);
    if (opts & CXO_MESSAGES)
        result += QStringLiteral(" CXO_MESSAGES");
    if (opts & CXO_CSRMESSAGES)
        result += QStringLiteral(" CXO_CSRMESSAGES");
    if (m_tiltSupport)
        result += QStringLiteral(" tilt");
    return result;
}

void QWindowsTabletSupport::notifyActivate()
{
    // Cooperate with other tablet applications, but when we get focus, I want to use the tablet.
    const bool result = QWindowsTabletSupport::m_winTab32DLL.wTEnable(m_context, true)
        && QWindowsTabletSupport::m_winTab32DLL.wTOverlap(m_context, true);
   qCDebug(lcQpaTablet) << __FUNCTION__ << result;
}

static inline int indexOfDevice(const QVector<QWindowsTabletDeviceData> &devices, qint64 uniqueId)
{
    for (int i = 0; i < devices.size(); ++i)
        if (devices.at(i).uniqueId == uniqueId)
            return i;
    return -1;
}

static inline QTabletEvent::TabletDevice deviceType(const UINT cursorType)
{
    if (((cursorType & 0x0006) == 0x0002) && ((cursorType & CursorTypeBitMask) != 0x0902))
        return QTabletEvent::Stylus;
    switch (cursorType & CursorTypeBitMask) {
    case 0x0802:
        return QTabletEvent::Stylus;
    case 0x0902:
        return QTabletEvent::Airbrush;
    case 0x0004:
        return QTabletEvent::FourDMouse;
    case 0x0006:
        return QTabletEvent::Puck;
    case 0x0804:
        return QTabletEvent::RotationStylus;
    default:
        break;
    }
    return QTabletEvent::NoDevice;
}

static inline QTabletEvent::PointerType pointerType(unsigned currentCursor)
{
    switch (currentCursor % 3) { // %3 for dual track
    case 0:
        return QTabletEvent::Cursor;
    case 1:
        return QTabletEvent::Pen;
    case 2:
        return QTabletEvent::Eraser;
    default:
        break;
    }
    return QTabletEvent::UnknownPointer;
}

QDebug operator<<(QDebug d, const QWindowsTabletDeviceData &t)
{
    d << "TabletDevice id:" << t.uniqueId << " pressure: " << t.minPressure
      << ".." << t.maxPressure << " tan pressure: " << t.minTanPressure << ".."
      << t.maxTanPressure << " area:" << t.minX << t.minY <<t.minZ
      << ".." << t.maxX << t.maxY << t.maxZ << " device " << t.currentDevice
      << " pointer " << t.currentPointerType;
    return d;
}

QWindowsTabletDeviceData QWindowsTabletSupport::tabletInit(const quint64 uniqueId, const UINT cursorType) const
{
    QWindowsTabletDeviceData result;
    result.uniqueId = uniqueId;
    /* browse WinTab's many info items to discover pressure handling. */
    AXIS axis;
    LOGCONTEXT lc;
    /* get the current context for its device variable. */
    QWindowsTabletSupport::m_winTab32DLL.wTGet(m_context, &lc);
    /* get the size of the pressure axis. */
    QWindowsTabletSupport::m_winTab32DLL.wTInfo(WTI_DEVICES + lc.lcDevice, DVC_NPRESSURE, &axis);
    result.minPressure = int(axis.axMin);
    result.maxPressure = int(axis.axMax);

    QWindowsTabletSupport::m_winTab32DLL.wTInfo(WTI_DEVICES + lc.lcDevice, DVC_TPRESSURE, &axis);
    result.minTanPressure = int(axis.axMin);
    result.maxTanPressure = int(axis.axMax);

    LOGCONTEXT defaultLc;
    /* get default region */
    QWindowsTabletSupport::m_winTab32DLL.wTInfo(WTI_DEFCONTEXT, 0, &defaultLc);
    result.maxX = int(defaultLc.lcInExtX) - int(defaultLc.lcInOrgX);
    result.maxY = int(defaultLc.lcInExtY) - int(defaultLc.lcInOrgY);
    result.maxZ = int(defaultLc.lcInExtZ) - int(defaultLc.lcInOrgZ);
    result.currentDevice = deviceType(cursorType);
    return result;
}

bool QWindowsTabletSupport::translateTabletProximityEvent(WPARAM /* wParam */, LPARAM lParam)
{
    const bool enteredProximity = LOWORD(lParam) != 0;
    PACKET proximityBuffer[1]; // we are only interested in the first packet in this case
    const int totalPacks = QWindowsTabletSupport::m_winTab32DLL.wTPacketsGet(m_context, 1, proximityBuffer);
    if (!totalPacks)
        return false;
    const UINT currentCursor = proximityBuffer[0].pkCursor;
    UINT physicalCursorId;
    QWindowsTabletSupport::m_winTab32DLL.wTInfo(WTI_CURSORS + currentCursor, CSR_PHYSID, &physicalCursorId);
    UINT cursorType;
    QWindowsTabletSupport::m_winTab32DLL.wTInfo(WTI_CURSORS + currentCursor, CSR_TYPE, &cursorType);
    const qint64 uniqueId = (qint64(cursorType & DeviceIdMask) << 32L) | qint64(physicalCursorId);
    // initializing and updating the cursor should be done in response to
    // WT_CSRCHANGE. We do it in WT_PROXIMITY because some wintab never send
    // the event WT_CSRCHANGE even if asked with CXO_CSRMESSAGES
    m_currentDevice = indexOfDevice(m_devices, uniqueId);
    if (m_currentDevice < 0) {
        m_currentDevice = m_devices.size();
        m_devices.push_back(tabletInit(uniqueId, cursorType));
    }
    m_devices[m_currentDevice].currentPointerType = pointerType(currentCursor);
    qCDebug(lcQpaTablet) << __FUNCTION__ << (enteredProximity ? "enter" : "leave")
        << " proximity for device #" << m_currentDevice << m_devices.at(m_currentDevice);
    if (enteredProximity) {
        QWindowSystemInterface::handleTabletEnterProximityEvent(m_devices.at(m_currentDevice).currentDevice,
                                                                m_devices.at(m_currentDevice).currentPointerType,
                                                                m_devices.at(m_currentDevice).uniqueId);
    } else {
        QWindowSystemInterface::handleTabletLeaveProximityEvent(m_devices.at(m_currentDevice).currentDevice,
                                                                m_devices.at(m_currentDevice).currentPointerType,
                                                                m_devices.at(m_currentDevice).uniqueId);
    }
    return true;
}

bool QWindowsTabletSupport::translateTabletPacketEvent()
{
    static PACKET localPacketBuf[TabletPacketQSize];  // our own tablet packet queue.
    const int packetCount = QWindowsTabletSupport::m_winTab32DLL.wTPacketsGet(m_context, TabletPacketQSize, &localPacketBuf);
    if (!packetCount || m_currentDevice < 0)
        return false;

    const int currentDevice = m_devices.at(m_currentDevice).currentDevice;
    const int currentPointer = m_devices.at(m_currentDevice).currentPointerType;

    // The tablet can be used in 2 different modes, depending on it settings:
    // 1) Absolute (pen) mode:
    //    The coordinates are scaled to the virtual desktop (by default). The user
    //    can also choose to scale to the monitor or a region of the screen.
    //    When entering proximity, the tablet driver snaps the mouse pointer to the
    //    tablet position scaled to that area and keeps it in sync.
    // 2) Relative (mouse) mode:
    //    The pen follows the mouse. The constant 'absoluteRange' specifies the
    //    manhattanLength difference for detecting if a tablet input device is in this mode,
    //    in which case we snap the position to the mouse position.
    // It seems there is no way to find out the mode programmatically, the LOGCONTEXT orgX/Y/Ext
    // area is always the virtual desktop.
    const QRect virtualDesktopArea = QGuiApplication::primaryScreen()->virtualGeometry();

    qCDebug(lcQpaTablet) << __FUNCTION__ << "processing " << packetCount
        << "target:" << QGuiApplicationPrivate::tabletPressTarget;

    const Qt::KeyboardModifiers keyboardModifiers = QWindowsKeyMapper::queryKeyboardModifiers();

    for (int i = 0; i < packetCount ; ++i) {
        const PACKET &packet = localPacketBuf[i];

        const int z = currentDevice == QTabletEvent::FourDMouse ? int(packet.pkZ) : 0;

        // This code is to delay the tablet data one cycle to sync with the mouse location.
        QPointF globalPosF = m_oldGlobalPosF;
        m_oldGlobalPosF = m_devices.at(m_currentDevice).scaleCoordinates(packet.pkX, packet.pkY, virtualDesktopArea);

        QWindow *target = QGuiApplicationPrivate::tabletPressTarget; // Pass to window that grabbed it.
        QPoint globalPos = globalPosF.toPoint();

        // Get Mouse Position and compare to tablet info
        const QPoint mouseLocation = QWindowsCursor::mousePosition();

        // Positions should be almost the same if we are in absolute
        // mode. If they are not, use the mouse location.
        if ((mouseLocation - globalPos).manhattanLength() > m_absoluteRange) {
            globalPos = mouseLocation;
            globalPosF = globalPos;
        }

        if (!target)
            if (QPlatformWindow *pw = QWindowsContext::instance()->findPlatformWindowAt(GetDesktopWindow(), globalPos, CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT))
                target = pw->window();
        if (!target)
            continue;

        const QPoint localPos = target->mapFromGlobal(globalPos);

        const qreal pressureNew = packet.pkButtons && (currentPointer == QTabletEvent::Pen || currentPointer == QTabletEvent::Eraser) ?
            m_devices.at(m_currentDevice).scalePressure(packet.pkNormalPressure) :
            qreal(0);
        const qreal tangentialPressure = currentDevice == QTabletEvent::Airbrush ?
            m_devices.at(m_currentDevice).scaleTangentialPressure(packet.pkTangentPressure) :
            qreal(0);

        int tiltX = 0;
        int tiltY = 0;
        qreal rotation = 0;
        if (m_tiltSupport) {
            // Convert from azimuth and altitude to x tilt and y tilt. What
            // follows is the optimized version. Here are the equations used:
            // X = sin(azimuth) * cos(altitude)
            // Y = cos(azimuth) * cos(altitude)
            // Z = sin(altitude)
            // X Tilt = arctan(X / Z)
            // Y Tilt = arctan(Y / Z)
            const double radAzim = (packet.pkOrientation.orAzimuth / 10) * (M_PI / 180);
            const double tanAlt = tan((abs(packet.pkOrientation.orAltitude / 10)) * (M_PI / 180));

            const double degX = atan(sin(radAzim) / tanAlt);
            const double degY = atan(cos(radAzim) / tanAlt);
            tiltX = int(degX * (180 / M_PI));
            tiltY = int(-degY * (180 / M_PI));
            rotation = packet.pkOrientation.orTwist;
        }

        if (QWindowsContext::verbose > 1)  {
            qCDebug(lcQpaTablet)
                << "Packet #" << i << '/' << packetCount << "button:" << packet.pkButtons
                << globalPosF << z << "to:" << target << localPos << "(packet" << packet.pkX
                << packet.pkY << ") dev:" << currentDevice << "pointer:"
                << currentPointer << "P:" << pressureNew << "tilt:" << tiltX << ','
                << tiltY << "tanP:" << tangentialPressure << "rotation:" << rotation;
        }

        QWindowSystemInterface::handleTabletEvent(target, packet.pkButtons, localPos, globalPosF,
                                                  currentDevice, currentPointer,
                                                  pressureNew, tiltX, tiltY,
                                                  tangentialPressure, rotation, z,
                                                  m_devices.at(m_currentDevice).uniqueId,
                                                  keyboardModifiers);
    }
    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_TABLETEVENT
