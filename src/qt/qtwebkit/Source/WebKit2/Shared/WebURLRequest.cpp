/*
 *  WebURLRequest.cpp
 *  WebKit2
 *
 *  Created by Sam Weinig on 8/30/10.
 *  Copyright 2010 Apple Inc. All rights reserved.
 *
 */

#include "config.h"
#include "WebURLRequest.h"

#include "WebContext.h"

using namespace WebCore;

namespace WebKit {

PassRefPtr<WebURLRequest> WebURLRequest::create(const KURL& url)
{
    return adoptRef(new WebURLRequest(ResourceRequest(url)));
}

WebURLRequest::WebURLRequest(const ResourceRequest& request)
    : m_request(request)
{
}

double WebURLRequest::defaultTimeoutInterval()
{
    return ResourceRequest::defaultTimeoutInterval();
}

// FIXME: This function should really be on WebContext.
void WebURLRequest::setDefaultTimeoutInterval(double timeoutInterval)
{
    ResourceRequest::setDefaultTimeoutInterval(timeoutInterval);

    const Vector<WebContext*>& contexts = WebContext::allContexts();
    for (size_t i = 0; i < contexts.size(); ++i)
        contexts[i]->setDefaultRequestTimeoutInterval(timeoutInterval);
}

} // namespace WebKit
