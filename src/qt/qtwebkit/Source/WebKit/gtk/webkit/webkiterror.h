/*
 * Copyright (C) 2008 Luca Bruno <lethalman88@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef webkiterror_h
#define webkiterror_h

#include <glib.h>

#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

#define WEBKIT_NETWORK_ERROR        webkit_network_error_quark ()
#define WEBKIT_POLICY_ERROR         webkit_policy_error_quark  ()
#define WEBKIT_PLUGIN_ERROR         webkit_plugin_error_quark  ()

/* The enum values are synchronized with Mac's WebKit error values. */

/**
 * WebKitNetworkError:
 * @WEBKIT_NETWORK_ERROR_FAILED: Generic load failure
 * @WEBKIT_NETWORK_ERROR_TRANSPORT: Load failure due to transport error
 * @WEBKIT_NETWORK_ERROR_UNKNOWN_PROTOCOL: Load failure due to unknown protocol
 * @WEBKIT_NETWORK_ERROR_CANCELLED: Load failure due to cancellation
 * @WEBKIT_NETWORK_ERROR_FILE_DOES_NOT_EXIST: Load failure due to missing file
 *
 * Enum values used to denote the various network errors.
 **/
typedef enum {
    WEBKIT_NETWORK_ERROR_FAILED                                 = 399,
    WEBKIT_NETWORK_ERROR_TRANSPORT                              = 300,
    WEBKIT_NETWORK_ERROR_UNKNOWN_PROTOCOL                       = 301,
    WEBKIT_NETWORK_ERROR_CANCELLED                              = 302,
    WEBKIT_NETWORK_ERROR_FILE_DOES_NOT_EXIST                    = 303,
} WebKitNetworkError;

/**
 * WebKitPolicyError:
 * @WEBKIT_POLICY_ERROR_FAILED: Generic load failure due to policy error
 * @WEBKIT_POLICY_ERROR_CANNOT_SHOW_MIME_TYPE: Load failure due to unsupported mime type
 * @WEBKIT_POLICY_ERROR_CANNOT_SHOW_URL: Load failure due to URI that can not be shown
 * @WEBKIT_POLICY_ERROR_FRAME_LOAD_INTERRUPTED_BY_POLICY_CHANGE: Load failure due to frame load interruption by policy change
 * @WEBKIT_POLICY_ERROR_CANNOT_USE_RESTRICTED_PORT: Load failure due to port restriction
 *
 * Enum values used to denote the various policy errors.
 **/
typedef enum {
    WEBKIT_POLICY_ERROR_FAILED                                  = 199,
    WEBKIT_POLICY_ERROR_CANNOT_SHOW_MIME_TYPE                   = 100,
    WEBKIT_POLICY_ERROR_CANNOT_SHOW_URL                         = 101,
    WEBKIT_POLICY_ERROR_FRAME_LOAD_INTERRUPTED_BY_POLICY_CHANGE = 102,
    WEBKIT_POLICY_ERROR_CANNOT_USE_RESTRICTED_PORT              = 103,
} WebKitPolicyError;

/**
 * WebKitPluginError:
 * @WEBKIT_PLUGIN_ERROR_FAILED: Generic plugin load failure
 * @WEBKIT_PLUGIN_ERROR_CANNOT_FIND_PLUGIN: Load failure due to missing plugin
 * @WEBKIT_PLUGIN_ERROR_CANNOT_LOAD_PLUGIN: Load failure due to inability to load plugin
 * @WEBKIT_PLUGIN_ERROR_JAVA_UNAVAILABLE: Load failue due to missing Java support that is required to load plugin
 * @WEBKIT_PLUGIN_ERROR_CONNECTION_CANCELLED: Load failure due to connection cancellation
 * @WEBKIT_PLUGIN_ERROR_WILL_HANDLE_LOAD: Load failure since plugin handles the load
 *
 * Enum values used to denote the various plugin errors.
 **/
typedef enum {
    WEBKIT_PLUGIN_ERROR_FAILED                                  = 299,
    WEBKIT_PLUGIN_ERROR_CANNOT_FIND_PLUGIN                      = 200,
    WEBKIT_PLUGIN_ERROR_CANNOT_LOAD_PLUGIN                      = 201,
    WEBKIT_PLUGIN_ERROR_JAVA_UNAVAILABLE                        = 202,
    WEBKIT_PLUGIN_ERROR_CONNECTION_CANCELLED                    = 203,
    WEBKIT_PLUGIN_ERROR_WILL_HANDLE_LOAD                        = 204,
} WebKitPluginError;


WEBKIT_API GQuark
webkit_network_error_quark (void);

WEBKIT_API GQuark
webkit_policy_error_quark  (void);

WEBKIT_API GQuark
webkit_plugin_error_quark  (void);

G_END_DECLS

#endif
