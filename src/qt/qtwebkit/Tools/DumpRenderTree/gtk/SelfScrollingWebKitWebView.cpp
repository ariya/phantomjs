/*
 * Copyright (C) 2011 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SelfScrollingWebKitWebView.h"

#include <webkit/webkit.h>

G_BEGIN_DECLS

#ifdef GTK_API_VERSION_2
static void sizeRequestMethod(GtkWidget*, GtkRequisition*);
#else
static void getPreferredSizeMethod(GtkWidget*, gint* minimum, gint* natural);
#endif

G_DEFINE_TYPE(SelfScrollingWebKitWebView, self_scrolling_webkit_web_view, WEBKIT_TYPE_WEB_VIEW)

static void self_scrolling_webkit_web_view_class_init(SelfScrollingWebKitWebViewClass* klass)
{
    GtkWidgetClass* widgetClass = GTK_WIDGET_CLASS(klass);
#ifdef GTK_API_VERSION_2
    widgetClass->size_request = sizeRequestMethod;
#else
    widgetClass->get_preferred_width = getPreferredSizeMethod;
    widgetClass->get_preferred_height = getPreferredSizeMethod;
#endif
}

static void self_scrolling_webkit_web_view_init(SelfScrollingWebKitWebView* webView)
{
}

GtkWidget* self_scrolling_webkit_web_view_new()
{
    return GTK_WIDGET(g_object_new(self_scrolling_webkit_web_view_get_type(), "self-scrolling", TRUE, NULL));
}

#ifdef GTK_API_VERSION_2
static void sizeRequestMethod(GtkWidget*, GtkRequisition* requisition)
{
    requisition->width = 1;
    requisition->height = 1;
}
#else
static void getPreferredSizeMethod(GtkWidget*, gint* minimum, gint* natural)
{
    *minimum = 1;
    *natural = 1;
}
#endif

G_END_DECLS
