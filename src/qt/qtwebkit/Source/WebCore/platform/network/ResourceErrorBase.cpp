/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
#include "ResourceError.h"

namespace WebCore {

const char* const errorDomainWebKitInternal = "WebKitInternal";

ResourceError ResourceErrorBase::copy() const
{
    lazyInit();

    ResourceError errorCopy;
    errorCopy.m_domain = m_domain.isolatedCopy();
    errorCopy.m_errorCode = m_errorCode;
    errorCopy.m_failingURL = m_failingURL.isolatedCopy();
    errorCopy.m_localizedDescription = m_localizedDescription.isolatedCopy();
    errorCopy.m_isNull = m_isNull;
    errorCopy.m_isCancellation = m_isCancellation;
    errorCopy.m_isTimeout = m_isTimeout;
    platformCopy(errorCopy);
    return errorCopy;
}

void ResourceErrorBase::lazyInit() const
{
    const_cast<ResourceError*>(static_cast<const ResourceError*>(this))->platformLazyInit();
}

bool ResourceErrorBase::compare(const ResourceError& a, const ResourceError& b)
{
    if (a.isNull() && b.isNull())
        return true;

    if (a.isNull() || b.isNull())
        return false;

    if (a.domain() != b.domain())
        return false;

    if (a.errorCode() != b.errorCode())
        return false;

    if (a.failingURL() != b.failingURL())
        return false;

    if (a.localizedDescription() != b.localizedDescription())
        return false;

    if (a.isCancellation() != b.isCancellation())
        return false;

    if (a.isTimeout() != b.isTimeout())
        return false;

    return platformCompare(a, b);
}

} // namespace WebCore
