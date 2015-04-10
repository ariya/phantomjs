/*
 * Copyright (C) 2013 Igalia S.L.
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
#include "WebKitWebViewGroup.h"

#include "ImmutableArray.h"
#include "WebKitPrivate.h"
#include "WebKitSettingsPrivate.h"
#include "WebKitWebViewGroupPrivate.h"
#include <glib/gi18n-lib.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

/**
 * SECTION: WebKitWebViewGroup
 * @Short_description: Group of web views
 * @Title: WebKitWebViewGroup
 * @See_also: #WebKitWebView, #WebKitSettings
 *
 * A WebKitWebViewGroup represents a group of #WebKitWebView<!-- -->s that
 * share things like settings. There's a default WebKitWebViewGroup where
 * all #WebKitWebView<!-- -->s of the same #WebKitWebContext are added by default.
 * To create a #WebKitWebView in a different WebKitWebViewGroup you can use
 * webkit_web_view_new_with_group().
 *
 * WebKitWebViewGroups are identified by a unique name given when the group is
 * created with webkit_web_view_group_new().
 * WebKitWebViewGroups have a #WebKitSettings to control the settings of all
 * #WebKitWebView<!-- -->s of the group. You can get the settings with
 * webkit_web_view_group_get_settings() to handle the settings, or you can set
 * your own #WebKitSettings with webkit_web_view_group_set_settings(). When
 * the #WebKitSettings of a WebKitWebViewGroup changes, the signal notify::settings
 * is emitted on the group.
 */

enum {
    PROP_0,

    PROP_SETTINGS
};

struct _WebKitWebViewGroupPrivate {
    RefPtr<WebPageGroup> pageGroup;
    CString name;
    GRefPtr<WebKitSettings> settings;
};

WEBKIT_DEFINE_TYPE(WebKitWebViewGroup, webkit_web_view_group, G_TYPE_OBJECT)

static void webkitWebViewGroupSetProperty(GObject* object, guint propId, const GValue* value, GParamSpec* paramSpec)
{
    WebKitWebViewGroup* group = WEBKIT_WEB_VIEW_GROUP(object);

    switch (propId) {
    case PROP_SETTINGS:
        webkit_web_view_group_set_settings(group, WEBKIT_SETTINGS(g_value_get_object(value)));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, paramSpec);
    }
}

static void webkitWebViewGroupGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* paramSpec)
{
    WebKitWebViewGroup* group = WEBKIT_WEB_VIEW_GROUP(object);

    switch (propId) {
    case PROP_SETTINGS:
        g_value_set_object(value, webkit_web_view_group_get_settings(group));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, paramSpec);
    }
}

static void webkitWebViewGroupConstructed(GObject* object)
{
    G_OBJECT_CLASS(webkit_web_view_group_parent_class)->constructed(object);

    WebKitWebViewGroupPrivate* priv = WEBKIT_WEB_VIEW_GROUP(object)->priv;
    priv->settings = adoptGRef(webkit_settings_new());
}

static void webkit_web_view_group_class_init(WebKitWebViewGroupClass* hitTestResultClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(hitTestResultClass);
    objectClass->set_property = webkitWebViewGroupSetProperty;
    objectClass->get_property = webkitWebViewGroupGetProperty;
    objectClass->constructed = webkitWebViewGroupConstructed;

    /**
     * WebKitWebViewGroup:settings:
     *
     * The #WebKitSettings of the web view group.
     */
    g_object_class_install_property(
        objectClass,
        PROP_SETTINGS,
        g_param_spec_object(
            "settings",
            _("Settings"),
            _("The settings of the web view group"),
            WEBKIT_TYPE_SETTINGS,
            WEBKIT_PARAM_READWRITE));
}

static void webkitWebViewGroupAttachSettingsToPageGroup(WebKitWebViewGroup* group)
{
    group->priv->pageGroup->setPreferences(webkitSettingsGetPreferences(group->priv->settings.get()));
}

WebKitWebViewGroup* webkitWebViewGroupCreate(WebPageGroup* pageGroup)
{
    WebKitWebViewGroup* group = WEBKIT_WEB_VIEW_GROUP(g_object_new(WEBKIT_TYPE_WEB_VIEW_GROUP, NULL));
    group->priv->pageGroup = pageGroup;
    webkitWebViewGroupAttachSettingsToPageGroup(group);
    return group;
}

WebPageGroup* webkitWebViewGroupGetPageGroup(WebKitWebViewGroup* group)
{
    return group->priv->pageGroup.get();
}

/**
 * webkit_web_view_group_new:
 * @name: (allow-none): the name of the group
 *
 * Creates a new #WebKitWebViewGroup with the given @name.
 * If @name is %NULL a unique identifier name will be created
 * automatically.
 * The newly created #WebKitWebViewGroup doesn't contain any
 * #WebKitWebView, web views are added to the new group when created
 * with webkit_web_view_new_with_group() passing the group.
 *
 * Returns: (transfer full): a new #WebKitWebViewGroup
 */
WebKitWebViewGroup* webkit_web_view_group_new(const char* name)
{
    WebKitWebViewGroup* group = WEBKIT_WEB_VIEW_GROUP(g_object_new(WEBKIT_TYPE_WEB_VIEW_GROUP, NULL));
    group->priv->pageGroup = WebPageGroup::create(name ? String::fromUTF8(name) : String());
    webkitWebViewGroupAttachSettingsToPageGroup(group);
    return group;
}

/**
 * webkit_web_view_group_get_name:
 * @group: a #WebKitWebViewGroup
 *
 * Gets the name that uniquely identifies the #WebKitWebViewGroup.
 *
 * Returns: the name of @group
 */
const char* webkit_web_view_group_get_name(WebKitWebViewGroup* group)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW_GROUP(group), 0);

    WebKitWebViewGroupPrivate* priv = group->priv;
    if (priv->name.isNull())
        priv->name = priv->pageGroup->identifier().utf8();

    return priv->name.data();
}

/**
 * webkit_web_view_group_get_settings:
 * @group: a #WebKitWebViewGroup
 *
 * Gets the #WebKitSettings of the #WebKitWebViewGroup.
 *
 * Returns: (transfer none): the settings of @group
 */
WebKitSettings* webkit_web_view_group_get_settings(WebKitWebViewGroup* group)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW_GROUP(group), 0);

    return group->priv->settings.get();
}

/**
 * webkit_web_view_group_set_settings:
 * @group: a #WebKitWebViewGroup
 * @settings: a #WebKitSettings
 *
 * Sets a new #WebKitSettings for the #WebKitWebViewGroup. The settings will
 * affect to all the #WebKitWebView<!-- -->s of the group.
 * #WebKitWebViewGroup<!-- -->s always have a #WebKitSettings so if you just want to
 * modify a setting you can use webkit_web_view_group_get_settings() and modify the
 * returned #WebKitSettings instead.
 * Setting the same #WebKitSettings multiple times doesn't have any effect.
 * You can monitor the settings of a #WebKitWebViewGroup by connecting to the
 * notify::settings signal of @group.
 */
void webkit_web_view_group_set_settings(WebKitWebViewGroup* group, WebKitSettings* settings)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW_GROUP(group));
    g_return_if_fail(WEBKIT_IS_SETTINGS(settings));

    if (group->priv->settings == settings)
        return;

    group->priv->settings = settings;
    webkitWebViewGroupAttachSettingsToPageGroup(group);
    g_object_notify(G_OBJECT(group), "settings");
}

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_INJECTED_CONTENT_FRAMES_ALL, WebCore::InjectInAllFrames);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_INJECTED_CONTENT_FRAMES_TOP_ONLY, WebCore::InjectInTopFrameOnly);

static PassRefPtr<ImmutableArray> toImmutableArray(const char* const* list)
{
    if (!list)
        return 0;

    Vector<RefPtr<APIObject> > entries;
    while (*list) {
        entries.append(WebString::createFromUTF8String(*list));
        list++;
    }
    return ImmutableArray::adopt(entries);
}

/**
 * webkit_web_view_group_add_user_style_sheet:
 * @group: a #WebKitWebViewGroup
 * @source: the source of the style_sheet to inject
 * @base_uri: (allow-none): the base URI to use when processing the style_sheet contents or %NULL for about:blank
 * @whitelist: (array zero-terminated=1) (allow-none): a whitelist of URI patterns or %NULL
 * @blacklist: (array zero-terminated=1) (allow-none): a blacklist of URI patterns or %NULL
 * @injected_frames: a #WebKitInjectedContentFrames describing to which frames the style_sheet should apply
 *
 * Inject an external style sheet into pages. It is possible to only apply the style sheet
 * to some URIs by passing non-null values for @whitelist or @blacklist. Passing a %NULL
 * whitelist implies that all URIs are on the whitelist. The style sheet is applied if a URI matches
 * the whitelist and not the blacklist. URI patterns must be of the form [protocol]://[host]/[path]
 * where the host and path components can contain the wildcard character ('*') to represent zero
 * or more other characters.
 */
void webkit_web_view_group_add_user_style_sheet(WebKitWebViewGroup* group, const char* source, const char* baseURI, const char* const* whitelist, const char* const* blacklist, WebKitInjectedContentFrames injectedFrames)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW_GROUP(group));
    g_return_if_fail(source);

    RefPtr<ImmutableArray> webWhitelist = toImmutableArray(whitelist);
    RefPtr<ImmutableArray> webBlacklist = toImmutableArray(blacklist);

    // We always use UserStyleUserLevel to match the behavior of WKPageGroupAddUserStyleSheet.
    group->priv->pageGroup->addUserStyleSheet(
        String::fromUTF8(source),
        String::fromUTF8(baseURI),
        webWhitelist.get(),
        webBlacklist.get(),
        static_cast<WebCore::UserContentInjectedFrames>(injectedFrames),
        WebCore::UserStyleUserLevel);
}

/**
 * webkit_web_view_group_remove_all_user_style_sheets:
 * @group: a #WebKitWebViewGroup
 *
 * Remove all style sheets previously injected into this #WebKitWebViewGroup 
 * via webkit_web_view_group_add_user_style_sheet().
 */
void webkit_web_view_group_remove_all_user_style_sheets(WebKitWebViewGroup* group)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW_GROUP(group));
    group->priv->pageGroup->removeAllUserStyleSheets();
}
