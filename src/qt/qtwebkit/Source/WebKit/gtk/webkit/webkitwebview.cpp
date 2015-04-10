/*
 *  Copyright (C) 2007, 2008 Holger Hans Peter Freyther
 *  Copyright (C) 2007, 2008, 2009 Christian Dywan <christian@imendio.com>
 *  Copyright (C) 2007 Xan Lopez <xan@gnome.org>
 *  Copyright (C) 2007, 2008 Alp Toker <alp@atoker.com>
 *  Copyright (C) 2008 Jan Alonzo <jmalonzo@unpluggable.com>
 *  Copyright (C) 2008 Gustavo Noronha Silva <gns@gnome.org>
 *  Copyright (C) 2008 Nuanti Ltd.
 *  Copyright (C) 2008, 2009, 2010 Collabora Ltd.
 *  Copyright (C) 2009, 2010, 2012 Igalia S.L.
 *  Copyright (C) 2009 Movial Creative Technologies Inc.
 *  Copyright (C) 2009 Bobby Powers
 *  Copyright (C) 2010 Joone Hur <joone@kldp.org>
 *  Copyright (C) 2012 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "webkitwebview.h"

#include "AXObjectCache.h"
#include "ArchiveResource.h"
#include "BackForwardListImpl.h"
#include "CairoUtilities.h"
#include "Chrome.h"
#include "ChromeClientGtk.h"
#include "ClipboardUtilitiesGtk.h"
#include "ContextMenu.h"
#include "ContextMenuClientGtk.h"
#include "ContextMenuController.h"
#include "Cursor.h"
#include "DatabaseManager.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "DragActions.h"
#include "DragClientGtk.h"
#include "DragController.h"
#include "DragData.h"
#include "DragSession.h"
#include "DumpRenderTreeSupportGtk.h"
#include "Editor.h"
#include "EditorClientGtk.h"
#include "EventHandler.h"
#include "FloatQuad.h"
#include "FocusController.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameLoaderTypes.h"
#include "FrameView.h"
#include "GOwnPtrGtk.h"
#include "GeolocationClientGtk.h"
#include "GeolocationController.h"
#include "GraphicsContext.h"
#include "GtkUtilities.h"
#include "GtkVersioning.h"
#include "HTMLNames.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "InspectorClientGtk.h"
#include "MemoryCache.h"
#include "MouseEventWithHitTestResults.h"
#include "NotImplemented.h"
#include "PageCache.h"
#include "Pasteboard.h"
#include "PasteboardHelper.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformWheelEvent.h"
#include "ProgressTracker.h"
#include "RenderView.h"
#include "ResourceHandle.h"
#include "RuntimeEnabledFeatures.h"
#include "ScriptController.h"
#include "ScriptValue.h"
#include "Settings.h"
#include "WebKitDOMDocumentPrivate.h"
#include "webkitdownload.h"
#include "webkitdownloadprivate.h"
#include "webkitenumtypes.h"
#include "webkitfavicondatabase.h"
#include "webkitgeolocationpolicydecision.h"
#include "webkitglobalsprivate.h"
#include "webkithittestresultprivate.h"
#include "webkiticondatabase.h"
#include "webkitmarshal.h"
#include "webkitnetworkrequest.h"
#include "webkitnetworkresponse.h"
#include "webkitviewportattributes.h"
#include "webkitviewportattributesprivate.h"
#include "webkitwebbackforwardlist.h"
#include "webkitwebframeprivate.h"
#include "webkitwebhistoryitem.h"
#include "webkitwebhistoryitemprivate.h"
#include "webkitwebinspector.h"
#include "webkitwebinspectorprivate.h"
#include "webkitwebpolicydecision.h"
#include "webkitwebresource.h"
#include "webkitwebresourceprivate.h"
#include "webkitwebsettingsprivate.h"
#include "webkitwebplugindatabaseprivate.h"
#include "webkitwebwindowfeatures.h"
#include "webkitwebviewprivate.h"
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n-lib.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

#if ENABLE(DEVICE_ORIENTATION)
#include "DeviceMotionClientGtk.h"
#include "DeviceOrientationClientGtk.h"
#endif

/**
 * SECTION:webkitwebview
 * @short_description: The central class of the WebKitGTK+ API
 * @see_also: #WebKitWebSettings, #WebKitWebFrame
 *
 * #WebKitWebView is the central class of the WebKitGTK+ API. It is a
 * #GtkWidget implementing the scrolling interface which means you can
 * embed in a #GtkScrolledWindow. It is responsible for managing the
 * drawing of the content, forwarding of events. You can load any URI
 * into the #WebKitWebView or any kind of data string. With #WebKitWebSettings
 * you can control various aspects of the rendering and loading of the content.
 * Each #WebKitWebView has exactly one #WebKitWebFrame as main frame. A
 * #WebKitWebFrame can have n children.
 *
 * <programlisting>
 * /<!-- -->* Create the widgets *<!-- -->/
 * GtkWidget *main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
 * GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
 * GtkWidget *web_view = webkit_web_view_new ();
 *
 * /<!-- -->* Place the WebKitWebView in the GtkScrolledWindow *<!-- -->/
 * gtk_container_add (GTK_CONTAINER (scrolled_window), web_view);
 * gtk_container_add (GTK_CONTAINER (main_window), scrolled_window);
 *
 * /<!-- -->* Open a webpage *<!-- -->/
 * webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view), "http://www.gnome.org");
 *
 * /<!-- -->* Show the result *<!-- -->/
 * gtk_window_set_default_size (GTK_WINDOW (main_window), 800, 600);
 * gtk_widget_show_all (main_window);
 * </programlisting>
 */

using namespace WebKit;
using namespace WebCore;

enum {
    /* normal signals */
    NAVIGATION_REQUESTED,
    NEW_WINDOW_POLICY_DECISION_REQUESTED,
    NAVIGATION_POLICY_DECISION_REQUESTED,
    MIME_TYPE_POLICY_DECISION_REQUESTED,
    CREATE_WEB_VIEW,
    WEB_VIEW_READY,
    WINDOW_OBJECT_CLEARED,
    LOAD_STARTED,
    LOAD_COMMITTED,
    LOAD_PROGRESS_CHANGED,
    LOAD_ERROR,
    LOAD_FINISHED,
    TITLE_CHANGED,
    HOVERING_OVER_LINK,
    POPULATE_POPUP,
    STATUS_BAR_TEXT_CHANGED,
    ICON_LOADED,
    SELECTION_CHANGED,
    CONSOLE_MESSAGE,
    SCRIPT_ALERT,
    SCRIPT_CONFIRM,
    SCRIPT_PROMPT,
    SELECT_ALL,
    COPY_CLIPBOARD,
    PASTE_CLIPBOARD,
    CUT_CLIPBOARD,
    DOWNLOAD_REQUESTED,
    MOVE_CURSOR,
    PRINT_REQUESTED,
    PLUGIN_WIDGET,
    CLOSE_WEB_VIEW,
    UNDO,
    REDO,
    DATABASE_QUOTA_EXCEEDED,
    RESOURCE_REQUEST_STARTING,
    DOCUMENT_LOAD_FINISHED,
    GEOLOCATION_POLICY_DECISION_REQUESTED,
    GEOLOCATION_POLICY_DECISION_CANCELLED,
    ONLOAD_EVENT,
    FRAME_CREATED,
    SHOULD_BEGIN_EDITING,
    SHOULD_END_EDITING,
    SHOULD_INSERT_NODE,
    SHOULD_INSERT_TEXT,
    SHOULD_DELETE_RANGE,
    SHOULD_SHOW_DELETE_INTERFACE_FOR_ELEMENT,
    SHOULD_CHANGE_SELECTED_RANGE,
    SHOULD_APPLY_STYLE,
    EDITING_BEGAN,
    USER_CHANGED_CONTENTS,
    EDITING_ENDED,
    VIEWPORT_ATTRIBUTES_RECOMPUTE_REQUESTED,
    VIEWPORT_ATTRIBUTES_CHANGED,
    RESOURCE_RESPONSE_RECEIVED,
    RESOURCE_LOAD_FINISHED,
    RESOURCE_CONTENT_LENGTH_RECEIVED,
    RESOURCE_LOAD_FAILED,
    ENTERING_FULLSCREEN,
    LEAVING_FULLSCREEN,
    CONTEXT_MENU,
    RUN_FILE_CHOOSER,

    LAST_SIGNAL
};

enum {
    PROP_0,

    PROP_TITLE,
    PROP_URI,
    PROP_COPY_TARGET_LIST,
    PROP_PASTE_TARGET_LIST,
    PROP_EDITABLE,
    PROP_SETTINGS,
    PROP_WEB_INSPECTOR,
    PROP_VIEWPORT_ATTRIBUTES,
    PROP_WINDOW_FEATURES,
    PROP_TRANSPARENT,
    PROP_ZOOM_LEVEL,
    PROP_FULL_CONTENT_ZOOM,
    PROP_LOAD_STATUS,
    PROP_PROGRESS,
    PROP_ENCODING,
    PROP_CUSTOM_ENCODING,
    PROP_ICON_URI,
    PROP_IM_CONTEXT,
#ifdef GTK_API_VERSION_2
    PROP_VIEW_MODE,
#else
    PROP_VIEW_MODE,
    PROP_HADJUSTMENT,
    PROP_VADJUSTMENT,
    PROP_HSCROLL_POLICY,
    PROP_VSCROLL_POLICY,
#endif

    // Undocumented. Leave these properties at the end of the list
    // so that we can remove them without breaking ABI compatibility.
    PROP_SELF_SCROLLING
};

static guint webkit_web_view_signals[LAST_SIGNAL] = { 0, };

#ifdef GTK_API_VERSION_2
G_DEFINE_TYPE(WebKitWebView, webkit_web_view, GTK_TYPE_CONTAINER)
#else
G_DEFINE_TYPE_WITH_CODE(WebKitWebView, webkit_web_view, GTK_TYPE_CONTAINER,
                        G_IMPLEMENT_INTERFACE(GTK_TYPE_SCROLLABLE, 0))
#endif

static void webkit_web_view_settings_notify(WebKitWebSettings* webSettings, GParamSpec* pspec, WebKitWebView* webView);
static void webkit_web_view_set_window_features(WebKitWebView* webView, WebKitWebWindowFeatures* webWindowFeatures);
static void webkitWebViewDirectionChanged(WebKitWebView*, GtkTextDirection previousDirection, gpointer);

#if ENABLE(CONTEXT_MENUS)
static void PopupMenuPositionFunc(GtkMenu* menu, gint *x, gint *y, gboolean *pushIn, gpointer userData)
{
    WebKitWebView* view = WEBKIT_WEB_VIEW(userData);
    WebKitWebViewPrivate* priv = view->priv;
    GdkScreen* screen = gtk_widget_get_screen(GTK_WIDGET(view));
    GtkRequisition menuSize;

#ifdef GTK_API_VERSION_2
    gtk_widget_size_request(GTK_WIDGET(menu), &menuSize);
#else
    gtk_widget_get_preferred_size(GTK_WIDGET(menu), &menuSize, NULL);
#endif

    *x = priv->lastPopupXPosition;
    if ((*x + menuSize.width) >= gdk_screen_get_width(screen))
      *x -= menuSize.width;

    *y = priv->lastPopupYPosition;
    if ((*y + menuSize.height) >= gdk_screen_get_height(screen))
      *y -= menuSize.height;

    *pushIn = FALSE;
}
#endif

static Node* getFocusedNode(Frame* frame)
{
    if (Document* doc = frame->document())
        return doc->focusedElement();
    return 0;
}

#if ENABLE(CONTEXT_MENUS)
static void contextMenuItemActivated(GtkMenuItem* item, ContextMenuController* controller)
{
    ContextMenuItem contextItem(item);
    controller->contextMenuItemSelected(&contextItem);
}

static void contextMenuConnectActivate(GtkMenuItem* item, ContextMenuController* controller)
{
    if (GTK_IS_SEPARATOR_MENU_ITEM(item))
        return;

    if (GtkWidget* menu = gtk_menu_item_get_submenu(item)) {
        gtk_container_foreach(GTK_CONTAINER(menu), (GtkCallback)contextMenuConnectActivate, controller);
        return;
    }

    g_signal_connect(item, "activate", G_CALLBACK(contextMenuItemActivated), controller);
}

static MouseEventWithHitTestResults prepareMouseEventForFrame(Frame* frame, const PlatformMouseEvent& event)
{
    HitTestRequest request(HitTestRequest::Active| HitTestRequest::DisallowShadowContent);
    IntPoint point = frame->view()->windowToContents(event.position());
    return frame->document()->prepareMouseEvent(request, point, event);
}

// Check enable-default-context-menu setting for compatibility.
static bool defaultContextMenuEnabled(WebKitWebView* webView)
{
    gboolean enableDefaultContextMenu;
    g_object_get(webkit_web_view_get_settings(webView), "enable-default-context-menu", &enableDefaultContextMenu, NULL);
    return enableDefaultContextMenu;
}

static gboolean webkit_web_view_forward_context_menu_event(WebKitWebView* webView, const PlatformMouseEvent& event, bool triggeredWithKeyboard)
{
    Page* page = core(webView);
    page->contextMenuController()->clearContextMenu();
    Frame* focusedFrame;
    Frame* mainFrame = page->mainFrame();
    gboolean mousePressEventResult = FALSE;
    GRefPtr<WebKitHitTestResult> hitTestResult;

    if (!mainFrame->view())
        return FALSE;

    mainFrame->view()->setCursor(pointerCursor());
    if (page->subframeCount()) {
        MouseEventWithHitTestResults mev = prepareMouseEventForFrame(mainFrame, event);
        Frame* targetFrame = EventHandler::subframeForHitTestResult(mev);
        if (!targetFrame)
            targetFrame = mainFrame;

        focusedFrame = page->focusController()->focusedOrMainFrame();
        if (targetFrame != focusedFrame) {
            page->focusController()->setFocusedFrame(targetFrame);
            focusedFrame = targetFrame;
        }
        if (focusedFrame == mainFrame)
            hitTestResult = adoptGRef(kit(mev.hitTestResult()));
    } else
        focusedFrame = mainFrame;

    if (focusedFrame->view() && focusedFrame->eventHandler()->handleMousePressEvent(event))
        mousePressEventResult = TRUE;

    bool handledEvent = focusedFrame->eventHandler()->sendContextMenuEvent(event);
    if (!handledEvent)
        return FALSE;

    // If coreMenu is NULL, this means WebCore decided to not create
    // the default context menu; this may happen when the page is
    // handling the right-click for reasons other than the context menu.
    ContextMenuController* controller = page->contextMenuController();
    ContextMenu* coreMenu = controller->contextMenu();
    if (!coreMenu)
        return mousePressEventResult;

    GtkMenu* defaultMenu = coreMenu->platformDescription();
    ASSERT(defaultMenu);

    // We connect the "activate" signal here rather than in ContextMenuGtk to avoid
    // a layering violation. ContextMenuGtk should not know about the ContextMenuController.
    gtk_container_foreach(GTK_CONTAINER(defaultMenu), reinterpret_cast<GtkCallback>(contextMenuConnectActivate), controller);

    if (!hitTestResult) {
        MouseEventWithHitTestResults mev = prepareMouseEventForFrame(focusedFrame, event);
        hitTestResult = adoptGRef(kit(mev.hitTestResult()));
    }

    gboolean handled;
    g_signal_emit(webView, webkit_web_view_signals[CONTEXT_MENU], 0, defaultMenu, hitTestResult.get(), triggeredWithKeyboard, &handled);
    if (handled)
        return TRUE;

    // Return now if default context menu is disabled by enable-default-context-menu setting.
    // Check enable-default-context-menu setting for compatibility.
    if (!defaultContextMenuEnabled(webView))
        return FALSE;

    // Emit populate-popup signal for compatibility.
    g_signal_emit(webView, webkit_web_view_signals[POPULATE_POPUP], 0, defaultMenu);

    // If the context menu is now empty, don't show it.
    GOwnPtr<GList> items(gtk_container_get_children(GTK_CONTAINER(defaultMenu)));
    if (!items)
        return FALSE;

    WebKitWebViewPrivate* priv = webView->priv;
    priv->currentMenu = defaultMenu;
    priv->lastPopupXPosition = event.globalPosition().x();
    priv->lastPopupYPosition = event.globalPosition().y();

    gtk_menu_popup(defaultMenu, 0, 0, &PopupMenuPositionFunc, webView, event.button() + 1, gtk_get_current_event_time());
    return TRUE;
}

static const int gContextMenuMargin = 1;
static IntPoint getLocationForKeyboardGeneratedContextMenu(Frame* frame)
{
    FrameSelection* selection = frame->selection();
    if (!selection->selection().isNonOrphanedCaretOrRange()
         || (selection->selection().isCaret() && !selection->selection().isContentEditable())) {
        if (Node* focusedNode = getFocusedNode(frame))
            return focusedNode->pixelSnappedBoundingBox().location();

        // There was no selection and no focused node, so just put the context
        // menu into the corner of the view, offset slightly.
        return IntPoint(gContextMenuMargin, gContextMenuMargin);
    }

    // selection->selection().firstRange can return 0 here, but if that was the case
    // selection->selection().isNonOrphanedCaretOrRange() would have returned false
    // above, so we do not have to check it.
    IntRect firstRect = frame->editor().firstRectForRange(selection->selection().firstRange().get());
    return IntPoint(firstRect.x(), firstRect.maxY());
}

static gboolean webkit_web_view_popup_menu_handler(GtkWidget* widget)
{
    Frame* frame = core(WEBKIT_WEB_VIEW(widget))->focusController()->focusedOrMainFrame();
    IntPoint location = getLocationForKeyboardGeneratedContextMenu(frame);

    FrameView* view = frame->view();
    if (!view)
        return FALSE;

    // Never let the context menu touch the very edge of the view.
    location = view->contentsToWindow(location);
    location.expandedTo(IntPoint(gContextMenuMargin, gContextMenuMargin));
    location.shrunkTo(IntPoint(view->width() - gContextMenuMargin, view->height() - gContextMenuMargin));

    IntPoint globalPoint(convertWidgetPointToScreenPoint(widget, location));
    PlatformMouseEvent event(location, globalPoint, RightButton, PlatformEvent::MousePressed, 0, false, false, false, false, gtk_get_current_event_time());
    return webkit_web_view_forward_context_menu_event(WEBKIT_WEB_VIEW(widget), event, true);
}
#endif // ENABLE(CONTEXT_MENUS)

static void setHorizontalAdjustment(WebKitWebView* webView, GtkAdjustment* adjustment)
{
    // This may be called after the page has been destroyed, in which case we do nothing.
    Page* page = core(webView);
    if (page)
        static_cast<WebKit::ChromeClient*>(page->chrome().client())->adjustmentWatcher()->setHorizontalAdjustment(adjustment);
}

static void setVerticalAdjustment(WebKitWebView* webView, GtkAdjustment* adjustment)
{
    // This may be called after the page has been destroyed, in which case we do nothing.
    Page* page = core(webView);
    if (page)
        static_cast<WebKit::ChromeClient*>(page->chrome().client())->adjustmentWatcher()->setVerticalAdjustment(adjustment);
}

#ifndef GTK_API_VERSION_2
static GtkAdjustment* getHorizontalAdjustment(WebKitWebView* webView)
{
    Page* page = core(webView);
    if (page)
        return static_cast<WebKit::ChromeClient*>(page->chrome().client())->adjustmentWatcher()->horizontalAdjustment();
    return 0;
}

static GtkAdjustment* getVerticalAdjustment(WebKitWebView* webView)
{
    Page* page = core(webView);
    if (page)
        return static_cast<WebKit::ChromeClient*>(page->chrome().client())->adjustmentWatcher()->verticalAdjustment();
    return 0;
}

static void setHorizontalScrollPolicy(WebKitWebView* webView, GtkScrollablePolicy policy)
{
    webView->priv->horizontalScrollingPolicy = policy;
    gtk_widget_queue_resize(GTK_WIDGET(webView));
}

static void setVerticalScrollPolicy(WebKitWebView* webView, GtkScrollablePolicy policy)
{
    webView->priv->verticalScrollingPolicy = policy;
    gtk_widget_queue_resize(GTK_WIDGET(webView));
}

static GtkScrollablePolicy getHorizontalScrollPolicy(WebKitWebView* webView)
{
    return webView->priv->horizontalScrollingPolicy;
}

static GtkScrollablePolicy getVerticalScrollPolicy(WebKitWebView* webView)
{
    return webView->priv->verticalScrollingPolicy;
}

#endif

static void webkit_web_view_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(object);

    switch(prop_id) {
    case PROP_TITLE:
        g_value_set_string(value, webkit_web_view_get_title(webView));
        break;
    case PROP_URI:
        g_value_set_string(value, webkit_web_view_get_uri(webView));
        break;
    case PROP_COPY_TARGET_LIST:
        g_value_set_boxed(value, webkit_web_view_get_copy_target_list(webView));
        break;
    case PROP_PASTE_TARGET_LIST:
        g_value_set_boxed(value, webkit_web_view_get_paste_target_list(webView));
        break;
    case PROP_EDITABLE:
        g_value_set_boolean(value, webkit_web_view_get_editable(webView));
        break;
    case PROP_SETTINGS:
        g_value_set_object(value, webkit_web_view_get_settings(webView));
        break;
    case PROP_WEB_INSPECTOR:
        g_value_set_object(value, webkit_web_view_get_inspector(webView));
        break;
    case PROP_VIEWPORT_ATTRIBUTES:
        g_value_set_object(value, webkit_web_view_get_viewport_attributes(webView));
        break;
    case PROP_WINDOW_FEATURES:
        g_value_set_object(value, webkit_web_view_get_window_features(webView));
        break;
    case PROP_TRANSPARENT:
        g_value_set_boolean(value, webkit_web_view_get_transparent(webView));
        break;
    case PROP_ZOOM_LEVEL:
        g_value_set_float(value, webkit_web_view_get_zoom_level(webView));
        break;
    case PROP_FULL_CONTENT_ZOOM:
        g_value_set_boolean(value, webkit_web_view_get_full_content_zoom(webView));
        break;
    case PROP_ENCODING:
        g_value_set_string(value, webkit_web_view_get_encoding(webView));
        break;
    case PROP_CUSTOM_ENCODING:
        g_value_set_string(value, webkit_web_view_get_custom_encoding(webView));
        break;
    case PROP_LOAD_STATUS:
        g_value_set_enum(value, webkit_web_view_get_load_status(webView));
        break;
    case PROP_PROGRESS:
        g_value_set_double(value, webkit_web_view_get_progress(webView));
        break;
    case PROP_ICON_URI:
        g_value_set_string(value, webkit_web_view_get_icon_uri(webView));
        break;
    case PROP_IM_CONTEXT:
        g_value_set_object(value, webView->priv->imFilter.context());
        break;
    case PROP_VIEW_MODE:
        g_value_set_enum(value, webkit_web_view_get_view_mode(webView));
        break;
#ifndef GTK_API_VERSION_2
    case PROP_HADJUSTMENT:
        g_value_set_object(value, getHorizontalAdjustment(webView));
        break;
    case PROP_VADJUSTMENT:
        g_value_set_object(value, getVerticalAdjustment(webView));
        break;
    case PROP_HSCROLL_POLICY:
        g_value_set_enum(value, getHorizontalScrollPolicy(webView));
        break;
    case PROP_VSCROLL_POLICY:
        g_value_set_enum(value, getVerticalScrollPolicy(webView));
        break;
#endif
    case PROP_SELF_SCROLLING:
        g_value_set_boolean(value, webView->priv->selfScrolling);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void webkit_web_view_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec *pspec)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(object);

    switch(prop_id) {
    case PROP_EDITABLE:
        webkit_web_view_set_editable(webView, g_value_get_boolean(value));
        break;
    case PROP_SETTINGS:
        webkit_web_view_set_settings(webView, WEBKIT_WEB_SETTINGS(g_value_get_object(value)));
        break;
    case PROP_WINDOW_FEATURES:
        webkit_web_view_set_window_features(webView, WEBKIT_WEB_WINDOW_FEATURES(g_value_get_object(value)));
        break;
    case PROP_TRANSPARENT:
        webkit_web_view_set_transparent(webView, g_value_get_boolean(value));
        break;
    case PROP_ZOOM_LEVEL:
        webkit_web_view_set_zoom_level(webView, g_value_get_float(value));
        break;
    case PROP_FULL_CONTENT_ZOOM:
        webkit_web_view_set_full_content_zoom(webView, g_value_get_boolean(value));
        break;
    case PROP_CUSTOM_ENCODING:
        webkit_web_view_set_custom_encoding(webView, g_value_get_string(value));
        break;
    case PROP_VIEW_MODE:
        webkit_web_view_set_view_mode(webView, static_cast<WebKitWebViewViewMode>(g_value_get_enum(value)));
        break;
#ifndef GTK_API_VERSION_2
    case PROP_HADJUSTMENT:
        setHorizontalAdjustment(webView, static_cast<GtkAdjustment*>(g_value_get_object(value)));
        break;
    case PROP_VADJUSTMENT:
        setVerticalAdjustment(webView, static_cast<GtkAdjustment*>(g_value_get_object(value)));
        break;
    case PROP_HSCROLL_POLICY:
        setHorizontalScrollPolicy(webView, static_cast<GtkScrollablePolicy>(g_value_get_enum(value)));
        break;
    case PROP_VSCROLL_POLICY:
        setVerticalScrollPolicy(webView, static_cast<GtkScrollablePolicy>(g_value_get_enum(value)));
        break;
#endif
    case PROP_SELF_SCROLLING:
        webView->priv->selfScrolling = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

#ifdef GTK_API_VERSION_2
static gboolean webkit_web_view_expose_event(GtkWidget* widget, GdkEventExpose* event)
{
    int rectCount;
    GOwnPtr<GdkRectangle> rects;
    gdk_region_get_rectangles(event->region, &rects.outPtr(), &rectCount);

    RefPtr<cairo_t> cr = adoptRef(gdk_cairo_create(event->window));
    for (int i = 0; i < rectCount; i++) {
        copyRectFromCairoSurfaceToContext(WEBKIT_WEB_VIEW(widget)->priv->backingStore->cairoSurface(),
                                          cr.get(), IntSize(), IntRect(rects.get()[i]));
    }

    // Chaining up to the parent forces child widgets to be drawn.
    GTK_WIDGET_CLASS(webkit_web_view_parent_class)->expose_event(widget, event);
    return FALSE;
}
#else
static gboolean webkit_web_view_draw(GtkWidget* widget, cairo_t* cr)
{
    GdkRectangle clipRect;
    if (!gdk_cairo_get_clip_rectangle(cr, &clipRect))
        return FALSE;

    WebKitWebViewPrivate* priv = WEBKIT_WEB_VIEW(widget)->priv;
#if USE(TEXTURE_MAPPER)
    if (priv->acceleratedCompositingContext->renderLayersToWindow(cr, clipRect)) {
        GTK_WIDGET_CLASS(webkit_web_view_parent_class)->draw(widget, cr);
        return FALSE;
    }
#endif

    cairo_rectangle_list_t* rectList = cairo_copy_clip_rectangle_list(cr);
    if (rectList->status || !rectList->num_rectangles) {
        cairo_rectangle_list_destroy(rectList);
        return FALSE;
    }

    Vector<IntRect> rects;
    for (int i = 0; i < rectList->num_rectangles; i++) {
        copyRectFromCairoSurfaceToContext(priv->backingStore->cairoSurface(), cr, IntSize(),
                                          enclosingIntRect(FloatRect(rectList->rectangles[i])));
    }
    cairo_rectangle_list_destroy(rectList);

    // Chaining up to the parent forces child widgets to be drawn.
    GTK_WIDGET_CLASS(webkit_web_view_parent_class)->draw(widget, cr);
    return FALSE;
}
#endif // GTK_API_VERSION_2

static gboolean webkit_web_view_key_press_event(GtkWidget* widget, GdkEventKey* event)
{
    if (WEBKIT_WEB_VIEW(widget)->priv->imFilter.filterKeyEvent(event))
        return TRUE;
    return GTK_WIDGET_CLASS(webkit_web_view_parent_class)->key_press_event(widget, event);
}

static gboolean webkit_web_view_key_release_event(GtkWidget* widget, GdkEventKey* event)
{
    if (WEBKIT_WEB_VIEW(widget)->priv->imFilter.filterKeyEvent(event))
        return TRUE;
    return GTK_WIDGET_CLASS(webkit_web_view_parent_class)->key_release_event(widget, event);
}

static gboolean webkit_web_view_button_press_event(GtkWidget* widget, GdkEventButton* event)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);
    WebKitWebViewPrivate* priv = webView->priv;

    // FIXME: need to keep track of subframe focus for key events
    gtk_widget_grab_focus(widget);

    if (!priv->clickCounter.shouldProcessButtonEvent(event))
        return TRUE;

    PlatformMouseEvent platformEvent(event);
    int count = priv->clickCounter.clickCountForGdkButtonEvent(widget, event);
    platformEvent.setClickCount(count);

#if ENABLE(CONTEXT_MENUS)
    if (event->button == 3)
        return webkit_web_view_forward_context_menu_event(webView, PlatformMouseEvent(event), false);
#endif

    Frame* frame = core(webView)->mainFrame();
    if (!frame->view())
        return FALSE;

    priv->imFilter.notifyMouseButtonPress();
    gboolean result = frame->eventHandler()->handleMousePressEvent(platformEvent);

    return result;
}

static gboolean webkit_web_view_button_release_event(GtkWidget* widget, GdkEventButton* event)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);

    Frame* mainFrame = core(webView)->mainFrame();
    if (mainFrame->view())
        mainFrame->eventHandler()->handleMouseReleaseEvent(PlatformMouseEvent(event));

    /* We always return FALSE here because WebKit can, for the same click, decide
     * to not handle press-event but handle release-event, which can totally confuse
     * some GTK+ containers when there are no other events in between. This way we
     * guarantee that this case never happens, and that if press-event goes through
     * release-event also goes through.
     */

    return FALSE;
}

static gboolean webkit_web_view_motion_event(GtkWidget* widget, GdkEventMotion* event)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);

    Frame* frame = core(webView)->mainFrame();
    if (!frame->view())
        return FALSE;

    return frame->eventHandler()->mouseMoved(PlatformMouseEvent(event));
}

static gboolean webkit_web_view_scroll_event(GtkWidget* widget, GdkEventScroll* event)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);

    Frame* frame = core(webView)->mainFrame();
    if (!frame->view())
        return FALSE;

    PlatformWheelEvent wheelEvent(event);
    return frame->eventHandler()->handleWheelEvent(wheelEvent);
}

#ifdef GTK_API_VERSION_2
static void webkit_web_view_size_request(GtkWidget* widget, GtkRequisition* requisition)
{
    WebKitWebView* web_view = WEBKIT_WEB_VIEW(widget);
    Frame* coreFrame = core(webkit_web_view_get_main_frame(web_view));
    if (!coreFrame)
        return;

    FrameView* view = coreFrame->view();
    if (!view)
        return;

    requisition->width = view->contentsWidth();
    requisition->height = view->contentsHeight();
}
#else
static void webkit_web_view_get_preferred_width(GtkWidget* widget, gint* minimum, gint* natural)
{
    WebKitWebView* web_view = WEBKIT_WEB_VIEW(widget);
    Frame* coreFrame = core(webkit_web_view_get_main_frame(web_view));
    if (!coreFrame)
        return;

    FrameView* view = coreFrame->view();
    if (!view)
        return;

    *minimum = *natural = view->contentsWidth();
}

static void webkit_web_view_get_preferred_height(GtkWidget* widget, gint* minimum, gint* natural)
{
    WebKitWebView* web_view = WEBKIT_WEB_VIEW(widget);
    Frame* coreFrame = core(webkit_web_view_get_main_frame(web_view));
    if (!coreFrame)
        return;

    FrameView* view = coreFrame->view();
    if (!view)
        return;

    *minimum = *natural = view->contentsHeight();
}
#endif

static void updateChildAllocationFromPendingAllocation(GtkWidget* child, void*)
{
    if (!gtk_widget_get_visible(child))
        return;

    GtkAllocation* allocation = static_cast<GtkAllocation*>(g_object_get_data(G_OBJECT(child), "delayed-allocation"));
    if (!allocation)
        return;

    g_object_set_data(G_OBJECT(child), "delayed-allocation", 0);
    gtk_widget_size_allocate(child, allocation);
    *allocation = IntRect();
}

static void resizeWebViewFromAllocation(WebKitWebView* webView, GtkAllocation* allocation, bool sizeChanged)
{
    Page* page = core(webView);
    IntSize oldSize;
    FrameView* frameView = page->mainFrame()->view();
    if (sizeChanged && frameView) {
        oldSize = frameView->size();
        frameView->resize(allocation->width, allocation->height);
    }

    gtk_container_forall(GTK_CONTAINER(webView), updateChildAllocationFromPendingAllocation, 0);

    if (!sizeChanged)
        return;

    WebKit::ChromeClient* chromeClient = static_cast<WebKit::ChromeClient*>(page->chrome().client());
    chromeClient->widgetSizeChanged(oldSize, IntSize(allocation->width, allocation->height));
    chromeClient->adjustmentWatcher()->updateAdjustmentsFromScrollbars();
}

static void webkit_web_view_size_allocate(GtkWidget* widget, GtkAllocation* allocation)
{
    GtkAllocation oldAllocation;
    gtk_widget_get_allocation(widget, &oldAllocation);
    bool sizeChanged = allocation->width != oldAllocation.width || allocation->height != oldAllocation.height;

    GTK_WIDGET_CLASS(webkit_web_view_parent_class)->size_allocate(widget, allocation);

    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);
    if (sizeChanged && !gtk_widget_get_mapped(widget)) {
        webView->priv->needsResizeOnMap = true;
        return;
    }
    resizeWebViewFromAllocation(webView, allocation, sizeChanged);
}

static void webkitWebViewMap(GtkWidget* widget)
{
    GTK_WIDGET_CLASS(webkit_web_view_parent_class)->map(widget);

    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);
    if (!webView->priv->needsResizeOnMap)
        return;

    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    resizeWebViewFromAllocation(webView, &allocation, true);
    webView->priv->needsResizeOnMap = false;
}

static void webkit_web_view_grab_focus(GtkWidget* widget)
{

    if (gtk_widget_is_sensitive(widget)) {
        WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);
        FocusController* focusController = core(webView)->focusController();

        focusController->setActive(true);

        if (focusController->focusedFrame())
            focusController->setFocused(true);
        else
            focusController->setFocusedFrame(core(webView)->mainFrame());
    }

    return GTK_WIDGET_CLASS(webkit_web_view_parent_class)->grab_focus(widget);
}

static gboolean webkit_web_view_focus_in_event(GtkWidget* widget, GdkEventFocus* event)
{
    // TODO: Improve focus handling as suggested in
    // http://bugs.webkit.org/show_bug.cgi?id=16910
    GtkWidget* toplevel = gtk_widget_get_toplevel(widget);
    if (!widgetIsOnscreenToplevelWindow(toplevel) || !gtk_window_has_toplevel_focus(GTK_WINDOW(toplevel)))
        return GTK_WIDGET_CLASS(webkit_web_view_parent_class)->focus_in_event(widget, event);

    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);
    FocusController* focusController = core(webView)->focusController();

    focusController->setActive(true);
    if (focusController->focusedFrame())
        focusController->setFocused(true);
    else
        focusController->setFocusedFrame(core(webView)->mainFrame());

    if (focusController->focusedFrame()->editor().canEdit())
        webView->priv->imFilter.notifyFocusedIn();
    return GTK_WIDGET_CLASS(webkit_web_view_parent_class)->focus_in_event(widget, event);
}

static gboolean webkit_web_view_focus_out_event(GtkWidget* widget, GdkEventFocus* event)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);

    // We may hit this code while destroying the widget, and we might
    // no longer have a page, then.
    if (Page* page = core(webView)) {
        page->focusController()->setActive(false);
        page->focusController()->setFocused(false);
    }

    webView->priv->imFilter.notifyFocusedOut();
    return GTK_WIDGET_CLASS(webkit_web_view_parent_class)->focus_out_event(widget, event);
}

static void webkit_web_view_realize(GtkWidget* widget)
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
#ifdef GTK_API_VERSION_2
    attributes.colormap = gtk_widget_get_colormap(widget);
#endif
    attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK
                            | GDK_EXPOSURE_MASK
                            | GDK_BUTTON_PRESS_MASK
                            | GDK_BUTTON_RELEASE_MASK
                            | GDK_SCROLL_MASK
#if GTK_CHECK_VERSION(3, 3, 18)
                            | GDK_SMOOTH_SCROLL_MASK
#endif
                            | GDK_POINTER_MOTION_MASK
                            | GDK_KEY_PRESS_MASK
                            | GDK_KEY_RELEASE_MASK
                            | GDK_BUTTON_MOTION_MASK
                            | GDK_BUTTON1_MOTION_MASK
                            | GDK_BUTTON2_MOTION_MASK
                            | GDK_BUTTON3_MOTION_MASK;

    gint attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
#ifdef GTK_API_VERSION_2
    attributes_mask |= GDK_WA_COLORMAP;
#endif
    GdkWindow* window = gdk_window_new(gtk_widget_get_parent_window(widget), &attributes, attributes_mask);

    gtk_widget_set_window(widget, window);
    gdk_window_set_user_data(window, widget);

#ifdef GTK_API_VERSION_2
    gtk_widget_style_attach(widget);
    gtk_style_set_background(gtk_widget_get_style(widget), window, GTK_STATE_NORMAL);
#else
    gtk_style_context_set_background(gtk_widget_get_style_context(widget), window);
#endif
}

#ifdef GTK_API_VERSION_2
static void webkit_web_view_set_scroll_adjustments(WebKitWebView* webView, GtkAdjustment* horizontalAdjustment, GtkAdjustment* verticalAdjustment)
{
    setHorizontalAdjustment(webView, horizontalAdjustment);
    setVerticalAdjustment(webView, verticalAdjustment);
}
#endif

static void webkit_web_view_container_add(GtkContainer* container, GtkWidget* widget)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(container);
    WebKitWebViewPrivate* priv = webView->priv;

    priv->children.add(widget);
    gtk_widget_set_parent(widget, GTK_WIDGET(container));
}

static void webkit_web_view_container_remove(GtkContainer* container, GtkWidget* widget)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(container);
    WebKitWebViewPrivate* priv = webView->priv;

    if (priv->children.contains(widget)) {
        gtk_widget_unparent(widget);
        priv->children.remove(widget);
    }
}

static void webkit_web_view_container_forall(GtkContainer* container, gboolean, GtkCallback callback, gpointer callbackData)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(container);
    WebKitWebViewPrivate* priv = webView->priv;

    HashSet<GtkWidget*> children = priv->children;
    HashSet<GtkWidget*>::const_iterator end = children.end();
    for (HashSet<GtkWidget*>::const_iterator current = children.begin(); current != end; ++current)
        (*callback)(*current, callbackData);
}

static WebKitWebView* webkit_web_view_real_create_web_view(WebKitWebView*, WebKitWebFrame*)
{
    return 0;
}

static gboolean webkit_web_view_real_web_view_ready(WebKitWebView*)
{
    return FALSE;
}

static gboolean webkit_web_view_real_close_web_view(WebKitWebView*)
{
    return FALSE;
}

static WebKitNavigationResponse webkit_web_view_real_navigation_requested(WebKitWebView*, WebKitWebFrame*, WebKitNetworkRequest*)
{
    return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;
}

static void webkit_web_view_real_window_object_cleared(WebKitWebView*, WebKitWebFrame*, JSGlobalContextRef context, JSObjectRef window_object)
{
    notImplemented();
}

static gchar* webkit_web_view_real_choose_file(WebKitWebView*, WebKitWebFrame*, const gchar* old_name)
{
    notImplemented();
    return g_strdup(old_name);
}

typedef enum {
    WEBKIT_SCRIPT_DIALOG_ALERT,
    WEBKIT_SCRIPT_DIALOG_CONFIRM,
    WEBKIT_SCRIPT_DIALOG_PROMPT
 } WebKitScriptDialogType;

static gboolean webkit_web_view_script_dialog(WebKitWebView* webView, WebKitWebFrame* frame, const gchar* message, WebKitScriptDialogType type, const gchar* defaultValue, gchar** value)
{
    GtkMessageType messageType;
    GtkButtonsType buttons;
    gint defaultResponse;
    GtkWidget* window;
    GtkWidget* dialog;
    GtkWidget* entry = 0;
    gboolean didConfirm = FALSE;

    switch (type) {
    case WEBKIT_SCRIPT_DIALOG_ALERT:
        messageType = GTK_MESSAGE_WARNING;
        buttons = GTK_BUTTONS_CLOSE;
        defaultResponse = GTK_RESPONSE_CLOSE;
        break;
    case WEBKIT_SCRIPT_DIALOG_CONFIRM:
        messageType = GTK_MESSAGE_QUESTION;
        buttons = GTK_BUTTONS_OK_CANCEL;
        defaultResponse = GTK_RESPONSE_OK;
        break;
    case WEBKIT_SCRIPT_DIALOG_PROMPT:
        messageType = GTK_MESSAGE_QUESTION;
        buttons = GTK_BUTTONS_OK_CANCEL;
        defaultResponse = GTK_RESPONSE_OK;
        break;
    default:
        g_warning("Unknown value for WebKitScriptDialogType.");
        return FALSE;
    }

    window = gtk_widget_get_toplevel(GTK_WIDGET(webView));
    dialog = gtk_message_dialog_new(widgetIsOnscreenToplevelWindow(window) ? GTK_WINDOW(window) : 0,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    messageType,
                                    buttons,
                                    "%s",
                                    message);
    gchar* title = g_strconcat("JavaScript - ", webkit_web_frame_get_uri(frame), NULL);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    g_free(title);

    if (type == WEBKIT_SCRIPT_DIALOG_PROMPT) {
        entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry), defaultValue);
        gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), entry);
        gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
        gtk_widget_show(entry);
    }

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), defaultResponse);
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    switch (response) {
    case GTK_RESPONSE_OK:
        didConfirm = TRUE;
        if (entry)
            *value = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
        break;
    case GTK_RESPONSE_CANCEL:
        didConfirm = FALSE;
        break;

    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    return didConfirm;
}

static gboolean webkit_web_view_real_script_alert(WebKitWebView* webView, WebKitWebFrame* frame, const gchar* message)
{
    webkit_web_view_script_dialog(webView, frame, message, WEBKIT_SCRIPT_DIALOG_ALERT, 0, 0);
    return TRUE;
}

static gboolean webkit_web_view_real_script_confirm(WebKitWebView* webView, WebKitWebFrame* frame, const gchar* message, gboolean* didConfirm)
{
    *didConfirm = webkit_web_view_script_dialog(webView, frame, message, WEBKIT_SCRIPT_DIALOG_CONFIRM, 0, 0);
    return TRUE;
}

static gboolean webkit_web_view_real_script_prompt(WebKitWebView* webView, WebKitWebFrame* frame, const gchar* message, const gchar* defaultValue, gchar** value)
{
    if (!webkit_web_view_script_dialog(webView, frame, message, WEBKIT_SCRIPT_DIALOG_PROMPT, defaultValue, value))
        *value = NULL;
    return TRUE;
}

static gboolean webkit_web_view_real_console_message(WebKitWebView* webView, const gchar* message, unsigned int line, const gchar* sourceId)
{
    g_message("console message: %s @%d: %s\n", sourceId, line, message);
    return TRUE;
}

static void webkit_web_view_real_select_all(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor().command("SelectAll").execute();
}

static void webkit_web_view_real_cut_clipboard(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor().command("Cut").execute();
}

static void webkit_web_view_real_copy_clipboard(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor().command("Copy").execute();
}

static void webkit_web_view_real_undo(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor().command("Undo").execute();
}

static void webkit_web_view_real_redo(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor().command("Redo").execute();
}

static gboolean webkit_web_view_real_move_cursor (WebKitWebView* webView, GtkMovementStep step, gint count)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW (webView), FALSE);
    g_return_val_if_fail(step == GTK_MOVEMENT_VISUAL_POSITIONS ||
                         step == GTK_MOVEMENT_DISPLAY_LINES ||
                         step == GTK_MOVEMENT_PAGES ||
                         step == GTK_MOVEMENT_BUFFER_ENDS, FALSE);
    g_return_val_if_fail(count == 1 || count == -1, FALSE);

    ScrollDirection direction;
    ScrollGranularity granularity;

    switch (step) {
    case GTK_MOVEMENT_DISPLAY_LINES:
        granularity = ScrollByLine;
        if (count == 1)
            direction = ScrollDown;
        else
            direction = ScrollUp;
        break;
    case GTK_MOVEMENT_VISUAL_POSITIONS:
        granularity = ScrollByLine;
        if (count == 1)
            direction = ScrollRight;
        else
            direction = ScrollLeft;
        break;
    case GTK_MOVEMENT_PAGES:
        granularity = ScrollByPage;
        if (count == 1)
            direction = ScrollDown;
        else
            direction = ScrollUp;
        break;
    case GTK_MOVEMENT_BUFFER_ENDS:
        granularity = ScrollByDocument;
        if (count == 1)
            direction = ScrollDown;
        else
            direction = ScrollUp;
        break;
    default:
        g_assert_not_reached();
        return false;
    }

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    if (!frame->eventHandler()->scrollOverflow(direction, granularity))
        frame->view()->scroll(direction, granularity);

    return true;
}

static void webkit_web_view_real_paste_clipboard(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor().command("Paste").execute();
}

static gboolean webkit_web_view_real_should_allow_editing_action(WebKitWebView*)
{
    return TRUE;
}

static gboolean webkit_web_view_real_entering_fullscreen(WebKitWebView* webView)
{
    return FALSE;
}

static gboolean webkit_web_view_real_leaving_fullscreen(WebKitWebView* webView)
{
    return FALSE;
}

static void fileChooserDialogResponseCallback(GtkDialog* dialog, gint responseID, WebKitFileChooserRequest* request)
{
    GRefPtr<WebKitFileChooserRequest> adoptedRequest = adoptGRef(request);
    if (responseID == GTK_RESPONSE_ACCEPT) {
        GOwnPtr<GSList> filesList(gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog)));
        GRefPtr<GPtrArray> filesArray = adoptGRef(g_ptr_array_new());
        for (GSList* file = filesList.get(); file; file = g_slist_next(file))
            g_ptr_array_add(filesArray.get(), file->data);
        g_ptr_array_add(filesArray.get(), 0);
        webkit_file_chooser_request_select_files(adoptedRequest.get(), reinterpret_cast<const gchar* const*>(filesArray->pdata));
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static gboolean webkitWebViewRealRunFileChooser(WebKitWebView* webView, WebKitFileChooserRequest* request)
{
    GtkWidget* toplevel = gtk_widget_get_toplevel(GTK_WIDGET(webView));
    if (!widgetIsOnscreenToplevelWindow(toplevel))
        toplevel = 0;

    gboolean allowsMultipleSelection = webkit_file_chooser_request_get_select_multiple(request);
    GtkWidget* dialog = gtk_file_chooser_dialog_new(allowsMultipleSelection ? _("Select Files") : _("Select File"),
                                                    toplevel ? GTK_WINDOW(toplevel) : 0,
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                    NULL);

    if (GtkFileFilter* filter = webkit_file_chooser_request_get_mime_types_filter(request))
        gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), allowsMultipleSelection);

    if (const gchar* const* selectedFiles = webkit_file_chooser_request_get_selected_files(request))
        gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(dialog), selectedFiles[0]);

    g_signal_connect(dialog, "response", G_CALLBACK(fileChooserDialogResponseCallback), g_object_ref(request));
    gtk_widget_show(dialog);

    return TRUE;
}

static void webkit_web_view_dispose(GObject* object)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(object);
    WebKitWebViewPrivate* priv = webView->priv;

    priv->disposing = TRUE;

    // Make sure GtkAdjustmentWatcher won't be reacting to adjustment changes after the
    // WebView is destroyed.
    setHorizontalAdjustment(webView, 0);
    setVerticalAdjustment(webView, 0);

    // These smart pointers are cleared manually, because some cleanup operations are
    // very sensitive to their value. We may crash if these are done in the wrong order.
    priv->backForwardList.clear();

    if (priv->corePage) {
        webkit_web_view_stop_loading(WEBKIT_WEB_VIEW(object));
        core(priv->mainFrame)->loader()->detachFromParent();
        delete priv->corePage;
        priv->corePage = 0;
    }

    if (priv->webSettings) {
        g_signal_handlers_disconnect_by_func(priv->webSettings.get(), reinterpret_cast<void*>(webkit_web_view_settings_notify), webView);
        priv->webSettings.clear();
    }

    if (priv->currentMenu) {
        gtk_widget_destroy(GTK_WIDGET(priv->currentMenu));
        priv->currentMenu = 0;
    }

    priv->webInspector.clear();
    priv->viewportAttributes.clear();
    priv->webWindowFeatures.clear();
    priv->mainResource.clear();
    priv->subResources.clear();

    G_OBJECT_CLASS(webkit_web_view_parent_class)->dispose(object);
}

static void webkit_web_view_finalize(GObject* object)
{
    // We need to manually call the destructor here, since this object's memory is managed
    // by GLib. This calls all C++ members' destructors and prevents memory leaks.
    WEBKIT_WEB_VIEW(object)->priv->~WebKitWebViewPrivate();
    G_OBJECT_CLASS(webkit_web_view_parent_class)->finalize(object);
}

static gboolean webkit_signal_accumulator_object_handled(GSignalInvocationHint* ihint, GValue* returnAccu, const GValue* handlerReturn, gpointer dummy)
{
    gpointer newWebView = g_value_get_object(handlerReturn);
    g_value_set_object(returnAccu, newWebView);

    // Continue if we don't have a newWebView
    return !newWebView;
}

static gboolean webkit_navigation_request_handled(GSignalInvocationHint* ihint, GValue* returnAccu, const GValue* handlerReturn, gpointer dummy)
{
    WebKitNavigationResponse navigationResponse = (WebKitNavigationResponse)g_value_get_enum(handlerReturn);
    g_value_set_enum(returnAccu, navigationResponse);

    if (navigationResponse != WEBKIT_NAVIGATION_RESPONSE_ACCEPT)
        return FALSE;

    return TRUE;
}

static AtkObject* webkit_web_view_get_accessible(GtkWidget* widget)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);
    if (!core(webView))
        return 0;

    if (!AXObjectCache::accessibilityEnabled())
        AXObjectCache::enableAccessibility();

    Frame* coreFrame = core(webView)->mainFrame();
    if (!coreFrame)
        return 0;

    Document* doc = coreFrame->document();
    if (!doc)
        return 0;

    AccessibilityObject* rootAccessible = doc->axObjectCache()->rootObject();
    if (!rootAccessible)
        return 0;

    AtkObject* axRoot = rootAccessible->wrapper();
    if (!axRoot || !ATK_IS_OBJECT(axRoot))
        return 0;

    // Ensure the parent is set to make top-down and bottom-up navigation work.
    GtkWidget* parentWidget = gtk_widget_get_parent(widget);
    AtkObject* axParent = parentWidget ? gtk_widget_get_accessible(parentWidget) : 0;
    if (axParent)
        atk_object_set_parent(axRoot, axParent);

    return axRoot;
}

static double screenDPI(GdkScreen* screen)
{
    // gdk_screen_get_resolution() returns -1 when no DPI is set.
    double dpi = gdk_screen_get_resolution(screen);
    if (dpi != -1)
        return dpi;

    static const double kMillimetresPerInch = 25.4;
    double diagonalSizeInPixels = hypot(gdk_screen_get_width(screen), gdk_screen_get_height(screen));
    double diagonalSizeInInches = hypot(gdk_screen_get_width_mm(screen), gdk_screen_get_height_mm(screen)) / kMillimetresPerInch;
    return diagonalSizeInPixels / diagonalSizeInInches;
}

static gdouble webViewGetDPI(WebKitWebView* webView)
{
    if (webView->priv->webSettings->priv->enforce96DPI)
        return 96;

    static const double defaultDPI = 96;
    GdkScreen* screen = gtk_widget_has_screen(GTK_WIDGET(webView)) ? gtk_widget_get_screen(GTK_WIDGET(webView)) : gdk_screen_get_default();
    return screen ? screenDPI(screen) : defaultDPI;
}

static inline gint webViewConvertFontSizeToPixels(WebKitWebView* webView, double fontSize)
{
    return fontSize / 72.0 * webViewGetDPI(webView);
}

static void webkit_web_view_screen_changed(GtkWidget* widget, GdkScreen* previousScreen)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);
    WebKitWebViewPrivate* priv = webView->priv;

    if (priv->disposing)
        return;

    WebKitWebSettings* webSettings = priv->webSettings.get();
    Settings* settings = core(webView)->settings();
    guint defaultFontSize, defaultMonospaceFontSize, minimumFontSize, minimumLogicalFontSize;

    g_object_get(webSettings,
                 "default-font-size", &defaultFontSize,
                 "default-monospace-font-size", &defaultMonospaceFontSize,
                 "minimum-font-size", &minimumFontSize,
                 "minimum-logical-font-size", &minimumLogicalFontSize,
                 NULL);

    settings->setDefaultFontSize(webViewConvertFontSizeToPixels(webView, defaultFontSize));
    settings->setDefaultFixedFontSize(webViewConvertFontSizeToPixels(webView, defaultMonospaceFontSize));
    settings->setMinimumFontSize(webViewConvertFontSizeToPixels(webView, minimumFontSize));
    settings->setMinimumLogicalFontSize(webViewConvertFontSizeToPixels(webView, minimumLogicalFontSize));
}

#if ENABLE(DRAG_SUPPORT)
static void webkit_web_view_drag_end(GtkWidget* widget, GdkDragContext* context)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);
    if (!webView->priv->dragAndDropHelper.handleDragEnd(context))
        return;

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    // Synthesize a button release event to send with the drag end action.
    GOwnPtr<GdkEvent> event(gdk_event_new(GDK_BUTTON_RELEASE));
    int x, y, xRoot, yRoot;
    GdkModifierType modifiers = static_cast<GdkModifierType>(0);
#ifdef GTK_API_VERSION_2
    GdkDisplay* display = gdk_display_get_default();
    gdk_display_get_pointer(display, 0, &xRoot, &yRoot, &modifiers);
    event->button.window = gdk_display_get_window_at_pointer(display, &x, &y);
#else
    GdkDevice* device = gdk_drag_context_get_device(context);
    event->button.window = gdk_device_get_window_at_position(device, &x, &y);
    gdk_device_get_position(device, 0, &xRoot, &yRoot);
#endif

    if (event->button.window)
        g_object_ref(event->button.window);
    event->button.x = x;
    event->button.y = y;
    event->button.x_root = xRoot;
    event->button.y_root = yRoot;
    event->button.state = modifiers;

    PlatformMouseEvent platformEvent(&event->button);
    frame->eventHandler()->dragSourceEndedAt(platformEvent, gdkDragActionToDragOperation(gdk_drag_context_get_selected_action(context)));
}

static void webkit_web_view_drag_data_get(GtkWidget* widget, GdkDragContext* context, GtkSelectionData* selectionData, guint info, guint)
{
    WEBKIT_WEB_VIEW(widget)->priv->dragAndDropHelper.handleGetDragData(context, selectionData, info);
}

static void dragExitedCallback(GtkWidget* widget, DragData* dragData, bool dropHappened)
{
    // Don't call dragExited if we have just received a drag-drop signal. This
    // happens in the case of a successful drop onto the view.
    if (!dropHappened)
        core(WEBKIT_WEB_VIEW(widget))->dragController()->dragExited(dragData);
    core(WEBKIT_WEB_VIEW(widget))->dragController()->dragEnded();
}

static void webkit_web_view_drag_leave(GtkWidget* widget, GdkDragContext* context, guint time)
{
    WEBKIT_WEB_VIEW(widget)->priv->dragAndDropHelper.handleDragLeave(context, dragExitedCallback);
}

static gboolean webkit_web_view_drag_motion(GtkWidget* widget, GdkDragContext* context, gint x, gint y, guint time)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);
    IntPoint position(x, y);
    DataObjectGtk* dataObject = webView->priv->dragAndDropHelper.handleDragMotion(context, position, time);
    if (!dataObject)
        return TRUE;

    DragData dragData(dataObject, position, convertWidgetPointToScreenPoint(widget, position), gdkDragActionToDragOperation(gdk_drag_context_get_actions(context)));
    DragOperation operation = core(webView)->dragController()->dragUpdated(&dragData).operation;
    gdk_drag_status(context, dragOperationToSingleGdkDragAction(operation), time);
    return TRUE;
}

static void webkit_web_view_drag_data_received(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* selectionData, guint info, guint time)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);
    IntPoint position;
    DataObjectGtk* dataObject = webView->priv->dragAndDropHelper.handleDragDataReceived(context, selectionData, info, position);
    if (!dataObject)
        return;

    DragData dragData(dataObject, position, convertWidgetPointToScreenPoint(widget, position), gdkDragActionToDragOperation(gdk_drag_context_get_actions(context)));
    DragOperation operation = core(webView)->dragController()->dragEntered(&dragData).operation;
    gdk_drag_status(context, dragOperationToSingleGdkDragAction(operation), time);
}

static gboolean webkit_web_view_drag_drop(GtkWidget* widget, GdkDragContext* context, gint x, gint y, guint time)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);
    DataObjectGtk* dataObject = webView->priv->dragAndDropHelper.handleDragDrop(context);
    if (!dataObject)
        return FALSE;

    IntPoint position(x, y);
    DragData dragData(dataObject, position, convertWidgetPointToScreenPoint(widget, position), gdkDragActionToDragOperation(gdk_drag_context_get_actions(context)));
    core(webView)->dragController()->performDrag(&dragData);
    gtk_drag_finish(context, TRUE, FALSE, time);
    return TRUE;
}
#endif // ENABLE(DRAG_SUPPORT)

static gboolean webkit_web_view_query_tooltip(GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip)
{
    WebKitWebViewPrivate* priv = WEBKIT_WEB_VIEW(widget)->priv;

    if (keyboard_mode) {
        WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);

        // Get the title of the current focused element.
        Frame* coreFrame = core(webView)->focusController()->focusedOrMainFrame();
        if (!coreFrame)
            return FALSE;

        Node* node = getFocusedNode(coreFrame);
        if (!node)
            return FALSE;

        for (Node* titleNode = node; titleNode; titleNode = titleNode->parentNode()) {
            if (titleNode->isElementNode()) {
                String title = toElement(titleNode)->title();
                if (!title.isEmpty()) {
                    if (FrameView* view = coreFrame->view()) {
                        GdkRectangle area = view->contentsToWindow(node->pixelSnappedBoundingBox());
                        gtk_tooltip_set_tip_area(tooltip, &area);
                    }
                    gtk_tooltip_set_text(tooltip, title.utf8().data());

                    return TRUE;
                }
            }
        }

        return FALSE;
    }

    if (priv->tooltipText.length() > 0) {
        if (!keyboard_mode) {
            if (!priv->tooltipArea.isEmpty()) {
                GdkRectangle area = priv->tooltipArea;
                gtk_tooltip_set_tip_area(tooltip, &area);
            } else
                gtk_tooltip_set_tip_area(tooltip, 0);
        }
        gtk_tooltip_set_text(tooltip, priv->tooltipText.data());
        return TRUE;
    }

    return FALSE;
}

static gboolean webkit_web_view_show_help(GtkWidget* widget, GtkWidgetHelpType help_type)
{
    if (help_type == GTK_WIDGET_HELP_TOOLTIP)
        gtk_widget_set_has_tooltip(widget, TRUE);

    return GTK_WIDGET_CLASS(webkit_web_view_parent_class)->show_help(widget, help_type);
}

static void webkit_web_view_class_init(WebKitWebViewClass* webViewClass)
{
    GtkBindingSet* binding_set;

    webkitInit();

    /*
     * Signals
     */

    /**
     * WebKitWebView::create-web-view:
     * @web_view: the object on which the signal is emitted
     * @frame: the #WebKitWebFrame
     *
     * Emitted when the creation of a new window is requested.
     * If this signal is handled the signal handler should return the
     * newly created #WebKitWebView.
     *
     * The new #WebKitWebView should not be displayed to the user
     * until the #WebKitWebView::web-view-ready signal is emitted.
     *
     * The signal handlers should not try to deal with the reference count for
     * the new #WebKitWebView. The widget to which the widget is added will
     * handle that.
     *
     * Return value: (transfer full): a newly allocated #WebKitWebView, or %NULL
     *
     * Since: 1.0.3
     */
    webkit_web_view_signals[CREATE_WEB_VIEW] = g_signal_new("create-web-view",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (WebKitWebViewClass, create_web_view),
        webkit_signal_accumulator_object_handled,
        NULL,
        webkit_marshal_OBJECT__OBJECT,
        WEBKIT_TYPE_WEB_VIEW , 1,
        WEBKIT_TYPE_WEB_FRAME);

    /**
     * WebKitWebView::web-view-ready:
     * @web_view: the object on which the signal is emitted
     *
     * Emitted after #WebKitWebView::create-web-view when the new #WebKitWebView
     * should be displayed to the user. When this signal is emitted
     * all the information about how the window should look, including
     * size, position, whether the location, status and scroll bars
     * should be displayed, is already set on the
     * #WebKitWebWindowFeatures object contained by the #WebKitWebView.
     *
     * Notice that some of that information may change during the life
     * time of the window, so you may want to connect to the ::notify
     * signal of the #WebKitWebWindowFeatures object to handle those.
     *
     * Return value: %TRUE to stop handlers from being invoked for the event or
     * %FALSE to propagate the event furter
     *
     * Since: 1.0.3
     */
    webkit_web_view_signals[WEB_VIEW_READY] = g_signal_new("web-view-ready",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (WebKitWebViewClass, web_view_ready),
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__VOID,
        G_TYPE_BOOLEAN, 0);

    /**
     * WebKitWebView::close-web-view:
     * @web_view: the object on which the signal is emitted
     *
     * Emitted when closing a #WebKitWebView is requested. This occurs when a
     * call is made from JavaScript's window.close function. The default
     * signal handler does not do anything. It is the owner's responsibility
     * to hide or delete the web view, if necessary.
     *
     * Return value: %TRUE to stop handlers from being invoked for the event or
     * %FALSE to propagate the event furter
     *
     * Since: 1.1.11
     */
    webkit_web_view_signals[CLOSE_WEB_VIEW] = g_signal_new("close-web-view",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (WebKitWebViewClass, close_web_view),
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__VOID,
        G_TYPE_BOOLEAN, 0);

    /**
     * WebKitWebView::navigation-requested:
     * @web_view: the object on which the signal is emitted
     * @frame: the #WebKitWebFrame that required the navigation
     * @request: a #WebKitNetworkRequest
     *
     * Emitted when @frame requests a navigation to another page.
     *
     * Return value: a #WebKitNavigationResponse
     *
     * Deprecated: Use WebKitWebView::navigation-policy-decision-requested
     * instead
     */
    webkit_web_view_signals[NAVIGATION_REQUESTED] = g_signal_new("navigation-requested",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (WebKitWebViewClass, navigation_requested),
        webkit_navigation_request_handled,
        NULL,
        webkit_marshal_ENUM__OBJECT_OBJECT,
        WEBKIT_TYPE_NAVIGATION_RESPONSE, 2,
        WEBKIT_TYPE_WEB_FRAME,
        WEBKIT_TYPE_NETWORK_REQUEST);

    /**
     * WebKitWebView::new-window-policy-decision-requested:
     * @web_view: the object on which the signal is emitted
     * @frame: the #WebKitWebFrame that required the navigation
     * @request: a #WebKitNetworkRequest
     * @navigation_action: a #WebKitWebNavigationAction
     * @policy_decision: a #WebKitWebPolicyDecision
     *
     * Emitted when @frame requests opening a new window. With this
     * signal the browser can use the context of the request to decide
     * about the new window. If the request is not handled the default
     * behavior is to allow opening the new window to load the URI,
     * which will cause a create-web-view signal emission where the
     * browser handles the new window action but without information
     * of the context that caused the navigation. The following
     * navigation-policy-decision-requested emissions will load the
     * page after the creation of the new window just with the
     * information of this new navigation context, without any
     * information about the action that made this new window to be
     * opened.
     *
     * Notice that if you return TRUE, meaning that you handled the
     * signal, you are expected to have decided what to do, by calling
     * webkit_web_policy_decision_ignore(),
     * webkit_web_policy_decision_use(), or
     * webkit_web_policy_decision_download() on the @policy_decision
     * object.
     *
     * Return value: %TRUE if a decision was made, %FALSE to have the
     * default behavior apply
     *
     * Since: 1.1.4
     */
    webkit_web_view_signals[NEW_WINDOW_POLICY_DECISION_REQUESTED] = g_signal_new("new-window-policy-decision-requested",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__OBJECT_OBJECT_OBJECT_OBJECT,
        G_TYPE_BOOLEAN, 4,
        WEBKIT_TYPE_WEB_FRAME,
        WEBKIT_TYPE_NETWORK_REQUEST,
        WEBKIT_TYPE_WEB_NAVIGATION_ACTION,
        WEBKIT_TYPE_WEB_POLICY_DECISION);

    /**
     * WebKitWebView::navigation-policy-decision-requested:
     * @web_view: the object on which the signal is emitted
     * @frame: the #WebKitWebFrame that required the navigation
     * @request: a #WebKitNetworkRequest
     * @navigation_action: a #WebKitWebNavigationAction
     * @policy_decision: a #WebKitWebPolicyDecision
     *
     * Emitted when @frame requests a navigation to another page.
     * If this signal is not handled, the default behavior is to allow the
     * navigation.
     *
     * Notice that if you return TRUE, meaning that you handled the
     * signal, you are expected to have decided what to do, by calling
     * webkit_web_policy_decision_ignore(),
     * webkit_web_policy_decision_use(), or
     * webkit_web_policy_decision_download() on the @policy_decision
     * object.
     *
     * Return value: %TRUE if a decision was made, %FALSE to have the
     * default behavior apply
     *
     * Since: 1.0.3
     */
    webkit_web_view_signals[NAVIGATION_POLICY_DECISION_REQUESTED] = g_signal_new("navigation-policy-decision-requested",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__OBJECT_OBJECT_OBJECT_OBJECT,
        G_TYPE_BOOLEAN, 4,
        WEBKIT_TYPE_WEB_FRAME,
        WEBKIT_TYPE_NETWORK_REQUEST,
        WEBKIT_TYPE_WEB_NAVIGATION_ACTION,
        WEBKIT_TYPE_WEB_POLICY_DECISION);

    /**
     * WebKitWebView::mime-type-policy-decision-requested:
     * @web_view: the object on which the signal is emitted
     * @frame: the #WebKitWebFrame that required the policy decision
     * @request: a WebKitNetworkRequest
     * @mimetype: the MIME type attempted to load
     * @policy_decision: a #WebKitWebPolicyDecision
     *
     * Decide whether or not to display the given MIME type.  If this
     * signal is not handled, the default behavior is to show the
     * content of the requested URI if WebKit can show this MIME
     * type and the content disposition is not a download; if WebKit
     * is not able to show the MIME type nothing happens.
     *
     * Notice that if you return TRUE, meaning that you handled the
     * signal, you are expected to be aware of the "Content-Disposition"
     * header. A value of "attachment" usually indicates a download
     * regardless of the MIME type, see also
     * soup_message_headers_get_content_disposition(). And you must call
     * webkit_web_policy_decision_ignore(),
     * webkit_web_policy_decision_use(), or
     * webkit_web_policy_decision_download() on the @policy_decision
     * object.
     *
     * Return value: %TRUE if a decision was made, %FALSE to have the
     * default behavior apply
     *
     * Since: 1.0.3
     */
    webkit_web_view_signals[MIME_TYPE_POLICY_DECISION_REQUESTED] = g_signal_new("mime-type-policy-decision-requested",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__OBJECT_OBJECT_STRING_OBJECT,
        G_TYPE_BOOLEAN, 4,
        WEBKIT_TYPE_WEB_FRAME,
        WEBKIT_TYPE_NETWORK_REQUEST,
        G_TYPE_STRING,
        WEBKIT_TYPE_WEB_POLICY_DECISION);

    /**
     * WebKitWebView::window-object-cleared:
     * @web_view: the object on which the signal is emitted
     * @frame: the #WebKitWebFrame to which @window_object belongs
     * @context: the #JSGlobalContextRef holding the global object and other
     * execution state; equivalent to the return value of
     * webkit_web_frame_get_global_context(@frame)
     * @window_object: the #JSObjectRef representing the frame's JavaScript
     * window object
     *
     * Emitted when the JavaScript window object in a #WebKitWebFrame has been
     * cleared in preparation for a new load. This is the preferred place to
     * set custom properties on the window object using the JavaScriptCore API.
     */
    webkit_web_view_signals[WINDOW_OBJECT_CLEARED] = g_signal_new("window-object-cleared",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (WebKitWebViewClass, window_object_cleared),
        NULL,
        NULL,
        webkit_marshal_VOID__OBJECT_POINTER_POINTER,
        G_TYPE_NONE, 3,
        WEBKIT_TYPE_WEB_FRAME,
        G_TYPE_POINTER,
        G_TYPE_POINTER);

    /**
     * WebKitWebView::download-requested:
     * @web_view: the object on which the signal is emitted
     * @download: a #WebKitDownload object that lets you control the
     * download process
     *
     * A new Download is being requested. By default, if the signal is
     * not handled, the download is cancelled. If you handle the download
     * and call webkit_download_set_destination_uri(), it will be
     * started for you. If you need to set the destination asynchronously
     * you are responsible for starting or cancelling it yourself.
     *
     * If you intend to handle downloads yourself rather than using
     * the #WebKitDownload helper object you must handle this signal,
     * and return %FALSE.
     *
     * Also, keep in mind that the default policy for WebKitGTK+ is to
     * ignore files with a MIME type that it does not know how to
     * handle, which means this signal won't be emitted in the default
     * setup. One way to trigger downloads is to connect to
     * WebKitWebView::mime-type-policy-decision-requested and call
     * webkit_web_policy_decision_download() on the
     * #WebKitWebPolicyDecision in the parameter list for the kind of
     * files you want your application to download (a common solution
     * is to download anything that WebKit can't handle, which you can
     * figure out by using webkit_web_view_can_show_mime_type()).
     *
     * Return value: TRUE if the download should be performed, %FALSE to
     * cancel it
     *
     * Since: 1.1.2
     */
    webkit_web_view_signals[DOWNLOAD_REQUESTED] = g_signal_new("download-requested",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__OBJECT,
        G_TYPE_BOOLEAN, 1,
        G_TYPE_OBJECT);

    /**
     * WebKitWebView::load-started:
     * @web_view: the object on which the signal is emitted
     * @frame: the frame going to do the load
     *
     * When a #WebKitWebFrame begins to load this signal is emitted.
     *
     * Deprecated: Use the "load-status" property instead.
     */
    webkit_web_view_signals[LOAD_STARTED] = g_signal_new("load-started",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        WEBKIT_TYPE_WEB_FRAME);

    /**
     * WebKitWebView::load-committed:
     * @web_view: the object on which the signal is emitted
     * @frame: the main frame that received the first data
     *
     * When a #WebKitWebFrame loaded the first data this signal is emitted.
     *
     * Deprecated: Use the "load-status" property instead.
     */
    webkit_web_view_signals[LOAD_COMMITTED] = g_signal_new("load-committed",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        WEBKIT_TYPE_WEB_FRAME);


    /**
     * WebKitWebView::load-progress-changed:
     * @web_view: the #WebKitWebView
     * @progress: the global progress
     *
     * Deprecated: Use the "progress" property instead.
     */
    webkit_web_view_signals[LOAD_PROGRESS_CHANGED] = g_signal_new("load-progress-changed",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        g_cclosure_marshal_VOID__INT,
        G_TYPE_NONE, 1,
        G_TYPE_INT);

    /**
     * WebKitWebView::load-error:
     * @web_view: the object on which the signal is emitted
     * @web_frame: the #WebKitWebFrame
     * @uri: the URI that triggered the error
     * @web_error: the #GError that was triggered
     *
     * An error occurred while loading. By default, if the signal is not
     * handled, the @web_view will display a stock error page. You need to
     * handle the signal if you want to provide your own error page.
     *
     * Since: 1.1.6
     *
     * Return value: %TRUE to stop other handlers from being invoked for the
     * event. %FALSE to propagate the event further.
     */
    webkit_web_view_signals[LOAD_ERROR] = g_signal_new("load-error",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST),
        0,
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__OBJECT_STRING_BOXED,
        G_TYPE_BOOLEAN, 3,
        WEBKIT_TYPE_WEB_FRAME,
        G_TYPE_STRING,
        G_TYPE_ERROR);

    /**
     * WebKitWebView::load-finished:
     * @web_view: the #WebKitWebView
     * @frame: the #WebKitWebFrame
     *
     * Deprecated: Use the "load-status" property instead.
     */
    webkit_web_view_signals[LOAD_FINISHED] = g_signal_new("load-finished",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        WEBKIT_TYPE_WEB_FRAME);

    /**
     * WebKitWebView::onload-event:
     * @web_view: the object on which the signal is emitted
     * @frame: the frame
     *
     * When a #WebKitWebFrame receives an onload event this signal is emitted.
     */
    webkit_web_view_signals[ONLOAD_EVENT] = g_signal_new("onload-event",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        WEBKIT_TYPE_WEB_FRAME);

    /**
     * WebKitWebView::title-changed:
     * @web_view: the object on which the signal is emitted
     * @frame: the main frame
     * @title: the new title
     *
     * When a #WebKitWebFrame changes the document title this signal is emitted.
     *
     * Deprecated: 1.1.4: Use "notify::title" instead.
     */
    webkit_web_view_signals[TITLE_CHANGED] = g_signal_new("title-changed",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        webkit_marshal_VOID__OBJECT_STRING,
        G_TYPE_NONE, 2,
        WEBKIT_TYPE_WEB_FRAME,
        G_TYPE_STRING);

    /**
     * WebKitWebView::hovering-over-link:
     * @web_view: the object on which the signal is emitted
     * @title: the link's title
     * @uri: the URI the link points to
     *
     * When the cursor is over a link, this signal is emitted.
     */
    webkit_web_view_signals[HOVERING_OVER_LINK] = g_signal_new("hovering-over-link",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        webkit_marshal_VOID__STRING_STRING,
        G_TYPE_NONE, 2,
        G_TYPE_STRING,
        G_TYPE_STRING);

    /**
     * WebKitWebView::populate-popup:
     * @web_view: the object on which the signal is emitted
     * @menu: the context menu
     *
     * When a context menu is about to be displayed this signal is emitted.
     *
     * Add menu items to #menu to extend the context menu.
     *
     * Deprecated: 1.10: Use #WebKitWebView::context-menu signal instead.
     */
    webkit_web_view_signals[POPULATE_POPUP] = g_signal_new("populate-popup",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        GTK_TYPE_MENU);

    /**
     * WebKitWebView::print-requested:
     * @web_view: the object in which the signal is emitted
     * @web_frame: the frame that is requesting to be printed
     *
     * Emitted when printing is requested by the frame, usually
     * because of a javascript call. When handling this signal you
     * should call webkit_web_frame_print_full() or
     * webkit_web_frame_print() to do the actual printing.
     *
     * The default handler will present a print dialog and carry a
     * print operation. Notice that this means that if you intend to
     * ignore a print request you must connect to this signal, and
     * return %TRUE.
     *
     * Return value: %TRUE if the print request has been handled, %FALSE if
     * the default handler should run
     *
     * Since: 1.1.5
     */
    webkit_web_view_signals[PRINT_REQUESTED] = g_signal_new("print-requested",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__OBJECT,
        G_TYPE_BOOLEAN, 1,
        WEBKIT_TYPE_WEB_FRAME);

    webkit_web_view_signals[STATUS_BAR_TEXT_CHANGED] = g_signal_new("status-bar-text-changed",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1,
        G_TYPE_STRING);

    /**
     * WebKitWebView::icon-loaded:
     * @web_view: the object on which the signal is emitted
     * @icon_uri: the URI for the icon
     *
     * This signal is emitted when the main frame has got a favicon.
     * See WebKitIconDatabase::icon-loaded if you want to keep track of
     * icons for child frames.
     *
     * Since: 1.1.18
     */
    webkit_web_view_signals[ICON_LOADED] = g_signal_new("icon-loaded",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1,
        G_TYPE_STRING);

    /**
     * WebKitWebView::console-message:
     * @web_view: the object on which the signal is emitted
     * @message: the message text
     * @line: the line where the error occured
     * @source_id: the source id
     *
     * A JavaScript console message was created.
     *
     * Return value: %TRUE to stop other handlers from being invoked for the
     * event. %FALSE to propagate the event further.
     */
    webkit_web_view_signals[CONSOLE_MESSAGE] = g_signal_new("console-message",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(WebKitWebViewClass, console_message),
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__STRING_INT_STRING,
        G_TYPE_BOOLEAN, 3,
        G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);

    /**
     * WebKitWebView::script-alert:
     * @web_view: the object on which the signal is emitted
     * @frame: the relevant frame
     * @message: the message text
     *
     * A JavaScript alert dialog was created.
     *
     * Return value: %TRUE to stop other handlers from being invoked for the
     * event. %FALSE to propagate the event further.
     */
    webkit_web_view_signals[SCRIPT_ALERT] = g_signal_new("script-alert",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(WebKitWebViewClass, script_alert),
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__OBJECT_STRING,
        G_TYPE_BOOLEAN, 2,
        WEBKIT_TYPE_WEB_FRAME, G_TYPE_STRING);

    /**
     * WebKitWebView::script-confirm:
     * @web_view: the object on which the signal is emitted
     * @frame: the relevant frame
     * @message: the message text
     * @confirmed: a pointer to a #gboolean where the callback should store
     * whether the user confirmed the dialog, when handling this signal
     *
     * A JavaScript confirm dialog was created, providing Yes and No buttons.
     *
     * Return value: %TRUE to stop other handlers from being invoked for the
     * event. %FALSE to propagate the event further.
     */
    webkit_web_view_signals[SCRIPT_CONFIRM] = g_signal_new("script-confirm",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(WebKitWebViewClass, script_confirm),
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__OBJECT_STRING_POINTER,
        G_TYPE_BOOLEAN, 3,
        WEBKIT_TYPE_WEB_FRAME, G_TYPE_STRING, G_TYPE_POINTER);

    /**
     * WebKitWebView::script-prompt:
     * @web_view: the object on which the signal is emitted
     * @frame: the relevant frame
     * @message: the message text
     * @default: the default value
     * @text: To be filled with the return value or NULL if the dialog was cancelled.
     *
     * A JavaScript prompt dialog was created, providing an entry to input text.
     *
     * Return value: %TRUE to stop other handlers from being invoked for the
     * event. %FALSE to propagate the event further.
     */
    webkit_web_view_signals[SCRIPT_PROMPT] = g_signal_new("script-prompt",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(WebKitWebViewClass, script_prompt),
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__OBJECT_STRING_STRING_STRING,
        G_TYPE_BOOLEAN, 4,
        WEBKIT_TYPE_WEB_FRAME, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

    /**
     * WebKitWebView::select-all:
     * @web_view: the object which received the signal
     *
     * The #WebKitWebView::select-all signal is a keybinding signal which gets emitted to
     * select the complete contents of the text view.
     *
     * The default bindings for this signal is Ctrl-a.
     */
    webkit_web_view_signals[::SELECT_ALL] = g_signal_new("select-all",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET(WebKitWebViewClass, select_all),
        NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    /**
     * WebKitWebView::cut-clipboard:
     * @web_view: the object which received the signal
     *
     * The #WebKitWebView::cut-clipboard signal is a keybinding signal which gets emitted to
     * cut the selection to the clipboard.
     *
     * The default bindings for this signal are Ctrl-x and Shift-Delete.
     */
    webkit_web_view_signals[CUT_CLIPBOARD] = g_signal_new("cut-clipboard",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET(WebKitWebViewClass, cut_clipboard),
        NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    /**
     * WebKitWebView::copy-clipboard:
     * @web_view: the object which received the signal
     *
     * The #WebKitWebView::copy-clipboard signal is a keybinding signal which gets emitted to
     * copy the selection to the clipboard.
     *
     * The default bindings for this signal are Ctrl-c and Ctrl-Insert.
     */
    webkit_web_view_signals[COPY_CLIPBOARD] = g_signal_new("copy-clipboard",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET(WebKitWebViewClass, copy_clipboard),
        NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    /**
     * WebKitWebView::paste-clipboard:
     * @web_view: the object which received the signal
     *
     * The #WebKitWebView::paste-clipboard signal is a keybinding signal which gets emitted to
     * paste the contents of the clipboard into the Web view.
     *
     * The default bindings for this signal are Ctrl-v and Shift-Insert.
     */
    webkit_web_view_signals[PASTE_CLIPBOARD] = g_signal_new("paste-clipboard",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET(WebKitWebViewClass, paste_clipboard),
        NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    /**
     * WebKitWebView::undo:
     * @web_view: the object which received the signal
     *
     * The #WebKitWebView::undo signal is a keybinding signal which gets emitted to
     * undo the last editing command.
     *
     * The default binding for this signal is Ctrl-z
     *
     * Since: 1.1.14
     */
    webkit_web_view_signals[UNDO] = g_signal_new("undo",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET(WebKitWebViewClass, undo),
        NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    /**
     * WebKitWebView::redo:
     * @web_view: the object which received the signal
     *
     * The #WebKitWebView::redo signal is a keybinding signal which gets emitted to
     * redo the last editing command.
     *
     * The default binding for this signal is Ctrl-Shift-z
     *
     * Since: 1.1.14
     */
    webkit_web_view_signals[REDO] = g_signal_new("redo",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET(WebKitWebViewClass, redo),
        NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    /**
     * WebKitWebView::move-cursor:
     * @web_view: the object which received the signal
     * @step: the type of movement, one of #GtkMovementStep
     * @count: an integer indicating the subtype of movement. Currently
     *         the permitted values are '1' = forward, '-1' = backwards.
     *
     * The #WebKitWebView::move-cursor will be emitted to apply the
     * cursor movement described by its parameters to the @view.
     *
     * Return value: %TRUE or %FALSE
     *
     * Since: 1.1.4
     */
    webkit_web_view_signals[MOVE_CURSOR] = g_signal_new("move-cursor",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET(WebKitWebViewClass, move_cursor),
        NULL, NULL,
        webkit_marshal_BOOLEAN__ENUM_INT,
        G_TYPE_BOOLEAN, 2,
        GTK_TYPE_MOVEMENT_STEP,
        G_TYPE_INT);

    /**
     * WebKitWebView::create-plugin-widget:
     * @web_view: the object which received the signal
     * @mime_type: the mimetype of the requested object
     * @uri: the URI to load
     * @param: a #GHashTable with additional attributes (strings)
     *
     * The #WebKitWebView::create-plugin-widget signal will be emitted to
     * create a plugin widget for embed or object HTML tags. This
     * allows to embed a GtkWidget as a plugin into HTML content. In
     * case of a textual selection of the GtkWidget WebCore will attempt
     * to set the property value of "webkit-widget-is-selected". This can
     * be used to draw a visual indicator of the selection.
     *
     * Return value: (transfer full): a new #GtkWidget, or %NULL
     *
     * Since: 1.1.8
     */
    webkit_web_view_signals[PLUGIN_WIDGET] = g_signal_new("create-plugin-widget",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        webkit_signal_accumulator_object_handled,
        NULL,
        webkit_marshal_OBJECT__STRING_STRING_POINTER,
        GTK_TYPE_WIDGET, 3,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_HASH_TABLE);

    /**
     * WebKitWebView::database-quota-exceeded:
     * @web_view: the object which received the signal
     * @frame: the relevant frame
     * @database: the #WebKitWebDatabase which exceeded the quota of its #WebKitSecurityOrigin
     *
     * The #WebKitWebView::database-quota-exceeded signal will be emitted when
     * a Web Database exceeds the quota of its security origin. This signal
     * may be used to increase the size of the quota before the originating
     * operation fails.
     *
     * Since: 1.1.14
     */
    webkit_web_view_signals[DATABASE_QUOTA_EXCEEDED] = g_signal_new("database-quota-exceeded",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        NULL, NULL,
        webkit_marshal_VOID__OBJECT_OBJECT,
        G_TYPE_NONE, 2,
        G_TYPE_OBJECT, G_TYPE_OBJECT);

    /**
     * WebKitWebView::resource-request-starting:
     * @web_view: the object which received the signal
     * @web_frame: the #WebKitWebFrame whose load dispatched this request
     * @web_resource: an empty #WebKitWebResource object
     * @request: the #WebKitNetworkRequest that will be dispatched
     * @response: the #WebKitNetworkResponse representing the redirect
     * response, if any
     *
     * Emitted when a request is about to be sent. You can modify the
     * request while handling this signal. You can set the URI in the
     * #WebKitNetworkRequest object itself, and add/remove/replace
     * headers using the #SoupMessage object it carries, if it is
     * present. See webkit_network_request_get_message(). Setting the
     * request URI to "about:blank" will effectively cause the request
     * to load nothing, and can be used to disable the loading of
     * specific resources.
     *
     * Notice that information about an eventual redirect is available
     * in @response's #SoupMessage, not in the #SoupMessage carried by
     * the @request. If @response is %NULL, then this is not a
     * redirected request.
     *
     * The #WebKitWebResource object will be the same throughout all
     * the lifetime of the resource, but the contents may change from
     * inbetween signal emissions.
     *
     * Since: 1.1.14
     */
    webkit_web_view_signals[RESOURCE_REQUEST_STARTING] = g_signal_new("resource-request-starting",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        NULL, NULL,
        webkit_marshal_VOID__OBJECT_OBJECT_OBJECT_OBJECT,
        G_TYPE_NONE, 4,
        WEBKIT_TYPE_WEB_FRAME,
        WEBKIT_TYPE_WEB_RESOURCE,
        WEBKIT_TYPE_NETWORK_REQUEST,
        WEBKIT_TYPE_NETWORK_RESPONSE);

    /**
     * WebKitWebView::geolocation-policy-decision-requested:
     * @web_view: the object on which the signal is emitted
     * @frame: the frame that requests permission
     * @policy_decision: a WebKitGeolocationPolicyDecision
     *
     * This signal is emitted when a @frame wants to obtain the user's
     * location. The decision can be made asynchronously, but you must
     * call g_object_ref() the @policy_decision, and return %TRUE if
     * you are going to handle the request. To actually make the
     * decision you need to call webkit_geolocation_policy_allow() or
     * webkit_geolocation_policy_deny() on @policy_decision.
     *
     * Since: 1.1.23
     */
    webkit_web_view_signals[GEOLOCATION_POLICY_DECISION_REQUESTED] = g_signal_new("geolocation-policy-decision-requested",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST),
        0,
        NULL, NULL,
        webkit_marshal_BOOLEAN__OBJECT_OBJECT,
        G_TYPE_BOOLEAN, 2,
        WEBKIT_TYPE_WEB_FRAME,
        WEBKIT_TYPE_GEOLOCATION_POLICY_DECISION);

    /**
     * WebKitWebView::geolocation-policy-decision-cancelled:
     * @web_view: the object on which the signal is emitted
     * @frame: the frame that cancels geolocation request.
     *
     * When a @frame wants to cancel geolocation permission it had requested
     * before.
     *
     * Since: 1.1.23
     */
    webkit_web_view_signals[GEOLOCATION_POLICY_DECISION_CANCELLED] = g_signal_new("geolocation-policy-decision-cancelled",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST),
        0,
        NULL, NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        WEBKIT_TYPE_WEB_FRAME);

    /*
     * DOM-related signals. These signals are experimental, for now,
     * and may change API and ABI. Their comments lack one * on
     * purpose, to make them not be catched by gtk-doc.
     */

    /**
     * WebKitWebView::document-load-finished:
     * @web_view: the object which received the signal
     * @web_frame: the #WebKitWebFrame whose load dispatched this request
     *
     * Emitted when the DOM document object load is finished for the
     * given frame.
     */
    webkit_web_view_signals[DOCUMENT_LOAD_FINISHED] = g_signal_new("document-load-finished",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        NULL, NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        WEBKIT_TYPE_WEB_FRAME);

    /**
     * WebKitWebView::frame-created:
     * @web_view: the object which received the signal
     * @web_frame: the #WebKitWebFrame which was just created.
     *
     * Emitted when a WebKitWebView has created a new frame. This signal will
     * be emitted for all sub-frames created during page load. It will not be
     * emitted for the main frame, which originates in the WebKitWebView constructor
     * and may be accessed at any time using webkit_web_view_get_main_frame.
     *
     * Since: 1.3.4
     */
    webkit_web_view_signals[FRAME_CREATED] = g_signal_new("frame-created",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        NULL, NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        WEBKIT_TYPE_WEB_FRAME);

     /**
     * WebKitWebView::run-file-chooser:
     * @web_view: the #WebKitWebView on which the signal is emitted
     * @request: a #WebKitFileChooserRequest
     *
     * This signal is emitted when the user interacts with a &lt;input
     * type='file' /&gt; HTML element, requesting from WebKit to show
     * a dialog to select one or more files to be uploaded. To let the
     * application know the details of the file chooser, as well as to
     * allow the client application to either cancel the request or
     * perform an actual selection of files, the signal will pass an
     * instance of the #WebKitFileChooserRequest in the @request
     * argument.
     *
     * The default signal handler will asynchronously run a regular
     * #GtkFileChooserDialog for the user to interact with.
     *
     * If this signal is to be handled asynchronously, you must
     * call g_object_ref() on the @request, and return %TRUE to indicate
     * that the request is being handled. When you are ready to complete the
     * request, call webkit_file_chooser_request_select_files().
     *
     * Returns: %TRUE to stop other handlers from being invoked for the event.
     *   %FALSE to propagate the event further.
     *
     */
    webkit_web_view_signals[RUN_FILE_CHOOSER] = g_signal_new("run-file-chooser",
        G_TYPE_FROM_CLASS(webViewClass),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(WebKitWebViewClass, run_file_chooser),
        g_signal_accumulator_true_handled, 0 /* accumulator data */,
        webkit_marshal_BOOLEAN__OBJECT,
        G_TYPE_BOOLEAN, 1, /* number of parameters */
        WEBKIT_TYPE_FILE_CHOOSER_REQUEST);

    /**
     * WebKitWebView::should-begin-editing:
     * @web_view: the #WebKitWebView on which the signal is emitted
     * @range: a #WebKitDOMRange
     *
     */
    webkit_web_view_signals[SHOULD_BEGIN_EDITING] = g_signal_new("should-begin-editing",
        G_TYPE_FROM_CLASS(webViewClass),
        static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET(WebKitWebViewClass, should_allow_editing_action),
        g_signal_accumulator_first_wins, 0,
        webkit_marshal_BOOLEAN__OBJECT,
        G_TYPE_BOOLEAN, 1,
        WEBKIT_TYPE_DOM_RANGE);

    /**
     * WebKitWebView::should-end-editing:
     * @web_view: the #WebKitWebView on which the signal is emitted
     * @range: a #WebKitDOMRange
     *
     */
    webkit_web_view_signals[SHOULD_END_EDITING] = g_signal_new("should-end-editing",
        G_TYPE_FROM_CLASS(webViewClass),
        static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET(WebKitWebViewClass, should_allow_editing_action),
        g_signal_accumulator_first_wins, 0,
        webkit_marshal_BOOLEAN__OBJECT,
        G_TYPE_BOOLEAN, 1,
        WEBKIT_TYPE_DOM_RANGE);

    /**
     * WebKitWebView::should-insert-node:
     * @web_view: the #WebKitWebView on which the signal is emitted
     * @node: a #WebKitDOMNode
     * @range: a #WebKitDOMRange
     * @action: a #WebKitInsertAction
     *
     */
    webkit_web_view_signals[SHOULD_INSERT_NODE] = g_signal_new("should-insert-node",
        G_TYPE_FROM_CLASS(webViewClass),
        static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET(WebKitWebViewClass, should_allow_editing_action),
        g_signal_accumulator_first_wins, 0,
        webkit_marshal_BOOLEAN__OBJECT_OBJECT_ENUM,
        G_TYPE_BOOLEAN, 3,
        WEBKIT_TYPE_DOM_NODE,
        WEBKIT_TYPE_DOM_RANGE,
        WEBKIT_TYPE_INSERT_ACTION);

    /**
     * WebKitWebView::should-insert-text:
     * @web_view: the #WebKitWebView on which the signal is emitted
     * @string: a string
     * @range: a #WebKitDOMRange
     * @action: a #WebKitInsertAction
     *
     */
    webkit_web_view_signals[SHOULD_INSERT_TEXT] = g_signal_new("should-insert-text",
        G_TYPE_FROM_CLASS(webViewClass),
        static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET(WebKitWebViewClass, should_allow_editing_action),
        g_signal_accumulator_first_wins, 0,
        webkit_marshal_BOOLEAN__STRING_OBJECT_ENUM,
        G_TYPE_BOOLEAN, 3,
        G_TYPE_STRING,
        WEBKIT_TYPE_DOM_RANGE,
        WEBKIT_TYPE_INSERT_ACTION);

    // Only exists for GTK+ API compatibility.
    /**
     * WebKitWebView::should-delete-range:
     * @web_view: the #WebKitWebView on which the signal is emitted
     * @range: a #WebKitDOMRange
     *
     */
    webkit_web_view_signals[SHOULD_DELETE_RANGE] = g_signal_new("should-delete-range",
        G_TYPE_FROM_CLASS(webViewClass),
        static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET(WebKitWebViewClass, should_allow_editing_action),
        g_signal_accumulator_first_wins, 0,
        webkit_marshal_BOOLEAN__OBJECT,
        G_TYPE_BOOLEAN, 1,
        WEBKIT_TYPE_DOM_RANGE);

    /**
     * WebKitWebView::should-show-delete-interface-for-element:
     * @web_view: the #WebKitWebView on which the signal is emitted
     * @element: a #WebKitDOMHtmlElement
     *
     */
    webkit_web_view_signals[SHOULD_SHOW_DELETE_INTERFACE_FOR_ELEMENT] = g_signal_new("should-show-delete-interface-for-element",
        G_TYPE_FROM_CLASS(webViewClass),
        static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        g_signal_accumulator_first_wins, 0,
        webkit_marshal_BOOLEAN__OBJECT,
        G_TYPE_BOOLEAN, 1,
        WEBKIT_TYPE_DOM_HTML_ELEMENT);

    /**
     * WebKitWebView::should-change-selected-range:
     * @web_view: the #WebKitWebView on which the signal is emitted
     * @fromRange: a #WebKitDOMRange
     * @toRange: a #WebKitDOMRange
     * @affinity: a #WebKitElectionAffinity
     * @stillSelecting: bool
     *
     */
    webkit_web_view_signals[SHOULD_CHANGE_SELECTED_RANGE] = g_signal_new("should-change-selected-range",
        G_TYPE_FROM_CLASS(webViewClass),
        static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET(WebKitWebViewClass, should_allow_editing_action),
        g_signal_accumulator_first_wins, 0,
        webkit_marshal_BOOLEAN__OBJECT_OBJECT_ENUM_BOOLEAN,
        G_TYPE_BOOLEAN, 4,
        WEBKIT_TYPE_DOM_RANGE,
        WEBKIT_TYPE_DOM_RANGE,
        WEBKIT_TYPE_SELECTION_AFFINITY,
        G_TYPE_BOOLEAN);

    /**
     * WebKitWebView::should-apply-style:
     * @web_view: the #WebKitWebView on which the signal is emitted
     * @set: a #WebKitDOMCSSStyleDeclaration
     * @range: a #WebKitDOMRange
     *
     */
    webkit_web_view_signals[SHOULD_APPLY_STYLE] = g_signal_new("should-apply-style",
        G_TYPE_FROM_CLASS(webViewClass),
        static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET(WebKitWebViewClass, should_allow_editing_action),
        g_signal_accumulator_first_wins, 0,
        webkit_marshal_BOOLEAN__OBJECT_OBJECT,
        G_TYPE_BOOLEAN, 2,
        WEBKIT_TYPE_DOM_CSS_STYLE_DECLARATION,
        WEBKIT_TYPE_DOM_RANGE);

    /**
     * WebKitWebView::editing-began:
     * @web_view: the #WebKitWebView on which the signal is emitted
     *
     */
    webkit_web_view_signals[EDITING_BEGAN] = g_signal_new("editing-began",
        G_TYPE_FROM_CLASS(webViewClass),
        static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0, 0,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    /**
     * WebKitWebView::user-changed-contents:
     * @web_view: the #WebKitWebView on which the signal is emitted
     *
     */
    webkit_web_view_signals[USER_CHANGED_CONTENTS] = g_signal_new("user-changed-contents",
        G_TYPE_FROM_CLASS(webViewClass),
        static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0, 0,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    /**
     * WebKitWebView::editing-ended:
     * @web_view: the #WebKitWebView on which the signal is emitted
     *
     */
    webkit_web_view_signals[EDITING_ENDED] = g_signal_new("editing-ended",
        G_TYPE_FROM_CLASS(webViewClass),
        static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0, 0,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    /**
     * WebKitWebView::selection-changed:
     * @web_view: the #WebKitWebView on which the signal is emitted
     *
     */
    webkit_web_view_signals[SELECTION_CHANGED] = g_signal_new("selection-changed",
        G_TYPE_FROM_CLASS(webViewClass),
        static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0, 0,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    /*
     * WebKitWebView::viewport-attributes-recompute-requested:
     * @web_view: the object which received the signal
     * @viewport_attributes: the #WebKitViewportAttributes which has the viewport attributes.
     *
     * The #WebKitWebView::viewport-attributes-recompute-requested
     * signal will be emitted when a page with a viewport meta tag
     * loads and when webkit_viewport_attributes_recompute is called.
     *
     * The #WebKitViewportAttributes will have device size, available size,
     * desktop width, and device DPI pre-filled by values that make sense
     * for the current screen and widget, but you can override those values
     * if you have special requirements (for instance, if you made your
     * widget bigger than the available visible area, you should override
     * the available-width and available-height properties to the actual
     * visible area).
     *
     * Since: 1.3.8
     */
    webkit_web_view_signals[VIEWPORT_ATTRIBUTES_RECOMPUTE_REQUESTED] = g_signal_new("viewport-attributes-recompute-requested",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0, 0,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
            WEBKIT_TYPE_VIEWPORT_ATTRIBUTES);

    /*
     * WebKitWebView::viewport-attributes-changed:
     * @web_view: the object which received the signal
     * @viewport_attributes: the #WebKitViewportAttributes which has the viewport attributes.
     *
     * The #WebKitWebView::viewport-attributes-changed signal will be emitted
     * after the emission of #WebKitWebView::viewport-attributes-recompute-requested
     * and the subsequent viewport attribute recomputation. At this point,
     * if the #WebKitViewportAttributes are valid, the viewport attributes are available.
     *
     * Since: 1.3.8
     */
    webkit_web_view_signals[VIEWPORT_ATTRIBUTES_CHANGED] = g_signal_new("viewport-attributes-changed",
        G_TYPE_FROM_CLASS(webViewClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0, 0,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        WEBKIT_TYPE_VIEWPORT_ATTRIBUTES);

    /**
     * WebKitWebView::entering-fullscreen:
     * @web_view: the #WebKitWebView on which the signal is emitted.
     * @element: the #WebKitDOMHTMLElement which has requested full screen display.
     *
     * Emitted when JavaScript code calls
     * <function>element.webkitRequestFullScreen</function>. If the
     * signal is not handled the WebView will proceed to full screen
     * its top level window. This signal can be used by client code to
     * request permission to the user prior doing the full screen
     * transition and eventually prepare the top-level window
     * (e.g. hide some widgets that would otherwise be part of the
     * full screen window).
     *
     * Returns: %TRUE to stop other handlers from being invoked for the event.
     *    %FALSE to continue emission of the event.
     *
     * Since: 1.9.0
     */
    webkit_web_view_signals[ENTERING_FULLSCREEN] = g_signal_new("entering-fullscreen",
        G_TYPE_FROM_CLASS(webViewClass),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(WebKitWebViewClass, entering_fullscreen),
        g_signal_accumulator_true_handled, 0,
        webkit_marshal_BOOLEAN__OBJECT,
        G_TYPE_BOOLEAN, 1, WEBKIT_TYPE_DOM_HTML_ELEMENT);


    /**
     * WebKitWebView::leaving-fullscreen:
     * @web_view: the #WebKitWebView on which the signal is emitted.
     * @element: the #WebKitDOMHTMLElement which is currently displayed full screen.
     *
     * Emitted when the WebView is about to restore its top level
     * window out of its full screen state. This signal can be used by
     * client code to restore widgets hidden during the
     * entering-fullscreen stage for instance.
     *
     * Returns: %TRUE to stop other handlers from being invoked for the event.
     *    %FALSE to continue emission of the event.
     *
     * Since: 1.9.0
     */
    webkit_web_view_signals[LEAVING_FULLSCREEN] = g_signal_new("leaving-fullscreen",
        G_TYPE_FROM_CLASS(webViewClass),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(WebKitWebViewClass, leaving_fullscreen),
        g_signal_accumulator_true_handled, 0,
        webkit_marshal_BOOLEAN__OBJECT,
        G_TYPE_BOOLEAN, 1, WEBKIT_TYPE_DOM_HTML_ELEMENT);

    /**
     * WebKitWebView::resource-response-received:
     * @web_view: the object which received the signal
     * @web_frame: the #WebKitWebFrame the response was received for
     * @web_resource: the #WebKitWebResource being loaded
     * @response: the #WebKitNetworkResponse that was received
     *
     * Emitted when the first byte of data arrives
     *
     * Since: 1.7.5
     */
    webkit_web_view_signals[RESOURCE_RESPONSE_RECEIVED] = g_signal_new("resource-response-received",
        G_TYPE_FROM_CLASS(webViewClass),
        G_SIGNAL_RUN_LAST,
        0,
        0, 0,
        webkit_marshal_VOID__OBJECT_OBJECT_OBJECT,
        G_TYPE_NONE, 3,
        WEBKIT_TYPE_WEB_FRAME,
        WEBKIT_TYPE_WEB_RESOURCE,
        WEBKIT_TYPE_NETWORK_RESPONSE);

    /**
     * WebKitWebView::resource-load-finished:
     * @web_view: the object which received the signal
     * @web_frame: the #WebKitWebFrame the response was received for
     * @web_resource: the #WebKitWebResource that was loaded
     *
     * Emitted when all the data for the resource was loaded
     *
     * Since: 1.7.5
     */
    webkit_web_view_signals[RESOURCE_LOAD_FINISHED] = g_signal_new("resource-load-finished",
        G_TYPE_FROM_CLASS(webViewClass),
        G_SIGNAL_RUN_LAST,
        0,
        0, 0,
        webkit_marshal_VOID__OBJECT_OBJECT,
        G_TYPE_NONE, 2,
        WEBKIT_TYPE_WEB_FRAME,
        WEBKIT_TYPE_WEB_RESOURCE);

    /**
     * WebKitWebView::resource-content-length-received:
     * @web_view: the object which received the signal
     * @web_frame: the #WebKitWebFrame the response was received for
     * @web_resource: the #WebKitWebResource that was loaded
     * @length_received: the amount of data received since the last signal emission
     *
     * Emitted when new resource data has been received. The
     * @length_received variable stores the amount of bytes received
     * since the last time this signal was emitted. This is useful to
     * provide progress information about the resource load operation.
     *
     * Since: 1.7.5
     */
    webkit_web_view_signals[RESOURCE_CONTENT_LENGTH_RECEIVED] = g_signal_new("resource-content-length-received",
        G_TYPE_FROM_CLASS(webViewClass),
        G_SIGNAL_RUN_LAST,
        0,
        0, 0,
        webkit_marshal_VOID__OBJECT_OBJECT_INT,
        G_TYPE_NONE, 3,
        WEBKIT_TYPE_WEB_FRAME,
        WEBKIT_TYPE_WEB_RESOURCE,
        G_TYPE_INT);

    /**
     * WebKitWebView::resource-load-failed:
     * @web_view: the object which received the signal
     * @web_frame: the #WebKitWebFrame the response was received for
     * @web_resource: the #WebKitWebResource that was loaded
     * @error: the #GError that was triggered
     *
     * Invoked when a resource failed to load
     *
     * Since: 1.7.5
     */
    webkit_web_view_signals[RESOURCE_LOAD_FAILED] = g_signal_new("resource-load-failed",
        G_TYPE_FROM_CLASS(webViewClass),
        G_SIGNAL_RUN_LAST,
        0,
        0, 0,
        webkit_marshal_VOID__OBJECT_OBJECT_BOXED,
        G_TYPE_NONE, 3,
        WEBKIT_TYPE_WEB_FRAME,
        WEBKIT_TYPE_WEB_RESOURCE,
        G_TYPE_ERROR);

    /**
     * WebKitWebView::context-menu:
     * @web_view: the object which received the signal
     * @default_menu: the default context menu
     * @hit_test_result: a #WebKitHitTestResult with the context of the current position.
     * @triggered_with_keyboard: %TRUE if the context menu was triggered using the keyboard
     *
     * Emmited when a context menu is about to be displayed to give the application
     * a chance to create and handle its own context menu. If you only want to add custom
     * options to the default context menu you can simply modify the given @default_menu.
     *
     * When @triggered_with_keyboard is %TRUE the coordinates of the given @hit_test_result should be
     * used to position the popup menu. When the context menu has been triggered by a
     * mouse event you could either use the @hit_test_result coordinates or pass %NULL
     * to the #GtkMenuPositionFunc parameter of gtk_menu_popup() function.
     * Note that coordinates of @hit_test_result are relative to @web_view window.
     *
     * If your application will create and display its own popup menu, %TRUE should be returned.
     * Note that when the context menu is handled by the application, the #WebKitWebSettings:enable-default-context-menu
     * setting will be ignored and the #WebKitWebView::populate-popup signal won't be emitted.
     * If you don't want any context menu to be shown, you can simply connect to this signal
     * and return %TRUE without doing anything else.
     *
     * Since: 1.10
     */
    webkit_web_view_signals[CONTEXT_MENU] = g_signal_new("context-menu",
        G_TYPE_FROM_CLASS(webViewClass),
        G_SIGNAL_RUN_LAST,
        0, 0, 0,
        webkit_marshal_BOOLEAN__OBJECT_OBJECT_BOOLEAN,
        G_TYPE_BOOLEAN, 3,
        GTK_TYPE_WIDGET,
        WEBKIT_TYPE_HIT_TEST_RESULT,
        G_TYPE_BOOLEAN);

    /*
     * implementations of virtual methods
     */
    webViewClass->create_web_view = webkit_web_view_real_create_web_view;
    webViewClass->web_view_ready = webkit_web_view_real_web_view_ready;
    webViewClass->close_web_view = webkit_web_view_real_close_web_view;
    webViewClass->navigation_requested = webkit_web_view_real_navigation_requested;
    webViewClass->window_object_cleared = webkit_web_view_real_window_object_cleared;
    webViewClass->choose_file = webkit_web_view_real_choose_file;
    webViewClass->script_alert = webkit_web_view_real_script_alert;
    webViewClass->script_confirm = webkit_web_view_real_script_confirm;
    webViewClass->script_prompt = webkit_web_view_real_script_prompt;
    webViewClass->console_message = webkit_web_view_real_console_message;
    webViewClass->select_all = webkit_web_view_real_select_all;
    webViewClass->cut_clipboard = webkit_web_view_real_cut_clipboard;
    webViewClass->copy_clipboard = webkit_web_view_real_copy_clipboard;
    webViewClass->paste_clipboard = webkit_web_view_real_paste_clipboard;
    webViewClass->undo = webkit_web_view_real_undo;
    webViewClass->redo = webkit_web_view_real_redo;
    webViewClass->move_cursor = webkit_web_view_real_move_cursor;
    webViewClass->should_allow_editing_action = webkit_web_view_real_should_allow_editing_action;
    webViewClass->entering_fullscreen = webkit_web_view_real_entering_fullscreen;
    webViewClass->leaving_fullscreen = webkit_web_view_real_leaving_fullscreen;
    webViewClass->run_file_chooser = webkitWebViewRealRunFileChooser;

    GObjectClass* objectClass = G_OBJECT_CLASS(webViewClass);
    objectClass->dispose = webkit_web_view_dispose;
    objectClass->finalize = webkit_web_view_finalize;
    objectClass->get_property = webkit_web_view_get_property;
    objectClass->set_property = webkit_web_view_set_property;

    GtkWidgetClass* widgetClass = GTK_WIDGET_CLASS(webViewClass);
    widgetClass->realize = webkit_web_view_realize;
#ifdef GTK_API_VERSION_2
    widgetClass->expose_event = webkit_web_view_expose_event;
#else
    widgetClass->draw = webkit_web_view_draw;
#endif
    widgetClass->key_press_event = webkit_web_view_key_press_event;
    widgetClass->key_release_event = webkit_web_view_key_release_event;
    widgetClass->button_press_event = webkit_web_view_button_press_event;
    widgetClass->button_release_event = webkit_web_view_button_release_event;
    widgetClass->motion_notify_event = webkit_web_view_motion_event;
    widgetClass->scroll_event = webkit_web_view_scroll_event;
    widgetClass->size_allocate = webkit_web_view_size_allocate;
#ifdef GTK_API_VERSION_2
    widgetClass->size_request = webkit_web_view_size_request;
#else
    widgetClass->get_preferred_width = webkit_web_view_get_preferred_width;
    widgetClass->get_preferred_height = webkit_web_view_get_preferred_height;
#endif
#if ENABLE(CONTEXT_MENUS)
    widgetClass->popup_menu = webkit_web_view_popup_menu_handler;
#else
    widgetClass->popup_menu = NULL;
#endif
    widgetClass->grab_focus = webkit_web_view_grab_focus;
    widgetClass->focus_in_event = webkit_web_view_focus_in_event;
    widgetClass->focus_out_event = webkit_web_view_focus_out_event;
    widgetClass->get_accessible = webkit_web_view_get_accessible;
    widgetClass->screen_changed = webkit_web_view_screen_changed;
#if ENABLE(DRAG_SUPPORT)
    widgetClass->drag_end = webkit_web_view_drag_end;
    widgetClass->drag_data_get = webkit_web_view_drag_data_get;
    widgetClass->drag_motion = webkit_web_view_drag_motion;
    widgetClass->drag_leave = webkit_web_view_drag_leave;
    widgetClass->drag_drop = webkit_web_view_drag_drop;
    widgetClass->drag_data_received = webkit_web_view_drag_data_received;
#else
    widgetClass->drag_end = NULL;
    widgetClass->drag_data_get = NULL;
    widgetClass->drag_motion = NULL;
    widgetClass->drag_leave = NULL;
    widgetClass->drag_drop = NULL;
    widgetClass->drag_data_received = NULL;
#endif
    widgetClass->query_tooltip = webkit_web_view_query_tooltip;
    widgetClass->show_help = webkit_web_view_show_help;
    widgetClass->map = webkitWebViewMap;

    GtkContainerClass* containerClass = GTK_CONTAINER_CLASS(webViewClass);
    containerClass->add = webkit_web_view_container_add;
    containerClass->remove = webkit_web_view_container_remove;
    containerClass->forall = webkit_web_view_container_forall;

    /*
     * make us scrollable (e.g. addable to a GtkScrolledWindow)
     */
#ifdef GTK_API_VERSION_2
    webViewClass->set_scroll_adjustments = webkit_web_view_set_scroll_adjustments;
    GTK_WIDGET_CLASS(webViewClass)->set_scroll_adjustments_signal = g_signal_new("set-scroll-adjustments",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, set_scroll_adjustments),
            NULL, NULL,
            webkit_marshal_VOID__OBJECT_OBJECT,
            G_TYPE_NONE, 2,
            GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
#else
    g_object_class_override_property(objectClass, PROP_HADJUSTMENT, "hadjustment");
    g_object_class_override_property(objectClass, PROP_VADJUSTMENT, "vadjustment");
    g_object_class_override_property(objectClass, PROP_HSCROLL_POLICY, "hscroll-policy");
    g_object_class_override_property(objectClass, PROP_VSCROLL_POLICY, "vscroll-policy");
#endif

    /*
     * Key bindings
     */

    binding_set = gtk_binding_set_by_class(webViewClass);

    gtk_binding_entry_add_signal(binding_set, GDK_a, GDK_CONTROL_MASK,
                                 "select_all", 0);

    /* Cut/copy/paste */

    gtk_binding_entry_add_signal(binding_set, GDK_x, GDK_CONTROL_MASK,
                                 "cut_clipboard", 0);
    gtk_binding_entry_add_signal(binding_set, GDK_c, GDK_CONTROL_MASK,
                                 "copy_clipboard", 0);
    gtk_binding_entry_add_signal(binding_set, GDK_v, GDK_CONTROL_MASK,
                                 "paste_clipboard", 0);
    gtk_binding_entry_add_signal(binding_set, GDK_z, GDK_CONTROL_MASK,
                                 "undo", 0);
    gtk_binding_entry_add_signal(binding_set, GDK_z, static_cast<GdkModifierType>(GDK_CONTROL_MASK | GDK_SHIFT_MASK),
                                 "redo", 0);

    gtk_binding_entry_add_signal(binding_set, GDK_Delete, GDK_SHIFT_MASK,
                                 "cut_clipboard", 0);
    gtk_binding_entry_add_signal(binding_set, GDK_Insert, GDK_CONTROL_MASK,
                                 "copy_clipboard", 0);
    gtk_binding_entry_add_signal(binding_set, GDK_Insert, GDK_SHIFT_MASK,
                                 "paste_clipboard", 0);

    /* Movement */

    gtk_binding_entry_add_signal(binding_set, GDK_Down, static_cast<GdkModifierType>(0),
                                 "move-cursor", 2,
                                 G_TYPE_ENUM, GTK_MOVEMENT_DISPLAY_LINES,
                                 G_TYPE_INT, 1);
    gtk_binding_entry_add_signal(binding_set, GDK_Up, static_cast<GdkModifierType>(0),
                                 "move-cursor", 2,
                                 G_TYPE_ENUM, GTK_MOVEMENT_DISPLAY_LINES,
                                 G_TYPE_INT, -1);
    gtk_binding_entry_add_signal(binding_set, GDK_Right, static_cast<GdkModifierType>(0),
                                 "move-cursor", 2,
                                 G_TYPE_ENUM, GTK_MOVEMENT_VISUAL_POSITIONS,
                                 G_TYPE_INT, 1);
    gtk_binding_entry_add_signal(binding_set, GDK_Left, static_cast<GdkModifierType>(0),
                                 "move-cursor", 2,
                                 G_TYPE_ENUM, GTK_MOVEMENT_VISUAL_POSITIONS,
                                 G_TYPE_INT, -1);
    gtk_binding_entry_add_signal(binding_set, GDK_space, static_cast<GdkModifierType>(0),
                                 "move-cursor", 2,
                                 G_TYPE_ENUM, GTK_MOVEMENT_PAGES,
                                 G_TYPE_INT, 1);
    gtk_binding_entry_add_signal(binding_set, GDK_space, GDK_SHIFT_MASK,
                                 "move-cursor", 2,
                                 G_TYPE_ENUM, GTK_MOVEMENT_PAGES,
                                 G_TYPE_INT, -1);
    gtk_binding_entry_add_signal(binding_set, GDK_Page_Down, static_cast<GdkModifierType>(0),
                                 "move-cursor", 2,
                                 G_TYPE_ENUM, GTK_MOVEMENT_PAGES,
                                 G_TYPE_INT, 1);
    gtk_binding_entry_add_signal(binding_set, GDK_Page_Up, static_cast<GdkModifierType>(0),
                                 "move-cursor", 2,
                                 G_TYPE_ENUM, GTK_MOVEMENT_PAGES,
                                 G_TYPE_INT, -1);
    gtk_binding_entry_add_signal(binding_set, GDK_End, static_cast<GdkModifierType>(0),
                                 "move-cursor", 2,
                                 G_TYPE_ENUM, GTK_MOVEMENT_BUFFER_ENDS,
                                 G_TYPE_INT, 1);
    gtk_binding_entry_add_signal(binding_set, GDK_Home, static_cast<GdkModifierType>(0),
                                 "move-cursor", 2,
                                 G_TYPE_ENUM, GTK_MOVEMENT_BUFFER_ENDS,
                                 G_TYPE_INT, -1);

    /*
     * properties
     */

    /**
    * WebKitWebView:title:
    *
    * Returns: the @web_view's document title.
    *
    * Since: 1.1.4
    */
    g_object_class_install_property(objectClass, PROP_TITLE,
                                    g_param_spec_string("title",
                                                        _("Title"),
                                                        _("Returns the @web_view's document title"),
                                                        NULL,
                                                        WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebView:uri:
    *
    * Returns: the current URI of the contents displayed by the @web_view.
    *
    * Since: 1.1.4
    */
    g_object_class_install_property(objectClass, PROP_URI,
                                    g_param_spec_string("uri",
                                                        _("URI"),
                                                        _("Returns the current URI of the contents displayed by the @web_view"),
                                                        NULL,
                                                        WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebView:copy-target-list:
    *
    * The list of targets this web view supports for clipboard copying.
    *
    * Since: 1.0.2
    */
    g_object_class_install_property(objectClass, PROP_COPY_TARGET_LIST,
                                    g_param_spec_boxed("copy-target-list",
                                                       _("Copy target list"),
                                                       _("The list of targets this web view supports for clipboard copying"),
                                                       GTK_TYPE_TARGET_LIST,
                                                       WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebView:paste-target-list:
    *
    * The list of targets this web view supports for clipboard pasting.
    *
    * Since: 1.0.2
    */
    g_object_class_install_property(objectClass, PROP_PASTE_TARGET_LIST,
                                    g_param_spec_boxed("paste-target-list",
                                                       _("Paste target list"),
                                                       _("The list of targets this web view supports for clipboard pasting"),
                                                       GTK_TYPE_TARGET_LIST,
                                                       WEBKIT_PARAM_READABLE));

    g_object_class_install_property(objectClass, PROP_SETTINGS,
                                    g_param_spec_object("settings",
                                                        _("Settings"),
                                                        _("An associated WebKitWebSettings instance"),
                                                        WEBKIT_TYPE_WEB_SETTINGS,
                                                        WEBKIT_PARAM_READWRITE));

    /**
    * WebKitWebView:web-inspector:
    *
    * The associated WebKitWebInspector instance.
    *
    * Since: 1.0.3
    */
    g_object_class_install_property(objectClass, PROP_WEB_INSPECTOR,
                                    g_param_spec_object("web-inspector",
                                                        _("Web Inspector"),
                                                        _("The associated WebKitWebInspector instance"),
                                                        WEBKIT_TYPE_WEB_INSPECTOR,
                                                        WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebView:viewport-attributes:
    *
    * The associated #WebKitViewportAttributes instance.
    *
    * Since: 1.3.8
    */
    g_object_class_install_property(objectClass, PROP_VIEWPORT_ATTRIBUTES,
                                    g_param_spec_object("viewport-attributes",
                                                        _("Viewport Attributes"),
                                                        _("The associated WebKitViewportAttributes instance"),
                                                        WEBKIT_TYPE_VIEWPORT_ATTRIBUTES,
                                                        WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebView:window-features:
    *
    * An associated WebKitWebWindowFeatures instance.
    *
    * Since: 1.0.3
    */
    g_object_class_install_property(objectClass, PROP_WINDOW_FEATURES,
                                    g_param_spec_object("window-features",
                                                        "Window Features",
                                                        "An associated WebKitWebWindowFeatures instance",
                                                        WEBKIT_TYPE_WEB_WINDOW_FEATURES,
                                                        WEBKIT_PARAM_READWRITE));

    g_object_class_install_property(objectClass, PROP_EDITABLE,
                                    g_param_spec_boolean("editable",
                                                         _("Editable"),
                                                         _("Whether content can be modified by the user"),
                                                         FALSE,
                                                         WEBKIT_PARAM_READWRITE));

    g_object_class_install_property(objectClass, PROP_TRANSPARENT,
                                    g_param_spec_boolean("transparent",
                                                         _("Transparent"),
                                                         _("Whether content has a transparent background"),
                                                         FALSE,
                                                         WEBKIT_PARAM_READWRITE));

    /**
    * WebKitWebView:zoom-level:
    *
    * The level of zoom of the content.
    *
    * Since: 1.0.1
    */
    g_object_class_install_property(objectClass, PROP_ZOOM_LEVEL,
                                    g_param_spec_float("zoom-level",
                                                       _("Zoom level"),
                                                       _("The level of zoom of the content"),
                                                       G_MINFLOAT,
                                                       G_MAXFLOAT,
                                                       1.0f,
                                                       WEBKIT_PARAM_READWRITE));

    /**
    * WebKitWebView:full-content-zoom:
    *
    * Whether the full content is scaled when zooming.
    *
    * Since: 1.0.1
    */
    g_object_class_install_property(objectClass, PROP_FULL_CONTENT_ZOOM,
                                    g_param_spec_boolean("full-content-zoom",
                                                         _("Full content zoom"),
                                                         _("Whether the full content is scaled when zooming"),
                                                         FALSE,
                                                         WEBKIT_PARAM_READWRITE));

    /**
     * WebKitWebView:encoding:
     *
     * The default encoding of the web view.
     *
     * Since: 1.1.2
     */
    g_object_class_install_property(objectClass, PROP_ENCODING,
                                    g_param_spec_string("encoding",
                                                        _("Encoding"),
                                                        _("The default encoding of the web view"),
                                                        NULL,
                                                        WEBKIT_PARAM_READABLE));

    /**
     * WebKitWebView:custom-encoding:
     *
     * The custom encoding of the web view.
     *
     * Since: 1.1.2
     */
    g_object_class_install_property(objectClass, PROP_CUSTOM_ENCODING,
                                    g_param_spec_string("custom-encoding",
                                                        _("Custom Encoding"),
                                                        _("The custom encoding of the web view"),
                                                        NULL,
                                                        WEBKIT_PARAM_READWRITE));

    /**
    * WebKitWebView:load-status:
    *
    * Determines the current status of the load.
    *
    * Connect to "notify::load-status" to monitor loading.
    *
    * Some versions of WebKitGTK+ emitted this signal for the default
    * error page, while loading it. This behavior was considered bad,
    * because it was essentially exposing an implementation
    * detail. From 1.1.19 onwards this signal is no longer emitted for
    * the default error pages, but keep in mind that if you override
    * the error pages by using webkit_web_frame_load_alternate_string()
    * the signals will be emitted.
    *
    * Since: 1.1.7
    */
    g_object_class_install_property(objectClass, PROP_LOAD_STATUS,
                                    g_param_spec_enum("load-status",
                                                      "Load Status",
                                                      "Determines the current status of the load",
                                                      WEBKIT_TYPE_LOAD_STATUS,
                                                      WEBKIT_LOAD_FINISHED,
                                                      WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebView:progress:
    *
    * Determines the current progress of the load.
    *
    * Since: 1.1.7
    */
    g_object_class_install_property(objectClass, PROP_PROGRESS,
                                    g_param_spec_double("progress",
                                                        "Progress",
                                                        "Determines the current progress of the load",
                                                        0.0, 1.0, 1.0,
                                                        WEBKIT_PARAM_READABLE));

    /**
     * WebKitWebView:icon-uri:
     *
     * The URI for the favicon for the #WebKitWebView.
     *
     * Since: 1.1.18
     */
    g_object_class_install_property(objectClass, PROP_ICON_URI,
                                    g_param_spec_string("icon-uri",
                                                        _("Icon URI"),
                                                        _("The URI for the favicon for the #WebKitWebView."),
                                                        NULL,
                                                        WEBKIT_PARAM_READABLE));
    /**
    * WebKitWebView:im-context:
    *
    * The GtkIMMulticontext for the #WebKitWebView.
    *
    * This is the input method context used for all text entry widgets inside
    * the #WebKitWebView. It can be used to generate context menu items for
    * controlling the active input method.
    *
    * Since: 1.1.20
    */
    g_object_class_install_property(objectClass, PROP_IM_CONTEXT,
                                    g_param_spec_object("im-context",
                                                        "IM Context",
                                                        "The GtkIMMultiContext for the #WebKitWebView.",
                                                        GTK_TYPE_IM_CONTEXT,
                                                        WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebView:view-mode:
    *
    * The "view-mode" media feature for the #WebKitWebView.
    *
    * The "view-mode" media feature is additional information for web
    * applications about how the application is running, when it comes
    * to user experience. Whether the application is running inside a
    * regular browser window, in a dedicated window, fullscreen, for
    * instance.
    *
    * This property stores a %WebKitWebViewViewMode value that matches
    * the "view-mode" media feature the web application will see.
    *
    * See http://www.w3.org/TR/view-mode/ for more information.
    *
    * Since: 1.3.4
    */
    g_object_class_install_property(objectClass, PROP_VIEW_MODE,
                                    g_param_spec_enum("view-mode",
                                                      "View Mode",
                                                      "The view-mode media feature for the #WebKitWebView.",
                                                      WEBKIT_TYPE_WEB_VIEW_VIEW_MODE,
                                                      WEBKIT_WEB_VIEW_VIEW_MODE_WINDOWED,
                                                      WEBKIT_PARAM_READWRITE));

    // This property should be undocumented for now. It's only used by DRT.
    g_object_class_install_property(objectClass, PROP_SELF_SCROLLING,
                                    g_param_spec_boolean("self-scrolling", "Self-scrolling",
                                                         "Whether or not this WebView draws its own scrollbars.",
                                                         FALSE,
                                                         static_cast<GParamFlags>(G_PARAM_CONSTRUCT_ONLY | WEBKIT_PARAM_READWRITE)));

    g_type_class_add_private(webViewClass, sizeof(WebKitWebViewPrivate));
}

static void webkit_web_view_update_settings(WebKitWebView* webView)
{
    WebKitWebSettingsPrivate* settingsPrivate = webView->priv->webSettings->priv;
    Settings* coreSettings = core(webView)->settings();

    coreSettings->setDefaultTextEncodingName(settingsPrivate->defaultEncoding.data());
    coreSettings->setCursiveFontFamily(settingsPrivate->cursiveFontFamily.data());
    coreSettings->setStandardFontFamily(settingsPrivate->defaultFontFamily.data());
    coreSettings->setFantasyFontFamily(settingsPrivate->fantasyFontFamily.data());
    coreSettings->setFixedFontFamily(settingsPrivate->monospaceFontFamily.data());
    coreSettings->setSansSerifFontFamily(settingsPrivate->sansSerifFontFamily.data());
    coreSettings->setSerifFontFamily(settingsPrivate->serifFontFamily.data());
    coreSettings->setLoadsImagesAutomatically(settingsPrivate->autoLoadImages);
    coreSettings->setShrinksStandaloneImagesToFit(settingsPrivate->autoShrinkImages);
    coreSettings->setShouldRespectImageOrientation(settingsPrivate->respectImageOrientation);
    coreSettings->setShouldPrintBackgrounds(settingsPrivate->printBackgrounds);
    coreSettings->setScriptEnabled(settingsPrivate->enableScripts);
    coreSettings->setPluginsEnabled(settingsPrivate->enablePlugins);
    coreSettings->setTextAreasAreResizable(settingsPrivate->resizableTextAreas);
    coreSettings->setUserStyleSheetLocation(KURL(KURL(), settingsPrivate->userStylesheetURI.data()));
    coreSettings->setDeveloperExtrasEnabled(settingsPrivate->enableDeveloperExtras);
    coreSettings->setPrivateBrowsingEnabled(settingsPrivate->enablePrivateBrowsing);
    coreSettings->setCaretBrowsingEnabled(settingsPrivate->enableCaretBrowsing);
    coreSettings->setLocalStorageEnabled(settingsPrivate->enableHTML5LocalStorage);
    coreSettings->setLocalStorageDatabasePath(settingsPrivate->html5LocalStorageDatabasePath.data());
    coreSettings->setXSSAuditorEnabled(settingsPrivate->enableXSSAuditor);
    coreSettings->setSpatialNavigationEnabled(settingsPrivate->enableSpatialNavigation);
    coreSettings->setFrameFlatteningEnabled(settingsPrivate->enableFrameFlattening);
    coreSettings->setJavaScriptCanOpenWindowsAutomatically(settingsPrivate->javascriptCanOpenWindowsAutomatically);
    coreSettings->setJavaScriptCanAccessClipboard(settingsPrivate->javascriptCanAccessClipboard);
    coreSettings->setOfflineWebApplicationCacheEnabled(settingsPrivate->enableOfflineWebApplicationCache);
    coreSettings->setEditingBehaviorType(static_cast<WebCore::EditingBehaviorType>(settingsPrivate->editingBehavior));
    coreSettings->setAllowUniversalAccessFromFileURLs(settingsPrivate->enableUniversalAccessFromFileURIs);
    coreSettings->setAllowFileAccessFromFileURLs(settingsPrivate->enableFileAccessFromFileURIs);
    coreSettings->setDOMPasteAllowed(settingsPrivate->enableDOMPaste);
    coreSettings->setNeedsSiteSpecificQuirks(settingsPrivate->enableSiteSpecificQuirks);
    coreSettings->setUsesPageCache(settingsPrivate->enablePageCache);
    coreSettings->setJavaEnabled(settingsPrivate->enableJavaApplet);
    coreSettings->setHyperlinkAuditingEnabled(settingsPrivate->enableHyperlinkAuditing);
    coreSettings->setDNSPrefetchingEnabled(settingsPrivate->enableDNSPrefetching);
    coreSettings->setMediaPlaybackRequiresUserGesture(settingsPrivate->mediaPlaybackRequiresUserGesture);
    coreSettings->setMediaPlaybackAllowsInline(settingsPrivate->mediaPlaybackAllowsInline);
    coreSettings->setAllowDisplayOfInsecureContent(settingsPrivate->enableDisplayOfInsecureContent);
    coreSettings->setAllowRunningOfInsecureContent(settingsPrivate->enableRunningOfInsecureContent);

#if ENABLE(SQL_DATABASE)
    DatabaseManager::manager().setIsAvailable(settingsPrivate->enableHTML5Database);
#endif

#if ENABLE(FULLSCREEN_API)
    coreSettings->setFullScreenEnabled(settingsPrivate->enableFullscreen);
#endif

#if ENABLE(SPELLCHECK)
    if (settingsPrivate->enableSpellChecking) {
        WebKit::EditorClient* client = static_cast<WebKit::EditorClient*>(core(webView)->editorClient());
        static_cast<WebKit::TextCheckerClientGtk*>(client->textChecker())->updateSpellCheckingLanguage(settingsPrivate->spellCheckingLanguages.data());
    }
#endif

#if ENABLE(WEBGL)
    coreSettings->setWebGLEnabled(settingsPrivate->enableWebgl);
#endif

#if ENABLE(MEDIA_STREAM)
    WebCore::RuntimeEnabledFeatures::setMediaStreamEnabled(settingsPrivate->enableMediaStream);
#endif

#if USE(ACCELERATED_COMPOSITING)
    coreSettings->setAcceleratedCompositingEnabled(settingsPrivate->enableAcceleratedCompositing);
    char* debugVisualsEnvironment = getenv("WEBKIT_SHOW_COMPOSITING_DEBUG_VISUALS");
    bool showDebugVisuals = debugVisualsEnvironment && !strcmp(debugVisualsEnvironment, "1");
    coreSettings->setShowDebugBorders(showDebugVisuals);
    coreSettings->setShowRepaintCounter(showDebugVisuals);
#endif

#if ENABLE(WEB_AUDIO)
    coreSettings->setWebAudioEnabled(settingsPrivate->enableWebAudio);
#endif

#if ENABLE(SMOOTH_SCROLLING)
    coreSettings->setScrollAnimatorEnabled(settingsPrivate->enableSmoothScrolling);
#endif

#if ENABLE(CSS_SHADERS)
    coreSettings->setCSSCustomFilterEnabled(settingsPrivate->enableCSSShaders);
#endif

    // Use mock scrollbars if in DumpRenderTree mode (i.e. testing layout tests).
    coreSettings->setMockScrollbarsEnabled(DumpRenderTreeSupportGtk::dumpRenderTreeModeEnabled());

    if (Page* page = core(webView))
        page->setTabKeyCyclesThroughElements(settingsPrivate->tabKeyCyclesThroughElements);

    webkit_web_view_screen_changed(GTK_WIDGET(webView), NULL);
}

static void webkit_web_view_settings_notify(WebKitWebSettings* webSettings, GParamSpec* pspec, WebKitWebView* webView)
{
    Settings* settings = core(webView)->settings();

    const gchar* name = g_intern_string(pspec->name);
    GValue value = { 0, { { 0 } } };
    g_value_init(&value, pspec->value_type);
    g_object_get_property(G_OBJECT(webSettings), name, &value);

    if (name == g_intern_string("default-encoding"))
        settings->setDefaultTextEncodingName(g_value_get_string(&value));
    else if (name == g_intern_string("cursive-font-family"))
        settings->setCursiveFontFamily(g_value_get_string(&value));
    else if (name == g_intern_string("default-font-family"))
        settings->setStandardFontFamily(g_value_get_string(&value));
    else if (name == g_intern_string("fantasy-font-family"))
        settings->setFantasyFontFamily(g_value_get_string(&value));
    else if (name == g_intern_string("monospace-font-family"))
        settings->setFixedFontFamily(g_value_get_string(&value));
    else if (name == g_intern_string("sans-serif-font-family"))
        settings->setSansSerifFontFamily(g_value_get_string(&value));
    else if (name == g_intern_string("serif-font-family"))
        settings->setSerifFontFamily(g_value_get_string(&value));
    else if (name == g_intern_string("default-font-size"))
        settings->setDefaultFontSize(webViewConvertFontSizeToPixels(webView, g_value_get_int(&value)));
    else if (name == g_intern_string("default-monospace-font-size"))
        settings->setDefaultFixedFontSize(webViewConvertFontSizeToPixels(webView, g_value_get_int(&value)));
    else if (name == g_intern_string("minimum-font-size"))
        settings->setMinimumFontSize(webViewConvertFontSizeToPixels(webView, g_value_get_int(&value)));
    else if (name == g_intern_string("minimum-logical-font-size"))
        settings->setMinimumLogicalFontSize(webViewConvertFontSizeToPixels(webView, g_value_get_int(&value)));
    else if (name == g_intern_string("enforce-96-dpi"))
        webkit_web_view_screen_changed(GTK_WIDGET(webView), NULL);
    else if (name == g_intern_string("auto-load-images"))
        settings->setLoadsImagesAutomatically(g_value_get_boolean(&value));
    else if (name == g_intern_string("auto-shrink-images"))
        settings->setShrinksStandaloneImagesToFit(g_value_get_boolean(&value));
    else if (name == g_intern_string("respect-image-orientation"))
        settings->setShouldRespectImageOrientation(g_value_get_boolean(&value));
    else if (name == g_intern_string("print-backgrounds"))
        settings->setShouldPrintBackgrounds(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-scripts"))
        settings->setScriptEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-plugins"))
        settings->setPluginsEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-dns-prefetching"))
        settings->setDNSPrefetchingEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("resizable-text-areas"))
        settings->setTextAreasAreResizable(g_value_get_boolean(&value));
    else if (name == g_intern_string("user-stylesheet-uri"))
        settings->setUserStyleSheetLocation(KURL(KURL(), g_value_get_string(&value)));
    else if (name == g_intern_string("enable-developer-extras"))
        settings->setDeveloperExtrasEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-private-browsing"))
        settings->setPrivateBrowsingEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-caret-browsing"))
        settings->setCaretBrowsingEnabled(g_value_get_boolean(&value));
#if ENABLE(SQL_DATABASE)
    else if (name == g_intern_string("enable-html5-database")) {
        DatabaseManager::manager().setIsAvailable(g_value_get_boolean(&value));
    }
#endif
    else if (name == g_intern_string("enable-html5-local-storage"))
        settings->setLocalStorageEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("html5-local-storage-database-path"))
        settings->setLocalStorageDatabasePath(g_value_get_string(&value));
    else if (name == g_intern_string("enable-xss-auditor"))
        settings->setXSSAuditorEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-spatial-navigation"))
        settings->setSpatialNavigationEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-frame-flattening"))
        settings->setFrameFlatteningEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("javascript-can-open-windows-automatically"))
        settings->setJavaScriptCanOpenWindowsAutomatically(g_value_get_boolean(&value));
    else if (name == g_intern_string("javascript-can-access-clipboard"))
        settings->setJavaScriptCanAccessClipboard(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-offline-web-application-cache"))
        settings->setOfflineWebApplicationCacheEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("editing-behavior"))
        settings->setEditingBehaviorType(static_cast<WebCore::EditingBehaviorType>(g_value_get_enum(&value)));
    else if (name == g_intern_string("enable-universal-access-from-file-uris"))
        settings->setAllowUniversalAccessFromFileURLs(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-file-access-from-file-uris"))
        settings->setAllowFileAccessFromFileURLs(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-dom-paste"))
        settings->setDOMPasteAllowed(g_value_get_boolean(&value));
    else if (name == g_intern_string("tab-key-cycles-through-elements")) {
        Page* page = core(webView);
        if (page)
            page->setTabKeyCyclesThroughElements(g_value_get_boolean(&value));
    } else if (name == g_intern_string("enable-site-specific-quirks"))
        settings->setNeedsSiteSpecificQuirks(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-page-cache"))
        settings->setUsesPageCache(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-java-applet"))
        settings->setJavaEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-hyperlink-auditing"))
        settings->setHyperlinkAuditingEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("media-playback-requires-user-gesture"))
        settings->setMediaPlaybackRequiresUserGesture(g_value_get_boolean(&value));
    else if (name == g_intern_string("media-playback-allows-inline"))
        settings->setMediaPlaybackAllowsInline(g_value_get_boolean(&value));

#if ENABLE(SPELLCHECK)
    else if (name == g_intern_string("spell-checking-languages")) {
        gboolean enableSpellChecking;
        g_object_get(G_OBJECT(webSettings), "enable-spell-checking", &enableSpellChecking, NULL);
        if (enableSpellChecking) {
            WebKit::EditorClient* client = static_cast<WebKit::EditorClient*>(core(webView)->editorClient());
            static_cast<WebKit::TextCheckerClientGtk*>(client->textChecker())->updateSpellCheckingLanguage(g_value_get_string(&value));
        }
    }
#endif

#if ENABLE(WEBGL)
    else if (name == g_intern_string("enable-webgl"))
        settings->setWebGLEnabled(g_value_get_boolean(&value));
#endif

#if USE(ACCELERATED_COMPOSITING)
    else if (name == g_intern_string("enable-accelerated-compositing"))
        settings->setAcceleratedCompositingEnabled(g_value_get_boolean(&value));
#endif

#if ENABLE(WEB_AUDIO)
    else if (name == g_intern_string("enable-webaudio"))
        settings->setWebAudioEnabled(g_value_get_boolean(&value));
#endif

#if ENABLE(SMOOTH_SCROLLING)
    else if (name == g_intern_string("enable-smooth-scrolling"))
        settings->setScrollAnimatorEnabled(g_value_get_boolean(&value));
#endif

#if ENABLE(CSS_SHADERS)
    else if (name == g_intern_string("enable-css-shaders"))
        settings->setCSSCustomFilterEnabled(g_value_get_boolean(&value));
#endif

    else if (!g_object_class_find_property(G_OBJECT_GET_CLASS(webSettings), name))
        g_warning("Unexpected setting '%s'", name);
    g_value_unset(&value);
}

static void webkit_web_view_init(WebKitWebView* webView)
{
    WebKitWebViewPrivate* priv = G_TYPE_INSTANCE_GET_PRIVATE(webView, WEBKIT_TYPE_WEB_VIEW, WebKitWebViewPrivate);
    webView->priv = priv;
    // This is the placement new syntax: http://www.parashift.com/c++-faq-lite/dtors.html#faq-11.10
    // It allows us to call a constructor on manually allocated locations in memory. We must use it
    // in this case, because GLib manages the memory for the private data section, but we wish it
    // to contain C++ object members. The use of placement new calls the constructor on all C++ data
    // members, which ensures they are initialized properly.
    new (priv) WebKitWebViewPrivate();

    priv->imFilter.setWebView(webView);

    Page::PageClients pageClients;
    pageClients.chromeClient = new WebKit::ChromeClient(webView);
#if ENABLE(CONTEXT_MENUS)
    pageClients.contextMenuClient = new WebKit::ContextMenuClient(webView);
#endif
    pageClients.editorClient = new WebKit::EditorClient(webView);
#if ENABLE(DRAG_SUPPORT)
    pageClients.dragClient = new WebKit::DragClient(webView);
#endif
    pageClients.inspectorClient = new WebKit::InspectorClient(webView);

    priv->corePage = new Page(pageClients);

    priv->corePage->addLayoutMilestones(DidFirstVisuallyNonEmptyLayout);

#if ENABLE(GEOLOCATION)
    if (DumpRenderTreeSupportGtk::dumpRenderTreeModeEnabled()) {
        priv->geolocationClientMock = adoptPtr(new GeolocationClientMock);
        WebCore::provideGeolocationTo(priv->corePage, priv->geolocationClientMock.get());
        priv->geolocationClientMock.get()->setController(GeolocationController::from(priv->corePage));
    } else
        WebCore::provideGeolocationTo(priv->corePage, new WebKit::GeolocationClient(webView));
#endif
#if ENABLE(DEVICE_ORIENTATION)
    WebCore::provideDeviceMotionTo(priv->corePage, new DeviceMotionClientGtk);
    WebCore::provideDeviceOrientationTo(priv->corePage, new DeviceOrientationClientGtk);
#endif

#if ENABLE(MEDIA_STREAM)
    priv->userMediaClient = adoptPtr(new UserMediaClientGtk);
    WebCore::provideUserMediaTo(priv->corePage, priv->userMediaClient.get());
#endif

#if ENABLE(NAVIGATOR_CONTENT_UTILS)
    priv->navigatorContentUtilsClient = WebKit::NavigatorContentUtilsClient::create();
    WebCore::provideNavigatorContentUtilsTo(priv->corePage, priv->navigatorContentUtilsClient.get());
#endif

    if (DumpRenderTreeSupportGtk::dumpRenderTreeModeEnabled()) {
        // Set some testing-specific settings
        priv->corePage->settings()->setInteractiveFormValidationEnabled(true);
        priv->corePage->settings()->setValidationMessageTimerMagnification(-1);
    }

    // Pages within a same session need to be linked together otherwise some functionalities such
    // as visited link coloration (across pages) and changing popup window location will not work.
    // To keep the default behavior simple (and because no PageGroup API exist in WebKitGTK at the
    // time of writing this comment), we simply set all the pages to the same group.
    priv->corePage->setGroupName(webkitPageGroupName());

    // We also add a simple wrapper class to provide the public
    // interface for the Web Inspector.
    priv->webInspector = adoptGRef(WEBKIT_WEB_INSPECTOR(g_object_new(WEBKIT_TYPE_WEB_INSPECTOR, NULL)));
    webkit_web_inspector_set_inspector_client(priv->webInspector.get(), priv->corePage);

    // And our ViewportAttributes friend.
    priv->viewportAttributes = adoptGRef(WEBKIT_VIEWPORT_ATTRIBUTES(g_object_new(WEBKIT_TYPE_VIEWPORT_ATTRIBUTES, NULL)));
    priv->viewportAttributes->priv->webView = webView;

    gtk_widget_set_can_focus(GTK_WIDGET(webView), TRUE);

    priv->mainFrame = WEBKIT_WEB_FRAME(webkit_web_frame_new(webView));
    priv->lastPopupXPosition = priv->lastPopupYPosition = -1;

    priv->backForwardList = adoptGRef(webkit_web_back_forward_list_new_with_web_view(webView));

    priv->zoomFullContent = FALSE;

    priv->webSettings = adoptGRef(webkit_web_settings_new());
    webkit_web_view_update_settings(webView);
    g_signal_connect(priv->webSettings.get(), "notify", G_CALLBACK(webkit_web_view_settings_notify), webView);

    priv->webWindowFeatures = adoptGRef(webkit_web_window_features_new());

    priv->subResources = adoptGRef(g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref));

#if ENABLE(DRAG_SUPPORT)
    priv->dragAndDropHelper.setWidget(GTK_WIDGET(webView));
    gtk_drag_dest_set(GTK_WIDGET(webView), static_cast<GtkDestDefaults>(0), 0, 0, static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_PRIVATE));
    gtk_drag_dest_set_target_list(GTK_WIDGET(webView), PasteboardHelper::defaultPasteboardHelper()->targetList());
#endif

    priv->selfScrolling = false;

#if USE(ACCELERATED_COMPOSITING)
    priv->acceleratedCompositingContext = AcceleratedCompositingContext::create(webView);
#endif

    g_signal_connect(webView, "direction-changed", G_CALLBACK(webkitWebViewDirectionChanged), 0);
}

GtkWidget* webkit_web_view_new(void)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW, NULL));

    return GTK_WIDGET(webView);
}

void webkitWebViewRunFileChooserRequest(WebKitWebView* webView, WebKitFileChooserRequest* request)
{
    gboolean returnValue;
    g_signal_emit(webView, webkit_web_view_signals[RUN_FILE_CHOOSER], 0, request, &returnValue);
}

// for internal use only
void webkit_web_view_notify_ready(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    gboolean isHandled = FALSE;
    g_signal_emit(webView, webkit_web_view_signals[WEB_VIEW_READY], 0, &isHandled);
}

void webkit_web_view_request_download(WebKitWebView* webView, WebKitNetworkRequest* request, const ResourceResponse& response, ResourceHandle* handle)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    WebKitDownload* download;

    if (handle)
        download = webkit_download_new_with_handle(request, handle, response);
    else
        download = webkit_download_new(request);

    gboolean handled;
    g_signal_emit(webView, webkit_web_view_signals[DOWNLOAD_REQUESTED], 0, download, &handled);

    if (!handled) {
        webkit_download_cancel(download);
        g_object_unref(download);
        return;
    }

    /* Start the download now if it has a destination URI, otherwise it
        may be handled asynchronously by the application. */
    if (webkit_download_get_destination_uri(download))
        webkit_download_start(download);
}

/**
 * webkit_web_view_set_settings:
 * @web_view: a #WebKitWebView
 * @settings: (transfer none): the #WebKitWebSettings to be set
 *
 * Replaces the #WebKitWebSettings instance that is currently attached
 * to @web_view with @settings. The reference held by the @web_view on
 * the old #WebKitWebSettings instance is dropped, and the reference
 * count of @settings is inscreased.
 *
 * The settings are automatically applied to @web_view.
 */
void webkit_web_view_set_settings(WebKitWebView* webView, WebKitWebSettings* webSettings)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(WEBKIT_IS_WEB_SETTINGS(webSettings));

    WebKitWebViewPrivate* priv = webView->priv;
    g_signal_handlers_disconnect_by_func(priv->webSettings.get(), reinterpret_cast<void*>(webkit_web_view_settings_notify), webView);

    priv->webSettings = webSettings;
    webkit_web_view_update_settings(webView);
    g_signal_connect(webSettings, "notify", G_CALLBACK(webkit_web_view_settings_notify), webView);
    g_object_notify(G_OBJECT(webView), "settings");
}

/**
 * webkit_web_view_get_settings:
 * @web_view: a #WebKitWebView
 *
 * Obtains the #WebKitWebSettings associated with the
 * #WebKitWebView. The #WebKitWebView always has an associated
 * instance of #WebKitWebSettings. The reference that is returned by
 * this call is owned by the #WebKitWebView. You may need to increase
 * its reference count if you intend to keep it alive for longer than
 * the #WebKitWebView.
 *
 * Return value: (transfer none): the #WebKitWebSettings instance
 */
WebKitWebSettings* webkit_web_view_get_settings(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);
    return webView->priv->webSettings.get();
}

/**
 * webkit_web_view_get_inspector:
 * @web_view: a #WebKitWebView
 *
 * Obtains the #WebKitWebInspector associated with the
 * #WebKitWebView. Every #WebKitWebView object has a
 * #WebKitWebInspector object attached to it as soon as it is created,
 * so this function will only return NULL if the argument is not a
 * valid #WebKitWebView.
 *
 * Return value: (transfer none): the #WebKitWebInspector instance.
 *
 * Since: 1.0.3
 */
WebKitWebInspector* webkit_web_view_get_inspector(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);
    return webView->priv->webInspector.get();
}

/**
 * webkit_web_view_get_viewport_attributes:
 * @web_view: a #WebKitWebView
 *
 * Obtains the #WebKitViewportAttributes associated with the
 * #WebKitWebView. Every #WebKitWebView object has a
 * #WebKitViewportAttributes object attached to it as soon as it is
 * created, so this function will only return NULL if the argument is
 * not a valid #WebKitWebView. Do note however that the viewport
 * attributes object only contains valid information when the current
 * page has a viewport meta tag. You can check whether the data should
 * be used by checking the #WebKitViewportAttributes:valid property.
 *
 * Return value: (transfer none): the #WebKitViewportAttributes instance.
 *
 * Since: 1.3.8
 */
WebKitViewportAttributes* webkit_web_view_get_viewport_attributes(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);
    return webView->priv->viewportAttributes.get();
}

// internal
static void webkit_web_view_set_window_features(WebKitWebView* webView, WebKitWebWindowFeatures* webWindowFeatures)
{
    if (!webWindowFeatures)
      return;
    if (webkit_web_window_features_equal(webView->priv->webWindowFeatures.get(), webWindowFeatures))
      return;
    webView->priv->webWindowFeatures = webWindowFeatures;
}

/**
 * webkit_web_view_get_window_features:
 * @web_view: a #WebKitWebView
 *
 * Returns: (transfer none): the instance of #WebKitWebWindowFeatures held by the given
 * #WebKitWebView.
 *
 * Since: 1.0.3
 */
WebKitWebWindowFeatures* webkit_web_view_get_window_features(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);
    return webView->priv->webWindowFeatures.get();
}

/**
 * webkit_web_view_get_title:
 * @web_view: a #WebKitWebView
 *
 * Returns the @web_view's document title
 *
 * Since: 1.1.4
 *
 * Return value: the title of @web_view
 */
const gchar* webkit_web_view_get_title(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);

    WebKitWebViewPrivate* priv = webView->priv;
    return priv->mainFrame->priv->title;
}

/**
 * webkit_web_view_get_uri:
 * @web_view: a #WebKitWebView
 *
 * Returns the current URI of the contents displayed by the @web_view
 *
 * Since: 1.1.4
 *
 * Return value: the URI of @web_view
 */
const gchar* webkit_web_view_get_uri(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);

    WebKitWebViewPrivate* priv = webView->priv;
    return priv->mainFrame->priv->uri;
}

/**
 * webkit_web_view_set_maintains_back_forward_list:
 * @web_view: a #WebKitWebView
 * @flag: to tell the view to maintain a back or forward list
 *
 * Set the view to maintain a back or forward list of history items.
 */
void webkit_web_view_set_maintains_back_forward_list(WebKitWebView* webView, gboolean flag)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    static_cast<BackForwardListImpl*>(core(webView)->backForwardList())->setEnabled(flag);
}

/**
 * webkit_web_view_get_back_forward_list:
 * @web_view: a #WebKitWebView
 *
 * Obtains the #WebKitWebBackForwardList associated with the given #WebKitWebView. The
 * #WebKitWebBackForwardList is owned by the #WebKitWebView.
 *
 * Return value: (transfer none): the #WebKitWebBackForwardList
 */
WebKitWebBackForwardList* webkit_web_view_get_back_forward_list(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);
    if (!core(webView) || !static_cast<BackForwardListImpl*>(core(webView)->backForwardList())->enabled())
        return 0;
    return webView->priv->backForwardList.get();
}

/**
 * webkit_web_view_go_to_back_forward_item:
 * @web_view: a #WebKitWebView
 * @item: a #WebKitWebHistoryItem*
 *
 * Go to the specified #WebKitWebHistoryItem
 *
 * Return value: %TRUE if loading of item is successful, %FALSE if not
 */
gboolean webkit_web_view_go_to_back_forward_item(WebKitWebView* webView, WebKitWebHistoryItem* item)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);
    g_return_val_if_fail(WEBKIT_IS_WEB_HISTORY_ITEM(item), FALSE);

    WebKitWebBackForwardList* backForwardList = webkit_web_view_get_back_forward_list(webView);
    if (!webkit_web_back_forward_list_contains_item(backForwardList, item))
        return FALSE;

    core(webView)->goToItem(core(item), FrameLoadTypeIndexedBackForward);
    return TRUE;
}

/**
 * webkit_web_view_go_back:
 * @web_view: a #WebKitWebView
 *
 * Loads the previous history item.
 */
void webkit_web_view_go_back(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    core(webView)->goBack();
}

/**
 * webkit_web_view_go_back_or_forward:
 * @web_view: a #WebKitWebView
 * @steps: the number of steps
 *
 * Loads the history item that is the number of @steps away from the current
 * item. Negative values represent steps backward while positive values
 * represent steps forward.
 */
void webkit_web_view_go_back_or_forward(WebKitWebView* webView, gint steps)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    core(webView)->goBackOrForward(steps);
}

/**
 * webkit_web_view_go_forward:
 * @web_view: a #WebKitWebView
 *
 * Loads the next history item.
 */
void webkit_web_view_go_forward(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    core(webView)->goForward();
}

/**
 * webkit_web_view_can_go_back:
 * @web_view: a #WebKitWebView
 *
 * Determines whether #web_view has a previous history item.
 *
 * Return value: %TRUE if able to move back, %FALSE otherwise
 */
gboolean webkit_web_view_can_go_back(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    if (!core(webView) || !core(webView)->backForwardList()->backItem())
        return FALSE;

    return TRUE;
}

/**
 * webkit_web_view_can_go_back_or_forward:
 * @web_view: a #WebKitWebView
 * @steps: the number of steps
 *
 * Determines whether #web_view has a history item of @steps. Negative values
 * represent steps backward while positive values represent steps forward.
 *
 * Return value: %TRUE if able to move back or forward the given number of
 * steps, %FALSE otherwise
 */
gboolean webkit_web_view_can_go_back_or_forward(WebKitWebView* webView, gint steps)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    return core(webView)->canGoBackOrForward(steps);
}

/**
 * webkit_web_view_can_go_forward:
 * @web_view: a #WebKitWebView
 *
 * Determines whether #web_view has a next history item.
 *
 * Return value: %TRUE if able to move forward, %FALSE otherwise
 */
gboolean webkit_web_view_can_go_forward(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Page* page = core(webView);

    if (!page)
        return FALSE;

    if (!page->backForwardList()->forwardItem())
        return FALSE;

    return TRUE;
}

/**
 * webkit_web_view_open:
 * @web_view: a #WebKitWebView
 * @uri: an URI
 *
 * Requests loading of the specified URI string.
 *
 * Deprecated: 1.1.1: Use webkit_web_view_load_uri() instead.
  */
void webkit_web_view_open(WebKitWebView* webView, const gchar* uri)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(uri);

    // We used to support local paths, unlike the newer
    // function webkit_web_view_load_uri
    if (g_path_is_absolute(uri)) {
        gchar* fileUri = g_filename_to_uri(uri, NULL, NULL);
        webkit_web_view_load_uri(webView, fileUri);
        g_free(fileUri);
    }
    else
        webkit_web_view_load_uri(webView, uri);
}

void webkit_web_view_reload(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    core(webView)->mainFrame()->loader()->reload();
}

/**
 * webkit_web_view_reload_bypass_cache:
 * @web_view: a #WebKitWebView
 *
 * Reloads the @web_view without using any cached data.
 *
 * Since: 1.0.3
 */
void webkit_web_view_reload_bypass_cache(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    core(webView)->mainFrame()->loader()->reload(true);
}

/**
 * webkit_web_view_load_uri:
 * @web_view: a #WebKitWebView
 * @uri: an URI string
 *
 * Requests loading of the specified URI string.
 *
 * Since: 1.1.1
 */
void webkit_web_view_load_uri(WebKitWebView* webView, const gchar* uri)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(uri);

    WebKitWebFrame* frame = webView->priv->mainFrame;
    webkit_web_frame_load_uri(frame, uri);
}

/**
  * webkit_web_view_load_string:
  * @web_view: a #WebKitWebView
  * @content: an URI string
  * @mime_type: the MIME type, or %NULL
  * @encoding: the encoding, or %NULL
  * @base_uri: the base URI for relative locations
  *
  * Requests loading of the given @content with the specified @mime_type,
  * @encoding and @base_uri.
  *
  * If @mime_type is %NULL, "text/html" is assumed.
  *
  * If @encoding is %NULL, "UTF-8" is assumed.
  */
void webkit_web_view_load_string(WebKitWebView* webView, const gchar* content, const gchar* mimeType, const gchar* encoding, const gchar* baseUri)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(content);

    WebKitWebFrame* frame = webView->priv->mainFrame;
    webkit_web_frame_load_string(frame, content, mimeType, encoding, baseUri);
}
/**
 * webkit_web_view_load_html_string:
 * @web_view: a #WebKitWebView
 * @content: an URI string
 * @base_uri: the base URI for relative locations
 *
 * Requests loading of the given @content with the specified @base_uri.
 *
 * Deprecated: 1.1.1: Use webkit_web_view_load_string() instead.
 */
void webkit_web_view_load_html_string(WebKitWebView* webView, const gchar* content, const gchar* baseUri)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(content);

    webkit_web_view_load_string(webView, content, NULL, NULL, baseUri);
}

/**
 * webkit_web_view_load_request:
 * @web_view: a #WebKitWebView
 * @request: a #WebKitNetworkRequest
 *
 * Requests loading of the specified asynchronous client request.
 *
 * Creates a provisional data source that will transition to a committed data
 * source once any data has been received. Use webkit_web_view_stop_loading() to
 * stop the load.
 *
 * Since: 1.1.1
 */
void webkit_web_view_load_request(WebKitWebView* webView, WebKitNetworkRequest* request)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(WEBKIT_IS_NETWORK_REQUEST(request));

    WebKitWebFrame* frame = webView->priv->mainFrame;
    webkit_web_frame_load_request(frame, request);
}

/**
 * webkit_web_view_stop_loading:
 * @web_view: a #WebKitWebView
 *
 * Stops any ongoing load in the @web_view.
 **/
void webkit_web_view_stop_loading(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    core(webView)->mainFrame()->loader()->stopForUserCancel();
}

/**
 * webkit_web_view_search_text:
 * @web_view: a #WebKitWebView
 * @text: a string to look for
 * @forward: whether to find forward or not
 * @case_sensitive: whether to respect the case of text
 * @wrap: whether to continue looking at the beginning after reaching the end
 *
 * Looks for a specified string inside #web_view.
 *
 * Return value: %TRUE on success or %FALSE on failure
 */
gboolean webkit_web_view_search_text(WebKitWebView* webView, const gchar* string, gboolean caseSensitive, gboolean forward, gboolean shouldWrap)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);
    g_return_val_if_fail(string, FALSE);

    TextCaseSensitivity caseSensitivity = caseSensitive ? TextCaseSensitive : TextCaseInsensitive;
    FindDirection direction = forward ? FindDirectionForward : FindDirectionBackward;

    return core(webView)->findString(String::fromUTF8(string), caseSensitivity, direction, shouldWrap);
}

/**
 * webkit_web_view_mark_text_matches:
 * @web_view: a #WebKitWebView
 * @string: a string to look for
 * @case_sensitive: whether to respect the case of text
 * @limit: the maximum number of strings to look for or 0 for all
 *
 * Attempts to highlight all occurances of #string inside #web_view.
 *
 * Return value: the number of strings highlighted
 */
guint webkit_web_view_mark_text_matches(WebKitWebView* webView, const gchar* string, gboolean caseSensitive, guint limit)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);
    g_return_val_if_fail(string, 0);

    TextCaseSensitivity caseSensitivity = caseSensitive ? TextCaseSensitive : TextCaseInsensitive;

    return core(webView)->markAllMatchesForText(String::fromUTF8(string), caseSensitivity, false, limit);
}

/**
 * webkit_web_view_set_highlight_text_matches:
 * @web_view: a #WebKitWebView
 * @highlight: whether to highlight text matches
 *
 * Highlights text matches previously marked by webkit_web_view_mark_text_matches.
 */
void webkit_web_view_set_highlight_text_matches(WebKitWebView* webView, gboolean shouldHighlight)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    Frame *frame = core(webView)->mainFrame();
    do {
        frame->editor().setMarkedTextMatchesAreHighlighted(shouldHighlight);
        frame = frame->tree()->traverseNextWithWrap(false);
    } while (frame);
}

/**
 * webkit_web_view_unmark_text_matches:
 * @web_view: a #WebKitWebView
 *
 * Removes highlighting previously set by webkit_web_view_mark_text_matches.
 */
void webkit_web_view_unmark_text_matches(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    return core(webView)->unmarkAllTextMatches();
}

/**
 * webkit_web_view_get_main_frame:
 * @web_view: a #WebKitWebView
 *
 * Returns the main frame for the @web_view.
 *
 * Return value: (transfer none): the main #WebKitWebFrame for @web_view
 */
WebKitWebFrame* webkit_web_view_get_main_frame(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);

    return webView->priv->mainFrame;
}

/**
 * webkit_web_view_get_focused_frame:
 * @web_view: a #WebKitWebView
 *
 * Returns the frame that has focus or an active text selection.
 *
 * Return value: (transfer none): The focused #WebKitWebFrame or %NULL if no frame is focused
 */
WebKitWebFrame* webkit_web_view_get_focused_frame(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);

    Frame* focusedFrame = core(webView)->focusController()->focusedFrame();
    return kit(focusedFrame);
}

void webkit_web_view_execute_script(WebKitWebView* webView, const gchar* script)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(script);

    core(webView)->mainFrame()->script()->executeScript(String::fromUTF8(script), true);
}

/**
 * webkit_web_view_can_cut_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Determines whether or not it is currently possible to cut to the clipboard.
 *
 * Return value: %TRUE if a selection can be cut, %FALSE if not
 */
gboolean webkit_web_view_can_cut_clipboard(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return frame->editor().canCut() || frame->editor().canDHTMLCut();
}

/**
 * webkit_web_view_can_copy_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Determines whether or not it is currently possible to copy to the clipboard.
 *
 * Return value: %TRUE if a selection can be copied, %FALSE if not
 */
gboolean webkit_web_view_can_copy_clipboard(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return frame->editor().canCopy() || frame->editor().canDHTMLCopy();
}

/**
 * webkit_web_view_can_paste_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Determines whether or not it is currently possible to paste from the clipboard.
 *
 * Return value: %TRUE if a selection can be pasted, %FALSE if not
 */
gboolean webkit_web_view_can_paste_clipboard(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return frame->editor().canPaste() || frame->editor().canDHTMLPaste();
}

/**
 * webkit_web_view_cut_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Cuts the current selection inside the @web_view to the clipboard.
 */
void webkit_web_view_cut_clipboard(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (webkit_web_view_can_cut_clipboard(webView))
        g_signal_emit(webView, webkit_web_view_signals[CUT_CLIPBOARD], 0);
}

/**
 * webkit_web_view_copy_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Copies the current selection inside the @web_view to the clipboard.
 */
void webkit_web_view_copy_clipboard(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (webkit_web_view_can_copy_clipboard(webView))
        g_signal_emit(webView, webkit_web_view_signals[COPY_CLIPBOARD], 0);
}

/**
 * webkit_web_view_paste_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Pastes the current contents of the clipboard to the @web_view.
 */
void webkit_web_view_paste_clipboard(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (webkit_web_view_can_paste_clipboard(webView))
        g_signal_emit(webView, webkit_web_view_signals[PASTE_CLIPBOARD], 0);
}

/**
 * webkit_web_view_delete_selection:
 * @web_view: a #WebKitWebView
 *
 * Deletes the current selection inside the @web_view.
 */
void webkit_web_view_delete_selection(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor().performDelete();
}

/**
 * webkit_web_view_has_selection:
 * @web_view: a #WebKitWebView
 *
 * Determines whether text was selected.
 *
 * Return value: %TRUE if there is selected text, %FALSE if not
 */
gboolean webkit_web_view_has_selection(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    return !core(webView)->selection().isNone();
}

/**
 * webkit_web_view_select_all:
 * @web_view: a #WebKitWebView
 *
 * Attempts to select everything inside the @web_view.
 */
void webkit_web_view_select_all(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    g_signal_emit(webView, webkit_web_view_signals[::SELECT_ALL], 0);
}

/**
 * webkit_web_view_get_editable:
 * @web_view: a #WebKitWebView
 *
 * Returns whether the user is allowed to edit the document.
 *
 * Returns %TRUE if @web_view allows the user to edit the HTML document, %FALSE if
 * it doesn't. You can change @web_view's document programmatically regardless of
 * this setting.
 *
 * Return value: a #gboolean indicating the editable state
 */
gboolean webkit_web_view_get_editable(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    return core(webView)->isEditable();
}

/**
 * webkit_web_view_set_editable:
 * @web_view: a #WebKitWebView
 * @flag: a #gboolean indicating the editable state
 *
 * Sets whether @web_view allows the user to edit its HTML document.
 *
 * If @flag is %TRUE, @web_view allows the user to edit the document. If @flag is
 * %FALSE, an element in @web_view's document can only be edited if the
 * CONTENTEDITABLE attribute has been set on the element or one of its parent
 * elements. You can change @web_view's document programmatically regardless of
 * this setting. By default a #WebKitWebView is not editable.

 * Normally, an HTML document is not editable unless the elements within the
 * document are editable. This function provides a low-level way to make the
 * contents of a #WebKitWebView editable without altering the document or DOM
 * structure.
 */
void webkit_web_view_set_editable(WebKitWebView* webView, gboolean flag)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    flag = flag != FALSE;
    if (flag == webkit_web_view_get_editable(webView))
        return;

    core(webView)->setEditable(flag);

    Frame* frame = core(webView)->mainFrame();
    g_return_if_fail(frame);

    if (flag) {
        frame->editor().applyEditingStyleToBodyElement();
        // TODO: If the WebKitWebView is made editable and the selection is empty, set it to something.
        //if (!webkit_web_view_get_selected_dom_range(webView))
        //    mainFrame->setSelectionFromNone();
    }
    g_object_notify(G_OBJECT(webView), "editable");
}

/**
 * webkit_web_view_get_copy_target_list:
 * @web_view: a #WebKitWebView
 *
 * This function returns the list of targets this #WebKitWebView can
 * provide for clipboard copying and as DND source. The targets in the list are
 * added with values from the #WebKitWebViewTargetInfo enum,
 * using gtk_target_list_add() and
 * gtk_target_list_add_text_targets().
 *
 * Return value: the #GtkTargetList
 **/
GtkTargetList* webkit_web_view_get_copy_target_list(WebKitWebView* webView)
{
    return PasteboardHelper::defaultPasteboardHelper()->targetList();
}

/**
 * webkit_web_view_get_paste_target_list:
 * @web_view: a #WebKitWebView
 *
 * This function returns the list of targets this #WebKitWebView can
 * provide for clipboard pasting and as DND destination. The targets in the list are
 * added with values from the #WebKitWebViewTargetInfo enum,
 * using gtk_target_list_add() and
 * gtk_target_list_add_text_targets().
 *
 * Return value: the #GtkTargetList
 **/
GtkTargetList* webkit_web_view_get_paste_target_list(WebKitWebView* webView)
{
    return PasteboardHelper::defaultPasteboardHelper()->targetList();
}

/**
 * webkit_web_view_can_show_mime_type:
 * @web_view: a #WebKitWebView
 * @mime_type: a MIME type
 *
 * This functions returns whether or not a MIME type can be displayed using this view.
 *
 * Return value: a #gboolean indicating if the MIME type can be displayed
 *
 * Since: 1.0.3
 **/

gboolean webkit_web_view_can_show_mime_type(WebKitWebView* webView, const gchar* mimeType)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webkit_web_view_get_main_frame(webView));
    return frame->loader()->client()->canShowMIMEType(String::fromUTF8(mimeType));
}

/**
 * webkit_web_view_get_transparent:
 * @web_view: a #WebKitWebView
 *
 * Returns whether the #WebKitWebView has a transparent background.
 *
 * Return value: %FALSE when the #WebKitWebView draws a solid background
 * (the default), otherwise %TRUE.
 */
gboolean webkit_web_view_get_transparent(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    WebKitWebViewPrivate* priv = webView->priv;
    return priv->transparent;
}

/**
 * webkit_web_view_set_transparent:
 * @web_view: a #WebKitWebView
 * @flag: whether or not @web_view should be transparent
 *
 * Sets whether the #WebKitWebView has a transparent background.
 *
 * Pass %FALSE to have the #WebKitWebView draw a solid background
 * (the default), otherwise %TRUE.
 */
void webkit_web_view_set_transparent(WebKitWebView* webView, gboolean flag)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    WebKitWebViewPrivate* priv = webView->priv;
    priv->transparent = flag;

    // TODO: This needs to be made persistent or it could become a problem when
    // the main frame is replaced.
    Frame* frame = core(webView)->mainFrame();
    g_return_if_fail(frame);
    frame->view()->setTransparent(flag);
    g_object_notify(G_OBJECT(webView), "transparent");
}

/**
 * webkit_web_view_get_zoom_level:
 * @web_view: a #WebKitWebView
 *
 * Returns the zoom level of @web_view, i.e. the factor by which elements in
 * the page are scaled with respect to their original size.
 * If the "full-content-zoom" property is set to %FALSE (the default)
 * the zoom level changes the text size, or if %TRUE, scales all
 * elements in the page.
 *
 * Return value: the zoom level of @web_view
 *
 * Since: 1.0.1
 */
gfloat webkit_web_view_get_zoom_level(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 1.0f);

    Frame* frame = core(webView)->mainFrame();
    if (!frame)
        return 1.0f;

    WebKitWebViewPrivate* priv = webView->priv;
    return priv->zoomFullContent ? frame->pageZoomFactor() : frame->textZoomFactor();
}

static void webkit_web_view_apply_zoom_level(WebKitWebView* webView, gfloat zoomLevel)
{
    Frame* frame = core(webView)->mainFrame();
    if (!frame)
        return;

    WebKitWebViewPrivate* priv = webView->priv;
    if (priv->zoomFullContent)
        frame->setPageZoomFactor(zoomLevel);
    else
        frame->setTextZoomFactor(zoomLevel);
}

/**
 * webkit_web_view_set_zoom_level:
 * @web_view: a #WebKitWebView
 * @zoom_level: the new zoom level
 *
 * Sets the zoom level of @web_view, i.e. the factor by which elements in
 * the page are scaled with respect to their original size.
 * If the "full-content-zoom" property is set to %FALSE (the default)
 * the zoom level changes the text size, or if %TRUE, scales all
 * elements in the page.
 *
 * Since: 1.0.1
 */
void webkit_web_view_set_zoom_level(WebKitWebView* webView, gfloat zoomLevel)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    webkit_web_view_apply_zoom_level(webView, zoomLevel);
    g_object_notify(G_OBJECT(webView), "zoom-level");
}

/**
 * webkit_web_view_zoom_in:
 * @web_view: a #WebKitWebView
 *
 * Increases the zoom level of @web_view. The current zoom
 * level is incremented by the value of the "zoom-step"
 * property of the #WebKitWebSettings associated with @web_view.
 *
 * Since: 1.0.1
 */
void webkit_web_view_zoom_in(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    WebKitWebViewPrivate* priv = webView->priv;
    gfloat zoomMultiplierRatio;
    g_object_get(priv->webSettings.get(), "zoom-step", &zoomMultiplierRatio, NULL);

    webkit_web_view_set_zoom_level(webView, webkit_web_view_get_zoom_level(webView) + zoomMultiplierRatio);
}

/**
 * webkit_web_view_zoom_out:
 * @web_view: a #WebKitWebView
 *
 * Decreases the zoom level of @web_view. The current zoom
 * level is decremented by the value of the "zoom-step"
 * property of the #WebKitWebSettings associated with @web_view.
 *
 * Since: 1.0.1
 */
void webkit_web_view_zoom_out(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    WebKitWebViewPrivate* priv = webView->priv;
    gfloat zoomMultiplierRatio;
    g_object_get(priv->webSettings.get(), "zoom-step", &zoomMultiplierRatio, NULL);

    webkit_web_view_set_zoom_level(webView, webkit_web_view_get_zoom_level(webView) - zoomMultiplierRatio);
}

/**
 * webkit_web_view_get_full_content_zoom:
 * @web_view: a #WebKitWebView
 *
 * Returns whether the zoom level affects only text or all elements.
 *
 * Return value: %FALSE if only text should be scaled (the default),
 * %TRUE if the full content of the view should be scaled.
 *
 * Since: 1.0.1
 */
gboolean webkit_web_view_get_full_content_zoom(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    WebKitWebViewPrivate* priv = webView->priv;
    return priv->zoomFullContent;
}

/**
 * webkit_web_view_set_full_content_zoom:
 * @web_view: a #WebKitWebView
 * @full_content_zoom: %FALSE if only text should be scaled (the default),
 * %TRUE if the full content of the view should be scaled.
 *
 * Sets whether the zoom level affects only text or all elements.
 *
 * Since: 1.0.1
 */
void webkit_web_view_set_full_content_zoom(WebKitWebView* webView, gboolean zoomFullContent)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    WebKitWebViewPrivate* priv = webView->priv;
    if (priv->zoomFullContent == zoomFullContent)
      return;

    Frame* frame = core(webView)->mainFrame();
    if (!frame)
      return;

    gfloat zoomLevel = priv->zoomFullContent ? frame->pageZoomFactor() : frame->textZoomFactor();

    priv->zoomFullContent = zoomFullContent;
    if (priv->zoomFullContent)
        frame->setPageAndTextZoomFactors(zoomLevel, 1);
    else
        frame->setPageAndTextZoomFactors(1, zoomLevel);

    g_object_notify(G_OBJECT(webView), "full-content-zoom");
}

/**
 * webkit_web_view_get_load_status:
 * @web_view: a #WebKitWebView
 *
 * Determines the current status of the load.
 *
 * Returns: a #WebKitLoadStatus specifying the status of the current load
 *
 * Since: 1.1.7
 */
WebKitLoadStatus webkit_web_view_get_load_status(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), WEBKIT_LOAD_FINISHED);

    WebKitWebViewPrivate* priv = webView->priv;
    return priv->loadStatus;
}

/**
 * webkit_web_view_get_progress:
 * @web_view: a #WebKitWebView
 *
 * Determines the current progress of the load.
 *
 * Returns: a gdouble representing the status of the current load. This value
 *  will be in the range [0, 1].
 *
 * Since: 1.1.7
 */
gdouble webkit_web_view_get_progress(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 1.0);

    return core(webView)->progress()->estimatedProgress();
}

/**
 * webkit_web_view_get_encoding:
 * @web_view: a #WebKitWebView
 *
 * Returns the default encoding of the #WebKitWebView.
 *
 * Return value: the default encoding
 *
 * Since: 1.1.1
 */
const gchar* webkit_web_view_get_encoding(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);
    String encoding = core(webView)->mainFrame()->document()->encoding();
    if (encoding.isEmpty())
        return 0;
    webView->priv->encoding = encoding.utf8();
    return webView->priv->encoding.data();
}

/**
 * webkit_web_view_set_custom_encoding:
 * @web_view: a #WebKitWebView
 * @encoding: the new encoding, or %NULL to restore the default encoding
 *
 * Sets the current #WebKitWebView encoding, without modifying the default one,
 * and reloads the page.
 *
 * Since: 1.1.1
 */
void webkit_web_view_set_custom_encoding(WebKitWebView* webView, const char* encoding)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    core(webView)->mainFrame()->loader()->reloadWithOverrideEncoding(String::fromUTF8(encoding));
}

/**
 * webkit_web_view_get_custom_encoding:
 * @web_view: a #WebKitWebView
 *
 * Returns the current encoding of the #WebKitWebView, not the default-encoding
 * of WebKitWebSettings.
 *
 * Return value: a string containing the current custom encoding for @web_view, or %NULL if there's none set.
 *
 * Since: 1.1.1
 */
const char* webkit_web_view_get_custom_encoding(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);
    String overrideEncoding = core(webView)->mainFrame()->loader()->documentLoader()->overrideEncoding();
    if (overrideEncoding.isEmpty())
        return 0;
    webView->priv->customEncoding = overrideEncoding.utf8();
    return webView->priv->customEncoding.data();
}

/**
 * webkit_web_view_set_view_mode:
 * @web_view: the #WebKitWebView that will have its view mode set
 * @mode: the %WebKitWebViewViewMode to be set
 *
 * Sets the view-mode property of the #WebKitWebView. Check the
 * property's documentation for more information.
 *
 * Since: 1.3.4
 */
void webkit_web_view_set_view_mode(WebKitWebView* webView, WebKitWebViewViewMode mode)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    Page* page = core(webView);

    switch (mode) {
    case WEBKIT_WEB_VIEW_VIEW_MODE_FLOATING:
        page->setViewMode(Page::ViewModeFloating);
        break;
    case WEBKIT_WEB_VIEW_VIEW_MODE_FULLSCREEN:
        page->setViewMode(Page::ViewModeFullscreen);
        break;
    case WEBKIT_WEB_VIEW_VIEW_MODE_MAXIMIZED:
        page->setViewMode(Page::ViewModeMaximized);
        break;
    case WEBKIT_WEB_VIEW_VIEW_MODE_MINIMIZED:
        page->setViewMode(Page::ViewModeMinimized);
        break;
    default:
        page->setViewMode(Page::ViewModeWindowed);
        break;
    }
}

/**
 * webkit_web_view_get_view_mode:
 * @web_view: the #WebKitWebView to obtain the view mode from
 *
 * Gets the value of the view-mode property of the
 * #WebKitWebView. Check the property's documentation for more
 * information.
 *
 * Return value: the %WebKitWebViewViewMode currently set for the
 * #WebKitWebView.
 *
 * Since: 1.3.4
 */
WebKitWebViewViewMode webkit_web_view_get_view_mode(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), WEBKIT_WEB_VIEW_VIEW_MODE_WINDOWED);

    Page* page = core(webView);
    Page::ViewMode mode = page->viewMode();

    if (mode == Page::ViewModeFloating)
        return WEBKIT_WEB_VIEW_VIEW_MODE_FLOATING;

    if (mode == Page::ViewModeFullscreen)
        return WEBKIT_WEB_VIEW_VIEW_MODE_FULLSCREEN;

    if (mode == Page::ViewModeMaximized)
        return WEBKIT_WEB_VIEW_VIEW_MODE_MAXIMIZED;

    if (mode == Page::ViewModeMinimized)
        return WEBKIT_WEB_VIEW_VIEW_MODE_MINIMIZED;

    return WEBKIT_WEB_VIEW_VIEW_MODE_WINDOWED;
}

/**
 * webkit_web_view_move_cursor:
 * @web_view: a #WebKitWebView
 * @step: a #GtkMovementStep
 * @count: integer describing the direction of the movement. 1 for forward, -1 for backwards.
 *
 * Move the cursor in @view as described by @step and @count.
 *
 * Since: 1.1.4
 */
void webkit_web_view_move_cursor(WebKitWebView* webView, GtkMovementStep step, gint count)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(step == GTK_MOVEMENT_VISUAL_POSITIONS ||
                     step == GTK_MOVEMENT_DISPLAY_LINES ||
                     step == GTK_MOVEMENT_PAGES ||
                     step == GTK_MOVEMENT_BUFFER_ENDS);
    g_return_if_fail(count == 1 || count == -1);

    gboolean handled;
    g_signal_emit(webView, webkit_web_view_signals[MOVE_CURSOR], 0, step, count, &handled);
}

/**
 * webkit_web_view_can_undo:
 * @web_view: a #WebKitWebView
 *
 * Determines whether or not it is currently possible to undo the last
 * editing command in the view.
 *
 * Return value: %TRUE if a undo can be done, %FALSE if not
 *
 * Since: 1.1.14
 */
gboolean webkit_web_view_can_undo(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return frame->editor().canUndo();
}

/**
 * webkit_web_view_undo:
 * @web_view: a #WebKitWebView
 *
 * Undoes the last editing command in the view, if possible.
 *
 * Since: 1.1.14
 */
void webkit_web_view_undo(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (webkit_web_view_can_undo(webView))
        g_signal_emit(webView, webkit_web_view_signals[UNDO], 0);
}

/**
 * webkit_web_view_can_redo:
 * @web_view: a #WebKitWebView
 *
 * Determines whether or not it is currently possible to redo the last
 * editing command in the view.
 *
 * Return value: %TRUE if a redo can be done, %FALSE if not
 *
 * Since: 1.1.14
 */
gboolean webkit_web_view_can_redo(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return frame->editor().canRedo();
}

/**
 * webkit_web_view_redo:
 * @web_view: a #WebKitWebView
 *
 * Redoes the last editing command in the view, if possible.
 *
 * Since: 1.1.14
 */
void webkit_web_view_redo(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (webkit_web_view_can_redo(webView))
        g_signal_emit(webView, webkit_web_view_signals[REDO], 0);
}


/**
 * webkit_web_view_set_view_source_mode:
 * @web_view: a #WebKitWebView
 * @view_source_mode: the mode to turn on or off view source mode
 *
 * Set whether the view should be in view source mode. Setting this mode to
 * %TRUE before loading a URI will display the source of the web page in a
 * nice and readable format.
 *
 * Since: 1.1.14
 */
void webkit_web_view_set_view_source_mode (WebKitWebView* webView, gboolean mode)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (Frame* mainFrame = core(webView)->mainFrame())
        mainFrame->setInViewSourceMode(mode);
}

/**
 * webkit_web_view_get_view_source_mode:
 * @web_view: a #WebKitWebView
 *
 * Return value: %TRUE if @web_view is in view source mode, %FALSE otherwise.
 *
 * Since: 1.1.14
 */
gboolean webkit_web_view_get_view_source_mode (WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    if (Frame* mainFrame = core(webView)->mainFrame())
        return mainFrame->inViewSourceMode();

    return FALSE;
}

// Internal subresource management
void webkit_web_view_add_main_resource(WebKitWebView* webView, const char* identifier, WebKitWebResource* webResource)
{
    WebKitWebViewPrivate* priv = webView->priv;

    priv->mainResource = adoptGRef(webResource);
    priv->mainResourceIdentifier = identifier;
}

void webkit_web_view_add_resource(WebKitWebView* webView, const char* identifier, WebKitWebResource* webResource)
{
    WebKitWebViewPrivate* priv = webView->priv;
    g_hash_table_insert(priv->subResources.get(), g_strdup(identifier), webResource);
}

void webkitWebViewRemoveSubresource(WebKitWebView* webView, const char* identifier)
{
    ASSERT(identifier);

    // Don't remove the main resource.
    const CString& mainResource = webView->priv->mainResourceIdentifier;
    if (!mainResource.isNull() && g_str_equal(identifier, mainResource.data()))
        return;
    g_hash_table_remove(webView->priv->subResources.get(), identifier);
}

WebKitWebResource* webkit_web_view_get_resource(WebKitWebView* webView, char* identifier)
{
    WebKitWebViewPrivate* priv = webView->priv;
    gpointer webResource = 0;
    gboolean resourceFound = g_hash_table_lookup_extended(priv->subResources.get(), identifier, NULL, &webResource);

    // The only resource we do not store in this hash table is the
    // main!  If we did not find a request, it probably means the load
    // has been interrupted while while a resource was still being
    // loaded.
    if (!resourceFound && !g_str_equal(identifier, priv->mainResourceIdentifier.data()))
        return 0;

    if (!webResource)
        return webkit_web_view_get_main_resource(webView);

    return WEBKIT_WEB_RESOURCE(webResource);
}

WebKitWebResource* webkit_web_view_get_main_resource(WebKitWebView* webView)
{
    return webView->priv->mainResource.get();
}

void webkit_web_view_clear_resources(WebKitWebView* webView)
{
    WebKitWebViewPrivate* priv = webView->priv;

    if (priv->subResources)
        g_hash_table_remove_all(priv->subResources.get());
}

static gboolean cleanupTemporarilyCachedSubresources(gpointer data)
{
    GList* subResources = static_cast<GList*>(data);
    g_list_foreach(subResources, reinterpret_cast<GFunc>(g_object_unref), NULL);
    g_list_free(subResources);
    return FALSE;
}

GList* webkit_web_view_get_subresources(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);
    GList* subResources = 0;
    Vector<PassRefPtr<ArchiveResource> > coreSubResources;

    core(webView)->mainFrame()->loader()->documentLoader()->getSubresources(coreSubResources);

    for (unsigned i = 0; i < coreSubResources.size(); i++) {
        WebKitWebResource* webResource = WEBKIT_WEB_RESOURCE(g_object_new(WEBKIT_TYPE_WEB_RESOURCE, NULL));
        webkit_web_resource_init_with_core_resource(webResource, coreSubResources[i]);
        subResources = g_list_append(subResources, webResource);
    }

    if (subResources)
        g_timeout_add(1, cleanupTemporarilyCachedSubresources, g_list_copy(subResources));

    return subResources;
}

/* From EventHandler.cpp */
static IntPoint documentPointForWindowPoint(Frame* frame, const IntPoint& windowPoint)
{
    FrameView* view = frame->view();
    // FIXME: Is it really OK to use the wrong coordinates here when view is 0?
    // Historically the code would just crash; this is clearly no worse than that.
    return view ? view->windowToContents(windowPoint) : windowPoint;
}

void webkit_web_view_set_tooltip_text(WebKitWebView* webView, const char* tooltip)
{
    WebKitWebViewPrivate* priv = webView->priv;
    if (tooltip && *tooltip != '\0') {
        priv->tooltipText = tooltip;
        gtk_widget_set_has_tooltip(GTK_WIDGET(webView), TRUE);
    } else {
        priv->tooltipText = "";
        gtk_widget_set_has_tooltip(GTK_WIDGET(webView), FALSE);
    }

    gtk_widget_trigger_tooltip_query(GTK_WIDGET(webView));
}

/**
 * webkit_web_view_get_hit_test_result:
 * @web_view: a #WebKitWebView
 * @event: a #GdkEventButton
 *
 * Does a 'hit test' in the coordinates specified by @event to figure
 * out context information about that position in the @web_view.
 *
 * Returns: (transfer full): a newly created #WebKitHitTestResult with the context of the
 * specified position.
 *
 * Since: 1.1.15
 **/
WebKitHitTestResult* webkit_web_view_get_hit_test_result(WebKitWebView* webView, GdkEventButton* event)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);
    g_return_val_if_fail(event, NULL);

    PlatformMouseEvent mouseEvent = PlatformMouseEvent(event);
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    HitTestRequest request(HitTestRequest::Active | HitTestRequest::DisallowShadowContent);
    IntPoint documentPoint = documentPointForWindowPoint(frame, mouseEvent.position());
    MouseEventWithHitTestResults mev = frame->document()->prepareMouseEvent(request, documentPoint, mouseEvent);

    return kit(mev.hitTestResult());
}

/**
 * webkit_web_view_get_icon_uri:
 * @web_view: the #WebKitWebView object
 *
 * Obtains the URI for the favicon for the given #WebKitWebView, or
 * %NULL if there is none.
 *
 * Return value: the URI for the favicon, or %NULL
 *
 * Since: 1.1.18
 */
const gchar* webkit_web_view_get_icon_uri(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);
    String iconURL = iconDatabase().synchronousIconURLForPageURL(core(webView)->mainFrame()->document()->url().string());
    webView->priv->iconURI = iconURL.utf8();
    return webView->priv->iconURI.data();
}

/**
 * webkit_web_view_get_icon_pixbuf:
 * @web_view: the #WebKitWebView object
 *
 * Obtains a #GdkPixbuf of the favicon for the given #WebKitWebView, or
 * a default icon if there is no icon for the given page. Use
 * webkit_web_view_get_icon_uri() if you need to distinguish these cases.
 * Usually you want to connect to WebKitWebView::icon-loaded and call this
 * method in the callback.
 *
 * The pixbuf will have the largest size provided by the server and should
 * be resized before it is displayed.
 * See also webkit_icon_database_get_icon_pixbuf().
 *
 * Returns: (transfer full): a new reference to a #GdkPixbuf, or %NULL
 *
 * Since: 1.3.13
 *
 * Deprecated: 1.8: Use webkit_web_view_try_get_favicon_pixbuf() instead.
 */
GdkPixbuf* webkit_web_view_get_icon_pixbuf(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);

    const gchar* pageURI = webkit_web_view_get_uri(webView);
    WebKitIconDatabase* database = webkit_get_icon_database();
    return webkit_icon_database_get_icon_pixbuf(database, pageURI);
}

/**
 * webkit_web_view_try_get_favicon_pixbuf:
 * @web_view: the #WebKitWebView object
 * @width: the desired width for the icon
 * @height: the desired height for the icon
 *
 * Obtains a #GdkPixbuf of the favicon for the given
 * #WebKitWebView. This will return %NULL is there is no icon for the
 * current #WebKitWebView or if the icon is in the database but not
 * available at the moment of this call. Use
 * webkit_web_view_get_icon_uri() if you need to distinguish these
 * cases.  Usually you want to connect to WebKitWebView::icon-loaded
 * and call this method in the callback.
 *
 * See also webkit_favicon_database_try_get_favicon_pixbuf(). Contrary
 * to this function the icon database one returns the URL of the page
 * containing the icon.
 *
 * Returns: (transfer full): a new reference to a #GdkPixbuf, or %NULL
 *
 * Since: 1.8
 */
GdkPixbuf* webkit_web_view_try_get_favicon_pixbuf(WebKitWebView* webView, guint width, guint height)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);

    const gchar* pageURI = webkit_web_view_get_uri(webView);
    WebKitFaviconDatabase* database = webkit_get_favicon_database();
    return webkit_favicon_database_try_get_favicon_pixbuf(database, pageURI, width, height);
}

/**
 * webkit_web_view_get_dom_document:
 * @web_view: a #WebKitWebView
 *
 * Returns: (transfer none): the #WebKitDOMDocument currently loaded in
 * the main frame of the @web_view or %NULL if no document is loaded
 *
 * Since: 1.3.1
 **/
WebKitDOMDocument*
webkit_web_view_get_dom_document(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);

    return webkit_web_frame_get_dom_document(webView->priv->mainFrame);
}

GtkMenu* webkit_web_view_get_context_menu(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);

#if ENABLE(CONTEXT_MENUS)
    ContextMenu* menu = core(webView)->contextMenuController()->contextMenu();
    if (!menu)
        return 0;
    return menu->platformDescription();
#else
    return 0;
#endif
}

/**
 * webkit_web_view_get_snapshot:
 * @web_view: a #WebKitWebView
 *
 * Retrieves a snapshot with the visible contents of @webview.
 *
 * Returns: (transfer full): a @cairo_surface_t
 *
 * Since: 1.10
 **/
cairo_surface_t*
webkit_web_view_get_snapshot(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);

    Frame* frame = core(webView)->mainFrame();
    if (!frame || !frame->contentRenderer() || !frame->view())
        return 0;

    frame->view()->updateLayoutAndStyleIfNeededRecursive();
    GtkAllocation allocation;
    gtk_widget_get_allocation(GTK_WIDGET(webView), &allocation);
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, allocation.width, allocation.height);
    RefPtr<cairo_t> cr = adoptRef(cairo_create(surface));
    GraphicsContext gc(cr.get());

    IntRect rect = allocation;
    gc.applyDeviceScaleFactor(frame->page()->deviceScaleFactor());
    gc.save();
    gc.clip(rect);
    if (webView->priv->transparent)
        gc.clearRect(rect);
    frame->view()->paint(&gc, rect);
    gc.restore();

    return surface;
}

#if ENABLE(ICONDATABASE)
void webkitWebViewIconLoaded(WebKitFaviconDatabase* database, const char* frameURI, WebKitWebView* webView)
{
    // Since we definitely have an icon the WebView doesn't need to
    // listen for notifications any longer.
    webkitWebViewRegisterForIconNotification(webView, false);

    // webkit_web_view_get_icon_uri() properly updates the "icon-uri" property.
    g_object_notify(G_OBJECT(webView), "icon-uri");
    g_signal_emit(webView, webkit_web_view_signals[ICON_LOADED], 0, webkit_web_view_get_icon_uri(webView));
}

void webkitWebViewRegisterForIconNotification(WebKitWebView* webView, bool shouldRegister)
{
    WebKitFaviconDatabase* database = webkit_get_favicon_database();
    if (shouldRegister) {
        if (!g_signal_handler_is_connected(database, webView->priv->iconLoadedHandler))
            webView->priv->iconLoadedHandler = g_signal_connect(database, "icon-loaded",
                                                                G_CALLBACK(webkitWebViewIconLoaded), webView);
    } else
        if (g_signal_handler_is_connected(database, webView->priv->iconLoadedHandler))
            g_signal_handler_disconnect(database, webView->priv->iconLoadedHandler);
}
#endif

void webkitWebViewDirectionChanged(WebKitWebView* webView, GtkTextDirection previousDirection, gpointer)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    GtkTextDirection direction = gtk_widget_get_direction(GTK_WIDGET(webView));

    Frame* focusedFrame = core(webView)->focusController()->focusedFrame();
    if (!focusedFrame)
        return;

    Editor& editor = focusedFrame->editor();
    if (!editor.canEdit())
        return;

    switch (direction) {
    case GTK_TEXT_DIR_NONE:
        editor.setBaseWritingDirection(NaturalWritingDirection);
        break;
    case GTK_TEXT_DIR_LTR:
        editor.setBaseWritingDirection(LeftToRightWritingDirection);
        break;
    case GTK_TEXT_DIR_RTL:
        editor.setBaseWritingDirection(RightToLeftWritingDirection);
        break;
    default:
        g_assert_not_reached();
        return;
    }
}

namespace WebKit {

WebCore::Page* core(WebKitWebView* webView)
{
    if (!webView)
        return 0;

    WebKitWebViewPrivate* priv = webView->priv;
    return priv ? priv->corePage : 0;
}

WebKitWebView* kit(WebCore::Page* corePage)
{
    if (!corePage)
        return 0;

    WebCore::ChromeClient* chromeClient = corePage->chrome().client();
    if (chromeClient->isEmptyChromeClient())
        return 0;

    return static_cast<WebKit::ChromeClient*>(chromeClient)->webView();
}

}
