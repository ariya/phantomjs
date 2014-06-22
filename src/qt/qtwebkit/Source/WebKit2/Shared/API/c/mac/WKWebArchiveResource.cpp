/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "WKWebArchiveResource.h"

#include "WKSharedAPICast.h"
#include "WebArchiveResource.h"
#include "WebData.h"

using namespace WebKit;

WKTypeID WKWebArchiveResourceGetTypeID()
{
    return toAPI(WebArchiveResource::APIType);
}

WKWebArchiveResourceRef WKWebArchiveResourceCreate(WKDataRef dataRef, WKURLRef URLRef, WKStringRef MIMETypeRef, WKStringRef textEncodingRef)
{
    RefPtr<WebArchiveResource> webArchiveResource = WebArchiveResource::create(toImpl(dataRef), toWTFString(URLRef), toWTFString(MIMETypeRef), toWTFString(textEncodingRef));
    return toAPI(webArchiveResource.release().leakRef());
}

WKDataRef WKWebArchiveResourceCopyData(WKWebArchiveResourceRef webArchiveResourceRef)
{
    RefPtr<WebData> data = toImpl(webArchiveResourceRef)->data();
    return toAPI(data.release().leakRef());
}

WKURLRef WKWebArchiveResourceCopyURL(WKWebArchiveResourceRef webArchiveResourceRef)
{
    return toCopiedURLAPI(toImpl(webArchiveResourceRef)->URL());
}

WKStringRef WKWebArchiveResourceCopyMIMEType(WKWebArchiveResourceRef webArchiveResourceRef)
{
    return toCopiedAPI(toImpl(webArchiveResourceRef)->MIMEType());
}

WKStringRef WKWebArchiveResourceCopyTextEncoding(WKWebArchiveResourceRef webArchiveResourceRef)
{
    return toCopiedAPI(toImpl(webArchiveResourceRef)->textEncoding());
}
