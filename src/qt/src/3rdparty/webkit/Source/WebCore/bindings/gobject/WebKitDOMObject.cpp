/*
 * Copyright (C) 2008 Luke Kenneth Casson Leighton <lkcl@lkcl.net>
 * Copyright (C) 2008 Martin Soto <soto@freedesktop.org>
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Apple Inc.
 * Copyright (C) 2009 Igalia S.L.
 */
#include "config.h"
#include "WebKitDOMObject.h"

#include "glib-object.h"
#include "WebKitDOMBinding.h"

enum {
    PROP_0,
    PROP_CORE_OBJECT
};

G_DEFINE_TYPE(WebKitDOMObject, webkit_dom_object, G_TYPE_OBJECT);

static void webkit_dom_object_init(WebKitDOMObject* object)
{
}

static void webkit_dom_object_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    switch (prop_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void webkit_dom_object_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    switch (prop_id) {
    case PROP_CORE_OBJECT:
        WEBKIT_DOM_OBJECT(object)->coreObject = g_value_get_pointer(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void webkit_dom_object_class_init(WebKitDOMObjectClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->set_property = webkit_dom_object_set_property;
    gobjectClass->get_property = webkit_dom_object_get_property;

    g_object_class_install_property(gobjectClass,
                                    PROP_CORE_OBJECT,
                                    g_param_spec_pointer("core-object",
                                                         "Core Object",
                                                         "The WebCore object the WebKitDOMObject wraps",
                                                         static_cast<GParamFlags>(G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
}

