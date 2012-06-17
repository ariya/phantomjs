/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#import "private/qcocoawindowdelegate_mac_p.h"
#ifdef QT_MAC_USE_COCOA
#include <private/qwidget_p.h>
#include <private/qapplication_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#include <qevent.h>
#include <qlayout.h>
#include <qcoreapplication.h>
#include <qmenubar.h>
#include <QMainWindow>
#include <QToolBar>
#include <private/qmainwindowlayout_p.h>

QT_BEGIN_NAMESPACE
extern QWidgetData *qt_qwidget_data(QWidget *); // qwidget.cpp
extern void onApplicationWindowChangedActivation(QWidget *, bool); //qapplication_mac.mm
extern bool qt_sendSpontaneousEvent(QObject *, QEvent *); // qapplication.cpp
QT_END_NAMESPACE

QT_USE_NAMESPACE

static QT_MANGLE_NAMESPACE(QCocoaWindowDelegate) *sharedCocoaWindowDelegate = nil;

// This is a singleton, but unlike most Cocoa singletons, it lives in a library and could be
// pontentially loaded and unloaded. This means we should at least attempt to do the
// memory management correctly.

static void cleanupCocoaWindowDelegate()
{
    [sharedCocoaWindowDelegate release];
}

@implementation QT_MANGLE_NAMESPACE(QCocoaWindowDelegate)

- (id)init
{
    self = [super init];
    if (self != nil) {
        m_windowHash = new QHash<NSWindow *, QWidget *>();
        m_drawerHash = new QHash<NSDrawer *, QWidget *>();
    }
    return self;
}

- (void)dealloc
{
    sharedCocoaWindowDelegate = nil;
    QHash<NSWindow *, QWidget *>::const_iterator windowIt = m_windowHash->constBegin();
    while (windowIt != m_windowHash->constEnd()) {
        [windowIt.key() setDelegate:nil];
        ++windowIt;
    }
    delete m_windowHash;
    QHash<NSDrawer *, QWidget *>::const_iterator drawerIt = m_drawerHash->constBegin();
    while (drawerIt != m_drawerHash->constEnd()) {
        [drawerIt.key() setDelegate:nil];
        ++drawerIt;
    }
    delete m_drawerHash;
    [super dealloc];
}

+ (id)allocWithZone:(NSZone *)zone
{
    @synchronized(self) {
        if (sharedCocoaWindowDelegate == nil) {
            sharedCocoaWindowDelegate = [super allocWithZone:zone];
            return sharedCocoaWindowDelegate;
            qAddPostRoutine(cleanupCocoaWindowDelegate);
        }
    }
    return nil;
}

+ (QT_MANGLE_NAMESPACE(QCocoaWindowDelegate)*)sharedDelegate
{
    @synchronized(self) {
        if (sharedCocoaWindowDelegate == nil)
            [[self alloc] init];
    }
    return [[sharedCocoaWindowDelegate retain] autorelease];
}

-(void)syncSizeForWidget:(QWidget *)qwidget toSize:(const QSize &)newSize fromSize:(const QSize &)oldSize
{
    qt_qwidget_data(qwidget)->crect.setSize(newSize);
    // ### static contents optimization needs to go here
    const OSViewRef view = qt_mac_nativeview_for(qwidget);
    [view setFrameSize:NSMakeSize(newSize.width(), newSize.height())];
    if (!qwidget->isVisible()) {
        qwidget->setAttribute(Qt::WA_PendingResizeEvent, true);
    } else {
        QResizeEvent qre(newSize, oldSize);
        if (qwidget->testAttribute(Qt::WA_PendingResizeEvent)) {
            qwidget->setAttribute(Qt::WA_PendingResizeEvent, false);
            QApplication::sendEvent(qwidget, &qre);
        } else {
            qt_sendSpontaneousEvent(qwidget, &qre);
        }
    }
}

- (void)dumpMaximizedStateforWidget:(QWidget*)qwidget window:(NSWindow *)window
{
    if (!window)
        return; // Nothing to do.
    QWidgetData *widgetData = qt_qwidget_data(qwidget);
    if ((widgetData->window_state & Qt::WindowMaximized) && ![window isZoomed]) {
        widgetData->window_state &= ~Qt::WindowMaximized;
        QWindowStateChangeEvent e(Qt::WindowState(widgetData->window_state | Qt::WindowMaximized));
        qt_sendSpontaneousEvent(qwidget, &e);
    }
}

- (NSSize)closestAcceptableSizeForWidget:(QWidget *)qwidget window:(NSWindow *)window
                             withNewSize:(NSSize)proposedSize
{
    [self dumpMaximizedStateforWidget:qwidget window:window];
    QSize newSize = QLayout::closestAcceptableSize(qwidget, 
                                                   QSize(proposedSize.width, proposedSize.height));
    return [NSWindow frameRectForContentRect:
            NSMakeRect(0., 0., newSize.width(), newSize.height())
                                   styleMask:[window styleMask]].size;
}

- (NSSize)windowWillResize:(NSWindow *)windowToResize toSize:(NSSize)proposedFrameSize
{
    QWidget *qwidget = m_windowHash->value(windowToResize);
    return [self closestAcceptableSizeForWidget:qwidget window:windowToResize
                                    withNewSize:[NSWindow contentRectForFrameRect:
                                                 NSMakeRect(0, 0, 
                                                            proposedFrameSize.width,
                                                            proposedFrameSize.height) 
                                                    styleMask:[windowToResize styleMask]].size];
}

- (NSSize)drawerWillResizeContents:(NSDrawer *)sender toSize:(NSSize)contentSize
{
    QWidget *qwidget = m_drawerHash->value(sender);
    return [self closestAcceptableSizeForWidget:qwidget window:nil withNewSize:contentSize];
}

-(void)windowDidMiniaturize:(NSNotification*)notification
{
    QWidget *qwidget = m_windowHash->value([notification object]);
    if (!qwidget->isMinimized()) {
        QWidgetData *widgetData = qt_qwidget_data(qwidget);
        widgetData->window_state = widgetData->window_state | Qt::WindowMinimized;
        QWindowStateChangeEvent e(Qt::WindowStates(widgetData->window_state & ~Qt::WindowMinimized));
        qt_sendSpontaneousEvent(qwidget, &e);
    }
    // Send hide to match Qt on X11 and Windows
    QEvent e(QEvent::Hide);
    qt_sendSpontaneousEvent(qwidget, &e);
}

- (void)windowDidResize:(NSNotification *)notification
{
    NSWindow *window = [notification object];
    QWidget *qwidget = m_windowHash->value(window);
    QWidgetData *widgetData = qt_qwidget_data(qwidget);
    if (!(qwidget->windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen)) && [window isZoomed]) {
        widgetData->window_state = widgetData->window_state | Qt::WindowMaximized;
        QWindowStateChangeEvent e(Qt::WindowStates(widgetData->window_state
                                                   & ~Qt::WindowMaximized));
        qt_sendSpontaneousEvent(qwidget, &e);
    } else {
        widgetData->window_state = widgetData->window_state & ~Qt::WindowMaximized;
        QWindowStateChangeEvent e(Qt::WindowStates(widgetData->window_state
                                                   | Qt::WindowMaximized));
        qt_sendSpontaneousEvent(qwidget, &e);
    }
    NSRect rect = [[window contentView] frame];
    const QSize newSize(rect.size.width, rect.size.height);
    const QSize &oldSize = widgetData->crect.size();
    if (newSize != oldSize) {
        QWidgetPrivate::qt_mac_update_sizer(qwidget);
        [self syncSizeForWidget:qwidget toSize:newSize fromSize:oldSize];
    }

    // We force the repaint to be synchronized with the resize of the window.
    // Otherwise, the resize looks sluggish because we paint one event loop later.
    if ([[window contentView] inLiveResize]) {
        qwidget->repaint();

        // We need to repaint the toolbar as well.
        QMainWindow* mWindow = qobject_cast<QMainWindow*>(qwidget->window());
        if (mWindow) {
            QMainWindowLayout *mLayout = qobject_cast<QMainWindowLayout*>(mWindow->layout());
            QList<QToolBar *> toolbarList = mLayout->qtoolbarsInUnifiedToolbarList;

            for (int i = 0; i < toolbarList.size(); ++i) {
                QToolBar* toolbar = toolbarList.at(i);
                toolbar->repaint();
            }
        }
    }
}

- (void)windowDidMove:(NSNotification *)notification
{
    // The code underneath needs to translate the window location
    // from bottom left (which is the origin used by Cocoa) to
    // upper left (which is the origin used by Qt):
    NSWindow *window = [notification object];
    NSRect newRect = [window frame];
    QWidget *qwidget = m_windowHash->value(window);
    QPoint qtPoint = flipPoint(NSMakePoint(newRect.origin.x,
                                           newRect.origin.y + newRect.size.height)).toPoint();
    const QRect &oldRect = qwidget->frameGeometry();

    if (qtPoint.x() != oldRect.x() || qtPoint.y() != oldRect.y()) {
        QWidgetData *widgetData = qt_qwidget_data(qwidget);
        QRect oldCRect = widgetData->crect;
        QWidgetPrivate *widgetPrivate = qt_widget_private(qwidget);
        const QRect &fStrut = widgetPrivate->frameStrut();
        widgetData->crect.moveTo(qtPoint.x() + fStrut.left(), qtPoint.y() + fStrut.top());
        if (!qwidget->isVisible()) {
            qwidget->setAttribute(Qt::WA_PendingMoveEvent, true);
        } else {
            QMoveEvent qme(qtPoint, oldRect.topLeft());
            qt_sendSpontaneousEvent(qwidget, &qme);
        }
    }
}

-(BOOL)windowShouldClose:(id)windowThatWantsToClose
{
    QWidget *qwidget = m_windowHash->value(windowThatWantsToClose);
    QScopedLoopLevelCounter counter(qt_widget_private(qwidget)->threadData);
    return qt_widget_private(qwidget)->close_helper(QWidgetPrivate::CloseWithSpontaneousEvent);
}

-(void)windowDidDeminiaturize:(NSNotification *)notification
{
    QWidget *qwidget = m_windowHash->value([notification object]);
    QWidgetData *widgetData = qt_qwidget_data(qwidget);
    Qt::WindowStates currState = Qt::WindowStates(widgetData->window_state);
    Qt::WindowStates newState = currState;
    if (currState & Qt::WindowMinimized)
        newState &= ~Qt::WindowMinimized;
    if (!(currState & Qt::WindowActive))
        newState |= Qt::WindowActive;
    if (newState != currState) {
        widgetData->window_state = newState;
        QWindowStateChangeEvent e(currState);
        qt_sendSpontaneousEvent(qwidget, &e);
    }
    QShowEvent qse;
    qt_sendSpontaneousEvent(qwidget, &qse);
}

-(void)windowDidBecomeMain:(NSNotification*)notification
{
    QWidget *qwidget = m_windowHash->value([notification object]);
    Q_ASSERT(qwidget);
    onApplicationWindowChangedActivation(qwidget, true);
}

-(void)windowDidResignMain:(NSNotification*)notification
{
    QWidget *qwidget = m_windowHash->value([notification object]);
    Q_ASSERT(qwidget);
    onApplicationWindowChangedActivation(qwidget, false);
}

// These are the same as main, but they are probably better to keep separate since there is a
// tiny difference between main and key windows.
-(void)windowDidBecomeKey:(NSNotification*)notification
{
    QWidget *qwidget = m_windowHash->value([notification object]);
    Q_ASSERT(qwidget);
    onApplicationWindowChangedActivation(qwidget, true);
}

-(void)windowDidResignKey:(NSNotification*)notification
{
    QWidget *qwidget = m_windowHash->value([notification object]);
    Q_ASSERT(qwidget);
    onApplicationWindowChangedActivation(qwidget, false);
}

-(QWidget *)qt_qwidgetForWindow:(NSWindow *)window
{
    return m_windowHash->value(window);
}

- (BOOL)windowShouldZoom:(NSWindow *)window toFrame:(NSRect)newFrame
{
    Q_UNUSED(newFrame);
    // saving the current window geometry before the window is maximized
    QWidget *qwidget = m_windowHash->value(window);
    QWidgetPrivate *widgetPrivate = qt_widget_private(qwidget);
    if (qwidget->isWindow()) {
        if(qwidget->windowState() & Qt::WindowMaximized) {
            // Restoring
            widgetPrivate->topData()->wasMaximized = false;
        } else {
            // Maximizing
            widgetPrivate->topData()->normalGeometry = qwidget->geometry();
            // If the window was maximized we need to update the coordinates since now it will start at 0,0.
            // We do this in a special field that is only used when not restoring but manually resizing the window.
            // Since the coordinates are fixed we just set a boolean flag.
            widgetPrivate->topData()->wasMaximized = true;
        }
    }
    return YES;
}

- (NSRect)windowWillUseStandardFrame:(NSWindow *)window defaultFrame:(NSRect)defaultFrame
{
    NSRect frameToReturn = defaultFrame;
    QWidget *qwidget = m_windowHash->value(window);
    QSizeF size = qwidget->maximumSize();
    NSRect windowFrameRect = [window frame];
    NSRect viewFrameRect = [[window contentView] frame];
    // consider additional size required for titlebar & frame
    frameToReturn.size.width = qMin<CGFloat>(frameToReturn.size.width,
            size.width()+(windowFrameRect.size.width - viewFrameRect.size.width));
    frameToReturn.size.height = qMin<CGFloat>(frameToReturn.size.height,
            size.height()+(windowFrameRect.size.height - viewFrameRect.size.height));
    return frameToReturn;
}

- (void)becomeDelegteForWindow:(NSWindow *)window  widget:(QWidget *)widget
{
    m_windowHash->insert(window, widget);
    [window setDelegate:self];
}

- (void)resignDelegateForWindow:(NSWindow *)window
{
    [window setDelegate:nil];
    m_windowHash->remove(window);
}

- (void)becomeDelegateForDrawer:(NSDrawer *)drawer widget:(QWidget *)widget
{
    m_drawerHash->insert(drawer, widget);
    [drawer setDelegate:self];
    NSWindow *window = [[drawer contentView] window];
    [self becomeDelegteForWindow:window widget:widget];
}

- (void)resignDelegateForDrawer:(NSDrawer *)drawer
{
    QWidget *widget = m_drawerHash->value(drawer);
    [drawer setDelegate:nil];
    if (widget)
        [self resignDelegateForWindow:[[drawer contentView] window]];
    m_drawerHash->remove(drawer);
}

- (BOOL)window:(NSWindow *)window shouldPopUpDocumentPathMenu:(NSMenu *)menu
{
    Q_UNUSED(menu);
    QWidget *qwidget = m_windowHash->value(window);
    if (qwidget && !qwidget->windowFilePath().isEmpty()) {
        return YES;
    }
    return NO;
}

- (BOOL)window:(NSWindow *)window shouldDragDocumentWithEvent:(NSEvent *)event
                                                          from:(NSPoint)dragImageLocation
                                                withPasteboard:(NSPasteboard *)pasteboard
{
    Q_UNUSED(event);
    Q_UNUSED(dragImageLocation);
    Q_UNUSED(pasteboard);
    QWidget *qwidget = m_windowHash->value(window);
    if (qwidget && !qwidget->windowFilePath().isEmpty()) {
        return YES;
    }
    return NO;
}

- (void)syncContentViewFrame: (NSNotification *)notification
{
    NSView *cView = [notification object];
    if (cView) {
        NSWindow *window = [cView window];
        QWidget *qwidget = m_windowHash->value(window);
        if (qwidget) {
            QWidgetData *widgetData = qt_qwidget_data(qwidget);
            NSRect rect = [cView frame];
            const QSize newSize(rect.size.width, rect.size.height);
            const QSize &oldSize = widgetData->crect.size();
            if (newSize != oldSize) {
                [self syncSizeForWidget:qwidget toSize:newSize fromSize:oldSize];
            }
        }

    }
}

@end
#endif// QT_MAC_USE_COCOA
