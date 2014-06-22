/*
 * Copyright (C) 2010 Igalia S.L.
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef WidgetRenderingContext_h
#define WidgetRenderingContext_h

#ifdef GTK_API_VERSION_2

#include "IntRect.h"

// Usually this is too expensive to have in headers, but GtkStateType GtkShadowType are
// enums and cannot be forward declared. WidgetRenderingContext.h is currently only
// included in RenderThemeGtk2.cpp and ScrollbarThemeGtk2.cpp.
#include <gtk/gtk.h>

namespace WebCore {

class GraphicsContext;
class RenderThemeGtk;

class WidgetRenderingContext {
public:
    WidgetRenderingContext(GraphicsContext*, const IntRect&);
    ~WidgetRenderingContext();

    void gtkPaintBox(const IntRect&, GtkWidget*, GtkStateType, GtkShadowType, const gchar*);
    void gtkPaintFlatBox(const IntRect&, GtkWidget*, GtkStateType, GtkShadowType, const gchar*);
    void gtkPaintFocus(const IntRect&, GtkWidget*, GtkStateType, const gchar*);
    void gtkPaintSlider(const IntRect&, GtkWidget*, GtkStateType, GtkShadowType, const gchar*, GtkOrientation);
    void gtkPaintCheck(const IntRect&, GtkWidget*, GtkStateType, GtkShadowType, const gchar*);
    void gtkPaintOption(const IntRect&, GtkWidget*, GtkStateType, GtkShadowType, const gchar*);
    void gtkPaintShadow(const IntRect&, GtkWidget*, GtkStateType, GtkShadowType, const gchar*);
    void gtkPaintArrow(const IntRect&, GtkWidget*, GtkStateType, GtkShadowType, int arrowDirection, const gchar*);
    void gtkPaintVLine(const IntRect&, GtkWidget*, GtkStateType, const gchar*);

private:
    void calculateClipRect(const IntRect&, GdkRectangle*);

    GraphicsContext* m_graphicsContext;
    IntRect m_targetRect;
    IntSize m_paintOffset;
    bool m_hadError;
    GdkDrawable* m_target;

};

}

#endif // GTK_API_VERSION_2
#endif // WidgetRenderingContext_h
