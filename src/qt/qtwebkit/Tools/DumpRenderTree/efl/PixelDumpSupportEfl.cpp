/*
 * Copyright (C) 2009 Zan Dobersek <zandobersek@gmail.com>
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#include "DumpRenderTree.h"
#include "DumpRenderTreeChrome.h"
#include "IntRect.h"
#include "PixelDumpSupportCairo.h"
#include "RefPtrCairo.h"
#include "WebCoreSupport/DumpRenderTreeSupportEfl.h"
#include "ewk_view.h"

PassRefPtr<BitmapContext> createBitmapContextFromWebView(bool, bool, bool, bool drawSelectionRect)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(browser->mainView()));
    Ewk_View_Private_Data* privateData = static_cast<Ewk_View_Private_Data*>(smartData->_priv);
    const Evas_Object* mainFrame = browser->mainFrame();

    int x, y, width, height;
    evas_object_geometry_get(browser->mainFrame(), &x, &y, &width, &height);
    const Eina_Rectangle rect = { x, y, width, height };

    RefPtr<cairo_surface_t> surface = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, rect.w, rect.h));
    RefPtr<cairo_t> context = adoptRef(cairo_create(surface.get()));

    if (!ewk_view_paint(privateData, context.get(), &rect))
        return 0;

    if (DumpRenderTreeSupportEfl::isTrackingRepaints(mainFrame)) {
        cairo_push_group(context.get());

        // Paint the gray mask over the original image.
        cairo_set_source_rgba(context.get(), 0, 0, 0, 0.66);
        cairo_paint(context.get());

        // Paint transparent rectangles over the mask to show the repainted regions.
        cairo_set_source_rgba(context.get(), 0, 0, 0, 0);
        cairo_set_operator(context.get(), CAIRO_OPERATOR_SOURCE);

        Eina_List* repaintRects = DumpRenderTreeSupportEfl::trackedRepaintRects(mainFrame);
        void* iter = 0;
        EINA_LIST_FREE(repaintRects, iter) {
            Eina_Rectangle* rect = static_cast<Eina_Rectangle*>(iter);

            cairo_rectangle(context.get(), rect->x, rect->y, rect->w, rect->h);
            cairo_fill(context.get());

            eina_rectangle_free(rect);
        }

        cairo_pop_group_to_source(context.get());
        cairo_paint(context.get());
    }

    if (drawSelectionRect) {
        const WebCore::IntRect selectionRect = DumpRenderTreeSupportEfl::selectionRectangle(mainFrame);

        if (!selectionRect.isEmpty()) {
            cairo_set_line_width(context.get(), 1.0);
            cairo_rectangle(context.get(), selectionRect.x(), selectionRect.y(), selectionRect.width(), selectionRect.height());
            cairo_set_source_rgba(context.get(), 1.0, 0.0, 0.0, 1.0);
            cairo_stroke(context.get());
        }
    }

    return BitmapContext::createByAdoptingBitmapAndContext(0, context.release().leakRef());
}
