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

#ifndef webkitwebsettingsprivate_h
#define webkitwebsettingsprivate_h

#include "webkitwebsettings.h"
#include <wtf/text/CString.h>

extern "C" {

struct _WebKitWebSettingsPrivate {
    CString defaultEncoding;
    CString cursiveFontFamily;
    CString defaultFontFamily;
    CString fantasyFontFamily;
    CString monospaceFontFamily;
    CString sansSerifFontFamily;
    CString serifFontFamily;
    guint defaultFontSize;
    guint defaultMonospaceFontSize;
    guint minimumFontSize;
    guint minimumLogicalFontSize;
    gboolean enforce96DPI;
    gboolean autoLoadImages;
    gboolean autoShrinkImages;
    gboolean respectImageOrientation;
    gboolean printBackgrounds;
    gboolean enableScripts;
    gboolean enablePlugins;
    gboolean resizableTextAreas;
    CString userStylesheetURI;
    gfloat zoomStep;
    gboolean enableDeveloperExtras;
    gboolean enablePrivateBrowsing;
    gboolean enableSpellChecking;
    CString spellCheckingLanguages;
    gboolean enableCaretBrowsing;
    gboolean enableHTML5Database;
    gboolean enableHTML5LocalStorage;
    CString html5LocalStorageDatabasePath;
    gboolean enableXSSAuditor;
    gboolean enableSpatialNavigation;
    gboolean enableFrameFlattening;
    CString userAgent;
    gboolean javascriptCanOpenWindowsAutomatically;
    gboolean javascriptCanAccessClipboard;
    gboolean enableOfflineWebApplicationCache;
    WebKitEditingBehavior editingBehavior;
    gboolean enableUniversalAccessFromFileURIs;
    gboolean enableFileAccessFromFileURIs;
    gboolean enableDOMPaste;
    gboolean tabKeyCyclesThroughElements;
    gboolean enableDefaultContextMenu;
    gboolean enableSiteSpecificQuirks;
    gboolean enablePageCache;
    gboolean autoResizeWindow;
    gboolean enableJavaApplet;
    gboolean enableHyperlinkAuditing;
    gboolean enableFullscreen;
    gboolean enableDNSPrefetching;
    gboolean enableWebgl;
    gboolean enableMediaStream;
    gboolean enableWebAudio;
    gboolean enableAcceleratedCompositing;
    gboolean enableSmoothScrolling;
    gboolean enableCSSShaders;
    gboolean mediaPlaybackRequiresUserGesture;
    gboolean mediaPlaybackAllowsInline;
    gboolean enableDisplayOfInsecureContent;
    gboolean enableRunningOfInsecureContent;
};

WEBKIT_API void webkit_web_settings_add_extra_plugin_directory(WebKitWebView*, const gchar* directory);

WEBKIT_API char* webkitWebSettingsUserAgentForURI(WebKitWebSettings*, const gchar* uri);

GSList* webkitWebViewGetEnchantDicts(WebKitWebView*);

}

#endif
