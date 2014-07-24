/*
 * Copyright (C) 2012 Igalia, S.L.
 * Copyright (C) 2013 Collabora Ltd.
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

#include "GStreamerVersioning.h"

#if USE(GSTREAMER)
#include "IntSize.h"

#ifdef GST_API_VERSION_1
#include <gst/audio/audio.h>
#else
#include <gst/audio/multichannel.h>
#endif

#ifdef GST_API_VERSION_1
const char* webkitGstMapInfoQuarkString = "webkit-gst-map-info";
#endif

void webkitGstObjectRefSink(GstObject* gstObject)
{
#ifdef GST_API_VERSION_1
    gst_object_ref_sink(gstObject);
#else
    gst_object_ref(gstObject);
    gst_object_sink(gstObject);
#endif
}

GstPad* webkitGstGhostPadFromStaticTemplate(GstStaticPadTemplate* staticPadTemplate, const gchar* name, GstPad* target)
{
    GstPad* pad;
    GstPadTemplate* padTemplate = gst_static_pad_template_get(staticPadTemplate);

    if (target)
        pad = gst_ghost_pad_new_from_template(name, target, padTemplate);
    else
        pad = gst_ghost_pad_new_no_target_from_template(name, padTemplate);

#ifdef GST_API_VERSION_1
    gst_object_unref(padTemplate);
#endif

    return pad;
}

GRefPtr<GstCaps> webkitGstGetPadCaps(GstPad* pad)
{
    if (!pad)
        return 0;

#ifdef GST_API_VERSION_1
    return adoptGRef(gst_pad_get_current_caps(pad)); // gst_pad_get_current_caps return a new reference.
#else
    return GST_PAD_CAPS(pad);
#endif
}

GRefPtr<GstBus> webkitGstPipelineGetBus(GstPipeline* pipeline)
{
#ifdef GST_API_VERSION_1
    return adoptGRef(gst_pipeline_get_bus(pipeline));
#else
    // gst_pipeline_get_bus returns a floating reference in
    // gstreamer 0.10 so we should not adopt.
    return gst_pipeline_get_bus(pipeline);
#endif
}

#if ENABLE(VIDEO)
bool getVideoSizeAndFormatFromCaps(GstCaps* caps, WebCore::IntSize& size, GstVideoFormat& format, int& pixelAspectRatioNumerator, int& pixelAspectRatioDenominator, int& stride)
{
#ifdef GST_API_VERSION_1
    GstVideoInfo info;

    if (!gst_caps_is_fixed(caps) || !gst_video_info_from_caps(&info, caps))
        return false;

    format = GST_VIDEO_INFO_FORMAT(&info);
    size.setWidth(GST_VIDEO_INFO_WIDTH(&info));
    size.setHeight(GST_VIDEO_INFO_HEIGHT(&info));
    pixelAspectRatioNumerator = GST_VIDEO_INFO_PAR_N(&info);
    pixelAspectRatioDenominator = GST_VIDEO_INFO_PAR_D(&info);
    stride = GST_VIDEO_INFO_PLANE_STRIDE(&info, 0);
#else
    gint width, height;
    if (!GST_IS_CAPS(caps) || !gst_caps_is_fixed(caps)
        || !gst_video_format_parse_caps(caps, &format, &width, &height)
        || !gst_video_parse_caps_pixel_aspect_ratio(caps, &pixelAspectRatioNumerator,
                                                    &pixelAspectRatioDenominator))
        return false;
    size.setWidth(width);
    size.setHeight(height);
    stride = size.width() * 4;
#endif

    return true;
}
#endif

GstBuffer* createGstBuffer(GstBuffer* buffer)
{
#ifndef GST_API_VERSION_1
    GstBuffer* newBuffer = gst_buffer_try_new_and_alloc(GST_BUFFER_SIZE(buffer));
#else
    gsize bufferSize = gst_buffer_get_size(buffer);
    GstBuffer* newBuffer = gst_buffer_new_and_alloc(bufferSize);
#endif

    if (!newBuffer)
        return 0;

#ifndef GST_API_VERSION_1
    gst_buffer_copy_metadata(newBuffer, buffer, static_cast<GstBufferCopyFlags>(GST_BUFFER_COPY_ALL));
#else
    gst_buffer_copy_into(newBuffer, buffer, static_cast<GstBufferCopyFlags>(GST_BUFFER_COPY_METADATA), 0, bufferSize);
#endif
    return newBuffer;
}

GstBuffer* createGstBufferForData(const char* data, int length)
{
    GstBuffer* buffer = gst_buffer_new_and_alloc(length);

#ifdef GST_API_VERSION_1
    gst_buffer_fill(buffer, 0, data, length);
#else
    memcpy(GST_BUFFER_DATA(buffer), data, length);
#endif

    return buffer;
}

int getGstBufferSize(GstBuffer* buffer)
{
#ifdef GST_API_VERSION_1
    return gst_buffer_get_size(buffer);
#else
    return GST_BUFFER_SIZE(buffer);
#endif
}

void setGstBufferSize(GstBuffer* buffer, int newSize)
{
#ifdef GST_API_VERSION_1
    gst_buffer_set_size(buffer, static_cast<gssize>(newSize));
#else
    GST_BUFFER_SIZE(buffer) = static_cast<gsize>(newSize);
#endif
}

char* getGstBufferDataPointer(GstBuffer* buffer)
{
#ifdef GST_API_VERSION_1
    GstMiniObject* miniObject = reinterpret_cast<GstMiniObject*>(buffer);
    GstMapInfo* mapInfo = static_cast<GstMapInfo*>(gst_mini_object_get_qdata(miniObject, g_quark_from_static_string(webkitGstMapInfoQuarkString)));
    return reinterpret_cast<char*>(mapInfo->data);
#else
    return reinterpret_cast<char*>(GST_BUFFER_DATA(buffer));
#endif
}

#ifdef GST_API_VERSION_1
void mapGstBuffer(GstBuffer* buffer)
{
    GstMapInfo* mapInfo = g_slice_new(GstMapInfo);
    if (!gst_buffer_map(buffer, mapInfo, GST_MAP_WRITE)) {
        g_slice_free(GstMapInfo, mapInfo);
        gst_buffer_unref(buffer);
        return;
    }

    GstMiniObject* miniObject = reinterpret_cast<GstMiniObject*>(buffer);
    gst_mini_object_set_qdata(miniObject, g_quark_from_static_string(webkitGstMapInfoQuarkString), mapInfo, 0);
}

void unmapGstBuffer(GstBuffer* buffer)
{
    GstMiniObject* miniObject = reinterpret_cast<GstMiniObject*>(buffer);
    GstMapInfo* mapInfo = static_cast<GstMapInfo*>(gst_mini_object_steal_qdata(miniObject, g_quark_from_static_string(webkitGstMapInfoQuarkString)));

    if (!mapInfo)
        return;

    gst_buffer_unmap(buffer, mapInfo);
    g_slice_free(GstMapInfo, mapInfo);
}
#endif

void setGstElementClassMetadata(GstElementClass* elementClass, const char* name, const char* longName, const char* description, const char* author)
{
#ifdef GST_API_VERSION_1
    gst_element_class_set_metadata(elementClass, name, longName, description, author);
#else
    gst_element_class_set_details_simple(elementClass, name, longName, description, author);
#endif
}

bool gstObjectIsFloating(GstObject* gstObject)
{
#ifdef GST_API_VERSION_1
    return g_object_is_floating(G_OBJECT(gstObject));
#else
    return GST_OBJECT_IS_FLOATING(gstObject);
#endif
}

void notifyGstTagsOnPad(GstElement* element, GstPad* pad, GstTagList* tags)
{
#ifdef GST_API_VERSION_1
    UNUSED_PARAM(element);
    gst_pad_push_event(GST_PAD_CAST(pad), gst_event_new_tag(tags));
#else
    gst_element_found_tags_for_pad(element, pad, tags);
#endif
}

#if ENABLE(WEB_AUDIO)
GstCaps* getGstAudioCaps(int channels, float sampleRate)
{
#ifdef GST_API_VERSION_1
    return gst_caps_new_simple("audio/x-raw", "rate", G_TYPE_INT, static_cast<int>(sampleRate),
        "channels", G_TYPE_INT, channels,
        "format", G_TYPE_STRING, gst_audio_format_to_string(GST_AUDIO_FORMAT_F32),
        "layout", G_TYPE_STRING, "interleaved", NULL);
#else
    return gst_caps_new_simple("audio/x-raw-float", "rate", G_TYPE_INT, static_cast<int>(sampleRate),
        "channels", G_TYPE_INT, channels,
        "endianness", G_TYPE_INT, G_BYTE_ORDER,
        "width", G_TYPE_INT, 32, NULL);
#endif
}
#endif

#endif // USE(GSTREAMER)
