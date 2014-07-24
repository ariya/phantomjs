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

#include "config.h"
#include "ErrorsGtk.h"

#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "PrintContext.h"
#include "ResourceError.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include <glib/gi18n-lib.h>

namespace WebCore {

ResourceError cancelledError(const ResourceRequest& request)
{
    return ResourceError(errorDomainNetwork, NetworkErrorCancelled,
                         request.url().string(), _("Load request cancelled"));
}

ResourceError blockedError(const ResourceRequest& request)
{
    return ResourceError(errorDomainPolicy, PolicyErrorCannotUseRestrictedPort,
                         request.url().string(), _("Not allowed to use restricted network port"));
}

ResourceError cannotShowURLError(const ResourceRequest& request)
{
    return ResourceError(errorDomainPolicy, PolicyErrorCannotShowURL,
                         request.url().string(), _("URL cannot be shown"));
}

ResourceError interruptedForPolicyChangeError(const ResourceRequest& request)
{
    return ResourceError(errorDomainPolicy, PolicyErrorFrameLoadInterruptedByPolicyChange,
                         request.url().string(), _("Frame load was interrupted"));
}

ResourceError cannotShowMIMETypeError(const ResourceResponse& response)
{
    return ResourceError(errorDomainPolicy, PolicyErrorCannotShowMimeType,
                         response.url().string(), _("Content with the specified MIME type cannot be shown"));
}

ResourceError fileDoesNotExistError(const ResourceResponse& response)
{
    return ResourceError(errorDomainNetwork, NetworkErrorFileDoesNotExist,
                         response.url().string(), _("File does not exist"));
}

ResourceError pluginWillHandleLoadError(const ResourceResponse& response)
{
    return ResourceError(errorDomainPlugin, PluginErrorWillHandleLoad,
                         response.url().string(), _("Plugin will handle load"));
}

ResourceError downloadNetworkError(const ResourceError& networkError)
{
    return ResourceError(errorDomainDownload, DownloadErrorNetwork,
                         networkError.failingURL(), networkError.localizedDescription());
}

ResourceError downloadCancelledByUserError(const ResourceResponse& response)
{
    return ResourceError(errorDomainDownload, DownloadErrorCancelledByUser,
                         response.url().string(), _("User cancelled the download"));
}

ResourceError downloadDestinationError(const ResourceResponse& response, const String& errorMessage)
{
    return ResourceError(errorDomainDownload, DownloadErrorDestination,
                         response.url().string(), errorMessage);
}

ResourceError printError(const PrintContext* printContext, const String& errorMessage)
{
    DocumentLoader* documentLoader = printContext->frame()->loader()->documentLoader();
    return ResourceError(errorDomainPrint, PrintErrorGeneral,
                         documentLoader ? documentLoader->url() : KURL(), errorMessage);
}

ResourceError printerNotFoundError(const PrintContext* printContext)
{
    DocumentLoader* documentLoader = printContext->frame()->loader()->documentLoader();
    return ResourceError(errorDomainPrint, PrintErrorPrinterNotFound,
                         documentLoader ? documentLoader->url() : KURL(), _("Printer not found"));
}

ResourceError invalidPageRangeToPrint(const PrintContext* printContext)
{
    DocumentLoader* documentLoader = printContext->frame()->loader()->documentLoader();
    return ResourceError(errorDomainPrint, PrintErrorInvalidPageRange,
                         documentLoader ? documentLoader->url() : KURL(), _("Invalid page range"));
}

} // namespace WebCore
