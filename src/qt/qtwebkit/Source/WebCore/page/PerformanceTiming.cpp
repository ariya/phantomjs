/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "config.h"
#include "PerformanceTiming.h"

#if ENABLE(WEB_TIMING)

#include "Document.h"
#include "DocumentLoadTiming.h"
#include "DocumentLoader.h"
#include "DocumentTiming.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "ResourceLoadTiming.h"
#include "ResourceResponse.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

static unsigned long long toIntegerMilliseconds(double seconds)
{
    ASSERT(seconds >= 0);
    return static_cast<unsigned long long>(seconds * 1000.0);
}

PerformanceTiming::PerformanceTiming(Frame* frame)
    : DOMWindowProperty(frame)
{
}

unsigned long long PerformanceTiming::navigationStart() const
{
    DocumentLoadTiming* timing = documentLoadTiming();
    if (!timing)
        return 0;

    return monotonicTimeToIntegerMilliseconds(timing->navigationStart());
}

unsigned long long PerformanceTiming::unloadEventStart() const
{
    DocumentLoadTiming* timing = documentLoadTiming();
    if (!timing)
        return 0;

    if (timing->hasCrossOriginRedirect() || !timing->hasSameOriginAsPreviousDocument())
        return 0;

    return monotonicTimeToIntegerMilliseconds(timing->unloadEventStart());
}

unsigned long long PerformanceTiming::unloadEventEnd() const
{
    DocumentLoadTiming* timing = documentLoadTiming();
    if (!timing)
        return 0;

    if (timing->hasCrossOriginRedirect() || !timing->hasSameOriginAsPreviousDocument())
        return 0;

    return monotonicTimeToIntegerMilliseconds(timing->unloadEventEnd());
}

unsigned long long PerformanceTiming::redirectStart() const
{
    DocumentLoadTiming* timing = documentLoadTiming();
    if (!timing)
        return 0;

    if (timing->hasCrossOriginRedirect())
        return 0;

    return monotonicTimeToIntegerMilliseconds(timing->redirectStart());
}

unsigned long long PerformanceTiming::redirectEnd() const
{
    DocumentLoadTiming* timing = documentLoadTiming();
    if (!timing)
        return 0;

    if (timing->hasCrossOriginRedirect())
        return 0;

    return monotonicTimeToIntegerMilliseconds(timing->redirectEnd());
}

unsigned long long PerformanceTiming::fetchStart() const
{
    DocumentLoadTiming* timing = documentLoadTiming();
    if (!timing)
        return 0;

    return monotonicTimeToIntegerMilliseconds(timing->fetchStart());
}

unsigned long long PerformanceTiming::domainLookupStart() const
{
    ResourceLoadTiming* timing = resourceLoadTiming();
    if (!timing)
        return fetchStart();

    // This will be -1 when a DNS request is not performed.
    // Rather than exposing a special value that indicates no DNS, we "backfill" with fetchStart.
    int dnsStart = timing->dnsStart;
    if (dnsStart < 0)
        return fetchStart();

    return resourceLoadTimeRelativeToAbsolute(dnsStart);
}

unsigned long long PerformanceTiming::domainLookupEnd() const
{
    ResourceLoadTiming* timing = resourceLoadTiming();
    if (!timing)
        return domainLookupStart();

    // This will be -1 when a DNS request is not performed.
    // Rather than exposing a special value that indicates no DNS, we "backfill" with domainLookupStart.
    int dnsEnd = timing->dnsEnd;
    if (dnsEnd < 0)
        return domainLookupStart();

    return resourceLoadTimeRelativeToAbsolute(dnsEnd);
}

unsigned long long PerformanceTiming::connectStart() const
{
    DocumentLoader* loader = documentLoader();
    if (!loader)
        return domainLookupEnd();

    ResourceLoadTiming* timing = loader->response().resourceLoadTiming();
    if (!timing)
        return domainLookupEnd();

    // connectStart will be -1 when a network request is not made.
    // Rather than exposing a special value that indicates no new connection, we "backfill" with domainLookupEnd.
    int connectStart = timing->connectStart;
    if (connectStart < 0 || loader->response().connectionReused())
        return domainLookupEnd();

    // ResourceLoadTiming's connect phase includes DNS, however Navigation Timing's
    // connect phase should not. So if there is DNS time, trim it from the start.
    if (timing->dnsEnd >= 0 && timing->dnsEnd > connectStart)
        connectStart = timing->dnsEnd;

    return resourceLoadTimeRelativeToAbsolute(connectStart);
}

unsigned long long PerformanceTiming::connectEnd() const
{
    DocumentLoader* loader = documentLoader();
    if (!loader)
        return connectStart();

    ResourceLoadTiming* timing = loader->response().resourceLoadTiming();
    if (!timing)
        return connectStart();

    // connectEnd will be -1 when a network request is not made.
    // Rather than exposing a special value that indicates no new connection, we "backfill" with connectStart.
    int connectEnd = timing->connectEnd;
    if (connectEnd < 0 || loader->response().connectionReused())
        return connectStart();

    return resourceLoadTimeRelativeToAbsolute(connectEnd);
}

unsigned long long PerformanceTiming::secureConnectionStart() const
{
    DocumentLoader* loader = documentLoader();
    if (!loader)
        return 0;

    ResourceLoadTiming* timing = loader->response().resourceLoadTiming();
    if (!timing)
        return 0;

    int sslStart = timing->sslStart;
    if (sslStart < 0)
        return 0;

    return resourceLoadTimeRelativeToAbsolute(sslStart);
}

unsigned long long PerformanceTiming::requestStart() const
{
    ResourceLoadTiming* timing = resourceLoadTiming();
    if (!timing)
        return connectEnd();

    ASSERT(timing->sendStart >= 0);
    return resourceLoadTimeRelativeToAbsolute(timing->sendStart);
}

unsigned long long PerformanceTiming::responseStart() const
{
    ResourceLoadTiming* timing = resourceLoadTiming();
    if (!timing)
        return requestStart();

    // FIXME: Response start needs to be the time of the first received byte.
    // However, the ResourceLoadTiming API currently only supports the time
    // the last header byte was received. For many responses with reasonable
    // sized cookies, the HTTP headers fit into a single packet so this time
    // is basically equivalent. But for some responses, particularly those with
    // headers larger than a single packet, this time will be too late.
    ASSERT(timing->receiveHeadersEnd >= 0);
    return resourceLoadTimeRelativeToAbsolute(timing->receiveHeadersEnd);
}

unsigned long long PerformanceTiming::responseEnd() const
{
    DocumentLoadTiming* timing = documentLoadTiming();
    if (!timing)
        return 0;

    return monotonicTimeToIntegerMilliseconds(timing->responseEnd());
}

unsigned long long PerformanceTiming::domLoading() const
{
    const DocumentTiming* timing = documentTiming();
    if (!timing)
        return fetchStart();

    return monotonicTimeToIntegerMilliseconds(timing->domLoading);
}

unsigned long long PerformanceTiming::domInteractive() const
{
    const DocumentTiming* timing = documentTiming();
    if (!timing)
        return 0;

    return monotonicTimeToIntegerMilliseconds(timing->domInteractive);
}

unsigned long long PerformanceTiming::domContentLoadedEventStart() const
{
    const DocumentTiming* timing = documentTiming();
    if (!timing)
        return 0;

    return monotonicTimeToIntegerMilliseconds(timing->domContentLoadedEventStart);
}

unsigned long long PerformanceTiming::domContentLoadedEventEnd() const
{
    const DocumentTiming* timing = documentTiming();
    if (!timing)
        return 0;

    return monotonicTimeToIntegerMilliseconds(timing->domContentLoadedEventEnd);
}

unsigned long long PerformanceTiming::domComplete() const
{
    const DocumentTiming* timing = documentTiming();
    if (!timing)
        return 0;

    return monotonicTimeToIntegerMilliseconds(timing->domComplete);
}

unsigned long long PerformanceTiming::loadEventStart() const
{
    DocumentLoadTiming* timing = documentLoadTiming();
    if (!timing)
        return 0;

    return monotonicTimeToIntegerMilliseconds(timing->loadEventStart());
}

unsigned long long PerformanceTiming::loadEventEnd() const
{
    DocumentLoadTiming* timing = documentLoadTiming();
    if (!timing)
        return 0;

    return monotonicTimeToIntegerMilliseconds(timing->loadEventEnd());
}

DocumentLoader* PerformanceTiming::documentLoader() const
{
    if (!m_frame)
        return 0;

    return m_frame->loader()->documentLoader();
}

const DocumentTiming* PerformanceTiming::documentTiming() const
{
    if (!m_frame)
        return 0;

    Document* document = m_frame->document();
    if (!document)
        return 0;

    return document->timing();
}

DocumentLoadTiming* PerformanceTiming::documentLoadTiming() const
{
    DocumentLoader* loader = documentLoader();
    if (!loader)
        return 0;

    return loader->timing();
}

ResourceLoadTiming* PerformanceTiming::resourceLoadTiming() const
{
    DocumentLoader* loader = documentLoader();
    if (!loader)
        return 0;

    return loader->response().resourceLoadTiming();
}

unsigned long long PerformanceTiming::resourceLoadTimeRelativeToAbsolute(int relativeMilliseconds) const
{
    ASSERT(relativeMilliseconds >= 0);
    ResourceLoadTiming* resourceTiming = resourceLoadTiming();
    ASSERT(resourceTiming);
    return monotonicTimeToIntegerMilliseconds(resourceTiming->convertResourceLoadTimeToMonotonicTime(relativeMilliseconds));
}

unsigned long long PerformanceTiming::monotonicTimeToIntegerMilliseconds(double monotonicSeconds) const
{
    ASSERT(monotonicSeconds >= 0);
    const DocumentLoadTiming* timing = documentLoadTiming();
    ASSERT(timing);
    return toIntegerMilliseconds(timing->monotonicTimeToPseudoWallTime(monotonicSeconds));
}

} // namespace WebCore

#endif // ENABLE(WEB_TIMING)
