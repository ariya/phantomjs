/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "InjectedScriptCanvasModule.h"

#include "InjectedScript.h"
#include "InjectedScriptCanvasModuleSource.h"
#include "InjectedScriptManager.h"
#include "ScriptFunctionCall.h"
#include "ScriptObject.h"

using WebCore::TypeBuilder::Array;
using WebCore::TypeBuilder::Canvas::ResourceId;
using WebCore::TypeBuilder::Canvas::ResourceInfo;
using WebCore::TypeBuilder::Canvas::ResourceState;
using WebCore::TypeBuilder::Canvas::TraceLog;
using WebCore::TypeBuilder::Canvas::TraceLogId;

namespace WebCore {

InjectedScriptCanvasModule::InjectedScriptCanvasModule()
    : InjectedScriptModule("InjectedScriptCanvasModule")
{
}

InjectedScriptCanvasModule InjectedScriptCanvasModule::moduleForState(InjectedScriptManager* injectedScriptManager, ScriptState* scriptState)
{
    InjectedScriptCanvasModule result;
    result.ensureInjected(injectedScriptManager, scriptState);
    return result;
}

String InjectedScriptCanvasModule::source() const
{
    return String(reinterpret_cast<const char*>(InjectedScriptCanvasModuleSource_js), sizeof(InjectedScriptCanvasModuleSource_js));
}

ScriptObject InjectedScriptCanvasModule::wrapCanvas2DContext(const ScriptObject& context)
{
    return callWrapContextFunction("wrapCanvas2DContext", context);
}

#if ENABLE(WEBGL)
ScriptObject InjectedScriptCanvasModule::wrapWebGLContext(const ScriptObject& glContext)
{
    return callWrapContextFunction("wrapWebGLContext", glContext);
}
#endif // ENABLE(WEBGL)

ScriptObject InjectedScriptCanvasModule::callWrapContextFunction(const String& functionName, const ScriptObject& context)
{
    ScriptFunctionCall function(injectedScriptObject(), functionName);
    function.appendArgument(context);
    bool hadException = false;
    ScriptValue resultValue = callFunctionWithEvalEnabled(function, hadException);
    if (hadException || resultValue.hasNoValue() || !resultValue.isObject()) {
        ASSERT_NOT_REACHED();
        return ScriptObject();
    }
    return ScriptObject(context.scriptState(), resultValue);
}

void InjectedScriptCanvasModule::markFrameEnd()
{
    ScriptFunctionCall function(injectedScriptObject(), "markFrameEnd");
    RefPtr<InspectorValue> resultValue;
    makeCall(function, &resultValue);
    ASSERT(resultValue);
}

void InjectedScriptCanvasModule::captureFrame(ErrorString* errorString, TraceLogId* traceLogId)
{
    callStartCapturingFunction("captureFrame", errorString, traceLogId);
}

void InjectedScriptCanvasModule::startCapturing(ErrorString* errorString, TraceLogId* traceLogId)
{
    callStartCapturingFunction("startCapturing", errorString, traceLogId);
}

void InjectedScriptCanvasModule::callStartCapturingFunction(const String& functionName, ErrorString* errorString, TraceLogId* traceLogId)
{
    ScriptFunctionCall function(injectedScriptObject(), functionName);
    RefPtr<InspectorValue> resultValue;
    makeCall(function, &resultValue);
    if (!resultValue || resultValue->type() != InspectorValue::TypeString || !resultValue->asString(traceLogId))
        *errorString = "Internal error: " + functionName;
}

void InjectedScriptCanvasModule::stopCapturing(ErrorString* errorString, const TraceLogId& traceLogId)
{
    callVoidFunctionWithTraceLogIdArgument("stopCapturing", errorString, traceLogId);
}

void InjectedScriptCanvasModule::dropTraceLog(ErrorString* errorString, const TraceLogId& traceLogId)
{
    callVoidFunctionWithTraceLogIdArgument("dropTraceLog", errorString, traceLogId);
}

void InjectedScriptCanvasModule::callVoidFunctionWithTraceLogIdArgument(const String& functionName, ErrorString* errorString, const TraceLogId& traceLogId)
{
    ScriptFunctionCall function(injectedScriptObject(), functionName);
    function.appendArgument(traceLogId);
    bool hadException = false;
    callFunctionWithEvalEnabled(function, hadException);
    ASSERT(!hadException);
    if (hadException)
        *errorString = "Internal error: " + functionName;
}

void InjectedScriptCanvasModule::traceLog(ErrorString* errorString, const TraceLogId& traceLogId, const int* startOffset, const int* maxLength, RefPtr<TraceLog>* traceLog)
{
    ScriptFunctionCall function(injectedScriptObject(), "traceLog");
    function.appendArgument(traceLogId);
    if (startOffset)
        function.appendArgument(*startOffset);
    if (maxLength)
        function.appendArgument(*maxLength);
    RefPtr<InspectorValue> resultValue;
    makeCall(function, &resultValue);
    if (!resultValue || resultValue->type() != InspectorValue::TypeObject) {
        if (!resultValue->asString(errorString))
            *errorString = "Internal error: traceLog";
        return;
    }
    *traceLog = TraceLog::runtimeCast(resultValue);
}

void InjectedScriptCanvasModule::replayTraceLog(ErrorString* errorString, const TraceLogId& traceLogId, int stepNo, RefPtr<ResourceState>* result)
{
    ScriptFunctionCall function(injectedScriptObject(), "replayTraceLog");
    function.appendArgument(traceLogId);
    function.appendArgument(stepNo);
    RefPtr<InspectorValue> resultValue;
    makeCall(function, &resultValue);
    if (!resultValue || resultValue->type() != InspectorValue::TypeObject) {
        if (!resultValue->asString(errorString))
            *errorString = "Internal error: replayTraceLog";
        return;
    }
    *result = ResourceState::runtimeCast(resultValue);
}

void InjectedScriptCanvasModule::resourceInfo(ErrorString* errorString, const ResourceId& resourceId, RefPtr<ResourceInfo>* result)
{
    ScriptFunctionCall function(injectedScriptObject(), "resourceInfo");
    function.appendArgument(resourceId);
    RefPtr<InspectorValue> resultValue;
    makeCall(function, &resultValue);
    if (!resultValue || resultValue->type() != InspectorValue::TypeObject) {
        if (!resultValue->asString(errorString))
            *errorString = "Internal error: resourceInfo";
        return;
    }
    *result = ResourceInfo::runtimeCast(resultValue);
}

void InjectedScriptCanvasModule::resourceState(ErrorString* errorString, const TraceLogId& traceLogId, const ResourceId& resourceId, RefPtr<ResourceState>* result)
{
    ScriptFunctionCall function(injectedScriptObject(), "resourceState");
    function.appendArgument(traceLogId);
    function.appendArgument(resourceId);
    RefPtr<InspectorValue> resultValue;
    makeCall(function, &resultValue);
    if (!resultValue || resultValue->type() != InspectorValue::TypeObject) {
        if (!resultValue->asString(errorString))
            *errorString = "Internal error: resourceState";
        return;
    }
    *result = ResourceState::runtimeCast(resultValue);
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
