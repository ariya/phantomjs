/*
 * Copyright (C) 2009 Jan Michael C. Alonzo
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
#include "webkitwebdatasource.h"

#include "ArchiveResource.h"
#include "DocumentLoaderGtk.h"
#include "FrameLoader.h"
#include "FrameLoaderClientGtk.h"
#include "KURL.h"
#include "ResourceBuffer.h"
#include "ResourceRequest.h"
#include "SharedBuffer.h"
#include "SubstituteData.h"
#include "runtime/InitializeThreading.h"
#include "webkitglobalsprivate.h"
#include "webkitnetworkrequestprivate.h"
#include "webkitwebdatasourceprivate.h"
#include "webkitwebframeprivate.h"
#include "webkitwebresource.h"
#include "webkitwebviewprivate.h"
#include <glib.h>
#include <wtf/Assertions.h>
#include <wtf/text/WTFString.h>

/**
 * SECTION:webkitwebdatasource
 * @short_description: Encapsulates the content to be displayed in a #WebKitWebFrame.
 * @see_also: #WebKitWebFrame
 *
 * Data source encapsulates the content of a #WebKitWebFrame. A
 * #WebKitWebFrame has a main resource and subresources and the data source
 * provides access to these resources. When a request gets loaded initially,
 * it is set to a provisional state. The application can request for the
 * request that initiated the load by asking for the provisional data source
 * and invoking the webkit_web_data_source_get_initial_request method of
 * #WebKitWebDataSource. This data source may not have enough data and some
 * methods may return empty values. To get a "full" data source with the data
 * and resources loaded, you need to get the non-provisional data source
 * through #WebKitWebFrame's webkit_web_frame_get_data_source method. This
 * data source will have the data after everything was loaded. Make sure that
 * the data source was finished loading before using any of its methods. You
 * can do this via webkit_web_data_source_is_loading.
 */

using namespace WebCore;
using namespace WebKit;

struct _WebKitWebDataSourcePrivate {
    WebKit::DocumentLoader* loader;

    WebKitNetworkRequest* initialRequest;
    WebKitNetworkRequest* networkRequest;
    WebKitWebResource* mainresource;

    GString* data;

    gchar* textEncoding;
    gchar* unreachableURL;
};

G_DEFINE_TYPE(WebKitWebDataSource, webkit_web_data_source, G_TYPE_OBJECT);

static void webkit_web_data_source_dispose(GObject* object)
{
    WebKitWebDataSource* webDataSource = WEBKIT_WEB_DATA_SOURCE(object);
    WebKitWebDataSourcePrivate* priv = webDataSource->priv;

    ASSERT(priv->loader);
    ASSERT(!priv->loader->isLoading());
    priv->loader->detachDataSource();
    priv->loader->deref();

    if (priv->initialRequest) {
        g_object_unref(priv->initialRequest);
        priv->initialRequest = NULL;
    }

    if (priv->networkRequest) {
        g_object_unref(priv->networkRequest);
        priv->networkRequest = NULL;
    }

    if (priv->mainresource) {
        g_object_unref(priv->mainresource);
        priv->mainresource = NULL;
    }

    G_OBJECT_CLASS(webkit_web_data_source_parent_class)->dispose(object);
}

static void webkit_web_data_source_finalize(GObject* object)
{
    WebKitWebDataSource* dataSource = WEBKIT_WEB_DATA_SOURCE(object);
    WebKitWebDataSourcePrivate* priv = dataSource->priv;

    g_free(priv->unreachableURL);
    g_free(priv->textEncoding);

    if (priv->data) {
        g_string_free(priv->data, TRUE);
        priv->data = NULL;
    }

    G_OBJECT_CLASS(webkit_web_data_source_parent_class)->finalize(object);
}

static void webkit_web_data_source_class_init(WebKitWebDataSourceClass* klass)
{
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = webkit_web_data_source_dispose;
    gobject_class->finalize = webkit_web_data_source_finalize;

    webkitInit();

    g_type_class_add_private(gobject_class, sizeof(WebKitWebDataSourcePrivate));
}

static void webkit_web_data_source_init(WebKitWebDataSource* webDataSource)
{
    webDataSource->priv = G_TYPE_INSTANCE_GET_PRIVATE(webDataSource, WEBKIT_TYPE_WEB_DATA_SOURCE, WebKitWebDataSourcePrivate);
}

/**
 * webkit_web_data_source_new:
 *
 * Creates a new #WebKitWebDataSource instance. The URL of the
 * #WebKitWebDataSource will be set to "about:blank".
 *
 * Returns: a new #WebKitWebDataSource.
 *
 * Since: 1.1.14
 */
WebKitWebDataSource* webkit_web_data_source_new()
{
    WebKitNetworkRequest* request = webkit_network_request_new("about:blank");
    WebKitWebDataSource* datasource = webkit_web_data_source_new_with_request(request);
    g_object_unref(request);

    return datasource;
}

/**
 * webkit_web_data_source_new_with_request:
 * @request: the #WebKitNetworkRequest to use to create this data source
 *
 * Creates a new #WebKitWebDataSource from a #WebKitNetworkRequest. Normally,
 * #WebKitWebFrame objects create their data sources so you will almost never
 * want to invoke this method directly.
 *
 * Returns: a new #WebKitWebDataSource
 *
 * Since: 1.1.14
 */
WebKitWebDataSource* webkit_web_data_source_new_with_request(WebKitNetworkRequest* request)
{
    ASSERT(request);

    const gchar* uri = webkit_network_request_get_uri(request);

    ResourceRequest resourceRequest(ResourceRequest(KURL(KURL(), String::fromUTF8(uri))));
    WebKitWebDataSource* datasource = kitNew(WebKit::DocumentLoader::create(resourceRequest, SubstituteData()));

    WebKitWebDataSourcePrivate* priv = datasource->priv;
    priv->initialRequest = request;

    return datasource;
}

/**
 * webkit_web_data_source_get_web_frame:
 * @data_source: a #WebKitWebDataSource
 *
 * Returns the #WebKitWebFrame that represents this data source
 *
 * Return value: (transfer none): the #WebKitWebFrame that represents
 * the @data_source. The #WebKitWebFrame is owned by WebKit and should
 * not be freed or destroyed.  This will return %NULL if the
 * @data_source is not attached to a frame.
 *
 * Since: 1.1.14
 */
WebKitWebFrame* webkit_web_data_source_get_web_frame(WebKitWebDataSource* webDataSource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATA_SOURCE(webDataSource), NULL);

    WebKitWebDataSourcePrivate* priv = webDataSource->priv;
    FrameLoader* frameLoader = priv->loader->frameLoader();

    if (!frameLoader)
        return NULL;

    return static_cast<WebKit::FrameLoaderClient*>(frameLoader->client())->webFrame();
}

/**
 * webkit_web_data_source_get_initial_request:
 * @data_source: a #WebKitWebDataSource
 *
 * Returns a reference to the original request that was used to load the web
 * content. The #WebKitNetworkRequest returned by this method is the request
 * prior to the "committed" load state. See webkit_web_data_source_get_request
 * for getting the "committed" request.
 *
 * Return value: (transfer none): the original #WebKitNetworkRequest
 *
 * Since: 1.1.14
 */
WebKitNetworkRequest* webkit_web_data_source_get_initial_request(WebKitWebDataSource* webDataSource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATA_SOURCE(webDataSource), NULL);

    WebKitWebDataSourcePrivate* priv = webDataSource->priv;
    ResourceRequest request = priv->loader->originalRequest();

    if (priv->initialRequest)
        g_object_unref(priv->initialRequest);

    priv->initialRequest = kitNew(request);
    return priv->initialRequest;
}

/**
 * webkit_web_data_source_get_request:
 * @data_source: a #WebKitWebDataSource
 *
 * Returns a #WebKitNetworkRequest that was used to create this
 * #WebKitWebDataSource. The #WebKitNetworkRequest returned by this method is
 * the request that was "committed", and hence, different from the request you
 * get from the webkit_web_data_source_get_initial_request method.
 *
 * Return value: (transfer none): the #WebKitNetworkRequest that
 * created the @data_source or %NULL if the @data_source is not
 * attached to the frame or the frame hasn't been loaded.
 *
 * Since: 1.1.14
 */
WebKitNetworkRequest* webkit_web_data_source_get_request(WebKitWebDataSource* webDataSource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATA_SOURCE(webDataSource), NULL);

    WebKitWebDataSourcePrivate* priv = webDataSource->priv;
    FrameLoader* frameLoader = priv->loader->frameLoader();
    if (!frameLoader || !frameLoader->frameHasLoaded())
        return NULL;

    ResourceRequest request = priv->loader->request();

     if (priv->networkRequest)
         g_object_unref(priv->networkRequest);

     priv->networkRequest = kitNew(request);
     return priv->networkRequest;
}

/**
 * webkit_web_data_source_get_encoding:
 * @data_source: a #WebKitWebDataSource
 *
 * Returns the text encoding name as set in the #WebKitWebView, or if not, the
 * text encoding of the response.
 *
 * Return value: the encoding name of the #WebKitWebView or of the response.
 *
 * Since: 1.1.14
 */
const gchar* webkit_web_data_source_get_encoding(WebKitWebDataSource* webDataSource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATA_SOURCE(webDataSource), NULL);

    WebKitWebDataSourcePrivate* priv = webDataSource->priv;
    String textEncodingName = priv->loader->overrideEncoding();

    if (!textEncodingName)
        textEncodingName = priv->loader->response().textEncodingName();

    CString encoding = textEncodingName.utf8();
    g_free(priv->textEncoding);
    priv->textEncoding = g_strdup(encoding.data());
    return priv->textEncoding;
}

/**
 * webkit_web_data_source_is_loading:
 * @data_source: a #WebKitWebDataSource
 *
 * Determines whether the data source is in the process of loading its content.
 *
 * Return value: %TRUE if the @data_source is still loading, %FALSE otherwise
 *
 * Since: 1.1.14
 */
gboolean webkit_web_data_source_is_loading(WebKitWebDataSource* webDataSource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATA_SOURCE(webDataSource), FALSE);

    WebKitWebDataSourcePrivate* priv = webDataSource->priv;

    return priv->loader->isLoadingInAPISense();
}

/**
 * webkit_web_data_source_get_data:
 * @data_source: a #WebKitWebDataSource
 *
 * Returns the raw data that represents the the frame's content.The data will
 * be incomplete until the data has finished loading. Returns %NULL if the web
 * frame hasn't loaded any data. Use webkit_web_data_source_is_loading to test
 * if data source is in the process of loading.
 *
 * Return value: (transfer none): a #GString which contains the raw
 * data that represents the @data_source or %NULL if the @data_source
 * hasn't loaded any data.
 *
 * Since: 1.1.14
 */
GString* webkit_web_data_source_get_data(WebKitWebDataSource* webDataSource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATA_SOURCE(webDataSource), NULL);

    WebKitWebDataSourcePrivate* priv = webDataSource->priv;

    RefPtr<ResourceBuffer> mainResourceData = priv->loader->mainResourceData();

    if (!mainResourceData)
        return NULL;

    if (priv->data) {
        g_string_free(priv->data, TRUE);
        priv->data = NULL;
    }

    priv->data = g_string_new_len(mainResourceData->data(), mainResourceData->size());
    return priv->data;
}

/**
 * webkit_web_data_source_get_main_resource:
 * @data_source: a #WebKitWebDataSource
 *
 * Returns the main resource of the @data_source
 *
 * Return value: (transfer none): a new #WebKitWebResource
 * representing the main resource of the @data_source.
 *
 * Since: 1.1.14
 */
WebKitWebResource* webkit_web_data_source_get_main_resource(WebKitWebDataSource* webDataSource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATA_SOURCE(webDataSource), NULL);

    WebKitWebDataSourcePrivate* priv = webDataSource->priv;

    if (priv->mainresource)
        return priv->mainresource;

    WebKitWebFrame* webFrame = webkit_web_data_source_get_web_frame(webDataSource);
    WebKitWebView* webView = getViewFromFrame(webFrame);

    priv->mainresource = WEBKIT_WEB_RESOURCE(g_object_ref(webkit_web_view_get_main_resource(webView)));

    return priv->mainresource;
}

/**
 * webkit_web_data_source_get_unreachable_uri:
 * @data_source: a #WebKitWebDataSource
 *
 * Return the unreachable URI of @data_source. The @data_source will have an
 * unreachable URL if it was created using #WebKitWebFrame's
 * webkit_web_frame_load_alternate_html_string method.
 *
 * Return value: the unreachable URL of @data_source or %NULL if there is no unreachable URL.
 *
 * Since: 1.1.14
 */
const gchar* webkit_web_data_source_get_unreachable_uri(WebKitWebDataSource* webDataSource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATA_SOURCE(webDataSource), NULL);

    WebKitWebDataSourcePrivate* priv = webDataSource->priv;
    const KURL& unreachableURL = priv->loader->unreachableURL();

    if (unreachableURL.isEmpty())
        return NULL;

    g_free(priv->unreachableURL);
    priv->unreachableURL = g_strdup(unreachableURL.string().utf8().data());
    return priv->unreachableURL;
}

/**
 * webkit_web_data_source_get_subresources:
 * @data_source: a #WebKitWebDataSource
 *
 * Gives you a #GList of #WebKitWebResource objects that compose the
 * #WebKitWebView to which this #WebKitWebDataSource is attached.
 *
 * Return value: (element-type WebKitWebResource) (transfer container):
 * a #GList of #WebKitWebResource objects; the objects are owned by
 * WebKit, but the GList must be freed.
 *
 * Since: 1.1.15
 */
GList* webkit_web_data_source_get_subresources(WebKitWebDataSource* webDataSource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATA_SOURCE(webDataSource), NULL);

    WebKitWebFrame* webFrame = webkit_web_data_source_get_web_frame(webDataSource);
    WebKitWebView* webView = getViewFromFrame(webFrame);

    return webkit_web_view_get_subresources(webView);
}

namespace WebKit {

WebKitWebDataSource* kitNew(PassRefPtr<WebKit::DocumentLoader> loader)
{
    WebKitWebDataSource* webDataSource = WEBKIT_WEB_DATA_SOURCE(g_object_new(WEBKIT_TYPE_WEB_DATA_SOURCE, NULL));
    WebKitWebDataSourcePrivate* priv = webDataSource->priv;
    priv->loader = loader.leakRef();

    return webDataSource;
}

}
