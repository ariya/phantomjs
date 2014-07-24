/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ewk_storage_manager.h"

#include "WKAPICast.h"
#include "WKArray.h"
#include "ewk_error_private.h"
#include "ewk_security_origin_private.h"
#include "ewk_storage_manager_private.h"

using namespace WebKit;

EwkStorageManager::EwkStorageManager(WKKeyValueStorageManagerRef storageManager)
    : m_storageManager(storageManager)
{
    ASSERT(storageManager);
}

void EwkStorageManager::getStorageOrigins(void* context, WKKeyValueStorageManagerGetKeyValueStorageOriginsFunction callback) const
{
    WKKeyValueStorageManagerGetKeyValueStorageOrigins(m_storageManager.get(), context, callback);
}

Eina_List* EwkStorageManager::createOriginList(WKArrayRef origins) const
{
    Eina_List* originList = 0;
    const size_t length = WKArrayGetSize(origins);

    for (size_t i = 0; i < length; ++i) {
        WKSecurityOriginRef wkOriginRef = static_cast<WKSecurityOriginRef>(WKArrayGetItemAtIndex(origins, i));
        RefPtr<EwkSecurityOrigin> origin = m_wrapperCache.get(wkOriginRef);
        if (!origin) {
            origin = EwkSecurityOrigin::create(wkOriginRef);
            m_wrapperCache.set(wkOriginRef, origin);
        }
        originList = eina_list_append(originList, origin.release().leakRef());
    }

    return originList;
}

struct Ewk_Storage_Origins_Async_Get_Context {
    const Ewk_Storage_Manager* manager;
    Ewk_Storage_Origins_Get_Cb callback;
    void* userData;

    Ewk_Storage_Origins_Async_Get_Context(const Ewk_Storage_Manager* manager, Ewk_Storage_Origins_Get_Cb callback, void* userData)
        : manager(manager)
        , callback(callback)
        , userData(userData)
    { }
};

static void getStorageOriginsCallback(WKArrayRef origins, WKErrorRef wkError, void* context)
{
    Eina_List* originList = 0;
    OwnPtr<Ewk_Storage_Origins_Async_Get_Context*> webStorageContext = adoptPtr(static_cast<Ewk_Storage_Origins_Async_Get_Context*>(context));

    originList = webStorageContext->manager->createOriginList(origins);

    OwnPtr<EwkError> ewkError = EwkError::create(wkError);
    webStorageContext->callback(originList, ewkError.get(), webStorageContext->userData);
}

Eina_Bool ewk_storage_manager_origins_get(const Ewk_Storage_Manager* ewkStorageManager, Ewk_Storage_Origins_Get_Cb callback, void* userData)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkStorageManager, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    Ewk_Storage_Origins_Async_Get_Context* context = new Ewk_Storage_Origins_Async_Get_Context(ewkStorageManager, callback, userData);
    ewkStorageManager->getStorageOrigins(context, getStorageOriginsCallback);

    return true;
}
