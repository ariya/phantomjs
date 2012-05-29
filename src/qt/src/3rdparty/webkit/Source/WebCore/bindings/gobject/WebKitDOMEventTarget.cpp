/*
 * Copyright (C) 2010 Igalia S.L.
 *
 * This file is derived by hand from an automatically generated file.
 * Keeping it up-to-date could potentially be done by adding
 * a make_names.pl generator, or by writing a separate
 * generater which takes JSHTMLElementWrapperFactory.h as input.
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

#include "config.h"
#include "WebKitDOMEventTarget.h"

#include "EventTarget.h"
#include "WebKitDOMEvent.h"

typedef WebKitDOMEventTargetIface WebKitDOMEventTargetInterface;
#if GLIB_CHECK_VERSION(2, 24, 0)
G_DEFINE_INTERFACE(WebKitDOMEventTarget, webkit_dom_event_target, G_TYPE_OBJECT)
#else
static void webkit_dom_event_target_default_init(WebKitDOMEventTargetIface*);

GType webkit_dom_event_target_get_type(void)
{
    static volatile gsize typeIdVolatile = 0;

    if (g_once_init_enter(&typeIdVolatile)) {
        GType typeId = g_type_register_static_simple(G_TYPE_INTERFACE,
                                                     g_intern_static_string("WebKitDOMEventTarget"),
                                                     sizeof(WebKitDOMEventTargetInterface),
                                                     (GClassInitFunc)webkit_dom_event_target_default_init,
                                                     0,
                                                     static_cast<GInstanceInitFunc>(0),
                                                     static_cast<GTypeFlags>(0));
        g_type_interface_add_prerequisite(typeId, G_TYPE_OBJECT);
        g_once_init_leave(&typeIdVolatile, typeId);
    }

    return typeIdVolatile;
}
#endif

static void webkit_dom_event_target_default_init(WebKitDOMEventTargetIface*)
{
}

void webkit_dom_event_target_dispatch_event(WebKitDOMEventTarget* target, WebKitDOMEvent* event, GError** error)
{
    g_return_if_fail(WEBKIT_DOM_IS_EVENT_TARGET(target));
    g_return_if_fail(WEBKIT_DOM_IS_EVENT(event));

    WebKitDOMEventTargetIface* iface = WEBKIT_DOM_EVENT_TARGET_GET_IFACE(target);

    if (iface->dispatch_event)
        iface->dispatch_event(target, event, error);
}

gboolean webkit_dom_event_target_add_event_listener(WebKitDOMEventTarget* target, const char* eventName, GCallback handler, gboolean bubble, gpointer userData)
{

    g_return_val_if_fail(WEBKIT_DOM_IS_EVENT_TARGET(target), FALSE);
    g_return_val_if_fail(eventName, FALSE);

    WebKitDOMEventTargetIface* iface = WEBKIT_DOM_EVENT_TARGET_GET_IFACE(target);

    if (iface->add_event_listener)
        return iface->add_event_listener(target, eventName, handler, bubble, userData);

    return FALSE;
}

gboolean webkit_dom_event_target_remove_event_listener(WebKitDOMEventTarget* target, const char* eventName, GCallback handler, gboolean bubble)
{
    g_return_val_if_fail(WEBKIT_DOM_IS_EVENT_TARGET(target), FALSE);
    g_return_val_if_fail(eventName, FALSE);

    WebKitDOMEventTargetIface* iface = WEBKIT_DOM_EVENT_TARGET_GET_IFACE(target);

    if (iface->remove_event_listener)
        return iface->remove_event_listener(target, eventName, handler, bubble);

    return FALSE;
}

namespace WebKit {

WebCore::EventTarget* core(WebKitDOMEventTarget* request)
{
    g_return_val_if_fail(request, 0);

    WebCore::EventTarget* coreObject = static_cast<WebCore::EventTarget*>(WEBKIT_DOM_OBJECT(request)->coreObject);
    g_return_val_if_fail(coreObject, 0);

    return coreObject;
}

} // namespace WebKit

