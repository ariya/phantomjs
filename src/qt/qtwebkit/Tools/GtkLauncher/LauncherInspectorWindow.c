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

#include "LauncherInspectorWindow.h"

struct _LauncherInspectorWindow {
    GtkWindow parent;

    WebKitWebInspector *inspector;
    GtkWidget *webView;
};

struct _LauncherInspectorWindowClass {
    GtkWindowClass parent;
};

G_DEFINE_TYPE(LauncherInspectorWindow, launcher_inspector_window, GTK_TYPE_WINDOW)

static void launcherInspectorWindowFinalize(GObject *gObject)
{
    LauncherInspectorWindow *inspectorWindow = LAUNCHER_INSPECTOR_WINDOW(gObject);
    if (inspectorWindow->inspector)
        g_object_unref(inspectorWindow->inspector);

    G_OBJECT_CLASS(launcher_inspector_window_parent_class)->finalize(gObject);
}

static void launcher_inspector_window_init(LauncherInspectorWindow *inspectorWindow)
{
    gtk_window_set_title(GTK_WINDOW(inspectorWindow), "Web Inspector");
    gtk_window_set_default_size(GTK_WINDOW(inspectorWindow), 800, 600);
}

static void launcher_inspector_window_class_init(LauncherInspectorWindowClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->finalize = launcherInspectorWindowFinalize;
}

static void inspectedURIChanged(WebKitWebInspector *inspector, GParamSpec *paramSpec, LauncherInspectorWindow *inspectorWindow)
{
    gchar *title = g_strdup_printf("Web Inspector - %s", webkit_web_inspector_get_inspected_uri(inspector));
    gtk_window_set_title(GTK_WINDOW(inspectorWindow), title);
    g_free(title);
}

static gboolean showInspectorWindow(WebKitWebInspector *inspector, LauncherInspectorWindow *inspectorWindow)
{
    gtk_widget_show(GTK_WIDGET(inspectorWindow));
    return TRUE;
}

static gboolean closeInspectorWindow(WebKitWebInspector *inspector, LauncherInspectorWindow *inspectorWindow)
{
    gtk_widget_hide(GTK_WIDGET(inspectorWindow));
    return TRUE;
}

GtkWidget *launcherInspectorWindowNew(WebKitWebInspector *inspector, GtkWindow *parent)
{
    LauncherInspectorWindow *inspectorWindow = LAUNCHER_INSPECTOR_WINDOW(g_object_new(LAUNCHER_TYPE_INSPECTOR_WINDOW, "type", GTK_WINDOW_TOPLEVEL, NULL));
    inspectorWindow->inspector = g_object_ref(inspector);
    inspectorWindow->webView = webkit_web_view_new();
    gtk_window_set_transient_for(GTK_WINDOW(inspectorWindow), parent);

    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), inspectorWindow->webView);
    gtk_widget_show(inspectorWindow->webView);

    gtk_container_add(GTK_CONTAINER(inspectorWindow), scrolledWindow);
    gtk_widget_show(scrolledWindow);

    g_signal_connect(inspector, "notify::inspected-uri", G_CALLBACK(inspectedURIChanged), inspectorWindow);
    g_signal_connect(inspector, "show-window", G_CALLBACK(showInspectorWindow), inspectorWindow);
    g_signal_connect(inspector, "close-window", G_CALLBACK(closeInspectorWindow), inspectorWindow);

    return GTK_WIDGET(inspectorWindow);
}

WebKitWebView *launcherInspectorWindowGetWebView(LauncherInspectorWindow *inspectorWindow)
{
    g_return_val_if_fail(LAUNCHER_IS_INSPECTOR_WINDOW(inspectorWindow), 0);

    return WEBKIT_WEB_VIEW(inspectorWindow->webView);
}
