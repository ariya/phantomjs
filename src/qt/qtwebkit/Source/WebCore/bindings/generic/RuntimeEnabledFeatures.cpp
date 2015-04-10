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

#include "DatabaseManager.h"
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
bool RuntimeEnabledFeatures::isTouchEnabled = true;
bool RuntimeEnabledFeatures::isDeviceMotionEnabled = true;
bool RuntimeEnabledFeatures::isDeviceOrientationEnabled = true;
bool RuntimeEnabledFeatures::isSpeechInputEnabled = true;
bool RuntimeEnabledFeatures::isCSSExclusionsEnabled = true;
bool RuntimeEnabledFeatures::isCSSShapesEnabled = true;
bool RuntimeEnabledFeatures::isCSSRegionsEnabled = false;
bool RuntimeEnabledFeatures::isCSSCompositingEnabled = false;
bool RuntimeEnabledFeatures::isLangAttributeAwareFormControlUIEnabled = false;

#if ENABLE(SCRIPTED_SPEECH)
bool RuntimeEnabledFeatures::isScriptedSpeechEnabled = false;
#endif

#if ENABLE(MEDIA_STREAM)
bool RuntimeEnabledFeatures::isMediaStreamEnabled = true;
bool RuntimeEnabledFeatures::isPeerConnectionEnabled = true;
#endif

#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
bool RuntimeEnabledFeatures::isLegacyCSSVendorPrefixesEnabled = false;
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

bool RuntimeEnabledFeatures::htmlSourceElementEnabled()
{
    return MediaPlayer::isAvailable();
}

bool RuntimeEnabledFeatures::mediaControllerEnabled()
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

#if ENABLE(VIDEO_TRACK)
#if PLATFORM(MAC) || PLATFORM(GTK) || PLATFORM(EFL) || PLATFORM(BLACKBERRY) || PLATFORM(WIN) || PLATFORM(QT)
    bool RuntimeEnabledFeatures::isVideoTrackEnabled = true;
#else
    bool RuntimeEnabledFeatures::isVideoTrackEnabled = false;
#endif
#endif

#if ENABLE(SHADOW_DOM)
bool RuntimeEnabledFeatures::isShadowDOMEnabled = false;

bool RuntimeEnabledFeatures::isAuthorShadowDOMForAnyElementEnabled = false;
#endif

#if ENABLE(CUSTOM_ELEMENTS)
bool RuntimeEnabledFeatures::isCustomDOMElementsEnabled = false;
#endif

#if ENABLE(STYLE_SCOPED)
bool RuntimeEnabledFeatures::isStyleScopedEnabled = false;
#endif

#if ENABLE(INPUT_TYPE_DATE)
bool RuntimeEnabledFeatures::isInputTypeDateEnabled = true;
#endif

#if ENABLE(INPUT_TYPE_DATETIME_INCOMPLETE)
bool RuntimeEnabledFeatures::isInputTypeDateTimeEnabled = false;
#endif

#if ENABLE(INPUT_TYPE_DATETIMELOCAL)
bool RuntimeEnabledFeatures::isInputTypeDateTimeLocalEnabled = true;
#endif

#if ENABLE(INPUT_TYPE_MONTH)
bool RuntimeEnabledFeatures::isInputTypeMonthEnabled = true;
#endif

#if ENABLE(INPUT_TYPE_TIME)
bool RuntimeEnabledFeatures::isInputTypeTimeEnabled = true;
#endif

#if ENABLE(INPUT_TYPE_WEEK)
bool RuntimeEnabledFeatures::isInputTypeWeekEnabled = true;
#endif

#if ENABLE(DIALOG_ELEMENT)
bool RuntimeEnabledFeatures::isDialogElementEnabled = false;
#endif

#if ENABLE(CSP_NEXT)
bool RuntimeEnabledFeatures::areExperimentalContentSecurityPolicyFeaturesEnabled = false;
#endif

#if ENABLE(IFRAME_SEAMLESS)
bool RuntimeEnabledFeatures::areSeamlessIFramesEnabled = false;
#endif

#if ENABLE(FONT_LOAD_EVENTS)
bool RuntimeEnabledFeatures::isFontLoadEventsEnabled = false;
#endif

} // namespace WebCore
