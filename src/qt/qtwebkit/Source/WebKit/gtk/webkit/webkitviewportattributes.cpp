/*
 * Copyright (C) 2010 Joone Hur <joone@kldp.org>
 * Copyright (C) 2010 Collabora Ltd.
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
#include "webkitviewportattributes.h"

#include "Chrome.h"
#include "Document.h"
#include "Frame.h"
#include "Page.h"
#include "webkitglobalsprivate.h"
#include "webkitviewportattributesprivate.h"
#include "webkitwebviewprivate.h"
#include <glib/gi18n-lib.h>

/**
 * SECTION:webkitviewportattributes
 * @short_description: Represents the viewport properties of a web page
 * @see_also: #WebKitWebView::viewport-attributes-recompute-requested, #WebKitWebView::viewport-attributes-changed
 *
 * #WebKitViewportAttributes offers the viewport properties to user agents to
 * control the viewport layout. It contains the viewport size, initial scale with limits,
 * and information about whether a user is able to scale the contents in the viewport.
 * This makes a web page fit the device screen.
 *
 * The #WebKitWebView::viewport-attributes-changed signal will be emitted with #WebKitViewportAttributes
 * when the viewport attributes are updated in the case of loading web pages contain
 * the viewport properties and calling webkit_viewport_attributes_recompute.
 *
 * If the device size, available size, desktop width, or device DPI needs to be changed due to
 * a consequence of an explicit browser request (caused by screen rotation, resizing, or similar reasons),
 * You should call #webkit_viewport_attributes_recompute to recompute the viewport properties and
 * override those values in the handler of #WebKitWebView::viewport-attributes-recompute-requested signal.
 *
 * For more information on the viewport properties, refer to the Safari reference library at
 * http://developer.apple.com/safari/library/documentation/appleapplications/reference/safarihtmlref/articles/metatags.html
 *
 * <informalexample><programlisting>
 * /<!-- -->* Connect to the viewport-attributes-changes signal *<!-- -->/
 * WebKitViewportAttributes* attributes = webkit_web_view_get_viewport_attributes (web_view);
 * g_signal_connect (web_view, "viewport-attributes-recompute-requested", G_CALLBACK (viewport_recompute_cb), window);
 * g_signal_connect (web_view, "viewport-attributes-changed", G_CALLBACK (viewport_changed_cb), window);
 * g_signal_connect (attributes, "notify::valid", G_CALLBACK (viewport_valid_changed_cb), web_view);
 *
 * /<!-- -->* Handle the viewport-attributes-recompute-requested signal to override the device width *<!-- -->/
 * static void
 * viewport_recompute_cb (WebKitWebView* web_view, WebKitViewportAttributes* attributes, GtkWidget* window)
 * {
 *     int override_available_width = 480;
 *     g_object_set (G_OBJECT(attributes), "available-width", override_available_width, NULL);
 * }
 *
 * /<!-- -->* Handle the viewport-attributes-changed signal to recompute the initial scale factor *<!-- -->/
 * static void
 * viewport_changed_cb (WebKitWebView* web_view, WebKitViewportAttributes* attributes, gpointer data)
 * {
 *     gfloat initialScale;
 *     g_object_get (G_OBJECT (atributes), "initial-scale-factor", &initialScale, NULL);
 *     webkit_web_view_set_zoom_level (web_view, initialScale);
 * }
 *
 * /<!-- -->* Handle the notify::valid signal to initialize the zoom level *<!-- -->/
 * static void
 * viewport_valid_changed_cb (WebKitViewportAttributes* attributes, GParamSpec* pspec, WebKitWebView* web_view)
 * {
 *     gboolean is_valid;
 *     g_object_get (attributes, "valid", &is_valid, NULL);
 *     if (!is_valid)
 *         webkit_web_view_set_zoom_level (web_view, 1.0);
 * }
 * </programlisting></informalexample>
 */

using namespace WebKit;
using namespace WebCore;

enum {
    PROP_0,

    PROP_DEVICE_WIDTH,
    PROP_DEVICE_HEIGHT,
    PROP_AVAILABLE_WIDTH,
    PROP_AVAILABLE_HEIGHT,
    PROP_DESKTOP_WIDTH,
    PROP_DEVICE_DPI,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_INITIAL_SCALE_FACTOR,
    PROP_MINIMUM_SCALE_FACTOR,
    PROP_MAXIMUM_SCALE_FACTOR,
    PROP_DEVICE_PIXEL_RATIO,
    PROP_USER_SCALABLE,
    PROP_VALID
};

G_DEFINE_TYPE(WebKitViewportAttributes, webkit_viewport_attributes, G_TYPE_OBJECT);

static void webkit_viewport_attributes_get_property(GObject* object, guint propertyID, GValue* value, GParamSpec* paramSpec);
static void webkit_viewport_attributes_set_property(GObject* object, guint propertyID, const GValue* value, GParamSpec* paramSpec);

static void webkit_viewport_attributes_class_init(WebKitViewportAttributesClass* kclass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(kclass);
    gobjectClass->get_property = webkit_viewport_attributes_get_property;
    gobjectClass->set_property = webkit_viewport_attributes_set_property;

    /**
     * WebKitViewportAttributs:device-width:
     *
     * The width of the screen. This value is always automatically
     * pre-computed during a viewport attributes recomputation, and
     * can be overridden by the handler of
     * WebKitWebView::viewport-attributes-recompute-requested. You
     * should not do that unless you have a very good reason.
     *
     * Since: 1.3.8
     */
    g_object_class_install_property(gobjectClass,
                                    PROP_DEVICE_WIDTH,
                                    g_param_spec_int(
                                    "device-width",
                                    _("Device Width"),
                                    _("The width of the screen."),
                                    0,
                                    G_MAXINT,
                                    0,
                                    WEBKIT_PARAM_READWRITE));

    /**
     * WebKitViewportAttributs:device-height:
     *
     * The height of the screen. This value is always automatically
     * pre-computed during a viewport attributes recomputation, and
     * can be overriden by the handler of
     * WebKitWebView::viewport-attributes-recompute-requested. You
     * should not do that unless you have a very good reason.
     *
     * Since: 1.3.8
     */
    g_object_class_install_property(gobjectClass,
                                    PROP_DEVICE_HEIGHT,
                                    g_param_spec_int(
                                    "device-height",
                                    _("Device Height"),
                                    _("The height of the screen."),
                                    0,
                                    G_MAXINT,
                                    0,
                                    WEBKIT_PARAM_READWRITE));

    /**
     * WebKitViewportAttributs:available-width:
     *
     * The width of the current visible area. This will usually be the
     * same as the space allocated to the widget, but in some cases
     * you may have decided to make the widget bigger than the visible
     * area. This value is by default initialized to the size
     * allocated by the widget, but you can override it in the handler
     * of WebKitWebView::viewport-attributes-recompute-requested to
     * let the engine know what the visible area is.
     *
     * Since: 1.3.8
     */
    g_object_class_install_property(gobjectClass,
                                    PROP_AVAILABLE_WIDTH,
                                    g_param_spec_int(
                                    "available-width",
                                    _("Available Width"),
                                    _("The width of the visible area."),
                                    0,
                                    G_MAXINT,
                                    0,
                                    WEBKIT_PARAM_READWRITE));

    /**
     * WebKitViewportAttributs:available-height:
     *
     * The height of the current visible area. This will usually be the
     * same as the space allocated to the widget, but in some cases
     * you may have decided to make the widget bigger than the visible
     * area. This value is by default initialized to the size
     * allocated by the widget, but you can override it in the handler
     * of WebKitWebView::viewport-attributes-recompute-requested to
     * let the engine know what the visible area is.
     *
     * Since: 1.3.8
     */
    g_object_class_install_property(gobjectClass,
                                    PROP_AVAILABLE_HEIGHT,
                                    g_param_spec_int(
                                    "available-height",
                                    _("Available Height"),
                                    _("The height of the visible area."),
                                    0,
                                    G_MAXINT,
                                    0,
                                    WEBKIT_PARAM_READWRITE));

    /**
     * WebKitViewportAttributs:desktop-width:
     *
     * The width of viewport that works well for most web pages designed for
     * desktop. This value is initialized to 980 pixels by default and used
     * during a viewport attributes recomputation. Also, it can be overriden by 
     * the handler of WebKitWebView::viewport-attributes-recompute-requested.
     * You should not do that unless you have a very good reason.
     *
     * Since: 1.3.8
     */
    g_object_class_install_property(gobjectClass,
                                    PROP_DESKTOP_WIDTH,
                                    g_param_spec_int(
                                    "desktop-width",
                                    _("Desktop Width"),
                                    _("The width of viewport that works well for most web pages designed for desktop."),
                                    0,
                                    G_MAXINT,
                                    980,
                                    WEBKIT_PARAM_READWRITE));

    /**
     * WebKitViewportAttributs:device-dpi:
     *
     * The number of dots per inch of the screen. This value is
     * initialized to 160 dpi by default and used during a viewport
     * attributes recomputation, because it is the dpi of the original
     * iPhone and Android devices. Also, it can be overriden by the
     * handler of WebKitWebView::viewport-attributes-recompute-requested.
     * You should not do that unless you have a very good reason.
     *
     * Since: 1.3.8
     */
    g_object_class_install_property(gobjectClass,
                                    PROP_DEVICE_DPI,
                                    g_param_spec_int(
                                    "device-dpi",
                                    _("Device DPI"),
                                    _("The number of dots per inch of the screen."),
                                    0,
                                    G_MAXINT,
                                    160,
                                    WEBKIT_PARAM_READWRITE));

    /**
     * WebKitViewportAttributs:width:
     *
     * The width of the viewport. Before getting this property,
     * you need to make sure that #WebKitViewportAttributes is valid.
     *
     * Since: 1.3.8
     */
    g_object_class_install_property(gobjectClass,
                                    PROP_WIDTH,
                                    g_param_spec_int(
                                    "width",
                                    _("Width"),
                                    _("The width of the viewport."),
                                    0,
                                    G_MAXINT,
                                    0,
                                    WEBKIT_PARAM_READABLE));

    /**
     * WebKitViewportAttributs:height:
     *
     * The height of the viewport. Before getting this property,
     * you need to make sure that #WebKitViewportAttributes is valid.
     *
     * Since: 1.3.8
     */
    g_object_class_install_property(gobjectClass,
                                    PROP_HEIGHT,
                                    g_param_spec_int(
                                    "height",
                                    _("Height"),
                                    _("The height of the viewport."),
                                    0,
                                    G_MAXINT,
                                    0,
                                    WEBKIT_PARAM_READABLE));

    /**
     * WebKitViewportAttributs:initial-scale-factor:
     *
     * The initial scale of the viewport. Before getting this property,
     * you need to make sure that #WebKitViewportAttributes is valid.
     *
     * Since: 1.3.8
     */
    g_object_class_install_property(gobjectClass,
                                    PROP_INITIAL_SCALE_FACTOR,
                                    g_param_spec_float(
                                    "initial-scale-factor",
                                    _("Initial Scale Factor"),
                                    _("The initial scale of the viewport."),
                                    -1,
                                    G_MAXFLOAT,
                                    -1,
                                    WEBKIT_PARAM_READABLE));

    /**
     * WebKitViewportAttributs:minimum-scale-factor:
     *
     * The minimum scale of the viewport. Before getting this property,
     * you need to make sure that #WebKitViewportAttributes is valid.
     *
     * Since: 1.3.8
     */
    g_object_class_install_property(gobjectClass,
                                    PROP_MINIMUM_SCALE_FACTOR,
                                    g_param_spec_float(
                                    "minimum-scale-factor",
                                    _("Minimum Scale Factor"),
                                    _("The minimum scale of the viewport."),
                                    -1,
                                    G_MAXFLOAT,
                                    -1,
                                    WEBKIT_PARAM_READABLE));

    /**
     * WebKitViewportAttributs:maximum-scale-factor:
     *
     * The maximum scale of the viewport. Before getting this property,
     * you need to make sure that #WebKitViewportAttributes is valid.
     *
     * Since: 1.3.8
     */
    g_object_class_install_property(gobjectClass,
                                    PROP_MAXIMUM_SCALE_FACTOR,
                                    g_param_spec_float(
                                    "maximum-scale-factor",
                                    _("Maximum Scale Factor"),
                                    _("The maximum scale of the viewport."),
                                    -1,
                                    G_MAXFLOAT,
                                    -1,
                                    WEBKIT_PARAM_READABLE));

    /**
     * WebKitViewportAttributs:device-pixel-ratio:
     *
     * The device pixel ratio of the viewport. Before getting this property,
     * you need to make sure that #WebKitViewportAttributes is valid.
     *
     * Since: 1.3.8
     */
    g_object_class_install_property(gobjectClass,
                                    PROP_DEVICE_PIXEL_RATIO,
                                    g_param_spec_float(
                                    "device-pixel-ratio",
                                    _("Device Pixel Ratio"),
                                    _("The device pixel ratio of the viewport."),
                                    -1,
                                    G_MAXFLOAT,
                                    -1,
                                    WEBKIT_PARAM_READABLE));

    /**
     * WebKitViewportAttributs:user-scalable:
     *
     * Determines whether or not the user can zoom in and out.
     * Before getting this property, you need to make sure that
     * #WebKitViewportAttributes is valid.
     *
     * Since: 1.3.8
     */
    g_object_class_install_property(gobjectClass,
                                    PROP_USER_SCALABLE,
                                    g_param_spec_boolean(
                                    "user-scalable",
                                    _("User Scalable"),
                                    _("Determines whether or not the user can zoom in and out."),
                                    TRUE,
                                    WEBKIT_PARAM_READABLE));

    /**
     * WebKitViewportAttributs:valid:
     *
     * Determines whether or not the attributes are valid.
     * #WebKitViewportAttributes are only valid on pages
     * which have a viewport meta tag, and have already
     * had the attributes calculated.
     *
     * Since: 1.3.8
     */
    g_object_class_install_property(gobjectClass,
                                    PROP_VALID,
                                    g_param_spec_boolean(
                                    "valid",
                                    _("Valid"),
                                    _("Determines whether or not the attributes are valid, and can be used."),
                                    FALSE,
                                    WEBKIT_PARAM_READABLE));

    g_type_class_add_private(kclass, sizeof(WebKitViewportAttributesPrivate));
}

static void webkit_viewport_attributes_init(WebKitViewportAttributes* viewport)
{
    viewport->priv = G_TYPE_INSTANCE_GET_PRIVATE(viewport, WEBKIT_TYPE_VIEWPORT_ATTRIBUTES, WebKitViewportAttributesPrivate);

    viewport->priv->deviceWidth = 0;
    viewport->priv->deviceHeight = 0;
    viewport->priv->availableWidth = 0;
    viewport->priv->availableHeight = 0;
    viewport->priv->desktopWidth = 980; // This value works well for most web pages designed for desktop browsers.
    viewport->priv->deviceDPI = 160; // It is the dpi of the original iPhone and Android devices.
    viewport->priv->width = 0;
    viewport->priv->height = 0;
    viewport->priv->initialScaleFactor = -1;
    viewport->priv->minimumScaleFactor = -1;
    viewport->priv->maximumScaleFactor = -1;
    viewport->priv->devicePixelRatio = -1;
    viewport->priv->userScalable = TRUE;
    viewport->priv->isValid = FALSE;
}

static void webkit_viewport_attributes_get_property(GObject* object, guint propertyID, GValue* value, GParamSpec* paramSpec)
{
    WebKitViewportAttributes* viewportAttributes = WEBKIT_VIEWPORT_ATTRIBUTES(object);
    WebKitViewportAttributesPrivate* priv = viewportAttributes->priv;

    switch (propertyID) {
    case PROP_DEVICE_WIDTH:
        g_value_set_int(value, priv->deviceWidth);
        break;
    case PROP_DEVICE_HEIGHT:
        g_value_set_int(value, priv->deviceHeight);
        break;
    case PROP_AVAILABLE_WIDTH:
        g_value_set_int(value, priv->availableWidth);
        break;
    case PROP_AVAILABLE_HEIGHT:
        g_value_set_int(value, priv->availableHeight);
        break;
    case PROP_DESKTOP_WIDTH:
        g_value_set_int(value, priv->desktopWidth);
        break;
    case PROP_DEVICE_DPI:
        g_value_set_int(value, priv->deviceDPI);
        break;
    case PROP_WIDTH:
        g_value_set_int(value, priv->width);
        break;
    case PROP_HEIGHT:
        g_value_set_int(value, priv->height);
        break;
    case PROP_INITIAL_SCALE_FACTOR:
        g_value_set_float(value, priv->initialScaleFactor);
        break;
    case PROP_MINIMUM_SCALE_FACTOR:
        g_value_set_float(value, priv->minimumScaleFactor);
        break;
    case PROP_MAXIMUM_SCALE_FACTOR:
        g_value_set_float(value, priv->maximumScaleFactor);
        break;
    case PROP_DEVICE_PIXEL_RATIO:
        g_value_set_float(value, priv->devicePixelRatio);
        break;
    case PROP_USER_SCALABLE:
        g_value_set_boolean(value, priv->userScalable);
        break;
    case PROP_VALID:
        g_value_set_boolean(value, priv->isValid);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyID, paramSpec);
        break;
    }
}

static void webkit_viewport_attributes_set_property(GObject* object, guint propertyID, const GValue* value, GParamSpec* paramSpec)
{
    WebKitViewportAttributes* viewportAttributes = WEBKIT_VIEWPORT_ATTRIBUTES(object);
    WebKitViewportAttributesPrivate* priv = viewportAttributes->priv;

    switch (propertyID) {
    case PROP_DEVICE_WIDTH:
        priv->deviceWidth = g_value_get_int(value);
        break;
    case PROP_DEVICE_HEIGHT:
        priv->deviceHeight = g_value_get_int(value);
        break;
    case PROP_AVAILABLE_WIDTH:
        priv->availableWidth = g_value_get_int(value);
        break;
    case PROP_AVAILABLE_HEIGHT:
        priv->availableHeight = g_value_get_int(value);
        break;
    case PROP_DESKTOP_WIDTH:
        priv->desktopWidth = g_value_get_int(value);
        break;
    case PROP_DEVICE_DPI:
        priv->deviceDPI = g_value_get_int(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyID, paramSpec);
        break;
    }
}

void webkitViewportAttributesRecompute(WebKitViewportAttributes* viewportAttributes)
{
    WebKitViewportAttributesPrivate* priv = viewportAttributes->priv;
    WebKitWebView* webView = priv->webView;

    IntRect windowRect(webView->priv->corePage->chrome().windowRect());
    priv->deviceWidth = windowRect.width();
    priv->deviceHeight = windowRect.height();

    IntRect rect(webView->priv->corePage->chrome().pageRect());
    priv->availableWidth = rect.width();
    priv->availableHeight = rect.height();

    // First of all, we give the application an opportunity to override some of the values.
    g_signal_emit_by_name(webView, "viewport-attributes-recompute-requested", viewportAttributes);

    ViewportArguments arguments = webView->priv->corePage->mainFrame()->document()->viewportArguments();

    float devicePixelRatio = priv->deviceDPI / ViewportArguments::deprecatedTargetDPI;
    ViewportAttributes attributes = computeViewportAttributes(arguments, priv->desktopWidth, priv->deviceWidth, priv->deviceHeight, devicePixelRatio, IntSize(priv->availableWidth, priv->availableHeight));
    restrictMinimumScaleFactorToViewportSize(attributes, IntSize(priv->availableWidth, priv->availableHeight), devicePixelRatio);
    restrictScaleFactorToInitialScaleIfNotUserScalable(attributes);

    priv->width = attributes.layoutSize.width();
    priv->height = attributes.layoutSize.height();
    priv->initialScaleFactor = attributes.initialScale;
    priv->minimumScaleFactor = attributes.minimumScale;
    priv->maximumScaleFactor = attributes.maximumScale;
    priv->devicePixelRatio = devicePixelRatio;
    priv->userScalable = static_cast<bool>(arguments.userZoom);

    if (!priv->isValid) {
        priv->isValid = TRUE;
        g_object_notify(G_OBJECT(viewportAttributes), "valid");
    }

    // Now let the application know it is safe to use the new values.
    g_signal_emit_by_name(webView, "viewport-attributes-changed", viewportAttributes);
}

/**
 * webkit_viewport_attributes_recompute:
 * @viewportAttributes: a #WebKitViewportAttributes
 *
 * Recompute the optimal viewport attributes and emit the viewport-attribute-changed signal.
 * The viewport-attributes-recompute-requested signal also will be handled to override
 * the device size, available size, desktop width, or device DPI.
 *
 * Since: 1.3.8
 */
void webkit_viewport_attributes_recompute(WebKitViewportAttributes* viewportAttributes)
{
    if (!viewportAttributes->priv->isValid)
        return;
    webkitViewportAttributesRecompute(viewportAttributes);
}
