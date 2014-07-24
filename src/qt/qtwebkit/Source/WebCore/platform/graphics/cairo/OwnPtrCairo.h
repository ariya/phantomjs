/*
 * Copyright (C) 2010 Collabora Ltd.
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
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#ifndef OwnPtrCairo_h
#define OwnPtrCairo_h

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

#if USE(FREETYPE)
typedef struct _FcObjectSet FcObjectSet;
typedef struct _FcFontSet FcFontSet;
#endif

typedef struct cairo_path cairo_path_t;

namespace WTF {

#if USE(FREETYPE)
template <> void deleteOwnedPtr<FcObjectSet>(FcObjectSet*);
template <> void deleteOwnedPtr<FcFontSet>(FcFontSet*);
#endif

template <> void deleteOwnedPtr<cairo_path_t>(cairo_path_t*);

} // namespace WTF

#endif
