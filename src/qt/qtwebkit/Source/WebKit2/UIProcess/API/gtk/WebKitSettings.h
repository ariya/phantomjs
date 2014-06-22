/*
 * Copyright (c) 2011 Motorola Mobility, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of Motorola Mobility, Inc. nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if !defined(__WEBKIT2_H_INSIDE__) && !defined(WEBKIT2_COMPILATION)
#error "Only <webkit2/webkit2.h> can be included directly."
#endif

#ifndef WebKitSettings_h
#define WebKitSettings_h

#include <glib-object.h>
#include <webkit2/WebKitDefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_SETTINGS            (webkit_settings_get_type())
#define WEBKIT_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_SETTINGS, WebKitSettings))
#define WEBKIT_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_SETTINGS, WebKitSettingsClass))
#define WEBKIT_IS_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_SETTINGS))
#define WEBKIT_IS_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_SETTINGS))
#define WEBKIT_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_SETTINGS, WebKitSettingsClass))

typedef struct _WebKitSettings WebKitSettings;
typedef struct _WebKitSettingsClass WebKitSettingsClass;
typedef struct _WebKitSettingsPrivate WebKitSettingsPrivate;

struct _WebKitSettings {
    GObject parent_instance;

    WebKitSettingsPrivate *priv;
};

struct _WebKitSettingsClass {
    GObjectClass parent_class;

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_settings_get_type(void);

WEBKIT_API WebKitSettings *
webkit_settings_new                                            (void);

WEBKIT_API WebKitSettings *
webkit_settings_new_with_settings                              (const gchar    *first_setting_name,
                                                                ...);

WEBKIT_API gboolean
webkit_settings_get_enable_javascript                          (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_javascript                          (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_auto_load_images                           (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_auto_load_images                           (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_load_icons_ignoring_image_load_setting     (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_load_icons_ignoring_image_load_setting     (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_offline_web_application_cache       (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_offline_web_application_cache       (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_html5_local_storage                 (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_html5_local_storage                 (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_html5_database                      (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_html5_database                      (WebKitSettings *settings,
                                                                gboolean        enabled);
WEBKIT_API gboolean
webkit_settings_get_enable_xss_auditor                         (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_xss_auditor                         (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_frame_flattening                    (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_frame_flattening                    (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_plugins                             (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_plugins                             (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_java                                (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_java                                (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_javascript_can_open_windows_automatically  (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_javascript_can_open_windows_automatically  (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_hyperlink_auditing                  (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_hyperlink_auditing                  (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API const gchar *
webkit_settings_get_default_font_family                        (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_default_font_family                        (WebKitSettings *settings,
                                                                const gchar    *default_font_family);

WEBKIT_API const gchar *
webkit_settings_get_monospace_font_family                      (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_monospace_font_family                      (WebKitSettings *settings,
                                                                const gchar    *monospace_font_family);

WEBKIT_API const gchar *
webkit_settings_get_serif_font_family                          (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_serif_font_family                          (WebKitSettings *settings,
                                                                const gchar    *serif_font_family);

WEBKIT_API const gchar *
webkit_settings_get_sans_serif_font_family                     (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_sans_serif_font_family                     (WebKitSettings *settings,
                                                                const gchar    *sans_serif_font_family);

WEBKIT_API const gchar *
webkit_settings_get_cursive_font_family                        (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_cursive_font_family                        (WebKitSettings *settings,
                                                                const gchar    *cursive_font_family);

WEBKIT_API const gchar *
webkit_settings_get_fantasy_font_family                        (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_fantasy_font_family                        (WebKitSettings *settings,
                                                                const gchar    *fantasy_font_family);

WEBKIT_API const gchar *
webkit_settings_get_pictograph_font_family                     (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_pictograph_font_family                     (WebKitSettings *settings,
                                                                const gchar    *pictograph_font_family);

WEBKIT_API guint32
webkit_settings_get_default_font_size                          (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_default_font_size                          (WebKitSettings *settings,
                                                                guint32         font_size);

WEBKIT_API guint32
webkit_settings_get_default_monospace_font_size                (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_default_monospace_font_size                (WebKitSettings *settings,
                                                                guint32         font_size);

WEBKIT_API guint32
webkit_settings_get_minimum_font_size                          (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_minimum_font_size                          (WebKitSettings *settings,
                                                                guint32         font_size);

WEBKIT_API const gchar *
webkit_settings_get_default_charset                            (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_default_charset                            (WebKitSettings *settings,
                                                                const gchar    *default_charset);

WEBKIT_API gboolean
webkit_settings_get_enable_private_browsing                    (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_private_browsing                    (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_developer_extras                    (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_developer_extras                    (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_resizable_text_areas                (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_resizable_text_areas                (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_tabs_to_links                       (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_tabs_to_links                       (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_dns_prefetching                     (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_dns_prefetching                     (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_caret_browsing                      (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_caret_browsing                      (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_fullscreen                          (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_fullscreen                          (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_print_backgrounds                          (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_print_backgrounds                          (WebKitSettings *settings,
                                                                gboolean        print_backgrounds);

WEBKIT_API gboolean
webkit_settings_get_enable_webaudio                            (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_webaudio                            (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_webgl                               (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_webgl                               (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API void
webkit_settings_set_allow_modal_dialogs                        (WebKitSettings *settings,
                                                                gboolean        allowed);

WEBKIT_API gboolean
webkit_settings_get_allow_modal_dialogs                        (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_zoom_text_only                             (WebKitSettings *settings,
                                                                gboolean        zoom_text_only);

WEBKIT_API gboolean
webkit_settings_get_zoom_text_only                             (WebKitSettings *settings);

WEBKIT_API gboolean
webkit_settings_get_javascript_can_access_clipboard            (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_javascript_can_access_clipboard            (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_media_playback_requires_user_gesture       (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_media_playback_requires_user_gesture       (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_media_playback_allows_inline               (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_media_playback_allows_inline               (WebKitSettings *settings,
                                                                gboolean        enabled);
WEBKIT_API gboolean
webkit_settings_get_draw_compositing_indicators                (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_draw_compositing_indicators                (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_site_specific_quirks                (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_site_specific_quirks                (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_page_cache                          (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_page_cache                          (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API const gchar *
webkit_settings_get_user_agent                                 (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_user_agent                                 (WebKitSettings *settings,
                                                                const gchar    *user_agent);
WEBKIT_API void
webkit_settings_set_user_agent_with_application_details        (WebKitSettings *settings,
                                                                const gchar    *application_name,
                                                                const gchar    *application_version);

WEBKIT_API gboolean
webkit_settings_get_enable_smooth_scrolling                    (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_smooth_scrolling                    (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_accelerated_2d_canvas               (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_accelerated_2d_canvas               (WebKitSettings *settings,
                                                                gboolean        enabled);

WEBKIT_API gboolean
webkit_settings_get_enable_write_console_messages_to_stdout    (WebKitSettings *settings);

WEBKIT_API void
webkit_settings_set_enable_write_console_messages_to_stdout    (WebKitSettings *settings,
                                                                gboolean        enabled);

G_END_DECLS

#endif /* WebKitSettings_h */
