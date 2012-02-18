/*
 *  Copyright (C) 2009,2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

#ifndef WebKitWebSourceGStreamer_h
#define WebKitWebSourceGStreamer_h
#if USE(GSTREAMER)

#include "Frame.h"
#include <gst/gst.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WEB_SRC            (webkit_web_src_get_type ())
#define WEBKIT_WEB_SRC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBKIT_TYPE_WEB_SRC, WebKitWebSrc))
#define WEBKIT_WEB_SRC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WEBKIT_TYPE_WEB_SRC, WebKitWebSrcClass))
#define WEBKIT_IS_WEB_SRC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBKIT_TYPE_WEB_SRC))
#define WEBKIT_IS_WEB_SRC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WEBKIT_TYPE_WEB_SRC))

typedef struct _WebKitWebSrc        WebKitWebSrc;
typedef struct _WebKitWebSrcClass   WebKitWebSrcClass;
typedef struct _WebKitWebSrcPrivate WebKitWebSrcPrivate;

struct _WebKitWebSrc {
    GstBin parent;

    WebKitWebSrcPrivate *priv;
};

struct _WebKitWebSrcClass {
    GstBinClass parentClass;
};

GType webkit_web_src_get_type(void);
void  webKitWebSrcSetFrame(WebKitWebSrc* src, WebCore::Frame* frame);

G_END_DECLS

#endif // USE(GSTREAMER)
#endif
