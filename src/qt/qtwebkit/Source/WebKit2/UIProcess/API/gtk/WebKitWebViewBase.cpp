/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebKitWebViewBase.h"

#include "DrawingAreaProxyImpl.h"
#include "NativeWebMouseEvent.h"
#include "NativeWebWheelEvent.h"
#include "PageClientImpl.h"
#include "WebContext.h"
#include "WebEventFactory.h"
#include "WebFullScreenClientGtk.h"
#include "WebInspectorProxy.h"
#include "WebKitAuthenticationDialog.h"
#include "WebKitPrivate.h"
#include "WebKitWebViewBaseAccessible.h"
#include "WebKitWebViewBasePrivate.h"
#include "WebPageProxy.h"
#include "WebViewBaseInputMethodFilter.h"
#include <WebCore/ClipboardUtilitiesGtk.h>
#include <WebCore/DataObjectGtk.h>
#include <WebCore/DragData.h>
#include <WebCore/DragIcon.h>
#include <WebCore/GOwnPtrGtk.h>
#include <WebCore/GtkClickCounter.h>
#include <WebCore/GtkDragAndDropHelper.h>
#include <WebCore/GtkUtilities.h>
#include <WebCore/GtkVersioning.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/PasteboardHelper.h>
#include <WebCore/RefPtrCairo.h>
#include <WebCore/Region.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <wtf/HashMap.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

#if ENABLE(FULLSCREEN_API)
#include "WebFullScreenManagerProxy.h"
#endif

#if USE(TEXTURE_MAPPER_GL) && defined(GDK_WINDOWING_X11)
#include <WebCore/RedirectedXCompositeWindow.h>
#endif

using namespace WebKit;
using namespace WebCore;

typedef HashMap<GtkWidget*, IntRect> WebKitWebViewChildrenMap;

#if USE(TEXTURE_MAPPER_GL)
void redirectedWindowDamagedCallback(void* data);
#endif

struct _WebKitWebViewBasePrivate {
    _WebKitWebViewBasePrivate()
#if USE(TEXTURE_MAPPER_GL)
        : redirectedWindow(RedirectedXCompositeWindow::create(IntSize(1, 1), RedirectedXCompositeWindow::DoNotCreateGLContext))
#endif
    {
    }

    ~_WebKitWebViewBasePrivate()
    {
        pageProxy->close();
    }

    WebKitWebViewChildrenMap children;
    OwnPtr<PageClientImpl> pageClient;
    RefPtr<WebPageProxy> pageProxy;
    bool shouldForwardNextKeyEvent;
    GtkClickCounter clickCounter;
    CString tooltipText;
    IntRect tooltipArea;
    GtkDragAndDropHelper dragAndDropHelper;
    DragIcon dragIcon;
    IntSize resizerSize;
    GRefPtr<AtkObject> accessible;
    bool needsResizeOnMap;
    GtkWidget* authenticationDialog;
    GtkWidget* inspectorView;
    unsigned inspectorViewHeight;
    GOwnPtr<GdkEvent> contextMenuEvent;
    WebContextMenuProxyGtk* activeContextMenuProxy;
    WebViewBaseInputMethodFilter inputMethodFilter;

    GtkWindow* toplevelOnScreenWindow;
    unsigned long toplevelResizeGripVisibilityID;
    unsigned long toplevelFocusInEventID;
    unsigned long toplevelFocusOutEventID;

    // View State.
    bool isInWindowActive : 1;
    bool isFocused : 1;
    bool isVisible : 1;

    WebKitWebViewBaseDownloadRequestHandler downloadHandler;

#if ENABLE(FULLSCREEN_API)
    bool fullScreenModeActive;
    WebFullScreenClientGtk fullScreenClient;
#endif

#if USE(TEXTURE_MAPPER_GL)
    OwnPtr<RedirectedXCompositeWindow> redirectedWindow;
#endif
};

WEBKIT_DEFINE_TYPE(WebKitWebViewBase, webkit_web_view_base, GTK_TYPE_CONTAINER)

static void webkitWebViewBaseNotifyResizerSize(WebKitWebViewBase* webViewBase)
{
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    if (!priv->toplevelOnScreenWindow)
        return;

    gboolean resizerVisible;
    g_object_get(G_OBJECT(priv->toplevelOnScreenWindow), "resize-grip-visible", &resizerVisible, NULL);

    IntSize resizerSize;
    if (resizerVisible) {
        GdkRectangle resizerRect;
        gtk_window_get_resize_grip_area(priv->toplevelOnScreenWindow, &resizerRect);
        GdkRectangle allocation;
        gtk_widget_get_allocation(GTK_WIDGET(webViewBase), &allocation);
        if (gdk_rectangle_intersect(&resizerRect, &allocation, 0))
            resizerSize = IntSize(resizerRect.width, resizerRect.height);
    }

    if (resizerSize != priv->resizerSize) {
        priv->resizerSize = resizerSize;
        priv->pageProxy->setWindowResizerSize(resizerSize);
    }
}

static void toplevelWindowResizeGripVisibilityChanged(GObject*, GParamSpec*, WebKitWebViewBase* webViewBase)
{
    webkitWebViewBaseNotifyResizerSize(webViewBase);
}

static gboolean toplevelWindowFocusInEvent(GtkWidget* widget, GdkEventFocus*, WebKitWebViewBase* webViewBase)
{
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    if (!priv->isInWindowActive) {
        priv->isInWindowActive = true;
        priv->pageProxy->viewStateDidChange(WebPageProxy::ViewWindowIsActive);
    }

    return FALSE;
}

static gboolean toplevelWindowFocusOutEvent(GtkWidget* widget, GdkEventFocus*, WebKitWebViewBase* webViewBase)
{
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    if (priv->isInWindowActive) {
        priv->isInWindowActive = false;
        priv->pageProxy->viewStateDidChange(WebPageProxy::ViewWindowIsActive);
    }

    return FALSE;
}

static void webkitWebViewBaseSetToplevelOnScreenWindow(WebKitWebViewBase* webViewBase, GtkWindow* window)
{
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    if (priv->toplevelOnScreenWindow == window)
        return;

    if (priv->toplevelResizeGripVisibilityID) {
        g_signal_handler_disconnect(priv->toplevelOnScreenWindow, priv->toplevelResizeGripVisibilityID);
        priv->toplevelResizeGripVisibilityID = 0;
    }
    if (priv->toplevelFocusInEventID) {
        g_signal_handler_disconnect(priv->toplevelOnScreenWindow, priv->toplevelFocusInEventID);
        priv->toplevelFocusInEventID = 0;
    }
    if (priv->toplevelFocusOutEventID) {
        g_signal_handler_disconnect(priv->toplevelOnScreenWindow, priv->toplevelFocusOutEventID);
        priv->toplevelFocusOutEventID = 0;
    }

    priv->toplevelOnScreenWindow = window;
    priv->pageProxy->viewStateDidChange(WebPageProxy::ViewIsInWindow);
    if (!priv->toplevelOnScreenWindow)
        return;

    webkitWebViewBaseNotifyResizerSize(webViewBase);

    priv->toplevelResizeGripVisibilityID =
        g_signal_connect(priv->toplevelOnScreenWindow, "notify::resize-grip-visible",
                         G_CALLBACK(toplevelWindowResizeGripVisibilityChanged), webViewBase);
    priv->toplevelFocusInEventID =
        g_signal_connect(priv->toplevelOnScreenWindow, "focus-in-event",
                         G_CALLBACK(toplevelWindowFocusInEvent), webViewBase);
    priv->toplevelFocusOutEventID =
        g_signal_connect(priv->toplevelOnScreenWindow, "focus-out-event",
                         G_CALLBACK(toplevelWindowFocusOutEvent), webViewBase);
}

static void webkitWebViewBaseRealize(GtkWidget* widget)
{
    gtk_widget_set_realized(widget, TRUE);

    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    GdkWindowAttr attributes;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK
        | GDK_EXPOSURE_MASK
        | GDK_BUTTON_PRESS_MASK
        | GDK_BUTTON_RELEASE_MASK
        | GDK_SCROLL_MASK
        | GDK_SMOOTH_SCROLL_MASK
        | GDK_POINTER_MOTION_MASK
        | GDK_KEY_PRESS_MASK
        | GDK_KEY_RELEASE_MASK
        | GDK_BUTTON_MOTION_MASK
        | GDK_BUTTON1_MOTION_MASK
        | GDK_BUTTON2_MOTION_MASK
        | GDK_BUTTON3_MOTION_MASK;

    gint attributesMask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

    GdkWindow* window = gdk_window_new(gtk_widget_get_parent_window(widget), &attributes, attributesMask);
    gtk_widget_set_window(widget, window);
    gdk_window_set_user_data(window, widget);

    gtk_style_context_set_background(gtk_widget_get_style_context(widget), window);

    WebKitWebViewBase* webView = WEBKIT_WEB_VIEW_BASE(widget);
    GtkWidget* toplevel = gtk_widget_get_toplevel(widget);
    if (widgetIsOnscreenToplevelWindow(toplevel))
        webkitWebViewBaseSetToplevelOnScreenWindow(webView, GTK_WINDOW(toplevel));
}

static bool webkitWebViewChildIsInternalWidget(WebKitWebViewBase* webViewBase, GtkWidget* widget)
{
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    return widget == priv->inspectorView || widget == priv->authenticationDialog;
}

static void webkitWebViewBaseContainerAdd(GtkContainer* container, GtkWidget* widget)
{
    WebKitWebViewBase* webView = WEBKIT_WEB_VIEW_BASE(container);
    WebKitWebViewBasePrivate* priv = webView->priv;

    // Internal widgets like the web inspector and authentication dialog have custom
    // allocations so we don't need to add them to our list of children.
    if (!webkitWebViewChildIsInternalWidget(webView, widget)) {
        GtkAllocation childAllocation;
        gtk_widget_get_allocation(widget, &childAllocation);
        priv->children.set(widget, childAllocation);
    }

    gtk_widget_set_parent(widget, GTK_WIDGET(container));
}

void webkitWebViewBaseAddAuthenticationDialog(WebKitWebViewBase* webViewBase, GtkWidget* dialog)
{
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    priv->authenticationDialog = dialog;
    gtk_container_add(GTK_CONTAINER(webViewBase), dialog);
    gtk_widget_show(dialog);

    // We need to draw the shadow over the widget.
    gtk_widget_queue_draw(GTK_WIDGET(webViewBase));
}

void webkitWebViewBaseCancelAuthenticationDialog(WebKitWebViewBase* webViewBase)
{
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    if (priv->authenticationDialog)
        gtk_widget_destroy(priv->authenticationDialog);
}

void webkitWebViewBaseAddWebInspector(WebKitWebViewBase* webViewBase, GtkWidget* inspector)
{
    webViewBase->priv->inspectorView = inspector;
    gtk_container_add(GTK_CONTAINER(webViewBase), inspector);
}

static void webkitWebViewBaseContainerRemove(GtkContainer* container, GtkWidget* widget)
{
    WebKitWebViewBase* webView = WEBKIT_WEB_VIEW_BASE(container);
    WebKitWebViewBasePrivate* priv = webView->priv;
    GtkWidget* widgetContainer = GTK_WIDGET(container);

    gboolean wasVisible = gtk_widget_get_visible(widget);
    gtk_widget_unparent(widget);

    if (priv->inspectorView == widget) {
        priv->inspectorView = 0;
        priv->inspectorViewHeight = 0;
    } else if (priv->authenticationDialog == widget) {
        priv->authenticationDialog = 0;
    } else {
        ASSERT(priv->children.contains(widget));
        priv->children.remove(widget);
    }
    if (wasVisible && gtk_widget_get_visible(widgetContainer))
        gtk_widget_queue_resize(widgetContainer);
}

static void webkitWebViewBaseContainerForall(GtkContainer* container, gboolean includeInternals, GtkCallback callback, gpointer callbackData)
{
    WebKitWebViewBase* webView = WEBKIT_WEB_VIEW_BASE(container);
    WebKitWebViewBasePrivate* priv = webView->priv;

    WebKitWebViewChildrenMap children = priv->children;
    WebKitWebViewChildrenMap::const_iterator end = children.end();
    for (WebKitWebViewChildrenMap::const_iterator current = children.begin(); current != end; ++current)
        (*callback)(current->key, callbackData);

    if (includeInternals && priv->inspectorView)
        (*callback)(priv->inspectorView, callbackData);

    if (includeInternals && priv->authenticationDialog)
        (*callback)(priv->authenticationDialog, callbackData);
}

void webkitWebViewBaseChildMoveResize(WebKitWebViewBase* webView, GtkWidget* child, const IntRect& childRect)
{
    const IntRect& geometry = webView->priv->children.get(child);
    if (geometry == childRect)
        return;

    webView->priv->children.set(child, childRect);
    gtk_widget_queue_resize_no_redraw(GTK_WIDGET(webView));
}

static void webkitWebViewBaseDispose(GObject* gobject)
{
    webkitWebViewBaseSetToplevelOnScreenWindow(WEBKIT_WEB_VIEW_BASE(gobject), 0);
    G_OBJECT_CLASS(webkit_web_view_base_parent_class)->dispose(gobject);
}

static void webkitWebViewBaseConstructed(GObject* object)
{
    G_OBJECT_CLASS(webkit_web_view_base_parent_class)->constructed(object);

    GtkWidget* viewWidget = GTK_WIDGET(object);
    gtk_widget_set_can_focus(viewWidget, TRUE);
    gtk_drag_dest_set(viewWidget, static_cast<GtkDestDefaults>(0), 0, 0,
                      static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_PRIVATE));
    gtk_drag_dest_set_target_list(viewWidget, PasteboardHelper::defaultPasteboardHelper()->targetList());

    WebKitWebViewBasePrivate* priv = WEBKIT_WEB_VIEW_BASE(object)->priv;
    priv->pageClient = PageClientImpl::create(viewWidget);
    priv->dragAndDropHelper.setWidget(viewWidget);

#if USE(TEXTURE_MAPPER_GL)
    if (priv->redirectedWindow)
        priv->redirectedWindow->setDamageNotifyCallback(redirectedWindowDamagedCallback, object);
#endif

    priv->authenticationDialog = 0;
}

#if USE(TEXTURE_MAPPER_GL)
static bool webkitWebViewRenderAcceleratedCompositingResults(WebKitWebViewBase* webViewBase, DrawingAreaProxyImpl* drawingArea, cairo_t* cr, GdkRectangle* clipRect)
{
    if (!drawingArea->isInAcceleratedCompositingMode())
        return false;

    // To avoid flashes when initializing accelerated compositing for the first
    // time, we wait until we know there's a frame ready before rendering.
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    if (!priv->redirectedWindow)
        return false;

    cairo_rectangle(cr, clipRect->x, clipRect->y, clipRect->width, clipRect->height);
    cairo_surface_t* surface = priv->redirectedWindow->cairoSurfaceForWidget(GTK_WIDGET(webViewBase));
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_fill(cr);
    return true;
}
#endif

static gboolean webkitWebViewBaseDraw(GtkWidget* widget, cairo_t* cr)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    DrawingAreaProxyImpl* drawingArea = static_cast<DrawingAreaProxyImpl*>(webViewBase->priv->pageProxy->drawingArea());
    if (!drawingArea)
        return FALSE;

    GdkRectangle clipRect;
    if (!gdk_cairo_get_clip_rectangle(cr, &clipRect))
        return FALSE;

#if USE(TEXTURE_MAPPER_GL)
    if (webkitWebViewRenderAcceleratedCompositingResults(webViewBase, drawingArea, cr, &clipRect))
        return FALSE;
#endif

    WebCore::Region unpaintedRegion; // This is simply unused.
    drawingArea->paint(cr, clipRect, unpaintedRegion);

    if (webViewBase->priv->authenticationDialog) {
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
        cairo_paint(cr);
    }

    GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->draw(widget, cr);

    return FALSE;
}

static void webkitWebViewBaseChildAllocate(GtkWidget* child, gpointer userData)
{
    if (!gtk_widget_get_visible(child))
        return;

    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(userData);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    const IntRect& geometry = priv->children.get(child);
    if (geometry.isEmpty())
        return;

    GtkAllocation childAllocation = geometry;
    gtk_widget_size_allocate(child, &childAllocation);
    priv->children.set(child, IntRect());
}

static void resizeWebKitWebViewBaseFromAllocation(WebKitWebViewBase* webViewBase, GtkAllocation* allocation, bool sizeChanged)
{
    gtk_container_foreach(GTK_CONTAINER(webViewBase), webkitWebViewBaseChildAllocate, webViewBase);

    IntRect viewRect(allocation->x, allocation->y, allocation->width, allocation->height);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    if (priv->inspectorView) {
        int inspectorViewHeight = std::min(static_cast<int>(priv->inspectorViewHeight), allocation->height);
        GtkAllocation childAllocation = viewRect;
        childAllocation.y = allocation->height - inspectorViewHeight;
        childAllocation.height = inspectorViewHeight;
        gtk_widget_size_allocate(priv->inspectorView, &childAllocation);

        viewRect.setHeight(std::max(allocation->height - inspectorViewHeight, 1));
    }

    // The authentication dialog is centered in the view rect, which means that it
    // never overlaps the web inspector. Thus, we need to calculate the allocation here
    // after calculating the inspector allocation.
    if (priv->authenticationDialog) {
        GtkRequisition naturalSize;
        gtk_widget_get_preferred_size(priv->authenticationDialog, 0, &naturalSize);

        GtkAllocation childAllocation = {
            (viewRect.width() - naturalSize.width) / 2,
            (viewRect.height() - naturalSize.height) / 2,
            naturalSize.width,
            naturalSize.height
        };
        gtk_widget_size_allocate(priv->authenticationDialog, &childAllocation);
    }

#if USE(TEXTURE_MAPPER_GL)
    if (sizeChanged && webViewBase->priv->redirectedWindow)
        webViewBase->priv->redirectedWindow->resize(viewRect.size());
#endif

    if (priv->pageProxy->drawingArea())
        priv->pageProxy->drawingArea()->setSize(viewRect.size(), IntSize(), IntSize());

    webkitWebViewBaseNotifyResizerSize(webViewBase);
}

static void webkitWebViewBaseSizeAllocate(GtkWidget* widget, GtkAllocation* allocation)
{
    bool sizeChanged = gtk_widget_get_allocated_width(widget) != allocation->width
                       || gtk_widget_get_allocated_height(widget) != allocation->height;

    GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->size_allocate(widget, allocation);

    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    if (sizeChanged && !gtk_widget_get_mapped(widget)) {
        webViewBase->priv->needsResizeOnMap = true;
        return;
    }

    resizeWebKitWebViewBaseFromAllocation(webViewBase, allocation, sizeChanged);
}

static void webkitWebViewBaseMap(GtkWidget* widget)
{
    GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->map(widget);

    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    if (!priv->isVisible) {
        priv->isVisible = true;
        priv->pageProxy->viewStateDidChange(WebPageProxy::ViewIsVisible);
    }

    if (!priv->needsResizeOnMap)
        return;

    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    resizeWebKitWebViewBaseFromAllocation(webViewBase, &allocation, true /* sizeChanged */);
    priv->needsResizeOnMap = false;
}

static void webkitWebViewBaseUnmap(GtkWidget* widget)
{
    GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->unmap(widget);

    WebKitWebViewBasePrivate* priv = WEBKIT_WEB_VIEW_BASE(widget)->priv;
    if (priv->isVisible) {
        priv->isVisible = false;
        priv->pageProxy->viewStateDidChange(WebPageProxy::ViewIsVisible);
    }
}

static gboolean webkitWebViewBaseFocusInEvent(GtkWidget* widget, GdkEventFocus* event)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    webkitWebViewBaseSetFocus(webViewBase, true);
    webViewBase->priv->inputMethodFilter.notifyFocusedIn();

    return GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->focus_in_event(widget, event);
}

static gboolean webkitWebViewBaseFocusOutEvent(GtkWidget* widget, GdkEventFocus* event)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    webkitWebViewBaseSetFocus(webViewBase, false);
    webViewBase->priv->inputMethodFilter.notifyFocusedOut();

    return GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->focus_out_event(widget, event);
}

static gboolean webkitWebViewBaseKeyPressEvent(GtkWidget* widget, GdkEventKey* event)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    if (priv->authenticationDialog)
        return GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->key_press_event(widget, event);

#if ENABLE(FULLSCREEN_API)
    if (priv->fullScreenModeActive) {
        switch (event->keyval) {
        case GDK_KEY_Escape:
        case GDK_KEY_f:
        case GDK_KEY_F:
            webkitWebViewBaseExitFullScreen(webViewBase);
            return TRUE;
        default:
            break;
        }
    }
#endif

    // Since WebProcess key event handling is not synchronous, handle the event in two passes.
    // When WebProcess processes the input event, it will call PageClientImpl::doneWithKeyEvent
    // with event handled status which determines whether to pass the input event to parent or not
    // using gtk_main_do_event().
    if (priv->shouldForwardNextKeyEvent) {
        priv->shouldForwardNextKeyEvent = FALSE;
        return GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->key_press_event(widget, event);
    }
    priv->inputMethodFilter.filterKeyEvent(event);
    return TRUE;
}

static gboolean webkitWebViewBaseKeyReleaseEvent(GtkWidget* widget, GdkEventKey* event)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    if (priv->shouldForwardNextKeyEvent) {
        priv->shouldForwardNextKeyEvent = FALSE;
        return GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->key_release_event(widget, event);
    }
    priv->inputMethodFilter.filterKeyEvent(event);
    return TRUE;
}

static gboolean webkitWebViewBaseButtonPressEvent(GtkWidget* widget, GdkEventButton* buttonEvent)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    if (priv->authenticationDialog)
        return TRUE;

    gtk_widget_grab_focus(widget);

    priv->inputMethodFilter.notifyMouseButtonPress();

    if (!priv->clickCounter.shouldProcessButtonEvent(buttonEvent))
        return TRUE;

    // If it's a right click event save it as a possible context menu event.
    if (buttonEvent->button == 3)
        priv->contextMenuEvent.set(gdk_event_copy(reinterpret_cast<GdkEvent*>(buttonEvent)));
    priv->pageProxy->handleMouseEvent(NativeWebMouseEvent(reinterpret_cast<GdkEvent*>(buttonEvent),
                                                     priv->clickCounter.clickCountForGdkButtonEvent(widget, buttonEvent)));
    return TRUE;
}

static gboolean webkitWebViewBaseButtonReleaseEvent(GtkWidget* widget, GdkEventButton* event)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    if (priv->authenticationDialog)
        return TRUE;

    gtk_widget_grab_focus(widget);
    priv->pageProxy->handleMouseEvent(NativeWebMouseEvent(reinterpret_cast<GdkEvent*>(event), 0 /* currentClickCount */));

    return TRUE;
}

static gboolean webkitWebViewBaseScrollEvent(GtkWidget* widget, GdkEventScroll* event)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    if (priv->authenticationDialog)
        return TRUE;

    priv->pageProxy->handleWheelEvent(NativeWebWheelEvent(reinterpret_cast<GdkEvent*>(event)));

    return TRUE;
}

static gboolean webkitWebViewBaseMotionNotifyEvent(GtkWidget* widget, GdkEventMotion* event)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    if (priv->authenticationDialog)
        return TRUE;

    priv->pageProxy->handleMouseEvent(NativeWebMouseEvent(reinterpret_cast<GdkEvent*>(event), 0 /* currentClickCount */));

    return TRUE;
}

static gboolean webkitWebViewBaseQueryTooltip(GtkWidget* widget, gint x, gint y, gboolean keyboardMode, GtkTooltip* tooltip)
{
    WebKitWebViewBasePrivate* priv = WEBKIT_WEB_VIEW_BASE(widget)->priv;

    if (keyboardMode) {
        // TODO: https://bugs.webkit.org/show_bug.cgi?id=61732.
        notImplemented();
        return FALSE;
    }

    if (priv->tooltipText.length() <= 0)
        return FALSE;

    if (!priv->tooltipArea.isEmpty()) {
        GdkRectangle area = priv->tooltipArea;
        gtk_tooltip_set_tip_area(tooltip, &area);
    } else
        gtk_tooltip_set_tip_area(tooltip, 0);
    gtk_tooltip_set_text(tooltip, priv->tooltipText.data());

    return TRUE;
}

static void webkitWebViewBaseDragDataGet(GtkWidget* widget, GdkDragContext* context, GtkSelectionData* selectionData, guint info, guint time)
{
    WEBKIT_WEB_VIEW_BASE(widget)->priv->dragAndDropHelper.handleGetDragData(context, selectionData, info);
}

static void webkitWebViewBaseDragEnd(GtkWidget* widget, GdkDragContext* context)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    if (!webViewBase->priv->dragAndDropHelper.handleDragEnd(context))
        return;

    GdkDevice* device = gdk_drag_context_get_device(context);
    int x = 0, y = 0;
    gdk_device_get_window_at_position(device, &x, &y);
    int xRoot = 0, yRoot = 0;
    gdk_device_get_position(device, 0, &xRoot, &yRoot);
    webViewBase->priv->pageProxy->dragEnded(IntPoint(x, y), IntPoint(xRoot, yRoot),
                                            gdkDragActionToDragOperation(gdk_drag_context_get_selected_action(context)));
}

static void webkitWebViewBaseDragDataReceived(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* selectionData, guint info, guint time)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    IntPoint position;
    DataObjectGtk* dataObject = webViewBase->priv->dragAndDropHelper.handleDragDataReceived(context, selectionData, info, position);
    if (!dataObject)
        return;

    DragData dragData(dataObject, position, convertWidgetPointToScreenPoint(widget, position), gdkDragActionToDragOperation(gdk_drag_context_get_actions(context)));
    webViewBase->priv->pageProxy->resetDragOperation();
    webViewBase->priv->pageProxy->dragEntered(&dragData);
    DragOperation operation = webViewBase->priv->pageProxy->dragSession().operation;
    gdk_drag_status(context, dragOperationToSingleGdkDragAction(operation), time);
}

static AtkObject* webkitWebViewBaseGetAccessible(GtkWidget* widget)
{
    // If the socket has already been created and embedded a plug ID, return it.
    WebKitWebViewBasePrivate* priv = WEBKIT_WEB_VIEW_BASE(widget)->priv;
    if (priv->accessible && atk_socket_is_occupied(ATK_SOCKET(priv->accessible.get())))
        return priv->accessible.get();

    // Create the accessible object and associate it to the widget.
    if (!priv->accessible) {
        priv->accessible = adoptGRef(ATK_OBJECT(webkitWebViewBaseAccessibleNew(widget)));

        // Set the parent not to break bottom-up navigation.
        GtkWidget* parentWidget = gtk_widget_get_parent(widget);
        AtkObject* axParent = parentWidget ? gtk_widget_get_accessible(parentWidget) : 0;
        if (axParent)
            atk_object_set_parent(priv->accessible.get(), axParent);
    }

    // Try to embed the plug in the socket, if posssible.
    String plugID = priv->pageProxy->accessibilityPlugID();
    if (plugID.isNull())
        return priv->accessible.get();

    atk_socket_embed(ATK_SOCKET(priv->accessible.get()), const_cast<gchar*>(plugID.utf8().data()));

    return priv->accessible.get();
}

static gboolean webkitWebViewBaseDragMotion(GtkWidget* widget, GdkDragContext* context, gint x, gint y, guint time)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    IntPoint position(x, y);
    DataObjectGtk* dataObject = webViewBase->priv->dragAndDropHelper.handleDragMotion(context, position, time);
    if (!dataObject)
        return TRUE;

    DragData dragData(dataObject, position, convertWidgetPointToScreenPoint(widget, position), gdkDragActionToDragOperation(gdk_drag_context_get_actions(context)));
    webViewBase->priv->pageProxy->dragUpdated(&dragData);
    DragOperation operation = webViewBase->priv->pageProxy->dragSession().operation;
    gdk_drag_status(context, dragOperationToSingleGdkDragAction(operation), time);
    return TRUE;
}

static void dragExitedCallback(GtkWidget* widget, DragData* dragData, bool dropHappened)
{
    // Don't call dragExited if we have just received a drag-drop signal. This
    // happens in the case of a successful drop onto the view.
    if (dropHappened)
        return;

    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    webViewBase->priv->pageProxy->dragExited(dragData);
    webViewBase->priv->pageProxy->resetDragOperation();
}

static void webkitWebViewBaseDragLeave(GtkWidget* widget, GdkDragContext* context, guint time)
{
    WEBKIT_WEB_VIEW_BASE(widget)->priv->dragAndDropHelper.handleDragLeave(context, dragExitedCallback);
}

static gboolean webkitWebViewBaseDragDrop(GtkWidget* widget, GdkDragContext* context, gint x, gint y, guint time)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    DataObjectGtk* dataObject = webViewBase->priv->dragAndDropHelper.handleDragDrop(context);
    if (!dataObject)
        return FALSE;

    IntPoint position(x, y);
    DragData dragData(dataObject, position, convertWidgetPointToScreenPoint(widget, position), gdkDragActionToDragOperation(gdk_drag_context_get_actions(context)));
    SandboxExtension::Handle handle;
    SandboxExtension::HandleArray sandboxExtensionForUpload;
    webViewBase->priv->pageProxy->performDrag(&dragData, String(), handle, sandboxExtensionForUpload);
    gtk_drag_finish(context, TRUE, FALSE, time);
    return TRUE;
}

static void webkitWebViewBaseParentSet(GtkWidget* widget, GtkWidget* oldParent)
{
    if (!gtk_widget_get_parent(widget))
        webkitWebViewBaseSetToplevelOnScreenWindow(WEBKIT_WEB_VIEW_BASE(widget), 0);
}

static gboolean webkitWebViewBaseFocus(GtkWidget* widget, GtkDirectionType direction)
{
    // If the authentication dialog is active, we need to forward focus events there. This
    // ensures that you can tab between elements in the box.
    WebKitWebViewBasePrivate* priv = WEBKIT_WEB_VIEW_BASE(widget)->priv;
    if (priv->authenticationDialog) {
        gboolean returnValue;
        g_signal_emit_by_name(priv->authenticationDialog, "focus", direction, &returnValue);
        return returnValue;
    }

    return GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->focus(widget, direction);
}

static void webkitWebViewBaseDestroy(GtkWidget* widget)
{
    WebKitWebViewBasePrivate* priv = WEBKIT_WEB_VIEW_BASE(widget)->priv;
    if (priv->authenticationDialog)
        gtk_widget_destroy(priv->authenticationDialog);

    GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->destroy(widget);
}

static void webkit_web_view_base_class_init(WebKitWebViewBaseClass* webkitWebViewBaseClass)
{
    GtkWidgetClass* widgetClass = GTK_WIDGET_CLASS(webkitWebViewBaseClass);
    widgetClass->realize = webkitWebViewBaseRealize;
    widgetClass->draw = webkitWebViewBaseDraw;
    widgetClass->size_allocate = webkitWebViewBaseSizeAllocate;
    widgetClass->map = webkitWebViewBaseMap;
    widgetClass->unmap = webkitWebViewBaseUnmap;
    widgetClass->focus = webkitWebViewBaseFocus;
    widgetClass->focus_in_event = webkitWebViewBaseFocusInEvent;
    widgetClass->focus_out_event = webkitWebViewBaseFocusOutEvent;
    widgetClass->key_press_event = webkitWebViewBaseKeyPressEvent;
    widgetClass->key_release_event = webkitWebViewBaseKeyReleaseEvent;
    widgetClass->button_press_event = webkitWebViewBaseButtonPressEvent;
    widgetClass->button_release_event = webkitWebViewBaseButtonReleaseEvent;
    widgetClass->scroll_event = webkitWebViewBaseScrollEvent;
    widgetClass->motion_notify_event = webkitWebViewBaseMotionNotifyEvent;
    widgetClass->query_tooltip = webkitWebViewBaseQueryTooltip;
    widgetClass->drag_end = webkitWebViewBaseDragEnd;
    widgetClass->drag_data_get = webkitWebViewBaseDragDataGet;
    widgetClass->drag_motion = webkitWebViewBaseDragMotion;
    widgetClass->drag_leave = webkitWebViewBaseDragLeave;
    widgetClass->drag_drop = webkitWebViewBaseDragDrop;
    widgetClass->drag_data_received = webkitWebViewBaseDragDataReceived;
    widgetClass->get_accessible = webkitWebViewBaseGetAccessible;
    widgetClass->parent_set = webkitWebViewBaseParentSet;
    widgetClass->destroy = webkitWebViewBaseDestroy;

    GObjectClass* gobjectClass = G_OBJECT_CLASS(webkitWebViewBaseClass);
    gobjectClass->constructed = webkitWebViewBaseConstructed;
    gobjectClass->dispose = webkitWebViewBaseDispose;

    GtkContainerClass* containerClass = GTK_CONTAINER_CLASS(webkitWebViewBaseClass);
    containerClass->add = webkitWebViewBaseContainerAdd;
    containerClass->remove = webkitWebViewBaseContainerRemove;
    containerClass->forall = webkitWebViewBaseContainerForall;
}

WebKitWebViewBase* webkitWebViewBaseCreate(WebContext* context, WebPageGroup* pageGroup)
{
    WebKitWebViewBase* webkitWebViewBase = WEBKIT_WEB_VIEW_BASE(g_object_new(WEBKIT_TYPE_WEB_VIEW_BASE, NULL));
    webkitWebViewBaseCreateWebPage(webkitWebViewBase, context, pageGroup);
    return webkitWebViewBase;
}

GtkIMContext* webkitWebViewBaseGetIMContext(WebKitWebViewBase* webkitWebViewBase)
{
    return webkitWebViewBase->priv->inputMethodFilter.context();
}

WebPageProxy* webkitWebViewBaseGetPage(WebKitWebViewBase* webkitWebViewBase)
{
    return webkitWebViewBase->priv->pageProxy.get();
}

void webkitWebViewBaseCreateWebPage(WebKitWebViewBase* webkitWebViewBase, WebContext* context, WebPageGroup* pageGroup)
{
    WebKitWebViewBasePrivate* priv = webkitWebViewBase->priv;

    priv->pageProxy = context->createWebPage(priv->pageClient.get(), pageGroup);
    priv->pageProxy->initializeWebPage();

#if ENABLE(FULLSCREEN_API)
    priv->pageProxy->fullScreenManager()->setWebView(webkitWebViewBase);
#endif

#if USE(TEXTURE_MAPPER_GL)
    if (priv->redirectedWindow)
        priv->pageProxy->setAcceleratedCompositingWindowId(priv->redirectedWindow->windowId());
#endif

    // This must happen here instead of the instance initializer, because the input method
    // filter must have access to the page.
    priv->inputMethodFilter.setWebView(webkitWebViewBase);
}

void webkitWebViewBaseSetTooltipText(WebKitWebViewBase* webViewBase, const char* tooltip)
{
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    if (tooltip && tooltip[0] != '\0') {
        priv->tooltipText = tooltip;
        gtk_widget_set_has_tooltip(GTK_WIDGET(webViewBase), TRUE);
    } else {
        priv->tooltipText = "";
        gtk_widget_set_has_tooltip(GTK_WIDGET(webViewBase), FALSE);
    }

    gtk_widget_trigger_tooltip_query(GTK_WIDGET(webViewBase));
}

void webkitWebViewBaseSetTooltipArea(WebKitWebViewBase* webViewBase, const IntRect& tooltipArea)
{
    webViewBase->priv->tooltipArea = tooltipArea;
}

void webkitWebViewBaseStartDrag(WebKitWebViewBase* webViewBase, const DragData& dragData, PassRefPtr<ShareableBitmap> dragImage)
{
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    RefPtr<DataObjectGtk> dataObject = adoptRef(dragData.platformData());
    GRefPtr<GtkTargetList> targetList = adoptGRef(PasteboardHelper::defaultPasteboardHelper()->targetListForDataObject(dataObject.get()));
    GOwnPtr<GdkEvent> currentEvent(gtk_get_current_event());
    GdkDragContext* context = gtk_drag_begin(GTK_WIDGET(webViewBase),
                                             targetList.get(),
                                             dragOperationToGdkDragActions(dragData.draggingSourceOperationMask()),
                                             1, /* button */
                                             currentEvent.get());
    priv->dragAndDropHelper.startedDrag(context, dataObject.get());


    // A drag starting should prevent a double-click from happening. This might
    // happen if a drag is followed very quickly by another click (like in the DRT).
    priv->clickCounter.reset();

    if (dragImage) {
        RefPtr<cairo_surface_t> image(dragImage->createCairoSurface());
        priv->dragIcon.setImage(image.get());
        priv->dragIcon.useForDrag(context);
    } else
        gtk_drag_set_icon_default(context);
}

void webkitWebViewBaseForwardNextKeyEvent(WebKitWebViewBase* webkitWebViewBase)
{
    webkitWebViewBase->priv->shouldForwardNextKeyEvent = TRUE;
}

void webkitWebViewBaseEnterFullScreen(WebKitWebViewBase* webkitWebViewBase)
{
#if ENABLE(FULLSCREEN_API)
    WebKitWebViewBasePrivate* priv = webkitWebViewBase->priv;
    if (priv->fullScreenModeActive)
        return;

    if (!priv->fullScreenClient.willEnterFullScreen())
        return;

    WebFullScreenManagerProxy* fullScreenManagerProxy = priv->pageProxy->fullScreenManager();
    fullScreenManagerProxy->willEnterFullScreen();

    GtkWidget* topLevelWindow = gtk_widget_get_toplevel(GTK_WIDGET(webkitWebViewBase));
    if (gtk_widget_is_toplevel(topLevelWindow))
        gtk_window_fullscreen(GTK_WINDOW(topLevelWindow));
    fullScreenManagerProxy->didEnterFullScreen();
    priv->fullScreenModeActive = true;
#endif
}

void webkitWebViewBaseExitFullScreen(WebKitWebViewBase* webkitWebViewBase)
{
#if ENABLE(FULLSCREEN_API)
    WebKitWebViewBasePrivate* priv = webkitWebViewBase->priv;
    if (!priv->fullScreenModeActive)
        return;

    if (!priv->fullScreenClient.willExitFullScreen())
        return;

    WebFullScreenManagerProxy* fullScreenManagerProxy = priv->pageProxy->fullScreenManager();
    fullScreenManagerProxy->willExitFullScreen();

    GtkWidget* topLevelWindow = gtk_widget_get_toplevel(GTK_WIDGET(webkitWebViewBase));
    if (gtk_widget_is_toplevel(topLevelWindow))
        gtk_window_unfullscreen(GTK_WINDOW(topLevelWindow));
    fullScreenManagerProxy->didExitFullScreen();
    priv->fullScreenModeActive = false;
#endif
}

void webkitWebViewBaseInitializeFullScreenClient(WebKitWebViewBase* webkitWebViewBase, const WKFullScreenClientGtk* wkClient)
{
    webkitWebViewBase->priv->fullScreenClient.initialize(wkClient);
}

void webkitWebViewBaseSetInspectorViewHeight(WebKitWebViewBase* webkitWebViewBase, unsigned height)
{
    if (webkitWebViewBase->priv->inspectorViewHeight == height)
        return;
    webkitWebViewBase->priv->inspectorViewHeight = height;
    if (webkitWebViewBase->priv->inspectorView)
        gtk_widget_queue_resize_no_redraw(GTK_WIDGET(webkitWebViewBase));
}

void webkitWebViewBaseSetActiveContextMenuProxy(WebKitWebViewBase* webkitWebViewBase, WebContextMenuProxyGtk* contextMenuProxy)
{
    webkitWebViewBase->priv->activeContextMenuProxy = contextMenuProxy;
}

WebContextMenuProxyGtk* webkitWebViewBaseGetActiveContextMenuProxy(WebKitWebViewBase* webkitWebViewBase)
{
    return webkitWebViewBase->priv->activeContextMenuProxy;
}

GdkEvent* webkitWebViewBaseTakeContextMenuEvent(WebKitWebViewBase* webkitWebViewBase)
{
    return webkitWebViewBase->priv->contextMenuEvent.release();
}

#if USE(TEXTURE_MAPPER_GL)
void redirectedWindowDamagedCallback(void* data)
{
    gtk_widget_queue_draw(GTK_WIDGET(data));
}
#endif

void webkitWebViewBaseSetFocus(WebKitWebViewBase* webViewBase, bool focused)
{
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    if (priv->isFocused == focused)
        return;

    unsigned viewStateFlags = WebPageProxy::ViewIsFocused;
    priv->isFocused = focused;

    // If the view has received the focus and the window is not active
    // mark the current window as active now. This can happen if the
    // toplevel window is a GTK_WINDOW_POPUP and the focus has been
    // set programatically like WebKitTestRunner does, because POPUP
    // can't be focused.
    if (priv->isFocused && !priv->isInWindowActive) {
        priv->isInWindowActive = true;
        viewStateFlags |= WebPageProxy::ViewWindowIsActive;
    }
    priv->pageProxy->viewStateDidChange(viewStateFlags);
}

bool webkitWebViewBaseIsInWindowActive(WebKitWebViewBase* webViewBase)
{
    return webViewBase->priv->isInWindowActive;
}

bool webkitWebViewBaseIsFocused(WebKitWebViewBase* webViewBase)
{
    return webViewBase->priv->isFocused;
}

bool webkitWebViewBaseIsVisible(WebKitWebViewBase* webViewBase)
{
    return webViewBase->priv->isVisible;
}

bool webkitWebViewBaseIsInWindow(WebKitWebViewBase* webViewBase)
{
    return webViewBase->priv->toplevelOnScreenWindow;
}

void webkitWebViewBaseSetDownloadRequestHandler(WebKitWebViewBase* webViewBase, WebKitWebViewBaseDownloadRequestHandler downloadHandler)
{
    webViewBase->priv->downloadHandler = downloadHandler;
}

void webkitWebViewBaseHandleDownloadRequest(WebKitWebViewBase* webViewBase, DownloadProxy* download)
{
    if (webViewBase->priv->downloadHandler)
        webViewBase->priv->downloadHandler(webViewBase, download);
}

void webkitWebViewBaseSetInputMethodState(WebKitWebViewBase* webkitWebViewBase, bool enabled)
{
    webkitWebViewBase->priv->inputMethodFilter.setEnabled(enabled);
}

void webkitWebViewBaseUpdateTextInputState(WebKitWebViewBase* webkitWebViewBase)
{
    webkitWebViewBase->priv->inputMethodFilter.setCursorRect(webkitWebViewBase->priv->pageProxy->editorState().cursorRect);
}
