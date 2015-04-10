/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#include "AuthenticationManager.h"
#include "Connection.h"
#include "DataReference.h"
#include "DownloadAuthenticationClient.h"
#include "DownloadProxyMessages.h"
#include "DownloadManager.h"
#include "SandboxExtension.h"
#include "WebCoreArgumentCoders.h"

using namespace WebCore;

namespace WebKit {

PassOwnPtr<Download> Download::create(DownloadManager& downloadManager, uint64_t downloadID, const ResourceRequest& request)
{
    return adoptPtr(new Download(downloadManager, downloadID, request));
}

Download::Download(DownloadManager& downloadManager, uint64_t downloadID, const ResourceRequest& request)
    : m_downloadManager(downloadManager)
    , m_downloadID(downloadID)
    , m_request(request)
#if USE(CFNETWORK)
    , m_allowOverwrite(false)
#endif
#if PLATFORM(QT)
    , m_qtDownloader(0)
#endif
{
    ASSERT(m_downloadID);

    m_downloadManager.didCreateDownload();
}

Download::~Download()
{
    platformInvalidate();

    m_downloadManager.didDestroyDownload();
}

void Download::didStart()
{
    send(Messages::DownloadProxy::DidStart(m_request));
}

void Download::didReceiveAuthenticationChallenge(const AuthenticationChallenge& authenticationChallenge)
{
    m_downloadManager.downloadsAuthenticationManager().didReceiveAuthenticationChallenge(this, authenticationChallenge);
}

void Download::didReceiveResponse(const ResourceResponse& response)
{
    send(Messages::DownloadProxy::DidReceiveResponse(response));
}

void Download::didReceiveData(uint64_t length)
{
    send(Messages::DownloadProxy::DidReceiveData(length));
}

bool Download::shouldDecodeSourceDataOfMIMEType(const String& mimeType)
{
    bool result;
    if (!sendSync(Messages::DownloadProxy::ShouldDecodeSourceDataOfMIMEType(mimeType), Messages::DownloadProxy::ShouldDecodeSourceDataOfMIMEType::Reply(result)))
        return true;

    return result;
}

String Download::retrieveDestinationWithSuggestedFilename(const String& filename, bool& allowOverwrite)
{
    String destination;
    SandboxExtension::Handle sandboxExtensionHandle;
    if (!sendSync(Messages::DownloadProxy::DecideDestinationWithSuggestedFilename(filename), Messages::DownloadProxy::DecideDestinationWithSuggestedFilename::Reply(destination, allowOverwrite, sandboxExtensionHandle)))
        return String();

    m_sandboxExtension = SandboxExtension::create(sandboxExtensionHandle);
    if (m_sandboxExtension)
        m_sandboxExtension->consume();

    return destination;
}

String Download::decideDestinationWithSuggestedFilename(const String& filename, bool& allowOverwrite)
{
    String destination = retrieveDestinationWithSuggestedFilename(filename, allowOverwrite);

    didDecideDestination(destination, allowOverwrite);

    return destination;
}

void Download::didCreateDestination(const String& path)
{
    send(Messages::DownloadProxy::DidCreateDestination(path));
}

void Download::didFinish()
{
    platformDidFinish();

    send(Messages::DownloadProxy::DidFinish());

    if (m_sandboxExtension) {
        m_sandboxExtension->revoke();
        m_sandboxExtension = nullptr;
    }

    m_downloadManager.downloadFinished(this);
}

void Download::didFail(const ResourceError& error, const CoreIPC::DataReference& resumeData)
{
    send(Messages::DownloadProxy::DidFail(error, resumeData));

    if (m_sandboxExtension) {
        m_sandboxExtension->revoke();
        m_sandboxExtension = nullptr;
    }
    m_downloadManager.downloadFinished(this);
}

void Download::didCancel(const CoreIPC::DataReference& resumeData)
{
    send(Messages::DownloadProxy::DidCancel(resumeData));

    if (m_sandboxExtension) {
        m_sandboxExtension->revoke();
        m_sandboxExtension = nullptr;
    }
    m_downloadManager.downloadFinished(this);
}

CoreIPC::Connection* Download::messageSenderConnection()
{
    return m_downloadManager.downloadProxyConnection();
}

uint64_t Download::messageSenderDestinationID()
{
    return m_downloadID;
}

} // namespace WebKit
