/*
 *  Copyright (C) 2012 Igalia S.L
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
#include "GStreamerUtilities.h"

#if USE(GSTREAMER)
#include <gst/gst.h>
#include <wtf/gobject/GOwnPtr.h>

namespace WebCore {

bool initializeGStreamer()
{
#if GST_CHECK_VERSION(0, 10, 31)
    if (gst_is_initialized())
        return true;
#endif

    GOwnPtr<GError> error;
    // FIXME: We should probably pass the arguments from the command line.
    bool gstInitialized = gst_init_check(0, 0, &error.outPtr());
    ASSERT_WITH_MESSAGE(gstInitialized, "GStreamer initialization failed: %s", error ? error->message : "unknown error occurred");
    return gstInitialized;
}

}

#endif // USE(GSTREAMER)
