/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ScrollbarThemeGtk.h"

#ifndef GTK_API_VERSION_2

#include "PlatformContextCairo.h"
#include "PlatformMouseEvent.h"
#include "RenderThemeGtk.h"
#include "ScrollView.h"
#include "Scrollbar.h"
#include <gtk/gtk.h>

namespace WebCore {

static void gtkStyleChangedCallback(GtkWidget*, ScrollbarThemeGtk* scrollbarTheme)
{
    scrollbarTheme->updateThemeProperties();
}

ScrollbarThemeGtk::ScrollbarThemeGtk()
    : m_context(static_cast<RenderThemeGtk*>(RenderTheme::defaultTheme().get())->gtkScrollbarStyle())
{
    updateThemeProperties();
    g_signal_connect(m_context, "changed", G_CALLBACK(gtkStyleChangedCallback), this);
}

void ScrollbarThemeGtk::updateThemeProperties()
{
    gtk_style_context_get_style(m_context,
                                "min-slider-length", &m_minThumbLength,
                                "slider-width", &m_thumbFatness,
                                "trough-border", &m_troughBorderWidth,
                                "stepper-size", &m_stepperSize,
                                "stepper-spacing", &m_stepperSpacing,
                                "trough-under-steppers", &m_troughUnderSteppers,
                                "has-backward-stepper", &m_hasBackButtonStartPart,
                                "has-forward-stepper", &m_hasForwardButtonEndPart,
                                "has-secondary-backward-stepper", &m_hasBackButtonEndPart,
                                "has-secondary-forward-stepper", &m_hasForwardButtonStartPart,
                                NULL);
    updateScrollbarsFrameThickness();
}

static void applyScrollbarStyleContextClasses(GtkStyleContext* context, ScrollbarOrientation orientation)
{
    gtk_style_context_add_class(context, GTK_STYLE_CLASS_SCROLLBAR);
    gtk_style_context_add_class(context, orientation == VerticalScrollbar ?  GTK_STYLE_CLASS_VERTICAL : GTK_STYLE_CLASS_HORIZONTAL);
}

void ScrollbarThemeGtk::paintTrackBackground(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect)
{
    // Paint the track background. If the trough-under-steppers property is true, this
    // should be the full size of the scrollbar, but if is false, it should only be the
    // track rect.
    IntRect fullScrollbarRect(rect);
    if (m_troughUnderSteppers)
        fullScrollbarRect = IntRect(scrollbar->x(), scrollbar->y(), scrollbar->width(), scrollbar->height());

    gtk_style_context_save(m_context);

    applyScrollbarStyleContextClasses(m_context, scrollbar->orientation());
    gtk_style_context_add_class(m_context, GTK_STYLE_CLASS_TROUGH);

    gtk_render_background(m_context, context->platformContext()->cr(),
                          fullScrollbarRect.x(), fullScrollbarRect.y(), fullScrollbarRect.width(), fullScrollbarRect.height());
    gtk_render_frame(m_context, context->platformContext()->cr(),
                     fullScrollbarRect.x(), fullScrollbarRect.y(), fullScrollbarRect.width(), fullScrollbarRect.height());

    gtk_style_context_restore(m_context);
}

void ScrollbarThemeGtk::paintScrollbarBackground(GraphicsContext* context, ScrollbarThemeClient* scrollbar)
{
    gtk_style_context_save(m_context);

    applyScrollbarStyleContextClasses(m_context, scrollbar->orientation());
    gtk_style_context_add_class(m_context, "scrolled-window");
    gtk_render_frame(m_context, context->platformContext()->cr(), scrollbar->x(), scrollbar->y(), scrollbar->width(), scrollbar->height());

    gtk_style_context_restore(m_context);
}

void ScrollbarThemeGtk::paintThumb(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect)
{
    gtk_style_context_save(m_context);

    ScrollbarOrientation orientation = scrollbar->orientation();
    applyScrollbarStyleContextClasses(m_context, orientation);
    gtk_style_context_add_class(m_context, GTK_STYLE_CLASS_SLIDER);

    guint flags = 0;
    if (scrollbar->pressedPart() == ThumbPart)
        flags |= GTK_STATE_FLAG_ACTIVE;
    if (scrollbar->hoveredPart() == ThumbPart)
        flags |= GTK_STATE_FLAG_PRELIGHT;
    gtk_style_context_set_state(m_context, static_cast<GtkStateFlags>(flags));

    gtk_render_slider(m_context, context->platformContext()->cr(), rect.x(), rect.y(), rect.width(), rect.height(),
                      orientation == VerticalScrollbar ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL);

    gtk_style_context_restore(m_context);
}

void ScrollbarThemeGtk::paintButton(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect, ScrollbarPart part)
{
    gtk_style_context_save(m_context);

    ScrollbarOrientation orientation = scrollbar->orientation();
    applyScrollbarStyleContextClasses(m_context, orientation);

    guint flags = 0;
    if ((BackButtonStartPart == part && scrollbar->currentPos())
        || (BackButtonEndPart == part && scrollbar->currentPos())
        || (ForwardButtonEndPart == part && scrollbar->currentPos() != scrollbar->maximum())
        || (ForwardButtonStartPart == part && scrollbar->currentPos() != scrollbar->maximum())) {
        if (part == scrollbar->pressedPart())
            flags |= GTK_STATE_FLAG_ACTIVE;
        if (part == scrollbar->hoveredPart())
            flags |= GTK_STATE_FLAG_PRELIGHT;
    } else
        flags |= GTK_STATE_FLAG_INSENSITIVE;
    gtk_style_context_set_state(m_context, static_cast<GtkStateFlags>(flags));

    gtk_style_context_add_class(m_context, GTK_STYLE_CLASS_BUTTON);
    gtk_render_background(m_context, context->platformContext()->cr(), rect.x(), rect.y(), rect.width(), rect.height());
    gtk_render_frame(m_context, context->platformContext()->cr(), rect.x(), rect.y(), rect.width(), rect.height());

    gfloat arrowScaling;
    gtk_style_context_get_style(m_context, "arrow-scaling", &arrowScaling, NULL);

    double arrowSize = std::min(rect.width(), rect.height()) * arrowScaling;
    FloatPoint arrowPoint(rect.x() + (rect.width() - arrowSize) / 2,
                          rect.y() + (rect.height() - arrowSize) / 2);

    if (flags & GTK_STATE_FLAG_ACTIVE) {
        gint arrowDisplacementX, arrowDisplacementY;
        gtk_style_context_get_style(m_context,
                                    "arrow-displacement-x", &arrowDisplacementX,
                                    "arrow-displacement-y", &arrowDisplacementY,
                                    NULL);
        arrowPoint.move(arrowDisplacementX, arrowDisplacementY);
    }

    gdouble angle;
    if (orientation == VerticalScrollbar) {
        angle = (part == ForwardButtonEndPart || part == ForwardButtonStartPart) ? G_PI : 0;
    } else {
        angle = (part == ForwardButtonEndPart || part == ForwardButtonStartPart) ? G_PI / 2 : 3 * (G_PI / 2);
    }

    gtk_render_arrow(m_context, context->platformContext()->cr(), angle, arrowPoint.x(), arrowPoint.y(), arrowSize);

    gtk_style_context_restore(m_context);
}

} // namespace WebCore

#endif // !GTK_API_VERSION_2
