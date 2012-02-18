/*
 *  Copyright (C) 2010 Igalia S.L
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "GStreamerGWorld.h"
#if USE(GSTREAMER)

#include "GOwnPtrGStreamer.h"
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <gst/pbutils/pbutils.h>

#if PLATFORM(GTK)
#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h> // for GDK_WINDOW_XID
#endif
#endif

using namespace std;

namespace WebCore {

gboolean gstGWorldSyncMessageCallback(GstBus* bus, GstMessage* message, gpointer data)
{
    ASSERT(GST_MESSAGE_TYPE(message) == GST_MESSAGE_ELEMENT);

    GStreamerGWorld* gstGWorld = static_cast<GStreamerGWorld*>(data);

    if (gst_structure_has_name(message->structure, "prepare-xwindow-id")
        || gst_structure_has_name(message->structure, "have-ns-view"))
        gstGWorld->setWindowOverlay(message);
    return TRUE;
}

PassRefPtr<GStreamerGWorld> GStreamerGWorld::createGWorld(GstElement* pipeline)
{
    return adoptRef(new GStreamerGWorld(pipeline));
}

GStreamerGWorld::GStreamerGWorld(GstElement* pipeline)
    : m_pipeline(pipeline)
    , m_dynamicPadName(0)
{
    // XOverlay messages need to be handled synchronously.
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
    gst_bus_set_sync_handler(bus, gst_bus_sync_signal_handler, this);
    g_signal_connect(bus, "sync-message::element", G_CALLBACK(gstGWorldSyncMessageCallback), this);
    gst_object_unref(bus);
}

GStreamerGWorld::~GStreamerGWorld()
{
    exitFullscreen();

    m_pipeline = 0;
}

bool GStreamerGWorld::enterFullscreen()
{
    if (m_dynamicPadName)
        return false;

    if (!m_videoWindow)
        m_videoWindow = PlatformVideoWindow::createWindow();

    GstElement* platformVideoSink = gst_element_factory_make("autovideosink", "platformVideoSink");
    GstElement* colorspace = gst_element_factory_make("ffmpegcolorspace", "colorspace");
    GstElement* queue = gst_element_factory_make("queue", "queue");
    GstElement* videoScale = gst_element_factory_make("videoscale", "videoScale");

    // Get video sink bin and the tee inside.
    GOwnPtr<GstElement> videoSink;
    g_object_get(m_pipeline, "video-sink", &videoSink.outPtr(), NULL);
    GstElement* tee = gst_bin_get_by_name(GST_BIN(videoSink.get()), "videoTee");
    GstElement* valve = gst_bin_get_by_name(GST_BIN(videoSink.get()), "videoValve");

    g_object_set(valve, "drop-probability", 1.0, NULL);

    // Add and link a queue, ffmpegcolorspace, videoscale and sink in the bin.
    gst_bin_add_many(GST_BIN(videoSink.get()), platformVideoSink, videoScale, colorspace, queue, NULL);
#if GST_CHECK_VERSION(0, 10, 30)
    // Faster elements linking, if possible.
    gst_element_link_pads_full(queue, "src", colorspace, "sink", GST_PAD_LINK_CHECK_NOTHING);
    gst_element_link_pads_full(colorspace, "src", videoScale, "sink", GST_PAD_LINK_CHECK_NOTHING);
    gst_element_link_pads_full(videoScale, "src", platformVideoSink, "sink", GST_PAD_LINK_CHECK_NOTHING);
#else
    gst_element_link_many(queue, colorspace, videoScale, platformVideoSink, NULL);
#endif

    // Link a new src pad from tee to queue.
    GstPad* srcPad = gst_element_get_request_pad(tee, "src%d");
    GstPad* sinkPad = gst_element_get_static_pad(queue, "sink");
    gst_pad_link(srcPad, sinkPad);
    gst_object_unref(GST_OBJECT(sinkPad));

    m_dynamicPadName = gst_pad_get_name(srcPad);

    // Roll new elements to pipeline state.
    gst_element_sync_state_with_parent(queue);
    gst_element_sync_state_with_parent(colorspace);
    gst_element_sync_state_with_parent(videoScale);
    gst_element_sync_state_with_parent(platformVideoSink);

    gst_object_unref(tee);

    // Query the current media segment informations and send them towards
    // the new tee branch downstream.

    GstQuery* query = gst_query_new_segment(GST_FORMAT_TIME);
    gboolean queryResult = gst_element_query(m_pipeline, query);

#if GST_CHECK_VERSION(0, 10, 30)
    if (!queryResult) {
        gst_query_unref(query);
        gst_object_unref(GST_OBJECT(srcPad));
        return true;
    }
#else
    // GStreamer < 0.10.30 doesn't set the query result correctly, so
    // just ignore it to avoid a compilation warning.
    // See https://bugzilla.gnome.org/show_bug.cgi?id=620490.
    (void) queryResult;
#endif

    GstFormat format;
    gint64 position;
    if (!gst_element_query_position(m_pipeline, &format, &position))
        position = 0;

    gdouble rate;
    gint64 startValue, stopValue;
    gst_query_parse_segment(query, &rate, &format, &startValue, &stopValue);

    GstEvent* event = gst_event_new_new_segment(FALSE, rate, format, startValue, stopValue, position);
    gst_pad_push_event(srcPad, event);

    gst_query_unref(query);
    gst_object_unref(GST_OBJECT(srcPad));
    return true;
}

void GStreamerGWorld::exitFullscreen()
{
    if (!m_dynamicPadName)
        return;

    // Get video sink bin and the elements to remove.
    GOwnPtr<GstElement> videoSink;
    g_object_get(m_pipeline, "video-sink", &videoSink.outPtr(), NULL);
    GstElement* tee = gst_bin_get_by_name(GST_BIN(videoSink.get()), "videoTee");
    GstElement* platformVideoSink = gst_bin_get_by_name(GST_BIN(videoSink.get()), "platformVideoSink");
    GstElement* queue = gst_bin_get_by_name(GST_BIN(videoSink.get()), "queue");
    GstElement* colorspace = gst_bin_get_by_name(GST_BIN(videoSink.get()), "colorspace");
    GstElement* videoScale = gst_bin_get_by_name(GST_BIN(videoSink.get()), "videoScale");

    GstElement* valve = gst_bin_get_by_name(GST_BIN(videoSink.get()), "videoValve");

    g_object_set(valve, "drop-probability", 0.0, NULL);

    // Get pads to unlink and remove.
    GstPad* srcPad = gst_element_get_static_pad(tee, m_dynamicPadName);
    GstPad* sinkPad = gst_element_get_static_pad(queue, "sink");

    // Block data flow towards the pipeline branch to remove.
    gst_pad_set_blocked(srcPad, true);

    // Unlink and release request pad.
    gst_pad_unlink(srcPad, sinkPad);
    gst_element_release_request_pad(tee, srcPad);
    gst_object_unref(GST_OBJECT(srcPad));
    gst_object_unref(GST_OBJECT(sinkPad));

    // Unlink, remove and cleanup queue, ffmpegcolorspace, videoScale and sink.
    gst_element_unlink_many(queue, colorspace, videoScale, platformVideoSink, NULL);
    gst_bin_remove_many(GST_BIN(videoSink.get()), queue, colorspace, videoScale, platformVideoSink, NULL);
    gst_element_set_state(queue, GST_STATE_NULL);
    gst_element_set_state(colorspace, GST_STATE_NULL);
    gst_element_set_state(videoScale, GST_STATE_NULL);
    gst_element_set_state(platformVideoSink, GST_STATE_NULL);
    gst_object_unref(queue);
    gst_object_unref(colorspace);
    gst_object_unref(videoScale);
    gst_object_unref(platformVideoSink);

    gst_object_unref(tee);
    m_dynamicPadName = 0;
}

void GStreamerGWorld::setWindowOverlay(GstMessage* message)
{
    GstObject* sink = GST_MESSAGE_SRC(message);

    if (!GST_IS_X_OVERLAY(sink))
        return;

    if (g_object_class_find_property(G_OBJECT_GET_CLASS(sink), "force-aspect-ratio"))
        g_object_set(sink, "force-aspect-ratio", TRUE, NULL);

    if (m_videoWindow) {
        m_videoWindow->prepareForOverlay(message);

// gst_x_overlay_set_window_handle was introduced in -plugins-base
// 0.10.31, just like the macro for checking the version.
#ifdef GST_CHECK_PLUGINS_BASE_VERSION
        gst_x_overlay_set_window_handle(GST_X_OVERLAY(sink), m_videoWindow->videoWindowId());
#else
        gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(sink), m_videoWindow->videoWindowId());
#endif
    }
}

}
#endif // USE(GSTREAMER)
