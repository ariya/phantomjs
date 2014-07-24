/*
 *  Copyright (C) 2011 Igalia S.L
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

#ifndef GRefPtrGStreamer_h
#define GRefPtrGStreamer_h
#if USE(GSTREAMER)

#include <wtf/gobject/GRefPtr.h>

typedef struct _GstElement GstElement;
typedef struct _GstPad GstPad;
typedef struct _GstPadTemplate GstPadTemplate;
typedef struct _GstCaps GstCaps;
typedef struct _GstTask GstTask;
typedef struct _GstBus GstBus;
typedef struct _GstElementFactory GstElementFactory;
typedef struct _GstBuffer GstBuffer;

namespace WTF {

template<> GRefPtr<GstElement> adoptGRef(GstElement* ptr);
template<> GstElement* refGPtr<GstElement>(GstElement* ptr);
template<> void derefGPtr<GstElement>(GstElement* ptr);

template<> GRefPtr<GstPad> adoptGRef(GstPad* ptr);
template<> GstPad* refGPtr<GstPad>(GstPad* ptr);
template<> void derefGPtr<GstPad>(GstPad* ptr);

template<> GRefPtr<GstPadTemplate> adoptGRef(GstPadTemplate* ptr);
template<> GstPadTemplate* refGPtr<GstPadTemplate>(GstPadTemplate* ptr);
template<> void derefGPtr<GstPadTemplate>(GstPadTemplate* ptr);

template<> GRefPtr<GstCaps> adoptGRef(GstCaps* ptr);
template<> GstCaps* refGPtr<GstCaps>(GstCaps* ptr);
template<> void derefGPtr<GstCaps>(GstCaps* ptr);

template<> GRefPtr<GstTask> adoptGRef(GstTask* ptr);
template<> GstTask* refGPtr<GstTask>(GstTask* ptr);
template<> void derefGPtr<GstTask>(GstTask* ptr);

template<> GRefPtr<GstBus> adoptGRef(GstBus* ptr);
template<> GstBus* refGPtr<GstBus>(GstBus* ptr);
template<> void derefGPtr<GstBus>(GstBus* ptr);

template<> GRefPtr<GstElementFactory> adoptGRef(GstElementFactory* ptr);
template<> GstElementFactory* refGPtr<GstElementFactory>(GstElementFactory* ptr);
template<> void derefGPtr<GstElementFactory>(GstElementFactory* ptr);

template<> GRefPtr<GstBuffer> adoptGRef(GstBuffer* ptr);
template<> GstBuffer* refGPtr<GstBuffer>(GstBuffer* ptr);
template<> void derefGPtr<GstBuffer>(GstBuffer* ptr);
}

#endif // USE(GSTREAMER)
#endif
