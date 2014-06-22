/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
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

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "IdentifierRep.h"
#include "npruntime_internal.h"
#include "npruntime_impl.h"
#include "npruntime_priv.h"

#include "c_utility.h"
#include <runtime/Identifier.h>
#include <runtime/JSLock.h>
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>

using namespace JSC::Bindings;
using namespace WebCore;

NPIdentifier _NPN_GetStringIdentifier(const NPUTF8* name)
{
    return static_cast<NPIdentifier>(IdentifierRep::get(name));
}

void _NPN_GetStringIdentifiers(const NPUTF8** names, int32_t nameCount, NPIdentifier* identifiers)
{
    ASSERT(names);
    ASSERT(identifiers);
    
    if (names && identifiers) {
        for (int i = 0; i < nameCount; i++)
            identifiers[i] = _NPN_GetStringIdentifier(names[i]);
    }
}

NPIdentifier _NPN_GetIntIdentifier(int32_t intid)
{
    return static_cast<NPIdentifier>(IdentifierRep::get(intid));
}

bool _NPN_IdentifierIsString(NPIdentifier identifier)
{
    return static_cast<IdentifierRep*>(identifier)->isString();
}

NPUTF8 *_NPN_UTF8FromIdentifier(NPIdentifier identifier)
{
    const char* string = static_cast<IdentifierRep*>(identifier)->string();
    if (!string)
        return 0;
    
    return strdup(string);
}

int32_t _NPN_IntFromIdentifier(NPIdentifier identifier)
{
    return static_cast<IdentifierRep*>(identifier)->number();
}

void NPN_InitializeVariantWithStringCopy(NPVariant* variant, const NPString* value)
{
    variant->type = NPVariantType_String;
    variant->value.stringValue.UTF8Length = value->UTF8Length;
    // Switching to fastMalloc would be better to avoid length check but this is not desirable
    // as NPN_MemAlloc is using malloc and there might be plugins that mix NPN_MemAlloc and malloc too.
    variant->value.stringValue.UTF8Characters = (NPUTF8*)malloc(sizeof(NPUTF8) * value->UTF8Length);
    if (value->UTF8Length && !variant->value.stringValue.UTF8Characters)
        CRASH();
    memcpy((void*)variant->value.stringValue.UTF8Characters, value->UTF8Characters, sizeof(NPUTF8) * value->UTF8Length);
}

void _NPN_ReleaseVariantValue(NPVariant* variant)
{
    ASSERT(variant);

    if (variant->type == NPVariantType_Object) {
        _NPN_ReleaseObject(variant->value.objectValue);
        variant->value.objectValue = 0;
    } else if (variant->type == NPVariantType_String) {
        free((void*)variant->value.stringValue.UTF8Characters);
        variant->value.stringValue.UTF8Characters = 0;
        variant->value.stringValue.UTF8Length = 0;
    }

    variant->type = NPVariantType_Void;
}

NPObject *_NPN_CreateObject(NPP npp, NPClass* aClass)
{
    ASSERT(aClass);

    if (aClass) {
        NPObject* obj;
        if (aClass->allocate != NULL)
            obj = aClass->allocate(npp, aClass);
        else
            obj = (NPObject*)malloc(sizeof(NPObject));
        if (!obj)
            CRASH();
        obj->_class = aClass;
        obj->referenceCount = 1;

        return obj;
    }

    return 0;
}

NPObject* _NPN_RetainObject(NPObject* obj)
{
    ASSERT(obj);

    if (obj)
        obj->referenceCount++;

    return obj;
}

void _NPN_ReleaseObject(NPObject* obj)
{
    ASSERT(obj);
    ASSERT(obj->referenceCount >= 1);

    if (obj && obj->referenceCount >= 1) {
        if (--obj->referenceCount == 0)
            _NPN_DeallocateObject(obj);
    }
}

void _NPN_DeallocateObject(NPObject *obj)
{
    ASSERT(obj);

    if (obj) {
        if (obj->_class->deallocate)
            obj->_class->deallocate(obj);
        else
            free(obj);
    }
}

#endif // ENABLE(NETSCAPE_PLUGIN_API)
