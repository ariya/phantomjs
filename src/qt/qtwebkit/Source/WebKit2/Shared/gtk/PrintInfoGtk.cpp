/*
 * Copyright (C) 2012 Igalia S.L.
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

#include "config.h"
#include "PrintInfo.h"

#include <gtk/gtk.h>

namespace WebKit {

PrintInfo::PrintInfo(GtkPrintSettings* settings, GtkPageSetup* pageSetup)
    : pageSetupScaleFactor(gtk_print_settings_get_scale(settings) / 100.0)
    , availablePaperWidth(gtk_page_setup_get_paper_width(pageSetup, GTK_UNIT_POINTS) - gtk_page_setup_get_left_margin(pageSetup, GTK_UNIT_POINTS) - gtk_page_setup_get_right_margin(pageSetup, GTK_UNIT_POINTS))
    , availablePaperHeight(gtk_page_setup_get_paper_height(pageSetup, GTK_UNIT_POINTS) - gtk_page_setup_get_top_margin(pageSetup, GTK_UNIT_POINTS) - gtk_page_setup_get_bottom_margin(pageSetup, GTK_UNIT_POINTS))
    , printSettings(settings)
    , pageSetup(pageSetup)
{
    ASSERT(settings);
    ASSERT(pageSetup);
}

}
