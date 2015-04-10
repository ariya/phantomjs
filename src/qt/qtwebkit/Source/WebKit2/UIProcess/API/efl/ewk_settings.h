/*
 * Copyright (C) 2012 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    ewk_settings.h
 * @brief   Describes the settings API.
 *
 * @note The ewk_settings is for setting the preference of specific ewk_view.
 * We can get the ewk_settings from ewk_view using ewk_view_settings_get() API.
 */

#ifndef ewk_settings_h
#define ewk_settings_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Creates a type name for Ewk_Settings */
typedef struct EwkSettings Ewk_Settings;


/**
 * Enables/disables the Javascript Fullscreen API. The Javascript API allows
 * to request full screen mode, for more information see:
 * http://dvcs.w3.org/hg/fullscreen/raw-file/tip/Overview.html
 *
 * Default value for Javascript Fullscreen API setting is @c EINA_TRUE .
 *
 * @param settings settings object to enable Javascript Fullscreen API
 * @param enable @c EINA_TRUE to enable Javascript Fullscreen API or
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_fullscreen_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether the Javascript Fullscreen API is enabled or not.
 *
 * @param settings settings object to query whether Javascript Fullscreen API is enabled
 *
 * @return @c EINA_TRUE if the Javascript Fullscreen API is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_fullscreen_enabled_get(const Ewk_Settings *settings);

/**
 * Enables/disables the javascript executing.
 *
 * By default, JavaScript execution is enabled.
 *
 * @param settings settings object to set javascript executing
 * @param enable @c EINA_TRUE to enable javascript executing
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_javascript_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether JavaScript execution is enabled.
 *
 * @param settings settings object to query if the javascript can be executed
 *
 * @return @c EINA_TRUE if the javascript can be executed
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_javascript_enabled_get(const Ewk_Settings *settings);

/**
 * Enables/disables auto loading of the images.
 *
 * By default, auto loading of the images is enabled.
 *
 * @param settings settings object to set auto loading of the images
 * @param automatic @c EINA_TRUE to enable auto loading of the images
 *                  @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_loads_images_automatically_set(Ewk_Settings *settings, Eina_Bool automatic);

/**
 * Returns whether the images can be loaded automatically or not.
 *
 * @param settings settings object to get auto loading of the images
 *
 * @return @c EINA_TRUE if the images are loaded automatically
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_loads_images_automatically_get(const Ewk_Settings *settings);

/**
 * Enables/disables developer extensions.
 *
 * By default, the developer extensions are disabled.
 *
 * @param settings settings object to set developer extensions
 * @param enable @c EINA_TRUE to enable developer extensions
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_developer_extras_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Queries if developer extensions are enabled.
 *
 * By default, the developer extensions are disabled.
 *
 * @param settings settings object to set developer extensions
 *
 * @return @c EINA_TRUE if developer extensions are enabled
           @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_developer_extras_enabled_get(const Ewk_Settings *settings);

/**
 * Allow / Disallow file access from file:// URLs.
 *
 * By default, file access from file:// URLs is not allowed.
 *
 * @param settings settings object to set file access permission
 * @param enable @c EINA_TRUE to enable file access permission
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_file_access_from_file_urls_allowed_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Queries if  file access from file:// URLs is allowed.
 *
 * By default, file access from file:// URLs is not allowed.
 *
 * @param settings settings object to query file access permission
 *
 * @return @c EINA_TRUE if file access from file:// URLs is allowed
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_file_access_from_file_urls_allowed_get(const Ewk_Settings *settings);

/**
 * Enables/disables frame flattening.
 *
 * By default, the frame flattening is disabled.
 *
 * @param settings settings object to set the frame flattening
 * @param enable @c EINA_TRUE to enable the frame flattening
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @see ewk_settings_enable_frame_flattening_get()
 */
EAPI Eina_Bool ewk_settings_frame_flattening_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether the frame flattening is enabled.
 *
 * The frame flattening is a feature which expands sub frames until all the frames become
 * one scrollable page.
 *
 * @param settings settings object to get the frame flattening.
 *
 * @return @c EINA_TRUE if the frame flattening is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_frame_flattening_enabled_get(const Ewk_Settings *settings);

/**
 * Enables/disables DNS prefetching.
 *
 * By default, DNS prefetching is disabled.
 *
 * @param settings settings object to set DNS prefetching
 * @param enable @c EINA_TRUE to enable DNS prefetching or
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @see ewk_settings_DNS_prefetching_enabled_get()
 */
EAPI Eina_Bool ewk_settings_dns_prefetching_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether DNS prefetching is enabled or not.
 *
 * DNS prefetching is an attempt to resolve domain names before a user tries to follow a link.
 *
 * @param settings settings object to get DNS prefetching
 *
 * @return @c EINA_TRUE if DNS prefetching is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_dns_prefetching_enabled_get(const Ewk_Settings *settings);

/**
 * Enables/disables the encoding detector.
 *
 * By default, the encoding detector is disabled.
 *
 * @param settings settings object to set the encoding detector
 * @param enable @c EINA_TRUE to enable the encoding detector,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_encoding_detector_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
* Returns whether the encoding detector is enabled or not.
 *
 * @param settings settings object to query whether encoding detector is enabled
 *
 * @return @c EINA_TRUE if the encoding detector is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_encoding_detector_enabled_get(const Ewk_Settings *settings);

/**
 * Sets preferred minimum contents width which is used as default minimum contents width
 * for non viewport meta element sites.
 *
 * By default, preferred minimum contents width is equal to @c 980.
 *
 * @param settings settings object to set the encoding detector
 * @param enable @c EINA_TRUE to enable the encoding detector,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_preferred_minimum_contents_width_set(Ewk_Settings *settings, unsigned width);

/**
 * Returns preferred minimum contents width or @c 0 on failure.
 *
 * @param settings settings object to query preferred minimum contents width
 *
 * @return preferred minimum contents width
 *         @c 0 on failure
 */
EAPI unsigned ewk_settings_preferred_minimum_contents_width_get(const Ewk_Settings *settings);

/**
 * Enables/disables the offline application cache.
 *
 * By default, the offline application cache is enabled.
 *
 * @param settings settings object to set the offline application cache state
 * @param enable @c EINA_TRUE to enable the offline application cache,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_offline_web_application_cache_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether the offline application cache is enabled or not.
 *
 * @param settings settings object to query whether offline application cache is enabled
 *
 * @return @c EINA_TRUE if the offline application cache is enabled
 *         @c EINA_FALSE if disabled or on failure
 */
EAPI Eina_Bool ewk_settings_offline_web_application_cache_enabled_get(const Ewk_Settings *settings);

/**
 * Enables/disables if the scripts can open new windows.
 *
 * By default, the scripts can open new windows.
 *
 * @param settings settings object to set if the scripts can open new windows
 * @param enable @c EINA_TRUE if the scripts can open new windows
 *        @c EINA_FALSE if not
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure (scripts are disabled)
 */
EAPI Eina_Bool ewk_settings_scripts_can_open_windows_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether the scripts can open new windows.
 *
 * @param settings settings object to query whether the scripts can open new windows
 *
 * @return @c EINA_TRUE if the scripts can open new windows
 *         @c EINA_FALSE if not or on failure (scripts are disabled)
 */
EAPI Eina_Bool ewk_settings_scripts_can_open_windows_get(const Ewk_Settings *settings);

/**
 * Enables/disables the HTML5 local storage functionality.
 *
 * Local storage provides simple synchronous storage access.
 * HTML5 local storage specification is available at
 * http://dev.w3.org/html5/webstorage/.
 *
 * By default, the HTML5 local storage is enabled.
 *
 * @param settings settings object to set the HTML5 local storage state
 * @param enable @c EINA_TRUE to enable HTML5 local storage,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_local_storage_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether the HTML5 local storage functionality is enabled or not.
 *
 * Local storage provides simple synchronous storage access.
 * HTML5 local storage specification is available at
 * http://dev.w3.org/html5/webstorage/.
 *
 * By default, the HTML5 local storage is enabled.
 *
 * @param settings settings object to query whether HTML5 local storage is enabled
 *
 * @return @c EINA_TRUE if the HTML5 local storage is enabled
 *         @c EINA_FALSE if disabled or on failure
 */
EAPI Eina_Bool ewk_settings_local_storage_enabled_get(const Ewk_Settings *settings);

/**
 * Toggles plug-ins support.
 *
 * By default, plug-ins support is enabled.
 *
 * @param settings settings object to set plug-ins support
 * @param enable @c EINA_TRUE to enable plug-ins support
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_plugins_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether plug-ins support is enabled or not.
 *
 * @param settings settings object to query whether plug-ins support is enabled
 *
 * @return @c EINA_TRUE if plug-ins support is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_plugins_enabled_get(const Ewk_Settings *settings);

/**
 * Sets the default font size.
 *
 * By default, the default font size is @c 16.
 *
 * @param settings settings object to set the default font size
 * @param size a new default font size to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_default_font_size_set(Ewk_Settings *settings, int size);

/**
 * Returns the default font size.
 *
 * @param settings settings object to get the default font size
 *
 * @return @c the default font size or @c 0 on failure
 */
EAPI int ewk_settings_default_font_size_get(const Ewk_Settings *settings);

/**
 * Enables/disables private browsing.
 *
 * By default, private browsing is disabled.
 *
 * @param settings settings object to set private browsing
 * @param enable @c EINA_TRUE to enable private browsing
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_private_browsing_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether private browsing is enabled or not.
 *
 * Private Browsing allows a user to browse the Internet without saving any information
 * about which sites and pages a user has visited.
 *
 * @param settings settings object to query whether private browsing is enabled
 *
 * @return @c EINA_TRUE if private browsing is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_private_browsing_enabled_get(const Ewk_Settings *settings);

/**
 * Enables/disables text autosizing.
 *
 * By default, the text autosizing is disabled.
 *
 * @param settings settings object to set the text autosizing
 * @param enable @c EINA_TRUE to enable the text autosizing
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @see ewk_settings_text_autosizing_enabled_get()
 */
EAPI Eina_Bool ewk_settings_text_autosizing_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether the text autosizing is enabled.
 *
 * The text autosizing is a feature which adjusts the font size of text in wide
 * columns, and makes text more legible.
 *
 * @param settings settings object to query whether text autosizing is enabled
 *
 * @return @c EINA_TRUE if the text autosizing is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_text_autosizing_enabled_get(const Ewk_Settings *settings);

#ifdef __cplusplus
}
#endif
#endif // ewk_settings_h
