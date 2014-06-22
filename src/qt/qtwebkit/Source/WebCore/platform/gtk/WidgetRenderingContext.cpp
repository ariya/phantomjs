/*
 * Copyright (C) 2010 Sencha, Inc.
 * Copyright (C) 2010 Igalia S.L.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WidgetRenderingContext.h"

#ifdef GTK_API_VERSION_2

#include "GraphicsContext.h"
#include "GtkVersioning.h"
#include "PlatformContextCairo.h"
#include "RefPtrCairo.h"
#include "RenderThemeGtk.h"
#include "Timer.h"
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {

static GdkPixmap* gScratchBuffer = 0;
static void purgeScratchBuffer()
{
    if (gScratchBuffer)
        g_object_unref(gScratchBuffer);
    gScratchBuffer = 0;
}

// Widget rendering needs a scratch image as the buffer for the intermediate
// render. Instead of creating and destroying the buffer for every operation,
// we create a buffer which will be automatically purged via a timer.
class PurgeScratchBufferTimer : public TimerBase {
private:
    virtual void fired() { purgeScratchBuffer(); }
};

static void scheduleScratchBufferPurge()
{
    DEFINE_STATIC_LOCAL(PurgeScratchBufferTimer, purgeScratchBufferTimer, ());
    if (purgeScratchBufferTimer.isActive())
        purgeScratchBufferTimer.stop();
    purgeScratchBufferTimer.startOneShot(2);
}

WidgetRenderingContext::WidgetRenderingContext(GraphicsContext* graphicsContext, const IntRect& targetRect)
    : m_graphicsContext(graphicsContext)
    , m_targetRect(targetRect)
    , m_hadError(false)
{
    RenderThemeGtk* theme = static_cast<RenderThemeGtk*>(RenderTheme::defaultTheme().get());

    // Fallback: We failed to create an RGBA colormap earlier, so we cannot properly paint 
    // to a temporary surface and preserve transparency. To ensure decent widget rendering, just
    // paint directly to the target drawable. This will not render CSS rotational transforms properly.
    if (!theme->m_themePartsHaveRGBAColormap && graphicsContext->gdkWindow()) {
        m_target = graphicsContext->gdkWindow();
        return;
    }

    // We never want to create a scratch buffer larger than the size of our target GdkDrawable.
    // This prevents giant pixmap allocations for very large widgets in smaller views.
    // FIXME: We need to implement this check for WebKit2 as well.
    if (GdkDrawable* drawable = graphicsContext->gdkWindow()) {
        int maxWidth = 0, maxHeight = 0;

        if (GDK_IS_PIXMAP(drawable))
            gdk_pixmap_get_size(GDK_PIXMAP(drawable), &maxWidth, &maxHeight);
        else if (GDK_IS_WINDOW(drawable)) {
            maxWidth = gdk_window_get_width(GDK_WINDOW(drawable));
            maxHeight = gdk_window_get_height(GDK_WINDOW(drawable));
        }
        m_targetRect.setSize(m_targetRect.size().shrunkTo(IntSize(maxWidth, maxHeight)));
    }

    // Widgets sometimes need to draw outside their boundaries for things such as
    // exterior focus. We want to allocate a some extra pixels in our surface for this.
    static int extraSpace = 15;
    m_targetRect.inflate(extraSpace);

    // This offset will map a point in the coordinate system of the widget to the coordinate system of the painting buffer.
    m_paintOffset = targetRect.location() - m_targetRect.location();

    int width = m_targetRect.width() + (extraSpace * 2);
    int height = m_targetRect.height() + (extraSpace * 2);
    int scratchWidth = 0;
    int scratchHeight = 0;
    if (gScratchBuffer)
        gdk_pixmap_get_size(gScratchBuffer, &scratchWidth, &scratchHeight);

    // We do not need to recreate the buffer if the current buffer is large enough.
    if (!gScratchBuffer || scratchWidth < width || scratchHeight < height) {
        purgeScratchBuffer();
        // Round to the nearest 32 pixels so we do not grow the buffer for similar sized requests.
        width = (1 + (width >> 5)) << 5;
        height = (1 + (height >> 5)) << 5;

        gScratchBuffer = gdk_pixmap_new(0, width, height, gdk_colormap_get_visual(theme->m_colormap)->depth);
        gdk_drawable_set_colormap(gScratchBuffer, theme->m_colormap);
    }
    m_target = gScratchBuffer;

    // Clear the scratch buffer.
    RefPtr<cairo_t> scratchBufferContext = adoptRef(gdk_cairo_create(gScratchBuffer));
    cairo_set_operator(scratchBufferContext.get(), CAIRO_OPERATOR_CLEAR);
    cairo_paint(scratchBufferContext.get());
}

WidgetRenderingContext::~WidgetRenderingContext()
{
    // We do not need to blit back to the target in the fallback case. See above.
    RenderThemeGtk* theme = static_cast<RenderThemeGtk*>(RenderTheme::defaultTheme().get());
    if (!theme->m_themePartsHaveRGBAColormap && m_graphicsContext->gdkWindow())
        return;

    // Don't paint the results back if there was an error.
    if (m_hadError) {
        scheduleScratchBufferPurge();
        return;
    }

    // FIXME: It's unclear if it is necessary to preserve the current source here.
    cairo_t* cairoContext = m_graphicsContext->platformContext()->cr();
    RefPtr<cairo_pattern_t> previousSource(cairo_get_source(cairoContext));

    gdk_cairo_set_source_pixmap(cairoContext, gScratchBuffer, m_targetRect.x(), m_targetRect.y());
    cairo_rectangle(cairoContext, m_targetRect.x(), m_targetRect.y(), m_targetRect.width(), m_targetRect.height());
    cairo_fill(cairoContext);
    cairo_set_source(cairoContext, previousSource.get());
    scheduleScratchBufferPurge();
}

void WidgetRenderingContext::calculateClipRect(const IntRect& rect, GdkRectangle* clipRect)
{
    clipRect->x = m_paintOffset.width() + rect.x();
    clipRect->y = m_paintOffset.height() + rect.y();
    clipRect->width = m_targetRect.width();
    clipRect->height = m_targetRect.height();
}

void WidgetRenderingContext::gtkPaintBox(const IntRect& rect, GtkWidget* widget, GtkStateType stateType, GtkShadowType shadowType, const gchar* detail)
{
    GdkRectangle clipRect;
    calculateClipRect(rect, &clipRect);

    // Some widgets also need their allocation adjusted to account for extra space.
    // Right now only scrollbar buttons have significant allocations.
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    allocation.x += clipRect.x;
    allocation.y += clipRect.y;
    gtk_widget_set_allocation(widget, &allocation);

    gtk_paint_box(gtk_widget_get_style(widget), m_target, stateType, shadowType, &clipRect,
                  widget, detail, clipRect.x, clipRect.y, rect.width(), rect.height());
}

void WidgetRenderingContext::gtkPaintFlatBox(const IntRect& rect, GtkWidget* widget, GtkStateType stateType, GtkShadowType shadowType, const gchar* detail)
{
    GdkRectangle clipRect;
    calculateClipRect(rect, &clipRect);
    gtk_paint_flat_box(gtk_widget_get_style(widget), m_target, stateType, shadowType, &clipRect,
                       widget, detail, clipRect.x, clipRect.y, rect.width(), rect.height());
}

void WidgetRenderingContext::gtkPaintFocus(const IntRect& rect, GtkWidget* widget, GtkStateType stateType, const gchar* detail)
{
    GdkRectangle clipRect;
    calculateClipRect(rect, &clipRect);
    gtk_paint_focus(gtk_widget_get_style(widget), m_target, stateType, &clipRect, widget,
                    detail, clipRect.x, clipRect.y, rect.width(), rect.height());
}

void WidgetRenderingContext::gtkPaintSlider(const IntRect& rect, GtkWidget* widget, GtkStateType stateType, GtkShadowType shadowType, const gchar* detail, GtkOrientation orientation)
{
    GdkRectangle clipRect;
    calculateClipRect(rect, &clipRect);
    gtk_paint_slider(gtk_widget_get_style(widget), m_target, stateType, shadowType, &clipRect, widget,
                     detail, clipRect.x, clipRect.y, rect.width(), rect.height(), orientation);
}

void WidgetRenderingContext::gtkPaintCheck(const IntRect& rect, GtkWidget* widget, GtkStateType stateType, GtkShadowType shadowType, const gchar* detail)
{
    GdkRectangle clipRect;
    calculateClipRect(rect, &clipRect);
    gtk_paint_check(gtk_widget_get_style(widget), m_target, stateType, shadowType, &clipRect, widget,
                    detail, clipRect.x, clipRect.y, rect.width(), rect.height());
}

void WidgetRenderingContext::gtkPaintOption(const IntRect& rect, GtkWidget* widget, GtkStateType stateType, GtkShadowType shadowType, const gchar* detail)
{
    GdkRectangle clipRect;
    calculateClipRect(rect, &clipRect);
    gtk_paint_option(gtk_widget_get_style(widget), m_target, stateType, shadowType, &clipRect, widget,
                     detail, clipRect.x, clipRect.y, rect.width(), rect.height());
}

void WidgetRenderingContext::gtkPaintShadow(const IntRect& rect, GtkWidget* widget, GtkStateType stateType, GtkShadowType shadowType, const gchar* detail)
{
    GdkRectangle clipRect;
    calculateClipRect(rect, &clipRect);
    gtk_paint_shadow(gtk_widget_get_style(widget), m_target, stateType, shadowType, &clipRect, widget,
                     detail, clipRect.x, clipRect.y, rect.width(), rect.height());
}

void WidgetRenderingContext::gtkPaintArrow(const IntRect& rect, GtkWidget* widget, GtkStateType stateType, GtkShadowType shadowType, int arrowDirection, const gchar* detail)
{
    GdkRectangle clipRect;
    calculateClipRect(rect, &clipRect);
    gtk_paint_arrow(gtk_widget_get_style(widget), m_target, stateType, shadowType, &clipRect, widget, detail,
                    static_cast<GtkArrowType>(arrowDirection), TRUE, clipRect.x, clipRect.y, rect.width(), rect.height());
}

void WidgetRenderingContext::gtkPaintVLine(const IntRect& rect, GtkWidget* widget, GtkStateType stateType, const gchar* detail)
{
    GdkRectangle clipRect;
    calculateClipRect(rect, &clipRect);
    gtk_paint_vline(gtk_widget_get_style(widget), m_target, stateType, &clipRect, widget, detail,
                    clipRect.y, clipRect.y + rect.height(), clipRect.x);

}

}

#endif // GTK_API_VERSION_2
