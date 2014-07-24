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

#if ENABLE(INSPECTOR)

#include "TimelineRecordFactory.h"

#include "Event.h"
#include "FloatQuad.h"
#include "InspectorValues.h"
#include "IntRect.h"
#include "LayoutRect.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ScriptCallStack.h"
#include "ScriptCallStackFactory.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

PassRefPtr<InspectorObject> TimelineRecordFactory::createGenericRecord(double startTime, int maxCallStackDepth)
{
    RefPtr<InspectorObject> record = InspectorObject::create();
    record->setNumber("startTime", startTime);

    if (maxCallStackDepth) {
        RefPtr<ScriptCallStack> stackTrace = createScriptCallStack(maxCallStackDepth, true);
        if (stackTrace && stackTrace->size())
            record->setValue("stackTrace", stackTrace->buildInspectorArray());
    }
    return record.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createBackgroundRecord(double startTime, const String& threadName)
{
    RefPtr<InspectorObject> record = InspectorObject::create();
    record->setNumber("startTime", startTime);
    record->setString("thread", threadName);
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

PassRefPtr<InspectorObject> TimelineRecordFactory::createTimeStampData(const String& message)
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

PassRefPtr<InspectorObject> TimelineRecordFactory::createResourceSendRequestData(const String& requestId, const ResourceRequest& request)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setString("requestId", requestId);
    data->setString("url", request.url().string());
    data->setString("requestMethod", request.httpMethod());
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createResourceReceiveResponseData(const String& requestId, const ResourceResponse& response)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setString("requestId", requestId);
    data->setNumber("statusCode", response.httpStatusCode());
    data->setString("mimeType", response.mimeType());
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createResourceFinishData(const String& requestId, bool didFail, double finishTime)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setString("requestId", requestId);
    data->setBoolean("didFail", didFail);
    if (finishTime)
        data->setNumber("networkTime", finishTime);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createReceiveResourceData(const String& requestId, int length)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setString("requestId", requestId);
    data->setNumber("encodedDataLength", length);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createLayoutData(unsigned dirtyObjects, unsigned totalObjects, bool partialLayout)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setNumber("dirtyObjects", dirtyObjects);
    data->setNumber("totalObjects", totalObjects);
    data->setBoolean("partialLayout", partialLayout);
    return data.release();
}
    
PassRefPtr<InspectorObject> TimelineRecordFactory::createDecodeImageData(const String& imageType)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setString("imageType", imageType);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createResizeImageData(bool shouldCache)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setBoolean("cached", shouldCache);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createMarkData(bool isMainFrame)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setBoolean("isMainFrame", isMainFrame);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createParseHTMLData(unsigned startLine)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setNumber("startLine", startLine);
    return data.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createAnimationFrameData(int callbackId)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setNumber("id", callbackId);
    return data.release();
}

static PassRefPtr<InspectorArray> createQuad(const FloatQuad& quad)
{
    RefPtr<InspectorArray> array = InspectorArray::create();
    array->pushNumber(quad.p1().x());
    array->pushNumber(quad.p1().y());
    array->pushNumber(quad.p2().x());
    array->pushNumber(quad.p2().y());
    array->pushNumber(quad.p3().x());
    array->pushNumber(quad.p3().y());
    array->pushNumber(quad.p4().x());
    array->pushNumber(quad.p4().y());
    return array.release();
}

PassRefPtr<InspectorObject> TimelineRecordFactory::createPaintData(const FloatQuad& quad)
{
    RefPtr<InspectorObject> data = InspectorObject::create();
    data->setArray("clip", createQuad(quad));
    return data.release();
}

void TimelineRecordFactory::appendLayoutRoot(InspectorObject* data, const FloatQuad& quad)
{
    data->setArray("root", createQuad(quad));
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
