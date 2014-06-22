/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#if ENABLE(QUOTA)

#include "StorageInfo.h"

#include "Document.h"
#include "ExceptionCode.h"
#include "ScriptExecutionContext.h"
#include "StorageErrorCallback.h"
#include "StorageQuota.h"
#include "StorageQuotaCallback.h"
#include "StorageUsageCallback.h"

namespace WebCore {

StorageInfo::StorageInfo()
{
}

StorageInfo::~StorageInfo()
{
}

void StorageInfo::queryUsageAndQuota(ScriptExecutionContext* scriptExecutionContext, int storageType, PassRefPtr<StorageUsageCallback> successCallback, PassRefPtr<StorageErrorCallback> errorCallback)
{
    // Dispatching the request to StorageQuota, as this interface is deprecated in favor of StorageQuota.
    StorageQuota* storageQuota = getStorageQuota(storageType);
    if (!storageQuota) {
        // Unknown storage type is requested.
        scriptExecutionContext->postTask(StorageErrorCallback::CallbackTask::create(errorCallback, NOT_SUPPORTED_ERR));
        return;
    }
    storageQuota->queryUsageAndQuota(scriptExecutionContext, successCallback, errorCallback);
}

void StorageInfo::requestQuota(ScriptExecutionContext* scriptExecutionContext, int storageType, unsigned long long newQuotaInBytes, PassRefPtr<StorageQuotaCallback> successCallback, PassRefPtr<StorageErrorCallback> errorCallback)
{
    // Dispatching the request to StorageQuota, as this interface is deprecated in favor of StorageQuota.
    StorageQuota* storageQuota = getStorageQuota(storageType);
    if (!storageQuota) {
        // Unknown storage type is requested.
        scriptExecutionContext->postTask(StorageErrorCallback::CallbackTask::create(errorCallback, NOT_SUPPORTED_ERR));
        return;
    }
    storageQuota->requestQuota(scriptExecutionContext, newQuotaInBytes, successCallback, errorCallback);
}

StorageQuota* StorageInfo::getStorageQuota(int storageType)
{
    switch (storageType) {
    case TEMPORARY:
        if (!m_temporaryStorage)
            m_temporaryStorage = StorageQuota::create(StorageQuota::Temporary);
        return m_temporaryStorage.get();
    case PERSISTENT:
        if (!m_persistentStorage)
            m_persistentStorage = StorageQuota::create(StorageQuota::Persistent);
        return m_persistentStorage.get();
    }
    return 0;
}

} // namespace WebCore

#endif // ENABLE(QUOTA)
