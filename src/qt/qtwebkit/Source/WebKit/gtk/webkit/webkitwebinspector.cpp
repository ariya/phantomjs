/*
 * Copyright (C) 2008 Gustavo Noronha Silva
 * Copyright (C) 2008, 2009 Holger Hans Peter Freyther
 * Copyright (C) 2009 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "webkitwebinspector.h"

#include "DumpRenderTreeSupportGtk.h"
#include "FocusController.h"
#include "Frame.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "InspectorClientGtk.h"
#include "InspectorController.h"
#include "InspectorInstrumentation.h"
#include "IntPoint.h"
#include "Page.h"
#include "RenderLayer.h"
#include "RenderView.h"
#include "WebKitDOMNodePrivate.h"
#include "webkitglobalsprivate.h"
#include "webkitmarshal.h"
#include "webkitwebinspectorprivate.h"
#include <glib/gi18n-lib.h>

/**
 * SECTION:webkitwebinspector
 * @short_description: Access to the WebKit Inspector
 *
 * The WebKit Inspector is a graphical tool to inspect and change
 * the content of a #WebKitWebView. It also includes an interactive
 * JavaScriptDebugger. Using this class one can get a GtkWidget which
 * can be embedded into an application to show the inspector.
 *
 * The inspector is available when the #WebKitWebSettings of the
 * #WebKitWebView has set the #WebKitWebSettings:enable-developer-extras
 * to true otherwise no inspector is available.
 *
 * <informalexample><programlisting>
 * /<!-- -->* Enable the developer extras *<!-- -->/
 * WebKitWebSettings *setting = webkit_web_view_get_settings (WEBKIT_WEB_VIEW(my_webview));
 * g_object_set (G_OBJECT(settings), "enable-developer-extras", TRUE, NULL);
 *
 * /<!-- -->* load some data or reload to be able to inspect the page*<!-- -->/
 * webkit_web_view_open (WEBKIT_WEB_VIEW(my_webview), "http://www.gnome.org");
 *
 * /<!-- -->* Embed the inspector somewhere *<!-- -->/
 * WebKitWebInspector *inspector = webkit_web_view_get_inspector (WEBKIT_WEB_VIEW(my_webview));
 * g_signal_connect (G_OBJECT (inspector), "inspect-web-view", G_CALLBACK(create_gtk_window_around_it), NULL);
 * g_signal_connect (G_OBJECT (inspector), "show-window", G_CALLBACK(show_inpector_window), NULL));
 * g_signal_connect (G_OBJECT (inspector), "notify::inspected-uri", G_CALLBACK(inspected_uri_changed_do_stuff), NULL);
 * </programlisting></informalexample>
 */

using namespace WebKit;
using namespace WebCore;

enum {
    INSPECT_WEB_VIEW,
    SHOW_WINDOW,
    ATTACH_WINDOW,
    DETACH_WINDOW,
    CLOSE_WINDOW,
    FINISHED,
    LAST_SIGNAL
};

static guint webkit_web_inspector_signals[LAST_SIGNAL] = { 0, };

enum {
    PROP_0,

    PROP_WEB_VIEW,
    PROP_INSPECTED_URI,
    PROP_JAVASCRIPT_PROFILING_ENABLED,
    PROP_TIMELINE_PROFILING_ENABLED
};

G_DEFINE_TYPE(WebKitWebInspector, webkit_web_inspector, G_TYPE_OBJECT)

struct _WebKitWebInspectorPrivate {
    WebCore::Page* page;
    WebKitWebView* inspector_view;
    gchar* inspected_uri;
};

static void webkit_web_inspector_finalize(GObject* object);

static void webkit_web_inspector_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);

static void webkit_web_inspector_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);

static gboolean webkit_inspect_web_view_request_handled(GSignalInvocationHint* ihint, GValue* returnAccu, const GValue* handlerReturn, gpointer dummy)
{
    gboolean continueEmission = TRUE;
    gpointer newWebView = g_value_get_object(handlerReturn);
    g_value_set_object(returnAccu, newWebView);

    if (newWebView)
        continueEmission = FALSE;

    return continueEmission;
}

static void webkit_web_inspector_class_init(WebKitWebInspectorClass* klass)
{
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = webkit_web_inspector_finalize;
    gobject_class->set_property = webkit_web_inspector_set_property;
    gobject_class->get_property = webkit_web_inspector_get_property;

    /**
     * WebKitWebInspector::inspect-web-view:
     * @web_inspector: the object on which the signal is emitted
     * @web_view: the #WebKitWebView which will be inspected
     *
     * Emitted when the user activates the 'inspect' context menu item
     * to inspect a web view. The application which is interested in
     * the inspector should create a window, or otherwise add the
     * #WebKitWebView it creates to an existing window.
     *
     * You don't need to handle the reference count of the
     * #WebKitWebView instance you create; the widget to which you add
     * it will do that.
     *
     * Return value: (transfer none): a newly allocated #WebKitWebView or %NULL
     *
     * Since: 1.0.3
     */
    webkit_web_inspector_signals[INSPECT_WEB_VIEW] = g_signal_new("inspect-web-view",
        G_TYPE_FROM_CLASS(klass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        webkit_inspect_web_view_request_handled,
        NULL,
        webkit_marshal_OBJECT__OBJECT,
        WEBKIT_TYPE_WEB_VIEW , 1,
        WEBKIT_TYPE_WEB_VIEW);

    /**
     * WebKitWebInspector::show-window:
     * @web_inspector: the object on which the signal is emitted
     * @return: %TRUE if the signal has been handled
     *
     * Emitted when the inspector window should be displayed. Notice
     * that the window must have been created already by handling
     * #WebKitWebInspector::inspect-web-view.
     *
     * Since: 1.0.3
     */
    webkit_web_inspector_signals[SHOW_WINDOW] = g_signal_new("show-window",
        G_TYPE_FROM_CLASS(klass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__VOID,
        G_TYPE_BOOLEAN , 0);

    /**
     * WebKitWebInspector::attach-window:
     * @web_inspector: the object on which the signal is emitted
     * @return: %TRUE if the signal has been handled
     *
     * Emitted when the inspector should appear at the same window as
     * the #WebKitWebView being inspected.
     *
     * Since: 1.0.3
     */
    webkit_web_inspector_signals[ATTACH_WINDOW] = g_signal_new("attach-window",
        G_TYPE_FROM_CLASS(klass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__VOID,
        G_TYPE_BOOLEAN , 0);

    /**
     * WebKitWebInspector::detach-window:
     * @web_inspector: the object on which the signal is emitted
     * @return: %TRUE if the signal has been handled
     *
     * Emitted when the inspector should appear in a separate window.
     *
     * Since: 1.0.3
     */
    webkit_web_inspector_signals[DETACH_WINDOW] = g_signal_new("detach-window",
        G_TYPE_FROM_CLASS(klass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__VOID,
        G_TYPE_BOOLEAN , 0);

    /**
     * WebKitWebInspector::close-window:
     * @web_inspector: the object on which the signal is emitted
     * @return: %TRUE if the signal has been handled
     *
     * Emitted when the inspector window should be closed. You can
     * destroy the window or hide it so that it can be displayed again
     * by handling #WebKitWebInspector::show-window later on.
     *
     * Notice that the inspected #WebKitWebView may no longer exist
     * when this signal is emitted.
     *
     * Notice, too, that if you decide to destroy the window,
     * #WebKitWebInspector::inspect-web-view will be emmited again, when the user
     * inspects an element.
     *
     * Since: 1.0.3
     */
    webkit_web_inspector_signals[CLOSE_WINDOW] = g_signal_new("close-window",
        G_TYPE_FROM_CLASS(klass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        g_signal_accumulator_true_handled,
        NULL,
        webkit_marshal_BOOLEAN__VOID,
        G_TYPE_BOOLEAN , 0);

    /**
     * WebKitWebInspector::finished:
     * @web_inspector: the object on which the signal is emitted
     *
     * Emitted when the inspection is done. You should release your
     * references on the inspector at this time. The inspected
     * #WebKitWebView may no longer exist when this signal is emitted.
     *
     * Since: 1.0.3
     */
    webkit_web_inspector_signals[FINISHED] = g_signal_new("finished",
        G_TYPE_FROM_CLASS(klass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE , 0);

    /*
     * properties
     */

    /**
     * WebKitWebInspector:web-view:
     *
     * The Web View that renders the Web Inspector itself.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(gobject_class, PROP_WEB_VIEW,
                                    g_param_spec_object("web-view",
                                                        _("Web View"),
                                                        _("The Web View that renders the Web Inspector itself"),
                                                        WEBKIT_TYPE_WEB_VIEW,
                                                        WEBKIT_PARAM_READABLE));

    /**
     * WebKitWebInspector:inspected-uri:
     *
     * The URI that is currently being inspected.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(gobject_class, PROP_INSPECTED_URI,
                                    g_param_spec_string("inspected-uri",
                                                        _("Inspected URI"),
                                                        _("The URI that is currently being inspected"),
                                                        NULL,
                                                        WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebInspector:javascript-profiling-enabled:
    *
    * This is enabling JavaScript profiling in the Inspector. This means
    * that Console.profiles will return the profiles.
    *
    * Since: 1.1.1
    */
    g_object_class_install_property(gobject_class,
                                    PROP_JAVASCRIPT_PROFILING_ENABLED,
                                    g_param_spec_boolean(
                                        "javascript-profiling-enabled",
                                        _("Enable JavaScript profiling"),
                                        _("Profile the executed JavaScript."),
                                        FALSE,
                                        WEBKIT_PARAM_READWRITE));

    /**
    * WebKitWebInspector:timeline-profiling-enabled:
    *
    * This is enabling Timeline profiling in the Inspector.
    *
    * Since: 1.1.17
    */
    g_object_class_install_property(gobject_class,
                                    PROP_TIMELINE_PROFILING_ENABLED,
                                    g_param_spec_boolean(
                                        "timeline-profiling-enabled",
                                        _("Enable Timeline profiling"),
                                        _("Profile the WebCore instrumentation."),
                                        FALSE,
                                        WEBKIT_PARAM_READWRITE));

    g_type_class_add_private(klass, sizeof(WebKitWebInspectorPrivate));
}

static void webkit_web_inspector_init(WebKitWebInspector* web_inspector)
{
    web_inspector->priv = G_TYPE_INSTANCE_GET_PRIVATE(web_inspector, WEBKIT_TYPE_WEB_INSPECTOR, WebKitWebInspectorPrivate);
}

static void webkit_web_inspector_finalize(GObject* object)
{
    WebKitWebInspector* web_inspector = WEBKIT_WEB_INSPECTOR(object);
    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    if (priv->inspector_view)
        g_object_unref(priv->inspector_view);

    if (priv->inspected_uri)
        g_free(priv->inspected_uri);

    G_OBJECT_CLASS(webkit_web_inspector_parent_class)->finalize(object);
}

static void webkit_web_inspector_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    WebKitWebInspector* web_inspector = WEBKIT_WEB_INSPECTOR(object);
    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    switch(prop_id) {
    case PROP_JAVASCRIPT_PROFILING_ENABLED: {
#if ENABLE(JAVASCRIPT_DEBUGGER)
        bool enabled = g_value_get_boolean(value);
        priv->page->inspectorController()->setProfilerEnabled(enabled);
#else
        g_message("PROP_JAVASCRIPT_PROFILING_ENABLED is not work because of the javascript debugger is disabled\n");
#endif
        break;
    }
    case PROP_TIMELINE_PROFILING_ENABLED: {
        g_message("PROP_TIMELINE_PROFILING_ENABLED has been deprecated\n");
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void webkit_web_inspector_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    WebKitWebInspector* web_inspector = WEBKIT_WEB_INSPECTOR(object);
    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    switch (prop_id) {
    case PROP_WEB_VIEW:
        g_value_set_object(value, priv->inspector_view);
        break;
    case PROP_INSPECTED_URI:
        g_value_set_string(value, priv->inspected_uri);
        break;
    case PROP_JAVASCRIPT_PROFILING_ENABLED:
#if ENABLE(JAVASCRIPT_DEBUGGER)
        g_value_set_boolean(value, priv->page->inspectorController()->profilerEnabled());
#else
        g_message("PROP_JAVASCRIPT_PROFILING_ENABLED is not work because of the javascript debugger is disabled\n");
#endif
        break;
    case PROP_TIMELINE_PROFILING_ENABLED:
        g_message("PROP_TIMELINE_PROFILING_ENABLED has been deprecated\n");
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

// internal use only
void webkit_web_inspector_set_web_view(WebKitWebInspector *web_inspector, WebKitWebView *web_view)
{
    g_return_if_fail(WEBKIT_IS_WEB_INSPECTOR(web_inspector));
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(web_view));

    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    if (priv->inspector_view)
        g_object_unref(priv->inspector_view);

    g_object_ref(web_view);
    priv->inspector_view = web_view;
}

/**
 * webkit_web_inspector_get_web_view:
 * @web_inspector: a #WebKitWebInspector
 *
 * Obtains the #WebKitWebView that is used to render the
 * inspector. The #WebKitWebView instance is created by the
 * application, by handling the #WebKitWebInspector::inspect-web-view signal. This means
 * that this method may return %NULL if the user hasn't inspected
 * anything.
 *
 * Returns: (transfer none): the #WebKitWebView instance that is used
 * to render the inspector or %NULL if it is not yet created.
 *
 * Since: 1.0.3
 **/
WebKitWebView* webkit_web_inspector_get_web_view(WebKitWebInspector *web_inspector)
{
    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    return priv->inspector_view;
}

// internal use only
void webkit_web_inspector_set_inspected_uri(WebKitWebInspector* web_inspector, const gchar* inspected_uri)
{
    g_return_if_fail(WEBKIT_IS_WEB_INSPECTOR(web_inspector));

    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    g_free(priv->inspected_uri);
    priv->inspected_uri = g_strdup(inspected_uri);
}

/**
 * webkit_web_inspector_get_inspected_uri:
 * @web_inspector: a #WebKitWebInspector
 *
 * Obtains the URI that is currently being inspected.
 *
 * Returns: a pointer to the URI as an internally allocated string; it
 * should not be freed, modified or stored.
 *
 * Since: 1.0.3
 **/
const gchar* webkit_web_inspector_get_inspected_uri(WebKitWebInspector *web_inspector)
{
    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    return priv->inspected_uri;
}

void
webkit_web_inspector_set_inspector_client(WebKitWebInspector* web_inspector, WebCore::Page* page)
{
    WebKitWebInspectorPrivate* priv = web_inspector->priv;

    priv->page = page;
}

/**
 * webkit_web_inspector_show:
 * @web_inspector: the #WebKitWebInspector that will be shown
 *
 * Causes the Web Inspector to be shown.
 *
 * Since: 1.1.17
 */
void webkit_web_inspector_show(WebKitWebInspector* webInspector)
{
    g_return_if_fail(WEBKIT_IS_WEB_INSPECTOR(webInspector));

    WebKitWebInspectorPrivate* priv = webInspector->priv;

    Frame* frame = priv->page->focusController()->focusedOrMainFrame();
    FrameView* view = frame->view();

    if (!view)
        return;

    priv->page->inspectorController()->show();
}

/**
 * webkit_web_inspector_inspect_node:
 * @web_inspector: the #WebKitWebInspector that will do the inspection
 * @node: the #WebKitDOMNode to inspect
 *
 * Causes the Web Inspector to inspect the given node.
 *
 * Since: 1.3.7
 */
void webkit_web_inspector_inspect_node(WebKitWebInspector* webInspector, WebKitDOMNode* node)
{
    g_return_if_fail(WEBKIT_IS_WEB_INSPECTOR(webInspector));
    g_return_if_fail(WEBKIT_DOM_IS_NODE(node));

    webInspector->priv->page->inspectorController()->inspect(core(node));
}

/**
 * webkit_web_inspector_inspect_coordinates:
 * @web_inspector: the #WebKitWebInspector that will do the inspection
 * @x: the X coordinate of the node to be inspected
 * @y: the Y coordinate of the node to be inspected
 *
 * Causes the Web Inspector to inspect the node that is located at the
 * given coordinates of the widget. The coordinates should be relative
 * to the #WebKitWebView widget, not to the scrollable content, and
 * may be obtained from a #GdkEvent directly.
 *
 * This means @x, and @y being zero doesn't guarantee you will hit the
 * left-most top corner of the content, since the contents may have
 * been scrolled.
 *
 * Since: 1.1.17
 */
void webkit_web_inspector_inspect_coordinates(WebKitWebInspector* webInspector, gdouble x, gdouble y)
{
    g_return_if_fail(WEBKIT_IS_WEB_INSPECTOR(webInspector));
    g_return_if_fail(x >= 0 && y >= 0);

    WebKitWebInspectorPrivate* priv = webInspector->priv;

    Frame* frame = priv->page->focusController()->focusedOrMainFrame();
    FrameView* view = frame->view();

    if (!view)
        return;

    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::DisallowShadowContent);
    IntPoint documentPoint = view->windowToContents(IntPoint(static_cast<int>(x), static_cast<int>(y)));
    HitTestResult result(documentPoint);

    frame->contentRenderer()->layer()->hitTest(request, result);
    priv->page->inspectorController()->inspect(result.innerNonSharedNode());
}

/**
 * webkit_web_inspector_close:
 * @web_inspector: the #WebKitWebInspector that will be closed
 *
 * Causes the Web Inspector to be closed.
 *
 * Since: 1.1.17
 */
void webkit_web_inspector_close(WebKitWebInspector* webInspector)
{
    g_return_if_fail(WEBKIT_IS_WEB_INSPECTOR(webInspector));

    WebKitWebInspectorPrivate* priv = webInspector->priv;
    priv->page->inspectorController()->close();
}

void webkit_web_inspector_execute_script(WebKitWebInspector* webInspector, long callId, const gchar* script)
{
    g_return_if_fail(WEBKIT_IS_WEB_INSPECTOR(webInspector));
    g_return_if_fail(script);

    WebKitWebInspectorPrivate* priv = webInspector->priv;
    priv->page->inspectorController()->evaluateForTestInFrontend(callId, script);
}
