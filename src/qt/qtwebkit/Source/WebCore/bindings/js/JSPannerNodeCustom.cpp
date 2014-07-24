/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "JSPannerNode.h"

#include "ExceptionCode.h"
#include "PannerNode.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

void JSPannerNode::setPanningModel(ExecState* exec, JSValue value)
{
    PannerNode* imp = static_cast<PannerNode*>(impl());

#if ENABLE(LEGACY_WEB_AUDIO)
    if (value.isNumber()) {
        uint32_t model = value.toUInt32(exec);
        if (!imp->setPanningModel(model))
            throwError(exec, createTypeError(exec, "Illegal panningModel"));
        return;
    }
#endif

    if (value.isString()) {
        String model = value.toString(exec)->value(exec);
        if (model == "equalpower" || model == "HRTF" || model == "soundfield") {
            imp->setPanningModel(model);
            return;
        }
    }
    
    throwError(exec, createTypeError(exec, "Illegal panningModel"));
}

void JSPannerNode::setDistanceModel(ExecState* exec, JSValue value)
{
    PannerNode* imp = static_cast<PannerNode*>(impl());

#if ENABLE(LEGACY_WEB_AUDIO)
    if (value.isNumber()) {
        uint32_t model = value.toUInt32(exec);
        if (!imp->setDistanceModel(model))
            throwError(exec, createTypeError(exec, "Illegal distanceModel"));
        return;
    }
#endif

    if (value.isString()) {
        String model = value.toString(exec)->value(exec);
        if (model == "linear" || model == "inverse" || model == "exponential") {
            imp->setDistanceModel(model);
            return;
        }
    }
    
    throwError(exec, createTypeError(exec, "Illegal distanceModel"));
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
