/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "WebOpenPanelResultListenerProxy.h"

#include "ImmutableArray.h"
#include "WebPageProxy.h"
#include <WebCore/KURL.h>
#include <wtf/Vector.h>

using namespace WebCore;

namespace WebKit {

WebOpenPanelResultListenerProxy::WebOpenPanelResultListenerProxy(WebPageProxy* page)
    : m_page(page)
{
}

WebOpenPanelResultListenerProxy::~WebOpenPanelResultListenerProxy()
{
}

void WebOpenPanelResultListenerProxy::chooseFiles(ImmutableArray* fileURLsArray)
{
    if (!m_page)
        return;

    size_t size = fileURLsArray->size();

    Vector<String> filePaths;
    filePaths.reserveInitialCapacity(size);

    for (size_t i = 0; i < size; ++i) {
        WebURL* webURL = fileURLsArray->at<WebURL>(i);
        if (webURL) {
            KURL url(KURL(), webURL->string()); 
            filePaths.uncheckedAppend(url.fileSystemPath());
        }
    }

    m_page->didChooseFilesForOpenPanel(filePaths);
}

void WebOpenPanelResultListenerProxy::cancel()
{
    if (!m_page)
        return;

    m_page->didCancelForOpenPanel();
}

void WebOpenPanelResultListenerProxy::invalidate()
{
    m_page = 0;
}

} // namespace WebKit
