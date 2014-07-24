/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef NPRuntimeUtilities_h
#define NPRuntimeUtilities_h

#if ENABLE(NETSCAPE_PLUGIN_API)

#include <WebCore/npruntime_internal.h>
#include <wtf/Forward.h>

struct NPClass;
struct NPObject;

namespace WebKit {

void* npnMemAlloc(uint32_t);
void npnMemFree(void*);

template<typename T> T* npnMemNew()
{
    return static_cast<T*>(npnMemAlloc(sizeof(T)));
}

template<typename T> T* npnMemNewArray(size_t count)
{
    return static_cast<T*>(npnMemAlloc(sizeof(T) * count));
}

NPString createNPString(const CString&);

NPObject* createNPObject(NPP, NPClass*);
void deallocateNPObject(NPObject*);

void retainNPObject(NPObject*);
void releaseNPObject(NPObject*);
    
// This function decrements the refcount of the specified object. If the
// refcount reaches 0 it will attempt to destroy the object. If the object has
// a custom deallocate function it will fail and return false, so it will be
// up to the caller to call deallocateNPObject.
// This function is used to implement the delayed finalization of NPObjects
// released during GC.
bool trySafeReleaseNPObject(NPObject*);

void releaseNPVariantValue(NPVariant*);

}

#endif // ENABLE(NETSCAPE_PLUGIN_API)

#endif // NPRuntimeUtilities_h
