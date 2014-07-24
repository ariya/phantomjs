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

#ifndef ewk_download_job_private_h
#define ewk_download_job_private_h

#include "WKBase.h"
#include "WKDownload.h"
#include "WKEinaSharedString.h"
#include "WKRetainPtr.h"
#include "ewk_download_job.h"
#include "ewk_url_request_private.h"
#include "ewk_url_response_private.h"
#include <Evas.h>
#include <wtf/PassRefPtr.h>

class EwkView;

class EwkDownloadJob : public EwkObject {
public:
    EWK_OBJECT_DECLARE(EwkDownloadJob)

    static PassRefPtr<EwkDownloadJob> create(WKDownloadRef download, EwkView* viewImpl)
    {
        return adoptRef(new EwkDownloadJob(download, viewImpl));
    }

    uint64_t id() const;
    EwkView* view() const;

    Ewk_Download_Job_State state() const;
    void setState(Ewk_Download_Job_State);

    EwkUrlRequest* request() const;
    EwkUrlResponse* response() const;
    void setResponse(PassRefPtr<EwkUrlResponse>);

    const char* destination() const;
    void setDestination(const char* destination);

    const char* suggestedFileName() const;
    void setSuggestedFileName(const char* fileName);

    bool cancel();

    double estimatedProgress() const;
    double elapsedTime() const;
    void incrementReceivedData(uint64_t length);

private:
    EwkDownloadJob(WKDownloadRef download, EwkView* view);

    WKRetainPtr<WKDownloadRef> m_download;
    EwkView* m_viewImpl;
    Ewk_Download_Job_State m_state;
    mutable RefPtr<EwkUrlRequest> m_request;
    RefPtr<EwkUrlResponse> m_response;
    double m_startTime;
    double m_endTime;
    uint64_t m_downloaded; // length already downloaded
    WKEinaSharedString m_destination;
    WKEinaSharedString m_suggestedFilename;
};

#endif // ewk_download_job_private_h
