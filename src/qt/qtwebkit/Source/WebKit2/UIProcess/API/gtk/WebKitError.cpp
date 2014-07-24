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

#include "config.h"
#include "WebKitError.h"

#include "WebKitPrivate.h"
#include <WebCore/ErrorsGtk.h>

using namespace WebCore;

/**
 * SECTION: WebKitError
 * @Short_description: Categorized WebKit errors
 * @Title: WebKitError
 *
 * Categorized WebKit errors.
 *
 */

GQuark webkit_network_error_quark()
{
    return g_quark_from_static_string(WebCore::errorDomainNetwork);
}

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NETWORK_ERROR_FAILED, NetworkErrorFailed);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NETWORK_ERROR_TRANSPORT, NetworkErrorTransport);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NETWORK_ERROR_UNKNOWN_PROTOCOL, NetworkErrorUnknownProtocol);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NETWORK_ERROR_CANCELLED, NetworkErrorCancelled);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NETWORK_ERROR_FILE_DOES_NOT_EXIST, NetworkErrorFileDoesNotExist);

GQuark webkit_policy_error_quark()
{
    return g_quark_from_static_string(WebCore::errorDomainPolicy);
}

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_POLICY_ERROR_FAILED, PolicyErrorFailed);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_POLICY_ERROR_CANNOT_SHOW_MIME_TYPE, PolicyErrorCannotShowMimeType);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_POLICY_ERROR_CANNOT_SHOW_URI, PolicyErrorCannotShowURL);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_POLICY_ERROR_FRAME_LOAD_INTERRUPTED_BY_POLICY_CHANGE, PolicyErrorFrameLoadInterruptedByPolicyChange);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_POLICY_ERROR_CANNOT_USE_RESTRICTED_PORT, PolicyErrorCannotUseRestrictedPort);

GQuark webkit_plugin_error_quark()
{
    return g_quark_from_static_string(WebCore::errorDomainPlugin);
}

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PLUGIN_ERROR_FAILED, PluginErrorFailed);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PLUGIN_ERROR_CANNOT_FIND_PLUGIN, PluginErrorCannotFindPlugin);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PLUGIN_ERROR_CANNOT_LOAD_PLUGIN, PluginErrorCannotLoadPlugin);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PLUGIN_ERROR_JAVA_UNAVAILABLE, PluginErrorJavaUnavailable);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PLUGIN_ERROR_CONNECTION_CANCELLED, PluginErrorConnectionCancelled);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PLUGIN_ERROR_WILL_HANDLE_LOAD, PluginErrorWillHandleLoad);

GQuark webkit_download_error_quark()
{
    return g_quark_from_static_string(WebCore::errorDomainDownload);
}

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_DOWNLOAD_ERROR_NETWORK, DownloadErrorNetwork);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER, DownloadErrorCancelledByUser);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_DOWNLOAD_ERROR_DESTINATION, DownloadErrorDestination);

GQuark webkit_print_error_quark()
{
    return g_quark_from_static_string(WebCore::errorDomainPrint);
}

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PRINT_ERROR_GENERAL, PrintErrorGeneral);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PRINT_ERROR_PRINTER_NOT_FOUND, PrintErrorPrinterNotFound);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PRINT_ERROR_INVALID_PAGE_RANGE, PrintErrorInvalidPageRange);

GQuark webkit_javascript_error_quark()
{
    return g_quark_from_static_string("WebKitJavascriptError");
}

GQuark webkit_snapshot_error_quark()
{
    return g_quark_from_static_string("WebKitSnapshotError");
}
