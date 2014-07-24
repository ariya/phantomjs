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

#include "config.h"
#include "NPRuntimeUtilities.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include <wtf/text/CString.h>

namespace WebKit {

void* npnMemAlloc(uint32_t size)
{
    // We could use fastMalloc here, but there might be plug-ins that mix NPN_MemAlloc/NPN_MemFree with malloc and free,
    // so having them be equivalent seems like a good idea.
    return malloc(size);
}

void npnMemFree(void* ptr)
{
    // We could use fastFree here, but there might be plug-ins that mix NPN_MemAlloc/NPN_MemFree with malloc and free,
    // so having them be equivalent seems like a good idea.
    free(ptr);
}

NPString createNPString(const CString& string)
{
    char* utf8Characters = npnMemNewArray<char>(string.length());
    memcpy(utf8Characters, string.data(), string.length());

    NPString npString;
    npString.UTF8Characters = utf8Characters;
    npString.UTF8Length = string.length();

    return npString;
}

NPObject* createNPObject(NPP npp, NPClass* npClass)
{
    ASSERT(npClass);
    
    NPObject* npObject;
    if (npClass->allocate)
        npObject = npClass->allocate(npp, npClass);
    else
        npObject = npnMemNew<NPObject>();

    npObject->_class = npClass;
    npObject->referenceCount = 1;
    
    return npObject;
}

void deallocateNPObject(NPObject* npObject)
{
    ASSERT(npObject);
    if (!npObject)
        return;

    if (npObject->_class->deallocate)
        npObject->_class->deallocate(npObject);
    else
        npnMemFree(npObject);
}

void retainNPObject(NPObject* npObject)
{
    ASSERT(npObject);
    if (!npObject)
        return;

    npObject->referenceCount++;
}

bool trySafeReleaseNPObject(NPObject* npObject)
{
    ASSERT(npObject);
    if (!npObject)
        return true;
    
    ASSERT(npObject->referenceCount >= 1);

    npObject->referenceCount--;
    if (npObject->referenceCount)
        return true;
    if (npObject->_class->deallocate)
        return false;
    deallocateNPObject(npObject);
    return true;
}

void releaseNPObject(NPObject* npObject)
{
    ASSERT(npObject);
    if (!npObject)
        return;
    
    ASSERT(npObject->referenceCount >= 1);
    npObject->referenceCount--;
    if (!npObject->referenceCount)
        deallocateNPObject(npObject);
}

void releaseNPVariantValue(NPVariant* variant)
{
    ASSERT(variant);
    
    switch (variant->type) {
    case NPVariantType_Void:
    case NPVariantType_Null:
    case NPVariantType_Bool:
    case NPVariantType_Int32:
    case NPVariantType_Double:
        // Nothing to do.
        break;
        
    case NPVariantType_String:
        npnMemFree(const_cast<NPUTF8*>(variant->value.stringValue.UTF8Characters));
        variant->value.stringValue.UTF8Characters = 0;
        variant->value.stringValue.UTF8Length = 0;
        break;
    case NPVariantType_Object:
        releaseNPObject(variant->value.objectValue);
        variant->value.objectValue = 0;
        break;
    }

    variant->type = NPVariantType_Void;
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
