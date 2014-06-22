/*
    Copyright (C) 2009 Igalia S.L.
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

#ifndef ewk_auth_soup_private_h
#define ewk_auth_soup_private_h

#include "ewk_auth.h"
#include <glib-object.h>
#include <glib.h>

#define EWK_TYPE_SOUP_AUTH_DIALOG            (ewk_auth_soup_dialog_get_type())
#define EWK_SOUP_AUTH_DIALOG(object)         (G_TYPE_CHECK_INSTANCE_CAST((object), EWK_TYPE_SOUP_AUTH_DIALOG, Ewk_Soup_Auth_Dialog))
#define EWK_SOUP_AUTH_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), EWK_TYPE_SOUP_AUTH_DIALOG, Ewk_Soup_Auth_Dialog))
#define EWK_IS_SOUP_AUTH_DIALOG(object)      (G_TYPE_CHECK_INSTANCE_TYPE((object), EWK_TYPE_SOUP_AUTH_DIALOG))
#define EWK_IS_SOUP_AUTH_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), EWK_TYPE_SOUP_AUTH_DIALOG))
#define EWK_SOUP_AUTH_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), EWK_TYPE_SOUP_AUTH_DIALOG, Ewk_Soup_Auth_Dialog))

typedef struct {
    GObject parent_instance;
} Ewk_Soup_Auth_Dialog;

typedef struct {
    GObjectClass parent_class;
} Ewk_Soup_Auth_DialogClass;

GType ewk_auth_soup_dialog_get_type(void);

/**
 *  Sets callback to be called when authentication is required.
 */
void ewk_auth_soup_show_dialog_callback_set(Ewk_Auth_Show_Dialog_Callback callback);

void ewk_auth_soup_credentials_set(const char *username, const char *password, void *data);

#endif // ewk_auth_soup_private_h
