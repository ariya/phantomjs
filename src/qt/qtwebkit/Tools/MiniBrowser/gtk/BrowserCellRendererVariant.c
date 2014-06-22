/*
 * Copyright (C) 2011 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "BrowserCellRendererVariant.h"
#include "BrowserMarshal.h"
#include <errno.h>

enum {
    PROP_0,

    PROP_VALUE,
    PROP_ADJUSTMENT
};

enum {
    CHANGED,

    LAST_SIGNAL
};

struct _BrowserCellRendererVariant {
    GtkCellRenderer parent;

    GValue *value;

    GtkCellRenderer *textRenderer;
    GtkCellRenderer *toggleRenderer;
    GtkCellRenderer *spinRenderer;
};

struct _BrowserCellRendererVariantClass {
    GtkCellRendererClass parent;
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(BrowserCellRendererVariant, browser_cell_renderer_variant, GTK_TYPE_CELL_RENDERER)

static void browserCellRendererVariantFinalize(GObject *object)
{
    BrowserCellRendererVariant *renderer = BROWSER_CELL_RENDERER_VARIANT(object);

    g_object_unref(renderer->toggleRenderer);
    g_object_unref(renderer->spinRenderer);
    g_object_unref(renderer->textRenderer);
    if (renderer->value)
        g_boxed_free(G_TYPE_VALUE, renderer->value);

    G_OBJECT_CLASS(browser_cell_renderer_variant_parent_class)->finalize(object);
}

static void browserCellRendererVariantGetProperty(GObject *object, guint propId, GValue *value, GParamSpec *pspec)
{
    BrowserCellRendererVariant *renderer = BROWSER_CELL_RENDERER_VARIANT(object);

    switch (propId) {
    case PROP_VALUE:
        g_value_set_boxed(value, renderer->value);
        break;
    case PROP_ADJUSTMENT: {
        GtkAdjustment *adjustment = NULL;
        g_object_get(G_OBJECT(renderer->spinRenderer), "adjustment", &adjustment, NULL);
        if (adjustment) {
            g_value_set_object(value, adjustment);
            g_object_unref(adjustment);
        }
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void browserCellRendererVariantSetModeForValue(BrowserCellRendererVariant *renderer)
{
    if (!renderer->value)
        return;

    GtkCellRendererMode mode;
    if (G_VALUE_HOLDS_BOOLEAN(renderer->value))
        mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
    else if (G_VALUE_HOLDS_STRING(renderer->value) || G_VALUE_HOLDS_UINT(renderer->value))
        mode = GTK_CELL_RENDERER_MODE_EDITABLE;
    else
        return;

    g_object_set(G_OBJECT(renderer), "mode", mode, NULL);
}

static void browserCellRendererVariantSetProperty(GObject *object, guint propId, const GValue *value, GParamSpec *pspec)
{
    BrowserCellRendererVariant *renderer = BROWSER_CELL_RENDERER_VARIANT(object);

    switch (propId) {
    case PROP_VALUE:
        if (renderer->value)
            g_boxed_free(G_TYPE_VALUE, renderer->value);
        renderer->value = g_value_dup_boxed(value);
        browserCellRendererVariantSetModeForValue(renderer);
        break;
    case PROP_ADJUSTMENT:
        g_object_set(G_OBJECT(renderer->spinRenderer), "adjustment", g_value_get_object(value), NULL);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static GtkCellRenderer *browserCellRendererVariantGetRendererForValue(BrowserCellRendererVariant *renderer)
{
    if (!renderer->value)
        return NULL;

    if (G_VALUE_HOLDS_BOOLEAN(renderer->value)) {
        g_object_set(G_OBJECT(renderer->toggleRenderer),
                     "active", g_value_get_boolean(renderer->value),
                     NULL);
        return renderer->toggleRenderer;
    }

    if (G_VALUE_HOLDS_STRING(renderer->value)) {
        g_object_set(G_OBJECT(renderer->textRenderer),
                     "text", g_value_get_string(renderer->value),
                     NULL);
        return renderer->textRenderer;
    }

    if (G_VALUE_HOLDS_UINT(renderer->value)) {
        gchar *text = g_strdup_printf("%u", g_value_get_uint(renderer->value));
        g_object_set(G_OBJECT(renderer->spinRenderer), "text", text, NULL);
        g_free(text);
        return renderer->spinRenderer;
    }

    return NULL;
}

static void browserCellRendererVariantCellRendererTextEdited(BrowserCellRendererVariant *renderer, const gchar *path, const gchar *newText)
{
    if (!renderer->value)
        return;

    if (!G_VALUE_HOLDS_STRING(renderer->value))
        return;

    g_value_set_string(renderer->value, newText);
    g_signal_emit(renderer, signals[CHANGED], 0, path, renderer->value);
}

static void browserCellRendererVariantCellRendererSpinEdited(BrowserCellRendererVariant *renderer, const gchar *path, const gchar *newText)
{
    if (!renderer->value)
        return;

    if (!G_VALUE_HOLDS_UINT(renderer->value))
        return;

    GtkAdjustment *adjustment;
    g_object_get(G_OBJECT(renderer->spinRenderer), "adjustment", &adjustment, NULL);
    if (!adjustment)
        return;

    errno = 0;
    gchar *endPtr;
    gdouble value = g_strtod(newText, &endPtr);
    if (errno || value > gtk_adjustment_get_upper(adjustment) || value < gtk_adjustment_get_lower(adjustment) || endPtr == newText) {
        g_warning("Invalid input for cell: %s\n", newText);
        return;
    }

    g_value_set_uint(renderer->value, (guint)value);
    g_signal_emit(renderer, signals[CHANGED], 0, path, renderer->value);
}

static gboolean browserCellRendererVariantCellRendererActivate(GtkCellRenderer *cell, GdkEvent *event, GtkWidget *widget, const gchar *path, const GdkRectangle *bgArea, const GdkRectangle *cellArea, GtkCellRendererState flags)
{
    BrowserCellRendererVariant *renderer = BROWSER_CELL_RENDERER_VARIANT(cell);

    if (!renderer->value)
        return TRUE;

    if (!G_VALUE_HOLDS_BOOLEAN(renderer->value))
        return TRUE;

    g_value_set_boolean(renderer->value, !g_value_get_boolean(renderer->value));
    g_signal_emit(renderer, signals[CHANGED], 0, path, renderer->value);

    return TRUE;
}

static void browserCellRendererVariantCellRendererRender(GtkCellRenderer *cell, cairo_t *cr, GtkWidget *widget, const GdkRectangle *bgArea, const GdkRectangle *cellArea, GtkCellRendererState flags)
{
    GtkCellRenderer *renderer = browserCellRendererVariantGetRendererForValue(BROWSER_CELL_RENDERER_VARIANT(cell));
    if (!renderer)
        return;

    GTK_CELL_RENDERER_GET_CLASS(renderer)->render(renderer, cr, widget, bgArea, cellArea, flags);
}

static GtkCellEditable *browserCellRendererVariantCellRendererStartEditing(GtkCellRenderer *cell, GdkEvent *event, GtkWidget *widget, const gchar *path, const GdkRectangle *bgArea, const GdkRectangle *cellArea, GtkCellRendererState flags)
{
    GtkCellRenderer *renderer = browserCellRendererVariantGetRendererForValue(BROWSER_CELL_RENDERER_VARIANT(cell));
    if (!renderer)
        return NULL;

    if (!GTK_CELL_RENDERER_GET_CLASS(renderer)->start_editing)
        return NULL;

    return GTK_CELL_RENDERER_GET_CLASS(renderer)->start_editing(renderer, event, widget, path, bgArea, cellArea, flags);
}

static void browserCellRendererVariantCellRendererGetPreferredWidth(GtkCellRenderer *cell, GtkWidget *widget, gint *minimumWidth, gint *naturalWidth)
{
    GtkCellRenderer *renderer = browserCellRendererVariantGetRendererForValue(BROWSER_CELL_RENDERER_VARIANT(cell));
    if (!renderer)
        return;

    GTK_CELL_RENDERER_GET_CLASS(renderer)->get_preferred_width(renderer, widget, minimumWidth, naturalWidth);
}

static void browserCellRendererVariantCellRendererGetPreferredHeight(GtkCellRenderer *cell, GtkWidget *widget, gint *minimumHeight, gint *naturalHeight)
{
    GtkCellRenderer *renderer = browserCellRendererVariantGetRendererForValue(BROWSER_CELL_RENDERER_VARIANT(cell));
    if (!renderer)
        return;

    GTK_CELL_RENDERER_GET_CLASS(renderer)->get_preferred_height(renderer, widget, minimumHeight, naturalHeight);
}

static void browserCellRendererVariantCellRendererGetPreferredWidthForHeight(GtkCellRenderer *cell, GtkWidget *widget, gint height, gint *minimumWidth, gint *naturalWidth)
{
    GtkCellRenderer *renderer = browserCellRendererVariantGetRendererForValue(BROWSER_CELL_RENDERER_VARIANT(cell));
    if (!renderer)
        return;

    GTK_CELL_RENDERER_GET_CLASS(renderer)->get_preferred_width_for_height(renderer, widget, height, minimumWidth, naturalWidth);
}

static void browserCellRendererVariantCellRendererGetPreferredHeightForWidth(GtkCellRenderer *cell, GtkWidget *widget, gint width, gint *minimumHeight, gint *naturalHeight)
{
    GtkCellRenderer *renderer = browserCellRendererVariantGetRendererForValue(BROWSER_CELL_RENDERER_VARIANT(cell));
    if (!renderer)
        return;

    GTK_CELL_RENDERER_GET_CLASS(renderer)->get_preferred_height_for_width(renderer, widget, width, minimumHeight, naturalHeight);
}

static void browserCellRendererVariantCellRendererGetAlignedArea(GtkCellRenderer *cell, GtkWidget *widget, GtkCellRendererState flags, const GdkRectangle *cellArea, GdkRectangle *alignedArea)
{
    GtkCellRenderer *renderer = browserCellRendererVariantGetRendererForValue(BROWSER_CELL_RENDERER_VARIANT(cell));
    if (!renderer)
        return;

    GTK_CELL_RENDERER_GET_CLASS(renderer)->get_aligned_area(renderer, widget, flags, cellArea, alignedArea);
}

static void browser_cell_renderer_variant_init(BrowserCellRendererVariant *renderer)
{
    g_object_set(renderer, "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);

    renderer->toggleRenderer = gtk_cell_renderer_toggle_new();
    g_object_set(G_OBJECT(renderer->toggleRenderer), "xalign", 0.0, NULL);
    g_object_ref_sink(renderer->toggleRenderer);

    renderer->textRenderer = gtk_cell_renderer_text_new();
    g_signal_connect_swapped(renderer->textRenderer, "edited",
                             G_CALLBACK(browserCellRendererVariantCellRendererTextEdited), renderer);
    g_object_set(G_OBJECT(renderer->textRenderer), "editable", TRUE, NULL);
    g_object_ref_sink(renderer->textRenderer);

    renderer->spinRenderer = gtk_cell_renderer_spin_new();
    g_signal_connect_swapped(renderer->spinRenderer, "edited",
                             G_CALLBACK(browserCellRendererVariantCellRendererSpinEdited), renderer);
    g_object_set(G_OBJECT(renderer->spinRenderer), "editable", TRUE, NULL);
}

static void browser_cell_renderer_variant_class_init(BrowserCellRendererVariantClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    GtkCellRendererClass *cellRendererClass = GTK_CELL_RENDERER_CLASS(klass);

    gobjectClass->get_property = browserCellRendererVariantGetProperty;
    gobjectClass->set_property = browserCellRendererVariantSetProperty;
    gobjectClass->finalize = browserCellRendererVariantFinalize;

    cellRendererClass->activate = browserCellRendererVariantCellRendererActivate;
    cellRendererClass->render = browserCellRendererVariantCellRendererRender;
    cellRendererClass->start_editing = browserCellRendererVariantCellRendererStartEditing;
    cellRendererClass->get_preferred_width = browserCellRendererVariantCellRendererGetPreferredWidth;
    cellRendererClass->get_preferred_height = browserCellRendererVariantCellRendererGetPreferredHeight;
    cellRendererClass->get_preferred_width_for_height = browserCellRendererVariantCellRendererGetPreferredWidthForHeight;
    cellRendererClass->get_preferred_height_for_width = browserCellRendererVariantCellRendererGetPreferredHeightForWidth;
    cellRendererClass->get_aligned_area = browserCellRendererVariantCellRendererGetAlignedArea;

    g_object_class_install_property(gobjectClass,
                                    PROP_VALUE,
                                    g_param_spec_boxed("value",
                                                       "Value",
                                                       "The cell renderer value",
                                                       G_TYPE_VALUE,
                                                       G_PARAM_READWRITE));
    g_object_class_install_property(gobjectClass,
                                    PROP_ADJUSTMENT,
                                    g_param_spec_object("adjustment",
                                                        "Adjustment",
                                                        "The adjustment that holds the value of the spin button",
                                                        GTK_TYPE_ADJUSTMENT,
                                                        G_PARAM_READWRITE));

    signals[CHANGED] =
        g_signal_new("changed",
                     G_TYPE_FROM_CLASS(gobjectClass),
                     G_SIGNAL_RUN_LAST,
                     0, NULL, NULL,
                     browser_marshal_VOID__STRING_BOXED,
                     G_TYPE_NONE, 2,
                     G_TYPE_STRING, G_TYPE_VALUE);
}

GtkCellRenderer *browser_cell_renderer_variant_new(void)
{
    return GTK_CELL_RENDERER(g_object_new(BROWSER_TYPE_CELL_RENDERER_VARIANT, NULL));
}

