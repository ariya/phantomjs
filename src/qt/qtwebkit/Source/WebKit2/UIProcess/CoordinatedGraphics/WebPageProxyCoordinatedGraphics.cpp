/*
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
#if USE(COORDINATED_GRAPHICS)

#include "WebPageProxy.h"

#include "PageClient.h"
#include "WebPageMessages.h"
#include "WebProcessProxy.h"

using namespace WebCore;

namespace WebKit {

void WebPageProxy::findZoomableAreaForPoint(const IntPoint& point, const IntSize& area)
{
    if (!isValid())
        return;

    m_process->send(Messages::WebPage::FindZoomableAreaForPoint(point, area), m_pageID);
}

void WebPageProxy::didFindZoomableArea(const IntPoint& target, const IntRect& area)
{
    m_pageClient->didFindZoomableArea(target, area);
}

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS)
