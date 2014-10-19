/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

/****************************************************************************
**
** Copyright (c) 2007-2008, Apple, Inc.
**
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
**   * Redistributions of source code must retain the above copyright notice,
**     this list of conditions and the following disclaimer.
**
**   * Redistributions in binary form must reproduce the above copyright notice,
**     this list of conditions and the following disclaimer in the documentation
**     and/or other materials provided with the distribution.
**
**   * Neither the name of Apple, Inc. nor the names of its contributors
**     may be used to endorse or promote products derived from this software
**     without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

#include <private/qcore_mac_p.h>
#include <qaction.h>
#include <qwidget.h>
#include <qdesktopwidget.h>
#include <qevent.h>
#include <qpixmapcache.h>
#include <qvarlengtharray.h>
#include <private/qevent_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#include <private/qt_mac_p.h>
#include <private/qapplication_p.h>
#include <private/qcocoaapplication_mac_p.h>
#include <private/qcocoawindow_mac_p.h>
#include <private/qcocoaview_mac_p.h>
#include <private/qkeymapper_p.h>
#include <private/qwidget_p.h>
#include <private/qcocoawindow_mac_p.h>

QT_BEGIN_NAMESPACE

#ifdef QT_MAC_USE_COCOA
// Cmd + left mousebutton should produce a right button
//  press (mainly for mac users with one-button mice):
static bool qt_leftButtonIsRightButton = false;
#endif

Q_GLOBAL_STATIC(QMacWindowFader, macwindowFader);

QMacWindowFader::QMacWindowFader()
    : m_duration(0.250)
{
}

QMacWindowFader *QMacWindowFader::currentFader()
{
    return macwindowFader();
}

void QMacWindowFader::registerWindowToFade(QWidget *window)
{
    m_windowsToFade.append(window);
}

void QMacWindowFader::performFade()
{
    const QWidgetList myWidgetsToFade = m_windowsToFade;
    const int widgetCount = myWidgetsToFade.count();
#if QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext] setDuration:NSTimeInterval(m_duration)];
#endif

    for (int i = 0; i < widgetCount; ++i) {
        QWidget *widget = m_windowsToFade.at(i);
        OSWindowRef window = qt_mac_window_for(widget);
#if QT_MAC_USE_COCOA
        [[window animator] setAlphaValue:0.0];
        QTimer::singleShot(qRound(m_duration * 1000), widget, SLOT(hide()));
#else
        TransitionWindowOptions options = {0, m_duration, 0, 0};
        TransitionWindowWithOptions(window, kWindowFadeTransitionEffect, kWindowHideTransitionAction,
                                    0, 1, &options);
#endif
    }
#if QT_MAC_USE_COCOA
    [NSAnimationContext endGrouping];
#endif
    m_duration = 0.250;
    m_windowsToFade.clear();
}

extern bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event); // qapplication.cpp;
extern QWidget * mac_mouse_grabber;
extern QWidget *qt_button_down; //qapplication_mac.cpp
extern QPointer<QWidget> qt_last_mouse_receiver;
extern OSViewRef qt_mac_effectiveview_for(const QWidget *w);
extern void qt_mac_updateCursorWithWidgetUnderMouse(QWidget *widgetUnderMouse); // qcursor_mac.mm

void macWindowFade(void * /*OSWindowRef*/ window, float durationSeconds)
{
#ifdef QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
#endif
    OSWindowRef wnd = static_cast<OSWindowRef>(window);
    if (wnd) {
        QWidget *widget;
#if QT_MAC_USE_COCOA
        widget = [wnd QT_MANGLE_NAMESPACE(qt_qwidget)];
#else
    const UInt32 kWidgetCreatorQt = kEventClassQt;
    enum {
        kWidgetPropertyQWidget = 'QWId' //QWidget *
    };
        if (GetWindowProperty(static_cast<WindowRef>(window), kWidgetCreatorQt, kWidgetPropertyQWidget, sizeof(widget), 0, &widget) != noErr)
            widget = 0;
#endif
        if (widget) {
            QMacWindowFader::currentFader()->setFadeDuration(durationSeconds);
            QMacWindowFader::currentFader()->registerWindowToFade(widget);
            QMacWindowFader::currentFader()->performFade();
        }
    }
}
struct dndenum_mapper
{
    NSDragOperation mac_code;
    Qt::DropAction qt_code;
    bool Qt2Mac;
};

#if defined(QT_MAC_USE_COCOA) && defined(__OBJC__)

static dndenum_mapper dnd_enums[] = {
    { NSDragOperationLink,  Qt::LinkAction, true },
    { NSDragOperationMove,  Qt::MoveAction, true },
    { NSDragOperationCopy,  Qt::CopyAction, true },
    { NSDragOperationGeneric,  Qt::CopyAction, false },
    { NSDragOperationEvery, Qt::ActionMask, false },
    { NSDragOperationNone, Qt::IgnoreAction, false }
};

NSDragOperation qt_mac_mapDropAction(Qt::DropAction action)
{
    for (int i=0; dnd_enums[i].qt_code; i++) {
        if (dnd_enums[i].Qt2Mac && (action & dnd_enums[i].qt_code)) {
            return dnd_enums[i].mac_code;
        }
    }
    return NSDragOperationNone;
}

NSDragOperation qt_mac_mapDropActions(Qt::DropActions actions)
{
    NSDragOperation nsActions = NSDragOperationNone;
    for (int i=0; dnd_enums[i].qt_code; i++) {
        if (dnd_enums[i].Qt2Mac && (actions & dnd_enums[i].qt_code))
            nsActions |= dnd_enums[i].mac_code;
    }
    return nsActions;
}

Qt::DropAction qt_mac_mapNSDragOperation(NSDragOperation nsActions)
{
    Qt::DropAction action = Qt::IgnoreAction;
    for (int i=0; dnd_enums[i].mac_code; i++) {
        if (nsActions & dnd_enums[i].mac_code)
            return dnd_enums[i].qt_code;
    }
    return action;
}

Qt::DropActions qt_mac_mapNSDragOperations(NSDragOperation nsActions)
{
    Qt::DropActions actions = Qt::IgnoreAction;
    for (int i=0; dnd_enums[i].mac_code; i++) {
        if (nsActions & dnd_enums[i].mac_code)
            actions |= dnd_enums[i].qt_code;
    }
    return actions;
}

Q_GLOBAL_STATIC(DnDParams, currentDnDParameters);
DnDParams *macCurrentDnDParameters()
{
    return currentDnDParameters();
}
#endif

bool macWindowIsTextured( void * /*OSWindowRef*/ window )
{
    OSWindowRef wnd = static_cast<OSWindowRef>(window);
#if QT_MAC_USE_COCOA
	return ( [wnd styleMask] & NSTexturedBackgroundWindowMask ) ? true : false;
#else
	WindowAttributes currentAttributes;
	GetWindowAttributes(wnd, &currentAttributes);
	return (currentAttributes & kWindowMetalAttribute) ? true : false;
#endif
}

void macWindowToolbarShow(const QWidget *widget, bool show )
{
    OSWindowRef wnd = qt_mac_window_for(widget);
#if QT_MAC_USE_COCOA
    if (NSToolbar *toolbar = [wnd toolbar]) {
        QMacCocoaAutoReleasePool pool;
        if (show != [toolbar isVisible]) {
           [toolbar setVisible:show];
        } else {
            // The toolbar may be in sync, but we are not, update our framestrut.
            qt_widget_private(const_cast<QWidget *>(widget))->updateFrameStrut();
        }
    }
#else
    qt_widget_private(const_cast<QWidget *>(widget))->updateFrameStrut();
    ShowHideWindowToolbar(wnd, show, false);
#endif
}


void macWindowToolbarSet( void * /*OSWindowRef*/ window, void *toolbarRef  )
{
    OSWindowRef wnd = static_cast<OSWindowRef>(window);
#if QT_MAC_USE_COCOA
    [wnd setToolbar:static_cast<NSToolbar *>(toolbarRef)];
#else
    SetWindowToolbar(wnd, static_cast<HIToolbarRef>(toolbarRef));
#endif
}

bool macWindowToolbarIsVisible( void * /*OSWindowRef*/ window )
{
    OSWindowRef wnd = static_cast<OSWindowRef>(window);
#if QT_MAC_USE_COCOA
    if (NSToolbar *toolbar = [wnd toolbar])
        return [toolbar isVisible];
    return false;
#else
    return IsWindowToolbarVisible(wnd);
#endif
}

void macWindowSetHasShadow( void * /*OSWindowRef*/ window, bool hasShadow  )
{
    OSWindowRef wnd = static_cast<OSWindowRef>(window);
#if QT_MAC_USE_COCOA
    [wnd setHasShadow:BOOL(hasShadow)];
#else
    if (hasShadow)
        ChangeWindowAttributes(wnd, 0, kWindowNoShadowAttribute);
    else
        ChangeWindowAttributes(wnd, kWindowNoShadowAttribute, 0);
#endif
}

void macWindowFlush(void * /*OSWindowRef*/ window)
{
    OSWindowRef wnd = static_cast<OSWindowRef>(window);
#if QT_MAC_USE_COCOA
    [wnd flushWindowIfNeeded];
#else
    HIWindowFlush(wnd);
#endif
}

void * /*NSImage */qt_mac_create_nsimage(const QPixmap &pm)
{
    QMacCocoaAutoReleasePool pool;
    if(QCFType<CGImageRef> image = pm.toMacCGImageRef()) {
        NSImage *newImage = 0;
        NSRect imageRect = NSMakeRect(0.0, 0.0, CGImageGetWidth(image), CGImageGetHeight(image));
        newImage = [[NSImage alloc] initWithSize:imageRect.size];
        [newImage lockFocus];
        {
            CGContextRef imageContext = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
            CGContextDrawImage(imageContext, *(CGRect*)&imageRect, image);
        }
        [newImage unlockFocus];
        return newImage;
    }
    return 0;
}

void qt_mac_update_mouseTracking(QWidget *widget)
{
#ifdef QT_MAC_USE_COCOA
    [qt_mac_nativeview_for(widget) updateTrackingAreas];
#else
    Q_UNUSED(widget);
#endif
}

OSStatus qt_mac_drawCGImage(CGContextRef inContext, const CGRect *inBounds, CGImageRef inImage)
{
    // Verbatim copy if HIViewDrawCGImage (as shown on Carbon-Dev)
    OSStatus err = noErr;

    require_action(inContext != NULL, InvalidContext, err = paramErr);
    require_action(inBounds != NULL, InvalidBounds, err = paramErr);
    require_action(inImage != NULL, InvalidImage, err = paramErr);

    CGContextSaveGState( inContext );
    CGContextTranslateCTM (inContext, 0, inBounds->origin.y + CGRectGetMaxY(*inBounds));
    CGContextScaleCTM(inContext, 1, -1);

    CGContextDrawImage(inContext, *inBounds, inImage);

    CGContextRestoreGState(inContext);
InvalidImage:
InvalidBounds:
InvalidContext:
	return err;
}

bool qt_mac_checkForNativeSizeGrip(const QWidget *widget)
{
#ifndef QT_MAC_USE_COCOA
    OSViewRef nativeSizeGrip = 0;
    HIViewFindByID(HIViewGetRoot(HIViewGetWindow(HIViewRef(widget->winId()))), kHIViewWindowGrowBoxID, &nativeSizeGrip);
    return (nativeSizeGrip != 0);
#else
    return [[reinterpret_cast<NSView *>(widget->effectiveWinId()) window] showsResizeIndicator];
#endif
}
struct qt_mac_enum_mapper
{
    int mac_code;
    int qt_code;
#if defined(DEBUG_MOUSE_MAPS)
#   define QT_MAC_MAP_ENUM(x) x, #x
    const char *desc;
#else
#   define QT_MAC_MAP_ENUM(x) x
#endif
};

//mouse buttons
static qt_mac_enum_mapper qt_mac_mouse_symbols[] = {
{ kEventMouseButtonPrimary, QT_MAC_MAP_ENUM(Qt::LeftButton) },
{ kEventMouseButtonSecondary, QT_MAC_MAP_ENUM(Qt::RightButton) },
{ kEventMouseButtonTertiary, QT_MAC_MAP_ENUM(Qt::MidButton) },
{ 4, QT_MAC_MAP_ENUM(Qt::XButton1) },
{ 5, QT_MAC_MAP_ENUM(Qt::XButton2) },
{ 0, QT_MAC_MAP_ENUM(0) }
};
Qt::MouseButtons qt_mac_get_buttons(int buttons)
{
#ifdef DEBUG_MOUSE_MAPS
    qDebug("Qt: internal: **Mapping buttons: %d (0x%04x)", buttons, buttons);
#endif
    Qt::MouseButtons ret = Qt::NoButton;
    for(int i = 0; qt_mac_mouse_symbols[i].qt_code; i++) {
        if (buttons & (0x01<<(qt_mac_mouse_symbols[i].mac_code-1))) {
#ifdef DEBUG_MOUSE_MAPS
            qDebug("Qt: internal: got button: %s", qt_mac_mouse_symbols[i].desc);
#endif
            ret |= Qt::MouseButtons(qt_mac_mouse_symbols[i].qt_code);
        }
    }
    return ret;
}
Qt::MouseButton qt_mac_get_button(EventMouseButton button)
{
#ifdef DEBUG_MOUSE_MAPS
    qDebug("Qt: internal: **Mapping button: %d (0x%04x)", button, button);
#endif
    Qt::MouseButtons ret = 0;
    for(int i = 0; qt_mac_mouse_symbols[i].qt_code; i++) {
        if (button == qt_mac_mouse_symbols[i].mac_code) {
#ifdef DEBUG_MOUSE_MAPS
            qDebug("Qt: internal: got button: %s", qt_mac_mouse_symbols[i].desc);
#endif
            return Qt::MouseButton(qt_mac_mouse_symbols[i].qt_code);
        }
    }
    return Qt::NoButton;
}

void macSendToolbarChangeEvent(QWidget *widget)
{
    QToolBarChangeEvent ev(!(GetCurrentKeyModifiers() & cmdKey));
    qt_sendSpontaneousEvent(widget, &ev);
}

Q_GLOBAL_STATIC(QMacTabletHash, tablet_hash)
QMacTabletHash *qt_mac_tablet_hash()
{
    return tablet_hash();
}

#ifdef QT_MAC_USE_COCOA

// Clears the QWidget pointer that each QCocoaView holds.
void qt_mac_clearCocoaViewQWidgetPointers(QWidget *widget)
{
    QT_MANGLE_NAMESPACE(QCocoaView) *cocoaView = reinterpret_cast<QT_MANGLE_NAMESPACE(QCocoaView) *>(qt_mac_nativeview_for(widget));
    if (cocoaView && [cocoaView respondsToSelector:@selector(qt_qwidget)]) {
        [cocoaView qt_clearQWidget];
    }
}

void qt_dispatchTabletProximityEvent(void * /*NSEvent * */ tabletEvent)
{
    NSEvent *proximityEvent = static_cast<NSEvent *>(tabletEvent);
    // simply construct a Carbon proximity record and handle it all in one spot.
    TabletProximityRec carbonProximityRec = { [proximityEvent vendorID],
                                              [proximityEvent tabletID],
                                              [proximityEvent pointingDeviceID],
                                              [proximityEvent deviceID],
                                              [proximityEvent systemTabletID],
                                              [proximityEvent vendorPointingDeviceType],
                                              [proximityEvent pointingDeviceSerialNumber],
                                              [proximityEvent uniqueID],
                                              [proximityEvent capabilityMask],
                                              [proximityEvent pointingDeviceType],
                                              [proximityEvent isEnteringProximity] };
    qt_dispatchTabletProximityEvent(carbonProximityRec);
}
#endif // QT_MAC_USE_COCOA

void qt_dispatchTabletProximityEvent(const ::TabletProximityRec &proxRec)
{
    QTabletDeviceData proximityDevice;
    proximityDevice.tabletUniqueID = proxRec.uniqueID;
    proximityDevice.capabilityMask = proxRec.capabilityMask;

    switch (proxRec.pointerType) {
        case NSUnknownPointingDevice:
        default:
            proximityDevice.tabletPointerType = QTabletEvent::UnknownPointer;
            break;
        case NSPenPointingDevice:
            proximityDevice.tabletPointerType = QTabletEvent::Pen;
            break;
        case NSCursorPointingDevice:
            proximityDevice.tabletPointerType = QTabletEvent::Cursor;
            break;
        case NSEraserPointingDevice:
            proximityDevice.tabletPointerType = QTabletEvent::Eraser;
            break;
    }
    uint bits = proxRec.vendorPointerType;
    if (bits == 0 && proximityDevice.tabletUniqueID != 0) {
        // Fallback. It seems that the driver doesn't always include all the information.
        // High-End Wacom devices store their "type" in the uper bits of the Unique ID.
        // I'm not sure how to handle it for consumer devices, but I'll test that in a bit.
        bits = proximityDevice.tabletUniqueID >> 32;
    }
    // Defined in the "EN0056-NxtGenImpGuideX"
    // on Wacom's Developer Website (www.wacomeng.com)
    if (((bits & 0x0006) == 0x0002) && ((bits & 0x0F06) != 0x0902)) {
        proximityDevice.tabletDeviceType = QTabletEvent::Stylus;
    } else {
        switch (bits & 0x0F06) {
            case 0x0802:
                proximityDevice.tabletDeviceType = QTabletEvent::Stylus;
                break;
            case 0x0902:
                proximityDevice.tabletDeviceType = QTabletEvent::Airbrush;
                break;
            case 0x0004:
                proximityDevice.tabletDeviceType = QTabletEvent::FourDMouse;
                break;
            case 0x0006:
                proximityDevice.tabletDeviceType = QTabletEvent::Puck;
                break;
            case 0x0804:
                proximityDevice.tabletDeviceType = QTabletEvent::RotationStylus;
                break;
            default:
                proximityDevice.tabletDeviceType = QTabletEvent::NoDevice;
        }
    }
    // The deviceID is "unique" while in the proximity, it's a key that we can use for
    // linking up TabletDeviceData to an event (especially if there are two devices in action).
    bool entering = proxRec.enterProximity;
    if (entering) {
        qt_mac_tablet_hash()->insert(proxRec.deviceID, proximityDevice);
    } else {
        qt_mac_tablet_hash()->remove(proxRec.deviceID);
    }

    QTabletEvent qtabletProximity(entering ? QEvent::TabletEnterProximity
                                  : QEvent::TabletLeaveProximity,
                                  QPoint(), QPoint(), QPointF(), proximityDevice.tabletDeviceType,
                                  proximityDevice.tabletPointerType, 0., 0, 0, 0., 0., 0, 0,
                                  proximityDevice.tabletUniqueID);

    qt_sendSpontaneousEvent(qApp, &qtabletProximity);
}

// Use this method to keep all the information in the TextSegment. As long as it is ordered
// we are in OK shape, and we can influence that ourselves.
struct KeyPair
{
    QChar cocoaKey;
    Qt::Key qtKey;
};

bool operator==(const KeyPair &entry, QChar qchar)
{
    return entry.cocoaKey == qchar;
}

bool operator<(const KeyPair &entry, QChar qchar)
{
    return entry.cocoaKey < qchar;
}

bool operator<(QChar qchar, const KeyPair &entry)
{
    return qchar < entry.cocoaKey;
}

bool operator<(const Qt::Key &key, const KeyPair &entry)
{
    return key < entry.qtKey;
}

bool operator<(const KeyPair &entry, const Qt::Key &key)
{
    return entry.qtKey < key;
}

static bool qtKey2CocoaKeySortLessThan(const KeyPair &entry1, const KeyPair &entry2)
{
    return entry1.qtKey < entry2.qtKey;
}

static const int NumEntries = 59;
static const KeyPair entries[NumEntries] = {
    { NSEnterCharacter, Qt::Key_Enter },
    { NSBackspaceCharacter, Qt::Key_Backspace },
    { NSTabCharacter, Qt::Key_Tab },
    { NSNewlineCharacter, Qt::Key_Return },
    { NSCarriageReturnCharacter, Qt::Key_Return },
    { NSBackTabCharacter, Qt::Key_Backtab },
    { kEscapeCharCode, Qt::Key_Escape },
    // Cocoa sends us delete when pressing backspace!
    // (NB when we reverse this list in qtKey2CocoaKey, there
    // will be two indices of Qt::Key_Backspace. But is seems to work
    // ok for menu shortcuts (which uses that function):
    { NSDeleteCharacter, Qt::Key_Backspace },
    { NSUpArrowFunctionKey, Qt::Key_Up },
    { NSDownArrowFunctionKey, Qt::Key_Down },
    { NSLeftArrowFunctionKey, Qt::Key_Left },
    { NSRightArrowFunctionKey, Qt::Key_Right },
    { NSF1FunctionKey, Qt::Key_F1 },
    { NSF2FunctionKey, Qt::Key_F2 },
    { NSF3FunctionKey, Qt::Key_F3 },
    { NSF4FunctionKey, Qt::Key_F4 },
    { NSF5FunctionKey, Qt::Key_F5 },
    { NSF6FunctionKey, Qt::Key_F6 },
    { NSF7FunctionKey, Qt::Key_F7 },
    { NSF8FunctionKey, Qt::Key_F8 },
    { NSF9FunctionKey, Qt::Key_F8 },
    { NSF10FunctionKey, Qt::Key_F10 },
    { NSF11FunctionKey, Qt::Key_F11 },
    { NSF12FunctionKey, Qt::Key_F12 },
    { NSF13FunctionKey, Qt::Key_F13 },
    { NSF14FunctionKey, Qt::Key_F14 },
    { NSF15FunctionKey, Qt::Key_F15 },
    { NSF16FunctionKey, Qt::Key_F16 },
    { NSF17FunctionKey, Qt::Key_F17 },
    { NSF18FunctionKey, Qt::Key_F18 },
    { NSF19FunctionKey, Qt::Key_F19 },
    { NSF20FunctionKey, Qt::Key_F20 },
    { NSF21FunctionKey, Qt::Key_F21 },
    { NSF22FunctionKey, Qt::Key_F22 },
    { NSF23FunctionKey, Qt::Key_F23 },
    { NSF24FunctionKey, Qt::Key_F24 },
    { NSF25FunctionKey, Qt::Key_F25 },
    { NSF26FunctionKey, Qt::Key_F26 },
    { NSF27FunctionKey, Qt::Key_F27 },
    { NSF28FunctionKey, Qt::Key_F28 },
    { NSF29FunctionKey, Qt::Key_F29 },
    { NSF30FunctionKey, Qt::Key_F30 },
    { NSF31FunctionKey, Qt::Key_F31 },
    { NSF32FunctionKey, Qt::Key_F32 },
    { NSF33FunctionKey, Qt::Key_F33 },
    { NSF34FunctionKey, Qt::Key_F34 },
    { NSF35FunctionKey, Qt::Key_F35 },
    { NSInsertFunctionKey, Qt::Key_Insert },
    { NSDeleteFunctionKey, Qt::Key_Delete },
    { NSHomeFunctionKey, Qt::Key_Home },
    { NSEndFunctionKey, Qt::Key_End },
    { NSPageUpFunctionKey, Qt::Key_PageUp },
    { NSPageDownFunctionKey, Qt::Key_PageDown },
    { NSPrintScreenFunctionKey, Qt::Key_Print },
    { NSScrollLockFunctionKey, Qt::Key_ScrollLock },
    { NSPauseFunctionKey, Qt::Key_Pause },
    { NSSysReqFunctionKey, Qt::Key_SysReq },
    { NSMenuFunctionKey, Qt::Key_Menu },
    { NSHelpFunctionKey, Qt::Key_Help },
};
static const KeyPair * const end = entries + NumEntries;

QChar qtKey2CocoaKey(Qt::Key key)
{
    // The first time this function is called, create a reverse
    // looup table sorted on Qt Key rather than Cocoa key:
    static QVector<KeyPair> rev_entries(NumEntries);
    static bool mustInit = true;
    if (mustInit){
        mustInit = false;
        for (int i=0; i<NumEntries; ++i)
            rev_entries[i] = entries[i];
        qSort(rev_entries.begin(), rev_entries.end(), qtKey2CocoaKeySortLessThan);
    }
    const QVector<KeyPair>::iterator i
            = qBinaryFind(rev_entries.begin(), rev_entries.end(), key);
    if (i == rev_entries.end())
        return QChar();
    return i->cocoaKey;
}

#ifdef QT_MAC_USE_COCOA
static Qt::Key cocoaKey2QtKey(QChar keyCode)
{
    const KeyPair *i = qBinaryFind(entries, end, keyCode);
    if (i == end)
        return Qt::Key(keyCode.unicode());
    return i->qtKey;
}

Qt::KeyboardModifiers qt_cocoaModifiers2QtModifiers(ulong modifierFlags)
{
    Qt::KeyboardModifiers qtMods =Qt::NoModifier;
    if (modifierFlags &  NSShiftKeyMask)
        qtMods |= Qt::ShiftModifier;
    if (modifierFlags & NSControlKeyMask)
        qtMods |= Qt::MetaModifier;
    if (modifierFlags & NSAlternateKeyMask)
        qtMods |= Qt::AltModifier;
    if (modifierFlags & NSCommandKeyMask)
        qtMods |= Qt::ControlModifier;
    if (modifierFlags & NSNumericPadKeyMask)
        qtMods |= Qt::KeypadModifier;
    return qtMods;
}

NSString *qt_mac_removePrivateUnicode(NSString* string)
{
    int len = [string length];
    if (len) {
        QVarLengthArray <unichar, 10> characters(len);
        bool changed = false;
        for (int i = 0; i<len; i++) {
            characters[i] = [string characterAtIndex:i];
            // check if they belong to key codes in private unicode range
            // currently we need to handle only the NSDeleteFunctionKey
            if (characters[i] == NSDeleteFunctionKey) {
                characters[i] = NSDeleteCharacter;
                changed = true;
            }
        }
        if (changed)
            return [NSString stringWithCharacters:characters.data() length:len];
    }
    return string;
}

Qt::KeyboardModifiers qt_cocoaDragOperation2QtModifiers(uint dragOperations)
{
    Qt::KeyboardModifiers qtMods =Qt::NoModifier;
    if (dragOperations &  NSDragOperationLink)
        qtMods |= Qt::MetaModifier;
    if (dragOperations & NSDragOperationGeneric)
        qtMods |= Qt::ControlModifier;
    if (dragOperations & NSDragOperationCopy)
        qtMods |= Qt::AltModifier;
    return qtMods;
}

static inline QEvent::Type cocoaEvent2QtEvent(NSUInteger eventType)
{
    // Handle the trivial cases that can be determined from the type.
    switch (eventType) {
    case NSKeyDown:
        return QEvent::KeyPress;
    case NSKeyUp:
        return QEvent::KeyRelease;
    case NSLeftMouseDown:
    case NSRightMouseDown:
    case NSOtherMouseDown:
        return QEvent::MouseButtonPress;
    case NSLeftMouseUp:
    case NSRightMouseUp:
    case NSOtherMouseUp:
        return QEvent::MouseButtonRelease;
    case NSMouseMoved:
    case NSLeftMouseDragged:
    case NSRightMouseDragged:
    case NSOtherMouseDragged:
        return QEvent::MouseMove;
    case NSScrollWheel:
        return QEvent::Wheel;
    }
    return QEvent::None;
}

static bool mustUseCocoaKeyEvent()
{
    QCFType<TISInputSourceRef> source = TISCopyCurrentKeyboardInputSource();
    return TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData) == 0;
}

bool qt_dispatchKeyEventWithCocoa(void * /*NSEvent * */ keyEvent, QWidget *widgetToGetEvent)
{
    NSEvent *event = static_cast<NSEvent *>(keyEvent);
    NSString *keyChars = [event charactersIgnoringModifiers];
    int keyLength = [keyChars length];
    if (keyLength == 0)
        return false; // Dead Key, nothing to do!
    bool ignoreText = false;
    Qt::Key qtKey = Qt::Key_unknown;
    if (keyLength == 1) {
        QChar ch([keyChars characterAtIndex:0]);
        if (ch.isLower())
            ch = ch.toUpper();
        qtKey = cocoaKey2QtKey(ch);
        // Do not set the text for Function-Key Unicodes characters (0xF700–0xF8FF).
        ignoreText = (ch.unicode() >= 0xF700 && ch.unicode() <= 0xF8FF);
    }
    Qt::KeyboardModifiers keyMods = qt_cocoaModifiers2QtModifiers([event modifierFlags]);
    QString text;

    // To quote from the Carbon port: This is actually wrong--but it is the best that
    // can be done for now because of the Control/Meta mapping issues
    // (we always get text on the Mac)
    if (!ignoreText && !(keyMods & (Qt::ControlModifier | Qt::MetaModifier)))
        text = QCFString::toQString(reinterpret_cast<CFStringRef>(keyChars));

    UInt32 macScanCode = 1;
    QKeyEventEx ke(cocoaEvent2QtEvent([event type]), qtKey, keyMods, text, [event isARepeat], qMax(1, keyLength),
                   macScanCode, [event keyCode], [event modifierFlags]);
    return qt_sendSpontaneousEvent(widgetToGetEvent, &ke) && ke.isAccepted();
}
#endif

Qt::MouseButton cocoaButton2QtButton(NSInteger buttonNum)
{
    if (buttonNum == 0)
        return Qt::LeftButton;
    if (buttonNum == 1)
        return Qt::RightButton;
    if (buttonNum == 2)
        return Qt::MidButton;
    if (buttonNum == 3)
        return Qt::XButton1;
    if (buttonNum == 4)
        return Qt::XButton2;
    return Qt::NoButton;
}

bool qt_dispatchKeyEvent(void * /*NSEvent * */ keyEvent, QWidget *widgetToGetEvent)
{
#ifndef QT_MAC_USE_COCOA
    Q_UNUSED(keyEvent);
    Q_UNUSED(widgetToGetEvent);
    return false;
#else
    NSEvent *event = static_cast<NSEvent *>(keyEvent);
    EventRef key_event = static_cast<EventRef>(const_cast<void *>([event eventRef]));
    Q_ASSERT(key_event);
    unsigned int info = 0;

    if ([event type] == NSKeyDown) {
        NSString *characters = [event characters];
        if ([characters length]) {
            unichar value = [characters characterAtIndex:0];
            qt_keymapper_private()->updateKeyMap(0, key_event, (void *)&value);
            info = value;
        }
    }

    if (qt_mac_sendMacEventToWidget(widgetToGetEvent, key_event))
        return true;

    if (mustUseCocoaKeyEvent())
        return qt_dispatchKeyEventWithCocoa(keyEvent, widgetToGetEvent);

    bool consumed = qt_keymapper_private()->translateKeyEvent(widgetToGetEvent, 0, key_event, &info, true);
    return consumed && (info != 0);
#endif
}

void qt_dispatchModifiersChanged(void * /*NSEvent * */flagsChangedEvent, QWidget *widgetToGetEvent)
{
#ifndef QT_MAC_USE_COCOA
    Q_UNUSED(flagsChangedEvent);
    Q_UNUSED(widgetToGetEvent);
#else
    UInt32 modifiers = 0;
    // Sync modifiers with Qt
    NSEvent *event = static_cast<NSEvent *>(flagsChangedEvent);
    EventRef key_event = static_cast<EventRef>(const_cast<void *>([event eventRef]));
    Q_ASSERT(key_event);
    GetEventParameter(key_event, kEventParamKeyModifiers, typeUInt32, 0,
                      sizeof(modifiers), 0, &modifiers);
    extern void qt_mac_send_modifiers_changed(quint32 modifiers, QObject *object);
    qt_mac_send_modifiers_changed(modifiers, widgetToGetEvent);
#endif
}

QPointF flipPoint(const NSPoint &p)
{
    return QPointF(p.x, flipYCoordinate(p.y));
}

NSPoint flipPoint(const QPoint &p)
{
    return NSMakePoint(p.x(), flipYCoordinate(p.y()));
}

NSPoint flipPoint(const QPointF &p)
{
    return NSMakePoint(p.x(), flipYCoordinate(p.y()));
}

#if QT_MAC_USE_COCOA && __OBJC__

void qt_mac_handleNonClientAreaMouseEvent(NSWindow *window, NSEvent *event)
{
    QWidget *widgetToGetEvent = [window QT_MANGLE_NAMESPACE(qt_qwidget)];
    if (widgetToGetEvent == 0)
        return;

    NSEventType evtType = [event type];
    QPoint qlocalPoint;
    QPoint qglobalPoint;
    bool processThisEvent = false;
    bool fakeNCEvents = false;
    bool fakeMouseEvents = false;

    // Check if this is a mouse event.
    if (evtType == NSLeftMouseDown || evtType == NSLeftMouseUp
        || evtType == NSRightMouseDown || evtType == NSRightMouseUp
        || evtType == NSOtherMouseDown || evtType == NSOtherMouseUp
        || evtType == NSMouseMoved     || evtType == NSLeftMouseDragged
        || evtType == NSRightMouseDragged || evtType == NSOtherMouseDragged) {
        // Check if we want to pass this message to another window
        if (mac_mouse_grabber  && mac_mouse_grabber != widgetToGetEvent) {
            NSWindow *grabWindow = static_cast<NSWindow *>(qt_mac_window_for(mac_mouse_grabber));
            if (window != grabWindow) {
                window = grabWindow;
                widgetToGetEvent = mac_mouse_grabber;
                fakeNCEvents = true;
            }
        }
        // Dont generate normal NC mouse events for Left Button dragged
        if(evtType != NSLeftMouseDragged || fakeNCEvents) {
            NSPoint windowPoint = [event locationInWindow];
            NSPoint globalPoint = [[event window] convertBaseToScreen:windowPoint];
            NSRect frameRect = [window frame];
            if (fakeNCEvents || NSMouseInRect(globalPoint, frameRect, NO)) {
                NSRect contentRect = [window contentRectForFrameRect:frameRect];
                qglobalPoint = QPoint(flipPoint(globalPoint).toPoint());
                QWidget *w = widgetToGetEvent->childAt(widgetToGetEvent->mapFromGlobal(qglobalPoint));
                // check that the mouse pointer is on the non-client area and
                // there are not widgets in it.
                if (fakeNCEvents || (!NSMouseInRect(globalPoint, contentRect, NO) && !w)) {
                    qglobalPoint = QPoint(flipPoint(globalPoint).toPoint());
                    qlocalPoint = widgetToGetEvent->mapFromGlobal(qglobalPoint);
                    processThisEvent = true;
                }
            }
        }
    }
    // This is not an NC area mouse message.
    if (!processThisEvent)
        return;

    // If the window is frame less, generate fake mouse events instead. (floating QToolBar)
    // or if someone already got an explicit or implicit grab
    if (mac_mouse_grabber || qt_button_down ||
        (fakeNCEvents && (widgetToGetEvent->window()->windowFlags() & Qt::FramelessWindowHint)))
        fakeMouseEvents = true;

    Qt::MouseButton button;
    QEvent::Type eventType;
    // Convert to Qt::Event type
    switch (evtType) {
        case NSLeftMouseDown:
            button = Qt::LeftButton;
            eventType = (!fakeMouseEvents) ? QEvent::NonClientAreaMouseButtonPress
                                           : QEvent::MouseButtonPress;
            break;
        case NSLeftMouseUp:
            button = Qt::LeftButton;
            eventType = (!fakeMouseEvents) ? QEvent::NonClientAreaMouseButtonRelease
                                           : QEvent::MouseButtonRelease;
            break;
        case NSRightMouseDown:
            button = Qt::RightButton;
            eventType = (!fakeMouseEvents) ? QEvent::NonClientAreaMouseButtonPress
                                           : QEvent::MouseButtonPress;
            break;
        case NSRightMouseUp:
            button = Qt::RightButton;
            eventType = (!fakeMouseEvents) ? QEvent::NonClientAreaMouseButtonRelease
                                           : QEvent::MouseButtonRelease;
            break;
        case NSOtherMouseDown:
            button = cocoaButton2QtButton([event buttonNumber]);
            eventType = (!fakeMouseEvents) ? QEvent::NonClientAreaMouseButtonPress
                                           : QEvent::MouseButtonPress;
            break;
        case NSOtherMouseUp:
            button = cocoaButton2QtButton([event buttonNumber]);
            eventType = (!fakeMouseEvents) ? QEvent::NonClientAreaMouseButtonRelease
                                           : QEvent::MouseButtonRelease;
            break;
        case NSMouseMoved:
            button = Qt::NoButton;
            eventType = (!fakeMouseEvents) ? QEvent::NonClientAreaMouseMove
                                           : QEvent::MouseMove;
            break;
        case NSLeftMouseDragged:
            button = Qt::LeftButton;
            eventType = (!fakeMouseEvents) ? QEvent::NonClientAreaMouseMove
                                           : QEvent::MouseMove;
            break;
        case NSRightMouseDragged:
            button = Qt::RightButton;
            eventType = (!fakeMouseEvents) ? QEvent::NonClientAreaMouseMove
                                           : QEvent::MouseMove;
            break;
        case NSOtherMouseDragged:
            button = cocoaButton2QtButton([event buttonNumber]);
            eventType = (!fakeMouseEvents) ? QEvent::NonClientAreaMouseMove
                                           : QEvent::MouseMove;
            break;
        default:
            qWarning("not handled! Non client area mouse message");
            return;
    }

    Qt::KeyboardModifiers keyMods = qt_cocoaModifiers2QtModifiers([event modifierFlags]);
    if (eventType == QEvent::NonClientAreaMouseButtonPress || eventType == QEvent::MouseButtonPress) {
        NSInteger clickCount = [event clickCount];
        if (clickCount % 2 == 0)
            eventType = (!fakeMouseEvents) ? QEvent::NonClientAreaMouseButtonDblClick
                                           : QEvent::MouseButtonDblClick;
        if (button == Qt::LeftButton && (keyMods & Qt::MetaModifier)) {
            button = Qt::RightButton;
            qt_leftButtonIsRightButton = true;
        }
    } else if (eventType == QEvent::NonClientAreaMouseButtonRelease || eventType == QEvent::MouseButtonRelease) {
        if (button == Qt::LeftButton && qt_leftButtonIsRightButton) {
            button = Qt::RightButton;
            qt_leftButtonIsRightButton = false;
        }
    }

    Qt::MouseButtons buttons = 0;
    {
        UInt32 mac_buttons;
        if (GetEventParameter((EventRef)[event eventRef], kEventParamMouseChord, typeUInt32, 0,
                              sizeof(mac_buttons), 0, &mac_buttons) == noErr)
            buttons = qt_mac_get_buttons(mac_buttons);
    }

    QMouseEvent qme(eventType, qlocalPoint, qglobalPoint, button, buttons, keyMods);
    qt_sendSpontaneousEvent(widgetToGetEvent, &qme);

    // We don't need to set the implicit grab widget here because we won't
    // reach this point if then event type is Press over a Qt widget.
    // However we might need to unset it if the event is Release.
    if (eventType == QEvent::MouseButtonRelease)
        qt_button_down = 0;
}

QWidget *qt_mac_getTargetForKeyEvent(QWidget *widgetThatReceivedEvent)
{
    if (QWidget *popup = QApplication::activePopupWidget()) {
        QWidget *focusInPopup = popup->focusWidget();
        return focusInPopup ? focusInPopup : popup;
    }

    QWidget *widgetToGetKey = qApp->focusWidget();
    if (!widgetToGetKey)
        widgetToGetKey = widgetThatReceivedEvent;

    return widgetToGetKey;
}

// This function will find the widget that should receive the
// mouse event. Because of explicit/implicit mouse grabs, popups,
// etc, this might not end up being the same as the widget under
// the mouse (which is more interresting when handling enter/leave
// events
QWidget *qt_mac_getTargetForMouseEvent(
    // You can call this function without providing an event.
    NSEvent *event,
    QEvent::Type eventType,
    QPoint &returnLocalPoint,
    QPoint &returnGlobalPoint,
    QWidget *nativeWidget,
    QWidget **returnWidgetUnderMouse)
{
    Q_UNUSED(event);
    NSPoint nsglobalpoint = event ? [[event window] convertBaseToScreen:[event locationInWindow]] : [NSEvent mouseLocation];
    QPointF globalPointF = flipPoint(nsglobalpoint);
    // Always truncate to convert to integer, same as Cocoa does for clicks
    returnGlobalPoint = QPoint(globalPointF.x(), globalPointF.y());
    QWidget *mouseGrabber = QWidget::mouseGrabber();
    bool buttonDownNotBlockedByModal = qt_button_down && !QApplicationPrivate::isBlockedByModal(qt_button_down);
    QWidget *popup = QApplication::activePopupWidget();

    // Resolve the widget under the mouse:
    QWidget *widgetUnderMouse = 0;
    if (popup || qt_button_down || !nativeWidget || !nativeWidget->isVisible()) {
        // Using QApplication::widgetAt for finding the widget under the mouse
        // is most safe, since it ignores cocoas own mouse down redirections (which
        // we need to be prepared for when using nativeWidget as starting point).
        // (the only exception is for QMacNativeWidget, where QApplication::widgetAt fails).
        // But it is also slower (I guess), so we try to avoid it and use nativeWidget if we can:
        widgetUnderMouse = QApplication::widgetAt(returnGlobalPoint);
    }

    if (!widgetUnderMouse && nativeWidget) {
        // Entering here should be the common case. We
        // also handle the QMacNativeWidget fallback case.
        QPoint p = nativeWidget->mapFromGlobal(returnGlobalPoint);
        widgetUnderMouse = nativeWidget->childAt(p);
        if (!widgetUnderMouse && nativeWidget->rect().contains(p))
            widgetUnderMouse = nativeWidget;
    }

    if (widgetUnderMouse) {
        // Check if widgetUnderMouse is blocked by a modal
        // window, or the mouse if over the frame strut:
        if (widgetUnderMouse == qt_button_down) {
            // Small optimization to avoid an extra call to isBlockedByModal:
            if (buttonDownNotBlockedByModal == false)
                widgetUnderMouse = 0;
        } else if (QApplicationPrivate::isBlockedByModal(widgetUnderMouse)) {
            widgetUnderMouse = 0;
        }

        if (widgetUnderMouse && widgetUnderMouse->isWindow()) {
            // Exclude the titlebar (and frame strut) when finding widget under mouse:
            QPoint p = widgetUnderMouse->mapFromGlobal(returnGlobalPoint);
            if (!widgetUnderMouse->rect().contains(p))
                widgetUnderMouse = 0;
        }
    }
    if (returnWidgetUnderMouse)
        *returnWidgetUnderMouse = widgetUnderMouse;

    // Resolve the target for the mouse event. Default will be
    // widgetUnderMouse, except if there is a grab (popup/mouse/button-down):
    if (popup && !mouseGrabber) {
        // We special case handling of popups, since they have an implicitt mouse grab.
        QWidget *candidate = buttonDownNotBlockedByModal ? qt_button_down : widgetUnderMouse;
        if (!popup->isAncestorOf(candidate)) {
            // INVARIANT: we have a popup, but the candidate is not
            // in it. But the popup will grab the mouse anyway,
            // except if the user scrolls:
            if (eventType == QEvent::Wheel)
                return 0;
            returnLocalPoint = popup->mapFromGlobal(returnGlobalPoint);
            return popup;
        } else if (popup == candidate) {
            // INVARIANT: The candidate is the popup itself, and not a child:
            returnLocalPoint = popup->mapFromGlobal(returnGlobalPoint);
            return popup;
        } else {
            // INVARIANT: The candidate is a child inside the popup:
            returnLocalPoint = candidate->mapFromGlobal(returnGlobalPoint);
            return candidate;
        }
    }

    QWidget *target = mouseGrabber;
    if (!target && buttonDownNotBlockedByModal)
        target = qt_button_down;
    if (!target)
        target = widgetUnderMouse;
    if (!target)
        return 0;

    returnLocalPoint = target->mapFromGlobal(returnGlobalPoint);
    return target;
}

QPointer<QWidget> qt_last_native_mouse_receiver = 0;

static inline void qt_mac_checkEnterLeaveForNativeWidgets(QWidget *maybeEnterWidget)
{
    // Dispatch enter/leave for the cases where QApplicationPrivate::sendMouseEvent do
    // not. This will in general be the cases when alien widgets are not involved:
    // 1. from a native widget to another native widget or
    // 2. from a native widget to no widget
    // 3. from no widget to a native or alien widget

    if (qt_button_down || QWidget::mouseGrabber())
        return;

    if ((maybeEnterWidget == qt_last_native_mouse_receiver) && qt_last_native_mouse_receiver)
        return;
    if (maybeEnterWidget) {
        if (!qt_last_native_mouse_receiver) {
            // case 3
            QApplicationPrivate::dispatchEnterLeave(maybeEnterWidget, 0);
            qt_last_native_mouse_receiver = maybeEnterWidget->internalWinId() ? maybeEnterWidget : maybeEnterWidget->nativeParentWidget();
        } else if (maybeEnterWidget->internalWinId()) {
            // case 1
            QApplicationPrivate::dispatchEnterLeave(maybeEnterWidget, qt_last_native_mouse_receiver);
            qt_last_native_mouse_receiver = maybeEnterWidget->internalWinId() ? maybeEnterWidget : maybeEnterWidget->nativeParentWidget();
        } // else at lest one of the widgets are alien, so enter/leave will be handled in QApplicationPrivate
    } else {
        if (qt_last_native_mouse_receiver) {
            // case 2
            QApplicationPrivate::dispatchEnterLeave(0, qt_last_native_mouse_receiver);
            qt_last_mouse_receiver = 0;
            qt_last_native_mouse_receiver = 0;
        }
    }
}

bool qt_mac_handleMouseEvent(NSEvent *event, QEvent::Type eventType, Qt::MouseButton button, QWidget *nativeWidget, bool fakeEvent)
{
    // Give the Input Manager a chance to process the mouse events.
    NSInputManager *currentIManager = [NSInputManager currentInputManager];
    if (currentIManager && [currentIManager wantsToHandleMouseEvents]) {
        [currentIManager handleMouseEvent:event];
    }

    // Find the widget that should receive the event, and the widget under the mouse. Those
    // can differ if an implicit or explicit mouse grab is active:
    QWidget *widgetUnderMouse = 0;
    QPoint localPoint, globalPoint;
    QWidget *widgetToGetMouse = qt_mac_getTargetForMouseEvent(event, eventType, localPoint, globalPoint, nativeWidget, &widgetUnderMouse);
    if (!widgetToGetMouse)
        return false;

    // From here on, we let nativeWidget actually be the native widget under widgetUnderMouse. The reason
    // for this, is that qt_mac_getTargetForMouseEvent will set cocoa's mouse event redirection aside when
    // determining which widget is under the mouse (in other words, it will usually ignore nativeWidget).
    // nativeWidget will be used in QApplicationPrivate::sendMouseEvent to correctly dispatch enter/leave events.
    if (widgetUnderMouse)
        nativeWidget = widgetUnderMouse->internalWinId() ? widgetUnderMouse : widgetUnderMouse->nativeParentWidget();
    if (!nativeWidget)
        return false;
    NSView *view = qt_mac_effectiveview_for(nativeWidget);

    // Handle tablet events (if any) first.
    if (qt_mac_handleTabletEvent(view, event)) {
        // Tablet event was handled. In Qt we aren't supposed to send the mouse event.
        return true;
    }

    EventRef carbonEvent = static_cast<EventRef>(const_cast<void *>([event eventRef]));
    if (qt_mac_sendMacEventToWidget(widgetToGetMouse, carbonEvent))
        return true;

    // Keep previousButton to make sure we don't send double click
    // events when the user double clicks using two different buttons:
    static Qt::MouseButton previousButton = Qt::NoButton;

    Qt::KeyboardModifiers keyMods = qt_cocoaModifiers2QtModifiers([event modifierFlags]);
    NSInteger clickCount = [event clickCount];
    Qt::MouseButtons buttons = 0;
    {
        UInt32 mac_buttons;
        if (GetEventParameter(carbonEvent, kEventParamMouseChord, typeUInt32, 0,
                              sizeof(mac_buttons), 0, &mac_buttons) == noErr)
            buttons = qt_mac_get_buttons(mac_buttons);
        if (fakeEvent && buttons == 0)
            buttons = qt_mac_get_buttons(QApplication::mouseButtons());
    }

    // Send enter/leave events for the cases when QApplicationPrivate::sendMouseEvent do not:
    qt_mac_checkEnterLeaveForNativeWidgets(widgetUnderMouse);

    switch (eventType) {
    default:
        qWarning("not handled! %d", eventType);
        break;
    case QEvent::MouseMove:
        if (button == Qt::LeftButton && qt_leftButtonIsRightButton)
            button = Qt::RightButton;
        break;
    case QEvent::MouseButtonPress:
        qt_button_down = widgetUnderMouse;
        if (clickCount % 2 == 0 && (previousButton == Qt::NoButton || previousButton == button))
            eventType = QEvent::MouseButtonDblClick;
        if (button == Qt::LeftButton && (keyMods & Qt::MetaModifier)) {
            button = Qt::RightButton;
            qt_leftButtonIsRightButton = true;
        }
        break;
    case QEvent::MouseButtonRelease:
        if (button == Qt::LeftButton && qt_leftButtonIsRightButton) {
            button = Qt::RightButton;
            qt_leftButtonIsRightButton = false;
        }
        qt_button_down = 0;
        break;
    }

    qt_mac_updateCursorWithWidgetUnderMouse(widgetUnderMouse);

    DnDParams *dndParams = currentDnDParameters();
    dndParams->view = view;
    dndParams->theEvent = event;
    dndParams->globalPoint = globalPoint;

    // Send the mouse event:
    QMouseEvent qme(eventType, localPoint, globalPoint, button, buttons, keyMods);
    QApplicationPrivate::sendMouseEvent(
                widgetToGetMouse, &qme, widgetUnderMouse, nativeWidget,
                &qt_button_down, qt_last_mouse_receiver, true);

    if (eventType == QEvent::MouseButtonPress && button == Qt::RightButton) {
        QContextMenuEvent qcme(QContextMenuEvent::Mouse, localPoint, globalPoint, keyMods);
        qt_sendSpontaneousEvent(widgetToGetMouse, &qcme);
    }

    if (eventType == QEvent::MouseButtonRelease) {
        // A mouse button was released, which means that the implicit grab was
        // released. We therefore need to re-check if should send (delayed) enter leave events:
        // qt_button_down has now become NULL since the call at the top of the function. Also, since
        // the relase might have closed a window, we dont give the nativeWidget hint
        qt_mac_getTargetForMouseEvent(0, QEvent::None, localPoint, globalPoint, nativeWidget, &widgetUnderMouse);
        qt_mac_checkEnterLeaveForNativeWidgets(widgetUnderMouse);
    }

    previousButton = button;
    return true;
}
#endif

bool qt_mac_handleTabletEvent(void * /*QCocoaView * */view, void * /*NSEvent * */tabletEvent)
{
#ifndef QT_MAC_USE_COCOA
    Q_UNUSED(view);
    Q_UNUSED(tabletEvent);
    return false;
#else
    QT_MANGLE_NAMESPACE(QCocoaView) *theView = static_cast<QT_MANGLE_NAMESPACE(QCocoaView) *>(view);
    NSView *theNSView = static_cast<NSView *>(view);
    NSEvent *theTabletEvent = static_cast<NSEvent *>(tabletEvent);

    NSEventType eventType = [theTabletEvent type];
    if (eventType != NSTabletPoint && [theTabletEvent subtype] != NSTabletPointEventSubtype)
        return false; // Not a tablet event.

    NSPoint windowPoint = [theTabletEvent locationInWindow];
    NSPoint globalPoint = [[theTabletEvent window] convertBaseToScreen:windowPoint];

    QWidget *qwidget = [theView qt_qwidget];
    QWidget *widgetToGetMouse = qwidget;
    QWidget *popup = qAppInstance()->activePopupWidget();
    if (popup && popup != qwidget->window())
        widgetToGetMouse = popup;

    if (qt_mac_sendMacEventToWidget(widgetToGetMouse,
                                static_cast<EventRef>(const_cast<void *>([theTabletEvent eventRef]))))
        return true;
    if (widgetToGetMouse != qwidget) {
        theNSView = qt_mac_nativeview_for(widgetToGetMouse);
        windowPoint = [[theNSView window] convertScreenToBase:globalPoint];
    }
    NSPoint localPoint = [theNSView convertPoint:windowPoint fromView:nil];
    // Tablet events do not handle WA_TransparentForMouseEvents ATM
    // In theory, people who set the WA_TransparentForMouseEvents attribute won't handle
    // tablet events either in which case they will fall into the mouse event case and get
    // them passed on. This will NOT handle the raw events, but that might not be a big problem.

    const QMacTabletHash *tabletHash = qt_mac_tablet_hash();
    if (!tabletHash->contains([theTabletEvent deviceID])) {
        qWarning("QCocoaView handleTabletEvent: This tablet device is unknown"
                 " (received no proximity event for it). Discarding event.");
        return false;
    }
    const QTabletDeviceData &deviceData = tabletHash->value([theTabletEvent deviceID]);


    QEvent::Type qType;
    switch (eventType) {
    case NSLeftMouseDown:
    case NSRightMouseDown:
        qType = QEvent::TabletPress;
        break;
    case NSLeftMouseUp:
    case NSRightMouseUp:
        qType = QEvent::TabletRelease;
        break;
    case NSMouseMoved:
    case NSTabletPoint:
    case NSLeftMouseDragged:
    case NSRightMouseDragged:
    default:
        qType = QEvent::TabletMove;
        break;
    }

    qreal pressure;
    if (eventType != NSMouseMoved) {
        pressure = [theTabletEvent pressure];
    } else {
        pressure = 0.0;
    }

    NSPoint tilt = [theTabletEvent tilt];
    int xTilt = qRound(tilt.x * 60.0);
    int yTilt = qRound(tilt.y * -60.0);
    qreal tangentialPressure = 0;
    qreal rotation = 0;
    int z = 0;
    if (deviceData.capabilityMask & 0x0200)
        z = [theTabletEvent absoluteZ];

    if (deviceData.capabilityMask & 0x0800)
        tangentialPressure = [theTabletEvent tangentialPressure];

    rotation = [theTabletEvent rotation];
    QPointF hiRes = flipPoint(globalPoint);
    QTabletEvent qtabletEvent(qType, QPoint(localPoint.x, localPoint.y),
                              hiRes.toPoint(), hiRes,
                              deviceData.tabletDeviceType, deviceData.tabletPointerType,
                              pressure, xTilt, yTilt, tangentialPressure, rotation, z,
                              qt_cocoaModifiers2QtModifiers([theTabletEvent modifierFlags]),
                              deviceData.tabletUniqueID);

    qt_sendSpontaneousEvent(widgetToGetMouse, &qtabletEvent);
    return qtabletEvent.isAccepted();
#endif
}

void qt_mac_updateContentBorderMetricts(void * /*OSWindowRef */window, const ::HIContentBorderMetrics &metrics)
{
    OSWindowRef theWindow = static_cast<OSWindowRef>(window);
#if !defined(QT_MAC_USE_COCOA)
#  if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5) {
        ::HIWindowSetContentBorderThickness(theWindow, &metrics);
    }
#  else
    Q_UNUSED(window);
    Q_UNUSED(metrics);
#  endif
#else
    if ([theWindow styleMask] & NSTexturedBackgroundWindowMask)
        [theWindow setContentBorderThickness:metrics.top forEdge:NSMaxYEdge];
    [theWindow setContentBorderThickness:metrics.bottom forEdge:NSMinYEdge];
#endif
}

#if QT_MAC_USE_COCOA
void qt_mac_replaceDrawRect(void * /*OSWindowRef */window, QWidgetPrivate *widget)
{
    QMacCocoaAutoReleasePool pool;
    OSWindowRef theWindow = static_cast<OSWindowRef>(window);
    if(!theWindow)
        return;
    id theClass = [[[theWindow contentView] superview] class];
    // What we do here is basically to add a new selector to NSThemeFrame called
    // "drawRectOriginal:" which will contain the original implementation of
    // "drawRect:". After that we get the new implementation from QCocoaWindow
    // and exchange them. The new implementation is called drawRectSpecial.
    // We cannot just add the method because it might have been added before and since
    // we cannot remove a method once it has been added we need to ask QCocoaWindow if
    // we did the swap or not.
    if(!widget->drawRectOriginalAdded) {
        Method m2 = class_getInstanceMethod(theClass, @selector(drawRect:));
        if(!m2) {
            // This case is pretty extreme, no drawRect means no drawing!
            return;
        }
        class_addMethod(theClass, @selector(drawRectOriginal:), method_getImplementation(m2), method_getTypeEncoding(m2));
        widget->drawRectOriginalAdded = true;
    }
    if(widget->originalDrawMethod) {
        Method m0 = class_getInstanceMethod([theWindow class], @selector(drawRectSpecial:));
        if(!m0) {
            // Ok, this means the methods were never swapped. Just ignore
            return;
        }
        Method m1 = class_getInstanceMethod(theClass, @selector(drawRect:));
        if(!m1) {
            // Ok, this means the methods were never swapped. Just ignore
            return;
        }
        // We have the original method here. Proceed and swap the methods.
        method_exchangeImplementations(m1, m0);
        widget->originalDrawMethod = false;
        [theWindow display];
    }
}

void qt_mac_replaceDrawRectOriginal(void * /*OSWindowRef */window, QWidgetPrivate *widget)
{
    QMacCocoaAutoReleasePool pool;
    OSWindowRef theWindow = static_cast<OSWindowRef>(window);
    id theClass = [[[theWindow contentView] superview] class];
    // Now we need to revert the methods to their original state.
    // We cannot remove the method, so we just keep track of it in QCocoaWindow.
    Method m0 = class_getInstanceMethod([theWindow class], @selector(drawRectSpecial:));
    if(!m0) {
        // Ok, this means the methods were never swapped. Just ignore
        return;
    }
    Method m1 = class_getInstanceMethod(theClass, @selector(drawRect:));
    if(!m1) {
        // Ok, this means the methods were never swapped. Just ignore
        return;
    }
    method_exchangeImplementations(m1, m0);
    widget->originalDrawMethod = true;
    [theWindow display];
}
#endif // QT_MAC_USE_COCOA

#if QT_MAC_USE_COCOA
void qt_mac_showBaseLineSeparator(void * /*OSWindowRef */window, bool show)
{
    if(!window)
        return;
    QMacCocoaAutoReleasePool pool;
    OSWindowRef theWindow = static_cast<OSWindowRef>(window);
    NSToolbar *macToolbar = [theWindow toolbar];
    [macToolbar setShowsBaselineSeparator:show];
}
#endif // QT_MAC_USE_COCOA

QStringList qt_mac_NSArrayToQStringList(void *nsarray)
{
    QStringList result;
    NSArray *array = static_cast<NSArray *>(nsarray);
    for (NSUInteger i=0; i<[array count]; ++i)
        result << qt_mac_NSStringToQString([array objectAtIndex:i]);
    return result;
}

void *qt_mac_QStringListToNSMutableArrayVoid(const QStringList &list)
{
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:list.size()];
    for (int i=0; i<list.size(); ++i){
        [result addObject:reinterpret_cast<const NSString *>(QCFString::toCFStringRef(list[i]))];
    }
    return result;
}

#if QT_MAC_USE_COCOA
void qt_syncCocoaTitleBarButtons(OSWindowRef window, QWidget *widgetForWindow)
{
    if (!widgetForWindow)
        return;

    Qt::WindowFlags flags = widgetForWindow->windowFlags();
    bool customize = flags & Qt::CustomizeWindowHint;

    NSButton *btn = [window standardWindowButton:NSWindowZoomButton];
    // BOOL is not an int, so the bitwise AND doesn't work.
    bool go = uint(customize && !(flags & Qt::WindowMaximizeButtonHint)) == 0;
    [btn setEnabled:go];

    btn = [window standardWindowButton:NSWindowMiniaturizeButton];
    go = uint(customize && !(flags & Qt::WindowMinimizeButtonHint)) == 0;
    [btn setEnabled:go];

    btn = [window standardWindowButton:NSWindowCloseButton];
    go = uint(customize && !(flags & Qt::WindowSystemMenuHint
                             || flags & Qt::WindowCloseButtonHint)) == 0;
    [btn setEnabled:go];

    [window setShowsToolbarButton:uint(flags & Qt::MacWindowToolBarButtonHint) != 0];
}
#endif // QT_MAC_USE_COCOA

// Carbon: Make sure you call QDEndContext on the context when done with it.
CGContextRef qt_mac_graphicsContextFor(QWidget *widget)
{
    if (!widget)
        return 0;

#ifndef QT_MAC_USE_COCOA
    CGContextRef context;
    CGrafPtr port = GetWindowPort(qt_mac_window_for(widget));
    QDBeginCGContext(port, &context);
#else
    CGContextRef context = (CGContextRef)[[NSGraphicsContext graphicsContextWithWindow:qt_mac_window_for(widget)] graphicsPort];
#endif
    return context;
}

void qt_mac_dispatchPendingUpdateRequests(QWidget *widget)
{
    if (!widget)
        return;
#ifndef QT_MAC_USE_COCOA
    HIViewRender(qt_mac_nativeview_for(widget));
#else
    [qt_mac_nativeview_for(widget) displayIfNeeded];
#endif
}

CGFloat qt_mac_get_scalefactor()
{
#ifndef QT_MAC_USE_COCOA
    return HIGetScaleFactor();
#endif

#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7)
    NSScreen *mainScreen = [NSScreen mainScreen];
    if ([mainScreen respondsToSelector:@selector(backingScaleFactor)])
        return [mainScreen backingScaleFactor];
#endif
    return 1.0;
}

QString qt_mac_get_pasteboardString(OSPasteboardRef paste)
{
    QMacCocoaAutoReleasePool pool;
    NSPasteboard *pb = nil;
    CFStringRef pbname;
    if (PasteboardCopyName(paste, &pbname) == noErr) {
        pb = [NSPasteboard pasteboardWithName:const_cast<NSString *>(reinterpret_cast<const NSString *>(pbname))];
        CFRelease(pbname);
    } else {
        pb = [NSPasteboard generalPasteboard];
    }
    if (pb) {
        NSString *text = [pb stringForType:NSStringPboardType];
        if (text)
            return qt_mac_NSStringToQString(text);
    }
    return QString();
}

QPixmap qt_mac_convert_iconref(const IconRef icon, int width, int height)
{
    QPixmap ret(width, height);
    ret.fill(QColor(0, 0, 0, 0));

    CGRect rect = CGRectMake(0, 0, width, height);

    CGContextRef ctx = qt_mac_cg_context(&ret);
    CGAffineTransform old_xform = CGContextGetCTM(ctx);
    CGContextConcatCTM(ctx, CGAffineTransformInvert(old_xform));
    CGContextConcatCTM(ctx, CGAffineTransformIdentity);

    ::RGBColor b;
    b.blue = b.green = b.red = 255*255;
    PlotIconRefInContext(ctx, &rect, kAlignNone, kTransformNone, &b, kPlotIconRefNormalFlags, icon);
    CGContextRelease(ctx);
    return ret;
}

void qt_mac_constructQIconFromIconRef(const IconRef icon, const IconRef overlayIcon, QIcon *retIcon, QStyle::StandardPixmap standardIcon)
{
    int size = 16;
    while (size <= 128) {

        const QString cacheKey = QLatin1String("qt_mac_constructQIconFromIconRef") + QString::number(standardIcon) + QString::number(size);
        QPixmap mainIcon;
        if (standardIcon >= QStyle::SP_CustomBase) {
            mainIcon = qt_mac_convert_iconref(icon, size, size);
        } else if (QPixmapCache::find(cacheKey, mainIcon) == false) {
            mainIcon = qt_mac_convert_iconref(icon, size, size);
            QPixmapCache::insert(cacheKey, mainIcon);
        }

        if (overlayIcon) {
            int littleSize = size / 2;
            QPixmap overlayPix = qt_mac_convert_iconref(overlayIcon, littleSize, littleSize);
            QPainter painter(&mainIcon);
            painter.drawPixmap(size - littleSize, size - littleSize, overlayPix);
        }

        retIcon->addPixmap(mainIcon);
        size += size;  // 16 -> 32 -> 64 -> 128
    }
}

#ifdef QT_MAC_USE_COCOA
void qt_mac_menu_collapseSeparators(void */*NSMenu **/ theMenu, bool collapse)
{
    QMacCocoaAutoReleasePool pool;
    OSMenuRef menu = static_cast<OSMenuRef>(theMenu);
    if (collapse) {
        bool previousIsSeparator = true; // setting to true kills all the separators placed at the top.
        NSMenuItem *previousItem = nil;
            
        NSArray *itemArray = [menu itemArray];
        for (unsigned int i = 0; i < [itemArray count]; ++i) {
            NSMenuItem *item = reinterpret_cast<NSMenuItem *>([itemArray objectAtIndex:i]);
            if ([item isSeparatorItem]) {
                [item setHidden:previousIsSeparator];
            }

            if (![item isHidden]) {
                previousItem = item;
                previousIsSeparator = ([previousItem isSeparatorItem]);
            }
        }

        // We now need to check the final item since we don't want any separators at the end of the list.
        if (previousItem && previousIsSeparator)
            [previousItem setHidden:YES];
    } else {
        NSArray *itemArray = [menu itemArray];
        for (unsigned int i = 0; i < [itemArray count]; ++i) {
            NSMenuItem *item = reinterpret_cast<NSMenuItem *>([itemArray objectAtIndex:i]);
            if (QAction *action = reinterpret_cast<QAction *>([item tag]))
                [item setHidden:!action->isVisible()];
        }
    }
}

class CocoaPostMessageAfterEventLoopExitHelp : public QObject
{
    id target;
    SEL selector;
    int argCount;
    id arg1;
    id arg2;
public:
    CocoaPostMessageAfterEventLoopExitHelp(id target, SEL selector, int argCount, id arg1, id arg2)
        : target(target), selector(selector), argCount(argCount), arg1(arg1), arg2(arg2){
        deleteLater();
    }

    ~CocoaPostMessageAfterEventLoopExitHelp()
    {
        qt_cocoaPostMessage(target, selector, argCount, arg1, arg2);
    }
};

void qt_cocoaPostMessage(id target, SEL selector, int argCount, id arg1, id arg2)
{
    // WARNING: data1 and data2 is truncated to from 64-bit to 32-bit on OS 10.5!
    // That is why we need to split the address in two parts:
    QCocoaPostMessageArgs *args = new QCocoaPostMessageArgs(target, selector, argCount, arg1, arg2);
    quint32 lower = quintptr(args);
    quint32 upper = quintptr(args) >> 32;
    NSEvent *e = [NSEvent otherEventWithType:NSApplicationDefined
        location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:0
        context:nil subtype:QtCocoaEventSubTypePostMessage data1:lower data2:upper];
    [[NSApplication sharedApplication] postEvent:e atStart:NO];
}

void qt_cocoaPostMessageAfterEventLoopExit(id target, SEL selector, int argCount, id arg1, id arg2)
{
    if (QApplicationPrivate::instance()->threadData->eventLoops.size() <= 1)
        qt_cocoaPostMessage(target, selector, argCount, arg1, arg2);
    else
        new CocoaPostMessageAfterEventLoopExitHelp(target, selector, argCount, arg1, arg2);
}

#endif

QMacCocoaAutoReleasePool::QMacCocoaAutoReleasePool()
{
#ifndef QT_MAC_USE_COCOA
    NSApplicationLoad();
#endif
    pool = (void*)[[NSAutoreleasePool alloc] init];
}

QMacCocoaAutoReleasePool::~QMacCocoaAutoReleasePool()
{
    [(NSAutoreleasePool*)pool release];
}

void qt_mac_post_retranslateAppMenu()
{
#ifdef QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
    qt_cocoaPostMessage([[NSApplication sharedApplication] QT_MANGLE_NAMESPACE(qt_qcocoamenuLoader)], @selector(qtTranslateApplicationMenu));
#endif
}

QWidgetPrivate *QMacScrollOptimization::_target = 0;
bool QMacScrollOptimization::_inWheelEvent = false;
int QMacScrollOptimization::_dx = 0;
int QMacScrollOptimization::_dy = 0;
QRect QMacScrollOptimization::_scrollRect = QRect(0, 0, -1, -1);

#ifdef QT_MAC_USE_COCOA
// This method implements the magic for the drawRectSpecial method.
// We draw a line at the upper edge of the content view in order to
// override the title baseline.
void macDrawRectOnTop(void * /*OSWindowRef */window)
{
    OSWindowRef theWindow = static_cast<OSWindowRef>(window);
    NSView *contentView = [theWindow contentView];
    if(!contentView)
        return;
    // Get coordinates of the content view
    NSRect contentRect = [contentView frame];
    // Draw a line on top of the already drawn line.
    // We need to check if we are active or not to use the proper color.
    if([theWindow isKeyWindow] || [theWindow isMainWindow]) {
        [[NSColor colorWithCalibratedRed:1.0 green:1.0 blue:1.0 alpha:1.0] set];
    } else {
        [[NSColor colorWithCalibratedRed:1.0 green:1.0 blue:1.0 alpha:1.0] set];
    }
    NSPoint origin = NSMakePoint(0, contentRect.size.height);
    NSPoint end = NSMakePoint(contentRect.size.width, contentRect.size.height);
    [NSBezierPath strokeLineFromPoint:origin toPoint:end];
}

// This method will (or at least should) get called only once.
// Its mission is to find out if we are active or not. If we are active
// we assume that we were launched via finder, otherwise we assume
// we were called from the command line. The distinction is important,
// since in the first case we don't need to trigger a paintEvent, while
// in the second case we do.
void macSyncDrawingOnFirstInvocation(void * /*OSWindowRef */window)
{
    OSWindowRef theWindow = static_cast<OSWindowRef>(window);
    NSApplication *application = [NSApplication sharedApplication];
    NSToolbar *toolbar = [theWindow toolbar];
    if([application isActive]) {
        // Launched from finder
        [toolbar setShowsBaselineSeparator:NO];
    } else {
        // Launched from commandline
        [toolbar setVisible:false];
        [toolbar setShowsBaselineSeparator:NO];
        [toolbar setVisible:true];
        [theWindow display];
    }
}

void qt_cocoaStackChildWindowOnTopOfOtherChildren(QWidget *childWidget)
{
    if (!childWidget)
        return;

    QWidget *parent = childWidget->parentWidget();
    if (childWidget->isWindow() && parent) {
        if ([[qt_mac_window_for(parent) childWindows] containsObject:qt_mac_window_for(childWidget)]) {
            QWidgetPrivate *d = qt_widget_private(childWidget);
            d->setSubWindowStacking(false);
            d->setSubWindowStacking(true);
        }
    }
}

void qt_mac_display(QWidget *widget)
{
    NSView *theNSView = qt_mac_nativeview_for(widget);
    [theNSView display];
}

void qt_mac_setNeedsDisplay(QWidget *widget)
{
    NSView *theNSView = qt_mac_nativeview_for(widget);
    [theNSView setNeedsDisplay:YES];
}

void qt_mac_setNeedsDisplayInRect(QWidget *widget, QRegion region)
{
    NSView *theNSView = qt_mac_nativeview_for(widget);
    if (region.isEmpty()) {
        [theNSView setNeedsDisplay:YES];
        return;
    }

    QVector<QRect> rects = region.rects();
    for (int i = 0; i < rects.count(); ++i) {
        const QRect &rect = rects.at(i);
        NSRect nsrect = NSMakeRect(rect.x(), rect.y(), rect.width(), rect.height());
        [theNSView setNeedsDisplayInRect:nsrect];
    }

}

#endif // QT_MAC_USE_COCOA

QT_END_NAMESPACE
