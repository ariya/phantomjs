/*
    Copyright (C) 2010 ProFUSION embedded systems
    Copyright (C) 2010 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "ewk_window_features.h"

#include "WindowFeatures.h"
#include "ewk_private.h"
#include <Eina.h>

/**
 * \struct  _Ewk_Window_Features
 * @brief   Contains the window features data.
 */
struct _Ewk_Window_Features {
    unsigned int __ref;
    WebCore::WindowFeatures* core;
};

void ewk_window_features_unref(Ewk_Window_Features* windowFeatures)
{
    EINA_SAFETY_ON_NULL_RETURN(windowFeatures);
    EINA_SAFETY_ON_FALSE_RETURN(windowFeatures->__ref > 0);

    if (--windowFeatures->__ref)
        return;

    delete windowFeatures->core;
    windowFeatures->core = 0;
    delete windowFeatures;
}

void ewk_window_features_ref(Ewk_Window_Features* windowFeatures)
{
    EINA_SAFETY_ON_NULL_RETURN(windowFeatures);
    windowFeatures->__ref++;
}

void ewk_window_features_bool_property_get(const Ewk_Window_Features* windowFeatures, Eina_Bool* toolbarVisible, Eina_Bool* statusbarVisible, Eina_Bool* scrollbarsVisible, Eina_Bool* menubarVisible, Eina_Bool* locationbarVisible, Eina_Bool* fullScreen)
{
    EINA_SAFETY_ON_NULL_RETURN(windowFeatures);
    EINA_SAFETY_ON_NULL_RETURN(windowFeatures->core);

    if (toolbarVisible)
        *toolbarVisible = windowFeatures->core->toolBarVisible;

    if (statusbarVisible)
        *statusbarVisible = windowFeatures->core->statusBarVisible;

    if (scrollbarsVisible)
        *scrollbarsVisible = windowFeatures->core->scrollbarsVisible;

    if (menubarVisible)
        *menubarVisible = windowFeatures->core->menuBarVisible;

    if (locationbarVisible)
        *locationbarVisible = windowFeatures->core->locationBarVisible;

    if (fullScreen)
        *fullScreen = windowFeatures->core->fullscreen;
}

void ewk_window_features_int_property_get(const Ewk_Window_Features* windowFeatures, int* x, int* y, int* width, int* height)
{
    EINA_SAFETY_ON_NULL_RETURN(windowFeatures);
    EINA_SAFETY_ON_NULL_RETURN(windowFeatures->core);

    if (x)
        *x = windowFeatures->core->xSet ? static_cast<int>(windowFeatures->core->x) : -1;

    if (y)
        *y = windowFeatures->core->ySet ? static_cast<int>(windowFeatures->core->y) : -1;

    if (width)
        *width = windowFeatures->core->widthSet ? static_cast<int>(windowFeatures->core->width) : -1;

    if (height)
        *height = windowFeatures->core->heightSet ? static_cast<int>(windowFeatures->core->height) : -1;
}

/* internal methods ****************************************************/

/**
 * @internal
 *
 * Creates a new Ewk_Window_Features object.
 *
 * @param core if not @c 0 a new WebCore::WindowFeatures is allocated copying core features and
 * it is embedded inside the Ewk_Window_Features whose ref count is initialized, if core is @c 0 a new one is created with the default features.
 * @return a new allocated the Ewk_Window_Features object on sucess or @c 0 on failure
 */
Ewk_Window_Features* ewk_window_features_new_from_core(const WebCore::WindowFeatures* core)
{
    Ewk_Window_Features* window_features = new Ewk_Window_Features;

    if (core)
        window_features->core = new WebCore::WindowFeatures(*core);
    else
        window_features->core = new WebCore::WindowFeatures();

    window_features->__ref = 1;

    return window_features;
}
