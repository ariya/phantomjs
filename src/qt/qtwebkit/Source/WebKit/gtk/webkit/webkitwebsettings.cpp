/*
 * Copyright (C) 2008 Christian Dywan <christian@imendio.com>
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2008 Holger Hans Peter Freyther
 * Copyright (C) 2009 Jan Michael Alonzo
 * Copyright (C) 2009 Movial Creative Technologies Inc.
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
#include "webkitwebsettings.h"

#include "EditingBehavior.h"
#include "FileSystem.h"
#include "KURL.h"
#include "PluginDatabase.h"
#include "UserAgentGtk.h"
#include "webkitenumtypes.h"
#include "webkitglobalsprivate.h"
#include "webkitversion.h"
#include "webkitwebsettingsprivate.h"
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringConcatenate.h>
#include <glib/gi18n-lib.h>

/**
 * SECTION:webkitwebsettings
 * @short_description: Control the behaviour of a #WebKitWebView
 *
 * #WebKitWebSettings can be applied to a #WebKitWebView to control text encoding,
 * color, font sizes, printing mode, script support, loading of images and various other things.
 * After creation, a #WebKitWebSettings object contains default settings.
 *
 * <informalexample><programlisting>
 * /<!-- -->* Create a new websettings and disable java script *<!-- -->/
 * WebKitWebSettings *settings = webkit_web_settings_new ();
 * g_object_set (G_OBJECT(settings), "enable-scripts", FALSE, NULL);
 *
 * /<!-- -->* Apply the result *<!-- -->/
 * webkit_web_view_set_settings (WEBKIT_WEB_VIEW(my_webview), settings);
 * </programlisting></informalexample>
 */

using namespace WebCore;

G_DEFINE_TYPE(WebKitWebSettings, webkit_web_settings, G_TYPE_OBJECT)

enum {
    PROP_0,

    PROP_DEFAULT_ENCODING,
    PROP_CURSIVE_FONT_FAMILY,
    PROP_DEFAULT_FONT_FAMILY,
    PROP_FANTASY_FONT_FAMILY,
    PROP_MONOSPACE_FONT_FAMILY,
    PROP_SANS_SERIF_FONT_FAMILY,
    PROP_SERIF_FONT_FAMILY,
    PROP_DEFAULT_FONT_SIZE,
    PROP_DEFAULT_MONOSPACE_FONT_SIZE,
    PROP_MINIMUM_FONT_SIZE,
    PROP_MINIMUM_LOGICAL_FONT_SIZE,
    PROP_ENFORCE_96_DPI,
    PROP_AUTO_LOAD_IMAGES,
    PROP_AUTO_SHRINK_IMAGES,
    PROP_RESPECT_IMAGE_ORIENTATION,
    PROP_PRINT_BACKGROUNDS,
    PROP_ENABLE_SCRIPTS,
    PROP_ENABLE_PLUGINS,
    PROP_RESIZABLE_TEXT_AREAS,
    PROP_USER_STYLESHEET_URI,
    PROP_ZOOM_STEP,
    PROP_ENABLE_DEVELOPER_EXTRAS,
    PROP_ENABLE_PRIVATE_BROWSING,
    PROP_ENABLE_SPELL_CHECKING,
    PROP_SPELL_CHECKING_LANGUAGES,
    PROP_ENABLE_CARET_BROWSING,
    PROP_ENABLE_HTML5_DATABASE,
    PROP_ENABLE_HTML5_LOCAL_STORAGE,
    PROP_HTML5_LOCAL_STORAGE_DATABASE_PATH,
    PROP_ENABLE_XSS_AUDITOR,
    PROP_ENABLE_SPATIAL_NAVIGATION,
    PROP_ENABLE_FRAME_FLATTENING,
    PROP_USER_AGENT,
    PROP_JAVASCRIPT_CAN_OPEN_WINDOWS_AUTOMATICALLY,
    PROP_JAVASCRIPT_CAN_ACCESS_CLIPBOARD,
    PROP_ENABLE_OFFLINE_WEB_APPLICATION_CACHE,
    PROP_EDITING_BEHAVIOR,
    PROP_ENABLE_UNIVERSAL_ACCESS_FROM_FILE_URIS,
    PROP_ENABLE_FILE_ACCESS_FROM_FILE_URIS,
    PROP_ENABLE_DOM_PASTE,
    PROP_TAB_KEY_CYCLES_THROUGH_ELEMENTS,
    PROP_ENABLE_DEFAULT_CONTEXT_MENU,
    PROP_ENABLE_SITE_SPECIFIC_QUIRKS,
    PROP_ENABLE_PAGE_CACHE,
    PROP_AUTO_RESIZE_WINDOW,
    PROP_ENABLE_JAVA_APPLET,
    PROP_ENABLE_HYPERLINK_AUDITING,
    PROP_ENABLE_FULLSCREEN,
    PROP_ENABLE_DNS_PREFETCHING,
    PROP_ENABLE_WEBGL,
    PROP_ENABLE_MEDIA_STREAM,
    PROP_ENABLE_WEB_AUDIO,
    PROP_ENABLE_ACCELERATED_COMPOSITING,
    PROP_ENABLE_SMOOTH_SCROLLING,
    PROP_MEDIA_PLAYBACK_REQUIRES_USER_GESTURE,
    PROP_MEDIA_PLAYBACK_ALLOWS_INLINE,
    PROP_ENABLE_CSS_SHADERS,
    PROP_ENABLE_RUNNING_OF_INSECURE_CONTENT,
    PROP_ENABLE_DISPLAY_OF_INSECURE_CONTENT
};

static void webkit_web_settings_finalize(GObject* object);

static void webkit_web_settings_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);

static void webkit_web_settings_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);

static void webkit_web_settings_class_init(WebKitWebSettingsClass* klass)
{
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = webkit_web_settings_finalize;
    gobject_class->set_property = webkit_web_settings_set_property;
    gobject_class->get_property = webkit_web_settings_get_property;

    webkitInit();

    GParamFlags flags = (GParamFlags)(WEBKIT_PARAM_READWRITE | G_PARAM_CONSTRUCT);

    g_object_class_install_property(gobject_class,
                                    PROP_DEFAULT_ENCODING,
                                    g_param_spec_string(
                                    "default-encoding",
                                    _("Default Encoding"),
                                    _("The default encoding used to display text."),
                                    "iso-8859-1",
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_CURSIVE_FONT_FAMILY,
                                    g_param_spec_string(
                                    "cursive-font-family",
                                    _("Cursive Font Family"),
                                    _("The default Cursive font family used to display text."),
                                    "serif",
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_DEFAULT_FONT_FAMILY,
                                    g_param_spec_string(
                                    "default-font-family",
                                    _("Default Font Family"),
                                    _("The default font family used to display text."),
                                    "sans-serif",
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_FANTASY_FONT_FAMILY,
                                    g_param_spec_string(
                                    "fantasy-font-family",
                                    _("Fantasy Font Family"),
                                    _("The default Fantasy font family used to display text."),
                                    "serif",
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_MONOSPACE_FONT_FAMILY,
                                    g_param_spec_string(
                                    "monospace-font-family",
                                    _("Monospace Font Family"),
                                    _("The default font family used to display monospace text."),
                                    "monospace",
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_SANS_SERIF_FONT_FAMILY,
                                    g_param_spec_string(
                                    "sans-serif-font-family",
                                    _("Sans Serif Font Family"),
                                    _("The default Sans Serif font family used to display text."),
                                    "sans-serif",
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_SERIF_FONT_FAMILY,
                                    g_param_spec_string(
                                    "serif-font-family",
                                    _("Serif Font Family"),
                                    _("The default Serif font family used to display text."),
                                    "serif",
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_DEFAULT_FONT_SIZE,
                                    g_param_spec_int(
                                    "default-font-size",
                                    _("Default Font Size"),
                                    _("The default font size used to display text."),
                                    5, G_MAXINT, 12,
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_DEFAULT_MONOSPACE_FONT_SIZE,
                                    g_param_spec_int(
                                    "default-monospace-font-size",
                                    _("Default Monospace Font Size"),
                                    _("The default font size used to display monospace text."),
                                    5, G_MAXINT, 10,
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_MINIMUM_FONT_SIZE,
                                    g_param_spec_int(
                                    "minimum-font-size",
                                    _("Minimum Font Size"),
                                    _("The minimum font size used to display text."),
                                    0, G_MAXINT, 5,
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_MINIMUM_LOGICAL_FONT_SIZE,
                                    g_param_spec_int(
                                    "minimum-logical-font-size",
                                    _("Minimum Logical Font Size"),
                                    _("The minimum logical font size used to display text."),
                                    1, G_MAXINT, 5,
                                    flags));

    /**
    * WebKitWebSettings:enforce-96-dpi:
    *
    * Enforce a resolution of 96 DPI. This is meant for compatibility
    * with web pages which cope badly with different screen resolutions
    * and for automated testing.
    * Web browsers and applications that typically display arbitrary
    * content from the web should provide a preference for this.
    *
    * Since: 1.0.3
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENFORCE_96_DPI,
                                    g_param_spec_boolean(
                                    "enforce-96-dpi",
                                    _("Enforce 96 DPI"),
                                    _("Enforce a resolution of 96 DPI"),
                                    FALSE,
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_AUTO_LOAD_IMAGES,
                                    g_param_spec_boolean(
                                    "auto-load-images",
                                    _("Auto Load Images"),
                                    _("Load images automatically."),
                                    TRUE,
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_AUTO_SHRINK_IMAGES,
                                    g_param_spec_boolean(
                                    "auto-shrink-images",
                                    _("Auto Shrink Images"),
                                    _("Automatically shrink standalone images to fit."),
                                    TRUE,
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_RESPECT_IMAGE_ORIENTATION,
                                    g_param_spec_boolean(
                                    "respect-image-orientation",
                                    _("Respect Image Orientation"),
                                    _("Whether WebKit should respect image orientation."),
                                    FALSE,
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_PRINT_BACKGROUNDS,
                                    g_param_spec_boolean(
                                    "print-backgrounds",
                                    _("Print Backgrounds"),
                                    _("Whether background images should be printed."),
                                    TRUE,
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_SCRIPTS,
                                    g_param_spec_boolean(
                                    "enable-scripts",
                                    _("Enable Scripts"),
                                    _("Enable embedded scripting languages."),
                                    TRUE,
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_PLUGINS,
                                    g_param_spec_boolean(
                                    "enable-plugins",
                                    _("Enable Plugins"),
                                    _("Enable embedded plugin objects."),
                                    TRUE,
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_RESIZABLE_TEXT_AREAS,
                                    g_param_spec_boolean(
                                    "resizable-text-areas",
                                    _("Resizable Text Areas"),
                                    _("Whether text areas are resizable."),
                                    TRUE,
                                    flags));

    g_object_class_install_property(gobject_class,
                                    PROP_USER_STYLESHEET_URI,
                                    g_param_spec_string("user-stylesheet-uri",
                                    _("User Stylesheet URI"),
                                    _("The URI of a stylesheet that is applied to every page."),
                                    0,
                                    flags));

    /**
    * WebKitWebSettings:zoom-step:
    *
    * The value by which the zoom level is changed when zooming in or out.
    *
    * Since: 1.0.1
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ZOOM_STEP,
                                    g_param_spec_float(
                                    "zoom-step",
                                    _("Zoom Stepping Value"),
                                    _("The value by which the zoom level is changed when zooming in or out."),
                                    0.0f, G_MAXFLOAT, 0.1f,
                                    flags));

    /**
    * WebKitWebSettings:enable-developer-extras:
    *
    * Whether developer extensions should be enabled. This enables,
    * for now, the Web Inspector, which can be controlled using the
    * #WebKitWebInspector instance held by the #WebKitWebView this
    * setting is enabled for.
    *
    * Since: 1.0.3
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_DEVELOPER_EXTRAS,
                                    g_param_spec_boolean(
                                    "enable-developer-extras",
                                    _("Enable Developer Extras"),
                                    _("Enables special extensions that help developers"),
                                    FALSE,
                                    flags));

    /**
    * WebKitWebSettings:enable-private-browsing:
    *
    * Whether to enable private browsing mode. Private browsing mode prevents
    * WebKit from updating the global history and storing any session
    * information e.g., on-disk cache, as well as suppressing any messages
    * from being printed into the (javascript) console.
    *
    * This is currently experimental for WebKitGtk.
    *
    * Since: 1.1.2
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_PRIVATE_BROWSING,
                                    g_param_spec_boolean(
                                    "enable-private-browsing",
                                    _("Enable Private Browsing"),
                                    _("Enables private browsing mode"),
                                    FALSE,
                                    flags));

    /**
    * WebKitWebSettings:enable-spell-checking:
    *
    * Whether to enable spell checking while typing.
    *
    * Since: 1.1.6
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_SPELL_CHECKING,
                                    g_param_spec_boolean(
                                    "enable-spell-checking",
                                    _("Enable Spell Checking"),
                                    _("Enables spell checking while typing"),
                                    FALSE,
                                    flags));

    /**
    * WebKitWebSettings:spell-checking-languages:
    *
    * The languages to be used for spell checking, separated by commas.
    *
    * The locale string typically is in the form lang_COUNTRY, where lang
    * is an ISO-639 language code, and COUNTRY is an ISO-3166 country code.
    * For instance, sv_FI for Swedish as written in Finland or pt_BR
    * for Portuguese as written in Brazil.
    *
    * If no value is specified then the value returned by
    * gtk_get_default_language will be used.
    *
    * Since: 1.1.6
    */
    g_object_class_install_property(gobject_class,
                                    PROP_SPELL_CHECKING_LANGUAGES,
                                    g_param_spec_string(
                                    "spell-checking-languages",
                                    _("Languages to use for spell checking"),
                                    _("Comma separated list of languages to use for spell checking"),
                                    0,
                                    flags));

    /**
    * WebKitWebSettings:enable-caret-browsing:
    *
    * Whether to enable caret browsing mode.
    *
    * Since: 1.1.6
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_CARET_BROWSING,
                                    g_param_spec_boolean("enable-caret-browsing",
                                                         _("Enable Caret Browsing"),
                                                         _("Whether to enable accessibility enhanced keyboard navigation"),
                                                         FALSE,
                                                         flags));
    /**
    * WebKitWebSettings:enable-html5-database:
    *
    * Whether to enable HTML5 client-side SQL database support. Client-side
    * SQL database allows web pages to store structured data and be able to
    * use SQL to manipulate that data asynchronously.
    *
    * Since: 1.1.8
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_HTML5_DATABASE,
                                    g_param_spec_boolean("enable-html5-database",
                                                         _("Enable HTML5 Database"),
                                                         _("Whether to enable HTML5 database support"),
                                                         TRUE,
                                                         flags));

    /**
    * WebKitWebSettings:enable-html5-local-storage:
    *
    * Whether to enable HTML5 localStorage support. localStorage provides
    * simple synchronous storage access.
    *
    * Since: 1.1.8
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_HTML5_LOCAL_STORAGE,
                                    g_param_spec_boolean("enable-html5-local-storage",
                                                         _("Enable HTML5 Local Storage"),
                                                         _("Whether to enable HTML5 Local Storage support"),
                                                         TRUE,
                                                         flags));
    /**
    * WebKitWebSettings:html5-local-storage-database-path:
    *
    * Path to store persistent HTML5 localStorage databases, which are enabled by
    * "enable-html5-local-storage". The default path is $XDG_DATA_HOME/webkit/databases/.
    *
    * Since: 1.5.2
    */
    GOwnPtr<gchar> localStoragePath(g_build_filename(g_get_user_data_dir(), "webkit", "databases", NULL));
    g_object_class_install_property(gobject_class,
                                    PROP_HTML5_LOCAL_STORAGE_DATABASE_PATH,
                                    g_param_spec_string("html5-local-storage-database-path",
                                                        _("Local Storage Database Path"),
                                                        _("The path to where HTML5 Local Storage databases are stored."),
                                                        localStoragePath.get(),
                                                        flags));
    /**
    * WebKitWebSettings:enable-xss-auditor:
    *
    * Whether to enable the XSS Auditor. This feature filters some kinds of
    * reflective XSS attacks on vulnerable web sites.
    *
    * Since: 1.1.11
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_XSS_AUDITOR,
                                    g_param_spec_boolean("enable-xss-auditor",
                                                         _("Enable XSS Auditor"),
                                                         _("Whether to enable the XSS auditor"),
                                                         TRUE,
                                                         flags));
    /**
    * WebKitWebSettings:enable-spatial-navigation:
    *
    * Whether to enable the Spatial Navigation. This feature consists in the ability
    * to navigate between focusable elements in a Web page, such as hyperlinks and
    * form controls, by using Left, Right, Up and Down arrow keys. For example, if
    * an user presses the Right key, heuristics determine whether there is an element
    * he might be trying to reach towards the right, and if there are multiple elements,
    * which element he probably wants.
    *
    * Since: 1.1.23
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_SPATIAL_NAVIGATION,
                                    g_param_spec_boolean("enable-spatial-navigation",
                                                         _("Enable Spatial Navigation"),
                                                         _("Whether to enable Spatial Navigation"),
                                                         FALSE,
                                                         flags));
    /**
    * WebKitWebSettings:enable-frame-flattening:
    *
    * Whether to enable the Frame Flattening. With this setting each subframe is expanded
    * to its contents, which will flatten all the frames to become one scrollable page.
    * On touch devices, it is desired to not have any scrollable sub parts of the page as
    * it results in a confusing user experience, with scrolling sometimes scrolling sub parts
    * and at other times scrolling the page itself. For this reason iframes and framesets are
    * barely usable on touch devices.
    *
    * Since: 1.3.5
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_FRAME_FLATTENING,
                                    g_param_spec_boolean("enable-frame-flattening",
                                                         _("Enable Frame Flattening"),
                                                         _("Whether to enable Frame Flattening"),
                                                         FALSE,
                                                         flags));
    /**
     * WebKitWebSettings:user-agent:
     *
     * The User-Agent string used by WebKitGtk.
     *
     * This will return a default User-Agent string if a custom string wasn't
     * provided by the application. Setting this property to a NULL value or
     * an empty string will result in the User-Agent string being reset to the
     * default value.
     *
     * Since: 1.1.11
     */
    g_object_class_install_property(gobject_class, PROP_USER_AGENT,
                                    g_param_spec_string("user-agent",
                                                        _("User Agent"),
                                                        _("The User-Agent string used by WebKitGtk"),
                                                        "", // An empty string means the default user-agent.
                                                        flags));

    /**
    * WebKitWebSettings:javascript-can-open-windows-automatically:
    *
    * Whether JavaScript can open popup windows automatically without user
    * intervention.
    *
    * Since: 1.1.11
    */
    g_object_class_install_property(gobject_class,
                                    PROP_JAVASCRIPT_CAN_OPEN_WINDOWS_AUTOMATICALLY,
                                    g_param_spec_boolean("javascript-can-open-windows-automatically",
                                                         _("JavaScript can open windows automatically"),
                                                         _("Whether JavaScript can open windows automatically"),
                                                         FALSE,
                                                         flags));

    /**
    * WebKitWebSettings:javascript-can-access-clipboard:
    *
    * Whether JavaScript can access Clipboard.
    *
    * Since: 1.3.0
    */
    g_object_class_install_property(gobject_class,
                                    PROP_JAVASCRIPT_CAN_ACCESS_CLIPBOARD,
                                    g_param_spec_boolean("javascript-can-access-clipboard",
                                                         _("JavaScript can access Clipboard"),
                                                         _("Whether JavaScript can access Clipboard"),
                                                         FALSE,
                                                         flags));

    /**
    * WebKitWebSettings:enable-offline-web-application-cache:
    *
    * Whether to enable HTML5 offline web application cache support. Offline
    * Web Application Cache ensures web applications are available even when
    * the user is not connected to the network.
    *
    * Since: 1.1.13
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_OFFLINE_WEB_APPLICATION_CACHE,
                                    g_param_spec_boolean("enable-offline-web-application-cache",
                                                         _("Enable offline web application cache"),
                                                         _("Whether to enable offline web application cache"),
                                                         TRUE,
                                                         flags));


    /**
    * WebKitWebSettings:editing-behavior:
    *
    * This setting controls various editing behaviors that differ
    * between platforms and that have been combined in two groups,
    * 'Mac' and 'Windows'. Some examples:
    *
    *  1) Clicking below the last line of an editable area puts the
    * caret at the end of the last line on Mac, but in the middle of
    * the last line on Windows.
    *
    *  2) Pushing down the arrow key on the last line puts the caret
    *  at the end of the last line on Mac, but does nothing on
    *  Windows. A similar case exists on the top line.
    *
    * Since: 1.1.13
    */
    g_object_class_install_property(gobject_class,
                                    PROP_EDITING_BEHAVIOR,
                                    g_param_spec_enum("editing-behavior",
                                                      _("Editing behavior"),
                                                      _("The behavior mode to use in editing mode"),
                                                      WEBKIT_TYPE_EDITING_BEHAVIOR,
                                                      WEBKIT_EDITING_BEHAVIOR_UNIX,
                                                      flags));

    /**
     * WebKitWebSettings:enable-universal-access-from-file-uris:
     *
     * Whether to allow files loaded through file:// URIs universal access to
     * all pages.
     *
     * Since: 1.1.13
     */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_UNIVERSAL_ACCESS_FROM_FILE_URIS,
                                    g_param_spec_boolean("enable-universal-access-from-file-uris",
                                                         _("Enable universal access from file URIs"),
                                                         _("Whether to allow universal access from file URIs"),
                                                         FALSE,
                                                         flags));

    /**
     * WebKitWebSettings:enable-dom-paste:
     *
     * Whether to enable DOM paste. If set to %TRUE, document.execCommand("Paste")
     * will correctly execute and paste content of the clipboard.
     *
     * Since: 1.1.16
     */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_DOM_PASTE,
                                    g_param_spec_boolean("enable-dom-paste",
                                                         _("Enable DOM paste"),
                                                         _("Whether to enable DOM paste"),
                                                         FALSE,
                                                         flags));
    /**
    * WebKitWebSettings:tab-key-cycles-through-elements:
    *
    * Whether the tab key cycles through elements on the page.
    *
    * If @flag is %TRUE, pressing the tab key will focus the next element in
    * the @web_view. If @flag is %FALSE, the @web_view will interpret tab
    * key presses as normal key presses. If the selected element is editable, the
    * tab key will cause the insertion of a tab character.
    *
    * Since: 1.1.17
    */
    g_object_class_install_property(gobject_class,
                                    PROP_TAB_KEY_CYCLES_THROUGH_ELEMENTS,
                                    g_param_spec_boolean("tab-key-cycles-through-elements",
                                                         _("Tab key cycles through elements"),
                                                         _("Whether the tab key cycles through elements on the page."),
                                                         TRUE,
                                                         flags));

    /**
     * WebKitWebSettings:enable-default-context-menu:
     *
     * Whether right-clicks should be handled automatically to create,
     * and display the context menu. Turning this off will make
     * WebKitGTK+ not emit the populate-popup signal. Notice that the
     * default button press event handler may still handle right
     * clicks for other reasons, such as in-page context menus, or
     * right-clicks that are handled by the page itself.
     *
     * Since: 1.1.18
     *
     * Deprecated: 1.10: Use #WebKitWebView::context-menu signal instead.
     */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_DEFAULT_CONTEXT_MENU,
                                    g_param_spec_boolean(
                                    "enable-default-context-menu",
                                    _("Enable Default Context Menu"),
                                    _("Enables the handling of right-clicks for the creation of the default context menu"),
                                    TRUE,
                                    flags));

    /**
     * WebKitWebSettings::enable-site-specific-quirks:
     *
     * Whether to turn on site-specific hacks.  Turning this on will
     * tell WebKitGTK+ to use some site-specific workarounds for
     * better web compatibility.  For example, older versions of
     * MediaWiki will incorrectly send WebKit a css file with KHTML
     * workarounds.  By turning on site-specific quirks, WebKit will
     * special-case this and other cases to make the sites work.
     *
     * Since: 1.1.18
     */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_SITE_SPECIFIC_QUIRKS,
                                    g_param_spec_boolean(
                                    "enable-site-specific-quirks",
                                    _("Enable Site Specific Quirks"),
                                    _("Enables the site-specific compatibility workarounds"),
                                    FALSE,
                                    flags));

    /**
    * WebKitWebSettings:enable-page-cache:
    *
    * Enable or disable the page cache. Disabling the page cache is
    * generally only useful for special circumstances like low-memory
    * scenarios or special purpose applications like static HTML
    * viewers. This setting only controls the Page Cache, this cache
    * is different than the disk-based or memory-based traditional
    * resource caches, its point is to make going back and forth
    * between pages much faster. For details about the different types
    * of caches and their purposes see:
    * http://webkit.org/blog/427/webkit-page-cache-i-the-basics/
    *
    * Since: 1.1.18
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_PAGE_CACHE,
                                    g_param_spec_boolean("enable-page-cache",
                                                         _("Enable page cache"),
                                                         _("Whether the page cache should be used"),
                                                         FALSE,
                                                         flags));

    /**
    * WebKitWebSettings:auto-resize-window:
    *
    * Web pages can request to modify the size and position of the
    * window containing the #WebKitWebView through various DOM methods
    * (resizeTo, moveTo, resizeBy, moveBy). By default WebKit will not
    * honor this requests, but you can set this property to %TRUE if
    * you'd like it to do so. If you wish to handle this manually, you
    * can connect to the notify signal for the
    * #WebKitWebWindowFeatures of your #WebKitWebView.
    *
    * Since: 1.1.22
    */
    g_object_class_install_property(gobject_class,
                                    PROP_AUTO_RESIZE_WINDOW,
                                    g_param_spec_boolean("auto-resize-window",
                                                         _("Auto Resize Window"),
                                                         _("Automatically resize the toplevel window when a page requests it"),
                                                         FALSE,
                                                         flags));

    /**
     * WebKitWebSettings:enable-file-access-from-file-uris:
     *
     * Boolean property to control file access for file:// URIs. If this
     * option is enabled every file:// will have its own security unique domain.
     *
     * Since: 1.1.22
     */
     g_object_class_install_property(gobject_class,
                                     PROP_ENABLE_FILE_ACCESS_FROM_FILE_URIS,
                                     g_param_spec_boolean("enable-file-access-from-file-uris",
                                                          "Enable file access from file URIs",
                                                          "Controls file access for file:// URIs.",
                                                          FALSE,
                                                          flags));

   /**
    * WebKitWebSettings:enable-java-applet:
    *
    * Enable or disable support for the Java &lt;applet&gt; tag. Keep in
    * mind that Java content can be still shown in the page through
    * &lt;object&gt; or &lt;embed&gt;, which are the preferred tags for this task.
    *
    * Since: 1.1.22
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_JAVA_APPLET,
                                    g_param_spec_boolean("enable-java-applet",
                                                         _("Enable Java Applet"),
                                                         _("Whether Java Applet support through <applet> should be enabled"),
                                                         TRUE,
                                                         flags));

    /**
    * WebKitWebSettings:enable-hyperlink-auditing:
    *
    * Enable or disable support for &lt;a ping&gt;.
    *
    * Since: 1.2.5
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_HYPERLINK_AUDITING,
                                    g_param_spec_boolean("enable-hyperlink-auditing",
                                                         _("Enable Hyperlink Auditing"),
                                                         _("Whether <a ping> should be able to send pings"),
                                                         FALSE,
                                                         flags));

    /* Undocumented for now */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_FULLSCREEN,
                                    g_param_spec_boolean("enable-fullscreen",
                                                         _("Enable Fullscreen"),
                                                         _("Whether the Mozilla style API should be enabled."),
                                                         FALSE,
                                                         flags));
    /**
    * WebKitWebSettings:enable-webgl:
    *
    * Enable or disable support for WebGL on pages. WebGL is an experimental
    * proposal for allowing web pages to use OpenGL ES-like calls directly. The
    * standard is currently a work-in-progress by the Khronos Group.
    *
    * Since: 1.3.14
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_WEBGL,
                                    g_param_spec_boolean("enable-webgl",
                                                         _("Enable WebGL"),
                                                         _("Whether WebGL content should be rendered"),
                                                         FALSE,
                                                         flags));

    /**
    * WebKitWebSettings:enable-accelerated-compositing:
    *
    * Enable or disable support for accelerated compositing on pages. Accelerated
    * compositing uses the GPU to render animations on pages smoothly and also allows
    * proper rendering of 3D CSS transforms.
    *
    * Since: 1.7.5
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_ACCELERATED_COMPOSITING,
                                    g_param_spec_boolean("enable-accelerated-compositing",
                                                         _("Enable accelerated compositing"),
                                                         _("Whether accelerated compositing should be enabled"),
                                                         FALSE,
                                                         flags));
    /**
    * WebKitWebSettings:enable-webaudio:
    *
    * Enable or disable support for WebAudio on pages. WebAudio is an
    * experimental proposal for allowing web pages to generate Audio
    * WAVE data from JavaScript. The standard is currently a
    * work-in-progress by the W3C Audio Working Group.
    *
    * See also https://dvcs.w3.org/hg/audio/raw-file/tip/webaudio/specification.html
    *
    * Since: TODO
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_WEB_AUDIO,
                                    g_param_spec_boolean("enable-webaudio",
                                                         _("Enable WebAudio"),
                                                         _("Whether WebAudio content should be handled"),
                                                         FALSE,
                                                         flags));


    /**
    * WebKitWebSettings:enable-dns-prefetching:
    *
    * Whether webkit prefetches domain names.  This is a separate knob from private browsing.
    * Whether private browsing should set this or not is up for debate, for now it doesn't.
    *
    * Since: 1.3.13.
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_DNS_PREFETCHING,
                                    g_param_spec_boolean("enable-dns-prefetching",
                                                         _("WebKit prefetches domain names"),
                                                         _("Whether WebKit prefetches domain names"),
                                                         TRUE,
                                                         flags));
    /**
    * WebKitWebSettings:enable-media-stream:
    *
    * Enable or disable support for Media Stream on pages. Media Stream is
    * an experimental proposal for allowing web pages to access local video and
    * audio input devices.  The standard is currently a work-in-progress as part
    * of the Web Applications 1.0 specification from WHATWG.
    *
    * See also http://www.w3.org/TR/mediacapture-streams/
    *
    * Since: 1.10.0
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_MEDIA_STREAM,
                                    g_param_spec_boolean("enable-media-stream",
                                                         _("Enable Media Stream"),
                                                         _("Whether Media Stream should be enabled"),
                                                         FALSE,
                                                         flags));

    /**
    * WebKitWebSettings:enable-smooth-scrolling:
    *
    * Enable or disable support for smooth scrolling. The current implementation relies upon
    * CPU work to produce a smooth scrolling experience.
    *
    * Since: 1.9.0
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_SMOOTH_SCROLLING,
                                    g_param_spec_boolean("enable-smooth-scrolling",
                                                         _("Enable smooth scrolling"),
                                                         _("Whether to enable smooth scrolling"),
                                                         FALSE,
                                                         flags));

    /**
    * WebKitWebSettings:media-playback-requires-user-gesture:
    *
    * Whether a user gesture (such as clicking the play button) would
    * be required to start media playback or load media. This is off
    * by default, so media playback could start automatically. Setting
    * it on requires a gesture by the user to start playback, or to load
    * the media.
    *
    * Since: 1.10.0
    */
    g_object_class_install_property(gobject_class,
                                    PROP_MEDIA_PLAYBACK_REQUIRES_USER_GESTURE,
                                    g_param_spec_boolean("media-playback-requires-user-gesture",
                                                         _("Media playback requires user gesture"),
                                                         _("Whether media playback requires user gesture"),
                                                         FALSE,
                                                         flags));

    /**
    * WebKitWebSettings:media-playback-allows-inline:
    *
    * Whether media playback is full-screen only or inline playback is allowed.
    * This is true by default, so media playback can be inline. Setting it to
    * false allows specifying that media playback should be always fullscreen.
    *
    * Since: 1.10.0
    */
    g_object_class_install_property(gobject_class,
                                    PROP_MEDIA_PLAYBACK_ALLOWS_INLINE,
                                    g_param_spec_boolean("media-playback-allows-inline",
                                                         _("Media playback allows inline"),
                                                         _("Whether media playback allows inline"),
                                                         TRUE,
                                                         flags));

    /**
    * WebKitWebSettings:enable-css-shaders:
    *
    * Enable or disable support for css shaders (css custom filters).
    * Accelerated compositing needs to be enabled at compile time, but needs
    * not be enabled at runtime.
    *
    * See also https://dvcs.w3.org/hg/FXTF/raw-file/tip/custom/index.html
    *
    * Since: 2.0
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ENABLE_CSS_SHADERS,
                                    g_param_spec_boolean("enable-css-shaders",
                                                         _("Enable CSS shaders"),
                                                         _("Whether to enable css shaders"),
                                                         FALSE,
                                                         flags));

    /**
    * WebKitWebSettings:enable-display-of-insecure-content:
    *
    * Whether pages loaded via HTTPS should load subresources such as
    * images and frames from non-HTTPS URLs.
    *
    * Since: 2.0
    */
    g_object_class_install_property(gobject_class,
        PROP_ENABLE_DISPLAY_OF_INSECURE_CONTENT,
        g_param_spec_boolean("enable-display-of-insecure-content",
            _("Enable display of insecure content"),
            _("Whether non-HTTPS resources can display on HTTPS pages."),
            TRUE,
            flags));

    /**
    * WebKitWebSettings:enable-running-of-insecure-content:
    *
    * Whether pages loaded via HTTPS should run subresources such as
    * CSS, scripts, and plugins from non-HTTPS URLs.
    *
    * Since: 2.0
    */
    g_object_class_install_property(gobject_class,
        PROP_ENABLE_RUNNING_OF_INSECURE_CONTENT,
        g_param_spec_boolean("enable-running-of-insecure-content",
            _("Enable running of insecure content"),
            _("Whether non-HTTPS resources can run on HTTPS pages."),
            TRUE,
            flags));
}

static void webkit_web_settings_init(WebKitWebSettings* web_settings)
{
    web_settings->priv = new WebKitWebSettingsPrivate;
}

static void webkit_web_settings_finalize(GObject* object)
{
    delete WEBKIT_WEB_SETTINGS(object)->priv;
    G_OBJECT_CLASS(webkit_web_settings_parent_class)->finalize(object);
}

static void webkit_web_settings_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    WebKitWebSettings* web_settings = WEBKIT_WEB_SETTINGS(object);
    WebKitWebSettingsPrivate* priv = web_settings->priv;

    switch(prop_id) {
    case PROP_DEFAULT_ENCODING:
        priv->defaultEncoding = g_value_get_string(value);
        break;
    case PROP_CURSIVE_FONT_FAMILY:
        priv->cursiveFontFamily = g_value_get_string(value);
        break;
    case PROP_DEFAULT_FONT_FAMILY:
        priv->defaultFontFamily = g_value_get_string(value);
        break;
    case PROP_FANTASY_FONT_FAMILY:
        priv->fantasyFontFamily = g_value_get_string(value);
        break;
    case PROP_MONOSPACE_FONT_FAMILY:
        priv->monospaceFontFamily = g_value_get_string(value);
        break;
    case PROP_SANS_SERIF_FONT_FAMILY:
        priv->sansSerifFontFamily = g_value_get_string(value);
        break;
    case PROP_SERIF_FONT_FAMILY:
        priv->serifFontFamily = g_value_get_string(value);
        break;
    case PROP_DEFAULT_FONT_SIZE:
        priv->defaultFontSize = g_value_get_int(value);
        break;
    case PROP_DEFAULT_MONOSPACE_FONT_SIZE:
        priv->defaultMonospaceFontSize = g_value_get_int(value);
        break;
    case PROP_MINIMUM_FONT_SIZE:
        priv->minimumFontSize = g_value_get_int(value);
        break;
    case PROP_MINIMUM_LOGICAL_FONT_SIZE:
        priv->minimumLogicalFontSize = g_value_get_int(value);
        break;
    case PROP_ENFORCE_96_DPI:
        priv->enforce96DPI = g_value_get_boolean(value);
        break;
    case PROP_AUTO_LOAD_IMAGES:
        priv->autoLoadImages = g_value_get_boolean(value);
        break;
    case PROP_AUTO_SHRINK_IMAGES:
        priv->autoShrinkImages = g_value_get_boolean(value);
        break;
    case PROP_RESPECT_IMAGE_ORIENTATION:
        priv->respectImageOrientation = g_value_get_boolean(value);
        break;
    case PROP_PRINT_BACKGROUNDS:
        priv->printBackgrounds = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_SCRIPTS:
        priv->enableScripts = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_PLUGINS:
        priv->enablePlugins = g_value_get_boolean(value);
        break;
    case PROP_RESIZABLE_TEXT_AREAS:
        priv->resizableTextAreas = g_value_get_boolean(value);
        break;
    case PROP_USER_STYLESHEET_URI:
        priv->userStylesheetURI = g_value_get_string(value);
        break;
    case PROP_ZOOM_STEP:
        priv->zoomStep = g_value_get_float(value);
        break;
    case PROP_ENABLE_DEVELOPER_EXTRAS:
        priv->enableDeveloperExtras = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_PRIVATE_BROWSING:
        priv->enablePrivateBrowsing = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_CARET_BROWSING:
        priv->enableCaretBrowsing = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_HTML5_DATABASE:
        priv->enableHTML5Database = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_HTML5_LOCAL_STORAGE:
        priv->enableHTML5LocalStorage = g_value_get_boolean(value);
        break;
    case PROP_HTML5_LOCAL_STORAGE_DATABASE_PATH:
        priv->html5LocalStorageDatabasePath = g_value_get_string(value);
        break;
    case PROP_ENABLE_SPELL_CHECKING:
        priv->enableSpellChecking = g_value_get_boolean(value);
        break;
    case PROP_SPELL_CHECKING_LANGUAGES:
        priv->spellCheckingLanguages = g_value_get_string(value);
        break;
    case PROP_ENABLE_XSS_AUDITOR:
        priv->enableXSSAuditor = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_SPATIAL_NAVIGATION:
        priv->enableSpatialNavigation = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_FRAME_FLATTENING:
        priv->enableFrameFlattening = g_value_get_boolean(value);
        break;
    case PROP_USER_AGENT:
        if (!g_value_get_string(value) || !strlen(g_value_get_string(value)))
            priv->userAgent = standardUserAgent().utf8();
        else
            priv->userAgent = g_value_get_string(value);
        break;
    case PROP_JAVASCRIPT_CAN_OPEN_WINDOWS_AUTOMATICALLY:
        priv->javascriptCanOpenWindowsAutomatically = g_value_get_boolean(value);
        break;
    case PROP_JAVASCRIPT_CAN_ACCESS_CLIPBOARD:
        priv->javascriptCanAccessClipboard = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_OFFLINE_WEB_APPLICATION_CACHE:
        priv->enableOfflineWebApplicationCache = g_value_get_boolean(value);
        break;
    case PROP_EDITING_BEHAVIOR:
        priv->editingBehavior = static_cast<WebKitEditingBehavior>(g_value_get_enum(value));
        break;
    case PROP_ENABLE_UNIVERSAL_ACCESS_FROM_FILE_URIS:
        priv->enableUniversalAccessFromFileURIs = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_FILE_ACCESS_FROM_FILE_URIS:
        priv->enableFileAccessFromFileURIs = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_DOM_PASTE:
        priv->enableDOMPaste = g_value_get_boolean(value);
        break;
    case PROP_TAB_KEY_CYCLES_THROUGH_ELEMENTS:
        priv->tabKeyCyclesThroughElements = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_DEFAULT_CONTEXT_MENU:
        priv->enableDefaultContextMenu = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_SITE_SPECIFIC_QUIRKS:
        priv->enableSiteSpecificQuirks = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_PAGE_CACHE:
        priv->enablePageCache = g_value_get_boolean(value);
        break;
    case PROP_AUTO_RESIZE_WINDOW:
        priv->autoResizeWindow = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_JAVA_APPLET:
        priv->enableJavaApplet = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_HYPERLINK_AUDITING:
        priv->enableHyperlinkAuditing = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_FULLSCREEN:
        priv->enableFullscreen = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_DNS_PREFETCHING:
        priv->enableDNSPrefetching = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_WEBGL:
        priv->enableWebgl = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_MEDIA_STREAM:
        priv->enableMediaStream = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_WEB_AUDIO:
        priv->enableWebAudio = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_ACCELERATED_COMPOSITING:
        priv->enableAcceleratedCompositing = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_SMOOTH_SCROLLING:
        priv->enableSmoothScrolling = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_CSS_SHADERS:
        priv->enableCSSShaders = g_value_get_boolean(value);
        break;
    case PROP_MEDIA_PLAYBACK_REQUIRES_USER_GESTURE:
        priv->mediaPlaybackRequiresUserGesture = g_value_get_boolean(value);
        break;
    case PROP_MEDIA_PLAYBACK_ALLOWS_INLINE:
        priv->mediaPlaybackAllowsInline = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_DISPLAY_OF_INSECURE_CONTENT:
        priv->enableDisplayOfInsecureContent = g_value_get_boolean(value);
        break;
    case PROP_ENABLE_RUNNING_OF_INSECURE_CONTENT:
        priv->enableRunningOfInsecureContent = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void webkit_web_settings_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    WebKitWebSettings* web_settings = WEBKIT_WEB_SETTINGS(object);
    WebKitWebSettingsPrivate* priv = web_settings->priv;

    switch (prop_id) {
    case PROP_DEFAULT_ENCODING:
        g_value_set_string(value, priv->defaultEncoding.data());
        break;
    case PROP_CURSIVE_FONT_FAMILY:
        g_value_set_string(value, priv->cursiveFontFamily.data());
        break;
    case PROP_DEFAULT_FONT_FAMILY:
        g_value_set_string(value, priv->defaultFontFamily.data());
        break;
    case PROP_FANTASY_FONT_FAMILY:
        g_value_set_string(value, priv->fantasyFontFamily.data());
        break;
    case PROP_MONOSPACE_FONT_FAMILY:
        g_value_set_string(value, priv->monospaceFontFamily.data());
        break;
    case PROP_SANS_SERIF_FONT_FAMILY:
        g_value_set_string(value, priv->sansSerifFontFamily.data());
        break;
    case PROP_SERIF_FONT_FAMILY:
        g_value_set_string(value, priv->serifFontFamily.data());
        break;
    case PROP_DEFAULT_FONT_SIZE:
        g_value_set_int(value, priv->defaultFontSize);
        break;
    case PROP_DEFAULT_MONOSPACE_FONT_SIZE:
        g_value_set_int(value, priv->defaultMonospaceFontSize);
        break;
    case PROP_MINIMUM_FONT_SIZE:
        g_value_set_int(value, priv->minimumFontSize);
        break;
    case PROP_MINIMUM_LOGICAL_FONT_SIZE:
        g_value_set_int(value, priv->minimumLogicalFontSize);
        break;
    case PROP_ENFORCE_96_DPI:
        g_value_set_boolean(value, priv->enforce96DPI);
        break;
    case PROP_AUTO_LOAD_IMAGES:
        g_value_set_boolean(value, priv->autoLoadImages);
        break;
    case PROP_AUTO_SHRINK_IMAGES:
        g_value_set_boolean(value, priv->autoShrinkImages);
        break;
    case PROP_RESPECT_IMAGE_ORIENTATION:
        g_value_set_boolean(value, priv->respectImageOrientation);
        break;
    case PROP_PRINT_BACKGROUNDS:
        g_value_set_boolean(value, priv->printBackgrounds);
        break;
    case PROP_ENABLE_SCRIPTS:
        g_value_set_boolean(value, priv->enableScripts);
        break;
    case PROP_ENABLE_PLUGINS:
        g_value_set_boolean(value, priv->enablePlugins);
        break;
    case PROP_RESIZABLE_TEXT_AREAS:
        g_value_set_boolean(value, priv->resizableTextAreas);
        break;
    case PROP_USER_STYLESHEET_URI:
        g_value_set_string(value, priv->userStylesheetURI.data());
        break;
    case PROP_ZOOM_STEP:
        g_value_set_float(value, priv->zoomStep);
        break;
    case PROP_ENABLE_DEVELOPER_EXTRAS:
        g_value_set_boolean(value, priv->enableDeveloperExtras);
        break;
    case PROP_ENABLE_PRIVATE_BROWSING:
        g_value_set_boolean(value, priv->enablePrivateBrowsing);
        break;
    case PROP_ENABLE_CARET_BROWSING:
        g_value_set_boolean(value, priv->enableCaretBrowsing);
        break;
    case PROP_ENABLE_HTML5_DATABASE:
        g_value_set_boolean(value, priv->enableHTML5Database);
        break;
    case PROP_ENABLE_HTML5_LOCAL_STORAGE:
        g_value_set_boolean(value, priv->enableHTML5LocalStorage);
        break;
    case PROP_HTML5_LOCAL_STORAGE_DATABASE_PATH:
        g_value_set_string(value, priv->html5LocalStorageDatabasePath.data());
        break;
    case PROP_ENABLE_SPELL_CHECKING:
        g_value_set_boolean(value, priv->enableSpellChecking);
        break;
    case PROP_SPELL_CHECKING_LANGUAGES:
        g_value_set_string(value, priv->spellCheckingLanguages.data());
        break;
    case PROP_ENABLE_XSS_AUDITOR:
        g_value_set_boolean(value, priv->enableXSSAuditor);
        break;
    case PROP_ENABLE_SPATIAL_NAVIGATION:
        g_value_set_boolean(value, priv->enableSpatialNavigation);
        break;
    case PROP_ENABLE_FRAME_FLATTENING:
        g_value_set_boolean(value, priv->enableFrameFlattening);
        break;
    case PROP_USER_AGENT:
        g_value_set_string(value, priv->userAgent.data());
        break;
    case PROP_JAVASCRIPT_CAN_OPEN_WINDOWS_AUTOMATICALLY:
        g_value_set_boolean(value, priv->javascriptCanOpenWindowsAutomatically);
        break;
    case PROP_JAVASCRIPT_CAN_ACCESS_CLIPBOARD:
        g_value_set_boolean(value, priv->javascriptCanAccessClipboard);
        break;
    case PROP_ENABLE_OFFLINE_WEB_APPLICATION_CACHE:
        g_value_set_boolean(value, priv->enableOfflineWebApplicationCache);
        break;
    case PROP_EDITING_BEHAVIOR:
        g_value_set_enum(value, priv->editingBehavior);
        break;
    case PROP_ENABLE_UNIVERSAL_ACCESS_FROM_FILE_URIS:
        g_value_set_boolean(value, priv->enableUniversalAccessFromFileURIs);
        break;
    case PROP_ENABLE_FILE_ACCESS_FROM_FILE_URIS:
        g_value_set_boolean(value, priv->enableFileAccessFromFileURIs);
        break;
    case PROP_ENABLE_DOM_PASTE:
        g_value_set_boolean(value, priv->enableDOMPaste);
        break;
    case PROP_TAB_KEY_CYCLES_THROUGH_ELEMENTS:
        g_value_set_boolean(value, priv->tabKeyCyclesThroughElements);
        break;
    case PROP_ENABLE_DEFAULT_CONTEXT_MENU:
        g_value_set_boolean(value, priv->enableDefaultContextMenu);
        break;
    case PROP_ENABLE_SITE_SPECIFIC_QUIRKS:
        g_value_set_boolean(value, priv->enableSiteSpecificQuirks);
        break;
    case PROP_ENABLE_PAGE_CACHE:
        g_value_set_boolean(value, priv->enablePageCache);
        break;
    case PROP_AUTO_RESIZE_WINDOW:
        g_value_set_boolean(value, priv->autoResizeWindow);
        break;
    case PROP_ENABLE_JAVA_APPLET:
        g_value_set_boolean(value, priv->enableJavaApplet);
        break;
    case PROP_ENABLE_HYPERLINK_AUDITING:
        g_value_set_boolean(value, priv->enableHyperlinkAuditing);
        break;
    case PROP_ENABLE_FULLSCREEN:
        g_value_set_boolean(value, priv->enableFullscreen);
        break;
    case PROP_ENABLE_DNS_PREFETCHING:
        g_value_set_boolean(value, priv->enableDNSPrefetching);
        break;
    case PROP_ENABLE_WEBGL:
        g_value_set_boolean(value, priv->enableWebgl);
        break;
    case PROP_ENABLE_MEDIA_STREAM:
        g_value_set_boolean(value, priv->enableMediaStream);
        break;
    case PROP_ENABLE_WEB_AUDIO:
        g_value_set_boolean(value, priv->enableWebAudio);
        break;
    case PROP_ENABLE_ACCELERATED_COMPOSITING:
        g_value_set_boolean(value, priv->enableAcceleratedCompositing);
        break;
    case PROP_ENABLE_SMOOTH_SCROLLING:
        g_value_set_boolean(value, priv->enableSmoothScrolling);
        break;
    case PROP_ENABLE_CSS_SHADERS:
        g_value_set_boolean(value, priv->enableCSSShaders);
        break;
    case PROP_MEDIA_PLAYBACK_REQUIRES_USER_GESTURE:
        g_value_set_boolean(value, priv->mediaPlaybackRequiresUserGesture);
        break;
    case PROP_MEDIA_PLAYBACK_ALLOWS_INLINE:
        g_value_set_boolean(value, priv->mediaPlaybackAllowsInline);
        break;
    case PROP_ENABLE_DISPLAY_OF_INSECURE_CONTENT:
        g_value_set_boolean(value, priv->enableDisplayOfInsecureContent);
        break;
    case PROP_ENABLE_RUNNING_OF_INSECURE_CONTENT:
        g_value_set_boolean(value, priv->enableRunningOfInsecureContent);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/**
 * webkit_web_settings_new:
 *
 * Creates a new #WebKitWebSettings instance with default values. It must
 * be manually attached to a WebView.
 *
 * Returns: a new #WebKitWebSettings instance
 **/
WebKitWebSettings* webkit_web_settings_new()
{
    return WEBKIT_WEB_SETTINGS(g_object_new(WEBKIT_TYPE_WEB_SETTINGS, NULL));
}

/**
 * webkit_web_settings_copy:
 * @web_settings: a #WebKitWebSettings to copy.
 *
 * Copies an existing #WebKitWebSettings instance.
 *
 * Returns: (transfer full): a new #WebKitWebSettings instance
 **/
WebKitWebSettings* webkit_web_settings_copy(WebKitWebSettings* original)
{
    unsigned numberOfProperties = 0;
    GOwnPtr<GParamSpec*> properties(g_object_class_list_properties(
        G_OBJECT_CLASS(WEBKIT_WEB_SETTINGS_GET_CLASS(original)), &numberOfProperties));
    GOwnPtr<GParameter> parameters(g_new0(GParameter, numberOfProperties));

    for (size_t i = 0; i < numberOfProperties; i++) {
        GParamSpec* property = properties.get()[i];
        GParameter* parameter = parameters.get() + i;

        if (!(property->flags & (G_PARAM_CONSTRUCT | G_PARAM_READWRITE)))
            continue;

        parameter->name = property->name;
        g_value_init(&parameter->value, property->value_type);
        g_object_get_property(G_OBJECT(original), property->name, &parameter->value);
    }

    return WEBKIT_WEB_SETTINGS(g_object_newv(WEBKIT_TYPE_WEB_SETTINGS, numberOfProperties, parameters.get()));

}

/**
 * webkit_web_settings_add_extra_plugin_directory:
 * @web_view: a #WebKitWebView
 * @directory: the directory to add
 *
 * Adds the @directory to paths where @web_view will search for plugins.
 *
 * Since: 1.0.3
 */
void webkit_web_settings_add_extra_plugin_directory(WebKitWebView* webView, const gchar* directory)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    PluginDatabase::installedPlugins()->addExtraPluginDirectory(filenameToString(directory));
}

/**
 * webkit_web_settings_get_user_agent:
 * @web_settings: a #WebKitWebSettings
 *
 * Returns: the User-Agent string currently used by the web view(s) associated
 * with the @web_settings.
 *
 * Since: 1.1.11
 */
const gchar* webkit_web_settings_get_user_agent(WebKitWebSettings* webSettings)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_SETTINGS(webSettings), 0);
    return webSettings->priv->userAgent.data();
}

static void initializeDomainsList(HashSet<String>& googleDomains)
{
    // Google search domains.
    googleDomains.add("biz");
    googleDomains.add("com");
    googleDomains.add("net");
    googleDomains.add("org");
    googleDomains.add("ae");
    googleDomains.add("ag");
    googleDomains.add("am");
    googleDomains.add("at");
    googleDomains.add("az");
    googleDomains.add("be");
    googleDomains.add("bi");
    googleDomains.add("ca");
    googleDomains.add("cc");
    googleDomains.add("cd");
    googleDomains.add("cg");
    googleDomains.add("ch");
    googleDomains.add("cl");
    googleDomains.add("com.br");
    googleDomains.add("com.do");
    googleDomains.add("co.uk");
    googleDomains.add("co.kr");
    googleDomains.add("co.jp");
    googleDomains.add("de");
    googleDomains.add("dj");
    googleDomains.add("dk");
    googleDomains.add("ee");
    googleDomains.add("es");
    googleDomains.add("fi");
    googleDomains.add("fm");
    googleDomains.add("fr");
    googleDomains.add("gg");
    googleDomains.add("gl");
    googleDomains.add("gm");
    googleDomains.add("gs");
    googleDomains.add("hn");
    googleDomains.add("hu");
    googleDomains.add("ie");
    googleDomains.add("it");
    googleDomains.add("je");
    googleDomains.add("kz");
    googleDomains.add("li");
    googleDomains.add("lt");
    googleDomains.add("lu");
    googleDomains.add("lv");
    googleDomains.add("ma");
    googleDomains.add("ms");
    googleDomains.add("mu");
    googleDomains.add("mw");
    googleDomains.add("nl");
    googleDomains.add("no");
    googleDomains.add("nu");
    googleDomains.add("pl");
    googleDomains.add("pn");
    googleDomains.add("pt");
    googleDomains.add("ru");
    googleDomains.add("rw");
    googleDomains.add("sh");
    googleDomains.add("sk");
    googleDomains.add("sm");
    googleDomains.add("st");
    googleDomains.add("td");
    googleDomains.add("tk");
    googleDomains.add("tp");
    googleDomains.add("tv");
    googleDomains.add("us");
    googleDomains.add("uz");
    googleDomains.add("ws");
}

static void initializeOtherGoogleDomains(Vector<String>& otherGoogleDomains)
{
    otherGoogleDomains.append("gmail.com");
    otherGoogleDomains.append("youtube.com");
    otherGoogleDomains.append("gstatic.com");
    otherGoogleDomains.append("ytimg.com");
}

static bool isGoogleDomain(String host)
{
    DEFINE_STATIC_LOCAL(HashSet<String>, googleDomains, ());
    DEFINE_STATIC_LOCAL(Vector<String>, otherGoogleDomains, ());

    if (googleDomains.isEmpty())
        initializeDomainsList(googleDomains);

    if (otherGoogleDomains.isEmpty())
        initializeOtherGoogleDomains(otherGoogleDomains);

    // First check if this is one of the various google.com international domains.
    int position = host.find(".google.");
    if (position > 0 && googleDomains.contains(host.substring(position + sizeof(".google.") - 1)))
        return true;

    // Then we check the possibility of it being one of the other, .com-only google domains.
    for (unsigned int i = 0; i < otherGoogleDomains.size(); i++) {
        if (host.endsWith(otherGoogleDomains.at(i)))
            return true;
    }

    return false;
}

static String userAgentForURL(const KURL& url)
{
    // For Google domains, drop the browser's custom User Agent string, and use the
    // standard Chrome one, so they don't give us a broken experience.
    if (isGoogleDomain(url.host()))
        return standardUserAgent();

    return String();
}

/*
 * Private usage only.
 * webkitWebSettingsUserAgentForURI:
 * @web_settings: the #WebKitWebSettings object to query
 * @uri: the URI we want to know the User-Agent for
 *
 * Some web sites have been using User-Agent parsing heavily to decide
 * the kind of content that is sent to the browser. When
 * WebKitWebSettings:enable-site-specific-quirks is enabled WebKitGTK+
 * will use its knowledge of sites doing bad things and lie to them by
 * sending either the default User-Agent, i.e. not using the one
 * specified by the browser in WebKitWebSettings:user-agent, or the
 * Safari one (including lying about the underlying operating system).
 *
 * This function allows the browser to figure out what User-Agent is
 * going to be sent to a particular URI.
 *
 * Please note that if WebKitWebSettings:use-site-specific-quirks is
 * turned off calling this function is the same as calling
 * webkit_web_settings_get_user_agent(), except you have to free the
 * result.
 *
 * Returns: (transfer full): a newly allocated string containing the
 * User-Agent that will be sent for the given URI.
 */
char* webkitWebSettingsUserAgentForURI(WebKitWebSettings* webSettings, const char* uri)
{
    if (webSettings->priv->enableSiteSpecificQuirks) {
        String userAgentString = userAgentForURL(WebCore::KURL(WebCore::KURL(), String::fromUTF8(uri)));
        if (!userAgentString.isEmpty())
            return g_strdup(userAgentString.utf8().data());
    }

    return g_strdup(webkit_web_settings_get_user_agent(webSettings));
}

namespace WebKit {

WebCore::EditingBehaviorType core(WebKitEditingBehavior type)
{
    return static_cast<WebCore::EditingBehaviorType>(type);
}

}
