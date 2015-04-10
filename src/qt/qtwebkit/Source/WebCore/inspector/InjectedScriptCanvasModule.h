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

#ifndef InjectedScriptCanvasModule_h
#define InjectedScriptCanvasModule_h

#include "InjectedScriptModule.h"
#include "ScriptState.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

class InjectedScriptManager;
class ScriptObject;

#if ENABLE(INSPECTOR)

class InjectedScriptCanvasModule : public InjectedScriptModule {
public:
    InjectedScriptCanvasModule();
    
    virtual String source() const;

    static InjectedScriptCanvasModule moduleForState(InjectedScriptManager*, ScriptState*);

    ScriptObject wrapCanvas2DContext(const ScriptObject&);
#if ENABLE(WEBGL)
    ScriptObject wrapWebGLContext(const ScriptObject&);
#endif
    void markFrameEnd();

    void captureFrame(ErrorString*, TypeBuilder::Canvas::TraceLogId*);
    void startCapturing(ErrorString*, TypeBuilder::Canvas::TraceLogId*);
    void stopCapturing(ErrorString*, const TypeBuilder::Canvas::TraceLogId&);
    void dropTraceLog(ErrorString*, const TypeBuilder::Canvas::TraceLogId&);
    void traceLog(ErrorString*, const String&, const int*, const int*, RefPtr<TypeBuilder::Canvas::TraceLog>*);
    void replayTraceLog(ErrorString*, const TypeBuilder::Canvas::TraceLogId&, int, RefPtr<TypeBuilder::Canvas::ResourceState>*);
    void resourceInfo(ErrorString*, const TypeBuilder::Canvas::ResourceId&, RefPtr<TypeBuilder::Canvas::ResourceInfo>*);
    void resourceState(ErrorString*, const TypeBuilder::Canvas::TraceLogId&, const TypeBuilder::Canvas::ResourceId&, RefPtr<TypeBuilder::Canvas::ResourceState>*);

private:
    ScriptObject callWrapContextFunction(const String&, const ScriptObject&);
    void callStartCapturingFunction(const String&, ErrorString*, String*);
    void callVoidFunctionWithTraceLogIdArgument(const String&, ErrorString*, const String&);
};

#endif

} // namespace WebCore

#endif // !defined(InjectedScriptCanvasModule_h)
