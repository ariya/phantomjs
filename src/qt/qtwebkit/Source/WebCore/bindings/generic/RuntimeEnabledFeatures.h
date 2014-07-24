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

#ifndef RuntimeEnabledFeatures_h
#define RuntimeEnabledFeatures_h

#include "PlatformExportMacros.h"

namespace WebCore {

// A class that stores static enablers for all experimental features. Note that
// the method names must line up with the JavaScript method they enable for code
// generation to work properly.

class RuntimeEnabledFeatures {
public:
    static void setLocalStorageEnabled(bool isEnabled) { isLocalStorageEnabled = isEnabled; }
    static bool localStorageEnabled() { return isLocalStorageEnabled; }

    static void setSessionStorageEnabled(bool isEnabled) { isSessionStorageEnabled = isEnabled; }
    static bool sessionStorageEnabled() { return isSessionStorageEnabled; }

    static void setWebkitNotificationsEnabled(bool isEnabled) { isWebkitNotificationsEnabled = isEnabled; }
    static bool webkitNotificationsEnabled() { return isWebkitNotificationsEnabled; }

    static void setApplicationCacheEnabled(bool isEnabled) { isApplicationCacheEnabled = isEnabled; }
    static bool applicationCacheEnabled() { return isApplicationCacheEnabled; }

    static void setDataTransferItemsEnabled(bool isEnabled) { isDataTransferItemsEnabled = isEnabled; }
    static bool dataTransferItemsEnabled() { return isDataTransferItemsEnabled; }

    static void setGeolocationEnabled(bool isEnabled) { isGeolocationEnabled = isEnabled; }
    static bool geolocationEnabled() { return isGeolocationEnabled; }

    static void setWebkitIndexedDBEnabled(bool isEnabled) { isIndexedDBEnabled = isEnabled; }
    static bool webkitIndexedDBEnabled() { return isIndexedDBEnabled; }
    static bool indexedDBEnabled() { return isIndexedDBEnabled; }

#if ENABLE(CSS_EXCLUSIONS)
    static void setCSSExclusionsEnabled(bool isEnabled) { isCSSExclusionsEnabled = isEnabled; }
    static bool cssExclusionsEnabled() { return isCSSExclusionsEnabled; }
#else
    static void setCSSExclusionsEnabled(bool) { }
    static bool cssExclusionsEnabled() { return false; }
#endif

#if ENABLE(CSS_SHAPES)
    static void setCSSShapesEnabled(bool isEnabled) { isCSSShapesEnabled = isEnabled; }
    static bool cssShapesEnabled() { return isCSSShapesEnabled; }
#else
    static void setCSSShapesEnabled(bool) { }
    static bool cssShapesEnabled() { return false; }
#endif

#if ENABLE(CSS_REGIONS)
    static void setCSSRegionsEnabled(bool isEnabled) { isCSSRegionsEnabled = isEnabled; }
    static bool cssRegionsEnabled() { return isCSSRegionsEnabled; }
#else
    static void setCSSRegionsEnabled(bool) { }
    static bool cssRegionsEnabled() { return false; }
#endif

    static void setCSSCompositingEnabled(bool isEnabled) { isCSSCompositingEnabled = isEnabled; }
    static bool cssCompositingEnabled() { return isCSSCompositingEnabled; }

#if ENABLE(FONT_LOAD_EVENTS)
    static void setFontLoadEventsEnabled(bool isEnabled) { isFontLoadEventsEnabled = isEnabled; }
    static bool fontLoadEventsEnabled() { return isFontLoadEventsEnabled; }
#else
    static void setFontLoadEventsEnabled(bool) { }
    static bool fontLoadEventsEnabled() { return false; }
#endif

#if ENABLE(VIDEO)
    static bool audioEnabled();
    static bool htmlMediaElementEnabled();
    static bool htmlAudioElementEnabled();
    static bool htmlVideoElementEnabled();
    static bool htmlSourceElementEnabled();
    static bool mediaControllerEnabled();
    static bool mediaErrorEnabled();
    static bool timeRangesEnabled();
#endif

#if ENABLE(SHARED_WORKERS)
    static bool sharedWorkerEnabled();
#endif

#if ENABLE(WEB_SOCKETS)
    static bool webSocketEnabled();
#endif

#if ENABLE(TOUCH_EVENTS)
    static bool touchEnabled() { return isTouchEnabled; }
    static void setTouchEnabled(bool isEnabled) { isTouchEnabled = isEnabled; }
#endif

    static void setDeviceMotionEnabled(bool isEnabled) { isDeviceMotionEnabled = isEnabled; }
    static bool deviceMotionEnabled() { return isDeviceMotionEnabled; }
    static bool deviceMotionEventEnabled() { return isDeviceMotionEnabled; }
    static bool ondevicemotionEnabled() { return isDeviceMotionEnabled; }

    static void setDeviceOrientationEnabled(bool isEnabled) { isDeviceOrientationEnabled = isEnabled; }
    static bool deviceOrientationEnabled() { return isDeviceOrientationEnabled; }
    static bool deviceOrientationEventEnabled() { return isDeviceOrientationEnabled; }
    static bool ondeviceorientationEnabled() { return isDeviceOrientationEnabled; }

    static void setSpeechInputEnabled(bool isEnabled) { isSpeechInputEnabled = isEnabled; }
    static bool speechInputEnabled() { return isSpeechInputEnabled; }
    static bool webkitSpeechEnabled() { return isSpeechInputEnabled; }
    static bool webkitGrammarEnabled() { return isSpeechInputEnabled; }

#if ENABLE(SCRIPTED_SPEECH)
    static void setScriptedSpeechEnabled(bool isEnabled) { isScriptedSpeechEnabled = isEnabled; }
    static bool scriptedSpeechEnabled() { return isScriptedSpeechEnabled; }
    static bool webkitSpeechRecognitionEnabled() { return isScriptedSpeechEnabled; }
    static bool webkitSpeechRecognitionErrorEnabled() { return isScriptedSpeechEnabled; }
    static bool webkitSpeechRecognitionEventEnabled() { return isScriptedSpeechEnabled; }
    static bool webkitSpeechGrammarEnabled() { return isScriptedSpeechEnabled; }
    static bool webkitSpeechGrammarListEnabled() { return isScriptedSpeechEnabled; }
#endif

#if ENABLE(FILE_SYSTEM)
    static bool fileSystemEnabled();
    static void setFileSystemEnabled(bool isEnabled) { isFileSystemEnabled = isEnabled; }
#endif

#if ENABLE(JAVASCRIPT_I18N_API)
    static bool javaScriptI18NAPIEnabled();
    static void setJavaScriptI18NAPIEnabled(bool isEnabled) { isJavaScriptI18NAPIEnabled = isEnabled; }
#endif

#if ENABLE(MEDIA_STREAM)
    static bool mediaStreamEnabled() { return isMediaStreamEnabled; }
    static void setMediaStreamEnabled(bool isEnabled) { isMediaStreamEnabled = isEnabled; }
    static bool webkitGetUserMediaEnabled() { return isMediaStreamEnabled; }
    static bool webkitMediaStreamEnabled() { return isMediaStreamEnabled; }

    static bool peerConnectionEnabled() { return isMediaStreamEnabled && isPeerConnectionEnabled; }
    static void setPeerConnectionEnabled(bool isEnabled) { isPeerConnectionEnabled = isEnabled; }
    static bool webkitRTCPeerConnectionEnabled() { return peerConnectionEnabled(); }
#endif

#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
    static void setLegacyCSSVendorPrefixesEnabled(bool isEnabled) { isLegacyCSSVendorPrefixesEnabled = isEnabled; }
    static bool legacyCSSVendorPrefixesEnabled() { return isLegacyCSSVendorPrefixesEnabled; }
#endif

#if ENABLE(VIDEO_TRACK)
    static bool webkitVideoTrackEnabled() { return isVideoTrackEnabled; }
    static void setWebkitVideoTrackEnabled(bool isEnabled) { isVideoTrackEnabled = isEnabled; }
#endif

#if ENABLE(SHADOW_DOM)
    static bool shadowDOMEnabled() { return isShadowDOMEnabled; }
    static void setShadowDOMEnabled(bool isEnabled) { isShadowDOMEnabled = isEnabled; }

    static bool authorShadowDOMForAnyElementEnabled() { return isAuthorShadowDOMForAnyElementEnabled; }
    static void setAuthorShadowDOMForAnyElementEnabled(bool isEnabled) { isAuthorShadowDOMForAnyElementEnabled = isEnabled; }
#endif

#if ENABLE(CUSTOM_ELEMENTS)
    static bool customDOMElementsEnabled() { return isCustomDOMElementsEnabled; }
    static void setCustomDOMElements(bool isEnabled) { isCustomDOMElementsEnabled = isEnabled; }
#endif

#if ENABLE(STYLE_SCOPED)
    static bool styleScopedEnabled() { return isStyleScopedEnabled; }
    static void setStyleScopedEnabled(bool isEnabled) { isStyleScopedEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_DATE)
    static bool inputTypeDateEnabled() { return isInputTypeDateEnabled; }
    static void setInputTypeDateEnabled(bool isEnabled) { isInputTypeDateEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_DATETIME_INCOMPLETE)
    static bool inputTypeDateTimeEnabled() { return isInputTypeDateTimeEnabled; }
    static void setInputTypeDateTimeEnabled(bool isEnabled) { isInputTypeDateTimeEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_DATETIMELOCAL)
    static bool inputTypeDateTimeLocalEnabled() { return isInputTypeDateTimeLocalEnabled; }
    static void setInputTypeDateTimeLocalEnabled(bool isEnabled) { isInputTypeDateTimeLocalEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_MONTH)
    static bool inputTypeMonthEnabled() { return isInputTypeMonthEnabled; }
    static void setInputTypeMonthEnabled(bool isEnabled) { isInputTypeMonthEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_TIME)
    static bool inputTypeTimeEnabled() { return isInputTypeTimeEnabled; }
    static void setInputTypeTimeEnabled(bool isEnabled) { isInputTypeTimeEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_WEEK)
    static bool inputTypeWeekEnabled() { return isInputTypeWeekEnabled; }
    static void setInputTypeWeekEnabled(bool isEnabled) { isInputTypeWeekEnabled = isEnabled; }
#endif

#if ENABLE(DIALOG_ELEMENT)
    static bool dialogElementEnabled() { return isDialogElementEnabled; }
    static void setDialogElementEnabled(bool isEnabled) { isDialogElementEnabled = isEnabled; }
#endif

#if ENABLE(CSP_NEXT)
    static bool experimentalContentSecurityPolicyFeaturesEnabled() { return areExperimentalContentSecurityPolicyFeaturesEnabled; }
    static void setExperimentalContentSecurityPolicyFeaturesEnabled(bool isEnabled) { areExperimentalContentSecurityPolicyFeaturesEnabled = isEnabled; }
#endif

#if ENABLE(IFRAME_SEAMLESS)
    static bool seamlessIFramesEnabled() { return areSeamlessIFramesEnabled; }
    static void setSeamlessIFramesEnabled(bool isEnabled) { areSeamlessIFramesEnabled = isEnabled; }
#endif

    static bool langAttributeAwareFormControlUIEnabled() { return isLangAttributeAwareFormControlUIEnabled; }
    // The lang attribute support is incomplete and should only be turned on for tests.
    static void setLangAttributeAwareFormControlUIEnabled(bool isEnabled) { isLangAttributeAwareFormControlUIEnabled = isEnabled; }

private:
    // Never instantiate.
    RuntimeEnabledFeatures() { }

    static bool isLocalStorageEnabled;
    static bool isSessionStorageEnabled;
    static bool isWebkitNotificationsEnabled;
    static bool isApplicationCacheEnabled;
    static bool isDataTransferItemsEnabled;
    static bool isGeolocationEnabled;
    static bool isIndexedDBEnabled;
    static bool isTouchEnabled;
    static bool isDeviceMotionEnabled;
    static bool isDeviceOrientationEnabled;
    static bool isSpeechInputEnabled;
    static bool isCSSExclusionsEnabled;
    static bool isCSSShapesEnabled;
    static bool isCSSRegionsEnabled;
    static bool isCSSCompositingEnabled;
    WEBCORE_TESTING static bool isLangAttributeAwareFormControlUIEnabled;
#if ENABLE(SCRIPTED_SPEECH)
    static bool isScriptedSpeechEnabled;
#endif
#if ENABLE(FILE_SYSTEM)
    static bool isFileSystemEnabled;
#endif

#if ENABLE(JAVASCRIPT_I18N_API)
    static bool isJavaScriptI18NAPIEnabled;
#endif

#if ENABLE(MEDIA_STREAM)
    static bool isMediaStreamEnabled;
    static bool isPeerConnectionEnabled;
#endif

#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
    static bool isLegacyCSSVendorPrefixesEnabled;
#endif

#if ENABLE(VIDEO_TRACK)
    static bool isVideoTrackEnabled;
#endif

#if ENABLE(SHADOW_DOM)
    static bool isShadowDOMEnabled;

    static bool isAuthorShadowDOMForAnyElementEnabled;
#endif

#if ENABLE(CUSTOM_ELEMENTS)
    static bool isCustomDOMElementsEnabled;
#endif

#if ENABLE(STYLE_SCOPED)
    static bool isStyleScopedEnabled;
#endif

#if ENABLE(INPUT_TYPE_DATE)
    static bool isInputTypeDateEnabled;
#endif

#if ENABLE(INPUT_TYPE_DATETIME_INCOMPLETE)
    static bool isInputTypeDateTimeEnabled;
#endif

#if ENABLE(INPUT_TYPE_DATETIMELOCAL)
    static bool isInputTypeDateTimeLocalEnabled;
#endif

#if ENABLE(INPUT_TYPE_MONTH)
    static bool isInputTypeMonthEnabled;
#endif

#if ENABLE(INPUT_TYPE_TIME)
    static bool isInputTypeTimeEnabled;
#endif

#if ENABLE(INPUT_TYPE_WEEK)
    static bool isInputTypeWeekEnabled;
#endif

#if ENABLE(DIALOG_ELEMENT)
    static bool isDialogElementEnabled;
#endif

#if ENABLE(CSP_NEXT)
    static bool areExperimentalContentSecurityPolicyFeaturesEnabled;
#endif

#if ENABLE(IFRAME_SEAMLESS)
    static bool areSeamlessIFramesEnabled;
#endif

#if ENABLE(FONT_LOAD_EVENTS)
    static bool isFontLoadEventsEnabled;
#endif

};

} // namespace WebCore

#endif // RuntimeEnabledFeatures_h
