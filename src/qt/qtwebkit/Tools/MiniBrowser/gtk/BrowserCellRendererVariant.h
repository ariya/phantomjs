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

#ifndef BrowserCellRendererVariant_h
#define BrowserCellRendererVariant_h

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BROWSER_TYPE_CELL_RENDERER_VARIANT            (browser_cell_renderer_variant_get_type())
#define BROWSER_CELL_RENDERER_VARIANT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BROWSER_TYPE_CELL_RENDERER_VARIANT, BrowserCellRendererVariant))
#define BROWSER_CELL_RENDERER_VARIANT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  BROWSER_TYPE_CELL_RENDERER_VARIANT, BrowserCellRendererVariantClass))
#define BROWSER_IS_CELL_RENDERER_VARIANT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BROWSER_TYPE_CELL_RENDERER_VARIANT))
#define BROWSER_IS_CELL_RENDERER_VARIANT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  BROWSER_TYPE_CELL_RENDERER_VARIANT))
#define BROWSER_CELL_RENDERER_VARIANT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  BROWSER_TYPE_CELL_RENDERER_VARIANT, BrowserCellRendererVariantClass))

typedef struct _BrowserCellRendererVariant        BrowserCellRendererVariant;
typedef struct _BrowserCellRendererVariantClass   BrowserCellRendererVariantClass;

GType browser_cell_renderer_variant_get_type(void);

GtkCellRenderer* browser_cell_renderer_variant_new(void);

G_END_DECLS

#endif
