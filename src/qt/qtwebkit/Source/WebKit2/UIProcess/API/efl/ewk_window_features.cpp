/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
#include "ewk_window_features.h"

#include "EwkView.h"
#include "WKDictionary.h"
#include "WKNumber.h"
#include "WKString.h"
#include "ewk_window_features_private.h"

using namespace WebKit;

EwkWindowFeatures::EwkWindowFeatures(WKDictionaryRef windowFeatures, EwkView* view)
    : m_view(view)
    , m_toolbarVisible(getWindowFeatureBoolValue(windowFeatures, "toolBarVisible", true))
    , m_statusBarVisible(getWindowFeatureBoolValue(windowFeatures, "statusBarVisible", true))
    , m_scrollbarsVisible(getWindowFeatureBoolValue(windowFeatures, "scrollbarsVisible", true))
    , m_menuBarVisible(getWindowFeatureBoolValue(windowFeatures, "menuBarVisible", true))
    , m_locationBarVisible(getWindowFeatureBoolValue(windowFeatures, "locationBarVisible", true))
    , m_resizable(getWindowFeatureBoolValue(windowFeatures, "resizable", true))
    , m_fullScreen(getWindowFeatureBoolValue(windowFeatures, "fullscreen", false))
{
    m_geometry.x = getWindowFeatureDoubleValue(windowFeatures, "x", 0);
    m_geometry.y = getWindowFeatureDoubleValue(windowFeatures, "y", 0);
    m_geometry.w = getWindowFeatureDoubleValue(windowFeatures, "width", 0);
    m_geometry.h = getWindowFeatureDoubleValue(windowFeatures, "height", 0);
}

static inline WKTypeRef getWindowFeatureValue(WKDictionaryRef windowFeatures, const char* featureName)
{
    ASSERT(featureName);
    if (!windowFeatures)
        return 0;

    WKRetainPtr<WKStringRef> key(AdoptWK, WKStringCreateWithUTF8CString(featureName));
    return WKDictionaryGetItemForKey(windowFeatures, key.get());
}

bool EwkWindowFeatures::getWindowFeatureBoolValue(WKDictionaryRef windowFeatures, const char* featureName, bool defaultValue)
{
    WKBooleanRef value = static_cast<WKBooleanRef>(getWindowFeatureValue(windowFeatures, featureName));

    return value ? WKBooleanGetValue(value) : defaultValue;
}

double EwkWindowFeatures::getWindowFeatureDoubleValue(WKDictionaryRef windowFeatures, const char* featureName, double defaultValue)
{
    WKDoubleRef value = static_cast<WKDoubleRef>(getWindowFeatureValue(windowFeatures, featureName));

    return value ?  WKDoubleGetValue(value) : defaultValue;
}

void EwkWindowFeatures::setToolbarVisible(bool toolbarVisible)
{
    m_toolbarVisible = toolbarVisible;
    m_view->smartCallback<EwkViewCallbacks::ToolbarVisible>().call(&toolbarVisible);
}

void EwkWindowFeatures::setStatusBarVisible(bool statusBarVisible)
{
    m_statusBarVisible = statusBarVisible;
    m_view->smartCallback<EwkViewCallbacks::StatusBarVisible>().call(&statusBarVisible);
}

void EwkWindowFeatures::setMenuBarVisible(bool menuBarVisible)
{
    m_menuBarVisible = menuBarVisible;
    m_view->smartCallback<EwkViewCallbacks::MenuBarVisible>().call(&menuBarVisible);
}

void EwkWindowFeatures::setResizable(bool resizable)
{
    m_resizable = resizable;
    m_view->smartCallback<EwkViewCallbacks::WindowResizable>().call(&resizable);
}

Eina_Bool ewk_window_features_toolbar_visible_get(const Ewk_Window_Features* window_features)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkWindowFeatures, window_features, impl, false);

    return impl->toolbarVisible();
}

Eina_Bool ewk_window_features_statusbar_visible_get(const Ewk_Window_Features* window_features)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkWindowFeatures, window_features, impl, false);

    return impl->statusBarVisible();
}

Eina_Bool ewk_window_features_scrollbars_visible_get(const Ewk_Window_Features* window_features)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkWindowFeatures, window_features, impl, false);

    return impl->scrollbarsVisible();
}

Eina_Bool ewk_window_features_menubar_visible_get(const Ewk_Window_Features* window_features)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkWindowFeatures, window_features, impl, false);

    return impl->menuBarVisible();
}

Eina_Bool ewk_window_features_locationbar_visible_get(const Ewk_Window_Features* window_features)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkWindowFeatures, window_features, impl, false);

    return impl->locationBarVisible();
}

Eina_Bool ewk_window_features_resizable_get(const Ewk_Window_Features* window_features)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkWindowFeatures, window_features, impl, false);

    return impl->resizable();
}

Eina_Bool ewk_window_features_fullscreen_get(const Ewk_Window_Features* window_features)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkWindowFeatures, window_features, impl, false);

    return impl->fullScreen();
}

void ewk_window_features_geometry_get(const Ewk_Window_Features* window_features, Evas_Coord* x, Evas_Coord* y, Evas_Coord* width, Evas_Coord* height)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkWindowFeatures, window_features, impl);

    const Evas_Coord_Rectangle& geometry = impl->geometry();
    if (x)
        *x = geometry.x;

    if (y)
        *y = geometry.y;

    if (width)
        *width = geometry.w;

    if (height)
        *height = geometry.h;
}
