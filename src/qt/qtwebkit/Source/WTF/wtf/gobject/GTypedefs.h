/*
 * Copyright (C) 2010 Igalia, S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef GtkTypedefs_h
#define GtkTypedefs_h

/* Vanilla C code does not seem to be able to handle forward-declaration typedefs. */
#ifdef  __cplusplus

typedef char gchar;
typedef double gdouble;
typedef float gfloat;
typedef int gint;
typedef gint gboolean;
typedef long glong;
typedef short gshort;
typedef unsigned char guchar;
typedef unsigned int guint;
typedef unsigned long gulong;
typedef unsigned short gushort;
typedef void* gpointer;

typedef struct _GAsyncResult GAsyncResult;
typedef struct _GCancellable GCancellable;
typedef struct _GCharsetConverter GCharsetConverter;
typedef struct _GDir GDir;
typedef struct _GdkAtom* GdkAtom;
typedef struct _GdkCursor GdkCursor;
typedef struct _GdkDragContext GdkDragContext;
typedef struct _GdkEventConfigure GdkEventConfigure;
typedef struct _GdkEventExpose GdkEventExpose;
typedef struct _GdkPixbuf GdkPixbuf;
typedef struct _GError GError;
typedef struct _GFile GFile;
typedef struct _GHashTable GHashTable;
typedef struct _GInputStream GInputStream;
typedef struct _GList GList;
typedef struct _GMainContext GMainContext;
typedef struct _GMainLoop GMainLoop;
typedef struct _GPatternSpec GPatternSpec;
typedef struct _GPollableOutputStream GPollableOutputStream;
typedef struct _GSList GSList;
typedef struct _GSocketClient GSocketClient;
typedef struct _GSocketConnection GSocketConnection;
typedef struct _GSource GSource;
typedef struct _GVariant GVariant;
typedef union _GdkEvent GdkEvent;
typedef struct _GTimer GTimer;
typedef struct _GKeyFile GKeyFile;
typedef struct _GPtrArray GPtrArray;
typedef struct _GByteArray GByteArray;
typedef struct _GBytes GBytes;

#if USE(CAIRO)
typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_rectangle_int cairo_rectangle_int_t;
#endif

#if USE(CLUTTER)
typedef struct _ClutterActor ClutterActor;
typedef struct _GraphicsLayerActor GraphicsLayerActor;
#endif

#if PLATFORM(GTK)
typedef struct _GtkAction GtkAction;
typedef struct _GtkAdjustment GtkAdjustment;
typedef struct _GtkBorder GtkBorder;
typedef struct _GtkClipboard GtkClipboard;
typedef struct _GtkContainer GtkContainer;
typedef struct _GtkIconInfo GtkIconInfo;
typedef struct _GtkMenu GtkMenu;
typedef struct _GtkMenuItem GtkMenuItem;
typedef struct _GtkObject GtkObject;
typedef struct _GtkSelectionData GtkSelectionData;
typedef struct _GtkStyle GtkStyle;
typedef struct _GtkTargetList GtkTargetList;
typedef struct _GtkThemeParts GtkThemeParts;
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkWindow GtkWindow;

#ifdef GTK_API_VERSION_2
typedef struct _GdkRectangle GdkRectangle;
typedef struct _GdkDrawable GdkWindow;
#else
typedef struct _GdkWindow GdkWindow;
typedef struct _GtkStyleContext GtkStyleContext;
#endif

#endif

#endif
#endif /* GtkTypedefs_h */
