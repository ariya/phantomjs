/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
#include "WebErrors.h"

#include "WKError.h"
#include "WebError.h"
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>

#include <QCoreApplication>
#include <QNetworkReply>

using namespace WebCore;

namespace WebKit {

ResourceError cancelledError(const ResourceRequest& request)
{
    ResourceError error = ResourceError("QtNetwork", QNetworkReply::OperationCanceledError, request.url().string(),
                                        QCoreApplication::translate("QWebFrame", "Request cancelled"));
    error.setIsCancellation(true);
    return error;    
}

ResourceError blockedError(const ResourceRequest& request)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodeCannotUseRestrictedPort, request.url().string(),
                         QCoreApplication::translate("QWebFrame", "Request blocked"));
}

ResourceError cannotShowURLError(const ResourceRequest& request)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodeCannotShowURL, request.url().string(),
                         QCoreApplication::translate("QWebFrame", "Cannot show URL"));
}

ResourceError interruptedForPolicyChangeError(const ResourceRequest& request)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodeFrameLoadInterruptedByPolicyChange, request.url().string(),
                         QCoreApplication::translate("QWebFrame", "Frame load interrupted by policy change"));
}

ResourceError cannotShowMIMETypeError(const ResourceResponse& response)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodeCannotShowMIMEType, response.url().string(),
                         QCoreApplication::translate("QWebFrame", "Cannot show mimetype"));
}

ResourceError fileDoesNotExistError(const ResourceResponse& response)
{
    return ResourceError("QtNetwork", QNetworkReply::ContentNotFoundError, response.url().string(),
                         QCoreApplication::translate("QWebFrame", "File does not exist"));
}

ResourceError pluginWillHandleLoadError(const ResourceResponse& response)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodePlugInWillHandleLoad, response.url().string(),
                         QCoreApplication::translate("QWebFrame", "Loading is handled by the media engine"));
}

} // namespace WebKit
