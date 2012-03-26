/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebCoreTestSupport.h"

#include "Document.h"
#include "Internals.h"
#include "ScriptExecutionContext.h"
#include "V8Internals.h"

#include <v8.h>

using namespace WebCore;

namespace WebCoreTestSupport {

void injectInternalsObject(v8::Local<v8::Context> context)
{
    v8::Context::Scope contextScope(context);
    v8::HandleScope scope;

    context->Global()->Set(v8::String::New(Internals::internalsId), toV8(Internals::create()));
}

void resetInternalsObject(v8::Local<v8::Context> context)
{
    v8::Context::Scope contextScope(context);
    v8::HandleScope scope;

    v8::Handle<v8::Object> object = v8::Handle<v8::Object>::Cast(context->Global()->Get(v8::String::New(Internals::internalsId)));
    Internals * internals = V8Internals::toNative(object);
    if (internals) {
        ScriptExecutionContext* scriptContext = getScriptExecutionContext();
        if (scriptContext->isDocument())
            internals->reset(static_cast<Document*>(scriptContext));
    }
}

}
