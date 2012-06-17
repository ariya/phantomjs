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

#include <private/qt_mac_p.h>
#include <private/qeventdispatcher_mac_p.h>

#include "qapplication.h"
#include "qapplication_p.h"
#include "qbitmap.h"
#include "qcursor.h"
#include "qdesktopwidget.h"
#include "qevent.h"
#include "qfileinfo.h"
#include "qimage.h"
#include "qlayout.h"
#include "qmenubar.h"
#include <private/qbackingstore_p.h>
#include <private/qwindowsurface_mac_p.h>
#include <private/qpaintengine_mac_p.h>
#include "qpainter.h"
#include "qstyle.h"
#include "qtimer.h"
#include "qfocusframe.h"
#include "qdebug.h"
#include <private/qmainwindowlayout_p.h>

#include <private/qabstractscrollarea_p.h>
#include <qabstractscrollarea.h>
#include <ApplicationServices/ApplicationServices.h>
#include <limits.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#include <private/qcocoaview_mac_p.h>
#include <private/qcocoawindow_mac_p.h>
#include <private/qcocoawindowdelegate_mac_p.h>
#include <private/qcocoapanel_mac_p.h>

#include "qwidget_p.h"
#include "qevent_p.h"
#include "qdnd_p.h"
#include <QtGui/qgraphicsproxywidget.h>
#include "qmainwindow.h"

QT_BEGIN_NAMESPACE

// qmainwindow.cpp
extern QMainWindowLayout *qt_mainwindow_layout(const QMainWindow *window);

#define XCOORD_MAX 16383
#define WRECT_MAX 8191

#ifndef QT_MAC_USE_COCOA

extern "C" {
    extern OSStatus _HIViewScrollRectWithOptions(HIViewRef, const HIRect *, CGFloat, CGFloat,
                                                 OptionBits) __attribute__ ((weak));
}
#define kHIViewScrollRectAdjustInvalid 1
#define kHIViewScrollRectDontInvalidateRevealedArea 2
#endif


/*****************************************************************************
  QWidget debug facilities
 *****************************************************************************/
//#define DEBUG_WINDOW_RGNS
//#define DEBUG_WINDOW_CREATE
//#define DEBUG_WINDOW_STATE
//#define DEBUG_WIDGET_PAINT

/*****************************************************************************
  QWidget globals
 *****************************************************************************/
#ifndef QT_MAC_USE_COCOA
typedef QHash<Qt::WindowFlags, WindowGroupRef> WindowGroupHash;
Q_GLOBAL_STATIC(WindowGroupHash, qt_mac_window_groups)
const UInt32 kWidgetCreatorQt = kEventClassQt;
enum {
    kWidgetPropertyQWidget = 'QWId' //QWidget *
};
#endif

static bool qt_mac_raise_process = true;
static OSWindowRef qt_root_win = 0;
QWidget *mac_mouse_grabber = 0;
QWidget *mac_keyboard_grabber = 0;

#ifndef QT_MAC_USE_COCOA
#ifdef QT_NAMESPACE

// produce the string "com.trolltech.qt-namespace.widget", where "namespace" is the contents of QT_NAMESPACE.
#define SS(x) #x
#define S0(x) SS(x)
#define S "com.trolltech.qt-" S0(QT_NAMESPACE) ".widget"

static CFStringRef kObjectQWidget = CFSTR(S);

#undef SS
#undef S0
#undef S

#else
static CFStringRef kObjectQWidget = CFSTR("com.trolltech.qt.widget");
#endif // QT_NAMESPACE
#endif // QT_MAC_USE_COCOA

/*****************************************************************************
  Externals
 *****************************************************************************/
extern QPointer<QWidget> qt_button_down; //qapplication_mac.cpp
extern QWidget *qt_mac_modal_blocked(QWidget *); //qapplication_mac.mm
extern void qt_event_request_activate(QWidget *); //qapplication_mac.mm
extern bool qt_event_remove_activate(); //qapplication_mac.mm
extern void qt_mac_event_release(QWidget *w); //qapplication_mac.mm
extern void qt_event_request_showsheet(QWidget *); //qapplication_mac.mm
extern void qt_event_request_window_change(QWidget *); //qapplication_mac.mm
extern QPointer<QWidget> qt_last_mouse_receiver; //qapplication_mac.mm
extern QPointer<QWidget> qt_last_native_mouse_receiver; //qt_cocoa_helpers_mac.mm
extern IconRef qt_mac_create_iconref(const QPixmap &); //qpixmap_mac.cpp
extern void qt_mac_set_cursor(const QCursor *, const QPoint &); //qcursor_mac.mm
extern void qt_mac_update_cursor(); //qcursor_mac.mm
extern bool qt_nograb();
extern CGImageRef qt_mac_create_cgimage(const QPixmap &, bool); //qpixmap_mac.cpp
extern RgnHandle qt_mac_get_rgn(); //qregion_mac.cpp
extern QRegion qt_mac_convert_mac_region(RgnHandle rgn); //qregion_mac.cpp
extern void qt_mac_setMouseGrabCursor(bool set, QCursor *cursor = 0); // qcursor_mac.mm
extern QPointer<QWidget> topLevelAt_cache; // qapplication_mac.mm
/*****************************************************************************
  QWidget utility functions
 *****************************************************************************/
void Q_GUI_EXPORT qt_mac_set_raise_process(bool b) { qt_mac_raise_process = b; }
static QSize qt_mac_desktopSize()
{
    int w = 0, h = 0;
    CGDisplayCount cg_count;
    CGGetActiveDisplayList(0, 0, &cg_count);
    QVector<CGDirectDisplayID> displays(cg_count);
    CGGetActiveDisplayList(cg_count, displays.data(), &cg_count);
    Q_ASSERT(cg_count == (CGDisplayCount)displays.size());
    for(int i = 0; i < (int)cg_count; ++i) {
        CGRect r = CGDisplayBounds(displays.at(i));
        w = qMax<int>(w, qRound(r.origin.x + r.size.width));
        h = qMax<int>(h, qRound(r.origin.y + r.size.height));
    }
    return QSize(w, h);
}

#ifdef QT_MAC_USE_COCOA
static NSDrawer *qt_mac_drawer_for(const QWidget *widget)
{
    NSView *widgetView = reinterpret_cast<NSView *>(widget->window()->effectiveWinId());
    NSArray *windows = [NSApp windows];
    for (NSWindow *window in windows) {
        NSArray *drawers = [window drawers];
        for (NSDrawer *drawer in drawers) {
            if ([drawer contentView] == widgetView)
                return drawer;
        }
    }
    return 0;
}
#endif

static void qt_mac_destructView(OSViewRef view)
{
#ifdef QT_MAC_USE_COCOA
    NSWindow *window = [view window];
    if ([window contentView] == view) {
        NSView* newView = [[NSView alloc] initWithFrame:[view bounds]];
        [window setContentView:newView];
        [newView release];
    } else {
        [view removeFromSuperview];
        [view release];
    }
#else
    HIViewRemoveFromSuperview(view);
    CFRelease(view);
#endif
}

static void qt_mac_destructWindow(OSWindowRef window)
{
#ifdef QT_MAC_USE_COCOA
    if ([window isVisible] && [window isSheet]){
        [NSApp endSheet:window];
        [window orderOut:window];
    }

    [[QT_MANGLE_NAMESPACE(QCocoaWindowDelegate) sharedDelegate] resignDelegateForWindow:window];
    [window release];
#else
    // Remove property to clean up memory:
    RemoveWindowProperty(window, kWidgetCreatorQt, kWidgetPropertyQWidget);
    CFRelease(window);
#endif
}

static void qt_mac_destructDrawer(NSDrawer *drawer)
{
#ifdef QT_MAC_USE_COCOA
    [[QT_MANGLE_NAMESPACE(QCocoaWindowDelegate) sharedDelegate] resignDelegateForDrawer:drawer];
    [drawer release];
#else
    Q_UNUSED(drawer);
#endif
}

bool qt_mac_can_clickThrough(const QWidget *w)
{
    static int qt_mac_carbon_clickthrough = -1;
    if (qt_mac_carbon_clickthrough < 0)
        qt_mac_carbon_clickthrough = !qgetenv("QT_MAC_NO_COCOA_CLICKTHROUGH").isEmpty();
    bool ret = !qt_mac_carbon_clickthrough;
    for ( ; w; w = w->parentWidget()) {
        if (w->testAttribute(Qt::WA_MacNoClickThrough)) {
            ret = false;
            break;
        }
    }
    return ret;
}

bool qt_mac_is_macsheet(const QWidget *w)
{
    if (!w)
        return false;

    Qt::WindowModality modality = w->windowModality();
    if (modality == Qt::ApplicationModal)
        return false;
    return w->parentWidget() && (modality == Qt::WindowModal || w->windowType() == Qt::Sheet);
}

bool qt_mac_is_macdrawer(const QWidget *w)
{
    return (w && w->parentWidget() && w->windowType() == Qt::Drawer);
}

bool qt_mac_insideKeyWindow(const QWidget *w)
{
#ifdef QT_MAC_USE_COCOA
    return [[reinterpret_cast<NSView *>(w->effectiveWinId()) window] isKeyWindow];
#else
    Q_UNUSED(w);
#endif
    return false;
}

bool qt_mac_set_drawer_preferred_edge(QWidget *w, Qt::DockWidgetArea where) //users of Qt for Mac OS X can use this..
{
    if(!qt_mac_is_macdrawer(w))
        return false;

#if QT_MAC_USE_COCOA
    NSDrawer *drawer = qt_mac_drawer_for(w);
    if (!drawer)
        return false;
	NSRectEdge	edge;
    if (where & Qt::LeftDockWidgetArea)
        edge = NSMinXEdge;
    else if (where & Qt::RightDockWidgetArea)
        edge = NSMaxXEdge;
    else if (where & Qt::TopDockWidgetArea)
		edge = NSMaxYEdge;
    else if (where & Qt::BottomDockWidgetArea)
        edge = NSMinYEdge;
    else
        return false;

    if (edge == [drawer preferredEdge]) //no-op
        return false;

    if (w->isVisible()) {
	    [drawer close];
	    [drawer openOnEdge:edge];
	}
	[drawer setPreferredEdge:edge];
#else
    OSWindowRef window = qt_mac_window_for(w);
    OptionBits edge;
    if(where & Qt::LeftDockWidgetArea)
        edge = kWindowEdgeLeft;
    else if(where & Qt::RightDockWidgetArea)
        edge = kWindowEdgeRight;
    else if(where & Qt::TopDockWidgetArea)
        edge = kWindowEdgeTop;
    else if(where & Qt::BottomDockWidgetArea)
        edge = kWindowEdgeBottom;
    else
        return false;

    if(edge == GetDrawerPreferredEdge(window)) //no-op
        return false;

    //do it
    SetDrawerPreferredEdge(window, edge);
    if(w->isVisible()) {
        CloseDrawer(window, false);
        OpenDrawer(window, edge, true);
    }
#endif
    return true;
}

#ifndef QT_MAC_USE_COCOA
Q_GUI_EXPORT
#endif
QPoint qt_mac_posInWindow(const QWidget *w)
{
    QPoint ret = w->data->wrect.topLeft();
    while(w && !w->isWindow()) {
        ret += w->pos();
        w =  w->parentWidget();
    }
    return ret;
}

//find a QWidget from a OSWindowRef
QWidget *qt_mac_find_window(OSWindowRef window)
{
#ifdef QT_MAC_USE_COCOA
    return [window QT_MANGLE_NAMESPACE(qt_qwidget)];
#else
    if(!window)
        return 0;

    QWidget *ret;
    if(GetWindowProperty(window, kWidgetCreatorQt, kWidgetPropertyQWidget, sizeof(ret), 0, &ret) == noErr)
        return ret;
    return 0;
#endif
}

inline static void qt_mac_set_fullscreen_mode(bool b)
{
    extern bool qt_mac_app_fullscreen; //qapplication_mac.mm
    if(qt_mac_app_fullscreen == b)
        return;
    qt_mac_app_fullscreen = b;
    if (b) {
        SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);
    } else {
        SetSystemUIMode(kUIModeNormal, 0);
    }
}

Q_GUI_EXPORT OSViewRef qt_mac_nativeview_for(const QWidget *w)
{
    return reinterpret_cast<OSViewRef>(w->internalWinId());
}

Q_GUI_EXPORT OSViewRef qt_mac_effectiveview_for(const QWidget *w)
{
    // Get the first non-alien (parent) widget for
    // w, and return its NSView (if it has one):
    return reinterpret_cast<OSViewRef>(w->effectiveWinId());
}

Q_GUI_EXPORT OSViewRef qt_mac_get_contentview_for(OSWindowRef w)
{
#ifdef QT_MAC_USE_COCOA
    return [w contentView];
#else
    HIViewRef contentView = 0;
    OSStatus err = GetRootControl(w, &contentView);  // Returns the window's content view (Apple QA1214)
    if (err == errUnknownControl) {
        contentView = HIViewGetRoot(w);
    } else if (err != noErr) {
        qWarning("Qt:Could not get content or root view of window! %s:%d [%ld]",
                 __FILE__, __LINE__, err);
    }
    return contentView;
#endif
}

bool qt_mac_sendMacEventToWidget(QWidget *widget, EventRef ref)
{
    return widget->macEvent(0, ref);
}

Q_GUI_EXPORT OSWindowRef qt_mac_window_for(OSViewRef view)
{
#ifdef QT_MAC_USE_COCOA
    if (view)
        return [view window];
    return 0;
#else
    return HIViewGetWindow(view);
#endif
}

static bool qt_isGenuineQWidget(OSViewRef ref)
{
#ifdef QT_MAC_USE_COCOA
    return [ref isKindOfClass:[QT_MANGLE_NAMESPACE(QCocoaView) class]];
#else
    return HIObjectIsOfClass(HIObjectRef(ref), kObjectQWidget);
#endif
}

bool qt_isGenuineQWidget(const QWidget *window)
{
    if (!window)
        return false;

    if (!window->internalWinId())
        return true;  //alien

    return qt_isGenuineQWidget(OSViewRef(window->internalWinId()));
}

Q_GUI_EXPORT OSWindowRef qt_mac_window_for(const QWidget *w)
{
    if (OSViewRef hiview = qt_mac_effectiveview_for(w)) {
        OSWindowRef window = qt_mac_window_for(hiview);
        if (window)
            return window;

        if (qt_isGenuineQWidget(hiview)) {
            // This is a workaround for NSToolbar. When a widget is hidden
            // by clicking the toolbar button, Cocoa reparents the widgets
            // to another window (but Qt doesn't know about it).
            // When we start showing them, it reparents back,
            // but at this point it's window is nil, but the window it's being brought
            // into (the Qt one) is for sure created.
            // This stops the hierarchy moving under our feet.
            QWidget *toplevel = w->window();
            if (toplevel != w) {
                hiview = qt_mac_nativeview_for(toplevel);
                if (OSWindowRef w = qt_mac_window_for(hiview))
                    return w;
            }

            toplevel->d_func()->createWindow_sys();
            // Reget the hiview since "create window" could potentially move the view (I guess).
            hiview = qt_mac_nativeview_for(toplevel);
            return qt_mac_window_for(hiview);
        }
    }
    return 0;
}

#ifndef QT_MAC_USE_COCOA
/*  Checks if the current group is a 'stay on top' group. If so, the
    group gets removed from the hash table */
static void qt_mac_release_stays_on_top_group(WindowGroupRef group)
{
    for (WindowGroupHash::iterator it = qt_mac_window_groups()->begin(); it != qt_mac_window_groups()->end(); ++it) {
        if (it.value() == group) {
            qt_mac_window_groups()->remove(it.key());
            return;
        }
    }
}

/* Use this function instead of ReleaseWindowGroup, this will be sure to release the
   stays on top window group (created with qt_mac_get_stays_on_top_group below) */
static void qt_mac_release_window_group(WindowGroupRef group)
{
    ReleaseWindowGroup(group);
    if (GetWindowGroupRetainCount(group) == 0)
        qt_mac_release_stays_on_top_group(group);
}
#define ReleaseWindowGroup(x) Are you sure you wanted to do that? (you wanted qt_mac_release_window_group)

SInt32 qt_mac_get_group_level(WindowClass wclass)
{
    SInt32 group_level;
    CGWindowLevel tmpLevel;
    GetWindowGroupLevelOfType(GetWindowGroupOfClass(wclass), kWindowGroupLevelActive, &tmpLevel);
    group_level = tmpLevel;
    return group_level;
}
#endif

#ifndef QT_MAC_USE_COCOA
static void qt_mac_set_window_group(OSWindowRef window, Qt::WindowFlags flags, int level)
{
    WindowGroupRef group = 0;
    if (qt_mac_window_groups()->contains(flags)) {
        group = qt_mac_window_groups()->value(flags);
        RetainWindowGroup(group);
    } else {
        CreateWindowGroup(kWindowActivationScopeNone, &group);
        SetWindowGroupLevel(group, level);
        SetWindowGroupParent(group, GetWindowGroupOfClass(kAllWindowClasses));
        qt_mac_window_groups()->insert(flags, group);
    }
    SetWindowGroup(window, group);
}

inline static void qt_mac_set_window_group_to_stays_on_top(OSWindowRef window, Qt::WindowType type)
{
    // We create one static stays on top window group so that
    // all stays on top (aka popups) will fall into the same
    // group and be able to be raise()'d with releation to one another (from
    // within the same window group).
    qt_mac_set_window_group(window, type|Qt::WindowStaysOnTopHint, qt_mac_get_group_level(kOverlayWindowClass));
}

inline static void qt_mac_set_window_group_to_tooltip(OSWindowRef window)
{
    // Since new groups are created for 'stays on top' windows, the
    // same must be done for tooltips. Otherwise, tooltips would be drawn
    // below 'stays on top' widgets even tough they are on the same level.
    // Also, add 'two' to the group level to make sure they also get on top of popups.
    qt_mac_set_window_group(window, Qt::ToolTip, qt_mac_get_group_level(kHelpWindowClass)+2);
}

inline static void qt_mac_set_window_group_to_popup(OSWindowRef window)
{
    // In Qt, a popup is seen as a 'stay on top' window.
    // Since new groups are created for 'stays on top' windows, the
    // same must be done for popups. Otherwise, popups would be drawn
    // below 'stays on top' windows. Add 1 to get above pure stay-on-top windows.
    qt_mac_set_window_group(window, Qt::Popup, qt_mac_get_group_level(kOverlayWindowClass)+1);
}
#endif

inline static bool updateRedirectedToGraphicsProxyWidget(QWidget *widget, const QRect &rect)
{
    if (!widget)
        return false;

#ifndef QT_NO_GRAPHICSVIEW
    QWidget *tlw = widget->window();
    QWExtra *extra = qt_widget_private(tlw)->extra;
    if (extra && extra->proxyWidget) {
        extra->proxyWidget->update(rect.translated(widget->mapTo(tlw, QPoint())));
        return true;
    }
#endif

    return false;
}

inline static bool updateRedirectedToGraphicsProxyWidget(QWidget *widget, const QRegion &rgn)
{
    if (!widget)
        return false;

#ifndef QT_NO_GRAPHICSVIEW
    QWidget *tlw = widget->window();
    QWExtra *extra = qt_widget_private(tlw)->extra;
    if (extra && extra->proxyWidget) {
        const QPoint offset(widget->mapTo(tlw, QPoint()));
        const QVector<QRect> rects = rgn.rects();
        for (int i = 0; i < rects.size(); ++i)
            extra->proxyWidget->update(rects.at(i).translated(offset));
        return true;
    }
#endif

    return false;
}

void QWidgetPrivate::macSetNeedsDisplay(QRegion region)
{
    Q_Q(QWidget);
#ifndef QT_MAC_USE_COCOA
    if (region.isEmpty())
        HIViewSetNeedsDisplay(qt_mac_nativeview_for(q), true);
    else if (RgnHandle rgnHandle = region.toQDRgnForUpdate_sys())
        HIViewSetNeedsDisplayInRegion(qt_mac_nativeview_for(q), QMacSmartQuickDrawRegion(rgnHandle), true);
    else
        HIViewSetNeedsDisplay(qt_mac_nativeview_for(q), true); // do a complete repaint on overflow.
#else
    if (NSView *nativeView = qt_mac_nativeview_for(q)) {
        // INVARIANT: q is _not_ alien. So we can optimize a little:
        if (region.isEmpty()) {
            [nativeView setNeedsDisplay:YES];
        } else {
            QVector<QRect> rects = region.rects();
            for (int i = 0; i<rects.count(); ++i) {
                const QRect &rect = rects.at(i);
                NSRect nsrect = NSMakeRect(rect.x(), rect.y(), rect.width(), rect.height());
                [nativeView setNeedsDisplayInRect:nsrect];
            }
        }
    } else if (QWidget *effectiveWidget = q->nativeParentWidget()) {
        // INVARIANT: q is alien, and effectiveWidget is native.
        if (NSView *effectiveView = qt_mac_nativeview_for(effectiveWidget)) {
            if (region.isEmpty()) {
                const QRect &rect = q->rect();
                QPoint p = q->mapTo(effectiveWidget, rect.topLeft());
                NSRect nsrect = NSMakeRect(p.x(), p.y(), rect.width(), rect.height());
                [effectiveView setNeedsDisplayInRect:nsrect];
            } else {
                QVector<QRect> rects = region.rects();
                for (int i = 0; i<rects.count(); ++i) {
                    const QRect &rect = rects.at(i);
                    QPoint p = q->mapTo(effectiveWidget, rect.topLeft());
                    NSRect nsrect = NSMakeRect(p.x(), p.y(), rect.width(), rect.height());
                    [effectiveView setNeedsDisplayInRect:nsrect];
                }
            }
        }
    }
#endif
}

void QWidgetPrivate::macUpdateIsOpaque()
{
    Q_Q(QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created))
        return;
#ifndef QT_MAC_USE_COCOA
    HIViewFeatures bits;
    HIViewRef hiview = qt_mac_nativeview_for(q);
    HIViewGetFeatures(hiview, &bits);
    if ((bits & kHIViewIsOpaque) == isOpaque)
        return;
    if (isOpaque) {
        HIViewChangeFeatures(hiview, kHIViewIsOpaque, 0);
    } else {
        HIViewChangeFeatures(hiview, 0, kHIViewIsOpaque);
    }
    if (q->isVisible())
        HIViewReshapeStructure(qt_mac_nativeview_for(q));
#else
    if (isRealWindow() && !q->testAttribute(Qt::WA_MacBrushedMetal)) {
        bool opaque = isOpaque;
        if (extra && extra->imageMask)
            opaque = false; // we are never opaque when we have a mask.
        [qt_mac_window_for(q) setOpaque:opaque];
    }
#endif
}
#ifdef QT_MAC_USE_COCOA
static OSWindowRef qt_mac_create_window(QWidget *widget, WindowClass wclass,
                                        NSUInteger wattr, const QRect &crect)
{
    // Determine if we need to add in our "custom window" attribute. Cocoa is rather clever
    // in deciding if we need the maximize button or not (i.e., it's resizeable, so you
    // must need a maximize button). So, the only buttons we have control over are the
    // close and minimize buttons. If someone wants to customize and NOT have the maximize
    // button, then we have to do our hack. We only do it for these cases because otherwise
    // the window looks different when activated. This "QtMacCustomizeWindow" attribute is
    // intruding on a public space and WILL BREAK in the future.
    // One can hope that there is a more public API available by that time.
    Qt::WindowFlags flags = widget ? widget->windowFlags() : Qt::WindowFlags(0);
    if ((flags & Qt::CustomizeWindowHint)) {
        if ((flags & (Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint
                      | Qt::WindowMinimizeButtonHint | Qt::WindowTitleHint))
            && !(flags & Qt::WindowMaximizeButtonHint))
            wattr |= QtMacCustomizeWindow;
    }

    // If we haven't created the desktop widget, you have to pass the rectangle
    // in "cocoa coordinates" (i.e., top points to the lower left coordinate).
    // Otherwise, we do the conversion for you. Since we are the only ones that
    // create the desktop widget, this is OK (but confusing).
    NSRect geo = NSMakeRect(crect.left(),
                            (qt_root_win != 0) ? flipYCoordinate(crect.bottom() + 1) : crect.top(),
                            crect.width(), crect.height());
    QMacCocoaAutoReleasePool pool;
    OSWindowRef window;
    switch (wclass) {
    case kMovableModalWindowClass:
    case kModalWindowClass:
    case kSheetWindowClass:
    case kFloatingWindowClass:
    case kOverlayWindowClass:
    case kHelpWindowClass: {
        NSPanel *panel;
        BOOL needFloating = NO;
        BOOL worksWhenModal = widget && (widget->windowType() == Qt::Popup);
        // Add in the extra flags if necessary.
        switch (wclass) {
        case kSheetWindowClass:
            wattr |= NSDocModalWindowMask;
            break;
        case kFloatingWindowClass:
        case kHelpWindowClass:
            needFloating = YES;
            wattr |= NSUtilityWindowMask;
            break;
        default:
            break;
        }
        panel = [[QT_MANGLE_NAMESPACE(QCocoaPanel) alloc] QT_MANGLE_NAMESPACE(qt_initWithQWidget):widget contentRect:geo styleMask:wattr];
        [panel setFloatingPanel:needFloating];
        [panel setWorksWhenModal:worksWhenModal];
        window = panel;
        break;
    }
    case kDrawerWindowClass: {
        NSDrawer *drawer = [[NSDrawer alloc] initWithContentSize:geo.size preferredEdge:NSMinXEdge];
        [[QT_MANGLE_NAMESPACE(QCocoaWindowDelegate) sharedDelegate] becomeDelegateForDrawer:drawer widget:widget];
        QWidget *parentWidget = widget->parentWidget();
        if (parentWidget)
            [drawer setParentWindow:qt_mac_window_for(parentWidget)];
        [drawer setLeadingOffset:0.0];
        [drawer setTrailingOffset:25.0];
        window = [[drawer contentView] window];  // Just to make sure we actually return a window
        break;
    }
    default:
        window = [[QT_MANGLE_NAMESPACE(QCocoaWindow) alloc] QT_MANGLE_NAMESPACE(qt_initWithQWidget):widget contentRect:geo styleMask:wattr];
        break;
    }
    qt_syncCocoaTitleBarButtons(window, widget);
    return window;
}
#else
static OSWindowRef qt_mac_create_window(QWidget *, WindowClass wclass, WindowAttributes wattr,
                                        const QRect &crect)
{
    OSWindowRef window;
    Rect geo;
    SetRect(&geo, crect.left(), crect.top(), crect.right() + 1, crect.bottom() + 1);
    OSStatus err;
    if(geo.right <= geo.left)		geo.right = geo.left + 1;
    if(geo.bottom <= geo.top)		geo.bottom = geo.top + 1;
    Rect null_rect;
	SetRect(&null_rect, 0, 0, 1, 1);
    err = CreateNewWindow(wclass, wattr, &null_rect, &window);
    if(err == noErr) {
        err = SetWindowBounds(window, kWindowContentRgn, &geo);
        if(err != noErr)
            qWarning("QWidget: Internal error (%s:%d)", __FILE__, __LINE__);
    }
    return window;
}

#ifndef QT_NO_GESTURES
#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_6
/* We build the release package against the 10.4 SDK.
   So, to enable gestures for applications running on
   10.6+, we define the missing constants here: */
enum {
    kEventClassGesture              = 'gest',
    kEventGestureStarted            = 1,
    kEventGestureEnded              = 2,
    kEventGestureMagnify            = 4,
    kEventGestureSwipe              = 5,
    kEventGestureRotate             = 6,
    kEventParamRotationAmount       = 'rota',
    kEventParamSwipeDirection       = 'swip',
    kEventParamMagnificationAmount  = 'magn'
};
#endif
#endif // QT_NO_GESTURES

// window events
static EventTypeSpec window_events[] = {
    { kEventClassWindow, kEventWindowClose },
    { kEventClassWindow, kEventWindowExpanded },
    { kEventClassWindow, kEventWindowHidden },
    { kEventClassWindow, kEventWindowZoom },
    { kEventClassWindow, kEventWindowZoomed },
    { kEventClassWindow, kEventWindowCollapsed },
    { kEventClassWindow, kEventWindowToolbarSwitchMode },
    { kEventClassWindow, kEventWindowProxyBeginDrag },
    { kEventClassWindow, kEventWindowProxyEndDrag },
    { kEventClassWindow, kEventWindowResizeCompleted },
    { kEventClassWindow, kEventWindowBoundsChanging },
    { kEventClassWindow, kEventWindowGetRegion },
    { kEventClassWindow, kEventWindowGetClickModality },
    { kEventClassWindow, kEventWindowTransitionCompleted },
    { kEventClassGesture, kEventGestureStarted },
    { kEventClassGesture, kEventGestureEnded },
    { kEventClassGesture, kEventGestureMagnify },
    { kEventClassGesture, kEventGestureSwipe },
    { kEventClassGesture, kEventGestureRotate },
    { kEventClassMouse, kEventMouseDown }
};
static EventHandlerUPP mac_win_eventUPP = 0;
static void cleanup_win_eventUPP()
{
    DisposeEventHandlerUPP(mac_win_eventUPP);
    mac_win_eventUPP = 0;
}
static const EventHandlerUPP make_win_eventUPP()
{
    if(mac_win_eventUPP)
        return mac_win_eventUPP;
    qAddPostRoutine(cleanup_win_eventUPP);
    return mac_win_eventUPP = NewEventHandlerUPP(QWidgetPrivate::qt_window_event);
}
OSStatus QWidgetPrivate::qt_window_event(EventHandlerCallRef er, EventRef event, void *)
{
    QScopedLoopLevelCounter loopLevelCounter(qApp->d_func()->threadData);
    bool handled_event = true;
    UInt32 ekind = GetEventKind(event), eclass = GetEventClass(event);
    switch(eclass) {
    case kEventClassWindow: {
        WindowRef wid = 0;
        GetEventParameter(event, kEventParamDirectObject, typeWindowRef, 0,
                          sizeof(WindowRef), 0, &wid);
        QWidget *widget = qt_mac_find_window(wid);
        if(!widget) {
            handled_event = false;
        } else if(ekind == kEventWindowGetClickModality) {
            // Carbon will send us kEventWindowGetClickModality before every
            // mouse press / release event. By returning 'true', we tell Carbon
            // that we would like the event target to receive the mouse event even
            // if the target is modally shaddowed. In Qt, this makes sense when we
            // e.g. have a popup showing, as the popup will grab the event
            // and perhaps use it to close itself.
            // By also setting the current modal window back into the event, we
            // help Carbon determining which window is supposed to be raised.
            handled_event = qApp->activePopupWidget() ? true : false;
        } else if(ekind == kEventWindowClose) {
            widget->d_func()->close_helper(QWidgetPrivate::CloseWithSpontaneousEvent);
            QMenuBar::macUpdateMenuBar();
        } else if (ekind == kEventWindowTransitionCompleted) {
            WindowTransitionAction transitionAction;
            GetEventParameter(event, kEventParamWindowTransitionAction, typeWindowTransitionAction,
                              0, sizeof(transitionAction), 0, &transitionAction);
            if (transitionAction == kWindowHideTransitionAction)
                widget->hide();
        } else if(ekind == kEventWindowExpanded) {
            Qt::WindowStates currState = Qt::WindowStates(widget->data->window_state);
            Qt::WindowStates newState = currState;
            if (currState & Qt::WindowMinimized)
                newState &= ~Qt::WindowMinimized;
            if (!(currState & Qt::WindowActive))
                newState |= Qt::WindowActive;
            if (newState != currState) {
                // newState will differ from currState if the window
                // was expanded after clicking on the jewels (as opposed
                // to calling QWidget::setWindowState)
                widget->data->window_state = newState;
                QWindowStateChangeEvent e(currState);
                QApplication::sendSpontaneousEvent(widget, &e);
            }

            QShowEvent qse;
            QApplication::sendSpontaneousEvent(widget, &qse);
        } else if(ekind == kEventWindowZoom) {
            widget->d_func()->topData()->normalGeometry = widget->geometry();
            handled_event = false;
        } else if(ekind == kEventWindowZoomed) {
            WindowPartCode windowPart;
            GetEventParameter(event, kEventParamWindowPartCode,
                              typeWindowPartCode, 0, sizeof(windowPart), 0, &windowPart);
            if(windowPart == inZoomIn && widget->isMaximized()) {

                widget->data->window_state = widget->data->window_state & ~Qt::WindowMaximized;
                QWindowStateChangeEvent e(Qt::WindowStates(widget->data->window_state | Qt::WindowMaximized));
                QApplication::sendSpontaneousEvent(widget, &e);
            } else if(windowPart == inZoomOut && !widget->isMaximized()) {
                widget->data->window_state = widget->data->window_state | Qt::WindowMaximized;
                QWindowStateChangeEvent e(Qt::WindowStates(widget->data->window_state
                                                           & ~Qt::WindowMaximized));
                QApplication::sendSpontaneousEvent(widget, &e);
            }
            qt_button_down = 0;
        } else if(ekind == kEventWindowCollapsed) {
            if (!widget->isMinimized()) {
                widget->data->window_state = widget->data->window_state | Qt::WindowMinimized;
                QWindowStateChangeEvent e(Qt::WindowStates(widget->data->window_state & ~Qt::WindowMinimized));
                QApplication::sendSpontaneousEvent(widget, &e);
            }

            // Deactivate this window:
            if (widget->isActiveWindow() && !(widget->windowType() == Qt::Popup)) {
                QWidget *w = 0;
                if (widget->parentWidget())
                    w = widget->parentWidget()->window();
                if (!w || (!w->isVisible() && !w->isMinimized())) {
                    for (WindowPtr wp = GetFrontWindowOfClass(kDocumentWindowClass, true);
                        wp; wp = GetNextWindowOfClass(wp, kDocumentWindowClass, true)) {
                        if ((w = qt_mac_find_window(wp)))
                            break;
                    }
                }
                if(!(w && w->isVisible() && !w->isMinimized()))
                    qApp->setActiveWindow(0);
            }

            //we send a hide to be like X11/Windows
            QEvent e(QEvent::Hide);
            QApplication::sendSpontaneousEvent(widget, &e);
            qt_button_down = 0;
        } else if(ekind == kEventWindowToolbarSwitchMode) {
            macSendToolbarChangeEvent(widget);
            HIToolbarRef toolbar;
            if (GetWindowToolbar(wid, &toolbar) == noErr) {
                if (toolbar) {
                    // Let HIToolbar do its thang, but things like the OpenGL context
                    // needs to know about it.
                    CallNextEventHandler(er, event);
                    qt_event_request_window_change(widget);
                    widget->data->fstrut_dirty = true;
                }
            }
        } else if(ekind == kEventWindowGetRegion) {
            WindowRef window;
            GetEventParameter(event, kEventParamDirectObject, typeWindowRef, 0,
                              sizeof(window), 0, &window);
            WindowRegionCode wcode;
            GetEventParameter(event, kEventParamWindowRegionCode, typeWindowRegionCode, 0,
                              sizeof(wcode), 0, &wcode);
            if (wcode != kWindowOpaqueRgn){
                // If the region is kWindowOpaqueRgn, don't call next
                // event handler cause this will make the shadow of
                // masked windows become offset. Unfortunately, we're not sure why.
                CallNextEventHandler(er, event);
            }
			RgnHandle rgn;
            GetEventParameter(event, kEventParamRgnHandle, typeQDRgnHandle, 0,
                              sizeof(rgn), 0, &rgn);

            if(QWidgetPrivate::qt_widget_rgn(qt_mac_find_window(window), wcode, rgn, false))
                SetEventParameter(event, kEventParamRgnHandle, typeQDRgnHandle, sizeof(rgn), &rgn);
        } else if(ekind == kEventWindowProxyBeginDrag) {
            QIconDragEvent e;
            QApplication::sendSpontaneousEvent(widget, &e);
        } else if(ekind == kEventWindowResizeCompleted) {
            // Create a mouse up event, since such an event is not send by carbon to the
            // application event handler (while a mouse down <b>is</b> on kEventWindowResizeStarted)
            EventRef mouseUpEvent;
            CreateEvent(0, kEventClassMouse, kEventMouseUp, 0, kEventAttributeUserEvent, &mouseUpEvent);
            UInt16 mbutton = kEventMouseButtonPrimary;
            SetEventParameter(mouseUpEvent, kEventParamMouseButton, typeMouseButton, sizeof(mbutton), &mbutton);
            WindowRef window;
            GetEventParameter(event, kEventParamDirectObject, typeWindowRef, 0, sizeof(window), 0, &window);
            Rect dragRect;
            GetWindowBounds(window, kWindowGrowRgn, &dragRect);
            Point pos = {dragRect.bottom, dragRect.right};
            SetEventParameter(mouseUpEvent, kEventParamMouseLocation, typeQDPoint, sizeof(pos), &pos);
            SendEventToApplication(mouseUpEvent);
            ReleaseEvent(mouseUpEvent);
        } else if(ekind == kEventWindowBoundsChanging) {
            UInt32 flags = 0;
            GetEventParameter(event, kEventParamAttributes, typeUInt32, 0,
                                  sizeof(flags), 0, &flags);
            Rect nr;
            GetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle, 0,
                                  sizeof(nr), 0, &nr);

            QRect newRect(nr.left, nr.top, nr.right - nr.left, nr.bottom - nr.top);

            QTLWExtra * const tlwExtra = widget->d_func()->maybeTopData();
            if (tlwExtra && tlwExtra->isSetGeometry == 1) {
                widget->d_func()->setGeometry_sys_helper(newRect.left(), newRect.top(), newRect.width(), newRect.height(), tlwExtra->isMove);
            } else {
                //implicitly removes the maximized bit
                if((widget->data->window_state & Qt::WindowMaximized) &&
                   IsWindowInStandardState(wid, 0, 0)) {
                    widget->data->window_state &= ~Qt::WindowMaximized;
                    QWindowStateChangeEvent e(Qt::WindowStates(widget->data->window_state
                                                | Qt::WindowMaximized));
                    QApplication::sendSpontaneousEvent(widget, &e);

                }

                handled_event = false;
                const QRect oldRect = widget->data->crect;
                if((flags & kWindowBoundsChangeOriginChanged)) {
                    if(nr.left != oldRect.x() || nr.top != oldRect.y()) {
                        widget->data->crect.moveTo(nr.left, nr.top);
                        QMoveEvent qme(widget->data->crect.topLeft(), oldRect.topLeft());
                        QApplication::sendSpontaneousEvent(widget, &qme);
                    }
                }
                if((flags & kWindowBoundsChangeSizeChanged)) {
                    if (widget->isWindow()) {
                        QSize newSize = QLayout::closestAcceptableSize(widget, newRect.size());
                        int dh = newSize.height() - newRect.height();
                        int dw = newSize.width() - newRect.width();
                        if (dw != 0 || dh != 0) {
                            handled_event = true;  // We want to change the bounds, so we handle the event

                            // set the rect, so we can also do the resize down below (yes, we need to resize).
                            newRect.setBottom(newRect.bottom() + dh);
                            newRect.setRight(newRect.right() + dw);

                            nr.left = newRect.x();
                            nr.top = newRect.y();
                            nr.right = nr.left + newRect.width();
                            nr.bottom = nr.top + newRect.height();
                            SetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle, sizeof(Rect), &nr);
                        }
                    }

                    if (oldRect.width() != newRect.width() || oldRect.height() != newRect.height()) {
                        widget->data->crect.setSize(newRect.size());
                        HIRect bounds = CGRectMake(0, 0, newRect.width(), newRect.height());

                        // If the WA_StaticContents attribute is set we can optimize the resize
                        // by only repainting the newly exposed area. We do this by disabling
                        // painting when setting the size of the view. The OS will invalidate
                        // the newly exposed area for us.
                        const bool staticContents = widget->testAttribute(Qt::WA_StaticContents);
                        const HIViewRef view = qt_mac_nativeview_for(widget);
                        if (staticContents)
                            HIViewSetDrawingEnabled(view, false);
                        HIViewSetFrame(view, &bounds);
                        if (staticContents)
                            HIViewSetDrawingEnabled(view, true);

                        QResizeEvent qre(newRect.size(), oldRect.size());
                        QApplication::sendSpontaneousEvent(widget, &qre);
                        qt_event_request_window_change(widget);
                    }
                }
            }
        } else if (ekind == kEventWindowHidden) {
            // Make sure that we also hide any visible sheets on our window.
            // Cocoa does the right thing for us.
            const QObjectList children = widget->children();
            const int childCount = children.count();
            for (int i = 0; i < childCount; ++i) {
                QObject *obj = children.at(i);
                if (obj->isWidgetType()) {
                    QWidget *widget = static_cast<QWidget *>(obj);
                    if (qt_mac_is_macsheet(widget) && widget->isVisible())
                        widget->hide();
                }
            }
        } else {
            handled_event = false;
        }
        break; }
    case kEventClassMouse: {
#if 0
        return SendEventToApplication(event);
#endif

        bool send_to_app = false;
        {
            WindowPartCode wpc;
            if (GetEventParameter(event, kEventParamWindowPartCode, typeWindowPartCode, 0,
                                  sizeof(wpc), 0, &wpc) == noErr && wpc != inContent)
                send_to_app = true;
        }
        if(!send_to_app) {
            WindowRef window;
            if(GetEventParameter(event, kEventParamWindowRef, typeWindowRef, 0,
                                 sizeof(window), 0, &window) == noErr) {
                HIViewRef hiview;
                if(HIViewGetViewForMouseEvent(HIViewGetRoot(window), event, &hiview) == noErr) {
                    if(QWidget *w = QWidget::find((WId)hiview)) {
#if 0
                        send_to_app = !w->isActiveWindow();
#else
                        Q_UNUSED(w);
                        send_to_app = true;
#endif
                    }
                }
            }
        }
        if(send_to_app)
            return SendEventToApplication(event);
        handled_event = false;
        break; }

#ifndef QT_NO_GESTURES
    case kEventClassGesture: {
        // First, find the widget that was under
        // the mouse when the gesture happened:
        HIPoint screenLocation;
        if (GetEventParameter(event, kEventParamMouseLocation, typeHIPoint, 0,
                    sizeof(screenLocation), 0, &screenLocation) != noErr) {
            handled_event = false;
            break;
        }
        QWidget *widget = QApplication::widgetAt(screenLocation.x, screenLocation.y);
        if (!widget) {
            handled_event = false;
            break;
        }

        QNativeGestureEvent qNGEvent;
        qNGEvent.position = QPoint(screenLocation.x, screenLocation.y);

        switch (ekind) {
            case kEventGestureStarted:
                qNGEvent.gestureType = QNativeGestureEvent::GestureBegin;
                break;
            case kEventGestureEnded:
                qNGEvent.gestureType = QNativeGestureEvent::GestureEnd;
                break;
            case kEventGestureRotate: {
                CGFloat amount;
                if (GetEventParameter(event, kEventParamRotationAmount, 'cgfl', 0,
                            sizeof(amount), 0, &amount) != noErr) {
                    handled_event = false;
                    break;
                }
                qNGEvent.gestureType = QNativeGestureEvent::Rotate;
                qNGEvent.percentage = float(-amount);
                break; }
            case kEventGestureSwipe: {
                HIPoint swipeDirection;
                if (GetEventParameter(event, kEventParamSwipeDirection, typeHIPoint, 0,
                            sizeof(swipeDirection), 0, &swipeDirection) != noErr) {
                    handled_event = false;
                    break;
                }
                qNGEvent.gestureType = QNativeGestureEvent::Swipe;
                if (swipeDirection.x == 1)
                    qNGEvent.angle = 180.0f;
                else if (swipeDirection.x == -1)
                    qNGEvent.angle = 0.0f;
                else if (swipeDirection.y == 1)
                    qNGEvent.angle = 90.0f;
                else if (swipeDirection.y == -1)
                    qNGEvent.angle = 270.0f;
                break; }
            case kEventGestureMagnify: {
                CGFloat amount;
                if (GetEventParameter(event, kEventParamMagnificationAmount, 'cgfl', 0,
                            sizeof(amount), 0, &amount) != noErr) {
                    handled_event = false;
                    break;
                }
                qNGEvent.gestureType = QNativeGestureEvent::Zoom;
                qNGEvent.percentage = float(amount);
                break; }
        }

        QApplication::sendSpontaneousEvent(widget, &qNGEvent);
    break; }
#endif // QT_NO_GESTURES

    default:
        handled_event = false;
    }
    if(!handled_event) //let the event go through
        return eventNotHandledErr;
    return noErr; //we eat the event
}

// widget events
static HIObjectClassRef widget_class = 0;
static EventTypeSpec widget_events[] = {
    { kEventClassHIObject, kEventHIObjectConstruct },
    { kEventClassHIObject, kEventHIObjectDestruct },

    { kEventClassControl, kEventControlDraw },
    { kEventClassControl, kEventControlInitialize },
    { kEventClassControl, kEventControlGetPartRegion },
    { kEventClassControl, kEventControlGetClickActivation },
    { kEventClassControl, kEventControlSetFocusPart },
    { kEventClassControl, kEventControlDragEnter },
    { kEventClassControl, kEventControlDragWithin },
    { kEventClassControl, kEventControlDragLeave },
    { kEventClassControl, kEventControlDragReceive },
    { kEventClassControl, kEventControlOwningWindowChanged },
    { kEventClassControl, kEventControlBoundsChanged },
    { kEventClassControl, kEventControlGetSizeConstraints },
    { kEventClassControl, kEventControlVisibilityChanged },

    { kEventClassMouse, kEventMouseDown },
    { kEventClassMouse, kEventMouseUp },
    { kEventClassMouse, kEventMouseMoved },
    { kEventClassMouse, kEventMouseDragged }
};
static EventHandlerUPP mac_widget_eventUPP = 0;
static void cleanup_widget_eventUPP()
{
    DisposeEventHandlerUPP(mac_widget_eventUPP);
    mac_widget_eventUPP = 0;
}
static const EventHandlerUPP make_widget_eventUPP()
{
    if(mac_widget_eventUPP)
        return mac_widget_eventUPP;
    qAddPostRoutine(cleanup_widget_eventUPP);
    return mac_widget_eventUPP = NewEventHandlerUPP(QWidgetPrivate::qt_widget_event);
}
OSStatus QWidgetPrivate::qt_widget_event(EventHandlerCallRef er, EventRef event, void *)
{
    QScopedLoopLevelCounter loopLevelCounter(QApplicationPrivate::instance()->threadData);

    bool handled_event = true;
    UInt32 ekind = GetEventKind(event), eclass = GetEventClass(event);
    switch(eclass) {
    case kEventClassHIObject: {
        HIViewRef view = 0;
        GetEventParameter(event, kEventParamHIObjectInstance, typeHIObjectRef,
                          0, sizeof(view), 0, &view);
        if(ekind == kEventHIObjectConstruct) {
            if(view) {
                HIViewChangeFeatures(view, kHIViewAllowsSubviews, 0);
                SetEventParameter(event, kEventParamHIObjectInstance,
                                  typeVoidPtr, sizeof(view), &view);
            }
        } else if(ekind == kEventHIObjectDestruct) {
            //nothing to really do.. or is there?
        } else {
            handled_event = false;
        }
        break; }
    case kEventClassControl: {
        QWidget *widget = 0;
        HIViewRef hiview = 0;
        if(GetEventParameter(event, kEventParamDirectObject, typeControlRef,
                             0, sizeof(hiview), 0, &hiview) == noErr)
            widget = QWidget::find((WId)hiview);
        if (widget && widget->macEvent(er, event))
            return noErr;
        if(ekind == kEventControlDraw) {
            if(widget && qt_isGenuineQWidget(hiview)) {

                // if there is a window change event pending for any gl child wigets,
                // send it immediately. (required for flicker-free resizing)
                extern void qt_mac_send_posted_gl_updates(QWidget *widget);
                qt_mac_send_posted_gl_updates(widget);

                if (QApplicationPrivate::graphicsSystem() && !widget->d_func()->paintOnScreen()) {
                    widget->d_func()->syncBackingStore();
                    widget->d_func()->dirtyOnWidget = QRegion();
                    return noErr;
                }

                //requested rgn
                RgnHandle rgn;
                GetEventParameter(event, kEventParamRgnHandle, typeQDRgnHandle, 0, sizeof(rgn), 0, &rgn);
                QRegion qrgn(qt_mac_convert_mac_region(rgn));

                //update handles
                GrafPtr qd = 0;
                CGContextRef cg = 0;
                if(GetEventParameter(event, kEventParamCGContextRef, typeCGContextRef, 0, sizeof(cg), 0, &cg) != noErr) {
                    Q_ASSERT(false);
                }
                widget->d_func()->hd = cg;
                widget->d_func()->qd_hd = qd;
                CGContextSaveGState(cg);

#ifdef DEBUG_WIDGET_PAINT
                const bool doDebug = true;
                if(doDebug)  {
                    qDebug("asked to draw %p[%p] [%s::%s] %p[%p] [%d] [%dx%d]", widget, hiview, widget->metaObject()->className(),
                           widget->objectName().local8Bit().data(), widget->parentWidget(),
                           (HIViewRef)(widget->parentWidget() ? qt_mac_nativeview_for(widget->parentWidget()) : (HIViewRef)0),
                           HIViewIsCompositingEnabled(hiview), qt_mac_posInWindow(widget).x(), qt_mac_posInWindow(widget).y());
#if 0
                    QVector<QRect> region_rects = qrgn.rects();
                    qDebug("Region! %d", region_rects.count());
                    for(int i = 0; i < region_rects.count(); i++)
                        qDebug("%d %d %d %d", region_rects[i].x(), region_rects[i].y(),
                               region_rects[i].width(), region_rects[i].height());
                    region_rects = widget->d_func()->clp.rects();
                    qDebug("Widget Region! %d", region_rects.count());
                    for(int i = 0; i < region_rects.count(); i++)
                        qDebug("%d %d %d %d", region_rects[i].x(), region_rects[i].y(),
                               region_rects[i].width(), region_rects[i].height());
#endif
                }
#endif
                if (widget->isVisible() && widget->updatesEnabled()) { //process the actual paint event.
                    if(widget->testAttribute(Qt::WA_WState_InPaintEvent))
                        qWarning("QWidget::repaint: Recursive repaint detected");
                    if (widget->isWindow() && !widget->d_func()->isOpaque
                        && !widget->testAttribute(Qt::WA_MacBrushedMetal)) {
                        QRect qrgnRect = qrgn.boundingRect();
                        CGContextClearRect(cg, CGRectMake(qrgnRect.x(), qrgnRect.y(), qrgnRect.width(), qrgnRect.height()));
                    }

                    QPoint redirectionOffset(0, 0);
                    QWidget *tl = widget->window();
                    if (tl) {
                        Qt::WindowFlags flags = tl->windowFlags();
                        if (flags & Qt::FramelessWindowHint
                            || (flags & Qt::CustomizeWindowHint && !(flags & Qt::WindowTitleHint))) {
                            if(tl->d_func()->extra && !tl->d_func()->extra->mask.isEmpty())
                                redirectionOffset += tl->d_func()->extra->mask.boundingRect().topLeft();
                        }
                    }

                    //setup the context
                    widget->setAttribute(Qt::WA_WState_InPaintEvent);
                    QPaintEngine *engine = widget->paintEngine();
                    if (engine)
                        engine->setSystemClip(qrgn);

                    //handle the erase
                    if (engine && (!widget->testAttribute(Qt::WA_NoSystemBackground)
                        && (widget->isWindow() || widget->autoFillBackground())
                        || widget->testAttribute(Qt::WA_TintedBackground)
                        || widget->testAttribute(Qt::WA_StyledBackground))) {
#ifdef DEBUG_WIDGET_PAINT
                        if(doDebug)
                            qDebug(" Handling erase for [%s::%s]", widget->metaObject()->className(),
                                   widget->objectName().local8Bit().data());
#endif
                        if (!redirectionOffset.isNull())
                            widget->d_func()->setRedirected(widget, redirectionOffset);

                        bool was_unclipped = widget->testAttribute(Qt::WA_PaintUnclipped);
                        widget->setAttribute(Qt::WA_PaintUnclipped, false);
                        QPainter p(widget);
                        p.setClipping(false);
                        if(was_unclipped)
                            widget->setAttribute(Qt::WA_PaintUnclipped);
                        widget->d_func()->paintBackground(&p, qrgn, widget->isWindow() ? DrawAsRoot : 0);
                        if (widget->testAttribute(Qt::WA_TintedBackground)) {
                            QColor tint = widget->palette().window().color();
                            tint.setAlphaF(.6);
                            const QVector<QRect> &rects = qrgn.rects();
                            for (int i = 0; i < rects.size(); ++i)
                                p.fillRect(rects.at(i), tint);
                        }
                        p.end();
                        if (!redirectionOffset.isNull())
                            widget->d_func()->restoreRedirected();
                    }

                    if(!HIObjectIsOfClass((HIObjectRef)hiview, kObjectQWidget))
                        CallNextEventHandler(er, event);

                    //send the paint
                    redirectionOffset += widget->data->wrect.topLeft(); // Map from system to qt coordinates
                    if (!redirectionOffset.isNull())
                        widget->d_func()->setRedirected(widget, redirectionOffset);
                    qrgn.translate(redirectionOffset);
                    QPaintEvent e(qrgn);
                    widget->d_func()->dirtyOnWidget = QRegion();
#ifdef QT3_SUPPORT
                    e.setErased(true);
#endif
                    QApplication::sendSpontaneousEvent(widget, &e);
                    if (!redirectionOffset.isNull())
                        widget->d_func()->restoreRedirected();

                    //cleanup
                    if (engine)
                        engine->setSystemClip(QRegion());

                    widget->setAttribute(Qt::WA_WState_InPaintEvent, false);
                    if(!widget->testAttribute(Qt::WA_PaintOutsidePaintEvent) && widget->paintingActive())
                        qWarning("QWidget: It is dangerous to leave painters active on a widget outside of the PaintEvent");
                }

                widget->d_func()->hd = 0;
                widget->d_func()->qd_hd = 0;
                CGContextRestoreGState(cg);
            } else if(!HIObjectIsOfClass((HIObjectRef)hiview, kObjectQWidget)) {
                CallNextEventHandler(er, event);
            }
        } else if(ekind == kEventControlInitialize) {
            if(HIObjectIsOfClass((HIObjectRef)hiview, kObjectQWidget)) {
                UInt32 features = kControlSupportsDragAndDrop | kControlSupportsClickActivation | kControlSupportsFocus;
                SetEventParameter(event, kEventParamControlFeatures, typeUInt32, sizeof(features), &features);
            } else {
                handled_event = false;
            }
        } else if(ekind == kEventControlSetFocusPart) {
            if(widget) {
                ControlPartCode part;
                GetEventParameter(event, kEventParamControlPart, typeControlPartCode, 0,
                                  sizeof(part), 0, &part);
                if(part == kControlFocusNoPart){
                    if (widget->hasFocus())
                        QApplicationPrivate::setFocusWidget(0, Qt::OtherFocusReason);
                } else
                    widget->setFocus();
            }
            if(!HIObjectIsOfClass((HIObjectRef)hiview, kObjectQWidget))
                CallNextEventHandler(er, event);
        } else if(ekind == kEventControlGetClickActivation) {
            ClickActivationResult clickT = kActivateAndIgnoreClick;
            SetEventParameter(event, kEventParamClickActivation, typeClickActivationResult,
                              sizeof(clickT), &clickT);
        } else if(ekind == kEventControlGetPartRegion) {
            handled_event = false;
            if(!HIObjectIsOfClass((HIObjectRef)hiview, kObjectQWidget) && CallNextEventHandler(er, event) == noErr) {
                handled_event = true;
                break;
            }
            if(widget && !widget->isWindow()) {
                ControlPartCode part;
                GetEventParameter(event, kEventParamControlPart, typeControlPartCode, 0,
                                  sizeof(part), 0, &part);
                if(part == kControlClickableMetaPart && widget->testAttribute(Qt::WA_TransparentForMouseEvents)) {
                    RgnHandle rgn;
                    GetEventParameter(event, kEventParamControlRegion, typeQDRgnHandle, 0,
                                      sizeof(rgn), 0, &rgn);
                    SetEmptyRgn(rgn);
                    handled_event = true;
                } else if(part == kControlStructureMetaPart || part == kControlClickableMetaPart) {
                    RgnHandle rgn;
                    GetEventParameter(event, kEventParamControlRegion, typeQDRgnHandle, 0,
                                      sizeof(rgn), 0, &rgn);
                    SetRectRgn(rgn, 0, 0, widget->width(), widget->height());
                    if(QWidgetPrivate::qt_widget_rgn(widget, kWindowStructureRgn, rgn, false))
                        handled_event = true;
                } else if(part == kControlOpaqueMetaPart) {
                    if(widget->d_func()->isOpaque) {
                        RgnHandle rgn;
                        GetEventParameter(event, kEventParamControlRegion, typeQDRgnHandle, 0,
                                          sizeof(RgnHandle), 0, &rgn);
                        SetRectRgn(rgn, 0, 0, widget->width(), widget->height());
                        QWidgetPrivate::qt_widget_rgn(widget, kWindowStructureRgn, rgn, false);
                        SetEventParameter(event, kEventParamControlRegion, typeQDRgnHandle,
                                sizeof(RgnHandle), &rgn);
                        handled_event = true;
                    }
                }
            }
        } else if(ekind == kEventControlOwningWindowChanged) {
            if(!HIObjectIsOfClass((HIObjectRef)hiview, kObjectQWidget))
                CallNextEventHandler(er, event);
            if(widget && qt_mac_window_for(hiview)) {
                WindowRef foo = 0;
                GetEventParameter(event, kEventParamControlCurrentOwningWindow, typeWindowRef, 0,
                                  sizeof(foo), 0, &foo);
                widget->d_func()->initWindowPtr();
            }
            if (widget)
                qt_event_request_window_change(widget);
        } else if(ekind == kEventControlDragEnter || ekind == kEventControlDragWithin ||
                  ekind == kEventControlDragLeave || ekind == kEventControlDragReceive) {
            // dnd are really handled in qdnd_mac.cpp,
            // just modularize the code a little...
            DragRef drag;
            GetEventParameter(event, kEventParamDragRef, typeDragRef, 0, sizeof(drag), 0, &drag);
            handled_event = false;
            bool drag_allowed = false;

            QWidget *dropWidget = widget;
            if (qobject_cast<QFocusFrame *>(widget)){
                // We might shadow widgets underneath the focus
                // frame, so stay interrested, and let the dnd through
                drag_allowed = true;
                handled_event = true;
                Point where;
                GetDragMouse(drag, &where, 0);
                dropWidget = QApplication::widgetAt(QPoint(where.h, where.v));

                if (dropWidget != QDragManager::self()->currentTarget()) {
                    // We have to 'fake' enter and leave events for the shaddowed widgets:
                    if (ekind == kEventControlDragEnter) {
                        if (QDragManager::self()->currentTarget())
                            QDragManager::self()->currentTarget()->d_func()->qt_mac_dnd_event(kEventControlDragLeave, drag);
                        if (dropWidget) {
                            dropWidget->d_func()->qt_mac_dnd_event(kEventControlDragEnter, drag);
                        }
                        // Set dropWidget to zero, so qt_mac_dnd_event
                        // doesn't get called a second time below:
                        dropWidget = 0;
                    } else if (ekind == kEventControlDragLeave) {
                        dropWidget = QDragManager::self()->currentTarget();
                        if (dropWidget) {
                            dropWidget->d_func()->qt_mac_dnd_event(kEventControlDragLeave, drag);
                        }
                        // Set dropWidget to zero, so qt_mac_dnd_event
                        // doesn't get called a second time below:
                        dropWidget = 0;
                    }
                }
            }

            // Send the dnd event to the widget:
            if (dropWidget && dropWidget->d_func()->qt_mac_dnd_event(ekind, drag)) {
                drag_allowed = true;
                handled_event = true;
            }

            if (ekind == kEventControlDragEnter) {
                // If we don't accept the enter event, we will
                // receive no more drag events for this widget
                const Boolean wouldAccept = drag_allowed ? true : false;
                SetEventParameter(event, kEventParamControlWouldAcceptDrop, typeBoolean,
                        sizeof(wouldAccept), &wouldAccept);
            }
        } else if (ekind == kEventControlBoundsChanged) {
            if (!widget || widget->isWindow() || widget->testAttribute(Qt::WA_Moved) || widget->testAttribute(Qt::WA_Resized)) {
                handled_event = false;
            } else {
                // Sync our view in case some other (non-Qt) view is controlling us.
                handled_event = true;
                Rect newBounds;
                GetEventParameter(event, kEventParamCurrentBounds,
                                  typeQDRectangle, 0, sizeof(Rect), 0, &newBounds);
                QRect rect(newBounds.left, newBounds.top,
                            newBounds.right - newBounds.left, newBounds.bottom - newBounds.top);

                bool moved = widget->testAttribute(Qt::WA_Moved);
                bool resized = widget->testAttribute(Qt::WA_Resized);
                widget->setGeometry(rect);
                widget->setAttribute(Qt::WA_Moved, moved);
                widget->setAttribute(Qt::WA_Resized, resized);
                qt_event_request_window_change(widget);
            }
        } else if (ekind == kEventControlGetSizeConstraints) {
            if (!widget || !qt_isGenuineQWidget(widget)) {
                handled_event = false;
            } else {
                handled_event = true;
                QWidgetItem item(widget);
                QSize size = item.minimumSize();
                HISize hisize = { size.width(), size.height() };
                SetEventParameter(event, kEventParamMinimumSize, typeHISize, sizeof(HISize), &hisize);
                size = item.maximumSize();
                hisize.width = size.width() + 2; // ### shouldn't have to add 2 (but it works).
                hisize.height = size.height();
                SetEventParameter(event, kEventParamMaximumSize, typeHISize, sizeof(HISize), &hisize);
            }
        } else if (ekind == kEventControlVisibilityChanged) {
            handled_event = false;
            if (widget) {
                qt_event_request_window_change(widget);
                if (!HIViewIsVisible(HIViewRef(widget->winId()))) {
                    if (widget == qt_button_down)
                        qt_button_down = 0;
                }
            }
        }
        break; }
    case kEventClassMouse: {
        bool send_to_app = false;
        if(qt_button_down)
            send_to_app = true;
        if(send_to_app) {
            OSStatus err = SendEventToApplication(event);
            if(err != noErr)
                handled_event = false;
        } else {
            CallNextEventHandler(er, event);
        }
        break; }
    default:
        handled_event = false;
        break;
    }
    if(!handled_event) //let the event go through
        return eventNotHandledErr;
    return noErr; //we eat the event
}
#endif

OSViewRef qt_mac_create_widget(QWidget *widget, QWidgetPrivate *widgetPrivate, OSViewRef parent)
{
#ifdef QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
    QT_MANGLE_NAMESPACE(QCocoaView) *view = [[QT_MANGLE_NAMESPACE(QCocoaView) alloc] initWithQWidget:widget widgetPrivate:widgetPrivate];

#ifdef ALIEN_DEBUG
    qDebug() << "Creating NSView for" << widget;
#endif

    if (view && parent)
        [parent addSubview:view];
    return view;
#else
    Q_UNUSED(widget);
    Q_UNUSED(widgetPrivate);
    if(!widget_class) {
        OSStatus err = HIObjectRegisterSubclass(kObjectQWidget, kHIViewClassID, 0, make_widget_eventUPP(),
                                                GetEventTypeCount(widget_events), widget_events,
                                                0, &widget_class);
        if (err && err != hiObjectClassExistsErr)
            qWarning("QWidget: Internal error (%d)", __LINE__);
    }
    HIViewRef ret = 0;
    if(HIObjectCreate(kObjectQWidget, 0, (HIObjectRef*)&ret) != noErr)
        qWarning("QWidget: Internal error (%d)", __LINE__);
    if(ret && parent)
        HIViewAddSubview(parent, ret);
    return ret;
#endif
}

void qt_mac_unregister_widget()
{
#ifndef QT_MAC_USE_COCOA
    HIObjectUnregisterClass(widget_class);
    widget_class = 0;
#endif
}

void QWidgetPrivate::toggleDrawers(bool visible)
{
    for (int i = 0; i < children.size(); ++i) {
        register QObject *object = children.at(i);
        if (!object->isWidgetType())
            continue;
        QWidget *widget = static_cast<QWidget*>(object);
        if(qt_mac_is_macdrawer(widget)) {
            bool oldState = widget->testAttribute(Qt::WA_WState_ExplicitShowHide);
            if(visible) {
                if (!widget->testAttribute(Qt::WA_WState_ExplicitShowHide))
                    widget->show();
            } else {
                widget->hide();
                if(!oldState)
                    widget->setAttribute(Qt::WA_WState_ExplicitShowHide, false);
            }
        }
    }
}

/*****************************************************************************
  QWidgetPrivate member functions
 *****************************************************************************/
bool QWidgetPrivate::qt_mac_update_sizer(QWidget *w, int up)
{
    // I'm not sure what "up" is
    if(!w || !w->isWindow())
        return false;

    QTLWExtra *topData = w->d_func()->topData();
    QWExtra *extraData = w->d_func()->extraData();
    // topData->resizer is only 4 bits, so subtracting -1 from zero causes bad stuff
    // to happen, prevent that here (you really want the thing hidden).
    if (up >= 0 || topData->resizer != 0)
        topData->resizer += up;
    OSWindowRef windowRef = qt_mac_window_for(OSViewRef(w->effectiveWinId()));
    {
#ifndef QT_MAC_USE_COCOA
        WindowClass wclass;
        GetWindowClass(windowRef, &wclass);
        if(!(GetAvailableWindowAttributes(wclass) & kWindowResizableAttribute))
            return true;
#endif
    }
    bool remove_grip = (topData->resizer || (w->windowFlags() & Qt::FramelessWindowHint)
                        || (extraData->maxw && extraData->maxh &&
                            extraData->maxw == extraData->minw && extraData->maxh == extraData->minh));
#ifndef QT_MAC_USE_COCOA
    WindowAttributes attr;
    GetWindowAttributes(windowRef, &attr);
    if(remove_grip) {
        if(attr & kWindowResizableAttribute) {
            ChangeWindowAttributes(qt_mac_window_for(w), kWindowNoAttributes,
                                   kWindowResizableAttribute);
            ReshapeCustomWindow(qt_mac_window_for(w));
        }
    } else if(!(attr & kWindowResizableAttribute)) {
        ChangeWindowAttributes(windowRef, kWindowResizableAttribute,
                               kWindowNoAttributes);
        ReshapeCustomWindow(windowRef);
    }
#else
    [windowRef setShowsResizeIndicator:!remove_grip];
#endif
    return true;
}

void QWidgetPrivate::qt_clean_root_win()
{
#ifdef QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
    [qt_root_win release];
#else
    if(!qt_root_win)
        return;
    CFRelease(qt_root_win);
#endif
    qt_root_win = 0;
}

bool QWidgetPrivate::qt_create_root_win()
{
    if(qt_root_win)
        return false;
    const QSize desktopSize = qt_mac_desktopSize();
    QRect desktopRect(QPoint(0, 0), desktopSize);
#ifdef QT_MAC_USE_COCOA
    qt_root_win = qt_mac_create_window(0, kOverlayWindowClass, NSBorderlessWindowMask, desktopRect);
#else
    WindowAttributes wattr = (kWindowCompositingAttribute | kWindowStandardHandlerAttribute);
    qt_root_win = qt_mac_create_window(0, kOverlayWindowClass, wattr, desktopRect);
#endif
    if(!qt_root_win)
        return false;
    qAddPostRoutine(qt_clean_root_win);
    return true;
}

bool QWidgetPrivate::qt_widget_rgn(QWidget *widget, short wcode, RgnHandle rgn, bool force = false)
{
    bool ret = false;
#ifndef QT_MAC_USE_COCOA
    switch(wcode) {
    case kWindowStructureRgn: {
        if(widget) {
            if(widget->d_func()->extra && !widget->d_func()->extra->mask.isEmpty()) {
                QRegion rin = qt_mac_convert_mac_region(rgn);
                if(!rin.isEmpty()) {
                    QPoint rin_tl = rin.boundingRect().topLeft(); //in offset
                    rin.translate(-rin_tl.x(), -rin_tl.y()); //bring into same space as below
                    QRegion mask = widget->d_func()->extra->mask;
                    Qt::WindowFlags flags = widget->windowFlags();
                    if(widget->isWindow()
                       && !(flags & Qt::FramelessWindowHint
                            || (flags & Qt::CustomizeWindowHint && !(flags & Qt::WindowTitleHint)))) {
                        QRegion title;
                        {
                            QMacSmartQuickDrawRegion rgn(qt_mac_get_rgn());
                            GetWindowRegion(qt_mac_window_for(widget), kWindowTitleBarRgn, rgn);
                            title = qt_mac_convert_mac_region(rgn);
                        }
                        QRect br = title.boundingRect();
                        mask.translate(0, br.height()); //put the mask 'under' the title bar..
                        title.translate(-br.x(), -br.y());
                        mask += title;
                    }

                    QRegion cr = rin & mask;
                    cr.translate(rin_tl.x(), rin_tl.y()); //translate back to incoming space
                    CopyRgn(QMacSmartQuickDrawRegion(cr.toQDRgn()), rgn);
                }
                ret = true;
            } else if(force) {
                QRegion cr(widget->geometry());
                CopyRgn(QMacSmartQuickDrawRegion(cr.toQDRgn()), rgn);
                ret = true;
            }
        }
        break; }
    default: break;
    }
    //qDebug() << widget << ret << wcode << qt_mac_convert_mac_region(rgn);
#else
    Q_UNUSED(widget);
    Q_UNUSED(wcode);
    Q_UNUSED(rgn);
    Q_UNUSED(force);
#endif
    return ret;
}

/*****************************************************************************
  QWidget member functions
 *****************************************************************************/
void QWidgetPrivate::determineWindowClass()
{
    Q_Q(QWidget);
#if !defined(QT_NO_MAINWINDOW) && !defined(QT_NO_TOOLBAR)
    // Make sure that QMainWindow has the MacWindowToolBarButtonHint when the
    // unifiedTitleAndToolBarOnMac property is ON. This is to avoid reentry of
    // setParent() triggered by the QToolBar::event(QEvent::ParentChange).
    QMainWindow *mainWindow = qobject_cast<QMainWindow *>(q);
    if (mainWindow && mainWindow->unifiedTitleAndToolBarOnMac()) {
        data.window_flags |= Qt::MacWindowToolBarButtonHint;
    }
#endif
#ifndef QT_MAC_USE_COCOA
// ### COCOA:Interleave these better!

    const Qt::WindowType type = q->windowType();
    Qt::WindowFlags &flags = data.window_flags;
    const bool popup = (type == Qt::Popup);
    if (type == Qt::ToolTip || type == Qt::SplashScreen || popup)
        flags |= Qt::FramelessWindowHint;

    WindowClass wclass = kSheetWindowClass;
    if(qt_mac_is_macdrawer(q))
        wclass = kDrawerWindowClass;
    else if (q->testAttribute(Qt::WA_ShowModal) && flags & Qt::CustomizeWindowHint)
        wclass = kDocumentWindowClass;
    else if(popup || (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5 && type == Qt::SplashScreen))
        wclass = kModalWindowClass;
    else if(q->testAttribute(Qt::WA_ShowModal))
        wclass = kMovableModalWindowClass;
    else if(type == Qt::ToolTip)
        wclass = kHelpWindowClass;
    else if(type == Qt::Tool || (QSysInfo::MacintoshVersion < QSysInfo::MV_10_5
                                 && type == Qt::SplashScreen))
        wclass = kFloatingWindowClass;
    else
        wclass = kDocumentWindowClass;

    WindowGroupRef grp = 0;
    WindowAttributes wattr = (kWindowCompositingAttribute | kWindowStandardHandlerAttribute);
    if (q->testAttribute(Qt::WA_MacFrameworkScaled))
        wattr |= kWindowFrameworkScaledAttribute;
    if(qt_mac_is_macsheet(q)) {
        //grp = GetWindowGroupOfClass(kMovableModalWindowClass);
        wclass = kSheetWindowClass;
    } else {
        grp = GetWindowGroupOfClass(wclass);
        // Shift things around a bit to get the correct window class based on the presence
        // (or lack) of the border.
	bool customize = flags & Qt::CustomizeWindowHint;
        bool framelessWindow = (flags & Qt::FramelessWindowHint || (customize && !(flags & Qt::WindowTitleHint)));
        if (framelessWindow) {
            if(wclass == kDocumentWindowClass) {
                wattr |= kWindowNoTitleBarAttribute;
            } else if(wclass == kFloatingWindowClass) {
                wattr |= kWindowNoTitleBarAttribute;
            } else if (wclass  == kMovableModalWindowClass) {
                wclass = kModalWindowClass;
            }
        } else {
            if(wclass != kModalWindowClass)
                wattr |= kWindowResizableAttribute;
        }
        // Only add extra decorations (well, buttons) for widgets that can have them
        // and have an actual border we can put them on.
        if(wclass != kModalWindowClass && wclass != kMovableModalWindowClass
                && wclass != kSheetWindowClass && wclass != kPlainWindowClass
                && !framelessWindow && wclass != kDrawerWindowClass
                && wclass != kHelpWindowClass) {
            if (flags & Qt::WindowMaximizeButtonHint)
                wattr |= kWindowFullZoomAttribute;
            if (flags & Qt::WindowMinimizeButtonHint)
                wattr |= kWindowCollapseBoxAttribute;
            if (flags & Qt::WindowSystemMenuHint || flags & Qt::WindowCloseButtonHint)
                wattr |= kWindowCloseBoxAttribute;
            if (flags & Qt::MacWindowToolBarButtonHint)
                wattr |= kWindowToolbarButtonAttribute;
        } else {
            // Clear these hints so that we aren't call them on invalid windows
            flags &= ~(Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint
                       | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint);
        }
    }
    if((popup || type == Qt::Tool) && !q->isModal())
        wattr |= kWindowHideOnSuspendAttribute;
    wattr |= kWindowLiveResizeAttribute;

#ifdef DEBUG_WINDOW_CREATE
#define ADD_DEBUG_WINDOW_NAME(x) { x, #x }
    struct {
        UInt32 tag;
        const char *name;
    } known_attribs[] = {
        ADD_DEBUG_WINDOW_NAME(kWindowCompositingAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowStandardHandlerAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowMetalAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowHideOnSuspendAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowStandardHandlerAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowCollapseBoxAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowHorizontalZoomAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowVerticalZoomAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowResizableAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowNoActivatesAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowNoUpdatesAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowOpaqueForEventsAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowLiveResizeAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowCloseBoxAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowHideOnSuspendAttribute),
        { 0, 0 }
    }, known_classes[] = {
        ADD_DEBUG_WINDOW_NAME(kHelpWindowClass),
        ADD_DEBUG_WINDOW_NAME(kPlainWindowClass),
        ADD_DEBUG_WINDOW_NAME(kDrawerWindowClass),
        ADD_DEBUG_WINDOW_NAME(kUtilityWindowClass),
        ADD_DEBUG_WINDOW_NAME(kToolbarWindowClass),
        ADD_DEBUG_WINDOW_NAME(kSheetWindowClass),
        ADD_DEBUG_WINDOW_NAME(kFloatingWindowClass),
        ADD_DEBUG_WINDOW_NAME(kUtilityWindowClass),
        ADD_DEBUG_WINDOW_NAME(kDocumentWindowClass),
        ADD_DEBUG_WINDOW_NAME(kToolbarWindowClass),
        ADD_DEBUG_WINDOW_NAME(kMovableModalWindowClass),
        ADD_DEBUG_WINDOW_NAME(kModalWindowClass),
        { 0, 0 }
    };
    qDebug("Qt: internal: ************* Creating new window %p (%s::%s)", q, q->metaObject()->className(),
            q->objectName().toLocal8Bit().constData());
    bool found_class = false;
    for(int i = 0; known_classes[i].name; i++) {
        if(wclass == known_classes[i].tag) {
            found_class = true;
            qDebug("Qt: internal: ** Class: %s", known_classes[i].name);
            break;
        }
    }
    if(!found_class)
        qDebug("Qt: internal: !! Class: Unknown! (%d)", (int)wclass);
    if(wattr) {
        WindowAttributes tmp_wattr = wattr;
        qDebug("Qt: internal: ** Attributes:");
        for(int i = 0; tmp_wattr && known_attribs[i].name; i++) {
            if((tmp_wattr & known_attribs[i].tag) == known_attribs[i].tag) {
                tmp_wattr ^= known_attribs[i].tag;
                qDebug("Qt: internal: * %s %s", known_attribs[i].name,
                        (GetAvailableWindowAttributes(wclass) & known_attribs[i].tag) ? "" : "(*)");
            }
        }
        if(tmp_wattr)
            qDebug("Qt: internal: !! Attributes: Unknown (%d)", (int)tmp_wattr);
    }
#endif

    /* Just to be extra careful we will change to the kUtilityWindowClass if the
       requested attributes cannot be used */
    if((GetAvailableWindowAttributes(wclass) & wattr) != wattr) {
        WindowClass tmp_class = wclass;
        if(wclass == kToolbarWindowClass || wclass == kUtilityWindowClass)
            wclass = kFloatingWindowClass;
        if(tmp_class != wclass) {
            if(!grp)
                grp = GetWindowGroupOfClass(wclass);
            wclass = tmp_class;
        }
    }
    topData()->wclass = wclass;
    topData()->wattr = wattr;
#else
    const Qt::WindowType type = q->windowType();
    Qt::WindowFlags &flags = data.window_flags;
    const bool popup = (type == Qt::Popup);
    if (type == Qt::ToolTip || type == Qt::SplashScreen || popup)
        flags |= Qt::FramelessWindowHint;

    WindowClass wclass = kSheetWindowClass;
    if(qt_mac_is_macdrawer(q))
        wclass = kDrawerWindowClass;
    else if (q->testAttribute(Qt::WA_ShowModal) && flags & Qt::CustomizeWindowHint)
        wclass = kDocumentWindowClass;
    else if(popup || (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5 && type == Qt::SplashScreen))
        wclass = kModalWindowClass;
    else if(type == Qt::Dialog)
        wclass = kMovableModalWindowClass;
    else if(type == Qt::ToolTip)
        wclass = kHelpWindowClass;
    else if(type == Qt::Tool || (QSysInfo::MacintoshVersion < QSysInfo::MV_10_5
                                 && type == Qt::SplashScreen))
        wclass = kFloatingWindowClass;
    else if(q->testAttribute(Qt::WA_ShowModal))
        wclass = kMovableModalWindowClass;
    else
        wclass = kDocumentWindowClass;

    WindowAttributes wattr = NSBorderlessWindowMask;
    if(qt_mac_is_macsheet(q)) {
        //grp = GetWindowGroupOfClass(kMovableModalWindowClass);
        wclass = kSheetWindowClass;
        wattr = NSTitledWindowMask | NSResizableWindowMask;
    } else {
#ifndef QT_MAC_USE_COCOA
        grp = GetWindowGroupOfClass(wclass);
#endif
        // Shift things around a bit to get the correct window class based on the presence
        // (or lack) of the border.
	bool customize = flags & Qt::CustomizeWindowHint;
        bool framelessWindow = (flags & Qt::FramelessWindowHint || (customize && !(flags & Qt::WindowTitleHint)));
        if (framelessWindow) {
            if (wclass == kDocumentWindowClass) {
                wclass = kSimpleWindowClass;
            } else if (wclass == kFloatingWindowClass) {
                wclass = kToolbarWindowClass;
            } else if (wclass  == kMovableModalWindowClass) {
                wclass  = kModalWindowClass;
            }
        } else {
            wattr |= NSTitledWindowMask;
            if (wclass != kModalWindowClass)
                wattr |= NSResizableWindowMask;
        }
        // Only add extra decorations (well, buttons) for widgets that can have them
        // and have an actual border we can put them on.
        if (wclass != kModalWindowClass
                && wclass != kSheetWindowClass && wclass != kPlainWindowClass
                && !framelessWindow && wclass != kDrawerWindowClass
                && wclass != kHelpWindowClass) {
            if (flags & Qt::WindowMinimizeButtonHint)
                wattr |= NSMiniaturizableWindowMask;
            if (flags & Qt::WindowSystemMenuHint || flags & Qt::WindowCloseButtonHint)
                wattr |= NSClosableWindowMask;
        } else {
            // Clear these hints so that we aren't call them on invalid windows
            flags &= ~(Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint
                       | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint);
        }
    }
    if (q->testAttribute(Qt::WA_MacBrushedMetal))
        wattr |= NSTexturedBackgroundWindowMask;

#ifdef DEBUG_WINDOW_CREATE
#define ADD_DEBUG_WINDOW_NAME(x) { x, #x }
    struct {
        UInt32 tag;
        const char *name;
    } known_attribs[] = {
        ADD_DEBUG_WINDOW_NAME(kWindowCompositingAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowStandardHandlerAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowMetalAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowHideOnSuspendAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowStandardHandlerAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowCollapseBoxAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowHorizontalZoomAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowVerticalZoomAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowResizableAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowNoActivatesAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowNoUpdatesAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowOpaqueForEventsAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowLiveResizeAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowCloseBoxAttribute),
        ADD_DEBUG_WINDOW_NAME(kWindowHideOnSuspendAttribute),
        { 0, 0 }
    }, known_classes[] = {
        ADD_DEBUG_WINDOW_NAME(kHelpWindowClass),
        ADD_DEBUG_WINDOW_NAME(kPlainWindowClass),
        ADD_DEBUG_WINDOW_NAME(kDrawerWindowClass),
        ADD_DEBUG_WINDOW_NAME(kUtilityWindowClass),
        ADD_DEBUG_WINDOW_NAME(kToolbarWindowClass),
        ADD_DEBUG_WINDOW_NAME(kSheetWindowClass),
        ADD_DEBUG_WINDOW_NAME(kFloatingWindowClass),
        ADD_DEBUG_WINDOW_NAME(kUtilityWindowClass),
        ADD_DEBUG_WINDOW_NAME(kDocumentWindowClass),
        ADD_DEBUG_WINDOW_NAME(kToolbarWindowClass),
        ADD_DEBUG_WINDOW_NAME(kMovableModalWindowClass),
        ADD_DEBUG_WINDOW_NAME(kModalWindowClass),
        { 0, 0 }
    };
    qDebug("Qt: internal: ************* Creating new window %p (%s::%s)", q, q->metaObject()->className(),
            q->objectName().toLocal8Bit().constData());
    bool found_class = false;
    for(int i = 0; known_classes[i].name; i++) {
        if(wclass == known_classes[i].tag) {
            found_class = true;
            qDebug("Qt: internal: ** Class: %s", known_classes[i].name);
            break;
        }
    }
    if(!found_class)
        qDebug("Qt: internal: !! Class: Unknown! (%d)", (int)wclass);
    if(wattr) {
        WindowAttributes tmp_wattr = wattr;
        qDebug("Qt: internal: ** Attributes:");
        for(int i = 0; tmp_wattr && known_attribs[i].name; i++) {
            if((tmp_wattr & known_attribs[i].tag) == known_attribs[i].tag) {
                tmp_wattr ^= known_attribs[i].tag;
            }
        }
        if(tmp_wattr)
            qDebug("Qt: internal: !! Attributes: Unknown (%d)", (int)tmp_wattr);
    }
#endif

#ifndef QT_MAC_USE_COCOA
    /* Just to be extra careful we will change to the kUtilityWindowClass if the
       requested attributes cannot be used */
    if((GetAvailableWindowAttributes(wclass) & wattr) != wattr) {
        WindowClass tmp_class = wclass;
        if(wclass == kToolbarWindowClass || wclass == kUtilityWindowClass)
            wclass = kFloatingWindowClass;
        if(tmp_class != wclass) {
            if(!grp)
                grp = GetWindowGroupOfClass(wclass);
            wclass = tmp_class;
        }
    }
#endif
#endif
    topData()->wclass = wclass;
    topData()->wattr = wattr;
}

#ifndef QT_MAC_USE_COCOA  // This is handled in Cocoa via our category.
void QWidgetPrivate::initWindowPtr()
{
    Q_Q(QWidget);
    OSWindowRef windowRef = qt_mac_window_for(qt_mac_nativeview_for(q)); //do not create!
    if(!windowRef)
        return;
    QWidget *window = q->window(), *oldWindow = 0;
    if(GetWindowProperty(windowRef, kWidgetCreatorQt, kWidgetPropertyQWidget, sizeof(oldWindow), 0, &oldWindow) == noErr) {
        Q_ASSERT(window == oldWindow);
        return;
    }

    if(SetWindowProperty(windowRef, kWidgetCreatorQt, kWidgetPropertyQWidget, sizeof(window), &window) != noErr)
        qWarning("Qt:Internal error (%s:%d)", __FILE__, __LINE__); //no real way to recover
    if(!q->windowType() != Qt::Desktop) { //setup an event callback handler on the window
        InstallWindowEventHandler(windowRef, make_win_eventUPP(), GetEventTypeCount(window_events),
                window_events, static_cast<void *>(qApp), &window_event);
    }
}

void QWidgetPrivate::finishCreateWindow_sys_Carbon(OSWindowRef windowRef)
{
    Q_Q(QWidget);
    const Qt::WindowType type = q->windowType();
    Qt::WindowFlags &flags = data.window_flags;
    QWidget *parentWidget = q->parentWidget();

    const bool desktop = (type == Qt::Desktop);
    const bool dialog = (type == Qt::Dialog
                         || type == Qt::Sheet
                         || type == Qt::Drawer
                         || (flags & Qt::MSWindowsFixedSizeDialogHint));
    QTLWExtra *topExtra = topData();
    quint32 wattr = topExtra->wattr;
    if (!desktop)
        SetAutomaticControlDragTrackingEnabledForWindow(windowRef, true);
    HIWindowChangeFeatures(windowRef, kWindowCanCollapse, 0);
    if (wattr & kWindowHideOnSuspendAttribute)
        HIWindowChangeAvailability(windowRef, kHIWindowExposeHidden, 0);
    else
        HIWindowChangeAvailability(windowRef, 0, kHIWindowExposeHidden);
    if ((flags & Qt::WindowStaysOnTopHint))
        ChangeWindowAttributes(windowRef, kWindowNoAttributes, kWindowHideOnSuspendAttribute);
    if (qt_mac_is_macdrawer(q) && parentWidget)
        SetDrawerParent(windowRef, qt_mac_window_for (parentWidget));
    if (topExtra->group) {
        qt_mac_release_window_group(topExtra->group);
        topExtra->group = 0;
    }
    if (type == Qt::ToolTip)
        qt_mac_set_window_group_to_tooltip(windowRef);
    else if (type == Qt::Popup && (flags & Qt::WindowStaysOnTopHint))
        qt_mac_set_window_group_to_popup(windowRef);
    else if (flags & Qt::WindowStaysOnTopHint)
        qt_mac_set_window_group_to_stays_on_top(windowRef, type);
    else if (dialog)
        SetWindowGroup(windowRef, GetWindowGroupOfClass(kMovableModalWindowClass));

#ifdef DEBUG_WINDOW_CREATE
    if (WindowGroupRef grpf = GetWindowGroup(windowRef)) {
        QCFString cfname;
        CopyWindowGroupName(grpf, &cfname);
        SInt32 lvl;
        GetWindowGroupLevel(grpf, &lvl);
        const char *from = "Default";
        if (topExtra && grpf == topData()->group)
            from = "Created";
        else if (grpf == grp)
            from = "Copied";
        qDebug("Qt: internal: With window group '%s' [%p] @ %d: %s",
                static_cast<QString>(cfname).toLatin1().constData(), grpf, (int)lvl, from);
    } else {
        qDebug("Qt: internal: No window group!!!");
    }
    HIWindowAvailability hi_avail = 0;
    if (HIWindowGetAvailability(windowRef, &hi_avail) == noErr) {
        struct {
            UInt32 tag;
            const char *name;
        } known_avail[] = {
            ADD_DEBUG_WINDOW_NAME(kHIWindowExposeHidden),
            { 0, 0 }
        };
        qDebug("Qt: internal: ** HIWindowAvailibility:");
        for (int i = 0; hi_avail && known_avail[i].name; i++) {
            if ((hi_avail & known_avail[i].tag) == known_avail[i].tag) {
                hi_avail ^= known_avail[i].tag;
                qDebug("Qt: internal: * %s", known_avail[i].name);
            }
        }
        if (hi_avail)
            qDebug("Qt: internal: !! Attributes: Unknown (%d)", (int)hi_avail);
    }
#undef ADD_DEBUG_WINDOW_NAME
#endif
    if (extra && !extra->mask.isEmpty())
        ReshapeCustomWindow(windowRef);
    SetWindowModality(windowRef, kWindowModalityNone, 0);
    if (qt_mac_is_macdrawer(q))
        SetDrawerOffsets(windowRef, 0.0, 25.0);
    data.fstrut_dirty = true; // when we create a toplevel widget, the frame strut should be dirty
    HIViewRef hiview = (HIViewRef)data.winid;
    HIViewRef window_hiview = qt_mac_get_contentview_for(windowRef);
    if(!hiview) {
        hiview = qt_mac_create_widget(q, this, window_hiview);
        setWinId((WId)hiview);
    } else {
        HIViewAddSubview(window_hiview, hiview);
    }
    if (hiview) {
        Rect win_rect;
        GetWindowBounds(qt_mac_window_for (window_hiview), kWindowContentRgn, &win_rect);
        HIRect bounds = CGRectMake(0, 0, win_rect.right-win_rect.left, win_rect.bottom-win_rect.top);
        HIViewSetFrame(hiview, &bounds);
        HIViewSetVisible(hiview, true);
        if (q->testAttribute(Qt::WA_DropSiteRegistered))
            registerDropSite(true);
        transferChildren();
    }
    initWindowPtr();

    if (topExtra->posFromMove) {
        updateFrameStrut();
        const QRect &fStrut = frameStrut();
        Rect r;
        SetRect(&r, data.crect.left(), data.crect.top(), data.crect.right() + 1, data.crect.bottom() + 1);
        SetRect(&r, r.left + fStrut.left(), r.top + fStrut.top(),
                    (r.left + fStrut.left() + data.crect.width()) - fStrut.right(),
                    (r.top + fStrut.top() + data.crect.height()) - fStrut.bottom());
        SetWindowBounds(windowRef, kWindowContentRgn, &r);
        topExtra->posFromMove = false;
    }

    if (q->testAttribute(Qt::WA_WState_WindowOpacitySet)){
        q->setWindowOpacity(topExtra->opacity / 255.0f);
    } else if (qt_mac_is_macsheet(q)){
        SetThemeWindowBackground(qt_mac_window_for(q), kThemeBrushSheetBackgroundTransparent, true);
        CGFloat alpha = 0;
        GetWindowAlpha(qt_mac_window_for(q), &alpha);
        if (alpha == 1){
            // For some reason the 'SetThemeWindowBackground' does not seem
            // to work. So we do this little hack until it hopefully starts to
            // work in newer versions of mac OS.
            q->setWindowOpacity(0.95f);
            q->setAttribute(Qt::WA_WState_WindowOpacitySet, false);
        }
    } else{
        // If the window has been recreated after beeing e.g. a sheet,
        // make sure that we don't report a faulty opacity:
        q->setWindowOpacity(1.0f);
        q->setAttribute(Qt::WA_WState_WindowOpacitySet, false);
    }

    // Since we only now have a window, sync our state.
    macUpdateHideOnSuspend();
    macUpdateOpaqueSizeGrip();
    macUpdateMetalAttribute();
    macUpdateIgnoreMouseEvents();
    setWindowTitle_helper(extra->topextra->caption);
    setWindowIconText_helper(extra->topextra->iconText);
    setWindowFilePath_helper(extra->topextra->filePath);
    setWindowModified_sys(q->isWindowModified());
    updateFrameStrut();
    qt_mac_update_sizer(q);
    applyMaxAndMinSizeOnWindow();
}
#else  // QT_MAC_USE_COCOA

void QWidgetPrivate::setWindowLevel()
{
    Q_Q(QWidget);
    const QWidget * const windowParent = q->window()->parentWidget();
    const QWidget * const primaryWindow = windowParent ? windowParent->window() : 0;
    NSInteger winLevel = -1;

    if (q->windowType() == Qt::Popup) {
        winLevel = NSPopUpMenuWindowLevel;
        // Popup should be in at least the same level as its parent.
        if (primaryWindow) {
            OSWindowRef parentRef = qt_mac_window_for(primaryWindow);
            winLevel = qMax([parentRef level], winLevel);
        }
    } else if (q->windowType() == Qt::Tool) {
        winLevel = NSFloatingWindowLevel;
    } else if (q->windowType() == Qt::Dialog) {
        // Correct modality level (NSModalPanelWindowLevel) will be
        // set by cocoa when creating a modal session later.
        winLevel = NSNormalWindowLevel;
    }

    // StayOnTop window should appear above Tool windows.
    if (data.window_flags & Qt::WindowStaysOnTopHint)
        winLevel = NSPopUpMenuWindowLevel;
    // Tooltips should appear above StayOnTop windows.
    if (q->windowType() == Qt::ToolTip)
        winLevel = NSScreenSaverWindowLevel;
    // All other types are Normal level.
    if (winLevel == -1)
        winLevel = NSNormalWindowLevel;
    [qt_mac_window_for(q) setLevel:winLevel];
}

void QWidgetPrivate::finishCreateWindow_sys_Cocoa(void * /*NSWindow * */ voidWindowRef)
{
    Q_Q(QWidget);
    QMacCocoaAutoReleasePool pool;
    NSWindow *windowRef = static_cast<NSWindow *>(voidWindowRef);
    const Qt::WindowType type = q->windowType();
    Qt::WindowFlags &flags = data.window_flags;
    QWidget *parentWidget = q->parentWidget();

    const bool popup = (type == Qt::Popup);
    const bool dialog = (type == Qt::Dialog
                         || type == Qt::Sheet
                         || type == Qt::Drawer
                         || (flags & Qt::MSWindowsFixedSizeDialogHint));
    QTLWExtra *topExtra = topData();

    if ((popup || type == Qt::Tool || type == Qt::ToolTip) && !q->isModal()) {
        [windowRef setHidesOnDeactivate:YES];
    } else {
        [windowRef setHidesOnDeactivate:NO];
    }
    if (q->testAttribute(Qt::WA_MacNoShadow))
        [windowRef setHasShadow:NO];
    else
        [windowRef setHasShadow:YES];
    Q_UNUSED(parentWidget);
    Q_UNUSED(dialog);

    data.fstrut_dirty = true; // when we create a toplevel widget, the frame strut should be dirty

    OSViewRef nsview = (OSViewRef)data.winid;
    if (!nsview) {
        nsview = qt_mac_create_widget(q, this, 0);
        setWinId(WId(nsview));
    }
    [windowRef setContentView:nsview];
    [nsview setHidden:NO];
    transferChildren();

    // Tell Cocoa explicit that we wan't the view to receive key events
    // (regardless of focus policy) because this is how it works on other
    // platforms (and in the carbon port):
    [windowRef makeFirstResponder:nsview];

    if (topExtra->posFromMove) {
        updateFrameStrut();

        const QRect &fStrut = frameStrut();
        const QRect &crect = data.crect;
        const QRect frameRect(QPoint(crect.left(), crect.top()),
                              QSize(fStrut.left() + fStrut.right() + crect.width(),
                                    fStrut.top() + fStrut.bottom() + crect.height()));
        NSRect cocoaFrameRect = NSMakeRect(frameRect.x(), flipYCoordinate(frameRect.bottom() + 1),
                                           frameRect.width(), frameRect.height());
        [windowRef setFrame:cocoaFrameRect display:NO];
        topExtra->posFromMove = false;
    }

    if (q->testAttribute(Qt::WA_WState_WindowOpacitySet)){
        q->setWindowOpacity(topExtra->opacity / 255.0f);
    } else if (qt_mac_is_macsheet(q)){
        CGFloat alpha = [qt_mac_window_for(q) alphaValue];
        if (alpha >= 1.0) {
            q->setWindowOpacity(0.95f);
            q->setAttribute(Qt::WA_WState_WindowOpacitySet, false);
        }
    } else{
        // If the window has been recreated after beeing e.g. a sheet,
        // make sure that we don't report a faulty opacity:
        q->setWindowOpacity(1.0f);
        q->setAttribute(Qt::WA_WState_WindowOpacitySet, false);
    }

    // Its more performant to handle the mouse cursor
    // ourselves, expecially when using alien widgets:
    [windowRef disableCursorRects];

    setWindowLevel();
    macUpdateHideOnSuspend();
    macUpdateOpaqueSizeGrip();
    macUpdateIgnoreMouseEvents();
    setWindowTitle_helper(extra->topextra->caption);
    setWindowIconText_helper(extra->topextra->iconText);
    setWindowModified_sys(q->isWindowModified());
    updateFrameStrut();
    syncCocoaMask();
    macUpdateIsOpaque();
    qt_mac_update_sizer(q);
    applyMaxAndMinSizeOnWindow();
}

#endif // QT_MAC_USE_COCOA

/*
 Recreates widget window. Useful if immutable
 properties for it has changed.
 */
void QWidgetPrivate::recreateMacWindow()
{
    Q_Q(QWidget);
    OSViewRef myView = qt_mac_nativeview_for(q);
    OSWindowRef oldWindow = qt_mac_window_for(myView);
#ifndef QT_MAC_USE_COCOA
    HIViewRemoveFromSuperview(myView);
    determineWindowClass();
    createWindow_sys();

    if (QMainWindowLayout *mwl = qt_mainwindow_layout(qobject_cast<QMainWindow *>(q))) {
        mwl->updateHIToolBarStatus();
    }

    if (IsWindowVisible(oldWindow))
        show_sys();
#else
    QMacCocoaAutoReleasePool pool;
    [myView removeFromSuperview];
    determineWindowClass();
    createWindow_sys();
    if (NSToolbar *toolbar = [oldWindow toolbar]) {
        OSWindowRef newWindow = qt_mac_window_for(myView);
        [newWindow setToolbar:toolbar];
        [toolbar setVisible:[toolbar isVisible]];
    }
    if ([oldWindow isVisible]){
        if ([oldWindow isSheet])
            [NSApp endSheet:oldWindow];
        [oldWindow orderOut:oldWindow];
        show_sys();
    }
#endif // QT_MAC_USE_COCOA

    // Release the window after creating the new window, because releasing it early
    // may cause the app to quit ("close on last window closed attribute")
    qt_mac_destructWindow(oldWindow);
}

void QWidgetPrivate::createWindow_sys()
{
    Q_Q(QWidget);
    Qt::WindowFlags &flags = data.window_flags;
    QWidget *parentWidget = q->parentWidget();

    QTLWExtra *topExtra = topData();
    if (topExtra->embedded)
        return;  // Simply return because this view "is" the top window.
    quint32 wattr = topExtra->wattr;

    if(parentWidget && (parentWidget->window()->windowFlags() & Qt::WindowStaysOnTopHint)) // If our parent has Qt::WStyle_StaysOnTop, so must we
        flags |= Qt::WindowStaysOnTopHint;

    data.fstrut_dirty = true;

    OSWindowRef windowRef = qt_mac_create_window(q, topExtra->wclass, wattr, data.crect);
    if (windowRef == 0)
        qWarning("QWidget: Internal error: %s:%d: If you reach this error please contact Qt Support and include the\n"
                "      WidgetFlags used in creating the widget.", __FILE__, __LINE__);
#ifndef QT_MAC_USE_COCOA
    finishCreateWindow_sys_Carbon(windowRef);
#else
    finishCreateWindow_sys_Cocoa(windowRef);
#endif
}

void QWidgetPrivate::create_sys(WId window, bool initializeWindow, bool destroyOldWindow)
{
    Q_Q(QWidget);
    QMacCocoaAutoReleasePool pool;

    OSViewRef destroyid = 0;
#ifndef QT_MAC_USE_COCOA
    window_event = 0;
#endif

    Qt::WindowType type = q->windowType();
    Qt::WindowFlags flags = data.window_flags;
    QWidget *parentWidget = q->parentWidget();

    bool topLevel = (flags & Qt::Window);
    bool popup = (type == Qt::Popup);
    bool dialog = (type == Qt::Dialog
                   || type == Qt::Sheet
                   || type == Qt::Drawer
                   || (flags & Qt::MSWindowsFixedSizeDialogHint));
    bool desktop = (type == Qt::Desktop);

    // Determine this early for top-levels so, we can use it later.
    if (topLevel)
        determineWindowClass();

    if (desktop) {
        QSize desktopSize = qt_mac_desktopSize();
        q->setAttribute(Qt::WA_WState_Visible);
        data.crect.setRect(0, 0, desktopSize.width(), desktopSize.height());
        dialog = popup = false;                  // force these flags off
    } else {
        if (topLevel && (type != Qt::Drawer)) {
            if (QDesktopWidget *dsk = QApplication::desktop()) { // calc pos/size from screen
                const bool wasResized = q->testAttribute(Qt::WA_Resized);
                const bool wasMoved = q->testAttribute(Qt::WA_Moved);
                int deskn = dsk->primaryScreen();
                if (parentWidget && parentWidget->windowType() != Qt::Desktop)
                    deskn = dsk->screenNumber(parentWidget);
                QRect screenGeo = dsk->screenGeometry(deskn);
                if (!wasResized) {
#ifndef QT_MAC_USE_COCOA
                    data.crect.setSize(QSize(screenGeo.width()/2, 4*screenGeo.height()/10));
#else
                    NSRect newRect = [NSWindow frameRectForContentRect:NSMakeRect(0, 0,
                                                                  screenGeo.width() / 2.,
                                                                  4 * screenGeo.height() / 10.)
                                        styleMask:topData()->wattr];
                    data.crect.setSize(QSize(newRect.size.width, newRect.size.height));
#endif
                    // Constrain to minimums and maximums we've set
                    if (extra->minw > 0)
                        data.crect.setWidth(qMax(extra->minw, data.crect.width()));
                    if (extra->minh > 0)
                        data.crect.setHeight(qMax(extra->minh, data.crect.height()));
                    if (extra->maxw > 0)
                        data.crect.setWidth(qMin(extra->maxw, data.crect.width()));
                    if (extra->maxh > 0)
                        data.crect.setHeight(qMin(extra->maxh, data.crect.height()));
                }
                if (!wasMoved && !q->testAttribute(Qt::WA_DontShowOnScreen))
                    data.crect.moveTopLeft(QPoint(screenGeo.width()/4,
                                                  3 * screenGeo.height() / 10));
            }
        }
    }


    if(!window)                              // always initialize
        initializeWindow=true;

    hd = 0;
    if(window) {                                // override the old window (with a new NSView)
        OSViewRef nativeView = OSViewRef(window);
        OSViewRef parent = 0;
#ifndef QT_MAC_USE_COCOA
        CFRetain(nativeView);
#else
        [nativeView retain];
#endif
        if (destroyOldWindow)
            destroyid = qt_mac_nativeview_for(q);
        bool transfer = false;
        setWinId((WId)nativeView);
#ifndef QT_MAC_USE_COCOA
#ifndef HIViewInstallEventHandler
        // Macro taken from the CarbonEvents Header on Tiger
#define HIViewInstallEventHandler( target, handler, numTypes, list, userData, outHandlerRef ) \
               InstallEventHandler( HIObjectGetEventTarget( (HIObjectRef) (target) ), (handler), (numTypes), (list), (userData), (outHandlerRef) )
#endif
        HIViewInstallEventHandler(nativeView, make_widget_eventUPP(), GetEventTypeCount(widget_events), widget_events, 0, 0);
#endif
        if(topLevel) {
            for(int i = 0; i < 2; ++i) {
                if(i == 1) {
                    if(!initializeWindow)
                        break;
                    createWindow_sys();
                }
                if(OSWindowRef windowref = qt_mac_window_for(nativeView)) {
#ifndef QT_MAC_USE_COCOA
                    CFRetain(windowref);
#else
                    [windowref retain];
#endif
                    if (initializeWindow) {
                        parent = qt_mac_get_contentview_for(windowref);
                    } else {
#ifndef QT_MAC_USE_COCOA
                        parent = HIViewGetSuperview(nativeView);
#else
                        parent = [nativeView superview];
#endif
                    }
                    break;
                }
            }
            if(!parent)
                transfer = true;
        } else if (parentWidget) {
            // I need to be added to my parent, therefore my parent needs an NSView
            // Alien note: a 'window' was supplied as argument, meaning this widget
            // is not alien. So therefore the parent cannot be alien either.
            parentWidget->createWinId();
            parent = qt_mac_nativeview_for(parentWidget);
        }
        if(parent != nativeView && parent) {
#ifndef QT_MAC_USE_COCOA
            HIViewAddSubview(parent, nativeView);
#else
            [parent addSubview:nativeView];
#endif
        }
        if(transfer)
            transferChildren();
        data.fstrut_dirty = true; // we'll re calculate this later
        q->setAttribute(Qt::WA_WState_Visible,
#ifndef QT_MAC_USE_COCOA
                        HIViewIsVisible(nativeView)
#else
                        ![nativeView isHidden]
#endif
                        );
        if(initializeWindow) {
#ifndef QT_MAC_USE_COCOA
            HIRect bounds = CGRectMake(data.crect.x(), data.crect.y(), data.crect.width(), data.crect.height());
            HIViewSetFrame(nativeView, &bounds);
            q->setAttribute(Qt::WA_WState_Visible, HIViewIsVisible(nativeView));
#else
            NSRect bounds = NSMakeRect(data.crect.x(), data.crect.y(), data.crect.width(), data.crect.height());
            [nativeView setFrame:bounds];
            q->setAttribute(Qt::WA_WState_Visible, [nativeView isHidden]);
#endif
        }
#ifndef QT_MAC_USE_COCOA
        initWindowPtr();
#endif
    } else if (desktop) {                        // desktop widget
        if (!qt_root_win)
            QWidgetPrivate::qt_create_root_win();
        Q_ASSERT(qt_root_win);
        WId rootWinID = 0;
#ifndef QT_MAC_USE_COCOA
        CFRetain(qt_root_win);
        if(HIViewRef rootContentView = HIViewGetRoot(qt_root_win)) {
            rootWinID = (WId)rootContentView;
            CFRetain(rootContentView);
        }
#else
        [qt_root_win retain];
        if (OSViewRef rootContentView = [qt_root_win contentView]) {
            rootWinID = (WId)rootContentView;
            [rootContentView retain];
        }
#endif
        setWinId(rootWinID);
    } else if (topLevel) {
        determineWindowClass();
        if(OSViewRef osview = qt_mac_create_widget(q, this, 0)) {
#ifndef QT_MAC_USE_COCOA
            HIRect bounds = CGRectMake(data.crect.x(), data.crect.y(),
                                       data.crect.width(), data.crect.height());
            HIViewSetFrame(osview, &bounds);
#else
            NSRect bounds = NSMakeRect(data.crect.x(), flipYCoordinate(data.crect.y()),
                                       data.crect.width(), data.crect.height());
            [osview setFrame:bounds];
#endif
            setWinId((WId)osview);
        }
    } else {
        data.fstrut_dirty = false; // non-toplevel widgets don't have a frame, so no need to update the strut

#ifdef QT_MAC_USE_COCOA
        if (q->testAttribute(Qt::WA_NativeWindow) == false || q->internalWinId() != 0) {
            // INVARIANT: q is Alien, and we should not create an NSView to back it up.
        } else
#endif
        if (OSViewRef osview = qt_mac_create_widget(q, this, qt_mac_nativeview_for(parentWidget))) {
#ifndef QT_MAC_USE_COCOA
            HIRect bounds = CGRectMake(data.crect.x(), data.crect.y(), data.crect.width(), data.crect.height());
            HIViewSetFrame(osview, &bounds);
            setWinId((WId)osview);
#else
            NSRect bounds = NSMakeRect(data.crect.x(), data.crect.y(), data.crect.width(), data.crect.height());
            [osview setFrame:bounds];
            setWinId((WId)osview);
            if (q->isVisible()) {
                // If q were Alien before, but now became native (e.g. if a call to
                // winId was done from somewhere), we need to show the view immidiatly:
                QMacCocoaAutoReleasePool pool;
                [osview setHidden:NO];
            }
#endif
        }
    }

    updateIsOpaque();

    if (q->testAttribute(Qt::WA_DropSiteRegistered))
        registerDropSite(true);
    if (q->hasFocus())
        setFocus_sys();
    if (!topLevel && initializeWindow)
        setWSGeometry();
    if (destroyid)
        qt_mac_destructView(destroyid);
}

/*!
    Returns the QuickDraw handle of the widget. Use of this function is not
    portable. This function will return 0 if QuickDraw is not supported, or
    if the handle could not be created.

    \warning This function is only available on Mac OS X.
*/

Qt::HANDLE
QWidget::macQDHandle() const
{
#ifndef QT_MAC_USE_COCOA
    return d_func()->qd_hd;
#else
    return 0;
#endif
}

/*!
  Returns the CoreGraphics handle of the widget. Use of this function is
  not portable. This function will return 0 if no painter context can be
  established, or if the handle could not be created.

  \warning This function is only available on Mac OS X.
*/
Qt::HANDLE
QWidget::macCGHandle() const
{
    return handle();
}

void qt_mac_updateParentUnderAlienWidget(QWidget *alienWidget)
{
    QWidget *nativeParent = alienWidget->nativeParentWidget();
    if (!nativeParent)
        return;

    QPoint globalPos = alienWidget->mapToGlobal(QPoint(0, 0));
    QRect dirtyRect = QRect(nativeParent->mapFromGlobal(globalPos), alienWidget->size());
    nativeParent->update(dirtyRect);
}

void QWidget::destroy(bool destroyWindow, bool destroySubWindows)
{
    Q_D(QWidget);
    QMacCocoaAutoReleasePool pool;
    d->aboutToDestroy();
    if (!isWindow() && parentWidget())
        parentWidget()->d_func()->invalidateBuffer(d->effectiveRectFor(geometry()));
    if (!internalWinId())
        qt_mac_updateParentUnderAlienWidget(this);
    d->deactivateWidgetCleanup();
    qt_mac_event_release(this);
    if(testAttribute(Qt::WA_WState_Created)) {
        setAttribute(Qt::WA_WState_Created, false);
        QObjectList chldrn = children();
        for(int i = 0; i < chldrn.size(); i++) {  // destroy all widget children
            QObject *obj = chldrn.at(i);
            if(obj->isWidgetType())
                static_cast<QWidget*>(obj)->destroy(destroySubWindows, destroySubWindows);
        }
        if(mac_mouse_grabber == this)
            releaseMouse();
        if(mac_keyboard_grabber == this)
            releaseKeyboard();

        if(testAttribute(Qt::WA_ShowModal))          // just be sure we leave modal
            QApplicationPrivate::leaveModal(this);
        else if((windowType() == Qt::Popup))
            qApp->d_func()->closePopup(this);
        if (destroyWindow) {
            if(OSViewRef hiview = qt_mac_nativeview_for(this)) {
                OSWindowRef window = 0;
                NSDrawer *drawer = nil;
#ifdef QT_MAC_USE_COCOA
                if (qt_mac_is_macdrawer(this)) {
                    drawer = qt_mac_drawer_for(this);
                } else
#endif
                if (isWindow())
                    window = qt_mac_window_for(hiview);

                // Because of how "destruct" works, we have to do just a normal release for the root_win.
                if (window && window == qt_root_win) {
#ifndef QT_MAC_USE_COCOA
                    CFRelease(hiview);
#else
                    [hiview release];
#endif
                } else {
                    qt_mac_destructView(hiview);
                }
                if (drawer)
                    qt_mac_destructDrawer(drawer);
                if (window)
                    qt_mac_destructWindow(window);
            }
        }
        QT_TRY {
            d->setWinId(0);
        } QT_CATCH (const std::bad_alloc &) {
            // swallow - destructors must not throw
	}
    }
}

void QWidgetPrivate::transferChildren()
{
    Q_Q(QWidget);
    if (!q->internalWinId())
        return;  // Can't add any views anyway

    QObjectList chlist = q->children();
    for (int i = 0; i < chlist.size(); ++i) {
        QObject *obj = chlist.at(i);
        if (obj->isWidgetType()) {
            QWidget *w = (QWidget *)obj;
            if (!w->isWindow()) {
                // This seems weird, no need to call it in a loop right?
                if (!topData()->caption.isEmpty())
                    setWindowTitle_helper(extra->topextra->caption);
                if (w->internalWinId()) {
#ifndef QT_MAC_USE_COCOA
                    HIViewAddSubview(qt_mac_nativeview_for(q), qt_mac_nativeview_for(w));
#else
                    // New NSWindows get an extra reference when drops are
                    // registered (at least in 10.5) which means that we may
                    // access the window later and get a crash (becasue our
                    // widget is dead). Work around this be having the drop
                    // site disabled until it is part of the new hierarchy.
                    bool oldRegistered = w->testAttribute(Qt::WA_DropSiteRegistered);
                    w->setAttribute(Qt::WA_DropSiteRegistered, false);
                    [qt_mac_nativeview_for(w) retain];
                    [qt_mac_nativeview_for(w) removeFromSuperview];
                    [qt_mac_nativeview_for(q) addSubview:qt_mac_nativeview_for(w)];
                    [qt_mac_nativeview_for(w) release];
                    w->setAttribute(Qt::WA_DropSiteRegistered, oldRegistered);
#endif
                }
            }
        }
    }
}

#ifdef QT_MAC_USE_COCOA
void QWidgetPrivate::setSubWindowStacking(bool set)
{
    // After hitting too many unforeseen bugs trying to put Qt on top of the cocoa child
    // window API, we have decided to revert this behaviour as much as we can. We
    // therefore now only allow child windows to exist for children of modal dialogs.
    static bool use_behaviour_qt473 = !qgetenv("QT_MAC_USE_CHILDWINDOWS").isEmpty();

    // This will set/remove a visual relationship between parent and child on screen.
    // The reason for doing this is to ensure that a child always stacks infront of
    // its parent. Unfortunatly is turns out that [NSWindow addChildWindow] has
    // several unwanted side-effects, one of them being the moving of a child when
    // moving the parent, which we choose to accept. A way tougher side-effect is
    // that Cocoa will hide the parent if you hide the child. And in the case of
    // a tool window, since it will normally hide when you deactivate the
    // application, Cocoa will hide the parent upon deactivate as well. The result often
    // being no more visible windows on screen. So, to make a long story short, we only
    // allow parent-child relationships between windows that both are either a plain window
    // or a dialog.

    Q_Q(QWidget);
    if (!q->isWindow())
        return;
    NSWindow *qwin = [qt_mac_nativeview_for(q) window];
    if (!qwin)
        return;
    Qt::WindowType qtype = q->windowType();
    if (set && !(qtype == Qt::Window || qtype == Qt::Dialog))
        return;
    if (set && ![qwin isVisible])
        return;

    if (QWidget *parent = q->parentWidget()) {
        if (NSWindow *pwin = [qt_mac_nativeview_for(parent) window]) {
            if (set) {
                Qt::WindowType ptype = parent->window()->windowType();
                if ([pwin isVisible]
                    && (ptype == Qt::Window || ptype == Qt::Dialog)
                    && ![qwin parentWindow]
                    && (use_behaviour_qt473 || parent->windowModality() == Qt::ApplicationModal)) {
                    NSInteger level = [qwin level];
                    [pwin addChildWindow:qwin ordered:NSWindowAbove];
                    if ([qwin level] < level)
                        [qwin setLevel:level];
                }
            } else {
                [pwin removeChildWindow:qwin];
            }
        }
    }

    // Only set-up child windows for q if q is modal:
    if (set && !use_behaviour_qt473 && q->windowModality() != Qt::ApplicationModal)
        return;

    QObjectList widgets = q->children();
    for (int i=0; i<widgets.size(); ++i) {
        QWidget *child = qobject_cast<QWidget *>(widgets.at(i));
        if (child && child->isWindow()) {
            if (NSWindow *cwin = [qt_mac_nativeview_for(child) window]) {
                if (set) {
                    Qt::WindowType ctype = child->window()->windowType();
                    if ([cwin isVisible] && (ctype == Qt::Window || ctype == Qt::Dialog) && ![cwin parentWindow]) {
                        NSInteger level = [cwin level];
                        [qwin addChildWindow:cwin ordered:NSWindowAbove];
                        if ([cwin level] < level)
                            [cwin setLevel:level];
                    }
                } else {
                    [qwin removeChildWindow:qt_mac_window_for(child)];
                }
            }
        }
    }
}
#endif

void QWidgetPrivate::setParent_sys(QWidget *parent, Qt::WindowFlags f)
{
    Q_Q(QWidget);
    QMacCocoaAutoReleasePool pool;
    QTLWExtra *topData = maybeTopData();
    bool wasCreated = q->testAttribute(Qt::WA_WState_Created);
#ifdef QT_MAC_USE_COCOA
    bool wasWindow = q->isWindow();
#endif
    OSViewRef old_id = 0;

    if (q->isVisible() && q->parentWidget() && parent != q->parentWidget())
        q->parentWidget()->d_func()->invalidateBuffer(effectiveRectFor(q->geometry()));

    // Maintain the glWidgets list on parent change: remove "our" gl widgets
    // from the list on the old parent and grandparents.
    if (glWidgets.isEmpty() == false) {
        QWidget *current = q->parentWidget();
        while (current) {
            for (QList<QWidgetPrivate::GlWidgetInfo>::const_iterator it = glWidgets.constBegin();
                 it != glWidgets.constEnd(); ++it)
                current->d_func()->glWidgets.removeAll(*it);

            if (current->isWindow())
                break;
            current = current->parentWidget();
        }
    }

#ifndef QT_MAC_USE_COCOA
    EventHandlerRef old_window_event = 0;
#else
    bool oldToolbarVisible = false;
    NSDrawer *oldDrawer = nil;
    NSToolbar *oldToolbar = 0;
#endif
    if (wasCreated && !(q->windowType() == Qt::Desktop)) {
        old_id = qt_mac_nativeview_for(q);
#ifndef QT_MAC_USE_COCOA
        old_window_event = window_event;
#else
        if (qt_mac_is_macdrawer(q)) {
            oldDrawer = qt_mac_drawer_for(q);
        }
        if (wasWindow) {
            OSWindowRef oldWindow = qt_mac_window_for(old_id);
            oldToolbar = [oldWindow toolbar];
            if (oldToolbar) {
                [oldToolbar retain];
                oldToolbarVisible = [oldToolbar isVisible];
                [oldWindow setToolbar:nil];
            }
        }
#endif
    }
    QWidget* oldtlw = q->window();

    if (q->testAttribute(Qt::WA_DropSiteRegistered))
        q->setAttribute(Qt::WA_DropSiteRegistered, false);

    //recreate and setup flags
    QObjectPrivate::setParent_helper(parent);
    bool explicitlyHidden = q->testAttribute(Qt::WA_WState_Hidden) && q->testAttribute(Qt::WA_WState_ExplicitShowHide);
    if (wasCreated && !qt_isGenuineQWidget(q))
        return;

    if (!q->testAttribute(Qt::WA_WState_WindowOpacitySet)) {
        q->setWindowOpacity(1.0f);
        q->setAttribute(Qt::WA_WState_WindowOpacitySet, false);
    }

    setWinId(0); //do after the above because they may want the id

    data.window_flags = f;
    q->setAttribute(Qt::WA_WState_Created, false);
    q->setAttribute(Qt::WA_WState_Visible, false);
    q->setAttribute(Qt::WA_WState_Hidden, false);
    adjustFlags(data.window_flags, q);
    // keep compatibility with previous versions, we need to preserve the created state.
    // (but we recreate the winId for the widget being reparented, again for compatibility,
    // unless this is an alien widget. )
    const bool nonWindowWithCreatedParent = !q->isWindow() && parent->testAttribute(Qt::WA_WState_Created);
    const bool nativeWidget = q->internalWinId() != 0;
    if (wasCreated || (nativeWidget && nonWindowWithCreatedParent)) {
        createWinId();
        if (q->isWindow()) {
#ifndef QT_MAC_USE_COCOA
            // We do this down below for wasCreated, so avoid doing this twice
            // (only for performance, it gets called a lot anyway).
            if (!wasCreated) {
                if (QMainWindowLayout *mwl = qt_mainwindow_layout(qobject_cast<QMainWindow *>(q))) {
                    mwl->updateHIToolBarStatus();
                }
            }
#else
            // Simply transfer our toolbar over. Everything should stay put, unlike in Carbon.
            if (oldToolbar && !(f & Qt::FramelessWindowHint)) {
                OSWindowRef newWindow = qt_mac_window_for(q);
                [newWindow setToolbar:oldToolbar];
                [oldToolbar release];
                [oldToolbar setVisible:oldToolbarVisible];
            }
#endif
        }
    }
    if (q->isWindow() || (!parent || parent->isVisible()) || explicitlyHidden)
        q->setAttribute(Qt::WA_WState_Hidden);
    q->setAttribute(Qt::WA_WState_ExplicitShowHide, explicitlyHidden);

#ifdef QT_MAC_USE_COCOA
    // If we add a child to the unified toolbar, we have to redirect the painting.
    if (parent && parent->d_func() && parent->d_func()->isInUnifiedToolbar) {
        if (parent->d_func()->unifiedSurface) {
            QWidget *toolbar = parent->d_func()->toolbar_ancestor;
            parent->d_func()->unifiedSurface->recursiveRedirect(toolbar, toolbar, toolbar->d_func()->toolbar_offset);
        }
    }
#endif // QT_MAC_USE_COCOA

    if (wasCreated) {
        transferChildren();
#ifndef QT_MAC_USE_COCOA
        // If we were a unified window, We just transfered our toolbars out of the unified toolbar.
        // So redo the status one more time. It apparently is not an issue with Cocoa.
        if (q->isWindow()) {
            if (QMainWindowLayout *mwl = qt_mainwindow_layout(qobject_cast<QMainWindow *>(q))) {
                mwl->updateHIToolBarStatus();
            }
        }
#endif

        if (topData &&
                (!topData->caption.isEmpty() || !topData->filePath.isEmpty()))
            setWindowTitle_helper(q->windowTitle());
    }

    if (q->testAttribute(Qt::WA_AcceptDrops)
        || (!q->isWindow() && q->parentWidget()
            && q->parentWidget()->testAttribute(Qt::WA_DropSiteRegistered)))
        q->setAttribute(Qt::WA_DropSiteRegistered, true);

    //cleanup
#ifndef QT_MAC_USE_COCOA
    if (old_window_event)
        RemoveEventHandler(old_window_event);
#endif
    if (old_id) { //don't need old window anymore
        OSWindowRef window = (oldtlw == q) ? qt_mac_window_for(old_id) : 0;
        qt_mac_destructView(old_id);

#ifdef QT_MAC_USE_COCOA
        if (oldDrawer) {
            qt_mac_destructDrawer(oldDrawer);
        } else
#endif
        if (window)
            qt_mac_destructWindow(window);
    }

    // Maintain the glWidgets list on parent change: add "our" gl widgets
    // to the list on the new parent and grandparents.
    if (glWidgets.isEmpty() == false) {
        QWidget *current = q->parentWidget();
        while (current) {
            current->d_func()->glWidgets += glWidgets;
            if (current->isWindow())
                break;
            current = current->parentWidget();
        }
    }
    invalidateBuffer(q->rect());
    qt_event_request_window_change(q);
}

QPoint QWidget::mapToGlobal(const QPoint &pos) const
{
    Q_D(const QWidget);
    if (!internalWinId()) {
        QPoint p = pos + data->crect.topLeft();
        return isWindow() ?  p : parentWidget()->mapToGlobal(p);
    }
#ifndef QT_MAC_USE_COCOA
    QPoint tmp = d->mapToWS(pos);
    HIPoint hi_pos = CGPointMake(tmp.x(), tmp.y());
    HIViewConvertPoint(&hi_pos, qt_mac_nativeview_for(this), 0);
    Rect win_rect;
    GetWindowBounds(qt_mac_window_for(this), kWindowStructureRgn, &win_rect);
    return QPoint((int)hi_pos.x+win_rect.left, (int)hi_pos.y+win_rect.top);
#else
    QPoint tmp = d->mapToWS(pos);
    NSPoint hi_pos = NSMakePoint(tmp.x(), tmp.y());
    hi_pos = [qt_mac_nativeview_for(this) convertPoint:hi_pos toView:nil];
    NSRect win_rect = [qt_mac_window_for(this) frame];
    hi_pos.x += win_rect.origin.x;
    hi_pos.y += win_rect.origin.y;
    // If we aren't the desktop we need to flip, if you flip the desktop on itself, you get the other problem.
    return ((window()->windowFlags() & Qt::Desktop) == Qt::Desktop) ? QPointF(hi_pos.x, hi_pos.y).toPoint()
                                                                    : flipPoint(hi_pos).toPoint();
#endif
}

QPoint QWidget::mapFromGlobal(const QPoint &pos) const
{
    Q_D(const QWidget);
    if (!internalWinId()) {
        QPoint p = isWindow() ?  pos : parentWidget()->mapFromGlobal(pos);
        return p - data->crect.topLeft();
    }
#ifndef QT_MAC_USE_COCOA
    Rect win_rect;
    GetWindowBounds(qt_mac_window_for(this), kWindowStructureRgn, &win_rect);
    HIPoint hi_pos = CGPointMake(pos.x()-win_rect.left, pos.y()-win_rect.top);
    HIViewConvertPoint(&hi_pos, 0, qt_mac_nativeview_for(this));
    return d->mapFromWS(QPoint((int)hi_pos.x, (int)hi_pos.y));
#else
    NSRect win_rect = [qt_mac_window_for(this) frame];
    // The Window point is in "Cocoa coordinates," but the view is in "Qt coordinates"
    // so make sure to keep them in sync.
    NSPoint hi_pos = NSMakePoint(pos.x()-win_rect.origin.x,
                                 flipYCoordinate(pos.y())-win_rect.origin.y);
    hi_pos = [qt_mac_nativeview_for(this) convertPoint:hi_pos fromView:0];
    return d->mapFromWS(QPoint(qRound(hi_pos.x), qRound(hi_pos.y)));
#endif
}

void QWidgetPrivate::updateSystemBackground()
{
}

void QWidgetPrivate::setCursor_sys(const QCursor &)
{
    qt_mac_update_cursor();
}

void QWidgetPrivate::unsetCursor_sys()
{
    qt_mac_update_cursor();
}

void QWidgetPrivate::setWindowTitle_sys(const QString &caption)
{
    Q_Q(QWidget);
    if (q->isWindow()) {
#ifndef QT_MAC_USE_COCOA
        SetWindowTitleWithCFString(qt_mac_window_for(q), QCFString(caption));
#else
        QMacCocoaAutoReleasePool pool;
        [qt_mac_window_for(q) setTitle:qt_mac_QStringToNSString(caption)];
#endif
    }
}

void QWidgetPrivate::setWindowModified_sys(bool mod)
{
    Q_Q(QWidget);
    if (q->isWindow() && q->testAttribute(Qt::WA_WState_Created)) {
#ifndef QT_MAC_USE_COCOA
        SetWindowModified(qt_mac_window_for(q), mod);
#else
        [qt_mac_window_for(q) setDocumentEdited:mod];
#endif
    }
}

void QWidgetPrivate::setWindowFilePath_sys(const QString &filePath)
{
    Q_Q(QWidget);
#ifdef QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
    QFileInfo fi(filePath);
    [qt_mac_window_for(q) setRepresentedFilename:fi.exists() ? qt_mac_QStringToNSString(filePath) : @""];
#else
    bool validRef = false;
    FSRef ref;
    bzero(&ref, sizeof(ref));
    OSStatus status;

    if (!filePath.isEmpty()) {
        status = FSPathMakeRef(reinterpret_cast<const UInt8 *>(filePath.toUtf8().constData()), &ref, 0);
        validRef = (status == noErr);
    }
    // Set the proxy regardless, since this is our way of clearing it as well, but ignore the
    // return value as well.
    if (validRef) {
        status = HIWindowSetProxyFSRef(qt_mac_window_for(q), &ref);
    } else {
        status = RemoveWindowProxy(qt_mac_window_for(q));
    }
    if (status != noErr)
        qWarning("QWidget::setWindowFilePath: Error setting proxyicon for path (%s):%ld",
                qPrintable(filePath), status);
#endif
}

void QWidgetPrivate::setWindowIcon_sys(bool forceReset)
{
    Q_Q(QWidget);

    if (!q->testAttribute(Qt::WA_WState_Created))
        return;

    QTLWExtra *topData = this->topData();
    if (topData->iconPixmap && !forceReset) // already set
        return;

    QIcon icon = q->windowIcon();
    QPixmap *pm = 0;
    if (!icon.isNull()) {
        // now create the extra
        if (!topData->iconPixmap) {
            pm = new QPixmap(icon.pixmap(QSize(22, 22)));
            topData->iconPixmap = pm;
        } else {
            pm = topData->iconPixmap;
        }
    }
    if (q->isWindow()) {
#ifndef QT_MAC_USE_COCOA
        IconRef previousIcon = 0;
        if (icon.isNull()) {
            RemoveWindowProxy(qt_mac_window_for(q));
            previousIcon = topData->windowIcon;
            topData->windowIcon = 0;
        } else {
            WindowClass wclass;
            GetWindowClass(qt_mac_window_for(q), &wclass);

            if (wclass == kDocumentWindowClass) {
                IconRef newIcon = qt_mac_create_iconref(*pm);
                previousIcon = topData->windowIcon;
                topData->windowIcon = newIcon;
                SetWindowProxyIcon(qt_mac_window_for(q), newIcon);
            }
        }

        // Release the previous icon if it was set by this function.
        if (previousIcon != 0)
            ReleaseIconRef(previousIcon);
#else
        QMacCocoaAutoReleasePool pool;
        if (icon.isNull())
            return;
        NSButton *iconButton = [qt_mac_window_for(q) standardWindowButton:NSWindowDocumentIconButton];
        if (iconButton == nil) {
            QCFString string(q->windowTitle());
            const NSString *tmpString = reinterpret_cast<const NSString *>((CFStringRef)string);
            [qt_mac_window_for(q) setRepresentedURL:[NSURL fileURLWithPath:const_cast<NSString *>(tmpString)]];
            iconButton = [qt_mac_window_for(q) standardWindowButton:NSWindowDocumentIconButton];
        }
        if (icon.isNull()) {
            [iconButton setImage:nil];
        } else {
            QPixmap scaled = pm->scaled(QSize(16,16), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            NSImage *image = static_cast<NSImage *>(qt_mac_create_nsimage(scaled));
            [iconButton setImage:image];
            [image release];
        }
#endif
    }
}

void QWidgetPrivate::setWindowIconText_sys(const QString &iconText)
{
    Q_Q(QWidget);
    if(q->isWindow() && !iconText.isEmpty()) {
#ifndef QT_MAC_USE_COCOA
        SetWindowAlternateTitle(qt_mac_window_for(q), QCFString(iconText));
#else
        QMacCocoaAutoReleasePool pool;
        [qt_mac_window_for(q) setMiniwindowTitle:qt_mac_QStringToNSString(iconText)];
#endif
    }
}

void QWidget::grabMouse()
{
    if(isVisible() && !qt_nograb()) {
        if(mac_mouse_grabber)
            mac_mouse_grabber->releaseMouse();
        mac_mouse_grabber=this;
        qt_mac_setMouseGrabCursor(true);
    }
}

#ifndef QT_NO_CURSOR
void QWidget::grabMouse(const QCursor &cursor)
{
    if(isVisible() && !qt_nograb()) {
        if(mac_mouse_grabber)
            mac_mouse_grabber->releaseMouse();
        mac_mouse_grabber=this;
        qt_mac_setMouseGrabCursor(true, const_cast<QCursor *>(&cursor));
    }
}
#endif

void QWidget::releaseMouse()
{
    if(!qt_nograb() && mac_mouse_grabber == this) {
        mac_mouse_grabber = 0;
        qt_mac_setMouseGrabCursor(false);
    }
}

void QWidget::grabKeyboard()
{
    if(!qt_nograb()) {
        if(mac_keyboard_grabber)
            mac_keyboard_grabber->releaseKeyboard();
        mac_keyboard_grabber = this;
    }
}

void QWidget::releaseKeyboard()
{
    if(!qt_nograb() && mac_keyboard_grabber == this)
        mac_keyboard_grabber = 0;
}

QWidget *QWidget::mouseGrabber()
{
    return mac_mouse_grabber;
}

QWidget *QWidget::keyboardGrabber()
{
    return mac_keyboard_grabber;
}

void QWidget::activateWindow()
{
    QWidget *tlw = window();
    if(!tlw->isVisible() || !tlw->isWindow() || (tlw->windowType() == Qt::Desktop))
        return;
    qt_event_remove_activate();

    QWidget *fullScreenWidget = tlw;
    QWidget *parentW = tlw;
    // Find the oldest parent or the parent with fullscreen, whichever comes first.
    while (parentW) {
        fullScreenWidget = parentW->window();
        if (fullScreenWidget->windowState() & Qt::WindowFullScreen)
            break;
        parentW = fullScreenWidget->parentWidget();
    }

    if (fullScreenWidget->windowType() != Qt::ToolTip) {
        qt_mac_set_fullscreen_mode((fullScreenWidget->windowState() & Qt::WindowFullScreen) &&
                                               qApp->desktop()->screenNumber(this) == 0);
    }

    bool windowActive;
    OSWindowRef win = qt_mac_window_for(tlw);
#ifndef QT_MAC_USE_COCOA
    windowActive = IsWindowActive(win);
#else
    QMacCocoaAutoReleasePool pool;
    windowActive = [win isKeyWindow];
#endif
    if ((tlw->windowType() == Qt::Popup)
            || (tlw->windowType() == Qt::Tool)
            || qt_mac_is_macdrawer(tlw)
            || windowActive) {
#ifndef QT_MAC_USE_COCOA
        ActivateWindow(win, true);
        qApp->setActiveWindow(tlw);
#else
        [win makeKeyWindow];
#endif
    } else if(!isMinimized()) {
#ifndef QT_MAC_USE_COCOA
        SelectWindow(win);
#else
        [win makeKeyAndOrderFront:win];
#endif
    }
}

QWindowSurface *QWidgetPrivate::createDefaultWindowSurface_sys()
{
    return new QMacWindowSurface(q_func());
}

void QWidgetPrivate::update_sys(const QRect &r)
{
    Q_Q(QWidget);
    if (updateRedirectedToGraphicsProxyWidget(q, r))
        return;
    dirtyOnWidget += r;
    macSetNeedsDisplay(r != q->rect() ? r : QRegion());
}

void QWidgetPrivate::update_sys(const QRegion &rgn)
{
    Q_Q(QWidget);
    if (updateRedirectedToGraphicsProxyWidget(q, rgn))
        return;
    dirtyOnWidget += rgn;
    macSetNeedsDisplay(rgn);
}

bool QWidgetPrivate::isRealWindow() const
{
    return q_func()->isWindow() && !topData()->embedded;
}

void QWidgetPrivate::show_sys()
{
    Q_Q(QWidget);
    if ((q->windowType() == Qt::Desktop)) //desktop is always visible
        return;

    invalidateBuffer(q->rect());
    if (q->testAttribute(Qt::WA_OutsideWSRange))
        return;
    QMacCocoaAutoReleasePool pool;
    q->setAttribute(Qt::WA_Mapped);
    if (q->testAttribute(Qt::WA_DontShowOnScreen))
        return;

    bool realWindow = isRealWindow();
#ifndef QT_MAC_USE_COCOA
    if (realWindow && !q->testAttribute(Qt::WA_Moved)) {
        if (qt_mac_is_macsheet(q))
            recreateMacWindow();
        q->createWinId();
        if (QWidget *p = q->parentWidget()) {
            p->createWinId();
            RepositionWindow(qt_mac_window_for(q), qt_mac_window_for(p), kWindowCenterOnParentWindow);
        } else {
            RepositionWindow(qt_mac_window_for(q), 0, kWindowCenterOnMainScreen);
        }
    }
#endif

    data.fstrut_dirty = true;
    if (realWindow) {
        bool isCurrentlyMinimized = (q->windowState() & Qt::WindowMinimized);
        setModal_sys();
        OSWindowRef window = qt_mac_window_for(q);
#ifndef QT_MAC_USE_COCOA
        SizeWindow(window, q->width(), q->height(), true);
#endif

#ifdef QT_MAC_USE_COCOA
        // Make sure that we end up sending a repaint event to
        // the widget if the window has been visible one before:
        [qt_mac_get_contentview_for(window) setNeedsDisplay:YES];
#endif
        if(qt_mac_is_macsheet(q)) {
            qt_event_request_showsheet(q);
        } else if(qt_mac_is_macdrawer(q)) {
#ifndef QT_MAC_USE_COCOA
            OpenDrawer(window, kWindowEdgeDefault, false);
#else
            NSDrawer *drawer = qt_mac_drawer_for(q);
            [drawer openOnEdge:[drawer preferredEdge]];
#endif
        } else {
#ifndef QT_MAC_USE_COCOA
            ShowHide(window, true);
#else
            // sync the opacity value back (in case of a fade).
            [window setAlphaValue:q->windowOpacity()];

            QWidget *top = 0;
            if (QApplicationPrivate::tryModalHelper(q, &top)) {
                if (q->testAttribute(Qt::WA_ShowWithoutActivating))
                    [window orderFront:window];
                else
                    [window makeKeyAndOrderFront:window];
                // If this window is app modal, we need to start spinning
                // a modal session for it. Interrupting
                // the event dispatcher will make this happend:
                if (data.window_modality == Qt::ApplicationModal)
                    QEventDispatcherMac::instance()->interrupt();
            } else {
                // The window is modally shaddowed, so we need to make
                // sure that we don't pop in front of the modal window:
                [window orderFront:window];
                if (!top->testAttribute(Qt::WA_DontShowOnScreen)) {
                    if (NSWindow *modalWin = qt_mac_window_for(top))
                        [modalWin orderFront:window];
                }
            }
            setSubWindowStacking(true);
            qt_mac_update_cursor();
#endif
            if (q->windowType() == Qt::Popup) {
                qt_button_down = 0;
                if (q->focusWidget())
				    q->focusWidget()->d_func()->setFocus_sys();
				else
                    setFocus_sys();
			}
            toggleDrawers(true);
        }
        if (isCurrentlyMinimized) { //show in collapsed state
#ifndef QT_MAC_USE_COCOA
            CollapseWindow(window, true);
#else
            [window miniaturize:window];
#endif
        } else if (!q->testAttribute(Qt::WA_ShowWithoutActivating)) {
#ifndef QT_MAC_USE_COCOA
            qt_event_request_activate(q);
#endif
        }
    } else if(topData()->embedded || !q->parentWidget() || q->parentWidget()->isVisible()) {
#ifndef QT_MAC_USE_COCOA
        HIViewSetVisible(qt_mac_nativeview_for(q), true);
#else
        if (NSView *view = qt_mac_nativeview_for(q)) {
            // INVARIANT: q is native. Just show the view:
            [view setHidden:NO];
        } else {
            // INVARIANT: q is alien. Update q instead:
            q->update();
        }
#endif
    }

#ifdef QT_MAC_USE_COCOA
    if ([NSApp isActive] && !qt_button_down && !QWidget::mouseGrabber()){
        // Update enter/leave immidiatly, don't wait for a move event. But only
        // if no grab exists (even if the grab points to this widget, it seems, ref X11)
        QPoint qlocal, qglobal;
        QWidget *widgetUnderMouse = 0;
        qt_mac_getTargetForMouseEvent(0, QEvent::Enter, qlocal, qglobal, 0, &widgetUnderMouse);
        QApplicationPrivate::dispatchEnterLeave(widgetUnderMouse, qt_last_mouse_receiver);
        qt_last_mouse_receiver = widgetUnderMouse;
        qt_last_native_mouse_receiver = widgetUnderMouse ?
            (widgetUnderMouse->internalWinId() ? widgetUnderMouse : widgetUnderMouse->nativeParentWidget()) : 0;
    }
#endif

    topLevelAt_cache = 0;
    qt_event_request_window_change(q);
}

QPoint qt_mac_nativeMapFromParent(const QWidget *child, const QPoint &pt)
{
#ifndef QT_MAC_USE_COCOA
    CGPoint nativePoint = CGPointMake(pt.x(), pt.y());
    HIViewConvertPoint(&nativePoint, qt_mac_nativeview_for(child->parentWidget()),
                       qt_mac_nativeview_for(child));
#else
    NSPoint nativePoint = [qt_mac_nativeview_for(child) convertPoint:NSMakePoint(pt.x(), pt.y()) fromView:qt_mac_nativeview_for(child->parentWidget())];
#endif
    return QPoint(nativePoint.x, nativePoint.y);
}


void QWidgetPrivate::hide_sys()
{
    Q_Q(QWidget);
    if((q->windowType() == Qt::Desktop)) //you can't hide the desktop!
        return;
    QMacCocoaAutoReleasePool pool;
    if(q->isWindow()) {
#ifdef QT_MAC_USE_COCOA
        setSubWindowStacking(false);
#endif
        OSWindowRef window = qt_mac_window_for(q);
        if(qt_mac_is_macsheet(q)) {
#ifndef QT_MAC_USE_COCOA
            WindowRef parent = 0;
            if(GetSheetWindowParent(window, &parent) != noErr || !parent)
                ShowHide(window, false);
            else
                HideSheetWindow(window);
#else
            [NSApp endSheet:window];
            [window orderOut:window];
#endif
        } else if(qt_mac_is_macdrawer(q)) {
#ifndef QT_MAC_USE_COCOA
            CloseDrawer(window, false);
#else
            [qt_mac_drawer_for(q) close];
#endif
        } else {
#ifndef QT_MAC_USE_COCOA
            ShowHide(window, false);
#else
            [window orderOut:window];
            // Unfortunately it is not as easy as just hiding the window, we need
            // to find out if we were in full screen mode. If we were and this is
            // the last window in full screen mode then we need to unset the full screen
            // mode. If this is not the last visible window in full screen mode then we
            // don't change the full screen mode.
            if(q->isFullScreen())
            {
                bool keepFullScreen = false;
                QWidgetList windowList = qApp->topLevelWidgets();
                int windowCount = windowList.count();
                for(int i = 0; i < windowCount; i++)
                {
                    QWidget *w = windowList[i];
                    // If it is the same window, we don't need to check :-)
                    if(q == w)
                        continue;
                    // If they are not visible or if they are minimized then
                    // we just ignore them.
                    if(!w->isVisible() || w->isMinimized())
                        continue;
                    // Is it full screen?
                    // Notice that if there is one window in full screen mode then we
                    // cannot switch the full screen mode off, therefore we just abort.
                    if(w->isFullScreen()) {
                        keepFullScreen = true;
                        break;
                    }
                }
                // No windows in full screen mode, so let just unset that flag.
                if(!keepFullScreen)
                    qt_mac_set_fullscreen_mode(false);
            }
#endif
            toggleDrawers(false);
            qt_mac_update_cursor();
#ifndef QT_MAC_USE_COCOA
            // Clear modality (because it seems something that we've always done).
            if (data.window_modality != Qt::NonModal) {
                SetWindowModality(window, kWindowModalityNone,
                          q->parentWidget() ? qt_mac_window_for(q->parentWidget()->window()) : 0);
            }
#endif
        }
#ifndef QT_MAC_USE_COCOA
        // If the window we now hide was the active window, we need
        // to find, and activate another window on screen. NB: Cocoa takes care of this
        // logic for us (and distinquishes between main windows and key windows)
        if (q->isActiveWindow() && !(q->windowType() == Qt::Popup)) {
            QWidget *w = 0;
            if(q->parentWidget())
                w = q->parentWidget()->window();
            if(!w || (!w->isVisible() && !w->isMinimized())) {
                for (WindowPtr wp = GetFrontWindowOfClass(kMovableModalWindowClass, true);
                    wp; wp = GetNextWindowOfClass(wp, kMovableModalWindowClass, true)) {
                    if((w = qt_mac_find_window(wp)))
                        break;
                }
                if (!w){
                    for (WindowPtr wp = GetFrontWindowOfClass(kDocumentWindowClass, true);
                            wp; wp = GetNextWindowOfClass(wp, kDocumentWindowClass, true)) {
                        if((w = qt_mac_find_window(wp)))
                            break;
                    }
                }
                if (!w){
                    for(WindowPtr wp = GetFrontWindowOfClass(kSimpleWindowClass, true);
                        wp; wp = GetNextWindowOfClass(wp, kSimpleWindowClass, true)) {
                        if((w = qt_mac_find_window(wp)))
                            break;
                    }
                }
            }
            if(w && w->isVisible() && !w->isMinimized()) {
                qt_event_request_activate(w);
            }
        }
#endif
    } else {
         invalidateBuffer(q->rect());
#ifndef QT_MAC_USE_COCOA
        HIViewSetVisible(qt_mac_nativeview_for(q), false);
#else
        if (NSView *view = qt_mac_nativeview_for(q)) {
            // INVARIANT: q is native. Just hide the view:
            [view setHidden:YES];
        } else {
            // INVARIANT: q is alien. Repaint where q is placed instead:
            qt_mac_updateParentUnderAlienWidget(q);
        }
#endif
    }

#ifdef QT_MAC_USE_COCOA
    if ([NSApp isActive] && !qt_button_down && !QWidget::mouseGrabber()){
        // Update enter/leave immidiatly, don't wait for a move event. But only
        // if no grab exists (even if the grab points to this widget, it seems, ref X11)
        QPoint qlocal, qglobal;
        QWidget *widgetUnderMouse = 0;
        qt_mac_getTargetForMouseEvent(0, QEvent::Leave, qlocal, qglobal, 0, &widgetUnderMouse);
        QApplicationPrivate::dispatchEnterLeave(widgetUnderMouse, qt_last_native_mouse_receiver);
        qt_last_mouse_receiver = widgetUnderMouse;
        qt_last_native_mouse_receiver = widgetUnderMouse ?
            (widgetUnderMouse->internalWinId() ? widgetUnderMouse : widgetUnderMouse->nativeParentWidget()) : 0;
     }
#endif

    topLevelAt_cache = 0;
    qt_event_request_window_change(q);
    deactivateWidgetCleanup();
    qt_mac_event_release(q);
}

void QWidget::setWindowState(Qt::WindowStates newstate)
{
    Q_D(QWidget);
    bool needShow = false;
    Qt::WindowStates oldstate = windowState();
    if (oldstate == newstate)
        return;

#ifdef QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
#endif
    bool needSendStateChange = true;
    if(isWindow()) {
        if((oldstate & Qt::WindowFullScreen) != (newstate & Qt::WindowFullScreen)) {
            if(newstate & Qt::WindowFullScreen) {
                if(QTLWExtra *tlextra = d->topData()) {
                    if(tlextra->normalGeometry.width() < 0) {
                        if(!testAttribute(Qt::WA_Resized))
                            adjustSize();
                        tlextra->normalGeometry = geometry();
                    }
                    tlextra->savedFlags = windowFlags();
                }
                needShow = isVisible();
                const QRect fullscreen(qApp->desktop()->screenGeometry(qApp->desktop()->screenNumber(this)));
                setParent(parentWidget(), Qt::Window | Qt::FramelessWindowHint | (windowFlags() & 0xffff0000)); //save
                setGeometry(fullscreen);
                if(!qApp->desktop()->screenNumber(this))
                    qt_mac_set_fullscreen_mode(true);
            } else {
                needShow = isVisible();
                if(!qApp->desktop()->screenNumber(this))
                    qt_mac_set_fullscreen_mode(false);
                setParent(parentWidget(), d->topData()->savedFlags);
                setGeometry(d->topData()->normalGeometry);
                d->topData()->normalGeometry.setRect(0, 0, -1, -1);
            }
        }

        d->createWinId();

        OSWindowRef window = qt_mac_window_for(this);
        if((oldstate & Qt::WindowMinimized) != (newstate & Qt::WindowMinimized)) {
            if (newstate & Qt::WindowMinimized) {
#ifndef QT_MAC_USE_COCOA
                CollapseWindow(window, true);
#else
                [window miniaturize:window];
#endif
            } else {
#ifndef QT_MAC_USE_COCOA
                CollapseWindow(window, false);
#else
                [window deminiaturize:window];
#endif
            }
            needSendStateChange = oldstate == windowState(); // Collapse didn't change our flags.
        }

        if((newstate & Qt::WindowMaximized) && !((newstate & Qt::WindowFullScreen))) {
            if(QTLWExtra *tlextra = d->topData()) {
                if(tlextra->normalGeometry.width() < 0) {
                    if(!testAttribute(Qt::WA_Resized))
                        adjustSize();
                    tlextra->normalGeometry = geometry();
                }
            }
        } else if(!(newstate & Qt::WindowFullScreen)) {
//            d->topData()->normalGeometry = QRect(0, 0, -1, -1);
        }

#ifdef DEBUG_WINDOW_STATE
#define WSTATE(x) qDebug("%s -- %s --> %s", #x, (oldstate & x) ? "true" : "false", (newstate & x) ? "true" : "false")
        WSTATE(Qt::WindowMinimized);
        WSTATE(Qt::WindowMaximized);
        WSTATE(Qt::WindowFullScreen);
#undef WSTATE
#endif
        if(!(newstate & (Qt::WindowMinimized|Qt::WindowFullScreen)) &&
           ((oldstate & Qt::WindowFullScreen) || (oldstate & Qt::WindowMinimized) ||
            (oldstate & Qt::WindowMaximized) != (newstate & Qt::WindowMaximized))) {
            if(newstate & Qt::WindowMaximized) {
                data->fstrut_dirty = true;
#ifndef QT_MAC_USE_COCOA
                HIToolbarRef toolbarRef;
                if (GetWindowToolbar(window, &toolbarRef) == noErr && toolbarRef
                        && !isVisible() && !IsWindowToolbarVisible(window)) {
                    // HIToolbar, needs to be shown so that it's in the structure window
                    // Typically this is part of a main window and will get shown
                    // during the show, but it's will make the maximize all wrong.
                    ShowHideWindowToolbar(window, true, false);
                    d->updateFrameStrut();  // In theory the dirty would work, but it's optimized out if the window is not visible :(
                }
                Rect bounds;
                QDesktopWidget *dsk = QApplication::desktop();
                QRect avail = dsk->availableGeometry(dsk->screenNumber(this));
                SetRect(&bounds, avail.x(), avail.y(), avail.x() + avail.width(), avail.y() + avail.height());
                if(QWExtra *extra = d->extraData()) {
                    if(bounds.right - bounds.left > extra->maxw)
                        bounds.right = bounds.left + extra->maxw;
                    if(bounds.bottom - bounds.top > extra->maxh)
                        bounds.bottom = bounds.top + extra->maxh;
                }
                if(d->topData()) {
                    QRect fs = d->frameStrut();
                    bounds.left += fs.left();
                    if(bounds.right < avail.x()+avail.width())
                        bounds.right = qMin<short>((uint)avail.x()+avail.width(), bounds.right+fs.left());
                    if(bounds.bottom < avail.y()+avail.height())
                        bounds.bottom = qMin<short>((uint)avail.y()+avail.height(), bounds.bottom+fs.top());
                    bounds.top += fs.top();
                    bounds.right -= fs.right();
                    bounds.bottom -= fs.bottom();
                }
                QRect orect(geometry().x(), geometry().y(), width(), height()),
                      nrect(bounds.left, bounds.top, bounds.right - bounds.left,
                            bounds.bottom - bounds.top);
                if(orect != nrect) { // the new rect differ from the old
                    Point idealSize  = { nrect.height(), nrect.width() };
                    ZoomWindowIdeal(window, inZoomOut, &idealSize);
                }
#else
                NSToolbar *toolbarRef = [window toolbar];
                if (toolbarRef && !isVisible() && ![toolbarRef isVisible]) {
                    // HIToolbar, needs to be shown so that it's in the structure window
                    // Typically this is part of a main window and will get shown
                    // during the show, but it's will make the maximize all wrong.
                    // ### Not sure this is right for NSToolbar...
                    [toolbarRef setVisible:true];
//                    ShowHideWindowToolbar(window, true, false);
                    d->updateFrameStrut();  // In theory the dirty would work, but it's optimized out if the window is not visible :(
                }
                // Everything should be handled by Cocoa.
                [window zoom:window];
#endif
                needSendStateChange = oldstate == windowState(); // Zoom didn't change flags.
            } else if(oldstate & Qt::WindowMaximized && !(oldstate & Qt::WindowFullScreen)) {
#ifndef QT_MAC_USE_COCOA
                Point idealSize;
                ZoomWindowIdeal(window, inZoomIn, &idealSize);
#else
                [window zoom:window];
#endif
                if(QTLWExtra *tlextra = d->topData()) {
                    setGeometry(tlextra->normalGeometry);
                    tlextra->normalGeometry.setRect(0, 0, -1, -1);
                }
            }
        }
    }

    data->window_state = newstate;

    if(needShow)
        show();

    if(newstate & Qt::WindowActive)
        activateWindow();

    qt_event_request_window_change(this);
    if (needSendStateChange) {
        QWindowStateChangeEvent e(oldstate);
        QApplication::sendEvent(this, &e);
    }
}

void QWidgetPrivate::setFocus_sys()
{
    Q_Q(QWidget);
    if (q->testAttribute(Qt::WA_WState_Created)) {
#ifdef QT_MAC_USE_COCOA
        QMacCocoaAutoReleasePool pool;
        NSView *view = qt_mac_nativeview_for(q);
        [[view window] makeFirstResponder:view];
#else
        SetKeyboardFocus(qt_mac_window_for(q), qt_mac_nativeview_for(q), 1);
#endif
    }
}

NSComparisonResult compareViews2Raise(id view1, id view2, void *context)
{
    id topView = reinterpret_cast<id>(context);
    if (view1 == topView)
        return NSOrderedDescending;
    if (view2 == topView)
        return NSOrderedAscending;
    return NSOrderedSame;
}

void QWidgetPrivate::raise_sys()
{
    Q_Q(QWidget);
    if((q->windowType() == Qt::Desktop))
        return;

#if QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
    if (isRealWindow()) {
        // With the introduction of spaces it is not as simple as just raising the window.
        // First we need to check if we are in the right space. If we are, then we just continue
        // as usual. The problem comes when we are not in the active space. There are two main cases:
        // 1. Our parent was moved to a new space. In this case we want the window to be raised
        // in the same space as its parent.
        // 2. We don't have a parent. For this case we will just raise the window and let Cocoa
        // switch to the corresponding space.
        // NOTICE: There are a lot of corner cases here. We are keeping this simple for now, if
        // required we will introduce special handling for some of them.
        if (!q->testAttribute(Qt::WA_DontShowOnScreen) && q->isVisible()) {
            OSWindowRef window = qt_mac_window_for(q);
            // isOnActiveSpace is available only from 10.6 onwards, so we need to check if it is
            // available before calling it.
            if([window respondsToSelector:@selector(isOnActiveSpace)]) {
                if(![window performSelector:@selector(isOnActiveSpace)]) {
                    QWidget *parentWidget = q->parentWidget();
                    if(parentWidget) {
                        OSWindowRef parentWindow = qt_mac_window_for(parentWidget);
                        if(parentWindow && [parentWindow respondsToSelector:@selector(isOnActiveSpace)]) {
                            if ([parentWindow performSelector:@selector(isOnActiveSpace)]) {
                                // The window was created in a different space. Therefore if we want
                                // to show it in the current space we need to recreate it in the new
                                // space.
                                recreateMacWindow();
                                window = qt_mac_window_for(q);
                            }
                        }
                    }
                }
            }
            [window orderFront:window];
        }
        if (qt_mac_raise_process) { //we get to be the active process now
            ProcessSerialNumber psn;
            GetCurrentProcess(&psn);
            SetFrontProcessWithOptions(&psn, kSetFrontProcessFrontWindowOnly);
        }
    } else {
        NSView *view = qt_mac_nativeview_for(q);
        NSView *parentView = [view superview];
        [parentView sortSubviewsUsingFunction:compareViews2Raise context:reinterpret_cast<void *>(view)];
    }
    topLevelAt_cache = 0;
#else
    if(q->isWindow()) {
        //raise this window
        BringToFront(qt_mac_window_for(q));
        if(qt_mac_raise_process) { //we get to be the active process now
            ProcessSerialNumber psn;
            GetCurrentProcess(&psn);
            SetFrontProcessWithOptions(&psn, kSetFrontProcessFrontWindowOnly);
        }
    } else if(q->parentWidget()) {
        HIViewSetZOrder(qt_mac_nativeview_for(q), kHIViewZOrderAbove, 0);
        qt_event_request_window_change(q);
    }
#endif
}

NSComparisonResult compareViews2Lower(id view1, id view2, void *context)
{
    id topView = reinterpret_cast<id>(context);
    if (view1 == topView)
        return NSOrderedAscending;
    if (view2 == topView)
        return NSOrderedDescending;
    return NSOrderedSame;
}

void QWidgetPrivate::lower_sys()
{
    Q_Q(QWidget);
    if((q->windowType() == Qt::Desktop))
        return;
#ifdef QT_MAC_USE_COCOA
    if (isRealWindow()) {
        OSWindowRef window = qt_mac_window_for(q);
        [window orderBack:window];
    } else {
        NSView *view = qt_mac_nativeview_for(q);
        NSView *parentView = [view superview];
        [parentView sortSubviewsUsingFunction:compareViews2Lower context:reinterpret_cast<void *>(view)];
    }
    topLevelAt_cache = 0;
#else
    if(q->isWindow()) {
        SendBehind(qt_mac_window_for(q), 0);
    } else if(q->parentWidget()) {
        invalidateBuffer(q->rect());
        HIViewSetZOrder(qt_mac_nativeview_for(q), kHIViewZOrderBelow, 0);
        qt_event_request_window_change(q);
    }
#endif
}

NSComparisonResult compareViews2StackUnder(id view1, id view2, void *context)
{
    const QHash<NSView *, int> &viewOrder = *reinterpret_cast<QHash<NSView *, int> *>(context);
    if (viewOrder[view1] < viewOrder[view2])
        return NSOrderedAscending;
    if (viewOrder[view1] > viewOrder[view2])
        return NSOrderedDescending;
    return NSOrderedSame;
}

void QWidgetPrivate::stackUnder_sys(QWidget *w)
{
    // stackUnder
    Q_Q(QWidget);
    if(!w || q->isWindow() || (q->windowType() == Qt::Desktop))
        return;
#ifdef QT_MAC_USE_COCOA
    // Do the same trick as lower_sys() and put this widget before the widget passed in.
    NSView *myView = qt_mac_nativeview_for(q);
    NSView *wView = qt_mac_nativeview_for(w);

    QHash<NSView *, int> viewOrder;
    NSView *parentView = [myView superview];
    NSArray *subviews = [parentView subviews];
    NSUInteger index = 1;
    // make a hash of view->zorderindex and make sure z-value is always odd,
    // so that when we modify the order we create a new (even) z-value which
    // will not interfere with others.
    for (NSView *subview in subviews) {
        viewOrder.insert(subview, index * 2);
        ++index;
    }
    viewOrder[myView] = viewOrder[wView] - 1;

    [parentView sortSubviewsUsingFunction:compareViews2StackUnder context:reinterpret_cast<void *>(&viewOrder)];
#else
    QWidget *p = q->parentWidget();
    if(!p || p != w->parentWidget())
        return;
    invalidateBuffer(q->rect());
    HIViewSetZOrder(qt_mac_nativeview_for(q), kHIViewZOrderBelow, qt_mac_nativeview_for(w));
    qt_event_request_window_change(q);
#endif
}

#ifndef QT_MAC_USE_COCOA
/*
    Modifies the bounds for a widgets backing HIView during moves and resizes. Also updates the
    widget, either by scrolling its contents or repainting, depending on the WA_StaticContents
    flag
*/
static void qt_mac_update_widget_position(QWidget *q, QRect oldRect, QRect newRect)
{
    HIRect bounds = CGRectMake(newRect.x(), newRect.y(),
                               newRect.width(), newRect.height());

    const HIViewRef view = qt_mac_nativeview_for(q);
    const bool isMove = (oldRect.topLeft() != newRect.topLeft());
    const bool isResize = (oldRect.size() != newRect.size());

//    qDebug() << oldRect << newRect << isMove << isResize << q->testAttribute(Qt::WA_OpaquePaintEvent) << q->testAttribute(Qt::WA_StaticContents);
    QWidgetPrivate *qd = qt_widget_private(q);

    // Perform a normal (complete repaint) update in some cases:
    if (
        // always repaint on move.
        (isMove) ||

        // limited update on resize requires WA_StaticContents.
        (isResize && q->testAttribute(Qt::WA_StaticContents) == false) ||

        // one of the rects are invalid
        (oldRect.isValid() == false || newRect.isValid() == false)  ||

        // the position update is a part of a drag-and-drop operation
        QDragManager::self()->object ||

        // we are on Panther (no HIViewSetNeedsDisplayInRect)
        QSysInfo::MacintoshVersion < QSysInfo::MV_10_4
    ){
        HIViewSetFrame(view, &bounds);
        return;
    }

    const int dx = newRect.x() - oldRect.x();
    const int dy = newRect.y() - oldRect.y();

    if (isMove) {
        // HIViewScrollRect silently fails if we try to scroll anything under the grow box.
        // Check if there's one present within the widget rect, and if there is fall back
        // to repainting the entire widget.
        QWidget const * const parentWidget = q->parentWidget();
        const HIViewRef parentView = qt_mac_nativeview_for(parentWidget);
        HIViewRef nativeSizeGrip = 0;
        if (q->testAttribute(Qt::WA_WState_Created))
            HIViewFindByID(HIViewGetRoot(HIViewGetWindow(HIViewRef(q->winId()))), kHIViewWindowGrowBoxID, &nativeSizeGrip);
        if (nativeSizeGrip) {
            QWidget * const window = q->window();

            const int sizeGripSize = 20;
            const QRect oldWidgetRect = QRect(q->mapTo(window, QPoint(0, 0)), QSize(oldRect.width(), oldRect.height()));
            const QRect newWidgetRect = QRect(q->mapTo(window, QPoint(0, 0)), QSize(newRect.width(), newRect.height()));
            const QRect sizeGripRect = QRect(window->rect().bottomRight() - QPoint(sizeGripSize, sizeGripSize),
                                             window->rect().bottomRight());

            if (sizeGripRect.intersects(oldWidgetRect) || sizeGripRect.intersects(newWidgetRect)) {
                HIViewSetFrame(view, &bounds);
                return;
            }
        }

        // Don't scroll anything outside the parent widget rect.
        const QRect scrollRect = (oldRect | newRect) & parentWidget->rect();
        const HIRect scrollBounds =
            CGRectMake(scrollRect.x(), scrollRect.y(), scrollRect.width(), scrollRect.height());

        // We cannot scroll when the widget has a mask as that would
        // scroll the masked out areas too
        if (qd->extra && qd->extra->hasMask) {
            HIViewMoveBy(view, dx, dy);
            return;
        }

        OSStatus err = HIViewScrollRect(parentView, &scrollBounds, dx, dy);
        if (err != noErr) {
            HIViewSetNeedsDisplay(view, true);
            qWarning("QWidget: Internal error (%s:%d)", __FILE__, __LINE__);
        }
    }
    // Set the view bounds with drawing disabled to prevent repaints.
    HIViewSetDrawingEnabled(view, false);
    HIViewSetFrame(view, &bounds);
    HIViewSetDrawingEnabled(view, true);

    // Update any newly exposed areas due to resizing.
    const int startx = oldRect.width();
    const int stopx = newRect.width();
    const int starty = oldRect.height();
    const int stopy = newRect.height();

    const HIRect verticalSlice = CGRectMake(startx, 0, stopx , stopy);
    HIViewSetNeedsDisplayInRect(view, &verticalSlice, true);
    const HIRect horizontalSlice = CGRectMake(0, starty, startx, stopy);
    HIViewSetNeedsDisplayInRect(view, &horizontalSlice, true);
}
#endif

/*
  Helper function for non-toplevel widgets. Helps to map Qt's 32bit
  coordinate system to OS X's 16bit coordinate system.

  Sets the geometry of the widget to data.crect, but clipped to sizes
  that OS X can handle. Unmaps widgets that are completely outside the
  valid range.

  Maintains data.wrect, which is the geometry of the OS X widget,
  measured in this widget's coordinate system.

  if the parent is not clipped, parentWRect is empty, otherwise
  parentWRect is the geometry of the parent's OS X rect, measured in
  parent's coord sys
*/
void QWidgetPrivate::setWSGeometry(bool dontShow, const QRect &oldRect)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));

    if (!q->internalWinId() && QApplicationPrivate::graphicsSystem() != 0) {
        // We have no view to move, and no paint engine that
        // we can update dirty regions on. So just return:
        return;
    }

    QMacCocoaAutoReleasePool pool;

    /*
      There are up to four different coordinate systems here:
      Qt coordinate system for this widget.
      X coordinate system for this widget (relative to wrect).
      Qt coordinate system for parent
      X coordinate system for parent (relative to parent's wrect).
    */

    // wrect is the same as crect, except that it is
    // clipped to fit inside parent (and screen):
    QRect wrect;

    // wrectInParentCoordSys will be the same as wrect, except that it is
    // originated in q's parent rather than q itself. It starts out in
    // parent's Qt coord system, and ends up in parent's coordinate system:
    QRect wrectInParentCoordSys = data.crect;

    // If q's parent has been clipped, parentWRect will
    // be filled with the parents clipped crect:
    QRect parentWRect;

    // Embedded have different meaning on each platform, and on
    // Mac, it means that q is a QMacNativeWidget.
    bool isEmbeddedWindow = (q->isWindow() && topData()->embedded);
#ifdef QT_MAC_USE_COCOA
    NSView *nsview = qt_mac_nativeview_for(q);
#endif
    if (!isEmbeddedWindow) {
        parentWRect = q->parentWidget()->data->wrect;
    } else {
        // INVARIANT: q's parent view is not owned by Qt. So we need to
        // do some extra calls to get the clipped rect of the parent view:
#ifndef QT_MAC_USE_COCOA
        HIViewRef parentView = HIViewGetSuperview(qt_mac_nativeview_for(q));
#else
        NSView *parentView = [qt_mac_nativeview_for(q) superview];
#endif
        if (parentView) {
#ifndef QT_MAC_USE_COCOA
            HIRect tmpRect;
            HIViewGetFrame(parentView, &tmpRect);
#else
            NSRect tmpRect = [parentView frame];
#endif
            parentWRect = QRect(tmpRect.origin.x, tmpRect.origin.y,
                                tmpRect.size.width, tmpRect.size.height);
        } else {
            const QRect wrectRange(-WRECT_MAX,-WRECT_MAX, 2*WRECT_MAX, 2*WRECT_MAX);
            parentWRect = wrectRange;
        }
    }

    if (parentWRect.isValid()) {
        // INVARIANT: q's parent has been clipped.
        // So we fit our own wrects inside it:
        if (!parentWRect.contains(wrectInParentCoordSys) && !isEmbeddedWindow) {
            wrectInParentCoordSys &= parentWRect;
            wrect = wrectInParentCoordSys;
            // Make sure wrect is originated in q's coordinate system:
            wrect.translate(-data.crect.topLeft());
        }
        // // Make sure wrectInParentCoordSys originated in q's parent coordinate system:
        wrectInParentCoordSys.translate(-parentWRect.topLeft());
    } else {
        // INVARIANT: we dont know yet the clipping rect of q's parent.
        // So we may or may not have to adjust our wrects:

        if (data.wrect.isValid() && QRect(QPoint(),data.crect.size()).contains(data.wrect)) {
            // This is where the main optimization is: we have an old wrect from an earlier
            // setGeometry call, and the new crect is smaller than it. If the final wrect is
            // also inside the old wrect, we can just move q and its children to the new
            // location without any clipping:

             // vrect will be the part of q that's will be visible inside
             // q's parent. If it inside the old wrect, then we can just move:
            QRect vrect = wrectInParentCoordSys & q->parentWidget()->rect();
            vrect.translate(-data.crect.topLeft());

            if (data.wrect.contains(vrect)) {
                wrectInParentCoordSys = data.wrect;
                wrectInParentCoordSys.translate(data.crect.topLeft());
#ifndef QT_MAC_USE_COCOA
                HIRect bounds = CGRectMake(wrectInParentCoordSys.x(), wrectInParentCoordSys.y(),
                                           wrectInParentCoordSys.width(), wrectInParentCoordSys.height());
                HIViewSetFrame(qt_mac_nativeview_for(q), &bounds);
#else
                if (nsview) {
                    // INVARIANT: q is native. Set view frame:
                    NSRect bounds = NSMakeRect(wrectInParentCoordSys.x(), wrectInParentCoordSys.y(),
                        wrectInParentCoordSys.width(), wrectInParentCoordSys.height());
                        [nsview setFrame:bounds];
                } else {
                    // INVARIANT: q is alien. Repaint wrect instead (includes old and new wrect):
                    QWidget *parent = q->parentWidget();
                    QPoint globalPosWRect = parent->mapToGlobal(data.wrect.topLeft());

                    QWidget *nativeParent = q->nativeParentWidget();
                    QRect dirtyWRect = QRect(nativeParent->mapFromGlobal(globalPosWRect), data.wrect.size());

                    nativeParent->update(dirtyWRect);
                }
#endif
                if (q->testAttribute(Qt::WA_OutsideWSRange)) {
                    q->setAttribute(Qt::WA_OutsideWSRange, false);
                    if (!dontShow) {
                        q->setAttribute(Qt::WA_Mapped);
#ifndef QT_MAC_USE_COCOA
                        HIViewSetVisible(qt_mac_nativeview_for(q), true);
#else
                        // If q is Alien, the following call does nothing:
                        [nsview setHidden:NO];
#endif
                    }
                }
                return;
            }
        }

#ifndef QT_MAC_USE_COCOA
        const QRect validRange(-XCOORD_MAX,-XCOORD_MAX, 2*XCOORD_MAX, 2*XCOORD_MAX);
        if (!validRange.contains(wrectInParentCoordSys)) {
            // We're too big, and must clip:
            QPoint screenOffset(0, 0); // offset of the part being on screen
            const QWidget *parentWidget = q->parentWidget();
            while (parentWidget && !parentWidget->isWindow()) {
                screenOffset -= parentWidget->data->crect.topLeft();
                parentWidget = parentWidget->parentWidget();
            }
            QRect cropRect(screenOffset.x() - WRECT_MAX,
                           screenOffset.y() - WRECT_MAX,
                           2*WRECT_MAX,
                           2*WRECT_MAX);

            wrectInParentCoordSys &=cropRect;
            wrect = wrectInParentCoordSys;
            wrect.translate(-data.crect.topLeft());
        }
#endif //QT_MAC_USE_COCOA
    }

    // unmap if we are outside the valid window system coord system
    bool outsideRange = !wrectInParentCoordSys.isValid();
    bool mapWindow = false;
    if (q->testAttribute(Qt::WA_OutsideWSRange) != outsideRange) {
        q->setAttribute(Qt::WA_OutsideWSRange, outsideRange);
        if (outsideRange) {
#ifndef QT_MAC_USE_COCOA
            HIViewSetVisible(qt_mac_nativeview_for(q), false);
#else
            // If q is Alien, the following call does nothing:
            [nsview setHidden:YES];
#endif
            q->setAttribute(Qt::WA_Mapped, false);
        } else if (!q->isHidden()) {
            mapWindow = true;
        }
    }

    if (outsideRange)
        return;

    // Store the new clipped rect:
    bool jump = (data.wrect != wrect);
    data.wrect = wrect;

    // and now recursively for all children...
    // ### can be optimized
    for (int i = 0; i < children.size(); ++i) {
        QObject *object = children.at(i);
        if (object->isWidgetType()) {
            QWidget *w = static_cast<QWidget *>(object);
            if (!w->isWindow() && w->testAttribute(Qt::WA_WState_Created))
                w->d_func()->setWSGeometry();
        }
    }

#ifndef QT_MAC_USE_COCOA
    // Move the actual HIView:
    qt_mac_update_widget_position(q, oldRect, wrectInParentCoordSys);
    if (jump)
        q->update();
#else
    if (nsview) {
        // INVARIANT: q is native. Move the actual NSView:
        NSRect bounds = NSMakeRect(
            wrectInParentCoordSys.x(), wrectInParentCoordSys.y(),
            wrectInParentCoordSys.width(), wrectInParentCoordSys.height());
        [nsview setFrame:bounds];
        if (jump)
            q->update();
    } else if (QApplicationPrivate::graphicsSystem() == 0){
        // INVARIANT: q is alien and we use native paint engine.
        // Schedule updates where q is moved from and to:
        const QWidget *parent = q->parentWidget();
        const QPoint globalPosOldWRect = parent->mapToGlobal(oldRect.topLeft());
        const QPoint globalPosNewWRect = parent->mapToGlobal(wrectInParentCoordSys.topLeft());

        QWidget *nativeParent = q->nativeParentWidget();
        const QRegion dirtyOldWRect = QRect(nativeParent->mapFromGlobal(globalPosOldWRect), oldRect.size());
        const QRegion dirtyNewWRect = QRect(nativeParent->mapFromGlobal(globalPosNewWRect), wrectInParentCoordSys.size());

        const bool sizeUnchanged = oldRect.size() == wrectInParentCoordSys.size();
        const bool posUnchanged = oldRect.topLeft() == wrectInParentCoordSys.topLeft();

        // Resolve/minimize the region that needs to update:
        if (sizeUnchanged && q->testAttribute(Qt::WA_OpaquePaintEvent)) {
            // INVARIANT: q is opaque, and is only moved (not resized). So in theory we only
            // need to blit pixels, and skip a repaint. But we can only make this work if we
            // had access to the backbuffer, so we need to update all:
            nativeParent->update(dirtyOldWRect | dirtyNewWRect);
        } else if (posUnchanged && q->testAttribute(Qt::WA_StaticContents)) {
            // We only need to redraw exposed areas:
            nativeParent->update(dirtyNewWRect - dirtyOldWRect);
        } else {
            nativeParent->update(dirtyOldWRect | dirtyNewWRect);
        }
    }
#endif

    if (mapWindow && !dontShow) {
        q->setAttribute(Qt::WA_Mapped);
#ifndef QT_MAC_USE_COCOA
        HIViewSetVisible(qt_mac_nativeview_for(q), true);
#else
        // If q is Alien, the following call does nothing:
        [nsview setHidden:NO];
#endif
    }
}

void QWidgetPrivate::adjustWithinMaxAndMinSize(int &w, int &h)
{
    if (QWExtra *extra = extraData()) {
        w = qMin(w, extra->maxw);
        h = qMin(h, extra->maxh);
        w = qMax(w, extra->minw);
        h = qMax(h, extra->minh);

        // Deal with size increment
        if (QTLWExtra *top = topData()) {
            if(top->incw) {
                w = w/top->incw;
                w *= top->incw;
            }
            if(top->inch) {
                h = h/top->inch;
                h *= top->inch;
            }
        }
    }

    if (isRealWindow()) {
        w = qMax(0, w);
        h = qMax(0, h);
    }
}

void QWidgetPrivate::applyMaxAndMinSizeOnWindow()
{
    Q_Q(QWidget);
    QMacCocoaAutoReleasePool pool;

    const float max_f(20000);
#ifndef QT_MAC_USE_COCOA
#define SF(x) ((x > max_f) ? max_f : x)
    HISize max = CGSizeMake(SF(extra->maxw), SF(extra->maxh));
    HISize min = CGSizeMake(SF(extra->minw), SF(extra->minh));
#undef SF
    SetWindowResizeLimits(qt_mac_window_for(q), &min, &max);
#else
#define SF(x) ((x > max_f) ? max_f : x)
    NSSize max = NSMakeSize(SF(extra->maxw), SF(extra->maxh));
    NSSize min = NSMakeSize(SF(extra->minw), SF(extra->minh));
#undef SF
    [qt_mac_window_for(q) setContentMinSize:min];
    [qt_mac_window_for(q) setContentMaxSize:max];
#endif
}

void QWidgetPrivate::setGeometry_sys(int x, int y, int w, int h, bool isMove)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));

    if(q->windowType() == Qt::Desktop)
        return;

    QMacCocoaAutoReleasePool pool;
    bool realWindow = isRealWindow();

    if (realWindow && !q->testAttribute(Qt::WA_DontShowOnScreen)){
        adjustWithinMaxAndMinSize(w, h);
#ifndef QT_MAC_USE_COCOA
        if (w != 0 && h != 0) {
            topData()->isSetGeometry = 1;
            topData()->isMove = isMove;
            Rect r; SetRect(&r, x, y, x + w, y + h);
            SetWindowBounds(qt_mac_window_for(q), kWindowContentRgn, &r);
            topData()->isSetGeometry = 0;
        } else {
            setGeometry_sys_helper(x, y, w, h, isMove);
        }
#else
        if (!isMove && !q->testAttribute(Qt::WA_Moved) && !q->isVisible()) {
            // INVARIANT: The location of the window has not yet been set. The default will
            // instead be to center it on the desktop, or over the parent, if any. Since we now
            // resize the window, we need to adjust the top left position to keep the window
            // centeralized. And we need to to this now (and before show) in case the positioning
            // of other windows (e.g. sub-windows) depend on this position:
            if (QWidget *p = q->parentWidget()) {
                x = p->geometry().center().x() - (w / 2);
                y = p->geometry().center().y() - (h / 2);
            } else {
                QRect availGeo = QApplication::desktop()->availableGeometry(q);
                x = availGeo.center().x() - (w / 2);
                y = availGeo.center().y() - (h / 2);
            }
        }

        QSize  olds = q->size();
        const bool isResize = (olds != QSize(w, h));
        NSWindow *window = qt_mac_window_for(q);
        const QRect &fStrut = frameStrut();
        const QRect frameRect(QPoint(x - fStrut.left(), y - fStrut.top()),
                              QSize(fStrut.left() + fStrut.right() + w,
                                    fStrut.top() + fStrut.bottom() + h));
        NSRect cocoaFrameRect = NSMakeRect(frameRect.x(), flipYCoordinate(frameRect.bottom() + 1),
                                           frameRect.width(), frameRect.height());
        // The setFrame call will trigger a 'windowDidResize' notification for the corresponding
        // NSWindow. The pending flag is set, so that the resize event can be send as non-spontaneous.
        if (isResize)
            q->setAttribute(Qt::WA_PendingResizeEvent);
        QPoint currTopLeft = data.crect.topLeft();
        if (currTopLeft.x() == x && currTopLeft.y() == y
                && cocoaFrameRect.size.width != 0
                && cocoaFrameRect.size.height != 0) {
            [window setFrame:cocoaFrameRect display:realWindow];
        } else {
            // The window is moved and resized (or resized to zero).
            // Since Cocoa usually only sends us a resize callback after
            // setting a window frame, we issue an explicit move as
            // well. To stop Cocoa from optimize away the move (since the move
            // would have the same origin as the setFrame call) we shift the
            // window back and forth inbetween.
            cocoaFrameRect.origin.y += 1;
            [window setFrame:cocoaFrameRect display:realWindow];
            cocoaFrameRect.origin.y -= 1;
            [window setFrameOrigin:cocoaFrameRect.origin];
        }
#endif
    } else {
        setGeometry_sys_helper(x, y, w, h, isMove);
    }

    topLevelAt_cache = 0;
}

void QWidgetPrivate::setGeometry_sys_helper(int x, int y, int w, int h, bool isMove)
{
    Q_Q(QWidget);
    bool realWindow = isRealWindow();

    QPoint oldp = q->pos();
    QSize  olds = q->size();
    // Apply size restrictions, applicable for Windows & Widgets.
    if (QWExtra *extra = extraData()) {
        w = qBound(extra->minw, w, extra->maxw);
        h = qBound(extra->minh, h, extra->maxh);
    }
    const bool isResize = (olds != QSize(w, h));

    if (!realWindow && !isResize && QPoint(x, y) == oldp)
        return;

    if (isResize)
        data.window_state = data.window_state & ~Qt::WindowMaximized;

    const bool visible = q->isVisible();
    data.crect = QRect(x, y, w, h);

    if (realWindow) {
        adjustWithinMaxAndMinSize(w, h);
        qt_mac_update_sizer(q);

#ifndef QT_MAC_USE_COCOA
        if (q->windowFlags() & Qt::WindowMaximizeButtonHint) {
            OSWindowRef window = qt_mac_window_for(q);
            if (extra->maxw && extra->maxh && extra->maxw == extra->minw
                    && extra->maxh == extra->minh) {
                ChangeWindowAttributes(window, kWindowNoAttributes, kWindowFullZoomAttribute);
            } else {
                ChangeWindowAttributes(window, kWindowFullZoomAttribute, kWindowNoAttributes);
            }
        }
        HIRect bounds = CGRectMake(0, 0, w, h);
        HIViewSetFrame(qt_mac_nativeview_for(q), &bounds);
#else
        [qt_mac_nativeview_for(q) setFrame:NSMakeRect(0, 0, w, h)];
#endif
    } else {
        const QRect oldRect(oldp, olds);
        if (!isResize && QApplicationPrivate::graphicsSystem())
            moveRect(oldRect, x - oldp.x(), y - oldp.y());

        setWSGeometry(false, oldRect);

        if (isResize && QApplicationPrivate::graphicsSystem())
            invalidateBuffer_resizeHelper(oldp, olds);
    }

    if(isMove || isResize) {
        if(!visible) {
            if(isMove && q->pos() != oldp)
                q->setAttribute(Qt::WA_PendingMoveEvent, true);
            if(isResize)
                q->setAttribute(Qt::WA_PendingResizeEvent, true);
        } else {
            if(isResize) { //send the resize event..
                QResizeEvent e(q->size(), olds);
                QApplication::sendEvent(q, &e);
            }
            if(isMove && q->pos() != oldp) { //send the move event..
                QMoveEvent e(q->pos(), oldp);
                QApplication::sendEvent(q, &e);
            }
        }
    }
    qt_event_request_window_change(q);
}

void QWidgetPrivate::setConstraints_sys()
{
    updateMaximizeButton_sys();
    applyMaxAndMinSizeOnWindow();
}

void QWidgetPrivate::updateMaximizeButton_sys()
{
    Q_Q(QWidget);
    if (q->data->window_flags & Qt::CustomizeWindowHint)
        return;

    OSWindowRef window = qt_mac_window_for(q);
    QTLWExtra * tlwExtra = topData();
#ifdef QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
    NSButton *maximizeButton = [window standardWindowButton:NSWindowZoomButton];
#endif
    if (extra->maxw && extra->maxh
        && extra->maxw == extra->minw
        && extra->maxh == extra->minh) {
        // The window has a fixed size, so gray out the maximize button:
        if (!tlwExtra->savedWindowAttributesFromMaximized) {
#ifndef QT_MAC_USE_COCOA
            GetWindowAttributes(window,
                                (WindowAttributes*)&extra->topextra->savedWindowAttributesFromMaximized);

#else
            tlwExtra->savedWindowAttributesFromMaximized = (![maximizeButton isHidden] && [maximizeButton isEnabled]);
#endif
        }
#ifndef QT_MAC_USE_COCOA
        ChangeWindowAttributes(window, kWindowNoAttributes, kWindowFullZoomAttribute);
#else
       [maximizeButton setEnabled:NO];
#endif


    } else {
        if (tlwExtra->savedWindowAttributesFromMaximized) {
#ifndef QT_MAC_USE_COCOA
            ChangeWindowAttributes(window,
                                   extra->topextra->savedWindowAttributesFromMaximized,
                                   kWindowNoAttributes);
#else
            [maximizeButton setEnabled:YES];
#endif
            tlwExtra->savedWindowAttributesFromMaximized = 0;
        }
    }


}

void QWidgetPrivate::scroll_sys(int dx, int dy)
{
    if (QApplicationPrivate::graphicsSystem() && !paintOnScreen()) {
        // INVARIANT: Alien paint engine
        scrollChildren(dx, dy);
        scrollRect(q_func()->rect(), dx, dy);
    } else {
        scroll_sys(dx, dy, QRect());
    }
}

void QWidgetPrivate::scroll_sys(int dx, int dy, const QRect &qscrollRect)
{
    if (QMacScrollOptimization::delayScroll(this, dx, dy, qscrollRect))
        return;

    Q_Q(QWidget);
    if (QApplicationPrivate::graphicsSystem() && !paintOnScreen()) {
        // INVARIANT: Alien paint engine
        scrollRect(qscrollRect, dx, dy);
        return;
    }

    static int accelEnv = -1;
    if (accelEnv == -1) {
        accelEnv = qgetenv("QT_NO_FAST_SCROLL").toInt() == 0;
    }

    // Scroll the whole widget if qscrollRect is not valid:
    QRect validScrollRect = qscrollRect.isValid() ? qscrollRect : q->rect();
    validScrollRect &= clipRect();

    // If q is overlapped by other widgets, we cannot just blit pixels since
    // this will move overlapping widgets as well. In case we just update:
    const bool overlapped = isOverlapped(validScrollRect.translated(data.crect.topLeft()));
    const bool accelerateScroll = accelEnv && isOpaque && !overlapped;
    const bool isAlien = (q->internalWinId() == 0);
    const QPoint scrollDelta(dx, dy);

    // If qscrollRect is valid, we are _not_ supposed to scroll q's children (as documented).
    // But we do scroll children (and the whole of q) if qscrollRect is invalid. This case is
    // documented as undefined, but we exploit it to help factor our code into one function.
    const bool scrollChildren = !qscrollRect.isValid();

    if (!q->updatesEnabled()) {
        // We are told not to update anything on q at this point. So unless
        // we are supposed to scroll children, we bail out early:
        if (!scrollChildren || q->children().isEmpty())
            return;
    }

    if (!accelerateScroll) {
        if (overlapped) {
            QRegion region(validScrollRect);
            subtractOpaqueSiblings(region);
            update_sys(region);
        }else {
            update_sys(qscrollRect);
        }
        return;
    }

#ifdef QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
#else
    Q_UNUSED(isAlien);
    // We're not sure what the following call is supposed to achive
    // but until we see what it breaks, we don't bring it into the
    // Cocoa port:
    qt_event_request_window_change(q);
#endif

    // First move all native children. Alien children will indirectly be
    // moved when the parent is scrolled. All directly or indirectly moved
    // children will receive a move event before the function call returns.
    QWidgetList movedChildren;
    if (scrollChildren) {
        QObjectList children = q->children();

        for (int i=0; i<children.size(); i++) {
            QObject *obj = children.at(i);
            if (QWidget *w = qobject_cast<QWidget*>(obj)) {
                if (!w->isWindow()) {
                    w->data->crect = QRect(w->pos() + scrollDelta, w->size());
#ifndef QT_MAC_USE_COCOA
                    if (w->testAttribute(Qt::WA_WState_Created)) {
                        HIRect bounds = CGRectMake(w->data->crect.x(), w->data->crect.y(),
                                                   w->data->crect.width(), w->data->crect.height());
                        HIViewRef hiview = qt_mac_nativeview_for(w);
                        const bool opaque = q->testAttribute(Qt::WA_OpaquePaintEvent);

                        if (opaque)
                            HIViewSetDrawingEnabled(hiview,  false);
                        HIViewSetFrame(hiview, &bounds);
                        if (opaque)
                            HIViewSetDrawingEnabled(hiview,  true);
                    }
#else
                    if (NSView *view = qt_mac_nativeview_for(w)) {
                        // INVARIANT: w is not alien
                        [view setFrame:NSMakeRect(
                            w->data->crect.x(), w->data->crect.y(),
                            w->data->crect.width(), w->data->crect.height())];
                    }
#endif
                    movedChildren.append(w);
                }
            }
        }
    }

    if (q->testAttribute(Qt::WA_WState_Created) && q->isVisible()) {
        // Scroll q itself according to the qscrollRect, and
        // call update on any exposed areas so that they get redrawn:

#ifndef QT_MAC_USE_COCOA
        OSViewRef view = qt_mac_nativeview_for(q);
        HIRect scrollrect = CGRectMake(qscrollRect.x(), qscrollRect.y(), qscrollRect.width(), qscrollRect.height());
        OSStatus err = _HIViewScrollRectWithOptions(view, qscrollRect.isValid() ? &scrollrect : 0, dx, dy, kHIViewScrollRectAdjustInvalid);
        if (err) {
           // The only parameter that can go wrong, is the rect.
           qWarning("QWidget::scroll: Your rectangle was too big for the widget, clipping rect");
           scrollrect = CGRectMake(qMax(qscrollRect.x(), 0), qMax(qscrollRect.y(), 0),
                                   qMin(qscrollRect.width(), q->width()), qMin(qscrollRect.height(), q->height()));
           _HIViewScrollRectWithOptions(view, qscrollRect.isValid() ? &scrollrect : 0, dx, dy, kHIViewScrollRectAdjustInvalid);
        }
#else

        QWidget *nativeWidget = isAlien ? q->nativeParentWidget() : q;
        if (!nativeWidget)
            return;
        OSViewRef view = qt_mac_nativeview_for(nativeWidget);
        if (!view)
            return;

        // Calculate the rectangles that needs to be redrawn
        // after the scroll. This will be source rect minus destination rect:
        QRect deltaXRect;
        if (dx != 0) {
            deltaXRect.setY(validScrollRect.y());
            deltaXRect.setHeight(validScrollRect.height());
            if (dx > 0) {
                deltaXRect.setX(validScrollRect.x());
                deltaXRect.setWidth(dx);
            } else {
                deltaXRect.setX(validScrollRect.x() + validScrollRect.width() + dx);
                deltaXRect.setWidth(-dx);
            }
        }

        QRect deltaYRect;
        if (dy != 0) {
            deltaYRect.setX(validScrollRect.x());
            deltaYRect.setWidth(validScrollRect.width());
            if (dy > 0) {
                deltaYRect.setY(validScrollRect.y());
                deltaYRect.setHeight(dy);
           } else {
                deltaYRect.setY(validScrollRect.y() + validScrollRect.height() + dy);
                deltaYRect.setHeight(-dy);
            }
        }

        if (isAlien) {
            // Adjust the scroll rect to the location as seen from the native parent:
            QPoint scrollTopLeftInsideNative = nativeWidget->mapFromGlobal(q->mapToGlobal(validScrollRect.topLeft()));
            validScrollRect.moveTo(scrollTopLeftInsideNative);
        }

        // Make the pixel copy rect within the validScrollRect bounds:
        NSRect nsscrollRect = NSMakeRect(
            validScrollRect.x() + (dx < 0 ? -dx : 0),
            validScrollRect.y() + (dy < 0 ? -dy : 0),
            validScrollRect.width() + (dx > 0 ? -dx : 0),
            validScrollRect.height() + (dy > 0 ? -dy : 0));

        NSSize deltaSize = NSMakeSize(dx, dy);
        [view scrollRect:nsscrollRect by:deltaSize];

        // Some areas inside the scroll rect might have been marked as dirty from before, which
        // means that they are scheduled to be redrawn. But as we now scroll, those dirty rects
        // should also move along to ensure that q receives repaints on the correct places.
        // Since some of the dirty rects might lay outside, or only intersect with, the scroll
        // rect, the old calls to setNeedsDisplay still makes sense.
        // NB: Using [view translateRectsNeedingDisplayInRect:nsscrollRect by:deltaSize] have
        // so far not been proven fruitful to solve this problem.
        const QVector<QRect> &dirtyRectsToScroll = dirtyOnWidget.rects();
        for (int i=0; i<dirtyRectsToScroll.size(); ++i) {
            QRect qdirtyRect = dirtyRectsToScroll[i];
            qdirtyRect.translate(dx, dy);
            update_sys(qdirtyRect);
        }

        // Update newly exposed areas. This will generate new dirty areas on
        // q, and therefore, we do it after updating the old dirty rects above:
        if (dx != 0)
            update_sys(deltaXRect);
        if (dy != 0)
            update_sys(deltaYRect);

#endif // QT_MAC_USE_COCOA
    }

    for (int i=0; i<movedChildren.size(); i++) {
        QWidget *w = movedChildren.at(i);
        QMoveEvent e(w->pos(), w->pos() - scrollDelta);
        QApplication::sendEvent(w, &e);
    }
}

int QWidget::metric(PaintDeviceMetric m) const
{
    switch(m) {
    case PdmHeightMM:
        return qRound(metric(PdmHeight) * 25.4 / qreal(metric(PdmDpiY)));
    case PdmWidthMM:
        return qRound(metric(PdmWidth) * 25.4 / qreal(metric(PdmDpiX)));
    case PdmHeight:
    case PdmWidth:
#ifndef QT_MAC_USE_COCOA
        { HIRect rect;
        HIViewGetFrame(qt_mac_nativeview_for(this), &rect);
        if(m == PdmWidth)
            return (int)rect.size.width;
        return (int)rect.size.height; }
#else
        if (m == PdmWidth)
            return data->crect.width();
        else
            return data->crect.height();
#endif
    case PdmDepth:
        return 32;
    case PdmNumColors:
        return INT_MAX;
    case PdmDpiX:
    case PdmPhysicalDpiX: {
        Q_D(const QWidget);
        if (d->extra && d->extra->customDpiX)
            return d->extra->customDpiX;
        else if (d->parent)
            return static_cast<QWidget *>(d->parent)->metric(m);
        extern float qt_mac_defaultDpi_x(); //qpaintdevice_mac.cpp
        return int(qt_mac_defaultDpi_x()); }
    case PdmDpiY:
    case PdmPhysicalDpiY: {
        Q_D(const QWidget);
        if (d->extra && d->extra->customDpiY)
            return d->extra->customDpiY;
        else if (d->parent)
            return static_cast<QWidget *>(d->parent)->metric(m);
        extern float qt_mac_defaultDpi_y(); //qpaintdevice_mac.cpp
        return int(qt_mac_defaultDpi_y()); }
    default: //leave this so the compiler complains when new ones are added
        qWarning("QWidget::metric: Unhandled parameter %d", m);
        return QPaintDevice::metric(m);
    }
    return 0;
}

void QWidgetPrivate::createSysExtra()
{
#ifdef QT_MAC_USE_COCOA
    extra->imageMask = 0;
#endif
}

void QWidgetPrivate::deleteSysExtra()
{
#ifdef QT_MAC_USE_COCOA
    if (extra->imageMask)
        CFRelease(extra->imageMask);
#endif
}

void QWidgetPrivate::createTLSysExtra()
{
    extra->topextra->resizer = 0;
    extra->topextra->isSetGeometry = 0;
    extra->topextra->isMove = 0;
    extra->topextra->wattr = 0;
    extra->topextra->wclass = 0;
    extra->topextra->group = 0;
    extra->topextra->windowIcon = 0;
    extra->topextra->savedWindowAttributesFromMaximized = 0;
}

void QWidgetPrivate::deleteTLSysExtra()
{
#ifndef QT_MAC_USE_COCOA
    if (extra->topextra->group) {
        qt_mac_release_window_group(extra->topextra->group);
        extra->topextra->group = 0;
    }
    if (extra->topextra->windowIcon) {
        ReleaseIconRef(extra->topextra->windowIcon);
        extra->topextra->windowIcon = 0;
    }
#endif
}

void QWidgetPrivate::updateFrameStrut()
{
    Q_Q(QWidget);

    QWidgetPrivate *that = const_cast<QWidgetPrivate*>(this);

    that->data.fstrut_dirty = false;
    QTLWExtra *top = that->topData();

#if QT_MAC_USE_COCOA
    // 1 Get the window frame
    OSWindowRef oswnd = qt_mac_window_for(q);
    NSRect frameW = [oswnd frame];
    // 2 Get the content frame - so now
    NSRect frameC = [oswnd contentRectForFrameRect:frameW];
    top->frameStrut.setCoords(frameC.origin.x - frameW.origin.x,
                              (frameW.origin.y + frameW.size.height) - (frameC.origin.y + frameC.size.height),
                              (frameW.origin.x + frameW.size.width) - (frameC.origin.x + frameC.size.width),
                              frameC.origin.y - frameW.origin.y);
#else
    Rect window_r;
    GetWindowStructureWidths(qt_mac_window_for(q), &window_r);
    top->frameStrut.setCoords(window_r.left, window_r.top, window_r.right, window_r.bottom);
#endif
}

void QWidgetPrivate::registerDropSite(bool on)
{
    Q_Q(QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created))
        return;
#ifndef QT_MAC_USE_COCOA
    SetControlDragTrackingEnabled(qt_mac_nativeview_for(q), on);
#else
    NSWindow *win = qt_mac_window_for(q);
    if (on) {
        if ([win isKindOfClass:[QT_MANGLE_NAMESPACE(QCocoaWindow) class]])
            [static_cast<QT_MANGLE_NAMESPACE(QCocoaWindow) *>(win) registerDragTypes];
        else if ([win isKindOfClass:[QT_MANGLE_NAMESPACE(QCocoaPanel) class]])
            [static_cast<QT_MANGLE_NAMESPACE(QCocoaPanel) *>(win) registerDragTypes];
    }
#endif
}

void QWidgetPrivate::registerTouchWindow(bool enable)
{
    Q_UNUSED(enable);
#ifdef QT_MAC_USE_COCOA
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
    if (QSysInfo::MacintoshVersion < QSysInfo::MV_10_6)
        return;

    Q_Q(QWidget);
    if (enable == touchEventsEnabled)
        return;

    QT_MANGLE_NAMESPACE(QCocoaView) *view = static_cast<QT_MANGLE_NAMESPACE(QCocoaView) *>(qt_mac_effectiveview_for(q));
    if (!view)
        return;

    if (enable) {
        ++view->alienTouchCount;
        if (view->alienTouchCount == 1) {
            touchEventsEnabled = true;
            [view setAcceptsTouchEvents:YES];
        }
    } else {
        --view->alienTouchCount;
        if (view->alienTouchCount == 0) {
            touchEventsEnabled = false;
            [view setAcceptsTouchEvents:NO];
        }
    }
#endif
#endif
}

void QWidgetPrivate::setMask_sys(const QRegion &region)
{
    Q_UNUSED(region);
    Q_Q(QWidget);

#ifndef QT_MAC_USE_COCOA
    if (q->isWindow())
        ReshapeCustomWindow(qt_mac_window_for(q));
    else
        HIViewReshapeStructure(qt_mac_nativeview_for(q));
#else
    if (!q->internalWinId())
        return;

    if (extra->mask.isEmpty()) {
        extra->maskBits = QImage();
        finishCocoaMaskSetup();
    } else {
        syncCocoaMask();
    }

    topLevelAt_cache = 0;
#endif
}

void QWidgetPrivate::setWindowOpacity_sys(qreal level)
{
    Q_Q(QWidget);

    if (!q->isWindow())
        return;

    level = qBound(0.0, level, 1.0);
    topData()->opacity = (uchar)(level * 255);
    if (!q->testAttribute(Qt::WA_WState_Created))
        return;

    OSWindowRef oswindow = qt_mac_window_for(q);
#if QT_MAC_USE_COCOA
    [oswindow setAlphaValue:level];
#else
    SetWindowAlpha(oswindow, level);
#endif
}

#ifdef QT_MAC_USE_COCOA
void QWidgetPrivate::syncCocoaMask()
{
    Q_Q(QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created) || !extra)
        return;

    if (extra->hasMask) {
        if(extra->maskBits.size() != q->size()) {
            extra->maskBits = QImage(q->size(), QImage::Format_Mono);
        }
        extra->maskBits.fill(QColor(Qt::color1).rgba());
        extra->maskBits.setNumColors(2);
        extra->maskBits.setColor(0, QColor(Qt::color0).rgba());
        extra->maskBits.setColor(1, QColor(Qt::color1).rgba());
        QPainter painter(&extra->maskBits);
        painter.setBrush(Qt::color1);
        painter.setPen(Qt::NoPen);
        painter.drawRects(extra->mask.rects());
        painter.end();
        finishCocoaMaskSetup();
    }
}

void QWidgetPrivate::finishCocoaMaskSetup()
{
    Q_Q(QWidget);

    if (!q->testAttribute(Qt::WA_WState_Created) || !extra)
        return;

    // Technically this is too late to release, because the data behind the image
    // has already been released. But it's more tidy to do it here.
    // If you are seeing a crash, consider doing a CFRelease before changing extra->maskBits.
    if (extra->imageMask) {
        CFRelease(extra->imageMask);
        extra->imageMask = 0;
    }

    if (!extra->maskBits.isNull()) {
        QCFType<CGDataProviderRef> dataProvider = CGDataProviderCreateWithData(0,
                                                                       extra->maskBits.bits(),
                                                                       extra->maskBits.numBytes(),
                                                                       0); // shouldn't need to release.
        CGFloat decode[2] = {1, 0};
        extra->imageMask = CGImageMaskCreate(extra->maskBits.width(), extra->maskBits.height(),
                                             1, 1, extra->maskBits.bytesPerLine(), dataProvider,
                                             decode, false);
    }
    if (q->isWindow()) {
        NSWindow *window = qt_mac_window_for(q);
        [window setOpaque:(extra->imageMask == 0)];
        [window invalidateShadow];
    }
    macSetNeedsDisplay(QRegion());
}
#endif

struct QPaintEngineCleanupHandler
{
    inline QPaintEngineCleanupHandler() : engine(0) {}
    inline ~QPaintEngineCleanupHandler() { delete engine; }
    QPaintEngine *engine;
};

Q_GLOBAL_STATIC(QPaintEngineCleanupHandler, engineHandler)

QPaintEngine *QWidget::paintEngine() const
{
    QPaintEngine *&pe = engineHandler()->engine;
    if (!pe)
        pe = new QCoreGraphicsPaintEngine();
    if (pe->isActive()) {
        QPaintEngine *engine = new QCoreGraphicsPaintEngine();
        engine->setAutoDestruct(true);
        return engine;
    }
    return pe;
}

void QWidgetPrivate::setModal_sys()
{
    Q_Q(QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created) || !q->isWindow())
        return;
    const QWidget * const windowParent = q->window()->parentWidget();
    const QWidget * const primaryWindow = windowParent ? windowParent->window() : 0;
    OSWindowRef windowRef = qt_mac_window_for(q);

#ifdef QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
    bool alreadySheet = [windowRef styleMask] & NSDocModalWindowMask;

    if (windowParent && q->windowModality() == Qt::WindowModal){
        // INVARIANT: Window should be window-modal (which implies a sheet).
        if (!alreadySheet) {
            // NB: the following call will call setModal_sys recursivly:
            recreateMacWindow();
            windowRef = qt_mac_window_for(q);
        }
        if ([windowRef isKindOfClass:[NSPanel class]]){
            // If the primary window of the sheet parent is a child of a modal dialog,
            // the sheet parent should not be modally shaddowed.
            // This goes for the sheet as well:
            OSWindowRef ref = primaryWindow ? qt_mac_window_for(primaryWindow) : 0;
            bool isDialog = ref ? [ref isKindOfClass:[NSPanel class]] : false;
            bool worksWhenModal = isDialog ? [static_cast<NSPanel *>(ref) worksWhenModal] : false;
            if (worksWhenModal)
                [static_cast<NSPanel *>(windowRef) setWorksWhenModal:YES];
        }
    } else {
        // INVARIANT: Window shold _not_ be window-modal (and as such, not a sheet).
        if (alreadySheet){
            // NB: the following call will call setModal_sys recursivly:
            recreateMacWindow();
            windowRef = qt_mac_window_for(q);
        }
        if (q->windowModality() == Qt::NonModal
            && primaryWindow && primaryWindow->windowModality() == Qt::ApplicationModal) {
            // INVARIANT: Our window has a parent that is application modal.
            // This means that q is supposed to be on top of this window and
            // not be modally shaddowed:
            if ([windowRef isKindOfClass:[NSPanel class]])
                [static_cast<NSPanel *>(windowRef) setWorksWhenModal:YES];
        }
    }

#else
    const bool primaryWindowModal = primaryWindow ? primaryWindow->testAttribute(Qt::WA_ShowModal) : false;
    const bool modal = q->testAttribute(Qt::WA_ShowModal);

    WindowClass old_wclass;
    GetWindowClass(windowRef, &old_wclass);

    if (modal || primaryWindowModal) {
        if (q->windowModality() == Qt::WindowModal
                || (primaryWindow && primaryWindow->windowModality() == Qt::WindowModal)){
            // Window should be window-modal (which implies a sheet).
            if (old_wclass != kSheetWindowClass){
                // We cannot convert a created window to a sheet.
                // So we recreate the window:
                recreateMacWindow();
                return;
            }
        } else {
            // Window should be application-modal (which implies NOT using a sheet).
            if (old_wclass == kSheetWindowClass){
                // We cannot convert a sheet to a window.
                // So we recreate the window:
                recreateMacWindow();
                return;
            } else if (!(q->data->window_flags & Qt::CustomizeWindowHint)) {
                if (old_wclass == kDocumentWindowClass || old_wclass == kFloatingWindowClass || old_wclass == kUtilityWindowClass){
                    // Only change the class to kMovableModalWindowClass if the no explicit jewels
                    // are set (kMovableModalWindowClass can't contain them), and the current window class
                    // can be converted to modal (according to carbon doc). Mind the order of
                    // HIWindowChangeClass and ChangeWindowAttributes.
                    WindowGroupRef group = GetWindowGroup(windowRef);
                    HIWindowChangeClass(windowRef, kMovableModalWindowClass);
                    quint32 tmpWattr = kWindowCloseBoxAttribute | kWindowHorizontalZoomAttribute;
                    ChangeWindowAttributes(windowRef, tmpWattr, kWindowNoAttributes);
                    ChangeWindowAttributes(windowRef, kWindowNoAttributes, tmpWattr);
                    // If the window belongs to a qt-created group, set that group once more:
                    if (data.window_flags & Qt::WindowStaysOnTopHint
                            || q->windowType() == Qt::Popup
                            || q->windowType() == Qt::ToolTip)
                        SetWindowGroup(windowRef, group);
                }
                // Popups are usually handled "special" and are never modal.
                Qt::WindowType winType = q->windowType();
                if (winType != Qt::Popup && winType != Qt::ToolTip)
                    SetWindowModality(windowRef, kWindowModalityAppModal, 0);
            }
        }
    } else if (windowRef) {
        if (old_wclass == kSheetWindowClass){
            // Converting a sheet to a window is complex. It's easier to recreate:
            recreateMacWindow();
            return;
        }

        SetWindowModality(windowRef, kWindowModalityNone, 0);
	if (!(q->data->window_flags & Qt::CustomizeWindowHint)) {
	    if (q->window()->d_func()->topData()->wattr |= kWindowCloseBoxAttribute)
		ChangeWindowAttributes(windowRef, kWindowCloseBoxAttribute, kWindowNoAttributes);
	    if (q->window()->d_func()->topData()->wattr |= kWindowHorizontalZoomAttribute)
		ChangeWindowAttributes(windowRef, kWindowHorizontalZoomAttribute, kWindowNoAttributes);
	    if (q->window()->d_func()->topData()->wattr |= kWindowCollapseBoxAttribute)
                ChangeWindowAttributes(windowRef, kWindowCollapseBoxAttribute, kWindowNoAttributes);
	}

        WindowClass newClass = q->window()->d_func()->topData()->wclass;
        if (old_wclass != newClass && newClass != 0){
            WindowGroupRef group = GetWindowGroup(windowRef);
            HIWindowChangeClass(windowRef, newClass);
            // If the window belongs to a qt-created group, set that group once more:
            if (data.window_flags & Qt::WindowStaysOnTopHint
                || q->windowType() == Qt::Popup
                || q->windowType() == Qt::ToolTip)
                SetWindowGroup(windowRef, group);
        }
    }

    // Make sure that HIWindowChangeClass didn't remove drag support
    // or reset the opaque size grip setting:
    SetAutomaticControlDragTrackingEnabledForWindow(windowRef, true);
    macUpdateOpaqueSizeGrip();
#endif
}

void QWidgetPrivate::macUpdateHideOnSuspend()
{
    Q_Q(QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created) || !q->isWindow() || q->windowType() != Qt::Tool)
        return;
#ifndef QT_MAC_USE_COCOA
    if(q->testAttribute(Qt::WA_MacAlwaysShowToolWindow))
        ChangeWindowAttributes(qt_mac_window_for(q), 0, kWindowHideOnSuspendAttribute);
    else
        ChangeWindowAttributes(qt_mac_window_for(q), kWindowHideOnSuspendAttribute, 0);
#else
    if(q->testAttribute(Qt::WA_MacAlwaysShowToolWindow))
        [qt_mac_window_for(q) setHidesOnDeactivate:NO];
    else
        [qt_mac_window_for(q) setHidesOnDeactivate:YES];
#endif
}

void QWidgetPrivate::macUpdateOpaqueSizeGrip()
{
    Q_Q(QWidget);

    if (!q->testAttribute(Qt::WA_WState_Created) || !q->isWindow())
        return;

#ifndef QT_MAC_USE_COCOA	// Growbox is always transparent on Cocoa. Can emulate with setting a QSizeGrip
    HIViewRef growBox;
    HIViewFindByID(HIViewGetRoot(qt_mac_window_for(q)), kHIViewWindowGrowBoxID, &growBox);
    if (!growBox)
        return;
    HIGrowBoxViewSetTransparent(growBox, !q->testAttribute(Qt::WA_MacOpaqueSizeGrip));
#endif
}

void QWidgetPrivate::macUpdateSizeAttribute()
{
    Q_Q(QWidget);
    QEvent event(QEvent::MacSizeChange);
    QApplication::sendEvent(q, &event);
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(children.at(i));
        if (w && (!w->isWindow() || w->testAttribute(Qt::WA_WindowPropagation))
              && !q->testAttribute(Qt::WA_MacMiniSize) // no attribute set? inherit from parent
              && !w->testAttribute(Qt::WA_MacSmallSize)
              && !w->testAttribute(Qt::WA_MacNormalSize))
            w->d_func()->macUpdateSizeAttribute();
    }
    resolveFont();
}

void QWidgetPrivate::macUpdateIgnoreMouseEvents()
{
#ifndef QT_MAC_USE_COCOA  // This is handled inside the mouse handler on Cocoa.
    Q_Q(QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created))
        return;

    if(q->isWindow())
	{
        if(q->testAttribute(Qt::WA_TransparentForMouseEvents))
            ChangeWindowAttributes(qt_mac_window_for(q), kWindowIgnoreClicksAttribute, 0);
        else
            ChangeWindowAttributes(qt_mac_window_for(q), 0, kWindowIgnoreClicksAttribute);
        ReshapeCustomWindow(qt_mac_window_for(q));
    } else {
#ifndef kHIViewFeatureIgnoresClicks
#define kHIViewFeatureIgnoresClicks kHIViewIgnoresClicks
#endif
        if(q->testAttribute(Qt::WA_TransparentForMouseEvents))
            HIViewChangeFeatures(qt_mac_nativeview_for(q), kHIViewFeatureIgnoresClicks, 0);
        else
            HIViewChangeFeatures(qt_mac_nativeview_for(q), 0, kHIViewFeatureIgnoresClicks);
        HIViewReshapeStructure(qt_mac_nativeview_for(q));
    }
#endif
}

void QWidgetPrivate::macUpdateMetalAttribute()
{
    Q_Q(QWidget);
    bool realWindow = isRealWindow();
    if (!q->testAttribute(Qt::WA_WState_Created) || !realWindow)
        return;

    if (realWindow) {
#if QT_MAC_USE_COCOA
        // Cocoa doesn't let us change the style mask once it's been changed
        // So, that means we need to recreate the window.
        OSWindowRef cocoaWindow = qt_mac_window_for(q);
        if ([cocoaWindow styleMask] & NSTexturedBackgroundWindowMask)
            return;
        recreateMacWindow();
#else
        QMainWindowLayout *layout = qt_mainwindow_layout(qobject_cast<QMainWindow *>(q));
        if (q->testAttribute(Qt::WA_MacBrushedMetal)) {
            if (layout)
                layout->updateHIToolBarStatus();
            ChangeWindowAttributes(qt_mac_window_for(q), kWindowMetalAttribute, 0);
            ChangeWindowAttributes(qt_mac_window_for(q), kWindowMetalNoContentSeparatorAttribute, 0);
        } else {
            ChangeWindowAttributes(qt_mac_window_for(q), 0, kWindowMetalNoContentSeparatorAttribute);
            ChangeWindowAttributes(qt_mac_window_for(q), 0, kWindowMetalAttribute);
            if (layout)
                layout->updateHIToolBarStatus();
        }
#endif
    }
}

void QWidgetPrivate::setEnabled_helper_sys(bool enable)
{
#ifdef QT_MAC_USE_COCOA
    Q_Q(QWidget);
    NSView *view = qt_mac_nativeview_for(q);
    if ([view isKindOfClass:[NSControl class]])
        [static_cast<NSControl *>(view) setEnabled:enable];
#else
    Q_UNUSED(enable);
#endif
}

QT_END_NAMESPACE
