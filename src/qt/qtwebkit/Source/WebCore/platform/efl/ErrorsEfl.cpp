/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ErrorsEfl.h"

#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "PrintContext.h"
#include "ResourceError.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"

namespace WebCore {

ResourceError cancelledError(const ResourceRequest& request)
{
    return ResourceError(errorDomainNetwork, NetworkErrorCancelled, request.url().string(), "Load request cancelled");
}

ResourceError blockedError(const ResourceRequest& request)
{
    return ResourceError(errorDomainPolicy, PolicyErrorCannotUseRestrictedPort, request.url().string(), "Not allowed to use restricted network port");
}

ResourceError cannotShowURLError(const ResourceRequest& request)
{
    return ResourceError(errorDomainPolicy, PolicyErrorCannotShowURL, request.url().string(), "URL cannot be shown");
}

ResourceError interruptedForPolicyChangeError(const ResourceRequest& request)
{
    return ResourceError(errorDomainPolicy, PolicyErrorFrameLoadInterruptedByPolicyChange, request.url().string(), "Frame load was interrupted");
}

ResourceError cannotShowMIMETypeError(const ResourceResponse& response)
{
    return ResourceError(errorDomainPolicy, PolicyErrorCannotShowMimeType, response.url().string(), "Content with the specified MIME type cannot be shown");
}

ResourceError fileDoesNotExistError(const ResourceResponse& response)
{
    return ResourceError(errorDomainNetwork, NetworkErrorFileDoesNotExist, response.url().string(), "File does not exist");
}

ResourceError pluginWillHandleLoadError(const ResourceResponse& response)
{
    return ResourceError(errorDomainPlugin, PluginErrorWillHandleLoad, response.url().string(), "Plugin will handle load");
}

ResourceError downloadNetworkError(const ResourceError& networkError)
{
    return ResourceError(errorDomainDownload, DownloadErrorNetwork, networkError.failingURL(), networkError.localizedDescription());
}

ResourceError downloadCancelledByUserError(const ResourceResponse& response)
{
    return ResourceError(errorDomainDownload, DownloadErrorCancelledByUser, response.url().string(), "User cancelled the download");
}

ResourceError downloadDestinationError(const ResourceResponse& response, const String& errorMessage)
{
    return ResourceError(errorDomainDownload, DownloadErrorDestination, response.url().string(), errorMessage);
}

ResourceError printError(const PrintContext* printContext, const String& errorMessage)
{
    DocumentLoader* documentLoader = printContext->frame()->loader()->documentLoader();

    return ResourceError(errorDomainPrint, PrintErrorGeneral, documentLoader ? documentLoader->url() : KURL(), errorMessage);
}

ResourceError printerNotFoundError(const PrintContext* printContext)
{
    DocumentLoader* documentLoader = printContext->frame()->loader()->documentLoader();

    return ResourceError(errorDomainPrint, PrintErrorPrinterNotFound, documentLoader ? documentLoader->url() : KURL(), "Printer not found");
}

ResourceError invalidPageRangeToPrint(const PrintContext* printContext)
{
    DocumentLoader* documentLoader = printContext->frame()->loader()->documentLoader();

    return ResourceError(errorDomainPrint, PrintErrorInvalidPageRange, documentLoader ? documentLoader->url() : KURL(), "Invalid page range");
}

} // namespace WebCore
