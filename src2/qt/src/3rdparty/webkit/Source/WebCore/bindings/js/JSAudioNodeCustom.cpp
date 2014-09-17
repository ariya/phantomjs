/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
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

#include "JSAudioNode.h"

#include "AudioNode.h"
#include <runtime/Error.h>

namespace WebCore {

JSC::JSValue JSAudioNode::connect(JSC::ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return throwError(exec, createSyntaxError(exec, "Not enough arguments"));

    unsigned outputIndex = 0;
    unsigned inputIndex = 0;
    
    AudioNode* destinationNode = toAudioNode(exec->argument(0));
    if (!destinationNode)
        return throwError(exec, createSyntaxError(exec, "Invalid destination node"));
    
    if (exec->argumentCount() > 1)
        outputIndex = exec->argument(1).toInt32(exec);

    if (exec->argumentCount() > 2)
        inputIndex = exec->argument(2).toInt32(exec);

    AudioNode* audioNode = static_cast<AudioNode*>(impl());
    bool success = audioNode->connect(destinationNode, outputIndex, inputIndex);
    if (!success)
        return throwError(exec, createSyntaxError(exec, "Invalid index parameter"));
    
    return JSC::jsUndefined();
}

JSC::JSValue JSAudioNode::disconnect(JSC::ExecState* exec)
{    
    unsigned outputIndex = 0;
    if (exec->argumentCount() > 0)
        outputIndex = exec->argument(0).toInt32(exec);

    AudioNode* audioNode = static_cast<AudioNode*>(impl());
    audioNode->disconnect(outputIndex);
    return JSC::jsUndefined();
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
