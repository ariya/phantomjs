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

#import <private/qcocoaview_mac_p.h>
#ifdef QT_MAC_USE_COCOA

#include <private/qwidget_p.h>
#include <private/qt_mac_p.h>
#include <private/qapplication_p.h>
#include <private/qabstractscrollarea_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#include <private/qdnd_p.h>
#include <private/qmacinputcontext_p.h>
#include <private/qevent_p.h>
#include <private/qbackingstore_p.h>
#include <private/qwindowsurface_raster_p.h>
#include <private/qunifiedtoolbarsurface_mac_p.h>

#include <qscrollarea.h>
#include <qhash.h>
#include <qtextformat.h>
#include <qpaintengine.h>
#include <QUrl>
#include <QAccessible>
#include <QFileInfo>
#include <QFile>

#include <qdebug.h>

@interface NSEvent (Qt_Compile_Leopard_DeviceDelta)
  // SnowLeopard:
  - (CGFloat)deviceDeltaX;
  - (CGFloat)deviceDeltaY;
  - (CGFloat)deviceDeltaZ;
  // Lion:
  - (CGFloat)scrollingDeltaX;
  - (CGFloat)scrollingDeltaY;
  - (CGFloat)scrollingDeltaZ;
@end

@interface NSEvent (Qt_Compile_Leopard_Gestures)
  - (CGFloat)magnification;
@end

QT_BEGIN_NAMESPACE

extern void qt_mac_update_cursor(); // qcursor_mac.mm
extern bool qt_sendSpontaneousEvent(QObject *, QEvent *); // qapplication.cpp
extern QPointer<QWidget> qt_last_mouse_receiver; // qapplication_mac.cpp
extern QPointer<QWidget> qt_last_native_mouse_receiver; // qt_cocoa_helpers_mac.mm
extern OSViewRef qt_mac_nativeview_for(const QWidget *w); // qwidget_mac.mm
extern OSViewRef qt_mac_effectiveview_for(const QWidget *w); // qwidget_mac.mm
extern QPointer<QWidget> qt_button_down; //qapplication_mac.cpp
extern Qt::MouseButton cocoaButton2QtButton(NSInteger buttonNum);
extern QWidget *mac_mouse_grabber;
extern bool qt_mac_clearDirtyOnWidgetInsideDrawWidget; // qwidget.cpp

static QColor colorFrom(NSColor *color)
{
    QColor qtColor;
    NSString *colorSpace = [color colorSpaceName];
    if (colorSpace == NSDeviceCMYKColorSpace) {
        CGFloat cyan, magenta, yellow, black, alpha;
        [color getCyan:&cyan magenta:&magenta yellow:&yellow black:&black alpha:&alpha];
        qtColor.setCmykF(cyan, magenta, yellow, black, alpha);
    } else {
        NSColor *tmpColor;
        tmpColor = [color colorUsingColorSpaceName:NSDeviceRGBColorSpace];
        CGFloat red, green, blue, alpha;
        [tmpColor getRed:&red green:&green blue:&blue alpha:&alpha];
        qtColor.setRgbF(red, green, blue, alpha);
    }
    return qtColor;
}

QT_END_NAMESPACE

QT_FORWARD_DECLARE_CLASS(QMacCocoaAutoReleasePool)
QT_FORWARD_DECLARE_CLASS(QCFString)
QT_FORWARD_DECLARE_CLASS(QDragManager)
QT_FORWARD_DECLARE_CLASS(QMimeData)
QT_FORWARD_DECLARE_CLASS(QPoint)
QT_FORWARD_DECLARE_CLASS(QApplication)
QT_FORWARD_DECLARE_CLASS(QApplicationPrivate)
QT_FORWARD_DECLARE_CLASS(QDragEnterEvent)
QT_FORWARD_DECLARE_CLASS(QDragMoveEvent)
QT_FORWARD_DECLARE_CLASS(QStringList)
QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QRect)
QT_FORWARD_DECLARE_CLASS(QRegion)
QT_FORWARD_DECLARE_CLASS(QAbstractScrollArea)
QT_FORWARD_DECLARE_CLASS(QAbstractScrollAreaPrivate)
QT_FORWARD_DECLARE_CLASS(QPaintEvent)
QT_FORWARD_DECLARE_CLASS(QPainter)
QT_FORWARD_DECLARE_CLASS(QHoverEvent)
QT_FORWARD_DECLARE_CLASS(QCursor)
QT_USE_NAMESPACE
extern "C" {
    extern NSString *NSTextInputReplacementRangeAttributeName;
}

//#define ALIEN_DEBUG 1
#ifdef ALIEN_DEBUG
static int qCocoaViewCount = 0;
#endif

@implementation QT_MANGLE_NAMESPACE(QCocoaView)

- (id)initWithQWidget:(QWidget *)widget widgetPrivate:(QWidgetPrivate *)widgetprivate
{
    self = [super init];
    if (self) {
        [self finishInitWithQWidget:widget widgetPrivate:widgetprivate];
    }
    [self setFocusRingType:NSFocusRingTypeNone];
    composingText = new QString();

#ifdef ALIEN_DEBUG
    ++qCocoaViewCount;
    qDebug() << "Alien: create native view for" << widget << ". qCocoaViewCount is:" << qCocoaViewCount;
#endif

    composing = false;
    sendKeyEvents = true;
    fromKeyDownEvent = false;
    alienTouchCount = 0;

    [self setHidden:YES];
    return self;
}

- (void) finishInitWithQWidget:(QWidget *)widget widgetPrivate:(QWidgetPrivate *)widgetprivate
{
    qwidget = widget;
    qwidgetprivate = widgetprivate;
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(frameDidChange:)
                                                 name:@"NSViewFrameDidChangeNotification"
                                               object:self];
}

- (void)dealloc
{
    QMacCocoaAutoReleasePool pool;
    delete composingText;
    [[NSNotificationCenter defaultCenter] removeObserver:self];

#ifdef ALIEN_DEBUG
    --qCocoaViewCount;
    qDebug() << "Alien: widget deallocated. qCocoaViewCount is:" << qCocoaViewCount;
#endif

    [super dealloc];
}

- (BOOL)isOpaque
{
    if (!qwidgetprivate)
        return [super isOpaque];
    return qwidgetprivate->isOpaque;
}

- (BOOL)isFlipped
{
    return YES;
}

// We preserve the content of the view if WA_StaticContents is defined.
//
// More info in the Cocoa documentation:
// http://developer.apple.com/mac/library/documentation/cocoa/conceptual/CocoaViewsGuide/Optimizing/Optimizing.html
- (BOOL) preservesContentDuringLiveResize
{
    return qwidget->testAttribute(Qt::WA_StaticContents);
}

- (void) setFrameSize:(NSSize)newSize
{
    [super setFrameSize:newSize];

    // A change in size has required the view to be invalidated.
    if ([self inLiveResize]) {
        NSRect rects[4];
        NSInteger count;
        [self getRectsExposedDuringLiveResize:rects count:&count];
        while (count-- > 0)
        {
            [self setNeedsDisplayInRect:rects[count]];
        }
    } else {
        [self setNeedsDisplay:YES];
    }

    // Make sure the opengl context is updated on resize.
    if (qwidgetprivate && qwidgetprivate->isGLWidget && [self window]) {
        qwidgetprivate->needWindowChange = true;
        QEvent event(QEvent::MacGLWindowChange);
        qApp->sendEvent(qwidget, &event);
    }
}

// We catch the 'setNeedsDisplay:' message in order to avoid a useless full repaint.
// During the resize, the top of the widget is repainted, probably because of the
// change of coordinate space (Quartz vs Qt). This is then followed by this message:
// -[NSView _setNeedsDisplayIfTopLeftChanged]
// which force a full repaint by sending the message 'setNeedsDisplay:'.
// That is what we are preventing here.
- (void)setNeedsDisplay:(BOOL)flag {
    if (![self inLiveResize] || !(qwidget->testAttribute(Qt::WA_StaticContents))) {
        [super setNeedsDisplay:flag];
    }
}

- (void)drawRect:(NSRect)aRect
{
    if (!qwidget)
        return;

    // Getting context.
    CGContextRef context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
    qt_mac_retain_graphics_context(context);

    // We use a different graphics system.
    //
    // Widgets that are set to paint on screen, specifically QGLWidget,
    // requires the native engine to execute in order to be drawn.
    if (QApplicationPrivate::graphicsSystem() != 0 && !qwidget->testAttribute(Qt::WA_PaintOnScreen)) {

        // Raster engine.
        if (QApplicationPrivate::graphics_system_name == QLatin1String("raster")) {

            if (!qwidgetprivate->isInUnifiedToolbar) {

                // Qt handles the painting occuring inside the window.
                // Cocoa also keeps track of all widgets as NSView and therefore might
                // ask for a repainting of a widget even if Qt is already taking care of it.
                //
                // The only valid reason for Cocoa to call drawRect: is for window manipulation
                // (ie. resize, ...).
                //
                // Qt will then forward the update to the children.
                if (!qwidget->isWindow()) {
                    qt_mac_release_graphics_context(context);
                    return;
                }

                QRasterWindowSurface *winSurface = dynamic_cast<QRasterWindowSurface *>(qwidget->windowSurface());
                if (!winSurface || !winSurface->needsFlush) {
                    qt_mac_release_graphics_context(context);
                    return;
                }

                // Clip to region.
                const QVector<QRect> &rects = winSurface->regionToFlush.rects();
                for (int i = 0; i < rects.size(); ++i) {
                    const QRect &rect = rects.at(i);
                    CGContextAddRect(context, CGRectMake(rect.x(), rect.y(), rect.width(), rect.height()));
                }
                CGContextClip(context);

                QRect r = winSurface->regionToFlush.boundingRect();
                const CGRect area = CGRectMake(r.x(), r.y(), r.width(), r.height());

                qt_mac_draw_image(context, winSurface->imageContext(), area, area);

                winSurface->needsFlush = false;
                winSurface->regionToFlush = QRegion();

            } else {

                QUnifiedToolbarSurface *unifiedSurface = qwidgetprivate->unifiedSurface;
                if (!unifiedSurface) {
                    qt_mac_release_graphics_context(context);
                    return;
                }

                int areaX = qwidgetprivate->toolbar_offset.x();
                int areaY = qwidgetprivate->toolbar_offset.y();
                int areaWidth = qwidget->geometry().width();
                int areaHeight = qwidget->geometry().height();
                const CGRect area = CGRectMake(areaX, areaY, areaWidth, areaHeight);
                const CGRect drawingArea = CGRectMake(0, 0, areaWidth, areaHeight);

                qt_mac_draw_image(context, unifiedSurface->imageContext(), area, drawingArea);

                qwidgetprivate->flushRequested = false;

            }

            CGContextSynchronize(context);
            qt_mac_release_graphics_context(context);
            return;
        }

        // Qt handles the painting occuring inside the window.
        // Cocoa also keeps track of all widgets as NSView and therefore might
        // ask for a repainting of a widget even if Qt is already taking care of it.
        //
        // The only valid reason for Cocoa to call drawRect: is for window manipulation
        // (ie. resize, ...).
        //
        // Qt will then forward the update to the children.
        if (qwidget->isWindow()) {
            qwidget->update(qwidget->rect());
            qwidgetprivate->syncBackingStore(qwidget->rect());
        }
    }

    // Native engine.
    qwidgetprivate->hd = context;

    if (qwidget->isVisible() && qwidget->updatesEnabled()) { //process the actual paint event.
        if (qwidget->testAttribute(Qt::WA_WState_InPaintEvent))
            qWarning("QWidget::repaint: Recursive repaint detected");

        const QRect qrect = QRect(aRect.origin.x, aRect.origin.y, aRect.size.width, aRect.size.height);
        QRegion qrgn;

        const NSRect *rects;
        NSInteger count;
        [self getRectsBeingDrawn:&rects count:&count];
        for (int i = 0; i < count; ++i) {
            QRect tmpRect = QRect(rects[i].origin.x, rects[i].origin.y, rects[i].size.width, rects[i].size.height);
            qrgn += tmpRect;
        }

        if (!qwidget->isWindow() && !qobject_cast<QAbstractScrollArea *>(qwidget->parent())) {
            const QRegion &parentMask = qwidget->window()->mask();
            if (!parentMask.isEmpty()) {
                const QPoint mappedPoint = qwidget->mapTo(qwidget->window(), qrect.topLeft());
                qrgn.translate(mappedPoint);
                qrgn &= parentMask;
                qrgn.translate(-mappedPoint.x(), -mappedPoint.y());
            }
        }

        QPoint redirectionOffset(0, 0);
        //setup the context
        qwidget->setAttribute(Qt::WA_WState_InPaintEvent);
        QPaintEngine *engine = qwidget->paintEngine();
        if (engine)
            engine->setSystemClip(qrgn);
        if (qwidgetprivate->extra && qwidgetprivate->extra->hasMask) {
            CGRect widgetRect = CGRectMake(0, 0, qwidget->width(), qwidget->height());
            CGContextTranslateCTM (context, 0, widgetRect.size.height);
            CGContextScaleCTM(context, 1, -1);
            if (qwidget->isWindow())
                CGContextClearRect(context, widgetRect);
            CGContextClipToMask(context, widgetRect, qwidgetprivate->extra->imageMask);
            CGContextScaleCTM(context, 1, -1);
            CGContextTranslateCTM (context, 0, -widgetRect.size.height);
        }

        if (qwidget->isWindow() && !qwidgetprivate->isOpaque
            && !qwidget->testAttribute(Qt::WA_MacBrushedMetal)) {
            CGContextClearRect(context, NSRectToCGRect(aRect));
        }

        qwidget->setAttribute(Qt::WA_WState_InPaintEvent, false);
        QWidgetPrivate *qwidgetPrivate = qt_widget_private(qwidget);

        // We specify that we want to draw the widget itself, and
        // all its children recursive. But we skip native children, because
        // they will receive drawRect calls by themselves as needed:
        int flags = QWidgetPrivate::DrawPaintOnScreen
            | QWidgetPrivate::DrawRecursive
            | QWidgetPrivate::DontDrawNativeChildren;

        if (qwidget->isWindow())
            flags |= QWidgetPrivate::DrawAsRoot;

        // Start to draw:
        qt_mac_clearDirtyOnWidgetInsideDrawWidget = true;
        qwidgetPrivate->drawWidget(qwidget, qrgn, QPoint(), flags, 0);
        qt_mac_clearDirtyOnWidgetInsideDrawWidget = false;

        if (!redirectionOffset.isNull())
            QPainter::restoreRedirected(qwidget);
        if (engine)
            engine->setSystemClip(QRegion());
        qwidget->setAttribute(Qt::WA_WState_InPaintEvent, false);
        if(!qwidget->testAttribute(Qt::WA_PaintOutsidePaintEvent) && qwidget->paintingActive())
            qWarning("QWidget: It is dangerous to leave painters active on a"
            " widget outside of the PaintEvent");
    }
    qwidgetprivate->hd = 0;
    qt_mac_release_graphics_context(context);
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    // Find the widget that should receive the event:
    QPoint qlocal, qglobal;
    QWidget *widgetToGetMouse = qt_mac_getTargetForMouseEvent(theEvent, QEvent::MouseButtonPress, qlocal, qglobal, qwidget, 0);
    if (!widgetToGetMouse)
        return NO;

    return !widgetToGetMouse->testAttribute(Qt::WA_MacNoClickThrough);
}

- (NSView *)hitTest:(NSPoint)aPoint
{
    if (!qwidget)
        return [super hitTest:aPoint];

    if (qwidget->testAttribute(Qt::WA_TransparentForMouseEvents))
        return nil; // You cannot hit a transparent for mouse event widget.
    return [super hitTest:aPoint];
}

- (void)updateTrackingAreas
{
    if (!qwidget)
        return;

    // [NSView addTrackingArea] is slow, so bail out early if we can:
    if (NSIsEmptyRect([self visibleRect]))
        return;

    QMacCocoaAutoReleasePool pool;
    if (NSArray *trackingArray = [self trackingAreas]) {
        NSUInteger size = [trackingArray count];
        for (NSUInteger i = 0; i < size; ++i) {
            NSTrackingArea *t = [trackingArray objectAtIndex:i];
            [self removeTrackingArea:t];
        }
    }

    // Ideally, we shouldn't have NSTrackingMouseMoved events included below, it should
    // only be turned on if mouseTracking, hover is on or a tool tip is set.
    // Unfortunately, Qt will send "tooltip" events on mouse moves, so we need to
    // turn it on in ALL case. That means EVERY QCocoaView gets to pay the cost of
    // mouse moves delivered to it (Apple recommends keeping it OFF because there
    // is a performance hit). So it goes.
    NSUInteger trackingOptions = NSTrackingMouseEnteredAndExited | NSTrackingActiveInActiveApp
                                 | NSTrackingInVisibleRect | NSTrackingMouseMoved;
    NSTrackingArea *ta = [[NSTrackingArea alloc] initWithRect:NSMakeRect(0, 0,
                                                                         qwidget->width(),
                                                                         qwidget->height())
                                                      options:trackingOptions
                                                        owner:self
                                                     userInfo:nil];
    [self addTrackingArea:ta];
    [ta release];
}

- (void)mouseEntered:(NSEvent *)event
{
    // Cocoa will not send a move event on mouseEnter. But since
    // Qt expect this, we fake one now. See also mouseExited below
    // for info about enter/leave event handling
    NSEvent *nsmoveEvent = [NSEvent
        mouseEventWithType:NSMouseMoved
        location:[[self window] mouseLocationOutsideOfEventStream]
        modifierFlags: [event modifierFlags]
        timestamp: [event timestamp]
        windowNumber: [event windowNumber]
        context: [event context]
        eventNumber: [event eventNumber]
        clickCount: 0
        pressure: 0];

    // Important: Cocoa sends us mouseEnter on all views under the mouse
    // and not just the one on top. Therefore, to we cannot use qwidget
    // as native widget for this case. Instead, we let qt_mac_handleMouseEvent
    // resolve it (last argument set to 0):
    qt_mac_handleMouseEvent(nsmoveEvent, QEvent::MouseMove, Qt::NoButton, 0);
}

- (void)mouseExited:(NSEvent *)event
{
    // Note: normal enter/leave handling is done from within mouseMove. This handler
    // catches the case when the mouse moves out of the window (which mouseMove do not).
    // Updating the mouse cursor follows the same logic as enter/leave. And we update
    // neither if a grab exists (even if the grab points to this widget, it seems, ref X11)
    Q_UNUSED(event);
    if (self == [[self window] contentView] && !qt_button_down && !QWidget::mouseGrabber()) {
        qt_mac_update_cursor();
        // If the mouse exits the content view, but qt_mac_getTargetForMouseEvent still
        // reports a target, it means that either there is a grab involved, or the mouse
        // hovered over another window in the application. In both cases, move events will
        // cause qt_mac_handleMouseEvent to be called, which will handle enter/leave.
        QPoint qlocal, qglobal;
        QWidget *widgetUnderMouse = 0;
        qt_mac_getTargetForMouseEvent(event, QEvent::Leave, qlocal, qglobal, qwidget, &widgetUnderMouse);

        if (widgetUnderMouse == 0) {
            QApplicationPrivate::dispatchEnterLeave(0, qt_last_mouse_receiver);
            qt_last_mouse_receiver = 0;
            qt_last_native_mouse_receiver = 0;
        }
    }
}

- (void)flagsChanged:(NSEvent *)theEvent
{
    QWidget *widgetToGetKey = qt_mac_getTargetForKeyEvent(qwidget);
    if (!widgetToGetKey)
        return;

    qt_dispatchModifiersChanged(theEvent, widgetToGetKey);
    [super flagsChanged:theEvent];
}

- (void)mouseMoved:(NSEvent *)theEvent
{
    // Important: this method will only be called when the view's window is _not_ inside
    // QCocoaWindow/QCocoaPanel. Otherwise, [QCocoaWindow sendEvent] will handle the event
    // before it ends up here. So, this method is added for supporting QMacNativeWidget.
    // TODO: Cocoa send move events to all views under the mouse. So make sure we only
    // handle the event for the widget on top when using QMacNativeWidget.
    qt_mac_handleMouseEvent(theEvent, QEvent::MouseMove, Qt::NoButton, qwidget);
}

- (void)mouseDown:(NSEvent *)theEvent
{
    qt_mac_handleMouseEvent(theEvent, QEvent::MouseButtonPress, Qt::LeftButton, qwidget);
    // Don't call super here. This prevents us from getting the mouseUp event,
    // which we need to send even if the mouseDown event was not accepted.
    // (this is standard Qt behavior.)
}

- (void)mouseUp:(NSEvent *)theEvent
{
    qt_mac_handleMouseEvent(theEvent, QEvent::MouseButtonRelease, Qt::LeftButton, qwidget);
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
    qt_mac_handleMouseEvent(theEvent, QEvent::MouseButtonPress, Qt::RightButton, qwidget);
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
    qt_mac_handleMouseEvent(theEvent, QEvent::MouseButtonRelease, Qt::RightButton, qwidget);
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
    Qt::MouseButton mouseButton = cocoaButton2QtButton([theEvent buttonNumber]);
    qt_mac_handleMouseEvent(theEvent, QEvent::MouseButtonPress, mouseButton, qwidget);
}

- (void)otherMouseUp:(NSEvent *)theEvent
{
    Qt::MouseButton mouseButton = cocoaButton2QtButton([theEvent buttonNumber]);
    qt_mac_handleMouseEvent(theEvent,  QEvent::MouseButtonRelease, mouseButton, qwidget);
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    qt_mac_handleMouseEvent(theEvent, QEvent::MouseMove, Qt::NoButton, qwidget);
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
    qt_mac_handleMouseEvent(theEvent, QEvent::MouseMove, Qt::NoButton, qwidget);
}

- (void)otherMouseDragged:(NSEvent *)theEvent
{
    qt_mac_handleMouseEvent(theEvent, QEvent::MouseMove, Qt::NoButton, qwidget);
}

- (void)scrollWheel:(NSEvent *)theEvent
{
    // Give the Input Manager a chance to process the wheel event.
    NSInputManager *currentIManager = [NSInputManager currentInputManager];
    if (currentIManager && [currentIManager wantsToHandleMouseEvents]) {
        [currentIManager handleMouseEvent:theEvent];
    }

    Qt::MouseButtons buttons = QApplication::mouseButtons();
    Qt::KeyboardModifiers keyMods = qt_cocoaModifiers2QtModifiers([theEvent modifierFlags]);

    // Find the widget that should receive the event:
    QPoint qlocal, qglobal;
    QWidget *widgetToGetMouse = qt_mac_getTargetForMouseEvent(theEvent, QEvent::Wheel, qlocal, qglobal, qwidget, 0);
    if (!widgetToGetMouse)
        return;

    int deltaX = 0;
    int deltaY = 0;

    const EventRef carbonEvent = (EventRef)[theEvent eventRef];
    const UInt32 carbonEventKind = carbonEvent ? ::GetEventKind(carbonEvent) : 0;
    const bool scrollEvent = carbonEventKind == kEventMouseScroll;

    if (scrollEvent) {
        // The mouse device containts pixel scroll wheel support (Mighty Mouse, Trackpad).
        // Since deviceDelta is delivered as pixels rather than degrees, we need to
        // convert from pixels to degrees in a sensible manner.
        // It looks like 1/4 degrees per pixel behaves most native.
        // (NB: Qt expects the unit for delta to be 8 per degree):
        const int pixelsToDegrees = 2; // 8 * 1/4
        if (QSysInfo::MacintoshVersion <= QSysInfo::MV_10_6) {
            // Mac OS 10.6
            deltaX = [theEvent deviceDeltaX] * pixelsToDegrees;
            deltaY = [theEvent deviceDeltaY] * pixelsToDegrees;
        } else {
            // Mac OS 10.7+
            deltaX = [theEvent scrollingDeltaX] * pixelsToDegrees;
            deltaY = [theEvent scrollingDeltaY] * pixelsToDegrees;
        }
    } else {
        // carbonEventKind == kEventMouseWheelMoved
        // Remove acceleration, and use either -120 or 120 as delta:
        deltaX = qBound(-120, int([theEvent deltaX] * 10000), 120);
        deltaY = qBound(-120, int([theEvent deltaY] * 10000), 120);
    }

#ifndef QT_NO_WHEELEVENT
    // ### Qt 5: Send one QWheelEvent with dx, dy and dz

    if (deltaX != 0 && deltaY != 0)
        QMacScrollOptimization::initDelayedScroll();

    if (deltaX != 0) {
        QWheelEvent qwe(qlocal, qglobal, deltaX, buttons, keyMods, Qt::Horizontal);
        qt_sendSpontaneousEvent(widgetToGetMouse, &qwe);
    }

    if (deltaY != 0) {
        QWheelEvent qwe(qlocal, qglobal, deltaY, buttons, keyMods, Qt::Vertical);
        qt_sendSpontaneousEvent(widgetToGetMouse, &qwe);
    }

    if (deltaX != 0 && deltaY != 0)
        QMacScrollOptimization::performDelayedScroll();
#endif //QT_NO_WHEELEVENT
}

- (void)tabletProximity:(NSEvent *)tabletEvent
{
    qt_dispatchTabletProximityEvent(tabletEvent);
}

- (void)tabletPoint:(NSEvent *)tabletEvent
{
    if (!qt_mac_handleTabletEvent(self, tabletEvent))
        [super tabletPoint:tabletEvent];
}

- (void)magnifyWithEvent:(NSEvent *)event
{
    QPoint qlocal, qglobal;
    QWidget *widgetToGetGesture = 0;
    qt_mac_getTargetForMouseEvent(event, QEvent::Gesture, qlocal, qglobal, qwidget, &widgetToGetGesture);
    if (!widgetToGetGesture)
        return;
    if (!QApplicationPrivate::tryModalHelper(widgetToGetGesture, 0))
        return;

#ifndef QT_NO_GESTURES
    QNativeGestureEvent qNGEvent;
    qNGEvent.gestureType = QNativeGestureEvent::Zoom;
    NSPoint p = [[event window] convertBaseToScreen:[event locationInWindow]];
    qNGEvent.position = flipPoint(p).toPoint();
    qNGEvent.percentage = [event magnification];
    qt_sendSpontaneousEvent(widgetToGetGesture, &qNGEvent);
#endif // QT_NO_GESTURES
}

- (void)rotateWithEvent:(NSEvent *)event
{
    QPoint qlocal, qglobal;
    QWidget *widgetToGetGesture = 0;
    qt_mac_getTargetForMouseEvent(event, QEvent::Gesture, qlocal, qglobal, qwidget, &widgetToGetGesture);
    if (!widgetToGetGesture)
        return;
    if (!QApplicationPrivate::tryModalHelper(widgetToGetGesture, 0))
        return;

#ifndef QT_NO_GESTURES
    QNativeGestureEvent qNGEvent;
    qNGEvent.gestureType = QNativeGestureEvent::Rotate;
    NSPoint p = [[event window] convertBaseToScreen:[event locationInWindow]];
    qNGEvent.position = flipPoint(p).toPoint();
    qNGEvent.percentage = -[event rotation];
    qt_sendSpontaneousEvent(widgetToGetGesture, &qNGEvent);
#endif // QT_NO_GESTURES
}

- (void)swipeWithEvent:(NSEvent *)event
{
    QPoint qlocal, qglobal;
    QWidget *widgetToGetGesture = 0;
    qt_mac_getTargetForMouseEvent(event, QEvent::Gesture, qlocal, qglobal, qwidget, &widgetToGetGesture);
    if (!widgetToGetGesture)
        return;
    if (!QApplicationPrivate::tryModalHelper(widgetToGetGesture, 0))
        return;

#ifndef QT_NO_GESTURES
    QNativeGestureEvent qNGEvent;
    qNGEvent.gestureType = QNativeGestureEvent::Swipe;
    NSPoint p = [[event window] convertBaseToScreen:[event locationInWindow]];
    qNGEvent.position = flipPoint(p).toPoint();
    if ([event deltaX] == 1)
        qNGEvent.angle = 180.0f;
    else if ([event deltaX] == -1)
        qNGEvent.angle = 0.0f;
    else if ([event deltaY] == 1)
        qNGEvent.angle = 90.0f;
    else if ([event deltaY] == -1)
        qNGEvent.angle = 270.0f;
    qt_sendSpontaneousEvent(widgetToGetGesture, &qNGEvent);
#endif // QT_NO_GESTURES
}

- (void)beginGestureWithEvent:(NSEvent *)event
{
    QPoint qlocal, qglobal;
    QWidget *widgetToGetGesture = 0;
    qt_mac_getTargetForMouseEvent(event, QEvent::Gesture, qlocal, qglobal, qwidget, &widgetToGetGesture);
    if (!widgetToGetGesture)
        return;
    if (!QApplicationPrivate::tryModalHelper(widgetToGetGesture, 0))
        return;

#ifndef QT_NO_GESTURES
    QNativeGestureEvent qNGEvent;
    qNGEvent.gestureType = QNativeGestureEvent::GestureBegin;
    NSPoint p = [[event window] convertBaseToScreen:[event locationInWindow]];
    qNGEvent.position = flipPoint(p).toPoint();
    qt_sendSpontaneousEvent(widgetToGetGesture, &qNGEvent);
#endif // QT_NO_GESTURES
}

- (void)endGestureWithEvent:(NSEvent *)event
{
    QPoint qlocal, qglobal;
    QWidget *widgetToGetGesture = 0;
    qt_mac_getTargetForMouseEvent(event, QEvent::Gesture, qlocal, qglobal, qwidget, &widgetToGetGesture);
    if (!widgetToGetGesture)
        return;
    if (!QApplicationPrivate::tryModalHelper(widgetToGetGesture, 0))
        return;

#ifndef QT_NO_GESTURES
    QNativeGestureEvent qNGEvent;
    qNGEvent.gestureType = QNativeGestureEvent::GestureEnd;
    NSPoint p = [[event window] convertBaseToScreen:[event locationInWindow]];
    qNGEvent.position = flipPoint(p).toPoint();
    qt_sendSpontaneousEvent(widgetToGetGesture, &qNGEvent);
}
#endif // QT_NO_GESTURES

- (void)frameDidChange:(NSNotification *)note
{
    Q_UNUSED(note);
    if (!qwidget)
        return;
    if (qwidget->isWindow())
        return;
    NSRect newFrame = [self frame];
    QRect newGeo(newFrame.origin.x, newFrame.origin.y, newFrame.size.width, newFrame.size.height);
    bool moved = qwidget->testAttribute(Qt::WA_Moved);
    bool resized = qwidget->testAttribute(Qt::WA_Resized);
    qwidget->setGeometry(newGeo);
    qwidget->setAttribute(Qt::WA_Moved, moved);
    qwidget->setAttribute(Qt::WA_Resized, resized);
    qwidgetprivate->syncCocoaMask();
}

- (BOOL)isEnabled
{
    if (!qwidget)
        return [super isEnabled];
    return [super isEnabled] && qwidget->isEnabled();
}

- (void)setEnabled:(BOOL)flag
{
    QMacCocoaAutoReleasePool pool;
    [super setEnabled:flag];
    if (qwidget && qwidget->isEnabled() != flag)
        qwidget->setEnabled(flag);
}

+ (Class)cellClass
{
    return [NSActionCell class];
}

- (BOOL)acceptsFirstResponder
{
    if (!qwidget)
        return NO;

    // Disabled widget shouldn't get focus even if it's a window.
    // hence disabled windows will not get any key or mouse events.
    if (!qwidget->isEnabled())
        return NO;

    if (qwidget->isWindow() && !qt_widget_private(qwidget)->topData()->embedded) {
        QWidget *focusWidget = qApp->focusWidget();
        if (!focusWidget) {
            // There is no focus widget, but we still want to receive key events
            // for shortcut handling etc. So we accept first responer for the
            // content view as a last resort:
            return YES;
        }
        if (!focusWidget->internalWinId() && focusWidget->nativeParentWidget() == qwidget) {
            // The current focus widget is alien, and hence, cannot get acceptsFirstResponder
            // calls. Since the focus widget is a child of qwidget, we let this view say YES:
            return YES;
        }
        if (focusWidget->window() != qwidget) {
            // The current focus widget is in another window. Since cocoa
            // suggest that this window should be key now, we accept:
            return YES;
        }
    }

    return qwidget->focusPolicy() != Qt::NoFocus;
}

- (BOOL)resignFirstResponder
{
    if (!qwidget)
        return YES;

    // Seems like the following test only triggers if this
    // view is inside a QMacNativeWidget:
//    if (QWidget *fw = QApplication::focusWidget()) {
//        if (qwidget == fw || qwidget == fw->nativeParentWidget())
//            fw->clearFocus();
//    }
    return YES;
}

- (BOOL)becomeFirstResponder
{
    // see the comment in the acceptsFirstResponder - if the window "stole" focus
    // let it become the responder, but don't tell Qt
    if (qwidget && qt_widget_private(qwidget->window())->topData()->embedded
        && !QApplication::focusWidget() && qwidget->focusPolicy() != Qt::NoFocus)
        qwidget->setFocus(Qt::OtherFocusReason);
    return YES;
}

- (NSDragOperation)draggingSourceOperationMaskForLocal:(BOOL)isLocal
{
    Q_UNUSED(isLocal);
    return supportedActions;
}

- (void)setSupportedActions:(NSDragOperation)actions
{
    supportedActions = actions;
}

- (void)draggedImage:(NSImage *)anImage endedAt:(NSPoint)aPoint operation:(NSDragOperation)operation
{
    Q_UNUSED(anImage);
    Q_UNUSED(aPoint);
    macCurrentDnDParameters()->performedAction = operation;
    if (QDragManager::self()->object
        && QDragManager::self()->dragPrivate()->executed_action != Qt::ActionMask) {
        macCurrentDnDParameters()->performedAction =
            qt_mac_mapDropAction(QDragManager::self()->dragPrivate()->executed_action);
    }
}

- (QWidget *)qt_qwidget
{
    return qwidget;
}

- (void) qt_clearQWidget
{
    qwidget = 0;
    qwidgetprivate = 0;
}

- (void)keyDown:(NSEvent *)theEvent
{
    if (!qwidget)
        return;
    QWidget *widgetToGetKey = qt_mac_getTargetForKeyEvent(qwidget);
    if (!widgetToGetKey)
        return;

    sendKeyEvents = true;

    if (widgetToGetKey->testAttribute(Qt::WA_InputMethodEnabled)
            && !(widgetToGetKey->inputMethodHints() & Qt::ImhDigitsOnly
                 || widgetToGetKey->inputMethodHints() & Qt::ImhFormattedNumbersOnly
                 || widgetToGetKey->inputMethodHints() & Qt::ImhHiddenText)) {
        fromKeyDownEvent = true;
        [qt_mac_nativeview_for(qwidget) interpretKeyEvents:[NSArray arrayWithObject: theEvent]];
        fromKeyDownEvent = false;
    }

    if (sendKeyEvents && !composing) {
        bool keyEventEaten = qt_dispatchKeyEvent(theEvent, widgetToGetKey);
        if (!keyEventEaten && qwidget) {
            // The event is not yet eaten, and if Qt is embedded inside a native
            // cocoa application, send it to first responder not owned by Qt.
            // The exception is if widgetToGetKey was redirected to a popup.
            QWidget *toplevel = qwidget->window();
            if (toplevel == widgetToGetKey->window()) {
                if (qt_widget_private(toplevel)->topData()->embedded) {
                    if (NSResponder *w = [qt_mac_nativeview_for(toplevel) superview])
                        [w keyDown:theEvent];
                }
            }
        }
    }
}


- (void)keyUp:(NSEvent *)theEvent
{
    if (sendKeyEvents) {
        QWidget *widgetToGetKey = qt_mac_getTargetForKeyEvent(qwidget);
        if (!widgetToGetKey)
            return;

        bool keyEventEaten = qt_dispatchKeyEvent(theEvent, widgetToGetKey);
        if (!keyEventEaten && qwidget) {
            // The event is not yet eaten, and if Qt is embedded inside a native
            // cocoa application, send it to first responder not owned by Qt.
            // The exception is if widgetToGetKey was redirected to a popup.
            QWidget *toplevel = qwidget->window();
            if (toplevel == widgetToGetKey->window()) {
                if (qt_widget_private(toplevel)->topData()->embedded) {
                    if (NSResponder *w = [qt_mac_nativeview_for(toplevel) superview])
                        [w keyUp:theEvent];
                }
            }
        }
    }
}

- (void)viewWillMoveToWindow:(NSWindow *)window
{
    if (qwidget == 0)
        return;

    if (qwidget->windowFlags() & Qt::MSWindowsOwnDC
          && (window != [self window])) { // OpenGL Widget
        QEvent event(QEvent::MacGLClearDrawable);
        qApp->sendEvent(qwidget, &event);
    }
}

- (void)viewDidMoveToWindow
{
    if (qwidget == 0)
        return;

    if (qwidget->windowFlags() & Qt::MSWindowsOwnDC && [self window]) {
        // call update paint event
        qwidgetprivate->needWindowChange = true;
        QEvent event(QEvent::MacGLWindowChange);
        qApp->sendEvent(qwidget, &event);
    }
}


// NSTextInput Protocol implementation

- (void) insertText:(id)aString
{
    QString commitText;
    if ([aString length]) {
        if ([aString isKindOfClass:[NSAttributedString class]]) {
            commitText = QCFString::toQString(reinterpret_cast<CFStringRef>([aString string]));
        } else {
            commitText = QCFString::toQString(reinterpret_cast<CFStringRef>(aString));
        };
    }

    // When entering characters through Character Viewer or Keyboard Viewer, the text is passed
    // through this insertText method. Since we dont receive a keyDown Event in such cases, the
    // composing flag will be false.
    //
    // Characters can be sent through input method directly without composing process as well,
    // for instance a Chinese input method will send "，" (U+FF0C) to insertText: when "," key
    // is pressed. In that case we want to set commit string directly instead of going through
    // key events handling again. Hence we only leave the string with Unicode value less than
    // 256 to the key events handling process.
    if (([aString length] && (composing || commitText.at(0).unicode() > 0xff)) || !fromKeyDownEvent) {
        // Send the commit string to the widget.
        QInputMethodEvent e;
        e.setCommitString(commitText);
        QWidget *widgetToGetKey = 0;
        if (!composing || qApp->focusWidget())
            widgetToGetKey = qt_mac_getTargetForKeyEvent(qwidget);
        else if (QMacInputContext *mic = qobject_cast<QMacInputContext *>(qApp->inputContext()))
            widgetToGetKey = mic->lastFocusWidget();
        if (widgetToGetKey)
            qt_sendSpontaneousEvent(widgetToGetKey, &e);
        composing = false;
        sendKeyEvents = false;
    } else {
        // The key sequence "`q" on a French Keyboard will generate two calls to insertText before
        // it returns from interpretKeyEvents. The first call will turn off 'composing' and accept
        // the "`" key. The last keyDown event needs to be processed by the widget to get the
        // character "q". The string parameter is ignored for the second call.
        sendKeyEvents = true;
    }

    composingText->clear();
}

- (void) setMarkedText:(id)aString selectedRange:(NSRange)selRange
{
    // Generate the QInputMethodEvent with preedit string and the attributes
    // for rendering it. The attributes handled here are 'underline',
    // 'underline color' and 'cursor position'.
    sendKeyEvents = false;
    composing = true;
    QString qtText;
    // Cursor position is retrived from the range.
    QList<QInputMethodEvent::Attribute> attrs;
    attrs<<QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, selRange.location + selRange.length, 1, QVariant());
    if ([aString isKindOfClass:[NSAttributedString class]]) {
        qtText = QCFString::toQString(reinterpret_cast<CFStringRef>([aString string]));
        composingLength = qtText.length();
        int index = 0;
        // Create attributes for individual sections of preedit text
        while (index < composingLength) {
            NSRange effectiveRange;
            NSRange range = NSMakeRange(index, composingLength-index);
            NSDictionary *attributes = [aString attributesAtIndex:index
                                            longestEffectiveRange:&effectiveRange
                                                          inRange:range];
            NSNumber *underlineStyle = [attributes objectForKey:NSUnderlineStyleAttributeName];
            if (underlineStyle) {
                QColor clr (Qt::black);
                NSColor *color = [attributes objectForKey:NSUnderlineColorAttributeName];
                if (color) {
                    clr = colorFrom(color);
                }
                QTextCharFormat format;
                format.setFontUnderline(true);
                format.setUnderlineColor(clr);
                attrs<<QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
                                                    effectiveRange.location,
                                                    effectiveRange.length,
                                                    format);
            }
            index = effectiveRange.location + effectiveRange.length;
        }
    } else {
        // No attributes specified, take only the preedit text.
        qtText = QCFString::toQString(reinterpret_cast<CFStringRef>(aString));
        composingLength = qtText.length();
    }
    // Make sure that we have at least one text format.
    if (attrs.size() <= 1) {
        QTextCharFormat format;
        format.setFontUnderline(true);
        attrs<<QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
                                            0, composingLength, format);
    }
    *composingText = qtText;

    QInputMethodEvent e(qtText, attrs);
    if (QWidget *widgetToGetKey = qt_mac_getTargetForKeyEvent(qwidget))
        qt_sendSpontaneousEvent(widgetToGetKey, &e);

    if (!composingLength)
        composing = false;
}

- (void) unmarkText
{
    if (composing) {
        QInputMethodEvent e;
        e.setCommitString(*composingText);
        if (QWidget *widgetToGetKey = qt_mac_getTargetForKeyEvent(qwidget))
            qt_sendSpontaneousEvent(widgetToGetKey, &e);
    }
    composingText->clear();
    composing = false;
}

- (BOOL) hasMarkedText
{
    return (composing ? YES: NO);
}

- (void) doCommandBySelector:(SEL)aSelector
{
    Q_UNUSED(aSelector);
}

- (BOOL)isComposing
{
    return composing;
}

- (NSInteger) conversationIdentifier
{
    // Return a unique identifier fot this ime conversation
    return (NSInteger)self;
}

- (NSAttributedString *) attributedSubstringFromRange:(NSRange)theRange
{
    QString selectedText(qwidget->inputMethodQuery(Qt::ImCurrentSelection).toString());
    if (!selectedText.isEmpty()) {
        QCFString string(selectedText.mid(theRange.location, theRange.length));
        const NSString *tmpString = reinterpret_cast<const NSString *>((CFStringRef)string);
        return [[[NSAttributedString alloc]  initWithString:const_cast<NSString *>(tmpString)] autorelease];
    } else {
        return nil;
    }
}

- (NSRange) markedRange
{
    NSRange range;
    if (composing) {
        range.location = 0;
        range.length = composingLength;
    } else {
        range.location = NSNotFound;
        range.length = 0;
    }
    return range;
}

- (NSRange) selectedRange
{
    NSRange selRange;
    QString selectedText(qwidget->inputMethodQuery(Qt::ImCurrentSelection).toString());
    if (!selectedText.isEmpty()) {
        // Consider only the selected text.
        selRange.location = 0;
        selRange.length = selectedText.length();
    } else {
        // No selected text.
        selRange.location = NSNotFound;
        selRange.length = 0;
    }
    return selRange;

}

- (NSRect) firstRectForCharacterRange:(NSRange)theRange
{
    Q_UNUSED(theRange);
    // The returned rect is always based on the internal cursor.
    QWidget *widgetToGetKey = qt_mac_getTargetForKeyEvent(qwidget);
    if (!widgetToGetKey)
        return NSZeroRect;

    QRect mr(widgetToGetKey->inputMethodQuery(Qt::ImMicroFocus).toRect());
    QPoint mp(widgetToGetKey->mapToGlobal(QPoint(mr.bottomLeft())));
    NSRect rect ;
    rect.origin.x = mp.x();
    rect.origin.y = flipYCoordinate(mp.y());
    rect.size.width = mr.width();
    rect.size.height = mr.height();
    return rect;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)thePoint
{
    // We dont support cursor movements using mouse while composing.
    Q_UNUSED(thePoint);
    return NSNotFound;
}

- (NSArray*) validAttributesForMarkedText
{
    QWidget *widgetToGetKey = qt_mac_getTargetForKeyEvent(qwidget);
    if (!widgetToGetKey)
        return nil;

    if (!widgetToGetKey->testAttribute(Qt::WA_InputMethodEnabled))
        return nil;  // Not sure if that's correct, but it's saves a malloc.

    // Support only underline color/style.
    return [NSArray arrayWithObjects:NSUnderlineColorAttributeName,
                                     NSUnderlineStyleAttributeName, nil];
}
@end

QT_BEGIN_NAMESPACE
void QMacInputContext::reset()
{
    QWidget *w = QInputContext::focusWidget();
    if (w) {
        NSView *view = qt_mac_effectiveview_for(w);
        if ([view isKindOfClass:[QT_MANGLE_NAMESPACE(QCocoaView) class]]) {
            QMacCocoaAutoReleasePool pool;
            QT_MANGLE_NAMESPACE(QCocoaView) *qc = static_cast<QT_MANGLE_NAMESPACE(QCocoaView) *>(view);
            NSInputManager *currentIManager = [NSInputManager currentInputManager];
            if (currentIManager) {
                [currentIManager markedTextAbandoned:view];
                [qc unmarkText];
            }
        }
    }
}

bool QMacInputContext::isComposing() const
{
    QWidget *w = QInputContext::focusWidget();
    if (w) {
        NSView *view = qt_mac_effectiveview_for(w);
        if ([view isKindOfClass:[QT_MANGLE_NAMESPACE(QCocoaView) class]]) {
            return [static_cast<QT_MANGLE_NAMESPACE(QCocoaView) *>(view) isComposing];
        }
    }
    return false;
}

extern bool qt_mac_in_drag;
void * /*NSImage */qt_mac_create_nsimage(const QPixmap &pm);
static const int default_pm_hotx = -2;
static const int default_pm_hoty = -16;
static const char* default_pm[] = {
    "13 9 3 1",
    ".      c None",
    "       c #000000",
    "X      c #FFFFFF",
    "X X X X X X X",
    " X X X X X X ",
    "X ......... X",
    " X.........X ",
    "X ......... X",
    " X.........X ",
    "X ......... X",
    " X X X X X X ",
    "X X X X X X X",
};

Qt::DropAction QDragManager::drag(QDrag *o)
{
    if(qt_mac_in_drag) {     //just make sure..
        qWarning("Qt: Internal error: WH0A, unexpected condition reached");
        return Qt::IgnoreAction;
    }
    if(object == o)
        return Qt::IgnoreAction;
    /* At the moment it seems clear that Mac OS X does not want to drag with a non-left button
     so we just bail early to prevent it */
    if(!(GetCurrentEventButtonState() & kEventMouseButtonPrimary))
        return Qt::IgnoreAction;

    if(object) {
        dragPrivate()->source->removeEventFilter(this);
        cancel();
        beingCancelled = false;
    }

    object = o;
    dragPrivate()->target = 0;

#ifndef QT_NO_ACCESSIBILITY
    QAccessible::updateAccessibility(this, 0, QAccessible::DragDropStart);
#endif

    // setup the data
    QMacPasteboard dragBoard((CFStringRef) NSDragPboard, QMacPasteboardMime::MIME_DND);
    dragPrivate()->data->setData(QLatin1String("application/x-qt-mime-type-name"), QByteArray("dummy"));
    dragBoard.setMimeData(dragPrivate()->data);

    // create the image
    QPoint hotspot;
    QPixmap pix = dragPrivate()->pixmap;
    if(pix.isNull()) {
        if(dragPrivate()->data->hasText() || dragPrivate()->data->hasUrls()) {
            // get the string
            QString s = dragPrivate()->data->hasText() ? dragPrivate()->data->text()
            : dragPrivate()->data->urls().first().toString();
            if(s.length() > 26)
                s = s.left(23) + QChar(0x2026);
            if(!s.isEmpty()) {
                // draw it
                QFont f(qApp->font());
                f.setPointSize(12);
                QFontMetrics fm(f);
                QPixmap tmp(fm.width(s), fm.height());
                if(!tmp.isNull()) {
                    QPainter p(&tmp);
                    p.fillRect(0, 0, tmp.width(), tmp.height(), Qt::color0);
                    p.setPen(Qt::color1);
                    p.setFont(f);
                    p.drawText(0, fm.ascent(), s);
                    // save it
                    pix = tmp;
                    hotspot = QPoint(tmp.width() / 2, tmp.height() / 2);
                }
            }
        } else {
            pix = QPixmap(default_pm);
            hotspot = QPoint(default_pm_hotx, default_pm_hoty);
        }
    } else {
        hotspot = dragPrivate()->hotspot;
    }

    // Convert the image to NSImage:
    NSImage *image = (NSImage *)qt_mac_create_nsimage(pix);
    [image retain];

    DnDParams *dndParams = macCurrentDnDParameters();
    QT_MANGLE_NAMESPACE(QCocoaView) *theView = static_cast<QT_MANGLE_NAMESPACE(QCocoaView) *>(dndParams->view);

    // Save supported actions:
    [theView setSupportedActions: qt_mac_mapDropActions(dragPrivate()->possible_actions)];
    QPoint pointInView = [theView qt_qwidget]->mapFromGlobal(dndParams->globalPoint);
    NSPoint imageLoc = {pointInView.x() - hotspot.x(), pointInView.y() + pix.height() - hotspot.y()};
    NSSize mouseOffset = {0.0, 0.0};
    NSPasteboard *pboard = [NSPasteboard pasteboardWithName:NSDragPboard];
    dragPrivate()->executed_action = Qt::ActionMask;

    // Execute the drag:
    [theView retain];
    [theView dragImage:image
        at:imageLoc
        offset:mouseOffset
        event:dndParams->theEvent
        pasteboard:pboard
        source:theView
        slideBack:YES];

    // Reset the implicit grab widget when drag ends because we will not
    // receive the mouse release event when DND is active:
    qt_button_down = 0;
    [theView release];
    [image release];
    if (dragPrivate())
        dragPrivate()->executed_action = Qt::IgnoreAction;
    object = 0;
    Qt::DropAction performedAction(qt_mac_mapNSDragOperation(dndParams->performedAction));

    // Do post drag processing, if required.
    if (performedAction != Qt::IgnoreAction) {
        // Check if the receiver points us to a file location.
        // if so, we need to do the file copy/move ourselves.
        QCFType<CFURLRef> pasteLocation = 0;
        PasteboardCopyPasteLocation(dragBoard.pasteBoard(), &pasteLocation);
        if (pasteLocation) {
            QList<QUrl> urls = o->mimeData()->urls();
            for (int i = 0; i < urls.size(); ++i) {
                QUrl fromUrl = urls.at(i);
                QString filename = QFileInfo(fromUrl.path()).fileName();
                QUrl toUrl(QCFString::toQString(CFURLGetString(pasteLocation)) + filename);
                if (performedAction == Qt::MoveAction)
                    QFile::rename(fromUrl.path(), toUrl.path());
                else if (performedAction == Qt::CopyAction)
                    QFile::copy(fromUrl.path(), toUrl.path());
            }
        }
    }

    // Clean-up:
    o->setMimeData(0);
    o->deleteLater();
    return performedAction;
}

QT_END_NAMESPACE

#endif // QT_MAC_USE_COCOA
