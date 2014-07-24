/*
 * Copyright (C) 2012 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#include "config.h"
#include "ewk_settings.h"

#include "EwkView.h"
#include "ewk_settings_private.h"
#include <WebKit2/WebPageGroup.h>
#include <WebKit2/WebPageProxy.h>
#include <WebKit2/WebPreferences.h>

using namespace WebKit;

const WebKit::WebPreferences* EwkSettings::preferences() const
{
    return m_view->page()->pageGroup()->preferences();
}

WebKit::WebPreferences* EwkSettings::preferences()
{
    return m_view->page()->pageGroup()->preferences();
}

Eina_Bool ewk_settings_fullscreen_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
#if ENABLE(FULLSCREEN_API)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->preferences()->setFullScreenEnabled(enable);
    return true;
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(enable);
    return false;
#endif
}

Eina_Bool ewk_settings_fullscreen_enabled_get(const Ewk_Settings* settings)
{
#if ENABLE(FULLSCREEN_API)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    return settings->preferences()->fullScreenEnabled();
#else
    UNUSED_PARAM(settings);
    return false;
#endif
}

Eina_Bool ewk_settings_javascript_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setJavaScriptEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_javascript_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->javaScriptEnabled();
}

Eina_Bool ewk_settings_loads_images_automatically_set(Ewk_Settings* settings, Eina_Bool automatic)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setLoadsImagesAutomatically(automatic);

    return true;
}

Eina_Bool ewk_settings_loads_images_automatically_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->loadsImagesAutomatically();
}

Eina_Bool ewk_settings_developer_extras_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setDeveloperExtrasEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_developer_extras_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->developerExtrasEnabled();
}

Eina_Bool ewk_settings_file_access_from_file_urls_allowed_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setAllowFileAccessFromFileURLs(enable);

    return true;
}

Eina_Bool ewk_settings_file_access_from_file_urls_allowed_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->allowFileAccessFromFileURLs();
}

Eina_Bool ewk_settings_frame_flattening_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setFrameFlatteningEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_frame_flattening_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->frameFlatteningEnabled();
}

Eina_Bool ewk_settings_dns_prefetching_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setDNSPrefetchingEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_dns_prefetching_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->dnsPrefetchingEnabled();
}

Eina_Bool ewk_settings_encoding_detector_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setUsesEncodingDetector(enable);

    return true;
}

Eina_Bool ewk_settings_encoding_detector_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->usesEncodingDetector();
}

Eina_Bool ewk_settings_preferred_minimum_contents_width_set(Ewk_Settings *settings, unsigned width)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setLayoutFallbackWidth(width);

    return true;
}

unsigned ewk_settings_preferred_minimum_contents_width_get(const Ewk_Settings *settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->layoutFallbackWidth();
}

Eina_Bool ewk_settings_offline_web_application_cache_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->preferences()->setOfflineWebApplicationCacheEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_offline_web_application_cache_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->offlineWebApplicationCacheEnabled();
}

Eina_Bool ewk_settings_scripts_can_open_windows_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->preferences()->setJavaScriptCanOpenWindowsAutomatically(enable);

    return true;
}

Eina_Bool ewk_settings_scripts_can_open_windows_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->javaScriptCanOpenWindowsAutomatically();
}

Eina_Bool ewk_settings_local_storage_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setLocalStorageEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_local_storage_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->localStorageEnabled();
}

Eina_Bool ewk_settings_plugins_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setPluginsEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_plugins_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->pluginsEnabled();
}

Eina_Bool ewk_settings_default_font_size_set(Ewk_Settings* settings, int size)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setDefaultFontSize(size);

    return true;
}

int ewk_settings_default_font_size_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, 0);

    return settings->preferences()->defaultFontSize();
}

Eina_Bool ewk_settings_private_browsing_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setPrivateBrowsingEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_private_browsing_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->privateBrowsingEnabled();
}

Eina_Bool ewk_settings_text_autosizing_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
#if ENABLE(TEXT_AUTOSIZING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setTextAutosizingEnabled(enable);

    return true;
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(enable);
    return false;
#endif
}

Eina_Bool ewk_settings_text_autosizing_enabled_get(const Ewk_Settings* settings)
{
#if ENABLE(TEXT_AUTOSIZING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->textAutosizingEnabled();
#else
    UNUSED_PARAM(settings);
    return false;
#endif
}

