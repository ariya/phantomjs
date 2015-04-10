/*
 *  Copyright (C) 2007 OpenedHand
 *  Copyright (C) 2007 Alp Toker <alp@atoker.com>
 *  Copyright (C) 2009, 2010, 2011, 2012 Igalia S.L
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

/*
 *
 * WebKitVideoSink is a GStreamer sink element that triggers
 * repaints in the WebKit GStreamer media player for the
 * current video buffer.
 */

#include "config.h"
#include "VideoSinkGStreamer.h"

#if ENABLE(VIDEO) && USE(GSTREAMER)
#include "GRefPtrGStreamer.h"
#include "GStreamerVersioning.h"
#include "IntSize.h"
#include <glib.h>
#include <gst/gst.h>
#ifdef GST_API_VERSION_1
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>
#endif
#include <wtf/FastAllocBase.h>

// CAIRO_FORMAT_RGB24 used to render the video buffers is little/big endian dependant.
#ifdef GST_API_VERSION_1
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define GST_CAPS_FORMAT "{ BGRx, BGRA }"
#else
#define GST_CAPS_FORMAT "{ xRGB, ARGB }"
#endif
#if GST_CHECK_VERSION(1, 1, 0)
#define GST_FEATURED_CAPS GST_VIDEO_CAPS_MAKE_WITH_FEATURES(GST_CAPS_FEATURE_META_GST_VIDEO_GL_TEXTURE_UPLOAD_META, GST_CAPS_FORMAT) ";"
#else
#define GST_FEATURED_CAPS
#endif
#endif // GST_API_VERSION_1

#ifdef GST_API_VERSION_1
#define WEBKIT_VIDEO_SINK_PAD_CAPS GST_FEATURED_CAPS GST_VIDEO_CAPS_MAKE(GST_CAPS_FORMAT)
#else
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define WEBKIT_VIDEO_SINK_PAD_CAPS GST_VIDEO_CAPS_BGRx ";" GST_VIDEO_CAPS_BGRA
#else
#define WEBKIT_VIDEO_SINK_PAD_CAPS GST_VIDEO_CAPS_xRGB ";" GST_VIDEO_CAPS_ARGB
#endif
#endif // GST_API_VERSION_1

static GstStaticPadTemplate s_sinkTemplate = GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(WEBKIT_VIDEO_SINK_PAD_CAPS));


GST_DEBUG_CATEGORY_STATIC(webkitVideoSinkDebug);
#define GST_CAT_DEFAULT webkitVideoSinkDebug

enum {
    REPAINT_REQUESTED,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_CAPS
};

static guint webkitVideoSinkSignals[LAST_SIGNAL] = { 0, };

struct _WebKitVideoSinkPrivate {
    GstBuffer* buffer;
    guint timeoutId;
    GMutex* bufferMutex;
    GCond* dataCondition;

#ifdef GST_API_VERSION_1
    GstVideoInfo info;
#endif

#if USE(NATIVE_FULLSCREEN_VIDEO)
    WebCore::GStreamerGWorld* gstGWorld;
#endif

    GstCaps* currentCaps;

    // If this is TRUE all processing should finish ASAP
    // This is necessary because there could be a race between
    // unlock() and render(), where unlock() wins, signals the
    // GCond, then render() tries to render a frame although
    // everything else isn't running anymore. This will lead
    // to deadlocks because render() holds the stream lock.
    //
    // Protected by the buffer mutex
    bool unlocked;
};

#define webkit_video_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(WebKitVideoSink, webkit_video_sink, GST_TYPE_VIDEO_SINK, GST_DEBUG_CATEGORY_INIT(webkitVideoSinkDebug, "webkitsink", 0, "webkit video sink"));


static void webkit_video_sink_init(WebKitVideoSink* sink)
{
    sink->priv = G_TYPE_INSTANCE_GET_PRIVATE(sink, WEBKIT_TYPE_VIDEO_SINK, WebKitVideoSinkPrivate);
#if GLIB_CHECK_VERSION(2, 31, 0)
    sink->priv->dataCondition = WTF::fastNew<GCond>();
    g_cond_init(sink->priv->dataCondition);
    sink->priv->bufferMutex = WTF::fastNew<GMutex>();
    g_mutex_init(sink->priv->bufferMutex);
#else
    sink->priv->dataCondition = g_cond_new();
    sink->priv->bufferMutex = g_mutex_new();
#endif

#ifdef GST_API_VERSION_1
    gst_video_info_init(&sink->priv->info);
#endif
}

static gboolean webkitVideoSinkTimeoutCallback(gpointer data)
{
    WebKitVideoSink* sink = reinterpret_cast<WebKitVideoSink*>(data);
    WebKitVideoSinkPrivate* priv = sink->priv;

    g_mutex_lock(priv->bufferMutex);
    GstBuffer* buffer = priv->buffer;
    priv->buffer = 0;
    priv->timeoutId = 0;

    if (!buffer || priv->unlocked || UNLIKELY(!GST_IS_BUFFER(buffer))) {
        g_cond_signal(priv->dataCondition);
        g_mutex_unlock(priv->bufferMutex);
        return FALSE;
    }

    g_signal_emit(sink, webkitVideoSinkSignals[REPAINT_REQUESTED], 0, buffer);
    gst_buffer_unref(buffer);
    g_cond_signal(priv->dataCondition);
    g_mutex_unlock(priv->bufferMutex);

    return FALSE;
}

static GstFlowReturn webkitVideoSinkRender(GstBaseSink* baseSink, GstBuffer* buffer)
{
    WebKitVideoSink* sink = WEBKIT_VIDEO_SINK(baseSink);
    WebKitVideoSinkPrivate* priv = sink->priv;

    g_mutex_lock(priv->bufferMutex);

    if (priv->unlocked) {
        g_mutex_unlock(priv->bufferMutex);
        return GST_FLOW_OK;
    }

#if USE(NATIVE_FULLSCREEN_VIDEO)
    // Ignore buffers if the video is already in fullscreen using
    // another sink.
    if (priv->gstGWorld->isFullscreen()) {
        g_mutex_unlock(priv->bufferMutex);
        return GST_FLOW_OK;
    }
#endif

    priv->buffer = gst_buffer_ref(buffer);

#ifndef GST_API_VERSION_1
    // For the unlikely case where the buffer has no caps, the caps
    // are implicitely the caps of the pad. This shouldn't happen.
    if (UNLIKELY(!GST_BUFFER_CAPS(buffer))) {
        buffer = priv->buffer = gst_buffer_make_metadata_writable(priv->buffer);
        gst_buffer_set_caps(priv->buffer, GST_PAD_CAPS(GST_BASE_SINK_PAD(baseSink)));
    }

    GRefPtr<GstCaps> caps = GST_BUFFER_CAPS(buffer);
#else
    GRefPtr<GstCaps> caps = adoptGRef(gst_video_info_to_caps(&priv->info));
#endif

    GstVideoFormat format;
    WebCore::IntSize size;
    int pixelAspectRatioNumerator, pixelAspectRatioDenominator, stride;
    if (!getVideoSizeAndFormatFromCaps(caps.get(), size, format, pixelAspectRatioNumerator, pixelAspectRatioDenominator, stride)) {
        gst_buffer_unref(buffer);
        g_mutex_unlock(priv->bufferMutex);
        return GST_FLOW_ERROR;
    }

    // Cairo's ARGB has pre-multiplied alpha while GStreamer's doesn't.
    // Here we convert to Cairo's ARGB.
    if (format == GST_VIDEO_FORMAT_ARGB || format == GST_VIDEO_FORMAT_BGRA) {
        // Because GstBaseSink::render() only owns the buffer reference in the
        // method scope we can't use gst_buffer_make_writable() here. Also
        // The buffer content should not be changed here because the same buffer
        // could be passed multiple times to this method (in theory).

        GstBuffer* newBuffer = createGstBuffer(buffer);

        // Check if allocation failed.
        if (UNLIKELY(!newBuffer)) {
            g_mutex_unlock(priv->bufferMutex);
            return GST_FLOW_ERROR;
        }

        // We don't use Color::premultipliedARGBFromColor() here because
        // one function call per video pixel is just too expensive:
        // For 720p/PAL for example this means 1280*720*25=23040000
        // function calls per second!
#ifndef GST_API_VERSION_1
        const guint8* source = GST_BUFFER_DATA(buffer);
        guint8* destination = GST_BUFFER_DATA(newBuffer);
#else
        GstMapInfo sourceInfo;
        GstMapInfo destinationInfo;
        gst_buffer_map(buffer, &sourceInfo, GST_MAP_READ);
        const guint8* source = const_cast<guint8*>(sourceInfo.data);
        gst_buffer_map(newBuffer, &destinationInfo, GST_MAP_WRITE);
        guint8* destination = static_cast<guint8*>(destinationInfo.data);
#endif

        for (int x = 0; x < size.height(); x++) {
            for (int y = 0; y < size.width(); y++) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                unsigned short alpha = source[3];
                destination[0] = (source[0] * alpha + 128) / 255;
                destination[1] = (source[1] * alpha + 128) / 255;
                destination[2] = (source[2] * alpha + 128) / 255;
                destination[3] = alpha;
#else
                unsigned short alpha = source[0];
                destination[0] = alpha;
                destination[1] = (source[1] * alpha + 128) / 255;
                destination[2] = (source[2] * alpha + 128) / 255;
                destination[3] = (source[3] * alpha + 128) / 255;
#endif
                source += 4;
                destination += 4;
            }
        }

#ifdef GST_API_VERSION_1
        gst_buffer_unmap(buffer, &sourceInfo);
        gst_buffer_unmap(newBuffer, &destinationInfo);
#endif
        gst_buffer_unref(buffer);
        buffer = priv->buffer = newBuffer;
    }

    // This should likely use a lower priority, but glib currently starves
    // lower priority sources.
    // See: https://bugzilla.gnome.org/show_bug.cgi?id=610830.
    priv->timeoutId = g_timeout_add_full(G_PRIORITY_DEFAULT, 0, webkitVideoSinkTimeoutCallback,
                                          gst_object_ref(sink), reinterpret_cast<GDestroyNotify>(gst_object_unref));

    g_cond_wait(priv->dataCondition, priv->bufferMutex);
    g_mutex_unlock(priv->bufferMutex);
    return GST_FLOW_OK;
}

static void webkitVideoSinkDispose(GObject* object)
{
    WebKitVideoSink* sink = WEBKIT_VIDEO_SINK(object);
    WebKitVideoSinkPrivate* priv = sink->priv;

    if (priv->dataCondition) {
#if GLIB_CHECK_VERSION(2, 31, 0)
        g_cond_clear(priv->dataCondition);
        WTF::fastDelete(priv->dataCondition);
#else
        g_cond_free(priv->dataCondition);
#endif
        priv->dataCondition = 0;
    }

    if (priv->bufferMutex) {
#if GLIB_CHECK_VERSION(2, 31, 0)
        g_mutex_clear(priv->bufferMutex);
        WTF::fastDelete(priv->bufferMutex);
#else
        g_mutex_free(priv->bufferMutex);
#endif
        priv->bufferMutex = 0;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void webkitVideoSinkGetProperty(GObject* object, guint propertyId, GValue* value, GParamSpec* parameterSpec)
{
    WebKitVideoSink* sink = WEBKIT_VIDEO_SINK(object);
    WebKitVideoSinkPrivate* priv = sink->priv;

    switch (propertyId) {
    case PROP_CAPS: {
        GstCaps* caps = priv->currentCaps;
        if (caps)
            gst_caps_ref(caps);
        g_value_take_boxed(value, caps);
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyId, parameterSpec);
    }
}

static void unlockBufferMutex(WebKitVideoSinkPrivate* priv)
{
    g_mutex_lock(priv->bufferMutex);

    if (priv->buffer) {
        gst_buffer_unref(priv->buffer);
        priv->buffer = 0;
    }

    priv->unlocked = true;

    g_cond_signal(priv->dataCondition);
    g_mutex_unlock(priv->bufferMutex);
}

static gboolean webkitVideoSinkUnlock(GstBaseSink* baseSink)
{
    WebKitVideoSink* sink = WEBKIT_VIDEO_SINK(baseSink);

    unlockBufferMutex(sink->priv);

    return GST_CALL_PARENT_WITH_DEFAULT(GST_BASE_SINK_CLASS, unlock, (baseSink), TRUE);
}

static gboolean webkitVideoSinkUnlockStop(GstBaseSink* baseSink)
{
    WebKitVideoSinkPrivate* priv = WEBKIT_VIDEO_SINK(baseSink)->priv;

    g_mutex_lock(priv->bufferMutex);
    priv->unlocked = false;
    g_mutex_unlock(priv->bufferMutex);

    return GST_CALL_PARENT_WITH_DEFAULT(GST_BASE_SINK_CLASS, unlock_stop, (baseSink), TRUE);
}

static gboolean webkitVideoSinkStop(GstBaseSink* baseSink)
{
    WebKitVideoSinkPrivate* priv = WEBKIT_VIDEO_SINK(baseSink)->priv;

    unlockBufferMutex(priv);

    if (priv->currentCaps) {
        gst_caps_unref(priv->currentCaps);
        priv->currentCaps = 0;
    }

    return TRUE;
}

static gboolean webkitVideoSinkStart(GstBaseSink* baseSink)
{
    WebKitVideoSinkPrivate* priv = WEBKIT_VIDEO_SINK(baseSink)->priv;

    g_mutex_lock(priv->bufferMutex);
    priv->unlocked = false;
    g_mutex_unlock(priv->bufferMutex);
    return TRUE;
}

static gboolean webkitVideoSinkSetCaps(GstBaseSink* baseSink, GstCaps* caps)
{
    WebKitVideoSink* sink = WEBKIT_VIDEO_SINK(baseSink);
    WebKitVideoSinkPrivate* priv = sink->priv;

    GST_DEBUG_OBJECT(sink, "Current caps %" GST_PTR_FORMAT ", setting caps %" GST_PTR_FORMAT, priv->currentCaps, caps);

#ifdef GST_API_VERSION_1
    GstVideoInfo info;
    if (!gst_video_info_from_caps(&info, caps)) {
        GST_ERROR_OBJECT(sink, "Invalid caps %" GST_PTR_FORMAT, caps);
        return FALSE;
    }
#endif

    gst_caps_replace(&priv->currentCaps, caps);
    return TRUE;
}

#ifdef GST_API_VERSION_1
static gboolean webkitVideoSinkProposeAllocation(GstBaseSink* baseSink, GstQuery* query)
{
    GstCaps* caps;
    gst_query_parse_allocation(query, &caps, 0);
    if (!caps)
        return FALSE;

    WebKitVideoSink* sink = WEBKIT_VIDEO_SINK(baseSink);
    if (!gst_video_info_from_caps(&sink->priv->info, caps))
        return FALSE;

    gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, 0);
    gst_query_add_allocation_meta(query, GST_VIDEO_CROP_META_API_TYPE, 0);
#if GST_CHECK_VERSION(1, 1, 0)
    gst_query_add_allocation_meta(query, GST_VIDEO_GL_TEXTURE_UPLOAD_META_API_TYPE, 0);
#endif
    return TRUE;
}
#endif

#ifndef GST_API_VERSION_1
static void webkitVideoSinkMarshalVoidAndMiniObject(GClosure* closure, GValue*, guint parametersNumber, const GValue* parameterValues, gpointer, gpointer marshalData)
{
    typedef void (*marshalfunc_VOID__MINIOBJECT) (gpointer obj, gpointer arg1, gpointer data2);
    marshalfunc_VOID__MINIOBJECT callback;
    GCClosure* cclosure = reinterpret_cast<GCClosure*>(closure);
    gpointer data1, data2;

    g_return_if_fail(parametersNumber == 2);

    if (G_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer(parameterValues + 0);
    } else {
        data1 = g_value_peek_pointer(parameterValues + 0);
        data2 = closure->data;
    }

    callback = (marshalfunc_VOID__MINIOBJECT) (marshalData ? marshalData : cclosure->callback);
    callback(data1, gst_value_get_mini_object(parameterValues + 1), data2);
}
#endif

static void webkit_video_sink_class_init(WebKitVideoSinkClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    GstBaseSinkClass* baseSinkClass = GST_BASE_SINK_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);

    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&s_sinkTemplate));
    setGstElementClassMetadata(elementClass, "WebKit video sink", "Sink/Video", "Sends video data from a GStreamer pipeline to a Cairo surface", "Alp Toker <alp@atoker.com>");

    g_type_class_add_private(klass, sizeof(WebKitVideoSinkPrivate));

    gobjectClass->dispose = webkitVideoSinkDispose;
    gobjectClass->get_property = webkitVideoSinkGetProperty;

    baseSinkClass->unlock = webkitVideoSinkUnlock;
    baseSinkClass->unlock_stop = webkitVideoSinkUnlockStop;
    baseSinkClass->render = webkitVideoSinkRender;
    baseSinkClass->preroll = webkitVideoSinkRender;
    baseSinkClass->stop = webkitVideoSinkStop;
    baseSinkClass->start = webkitVideoSinkStart;
    baseSinkClass->set_caps = webkitVideoSinkSetCaps;
#ifdef GST_API_VERSION_1
    baseSinkClass->propose_allocation = webkitVideoSinkProposeAllocation;
#endif

    g_object_class_install_property(gobjectClass, PROP_CAPS,
        g_param_spec_boxed("current-caps", "Current-Caps", "Current caps", GST_TYPE_CAPS, G_PARAM_READABLE));

    webkitVideoSinkSignals[REPAINT_REQUESTED] = g_signal_new("repaint-requested",
            G_TYPE_FROM_CLASS(klass),
            static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0, // Class offset
            0, // Accumulator
            0, // Accumulator data
#ifndef GST_API_VERSION_1
            webkitVideoSinkMarshalVoidAndMiniObject,
#else
            g_cclosure_marshal_generic,
#endif
            G_TYPE_NONE, // Return type
            1, // Only one parameter
            GST_TYPE_BUFFER);
}


#if USE(NATIVE_FULLSCREEN_VIDEO)
GstElement* webkitVideoSinkNew(WebCore::GStreamerGWorld* gstGWorld)
{
    GstElement* element = GST_ELEMENT(g_object_new(WEBKIT_TYPE_VIDEO_SINK, 0));
    WEBKIT_VIDEO_SINK(element)->priv->gstGWorld = gstGWorld;
    return element;
}
#else
GstElement* webkitVideoSinkNew()
{
    return GST_ELEMENT(g_object_new(WEBKIT_TYPE_VIDEO_SINK, 0));
}
#endif

#endif // ENABLE(VIDEO) && USE(GSTREAMER)
