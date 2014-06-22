/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: set expandtab shiftwidth=2 tabstop=2: */
 
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Gtk2XtBin Widget Implementation.
 *
 * The Initial Developer of the Original Code is
 * Sun Microsystems, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */   

#ifndef __GTK_XTBIN_H__
#define __GTK_XTBIN_H__

#include <gtk/gtk.h>
#ifndef GTK_API_VERSION_2
#include <gtk/gtkx.h>
#endif
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#ifdef MOZILLA_CLIENT
#include "nscore.h"
#ifdef _IMPL_GTKXTBIN_API
#define GTKXTBIN_API(type) NS_EXPORT_(type)
#else
#define GTKXTBIN_API(type) NS_IMPORT_(type)
#endif
#else
#define GTKXTBIN_API(type) type
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _GtkXtBin GtkXtBin;
typedef struct _GtkXtBinClass GtkXtBinClass;

#define GTK_TYPE_XTBIN                  (gtk_xtbin_get_type ())
#define GTK_XTBIN(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                         GTK_TYPE_XTBIN, GtkXtBin))
#define GTK_XTBIN_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                         GTK_TYPE_XTBIN, GtkXtBinClass))
#define GTK_IS_XTBIN(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                         GTK_TYPE_XTBIN))
#define GTK_IS_XTBIN_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                         GTK_TYPE_XTBIN))
typedef struct _XtClient XtClient;

struct _XtClient {
  Display	*xtdisplay;
  Widget	top_widget;    /* The toplevel widget */
  Widget	child_widget;  /* The embedded widget */
  Visual	*xtvisual;
  int		xtdepth;
  Colormap	xtcolormap;
  Window	oldwindow;
};

struct _GtkXtBin
{
  GtkSocket      gsocket;
  GdkWindow     *parent_window;
  Display       *xtdisplay;        /* Xt Toolkit Display */

  Window         xtwindow;         /* Xt Toolkit XWindow */
  gint           x, y;
  gint           width, height;
  XtClient	 xtclient;         /* Xt Client for XEmbed */
};
  
struct _GtkXtBinClass
{
  GtkSocketClass widget_class;
};

GTKXTBIN_API(GType)       gtk_xtbin_get_type (void);
GTKXTBIN_API(GtkWidget *) gtk_xtbin_new (GtkWidget *parent_widget, String *f);
GTKXTBIN_API(void)        gtk_xtbin_set_position (GtkXtBin *xtbin,
                                                  gint       x,
                                                  gint       y);
GTKXTBIN_API(void)       gtk_xtbin_resize (GtkWidget *widget,
                                           gint       width,
                                           gint       height);

typedef struct _XtTMRec {
    XtTranslations  translations;       /* private to Translation Manager    */
    XtBoundActions  proc_table;         /* procedure bindings for actions    */
    struct _XtStateRec *current_state;  /* Translation Manager state ptr     */
    unsigned long   lastEventTime;
} XtTMRec, *XtTM;   

typedef struct _CorePart {
    Widget          self;               /* pointer to widget itself          */
    WidgetClass     widget_class;       /* pointer to Widget's ClassRec      */
    Widget          parent;             /* parent widget                     */
    XrmName         xrm_name;           /* widget resource name quarkified   */
    Boolean         being_destroyed;    /* marked for destroy                */
    XtCallbackList  destroy_callbacks;  /* who to call when widget destroyed */
    XtPointer       constraints;        /* constraint record                 */
    Position        x, y;               /* window position                   */
    Dimension       width, height;      /* window dimensions                 */
    Dimension       border_width;       /* window border width               */
    Boolean         managed;            /* is widget geometry managed?       */
    Boolean         sensitive;          /* is widget sensitive to user events*/
    Boolean         ancestor_sensitive; /* are all ancestors sensitive?      */
    XtEventTable    event_table;        /* private to event dispatcher       */
    XtTMRec         tm;                 /* translation management            */
    XtTranslations  accelerators;       /* accelerator translations          */
    Pixel           border_pixel;       /* window border pixel               */
    Pixmap          border_pixmap;      /* window border pixmap or NULL      */
    WidgetList      popup_list;         /* list of popups                    */
    Cardinal        num_popups;         /* how many popups                   */
    String          name;               /* widget resource name              */
    Screen          *screen;            /* window's screen                   */
    Colormap        colormap;           /* colormap                          */
    Window          window;             /* window ID                         */
    Cardinal        depth;              /* number of planes in window        */
    Pixel           background_pixel;   /* window background pixel           */
    Pixmap          background_pixmap;  /* window background pixmap or NULL  */
    Boolean         visible;            /* is window mapped and not occluded?*/
    Boolean         mapped_when_managed;/* map window if it's managed?       */
} CorePart;

typedef struct _WidgetRec {
    CorePart    core;
 } WidgetRec, CoreRec;   

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __GTK_XTBIN_H__ */
