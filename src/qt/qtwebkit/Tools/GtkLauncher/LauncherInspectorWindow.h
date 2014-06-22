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

#ifndef LauncherInspectorWindow_h
#define LauncherInspectorWindow_h

#include <gtk/gtk.h>
#include <webkit/webkit.h>

G_BEGIN_DECLS

#define LAUNCHER_TYPE_INSPECTOR_WINDOW            (launcher_inspector_window_get_type())
#define LAUNCHER_INSPECTOR_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), LAUNCHER_TYPE_INSPECTOR_WINDOW, LauncherInspectorWindow))
#define LAUNCHER_IS_INSPECTOR_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), LAUNCHER_TYPE_INSPECTOR_WINDOW))
#define LAUNCHER_INSPECTOR_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  LAUNCHER_TYPE_INSPECTOR_WINDOW, LauncherInspectorWindowClass))
#define LAUNCHER_IS_INSPECTOR_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  LAUNCHER_TYPE_INSPECTOR_WINDOW))
#define LAUNCHER_INSPECTOR_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  LAUNCHER_TYPE_INSPECTOR_WINDOW, LauncherInspectorWindowClass))

typedef struct _LauncherInspectorWindow        LauncherInspectorWindow;
typedef struct _LauncherInspectorWindowClass   LauncherInspectorWindowClass;

GType launcher_inspector_window_get_type(void);

GtkWidget *launcherInspectorWindowNew(WebKitWebInspector *, GtkWindow *parent);
WebKitWebView *launcherInspectorWindowGetWebView(LauncherInspectorWindow *);

G_END_DECLS

#endif
