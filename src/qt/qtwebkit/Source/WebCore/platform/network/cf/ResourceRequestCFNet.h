/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef ResourceRequestCFNet_h
#define ResourceRequestCFNet_h

#include "ResourceLoadPriority.h"

#if USE(CFNETWORK)
typedef const struct _CFURLRequest* CFURLRequestRef;
#endif

namespace WebCore {

class ResourceRequest;

#if USE(CFNETWORK)
void getResourceRequest(ResourceRequest&, CFURLRequestRef);
CFURLRequestRef cfURLRequest(const ResourceRequest&);
#endif

inline ResourceLoadPriority toResourceLoadPriority(int priority)
{
    switch (priority) {
    case -1:
        return ResourceLoadPriorityUnresolved;
    case 0:
        return ResourceLoadPriorityVeryLow;
    case 1:
        return ResourceLoadPriorityLow;
    case 2:
        return ResourceLoadPriorityMedium;
    case 3:
        return ResourceLoadPriorityHigh;
    case 4:
        return ResourceLoadPriorityVeryHigh;
    default:
        ASSERT_NOT_REACHED();
        return ResourceLoadPriorityLowest;
    }
}

inline int toHTTPPipeliningPriority(ResourceLoadPriority priority)
{
    switch (priority) {
    case ResourceLoadPriorityUnresolved:
        return -1;
    case ResourceLoadPriorityVeryLow:
        return 0;
    case ResourceLoadPriorityLow:
        return 1;
    case ResourceLoadPriorityMedium:
        return 2;
    case ResourceLoadPriorityHigh:
        return 3;
    case ResourceLoadPriorityVeryHigh:
        return 4;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

} // namespace WebCore

#endif // ResourceRequestCFNet_h
