/*
 * Copyright (C) 2007, 2008, 2009 Holger Hans Peter Freyther
 * Copyright (C) 2008 Jan Michael C. Alonzo
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2010 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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

#ifndef webkitwebinspectorprivate_h
#define webkitwebinspectorprivate_h

extern "C" {

void webkit_web_inspector_set_inspector_client(WebKitWebInspector*, WebCore::Page*);

void webkit_web_inspector_set_web_view(WebKitWebInspector*, WebKitWebView*);

void webkit_web_inspector_set_inspected_uri(WebKitWebInspector*, const gchar*);

WEBKIT_API void webkit_web_inspector_execute_script(WebKitWebInspector*, long callId, const gchar* script);

}

#endif
