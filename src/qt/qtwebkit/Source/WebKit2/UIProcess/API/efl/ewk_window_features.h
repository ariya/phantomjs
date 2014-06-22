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

/**
 * @file    ewk_window_features.h
 * @brief   Access to the features of window.
 */

#ifndef ewk_window_features_h
#define ewk_window_features_h

#include <Eina.h>
#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declare Ewk_Window_Features as Ewk_Object.
 *
 * @see Ewk_Object
 */
typedef struct EwkObject Ewk_Window_Features;

/**
 * Queries the toolbar visibility of the window feature.
 *
 * @param window_features the object to get properties
 *
 * @return @c EINA_TRUE is the toolbar should be visible, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_window_features_toolbar_visible_get(const Ewk_Window_Features *window_features);

/**
 * Queries the statusbar visibility of the window feature.
 *
 * @param window_features the object to get properties
 *
 * @return @c EINA_TRUE is the statusbar should be visible, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_window_features_statusbar_visible_get(const Ewk_Window_Features *window_features);

/**
 * Queries the scrollbar visibility of the window feature.
 *
 * @param window_features the object to get properties
 *
 * @return @c EINA_TRUE is the scrollbars should be visible, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_window_features_scrollbars_visible_get(const Ewk_Window_Features *window_features);

/**
 * Queries the menubar visibility of the window feature.
 *
 * @param window_features the object to get properties
 *
 * @return @c EINA_TRUE is the menubar should be visible, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_window_features_menubar_visible_get(const Ewk_Window_Features *window_features);

/**
 * Queries the locationbar visibility of the window feature.
 *
 * @param window_features the object to get properties
 *
 * @return @c EINA_TRUE is the locationbar should be visible, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_window_features_locationbar_visible_get(const Ewk_Window_Features *window_features);

/**
 * Queries if the window is resizable.
 *
 * @param window_features the object to get properties
 *
 * @return @c EINA_TRUE is the window should be resizable, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_window_features_resizable_get(const Ewk_Window_Features *window_features);

/**
 * Queries the the window is fullscreen
 *
 * @param window_features the object to get properties
 *
 * @return @c EINA_TRUE is the window should be fullscreen, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_window_features_fullscreen_get(const Ewk_Window_Features *window_features);

/**
 * Gets geometry properties of an Ewk_Window_Features.
 *
 * Properties are returned in the respective pointers. Passing @c NULL to any of
 * these pointers will cause that property to not be returned.
 *
 * @param window_features the window's features
 * @param x the pointer to store x position
 * @param y the pointer to store y position
 * @param w the pointer to store width
 * @param h the pointer to store height
 */
EAPI void ewk_window_features_geometry_get(const Ewk_Window_Features *window_features, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h);

#ifdef __cplusplus
}
#endif

#endif // ewk_window_features_h
