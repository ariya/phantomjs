/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WebSystemInterface.h"

#import <WebCore/WebCoreSystemInterface.h>
#import <WebKitSystemInterface.h>

#define INIT(function) wk##function = WK##function

void InitWebCoreSystemInterface(void)
{
    static dispatch_once_t initOnce;
    
    dispatch_once(&initOnce, ^{
        INIT(AdvanceDefaultButtonPulseAnimation);
        INIT(CALayerEnumerateRectsBeingDrawnWithBlock);
        INIT(CopyCFLocalizationPreferredName);
        INIT(CGContextGetShouldSmoothFonts);
        INIT(CGPatternCreateWithImageAndTransform);
        INIT(CGContextResetClip);
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
        INIT(CGContextDrawsWithCorrectShadowOffsets);
#endif
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
        INIT(CTFontTransformGlyphs);
#endif
        INIT(CopyCONNECTProxyResponse);
        INIT(CopyNSURLResponseStatusLine);
        INIT(CopyNSURLResponseCertificateChain);
        INIT(CreateCTLineWithUniCharProvider);
        INIT(CreateCustomCFReadStream);
        INIT(DrawBezeledTextArea);
        INIT(DrawBezeledTextFieldCell);
        INIT(DrawCapsLockIndicator);
        INIT(DrawFocusRing);
        INIT(DrawMediaSliderTrack);
        INIT(DrawMediaUIPart);
        INIT(DrawTextFieldCellFocusRing);
        INIT(GetExtensionsForMIMEType);
        INIT(GetFontInLanguageForCharacter);
        INIT(GetFontInLanguageForRange);
        INIT(GetGlyphTransformedAdvances);
        INIT(GetGlyphsForCharacters);
        INIT(GetVerticalGlyphsForCharacters);
        INIT(GetHTTPPipeliningPriority);
        INIT(GetMIMETypeForExtension);
        INIT(GetNSURLResponseLastModifiedDate);
        INIT(SignedPublicKeyAndChallengeString);
        INIT(GetPreferredExtensionForMIMEType);
        INIT(GetUserToBaseCTM);
        INIT(GetWheelEventDeltas);
        INIT(GetNSEventKeyChar);
        INIT(HitTestMediaUIPart);
        INIT(InitializeMaximumHTTPConnectionCountPerHost);
        INIT(MeasureMediaUIPart);
        INIT(MediaControllerThemeAvailable);
        INIT(PopupMenu);
        INIT(QTIncludeOnlyModernMediaFileTypes);
        INIT(QTMovieDataRate);
        INIT(QTMovieDisableComponent);
        INIT(QTMovieGetType);
        INIT(QTMovieHasClosedCaptions);
        INIT(QTMovieMaxTimeLoaded);
        INIT(QTMovieMaxTimeLoadedChangeNotification);
        INIT(QTMovieMaxTimeSeekable);
        INIT(QTMovieResolvedURL);
        INIT(QTMovieSelectPreferredAlternates);
        INIT(QTMovieSetShowClosedCaptions);
        INIT(QTMovieViewSetDrawSynchronously);
        INIT(QTGetSitesInMediaDownloadCache);
        INIT(QTClearMediaDownloadCacheForSite);
        INIT(QTClearMediaDownloadCache);
        INIT(SetBaseCTM);
        INIT(SetCGFontRenderingMode);
        INIT(SetCONNECTProxyAuthorizationForStream);
        INIT(SetCONNECTProxyForStream);
        INIT(SetDragImage);
        INIT(SetHTTPPipeliningMaximumPriority);
        INIT(SetHTTPPipeliningPriority);
        INIT(SetHTTPPipeliningMinimumFastLanePriority);
        INIT(SetNSURLConnectionDefersCallbacks);
        INIT(SetNSURLRequestShouldContentSniff);
        INIT(SetPatternPhaseInUserSpace);
        INIT(SetUpFontCache);
        INIT(SignalCFReadStreamEnd);
        INIT(SignalCFReadStreamError);
        INIT(SignalCFReadStreamHasBytes);
        INIT(CreatePrivateStorageSession);
        INIT(CopyRequestWithStorageSession);
        INIT(CopyHTTPCookieStorage);
        INIT(GetHTTPCookieAcceptPolicy);
        INIT(SetHTTPCookieAcceptPolicy);
        INIT(HTTPCookies);
        INIT(HTTPCookiesForURL);
        INIT(SetHTTPCookiesForURL);
        INIT(DeleteAllHTTPCookies);
        INIT(DeleteHTTPCookie);

        INIT(SetMetadataURL);
        
        INIT(IOSurfaceContextCreate);
        INIT(IOSurfaceContextCreateImage);
        INIT(CreateCTTypesetterWithUniCharProviderAndOptions);
        INIT(CTRunGetInitialAdvance);
        INIT(RecommendedScrollerStyle);
        INIT(ExecutableWasLinkedOnOrBeforeSnowLeopard);
        INIT(CopyDefaultSearchProviderDisplayName);
        INIT(SetCrashReportApplicationSpecificInformation);
        INIT(AVAssetResolvedURL);
        INIT(Cursor);

#if USE(CFNETWORK)
        INIT(GetDefaultHTTPCookieStorage);
        INIT(CopyCredentialFromCFPersistentStorage);
        INIT(SetCFURLRequestShouldContentSniff);
        INIT(CFURLRequestCopyHTTPRequestBodyParts);
        INIT(CFURLRequestSetHTTPRequestBodyParts);
        INIT(SetRequestStorageSession);
#endif

#if PLATFORM(MAC)
        INIT(SpeechSynthesisGetVoiceIdentifiers);
        INIT(SpeechSynthesisGetDefaultVoiceIdentifierForLocale);
#endif
        INIT(GetAXTextMarkerTypeID);
        INIT(GetAXTextMarkerRangeTypeID);
        INIT(CreateAXTextMarker);
        INIT(GetBytesFromAXTextMarker);
        INIT(CreateAXTextMarkerRange);
        INIT(CopyAXTextMarkerRangeStart);
        INIT(CopyAXTextMarkerRangeEnd);
        INIT(AccessibilityHandleFocusChanged);
        INIT(CreateAXUIElementRef);
        INIT(UnregisterUniqueIdForElement);

        INIT(GetCFURLResponseMIMEType);
        INIT(GetCFURLResponseURL);
        INIT(GetCFURLResponseHTTPResponse);
        INIT(CopyCFURLResponseSuggestedFilename);
        INIT(SetCFURLResponseMIMEType);

        INIT(CreateVMPressureDispatchOnMainQueue);

        INIT(DestroyRenderingResources);

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
        INIT(ExecutableWasLinkedOnOrBeforeLion);
#endif
        
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
        INIT(CreateMemoryStatusPressureCriticalDispatchOnMainQueue);
#endif

        INIT(CGPathAddRoundedRect);
        INIT(CFURLRequestAllowAllPostCaching);

#if USE(CONTENT_FILTERING)
        INIT(FilterIsManagedSession);
        INIT(FilterCreateInstance);
        INIT(FilterWasBlocked);
        INIT(FilterIsBuffering);
        INIT(FilterAddData);
        INIT(FilterDataComplete);
#endif

#if !PLATFORM(IOS) && PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
        INIT(NSElasticDeltaForTimeDelta);
        INIT(NSElasticDeltaForReboundDelta);
        INIT(NSReboundDeltaForElasticDelta);
#endif

#if ENABLE(PUBLIC_SUFFIX_LIST)
        INIT(IsPublicSuffix);
#endif

#if ENABLE(CACHE_PARTITIONING)
        INIT(CachePartitionKey);
#endif
    });
}
