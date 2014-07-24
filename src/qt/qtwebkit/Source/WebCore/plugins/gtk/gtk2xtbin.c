/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim:expandtab:shiftwidth=2:tabstop=2: */
 
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
 
/*
 * The GtkXtBin widget allows for Xt toolkit code to be used
 * inside a GTK application.  
 */

#include "config.h"
#include "GtkVersioning.h"
#include "xembed.h"
#include "gtk2xtbin.h"
#include <gtk/gtk.h>
#ifdef GTK_API_VERSION_2
#include <gdk/gdkx.h>
#endif
#include <glib.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Xlib/Xt stuff */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Shell.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

/* uncomment this if you want debugging information about widget
   creation and destruction */
#undef DEBUG_XTBIN

#define XTBIN_MAX_EVENTS 30

static void            gtk_xtbin_class_init (GtkXtBinClass *klass);
static void            gtk_xtbin_init       (GtkXtBin      *xtbin);
static void            gtk_xtbin_realize    (GtkWidget      *widget);
static void            gtk_xtbin_unrealize    (GtkWidget      *widget);
static void            gtk_xtbin_dispose    (GObject      *object);

/* Xt aware XEmbed */
static void       xt_client_init      (XtClient * xtclient, 
                                       Visual *xtvisual, 
                                       Colormap xtcolormap, 
                                       int xtdepth);
static void       xt_client_create    (XtClient * xtclient, 
                                       Window embeder, 
                                       int height, 
                                       int width );
static void       xt_client_unrealize (XtClient* xtclient);
static void       xt_client_destroy   (XtClient* xtclient);
static void       xt_client_set_info  (Widget xtplug, 
                                       unsigned long flags);
static void       xt_client_event_handler (Widget w, 
                                           XtPointer client_data, 
                                           XEvent *event);
static void       xt_client_handle_xembed_message (Widget w, 
                                                   XtPointer client_data, 
                                                   XEvent *event);
static void       xt_client_focus_listener       (Widget w, 
                                                   XtPointer user_data, 
                                                   XEvent *event);
static void       xt_add_focus_listener( Widget w, XtPointer user_data );
static void       xt_add_focus_listener_tree ( Widget treeroot, XtPointer user_data); 
static void       xt_remove_focus_listener(Widget w, XtPointer user_data);
static void       send_xembed_message (XtClient *xtclient,
                                       long message, 
                                       long detail, 
                                       long data1, 
                                       long data2,
                                       long time);  
static int        error_handler       (Display *display, 
                                       XErrorEvent *error);
/* For error trap of XEmbed */
static void       trap_errors(void);
static int        untrap_error(void);
static int        (*old_error_handler) (Display *, XErrorEvent *);
static int        trapped_error_code = 0;

static GtkWidgetClass *parent_class = NULL;

static Display         *xtdisplay = NULL;
static String          *fallback = NULL;
static gboolean         xt_is_initialized = FALSE;
static gint             num_widgets = 0;

static GPollFD          xt_event_poll_fd;
static gint             xt_polling_timer_id = 0;
static guint            tag = 0;

static gboolean
xt_event_prepare (GSource*  source_data,
                   gint     *timeout)
{   
  int mask;

  gdk_threads_enter();
  mask = XPending(xtdisplay);
  gdk_threads_leave();

  return (gboolean)mask;
}

static gboolean
xt_event_check (GSource*  source_data)
{
  gdk_threads_enter();

  if (xt_event_poll_fd.revents & G_IO_IN) {
    int mask;
    mask = XPending(xtdisplay);
    gdk_threads_leave();
    return (gboolean)mask;
  }

  gdk_threads_leave();
  return FALSE;
}   

static gboolean
xt_event_dispatch (GSource*  source_data,
                    GSourceFunc call_back,
                    gpointer  user_data)
{
  XtAppContext ac;
  int i = 0;

  ac = XtDisplayToApplicationContext(xtdisplay);

  gdk_threads_enter();

  /* Process only real X traffic here.  We only look for data on the
   * pipe, limit it to XTBIN_MAX_EVENTS and only call
   * XtAppProcessEvent so that it will look for X events.  There's no
   * timer processing here since we already have a timer callback that
   * does it.  */
  for (i=0; i < XTBIN_MAX_EVENTS && XPending(xtdisplay); i++) {
    XtAppProcessEvent(ac, XtIMXEvent);
  }

  gdk_threads_leave();

  return TRUE;  
}

typedef void (*GSourceFuncsFinalize) (GSource* source);

static GSourceFuncs xt_event_funcs = {
  xt_event_prepare,
  xt_event_check,
  xt_event_dispatch,
  (GSourceFuncsFinalize)g_free,
  (GSourceFunc)NULL,
  (GSourceDummyMarshal)NULL
};

static gboolean
xt_event_polling_timer_callback(gpointer user_data)
{
  Display * display;
  XtAppContext ac;
  int eventsToProcess = 20;

  display = (Display *)user_data;
  ac = XtDisplayToApplicationContext(display);

  /* We need to process many Xt events here. If we just process
     one event we might starve one or more Xt consumers. On the other hand
     this could hang the whole app if Xt events come pouring in. So process
     up to 20 Xt events right now and save the rest for later. This is a hack,
     but it oughta work. We *really* should have out of process plugins.
  */
  while (eventsToProcess-- && XtAppPending(ac))
    XtAppProcessEvent(ac, XtIMAll);
  return TRUE;
}

GType
gtk_xtbin_get_type (void)
{
  static GType xtbin_type = 0;

  if (!xtbin_type) {
      static const GTypeInfo xtbin_info =
      {
        sizeof (GtkXtBinClass),
        NULL,
        NULL,

        (GClassInitFunc)gtk_xtbin_class_init,
        NULL,
        NULL,

        sizeof (GtkXtBin),
        0,
        (GInstanceInitFunc)gtk_xtbin_init,
        NULL
      };
      xtbin_type = g_type_register_static (GTK_TYPE_SOCKET,
                                           "GtkXtBin",
                                           &xtbin_info,
                                           0);
  }
  return xtbin_type;
}

static void
gtk_xtbin_class_init (GtkXtBinClass *klass)
{
  GtkWidgetClass *widget_class;
  GObjectClass   *object_class;

  parent_class = g_type_class_peek_parent (klass);

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->realize = gtk_xtbin_realize;
  widget_class->unrealize = gtk_xtbin_unrealize;

  object_class = G_OBJECT_CLASS (klass);
  object_class->dispose = gtk_xtbin_dispose;
}

static void
gtk_xtbin_init (GtkXtBin *xtbin)
{
  xtbin->xtdisplay = NULL;
  xtbin->parent_window = NULL;
  xtbin->xtwindow = 0;
  xtbin->x = 0;
  xtbin->y = 0;
}

static void
gtk_xtbin_realize (GtkWidget *widget)
{
  GtkXtBin     *xtbin;
  GtkAllocation allocation = { 0, 0, 200, 200 };
  GtkAllocation widget_allocation;

#ifdef DEBUG_XTBIN
  printf("gtk_xtbin_realize()\n");
#endif

  g_return_if_fail (GTK_IS_XTBIN (widget));

  xtbin = GTK_XTBIN (widget);

  /* caculate the allocation before realize */
  allocation.width = gdk_window_get_width(xtbin->parent_window);
  allocation.height = gdk_window_get_height(xtbin->parent_window);
  gtk_widget_size_allocate (widget, &allocation);

#ifdef DEBUG_XTBIN
  printf("initial allocation %d %d %d %d\n", x, y, w, h);
#endif

  gtk_widget_get_allocation(widget, &widget_allocation);
  xtbin->width = widget_allocation.width;
  xtbin->height = widget_allocation.height;

  /* use GtkSocket's realize */
  (*GTK_WIDGET_CLASS(parent_class)->realize)(widget);

  /* create the Xt client widget */
  xt_client_create(&(xtbin->xtclient), 
       gtk_socket_get_id(GTK_SOCKET(xtbin)), 
       xtbin->height, 
       xtbin->width);
  xtbin->xtwindow = XtWindow(xtbin->xtclient.child_widget);

  gdk_flush();

  /* now that we have created the xt client, add it to the socket. */
  gtk_socket_add_id(GTK_SOCKET(widget), xtbin->xtwindow);
}



GtkWidget*
gtk_xtbin_new (GtkWidget *parent_widget, String *f)
{
  GtkXtBin *xtbin;
  gpointer user_data;
  GdkScreen *screen;
  GdkVisual* visual;
  Colormap colormap;
  GdkWindow* parent_window = gtk_widget_get_window(parent_widget);

  assert(parent_window != NULL);
  xtbin = g_object_new (GTK_TYPE_XTBIN, NULL);

  if (!xtbin)
    return (GtkWidget*)NULL;

  if (f)
    fallback = f;

  /* Initialize the Xt toolkit */
  xtbin->parent_window = parent_window;

  screen = gtk_widget_get_screen(parent_widget);
  visual = gdk_screen_get_system_visual(screen);
  colormap = XCreateColormap(GDK_DISPLAY_XDISPLAY(gdk_screen_get_display(screen)),
                             GDK_WINDOW_XWINDOW(gdk_screen_get_root_window(screen)),
                             GDK_VISUAL_XVISUAL(visual), AllocNone);

  xt_client_init(&(xtbin->xtclient), 
                 GDK_VISUAL_XVISUAL(visual),
                 colormap,
                 gdk_visual_get_depth(visual));

  if (!xtbin->xtclient.xtdisplay) {
    /* If XtOpenDisplay failed, we can't go any further.
     *  Bail out.
     */
#ifdef DEBUG_XTBIN
    printf("gtk_xtbin_init: XtOpenDisplay() returned NULL.\n");
#endif
    g_free (xtbin);
    return (GtkWidget *)NULL;
  }

  /* If this is the first running widget, hook this display into the
     mainloop */
  if (0 == num_widgets) {
    int           cnumber;
    /*
     * hook Xt event loop into the glib event loop.
     */

    /* the assumption is that gtk_init has already been called */
    GSource* gs = g_source_new(&xt_event_funcs, sizeof(GSource));
      if (!gs) {
       return NULL;
      }
    
    g_source_set_priority(gs, GDK_PRIORITY_EVENTS);
    g_source_set_can_recurse(gs, TRUE);
    tag = g_source_attach(gs, (GMainContext*)NULL);
#ifdef VMS
    cnumber = XConnectionNumber(xtdisplay);
#else
    cnumber = ConnectionNumber(xtdisplay);
#endif
    xt_event_poll_fd.fd = cnumber;
    xt_event_poll_fd.events = G_IO_IN; 
    xt_event_poll_fd.revents = 0;    /* hmm... is this correct? */

    g_main_context_add_poll ((GMainContext*)NULL, 
                             &xt_event_poll_fd, 
                             G_PRIORITY_LOW);
    /* add a timer so that we can poll and process Xt timers */
    xt_polling_timer_id =
      g_timeout_add(25,
                      (GSourceFunc)xt_event_polling_timer_callback,
                      xtdisplay);
  }

  /* Bump up our usage count */
  num_widgets++;

  /* Build the hierachy */
  xtbin->xtdisplay = xtbin->xtclient.xtdisplay;
  gtk_widget_set_parent_window(GTK_WIDGET(xtbin), parent_window);
  gdk_window_get_user_data(xtbin->parent_window, &user_data);
  if (user_data)
    gtk_container_add(GTK_CONTAINER(user_data), GTK_WIDGET(xtbin));

  return GTK_WIDGET (xtbin);
}

void
gtk_xtbin_set_position (GtkXtBin *xtbin,
                        gint       x,
                        gint       y)
{
  xtbin->x = x;
  xtbin->y = y;

  if (gtk_widget_get_realized (GTK_WIDGET(xtbin)))
    gdk_window_move (gtk_widget_get_window(GTK_WIDGET (xtbin)), x, y);
}

void
gtk_xtbin_resize (GtkWidget *widget,
                  gint       width,
                  gint       height)
{
  Arg args[2];
  GtkXtBin *xtbin = GTK_XTBIN (widget);
  GtkAllocation allocation;

#ifdef DEBUG_XTBIN
  printf("gtk_xtbin_resize %p %d %d\n", (void *)widget, width, height);
#endif

  xtbin->height = height;
  xtbin->width  = width;

  // Avoid BadValue errors in XtSetValues
  if (height <= 0 || width <=0) {
    height = 1;
    width = 1;
  }
  XtSetArg(args[0], XtNheight, height);
  XtSetArg(args[1], XtNwidth,  width);
  XtSetValues(xtbin->xtclient.top_widget, args, 2);

  /* we need to send a size allocate so the socket knows about the
     size changes */
  allocation.x = xtbin->x;
  allocation.y = xtbin->y;
  allocation.width = xtbin->width;
  allocation.height = xtbin->height;

  gtk_widget_size_allocate(widget, &allocation);
}

static void
gtk_xtbin_unrealize (GtkWidget *object)
{
  GtkXtBin *xtbin;
  GtkWidget *widget;

#ifdef DEBUG_XTBIN
  printf("gtk_xtbin_unrealize()\n");
#endif

  /* gtk_object_destroy() will already hold a refcount on object
   */
  xtbin = GTK_XTBIN(object);
  widget = GTK_WIDGET(object);

  gtk_widget_set_visible(widget, FALSE);
  if (gtk_widget_get_realized (widget)) {
    xt_client_unrealize(&(xtbin->xtclient));
  }

  (*GTK_WIDGET_CLASS (parent_class)->unrealize)(widget);
}

static void
gtk_xtbin_dispose (GObject *object)
{
  GtkXtBin *xtbin;

#ifdef DEBUG_XTBIN
  printf("gtk_xtbin_destroy()\n");
#endif

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_XTBIN (object));

  xtbin = GTK_XTBIN (object);

  if(xtbin->xtwindow) {
    /* remove the event handler */
    xt_client_destroy(&(xtbin->xtclient));
    xtbin->xtwindow = 0;

    num_widgets--; /* reduce our usage count */

    /* If this is the last running widget, remove the Xt display
       connection from the mainloop */
    if (0 == num_widgets) {
#ifdef DEBUG_XTBIN
      printf("removing the Xt connection from the main loop\n");
#endif
      g_main_context_remove_poll((GMainContext*)NULL, &xt_event_poll_fd);
      g_source_remove(tag);

      g_source_remove(xt_polling_timer_id);
      xt_polling_timer_id = 0;
    }
  }

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

/*
* Following is the implementation of Xt XEmbedded for client side
*/

/* Initial Xt plugin */
static void
xt_client_init( XtClient * xtclient, 
                Visual *xtvisual, 
                Colormap xtcolormap,
                int xtdepth)
{
  XtAppContext  app_context;
  char         *mArgv[1];
  int           mArgc = 0;

  /*
   * Initialize Xt stuff
   */
  xtclient->top_widget = NULL;
  xtclient->child_widget = NULL;
  xtclient->xtdisplay  = NULL;
  xtclient->xtvisual   = NULL;
  xtclient->xtcolormap = 0;
  xtclient->xtdepth = 0;

  if (!xt_is_initialized) {
#ifdef DEBUG_XTBIN
    printf("starting up Xt stuff\n");
#endif
    XtToolkitInitialize();
    app_context = XtCreateApplicationContext();
    if (fallback)
      XtAppSetFallbackResources(app_context, fallback);

    xtdisplay = XtOpenDisplay(app_context, gdk_get_display(), NULL, 
                            "Wrapper", NULL, 0, &mArgc, mArgv);
    if (xtdisplay)
      xt_is_initialized = TRUE;
  }
  xtclient->xtdisplay  = xtdisplay;
  xtclient->xtvisual   = xtvisual;
  xtclient->xtcolormap = xtcolormap;
  xtclient->xtdepth    = xtdepth;
}

/* Create the Xt client widgets
*  */
static void
xt_client_create ( XtClient* xtclient , 
                   Window embedderid, 
                   int height, 
                   int width ) 
{
  int           n;
  Arg           args[6];
  Widget        child_widget;
  Widget        top_widget;

#ifdef DEBUG_XTBIN
  printf("xt_client_create() \n");
#endif
  top_widget = XtAppCreateShell("drawingArea", "Wrapper", 
                                applicationShellWidgetClass, 
                                xtclient->xtdisplay, 
                                NULL, 0);
  xtclient->top_widget = top_widget;

  /* set size of Xt window */
  n = 0;
  XtSetArg(args[n], XtNheight,   height);n++;
  XtSetArg(args[n], XtNwidth,    width);n++;
  XtSetValues(top_widget, args, n);

  child_widget = XtVaCreateWidget("form", 
                                  compositeWidgetClass, 
                                  top_widget, NULL);

  n = 0;
  XtSetArg(args[n], XtNheight,   height);n++;
  XtSetArg(args[n], XtNwidth,    width);n++;
  XtSetArg(args[n], XtNvisual,   xtclient->xtvisual ); n++;
  XtSetArg(args[n], XtNdepth,    xtclient->xtdepth ); n++;
  XtSetArg(args[n], XtNcolormap, xtclient->xtcolormap ); n++;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  XtSetValues(child_widget, args, n);

  XSync(xtclient->xtdisplay, FALSE);
  xtclient->oldwindow = top_widget->core.window;
  top_widget->core.window = embedderid;

  /* this little trick seems to finish initializing the widget */
#if XlibSpecificationRelease >= 6
  XtRegisterDrawable(xtclient->xtdisplay, 
                     embedderid,
                     top_widget);
#else
  _XtRegisterWindow( embedderid,
                     top_widget);
#endif
  XtRealizeWidget(child_widget);

  /* listen to all Xt events */
  XSelectInput(xtclient->xtdisplay, 
               XtWindow(top_widget), 
               0x0FFFFF);
  xt_client_set_info (child_widget, 0);

  XtManageChild(child_widget);
  xtclient->child_widget = child_widget;

  /* set the event handler */
  XtAddEventHandler(child_widget,
                    0x0FFFFF & ~ResizeRedirectMask,
                    TRUE, 
                    (XtEventHandler)xt_client_event_handler, xtclient);
  XtAddEventHandler(child_widget, 
                    SubstructureNotifyMask | ButtonReleaseMask, 
                    TRUE, 
                    (XtEventHandler)xt_client_focus_listener, 
                    xtclient);
  XSync(xtclient->xtdisplay, FALSE);
}

static void
xt_client_unrealize ( XtClient* xtclient )
{
#if XlibSpecificationRelease >= 6
  XtUnregisterDrawable(xtclient->xtdisplay,
                       xtclient->top_widget->core.window);
#else
  _XtUnregisterWindow(xtclient->top_widget->core.window,
                      xtclient->top_widget);
#endif

  /* flush the queue before we returning origin top_widget->core.window
     or we can get X error since the window is gone */
  XSync(xtclient->xtdisplay, False);

  xtclient->top_widget->core.window = xtclient->oldwindow;
  XtUnrealizeWidget(xtclient->top_widget);
}

static void            
xt_client_destroy   (XtClient* xtclient)
{
  if(xtclient->top_widget) {
    XtRemoveEventHandler(xtclient->child_widget, 0x0FFFFF, TRUE, 
                         (XtEventHandler)xt_client_event_handler, xtclient);
    XtDestroyWidget(xtclient->top_widget);
    xtclient->top_widget = NULL;
  }
}

static void         
xt_client_set_info (Widget xtplug, unsigned long flags)
{
  unsigned long buffer[2];

  Atom infoAtom = XInternAtom(XtDisplay(xtplug), "_XEMBED_INFO", False); 

  buffer[1] = 0;                /* Protocol version */
  buffer[1] = flags;

  XChangeProperty (XtDisplay(xtplug), XtWindow(xtplug),
                   infoAtom, infoAtom, 32,
                   PropModeReplace,
                   (unsigned char *)buffer, 2);
}

static void
xt_client_handle_xembed_message(Widget w, XtPointer client_data, XEvent *event)
{
  XtClient *xtplug = (XtClient*)client_data;
  switch (event->xclient.data.l[1])
  {
  case XEMBED_EMBEDDED_NOTIFY:
    break;
  case XEMBED_WINDOW_ACTIVATE:
#ifdef DEBUG_XTBIN
    printf("Xt client get XEMBED_WINDOW_ACTIVATE\n");
#endif
    break;
  case XEMBED_WINDOW_DEACTIVATE:
#ifdef DEBUG_XTBIN
    printf("Xt client get XEMBED_WINDOW_DEACTIVATE\n");
#endif
    break;
  case XEMBED_MODALITY_ON:
#ifdef DEBUG_XTBIN
    printf("Xt client get XEMBED_MODALITY_ON\n");
#endif
    break;
  case XEMBED_MODALITY_OFF:
#ifdef DEBUG_XTBIN
    printf("Xt client get XEMBED_MODALITY_OFF\n");
#endif
    break;
  case XEMBED_FOCUS_IN:
  case XEMBED_FOCUS_OUT:
    {
      XEvent xevent;
      memset(&xevent, 0, sizeof(xevent));

      if(event->xclient.data.l[1] == XEMBED_FOCUS_IN) {
#ifdef DEBUG_XTBIN
        printf("XTEMBED got focus in\n");
#endif
        xevent.xfocus.type = FocusIn;
      }
      else {
#ifdef DEBUG_XTBIN
        printf("XTEMBED got focus out\n");
#endif
        xevent.xfocus.type = FocusOut;
      }

      xevent.xfocus.window = XtWindow(xtplug->child_widget);
      xevent.xfocus.display = XtDisplay(xtplug->child_widget);
      XSendEvent(XtDisplay(xtplug->child_widget), 
                 xevent.xfocus.window,
                 False, NoEventMask,
                 &xevent );
      XSync( XtDisplay(xtplug->child_widget), False);
    }
    break;
  default:
    break;
  } /* End of XEmbed Message */
}

static void         
xt_client_event_handler( Widget w, XtPointer client_data, XEvent *event)
{
  XtClient *xtplug = (XtClient*)client_data;
  
  switch(event->type)
    {
    case ClientMessage:
      /* Handle xembed message */
      if (event->xclient.message_type==
                 XInternAtom (XtDisplay(xtplug->child_widget),
                              "_XEMBED", False)) {
        xt_client_handle_xembed_message(w, client_data, event);
      }
      break;
    case ReparentNotify:
      break;
    case MappingNotify:
      xt_client_set_info (w, XEMBED_MAPPED);
      break;
    case UnmapNotify:
      xt_client_set_info (w, 0);
      break;
    case FocusIn:
      send_xembed_message ( xtplug,
                            XEMBED_REQUEST_FOCUS, 0, 0, 0, 0);
      break;
    case FocusOut:
      break;
    case KeyPress:
#ifdef DEBUG_XTBIN
      printf("Key Press Got!\n");
#endif
      break;
    default:
      break;
    } /* End of switch(event->type) */
}

static void
send_xembed_message (XtClient  *xtclient,
                     long      message,
                     long      detail, 
                     long      data1,  
                     long      data2,  
                     long      time)   
{
  XEvent xevent; 
  Window w=XtWindow(xtclient->top_widget);
  Display* dpy=xtclient->xtdisplay;
  int errorcode;

  memset(&xevent,0,sizeof(xevent));
  xevent.xclient.window = w;
  xevent.xclient.type = ClientMessage;
  xevent.xclient.message_type = XInternAtom(dpy,"_XEMBED",False);
  xevent.xclient.format = 32;
  xevent.xclient.data.l[0] = time; 
  xevent.xclient.data.l[1] = message;
  xevent.xclient.data.l[2] = detail; 
  xevent.xclient.data.l[3] = data1;
  xevent.xclient.data.l[4] = data2;

  trap_errors ();
  XSendEvent (dpy, w, False, NoEventMask, &xevent);
  XSync (dpy,False);

  if((errorcode = untrap_error())) {
#ifdef DEBUG_XTBIN
    printf("send_xembed_message error(%d)!!!\n",errorcode);
#endif
  }
}

static int             
error_handler(Display *display, XErrorEvent *error)
{
  trapped_error_code = error->error_code;
  return 0;
}

static void          
trap_errors(void)
{
  trapped_error_code =0;
  old_error_handler = XSetErrorHandler(error_handler);
}

static int         
untrap_error(void)
{
  XSetErrorHandler(old_error_handler);
  if(trapped_error_code) {
#ifdef DEBUG_XTBIN
    printf("Get X Window Error = %d\n", trapped_error_code);
#endif
  }
  return trapped_error_code;
}

static void         
xt_client_focus_listener( Widget w, XtPointer user_data, XEvent *event)
{
  Display *dpy = XtDisplay(w);
  XtClient *xtclient = user_data;
  Window win = XtWindow(w);

  switch(event->type)
    {
    case CreateNotify:
      if(event->xcreatewindow.parent == win) {
        Widget child=XtWindowToWidget( dpy, event->xcreatewindow.window);
        if (child)
          xt_add_focus_listener_tree(child, user_data);
      }
      break;
    case DestroyNotify:
      xt_remove_focus_listener( w, user_data);
      break;
    case ReparentNotify:
      if(event->xreparent.parent == win) {
        /* I am the new parent */
        Widget child=XtWindowToWidget(dpy, event->xreparent.window);
        if (child)
          xt_add_focus_listener_tree( child, user_data);
      }
      else if(event->xreparent.window == win) {
        /* I am the new child */
      }
      else {
        /* I am the old parent */
      }
      break;
    case ButtonRelease:
#if 0
      XSetInputFocus(dpy, XtWindow(xtclient->child_widget), RevertToParent, event->xbutton.time);
#endif
      send_xembed_message ( xtclient,
                            XEMBED_REQUEST_FOCUS, 0, 0, 0, 0);
      break;
    default:
      break;
    } /* End of switch(event->type) */
}

static void
xt_add_focus_listener( Widget w, XtPointer user_data)
{
  XWindowAttributes attr;
  long eventmask;
  XtClient *xtclient = user_data;

  trap_errors ();
  XGetWindowAttributes(XtDisplay(w), XtWindow(w), &attr);
  eventmask = attr.your_event_mask | SubstructureNotifyMask | ButtonReleaseMask;
  XSelectInput(XtDisplay(w),
               XtWindow(w), 
               eventmask);

  XtAddEventHandler(w, 
                    SubstructureNotifyMask | ButtonReleaseMask, 
                    TRUE, 
                    (XtEventHandler)xt_client_focus_listener, 
                    xtclient);
  untrap_error();
}

static void
xt_remove_focus_listener(Widget w, XtPointer user_data)
{
  trap_errors ();
  XtRemoveEventHandler(w, SubstructureNotifyMask | ButtonReleaseMask, TRUE, 
                      (XtEventHandler)xt_client_focus_listener, user_data);

  untrap_error();
}

static void
xt_add_focus_listener_tree ( Widget treeroot, XtPointer user_data) 
{
  Window win = XtWindow(treeroot);
  Window *children;
  Window root, parent;
  Display *dpy = XtDisplay(treeroot);
  unsigned int i, nchildren;

  /* ensure we don't add more than once */
  xt_remove_focus_listener( treeroot, user_data);
  xt_add_focus_listener( treeroot, user_data);
  trap_errors();
  if(!XQueryTree(dpy, win, &root, &parent, &children, &nchildren)) {
    untrap_error();
    return;
  }

  if(untrap_error()) 
    return;

  for(i=0; i<nchildren; ++i) {
    Widget child = XtWindowToWidget(dpy, children[i]);
    if (child) 
      xt_add_focus_listener_tree( child, user_data);
  }
  XFree((void*)children);

  return;
}
