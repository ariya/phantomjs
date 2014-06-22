/*
 *  Copyright (C) 2010 Igalia S.L.
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

#ifndef RefPtrCairo_h
#define RefPtrCairo_h

#include <wtf/RefPtr.h>

typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_font_face cairo_font_face_t;
typedef struct _cairo_scaled_font cairo_scaled_font_t;
typedef struct _cairo_pattern cairo_pattern_t;
typedef struct _cairo_region cairo_region_t;

#if USE(FREETYPE)
typedef struct _FcPattern FcPattern;
#endif

namespace WTF {

template<> void refIfNotNull(cairo_t* ptr);
template<> void derefIfNotNull(cairo_t* ptr);

template<> void refIfNotNull(cairo_surface_t* ptr);
template<> void derefIfNotNull(cairo_surface_t* ptr);

template<> void refIfNotNull(cairo_font_face_t* ptr);
template<> void derefIfNotNull(cairo_font_face_t* ptr);

template<> void refIfNotNull(cairo_scaled_font_t* ptr);
template<> void derefIfNotNull(cairo_scaled_font_t* ptr);

template<> void refIfNotNull(cairo_pattern_t*);
template<> void derefIfNotNull(cairo_pattern_t*);

template<> void refIfNotNull(cairo_region_t*);
template<> void derefIfNotNull(cairo_region_t*);

#if USE(FREETYPE)
template<> void refIfNotNull(FcPattern* ptr);
template<> void derefIfNotNull(FcPattern* ptr);
#endif

}

#endif // RefPtrCairo_h
