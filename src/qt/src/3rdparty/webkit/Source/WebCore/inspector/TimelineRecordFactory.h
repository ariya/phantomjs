/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
 
#ifndef TimelineRecordFactory_h
#define TimelineRecordFactory_h

#include "PlatformString.h"

namespace WebCore {

    class Event;
    class InspectorFrontend;
    class InspectorObject;
    class IntRect;
    class ResourceRequest;
    class ResourceResponse;

    class TimelineRecordFactory {
    public:
        static PassRefPtr<InspectorObject> createGenericRecord(double startTime);

        static PassRefPtr<InspectorObject> createGCEventData(const size_t usedHeapSizeDelta);

        static PassRefPtr<InspectorObject> createFunctionCallData(const String& scriptName, int scriptLine);

        static PassRefPtr<InspectorObject> createEventDispatchData(const Event&);

        static PassRefPtr<InspectorObject> createGenericTimerData(int timerId);

        static PassRefPtr<InspectorObject> createTimerInstallData(int timerId, int timeout, bool singleShot);

        static PassRefPtr<InspectorObject> createXHRReadyStateChangeData(const String& url, int readyState);

        static PassRefPtr<InspectorObject> createXHRLoadData(const String& url);

        static PassRefPtr<InspectorObject> createEvaluateScriptData(const String&, double lineNumber);

        static PassRefPtr<InspectorObject> createMarkTimelineData(const String&);

        static PassRefPtr<InspectorObject> createResourceSendRequestData(unsigned long identifier, const ResourceRequest&);

        static PassRefPtr<InspectorObject> createScheduleResourceRequestData(const String&);

        static PassRefPtr<InspectorObject> createResourceReceiveResponseData(unsigned long identifier, const ResourceResponse&);

        static PassRefPtr<InspectorObject> createReceiveResourceData(unsigned long identifier);

        static PassRefPtr<InspectorObject> createResourceFinishData(unsigned long identifier, bool didFail, double finishTime);

        static PassRefPtr<InspectorObject> createPaintData(const IntRect&);

        static PassRefPtr<InspectorObject> createParseHTMLData(unsigned int length, unsigned int startLine);

    private:
        TimelineRecordFactory() { }
    };

} // namespace WebCore

#endif // !defined(TimelineRecordFactory_h)
