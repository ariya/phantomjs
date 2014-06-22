/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WTF_FeatureDefines_h
#define WTF_FeatureDefines_h

/* Use this file to list _all_ ENABLE() macros. Define the macros to be one of the following values:
 *  - "0" disables the feature by default. The feature can still be enabled for a specific port or environment.
 *  - "1" enables the feature by default. The feature can still be disabled for a specific port or environment.
 *
 * The feature defaults in this file are only taken into account if the (port specific) build system
 * has not enabled or disabled a particular feature. 
 *
 * Use this file to define ENABLE() macros only. Do not use this file to define USE() or macros !
 *
 * Only define a macro if it was not defined before - always check for !defined first.
 * 
 * Keep the file sorted by the name of the defines. As an exception you can change the order
 * to allow interdependencies between the default values.
 * 
 * Below are a few potential commands to take advantage of this file running from the Source/WTF directory
 *
 * Get the list of feature defines: grep -o "ENABLE_\(\w\+\)" wtf/FeatureDefines.h | sort | uniq
 * Get the list of features enabled by default for a PLATFORM(XXX): gcc -E -dM -I. -DWTF_PLATFORM_XXX "wtf/Platform.h" | grep "ENABLE_\w\+ 1" | cut -d' ' -f2 | sort 
 */

/* FIXME: Move out the PLATFORM specific rules into platform specific files. */

/* --------- Apple IOS (but not MAC) port --------- */
/* PLATFORM(IOS) is a specialization of PLATFORM(MAC). */
/* PLATFORM(MAC) is always enabled when PLATFORM(IOS) is enabled. */
#if PLATFORM(IOS)

#if !defined(ENABLE_8BIT_TEXTRUN)
#define ENABLE_8BIT_TEXTRUN 1
#endif

#if !defined(ENABLE_CONTEXT_MENUS)
#define ENABLE_CONTEXT_MENUS 0
#endif

#if !defined(ENABLE_CSS_IMAGE_SET)
#define ENABLE_CSS_IMAGE_SET 1
#endif

#if !defined(ENABLE_DRAG_SUPPORT)
#define ENABLE_DRAG_SUPPORT 0
#endif

#if !defined(ENABLE_GEOLOCATION)
#define ENABLE_GEOLOCATION 1
#endif

#if !defined(ENABLE_ICONDATABASE)
#define ENABLE_ICONDATABASE 0
#endif

#if !defined(ENABLE_NETSCAPE_PLUGIN_API)
#define ENABLE_NETSCAPE_PLUGIN_API 0
#endif

#if !defined(ENABLE_ORIENTATION_EVENTS)
#define ENABLE_ORIENTATION_EVENTS 1 
#endif

#if !defined(ENABLE_REPAINT_THROTTLING)
#define ENABLE_REPAINT_THROTTLING 1 
#endif

#if !defined(ENABLE_TEXT_CARET)
#define ENABLE_TEXT_CARET 0
#endif

#if !defined(ENABLE_WEB_ARCHIVE)
#define ENABLE_WEB_ARCHIVE 1
#endif

#if !defined(ENABLE_VIEW_MODE_CSS_MEDIA)
#define ENABLE_VIEW_MODE_CSS_MEDIA 0
#endif

#if !defined(ENABLE_WEBGL)
#define ENABLE_WEBGL 1
#endif

#endif /* PLATFORM(IOS) */

/* --------- Apple MAC port (not IOS) --------- */
#if PLATFORM(MAC) && !PLATFORM(IOS)

#if !defined(ENABLE_8BIT_TEXTRUN)
#define ENABLE_8BIT_TEXTRUN 1
#endif

#if !defined(ENABLE_CSS_IMAGE_SET)
#define ENABLE_CSS_IMAGE_SET 1
#endif

#if !defined(ENABLE_DASHBOARD_SUPPORT)
#define ENABLE_DASHBOARD_SUPPORT 1
#endif

#if !defined(ENABLE_DELETION_UI)
#define ENABLE_DELETION_UI 1
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
#if !defined(ENABLE_ENCRYPTED_MEDIA)
#define ENABLE_ENCRYPTED_MEDIA 1
#endif
#if !defined(ENABLE_ENCRYPTED_MEDIA_V2)
#define ENABLE_ENCRYPTED_MEDIA_V2 1
#endif
#endif

#if !defined(ENABLE_FULLSCREEN_API)
#define ENABLE_FULLSCREEN_API 1
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
#if !defined(ENABLE_GESTURE_EVENTS)
#define ENABLE_GESTURE_EVENTS 1
#endif
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
#if !defined(ENABLE_RUBBER_BANDING)
#define ENABLE_RUBBER_BANDING 1
#endif
#endif

#if !defined(ENABLE_SMOOTH_SCROLLING)
#define ENABLE_SMOOTH_SCROLLING 1
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
#if !defined(ENABLE_THREADED_SCROLLING)
#define ENABLE_THREADED_SCROLLING 1
#endif
#endif

#if ENABLE(VIDEO)
#if !defined(ENABLE_VIDEO_TRACK)
#define ENABLE_VIDEO_TRACK 1
#endif
#endif

#if !defined(ENABLE_VIEW_MODE_CSS_MEDIA)
#define ENABLE_VIEW_MODE_CSS_MEDIA 0
#endif

#if !defined(ENABLE_WEB_ARCHIVE)
#define ENABLE_WEB_ARCHIVE 1
#endif

#if !defined(ENABLE_WEB_AUDIO)
#define ENABLE_WEB_AUDIO 1
#endif

#if !defined(ENABLE_CURSOR_VISIBILITY)
#define ENABLE_CURSOR_VISIBILITY 1
#endif

#endif /* PLATFORM(MAC) && !PLATFORM(IOS) */

/* --------- Apple Windows port --------- */
#if PLATFORM(WIN) && !OS(WINCE) && !PLATFORM(WIN_CAIRO)

#if !defined(ENABLE_FULLSCREEN_API)
#define ENABLE_FULLSCREEN_API 1
#endif

#if !defined(ENABLE_WEB_ARCHIVE)
#define ENABLE_WEB_ARCHIVE 1
#endif

#endif /* PLATFORM(WIN) && !OS(WINCE) && !PLATFORM(WIN_CAIRO) */

/* --------- WinCE port --------- */
/* WinCE port is a specialization of PLATFORM(WIN). */
/* PLATFORM(WIN) is always enabled when building for the WinCE port. */
#if PLATFORM(WIN) && OS(WINCE)

#if !defined(ENABLE_DRAG_SUPPORT)
#define ENABLE_DRAG_SUPPORT 0
#endif

#if !defined(ENABLE_FTPDIR)
#define ENABLE_FTPDIR 0
#endif

#if !defined(ENABLE_INSPECTOR)
#define ENABLE_INSPECTOR 0
#endif

#endif /* PLATFORM(WIN) && OS(WINCE) */

/* --------- Windows CAIRO port --------- */
/* PLATFORM(WIN_CAIRO) is a specialization of PLATFORM(WIN). */
/* PLATFORM(WIN) is always enabled when PLATFORM(WIN_CAIRO) is enabled. */
#if PLATFORM(WIN_CAIRO)

#if !defined(ENABLE_WEB_ARCHIVE)
#define ENABLE_WEB_ARCHIVE 1
#endif

#if !defined(ENABLE_VIEW_MODE_CSS_MEDIA)
#define ENABLE_VIEW_MODE_CSS_MEDIA 0
#endif

#endif /* PLATFORM(WIN_CAIRO) */

/* --------- EFL port (Unix) --------- */
#if PLATFORM(EFL)

#if !defined(ENABLE_PLUGIN_PACKAGE_SIMPLE_HASH)
#define ENABLE_PLUGIN_PACKAGE_SIMPLE_HASH 1
#endif

#if !defined(ENABLE_SUBPIXEL_LAYOUT)
#define ENABLE_SUBPIXEL_LAYOUT 1
#endif

#endif /* PLATFORM(EFL) */

/* --------- Gtk port (Unix, Windows, Mac) --------- */
#if PLATFORM(GTK)

#if OS(UNIX)
#if !defined(ENABLE_PLUGIN_PACKAGE_SIMPLE_HASH)
#define ENABLE_PLUGIN_PACKAGE_SIMPLE_HASH 1
#endif
#endif

#endif /* PLATFORM(GTK) */

/* --------- Qt port (Unix, Windows, Mac, WinCE) --------- */
#if PLATFORM(QT)

#if !defined(ENABLE_PLUGIN_PACKAGE_SIMPLE_HASH)
#define ENABLE_PLUGIN_PACKAGE_SIMPLE_HASH 1
#endif

#endif /* PLATFORM(QT) */

/* --------- Blackberry port (QNX) --------- */
#if PLATFORM(BLACKBERRY)

#if !defined(ENABLE_BLACKBERRY_CREDENTIAL_PERSIST)
#define ENABLE_BLACKBERRY_CREDENTIAL_PERSIST 1
#endif

#endif /* PLATFORM(BLACKBERRY) */

/* ENABLE macro defaults for WebCore */
/* Do not use PLATFORM() tests in this section ! */

#if !defined(ENABLE_3D_RENDERING)
#define ENABLE_3D_RENDERING 0
#endif

#if !defined(ENABLE_8BIT_TEXTRUN)
#define ENABLE_8BIT_TEXTRUN 0
#endif

#if !defined(ENABLE_ACCELERATED_2D_CANVAS)
#define ENABLE_ACCELERATED_2D_CANVAS 0
#endif

#if !defined(ENABLE_ACCELERATED_OVERFLOW_SCROLLING)
#define ENABLE_ACCELERATED_OVERFLOW_SCROLLING 0
#endif

#if !defined(ENABLE_BATTERY_STATUS)
#define ENABLE_BATTERY_STATUS 0
#endif

#if !defined(ENABLE_BLOB)
#define ENABLE_BLOB 0
#endif

#if !defined(ENABLE_CANVAS_PATH)
#define ENABLE_CANVAS_PATH 1
#endif

#if !defined(ENABLE_CANVAS_PROXY)
#define ENABLE_CANVAS_PROXY 0
#endif

#if !defined(ENABLE_CHANNEL_MESSAGING)
#define ENABLE_CHANNEL_MESSAGING 1
#endif

#if !defined(ENABLE_CONTEXT_MENUS)
#define ENABLE_CONTEXT_MENUS 1
#endif

#if !defined(ENABLE_CSP_NEXT)
#define ENABLE_CSP_NEXT 0
#endif

#if !defined(ENABLE_CSS3_CONDITIONAL_RULES)
#define ENABLE_CSS3_CONDITIONAL_RULES 0
#endif

#if !defined(ENABLE_CSS3_TEXT)
#define ENABLE_CSS3_TEXT 0
#endif

#if !defined(ENABLE_CSS_BOX_DECORATION_BREAK)
#define ENABLE_CSS_BOX_DECORATION_BREAK 1
#endif

#if !defined(ENABLE_CSS_DEVICE_ADAPTATION)
#define ENABLE_CSS_DEVICE_ADAPTATION 0
#endif

#if !defined(ENABLE_CSS_COMPOSITING)
#define ENABLE_CSS_COMPOSITING 0
#endif

#if !defined(ENABLE_CSS_FILTERS)
#define ENABLE_CSS_FILTERS 0
#endif

#if !defined(ENABLE_CSS_IMAGE_ORIENTATION)
#define ENABLE_CSS_IMAGE_ORIENTATION 0
#endif

#if !defined(ENABLE_CSS_IMAGE_RESOLUTION)
#define ENABLE_CSS_IMAGE_RESOLUTION 0
#endif

#if !defined(ENABLE_CSS_IMAGE_SET)
#define ENABLE_CSS_IMAGE_SET 0
#endif

#if !defined(ENABLE_CSS_SHADERS)
#define ENABLE_CSS_SHADERS 0
#endif

#if !defined(ENABLE_CSS_STICKY_POSITION)
#define ENABLE_CSS_STICKY_POSITION 0
#endif

#if !defined(ENABLE_CSS_TRANSFORMS_ANIMATIONS_TRANSITIONS_UNPREFIXED)
#define ENABLE_CSS_TRANSFORMS_ANIMATIONS_TRANSITIONS_UNPREFIXED 0
#endif

#if !defined(ENABLE_CSS_VARIABLES)
#define ENABLE_CSS_VARIABLES 0
#endif

#if !defined(ENABLE_CUSTOM_SCHEME_HANDLER)
#define ENABLE_CUSTOM_SCHEME_HANDLER 0
#endif

#if !defined(ENABLE_DASHBOARD_SUPPORT)
#define ENABLE_DASHBOARD_SUPPORT 0
#endif

#if !defined(ENABLE_DATALIST_ELEMENT)
#define ENABLE_DATALIST_ELEMENT 0
#endif

#if !defined(ENABLE_DATA_TRANSFER_ITEMS)
#define ENABLE_DATA_TRANSFER_ITEMS 0
#endif

#if !defined(ENABLE_DELETION_UI)
#define ENABLE_DELETION_UI 0
#endif

#if !defined(ENABLE_DETAILS_ELEMENT)
#define ENABLE_DETAILS_ELEMENT 1
#endif

#if !defined(ENABLE_DEVICE_ORIENTATION)
#define ENABLE_DEVICE_ORIENTATION 0
#endif

#if !defined(ENABLE_DIALOG_ELEMENT)
#define ENABLE_DIALOG_ELEMENT 0
#endif

#if !defined(ENABLE_DIRECTORY_UPLOAD)
#define ENABLE_DIRECTORY_UPLOAD 0
#endif

#if !defined(ENABLE_DOWNLOAD_ATTRIBUTE)
#define ENABLE_DOWNLOAD_ATTRIBUTE 0
#endif

#if !defined(ENABLE_DRAGGABLE_REGION)
#define ENABLE_DRAGGABLE_REGION 0
#endif

#if !defined(ENABLE_DRAG_SUPPORT)
#define ENABLE_DRAG_SUPPORT 1
#endif

#if !defined(ENABLE_ENCRYPTED_MEDIA)
#define ENABLE_ENCRYPTED_MEDIA 0
#endif

#if !defined(ENABLE_ENCRYPTED_MEDIA_V2)
#define ENABLE_ENCRYPTED_MEDIA_V2 0
#endif

#if !defined(ENABLE_FAST_MOBILE_SCROLLING)
#define ENABLE_FAST_MOBILE_SCROLLING 0
#endif

#if !defined(ENABLE_FILE_SYSTEM)
#define ENABLE_FILE_SYSTEM 0
#endif

#if !defined(ENABLE_FILTERS)
#define ENABLE_FILTERS 0
#endif

#if !defined(ENABLE_FONT_LOAD_EVENTS)
#define ENABLE_FONT_LOAD_EVENTS 0
#endif

#if !defined(ENABLE_FTPDIR)
#define ENABLE_FTPDIR 1
#endif

#if !defined(ENABLE_FULLSCREEN_API)
#define ENABLE_FULLSCREEN_API 0
#endif

#if !defined(ENABLE_GAMEPAD)
#define ENABLE_GAMEPAD 0
#endif

#if !defined(ENABLE_GEOLOCATION)
#define ENABLE_GEOLOCATION 0
#endif

#if !defined(ENABLE_GESTURE_EVENTS)
#define ENABLE_GESTURE_EVENTS 0
#endif

#if !defined(ENABLE_GLIB_SUPPORT)
#define ENABLE_GLIB_SUPPORT 0
#endif

#if !defined(ENABLE_HIDDEN_PAGE_DOM_TIMER_THROTTLING)
#define ENABLE_HIDDEN_PAGE_DOM_TIMER_THROTTLING 0
#endif

#if !defined(ENABLE_HIGH_DPI_CANVAS)
#define ENABLE_HIGH_DPI_CANVAS 0
#endif

#if !defined(ENABLE_ICONDATABASE)
#define ENABLE_ICONDATABASE 1
#endif

#if !defined(ENABLE_IFRAME_SEAMLESS)
#define ENABLE_IFRAME_SEAMLESS 1
#endif

#if !defined(ENABLE_IMAGE_DECODER_DOWN_SAMPLING)
#define ENABLE_IMAGE_DECODER_DOWN_SAMPLING 0
#endif

#if !defined(ENABLE_INDEXED_DATABASE)
#define ENABLE_INDEXED_DATABASE 0
#endif

#if !defined(ENABLE_INPUT_SPEECH)
#define ENABLE_INPUT_SPEECH 0
#endif

#if !defined(ENABLE_INPUT_TYPE_COLOR)
#define ENABLE_INPUT_TYPE_COLOR 0
#endif

#if !defined(ENABLE_INPUT_TYPE_DATE)
#define ENABLE_INPUT_TYPE_DATE 0
#endif

#if !defined(ENABLE_INPUT_TYPE_DATETIME_INCOMPLETE)
#define ENABLE_INPUT_TYPE_DATETIME_INCOMPLETE 0
#endif

#if !defined(ENABLE_INPUT_TYPE_DATETIMELOCAL)
#define ENABLE_INPUT_TYPE_DATETIMELOCAL 0
#endif

#if !defined(ENABLE_INPUT_TYPE_MONTH)
#define ENABLE_INPUT_TYPE_MONTH 0
#endif

#if !defined(ENABLE_INPUT_TYPE_TIME)
#define ENABLE_INPUT_TYPE_TIME 0
#endif

#if !defined(ENABLE_INPUT_TYPE_WEEK)
#define ENABLE_INPUT_TYPE_WEEK 0
#endif

#if ENABLE(INPUT_TYPE_DATE) || ENABLE(INPUT_TYPE_DATETIME_INCOMPLETE) || ENABLE(INPUT_TYPE_DATETIMELOCAL) || ENABLE(INPUT_TYPE_MONTH) || ENABLE(INPUT_TYPE_TIME) || ENABLE(INPUT_TYPE_WEEK)
#if !defined(ENABLE_DATE_AND_TIME_INPUT_TYPES)
#define ENABLE_DATE_AND_TIME_INPUT_TYPES 1
#endif
#endif

#if !defined(ENABLE_INSPECTOR)
#define ENABLE_INSPECTOR 1
#endif

#if !defined(ENABLE_JAVASCRIPT_DEBUGGER)
#define ENABLE_JAVASCRIPT_DEBUGGER 1
#endif

#if !defined(ENABLE_JAVASCRIPT_I18N_API)
#define ENABLE_JAVASCRIPT_I18N_API 0
#endif

#if !defined(ENABLE_LEGACY_CSS_VENDOR_PREFIXES)
#define ENABLE_LEGACY_CSS_VENDOR_PREFIXES 0
#endif

#if !defined(ENABLE_LEGACY_NOTIFICATIONS)
#define ENABLE_LEGACY_NOTIFICATIONS 0
#endif

#if !defined(ENABLE_LEGACY_VENDOR_PREFIXES)
#define ENABLE_LEGACY_VENDOR_PREFIXES 0
#endif

#if !defined(ENABLE_LEGACY_VIEWPORT_ADAPTION)
#define ENABLE_LEGACY_VIEWPORT_ADAPTION 0
#endif

#if !defined(ENABLE_LINK_PREFETCH)
#define ENABLE_LINK_PREFETCH 0
#endif

#if !defined(ENABLE_MATHML)
#define ENABLE_MATHML 1
#endif

#if !defined(ENABLE_MEDIA_CAPTURE)
#define ENABLE_MEDIA_CAPTURE 0
#endif

#if !defined(ENABLE_MEDIA_SOURCE)
#define ENABLE_MEDIA_SOURCE 0
#endif

#if !defined(ENABLE_MEDIA_STATISTICS)
#define ENABLE_MEDIA_STATISTICS 0
#endif

#if !defined(ENABLE_MEDIA_STREAM)
#define ENABLE_MEDIA_STREAM 0
#endif

#if !defined(ENABLE_METER_ELEMENT)
#define ENABLE_METER_ELEMENT 1
#endif

#if !defined(ENABLE_MHTML)
#define ENABLE_MHTML 0
#endif

#if !defined(ENABLE_MICRODATA)
#define ENABLE_MICRODATA 0
#endif

#if !defined(ENABLE_MOUSE_CURSOR_SCALE)
#define ENABLE_MOUSE_CURSOR_SCALE 0
#endif

#if !defined(ENABLE_NAVIGATOR_CONTENT_UTILS)
#define ENABLE_NAVIGATOR_CONTENT_UTILS 0
#endif

#undef ENABLE_NETSCAPE_PLUGIN_API
#define ENABLE_NETSCAPE_PLUGIN_API 0

#if !defined(ENABLE_NETSCAPE_PLUGIN_METADATA_CACHE)
#define ENABLE_NETSCAPE_PLUGIN_METADATA_CACHE 0
#endif

#if !defined(ENABLE_NETWORK_INFO)
#define ENABLE_NETWORK_INFO 0
#endif

#if !defined(ENABLE_NOTIFICATIONS)
#define ENABLE_NOTIFICATIONS 0
#endif

#if !defined(ENABLE_OBJECT_MARK_LOGGING)
#define ENABLE_OBJECT_MARK_LOGGING 0
#endif

#if !defined(ENABLE_OPENCL)
#define ENABLE_OPENCL 0
#endif

#if !defined(ENABLE_OPENTYPE_VERTICAL)
#define ENABLE_OPENTYPE_VERTICAL 0
#endif

#if !defined(ENABLE_ORIENTATION_EVENTS)
#define ENABLE_ORIENTATION_EVENTS 0
#endif

#if !defined(ENABLE_PAGE_VISIBILITY_API)
#define ENABLE_PAGE_VISIBILITY_API 0
#endif

#if OS(WINDOWS)
#if !defined(ENABLE_PAN_SCROLLING)
#define ENABLE_PAN_SCROLLING 1
#endif
#endif

#if !defined(ENABLE_PLUGIN_PACKAGE_SIMPLE_HASH)
#define ENABLE_PLUGIN_PACKAGE_SIMPLE_HASH 0
#endif

#if !defined(ENABLE_PLUGIN_PROXY_FOR_VIDEO)
#define ENABLE_PLUGIN_PROXY_FOR_VIDEO 0
#endif

#if !defined(ENABLE_POINTER_LOCK)
#define ENABLE_POINTER_LOCK 0
#endif

#if !defined(ENABLE_PROGRESS_ELEMENT)
#define ENABLE_PROGRESS_ELEMENT 0
#endif

#if !defined(ENABLE_PROXIMITY_EVENTS)
#define ENABLE_PROXIMITY_EVENTS 0
#endif

#if !defined(ENABLE_QUOTA)
#define ENABLE_QUOTA 0
#endif

#if !defined(ENABLE_REPAINT_THROTTLING)
#define ENABLE_REPAINT_THROTTLING 0
#endif

#if !defined(ENABLE_REQUEST_ANIMATION_FRAME)
#define ENABLE_REQUEST_ANIMATION_FRAME 0
#endif

#if !defined(ENABLE_RUBBER_BANDING)
#define ENABLE_RUBBER_BANDING 0
#endif

#if !defined(ENABLE_SATURATED_LAYOUT_ARITHMETIC)
#define ENABLE_SATURATED_LAYOUT_ARITHMETIC 0
#endif

#if !defined(ENABLE_SCRIPTED_SPEECH)
#define ENABLE_SCRIPTED_SPEECH 0
#endif

#if !defined(ENABLE_SHADOW_DOM)
#define ENABLE_SHADOW_DOM 0
#endif

#if !defined(ENABLE_SHARED_WORKERS)
#define ENABLE_SHARED_WORKERS 0
#endif

#if !defined(ENABLE_SMOOTH_SCROLLING)
#define ENABLE_SMOOTH_SCROLLING 0
#endif

#if !defined(ENABLE_SPEECH_SYNTHESIS)
#define ENABLE_SPEECH_SYNTHESIS 0
#endif

#if !defined(ENABLE_SPELLCHECK)
#define ENABLE_SPELLCHECK 0
#endif

#if !defined(ENABLE_SQL_DATABASE)
#define ENABLE_SQL_DATABASE 1
#endif

#if !defined(ENABLE_STYLE_SCOPED)
#define ENABLE_STYLE_SCOPED 0
#endif

#if !defined(ENABLE_SUBPIXEL_LAYOUT)
#define ENABLE_SUBPIXEL_LAYOUT 0
#endif

#if !defined(ENABLE_SVG)
#define ENABLE_SVG 1
#endif

#if ENABLE(SVG)
#if !defined(ENABLE_SVG_FONTS)
#define ENABLE_SVG_FONTS 1
#endif
#endif

#if !defined(ENABLE_TEMPLATE_ELEMENT)
#define ENABLE_TEMPLATE_ELEMENT 0
#endif

#if !defined(ENABLE_TEXT_AUTOSIZING)
#define ENABLE_TEXT_AUTOSIZING 0
#endif

#if !defined(ENABLE_TEXT_CARET)
#define ENABLE_TEXT_CARET 1
#endif

#if !defined(ENABLE_THREADED_HTML_PARSER)
#define ENABLE_THREADED_HTML_PARSER 0
#endif

#if !defined(ENABLE_THREADED_SCROLLING)
#define ENABLE_THREADED_SCROLLING 0
#endif

#if !defined(ENABLE_TOUCH_EVENTS)
#define ENABLE_TOUCH_EVENTS 0
#endif

#if !defined(ENABLE_TOUCH_ICON_LOADING)
#define ENABLE_TOUCH_ICON_LOADING 0
#endif

#if !defined(ENABLE_VIBRATION)
#define ENABLE_VIBRATION 0
#endif

#if !defined(ENABLE_VIDEO)
#define ENABLE_VIDEO 0
#endif

#if !defined(ENABLE_VIDEO_TRACK)
#define ENABLE_VIDEO_TRACK 0
#endif

#if !defined(ENABLE_VIEWPORT)
#define ENABLE_VIEWPORT 0
#endif

#if !defined(ENABLE_VIEWSOURCE_ATTRIBUTE)
#define ENABLE_VIEWSOURCE_ATTRIBUTE 1
#endif

#if !defined(ENABLE_VIEW_MODE_CSS_MEDIA)
#define ENABLE_VIEW_MODE_CSS_MEDIA 1
#endif

#if !defined(ENABLE_WEBGL)
#define ENABLE_WEBGL 0
#endif

#if !defined(ENABLE_WEB_ARCHIVE)
#define ENABLE_WEB_ARCHIVE 0
#endif

#if !defined(ENABLE_WEB_AUDIO)
#define ENABLE_WEB_AUDIO 0
#endif

#if !defined(ENABLE_WEB_SOCKETS)
#define ENABLE_WEB_SOCKETS 1
#endif

#if !defined(ENABLE_WEB_TIMING)
#define ENABLE_WEB_TIMING 0
#endif

#if !defined(ENABLE_WORKERS)
#define ENABLE_WORKERS 0
#endif

#if !defined(ENABLE_XHR_TIMEOUT)
#define ENABLE_XHR_TIMEOUT 0
#endif

#if !defined(ENABLE_XSLT)
#define ENABLE_XSLT 1
#endif

/* Asserts, invariants for macro definitions */

#if ENABLE(SATURATED_LAYOUT_ARITHMETIC) && !ENABLE(SUBPIXEL_LAYOUT)
#error "ENABLE(SATURATED_LAYOUT_ARITHMETIC) requires ENABLE(SUBPIXEL_LAYOUT)"
#endif

#if ENABLE(SVG_FONTS) && !ENABLE(SVG)
#error "ENABLE(SVG_FONTS) requires ENABLE(SVG)"
#endif

#if ENABLE(VIDEO_TRACK) && !ENABLE(VIDEO)
#error "ENABLE(VIDEO_TRACK) requires ENABLE(VIDEO)"
#endif

#endif /* WTF_FeatureDefines_h */
