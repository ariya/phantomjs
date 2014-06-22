/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "DownloadProxyMap.h"

#include "ChildProcessProxy.h"
#include "DownloadProxy.h"
#include "DownloadProxyMessages.h"
#include "MessageReceiverMap.h"
#include <wtf/StdLibExtras.h>

namespace WebKit {

DownloadProxyMap::DownloadProxyMap(ChildProcessProxy* process)
    : m_process(process)
{
}

DownloadProxyMap::~DownloadProxyMap()
{
    ASSERT(m_downloads.isEmpty());
}

DownloadProxy* DownloadProxyMap::createDownloadProxy(WebContext* webContext)
{
    RefPtr<DownloadProxy> downloadProxy = DownloadProxy::create(*this, webContext);
    m_downloads.set(downloadProxy->downloadID(), downloadProxy);

    m_process->addMessageReceiver(Messages::DownloadProxy::messageReceiverName(), downloadProxy->downloadID(), downloadProxy.get());

    return downloadProxy.get();
}

void DownloadProxyMap::downloadFinished(DownloadProxy* downloadProxy)
{
    ASSERT(m_downloads.contains(downloadProxy->downloadID()));

    downloadProxy->invalidate();
    m_downloads.remove(downloadProxy->downloadID());

    m_process->removeMessageReceiver(Messages::DownloadProxy::messageReceiverName(), downloadProxy->downloadID());
}

void DownloadProxyMap::processDidClose()
{
    // Invalidate all outstanding downloads.
    for (HashMap<uint64_t, RefPtr<DownloadProxy> >::iterator::Values it = m_downloads.begin().values(), end = m_downloads.end().values(); it != end; ++it) {
        (*it)->processDidClose();
        (*it)->invalidate();
    }

    m_downloads.clear();
    m_process = 0;
}

} // namespace WebKit
