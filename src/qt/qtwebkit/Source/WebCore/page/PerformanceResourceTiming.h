/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2012 Intel Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PerformanceResourceTiming_h
#define PerformanceResourceTiming_h

#if ENABLE(RESOURCE_TIMING)

#include "PerformanceEntry.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class Document;
class KURL;
class ResourceLoadTiming;
class ResourceRequest;
class ResourceResponse;

class PerformanceResourceTiming : public PerformanceEntry {
public:
    static PassRefPtr<PerformanceResourceTiming> create(const AtomicString& initiatorType, const ResourceRequest& request, const ResourceResponse& response, double initiationTime, double finishTime, Document* requestingDocument)
    {
        return adoptRef(new PerformanceResourceTiming(initiatorType, request, response, initiationTime, finishTime, requestingDocument));
    }

    AtomicString initiatorType() const;

    double redirectStart() const;
    double redirectEnd() const;
    double fetchStart() const;
    double domainLookupStart() const;
    double domainLookupEnd() const;
    double connectStart() const;
    double connectEnd() const;
    double secureConnectionStart() const;
    double requestStart() const;
    double responseStart() const;
    double responseEnd() const;

    virtual bool isResource() { return true; }

private:
    PerformanceResourceTiming(const AtomicString& initatorType, const ResourceRequest&, const ResourceResponse&, double initiationTime, double finishTime, Document*);
    ~PerformanceResourceTiming();

    double resourceTimeToDocumentMilliseconds(int deltaMilliseconds) const;

    AtomicString m_initiatorType;
    RefPtr<ResourceLoadTiming> m_timing;
    double m_finishTime;
    bool m_didReuseConnection;
    bool m_shouldReportDetails;
    RefPtr<Document> m_requestingDocument;
};

}

#endif // ENABLE(RESOURCE_TIMING)

#endif // !defined(PerformanceResourceTiming_h)
