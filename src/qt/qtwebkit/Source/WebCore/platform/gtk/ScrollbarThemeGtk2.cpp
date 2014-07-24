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

#ifdef GTK_API_VERSION_2

#include "GtkVersioning.h"
#include "PlatformMouseEvent.h"
#include "RenderThemeGtk.h"
#include "ScrollView.h"
#include "Scrollbar.h"
#include "WidgetRenderingContext.h"
#include <gtk/gtk.h>

namespace WebCore {

static void gtkStyleSetCallback(GtkWidget* widget, GtkStyle* previous, ScrollbarThemeGtk* scrollbarTheme)
{
    scrollbarTheme->updateThemeProperties();
}

ScrollbarThemeGtk::ScrollbarThemeGtk()
{
    updateThemeProperties();
    g_signal_connect(static_cast<RenderThemeGtk*>(RenderTheme::defaultTheme().get())->gtkHScrollbar(),
         "style-set", G_CALLBACK(gtkStyleSetCallback), this);
}

void ScrollbarThemeGtk::updateThemeProperties()
{
    GtkWidget* scrollbar = static_cast<RenderThemeGtk*>(RenderTheme::defaultTheme().get())->gtkHScrollbar();
    gtk_widget_style_get(scrollbar,
                         "slider_width", &m_thumbFatness,
                         "trough_border", &m_troughBorderWidth,
                         "stepper-size", &m_stepperSize,
                         "trough-under-steppers", &m_troughUnderSteppers,
                         "has-backward-stepper", &m_hasBackButtonStartPart,
                         "has-forward-stepper", &m_hasForwardButtonEndPart,
                         "has-secondary-forward-stepper", &m_hasForwardButtonStartPart,
                         "has-secondary-backward-stepper", &m_hasBackButtonEndPart, NULL);
    m_minThumbLength = gtk_range_get_min_slider_size(GTK_RANGE(scrollbar));
    updateScrollbarsFrameThickness();
}

static GtkWidget* getWidgetForScrollbar(ScrollbarThemeClient* scrollbar)
{
    RenderThemeGtk* theme = static_cast<RenderThemeGtk*>(RenderTheme::defaultTheme().get());
    return scrollbar->orientation() == VerticalScrollbar ? theme->gtkVScrollbar() : theme->gtkHScrollbar();
}

void ScrollbarThemeGtk::paintTrackBackground(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect)
{
    // Paint the track background. If the trough-under-steppers property is true, this
    // should be the full size of the scrollbar, but if is false, it should only be the
    // track rect.
    IntRect fullScrollbarRect(rect);
    if (m_troughUnderSteppers)
        fullScrollbarRect = IntRect(scrollbar->x(), scrollbar->y(), scrollbar->width(), scrollbar->height());

    WidgetRenderingContext widgetContext(context, fullScrollbarRect);
    IntRect paintRect(IntPoint(), fullScrollbarRect.size());
    widgetContext.gtkPaintBox(paintRect, getWidgetForScrollbar(scrollbar),
                              GTK_STATE_ACTIVE, GTK_SHADOW_IN, "trough");
}

void ScrollbarThemeGtk::paintScrollbarBackground(GraphicsContext* context, ScrollbarThemeClient* scrollbar)
{
    IntRect fullScrollbarRect = IntRect(scrollbar->x(), scrollbar->y(), scrollbar->width(), scrollbar->height());

    WidgetRenderingContext widgetContext(context, fullScrollbarRect);
    widgetContext.gtkPaintBox(fullScrollbarRect, getWidgetForScrollbar(scrollbar),
                              GTK_STATE_NORMAL, GTK_SHADOW_IN, "scrolled_window");
}

void ScrollbarThemeGtk::paintThumb(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect)
{
    GtkWidget* widget = getWidgetForScrollbar(scrollbar);
    gboolean activateSlider;
    gtk_widget_style_get(widget, "activate-slider", &activateSlider, NULL);

    GtkStateType stateType = GTK_STATE_NORMAL;
    GtkShadowType shadowType = GTK_SHADOW_OUT;
    if (activateSlider && scrollbar->pressedPart() == ThumbPart) {
        stateType = GTK_STATE_ACTIVE;
        shadowType = GTK_SHADOW_IN;
    } else if (scrollbar->pressedPart() == ThumbPart || scrollbar->hoveredPart() == ThumbPart)
        stateType = GTK_STATE_PRELIGHT;

    // The adjustment controls the rendering of the scrollbar thumb. If it's not set
    // properly the theme may not draw the thumb borders properly.
    GtkAdjustment* adjustment = gtk_range_get_adjustment(GTK_RANGE(widget));
    gtk_adjustment_set_value(adjustment, scrollbar->currentPos());
    gtk_adjustment_set_lower(adjustment, 0);
    gtk_adjustment_set_upper(adjustment, scrollbar->maximum());

    GtkOrientation orientation = GTK_ORIENTATION_HORIZONTAL;
    if (scrollbar->orientation() == VerticalScrollbar) {
        gtk_adjustment_set_page_size(adjustment, rect.height());
        orientation = GTK_ORIENTATION_VERTICAL;
    } else
        gtk_adjustment_set_page_size(adjustment, rect.width());

    WidgetRenderingContext widgetContext(context, rect);
    IntRect sliderRect(IntPoint(), rect.size());
    widgetContext.gtkPaintSlider(sliderRect, widget, stateType, shadowType, "slider", orientation);
}

void ScrollbarThemeGtk::paintButton(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect, ScrollbarPart part)
{
    // The buttons will be disabled if the thumb is as the appropriate extreme.
    GtkShadowType shadowType = GTK_SHADOW_OUT;
    GtkStateType stateType = GTK_STATE_INSENSITIVE;
    bool pressed = (part == scrollbar->pressedPart());

    if ((BackButtonStartPart == part && scrollbar->currentPos())
        || (BackButtonEndPart == part && scrollbar->currentPos())
        || (ForwardButtonEndPart == part && scrollbar->currentPos() != scrollbar->maximum())
        || (ForwardButtonStartPart == part && scrollbar->currentPos() != scrollbar->maximum())) {
        stateType = GTK_STATE_NORMAL;
        if (pressed) {
            stateType = GTK_STATE_ACTIVE;
            shadowType = GTK_SHADOW_IN;
        } else if (part == scrollbar->hoveredPart())
            stateType = GTK_STATE_PRELIGHT;
    }

    // Themes determine how to draw the button (which button to draw) based on the allocation
    // of the widget. Where the target rect is in relation to the total widget allocation
    // determines the button.
    ScrollbarOrientation orientation = scrollbar->orientation();
    int buttonSize = (orientation == VerticalScrollbar) ? rect.height() : rect.width();
    int totalAllocation = buttonSize * 5; // One space for each button and one extra.
    int buttonOffset = 0;
    if (ForwardButtonStartPart == part)
        buttonOffset = buttonSize;
    else if (BackButtonEndPart == part)
        buttonOffset = 3 * buttonSize;
    else if (ForwardButtonEndPart == part)
        buttonOffset = 4 * buttonSize;

    // Now we want the allocation to be relative to the origin of the painted rect.
    GtkWidget* widget = getWidgetForScrollbar(scrollbar);
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    allocation.x = allocation.y = 0;
    allocation.width = rect.width();
    allocation.height = rect.height();

    if (orientation == VerticalScrollbar) {
        allocation.height = totalAllocation;
        allocation.y -= buttonOffset;
    } else {
        allocation.width = totalAllocation;
        allocation.x -= buttonOffset;
    }
    gtk_widget_set_allocation(widget, &allocation);

    const char* detail = orientation == VerticalScrollbar ? "vscrollbar" : "hscrollbar";
    WidgetRenderingContext widgetContext(context, rect);

    IntRect buttonRect(IntPoint(), rect.size());
    widgetContext.gtkPaintBox(buttonRect, widget, stateType, shadowType, detail);

    float arrowScaling;
    gtk_widget_style_get(widget, "arrow-scaling", &arrowScaling, NULL);
    IntSize arrowSize = rect.size();
    arrowSize.scale(arrowScaling);
    IntRect arrowRect(IntPoint(buttonRect.x() + (buttonRect.width() - arrowSize.width()) / 2,
                               buttonRect.y() + (buttonRect.height() - arrowSize.height()) / 2),
                      arrowSize);
    if (pressed) {
        int arrowDisplacementX, arrowDisplacementY;
        gtk_widget_style_get(widget,
                             "arrow-displacement-x", &arrowDisplacementX,
                             "arrow-displacement-y", &arrowDisplacementY,
                             NULL);
        arrowRect.move(arrowDisplacementX, arrowDisplacementY);
    }

    GtkArrowType arrowType = GTK_ARROW_DOWN;
    if (orientation == VerticalScrollbar) {
        if (part == BackButtonEndPart || part == BackButtonStartPart)
            arrowType = GTK_ARROW_UP;
    } else if (orientation == HorizontalScrollbar) {
        arrowType = GTK_ARROW_RIGHT;
        if (part == BackButtonEndPart || part == BackButtonStartPart)
            arrowType = GTK_ARROW_LEFT;
    }
    widgetContext.gtkPaintArrow(arrowRect, widget, stateType, shadowType, arrowType, detail);
}

} // namespace WebCore

#endif // GTK_API_VERSION_2
