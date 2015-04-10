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

/**
 * @file    ewk_window_features.h
 * @brief   Access to the features of window.
 */

#ifndef ewk_window_features_h
#define ewk_window_features_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Creates a type name for _Ewk_Window_Features. */
typedef struct _Ewk_Window_Features Ewk_Window_Features;

/**
 * Decreases the referece count of an Ewk_Window_Features, possibly freeing it.
 *
 * When the reference count of the object reaches 0, it is freed.
 *
 * @param window_features the object to decrease reference count
 */
EAPI void         ewk_window_features_unref(Ewk_Window_Features *window_features);

/**
 * Increases the reference count of an Ewk_Window_Features.
 *
 * @param window_features the object to increase reference count
 */
EAPI void         ewk_window_features_ref(Ewk_Window_Features *window_features);

/**
 * Gets boolean properties of an Ewk_Window_Features.
 *
 * Properties are returned in the respective pointers. Passing @c NULL to any of
 * these pointers will cause that property to not be returned.
 *
 * @param window_features the object to get boolean properties
 * @param toolbar_visible the pointer to store if toolbar is visible
 * @param statusbar_visible the pointer to store if statusbar is visible
 * @param scrollbars_visible the pointer to store if scrollbars is visible
 * @param menubar_visible the pointer to store if menubar is visible
 * @param locationbar_visible the pointer to store if locationbar is visible
 * @param fullscreen the pointer to store if fullscreen is enabled
 *
 * @see ewk_window_features_int_property_get
 */
EAPI void         ewk_window_features_bool_property_get(const Ewk_Window_Features *window_features, Eina_Bool *toolbar_visible, Eina_Bool *statusbar_visible, Eina_Bool *scrollbars_visible, Eina_Bool *menubar_visible, Eina_Bool *locationbar_visible, Eina_Bool *fullscreen);

/**
 * Gets int properties of an Ewk_Window_Features.
 *
 * Properties are returned in the respective pointers. Passing @c NULL to any of
 * these pointers will cause that property to not be returned.
 *
 * Make sure to check if the value returned is less than 0 before using it, since in
 * that case it means that property was not set in window_features object.
 *
 * @param window_features the window's features
 * @param x the pointer to store x position
 * @param y the pointer to store y position
 * @param w the pointer to store width
 * @param h the pointer to store height
 *
 * @see ewk_window_features_bool_property_get
 */
EAPI void         ewk_window_features_int_property_get(const Ewk_Window_Features *window_features, int *x, int *y, int *w, int *h);

#ifdef __cplusplus
}
#endif
#endif // ewk_window_features_h
