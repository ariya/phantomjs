/*
 * Copyright (C) 2011 Igalia S.L.
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

#ifndef ErrorsGtk_h
#define ErrorsGtk_h

#include <wtf/text/WTFString.h>

namespace WebCore {

class PrintContext;
class ResourceError;
class ResourceRequest;
class ResourceResponse;

const char* const errorDomainNetwork = "WebKitNetworkError";
const char* const errorDomainPolicy = "WebKitPolicyError";
const char* const errorDomainPlugin = "WebKitPluginError";
const char* const errorDomainDownload = "WebKitDownloadError";
const char* const errorDomainPrint = "WebKitPrintError";

enum NetworkError {
    NetworkErrorFailed = 399,
    NetworkErrorTransport = 300,
    NetworkErrorUnknownProtocol = 301,
    NetworkErrorCancelled = 302,
    NetworkErrorFileDoesNotExist = 303
};

// Sync'd with Mac's WebKit Errors.
enum PolicyError {
    PolicyErrorFailed = 199,
    PolicyErrorCannotShowMimeType = 100,
    PolicyErrorCannotShowURL = 101,
    PolicyErrorFrameLoadInterruptedByPolicyChange = 102,
    PolicyErrorCannotUseRestrictedPort = 103
};

enum PluginError {
    PluginErrorFailed = 299,
    PluginErrorCannotFindPlugin = 200,
    PluginErrorCannotLoadPlugin = 201,
    PluginErrorJavaUnavailable = 202,
    PluginErrorConnectionCancelled = 203,
    PluginErrorWillHandleLoad = 204
};

enum DownloadError {
    DownloadErrorNetwork = 499,
    DownloadErrorCancelledByUser = 400,
    DownloadErrorDestination = 401
};

enum PrintError {
    PrintErrorGeneral = 599,
    PrintErrorPrinterNotFound = 500,
    PrintErrorInvalidPageRange = 501
};

ResourceError cancelledError(const ResourceRequest&);
ResourceError blockedError(const ResourceRequest&);
ResourceError cannotShowURLError(const ResourceRequest&);
ResourceError interruptedForPolicyChangeError(const ResourceRequest&);
ResourceError cannotShowMIMETypeError(const ResourceResponse&);
ResourceError fileDoesNotExistError(const ResourceResponse&);
ResourceError pluginWillHandleLoadError(const ResourceResponse&);
ResourceError downloadNetworkError(const ResourceError&);
ResourceError downloadCancelledByUserError(const ResourceResponse&);
ResourceError downloadDestinationError(const ResourceResponse&, const String& errorMessage);
ResourceError printError(const PrintContext*, const String& errorMessage);
ResourceError printerNotFoundError(const PrintContext*);
ResourceError invalidPageRangeToPrint(const PrintContext*);

}

#endif
