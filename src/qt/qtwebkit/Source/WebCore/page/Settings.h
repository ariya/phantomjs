/*
 * Copyright (C) 2003, 2006, 2007, 2008, 2009, 2011, 2012 Apple Inc. All rights reserved.
 *           (C) 2006 Graham Dennis (graham.dennis@gmail.com)
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

#ifndef Settings_h
#define Settings_h

#include "EditingBehaviorTypes.h"
#include "FontRenderingMode.h"
#include "IntSize.h"
#include "KURL.h"
#include "SecurityOrigin.h"
#include "SettingsMacros.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/AtomicStringHash.h>
#include <wtf/unicode/Unicode.h>

namespace WebCore {

    class FontGenericFamilies;
    class Page;

    enum EditableLinkBehavior {
        EditableLinkDefaultBehavior,
        EditableLinkAlwaysLive,
        EditableLinkOnlyLiveWithShiftKey,
        EditableLinkLiveWhenNotFocused,
        EditableLinkNeverLive
    };

    enum TextDirectionSubmenuInclusionBehavior {
        TextDirectionSubmenuNeverIncluded,
        TextDirectionSubmenuAutomaticallyIncluded,
        TextDirectionSubmenuAlwaysIncluded
    };

    class Settings {
        WTF_MAKE_NONCOPYABLE(Settings); WTF_MAKE_FAST_ALLOCATED;
    public:
        static PassOwnPtr<Settings> create(Page*);

        ~Settings();

        void setStandardFontFamily(const AtomicString&, UScriptCode = USCRIPT_COMMON);
        const AtomicString& standardFontFamily(UScriptCode = USCRIPT_COMMON) const;

        void setFixedFontFamily(const AtomicString&, UScriptCode = USCRIPT_COMMON);
        const AtomicString& fixedFontFamily(UScriptCode = USCRIPT_COMMON) const;

        void setSerifFontFamily(const AtomicString&, UScriptCode = USCRIPT_COMMON);
        const AtomicString& serifFontFamily(UScriptCode = USCRIPT_COMMON) const;

        void setSansSerifFontFamily(const AtomicString&, UScriptCode = USCRIPT_COMMON);
        const AtomicString& sansSerifFontFamily(UScriptCode = USCRIPT_COMMON) const;

        void setCursiveFontFamily(const AtomicString&, UScriptCode = USCRIPT_COMMON);
        const AtomicString& cursiveFontFamily(UScriptCode = USCRIPT_COMMON) const;

        void setFantasyFontFamily(const AtomicString&, UScriptCode = USCRIPT_COMMON);
        const AtomicString& fantasyFontFamily(UScriptCode = USCRIPT_COMMON) const;

        void setPictographFontFamily(const AtomicString&, UScriptCode = USCRIPT_COMMON);
        const AtomicString& pictographFontFamily(UScriptCode = USCRIPT_COMMON) const;

#if ENABLE(TEXT_AUTOSIZING)
        void setTextAutosizingEnabled(bool);
        bool textAutosizingEnabled() const { return m_textAutosizingEnabled; }

        void setTextAutosizingFontScaleFactor(float);
        float textAutosizingFontScaleFactor() const { return m_textAutosizingFontScaleFactor; }

        // Only set by Layout Tests, and only used if textAutosizingEnabled() returns true.
        void setTextAutosizingWindowSizeOverride(const IntSize&);
        const IntSize& textAutosizingWindowSizeOverride() const { return m_textAutosizingWindowSizeOverride; }
#endif

        // Only set by Layout Tests.
        void setMediaTypeOverride(const String&);
        const String& mediaTypeOverride() const { return m_mediaTypeOverride; }

        // Unlike areImagesEnabled, this only suppresses the network load of
        // the image URL.  A cached image will still be rendered if requested.
        void setLoadsImagesAutomatically(bool);
        bool loadsImagesAutomatically() const { return m_loadsImagesAutomatically; }

        // Clients that execute script should call ScriptController::canExecuteScripts()
        // instead of this function. ScriptController::canExecuteScripts() checks the
        // HTML sandbox, plug-in sandboxing, and other important details.
        bool isScriptEnabled() const { return m_isScriptEnabled; }
        void setScriptEnabled(bool);

        SETTINGS_GETTERS_AND_SETTERS

        void setJavaEnabled(bool);
        bool isJavaEnabled() const { return m_isJavaEnabled; }

        // This settings is only consulted if isJavaEnabled() returns true;
        void setJavaEnabledForLocalFiles(bool);
        bool isJavaEnabledForLocalFiles() const { return m_isJavaEnabledForLocalFiles; }

        void setImagesEnabled(bool);
        bool areImagesEnabled() const { return m_areImagesEnabled; }

        void setPluginsEnabled(bool);
        bool arePluginsEnabled() const { return m_arePluginsEnabled; }

        // When this option is set, WebCore will avoid storing any record of browsing activity
        // that may persist on disk or remain displayed when the option is reset.
        // This option does not affect the storage of such information in RAM.
        // The following functions respect this setting:
        //  - HTML5/DOM Storage
        //  - Icon Database
        //  - Console Messages
        //  - MemoryCache
        //  - Application Cache
        //  - Back/Forward Page History
        //  - Page Search Results
        //  - HTTP Cookies
        //  - Plug-ins (that support NPNVprivateModeBool)
        void setPrivateBrowsingEnabled(bool);
        bool privateBrowsingEnabled() const { return m_privateBrowsingEnabled; }

        void setDNSPrefetchingEnabled(bool);
        bool dnsPrefetchingEnabled() const { return m_dnsPrefetchingEnabled; }

        void setUserStyleSheetLocation(const KURL&);
        const KURL& userStyleSheetLocation() const { return m_userStyleSheetLocation; }

        void setNeedsAdobeFrameReloadingQuirk(bool);
        bool needsAcrobatFrameReloadingQuirk() const { return m_needsAdobeFrameReloadingQuirk; }

        static void setDefaultMinDOMTimerInterval(double); // Interval specified in seconds.
        static double defaultMinDOMTimerInterval();
        
        static void setHiddenPageDOMTimerAlignmentInterval(double); // Interval specified in seconds.
        static double hiddenPageDOMTimerAlignmentInterval();

        void setMinDOMTimerInterval(double); // Per-page; initialized to default value.
        double minDOMTimerInterval();

        static void setDefaultDOMTimerAlignmentInterval(double);
        static double defaultDOMTimerAlignmentInterval();

        void setDOMTimerAlignmentInterval(double);
        double domTimerAlignmentInterval() const;

#if ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
        bool hiddenPageDOMTimerThrottlingEnabled() const { return m_hiddenPageDOMTimerThrottlingEnabled; }
        void setHiddenPageDOMTimerThrottlingEnabled(bool);
#endif

        void setUsesPageCache(bool);
        bool usesPageCache() const { return m_usesPageCache; }
        
        void setFontRenderingMode(FontRenderingMode mode);
        FontRenderingMode fontRenderingMode() const;

        void setCSSCustomFilterEnabled(bool enabled) { m_isCSSCustomFilterEnabled = enabled; }
        bool isCSSCustomFilterEnabled() const { return m_isCSSCustomFilterEnabled; }

#if ENABLE(CSS_STICKY_POSITION)
        void setCSSStickyPositionEnabled(bool enabled) { m_cssStickyPositionEnabled = enabled; }
        bool cssStickyPositionEnabled() const { return m_cssStickyPositionEnabled; }
#else
        void setCSSStickyPositionEnabled(bool) { }
        bool cssStickyPositionEnabled() const { return false; }
#endif

#if ENABLE(CSS_VARIABLES)
        void setCSSVariablesEnabled(bool enabled) { m_cssVariablesEnabled = enabled; }
        bool cssVariablesEnabled() const { return m_cssVariablesEnabled; }
#else
        void setCSSVariablesEnabled(bool) { }
        bool cssVariablesEnabled() const { return false; }
#endif

        void setShowTiledScrollingIndicator(bool);
        bool showTiledScrollingIndicator() const { return m_showTiledScrollingIndicator; }

#if PLATFORM(WIN)
        static void setShouldUseHighResolutionTimers(bool);
        static bool shouldUseHighResolutionTimers() { return gShouldUseHighResolutionTimers; }
#endif

        void setTiledBackingStoreEnabled(bool);
        bool tiledBackingStoreEnabled() const { return m_tiledBackingStoreEnabled; }

#if USE(AVFOUNDATION)
        static void setAVFoundationEnabled(bool flag);
        static bool isAVFoundationEnabled() { return gAVFoundationEnabled; }
#endif

#if PLATFORM(MAC)
        static void setQTKitEnabled(bool flag);
        static bool isQTKitEnabled() { return gQTKitEnabled; }
#endif

        static const unsigned defaultMaximumHTMLParserDOMTreeDepth = 512;

#if USE(SAFARI_THEME)
        // Windows debugging pref (global) for switching between the Aqua look and a native windows look.
        static void setShouldPaintNativeControls(bool);
        static bool shouldPaintNativeControls() { return gShouldPaintNativeControls; }
#endif

        static void setMockScrollbarsEnabled(bool flag);
        static bool mockScrollbarsEnabled();

        static void setUsesOverlayScrollbars(bool flag);
        static bool usesOverlayScrollbars();

#if ENABLE(TOUCH_EVENTS)
        void setTouchEventEmulationEnabled(bool enabled) { m_touchEventEmulationEnabled = enabled; }
        bool isTouchEventEmulationEnabled() const { return m_touchEventEmulationEnabled; }
#endif

        void setStorageBlockingPolicy(SecurityOrigin::StorageBlockingPolicy);
        SecurityOrigin::StorageBlockingPolicy storageBlockingPolicy() const { return m_storageBlockingPolicy; }

        void setScrollingPerformanceLoggingEnabled(bool);
        bool scrollingPerformanceLoggingEnabled() { return m_scrollingPerformanceLoggingEnabled; }
        
        void setAggressiveTileRetentionEnabled(bool);
        bool aggressiveTileRetentionEnabled() { return m_aggressiveTileRetentionEnabled; }

        static void setShouldRespectPriorityInCSSAttributeSetters(bool);
        static bool shouldRespectPriorityInCSSAttributeSetters();

        void setTimeWithoutMouseMovementBeforeHidingControls(double time) { m_timeWithoutMouseMovementBeforeHidingControls = time; }
        double timeWithoutMouseMovementBeforeHidingControls() const { return m_timeWithoutMouseMovementBeforeHidingControls; }

#if ENABLE(PAGE_VISIBILITY_API)
        bool hiddenPageCSSAnimationSuspensionEnabled() const { return m_hiddenPageCSSAnimationSuspensionEnabled; }
        void setHiddenPageCSSAnimationSuspensionEnabled(bool);
#endif

        static bool lowPowerVideoAudioBufferSizeEnabled() { return gLowPowerVideoAudioBufferSizeEnabled; }
        static void setLowPowerVideoAudioBufferSizeEnabled(bool);

    private:
        explicit Settings(Page*);

        void initializeDefaultFontFamilies();

        Page* m_page;

        String m_mediaTypeOverride;
        KURL m_userStyleSheetLocation;
        RefPtr<FontGenericFamilies> m_fontGenericFamilies;
        SecurityOrigin::StorageBlockingPolicy m_storageBlockingPolicy;
#if ENABLE(TEXT_AUTOSIZING)
        float m_textAutosizingFontScaleFactor;
        IntSize m_textAutosizingWindowSizeOverride;
        bool m_textAutosizingEnabled : 1;
#endif

        SETTINGS_MEMBER_VARIABLES

        bool m_isJavaEnabled : 1;
        bool m_isJavaEnabledForLocalFiles : 1;
        bool m_loadsImagesAutomatically : 1;
        bool m_privateBrowsingEnabled : 1;
        bool m_areImagesEnabled : 1;
        bool m_arePluginsEnabled : 1;
        bool m_isScriptEnabled : 1;
        bool m_needsAdobeFrameReloadingQuirk : 1;
        bool m_usesPageCache : 1;
        unsigned m_fontRenderingMode : 1;
        bool m_isCSSCustomFilterEnabled : 1;
#if ENABLE(CSS_STICKY_POSITION)
        bool m_cssStickyPositionEnabled : 1;
#endif
#if ENABLE(CSS_VARIABLES)
        bool m_cssVariablesEnabled : 1;
#endif
        bool m_showTiledScrollingIndicator : 1;
        bool m_tiledBackingStoreEnabled : 1;
        bool m_dnsPrefetchingEnabled : 1;

#if ENABLE(TOUCH_EVENTS)
        bool m_touchEventEmulationEnabled : 1;
#endif
        bool m_scrollingPerformanceLoggingEnabled : 1;
        bool m_aggressiveTileRetentionEnabled : 1;

        double m_timeWithoutMouseMovementBeforeHidingControls;

        Timer<Settings> m_setImageLoadingSettingsTimer;
        void imageLoadingSettingsTimerFired(Timer<Settings>*);

#if ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
        bool m_hiddenPageDOMTimerThrottlingEnabled : 1;
#endif
#if ENABLE(PAGE_VISIBILITY_API)
        bool m_hiddenPageCSSAnimationSuspensionEnabled : 1;
#endif
        static double gDefaultMinDOMTimerInterval;
        static double gDefaultDOMTimerAlignmentInterval;

#if USE(AVFOUNDATION)
        static bool gAVFoundationEnabled;
#endif

#if PLATFORM(MAC)
        static bool gQTKitEnabled;
#endif
        
        static bool gMockScrollbarsEnabled;
        static bool gUsesOverlayScrollbars;

#if USE(SAFARI_THEME)
        static bool gShouldPaintNativeControls;
#endif
#if PLATFORM(WIN)
        static bool gShouldUseHighResolutionTimers;
#endif
        static bool gShouldRespectPriorityInCSSAttributeSetters;

        static double gHiddenPageDOMTimerAlignmentInterval;

        static bool gLowPowerVideoAudioBufferSizeEnabled;
    };

} // namespace WebCore

#endif // Settings_h
