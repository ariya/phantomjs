/*
 * Copyright (C) 2012 Igalia S.L.
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
#include "WebKitHitTestResult.h"

#include "WebHitTestResult.h"
#include "WebKitHitTestResultPrivate.h"
#include <glib/gi18n-lib.h>
#include <wtf/text/CString.h>

using namespace WebKit;

/**
 * SECTION: WebKitHitTestResult
 * @Short_description: Result of a Hit Test
 * @Title: WebKitHitTestResult
 * @See_also: #WebKitWebView
 *
 * A Hit Test is an operation to get context information about a given
 * point in a #WebKitWebView. #WebKitHitTestResult represents the
 * result of a Hit Test. It provides context information about what is
 * at the coordinates of the Hit Test, such as if there's a link,
 * an image or a media.
 *
 * You can get the context of the HitTestResult with
 * webkit_hit_test_result_get_context() that returns a bitmask of
 * #WebKitHitTestResultContext flags. You can also use
 * webkit_hit_test_result_context_is_link(), webkit_hit_test_result_context_is_image() and
 * webkit_hit_test_result_context_is_media() to determine whether there's
 * a link, image or a media element at the coordinates of the Hit Test.
 * Note that it's possible that several #WebKitHitTestResultContext flags
 * are active at the same time, for example if there's a link containing an image.
 *
 * When the mouse is moved over a #WebKitWebView a Hit Test is performed
 * for the mouse coordinates and #WebKitWebView::mouse-target-changed
 * signal is emitted with a #WebKitHitTestResult.
 *
 */

enum {
    PROP_0,

    PROP_CONTEXT,
    PROP_LINK_URI,
    PROP_LINK_TITLE,
    PROP_LINK_LABEL,
    PROP_IMAGE_URI,
    PROP_MEDIA_URI
};

struct _WebKitHitTestResultPrivate {
    unsigned int context;
    CString linkURI;
    CString linkTitle;
    CString linkLabel;
    CString imageURI;
    CString mediaURI;
};

WEBKIT_DEFINE_TYPE(WebKitHitTestResult, webkit_hit_test_result, G_TYPE_OBJECT)

static void webkitHitTestResultGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* paramSpec)
{
    WebKitHitTestResult* hitTestResult = WEBKIT_HIT_TEST_RESULT(object);

    switch (propId) {
    case PROP_CONTEXT:
        g_value_set_uint(value, webkit_hit_test_result_get_context(hitTestResult));
        break;
    case PROP_LINK_URI:
        g_value_set_string(value, webkit_hit_test_result_get_link_uri(hitTestResult));
        break;
    case PROP_LINK_TITLE:
        g_value_set_string(value, webkit_hit_test_result_get_link_title(hitTestResult));
        break;
    case PROP_LINK_LABEL:
        g_value_set_string(value, webkit_hit_test_result_get_link_label(hitTestResult));
        break;
    case PROP_IMAGE_URI:
        g_value_set_string(value, webkit_hit_test_result_get_image_uri(hitTestResult));
        break;
    case PROP_MEDIA_URI:
        g_value_set_string(value, webkit_hit_test_result_get_media_uri(hitTestResult));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, paramSpec);
    }
}

static void webkitHitTestResultSetProperty(GObject* object, guint propId, const GValue* value, GParamSpec* paramSpec)
{
    WebKitHitTestResult* hitTestResult = WEBKIT_HIT_TEST_RESULT(object);

    switch (propId) {
    case PROP_CONTEXT:
        hitTestResult->priv->context = g_value_get_uint(value);
        break;
    case PROP_LINK_URI:
        hitTestResult->priv->linkURI = g_value_get_string(value);
        break;
    case PROP_LINK_TITLE:
        hitTestResult->priv->linkTitle = g_value_get_string(value);
        break;
    case PROP_LINK_LABEL:
        hitTestResult->priv->linkLabel = g_value_get_string(value);
        break;
    case PROP_IMAGE_URI:
        hitTestResult->priv->imageURI = g_value_get_string(value);
        break;
    case PROP_MEDIA_URI:
        hitTestResult->priv->mediaURI = g_value_get_string(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, paramSpec);
    }
}

static void webkit_hit_test_result_class_init(WebKitHitTestResultClass* hitTestResultClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(hitTestResultClass);
    objectClass->get_property = webkitHitTestResultGetProperty;
    objectClass->set_property = webkitHitTestResultSetProperty;

    GParamFlags paramFlags = static_cast<GParamFlags>(WEBKIT_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    /**
     * WebKitHitTestResult:context:
     *
     * Bitmask of #WebKitHitTestResultContext flags representing
     * the context of the #WebKitHitTestResult.
     */
    g_object_class_install_property(objectClass,
                                    PROP_CONTEXT,
                                    g_param_spec_uint("context",
                                                      _("Context"),
                                                      _("Flags with the context of the WebKitHitTestResult"),
                                                      0, G_MAXUINT, 0,
                                                      paramFlags));

    /**
     * WebKitHitTestResult:link-uri:
     *
     * The URI of the link if flag %WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK
     * is present in #WebKitHitTestResult:context
     */
    g_object_class_install_property(objectClass,
                                    PROP_LINK_URI,
                                    g_param_spec_string("link-uri",
                                                        _("Link URI"),
                                                        _("The link URI"),
                                                        0,
                                                        paramFlags));
    /**
     * WebKitHitTestResult:link-title:
     *
     * The title of the link if flag %WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK
     * is present in #WebKitHitTestResult:context
     */
    g_object_class_install_property(objectClass,
                                    PROP_LINK_TITLE,
                                    g_param_spec_string("link-title",
                                                        _("Link Title"),
                                                        _("The link title"),
                                                        0,
                                                        paramFlags));
    /**
     * WebKitHitTestResult:link-label:
     *
     * The label of the link if flag %WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK
     * is present in #WebKitHitTestResult:context
     */
    g_object_class_install_property(objectClass,
                                    PROP_LINK_LABEL,
                                    g_param_spec_string("link-label",
                                                        _("Link Label"),
                                                        _("The link label"),
                                                        0,
                                                        paramFlags));
    /**
     * WebKitHitTestResult:image-uri:
     *
     * The URI of the image if flag %WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE
     * is present in #WebKitHitTestResult:context
     */
    g_object_class_install_property(objectClass,
                                    PROP_IMAGE_URI,
                                    g_param_spec_string("image-uri",
                                                        _("Image URI"),
                                                        _("The image URI"),
                                                        0,
                                                        paramFlags));
    /**
     * WebKitHitTestResult:media-uri:
     *
     * The URI of the media if flag %WEBKIT_HIT_TEST_RESULT_CONTEXT_MEDIA
     * is present in #WebKitHitTestResult:context
     */
    g_object_class_install_property(objectClass,
                                    PROP_MEDIA_URI,
                                    g_param_spec_string("media-uri",
                                                        _("Media URI"),
                                                        _("The media URI"),
                                                        0,
                                                        paramFlags));
}

WebKitHitTestResult* webkitHitTestResultCreate(WebHitTestResult* hitTestResult)
{
    unsigned context = WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT;

    const String& linkURL = hitTestResult->absoluteLinkURL();
    if (!linkURL.isEmpty())
        context |= WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK;

    const String& imageURL = hitTestResult->absoluteImageURL();
    if (!imageURL.isEmpty())
        context |= WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE;

    const String& mediaURL = hitTestResult->absoluteMediaURL();
    if (!mediaURL.isEmpty())
        context |= WEBKIT_HIT_TEST_RESULT_CONTEXT_MEDIA;

    if (hitTestResult->isContentEditable())
        context |= WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE;

    if (hitTestResult->isScrollbar())
        context |= WEBKIT_HIT_TEST_RESULT_CONTEXT_SCROLLBAR;

    const String& linkTitle = hitTestResult->linkTitle();
    const String& linkLabel = hitTestResult->linkLabel();

    return WEBKIT_HIT_TEST_RESULT(g_object_new(WEBKIT_TYPE_HIT_TEST_RESULT,
                                               "context", context,
                                               "link-uri", !linkURL.isEmpty() ? linkURL.utf8().data() : 0,
                                               "image-uri", !imageURL.isEmpty() ? imageURL.utf8().data() : 0,
                                               "media-uri", !mediaURL.isEmpty() ? mediaURL.utf8().data() : 0,
                                               "link-title", !linkTitle.isEmpty() ? linkTitle.utf8().data() : 0,
                                               "link-label", !linkLabel.isEmpty() ? linkLabel.utf8().data() : 0,
                                               NULL));
}

static bool stringIsEqualToCString(const String& string, const CString& cString)
{
    return ((string.isEmpty() && cString.isNull()) || (string.utf8() == cString));
}

bool webkitHitTestResultCompare(WebKitHitTestResult* hitTestResult, WebHitTestResult* webHitTestResult)
{
    WebKitHitTestResultPrivate* priv = hitTestResult->priv;
    return webHitTestResult->isContentEditable() == webkit_hit_test_result_context_is_editable(hitTestResult)
        && webHitTestResult->isScrollbar() == webkit_hit_test_result_context_is_scrollbar(hitTestResult)
        && stringIsEqualToCString(webHitTestResult->absoluteLinkURL(), priv->linkURI)
        && stringIsEqualToCString(webHitTestResult->linkTitle(), priv->linkTitle)
        && stringIsEqualToCString(webHitTestResult->linkLabel(), priv->linkLabel)
        && stringIsEqualToCString(webHitTestResult->absoluteImageURL(), priv->imageURI)
        && stringIsEqualToCString(webHitTestResult->absoluteMediaURL(), priv->mediaURI);
}

/**
 * webkit_hit_test_result_get_context:
 * @hit_test_result: a #WebKitHitTestResult
 *
 * Gets the value of the #WebKitHitTestResult:context property.
 *
 * Returns: a bitmask of #WebKitHitTestResultContext flags
 */
guint webkit_hit_test_result_get_context(WebKitHitTestResult* hitTestResult)
{
    g_return_val_if_fail(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult), 0);

    return hitTestResult->priv->context;
}

/**
 * webkit_hit_test_result_context_is_link:
 * @hit_test_result: a #WebKitHitTestResult
 *
 * Gets whether %WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK flag is present in
 * #WebKitHitTestResult:context.
 *
 * Returns: %TRUE if there's a link element in the coordinates of the Hit Test,
 *    or %FALSE otherwise
 */
gboolean webkit_hit_test_result_context_is_link(WebKitHitTestResult* hitTestResult)
{
    g_return_val_if_fail(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult), FALSE);

    return hitTestResult->priv->context & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK;
}

/**
 * webkit_hit_test_result_context_is_image:
 * @hit_test_result: a #WebKitHitTestResult
 *
 * Gets whether %WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE flag is present in
 * #WebKitHitTestResult:context.
 *
 * Returns: %TRUE if there's an image element in the coordinates of the Hit Test,
 *    or %FALSE otherwise
 */
gboolean webkit_hit_test_result_context_is_image(WebKitHitTestResult* hitTestResult)
{
    g_return_val_if_fail(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult), FALSE);

    return hitTestResult->priv->context & WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE;
}

/**
 * webkit_hit_test_result_context_is_media:
 * @hit_test_result: a #WebKitHitTestResult
 *
 * Gets whether %WEBKIT_HIT_TEST_RESULT_CONTEXT_MEDIA flag is present in
 * #WebKitHitTestResult:context.
 *
 * Returns: %TRUE if there's a media element in the coordinates of the Hit Test,
 *    or %FALSE otherwise
 */
gboolean webkit_hit_test_result_context_is_media(WebKitHitTestResult* hitTestResult)
{
    g_return_val_if_fail(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult), FALSE);

    return hitTestResult->priv->context & WEBKIT_HIT_TEST_RESULT_CONTEXT_MEDIA;
}

/**
 * webkit_hit_test_result_context_is_editable:
 * @hit_test_result: a #WebKitHitTestResult
 *
 * Gets whether %WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE flag is present in
 * #WebKitHitTestResult:context.
 *
 * Returns: %TRUE if there's an editable element at the coordinates of the @hit_test_result,
 *    or %FALSE otherwise
 */
gboolean webkit_hit_test_result_context_is_editable(WebKitHitTestResult* hitTestResult)
{
    g_return_val_if_fail(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult), FALSE);

    return hitTestResult->priv->context & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE;
}

/**
 * webkit_hit_test_result_get_link_uri:
 * @hit_test_result: a #WebKitHitTestResult
 *
 * Gets the value of the #WebKitHitTestResult:link-uri property.
 *
 * Returns: the URI of the link element in the coordinates of the Hit Test,
 *    or %NULL if there ins't a link element in @hit_test_result context
 */
const gchar* webkit_hit_test_result_get_link_uri(WebKitHitTestResult* hitTestResult)
{
    g_return_val_if_fail(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult), 0);

    return hitTestResult->priv->linkURI.data();
}

/**
 * webkit_hit_test_result_get_link_title:
 * @hit_test_result: a #WebKitHitTestResult
 *
 * Gets the value of the #WebKitHitTestResult:link-title property.
 *
 * Returns: the title of the link element in the coordinates of the Hit Test,
 *    or %NULL if there ins't a link element in @hit_test_result context or the
 *    link element doesn't have a title
 */
const gchar* webkit_hit_test_result_get_link_title(WebKitHitTestResult* hitTestResult)
{
    g_return_val_if_fail(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult), 0);

    return hitTestResult->priv->linkTitle.data();
}

/**
 * webkit_hit_test_result_get_link_label:
 * @hit_test_result: a #WebKitHitTestResult
 *
 * Gets the value of the #WebKitHitTestResult:link-label property.
 *
 * Returns: the label of the link element in the coordinates of the Hit Test,
 *    or %NULL if there ins't a link element in @hit_test_result context or the
 *    link element doesn't have a label
 */
const gchar* webkit_hit_test_result_get_link_label(WebKitHitTestResult* hitTestResult)
{
    g_return_val_if_fail(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult), 0);

    return hitTestResult->priv->linkLabel.data();
}

/**
 * webkit_hit_test_result_get_image_uri:
 * @hit_test_result: a #WebKitHitTestResult
 *
 * Gets the value of the #WebKitHitTestResult:image-uri property.
 *
 * Returns: the URI of the image element in the coordinates of the Hit Test,
 *    or %NULL if there ins't an image element in @hit_test_result context
 */
const gchar* webkit_hit_test_result_get_image_uri(WebKitHitTestResult* hitTestResult)
{
    g_return_val_if_fail(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult), 0);

    return hitTestResult->priv->imageURI.data();
}

/**
 * webkit_hit_test_result_get_media_uri:
 * @hit_test_result: a #WebKitHitTestResult
 *
 * Gets the value of the #WebKitHitTestResult:media-uri property.
 *
 * Returns: the URI of the media element in the coordinates of the Hit Test,
 *    or %NULL if there ins't a media element in @hit_test_result context
 */
const gchar* webkit_hit_test_result_get_media_uri(WebKitHitTestResult* hitTestResult)
{
    g_return_val_if_fail(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult), 0);

    return hitTestResult->priv->mediaURI.data();
}

/**
 * webkit_hit_test_result_context_is_scrollbar:
 * @hit_test_result: a #WebKitHitTestResult
 *
 * Gets whether %WEBKIT_HIT_TEST_RESULT_CONTEXT_SCROLLBAR flag is present in
 * #WebKitHitTestResult:context.
 *
 * Returns: %TRUE if there's a scrollbar element at the coordinates of the @hit_test_result,
 *    or %FALSE otherwise
 */
gboolean webkit_hit_test_result_context_is_scrollbar(WebKitHitTestResult* hitTestResult)
{
    g_return_val_if_fail(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult), FALSE);

    return hitTestResult->priv->context & WEBKIT_HIT_TEST_RESULT_CONTEXT_SCROLLBAR;
}
