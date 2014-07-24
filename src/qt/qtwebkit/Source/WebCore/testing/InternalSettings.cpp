/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InternalSettings.h"

#include "CaptionUserPreferences.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "FrameView.h"
#include "Language.h"
#include "LocaleToScriptMapping.h"
#include "Page.h"
#include "PageGroup.h"
#include "RuntimeEnabledFeatures.h"
#include "Settings.h"
#include "Supplementable.h"
#include "TextRun.h"

#if ENABLE(INPUT_TYPE_COLOR)
#include "ColorChooser.h"
#endif

#define InternalSettingsGuardForSettingsReturn(returnValue) \
    if (!settings()) { \
        ec = INVALID_ACCESS_ERR; \
        return returnValue; \
    }

#define InternalSettingsGuardForSettings()  \
    if (!settings()) { \
        ec = INVALID_ACCESS_ERR; \
        return; \
    }

#define InternalSettingsGuardForPage() \
    if (!page()) { \
        ec = INVALID_ACCESS_ERR; \
        return; \
    }

namespace WebCore {

InternalSettings::Backup::Backup(Settings* settings)
    : m_originalCSSExclusionsEnabled(RuntimeEnabledFeatures::cssExclusionsEnabled())
    , m_originalCSSShapesEnabled(RuntimeEnabledFeatures::cssShapesEnabled())
    , m_originalCSSVariablesEnabled(settings->cssVariablesEnabled())
#if ENABLE(SHADOW_DOM)
    , m_originalShadowDOMEnabled(RuntimeEnabledFeatures::shadowDOMEnabled())
    , m_originalAuthorShadowDOMForAnyElementEnabled(RuntimeEnabledFeatures::authorShadowDOMForAnyElementEnabled())
#endif
#if ENABLE(STYLE_SCOPED)
    , m_originalStyleScoped(RuntimeEnabledFeatures::styleScopedEnabled())
#endif
    , m_originalEditingBehavior(settings->editingBehaviorType())
#if ENABLE(TEXT_AUTOSIZING)
    , m_originalTextAutosizingEnabled(settings->textAutosizingEnabled())
    , m_originalTextAutosizingWindowSizeOverride(settings->textAutosizingWindowSizeOverride())
    , m_originalTextAutosizingFontScaleFactor(settings->textAutosizingFontScaleFactor())
#endif
    , m_originalMediaTypeOverride(settings->mediaTypeOverride())
#if ENABLE(DIALOG_ELEMENT)
    , m_originalDialogElementEnabled(RuntimeEnabledFeatures::dialogElementEnabled())
#endif
    , m_originalCanvasUsesAcceleratedDrawing(settings->canvasUsesAcceleratedDrawing())
    , m_originalMockScrollbarsEnabled(settings->mockScrollbarsEnabled())
    , m_langAttributeAwareFormControlUIEnabled(RuntimeEnabledFeatures::langAttributeAwareFormControlUIEnabled())
    , m_imagesEnabled(settings->areImagesEnabled())
    , m_minimumTimerInterval(settings->minDOMTimerInterval())
#if ENABLE(VIDEO_TRACK)
    , m_shouldDisplaySubtitles(settings->shouldDisplaySubtitles())
    , m_shouldDisplayCaptions(settings->shouldDisplayCaptions())
    , m_shouldDisplayTextDescriptions(settings->shouldDisplayTextDescriptions())
#endif
    , m_defaultVideoPosterURL(settings->defaultVideoPosterURL())
    , m_originalTimeWithoutMouseMovementBeforeHidingControls(settings->timeWithoutMouseMovementBeforeHidingControls())
    , m_useLegacyBackgroundSizeShorthandBehavior(settings->useLegacyBackgroundSizeShorthandBehavior())
{
}

void InternalSettings::Backup::restoreTo(Settings* settings)
{
    RuntimeEnabledFeatures::setCSSExclusionsEnabled(m_originalCSSExclusionsEnabled);
    RuntimeEnabledFeatures::setCSSShapesEnabled(m_originalCSSShapesEnabled);
    settings->setCSSVariablesEnabled(m_originalCSSVariablesEnabled);
#if ENABLE(SHADOW_DOM)
    RuntimeEnabledFeatures::setShadowDOMEnabled(m_originalShadowDOMEnabled);
    RuntimeEnabledFeatures::setAuthorShadowDOMForAnyElementEnabled(m_originalAuthorShadowDOMForAnyElementEnabled);
#endif
#if ENABLE(STYLE_SCOPED)
    RuntimeEnabledFeatures::setStyleScopedEnabled(m_originalStyleScoped);
#endif
    settings->setEditingBehaviorType(m_originalEditingBehavior);
#if ENABLE(TEXT_AUTOSIZING)
    settings->setTextAutosizingEnabled(m_originalTextAutosizingEnabled);
    settings->setTextAutosizingWindowSizeOverride(m_originalTextAutosizingWindowSizeOverride);
    settings->setTextAutosizingFontScaleFactor(m_originalTextAutosizingFontScaleFactor);
#endif
    settings->setMediaTypeOverride(m_originalMediaTypeOverride);
#if ENABLE(DIALOG_ELEMENT)
    RuntimeEnabledFeatures::setDialogElementEnabled(m_originalDialogElementEnabled);
#endif
    settings->setCanvasUsesAcceleratedDrawing(m_originalCanvasUsesAcceleratedDrawing);
    settings->setMockScrollbarsEnabled(m_originalMockScrollbarsEnabled);
    RuntimeEnabledFeatures::setLangAttributeAwareFormControlUIEnabled(m_langAttributeAwareFormControlUIEnabled);
    settings->setImagesEnabled(m_imagesEnabled);
    settings->setMinDOMTimerInterval(m_minimumTimerInterval);
#if ENABLE(VIDEO_TRACK)
    settings->setShouldDisplaySubtitles(m_shouldDisplaySubtitles);
    settings->setShouldDisplayCaptions(m_shouldDisplayCaptions);
    settings->setShouldDisplayTextDescriptions(m_shouldDisplayTextDescriptions);
#endif
    settings->setDefaultVideoPosterURL(m_defaultVideoPosterURL);
    settings->setTimeWithoutMouseMovementBeforeHidingControls(m_originalTimeWithoutMouseMovementBeforeHidingControls);
    settings->setUseLegacyBackgroundSizeShorthandBehavior(m_useLegacyBackgroundSizeShorthandBehavior);
}

// We can't use RefCountedSupplement because that would try to make InternalSettings RefCounted
// and InternalSettings is already RefCounted via its base class, InternalSettingsGenerated.
// Instead, we manually make InternalSettings supplement Page.
class InternalSettingsWrapper : public Supplement<Page> {
public:
    explicit InternalSettingsWrapper(Page* page)
        : m_internalSettings(InternalSettings::create(page)) { }
    virtual ~InternalSettingsWrapper() { m_internalSettings->hostDestroyed(); }
#if !ASSERT_DISABLED
    virtual bool isRefCountedWrapper() const OVERRIDE { return true; }
#endif
    InternalSettings* internalSettings() const { return m_internalSettings.get(); }

private:
    RefPtr<InternalSettings> m_internalSettings;
};

const char* InternalSettings::supplementName()
{
    return "InternalSettings";
}

InternalSettings* InternalSettings::from(Page* page)
{
    if (!Supplement<Page>::from(page, supplementName()))
        Supplement<Page>::provideTo(page, supplementName(), adoptPtr(new InternalSettingsWrapper(page)));
    return static_cast<InternalSettingsWrapper*>(Supplement<Page>::from(page, supplementName()))->internalSettings();
}

InternalSettings::~InternalSettings()
{
}

InternalSettings::InternalSettings(Page* page)
    : InternalSettingsGenerated(page)
    , m_page(page)
    , m_backup(page->settings())
{
}

void InternalSettings::resetToConsistentState()
{
    page()->setPageScaleFactor(1, IntPoint(0, 0));
    page()->setCanStartMedia(true);

    m_backup.restoreTo(settings());
    m_backup = Backup(settings());

    InternalSettingsGenerated::resetToConsistentState();
}

Settings* InternalSettings::settings() const
{
    if (!page())
        return 0;
    return page()->settings();
}

void InternalSettings::setMockScrollbarsEnabled(bool enabled, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    settings()->setMockScrollbarsEnabled(enabled);
}

static bool urlIsWhitelistedForSetShadowDOMEnabled(const String& url)
{
    // This check is just for preventing fuzzers from crashing because of unintended API calls.
    // You can list your test if needed.
    return notFound != url.find("fast/dom/shadow/content-shadow-unknown.html")
        || notFound != url.find("fast/dom/shadow/insertion-points-with-shadow-disabled.html");
}

void InternalSettings::setShadowDOMEnabled(bool enabled, ExceptionCode& ec)
{
    if (!urlIsWhitelistedForSetShadowDOMEnabled(page()->mainFrame()->document()->url().string())) {
        ec = INVALID_ACCESS_ERR;
        return;
    }

#if ENABLE(SHADOW_DOM)
    RuntimeEnabledFeatures::setShadowDOMEnabled(enabled);
#else
    // Even SHADOW_DOM is off, InternalSettings allows setShadowDOMEnabled(false) to
    // have broader test coverage. But it cannot be setShadowDOMEnabled(true).
    if (enabled)
        ec = INVALID_ACCESS_ERR;
#endif
}

void InternalSettings::setAuthorShadowDOMForAnyElementEnabled(bool isEnabled)
{
#if ENABLE(SHADOW_DOM)
    RuntimeEnabledFeatures::setAuthorShadowDOMForAnyElementEnabled(isEnabled);
#else
    UNUSED_PARAM(isEnabled);
#endif
}

void InternalSettings::setStyleScopedEnabled(bool enabled)
{
#if ENABLE(STYLE_SCOPED)
    RuntimeEnabledFeatures::setStyleScopedEnabled(enabled);
#else
    UNUSED_PARAM(enabled);
#endif
}

void InternalSettings::setTouchEventEmulationEnabled(bool enabled, ExceptionCode& ec)
{
#if ENABLE(TOUCH_EVENTS)
    InternalSettingsGuardForSettings();
    settings()->setTouchEventEmulationEnabled(enabled);
#else
    UNUSED_PARAM(enabled);
    UNUSED_PARAM(ec);
#endif
}

typedef void (Settings::*SetFontFamilyFunction)(const AtomicString&, UScriptCode);
static void setFontFamily(Settings* settings, const String& family, const String& script, SetFontFamilyFunction setter)
{
    UScriptCode code = scriptNameToCode(script);
    if (code != USCRIPT_INVALID_CODE)
        (settings->*setter)(family, code);
}

void InternalSettings::setStandardFontFamily(const String& family, const String& script, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    setFontFamily(settings(), family, script, &Settings::setStandardFontFamily);
}

void InternalSettings::setSerifFontFamily(const String& family, const String& script, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    setFontFamily(settings(), family, script, &Settings::setSerifFontFamily);
}

void InternalSettings::setSansSerifFontFamily(const String& family, const String& script, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    setFontFamily(settings(), family, script, &Settings::setSansSerifFontFamily);
}

void InternalSettings::setFixedFontFamily(const String& family, const String& script, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    setFontFamily(settings(), family, script, &Settings::setFixedFontFamily);
}

void InternalSettings::setCursiveFontFamily(const String& family, const String& script, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    setFontFamily(settings(), family, script, &Settings::setCursiveFontFamily);
}

void InternalSettings::setFantasyFontFamily(const String& family, const String& script, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    setFontFamily(settings(), family, script, &Settings::setFantasyFontFamily);
}

void InternalSettings::setPictographFontFamily(const String& family, const String& script, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    setFontFamily(settings(), family, script, &Settings::setPictographFontFamily);
}

void InternalSettings::setTextAutosizingEnabled(bool enabled, ExceptionCode& ec)
{
#if ENABLE(TEXT_AUTOSIZING)
    InternalSettingsGuardForSettings();
    settings()->setTextAutosizingEnabled(enabled);
#else
    UNUSED_PARAM(enabled);
    UNUSED_PARAM(ec);
#endif
}

void InternalSettings::setTextAutosizingWindowSizeOverride(int width, int height, ExceptionCode& ec)
{
#if ENABLE(TEXT_AUTOSIZING)
    InternalSettingsGuardForSettings();
    settings()->setTextAutosizingWindowSizeOverride(IntSize(width, height));
#else
    UNUSED_PARAM(width);
    UNUSED_PARAM(height);
    UNUSED_PARAM(ec);
#endif
}

void InternalSettings::setMediaTypeOverride(const String& mediaType, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    settings()->setMediaTypeOverride(mediaType);
}

void InternalSettings::setTextAutosizingFontScaleFactor(float fontScaleFactor, ExceptionCode& ec)
{
#if ENABLE(TEXT_AUTOSIZING)
    InternalSettingsGuardForSettings();
    settings()->setTextAutosizingFontScaleFactor(fontScaleFactor);
#else
    UNUSED_PARAM(fontScaleFactor);
    UNUSED_PARAM(ec);
#endif
}

void InternalSettings::setCSSExclusionsEnabled(bool enabled, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    RuntimeEnabledFeatures::setCSSExclusionsEnabled(enabled);
}

void InternalSettings::setCSSShapesEnabled(bool enabled, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    RuntimeEnabledFeatures::setCSSShapesEnabled(enabled);
}

void InternalSettings::setCSSVariablesEnabled(bool enabled, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    settings()->setCSSVariablesEnabled(enabled);
}

bool InternalSettings::cssVariablesEnabled(ExceptionCode& ec)
{
    InternalSettingsGuardForSettingsReturn(false);
    return settings()->cssVariablesEnabled();
}

void InternalSettings::setCanStartMedia(bool enabled, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    m_page->setCanStartMedia(enabled);
}

void InternalSettings::setEditingBehavior(const String& editingBehavior, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    if (equalIgnoringCase(editingBehavior, "win"))
        settings()->setEditingBehaviorType(EditingWindowsBehavior);
    else if (equalIgnoringCase(editingBehavior, "mac"))
        settings()->setEditingBehaviorType(EditingMacBehavior);
    else if (equalIgnoringCase(editingBehavior, "unix"))
        settings()->setEditingBehaviorType(EditingUnixBehavior);
    else
        ec = SYNTAX_ERR;
}

void InternalSettings::setDialogElementEnabled(bool enabled, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
#if ENABLE(DIALOG_ELEMENT)
    RuntimeEnabledFeatures::setDialogElementEnabled(enabled);
#else
    UNUSED_PARAM(enabled);
#endif
}

void InternalSettings::setShouldDisplayTrackKind(const String& kind, bool enabled, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();

#if ENABLE(VIDEO_TRACK)
    if (!page())
        return;
    CaptionUserPreferences* captionPreferences = page()->group().captionPreferences();

    if (equalIgnoringCase(kind, "Subtitles"))
        captionPreferences->setUserPrefersSubtitles(enabled);
    else if (equalIgnoringCase(kind, "Captions"))
        captionPreferences->setUserPrefersCaptions(enabled);
    else if (equalIgnoringCase(kind, "TextDescriptions"))
        captionPreferences->setUserPrefersTextDescriptions(enabled);
    else
        ec = SYNTAX_ERR;
#else
    UNUSED_PARAM(kind);
    UNUSED_PARAM(enabled);
#endif
}

bool InternalSettings::shouldDisplayTrackKind(const String& kind, ExceptionCode& ec)
{
    InternalSettingsGuardForSettingsReturn(false);

#if ENABLE(VIDEO_TRACK)
    if (!page())
        return false;
    CaptionUserPreferences* captionPreferences = page()->group().captionPreferences();

    if (equalIgnoringCase(kind, "Subtitles"))
        return captionPreferences->userPrefersSubtitles();
    if (equalIgnoringCase(kind, "Captions"))
        return captionPreferences->userPrefersCaptions();
    if (equalIgnoringCase(kind, "TextDescriptions"))
        return captionPreferences->userPrefersTextDescriptions();

    ec = SYNTAX_ERR;
    return false;
#else
    UNUSED_PARAM(kind);
    return false;
#endif
}

void InternalSettings::setStorageBlockingPolicy(const String& mode, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();

    if (mode == "AllowAll")
        settings()->setStorageBlockingPolicy(SecurityOrigin::AllowAllStorage);
    else if (mode == "BlockThirdParty")
        settings()->setStorageBlockingPolicy(SecurityOrigin::BlockThirdPartyStorage);
    else if (mode == "BlockAll")
        settings()->setStorageBlockingPolicy(SecurityOrigin::BlockAllStorage);
    else
        ec = SYNTAX_ERR;
}

void InternalSettings::setLangAttributeAwareFormControlUIEnabled(bool enabled)
{
    RuntimeEnabledFeatures::setLangAttributeAwareFormControlUIEnabled(enabled);
}

void InternalSettings::setImagesEnabled(bool enabled, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    settings()->setImagesEnabled(enabled);
}

void InternalSettings::setMinimumTimerInterval(double intervalInSeconds, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    settings()->setMinDOMTimerInterval(intervalInSeconds);
}

void InternalSettings::setDefaultVideoPosterURL(const String& url, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    settings()->setDefaultVideoPosterURL(url);
}

void InternalSettings::setTimeWithoutMouseMovementBeforeHidingControls(double time, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    settings()->setTimeWithoutMouseMovementBeforeHidingControls(time);
}

void InternalSettings::setUseLegacyBackgroundSizeShorthandBehavior(bool enabled, ExceptionCode& ec)
{
    InternalSettingsGuardForSettings();
    settings()->setUseLegacyBackgroundSizeShorthandBehavior(enabled);
}

}
