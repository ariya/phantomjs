/*
 * Copyright (C) 2011, Igalia S.L.
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

#ifndef WidgetBackingStore_h
#define WidgetBackingStore_h

#include "IntRect.h"
#include "IntSize.h"
#include <wtf/FastAllocBase.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>

#if PLATFORM(GTK)
#include <gtk/gtk.h>
#elif PLATFORM(EFL)
#include <Evas.h>
#endif

typedef struct _cairo_surface cairo_surface_t;

namespace WebCore {

#if PLATFORM(GTK)
typedef GtkWidget* PlatformWidget;
#elif PLATFORM(EFL)
typedef Evas_Object* PlatformWidget;
#endif

class WidgetBackingStore {
    WTF_MAKE_NONCOPYABLE(WidgetBackingStore);
    WTF_MAKE_FAST_ALLOCATED;

public:
    virtual cairo_surface_t* cairoSurface() = 0;
    virtual void scroll(const IntRect& scrollRect, const IntSize& scrollOffset) = 0;
    const IntSize& size() { return m_size; }
    WidgetBackingStore(const IntSize& size) : m_size(size) { }
    virtual ~WidgetBackingStore() { }

private:
    IntSize m_size;
};

} // namespace WebCore

#endif // WidgetBackingStore_h
