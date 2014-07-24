/*
 *  Copyright (C) 2011, 2012 Igalia S.L
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

#ifndef WebKitWebAudioSourceGStreamer_h
#define WebKitWebAudioSourceGStreamer_h
#if USE(GSTREAMER)

#include <gst/gst.h>

#define WEBKIT_TYPE_WEB_AUDIO_SRC (webkit_web_audio_src_get_type())
#define WEBKIT_WEB_AUDIO_SRC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_WEB_AUDIO_SRC, WebKitWebAudioSrc))

typedef struct _WebKitWebAudioSrc WebKitWebAudioSrc;

GType webkit_web_audio_src_get_type();

#endif // USE(GSTREAMER)
#endif
