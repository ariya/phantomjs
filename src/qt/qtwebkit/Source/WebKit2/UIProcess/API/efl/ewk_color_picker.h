/*
   Copyright (C) 2011 Samsung Electronics
   Copyright (C) 2012 Intel Corporation. All rights reserved.

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

#ifndef ewk_color_picker_h
#define ewk_color_picker_h

#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Creates a type name for @a Ewk_Color_Picker. */
typedef struct EwkColorPicker Ewk_Color_Picker;

/**
 * Sets the selected color.
 *
 * The function should only be called when a color has been requested by the document.
 * If called when this is not the case or when the input picker has been dismissed, this
 * function will fail and return EINA_FALSE.
 *
 * @param color_picker color picker object
 * @param r red channel value to be set
 * @param g green channel value to be set
 * @param b blue channel value to be set
 * @param a alpha channel value to be set
 *
 * @return @c EINA_TRUE on success @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_color_picker_color_set(Ewk_Color_Picker *color_picker, int r, int g, int b, int a);

/**
 * Gets the currently selected color.
 *
 * @param color_picker color picker object
 * @param r red channel value to be get
 * @param g green channel value to be get
 * @param b blue channel value to be get
 * @param a alpha channel value to be get
 *
 * @return @c EINA_TRUE on success @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_color_picker_color_get(const Ewk_Color_Picker *color_picker, int *r, int *g, int *b, int *a);

#ifdef __cplusplus
}
#endif

#endif /* ewk_color_picker_h */
