/*
 * Copyright (C) 2007, 2008 Holger Hans Peter Freyther
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2007 Apple Inc.
 * Copyright (C) 2008 Christian Dywan <christian@imendio.com>
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Alonzo <jmalonzo@gmail.com>
 * Copyright (C) 2009 Gustavo Noronha Silva <gns@gnome.org>
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
#include "webkitwebframe.h"

#include "AXObjectCache.h"
#include "AnimationController.h"
#include "DOMObjectCache.h"
#include "DocumentFragment.h"
#include "DocumentLoader.h"
#include "DocumentLoaderGtk.h"
#include "Editor.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameLoaderClientGtk.h"
#include "FrameSelection.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "GCController.h"
#include "GraphicsContext.h"
#include "GtkUtilities.h"
#include "GtkVersioning.h"
#include "HTMLFrameOwnerElement.h"
#include "JSDOMBinding.h"
#include "JSDOMWindow.h"
#include "JSElement.h"
#include "PlatformContextCairo.h"
#include "PrintContext.h"
#include "RenderListItem.h"
#include "RenderTreeAsText.h"
#include "RenderView.h"
#include "ScriptController.h"
#include "SubstituteData.h"
#include "TextIterator.h"
#include "WebKitAccessibleWrapperAtk.h"
#include "WebKitDOMDocumentPrivate.h"
#include "WebKitDOMRangePrivate.h"
#include "markup.h"
#include "webkitenumtypes.h"
#include "webkitglobalsprivate.h"
#include "webkitmarshal.h"
#include "webkitnetworkresponse.h"
#include "webkitnetworkrequestprivate.h"
#include "webkitnetworkresponseprivate.h"
#include "webkitsecurityoriginprivate.h"
#include "webkitwebframeprivate.h"
#include "webkitwebresource.h"
#include "webkitwebview.h"
#include "webkitwebviewprivate.h"
#include <JavaScriptCore/APICast.h>
#include <atk/atk.h>
#include <glib/gi18n-lib.h>
#include <wtf/text/CString.h>

#if ENABLE(SVG)
#include "SVGSMILElement.h"
#endif

/**
 * SECTION:webkitwebframe
 * @short_description: The content of a #WebKitWebView
 *
 * A #WebKitWebView contains a main #WebKitWebFrame. A #WebKitWebFrame
 * contains the content of one URI. The URI and name of the frame can
 * be retrieved, the load status and progress can be observed using the
 * signals and can be controlled using the methods of the #WebKitWebFrame.
 * A #WebKitWebFrame can have any number of children and one child can
 * be found by using #webkit_web_frame_find_frame.
 *
 * <informalexample><programlisting>
 * /<!-- -->* Get the frame from the #WebKitWebView *<!-- -->/
 * WebKitWebFrame *frame = webkit_web_view_get_main_frame (WEBKIT_WEB_VIEW(my_view));
 * g_print ("The URI of this frame is '%s'", webkit_web_frame_get_uri (frame));
 * </programlisting></informalexample>
 */

using namespace WebKit;
using namespace WebCore;
using namespace std;

enum {
    CLEARED,
    LOAD_COMMITTED,
    LOAD_DONE,
    TITLE_CHANGED,
    HOVERING_OVER_LINK,
    SCROLLBARS_POLICY_CHANGED,
    // Resource loading signals
    RESOURCE_REQUEST_STARTING,
    RESOURCE_RESPONSE_RECEIVED,
    RESOURCE_LOAD_FINISHED,
    RESOURCE_CONTENT_LENGTH_RECEIVED,
    RESOURCE_LOAD_FAILED,
    INSECURE_CONTENT_RUN,

    LAST_SIGNAL
};

enum {
    PROP_0,

    PROP_NAME,
    PROP_TITLE,
    PROP_URI,
    PROP_LOAD_STATUS,
    PROP_HORIZONTAL_SCROLLBAR_POLICY,
    PROP_VERTICAL_SCROLLBAR_POLICY
};

static guint webkit_web_frame_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE(WebKitWebFrame, webkit_web_frame, G_TYPE_OBJECT)

static void webkit_web_frame_get_property(GObject* object, guint propertyId, GValue* value, GParamSpec* paramSpec)
{
    WebKitWebFrame* frame = WEBKIT_WEB_FRAME(object);

    switch (propertyId) {
    case PROP_NAME:
        g_value_set_string(value, webkit_web_frame_get_name(frame));
        break;
    case PROP_TITLE:
        g_value_set_string(value, webkit_web_frame_get_title(frame));
        break;
    case PROP_URI:
        g_value_set_string(value, webkit_web_frame_get_uri(frame));
        break;
    case PROP_LOAD_STATUS:
        g_value_set_enum(value, webkit_web_frame_get_load_status(frame));
        break;
    case PROP_HORIZONTAL_SCROLLBAR_POLICY:
        g_value_set_enum(value, webkit_web_frame_get_horizontal_scrollbar_policy(frame));
        break;
    case PROP_VERTICAL_SCROLLBAR_POLICY:
        g_value_set_enum(value, webkit_web_frame_get_vertical_scrollbar_policy(frame));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyId, paramSpec);
        break;
    }
}

// Called from the FrameLoaderClient when it is destroyed. Normally
// the unref in the FrameLoaderClient is destroying this object as
// well but due reference counting a user might have added a reference...
void webkit_web_frame_core_frame_gone(WebKitWebFrame* frame)
{
    ASSERT(WEBKIT_IS_WEB_FRAME(frame));
    WebKitWebFramePrivate* priv = frame->priv;
    if (priv->coreFrame)
        DOMObjectCache::clearByFrame(priv->coreFrame);
    priv->coreFrame = 0;
}

static WebKitWebDataSource* webkit_web_frame_get_data_source_from_core_loader(WebCore::DocumentLoader* loader)
{
    return loader ? static_cast<WebKit::DocumentLoader*>(loader)->dataSource() : 0;
}

static void webkit_web_frame_finalize(GObject* object)
{
    WebKitWebFrame* frame = WEBKIT_WEB_FRAME(object);
    WebKitWebFramePrivate* priv = frame->priv;

    if (priv->coreFrame) {
        DOMObjectCache::clearByFrame(priv->coreFrame);
        priv->coreFrame->loader()->cancelAndClear();
        priv->coreFrame = 0;
    }

    g_free(priv->name);
    g_free(priv->title);
    g_free(priv->uri);

    G_OBJECT_CLASS(webkit_web_frame_parent_class)->finalize(object);
}

static void webkit_web_frame_class_init(WebKitWebFrameClass* frameClass)
{
    webkitInit();

    /*
     * signals
     */
    webkit_web_frame_signals[CLEARED] = g_signal_new("cleared",
        G_TYPE_FROM_CLASS(frameClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        0,
        0,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    /**
     * WebKitWebFrame::load-committed:
     * @web_frame: the object on which the signal is emitted
     *
     * Emitted when frame loading is done.
     *
     * Deprecated: Use the "load-status" property instead.
     */
    webkit_web_frame_signals[LOAD_COMMITTED] = g_signal_new("load-committed",
        G_TYPE_FROM_CLASS(frameClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        0,
        0,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    /**
     * WebKitWebFrame::title-changed:
     * @frame: the object on which the signal is emitted
     * @title: the new title
     *
     * When a #WebKitWebFrame changes the document title this signal is emitted.
     *
     * Deprecated: 1.1.18: Use "notify::title" instead.
     */
    webkit_web_frame_signals[TITLE_CHANGED] = g_signal_new("title-changed",
        G_TYPE_FROM_CLASS(frameClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        0,
        0,
        webkit_marshal_VOID__STRING,
        G_TYPE_NONE, 1,
        G_TYPE_STRING);

    webkit_web_frame_signals[HOVERING_OVER_LINK] = g_signal_new("hovering-over-link",
        G_TYPE_FROM_CLASS(frameClass),
        (GSignalFlags)G_SIGNAL_RUN_LAST,
        0,
        0,
        0,
        webkit_marshal_VOID__STRING_STRING,
        G_TYPE_NONE, 2,
        G_TYPE_STRING, G_TYPE_STRING);

    /**
     * WebKitWebFrame::scrollbars-policy-changed:
     * @web_view: the object which received the signal
     *
     * Signal emitted when policy for one or both of the scrollbars of
     * the view has changed. The default handler will apply the new
     * policy to the container that holds the #WebKitWebFrame if it is
     * a #GtkScrolledWindow and the frame is the main frame. If you do
     * not want this to be handled automatically, you need to handle
     * this signal.
     *
     * The exception to this rule is that policies to disable the
     * scrollbars are applied as %GTK_POLICY_AUTOMATIC instead, since
     * the size request of the widget would force browser windows to
     * not be resizable.
     *
     * You can obtain the new policies from the
     * WebKitWebFrame:horizontal-scrollbar-policy and
     * WebKitWebFrame:vertical-scrollbar-policy properties.
     *
     * Return value: %TRUE to stop other handlers from being invoked for the
     * event. %FALSE to propagate the event further.
     *
     * Since: 1.1.14
     */
    webkit_web_frame_signals[SCROLLBARS_POLICY_CHANGED] = g_signal_new("scrollbars-policy-changed",
        G_TYPE_FROM_CLASS(frameClass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        g_signal_accumulator_true_handled,
        0,
        webkit_marshal_BOOLEAN__VOID,
        G_TYPE_BOOLEAN, 0);


    /**
     * WebKitWebFrame::resource-request-starting:
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
     * the lifetime of the resource, but the contents may change
     * between signal emissions.
     *
     * Since: 1.7.5
     */
    webkit_web_frame_signals[RESOURCE_REQUEST_STARTING] = g_signal_new("resource-request-starting",
        G_TYPE_FROM_CLASS(frameClass),
        G_SIGNAL_RUN_LAST,
        0,
        0, 0,
        webkit_marshal_VOID__OBJECT_OBJECT_OBJECT,
        G_TYPE_NONE, 3,
        WEBKIT_TYPE_WEB_RESOURCE,
        WEBKIT_TYPE_NETWORK_REQUEST,
        WEBKIT_TYPE_NETWORK_RESPONSE);

    /**
     * WebKitWebFrame::resource-response-received:
     * @web_frame: the #WebKitWebFrame the response was received for
     * @web_resource: the #WebKitWebResource being loaded
     * @response: the #WebKitNetworkResponse that was received.
     *
     * Emitted when the response is received from the server.
     *
     * Since: 1.7.5
     */
    webkit_web_frame_signals[RESOURCE_RESPONSE_RECEIVED] = g_signal_new("resource-response-received",
        G_TYPE_FROM_CLASS(frameClass),
        G_SIGNAL_RUN_LAST,
        0,
        0, 0,
        webkit_marshal_VOID__OBJECT_OBJECT,
        G_TYPE_NONE, 2,
        WEBKIT_TYPE_WEB_RESOURCE,
        WEBKIT_TYPE_NETWORK_RESPONSE);

    /**
     * WebKitWebFrame::resource-load-finished:
     * @web_frame: the #WebKitWebFrame the response was received for
     * @web_resource: the #WebKitWebResource being loaded
     *
     * Emitted when all the data for the resource was loaded.
     *
     * Since: 1.7.5
     */
    webkit_web_frame_signals[RESOURCE_LOAD_FINISHED] = g_signal_new("resource-load-finished",
        G_TYPE_FROM_CLASS(frameClass),
        G_SIGNAL_RUN_LAST,
        0,
        0, 0,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        WEBKIT_TYPE_WEB_RESOURCE);

    /**
     * WebKitWebFrame::resource-content-length-received:
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
    webkit_web_frame_signals[RESOURCE_CONTENT_LENGTH_RECEIVED] = g_signal_new("resource-content-length-received",
        G_TYPE_FROM_CLASS(frameClass),
        G_SIGNAL_RUN_LAST,
        0,
        0, 0,
        webkit_marshal_VOID__OBJECT_INT,
        G_TYPE_NONE, 2,
        WEBKIT_TYPE_WEB_RESOURCE,
        G_TYPE_INT);

    /**
     * WebKitWebFrame::resource-load-failed:
     * @web_frame: the #WebKitWebFrame the response was received for
     * @web_resource: the #WebKitWebResource that was loaded
     * @error: the #GError that was triggered
     *
     * Invoked when a resource failed to load.
     *
     * Since: 1.7.5
     */
    webkit_web_frame_signals[RESOURCE_LOAD_FAILED] = g_signal_new("resource-load-failed",
        G_TYPE_FROM_CLASS(frameClass),
        G_SIGNAL_RUN_LAST,
        0,
        0, 0,
        webkit_marshal_VOID__OBJECT_POINTER,
        G_TYPE_NONE, 2,
        WEBKIT_TYPE_WEB_RESOURCE,
        G_TYPE_POINTER);

    /**
     * WebKitWebFrame::insecure-content-run:
     * @web_frame: the #WebKitWebFrame the response was received for.
     * @security_origin: the #WebKitSecurityOrigin.
     * @url: the url of the insecure content.
     *
     * Invoked when insecure content is run from a secure page. This happens
     * when a page loaded via HTTPS loads a stylesheet, script, image or
     * iframe from an unencrypted HTTP URL.
     *
     * Since: 1.10.0
     */
    webkit_web_frame_signals[INSECURE_CONTENT_RUN] = g_signal_new("insecure-content-run",
        G_TYPE_FROM_CLASS(frameClass),
        G_SIGNAL_RUN_LAST,
        0,
        0, 0,
        webkit_marshal_VOID__OBJECT_STRING,
        G_TYPE_NONE, 2,
        WEBKIT_TYPE_SECURITY_ORIGIN,
        G_TYPE_STRING);

    /*
     * implementations of virtual methods
     */
    GObjectClass* objectClass = G_OBJECT_CLASS(frameClass);
    objectClass->finalize = webkit_web_frame_finalize;
    objectClass->get_property = webkit_web_frame_get_property;

    /*
     * properties
     */
    g_object_class_install_property(objectClass, PROP_NAME,
                                    g_param_spec_string("name",
                                                        _("Name"),
                                                        _("The name of the frame"),
                                                        0,
                                                        WEBKIT_PARAM_READABLE));

    g_object_class_install_property(objectClass, PROP_TITLE,
                                    g_param_spec_string("title",
                                                        _("Title"),
                                                        _("The document title of the frame"),
                                                        0,
                                                        WEBKIT_PARAM_READABLE));

    g_object_class_install_property(objectClass, PROP_URI,
                                    g_param_spec_string("uri",
                                                        _("URI"),
                                                        _("The current URI of the contents displayed by the frame"),
                                                        0,
                                                        WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebFrame:load-status:
    *
    * Determines the current status of the load.
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
     * WebKitWebFrame:horizontal-scrollbar-policy:
     *
     * Determines the current policy for the horizontal scrollbar of
     * the frame. For the main frame, make sure to set the same policy
     * on the scrollable widget containing the #WebKitWebView, unless
     * you know what you are doing.
     *
     * Since: 1.1.14
     */
    g_object_class_install_property(objectClass, PROP_HORIZONTAL_SCROLLBAR_POLICY,
                                    g_param_spec_enum("horizontal-scrollbar-policy",
                                                      _("Horizontal Scrollbar Policy"),
                                                      _("Determines the current policy for the horizontal scrollbar of the frame."),
                                                      GTK_TYPE_POLICY_TYPE,
                                                      GTK_POLICY_AUTOMATIC,
                                                      WEBKIT_PARAM_READABLE));

    /**
     * WebKitWebFrame:vertical-scrollbar-policy:
     *
     * Determines the current policy for the vertical scrollbar of
     * the frame. For the main frame, make sure to set the same policy
     * on the scrollable widget containing the #WebKitWebView, unless
     * you know what you are doing.
     *
     * Since: 1.1.14
     */
    g_object_class_install_property(objectClass, PROP_VERTICAL_SCROLLBAR_POLICY,
                                    g_param_spec_enum("vertical-scrollbar-policy",
                                                      _("Vertical Scrollbar Policy"),
                                                      _("Determines the current policy for the vertical scrollbar of the frame."),
                                                      GTK_TYPE_POLICY_TYPE,
                                                      GTK_POLICY_AUTOMATIC,
                                                      WEBKIT_PARAM_READABLE));

    g_type_class_add_private(frameClass, sizeof(WebKitWebFramePrivate));
}

static void webkit_web_frame_init(WebKitWebFrame* frame)
{
    WebKitWebFramePrivate* priv = G_TYPE_INSTANCE_GET_PRIVATE(frame, WEBKIT_TYPE_WEB_FRAME, WebKitWebFramePrivate);

    // TODO: Move constructor code here.
    frame->priv = priv;
}


/**
 * webkit_web_frame_new:
 * @web_view: the controlling #WebKitWebView
 *
 * Creates a new #WebKitWebFrame initialized with a controlling #WebKitWebView.
 *
 * Returns: a new #WebKitWebFrame
 *
 * Deprecated: 1.0.2: #WebKitWebFrame can only be used to inspect existing
 * frames.
 **/
WebKitWebFrame* webkit_web_frame_new(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);

    WebKitWebFrame* frame = WEBKIT_WEB_FRAME(g_object_new(WEBKIT_TYPE_WEB_FRAME, NULL));
    WebKitWebFramePrivate* priv = frame->priv;
    WebKitWebViewPrivate* viewPriv = webView->priv;

    priv->webView = webView;
    WebKit::FrameLoaderClient* client = new WebKit::FrameLoaderClient(frame);
    priv->coreFrame = Frame::create(viewPriv->corePage, 0, client).get();
    priv->coreFrame->init();

    priv->origin = 0;

    return frame;
}

/**
 * webkit_web_frame_get_title:
 * @frame: a #WebKitWebFrame
 *
 * Returns the @frame's document title
 *
 * Return value: the title of @frame
 */
const gchar* webkit_web_frame_get_title(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);

    WebKitWebFramePrivate* priv = frame->priv;
    return priv->title;
}

/**
 * webkit_web_frame_get_uri:
 * @frame: a #WebKitWebFrame
 *
 * Returns the current URI of the contents displayed by the @frame
 *
 * Return value: the URI of @frame
 */
const gchar* webkit_web_frame_get_uri(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);

    WebKitWebFramePrivate* priv = frame->priv;
    return priv->uri;
}

/**
 * webkit_web_frame_get_web_view:
 * @frame: a #WebKitWebFrame
 *
 * Returns the #WebKitWebView that manages this #WebKitWebFrame.
 *
 * The #WebKitWebView returned manages the entire hierarchy of #WebKitWebFrame
 * objects that contains @frame.
 *
 * Return value: (transfer none): the #WebKitWebView that manages @frame
 */
WebKitWebView* webkit_web_frame_get_web_view(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);

    WebKitWebFramePrivate* priv = frame->priv;
    return priv->webView;
}

/**
 * webkit_web_frame_get_name:
 * @frame: a #WebKitWebFrame
 *
 * Returns the @frame's name
 *
 * Return value: the name of @frame. This method will return NULL if
 * the #WebKitWebFrame is invalid or an empty string if it is not backed
 * by a live WebCore frame.
 */
const gchar* webkit_web_frame_get_name(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);
    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return "";

    WebKitWebFramePrivate* priv = frame->priv;
    CString frameName = coreFrame->tree()->uniqueName().string().utf8();
    if (!g_strcmp0(frameName.data(), priv->name))
        return priv->name;

    g_free(priv->name);
    priv->name = g_strdup(frameName.data());
    return priv->name;
}

/**
 * webkit_web_frame_get_parent:
 * @frame: a #WebKitWebFrame
 *
 * Returns the @frame's parent frame, or %NULL if it has none.
 *
 * Return value: (transfer none): the parent #WebKitWebFrame or %NULL in case there is none
 */
WebKitWebFrame* webkit_web_frame_get_parent(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);

    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return 0;

    return kit(coreFrame->tree()->parent());
}

/**
 * webkit_web_frame_load_uri:
 * @frame: a #WebKitWebFrame
 * @uri: an URI string
 *
 * Requests loading of the specified URI string.
 *
 * Since: 1.1.1
 */
void webkit_web_frame_load_uri(WebKitWebFrame* frame, const gchar* uri)
{
    g_return_if_fail(WEBKIT_IS_WEB_FRAME(frame));
    g_return_if_fail(uri);

    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return;

    coreFrame->loader()->load(FrameLoadRequest(coreFrame, ResourceRequest(KURL(KURL(), String::fromUTF8(uri)))));
}

static void webkit_web_frame_load_data(WebKitWebFrame* frame, const gchar* content, const gchar* mimeType, const gchar* encoding, const gchar* baseURL, const gchar* unreachableURL)
{
    Frame* coreFrame = core(frame);
    ASSERT(coreFrame);

    KURL baseKURL = baseURL ? KURL(KURL(), String::fromUTF8(baseURL)) : blankURL();

    ResourceRequest request(baseKURL);

    RefPtr<SharedBuffer> sharedBuffer = SharedBuffer::create(content, strlen(content));
    SubstituteData substituteData(sharedBuffer.release(),
                                  mimeType ? String::fromUTF8(mimeType) : String::fromUTF8("text/html"),
                                  encoding ? String::fromUTF8(encoding) : String::fromUTF8("UTF-8"),
                                  KURL(KURL(), String::fromUTF8(unreachableURL)),
                                  KURL(KURL(), String::fromUTF8(unreachableURL)));

    coreFrame->loader()->load(FrameLoadRequest(coreFrame, request, substituteData));
}

/**
 * webkit_web_frame_load_string:
 * @frame: a #WebKitWebFrame
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
 *
 * Since: 1.1.1
 */
void webkit_web_frame_load_string(WebKitWebFrame* frame, const gchar* content, const gchar* contentMimeType, const gchar* contentEncoding, const gchar* baseUri)
{
    g_return_if_fail(WEBKIT_IS_WEB_FRAME(frame));
    g_return_if_fail(content);

    webkit_web_frame_load_data(frame, content, contentMimeType, contentEncoding, baseUri, 0);
}

/**
 * webkit_web_frame_load_alternate_string:
 * @frame: a #WebKitWebFrame
 * @content: the alternate content to display as the main page of the @frame
 * @base_url: the base URI for relative locations
 * @unreachable_url: the URL for the alternate page content
 *
 * Request loading of an alternate content for a URL that is unreachable.
 * Using this method will preserve the back-forward list. The URI passed in
 * @base_url has to be an absolute URI.
 *
 * Since: 1.1.6
 */
void webkit_web_frame_load_alternate_string(WebKitWebFrame* frame, const gchar* content, const gchar* baseURL, const gchar* unreachableURL)
{
    g_return_if_fail(WEBKIT_IS_WEB_FRAME(frame));
    g_return_if_fail(content);

    webkit_web_frame_load_data(frame, content, 0, 0, baseURL, unreachableURL);
}

/**
 * webkit_web_frame_load_request:
 * @frame: a #WebKitWebFrame
 * @request: a #WebKitNetworkRequest
 *
 * Connects to a given URI by initiating an asynchronous client request.
 *
 * Creates a provisional data source that will transition to a committed data
 * source once any data has been received. Use webkit_web_frame_stop_loading() to
 * stop the load. This function is typically invoked on the main frame.
 */
void webkit_web_frame_load_request(WebKitWebFrame* frame, WebKitNetworkRequest* request)
{
    g_return_if_fail(WEBKIT_IS_WEB_FRAME(frame));
    g_return_if_fail(WEBKIT_IS_NETWORK_REQUEST(request));

    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return;

    coreFrame->loader()->load(FrameLoadRequest(coreFrame->document()->securityOrigin(), core(request)));
}

/**
 * webkit_web_frame_stop_loading:
 * @frame: a #WebKitWebFrame
 *
 * Stops any pending loads on @frame's data source, and those of its children.
 */
void webkit_web_frame_stop_loading(WebKitWebFrame* frame)
{
    g_return_if_fail(WEBKIT_IS_WEB_FRAME(frame));

    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return;

    coreFrame->loader()->stopAllLoaders();
}

/**
 * webkit_web_frame_reload:
 * @frame: a #WebKitWebFrame
 *
 * Reloads the initial request.
 */
void webkit_web_frame_reload(WebKitWebFrame* frame)
{
    g_return_if_fail(WEBKIT_IS_WEB_FRAME(frame));

    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return;

    coreFrame->loader()->reload();
}

/**
 * webkit_web_frame_find_frame:
 * @frame: a #WebKitWebFrame
 * @name: the name of the frame to be found
 *
 * For pre-defined names, returns @frame if @name is "_self" or "_current",
 * returns @frame's parent frame if @name is "_parent", and returns the main
 * frame if @name is "_top". Also returns @frame if it is the main frame and
 * @name is either "_parent" or "_top". For other names, this function returns
 * the first frame that matches @name. This function searches @frame and its
 * descendents first, then @frame's parent and its children moving up the
 * hierarchy until a match is found. If no match is found in @frame's
 * hierarchy, this function will search for a matching frame in other main
 * frame hierarchies. Returns %NULL if no match is found.
 *
 * Return value: (transfer none): the found #WebKitWebFrame or %NULL in case none is found
 */
WebKitWebFrame* webkit_web_frame_find_frame(WebKitWebFrame* frame, const gchar* name)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);
    g_return_val_if_fail(name, 0);

    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return 0;

    String nameString = String::fromUTF8(name);
    return kit(coreFrame->tree()->find(AtomicString(nameString)));
}

/**
 * webkit_web_frame_get_global_context:
 * @frame: a #WebKitWebFrame
 *
 * Gets the global JavaScript execution context. Use this function to bridge
 * between the WebKit and JavaScriptCore APIs.
 *
 * Return value: (transfer none): the global JavaScript context #JSGlobalContextRef
 */
JSGlobalContextRef webkit_web_frame_get_global_context(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);

    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return 0;

    return toGlobalRef(coreFrame->script()->globalObject(mainThreadNormalWorld())->globalExec());
}

/**
 * webkit_web_frame_get_data_source:
 * @frame: a #WebKitWebFrame
 *
 * Returns the committed data source.
 *
 * Return value: (transfer none): the committed #WebKitWebDataSource.
 *
 * Since: 1.1.14
 */
WebKitWebDataSource* webkit_web_frame_get_data_source(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);

    Frame* coreFrame = core(frame);
    return webkit_web_frame_get_data_source_from_core_loader(coreFrame->loader()->documentLoader());
}

/**
 * webkit_web_frame_get_provisional_data_source:
 * @frame: a #WebKitWebFrame
 *
 * You use the webkit_web_frame_load_request method to initiate a request that
 * creates a provisional data source. The provisional data source will
 * transition to a committed data source once any data has been received. Use
 * webkit_web_frame_get_data_source to get the committed data source.
 *
 * Return value: (transfer none): the provisional #WebKitWebDataSource or %NULL if a load
 * request is not in progress.
 *
 * Since: 1.1.14
 */
WebKitWebDataSource* webkit_web_frame_get_provisional_data_source(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);

    Frame* coreFrame = core(frame);
    return webkit_web_frame_get_data_source_from_core_loader(coreFrame->loader()->provisionalDocumentLoader());
}

static void begin_print_callback(GtkPrintOperation* op, GtkPrintContext* context, gpointer user_data)
{
    PrintContext* printContext = reinterpret_cast<PrintContext*>(user_data);

    float width = gtk_print_context_get_width(context);
    float height = gtk_print_context_get_height(context);
    FloatRect printRect = FloatRect(0, 0, width, height);

    printContext->begin(width);

    // TODO: Margin adjustments and header/footer support
    float headerHeight = 0;
    float footerHeight = 0;
    float pageHeight; // height of the page adjusted by margins
    printContext->computePageRects(printRect, headerHeight, footerHeight, 1.0, pageHeight);
    gtk_print_operation_set_n_pages(op, printContext->pageCount());
}

static void draw_page_callback(GtkPrintOperation*, GtkPrintContext* gtkPrintContext, gint pageNumber, PrintContext* corePrintContext)
{
    if (pageNumber >= static_cast<gint>(corePrintContext->pageCount()))
        return;

    cairo_t* cr = gtk_print_context_get_cairo_context(gtkPrintContext);
    float pageWidth = gtk_print_context_get_width(gtkPrintContext);

    PlatformContextCairo platformContext(cr);
    GraphicsContext graphicsContext(&platformContext);
    corePrintContext->spoolPage(graphicsContext, pageNumber, pageWidth);
}

static void end_print_callback(GtkPrintOperation* op, GtkPrintContext* context, gpointer user_data)
{
    PrintContext* printContext = reinterpret_cast<PrintContext*>(user_data);
    printContext->end();
}

/**
 * webkit_web_frame_print_full:
 * @frame: a #WebKitWebFrame to be printed
 * @operation: the #GtkPrintOperation to be carried
 * @action: the #GtkPrintOperationAction to be performed
 * @error: #GError for error return
 *
 * Prints the given #WebKitWebFrame, using the given #GtkPrintOperation
 * and #GtkPrintOperationAction. This function wraps a call to
 * gtk_print_operation_run() for printing the contents of the
 * #WebKitWebFrame.
 *
 * Returns: The #GtkPrintOperationResult specifying the result of this operation.
 *
 * Since: 1.1.5
 */
GtkPrintOperationResult webkit_web_frame_print_full(WebKitWebFrame* frame, GtkPrintOperation* operation, GtkPrintOperationAction action, GError** error)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), GTK_PRINT_OPERATION_RESULT_ERROR);
    g_return_val_if_fail(GTK_IS_PRINT_OPERATION(operation), GTK_PRINT_OPERATION_RESULT_ERROR);

    GtkWidget* topLevel = gtk_widget_get_toplevel(GTK_WIDGET(webkit_web_frame_get_web_view(frame)));
    if (!widgetIsOnscreenToplevelWindow(topLevel))
        topLevel = 0;

    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return GTK_PRINT_OPERATION_RESULT_ERROR;

    PrintContext printContext(coreFrame);

    g_signal_connect(operation, "begin-print", G_CALLBACK(begin_print_callback), &printContext);
    g_signal_connect(operation, "draw-page", G_CALLBACK(draw_page_callback), &printContext);
    g_signal_connect(operation, "end-print", G_CALLBACK(end_print_callback), &printContext);

    return gtk_print_operation_run(operation, action, GTK_WINDOW(topLevel), error);
}

/**
 * webkit_web_frame_print:
 * @frame: a #WebKitWebFrame
 *
 * Prints the given #WebKitWebFrame, by presenting a print dialog to the
 * user. If you need more control over the printing process, see
 * webkit_web_frame_print_full().
 *
 * Since: 1.1.5
 */
void webkit_web_frame_print(WebKitWebFrame* frame)
{
    g_return_if_fail(WEBKIT_IS_WEB_FRAME(frame));

    WebKitWebFramePrivate* priv = frame->priv;
    GtkPrintOperation* operation = gtk_print_operation_new();
    GError* error = 0;

    webkit_web_frame_print_full(frame, operation, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, &error);
    g_object_unref(operation);

    if (error) {
        GtkWidget* window = gtk_widget_get_toplevel(GTK_WIDGET(priv->webView));
        GtkWidget* dialog = gtk_message_dialog_new(widgetIsOnscreenToplevelWindow(window) ? GTK_WINDOW(window) : 0,
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "%s", error->message);

        g_error_free(error);

        g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
        gtk_widget_show(dialog);
    }
}

gchar* webkit_web_frame_get_response_mime_type(WebKitWebFrame* frame)
{
    Frame* coreFrame = core(frame);
    WebCore::DocumentLoader* docLoader = coreFrame->loader()->documentLoader();
    String mimeType = docLoader->responseMIMEType();
    return g_strdup(mimeType.utf8().data());
}

/**
 * webkit_web_frame_get_load_status:
 * @frame: a #WebKitWebView
 *
 * Determines the current status of the load.
 *
 * Returns: The #WebKitLoadStatus specifying the status of the current load.
 *
 * Since: 1.1.7
 */
WebKitLoadStatus webkit_web_frame_get_load_status(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), WEBKIT_LOAD_FINISHED);

    WebKitWebFramePrivate* priv = frame->priv;
    return priv->loadStatus;
}

GtkPolicyType webkit_web_frame_get_horizontal_scrollbar_policy(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), GTK_POLICY_AUTOMATIC);

    Frame* coreFrame = core(frame);
    FrameView* view = coreFrame->view();
    if (!view)
        return GTK_POLICY_AUTOMATIC;

    ScrollbarMode hMode = view->horizontalScrollbarMode();

    if (hMode == ScrollbarAlwaysOn)
        return GTK_POLICY_ALWAYS;

    if (hMode == ScrollbarAlwaysOff)
        return GTK_POLICY_NEVER;

    return GTK_POLICY_AUTOMATIC;
}

GtkPolicyType webkit_web_frame_get_vertical_scrollbar_policy(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), GTK_POLICY_AUTOMATIC);

    Frame* coreFrame = core(frame);
    FrameView* view = coreFrame->view();
    if (!view)
        return GTK_POLICY_AUTOMATIC;

    ScrollbarMode vMode = view->verticalScrollbarMode();

    if (vMode == ScrollbarAlwaysOn)
        return GTK_POLICY_ALWAYS;

    if (vMode == ScrollbarAlwaysOff)
        return GTK_POLICY_NEVER;

    return GTK_POLICY_AUTOMATIC;
}

/**
 * webkit_web_frame_get_security_origin:
 * @frame: a #WebKitWebFrame
 *
 * Returns the @frame's security origin.
 *
 * Return value: (transfer none): the security origin of @frame
 *
 * Since: 1.1.14
 */
WebKitSecurityOrigin* webkit_web_frame_get_security_origin(WebKitWebFrame* frame)
{
    WebKitWebFramePrivate* priv = frame->priv;
    if (!priv->coreFrame || !priv->coreFrame->document() || !priv->coreFrame->document()->securityOrigin())
        return 0;

    if (priv->origin && priv->origin->priv->coreOrigin.get() == priv->coreFrame->document()->securityOrigin())
        return priv->origin;

    if (priv->origin)
        g_object_unref(priv->origin);

    priv->origin = kit(priv->coreFrame->document()->securityOrigin());
    return priv->origin;
}

/**
 * webkit_web_frame_get_network_response:
 * @frame: a #WebKitWebFrame
 *
 * Returns a #WebKitNetworkResponse object representing the response
 * that was given to the request for the given frame, or NULL if the
 * frame was not created by a load. You must unref the object when you
 * are done with it.
 *
 * Return value: (transfer full): a #WebKitNetworkResponse object
 *
 * Since: 1.1.18
 */
WebKitNetworkResponse* webkit_web_frame_get_network_response(WebKitWebFrame* frame)
{
    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return 0;

    WebCore::DocumentLoader* loader = coreFrame->loader()->activeDocumentLoader();
    if (!loader)
        return 0;

    return kitNew(loader->response());
}

/**
 * webkit_web_frame_replace_selection:
 * @frame: a #WebKitWeFrame
 * @text: the text to insert in place of the current selection
 *
 * Replaces the current selection in @frame, if any, with @text.
 *
 * Since: 1.5.1
 **/
void webkit_web_frame_replace_selection(WebKitWebFrame* frame, const char* text)
{
    Frame* coreFrame = core(frame);
    bool selectReplacement = false;
    bool smartReplace = true;
    coreFrame->editor().replaceSelectionWithText(text, selectReplacement, smartReplace);
}

/**
 * webkit_web_frame_get_range_for_word_around_caret:
 * @frame: a #WebKitWebFrame
 *
 * Returns a #WebKitDOMRange for the word where the caret is currently
 * positioned.
 *
 * Returns: (transfer none): a #WebKitDOMRange spanning the word where the caret
 * currently is positioned. If there is no caret %NULL will be
 * returned.
 *
 * Since: 1.5.1.
 **/
WebKitDOMRange* webkit_web_frame_get_range_for_word_around_caret(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);

    Frame* coreFrame = core(frame);
    FrameSelection* selection = coreFrame->selection();
    if (selection->isNone() || selection->isRange())
        return 0;
    VisibleSelection visibleSelection(selection->selection().visibleStart());
    visibleSelection.expandUsingGranularity(WordGranularity);

    return kit(visibleSelection.firstRange().get());
}

/**
 * webkit_web_frame_get_dom_document:
 * @frame: a #WebKitWebFrame
 *
 * Returns: (transfer none): the #WebKitDOMDocument currently loaded
 * in the @frame or %NULL if no document is loaded
 *
 * Since: 1.10
 **/
WebKitDOMDocument* webkit_web_frame_get_dom_document(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);

    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return 0;

    Document* doc = coreFrame->document();
    if (!doc)
        return 0;

    return kit(doc);
}

namespace WebKit {

WebKitWebView* getViewFromFrame(WebKitWebFrame* frame)
{
    WebKitWebFramePrivate* priv = frame->priv;
    return priv->webView;
}

WebCore::Frame* core(WebKitWebFrame* frame)
{
    if (!frame)
        return 0;

    WebKitWebFramePrivate* priv = frame->priv;
    return priv ? priv->coreFrame : 0;
}

WebKitWebFrame* kit(WebCore::Frame* coreFrame)
{
    if (!coreFrame)
        return 0;

    ASSERT(coreFrame->loader());
    WebKit::FrameLoaderClient* client = static_cast<WebKit::FrameLoaderClient*>(coreFrame->loader()->client());
    return client ? client->webFrame() : 0;
}

}
