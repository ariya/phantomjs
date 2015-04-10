/*
 * Copyright (C) 2011 Igalia S.L.
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

#if !defined(__WEBKIT2_H_INSIDE__) && !defined(WEBKIT2_COMPILATION)
#error "Only <webkit2/webkit2.h> can be included directly."
#endif

#ifndef WebKitError_h
#define WebKitError_h

#include <webkit2/WebKitDefines.h>

G_BEGIN_DECLS

#define WEBKIT_NETWORK_ERROR    webkit_network_error_quark ()
#define WEBKIT_POLICY_ERROR     webkit_policy_error_quark ()
#define WEBKIT_PLUGIN_ERROR     webkit_plugin_error_quark ()
#define WEBKIT_DOWNLOAD_ERROR   webkit_download_error_quark ()
#define WEBKIT_PRINT_ERROR      webkit_print_error_quark ()
#define WEBKIT_JAVASCRIPT_ERROR webkit_print_error_quark ()
#define WEBKIT_SNAPSHOT_ERROR   webkit_snapshot_error_quark ()

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
    WEBKIT_NETWORK_ERROR_FAILED = 399,
    WEBKIT_NETWORK_ERROR_TRANSPORT = 300,
    WEBKIT_NETWORK_ERROR_UNKNOWN_PROTOCOL = 301,
    WEBKIT_NETWORK_ERROR_CANCELLED = 302,
    WEBKIT_NETWORK_ERROR_FILE_DOES_NOT_EXIST = 303
} WebKitNetworkError;

/**
 * WebKitPolicyError:
 * @WEBKIT_POLICY_ERROR_FAILED: Generic load failure due to policy error
 * @WEBKIT_POLICY_ERROR_CANNOT_SHOW_MIME_TYPE: Load failure due to unsupported mime type
 * @WEBKIT_POLICY_ERROR_CANNOT_SHOW_URI: Load failure due to URI that can not be shown
 * @WEBKIT_POLICY_ERROR_FRAME_LOAD_INTERRUPTED_BY_POLICY_CHANGE: Load failure due to frame load interruption by policy change
 * @WEBKIT_POLICY_ERROR_CANNOT_USE_RESTRICTED_PORT: Load failure due to port restriction
 *
 * Enum values used to denote the various policy errors.
 **/
typedef enum {
    WEBKIT_POLICY_ERROR_FAILED = 199,
    WEBKIT_POLICY_ERROR_CANNOT_SHOW_MIME_TYPE = 100,
    WEBKIT_POLICY_ERROR_CANNOT_SHOW_URI = 101,
    WEBKIT_POLICY_ERROR_FRAME_LOAD_INTERRUPTED_BY_POLICY_CHANGE = 102,
    WEBKIT_POLICY_ERROR_CANNOT_USE_RESTRICTED_PORT = 103
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
    WEBKIT_PLUGIN_ERROR_FAILED = 299,
    WEBKIT_PLUGIN_ERROR_CANNOT_FIND_PLUGIN = 200,
    WEBKIT_PLUGIN_ERROR_CANNOT_LOAD_PLUGIN = 201,
    WEBKIT_PLUGIN_ERROR_JAVA_UNAVAILABLE = 202,
    WEBKIT_PLUGIN_ERROR_CONNECTION_CANCELLED = 203,
    WEBKIT_PLUGIN_ERROR_WILL_HANDLE_LOAD = 204,
} WebKitPluginError;

/**
 * WebKitDownloadError:
 * @WEBKIT_DOWNLOAD_ERROR_NETWORK: Download failure due to network error
 * @WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER: Download was cancelled by user
 * @WEBKIT_DOWNLOAD_ERROR_DESTINATION: Download failure due to destination error
 *
 * Enum values used to denote the various download errors.
 */
typedef enum {
    WEBKIT_DOWNLOAD_ERROR_NETWORK = 499,
    WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER = 400,
    WEBKIT_DOWNLOAD_ERROR_DESTINATION = 401
} WebKitDownloadError;

/**
 * WebKitPrintError:
 * @WEBKIT_PRINT_ERROR_GENERAL: Unspecified error during a print operation
 * @WEBKIT_PRINT_ERROR_PRINTER_NOT_FOUND: Selected printer cannot be found
 * @WEBKIT_PRINT_ERROR_INVALID_PAGE_RANGE: Invalid page range
 *
 * Enum values used to denote the various print errors.
 */
typedef enum {
    WEBKIT_PRINT_ERROR_GENERAL = 599,
    WEBKIT_PRINT_ERROR_PRINTER_NOT_FOUND = 500,
    WEBKIT_PRINT_ERROR_INVALID_PAGE_RANGE = 501
} WebKitPrintError;

/**
 * WebKitJavascriptError:
 * @WEBKIT_JAVASCRIPT_ERROR_SCRIPT_FAILED: An exception was raised in Javascript execution
 *
 * Enum values used to denote errors happending when executing Javascript
 */
typedef enum {
    WEBKIT_JAVASCRIPT_ERROR_SCRIPT_FAILED = 699
} WebKitJavascriptError;

/**
 * WebKitSnapshotError:
 * @WEBKIT_SNAPSHOT_ERROR_FAILED_TO_CREATE: An error occurred when creating a webpage snapshot.
 *
 * Enum values used to denote errors happending when creating snapshots of #WebKitWebView
 */
typedef enum {
    WEBKIT_SNAPSHOT_ERROR_FAILED_TO_CREATE = 799
} WebKitSnapshotError;

WEBKIT_API GQuark
webkit_network_error_quark    (void);

WEBKIT_API GQuark
webkit_policy_error_quark     (void);

WEBKIT_API GQuark
webkit_plugin_error_quark     (void);

WEBKIT_API GQuark
webkit_download_error_quark   (void);

WEBKIT_API GQuark
webkit_print_error_quark      (void);

WEBKIT_API GQuark
webkit_javascript_error_quark (void);

WEBKIT_API GQuark
webkit_snapshot_error_quark   (void);

G_END_DECLS

#endif
