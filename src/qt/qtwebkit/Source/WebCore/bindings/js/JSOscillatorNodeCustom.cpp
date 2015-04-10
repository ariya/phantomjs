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

#include "JSOscillatorNode.h"

#include "ExceptionCode.h"
#include "OscillatorNode.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

void JSOscillatorNode::setType(ExecState* exec, JSValue value)
{
    OscillatorNode* imp = static_cast<OscillatorNode*>(impl());

#if ENABLE(LEGACY_WEB_AUDIO)
    if (value.isNumber()) {
        uint32_t type = value.toUInt32(exec);
        if (!imp->setType(type))
            throwError(exec, createTypeError(exec, "Illegal OscillatorNode type"));
        return;
    }
#endif

    if (value.isString()) {
        String type = value.toString(exec)->value(exec);
        if (type == "sine" || type == "square" || type == "sawtooth" || type == "triangle") {
            imp->setType(type);
            return;
        }
    }
    
    throwError(exec, createTypeError(exec, "Illegal OscillatorNode type"));
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
