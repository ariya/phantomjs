/*
 * Copyright (C) 2010, 2011, 2012 Igalia S.L.
 * Copyright (C) 2013 Samsung Electronics
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

#include "config.h"
#include "WebKitAccessibleHyperlink.h"

#if HAVE(ACCESSIBILITY)

#include "AXObjectCache.h"
#include "AccessibilityObject.h"
#include "NotImplemented.h"
#include "Position.h"
#include "Range.h"
#include "RenderListMarker.h"
#include "RenderObject.h"
#include "TextIterator.h"
#include "WebKitAccessibleUtil.h"
#include "WebKitAccessibleWrapperAtk.h"
#include "htmlediting.h"
#include <wtf/text/CString.h>

#include <atk/atk.h>
#include <glib.h>

using namespace WebCore;

struct _WebKitAccessibleHyperlinkPrivate {
    WebKitAccessible* hyperlinkImpl;

    // We cache these values so we can return them as const values.
    CString actionName;
    CString actionKeyBinding;
};

#define WEBKIT_ACCESSIBLE_HYPERLINK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_ACCESSIBLE_HYPERLINK, WebKitAccessibleHyperlinkPrivate))

enum {
    PROP_0,

    PROP_HYPERLINK_IMPL
};

static gpointer webkitAccessibleHyperlinkParentClass = 0;

static AccessibilityObject* core(WebKitAccessible* accessible)
{
    if (!accessible || !WEBKIT_IS_ACCESSIBLE(accessible))
        return 0;

    return webkitAccessibleGetAccessibilityObject(accessible);
}

static AccessibilityObject* core(WebKitAccessibleHyperlink* link)
{
    if (!link)
        return 0;

    return core(link->priv->hyperlinkImpl);
}

static AccessibilityObject* core(AtkHyperlink* link)
{
    if (!WEBKIT_IS_ACCESSIBLE_HYPERLINK(link))
        return 0;

    return core(WEBKIT_ACCESSIBLE_HYPERLINK(link));
}

static AccessibilityObject* core(AtkAction* action)
{
    return core(WEBKIT_ACCESSIBLE_HYPERLINK(action));
}


static gboolean webkitAccessibleHyperlinkActionDoAction(AtkAction* action, gint index)
{
    g_return_val_if_fail(WEBKIT_IS_ACCESSIBLE_HYPERLINK(action), FALSE);
    g_return_val_if_fail(WEBKIT_ACCESSIBLE_HYPERLINK(action)->priv->hyperlinkImpl, FALSE);
    g_return_val_if_fail(!index, FALSE);

    if (!ATK_IS_ACTION(WEBKIT_ACCESSIBLE_HYPERLINK(action)->priv->hyperlinkImpl))
        return FALSE;

    AccessibilityObject* coreObject = core(action);
    if (!coreObject)
        return FALSE;

    return coreObject->performDefaultAction();
}

static gint webkitAccessibleHyperlinkActionGetNActions(AtkAction* action)
{
    g_return_val_if_fail(WEBKIT_IS_ACCESSIBLE_HYPERLINK(action), 0);
    g_return_val_if_fail(WEBKIT_ACCESSIBLE_HYPERLINK(action)->priv->hyperlinkImpl, 0);

    if (!ATK_IS_ACTION(WEBKIT_ACCESSIBLE_HYPERLINK(action)->priv->hyperlinkImpl))
        return 0;

    return 1;
}

static const gchar* webkitAccessibleHyperlinkActionGetDescription(AtkAction* action, gint index)
{
    g_return_val_if_fail(WEBKIT_IS_ACCESSIBLE_HYPERLINK(action), 0);
    g_return_val_if_fail(WEBKIT_ACCESSIBLE_HYPERLINK(action)->priv->hyperlinkImpl, 0);
    g_return_val_if_fail(!index, 0);

    // TODO: Need a way to provide/localize action descriptions.
    notImplemented();
    return "";
}

static const gchar* webkitAccessibleHyperlinkActionGetKeybinding(AtkAction* action, gint index)
{
    g_return_val_if_fail(WEBKIT_IS_ACCESSIBLE_HYPERLINK(action), 0);
    g_return_val_if_fail(!index, 0);

    WebKitAccessibleHyperlinkPrivate* priv = WEBKIT_ACCESSIBLE_HYPERLINK(action)->priv;
    g_return_val_if_fail(priv->hyperlinkImpl, 0);

    if (!ATK_IS_ACTION(priv->hyperlinkImpl))
        return 0;

    AccessibilityObject* coreObject = core(action);
    if (!coreObject)
        return 0;

    priv->actionKeyBinding = coreObject->accessKey().string().utf8();
    return priv->actionKeyBinding.data();
}

static const gchar* webkitAccessibleHyperlinkActionGetName(AtkAction* action, gint index)
{
    g_return_val_if_fail(WEBKIT_IS_ACCESSIBLE_HYPERLINK(action), 0);
    g_return_val_if_fail(!index, 0);

    WebKitAccessibleHyperlinkPrivate* priv = WEBKIT_ACCESSIBLE_HYPERLINK(action)->priv;
    g_return_val_if_fail(priv->hyperlinkImpl, 0);

    if (!ATK_IS_ACTION(priv->hyperlinkImpl))
        return 0;

    AccessibilityObject* coreObject = core(action);
    if (!coreObject)
        return 0;

    priv->actionName = coreObject->actionVerb().utf8();
    return priv->actionName.data();
}

static void atkActionInterfaceInit(AtkActionIface* iface)
{
    iface->do_action = webkitAccessibleHyperlinkActionDoAction;
    iface->get_n_actions = webkitAccessibleHyperlinkActionGetNActions;
    iface->get_description = webkitAccessibleHyperlinkActionGetDescription;
    iface->get_keybinding = webkitAccessibleHyperlinkActionGetKeybinding;
    iface->get_name = webkitAccessibleHyperlinkActionGetName;
}

static gchar* webkitAccessibleHyperlinkGetURI(AtkHyperlink* link, gint index)
{
    g_return_val_if_fail(WEBKIT_IS_ACCESSIBLE_HYPERLINK(link), 0);
    // FIXME: Do NOT support more than one instance of an AtkObject
    // implementing AtkHyperlinkImpl in every instance of AtkHyperLink
    g_return_val_if_fail(!index, 0);

    AccessibilityObject* coreObject = core(link);
    if (!coreObject || coreObject->url().isNull())
        return 0;

    return g_strdup(coreObject->url().string().utf8().data());
}

static AtkObject* webkitAccessibleHyperlinkGetObject(AtkHyperlink* link, gint index)
{
    g_return_val_if_fail(WEBKIT_IS_ACCESSIBLE_HYPERLINK(link), 0);
    g_return_val_if_fail(WEBKIT_ACCESSIBLE_HYPERLINK(link)->priv->hyperlinkImpl, 0);

    // FIXME: Do NOT support more than one instance of an AtkObject
    // implementing AtkHyperlinkImpl in every instance of AtkHyperLink
    g_return_val_if_fail(!index, 0);

    return ATK_OBJECT(WEBKIT_ACCESSIBLE_HYPERLINK(link)->priv->hyperlinkImpl);
}

static gint getRangeLengthForObject(AccessibilityObject* obj, Range* range)
{
    // This is going to be the actual length in most of the cases
    int baseLength = TextIterator::rangeLength(range, true);

    // Check whether the current hyperlink belongs to a list item.
    // If so, we need to consider the length of the item's marker
    AccessibilityObject* parent = obj->parentObjectUnignored();
    if (!parent || !parent->isAccessibilityRenderObject() || !parent->isListItem())
        return baseLength;

    // Even if we don't expose list markers to Assistive
    // Technologies, we need to have a way to measure their length
    // for those cases when it's needed to take it into account
    // separately (as in getAccessibilityObjectForOffset)
    AccessibilityObject* markerObj = parent->firstChild();
    if (!markerObj)
        return baseLength;

    RenderObject* renderer = markerObj->renderer();
    if (!renderer || !renderer->isListMarker())
        return baseLength;

    RenderListMarker* marker = toRenderListMarker(renderer);
    return baseLength + marker->text().length() + marker->suffix().length();
}

static gint webkitAccessibleHyperlinkGetStartIndex(AtkHyperlink* link)
{
    g_return_val_if_fail(WEBKIT_IS_ACCESSIBLE_HYPERLINK(link), 0);

    AccessibilityObject* coreObject = core(link);
    if (!coreObject)
        return 0;

    AccessibilityObject* parentUnignored = coreObject->parentObjectUnignored();
    if (!parentUnignored)
        return 0;

    Node* node = coreObject->node();
    if (!node)
        return 0;

    Node* parentNode = parentUnignored->node();
    if (!parentNode)
        return 0;

    RefPtr<Range> range = Range::create(node->document(), firstPositionInOrBeforeNode(parentNode), firstPositionInOrBeforeNode(node));
    return getRangeLengthForObject(coreObject, range.get());
}

static gint webkitAccessibleHyperlinkGetEndIndex(AtkHyperlink* link)
{
    g_return_val_if_fail(WEBKIT_IS_ACCESSIBLE_HYPERLINK(link), 0);

    AccessibilityObject* coreObject = core(link);
    if (!coreObject)
        return 0;

    AccessibilityObject* parentUnignored = coreObject->parentObjectUnignored();
    if (!parentUnignored)
        return 0;

    Node* node = coreObject->node();
    if (!node)
        return 0;

    Node* parentNode = parentUnignored->node();
    if (!parentNode)
        return 0;

    RefPtr<Range> range = Range::create(node->document(), firstPositionInOrBeforeNode(parentNode), lastPositionInOrAfterNode(node));
    return getRangeLengthForObject(coreObject, range.get());
}

static gboolean webkitAccessibleHyperlinkIsValid(AtkHyperlink* link)
{
    g_return_val_if_fail(WEBKIT_IS_ACCESSIBLE_HYPERLINK(link), 0);
    g_return_val_if_fail(WEBKIT_ACCESSIBLE_HYPERLINK(link)->priv->hyperlinkImpl, FALSE);

    // Link is valid for the whole object's lifetime
    return TRUE;
}

static gint webkitAccessibleHyperlinkGetNAnchors(AtkHyperlink* link)
{
    // FIXME Do NOT support more than one instance of an AtkObject
    // implementing AtkHyperlinkImpl in every instance of AtkHyperLink
    g_return_val_if_fail(WEBKIT_IS_ACCESSIBLE_HYPERLINK(link), 0);
    g_return_val_if_fail(WEBKIT_ACCESSIBLE_HYPERLINK(link)->priv->hyperlinkImpl, 0);
    return 1;
}

static gboolean webkitAccessibleHyperlinkIsSelectedLink(AtkHyperlink*)
{
    // Not implemented: this function is deprecated in ATK now
    notImplemented();
    return FALSE;
}

static void webkitAccessibleHyperlinkGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* pspec)
{
    switch (propId) {
    case PROP_HYPERLINK_IMPL:
        g_value_set_object(value, WEBKIT_ACCESSIBLE_HYPERLINK(object)->priv->hyperlinkImpl);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void webkitAccessibleHyperlinkSetProperty(GObject* object, guint propId, const GValue* value, GParamSpec* pspec)
{
    WebKitAccessibleHyperlinkPrivate* priv = WEBKIT_ACCESSIBLE_HYPERLINK(object)->priv;

    switch (propId) {
    case PROP_HYPERLINK_IMPL:
        // No need to check and unref previous values of
        // priv->hyperlinkImpl as this is a CONSTRUCT ONLY property
        priv->hyperlinkImpl = WEBKIT_ACCESSIBLE(g_value_get_object(value));
        g_object_weak_ref(G_OBJECT(priv->hyperlinkImpl), (GWeakNotify)g_object_unref, object);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void webkitAccessibleHyperlinkFinalize(GObject* object)
{
    G_OBJECT_CLASS(webkitAccessibleHyperlinkParentClass)->finalize(object);
}

static void webkitAccessibleHyperlinkClassInit(AtkHyperlinkClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);

    webkitAccessibleHyperlinkParentClass = g_type_class_peek_parent(klass);

    gobjectClass->finalize = webkitAccessibleHyperlinkFinalize;
    gobjectClass->set_property = webkitAccessibleHyperlinkSetProperty;
    gobjectClass->get_property = webkitAccessibleHyperlinkGetProperty;

    klass->get_uri = webkitAccessibleHyperlinkGetURI;
    klass->get_object = webkitAccessibleHyperlinkGetObject;
    klass->get_start_index = webkitAccessibleHyperlinkGetStartIndex;
    klass->get_end_index = webkitAccessibleHyperlinkGetEndIndex;
    klass->is_valid = webkitAccessibleHyperlinkIsValid;
    klass->get_n_anchors = webkitAccessibleHyperlinkGetNAnchors;
    klass->is_selected_link = webkitAccessibleHyperlinkIsSelectedLink;

    g_object_class_install_property(gobjectClass, PROP_HYPERLINK_IMPL,
        g_param_spec_object("hyperlink-impl",
            "Hyperlink implementation",
            "The associated WebKitAccessible instance.",
            WEBKIT_TYPE_ACCESSIBLE,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS)));

    g_type_class_add_private(gobjectClass, sizeof(WebKitAccessibleHyperlinkPrivate));
}

static void webkitAccessibleHyperlinkInit(AtkHyperlink* link)
{
    WEBKIT_ACCESSIBLE_HYPERLINK(link)->priv = WEBKIT_ACCESSIBLE_HYPERLINK_GET_PRIVATE(link);
    WEBKIT_ACCESSIBLE_HYPERLINK(link)->priv->hyperlinkImpl = 0;
}

GType webkitAccessibleHyperlinkGetType()
{
    static volatile gsize typeVolatile = 0;

    if (g_once_init_enter(&typeVolatile)) {
        static const GTypeInfo tinfo = {
            sizeof(WebKitAccessibleHyperlinkClass),
            (GBaseInitFunc) 0,
            (GBaseFinalizeFunc) 0,
            (GClassInitFunc) webkitAccessibleHyperlinkClassInit,
            (GClassFinalizeFunc) 0,
            0, /* class data */
            sizeof(WebKitAccessibleHyperlink), /* instance size */
            0, /* nb preallocs */
            (GInstanceInitFunc) webkitAccessibleHyperlinkInit,
            0 /* value table */
        };

        static const GInterfaceInfo actionInfo = {
            (GInterfaceInitFunc)(GInterfaceInitFunc)atkActionInterfaceInit,
            (GInterfaceFinalizeFunc) 0, 0
        };

        GType type = g_type_register_static(ATK_TYPE_HYPERLINK, "WebKitAccessibleHyperlink", &tinfo, GTypeFlags(0));
        g_type_add_interface_static(type, ATK_TYPE_ACTION, &actionInfo);

        g_once_init_leave(&typeVolatile, type);
    }

    return typeVolatile;
}

WebKitAccessibleHyperlink* webkitAccessibleHyperlinkNew(AtkHyperlinkImpl* hyperlinkImpl)
{
    g_return_val_if_fail(ATK_IS_HYPERLINK_IMPL(hyperlinkImpl), 0);
    return WEBKIT_ACCESSIBLE_HYPERLINK(g_object_new(WEBKIT_TYPE_ACCESSIBLE_HYPERLINK, "hyperlink-impl", hyperlinkImpl, 0));
}

WebCore::AccessibilityObject* webkitAccessibleHyperlinkGetAccessibilityObject(WebKitAccessibleHyperlink* link)
{
    g_return_val_if_fail(WEBKIT_IS_ACCESSIBLE_HYPERLINK(link), 0);
    return core(link);
}

#endif // HAVE(ACCESSIBILITY)
