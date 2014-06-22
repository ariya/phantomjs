/*
 * Copyright (C) 2009 Collabora Ltd.
 * Copyright (C) 2009 Igalia S.L.
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
#include "webkithittestresult.h"

#include "Frame.h"
#include "FrameView.h"
#include "HitTestResult.h"
#include "KURL.h"
#include "WebKitDOMNodePrivate.h"
#include "webkitenumtypes.h"
#include "webkitglobals.h"
#include "webkitglobalsprivate.h"
#include <glib/gi18n-lib.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

/**
 * SECTION:webkithittestresult
 * @short_description: The target of a mouse event
 *
 * This class holds context information about the coordinates
 * specified by a GDK event.
 */

G_DEFINE_TYPE(WebKitHitTestResult, webkit_hit_test_result, G_TYPE_OBJECT)

struct _WebKitHitTestResultPrivate {
    guint context;
    char* linkURI;
    char* imageURI;
    char* mediaURI;
    GRefPtr<WebKitDOMNode> innerNode;
    WebCore::IntPoint position;
};

enum {
    PROP_0,

    PROP_CONTEXT,
    PROP_LINK_URI,
    PROP_IMAGE_URI,
    PROP_MEDIA_URI,
    PROP_INNER_NODE,
    PROP_X,
    PROP_Y
};

static void webkit_hit_test_result_finalize(GObject* object)
{
    WebKitHitTestResult* web_hit_test_result = WEBKIT_HIT_TEST_RESULT(object);
    WebKitHitTestResultPrivate* priv = web_hit_test_result->priv;

    g_free(priv->linkURI);
    g_free(priv->imageURI);
    g_free(priv->mediaURI);

    G_OBJECT_CLASS(webkit_hit_test_result_parent_class)->finalize(object);
}

static void webkit_hit_test_result_dispose(GObject* object)
{
    WEBKIT_HIT_TEST_RESULT(object)->priv->~WebKitHitTestResultPrivate();

    G_OBJECT_CLASS(webkit_hit_test_result_parent_class)->dispose(object);
}

static void webkit_hit_test_result_get_property(GObject* object, guint propertyID, GValue* value, GParamSpec* pspec)
{
    WebKitHitTestResult* web_hit_test_result = WEBKIT_HIT_TEST_RESULT(object);
    WebKitHitTestResultPrivate* priv = web_hit_test_result->priv;

    switch(propertyID) {
    case PROP_CONTEXT:
        g_value_set_flags(value, priv->context);
        break;
    case PROP_LINK_URI:
        g_value_set_string(value, priv->linkURI);
        break;
    case PROP_IMAGE_URI:
        g_value_set_string(value, priv->imageURI);
        break;
    case PROP_MEDIA_URI:
        g_value_set_string(value, priv->mediaURI);
        break;
    case PROP_INNER_NODE:
        g_value_set_object(value, priv->innerNode.get());
        break;
    case PROP_X:
        g_value_set_int(value, priv->position.x());
        break;
    case PROP_Y:
        g_value_set_int(value, priv->position.y());
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyID, pspec);
    }
}

static void webkit_hit_test_result_set_property(GObject* object, guint propertyID, const GValue* value, GParamSpec* pspec)
{
    WebKitHitTestResult* web_hit_test_result = WEBKIT_HIT_TEST_RESULT(object);
    WebKitHitTestResultPrivate* priv = web_hit_test_result->priv;

    switch(propertyID) {
    case PROP_CONTEXT:
        priv->context = g_value_get_flags(value);
        break;
    case PROP_LINK_URI:
        g_free (priv->linkURI);
        priv->linkURI = g_value_dup_string(value);
        break;
    case PROP_IMAGE_URI:
        g_free (priv->imageURI);
        priv->imageURI = g_value_dup_string(value);
        break;
    case PROP_MEDIA_URI:
        g_free (priv->mediaURI);
        priv->mediaURI = g_value_dup_string(value);
        break;
    case PROP_INNER_NODE:
        priv->innerNode = static_cast<WebKitDOMNode*>(g_value_get_object(value));
        break;
    case PROP_X:
        priv->position.setX(g_value_get_int(value));
        break;
    case PROP_Y:
        priv->position.setY(g_value_get_int(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyID, pspec);
    }
}

static void webkit_hit_test_result_class_init(WebKitHitTestResultClass* webHitTestResultClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(webHitTestResultClass);

    objectClass->finalize = webkit_hit_test_result_finalize;
    objectClass->dispose = webkit_hit_test_result_dispose;
    objectClass->get_property = webkit_hit_test_result_get_property;
    objectClass->set_property = webkit_hit_test_result_set_property;

    webkitInit();

    /**
     * WebKitHitTestResult:context:
     *
     * Flags indicating the kind of target that received the event.
     *
     * Since: 1.1.15
     */
    g_object_class_install_property(objectClass, PROP_CONTEXT,
                                    g_param_spec_flags("context",
                                                       _("Context"),
                                                       _("Flags indicating the kind of target that received the event."),
                                                       WEBKIT_TYPE_HIT_TEST_RESULT_CONTEXT,
                                                       WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT,
                                                       static_cast<GParamFlags>((WEBKIT_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY))));

    /**
     * WebKitHitTestResult:link-uri:
     *
     * The URI to which the target that received the event points, if any.
     *
     * Since: 1.1.15
     */
    g_object_class_install_property(objectClass, PROP_LINK_URI,
                                    g_param_spec_string("link-uri",
                                                        _("Link URI"),
                                                        _("The URI to which the target that received the event points, if any."),
                                                        NULL,
                                                        static_cast<GParamFlags>(WEBKIT_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY)));

    /**
     * WebKitHitTestResult:image-uri:
     *
     * The URI of the image that is part of the target that received the event, if any.
     *
     * Since: 1.1.15
     */
    g_object_class_install_property(objectClass, PROP_IMAGE_URI,
                                    g_param_spec_string("image-uri",
                                                        _("Image URI"),
                                                        _("The URI of the image that is part of the target that received the event, if any."),
                                                        NULL,
                                                        static_cast<GParamFlags>(WEBKIT_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY)));

    /**
     * WebKitHitTestResult:media-uri:
     *
     * The URI of the media that is part of the target that received the event, if any.
     *
     * Since: 1.1.15
     */
    g_object_class_install_property(objectClass, PROP_MEDIA_URI,
                                    g_param_spec_string("media-uri",
                                                        _("Media URI"),
                                                        _("The URI of the media that is part of the target that received the event, if any."),
                                                        NULL,
                                                        static_cast<GParamFlags>(WEBKIT_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY)));

    /**
     * WebKitHitTestResult:inner-node:
     *
     * The DOM node at the coordinates where the hit test
     * happened. Keep in mind that the node might not be
     * representative of the information given in the context
     * property, since WebKit uses a series of heuristics to figure
     * out that information. One common example is inner-node having
     * the text node inside the anchor (&lt;a&gt;) tag; WebKit knows the
     * whole context and will put WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK
     * in the 'context' property, but the user might be confused by
     * the lack of any link tag in 'inner-node'.
     *
     * Since: 1.3.2
     */
    g_object_class_install_property(objectClass, PROP_INNER_NODE,
                                    g_param_spec_object("inner-node",
                                                        _("Inner node"),
                                                        _("The inner DOM node associated with the hit test result."),
                                                        WEBKIT_TYPE_DOM_NODE,
                                                        static_cast<GParamFlags>(WEBKIT_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));

    /**
     * WebKitHitTestResult:x:
     *
     * The x coordinate of the event relative to the view's window.
     *
     * Since: 1.10
     */
    g_object_class_install_property(objectClass, PROP_X,
                                    g_param_spec_int("x",
                                                     _("X coordinate"),
                                                     _("The x coordinate of the event relative to the view's window."),
                                                     G_MININT, G_MAXINT, 0,
                                                     static_cast<GParamFlags>(WEBKIT_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));

    /**
     * WebKitHitTestResult:y:
     *
     * The x coordinate of the event relative to the view's window.
     *
     * Since: 1.10
     */
    g_object_class_install_property(objectClass, PROP_Y,
                                    g_param_spec_int("y",
                                                     _("Y coordinate"),
                                                     _("The y coordinate of the event relative to the view's window."),
                                                     G_MININT, G_MAXINT, 0,
                                                     static_cast<GParamFlags>(WEBKIT_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));

    g_type_class_add_private(webHitTestResultClass, sizeof(WebKitHitTestResultPrivate));
}

static void webkit_hit_test_result_init(WebKitHitTestResult* web_hit_test_result)
{
    web_hit_test_result->priv = G_TYPE_INSTANCE_GET_PRIVATE(web_hit_test_result, WEBKIT_TYPE_HIT_TEST_RESULT, WebKitHitTestResultPrivate);
    new (web_hit_test_result->priv) WebKitHitTestResultPrivate();
}

namespace WebKit {

WebKitHitTestResult* kit(const WebCore::HitTestResult& result)
{
    guint context = WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT;
    GOwnPtr<char> linkURI(0);
    GOwnPtr<char> imageURI(0);
    GOwnPtr<char> mediaURI(0);
    WebKitDOMNode* node = 0;
    WebCore::Frame* innerNodeFrame;
    WebCore::IntPoint point;

    if (!result.absoluteLinkURL().isEmpty()) {
        context |= WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK;
        linkURI.set(g_strdup(result.absoluteLinkURL().string().utf8().data()));
    }

    if (!result.absoluteImageURL().isEmpty()) {
        context |= WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE;
        imageURI.set(g_strdup(result.absoluteImageURL().string().utf8().data()));
    }

    if (!result.absoluteMediaURL().isEmpty()) {
        context |= WEBKIT_HIT_TEST_RESULT_CONTEXT_MEDIA;
        mediaURI.set(g_strdup(result.absoluteMediaURL().string().utf8().data()));
    }

    if (result.isSelected())
        context |= WEBKIT_HIT_TEST_RESULT_CONTEXT_SELECTION;

    if (result.isContentEditable())
        context |= WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE;

    if (result.innerNonSharedNode())
        node = kit(result.innerNonSharedNode());

    innerNodeFrame = result.innerNodeFrame();
    if (innerNodeFrame && innerNodeFrame->view()) {
        // Convert document coords to widget coords.
        point = innerNodeFrame->view()->contentsToWindow(result.roundedPointInInnerNodeFrame());
    } else {
        // FIXME: Main frame coords is not the same as window coords,
        // but we do not have pointer to  mainframe view here.
        point = result.roundedPointInMainFrame();
    }

    return WEBKIT_HIT_TEST_RESULT(g_object_new(WEBKIT_TYPE_HIT_TEST_RESULT,
                                               "link-uri", linkURI.get(),
                                               "image-uri", imageURI.get(),
                                               "media-uri", mediaURI.get(),
                                               "context", context,
                                               "inner-node", node,
                                               "x", point.x(),
                                               "y", point.y(),
                                               NULL));
}

}
