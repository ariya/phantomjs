/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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
#include "Download.h"

#include "QtFileDownloader.h"
#include "WebProcess.h"
#include <WebCore/NotImplemented.h>
#include <WebCore/QNetworkReplyHandler.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/ResourceHandleInternal.h>
#include <WebCore/ResourceResponse.h>

using namespace WebCore;

namespace WebKit {

void Download::start()
{
    QNetworkAccessManager* manager = WebProcess::shared().networkAccessManager();
    ASSERT(manager);
    ASSERT(!m_qtDownloader);

    m_qtDownloader = new QtFileDownloader(this, adoptPtr(manager->get(m_request.toNetworkRequest())));
    m_qtDownloader->init();
}

void Download::startWithHandle(ResourceHandle* handle, const ResourceResponse& resp)
{
    ASSERT(!m_qtDownloader);
    m_qtDownloader = new QtFileDownloader(this, adoptPtr(handle->getInternal()->m_job->release()));
    m_qtDownloader->init();
}

void Download::cancel()
{
    ASSERT(m_qtDownloader);
    m_qtDownloader->cancel();
}

void Download::platformInvalidate()
{
    ASSERT(m_qtDownloader);
    m_qtDownloader->deleteLater();
    m_qtDownloader = 0;
}

void Download::didDecideDestination(const String& destination, bool allowOverwrite)
{
    notImplemented();
}

void Download::startTransfer(const String& destination)
{
    m_qtDownloader->startTransfer(destination);
}

void Download::platformDidFinish()
{
    notImplemented();
}

void Download::receivedCredential(const AuthenticationChallenge& authenticationChallenge, const Credential& credential)
{
    notImplemented();
}

void Download::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge& authenticationChallenge)
{
    notImplemented();
}

void Download::receivedCancellation(const AuthenticationChallenge& authenticationChallenge)
{
    notImplemented();
}

} // namespace WebKit
