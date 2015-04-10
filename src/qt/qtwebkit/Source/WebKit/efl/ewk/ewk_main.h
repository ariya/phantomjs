/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2010 Samsung Electronics

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
 * @file    ewk_main.h
 * @brief   The main file of WebKit-EFL, not tied to any view object.
 */

#ifndef ewk_main_h
#define ewk_main_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes webkit's instance.
 *
 * - initializes components needed by Efl,
 * - sets web database location,
 * - sets page cache capacity,
 * - increases a reference count of webkit's instance.
 *
 * @return a reference count of webkit's instance on success or 0 on failure
 */
EAPI int ewk_init(void);

/**
 * Decreases a reference count of webkit's instance, possibly destroying it.
 *
 * If the reference count reaches 0 webkit's instance is destroyed.
 *
 * @return a reference count of webkit's instance
 */
EAPI int ewk_shutdown(void);

#ifdef __cplusplus
}
#endif
#endif // ewk_main_h
