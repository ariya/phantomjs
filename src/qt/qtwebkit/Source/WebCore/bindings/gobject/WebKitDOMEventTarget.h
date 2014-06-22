/*
 *  Copyright (C) 2010 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef WebKitDOMEventTarget_h
#define WebKitDOMEventTarget_h

#include <glib-object.h>
#include <webkitdom/webkitdomdefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_DOM_EVENT_TARGET            (webkit_dom_event_target_get_type ())
#define WEBKIT_DOM_EVENT_TARGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBKIT_TYPE_DOM_EVENT_TARGET, WebKitDOMEventTarget))
#define WEBKIT_DOM_EVENT_TARGET_CLASS(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj), WEBKIT_TYPE_DOM_EVENT_TARGET, WebKitDOMEventTargetIface))
#define WEBKIT_DOM_IS_EVENT_TARGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBKIT_TYPE_DOM_EVENT_TARGET))
#define WEBKIT_DOM_EVENT_TARGET_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), WEBKIT_TYPE_DOM_EVENT_TARGET, WebKitDOMEventTargetIface))

typedef struct _WebKitDOMEventTargetIface WebKitDOMEventTargetIface;

struct _WebKitDOMEventTargetIface {
    GTypeInterface gIface;

    /* virtual table */
    void          (* dispatch_event)(WebKitDOMEventTarget *target,
                                     WebKitDOMEvent       *event,
                                     GError              **error);

    gboolean      (* add_event_listener)(WebKitDOMEventTarget *target,
                                         const char           *event_name,
                                         GCallback             handler,
                                         gboolean              bubble,
                                         gpointer              user_data);
    gboolean      (* remove_event_listener)(WebKitDOMEventTarget *target,
                                            const char           *event_name,
                                            GCallback             handler,
                                            gboolean              bubble);
};


WEBKIT_API GType     webkit_dom_event_target_get_type(void) G_GNUC_CONST;

WEBKIT_API void      webkit_dom_event_target_dispatch_event(WebKitDOMEventTarget *target,
                                                            WebKitDOMEvent       *event,
                                                            GError              **error);

/**
 * webkit_dom_event_target_add_event_listener:
 * @target: A #WebKitDOMEventTarget
 * @event_name: A #gchar
 * @handler: (scope async): A #GCallback
 * @bubble: A #gboolean
 * @user_data: A #gpointer
 *
 */
WEBKIT_API gboolean  webkit_dom_event_target_add_event_listener(WebKitDOMEventTarget *target,
                                                                const char           *event_name,
                                                                GCallback             handler,
                                                                gboolean              bubble,
                                                                gpointer              user_data);

/**
 * webkit_dom_event_target_remove_event_listener:
 * @target: A #WebKitDOMEventTarget
 * @event_name: A #gchar
 * @handler: (scope call): A #GCallback
 * @bubble: A #gboolean
 *
 */
WEBKIT_API gboolean  webkit_dom_event_target_remove_event_listener(WebKitDOMEventTarget *target,
                                                                   const char           *event_name,
                                                                   GCallback             handler,
                                                                   gboolean              bubble);

G_END_DECLS

#endif /* WebKitDOMEventTarget_h */
