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

#include "config.h"
#include "TimelineRecordFactory.h"

#if ENABLE(INSPECTOR)

#include "Event.h"
#include "InspectorValues.h"
#include "IntRect.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ScriptCallStack.h"
#include "ScriptCallStackFactory.h"

namespace WebCore {

PassRefPtr<InspectorObject> TimelineRecordFactory::createGenericRecord(double startTime)
{
    RefPtr<InspectorObject> record = InspectorObject::create();
    record->setNumber("startTime", startTime);

    RefPtr<ScriptCallStack> stackTrace = createScriptCallStack(5, true);
    if (stackTrace && stackTrace->size())
        record->setArray("stackTrace", stackTrace->buildInspectorArray());
    return record.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createGCEventData(const size_t usedHeapSizeDelta)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setNumber("usedHeapSizeDelta", usedHeapSizeDelta);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createFunctionCallData(const String& scriptName, int scriptLine)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setString("scriptName", scriptName);
    data->setNumber("scriptLine", scriptLine);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createEventDispatchData(const Event& event)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setString("type", event.type().string());
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createGenericTimerData(int timerId)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setNumber("timerId", timerId);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createTimerInstallData(int timerId, int timeout, bool singleShot)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setNumber("timerId", timerId);
    data->setNumber("timeout", timeout);
    data->setBoolean("singleShot", singleShot);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createXHRReadyStateChangeData(const String& url, int readyState)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setString("url", url);
    data->setNumber("readyState", readyState);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createXHRLoadData(const String& url)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setString("url", url);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createEvaluateScriptData(const String& url, double lineNumber)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setString("url", url);
    data->setNumber("lineNumber", lineNumber);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createMarkTimelineData(const String& message)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setString("message", message);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createScheduleResourceRequestData(const String& url)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setString("url", url);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createResourceSendRequestData(unsigned long identifier, const ResourceRequest& request)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setNumber("identifier", identifier);
    data->setString("url", request.url().string());
    data->setString("requestMethod", request.httpMethod());
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createResourceReceiveResponseData(unsigned long identifier, const ResourceResponse& response)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setNumber("identifier", identifier);
    data->setNumber("statusCode", response.httpStatusCode());
    data->setString("mimeType", response.mimeType());
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createResourceFinishData(unsigned long identifier, bool didFail, double finishTime)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setNumber("identifier", identifier);
    data->setBoolean("didFail", didFail);
    if (finishTime)
        data->setNumber("networkTime", finishTime);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createReceiveResourceData(unsigned long identifier)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setNumber("identifier", identifier);
    return data.release();
}
    
PassRefPtr<InspectorObject> TimelineRecordFactory::createPaintData(const IntRect& rect)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setNumber("x", rect.x());
    data->setNumber("y", rect.y());
    data->setNumber("width", rect.width());
    data->setNumber("height", rect.height());
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createParseHTMLData(unsigned int length, unsigned int startLine)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setNumber("length", length);
    data->setNumber("startLine", startLine);
    return data.release();
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
