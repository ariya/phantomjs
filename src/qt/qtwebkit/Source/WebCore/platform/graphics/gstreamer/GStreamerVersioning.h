/*
 * Copyright (C) 2012 Igalia, S.L.
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

#ifndef GStreamerVersioning_h
#define GStreamerVersioning_h
#if USE(GSTREAMER)

#include "GRefPtrGStreamer.h"
#include <gst/gst.h>
#include <gst/video/video.h>

namespace WebCore {
class IntSize;
};

void webkitGstObjectRefSink(GstObject*);
GstPad* webkitGstGhostPadFromStaticTemplate(GstStaticPadTemplate*, const gchar* name, GstPad* target);
GRefPtr<GstCaps> webkitGstGetPadCaps(GstPad*);
GRefPtr<GstBus> webkitGstPipelineGetBus(GstPipeline*);
#if ENABLE(VIDEO)
bool getVideoSizeAndFormatFromCaps(GstCaps*, WebCore::IntSize&, GstVideoFormat&, int& pixelAspectRatioNumerator, int& pixelAspectRatioDenominator, int& stride);
#endif
GstBuffer* createGstBuffer(GstBuffer*);
GstBuffer* createGstBufferForData(const char* data, int length);
int getGstBufferSize(GstBuffer*);
void setGstBufferSize(GstBuffer*, int newSize);
char* getGstBufferDataPointer(GstBuffer*);
#ifdef GST_API_VERSION_1
void mapGstBuffer(GstBuffer*);
void unmapGstBuffer(GstBuffer*);
#endif
void setGstElementClassMetadata(GstElementClass*, const char* name, const char* longName, const char* description, const char* author);
bool gstObjectIsFloating(GstObject*);
void notifyGstTagsOnPad(GstElement*, GstPad*, GstTagList*);
#if ENABLE(WEB_AUDIO)
GstCaps* getGstAudioCaps(int channels, float sampleRate);
#endif
#endif // USE(GSTREAMER)
#endif // GStreamerVersioning_h
