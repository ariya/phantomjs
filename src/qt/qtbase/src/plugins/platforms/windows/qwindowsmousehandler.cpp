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

#include "qwindowsmousehandler.h"
#include "qwindowskeymapper.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"
#include "qwindowsintegration.h"
#include "qwindowsscreen.h"

#include <qpa/qwindowsysteminterface.h>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/QWindow>

#include <QtCore/QDebug>
#include <QtCore/QScopedArrayPointer>

#include <windowsx.h>

QT_BEGIN_NAMESPACE

static inline void compressMouseMove(MSG *msg)
{
    // Compress mouse move events
    if (msg->message == WM_MOUSEMOVE) {
        MSG mouseMsg;
        while (PeekMessage(&mouseMsg, msg->hwnd, WM_MOUSEFIRST,
                           WM_MOUSELAST, PM_NOREMOVE)) {
            if (mouseMsg.message == WM_MOUSEMOVE) {
#define PEEKMESSAGE_IS_BROKEN 1
#ifdef PEEKMESSAGE_IS_BROKEN
                // Since the Windows PeekMessage() function doesn't
                // correctly return the wParam for WM_MOUSEMOVE events
                // if there is a key release event in the queue
                // _before_ the mouse event, we have to also consider
                // key release events (kls 2003-05-13):
                MSG keyMsg;
                bool done = false;
                while (PeekMessage(&keyMsg, 0, WM_KEYFIRST, WM_KEYLAST,
                                   PM_NOREMOVE)) {
                    if (keyMsg.time < mouseMsg.time) {
                        if ((keyMsg.lParam & 0xC0000000) == 0x40000000) {
                            PeekMessage(&keyMsg, 0, keyMsg.message,
                                        keyMsg.message, PM_REMOVE);
                        } else {
                            done = true;
                            break;
                        }
                    } else {
                        break; // no key event before the WM_MOUSEMOVE event
                    }
                }
                if (done)
                    break;
#else
                // Actually the following 'if' should work instead of
                // the above key event checking, but apparently
                // PeekMessage() is broken :-(
                if (mouseMsg.wParam != msg.wParam)
                    break; // leave the message in the queue because
                // the key state has changed
#endif
                // Update the passed in MSG structure with the
                // most recent one.
                msg->lParam = mouseMsg.lParam;
                msg->wParam = mouseMsg.wParam;
                // Extract the x,y coordinates from the lParam as we do in the WndProc
                msg->pt.x = GET_X_LPARAM(mouseMsg.lParam);
                msg->pt.y = GET_Y_LPARAM(mouseMsg.lParam);
                ClientToScreen(msg->hwnd, &(msg->pt));
                // Remove the mouse move message
                PeekMessage(&mouseMsg, msg->hwnd, WM_MOUSEMOVE,
                            WM_MOUSEMOVE, PM_REMOVE);
            } else {
                break; // there was no more WM_MOUSEMOVE event
            }
        }
    }
}

/*!
    \class QWindowsMouseHandler
    \brief Windows mouse handler

    Dispatches mouse and touch events. Separate for code cleanliness.

    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsMouseHandler::QWindowsMouseHandler() :
    m_windowUnderMouse(0),
    m_trackedWindow(0),
    m_touchDevice(0),
    m_leftButtonDown(false),
    m_previousCaptureWindow(0)
{
}

Qt::MouseButtons QWindowsMouseHandler::queryMouseButtons()
{
    Qt::MouseButtons result = 0;
    const bool mouseSwapped = GetSystemMetrics(SM_SWAPBUTTON);
    if (GetAsyncKeyState(VK_LBUTTON) < 0)
        result |= mouseSwapped ? Qt::RightButton: Qt::LeftButton;
    if (GetAsyncKeyState(VK_RBUTTON) < 0)
        result |= mouseSwapped ? Qt::LeftButton : Qt::RightButton;
    if (GetAsyncKeyState(VK_MBUTTON) < 0)
        result |= Qt::MidButton;
    if (GetAsyncKeyState(VK_XBUTTON1) < 0)
        result |= Qt::XButton1;
    if (GetAsyncKeyState(VK_XBUTTON2) < 0)
        result |= Qt::XButton2;
    return result;
}

bool QWindowsMouseHandler::translateMouseEvent(QWindow *window, HWND hwnd,
                                               QtWindows::WindowsEventType et,
                                               MSG msg, LRESULT *result)
{
#ifdef Q_COMPILER_CLASS_ENUM
    enum : quint64 { signatureMask = 0xffffff00, miWpSignature = 0xff515700 };
#else
    static const quint64 signatureMask = 0xffffff00;
    static const quint64 miWpSignature = 0xff515700;
#endif // !Q_COMPILER_CLASS_ENUM

    if (et == QtWindows::MouseWheelEvent)
        return translateMouseWheelEvent(window, hwnd, msg, result);

    Qt::MouseEventSource source = Qt::MouseEventNotSynthesized;

#ifndef Q_OS_WINCE
    // Check for events synthesized from touch. Lower byte is touch index, 0 means pen.
    static const bool passSynthesizedMouseEvents =
            !(QWindowsIntegration::instance()->options() & QWindowsIntegration::DontPassOsMouseEventsSynthesizedFromTouch);
    // Check for events synthesized from touch. Lower 7 bits are touch/pen index, bit 8 indicates touch.
    // However, when tablet support is active, extraInfo is a packet serial number. This is not a problem
    // since we do not want to ignore mouse events coming from a tablet.
    const quint64 extraInfo = GetMessageExtraInfo();
    if ((extraInfo & signatureMask) == miWpSignature) {
        source = Qt::MouseEventSynthesizedBySystem;
        const bool fromTouch = extraInfo & 0x80; // (else: Tablet PC)
        if (fromTouch && !passSynthesizedMouseEvents)
            return false;
    }
#endif // !Q_OS_WINCE

    const QPoint winEventPosition(GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
    if (et & QtWindows::NonClientEventFlag) {
        const QPoint globalPosition = winEventPosition;
        const QPoint clientPosition = QWindowsGeometryHint::mapFromGlobal(hwnd, globalPosition);
        const Qt::MouseButtons buttons = QWindowsMouseHandler::queryMouseButtons();
        QWindowSystemInterface::handleFrameStrutMouseEvent(window, clientPosition,
                                                           globalPosition, buttons,
                                                           QWindowsKeyMapper::queryKeyboardModifiers(),
                                                           source);
        return false; // Allow further event processing (dragging of windows).
    }

    *result = 0;
    if (msg.message == WM_MOUSELEAVE) {
        qCDebug(lcQpaEvents) << "WM_MOUSELEAVE for " << window << " previous window under mouse = " << m_windowUnderMouse << " tracked window =" << m_trackedWindow;

        // When moving out of a window, WM_MOUSEMOVE within the moved-to window is received first,
        // so if m_trackedWindow is not the window here, it means the cursor has left the
        // application.
        if (window == m_trackedWindow) {
            QWindow *leaveTarget = m_windowUnderMouse ? m_windowUnderMouse : m_trackedWindow;
            qCDebug(lcQpaEvents) << "Generating leave event for " << leaveTarget;
            QWindowSystemInterface::handleLeaveEvent(leaveTarget);
            m_trackedWindow = 0;
            m_windowUnderMouse = 0;
        }
        return true;
    }

    QWindowsWindow *platformWindow = static_cast<QWindowsWindow *>(window->handle());
    const Qt::MouseButtons buttons = keyStateToMouseButtons((int)msg.wParam);

    // If the window was recently resized via mouse doubleclick on the frame or title bar,
    // we don't get WM_LBUTTONDOWN or WM_LBUTTONDBLCLK for the second click,
    // but we will get at least one WM_MOUSEMOVE with left button down and the WM_LBUTTONUP,
    // which will result undesired mouse press and release events.
    // To avoid those, we ignore any events with left button down if we didn't
    // get the original WM_LBUTTONDOWN/WM_LBUTTONDBLCLK.
    if (msg.message == WM_LBUTTONDOWN || msg.message == WM_LBUTTONDBLCLK) {
        m_leftButtonDown = true;
    } else {
        const bool actualLeftDown = buttons & Qt::LeftButton;
        if (!m_leftButtonDown && actualLeftDown) {
            // Autocapture the mouse for current window to and ignore further events until release.
            // Capture is necessary so we don't get WM_MOUSELEAVEs to confuse matters.
            // This autocapture is released normally when button is released.
            if (!platformWindow->hasMouseCapture()) {
                QWindowsWindow::baseWindowOf(window)->applyCursor();
                platformWindow->setMouseGrabEnabled(true);
                platformWindow->setFlag(QWindowsWindow::AutoMouseCapture);
                qCDebug(lcQpaEvents) << "Automatic mouse capture for missing buttondown event" << window;
            }
            m_previousCaptureWindow = window;
            return true;
        } else if (m_leftButtonDown && !actualLeftDown) {
            m_leftButtonDown = false;
        }
    }

    const QPoint globalPosition = QWindowsGeometryHint::mapToGlobal(hwnd, winEventPosition);
    // In this context, neither an invisible nor a transparent window (transparent regarding mouse
    // events, "click-through") can be considered as the window under mouse.
    QWindow *currentWindowUnderMouse = platformWindow->hasMouseCapture() ?
        QWindowsScreen::windowAt(globalPosition, CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT) : window;

    compressMouseMove(&msg);
    // Qt expects the platform plugin to capture the mouse on
    // any button press until release.
    if (!platformWindow->hasMouseCapture()
        && (msg.message == WM_LBUTTONDOWN || msg.message == WM_MBUTTONDOWN
            || msg.message == WM_RBUTTONDOWN || msg.message == WM_XBUTTONDOWN
            || msg.message == WM_LBUTTONDBLCLK || msg.message == WM_MBUTTONDBLCLK
            || msg.message == WM_RBUTTONDBLCLK || msg.message == WM_XBUTTONDBLCLK)) {
        platformWindow->setMouseGrabEnabled(true);
        platformWindow->setFlag(QWindowsWindow::AutoMouseCapture);
        qCDebug(lcQpaEvents) << "Automatic mouse capture " << window;
        // Implement "Click to focus" for native child windows (unless it is a native widget window).
        if (!window->isTopLevel() && !window->inherits("QWidgetWindow") && QGuiApplication::focusWindow() != window)
            window->requestActivate();
    } else if (platformWindow->hasMouseCapture()
               && platformWindow->testFlag(QWindowsWindow::AutoMouseCapture)
               && (msg.message == WM_LBUTTONUP || msg.message == WM_MBUTTONUP
                   || msg.message == WM_RBUTTONUP || msg.message == WM_XBUTTONUP)
               && !buttons) {
        platformWindow->setMouseGrabEnabled(false);
        qCDebug(lcQpaEvents) << "Releasing automatic mouse capture " << window;
    }

    const bool hasCapture = platformWindow->hasMouseCapture();
    const bool currentNotCapturing = hasCapture && currentWindowUnderMouse != window;
#ifndef Q_OS_WINCE
    // Enter new window: track to generate leave event.
    // If there is an active capture, only track if the current window is capturing,
    // so we don't get extra leave when cursor leaves the application.
    if (window != m_trackedWindow && !currentNotCapturing) {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        tme.dwHoverTime = HOVER_DEFAULT; //
        if (!TrackMouseEvent(&tme))
            qWarning("TrackMouseEvent failed.");
        m_trackedWindow =  window;
    }
#endif // !Q_OS_WINCE

    // No enter or leave events are sent as long as there is an autocapturing window.
    if (!hasCapture || !platformWindow->testFlag(QWindowsWindow::AutoMouseCapture)) {
        // Leave is needed if:
        // 1) There is no capture and we move from a window to another window.
        //    Note: Leaving the application entirely is handled in WM_MOUSELEAVE case.
        // 2) There is capture and we move out of the capturing window.
        // 3) There is a new capture and we were over another window.
        if ((m_windowUnderMouse && m_windowUnderMouse != currentWindowUnderMouse
                && (!hasCapture || window == m_windowUnderMouse))
            || (hasCapture && m_previousCaptureWindow != window && m_windowUnderMouse
                && m_windowUnderMouse != window)) {
            qCDebug(lcQpaEvents) << "Synthetic leave for " << m_windowUnderMouse;
            QWindowSystemInterface::handleLeaveEvent(m_windowUnderMouse);
            if (currentNotCapturing) {
                // Clear tracking if capturing and current window is not the capturing window
                // to avoid leave when mouse actually leaves the application.
                m_trackedWindow = 0;
                // We are not officially in any window, but we need to set some cursor to clear
                // whatever cursor the left window had, so apply the cursor of the capture window.
                QWindowsWindow::baseWindowOf(window)->applyCursor();
            }
        }
        // Enter is needed if:
        // 1) There is no capture and we move to a new window.
        // 2) There is capture and we move into the capturing window.
        // 3) The capture just ended and we are over non-capturing window.
        if ((currentWindowUnderMouse && m_windowUnderMouse != currentWindowUnderMouse
                && (!hasCapture || currentWindowUnderMouse == window))
            || (m_previousCaptureWindow && window != m_previousCaptureWindow && currentWindowUnderMouse
                && currentWindowUnderMouse != m_previousCaptureWindow)) {
            qCDebug(lcQpaEvents) << "Entering " << currentWindowUnderMouse;
            QWindowsWindow::baseWindowOf(currentWindowUnderMouse)->applyCursor();
            QWindowSystemInterface::handleEnterEvent(currentWindowUnderMouse,
                                                     currentWindowUnderMouse->mapFromGlobal(globalPosition),
                                                     globalPosition);
        }
        // We need to track m_windowUnderMouse separately from m_trackedWindow, as
        // Windows mouse tracking will not trigger WM_MOUSELEAVE for leaving window when
        // mouse capture is set.
        m_windowUnderMouse = currentWindowUnderMouse;
    }

    QWindowSystemInterface::handleMouseEvent(window, winEventPosition, globalPosition, buttons,
                                             QWindowsKeyMapper::queryKeyboardModifiers(),
                                             source);
    m_previousCaptureWindow = hasCapture ? window : 0;
    return true;
}

static bool isValidWheelReceiver(QWindow *candidate)
{
    if (candidate) {
        const QWindow *toplevel = QWindowsWindow::topLevelOf(candidate);
        if (const QWindowsWindow *ww = QWindowsWindow::baseWindowOf(toplevel))
            return !ww->testFlag(QWindowsWindow::BlockedByModal);
    }

    return false;
}

bool QWindowsMouseHandler::translateMouseWheelEvent(QWindow *window, HWND,
                                                    MSG msg, LRESULT *)
{
    const Qt::MouseButtons buttons = keyStateToMouseButtons((int)msg.wParam);
    const Qt::KeyboardModifiers mods = keyStateToModifiers((int)msg.wParam);

    int delta;
    if (msg.message == WM_MOUSEWHEEL || msg.message == WM_MOUSEHWHEEL)
        delta = (short) HIWORD (msg.wParam);
    else
        delta = (int) msg.wParam;

    Qt::Orientation orientation = (msg.message == WM_MOUSEHWHEEL
                                  || (buttons & Qt::AltModifier)) ?
                                  Qt::Horizontal : Qt::Vertical;

    // according to the MSDN documentation on WM_MOUSEHWHEEL:
    // a positive value indicates that the wheel was rotated to the right;
    // a negative value indicates that the wheel was rotated to the left.
    // Qt defines this value as the exact opposite, so we have to flip the value!
    if (msg.message == WM_MOUSEHWHEEL)
        delta = -delta;

    // Redirect wheel event to one of the following, in order of preference:
    // 1) The window under mouse
    // 2) The window receiving the event
    // If a window is blocked by modality, it can't get the event.
    const QPoint globalPos(GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
    QWindow *receiver = QWindowsScreen::windowAt(globalPos);
    bool handleEvent = true;
    if (!isValidWheelReceiver(receiver)) {
        receiver = window;
        if (!isValidWheelReceiver(receiver))
            handleEvent = false;
    }

    if (handleEvent) {
        QWindowSystemInterface::handleWheelEvent(receiver,
                                                 QWindowsGeometryHint::mapFromGlobal(receiver, globalPos),
                                                 globalPos,
                                                 delta, orientation, mods);
    }

    return true;
}

// from bool QApplicationPrivate::translateTouchEvent()
bool QWindowsMouseHandler::translateTouchEvent(QWindow *window, HWND,
                                               QtWindows::WindowsEventType,
                                               MSG msg, LRESULT *)
{
#ifndef Q_OS_WINCE
    typedef QWindowSystemInterface::TouchPoint QTouchPoint;
    typedef QList<QWindowSystemInterface::TouchPoint> QTouchPointList;

    const QRect screenGeometry = window->screen()->geometry();

    const int winTouchPointCount = msg.wParam;
    QScopedArrayPointer<TOUCHINPUT> winTouchInputs(new TOUCHINPUT[winTouchPointCount]);
    memset(winTouchInputs.data(), 0, sizeof(TOUCHINPUT) * winTouchPointCount);

    QTouchPointList touchPoints;
    touchPoints.reserve(winTouchPointCount);
    Qt::TouchPointStates allStates = 0;

    Q_ASSERT(QWindowsContext::user32dll.getTouchInputInfo);

    QWindowsContext::user32dll.getTouchInputInfo((HANDLE) msg.lParam, msg.wParam, winTouchInputs.data(), sizeof(TOUCHINPUT));
    for (int i = 0; i < winTouchPointCount; ++i) {
        const TOUCHINPUT &winTouchInput = winTouchInputs[i];
        int id = m_touchInputIDToTouchPointID.value(winTouchInput.dwID, -1);
        if (id == -1) {
            id = m_touchInputIDToTouchPointID.size();
            m_touchInputIDToTouchPointID.insert(winTouchInput.dwID, id);
        }
        QTouchPoint touchPoint;
        touchPoint.pressure = 1.0;
        touchPoint.id = id;
        if (m_lastTouchPositions.contains(id))
            touchPoint.normalPosition = m_lastTouchPositions.value(id);

        QPointF screenPos = QPointF(qreal(winTouchInput.x) / qreal(100.), qreal(winTouchInput.y) / qreal(100.));
        if (winTouchInput.dwMask & TOUCHINPUTMASKF_CONTACTAREA)
            touchPoint.area.setSize(QSizeF(qreal(winTouchInput.cxContact) / qreal(100.),
                                           qreal(winTouchInput.cyContact) / qreal(100.)));
        touchPoint.area.moveCenter(screenPos);
        QPointF normalPosition = QPointF(screenPos.x() / screenGeometry.width(),
                                         screenPos.y() / screenGeometry.height());
        const bool stationaryTouchPoint = (normalPosition == touchPoint.normalPosition);
        touchPoint.normalPosition = normalPosition;

        if (winTouchInput.dwFlags & TOUCHEVENTF_DOWN) {
            touchPoint.state = Qt::TouchPointPressed;
            m_lastTouchPositions.insert(id, touchPoint.normalPosition);
        } else if (winTouchInput.dwFlags & TOUCHEVENTF_UP) {
            touchPoint.state = Qt::TouchPointReleased;
            m_lastTouchPositions.remove(id);
        } else {
            touchPoint.state = (stationaryTouchPoint
                     ? Qt::TouchPointStationary
                     : Qt::TouchPointMoved);
            m_lastTouchPositions.insert(id, touchPoint.normalPosition);
        }

        allStates |= touchPoint.state;

        touchPoints.append(touchPoint);
    }

    QWindowsContext::user32dll.closeTouchInputHandle((HANDLE) msg.lParam);

    // all touch points released, forget the ids we've seen, they may not be reused
    if (allStates == Qt::TouchPointReleased)
        m_touchInputIDToTouchPointID.clear();

    if (!m_touchDevice) {
        m_touchDevice = new QTouchDevice;
        // TODO: Device used to be hardcoded to screen in previous code.
        m_touchDevice->setType(QTouchDevice::TouchScreen);
        m_touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::NormalizedPosition);
        QWindowSystemInterface::registerTouchDevice(m_touchDevice);
    }

    QWindowSystemInterface::handleTouchEvent(window,
                                             m_touchDevice,
                                             touchPoints);
    return true;
#else
    return false;
#endif
}

QT_END_NAMESPACE
