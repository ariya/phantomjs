/*
 * Copyright (C) 2007, 2008 Holger Hans Peter Freyther
 * Copyright (C) 2007, 2008 Christian Dywan <christian@imendio.com>
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2012 Igalia S. L.
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
#include "ChromeClientGtk.h"

#include "Chrome.h"
#include "Console.h"
#include "DumpRenderTreeSupportGtk.h"
#include "Editor.h"
#include "Element.h"
#include "FileChooser.h"
#include "FileIconLoader.h"
#include "FileSystem.h"
#include "FloatRect.h"
#include "FocusController.h"
#include "FrameLoadRequest.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "GtkUtilities.h"
#include "GtkVersioning.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "Icon.h"
#include "InspectorController.h"
#include "IntRect.h"
#include "KURL.h"
#include "NavigationAction.h"
#include "NotImplemented.h"
#include "PopupMenuClient.h"
#include "PopupMenuGtk.h"
#include "RefPtrCairo.h"
#include "SearchPopupMenuGtk.h"
#include "SecurityOrigin.h"
#include "WebKitDOMHTMLElementPrivate.h"
#include "WindowFeatures.h"
#include "webkitfilechooserrequestprivate.h"
#include "webkitgeolocationpolicydecision.h"
#include "webkitgeolocationpolicydecisionprivate.h"
#include "webkitnetworkrequest.h"
#include "webkitsecurityoriginprivate.h"
#include "webkitviewportattributesprivate.h"
#include "webkitwebframeprivate.h"
#include "webkitwebview.h"
#include "webkitwebviewprivate.h"
#include "webkitwebwindowfeaturesprivate.h"
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#ifdef GDK_WINDOWING_X11
#define Font XFont
#define Cursor XCursor
#define Region XRegion
#include <gdk/gdkx.h>
#undef Font
#undef Cursor
#undef Region
#undef None
#undef Status
#endif

#if ENABLE(SQL_DATABASE)
#include "DatabaseManager.h"
#endif

#if ENABLE(VIDEO) && USE(NATIVE_FULLSCREEN_VIDEO)
#include "HTMLMediaElement.h"
#endif

#ifdef GDK_WINDOWING_X11
#include "WidgetBackingStoreGtkX11.h"
#endif
#include "WidgetBackingStoreCairo.h"

using namespace WebCore;

namespace WebKit {

static PassOwnPtr<WidgetBackingStore> createBackingStore(GtkWidget* widget, const IntSize& size)
{
#ifdef GDK_WINDOWING_X11
    GdkDisplay* display = gdk_display_manager_get_default_display(gdk_display_manager_get());
    if (GDK_IS_X11_DISPLAY(display))
        return WebCore::WidgetBackingStoreGtkX11::create(widget, size);
#endif
    return WebCore::WidgetBackingStoreCairo::create(widget, size);
}

ChromeClient::ChromeClient(WebKitWebView* webView)
    : m_webView(webView)
    , m_adjustmentWatcher(webView)
    , m_closeSoonTimer(0)
    , m_displayTimer(this, &ChromeClient::paint)
    , m_forcePaint(false)
    , m_lastDisplayTime(0)
    , m_repaintSoonSourceId(0)
{
    ASSERT(m_webView);
}

void ChromeClient::chromeDestroyed()
{
    if (m_closeSoonTimer)
        g_source_remove(m_closeSoonTimer);

    if (m_repaintSoonSourceId)
        g_source_remove(m_repaintSoonSourceId);

    delete this;
}

FloatRect ChromeClient::windowRect()
{
    GtkWidget* window = gtk_widget_get_toplevel(GTK_WIDGET(m_webView));
    if (widgetIsOnscreenToplevelWindow(window)) {
        gint left, top, width, height;
        gtk_window_get_position(GTK_WINDOW(window), &left, &top);
        gtk_window_get_size(GTK_WINDOW(window), &width, &height);
        return IntRect(left, top, width, height);
    }
    return FloatRect();
}

void ChromeClient::setWindowRect(const FloatRect& rect)
{
    IntRect intrect = IntRect(rect);
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);

    g_object_set(webWindowFeatures,
                 "x", intrect.x(),
                 "y", intrect.y(),
                 "width", intrect.width(),
                 "height", intrect.height(),
                 NULL);

    gboolean autoResizeWindow;
    WebKitWebSettings* settings = webkit_web_view_get_settings(m_webView);
    g_object_get(settings, "auto-resize-window", &autoResizeWindow, NULL);

    if (!autoResizeWindow)
        return;

    GtkWidget* window = gtk_widget_get_toplevel(GTK_WIDGET(m_webView));
    if (widgetIsOnscreenToplevelWindow(window)) {
        gtk_window_move(GTK_WINDOW(window), intrect.x(), intrect.y());
        if (!intrect.isEmpty())
            gtk_window_resize(GTK_WINDOW(window), intrect.width(), intrect.height());
    }
}

static IntRect getWebViewRect(WebKitWebView* webView)
{
    GtkAllocation allocation;
    gtk_widget_get_allocation(GTK_WIDGET(webView), &allocation);
    return IntRect(allocation.x, allocation.y, allocation.width, allocation.height);
}

FloatRect ChromeClient::pageRect()
{
    return getWebViewRect(m_webView);
}

void ChromeClient::focus()
{
    gtk_widget_grab_focus(GTK_WIDGET(m_webView));
}

void ChromeClient::unfocus()
{
    GtkWidget* window = gtk_widget_get_toplevel(GTK_WIDGET(m_webView));
    if (widgetIsOnscreenToplevelWindow(window))
        gtk_window_set_focus(GTK_WINDOW(window), NULL);
}

Page* ChromeClient::createWindow(Frame* frame, const FrameLoadRequest& frameLoadRequest, const WindowFeatures& coreFeatures, const NavigationAction&)
{
    WebKitWebView* webView = 0;

    g_signal_emit_by_name(m_webView, "create-web-view", kit(frame), &webView);

    if (!webView)
        return 0;

    GRefPtr<WebKitWebWindowFeatures> webWindowFeatures(adoptGRef(kitNew(coreFeatures)));
    g_object_set(webView, "window-features", webWindowFeatures.get(), NULL);

    return core(webView);
}

void ChromeClient::show()
{
    webkit_web_view_notify_ready(m_webView);
}

bool ChromeClient::canRunModal()
{
    notImplemented();
    return false;
}

void ChromeClient::runModal()
{
    notImplemented();
}

void ChromeClient::setToolbarsVisible(bool visible)
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);

    g_object_set(webWindowFeatures, "toolbar-visible", visible, NULL);
}

bool ChromeClient::toolbarsVisible()
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);
    gboolean visible;

    g_object_get(webWindowFeatures, "toolbar-visible", &visible, NULL);
    return visible;
}

void ChromeClient::setStatusbarVisible(bool visible)
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);

    g_object_set(webWindowFeatures, "statusbar-visible", visible, NULL);
}

bool ChromeClient::statusbarVisible()
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);
    gboolean visible;

    g_object_get(webWindowFeatures, "statusbar-visible", &visible, NULL);
    return visible;
}

void ChromeClient::setScrollbarsVisible(bool visible)
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);

    g_object_set(webWindowFeatures, "scrollbar-visible", visible, NULL);
}

bool ChromeClient::scrollbarsVisible()
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);
    gboolean visible;

    g_object_get(webWindowFeatures, "scrollbar-visible", &visible, NULL);
    return visible;
}

void ChromeClient::setMenubarVisible(bool visible)
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);

    g_object_set(webWindowFeatures, "menubar-visible", visible, NULL);
}

bool ChromeClient::menubarVisible()
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);
    gboolean visible;

    g_object_get(webWindowFeatures, "menubar-visible", &visible, NULL);
    return visible;
}

void ChromeClient::setResizable(bool)
{
    // Ignored for now
}

static gboolean emitCloseWebViewSignalLater(WebKitWebView* view)
{
    gboolean isHandled;
    g_signal_emit_by_name(view, "close-web-view", &isHandled);
    return FALSE;
}

void ChromeClient::closeWindowSoon()
{
    // We may not have a WebView as create-web-view can return NULL.
    if (!m_webView)
        return;
    if (m_closeSoonTimer) // Don't call close-web-view more than once.
        return;

    // We need to remove the parent WebView from WebViewSets here, before it actually
    // closes, to make sure that JavaScript code that executes before it closes
    // can't find it. Otherwise, window.open will select a closed WebView instead of 
    // opening a new one <rdar://problem/3572585>.
    m_webView->priv->corePage->setGroupName("");

    // We also need to stop the load to prevent further parsing or JavaScript execution
    // after the window has torn down <rdar://problem/4161660>.
    webkit_web_view_stop_loading(m_webView);

    // Clients commonly destroy the web view during the close-web-view signal, but our caller
    // may need to send more signals to the web view. For instance, if this happened in the
    // onload handler, it will need to call FrameLoaderClient::dispatchDidHandleOnloadEvents.
    // Instead of firing the close-web-view signal now, fire it after the caller finishes.
    // This seems to match the Mac/Windows port behavior.
    m_closeSoonTimer = g_timeout_add(0, reinterpret_cast<GSourceFunc>(emitCloseWebViewSignalLater), m_webView);
}

bool ChromeClient::canTakeFocus(FocusDirection)
{
    return gtk_widget_get_can_focus(GTK_WIDGET(m_webView));
}

void ChromeClient::takeFocus(FocusDirection)
{
    unfocus();
}

void ChromeClient::focusedNodeChanged(Node*)
{
}

void ChromeClient::focusedFrameChanged(Frame*)
{
}

bool ChromeClient::canRunBeforeUnloadConfirmPanel()
{
    return true;
}

bool ChromeClient::runBeforeUnloadConfirmPanel(const WTF::String& message, WebCore::Frame* frame)
{
    return runJavaScriptConfirm(frame, message);
}

void ChromeClient::addMessageToConsole(WebCore::MessageSource source, WebCore::MessageLevel level, const WTF::String& message, unsigned lineNumber, unsigned columnNumber, const WTF::String& sourceId)
{
    gboolean retval;
    g_signal_emit_by_name(m_webView, "console-message", message.utf8().data(), lineNumber, sourceId.utf8().data(), &retval);
}

void ChromeClient::runJavaScriptAlert(Frame* frame, const String& message)
{
    gboolean retval;
    g_signal_emit_by_name(m_webView, "script-alert", kit(frame), message.utf8().data(), &retval);
}

bool ChromeClient::runJavaScriptConfirm(Frame* frame, const String& message)
{
    gboolean retval;
    gboolean didConfirm;
    g_signal_emit_by_name(m_webView, "script-confirm", kit(frame), message.utf8().data(), &didConfirm, &retval);
    return didConfirm == TRUE;
}

bool ChromeClient::runJavaScriptPrompt(Frame* frame, const String& message, const String& defaultValue, String& result)
{
    gboolean retval;
    gchar* value = 0;
    g_signal_emit_by_name(m_webView, "script-prompt", kit(frame), message.utf8().data(), defaultValue.utf8().data(), &value, &retval);
    if (value) {
        result = String::fromUTF8(value);
        g_free(value);
        return true;
    }
    return false;
}

void ChromeClient::setStatusbarText(const String& string)
{
    CString stringMessage = string.utf8();
    g_signal_emit_by_name(m_webView, "status-bar-text-changed", stringMessage.data());
}

bool ChromeClient::shouldInterruptJavaScript()
{
    notImplemented();
    return false;
}

KeyboardUIMode ChromeClient::keyboardUIMode()
{
    bool tabsToLinks = true;
    if (DumpRenderTreeSupportGtk::dumpRenderTreeModeEnabled())
        tabsToLinks = DumpRenderTreeSupportGtk::linksIncludedInFocusChain();

    return tabsToLinks ? KeyboardAccessTabsToLinks : KeyboardAccessDefault;
}

IntRect ChromeClient::windowResizerRect() const
{
    notImplemented();
    return IntRect();
}

static gboolean repaintEverythingSoonTimeout(ChromeClient* client)
{
    client->paint(0);
    return FALSE;
}

static void clipOutOldWidgetArea(cairo_t* cr, const IntSize& oldSize, const IntSize& newSize)
{
    cairo_move_to(cr, oldSize.width(), 0);
    cairo_line_to(cr, newSize.width(), 0);
    cairo_line_to(cr, newSize.width(), newSize.height());
    cairo_line_to(cr, 0, newSize.height());
    cairo_line_to(cr, 0, oldSize.height());
    cairo_line_to(cr, oldSize.width(), oldSize.height());
    cairo_close_path(cr);
    cairo_clip(cr);
}

static void clearEverywhereInBackingStore(WebKitWebView* webView, cairo_t* cr)
{
    // The strategy here is to quickly draw white into this new canvas, so that
    // when a user quickly resizes the WebView in an environment that has opaque
    // resizing (like Gnome Shell), there are no drawing artifacts.
    if (!webView->priv->transparent) {
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    } else
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
}

void ChromeClient::widgetSizeChanged(const IntSize& oldWidgetSize, IntSize newSize)
{
#if USE(ACCELERATED_COMPOSITING)
    AcceleratedCompositingContext* compositingContext = m_webView->priv->acceleratedCompositingContext.get();
    if (compositingContext->enabled()) {
        m_webView->priv->acceleratedCompositingContext->resizeRootLayer(newSize);
        return;
    }
#endif

    // Grow the backing store by at least 1.5 times the current size. This prevents
    // lots of unnecessary allocations during an opaque resize.
    WidgetBackingStore* backingStore = m_webView->priv->backingStore.get();
    if (backingStore && oldWidgetSize == newSize)
        return;

    if (backingStore) {
        const IntSize& oldSize = backingStore->size();
        if (newSize.width() > oldSize.width())
            newSize.setWidth(std::max(newSize.width(), static_cast<int>(oldSize.width() * 1.5)));
        if (newSize.height() > oldSize.height())
            newSize.setHeight(std::max(newSize.height(), static_cast<int>(oldSize.height() * 1.5)));
    }

    // If we did not have a backing store before or if the backing store is growing, we need
    // to reallocate a new one and set it up so that we don't see artifacts while resizing.
    if (!backingStore
        || newSize.width() > backingStore->size().width()
        || newSize.height() > backingStore->size().height()) {

        OwnPtr<WidgetBackingStore> newBackingStore = createBackingStore(GTK_WIDGET(m_webView), newSize);
        RefPtr<cairo_t> cr = adoptRef(cairo_create(newBackingStore->cairoSurface()));

        clearEverywhereInBackingStore(m_webView, cr.get());

        // Now we copy the old backing store image over the new cleared surface to prevent
        // annoying flashing as the widget grows. We do the "real" paint in a timeout
        // since we don't want to block resizing too long.
        if (backingStore) {
            cairo_set_source_surface(cr.get(), backingStore->cairoSurface(), 0, 0);
            cairo_rectangle(cr.get(), 0, 0, backingStore->size().width(), backingStore->size().height());
            cairo_fill(cr.get());
        }

        m_webView->priv->backingStore = newBackingStore.release();
        backingStore = m_webView->priv->backingStore.get();

    } else if (oldWidgetSize.width() < newSize.width() || oldWidgetSize.height() < newSize.height()) {
        // The widget is growing, but we did not need to create a new backing store.
        // We should clear any old data outside of the old widget region.
        RefPtr<cairo_t> cr = adoptRef(cairo_create(backingStore->cairoSurface()));
        clipOutOldWidgetArea(cr.get(), oldWidgetSize, newSize);
        clearEverywhereInBackingStore(m_webView, cr.get());
    }

    // We need to force a redraw and ignore the framerate cap.
    m_lastDisplayTime = 0;
    m_dirtyRegion.unite(IntRect(IntPoint(), backingStore->size()));

    // WebCore timers by default have a lower priority which leads to more artifacts when opaque
    // resize is on, thus we use g_timeout_add here to force a higher timeout priority.
    if (!m_repaintSoonSourceId)
        m_repaintSoonSourceId = g_timeout_add(0, reinterpret_cast<GSourceFunc>(repaintEverythingSoonTimeout), this);
}

static void coalesceRectsIfPossible(const IntRect& clipRect, Vector<IntRect>& rects)
{
    const unsigned int cRectThreshold = 10;
    const float cWastedSpaceThreshold = 0.75f;
    bool useUnionedRect = (rects.size() <= 1) || (rects.size() > cRectThreshold);
    if (!useUnionedRect) {
        // Attempt to guess whether or not we should use the unioned rect or the individual rects.
        // We do this by computing the percentage of "wasted space" in the union. If that wasted space
        // is too large, then we will do individual rect painting instead.
        float unionPixels = (clipRect.width() * clipRect.height());
        float singlePixels = 0;
        for (size_t i = 0; i < rects.size(); ++i)
            singlePixels += rects[i].width() * rects[i].height();
        float wastedSpace = 1 - (singlePixels / unionPixels);
        if (wastedSpace <= cWastedSpaceThreshold)
            useUnionedRect = true;
    }

    if (!useUnionedRect)
        return;

    rects.clear();
    rects.append(clipRect);
}

static void paintWebView(WebKitWebView* webView, Frame* frame, const Region& dirtyRegion)
{
    if (!webView->priv->backingStore)
        return;

    Vector<IntRect> rects = dirtyRegion.rects();
    coalesceRectsIfPossible(dirtyRegion.bounds(), rects);

    RefPtr<cairo_t> backingStoreContext = adoptRef(cairo_create(webView->priv->backingStore->cairoSurface()));
    GraphicsContext gc(backingStoreContext.get());
    gc.applyDeviceScaleFactor(frame->page()->deviceScaleFactor());
    for (size_t i = 0; i < rects.size(); i++) {
        const IntRect& rect = rects[i];

        gc.save();
        gc.clip(rect);
        if (webView->priv->transparent)
            gc.clearRect(rect);
        frame->view()->paint(&gc, rect);
        gc.restore();
    }

    gc.save();
    gc.clip(dirtyRegion.bounds());
    frame->page()->inspectorController()->drawHighlight(gc);
    gc.restore();
}

void ChromeClient::performAllPendingScrolls()
{
    if (!m_webView->priv->backingStore)
        return;

    // Scroll all pending scroll rects and invalidate those parts of the widget.
    for (size_t i = 0; i < m_rectsToScroll.size(); i++) {
        IntRect& scrollRect = m_rectsToScroll[i];
        m_webView->priv->backingStore->scroll(scrollRect, m_scrollOffsets[i]);
        gtk_widget_queue_draw_area(GTK_WIDGET(m_webView), scrollRect.x(), scrollRect.y(), scrollRect.width(), scrollRect.height());
    }

    m_rectsToScroll.clear();
    m_scrollOffsets.clear();
}

void ChromeClient::paint(WebCore::Timer<ChromeClient>*)
{
    static const double minimumFrameInterval = 1.0 / 60.0; // No more than 60 frames a second.
    double timeSinceLastDisplay = currentTime() - m_lastDisplayTime;
    double timeUntilNextDisplay = minimumFrameInterval - timeSinceLastDisplay;

    if (timeUntilNextDisplay > 0 && !m_forcePaint) {
        m_displayTimer.startOneShot(timeUntilNextDisplay);
        return;
    }

    Frame* frame = core(m_webView)->mainFrame();
    if (!frame || !frame->contentRenderer() || !frame->view())
        return;

    frame->view()->updateLayoutAndStyleIfNeededRecursive();
    performAllPendingScrolls();
    paintWebView(m_webView, frame, m_dirtyRegion);

    HashSet<GtkWidget*> children = m_webView->priv->children;
    HashSet<GtkWidget*>::const_iterator end = children.end();
    for (HashSet<GtkWidget*>::const_iterator current = children.begin(); current != end; ++current) {
        if (static_cast<GtkAllocation*>(g_object_get_data(G_OBJECT(*current), "delayed-allocation"))) {
            gtk_widget_queue_resize_no_redraw(GTK_WIDGET(m_webView));
            break;
        }
    }

    const IntRect& rect = m_dirtyRegion.bounds();
    gtk_widget_queue_draw_area(GTK_WIDGET(m_webView), rect.x(), rect.y(), rect.width(), rect.height());

    m_dirtyRegion = Region();
    m_lastDisplayTime = currentTime();
    m_repaintSoonSourceId = 0;

    // We update the IM context window location here, because we want it to be
    // synced with cursor movement. For instance, a text field can move without
    // the selection changing.
    Frame* focusedFrame = core(m_webView)->focusController()->focusedOrMainFrame();
    if (focusedFrame && focusedFrame->editor().canEdit())
        m_webView->priv->imFilter.setCursorRect(frame->selection()->absoluteCaretBounds());
}

void ChromeClient::forcePaint()
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_webView->priv->acceleratedCompositingContext->enabled())
        return;
#endif

    m_forcePaint = true;
    paint(0);
    m_forcePaint = false;
}

void ChromeClient::invalidateRootView(const IntRect&, bool immediate)
{
}

void ChromeClient::invalidateContentsAndRootView(const IntRect& updateRect, bool immediate)
{
#if USE(ACCELERATED_COMPOSITING)
    AcceleratedCompositingContext* acContext = m_webView->priv->acceleratedCompositingContext.get();
    if (acContext->enabled()) {
        acContext->setNonCompositedContentsNeedDisplay(updateRect);
        return;
    }
#endif

    if (updateRect.isEmpty())
        return;
    m_dirtyRegion.unite(updateRect);
    m_displayTimer.startOneShot(0);
}

void ChromeClient::invalidateContentsForSlowScroll(const IntRect& updateRect, bool immediate)
{
    m_adjustmentWatcher.updateAdjustmentsFromScrollbarsLater();

#if USE(ACCELERATED_COMPOSITING)
    AcceleratedCompositingContext* acContext = m_webView->priv->acceleratedCompositingContext.get();
    if (acContext->enabled()) {
        acContext->setNonCompositedContentsNeedDisplay(updateRect);
        return;
    }
#endif

    invalidateContentsAndRootView(updateRect, immediate);
}

void ChromeClient::scroll(const IntSize& delta, const IntRect& rectToScroll, const IntRect& clipRect)
{
    m_adjustmentWatcher.updateAdjustmentsFromScrollbarsLater();

#if USE(ACCELERATED_COMPOSITING)
    AcceleratedCompositingContext* compositingContext = m_webView->priv->acceleratedCompositingContext.get();
    if (compositingContext->enabled()) {
        ASSERT(!rectToScroll.isEmpty());
        ASSERT(delta.width() || delta.height());

        compositingContext->scrollNonCompositedContents(rectToScroll, delta);
        return;
    }
#endif

    m_rectsToScroll.append(rectToScroll);
    m_scrollOffsets.append(delta);

    // The code to calculate the scroll repaint region is originally from WebKit2.
    // Get the part of the dirty region that is in the scroll rect.
    Region dirtyRegionInScrollRect = intersect(rectToScroll, m_dirtyRegion);
    if (!dirtyRegionInScrollRect.isEmpty()) {
        // There are parts of the dirty region that are inside the scroll rect.
        // We need to subtract them from the region, move them and re-add them.
        m_dirtyRegion.subtract(rectToScroll);

        // Move the dirty parts.
        Region movedDirtyRegionInScrollRect = intersect(translate(dirtyRegionInScrollRect, delta), rectToScroll);

        // And add them back.
        m_dirtyRegion.unite(movedDirtyRegionInScrollRect);
    }

    // Compute the scroll repaint region. We ensure that we are not subtracting areas
    // that we've scrolled from outside the viewport from the repaint region.
    IntRect onScreenScrollRect = rectToScroll;
    onScreenScrollRect.intersect(IntRect(IntPoint(), enclosingIntRect(pageRect()).size()));
    Region scrollRepaintRegion = subtract(rectToScroll, translate(onScreenScrollRect, delta));

    m_dirtyRegion.unite(scrollRepaintRegion);
    m_displayTimer.startOneShot(0);
}

IntRect ChromeClient::rootViewToScreen(const IntRect& rect) const
{
    return IntRect(convertWidgetPointToScreenPoint(GTK_WIDGET(m_webView), rect.location()), rect.size());
}

IntPoint ChromeClient::screenToRootView(const IntPoint& point) const
{
    IntPoint widgetPositionOnScreen = convertWidgetPointToScreenPoint(GTK_WIDGET(m_webView), IntPoint());
    IntPoint result(point);
    result.move(-widgetPositionOnScreen.x(), -widgetPositionOnScreen.y());
    return result;
}

PlatformPageClient ChromeClient::platformPageClient() const
{
    return GTK_WIDGET(m_webView);
}

void ChromeClient::contentsSizeChanged(Frame* frame, const IntSize& size) const
{
    if (m_adjustmentWatcher.scrollbarsDisabled())
        return;

    // We need to queue a resize request only if the size changed,
    // otherwise we get into an infinite loop!
    GtkWidget* widget = GTK_WIDGET(m_webView);
    GtkRequisition requisition;
    gtk_widget_get_preferred_size(widget, &requisition, 0);
    if (gtk_widget_get_realized(widget)
        && (requisition.height != size.height()
        || requisition.width != size.width()))
        gtk_widget_queue_resize_no_redraw(widget);

    // If this was a main frame size change, update the scrollbars.
    if (frame != frame->page()->mainFrame())
        return;
    m_adjustmentWatcher.updateAdjustmentsFromScrollbarsLater();
}

void ChromeClient::scrollbarsModeDidChange() const
{
    WebKitWebFrame* webFrame = webkit_web_view_get_main_frame(m_webView);
    if (!webFrame)
        return;

    g_object_notify(G_OBJECT(webFrame), "horizontal-scrollbar-policy");
    g_object_notify(G_OBJECT(webFrame), "vertical-scrollbar-policy");

    gboolean isHandled;
    g_signal_emit_by_name(webFrame, "scrollbars-policy-changed", &isHandled);

    if (isHandled)
        return;

    GtkWidget* parent = gtk_widget_get_parent(GTK_WIDGET(m_webView));
    if (!parent || !GTK_IS_SCROLLED_WINDOW(parent))
        return;

    GtkPolicyType horizontalPolicy = webkit_web_frame_get_horizontal_scrollbar_policy(webFrame);
    GtkPolicyType verticalPolicy = webkit_web_frame_get_vertical_scrollbar_policy(webFrame);

    // ScrolledWindow doesn't like to display only part of a widget if
    // the scrollbars are completely disabled; We have a disparity
    // here on what the policy requested by the web app is and what we
    // can represent; the idea is not to show scrollbars, only.
    if (horizontalPolicy == GTK_POLICY_NEVER)
        horizontalPolicy = GTK_POLICY_AUTOMATIC;

    if (verticalPolicy == GTK_POLICY_NEVER)
        verticalPolicy = GTK_POLICY_AUTOMATIC;

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(parent),
                                   horizontalPolicy, verticalPolicy);
}

void ChromeClient::mouseDidMoveOverElement(const HitTestResult& hit, unsigned modifierFlags)
{
    // check if the element is a link...
    bool isLink = hit.isLiveLink();
    if (isLink) {
        KURL url = hit.absoluteLinkURL();
        if (!url.isEmpty() && url != m_hoveredLinkURL) {
            TextDirection dir;
            CString titleString = hit.title(dir).utf8();
            CString urlString = url.string().utf8();
            g_signal_emit_by_name(m_webView, "hovering-over-link", titleString.data(), urlString.data());
            m_hoveredLinkURL = url;
        }
    } else if (!isLink && !m_hoveredLinkURL.isEmpty()) {
        g_signal_emit_by_name(m_webView, "hovering-over-link", 0, 0);
        m_hoveredLinkURL = KURL();
    }

    if (Node* node = hit.innerNonSharedNode()) {
        Frame* frame = node->document()->frame();
        FrameView* view = frame ? frame->view() : 0;
        m_webView->priv->tooltipArea = view ? view->contentsToWindow(node->pixelSnappedBoundingBox()) : IntRect();
    } else
        m_webView->priv->tooltipArea = IntRect();
}

void ChromeClient::setToolTip(const String& toolTip, TextDirection)
{
    webkit_web_view_set_tooltip_text(m_webView, toolTip.utf8().data());
}

void ChromeClient::print(Frame* frame)
{
    WebKitWebFrame* webFrame = kit(frame);
    gboolean isHandled = false;
    g_signal_emit_by_name(m_webView, "print-requested", webFrame, &isHandled);

    if (isHandled)
        return;

    webkit_web_frame_print(webFrame);
}

#if ENABLE(SQL_DATABASE)
void ChromeClient::exceededDatabaseQuota(Frame* frame, const String& databaseName, DatabaseDetails)
{
    guint64 defaultQuota = webkit_get_default_web_database_quota();
    DatabaseManager::manager().setQuota(frame->document()->securityOrigin(), defaultQuota);

    WebKitWebFrame* webFrame = kit(frame);
    WebKitSecurityOrigin* origin = webkit_web_frame_get_security_origin(webFrame);
    WebKitWebDatabase* webDatabase = webkit_security_origin_get_web_database(origin, databaseName.utf8().data());
    g_signal_emit_by_name(m_webView, "database-quota-exceeded", webFrame, webDatabase);
}
#endif

void ChromeClient::reachedMaxAppCacheSize(int64_t spaceNeeded)
{
    // FIXME: Free some space.
    notImplemented();
}

void ChromeClient::reachedApplicationCacheOriginQuota(SecurityOrigin*, int64_t)
{
    notImplemented();
}

void ChromeClient::runOpenPanel(Frame*, PassRefPtr<FileChooser> prpFileChooser)
{
    GRefPtr<WebKitFileChooserRequest> request = adoptGRef(webkit_file_chooser_request_create(prpFileChooser));
    webkitWebViewRunFileChooserRequest(m_webView, request.get());
}

void ChromeClient::loadIconForFiles(const Vector<WTF::String>& filenames, WebCore::FileIconLoader* loader)
{
    loader->notifyFinished(Icon::createIconForFiles(filenames));
}

void ChromeClient::dispatchViewportPropertiesDidChange(const ViewportArguments& arguments) const
{
    // Recompute the viewport attributes making it valid.
    webkitViewportAttributesRecompute(webkit_web_view_get_viewport_attributes(m_webView));
}

void ChromeClient::setCursor(const Cursor& cursor)
{
    // [GTK] Widget::setCursor() gets called frequently
    // http://bugs.webkit.org/show_bug.cgi?id=16388
    // Setting the cursor may be an expensive operation in some backends,
    // so don't re-set the cursor if it's already set to the target value.
    GdkWindow* window = gtk_widget_get_window(platformPageClient());
    if (!window)
        return;

    GdkCursor* currentCursor = gdk_window_get_cursor(window);
    GdkCursor* newCursor = cursor.platformCursor().get();
    if (currentCursor != newCursor)
        gdk_window_set_cursor(window, newCursor);
}

void ChromeClient::setCursorHiddenUntilMouseMoves(bool)
{
    notImplemented();
}

bool ChromeClient::selectItemWritingDirectionIsNatural()
{
    return false;
}

bool ChromeClient::selectItemAlignmentFollowsMenuWritingDirection()
{
    return true;
}

bool ChromeClient::hasOpenedPopup() const
{
    notImplemented();
    return false;
}

PassRefPtr<WebCore::PopupMenu> ChromeClient::createPopupMenu(WebCore::PopupMenuClient* client) const
{
    return adoptRef(new PopupMenuGtk(client));
}

PassRefPtr<WebCore::SearchPopupMenu> ChromeClient::createSearchPopupMenu(WebCore::PopupMenuClient* client) const
{
    return adoptRef(new SearchPopupMenuGtk(client));
}

#if ENABLE(VIDEO) && USE(NATIVE_FULLSCREEN_VIDEO)
bool ChromeClient::supportsFullscreenForNode(const Node* node)
{
    return node->hasTagName(HTMLNames::videoTag);
}

void ChromeClient::enterFullscreenForNode(Node* node)
{
    if (!node)
        return;

    HTMLElement* element = static_cast<HTMLElement*>(node);
    if (element && element->isMediaElement()) {
        HTMLMediaElement* mediaElement = toHTMLMediaElement(element);
        if (mediaElement->player() && mediaElement->player()->canEnterFullscreen())
            mediaElement->player()->enterFullscreen();
    }
}

void ChromeClient::exitFullscreenForNode(Node* node)
{
    if (!node)
        return;

    HTMLElement* element = static_cast<HTMLElement*>(node);
    if (element && element->isMediaElement()) {
        HTMLMediaElement* mediaElement = toHTMLMediaElement(element);
        if (mediaElement->player())
            mediaElement->player()->exitFullscreen();
    }
}
#endif

#if ENABLE(FULLSCREEN_API)
bool ChromeClient::supportsFullScreenForElement(const WebCore::Element* element, bool withKeyboard)
{
    return !withKeyboard;
}

static gboolean onFullscreenGtkKeyPressEvent(GtkWidget* widget, GdkEventKey* event, ChromeClient* chromeClient)
{
    switch (event->keyval) {
    case GDK_KEY_Escape:
    case GDK_KEY_f:
    case GDK_KEY_F:
        chromeClient->cancelFullScreen();
        return TRUE;
    default:
        break;
    }

    return FALSE;
}

void ChromeClient::cancelFullScreen()
{
    ASSERT(m_fullScreenElement);
    m_fullScreenElement->document()->webkitCancelFullScreen();
}

void ChromeClient::enterFullScreenForElement(WebCore::Element* element)
{
    gboolean returnValue;
    GRefPtr<WebKitDOMHTMLElement> kitElement(adoptGRef(kit(reinterpret_cast<HTMLElement*>(element))));
    g_signal_emit_by_name(m_webView, "entering-fullscreen", kitElement.get(), &returnValue);
    if (returnValue)
        return;

#if ENABLE(VIDEO) && USE(NATIVE_FULLSCREEN_VIDEO)
    if (element && element->isMediaElement()) {
        HTMLMediaElement* mediaElement = toHTMLMediaElement(element);
        if (mediaElement->player() && mediaElement->player()->canEnterFullscreen()) {
            element->document()->webkitWillEnterFullScreenForElement(element);
            mediaElement->player()->enterFullscreen();
            m_fullScreenElement = element;
            element->document()->webkitDidEnterFullScreenForElement(element);
        }
        return;
    }
#endif

    GtkWidget* window = gtk_widget_get_toplevel(GTK_WIDGET(m_webView));
    if (!widgetIsOnscreenToplevelWindow(window))
        return;

    g_signal_connect(window, "key-press-event", G_CALLBACK(onFullscreenGtkKeyPressEvent), this);

    m_fullScreenElement = element;

    element->document()->webkitWillEnterFullScreenForElement(element);
    m_adjustmentWatcher.disableAllScrollbars();
    gtk_window_fullscreen(GTK_WINDOW(window));
    element->document()->webkitDidEnterFullScreenForElement(element);
}

void ChromeClient::exitFullScreenForElement(WebCore::Element*)
{
    // The element passed into this function is not reliable, i.e. it could
    // be null. In addition the parameter may be disappearing in the future.
    // So we use the reference to the element we saved above.
    ASSERT(m_fullScreenElement);

    gboolean returnValue;
    GRefPtr<WebKitDOMHTMLElement> kitElement(adoptGRef(kit(reinterpret_cast<HTMLElement*>(m_fullScreenElement.get()))));
    g_signal_emit_by_name(m_webView, "leaving-fullscreen", kitElement.get(), &returnValue);
    if (returnValue)
        return;

#if ENABLE(VIDEO) && USE(NATIVE_FULLSCREEN_VIDEO)
    if (m_fullScreenElement && m_fullScreenElement->isMediaElement()) {
        m_fullScreenElement->document()->webkitWillExitFullScreenForElement(m_fullScreenElement.get());
        HTMLMediaElement* mediaElement = toHTMLMediaElement(m_fullScreenElement.get());
        if (mediaElement->player()) {
            mediaElement->player()->exitFullscreen();
            m_fullScreenElement->document()->webkitDidExitFullScreenForElement(m_fullScreenElement.get());
            m_fullScreenElement.clear();
        }
        return;
    }
#endif

    GtkWidget* window = gtk_widget_get_toplevel(GTK_WIDGET(m_webView));
    ASSERT(widgetIsOnscreenToplevelWindow(window));
    g_signal_handlers_disconnect_by_func(window, reinterpret_cast<void*>(onFullscreenGtkKeyPressEvent), this);

    m_fullScreenElement->document()->webkitWillExitFullScreenForElement(m_fullScreenElement.get());
    gtk_window_unfullscreen(GTK_WINDOW(window));
    m_adjustmentWatcher.enableAllScrollbars();
    m_fullScreenElement->document()->webkitDidExitFullScreenForElement(m_fullScreenElement.get());
    m_fullScreenElement.clear();
}
#endif

#if USE(ACCELERATED_COMPOSITING)
void ChromeClient::attachRootGraphicsLayer(Frame* frame, GraphicsLayer* rootLayer)
{
    AcceleratedCompositingContext* context = m_webView->priv->acceleratedCompositingContext.get();
    bool turningOffCompositing = !rootLayer && context->enabled();
    bool turningOnCompositing = rootLayer && !context->enabled();

    context->setRootCompositingLayer(rootLayer);

    if (turningOnCompositing) {
        m_displayTimer.stop();
        m_webView->priv->backingStore = createBackingStore(GTK_WIDGET(m_webView), IntSize(1, 1));
    }

    if (turningOffCompositing) {
        m_webView->priv->backingStore = createBackingStore(GTK_WIDGET(m_webView), getWebViewRect(m_webView).size());
        RefPtr<cairo_t> cr = adoptRef(cairo_create(m_webView->priv->backingStore->cairoSurface()));
        clearEverywhereInBackingStore(m_webView, cr.get());
    }
}

void ChromeClient::setNeedsOneShotDrawingSynchronization()
{
    m_webView->priv->acceleratedCompositingContext->scheduleLayerFlush();
}

void ChromeClient::scheduleCompositingLayerFlush()
{
    m_webView->priv->acceleratedCompositingContext->scheduleLayerFlush();
}

ChromeClient::CompositingTriggerFlags ChromeClient::allowedCompositingTriggers() const
{
     if (!platformPageClient())
        return false;
#if USE(CLUTTER)
    // Currently, we only support CSS 3D Transforms.
    return ThreeDTransformTrigger | AnimationTrigger;
#else
    return AllTriggers;
#endif
}
#endif

}
