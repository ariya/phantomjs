/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Holger Hans Peter Freyther
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
#include "Icon.h"

#include "GraphicsContext.h"
#include "MIMETypeRegistry.h"
#include "PlatformContextCairo.h"

#include <gtk/gtk.h>

#include <wtf/PassRefPtr.h>
#include <wtf/text/CString.h>

namespace WebCore {

Icon::Icon()
    : m_icon(0)
{
}

Icon::~Icon()
{
    if (m_icon)
        g_object_unref(m_icon);
}

static String lookupIconName(String MIMEType)
{
    /*
     Lookup an appropriate icon according to either the Icon Naming Spec
     or conventional Gnome icon names respectively.

     See http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html

     The icon theme is probed for the following names:
     1. media-subtype
     2. gnome-mime-media-subtype
     3. media-x-generic
     4. gnome-mime-media

     In the worst case it falls back to the stock file icon.
    */
    int pos = MIMEType.find('/');
    if(pos >= 0) {
        String media = MIMEType.substring(0, pos);
        String subtype = MIMEType.substring(pos + 1);
        GtkIconTheme* iconTheme = gtk_icon_theme_get_default();
        String iconName = media + "-" + subtype;
        if(gtk_icon_theme_has_icon(iconTheme, iconName.utf8().data()))
            return iconName;
        iconName = "gnome-mime-" + media + "-" + subtype;
        if(gtk_icon_theme_has_icon(iconTheme, iconName.utf8().data()))
            return iconName;
        iconName = media + "-x-generic";
        if(gtk_icon_theme_has_icon(iconTheme, iconName.utf8().data()))
            return iconName;
        iconName = media + "gnome-mime-" + media;
        if(gtk_icon_theme_has_icon(iconTheme, iconName.utf8().data()))
            return iconName;
    }
    return GTK_STOCK_FILE;
}

// FIXME: Move the code to ChromeClient::iconForFiles().
PassRefPtr<Icon> Icon::createIconForFiles(const Vector<String>& filenames)
{
    if (filenames.isEmpty())
        return 0;

    if (filenames.size() == 1) {
        if (!g_path_skip_root(filenames[0].utf8().data()))
            return 0;

        String MIMEType = MIMETypeRegistry::getMIMETypeForPath(filenames[0]);
        String iconName = lookupIconName(MIMEType);

        RefPtr<Icon> icon = adoptRef(new Icon);
        icon->m_icon = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), iconName.utf8().data(), 16, GTK_ICON_LOOKUP_USE_BUILTIN, 0);
        if (!icon->m_icon)
            return 0;
        return icon.release();
    }

    //FIXME: Implement this
    return 0;
}

void Icon::paint(GraphicsContext* context, const IntRect& rect)
{
    if (context->paintingDisabled())
        return;

    // TODO: Scale/clip the image if necessary.
    cairo_t* cr = context->platformContext()->cr();
    cairo_save(cr);
    gdk_cairo_set_source_pixbuf(cr, m_icon, rect.x(), rect.y());
    cairo_paint(cr);
    cairo_restore(cr);
}

}
