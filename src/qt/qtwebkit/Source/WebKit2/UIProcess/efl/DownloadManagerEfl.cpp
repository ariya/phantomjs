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
#include "DownloadManagerEfl.h"

#include "EwkView.h"
#include "WKContext.h"
#include "WKDownload.h"
#include "WKString.h"
#include "ewk_context_private.h"
#include "ewk_error_private.h"
#include "ewk_view.h"

using namespace EwkViewCallbacks;

namespace WebKit {

static inline DownloadManagerEfl* toDownloadManagerEfl(const void* clientInfo)
{
    return static_cast<DownloadManagerEfl*>(const_cast<void*>(clientInfo));
}

WKStringRef DownloadManagerEfl::decideDestinationWithSuggestedFilename(WKContextRef, WKDownloadRef wkDownload, WKStringRef filename, bool* /*allowOverwrite*/, const void* clientInfo)
{
    EwkDownloadJob* download = toDownloadManagerEfl(clientInfo)->ewkDownloadJob(wkDownload);
    ASSERT(download);

    download->setSuggestedFileName(toImpl(filename)->string().utf8().data());

    // We send the new download signal on the Ewk_View only once we have received the response
    // and the suggested file name.
    download->view()->smartCallback<DownloadJobRequested>().call(download);

    // DownloadSoup expects the destination to be a URL.
    String destination = ASCIILiteral("file://") + String::fromUTF8(download->destination());

    return WKStringCreateWithUTF8CString(destination.utf8().data());
}

void DownloadManagerEfl::didReceiveResponse(WKContextRef, WKDownloadRef wkDownload, WKURLResponseRef wkResponse, const void* clientInfo)
{
    EwkDownloadJob* download = toDownloadManagerEfl(clientInfo)->ewkDownloadJob(wkDownload);
    ASSERT(download);
    download->setResponse(EwkUrlResponse::create(wkResponse));
}

void DownloadManagerEfl::didCreateDestination(WKContextRef, WKDownloadRef wkDownload, WKStringRef /*path*/, const void* clientInfo)
{
    EwkDownloadJob* download = toDownloadManagerEfl(clientInfo)->ewkDownloadJob(wkDownload);
    ASSERT(download);

    download->setState(EWK_DOWNLOAD_JOB_STATE_DOWNLOADING);
}

void DownloadManagerEfl::didReceiveData(WKContextRef, WKDownloadRef wkDownload, uint64_t length, const void* clientInfo)
{
    EwkDownloadJob* download = toDownloadManagerEfl(clientInfo)->ewkDownloadJob(wkDownload);
    ASSERT(download);
    download->incrementReceivedData(length);
}

void DownloadManagerEfl::didFail(WKContextRef, WKDownloadRef wkDownload, WKErrorRef error, const void* clientInfo)
{
    DownloadManagerEfl* downloadManager = toDownloadManagerEfl(clientInfo);
    EwkDownloadJob* download = downloadManager->ewkDownloadJob(wkDownload);
    ASSERT(download);

    OwnPtr<EwkError> ewkError = EwkError::create(error);
    download->setState(EWK_DOWNLOAD_JOB_STATE_FAILED);
    Ewk_Download_Job_Error downloadError = { download, ewkError.get() };
    download->view()->smartCallback<DownloadJobFailed>().call(&downloadError);
    downloadManager->unregisterDownloadJob(wkDownload);
}

void DownloadManagerEfl::didCancel(WKContextRef, WKDownloadRef wkDownload, const void* clientInfo)
{
    DownloadManagerEfl* downloadManager = toDownloadManagerEfl(clientInfo);
    EwkDownloadJob* download = downloadManager->ewkDownloadJob(wkDownload);
    ASSERT(download);

    download->setState(EWK_DOWNLOAD_JOB_STATE_CANCELLED);
    download->view()->smartCallback<DownloadJobCancelled>().call(download);
    downloadManager->unregisterDownloadJob(wkDownload);
}

void DownloadManagerEfl::didFinish(WKContextRef, WKDownloadRef wkDownload, const void* clientInfo)
{
    DownloadManagerEfl* downloadManager = toDownloadManagerEfl(clientInfo);
    EwkDownloadJob* download = downloadManager->ewkDownloadJob(wkDownload);
    ASSERT(download);

    download->setState(EWK_DOWNLOAD_JOB_STATE_FINISHED);
    download->view()->smartCallback<DownloadJobFinished>().call(download);
    downloadManager->unregisterDownloadJob(wkDownload);
}

DownloadManagerEfl::DownloadManagerEfl(WKContextRef context)
    : m_context(context)
{
    WKContextDownloadClient wkDownloadClient;
    memset(&wkDownloadClient, 0, sizeof(WKContextDownloadClient));

    wkDownloadClient.version = kWKContextDownloadClientCurrentVersion;
    wkDownloadClient.clientInfo = this;
    wkDownloadClient.didCancel = didCancel;
    wkDownloadClient.decideDestinationWithSuggestedFilename = decideDestinationWithSuggestedFilename;
    wkDownloadClient.didCreateDestination = didCreateDestination;
    wkDownloadClient.didReceiveResponse = didReceiveResponse;
    wkDownloadClient.didReceiveData = didReceiveData;
    wkDownloadClient.didFail = didFail;
    wkDownloadClient.didFinish = didFinish;

    WKContextSetDownloadClient(m_context.get(), &wkDownloadClient);
}

DownloadManagerEfl::~DownloadManagerEfl()
{
    WKContextSetDownloadClient(m_context.get(), 0);
}

void DownloadManagerEfl::registerDownloadJob(WKDownloadRef download, EwkView* viewImpl)
{
    uint64_t downloadId = WKDownloadGetID(download);
    if (m_downloadJobs.contains(downloadId))
        return;

    RefPtr<EwkDownloadJob> ewkDownload = EwkDownloadJob::create(download, viewImpl);
    m_downloadJobs.add(downloadId, ewkDownload);
}

EwkDownloadJob* DownloadManagerEfl::ewkDownloadJob(WKDownloadRef wkDownload)
{
    return m_downloadJobs.get(WKDownloadGetID(wkDownload));
}

void DownloadManagerEfl::unregisterDownloadJob(WKDownloadRef wkDownload)
{
    m_downloadJobs.remove(WKDownloadGetID(wkDownload));
}

} // namespace WebKit
