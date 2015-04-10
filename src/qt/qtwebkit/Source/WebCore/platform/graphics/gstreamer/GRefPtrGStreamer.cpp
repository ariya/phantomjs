/*
 *  Copyright (C) 2011 Igalia S.L
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
#include "GRefPtrGStreamer.h"
#include "GStreamerVersioning.h"

#if USE(GSTREAMER)
#include <gst/gstelement.h>

namespace WTF {

template <> GRefPtr<GstElement> adoptGRef(GstElement* ptr)
{
    ASSERT(!ptr || !gstObjectIsFloating(GST_OBJECT(ptr)));
    return GRefPtr<GstElement>(ptr, GRefPtrAdopt);
}

template <> GstElement* refGPtr<GstElement>(GstElement* ptr)
{
    if (ptr)
        webkitGstObjectRefSink(GST_OBJECT(ptr));

    return ptr;
}

template <> void derefGPtr<GstElement>(GstElement* ptr)
{
    if (ptr)
        gst_object_unref(ptr);
}

template <> GRefPtr<GstPad> adoptGRef(GstPad* ptr)
{
    ASSERT(!ptr || !gstObjectIsFloating(GST_OBJECT(ptr)));
    return GRefPtr<GstPad>(ptr, GRefPtrAdopt);
}

template <> GstPad* refGPtr<GstPad>(GstPad* ptr)
{
    if (ptr)
        webkitGstObjectRefSink(GST_OBJECT(ptr));

    return ptr;
}

template <> void derefGPtr<GstPad>(GstPad* ptr)
{
    if (ptr)
        gst_object_unref(GST_OBJECT(ptr));
}

template <> GRefPtr<GstPadTemplate> adoptGRef(GstPadTemplate* ptr)
{
    ASSERT(!ptr || !gstObjectIsFloating(GST_OBJECT(ptr)));
    return GRefPtr<GstPadTemplate>(ptr, GRefPtrAdopt);
}

template <> GstPadTemplate* refGPtr<GstPadTemplate>(GstPadTemplate* ptr)
{
    if (ptr)
        webkitGstObjectRefSink(GST_OBJECT(ptr));

    return ptr;
}

template <> void derefGPtr<GstPadTemplate>(GstPadTemplate* ptr)
{
    if (ptr)
        gst_object_unref(GST_OBJECT(ptr));
}

template <> GRefPtr<GstCaps> adoptGRef(GstCaps* ptr)
{
    return GRefPtr<GstCaps>(ptr, GRefPtrAdopt);
}

template <> GstCaps* refGPtr<GstCaps>(GstCaps* ptr)
{
    if (ptr)
        gst_caps_ref(ptr);
    return ptr;
}

template <> void derefGPtr<GstCaps>(GstCaps* ptr)
{
    if (ptr)
        gst_caps_unref(ptr);
}


template <> GRefPtr<GstTask> adoptGRef(GstTask* ptr)
{
    ASSERT(!gstObjectIsFloating(GST_OBJECT(ptr)));
    return GRefPtr<GstTask>(ptr, GRefPtrAdopt);
}

template <> GstTask* refGPtr<GstTask>(GstTask* ptr)
{
    if (ptr)
        webkitGstObjectRefSink(GST_OBJECT(ptr));

    return ptr;
}

template <> void derefGPtr<GstTask>(GstTask* ptr)
{
    if (ptr)
        gst_object_unref(ptr);
}

template <> GRefPtr<GstBus> adoptGRef(GstBus* ptr)
{
    ASSERT(!gstObjectIsFloating(GST_OBJECT(ptr)));
    return GRefPtr<GstBus>(ptr, GRefPtrAdopt);
}

template <> GstBus* refGPtr<GstBus>(GstBus* ptr)
{
    if (ptr)
        webkitGstObjectRefSink(GST_OBJECT(ptr));

    return ptr;
}

template <> void derefGPtr<GstBus>(GstBus* ptr)
{
    if (ptr)
        gst_object_unref(ptr);
}

template <> GRefPtr<GstElementFactory> adoptGRef(GstElementFactory* ptr)
{
    ASSERT(!gstObjectIsFloating(GST_OBJECT(ptr)));
    return GRefPtr<GstElementFactory>(ptr, GRefPtrAdopt);
}

template <> GstElementFactory* refGPtr<GstElementFactory>(GstElementFactory* ptr)
{
    if (ptr)
        webkitGstObjectRefSink(GST_OBJECT(ptr));

    return ptr;
}

template <> void derefGPtr<GstElementFactory>(GstElementFactory* ptr)
{
    if (ptr)
        gst_object_unref(ptr);
}

template<> GRefPtr<GstBuffer> adoptGRef(GstBuffer* ptr)
{
    return GRefPtr<GstBuffer>(ptr, GRefPtrAdopt);
}

template<> GstBuffer* refGPtr<GstBuffer>(GstBuffer* ptr)
{
    if (ptr)
        gst_buffer_ref(ptr);

    return ptr;
}

template<> void derefGPtr<GstBuffer>(GstBuffer* ptr)
{
    if (ptr)
        gst_buffer_unref(ptr);
}
}
#endif // USE(GSTREAMER)
