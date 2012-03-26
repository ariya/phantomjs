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
#include "RuntimeEnabledFeatures.h"

#include "AbstractDatabase.h"
#include "MediaPlayer.h"
#include "SharedWorkerRepository.h"
#include "WebSocket.h"

#if ENABLE(FILE_SYSTEM)
#include "AsyncFileSystem.h"
#endif

namespace WebCore {

bool RuntimeEnabledFeatures::isLocalStorageEnabled = true;
bool RuntimeEnabledFeatures::isSessionStorageEnabled = true;
bool RuntimeEnabledFeatures::isWebkitNotificationsEnabled = false;
bool RuntimeEnabledFeatures::isApplicationCacheEnabled = true;
bool RuntimeEnabledFeatures::isDataTransferItemsEnabled = true;
bool RuntimeEnabledFeatures::isGeolocationEnabled = true;
bool RuntimeEnabledFeatures::isIndexedDBEnabled = false;
bool RuntimeEnabledFeatures::isWebAudioEnabled = false;
bool RuntimeEnabledFeatures::isPushStateEnabled = false;
bool RuntimeEnabledFeatures::isTouchEnabled = true;
bool RuntimeEnabledFeatures::isDeviceMotionEnabled = true;
bool RuntimeEnabledFeatures::isDeviceOrientationEnabled = true;
bool RuntimeEnabledFeatures::isSpeechInputEnabled = true;

#if ENABLE(MEDIA_STREAM)
bool RuntimeEnabledFeatures::isMediaStreamEnabled = true;
#endif

#if ENABLE(XHR_RESPONSE_BLOB)
bool RuntimeEnabledFeatures::isXHRResponseBlobEnabled = false;
#endif

#if ENABLE(FILE_SYSTEM)
bool RuntimeEnabledFeatures::isFileSystemEnabled = false;

bool RuntimeEnabledFeatures::fileSystemEnabled()
{
    return isFileSystemEnabled && AsyncFileSystem::isAvailable();
}
#endif

#if ENABLE(JAVASCRIPT_I18N_API)
bool RuntimeEnabledFeatures::isJavaScriptI18NAPIEnabled = false;

bool RuntimeEnabledFeatures::javaScriptI18NAPIEnabled()
{
    return isJavaScriptI18NAPIEnabled;
}
#endif

#if ENABLE(VIDEO)

bool RuntimeEnabledFeatures::audioEnabled()
{
    return MediaPlayer::isAvailable();
}

bool RuntimeEnabledFeatures::htmlMediaElementEnabled()
{
    return MediaPlayer::isAvailable();
}

bool RuntimeEnabledFeatures::htmlAudioElementEnabled()
{
    return MediaPlayer::isAvailable();
}

bool RuntimeEnabledFeatures::htmlVideoElementEnabled()
{
    return MediaPlayer::isAvailable();
}

bool RuntimeEnabledFeatures::mediaErrorEnabled()
{
    return MediaPlayer::isAvailable();
}

bool RuntimeEnabledFeatures::timeRangesEnabled()
{
    return MediaPlayer::isAvailable();
}

#endif

#if ENABLE(SHARED_WORKERS)
bool RuntimeEnabledFeatures::sharedWorkerEnabled()
{
    return SharedWorkerRepository::isAvailable();
}
#endif

#if ENABLE(WEB_SOCKETS)
bool RuntimeEnabledFeatures::webSocketEnabled()
{
    return WebSocket::isAvailable();
}
#endif

#if ENABLE(DATABASE)
bool RuntimeEnabledFeatures::openDatabaseEnabled()
{
    return AbstractDatabase::isAvailable();
}

bool RuntimeEnabledFeatures::openDatabaseSyncEnabled()
{
    return AbstractDatabase::isAvailable();
}
#endif

#if ENABLE(QUOTA)
bool RuntimeEnabledFeatures::isQuotaEnabled = false;
#endif

} // namespace WebCore
