/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSWebKitPoint.h"

#include "WebKitPoint.h"

using namespace JSC;

namespace WebCore {

EncodedJSValue JSC_HOST_CALL JSWebKitPointConstructor::constructJSWebKitPoint(ExecState* exec)
{
    JSWebKitPointConstructor* jsConstructor = static_cast<JSWebKitPointConstructor*>(exec->callee());

    float x = 0;
    float y = 0;
    if (exec->argumentCount() >= 2) {
        x = static_cast<float>(exec->argument(0).toNumber(exec));
        y = static_cast<float>(exec->argument(1).toNumber(exec));
        if (isnan(x))
            x = 0;
        if (isnan(y))
            y = 0;
    }
    return JSValue::encode(asObject(toJS(exec, jsConstructor->globalObject(), WebKitPoint::create(x, y))));
}

} // namespace WebCore
