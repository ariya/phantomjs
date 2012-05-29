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

#include "CrossThreadCopier.h"

#include "KURL.h"
#include "PlatformString.h"
#include "ResourceError.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "SerializedScriptValue.h"

namespace WebCore {

CrossThreadCopierBase<false, false, KURL>::Type CrossThreadCopierBase<false, false, KURL>::copy(const KURL& url)
{
    return url.copy();
}

CrossThreadCopierBase<false, false, String>::Type CrossThreadCopierBase<false, false, String>::copy(const String& str)
{
    return str.crossThreadString();
}

CrossThreadCopierBase<false, false, ResourceError>::Type CrossThreadCopierBase<false, false, ResourceError>::copy(const ResourceError& error)
{
    return error.copy();
}

CrossThreadCopierBase<false, false, ResourceRequest>::Type CrossThreadCopierBase<false, false, ResourceRequest>::copy(const ResourceRequest& request)
{
    return request.copyData();
}

CrossThreadCopierBase<false, false, ResourceResponse>::Type CrossThreadCopierBase<false, false, ResourceResponse>::copy(const ResourceResponse& response)
{
    return response.copyData();
}

} // namespace WebCore
