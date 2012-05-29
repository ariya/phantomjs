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


#ifndef GStreamerGWorld_h
#define GStreamerGWorld_h
#if USE(GSTREAMER)

#include "PlatformVideoWindow.h"
#include "RefCounted.h"
#include "RefPtr.h"
#include <glib.h>

typedef struct _GstElement GstElement;
typedef struct _GstMessage GstMessage;
typedef struct _GstBus GstBus;
typedef struct _GstBin GstBin;

namespace WebCore {

class MediaPlayerPrivateGStreamer;

gboolean gstGWorldSyncMessageCallback(GstBus* bus, GstMessage* message, gpointer data);

class GStreamerGWorld : public RefCounted<GStreamerGWorld> {
    friend gboolean gstGWorldSyncMessageCallback(GstBus* bus, GstMessage* message, gpointer data);

public:
    static PassRefPtr<GStreamerGWorld> createGWorld(GstElement*);
    ~GStreamerGWorld();

    GstElement* pipeline() const { return m_pipeline; }

    // Returns the full-screen window created
    bool enterFullscreen();
    void exitFullscreen();

    void setWindowOverlay(GstMessage* message);
    PlatformVideoWindow* platformVideoWindow() const { return m_videoWindow.get(); }

private:
    GStreamerGWorld(GstElement*);
    GstElement* m_pipeline;
    RefPtr<PlatformVideoWindow> m_videoWindow;
    gchar* m_dynamicPadName;
};

}
#endif // USE(GSTREAMER)
#endif
