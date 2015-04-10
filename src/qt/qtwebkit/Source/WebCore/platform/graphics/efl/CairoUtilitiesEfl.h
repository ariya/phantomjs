/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2010 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef CairoUtilitiesEfl_h
#define CairoUtilitiesEfl_h

#include <Ecore_Evas.h>
#include <Evas.h>
#include <cairo.h>
#include <wtf/PassRefPtr.h>
#include <wtf/efl/RefPtrEfl.h>

namespace WebCore {

PassRefPtr<Evas_Object> evasObjectFromCairoImageSurface(Evas* canvas, cairo_surface_t*);
PassRefPtr<cairo_surface_t> createSurfaceForBackingStore(Ecore_Evas* ee);
PassRefPtr<cairo_surface_t> createSurfaceForImage(Evas_Object* image);

}

#endif // CairoUtilitiesEfl_h
