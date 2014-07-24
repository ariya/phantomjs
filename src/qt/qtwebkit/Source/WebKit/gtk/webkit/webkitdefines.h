/*
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora Ltd.
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

#ifndef webkitdefines_h
#define webkitdefines_h

#include <glib.h>

#ifdef G_OS_WIN32
    #ifdef BUILDING_WEBKIT
        #define WEBKIT_API __declspec(dllexport)
    #else
        #define WEBKIT_API __declspec(dllimport)
    #endif
    #define WEBKIT_OBSOLETE_API WEBKIT_API
#else
    #define WEBKIT_API __attribute__((visibility("default")))
    #define WEBKIT_OBSOLETE_API WEBKIT_API __attribute__((deprecated))
#endif

#ifndef WEBKIT_API
    #define WEBKIT_API
#endif

G_BEGIN_DECLS

typedef struct _WebKitIconDatabase WebKitIconDatabase;
typedef struct _WebKitIconDatabaseClass WebKitIconDatabaseClass;

typedef struct _WebKitNetworkRequest WebKitNetworkRequest;
typedef struct _WebKitNetworkRequestClass WebKitNetworkRequestClass;

typedef struct _WebKitNetworkResponse WebKitNetworkResponse;
typedef struct _WebKitNetworkResponseClass WebKitNetworkResponseClass;

typedef struct _WebKitWebBackForwardList WebKitWebBackForwardList;
typedef struct _WebKitWebBackForwardListClass WebKitWebBackForwardListClass;

typedef struct _WebKitWebHistoryItem WebKitWebHistoryItem;
typedef struct _WebKitWebHistoryItemClass WebKitWebHistoryItemClass;

typedef struct _WebKitWebFrame WebKitWebFrame;
typedef struct _WebKitWebFrameClass WebKitWebFrameClass;

typedef struct _WebKitWebPolicyDecision WebKitWebPolicyDecision;
typedef struct _WebKitWebPolicyDecisionClass WebKitWebPolicyDecisionClass;

typedef struct _WebKitWebSettings WebKitWebSettings;
typedef struct _WebKitWebSettingsClass WebKitWebSettingsClass;

typedef struct _WebKitWebInspector WebKitWebInspector;
typedef struct _WebKitWebInspectorClass WebKitWebInspectorClass;

typedef struct _WebKitWebWindowFeatures WebKitWebWindowFeatures;
typedef struct _WebKitWebWindowFeaturesClass WebKitWebWindowFeaturesClass;

typedef struct _WebKitWebView WebKitWebView;
typedef struct _WebKitWebViewClass WebKitWebViewClass;

typedef struct _WebKitDownload WebKitDownload;
typedef struct _WebKitDownloadClass WebKitDownloadClass;

typedef struct _WebKitWebResource WebKitWebResource;
typedef struct _WebKitWebResourceClass WebKitWebResourceClass;

typedef struct _WebKitWebDataSource WebKitWebDataSource;
typedef struct _WebKitWebDataSourceClass WebKitWebDataSourceClass;

typedef struct _WebKitWebDatabase WebKitWebDatabase;
typedef struct _WebKitWebDatabaseClass WebKitWebDatabaseClass;

typedef struct _WebKitSecurityOrigin WebKitSecurityOrigin;
typedef struct _WebKitSecurityOriginClass WebKitSecurityOriginClass;

typedef struct _WebKitHitTestResult WebKitHitTestResult;
typedef struct _WebKitHitTestResultClass WebKitHitTestResultClass;

typedef struct _WebKitGeolocationPolicyDecision WebKitGeolocationPolicyDecision;
typedef struct _WebKitGeolocationPolicyDecisionClass WebKitGeolocationPolicyDecisionClass;

typedef struct _WebKitViewportAttributes WebKitViewportAttributes;
typedef struct _WebKitViewportAttributesClass WebKitViewportAttributesClass;

typedef struct _WebKitWebPluginDatabase        WebKitWebPluginDatabase;
typedef struct _WebKitWebPluginDatabaseClass   WebKitWebPluginDatabaseClass;

typedef struct _WebKitWebPlugin      WebKitWebPlugin;
typedef struct _WebKitWebPluginClass WebKitWebPluginClass;

typedef struct _WebKitSpellChecker WebKitSpellChecker; /* dummy typedef */
typedef struct _WebKitSpellCheckerInterface WebKitSpellCheckerInterface;

typedef struct _WebKitFaviconDatabase WebKitFaviconDatabase;
typedef struct _WebKitFaviconDatabaseClass WebKitFaviconDatabaseClass;

G_END_DECLS

#endif
