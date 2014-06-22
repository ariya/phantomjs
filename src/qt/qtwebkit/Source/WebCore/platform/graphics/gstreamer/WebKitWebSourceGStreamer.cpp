/*
 *  Copyright (C) 2009, 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *  Copyright (C) 2013 Collabora Ltd.
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
#include "WebKitWebSourceGStreamer.h"

#if ENABLE(VIDEO) && USE(GSTREAMER)

#include "CachedRawResource.h"
#include "CachedRawResourceClient.h"
#include "CachedResourceHandle.h"
#include "CachedResourceLoader.h"
#include "CachedResourceRequest.h"
#include "Frame.h"
#include "GRefPtrGStreamer.h"
#include "GStreamerVersioning.h"
#include "MediaPlayer.h"
#include "NotImplemented.h"
#include "ResourceHandle.h"
#include "ResourceHandleClient.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/pbutils/missing-plugins.h>
#include <wtf/Noncopyable.h>
#include <wtf/gobject/GMutexLocker.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebCore;

static CachedResourceLoader* s_cachedResourceLoader = 0;

class StreamingClient {
    public:
        StreamingClient(WebKitWebSrc*);
        virtual ~StreamingClient();

        virtual bool loadFailed() const = 0;
        virtual void setDefersLoading(bool) = 0;

    protected:
        char* createReadBuffer(size_t requestedSize, size_t& actualSize);
        void handleResponseReceived(const ResourceResponse&);
        void handleDataReceived(const char*, int);
        void handleNotifyFinished();

        GRefPtr<GstElement> m_src;
};

class CachedResourceStreamingClient : public CachedRawResourceClient, public StreamingClient {
    WTF_MAKE_NONCOPYABLE(CachedResourceStreamingClient); WTF_MAKE_FAST_ALLOCATED;
    public:
        CachedResourceStreamingClient(WebKitWebSrc*, CachedResourceLoader*, const ResourceRequest&);
        virtual ~CachedResourceStreamingClient();

        // StreamingClient virtual methods.
        virtual bool loadFailed() const;
        virtual void setDefersLoading(bool);

    private:
        // CachedResourceClient virtual methods.
        virtual char* getOrCreateReadBuffer(CachedResource*, size_t requestedSize, size_t& actualSize);
        virtual void responseReceived(CachedResource*, const ResourceResponse&);
        virtual void dataReceived(CachedResource*, const char*, int);
        virtual void notifyFinished(CachedResource*);

        CachedResourceHandle<CachedRawResource> m_resource;
};

class ResourceHandleStreamingClient : public ResourceHandleClient, public StreamingClient {
    WTF_MAKE_NONCOPYABLE(ResourceHandleStreamingClient); WTF_MAKE_FAST_ALLOCATED;
    public:
        ResourceHandleStreamingClient(WebKitWebSrc*, const ResourceRequest&);
        virtual ~ResourceHandleStreamingClient();

        // StreamingClient virtual methods.
        virtual bool loadFailed() const;
        virtual void setDefersLoading(bool);

    private:
        // ResourceHandleClient virtual methods.
        virtual char* getOrCreateReadBuffer(size_t requestedSize, size_t& actualSize);
        virtual void willSendRequest(ResourceHandle*, ResourceRequest&, const ResourceResponse&);
        virtual void didReceiveResponse(ResourceHandle*, const ResourceResponse&);
        virtual void didReceiveData(ResourceHandle*, const char*, int, int);
        virtual void didFinishLoading(ResourceHandle*, double /*finishTime*/);
        virtual void didFail(ResourceHandle*, const ResourceError&);
        virtual void wasBlocked(ResourceHandle*);
        virtual void cannotShowURL(ResourceHandle*);

        RefPtr<ResourceHandle> m_resource;
};

#define WEBKIT_WEB_SRC_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_WEB_SRC, WebKitWebSrcPrivate))
struct _WebKitWebSrcPrivate {
    GstAppSrc* appsrc;
    GstPad* srcpad;
    gchar* uri;

    WebCore::MediaPlayer* player;

    StreamingClient* client;

    guint64 offset;
    guint64 size;
    gboolean seekable;
    gboolean paused;

    guint64 requestedOffset;

    guint startID;
    guint stopID;
    guint needDataID;
    guint enoughDataID;
    guint seekID;

    GRefPtr<GstBuffer> buffer;

    // icecast stuff
    gboolean iradioMode;
    gchar* iradioName;
    gchar* iradioGenre;
    gchar* iradioUrl;
    gchar* iradioTitle;

    // TRUE if appsrc's version is >= 0.10.27, see
    // https://bugzilla.gnome.org/show_bug.cgi?id=609423
    gboolean haveAppSrc27;
};

enum {
    PROP_IRADIO_MODE = 1,
    PROP_IRADIO_NAME,
    PROP_IRADIO_GENRE,
    PROP_IRADIO_URL,
    PROP_IRADIO_TITLE,
    PROP_LOCATION
};

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC(webkit_web_src_debug);
#define GST_CAT_DEFAULT webkit_web_src_debug

static void webKitWebSrcUriHandlerInit(gpointer gIface, gpointer ifaceData);

static void webKitWebSrcDispose(GObject*);
static void webKitWebSrcFinalize(GObject*);
static void webKitWebSrcSetProperty(GObject*, guint propertyID, const GValue*, GParamSpec*);
static void webKitWebSrcGetProperty(GObject*, guint propertyID, GValue*, GParamSpec*);
static GstStateChangeReturn webKitWebSrcChangeState(GstElement*, GstStateChange);

static gboolean webKitWebSrcQueryWithParent(GstPad*, GstObject*, GstQuery*);
#ifndef GST_API_VERSION_1
static gboolean webKitWebSrcQuery(GstPad*, GstQuery*);
#endif

static void webKitWebSrcNeedDataCb(GstAppSrc*, guint length, gpointer userData);
static void webKitWebSrcEnoughDataCb(GstAppSrc*, gpointer userData);
static gboolean webKitWebSrcSeekDataCb(GstAppSrc*, guint64 offset, gpointer userData);

static GstAppSrcCallbacks appsrcCallbacks = {
    webKitWebSrcNeedDataCb,
    webKitWebSrcEnoughDataCb,
    webKitWebSrcSeekDataCb,
    { 0 }
};

#define webkit_web_src_parent_class parent_class
// We split this out into another macro to avoid a check-webkit-style error.
#define WEBKIT_WEB_SRC_CATEGORY_INIT GST_DEBUG_CATEGORY_INIT(webkit_web_src_debug, "webkitwebsrc", 0, "websrc element");
G_DEFINE_TYPE_WITH_CODE(WebKitWebSrc, webkit_web_src, GST_TYPE_BIN,
                         G_IMPLEMENT_INTERFACE(GST_TYPE_URI_HANDLER, webKitWebSrcUriHandlerInit);
                         WEBKIT_WEB_SRC_CATEGORY_INIT);

static void webkit_web_src_class_init(WebKitWebSrcClass* klass)
{
    GObjectClass* oklass = G_OBJECT_CLASS(klass);
    GstElementClass* eklass = GST_ELEMENT_CLASS(klass);

    oklass->dispose = webKitWebSrcDispose;
    oklass->finalize = webKitWebSrcFinalize;
    oklass->set_property = webKitWebSrcSetProperty;
    oklass->get_property = webKitWebSrcGetProperty;

    gst_element_class_add_pad_template(eklass,
                                       gst_static_pad_template_get(&srcTemplate));
    setGstElementClassMetadata(eklass, "WebKit Web source element", "Source", "Handles HTTP/HTTPS uris",
                               "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

    // icecast stuff
    g_object_class_install_property(oklass,
                                    PROP_IRADIO_MODE,
                                    g_param_spec_boolean("iradio-mode",
                                                         "iradio-mode",
                                                         "Enable internet radio mode (extraction of shoutcast/icecast metadata)",
                                                         FALSE,
                                                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(oklass,
                                    PROP_IRADIO_NAME,
                                    g_param_spec_string("iradio-name",
                                                        "iradio-name",
                                                        "Name of the stream",
                                                        0,
                                                        (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(oklass,
                                    PROP_IRADIO_GENRE,
                                    g_param_spec_string("iradio-genre",
                                                        "iradio-genre",
                                                        "Genre of the stream",
                                                        0,
                                                        (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(oklass,
                                    PROP_IRADIO_URL,
                                    g_param_spec_string("iradio-url",
                                                        "iradio-url",
                                                        "Homepage URL for radio stream",
                                                        0,
                                                        (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(oklass,
                                    PROP_IRADIO_TITLE,
                                    g_param_spec_string("iradio-title",
                                                        "iradio-title",
                                                        "Name of currently playing song",
                                                        0,
                                                        (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));


    /* Allows setting the uri using the 'location' property, which is used
     * for example by gst_element_make_from_uri() */
    g_object_class_install_property(oklass,
                                    PROP_LOCATION,
                                    g_param_spec_string("location",
                                                        "location",
                                                        "Location to read from",
                                                        0,
                                                        (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    eklass->change_state = webKitWebSrcChangeState;

    g_type_class_add_private(klass, sizeof(WebKitWebSrcPrivate));
}

static void webkit_web_src_init(WebKitWebSrc* src)
{
    WebKitWebSrcPrivate* priv = WEBKIT_WEB_SRC_GET_PRIVATE(src);

    src->priv = priv;

    priv->appsrc = GST_APP_SRC(gst_element_factory_make("appsrc", 0));
    if (!priv->appsrc) {
        GST_ERROR_OBJECT(src, "Failed to create appsrc");
        return;
    }

    GstElementFactory* factory = GST_ELEMENT_FACTORY(GST_ELEMENT_GET_CLASS(priv->appsrc)->elementfactory);
    priv->haveAppSrc27 = gst_plugin_feature_check_version(GST_PLUGIN_FEATURE(factory), 0, 10, 27);

    gst_bin_add(GST_BIN(src), GST_ELEMENT(priv->appsrc));


    GRefPtr<GstPad> targetPad = adoptGRef(gst_element_get_static_pad(GST_ELEMENT(priv->appsrc), "src"));
    priv->srcpad = webkitGstGhostPadFromStaticTemplate(&srcTemplate, "src", targetPad.get());

    gst_element_add_pad(GST_ELEMENT(src), priv->srcpad);

#ifdef GST_API_VERSION_1
    GST_OBJECT_FLAG_SET(priv->srcpad, GST_PAD_FLAG_NEED_PARENT);
    gst_pad_set_query_function(priv->srcpad, webKitWebSrcQueryWithParent);
#else
    gst_pad_set_query_function(priv->srcpad, webKitWebSrcQuery);
#endif

    gst_app_src_set_callbacks(priv->appsrc, &appsrcCallbacks, src, 0);
    gst_app_src_set_emit_signals(priv->appsrc, FALSE);
    gst_app_src_set_stream_type(priv->appsrc, GST_APP_STREAM_TYPE_SEEKABLE);

    // 512k is a abitrary number but we should choose a value
    // here to not pause/unpause the SoupMessage too often and
    // to make sure there's always some data available for
    // GStreamer to handle.
    gst_app_src_set_max_bytes(priv->appsrc, 512 * 1024);

    // Emit the need-data signal if the queue contains less
    // than 20% of data. Without this the need-data signal
    // is emitted when the queue is empty, we then dispatch
    // the soup message unpausing to the main loop and from
    // there unpause the soup message. This already takes
    // quite some time and libsoup even needs some more time
    // to actually provide data again. If we do all this
    // already if the queue is 20% empty, it's much more
    // likely that libsoup already provides new data before
    // the queue is really empty.
    // This might need tweaking for ports not using libsoup.
    if (priv->haveAppSrc27)
        g_object_set(priv->appsrc, "min-percent", 20, NULL);

    gst_app_src_set_caps(priv->appsrc, 0);
    gst_app_src_set_size(priv->appsrc, -1);
}

static void webKitWebSrcDispose(GObject* object)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(object);
    WebKitWebSrcPrivate* priv = src->priv;

    if (priv->player) {
        priv->player = 0;
        s_cachedResourceLoader = 0;
    }

    GST_CALL_PARENT(G_OBJECT_CLASS, dispose, (object));
}

static void webKitWebSrcFinalize(GObject* object)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(object);
    WebKitWebSrcPrivate* priv = src->priv;

    g_free(priv->uri);

    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static void webKitWebSrcSetProperty(GObject* object, guint propID, const GValue* value, GParamSpec* pspec)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(object);
    WebKitWebSrcPrivate* priv = src->priv;

    switch (propID) {
    case PROP_IRADIO_MODE: {
        GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
        priv->iradioMode = g_value_get_boolean(value);
        break;
    }
    case PROP_LOCATION:
#ifdef GST_API_VERSION_1
        gst_uri_handler_set_uri(reinterpret_cast<GstURIHandler*>(src), g_value_get_string(value), 0);
#else
        gst_uri_handler_set_uri(reinterpret_cast<GstURIHandler*>(src), g_value_get_string(value));
#endif
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propID, pspec);
        break;
    }
}

static void webKitWebSrcGetProperty(GObject* object, guint propID, GValue* value, GParamSpec* pspec)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(object);
    WebKitWebSrcPrivate* priv = src->priv;

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    switch (propID) {
    case PROP_IRADIO_MODE:
        g_value_set_boolean(value, priv->iradioMode);
        break;
    case PROP_IRADIO_NAME:
        g_value_set_string(value, priv->iradioName);
        break;
    case PROP_IRADIO_GENRE:
        g_value_set_string(value, priv->iradioGenre);
        break;
    case PROP_IRADIO_URL:
        g_value_set_string(value, priv->iradioUrl);
        break;
    case PROP_IRADIO_TITLE:
        g_value_set_string(value, priv->iradioTitle);
        break;
    case PROP_LOCATION:
        g_value_set_string(value, priv->uri);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propID, pspec);
        break;
    }
}

static void removeTimeoutSources(WebKitWebSrc* src)
{
    WebKitWebSrcPrivate* priv = src->priv;

    if (priv->startID)
        g_source_remove(priv->startID);
    priv->startID = 0;

    if (priv->needDataID)
        g_source_remove(priv->needDataID);
    priv->needDataID = 0;

    if (priv->enoughDataID)
        g_source_remove(priv->enoughDataID);
    priv->enoughDataID = 0;

    if (priv->seekID)
        g_source_remove(priv->seekID);
    priv->seekID = 0;
}

static gboolean webKitWebSrcStop(WebKitWebSrc* src)
{
    WebKitWebSrcPrivate* priv = src->priv;

    ASSERT(isMainThread());

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));

    bool seeking = priv->seekID;

    removeTimeoutSources(src);
    priv->stopID = 0;

    if (priv->client) {
        delete priv->client;
        priv->client = 0;
    }

    if (priv->buffer) {
#ifdef GST_API_VERSION_1
        unmapGstBuffer(priv->buffer.get());
#endif
        priv->buffer.clear();
    }

    priv->paused = FALSE;

    g_free(priv->iradioName);
    priv->iradioName = 0;

    g_free(priv->iradioGenre);
    priv->iradioGenre = 0;

    g_free(priv->iradioUrl);
    priv->iradioUrl = 0;

    g_free(priv->iradioTitle);
    priv->iradioTitle = 0;

    priv->offset = 0;
    priv->seekable = FALSE;

    if (!seeking) {
        priv->size = 0;
        priv->requestedOffset = 0;
        if (priv->player) {
            priv->player = 0;
            s_cachedResourceLoader = 0;
        }
    }

    locker.unlock();

    if (priv->appsrc) {
        gst_app_src_set_caps(priv->appsrc, 0);
        if (!seeking)
            gst_app_src_set_size(priv->appsrc, -1);
    }

    GST_DEBUG_OBJECT(src, "Stopped request");

    return FALSE;
}

static gboolean webKitWebSrcStart(WebKitWebSrc* src)
{
    WebKitWebSrcPrivate* priv = src->priv;

    ASSERT(isMainThread());

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));

    priv->startID = 0;

    if (!priv->uri) {
        GST_ERROR_OBJECT(src, "No URI provided");
        locker.unlock();
        webKitWebSrcStop(src);
        return FALSE;
    }

    ASSERT(!priv->client);

    KURL url = KURL(KURL(), priv->uri);

    ResourceRequest request(url);
    request.setAllowCookies(true);

    if (priv->player)
        request.setHTTPReferrer(priv->player->referrer());

    // Let Apple web servers know we want to access their nice movie trailers.
    if (!g_ascii_strcasecmp("movies.apple.com", url.host().utf8().data())
        || !g_ascii_strcasecmp("trailers.apple.com", url.host().utf8().data()))
        request.setHTTPUserAgent("Quicktime/7.6.6");

    if (priv->requestedOffset) {
        GOwnPtr<gchar> val;

        val.set(g_strdup_printf("bytes=%" G_GUINT64_FORMAT "-", priv->requestedOffset));
        request.setHTTPHeaderField("Range", val.get());
    }
    priv->offset = priv->requestedOffset;

    if (priv->iradioMode)
        request.setHTTPHeaderField("icy-metadata", "1");

    // Needed to use DLNA streaming servers
    request.setHTTPHeaderField("transferMode.dlna", "Streaming");

    CachedResourceLoader* loader = 0;
    if (priv->player)
        loader = priv->player->cachedResourceLoader();
    else
        loader = s_cachedResourceLoader;

    if (loader)
        priv->client = new CachedResourceStreamingClient(src, loader, request);

    if (!priv->client)
        priv->client = new ResourceHandleStreamingClient(src, request);

    if (!priv->client || priv->client->loadFailed()) {
        GST_ERROR_OBJECT(src, "Failed to setup streaming client");
        if (priv->client) {
            delete priv->client;
            priv->client = 0;
        }
        locker.unlock();
        webKitWebSrcStop(src);
        return FALSE;
    }
    GST_DEBUG_OBJECT(src, "Started request");
    return FALSE;
}

static GstStateChangeReturn webKitWebSrcChangeState(GstElement* element, GstStateChange transition)
{
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    WebKitWebSrc* src = WEBKIT_WEB_SRC(element);
    WebKitWebSrcPrivate* priv = src->priv;

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        if (!priv->appsrc) {
            gst_element_post_message(element,
                                     gst_missing_element_message_new(element, "appsrc"));
            GST_ELEMENT_ERROR(src, CORE, MISSING_PLUGIN, (0), ("no appsrc"));
            return GST_STATE_CHANGE_FAILURE;
        }
        break;
    default:
        break;
    }

    ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    if (G_UNLIKELY(ret == GST_STATE_CHANGE_FAILURE)) {
        GST_DEBUG_OBJECT(src, "State change failed");
        return ret;
    }

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        GST_DEBUG_OBJECT(src, "READY->PAUSED");
        priv->startID = g_idle_add_full(G_PRIORITY_DEFAULT, (GSourceFunc) webKitWebSrcStart, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        GST_DEBUG_OBJECT(src, "PAUSED->READY");
        // cancel pending sources
        removeTimeoutSources(src);
        priv->stopID = g_idle_add_full(G_PRIORITY_DEFAULT, (GSourceFunc) webKitWebSrcStop, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
        break;
    default:
        break;
    }

    return ret;
}

static gboolean webKitWebSrcQueryWithParent(GstPad* pad, GstObject* parent, GstQuery* query)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(GST_ELEMENT(parent));
    gboolean result = FALSE;

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_DURATION: {
        GstFormat format;

        gst_query_parse_duration(query, &format, NULL);

        GST_DEBUG_OBJECT(src, "duration query in format %s", gst_format_get_name(format));
        GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
        if (format == GST_FORMAT_BYTES && src->priv->size > 0) {
            gst_query_set_duration(query, format, src->priv->size);
            result = TRUE;
        }
        break;
    }
    case GST_QUERY_URI: {
        GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
        gst_query_set_uri(query, src->priv->uri);
        result = TRUE;
        break;
    }
    default: {
        GRefPtr<GstPad> target = adoptGRef(gst_ghost_pad_get_target(GST_GHOST_PAD_CAST(pad)));

        // Forward the query to the proxy target pad.
        if (target)
            result = gst_pad_query(target.get(), query);
        break;
    }
    }

    return result;
}

#ifndef GST_API_VERSION_1
static gboolean webKitWebSrcQuery(GstPad* pad, GstQuery* query)
{
    GRefPtr<GstElement> src = adoptGRef(gst_pad_get_parent_element(pad));
    return webKitWebSrcQueryWithParent(pad, GST_OBJECT(src.get()), query);
}
#endif

// uri handler interface

#ifdef GST_API_VERSION_1
static GstURIType webKitWebSrcUriGetType(GType)
{
    return GST_URI_SRC;
}

const gchar* const* webKitWebSrcGetProtocols(GType)
{
    static const char* protocols[] = {"http", "https", 0 };
    return protocols;
}

static gchar* webKitWebSrcGetUri(GstURIHandler* handler)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(handler);
    gchar* ret;

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    ret = g_strdup(src->priv->uri);
    return ret;
}

static gboolean webKitWebSrcSetUri(GstURIHandler* handler, const gchar* uri, GError** error)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(handler);
    WebKitWebSrcPrivate* priv = src->priv;

    if (GST_STATE(src) >= GST_STATE_PAUSED) {
        GST_ERROR_OBJECT(src, "URI can only be set in states < PAUSED");
        return FALSE;
    }

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));

    g_free(priv->uri);
    priv->uri = 0;

    if (!uri)
        return TRUE;

    KURL url(KURL(), uri);

    if (!url.isValid() || !url.protocolIsInHTTPFamily()) {
        g_set_error(error, GST_URI_ERROR, GST_URI_ERROR_BAD_URI, "Invalid URI '%s'", uri);
        return FALSE;
    }

    priv->uri = g_strdup(url.string().utf8().data());
    return TRUE;
}

#else
static GstURIType webKitWebSrcUriGetType(void)
{
    return GST_URI_SRC;
}

static gchar** webKitWebSrcGetProtocols(void)
{
    static gchar* protocols[] = {(gchar*) "http", (gchar*) "https", 0 };
    return protocols;
}

static const gchar* webKitWebSrcGetUri(GstURIHandler* handler)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(handler);
    gchar* ret;

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    ret = g_strdup(src->priv->uri);
    return ret;
}

static gboolean webKitWebSrcSetUri(GstURIHandler* handler, const gchar* uri)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(handler);
    WebKitWebSrcPrivate* priv = src->priv;

    if (GST_STATE(src) >= GST_STATE_PAUSED) {
        GST_ERROR_OBJECT(src, "URI can only be set in states < PAUSED");
        return FALSE;
    }

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));

    g_free(priv->uri);
    priv->uri = 0;

    if (!uri)
        return TRUE;

    KURL url(KURL(), uri);

    if (!url.isValid() || !url.protocolIsInHTTPFamily()) {
        GST_ERROR_OBJECT(src, "Invalid URI '%s'", uri);
        return FALSE;
    }

    priv->uri = g_strdup(url.string().utf8().data());
    return TRUE;
}
#endif

static void webKitWebSrcUriHandlerInit(gpointer gIface, gpointer)
{
    GstURIHandlerInterface* iface = (GstURIHandlerInterface *) gIface;

    iface->get_type = webKitWebSrcUriGetType;
    iface->get_protocols = webKitWebSrcGetProtocols;
    iface->get_uri = webKitWebSrcGetUri;
    iface->set_uri = webKitWebSrcSetUri;
}

// appsrc callbacks

static gboolean webKitWebSrcNeedDataMainCb(WebKitWebSrc* src)
{
    WebKitWebSrcPrivate* priv = src->priv;

    ASSERT(isMainThread());

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    // already stopped
    if (!priv->needDataID)
        return FALSE;

    priv->paused = FALSE;
    priv->needDataID = 0;
    locker.unlock();

    if (priv->client)
        priv->client->setDefersLoading(false);
    return FALSE;
}

static void webKitWebSrcNeedDataCb(GstAppSrc*, guint length, gpointer userData)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(userData);
    WebKitWebSrcPrivate* priv = src->priv;

    GST_DEBUG_OBJECT(src, "Need more data: %u", length);

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    if (priv->needDataID || !priv->paused) {
        return;
    }

    priv->needDataID = g_idle_add_full(G_PRIORITY_DEFAULT, (GSourceFunc) webKitWebSrcNeedDataMainCb, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
}

static gboolean webKitWebSrcEnoughDataMainCb(WebKitWebSrc* src)
{
    WebKitWebSrcPrivate* priv = src->priv;

    ASSERT(isMainThread());

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    // already stopped
    if (!priv->enoughDataID)
        return FALSE;

    priv->paused = TRUE;
    priv->enoughDataID = 0;
    locker.unlock();

    if (priv->client)
        priv->client->setDefersLoading(true);
    return FALSE;
}

static void webKitWebSrcEnoughDataCb(GstAppSrc*, gpointer userData)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(userData);
    WebKitWebSrcPrivate* priv = src->priv;

    GST_DEBUG_OBJECT(src, "Have enough data");

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    if (priv->enoughDataID || priv->paused) {
        return;
    }

    priv->enoughDataID = g_idle_add_full(G_PRIORITY_DEFAULT, (GSourceFunc) webKitWebSrcEnoughDataMainCb, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
}

static gboolean webKitWebSrcSeekMainCb(WebKitWebSrc* src)
{
    WebKitWebSrcPrivate* priv = src->priv;

    ASSERT(isMainThread());

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    // already stopped
    if (!priv->seekID)
        return FALSE;
    locker.unlock();

    webKitWebSrcStop(src);
    webKitWebSrcStart(src);

    return FALSE;
}

static gboolean webKitWebSrcSeekDataCb(GstAppSrc*, guint64 offset, gpointer userData)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(userData);
    WebKitWebSrcPrivate* priv = src->priv;

    GST_DEBUG_OBJECT(src, "Seeking to offset: %" G_GUINT64_FORMAT, offset);
    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    if (offset == priv->offset && priv->requestedOffset == priv->offset)
        return TRUE;

    if (!priv->seekable)
        return FALSE;

    GST_DEBUG_OBJECT(src, "Doing range-request seek");
    priv->requestedOffset = offset;

    if (priv->seekID)
        g_source_remove(priv->seekID);
    priv->seekID = g_idle_add_full(G_PRIORITY_DEFAULT, (GSourceFunc) webKitWebSrcSeekMainCb, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
    return TRUE;
}

void webKitWebSrcSetMediaPlayer(WebKitWebSrc* src, WebCore::MediaPlayer* player)
{
    ASSERT(player);
    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    src->priv->player = player;
    s_cachedResourceLoader = player->cachedResourceLoader();
}

StreamingClient::StreamingClient(WebKitWebSrc* src)
    : m_src(adoptGRef(static_cast<GstElement*>(gst_object_ref(src))))
{
}

StreamingClient::~StreamingClient()
{
}

char* StreamingClient::createReadBuffer(size_t requestedSize, size_t& actualSize)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(m_src.get());
    WebKitWebSrcPrivate* priv = src->priv;

    ASSERT(!priv->buffer);

    GstBuffer* buffer = gst_buffer_new_and_alloc(requestedSize);

#ifdef GST_API_VERSION_1
    mapGstBuffer(buffer);
#endif

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    priv->buffer = adoptGRef(buffer);
    locker.unlock();

    actualSize = getGstBufferSize(buffer);
    return getGstBufferDataPointer(buffer);
}

void StreamingClient::handleResponseReceived(const ResourceResponse& response)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(m_src.get());
    WebKitWebSrcPrivate* priv = src->priv;

    GST_DEBUG_OBJECT(src, "Received response: %d", response.httpStatusCode());

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));

    // If we seeked we need 206 == PARTIAL_CONTENT
    if (priv->requestedOffset && response.httpStatusCode() != 206) {
        locker.unlock();
        GST_ELEMENT_ERROR(src, RESOURCE, READ, (0), (0));
        gst_app_src_end_of_stream(priv->appsrc);
        webKitWebSrcStop(src);
        return;
    }

    long long length = response.expectedContentLength();
    if (length > 0)
        length += priv->requestedOffset;

    priv->size = length >= 0 ? length : 0;
    priv->seekable = length > 0 && g_ascii_strcasecmp("none", response.httpHeaderField("Accept-Ranges").utf8().data());

#ifdef GST_API_VERSION_1
    GstTagList* tags = gst_tag_list_new_empty();
#else
    GstTagList* tags = gst_tag_list_new();
#endif
    String value = response.httpHeaderField("icy-name");
    if (!value.isEmpty()) {
        g_free(priv->iradioName);
        priv->iradioName = g_strdup(value.utf8().data());
        g_object_notify(G_OBJECT(src), "iradio-name");
        gst_tag_list_add(tags, GST_TAG_MERGE_REPLACE, GST_TAG_ORGANIZATION, priv->iradioName, NULL);
    }
    value = response.httpHeaderField("icy-genre");
    if (!value.isEmpty()) {
        g_free(priv->iradioGenre);
        priv->iradioGenre = g_strdup(value.utf8().data());
        g_object_notify(G_OBJECT(src), "iradio-genre");
        gst_tag_list_add(tags, GST_TAG_MERGE_REPLACE, GST_TAG_GENRE, priv->iradioGenre, NULL);
    }
    value = response.httpHeaderField("icy-url");
    if (!value.isEmpty()) {
        g_free(priv->iradioUrl);
        priv->iradioUrl = g_strdup(value.utf8().data());
        g_object_notify(G_OBJECT(src), "iradio-url");
        gst_tag_list_add(tags, GST_TAG_MERGE_REPLACE, GST_TAG_LOCATION, priv->iradioUrl, NULL);
    }
    value = response.httpHeaderField("icy-title");
    if (!value.isEmpty()) {
        g_free(priv->iradioTitle);
        priv->iradioTitle = g_strdup(value.utf8().data());
        g_object_notify(G_OBJECT(src), "iradio-title");
        gst_tag_list_add(tags, GST_TAG_MERGE_REPLACE, GST_TAG_TITLE, priv->iradioTitle, NULL);
    }

    locker.unlock();

    // notify size/duration
    if (length > 0) {
        gst_app_src_set_size(priv->appsrc, length);

#ifndef GST_API_VERSION_1
        if (!priv->haveAppSrc27) {
            gst_segment_set_duration(&GST_BASE_SRC(priv->appsrc)->segment, GST_FORMAT_BYTES, length);
            gst_element_post_message(GST_ELEMENT(priv->appsrc),
                gst_message_new_duration(GST_OBJECT(priv->appsrc),
                    GST_FORMAT_BYTES, length));
        }
#endif
    } else
        gst_app_src_set_size(priv->appsrc, -1);

    // icecast stuff
    value = response.httpHeaderField("icy-metaint");
    if (!value.isEmpty()) {
        gchar* endptr = 0;
        gint64 icyMetaInt = g_ascii_strtoll(value.utf8().data(), &endptr, 10);

        if (endptr && *endptr == '\0' && icyMetaInt > 0) {
            GRefPtr<GstCaps> caps = adoptGRef(gst_caps_new_simple("application/x-icy", "metadata-interval", G_TYPE_INT, (gint) icyMetaInt, NULL));

            gst_app_src_set_caps(priv->appsrc, caps.get());
        }
    } else
        gst_app_src_set_caps(priv->appsrc, 0);

    // notify tags
    if (gst_tag_list_is_empty(tags))
#ifdef GST_API_VERSION_1
        gst_tag_list_unref(tags);
#else
        gst_tag_list_free(tags);
#endif
    else
        notifyGstTagsOnPad(GST_ELEMENT(src), priv->srcpad, tags);
}

void StreamingClient::handleDataReceived(const char* data, int length)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(m_src.get());
    WebKitWebSrcPrivate* priv = src->priv;

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));

    GST_LOG_OBJECT(src, "Have %d bytes of data", priv->buffer ? getGstBufferSize(priv->buffer.get()) : length);

    ASSERT(!priv->buffer || data == getGstBufferDataPointer(priv->buffer.get()));

#ifdef GST_API_VERSION_1
    if (priv->buffer)
        unmapGstBuffer(priv->buffer.get());
#endif

    if (priv->seekID) {
        GST_DEBUG_OBJECT(src, "Seek in progress, ignoring data");
        priv->buffer.clear();
        return;
    }

    // Ports using the GStreamer backend but not the soup implementation of ResourceHandle
    // won't be using buffers provided by this client, the buffer is created here in that case.
    if (!priv->buffer)
        priv->buffer = adoptGRef(createGstBufferForData(data, length));
    else
        setGstBufferSize(priv->buffer.get(), length);

    GST_BUFFER_OFFSET(priv->buffer.get()) = priv->offset;
    if (priv->requestedOffset == priv->offset)
        priv->requestedOffset += length;
    priv->offset += length;
    // priv->size == 0 if received length on didReceiveResponse < 0.
    if (priv->size > 0 && priv->offset > priv->size) {
        GST_DEBUG_OBJECT(src, "Updating internal size from %" G_GUINT64_FORMAT " to %" G_GUINT64_FORMAT, priv->size, priv->offset);
        gst_app_src_set_size(priv->appsrc, priv->offset);
        priv->size = priv->offset;
    }
    GST_BUFFER_OFFSET_END(priv->buffer.get()) = priv->offset;

    locker.unlock();

    GstFlowReturn ret = gst_app_src_push_buffer(priv->appsrc, priv->buffer.leakRef());
#ifdef GST_API_VERSION_1
    if (ret != GST_FLOW_OK && ret != GST_FLOW_EOS)
#else
    if (ret != GST_FLOW_OK && ret != GST_FLOW_UNEXPECTED)
#endif
        GST_ELEMENT_ERROR(src, CORE, FAILED, (0), (0));
}

void StreamingClient::handleNotifyFinished()
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(m_src.get());
    WebKitWebSrcPrivate* priv = src->priv;

    GST_DEBUG_OBJECT(src, "Have EOS");

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    if (!priv->seekID) {
        locker.unlock();
        gst_app_src_end_of_stream(priv->appsrc);
    }
}

CachedResourceStreamingClient::CachedResourceStreamingClient(WebKitWebSrc* src, CachedResourceLoader* resourceLoader, const ResourceRequest& request)
    : StreamingClient(src)
{
    CachedResourceRequest cacheRequest(request, ResourceLoaderOptions(SendCallbacks, DoNotSniffContent, DoNotBufferData, DoNotAllowStoredCredentials, DoNotAskClientForCrossOriginCredentials, DoSecurityCheck, UseDefaultOriginRestrictionsForType));
    m_resource = resourceLoader->requestRawResource(cacheRequest);
    if (m_resource)
        m_resource->addClient(this);
}

CachedResourceStreamingClient::~CachedResourceStreamingClient()
{
    if (m_resource) {
        m_resource->removeClient(this);
        m_resource = 0;
    }
}

bool CachedResourceStreamingClient::loadFailed() const
{
    return !m_resource;
}

void CachedResourceStreamingClient::setDefersLoading(bool defers)
{
    if (m_resource)
        m_resource->setDefersLoading(defers);
}

char* CachedResourceStreamingClient::getOrCreateReadBuffer(CachedResource*, size_t requestedSize, size_t& actualSize)
{
    return createReadBuffer(requestedSize, actualSize);
}

void CachedResourceStreamingClient::responseReceived(CachedResource*, const ResourceResponse& response)
{
    handleResponseReceived(response);
}

void CachedResourceStreamingClient::dataReceived(CachedResource*, const char* data, int length)
{
    handleDataReceived(data, length);
}

void CachedResourceStreamingClient::notifyFinished(CachedResource* resource)
{
    if (resource->loadFailedOrCanceled()) {
        WebKitWebSrc* src = WEBKIT_WEB_SRC(m_src.get());

        if (!resource->wasCanceled()) {
            const ResourceError& error = resource->resourceError();
            GST_ERROR_OBJECT(src, "Have failure: %s", error.localizedDescription().utf8().data());
            GST_ELEMENT_ERROR(src, RESOURCE, FAILED, ("%s", error.localizedDescription().utf8().data()), (0));
        }
        gst_app_src_end_of_stream(src->priv->appsrc);
        return;
    }

    handleNotifyFinished();
}

ResourceHandleStreamingClient::ResourceHandleStreamingClient(WebKitWebSrc* src, const ResourceRequest& request)
    : StreamingClient(src)
{
    NetworkingContext* context = 0;
    if (s_cachedResourceLoader)
        context = s_cachedResourceLoader->frame()->loader()->networkingContext();
    m_resource = ResourceHandle::create(context, request, this, false, false);
}

ResourceHandleStreamingClient::~ResourceHandleStreamingClient()
{
    if (m_resource) {
        m_resource->cancel();
        m_resource.release();
        m_resource = 0;
    }
}

bool ResourceHandleStreamingClient::loadFailed() const
{
    return !m_resource;
}

void ResourceHandleStreamingClient::setDefersLoading(bool defers)
{
    if (m_resource)
        m_resource->setDefersLoading(defers);
}

char* ResourceHandleStreamingClient::getOrCreateReadBuffer(size_t requestedSize, size_t& actualSize)
{
    return createReadBuffer(requestedSize, actualSize);
}

void ResourceHandleStreamingClient::willSendRequest(ResourceHandle*, ResourceRequest&, const ResourceResponse&)
{
}

void ResourceHandleStreamingClient::didReceiveResponse(ResourceHandle*, const ResourceResponse& response)
{
    handleResponseReceived(response);
}

void ResourceHandleStreamingClient::didReceiveData(ResourceHandle*, const char* data, int length, int)
{
    handleDataReceived(data, length);
}

void ResourceHandleStreamingClient::didFinishLoading(ResourceHandle*, double)
{
    handleNotifyFinished();
}

void ResourceHandleStreamingClient::didFail(ResourceHandle*, const ResourceError& error)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(m_src.get());

    GST_ERROR_OBJECT(src, "Have failure: %s", error.localizedDescription().utf8().data());
    GST_ELEMENT_ERROR(src, RESOURCE, FAILED, ("%s", error.localizedDescription().utf8().data()), (0));
    gst_app_src_end_of_stream(src->priv->appsrc);
}

void ResourceHandleStreamingClient::wasBlocked(ResourceHandle*)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(m_src.get());
    GOwnPtr<gchar> uri;

    GST_ERROR_OBJECT(src, "Request was blocked");

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    uri.set(g_strdup(src->priv->uri));
    locker.unlock();

    GST_ELEMENT_ERROR(src, RESOURCE, OPEN_READ, ("Access to \"%s\" was blocked", uri.get()), (0));
}

void ResourceHandleStreamingClient::cannotShowURL(ResourceHandle*)
{
    WebKitWebSrc* src = WEBKIT_WEB_SRC(m_src.get());
    GOwnPtr<gchar> uri;

    GST_ERROR_OBJECT(src, "Cannot show URL");

    GMutexLocker locker(GST_OBJECT_GET_LOCK(src));
    uri.set(g_strdup(src->priv->uri));
    locker.unlock();

    GST_ELEMENT_ERROR(src, RESOURCE, OPEN_READ, ("Can't show \"%s\"", uri.get()), (0));
}

#endif // USE(GSTREAMER)

