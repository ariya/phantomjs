/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2,1 of the License, or (at your option) any later version.
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
 */

#include "config.h"

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

static void loadChangedCallback(WebKitWebView*, WebKitLoadEvent loadEvent, gpointer)
{
    // Send a message to the parent process when we're ready.
    if (loadEvent == WEBKIT_LOAD_FINISHED)
        g_print("OK");
}

int main(int argc, char** argv)
{
    // Make sure that both GAIL and the ATK bridge are loaded.
    g_setenv("GTK_MODULES", "gail:atk-bridge", TRUE);

    gtk_init(&argc, &argv);

    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    webkit_web_view_load_html(webView,
                              "<html>"
                              "  <body>"
                              "   <h1>This is a test</h1>"
                              "   <p>This is a paragraph with some plain text.</p>"
                              "   <p>This paragraph contains <a href=\"http://www.webkitgtk.org\">a link</a> in the middle.</p>"
                              "  </body>"
                              "</html>",
                              0);

    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(webView));
    gtk_widget_show_all(window);

    g_signal_connect(window, "delete-event", G_CALLBACK(gtk_main_quit), 0);
    g_signal_connect(webView, "load-changed", G_CALLBACK(loadChangedCallback), 0);

    gtk_main();
}
