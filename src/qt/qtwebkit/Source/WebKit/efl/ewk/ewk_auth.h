/*
    Copyright (C) 2011 Samsung Electronics

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
 * @file    ewk_auth.h
 * @brief   Describes the authentication API.
 */

#ifndef ewk_auth_h
#define ewk_auth_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Callback to be called when authentication is required.
 *
 * @param realm auth's realm
 * @param uri uri
 * @param data points to WebKitAuthData
 */
typedef void (*Ewk_Auth_Show_Dialog_Callback)(const char *realm, const char *uri, void *data);

/**
 * Sets callback to be called when authentication is required.
 *
 * @param callback callback to be called
 */
EAPI void ewk_auth_show_dialog_callback_set(Ewk_Auth_Show_Dialog_Callback);

/**
 * Calls authentication method for setting credentials.
 *
 * @param username username
 * @param password user password
 * @param data soup authentication data
 */
EAPI void ewk_auth_credentials_set(char *username, char *password, void *data);

#ifdef __cplusplus
}
#endif
#endif // ewk_auth_h
