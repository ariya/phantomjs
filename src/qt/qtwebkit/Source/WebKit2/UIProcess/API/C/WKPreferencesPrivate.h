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

#ifndef WKPreferencesPrivate_h
#define WKPreferencesPrivate_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

enum WKFontSmoothingLevel {
    kWKFontSmoothingLevelNoSubpixelAntiAliasing = 0,
    kWKFontSmoothingLevelLight = 1,
    kWKFontSmoothingLevelMedium = 2,
    kWKFontSmoothingLevelStrong = 3,
#if defined(WIN32) || defined(_WIN32)
    kWKFontSmoothingLevelWindows = 4,
#endif
};
typedef enum WKFontSmoothingLevel WKFontSmoothingLevel;

enum WKEditableLinkBehavior {
    kWKEditableLinkBehaviorDefault,
    kWKEditableLinkBehaviorAlwaysLive,
    kWKEditableLinkBehaviorOnlyLiveWithShiftKey,
    kWKEditableLinkBehaviorLiveWhenNotFocused,
    kWKEditableLinkBehaviorNeverLive
};
typedef enum WKEditableLinkBehavior WKEditableLinkBehavior;

// Creates a copy with no identifier.
WK_EXPORT WKPreferencesRef WKPreferencesCreateCopy(WKPreferencesRef);

// Defaults to kWKFontSmoothingLevelWindows on Windows, kWKFontSmoothingLevelMedium on other platforms.
WK_EXPORT void WKPreferencesSetFontSmoothingLevel(WKPreferencesRef, WKFontSmoothingLevel);
WK_EXPORT WKFontSmoothingLevel WKPreferencesGetFontSmoothingLevel(WKPreferencesRef);

// Defaults to EditableLinkNeverLive.
WK_EXPORT void WKPreferencesSetEditableLinkBehavior(WKPreferencesRef preferencesRef, WKEditableLinkBehavior);
WK_EXPORT WKEditableLinkBehavior WKPreferencesGetEditableLinkBehavior(WKPreferencesRef preferencesRef);   
    
// Defaults to false.
WK_EXPORT void WKPreferencesSetAcceleratedDrawingEnabled(WKPreferencesRef, bool);
WK_EXPORT bool WKPreferencesGetAcceleratedDrawingEnabled(WKPreferencesRef);

// Defaults to true.
WK_EXPORT void WKPreferencesSetCanvasUsesAcceleratedDrawing(WKPreferencesRef, bool);
WK_EXPORT bool WKPreferencesGetCanvasUsesAcceleratedDrawing(WKPreferencesRef);

// Defaults to true.
WK_EXPORT void WKPreferencesSetAcceleratedCompositingEnabled(WKPreferencesRef, bool);
WK_EXPORT bool WKPreferencesGetAcceleratedCompositingEnabled(WKPreferencesRef);

// Defaults to false.
WK_EXPORT void WKPreferencesSetAcceleratedCompositingForOverflowScrollEnabled(WKPreferencesRef, bool);
WK_EXPORT bool WKPreferencesGetAcceleratedCompositingForOverflowScrollEnabled(WKPreferencesRef);

// Defaults to false.
WK_EXPORT void WKPreferencesSetCompositingBordersVisible(WKPreferencesRef, bool);
WK_EXPORT bool WKPreferencesGetCompositingBordersVisible(WKPreferencesRef);

// Defaults to false.
WK_EXPORT void WKPreferencesSetCompositingRepaintCountersVisible(WKPreferencesRef, bool);
WK_EXPORT bool WKPreferencesGetCompositingRepaintCountersVisible(WKPreferencesRef);

// Defaults to false.
WK_EXPORT void WKPreferencesSetTiledScrollingIndicatorVisible(WKPreferencesRef, bool);
WK_EXPORT bool WKPreferencesGetTiledScrollingIndicatorVisible(WKPreferencesRef);

// Defaults to true.
WK_EXPORT void WKPreferencesSetCSSCustomFilterEnabled(WKPreferencesRef, bool flag);
WK_EXPORT bool WKPreferencesGetCSSCustomFilterEnabled(WKPreferencesRef);

// Defaults to false.
WK_EXPORT void WKPreferencesSetWebGLEnabled(WKPreferencesRef, bool);
WK_EXPORT bool WKPreferencesGetWebGLEnabled(WKPreferencesRef);

// Defaults to false.
WK_EXPORT void WKPreferencesSetAccelerated2DCanvasEnabled(WKPreferencesRef, bool);
WK_EXPORT bool WKPreferencesGetAccelerated2DCanvasEnabled(WKPreferencesRef);

// Defaults to true
WK_EXPORT void WKPreferencesSetCSSRegionsEnabled(WKPreferencesRef, bool flag);
WK_EXPORT bool WKPreferencesGetCSSRegionsEnabled(WKPreferencesRef);

// Defaults to false
WK_EXPORT void WKPreferencesSetCSSGridLayoutEnabled(WKPreferencesRef, bool flag);
WK_EXPORT bool WKPreferencesGetCSSGridLayoutEnabled(WKPreferencesRef);

// Defaults to false
WK_EXPORT void WKPreferencesSetRegionBasedColumnsEnabled(WKPreferencesRef, bool flag);
WK_EXPORT bool WKPreferencesGetRegionBasedColumnsEnabled(WKPreferencesRef);

// Defaults to false.
WK_EXPORT void WKPreferencesSetNeedsSiteSpecificQuirks(WKPreferencesRef, bool);
WK_EXPORT bool WKPreferencesGetNeedsSiteSpecificQuirks(WKPreferencesRef);

// Defaults to false.
WK_EXPORT void WKPreferencesSetForceFTPDirectoryListings(WKPreferencesRef preferences, bool force);
WK_EXPORT bool WKPreferencesGetForceFTPDirectoryListings(WKPreferencesRef preferences);

// Defaults to the empty string.
WK_EXPORT void WKPreferencesSetFTPDirectoryTemplatePath(WKPreferencesRef preferences, WKStringRef path);
WK_EXPORT WKStringRef WKPreferencesCopyFTPDirectoryTemplatePath(WKPreferencesRef preferences);

// Defaults to false.
WK_EXPORT void WKPreferencesSetWebArchiveDebugModeEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetWebArchiveDebugModeEnabled(WKPreferencesRef preferences);

// Defaults to false.
WK_EXPORT void WKPreferencesSetLocalFileContentSniffingEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetLocalFileContentSniffingEnabled(WKPreferencesRef preferences);

// Defaults to true.
WK_EXPORT void WKPreferencesSetPageCacheEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetPageCacheEnabled(WKPreferencesRef preferences);

// Defaults to true.
WK_EXPORT void WKPreferencesSetPageCacheSupportsPlugins(WKPreferencesRef preferences, bool pageCacheSupportsPlugins);
WK_EXPORT bool WKPreferencesGetPageCacheSupportsPlugins(WKPreferencesRef preferences);

// Defaults to false.
WK_EXPORT void WKPreferencesSetPaginateDuringLayoutEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetPaginateDuringLayoutEnabled(WKPreferencesRef preferences);

// Defaults to false.
WK_EXPORT void WKPreferencesSetDOMPasteAllowed(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetDOMPasteAllowed(WKPreferencesRef preferences);

// Defaults to true.
WK_EXPORT void WKPreferencesSetWebSecurityEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetWebSecurityEnabled(WKPreferencesRef preferences);

// Defaults to false.
WK_EXPORT void WKPreferencesSetUniversalAccessFromFileURLsAllowed(WKPreferencesRef preferences, bool allowed);
WK_EXPORT bool WKPreferencesGetUniversalAccessFromFileURLsAllowed(WKPreferencesRef preferences);

// Defaults to false.
WK_EXPORT void WKPreferencesSetFileAccessFromFileURLsAllowed(WKPreferencesRef preferences, bool allowed);
WK_EXPORT bool WKPreferencesGetFileAccessFromFileURLsAllowed(WKPreferencesRef preferences);

// Defaults to true.
WK_EXPORT void WKPreferencesSetHixie76WebSocketProtocolEnabled(WKPreferencesRef preferencesRef, bool enabled);
WK_EXPORT bool WKPreferencesGetHixie76WebSocketProtocolEnabled(WKPreferencesRef preferencesRef);

// Defaults to false.
WK_EXPORT void WKPreferencesSetMediaPlaybackRequiresUserGesture(WKPreferencesRef preferencesRef, bool flag);
WK_EXPORT bool WKPreferencesGetMediaPlaybackRequiresUserGesture(WKPreferencesRef preferencesRef);

// Defaults to true.
WK_EXPORT void WKPreferencesSetMediaPlaybackAllowsInline(WKPreferencesRef preferencesRef, bool flag);
WK_EXPORT bool WKPreferencesGetMediaPlaybackAllowsInline(WKPreferencesRef preferencesRef);

// Defaults to false.
WK_EXPORT void WKPreferencesSetShowsToolTipOverTruncatedText(WKPreferencesRef preferencesRef, bool flag);
WK_EXPORT bool WKPreferencesGetShowsToolTipOverTruncatedText(WKPreferencesRef preferencesRef);

// Defaults to false.
WK_EXPORT void WKPreferencesSetMockScrollbarsEnabled(WKPreferencesRef preferencesRef, bool flag);
WK_EXPORT bool WKPreferencesGetMockScrollbarsEnabled(WKPreferencesRef preferencesRef);
    
// Defaults to false.
WK_EXPORT void WKPreferencesSetApplicationChromeModeEnabled(WKPreferencesRef preferencesRef, bool enabled);
WK_EXPORT bool WKPreferencesGetApplicationChromeModeEnabled(WKPreferencesRef preferencesRef);

// Defaults to false.
WK_EXPORT void WKPreferencesSetInspectorUsesWebKitUserInterface(WKPreferencesRef preferencesRef, bool enabled);
WK_EXPORT bool WKPreferencesGetInspectorUsesWebKitUserInterface(WKPreferencesRef preferencesRef);

// Defaults to true.
WK_EXPORT void WKPreferencesSetJavaEnabledForLocalFiles(WKPreferencesRef preferences, bool javaEnabled);
WK_EXPORT bool WKPreferencesGetJavaEnabledForLocalFiles(WKPreferencesRef preferences);

// Defaults to true.
WK_EXPORT void WKPreferencesSetRequestAnimationFrameEnabled(WKPreferencesRef, bool);
WK_EXPORT bool WKPreferencesGetRequestAnimationFrameEnabled(WKPreferencesRef);

// Defaults to false
WK_EXPORT void WKPreferencesSetDiagnosticLoggingEnabled(WKPreferencesRef preferencesRef, bool enabled);
WK_EXPORT bool WKPreferencesGetDiagnosticLoggingEnabled(WKPreferencesRef preferencesRef);

// Defaults to false
WK_EXPORT void WKPreferencesSetAsynchronousPluginInitializationEnabled(WKPreferencesRef preferencesRef, bool enabled);
WK_EXPORT bool WKPreferencesGetAsynchronousPluginInitializationEnabled(WKPreferencesRef preferencesRef);

// Defaults to false
WK_EXPORT void WKPreferencesSetAsynchronousPluginInitializationEnabledForAllPlugins(WKPreferencesRef preferencesRef, bool enabled);
WK_EXPORT bool WKPreferencesGetAsynchronousPluginInitializationEnabledForAllPlugins(WKPreferencesRef preferencesRef);

// Defaults to false
WK_EXPORT void WKPreferencesSetArtificialPluginInitializationDelayEnabled(WKPreferencesRef preferencesRef, bool enabled);
WK_EXPORT bool WKPreferencesGetArtificialPluginInitializationDelayEnabled(WKPreferencesRef preferencesRef);

// Defaults to false
WK_EXPORT void WKPreferencesSetTabToLinksEnabled(WKPreferencesRef preferencesRef, bool enabled);
WK_EXPORT bool WKPreferencesGetTabToLinksEnabled(WKPreferencesRef preferencesRef);

// Defaults to false
WK_EXPORT void WKPreferencesSetInteractiveFormValidationEnabled(WKPreferencesRef preferencesRef, bool enabled);
WK_EXPORT bool WKPreferencesGetInteractiveFormValidationEnabled(WKPreferencesRef preferencesRef);

// Defaults to false
WK_EXPORT void WKPreferencesSetScrollingPerformanceLoggingEnabled(WKPreferencesRef preferencesRef, bool enabled);
WK_EXPORT bool WKPreferencesGetScrollingPerformanceLoggingEnabled(WKPreferencesRef preferencesRef);

// Defaults to true
WK_EXPORT void WKPreferencesSetScreenFontSubstitutionEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetScreenFontSubstitutionEnabled(WKPreferencesRef preferences);

// Defaults to true
WK_EXPORT void WKPreferencesSetCookieEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetCookieEnabled(WKPreferencesRef preferences);

// Defaults to false
WK_EXPORT void WKPreferencesSetPDFPluginEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetPDFPluginEnabled(WKPreferencesRef preferences);

// Defaults to false
WK_EXPORT void WKPreferencesSetAggressiveTileRetentionEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetAggressiveTileRetentionEnabled(WKPreferencesRef preferences);

// Defaults to false
WK_EXPORT void WKPreferencesSetLogsPageMessagesToSystemConsoleEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetLogsPageMessagesToSystemConsoleEnabled(WKPreferencesRef preferences);

// Defaults to false
WK_EXPORT void WKPreferencesSetPageVisibilityBasedProcessSuppressionEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetPageVisibilityBasedProcessSuppressionEnabled(WKPreferencesRef);

// Defaults to true
WK_EXPORT void WKPreferencesSetSmartInsertDeleteEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetSmartInsertDeleteEnabled(WKPreferencesRef preferences);

// Defaults to false
WK_EXPORT void WKPreferencesSetSelectTrailingWhitespaceEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetSelectTrailingWhitespaceEnabled(WKPreferencesRef preferences);

// Defaults to false
WK_EXPORT void WKPreferencesSetShowsURLsInToolTipsEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetShowsURLsInToolTipsEnabled(WKPreferencesRef preferences);

// Defaults to true on Mac, false on other platforms.
WK_EXPORT void WKPreferencesSetHiddenPageDOMTimerThrottlingEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetHiddenPageDOMTimerThrottlingEnabled(WKPreferencesRef preferences);

// Defaults to true on Mac, false on other platforms.
WK_EXPORT void WKPreferencesSetHiddenPageCSSAnimationSuspensionEnabled(WKPreferencesRef preferences, bool enabled);
WK_EXPORT bool WKPreferencesGetHiddenPageCSSAnimationSuspensionEnabled(WKPreferencesRef preferences);

// Defaults to false
WK_EXPORT void WKPreferencesSetSnapshotAllPlugIns(WKPreferencesRef preferencesRef, bool enabled);
WK_EXPORT bool WKPreferencesGetSnapshotAllPlugIns(WKPreferencesRef preferencesRef);

// Defaults to true
WK_EXPORT void WKPreferencesSetAutostartOriginPlugInSnapshottingEnabled(WKPreferencesRef preferencesRef, bool enabled);
WK_EXPORT bool WKPreferencesGetAutostartOriginPlugInSnapshottingEnabled(WKPreferencesRef preferencesRef);

// Defaults to true
WK_EXPORT void WKPreferencesSetPrimaryPlugInSnapshotDetectionEnabled(WKPreferencesRef preferencesRef, bool enabled);
WK_EXPORT bool WKPreferencesGetPrimaryPlugInSnapshotDetectionEnabled(WKPreferencesRef preferencesRef);

// Defaults to 5 seconds.
WK_EXPORT void WKPreferencesSetIncrementalRenderingSuppressionTimeout(WKPreferencesRef preferencesRef, double timeout);
WK_EXPORT double WKPreferencesGetIncrementalRenderingSuppressionTimeout(WKPreferencesRef preferencesRef);

WK_EXPORT void WKPreferencesResetTestRunnerOverrides(WKPreferencesRef preferencesRef);

#ifdef __cplusplus
}
#endif

#endif /* WKPreferencesPrivate_h */
