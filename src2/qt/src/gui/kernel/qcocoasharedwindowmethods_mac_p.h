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
 NB: This is not a header file, dispite the file name suffix. This file is
 included directly into the source code of qcocoawindow_mac.mm and
 qcocoapanel_mac.mm to avoid manually doing copy and paste of the exact
 same code needed at both places. This solution makes it more difficult
 to e.g fix a bug in qcocoawindow_mac.mm, but forget to do the same in
 qcocoapanel_mac.mm.
 The reason we need to do copy and paste in the first place, rather than
 resolve to method overriding, is that QCocoaPanel needs to inherit from
 NSPanel, while QCocoaWindow needs to inherit NSWindow rather than NSPanel).
****************************************************************************/

// WARNING: Don't include any header files from within this file. Put them
// directly into qcocoawindow_mac_p.h and qcocoapanel_mac_p.h

QT_BEGIN_NAMESPACE
extern Qt::MouseButton cocoaButton2QtButton(NSInteger buttonNum); // qcocoaview.mm
extern QPointer<QWidget> qt_button_down; //qapplication_mac.cpp
extern const QStringList& qEnabledDraggedTypes(); // qmime_mac.cpp
extern void qt_event_request_window_change(QWidget *); // qapplication_mac.mm
extern void qt_mac_send_posted_gl_updates(QWidget *widget); // qapplication_mac.mm

Q_GLOBAL_STATIC(QPointer<QWidget>, currentDragTarget);
QT_END_NAMESPACE

- (id)initWithContentRect:(NSRect)contentRect
    styleMask:(NSUInteger)windowStyle
    backing:(NSBackingStoreType)bufferingType
    defer:(BOOL)deferCreation
{
    self = [super initWithContentRect:contentRect styleMask:windowStyle
        backing:bufferingType defer:deferCreation];
    if (self) {
        currentCustomDragTypes = 0;
    }
    return self;
}

- (void)dealloc
{
    delete currentCustomDragTypes;
    [super dealloc];
}

- (BOOL)canBecomeKeyWindow
{
    QWidget *widget = [self QT_MANGLE_NAMESPACE(qt_qwidget)];
    if (!widget)
        return NO; // This should happen only for qt_root_win
    if (QApplicationPrivate::isBlockedByModal(widget))
        return NO;

    bool isToolTip = (widget->windowType() == Qt::ToolTip);
    bool isPopup = (widget->windowType() == Qt::Popup);
    return !(isPopup || isToolTip);
}

- (BOOL)canBecomeMainWindow
{
    QWidget *widget = [self QT_MANGLE_NAMESPACE(qt_qwidget)];
    if (!widget)
        return NO; // This should happen only for qt_root_win
    if ([self isSheet])
        return NO;

    bool isToolTip = (widget->windowType() == Qt::ToolTip);
    bool isPopup = (widget->windowType() == Qt::Popup);
    bool isTool = (widget->windowType() == Qt::Tool);
    return !(isPopup || isToolTip || isTool);
}

- (void)becomeMainWindow
{
    [super becomeMainWindow];
    // Cocoa sometimes tell a hidden window to become the
    // main window (and as such, show it). This can e.g
    // happend when the application gets activated. If
    // this is the case, we tell it to hide again:
    if (![self isVisible])
        [self orderOut:self];
}

- (void)toggleToolbarShown:(id)sender
{
    macSendToolbarChangeEvent([self QT_MANGLE_NAMESPACE(qt_qwidget)]);
    [super toggleToolbarShown:sender];
}

- (void)flagsChanged:(NSEvent *)theEvent
{
    qt_dispatchModifiersChanged(theEvent, [self QT_MANGLE_NAMESPACE(qt_qwidget)]);
    [super flagsChanged:theEvent];
}


- (void)tabletProximity:(NSEvent *)tabletEvent
{
    qt_dispatchTabletProximityEvent(tabletEvent);
}

- (void)terminate:(id)sender
{
    // This function is called from the quit item in the menubar when this window
    // is in the first responder chain (see also qtDispatcherToQAction above)
    [NSApp terminate:sender];
}

- (void)setLevel:(NSInteger)windowLevel
{
    // Cocoa will upon activating/deactivating applications level modal
    // windows up and down, regardsless of any explicit set window level.
    // To ensure that modal stays-on-top dialogs actually stays on top after
    // the application is activated (and therefore stacks in front of
    // other stays-on-top windows), we need to add this little special-case override:
    QWidget *widget = [[QT_MANGLE_NAMESPACE(QCocoaWindowDelegate) sharedDelegate] qt_qwidgetForWindow:self];
    if (widget && widget->isModal() && (widget->windowFlags() & Qt::WindowStaysOnTopHint))
        [super setLevel:NSPopUpMenuWindowLevel];
    else
        [super setLevel:windowLevel];
}

- (void)sendEvent:(NSEvent *)event
{
    [self retain];

    bool handled = false;
    switch([event type]) {
    case NSMouseMoved:
        // Cocoa sends move events to a parent and all its children under the mouse, much
        // like Qt handles hover events. But we only want to handle the move event once, so
        // to optimize a bit (since we subscribe for move event for all views), we handle it
        // here before this logic happends. Note: it might be tempting to do this shortcut for
        // all mouse events. The problem is that Cocoa does more than just find the correct view
        // when sending the event, like raising windows etc. So avoid it as much as possible:
        handled = qt_mac_handleMouseEvent(event, QEvent::MouseMove, Qt::NoButton, 0);
        break;
    default:
        break;
    }

    if (!handled) {
        [super sendEvent:event];
        qt_mac_handleNonClientAreaMouseEvent(self, event);
    }
    [self release];
}

- (void)setInitialFirstResponder:(NSView *)view
{
    // This method is called the first time the window is placed on screen and
    // is the earliest point in time we can connect OpenGL contexts to NSViews.
    QWidget *qwidget = [[QT_MANGLE_NAMESPACE(QCocoaWindowDelegate) sharedDelegate] qt_qwidgetForWindow:self];
    if (qwidget) {
        qt_event_request_window_change(qwidget);
        qt_mac_send_posted_gl_updates(qwidget);
    }

    [super setInitialFirstResponder:view];
}

- (BOOL)makeFirstResponder:(NSResponder *)responder
{
    // For some reason Cocoa wants to flip the first responder
    // when Qt doesn't want to, sorry, but "No" :-)
    if (responder == nil && qApp->focusWidget())
        return NO;
    return [super makeFirstResponder:responder];
}

+ (Class)frameViewClassForStyleMask:(NSUInteger)styleMask
{
    if (styleMask & QtMacCustomizeWindow)
        return [QT_MANGLE_NAMESPACE(QCocoaWindowCustomThemeFrame) class];
    return [super frameViewClassForStyleMask:styleMask];
}

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
- (void)touchesBeganWithEvent:(NSEvent *)event;
{
    QPoint qlocal, qglobal;
    QWidget *widgetToGetTouch = 0;
    qt_mac_getTargetForMouseEvent(event, QEvent::Gesture, qlocal, qglobal, 0, &widgetToGetTouch);
    if (!widgetToGetTouch)
        return;

    bool all = widgetToGetTouch->testAttribute(Qt::WA_TouchPadAcceptSingleTouchEvents);
    qt_translateRawTouchEvent(widgetToGetTouch, QTouchEvent::TouchPad, QCocoaTouch::getCurrentTouchPointList(event, all));
}

- (void)touchesMovedWithEvent:(NSEvent *)event;
{
    QPoint qlocal, qglobal;
    QWidget *widgetToGetTouch = 0;
    qt_mac_getTargetForMouseEvent(event, QEvent::Gesture, qlocal, qglobal, 0, &widgetToGetTouch);
    if (!widgetToGetTouch)
        return;

    bool all = widgetToGetTouch->testAttribute(Qt::WA_TouchPadAcceptSingleTouchEvents);
    qt_translateRawTouchEvent(widgetToGetTouch, QTouchEvent::TouchPad, QCocoaTouch::getCurrentTouchPointList(event, all));
}

- (void)touchesEndedWithEvent:(NSEvent *)event;
{
    QPoint qlocal, qglobal;
    QWidget *widgetToGetTouch = 0;
    qt_mac_getTargetForMouseEvent(event, QEvent::Gesture, qlocal, qglobal, 0, &widgetToGetTouch);
    if (!widgetToGetTouch)
        return;

    bool all = widgetToGetTouch->testAttribute(Qt::WA_TouchPadAcceptSingleTouchEvents);
    qt_translateRawTouchEvent(widgetToGetTouch, QTouchEvent::TouchPad, QCocoaTouch::getCurrentTouchPointList(event, all));
}

- (void)touchesCancelledWithEvent:(NSEvent *)event;
{
    QPoint qlocal, qglobal;
    QWidget *widgetToGetTouch = 0;
    qt_mac_getTargetForMouseEvent(event, QEvent::Gesture, qlocal, qglobal, 0, &widgetToGetTouch);
    if (!widgetToGetTouch)
        return;

    bool all = widgetToGetTouch->testAttribute(Qt::WA_TouchPadAcceptSingleTouchEvents);
    qt_translateRawTouchEvent(widgetToGetTouch, QTouchEvent::TouchPad, QCocoaTouch::getCurrentTouchPointList(event, all));
}
#endif // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6

-(void)registerDragTypes
{
    // Calling registerForDraggedTypes below is slow, so only do
    // it once for each window, or when the custom types change.
    QMacCocoaAutoReleasePool pool;
    const QStringList& customTypes = qEnabledDraggedTypes();
    if (currentCustomDragTypes == 0 || *currentCustomDragTypes != customTypes) {
        if (currentCustomDragTypes == 0)
            currentCustomDragTypes = new QStringList();
        *currentCustomDragTypes = customTypes;
        const NSString* mimeTypeGeneric = @"com.trolltech.qt.MimeTypeName";
        NSMutableArray *supportedTypes = [NSMutableArray arrayWithObjects:NSColorPboardType,
                       NSFilenamesPboardType, NSStringPboardType,
                       NSFilenamesPboardType, NSPostScriptPboardType, NSTIFFPboardType,
                       NSRTFPboardType, NSTabularTextPboardType, NSFontPboardType,
                       NSRulerPboardType, NSFileContentsPboardType, NSColorPboardType,
                       NSRTFDPboardType, NSHTMLPboardType, NSPICTPboardType,
                       NSURLPboardType, NSPDFPboardType, NSVCardPboardType,
                       NSFilesPromisePboardType, NSInkTextPboardType,
                       NSMultipleTextSelectionPboardType, mimeTypeGeneric, nil];
        // Add custom types supported by the application.
        for (int i = 0; i < customTypes.size(); i++) {
           [supportedTypes addObject:qt_mac_QStringToNSString(customTypes[i])];
        }
        [self registerForDraggedTypes:supportedTypes];
    }
}

- (void)removeDropData
{
    if (dropData) {
        delete dropData;
        dropData = 0;
    }
}

- (void)addDropData:(id <NSDraggingInfo>)sender
{
    [self removeDropData];
    CFStringRef dropPasteboard = (CFStringRef) [[sender draggingPasteboard] name];
    dropData = new QCocoaDropData(dropPasteboard);
}

- (void)changeDraggingCursor:(NSDragOperation)newOperation
{
    static SEL action = nil;
    static bool operationSupported = false;
    if (action == nil) {
        action = NSSelectorFromString(@"operationNotAllowedCursor");
        if ([NSCursor respondsToSelector:action]) {
            operationSupported = true;
        }
    }
    if (operationSupported) {
        NSCursor *notAllowedCursor = [NSCursor performSelector:action];
        bool isNotAllowedCursor = ([NSCursor currentCursor] == notAllowedCursor);
        if (newOperation == NSDragOperationNone && !isNotAllowedCursor) {
            [notAllowedCursor push];
        } else if (newOperation != NSDragOperationNone && isNotAllowedCursor) {
            [notAllowedCursor pop];
        }

    }
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
    // The user dragged something into the window. Send a draggingEntered message
    // to the QWidget under the mouse. As the drag moves over the window, and over
    // different widgets, we will handle enter and leave events from within
    // draggingUpdated below. The reason why we handle this ourselves rather than
    // subscribing for drag events directly in QCocoaView is that calling
    // registerForDraggedTypes on the views will severly degrade initialization time
    // for an application that uses a lot of drag subscribing widgets.

    NSPoint nswindowPoint = [sender draggingLocation];
    NSPoint nsglobalPoint = [[sender draggingDestinationWindow] convertBaseToScreen:nswindowPoint];
    QPoint globalPoint = flipPoint(nsglobalPoint).toPoint();

    QWidget *qwidget = QApplication::widgetAt(globalPoint);
    *currentDragTarget() = qwidget;
    if (!qwidget)
        return NSDragOperationNone;
    if (qwidget->testAttribute(Qt::WA_DropSiteRegistered) == false)
        return NSDragOperationNone;

    [self addDropData:sender];

    QMimeData *mimeData = dropData;
    if (QDragManager::self()->source())
        mimeData = QDragManager::self()->dragPrivate()->data;

    NSDragOperation nsActions = [sender draggingSourceOperationMask];
    Qt::DropActions qtAllowed = qt_mac_mapNSDragOperations(nsActions);
    QT_PREPEND_NAMESPACE(qt_mac_dnd_answer_rec.lastOperation) = nsActions;
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;

    if ([sender draggingSource] != nil) {
        // modifier flags might have changed, update it here since we don't send any input events.
        QApplicationPrivate::modifier_buttons = qt_cocoaModifiers2QtModifiers([[NSApp currentEvent] modifierFlags]);
        modifiers = QApplication::keyboardModifiers();
    } else {
        // when the source is from another application the above technique will not work.
        modifiers = qt_cocoaDragOperation2QtModifiers(nsActions);
    }

    // send the drag enter event to the widget.
    QPoint localPoint(qwidget->mapFromGlobal(globalPoint));
    QDragEnterEvent qDEEvent(localPoint, qtAllowed, mimeData, QApplication::mouseButtons(), modifiers);
    QApplication::sendEvent(qwidget, &qDEEvent);

    if (!qDEEvent.isAccepted()) {
        // The enter event was not accepted. We mark this by removing
        // the drop data so we don't send subsequent drag move events:
        [self removeDropData];
        [self changeDraggingCursor:NSDragOperationNone];
        return NSDragOperationNone;
    } else {
        // Send a drag move event immediately after a drag enter event (as per documentation).
        QDragMoveEvent qDMEvent(localPoint, qtAllowed, mimeData, QApplication::mouseButtons(), modifiers);
        qDMEvent.setDropAction(qDEEvent.dropAction());
        qDMEvent.accept(); // accept by default, since enter event was accepted.
        QApplication::sendEvent(qwidget, &qDMEvent);

        if (!qDMEvent.isAccepted() || qDMEvent.dropAction() == Qt::IgnoreAction) {
            // Since we accepted the drag enter event, the widget expects
            // future drage move events.
            nsActions = NSDragOperationNone;
            // Save as ignored in the answer rect.
            qDMEvent.setDropAction(Qt::IgnoreAction);
        } else {
            nsActions = QT_PREPEND_NAMESPACE(qt_mac_mapDropAction)(qDMEvent.dropAction());
        }

        QT_PREPEND_NAMESPACE(qt_mac_copy_answer_rect)(qDMEvent);
        [self changeDraggingCursor:nsActions];
        return nsActions;
    }
 }

- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender
{
    NSPoint nswindowPoint = [sender draggingLocation];
    NSPoint nsglobalPoint = [[sender draggingDestinationWindow] convertBaseToScreen:nswindowPoint];
    QPoint globalPoint = flipPoint(nsglobalPoint).toPoint();

    QWidget *qwidget = QApplication::widgetAt(globalPoint);
    if (!qwidget)
        return NSDragOperationNone;

    // First, check if the widget under the mouse has changed since the
    // last drag move events. If so, we need to change target, and dispatch
    // syntetic drag enter/leave events:
    if (qwidget != *currentDragTarget()) {
        if (*currentDragTarget() && dropData) {
            QDragLeaveEvent de;
            QApplication::sendEvent(*currentDragTarget(), &de);
            [self removeDropData];
        }
        return [self draggingEntered:sender];
    }

    if (qwidget->testAttribute(Qt::WA_DropSiteRegistered) == false)
        return NSDragOperationNone;

    // If we have no drop data (which will be assigned inside draggingEntered), it means
    // that the current drag target did not accept the enter event. If so, we ignore
    // subsequent move events as well:
    if (dropData == 0) {
        [self changeDraggingCursor:NSDragOperationNone];
        return NSDragOperationNone;
    }

    // If the mouse is still within the accepted rect (provided by
    // the application on a previous event), we follow the optimization
    // and just return the answer given at that point:
    NSDragOperation nsActions = [sender draggingSourceOperationMask];
    QPoint localPoint(qwidget->mapFromGlobal(globalPoint));
    if (qt_mac_mouse_inside_answer_rect(localPoint)
        && QT_PREPEND_NAMESPACE(qt_mac_dnd_answer_rec.lastOperation) == nsActions) {
        NSDragOperation operation = QT_PREPEND_NAMESPACE(qt_mac_mapDropActions)(QT_PREPEND_NAMESPACE(qt_mac_dnd_answer_rec.lastAction));
        [self changeDraggingCursor:operation];
        return operation;
    }

    QT_PREPEND_NAMESPACE(qt_mac_dnd_answer_rec.lastOperation) = nsActions;
    Qt::DropActions qtAllowed = QT_PREPEND_NAMESPACE(qt_mac_mapNSDragOperations)(nsActions);
    Qt::KeyboardModifiers modifiers  = Qt::NoModifier;

    // Update modifiers:
    if ([sender draggingSource] != nil) {
        QApplicationPrivate::modifier_buttons = qt_cocoaModifiers2QtModifiers([[NSApp currentEvent] modifierFlags]);
        modifiers = QApplication::keyboardModifiers();
    } else {
        modifiers = qt_cocoaDragOperation2QtModifiers(nsActions);
    }

    QMimeData *mimeData = dropData;
    if (QDragManager::self()->source())
        mimeData = QDragManager::self()->dragPrivate()->data;

    // Insert the same drop action on the event according to
    // what the application told us it should be on the previous event:
    QDragMoveEvent qDMEvent(localPoint, qtAllowed, mimeData, QApplication::mouseButtons(), modifiers);
    if (QT_PREPEND_NAMESPACE(qt_mac_dnd_answer_rec).lastAction != Qt::IgnoreAction
        && QT_PREPEND_NAMESPACE(qt_mac_dnd_answer_rec).buttons == qDMEvent.mouseButtons()
        && QT_PREPEND_NAMESPACE(qt_mac_dnd_answer_rec).modifiers == qDMEvent.keyboardModifiers())
        qDMEvent.setDropAction(QT_PREPEND_NAMESPACE(qt_mac_dnd_answer_rec).lastAction);

    // Now, end the drag move event to the widget:
    qDMEvent.accept();
    QApplication::sendEvent(qwidget, &qDMEvent);

    NSDragOperation operation = qt_mac_mapDropAction(qDMEvent.dropAction());
    if (!qDMEvent.isAccepted() || qDMEvent.dropAction() == Qt::IgnoreAction) {
        // Ignore this event (we will still receive further
        // notifications), save as ignored in the answer rect:
        operation = NSDragOperationNone;
        qDMEvent.setDropAction(Qt::IgnoreAction);
    }

    qt_mac_copy_answer_rect(qDMEvent);
    [self changeDraggingCursor:operation];

    return operation;
}

- (void)draggingExited:(id <NSDraggingInfo>)sender
{
    NSPoint nswindowPoint = [sender draggingLocation];
    NSPoint nsglobalPoint = [[sender draggingDestinationWindow] convertBaseToScreen:nswindowPoint];
    QPoint globalPoint = flipPoint(nsglobalPoint).toPoint();

    QWidget *qwidget = *currentDragTarget();
    if (!qwidget)
        return;

    if (dropData) {
        QDragLeaveEvent de;
        QApplication::sendEvent(qwidget, &de);
        [self removeDropData];
    }

    // Clean-up:
    [self removeDropData];
    *currentDragTarget() = 0;
    [self changeDraggingCursor:NSDragOperationEvery];
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    QWidget *qwidget = *currentDragTarget();
    if (!qwidget)
        return NO;

    *currentDragTarget() = 0;
    NSPoint nswindowPoint = [sender draggingLocation];
    NSPoint nsglobalPoint = [[sender draggingDestinationWindow] convertBaseToScreen:nswindowPoint];
    QPoint globalPoint = flipPoint(nsglobalPoint).toPoint();

    [self addDropData:sender];

    NSDragOperation nsActions = [sender draggingSourceOperationMask];
    Qt::DropActions qtAllowed = qt_mac_mapNSDragOperations(nsActions);
    QMimeData *mimeData = dropData;

    if (QDragManager::self()->source())
        mimeData = QDragManager::self()->dragPrivate()->data;
    if (QDragManager::self()->object)
        QDragManager::self()->dragPrivate()->target = qwidget;

    QPoint localPoint(qwidget->mapFromGlobal(globalPoint));
    QDropEvent de(localPoint, qtAllowed, mimeData,
                  QApplication::mouseButtons(), QApplication::keyboardModifiers());
    QApplication::sendEvent(qwidget, &de);

    if (QDragManager::self()->object)
        QDragManager::self()->dragPrivate()->executed_action = de.dropAction();

    return de.isAccepted();
}

// This is a hack and it should be removed once we find the real cause for
// the painting problems.
// We have a static variable that signals if we have been called before or not.
static bool firstDrawingInvocation = true;

// The method below exists only as a workaround to draw/not draw the baseline
// in the title bar. This is to support unifiedToolbar look.

// This method is very special. To begin with, it is a
// method that will get called only if we enable documentMode.
// Furthermore, it won't get called as a normal method, we swap
// this method with the normal implementation of drawRect in
// _NSThemeFrame. When this method is active, its mission is to
// first call the original drawRect implementation so the widget
// gets proper painting. After that, it needs to detect if there
// is a toolbar or not, in order to decide how to handle the unified
// look. The distinction is important since the presence and
// visibility of a toolbar change the way we enter into unified mode.
// When there is a toolbar and that toolbar is visible, the problem
// is as simple as to tell the toolbar not to draw its baseline.
// However when there is not toolbar or the toolbar is not visible,
// we need to draw a line on top of the baseline, because the baseline
// in that case will belong to the title. For this case we need to draw
// a line on top of the baseline.
// As usual, there is a special case. When we first are called, we might
// need to repaint ourselves one more time. We only need that if we
// didn't get the activation, i.e. when we are launched via the command
// line. And this only if the toolbar is visible from the beginning,
// so we have a special flag that signals if we need to repaint or not.
- (void)drawRectSpecial:(NSRect)rect
{
    // Call the original drawing method.
    [id(self) drawRectOriginal:rect];
    NSWindow *window = [id(self) window];
    NSToolbar *toolbar = [window toolbar];
    if(!toolbar) {
        // There is no toolbar, we have to draw a line on top of the line drawn by Cocoa.
        macDrawRectOnTop((void *)window);
    } else {
        if([toolbar isVisible]) {
            // We tell Cocoa to avoid drawing the line at the end.
            if(firstDrawingInvocation) {
                firstDrawingInvocation = false;
                macSyncDrawingOnFirstInvocation((void *)window);
            } else
                [toolbar setShowsBaselineSeparator:NO];
        } else {
            // There is a toolbar but it is not visible so
            // we have to draw a line on top of the line drawn by Cocoa.
            macDrawRectOnTop((void *)window);
        }
    }
}

- (void)drawRectOriginal:(NSRect)rect
{
    Q_UNUSED(rect)
    // This method implementation is here to silenct the compiler.
    // See drawRectSpecial for information.
}

