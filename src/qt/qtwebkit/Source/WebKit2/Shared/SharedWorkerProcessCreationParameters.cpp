/*
 * Copyright (C) 2010, 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SharedWorkerProcessCreationParameters.h"

#if ENABLE(SHARED_WORKER_PROCESS)

#include "ArgumentCoders.h"

namespace WebKit {

SharedWorkerProcessCreationParameters::SharedWorkerProcessCreationParameters()
{
}

void SharedWorkerProcessCreationParameters::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << minimumLifetime;
    encoder << terminationTimeout;

#if PLATFORM(MAC)
    encoder << parentProcessName;
#endif
}

bool SharedWorkerProcessCreationParameters::decode(CoreIPC::ArgumentDecoder& decoder, SharedWorkerProcessCreationParameters& result)
{
    if (!decoder.decode(result.minimumLifetime))
        return false;
    if (!decoder.decode(result.terminationTimeout))
        return false;
#if PLATFORM(MAC)
    if (!decoder.decode(result.parentProcessName))
        return false;
#endif

    return true;
}


} // namespace WebKit

#endif // ENABLE(SHARED_WORKER_PROCESS)
