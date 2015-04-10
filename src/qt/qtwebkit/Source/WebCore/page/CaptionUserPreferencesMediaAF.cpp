/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(VIDEO_TRACK)

#include "CaptionUserPreferencesMediaAF.h"

#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)
#include "CoreText/CoreText.h"
#endif
#include "DOMWrapperWorld.h"
#include "FloatConversion.h"
#include "HTMLMediaElement.h"
#include "KURL.h"
#include "Language.h"
#include "LocalizedStrings.h"
#include "Logging.h"
#include "MediaControlElements.h"
#include "PageGroup.h"
#include "SoftLinking.h"
#include "TextTrackCue.h"
#include "TextTrackList.h"
#include "UserStyleSheetTypes.h"
#include <wtf/NonCopyingSort.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/StringBuilder.h>

#if PLATFORM(IOS)
#import "WebCoreThreadRun.h"
#endif

#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)
#include "MediaAccessibility/MediaAccessibility.h"
#endif

#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)

#if !PLATFORM(WIN)
#define SOFT_LINK_AVF_FRAMEWORK(Lib) SOFT_LINK_FRAMEWORK_OPTIONAL(Lib)
#define SOFT_LINK_AVF(Lib, Name, Type) SOFT_LINK(Lib, Name, Type)
#define SOFT_LINK_AVF_POINTER(Lib, Name, Type) SOFT_LINK_POINTER_OPTIONAL(Lib, Name, Type)
#define SOFT_LINK_AVF_FRAMEWORK_IMPORT(Lib, Fun, ReturnType, Arguments, Signature) SOFT_LINK(Lib, Fun, ReturnType, Arguments, Signature)
#else
#define SOFT_LINK_AVF_FRAMEWORK(Lib) SOFT_LINK_LIBRARY(Lib)
#define SOFT_LINK_AVF(Lib, Name, Type) SOFT_LINK_DLL_IMPORT(Lib, Name, Type)
#define SOFT_LINK_AVF_POINTER(Lib, Name, Type) SOFT_LINK_VARIABLE_DLL_IMPORT_OPTIONAL(Lib, Name, Type)
#define SOFT_LINK_AVF_FRAMEWORK_IMPORT(Lib, Fun, ReturnType, Arguments, Signature) SOFT_LINK_DLL_IMPORT(Lib, Fun, ReturnType, __cdecl, Arguments, Signature)
#endif

SOFT_LINK_AVF_FRAMEWORK(MediaAccessibility)
SOFT_LINK_AVF_FRAMEWORK(CoreText)

SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceGetDisplayType, MACaptionAppearanceDisplayType, (MACaptionAppearanceDomain domain), (domain))
SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceSetDisplayType, void, (MACaptionAppearanceDomain domain, MACaptionAppearanceDisplayType displayType), (domain, displayType))
SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceCopyForegroundColor, CGColorRef, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceCopyBackgroundColor, CGColorRef, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceCopyWindowColor, CGColorRef, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceGetForegroundOpacity, CGFloat, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceGetBackgroundOpacity, CGFloat, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceGetWindowOpacity, CGFloat, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceGetWindowRoundedCornerRadius, CGFloat, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceCopyFontDescriptorForStyle, CTFontDescriptorRef, (MACaptionAppearanceDomain domain,  MACaptionAppearanceBehavior *behavior, MACaptionAppearanceFontStyle fontStyle), (domain, behavior, fontStyle))
SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceGetRelativeCharacterSize, CGFloat, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceGetTextEdgeStyle, MACaptionAppearanceTextEdgeStyle, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceAddSelectedLanguage, bool, (MACaptionAppearanceDomain domain, CFStringRef language), (domain, language));
SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceCopySelectedLanguages, CFArrayRef, (MACaptionAppearanceDomain domain), (domain));
SOFT_LINK_AVF_FRAMEWORK_IMPORT(MediaAccessibility, MACaptionAppearanceCopyPreferredCaptioningMediaCharacteristics,  CFArrayRef, (MACaptionAppearanceDomain domain), (domain));

SOFT_LINK_AVF_FRAMEWORK_IMPORT(CoreText, CTFontDescriptorCopyAttribute,  CFTypeRef, (CTFontDescriptorRef descriptor, CFStringRef attribute), (descriptor, attribute));

#if PLATFORM(WIN)
// These are needed on Windows due to the way DLLs work. We do not need them on other platforms
#define MACaptionAppearanceGetDisplayType softLink_MACaptionAppearanceGetDisplayType
#define MACaptionAppearanceSetDisplayType softLink_MACaptionAppearanceSetDisplayType
#define MACaptionAppearanceCopyForegroundColor softLink_MACaptionAppearanceCopyForegroundColor
#define MACaptionAppearanceCopyBackgroundColor softLink_MACaptionAppearanceCopyBackgroundColor
#define MACaptionAppearanceCopyWindowColor softLink_MACaptionAppearanceCopyWindowColor
#define MACaptionAppearanceGetForegroundOpacity softLink_MACaptionAppearanceGetForegroundOpacity
#define MACaptionAppearanceGetBackgroundOpacity softLink_MACaptionAppearanceGetBackgroundOpacity
#define MACaptionAppearanceGetWindowOpacity softLink_MACaptionAppearanceGetWindowOpacity
#define MACaptionAppearanceGetWindowRoundedCornerRadius softLink_MACaptionAppearanceGetWindowRoundedCornerRadius
#define MACaptionAppearanceCopyFontDescriptorForStyle softLink_MACaptionAppearanceCopyFontDescriptorForStyle
#define MACaptionAppearanceGetRelativeCharacterSize softLink_MACaptionAppearanceGetRelativeCharacterSize
#define MACaptionAppearanceGetTextEdgeStyle softLink_MACaptionAppearanceGetTextEdgeStyle
#define MACaptionAppearanceAddSelectedLanguage softLink_MACaptionAppearanceAddSelectedLanguage
#define MACaptionAppearanceCopySelectedLanguages softLink_MACaptionAppearanceCopySelectedLanguages
#define MACaptionAppearanceCopyPreferredCaptioningMediaCharacteristics softLink_MACaptionAppearanceCopyPreferredCaptioningMediaCharacteristics
#define CTFontDescriptorCopyAttribute softLink_CTFontDescriptorCopyAttribute
#endif

SOFT_LINK_AVF_POINTER(MediaAccessibility, kMAXCaptionAppearanceSettingsChangedNotification, CFStringRef)
#define kMAXCaptionAppearanceSettingsChangedNotification getkMAXCaptionAppearanceSettingsChangedNotification()

SOFT_LINK_AVF_POINTER(CoreText, kCTFontNameAttribute, CFStringRef)
#define kCTFontNameAttribute getkCTFontNameAttribute()
#endif

using namespace std;

namespace WebCore {

#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)
static void userCaptionPreferencesChangedNotificationCallback(CFNotificationCenterRef, void* observer, CFStringRef, const void *, CFDictionaryRef)
{
#if !PLATFORM(IOS)
    static_cast<CaptionUserPreferencesMediaAF*>(observer)->captionPreferencesChanged();
#else
    WebThreadRun(^{
        static_cast<CaptionUserPreferencesMediaAF*>(observer)->captionPreferencesChanged();
    });
#endif
}
#endif

CaptionUserPreferencesMediaAF::CaptionUserPreferencesMediaAF(PageGroup* group)
    : CaptionUserPreferences(group)
#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)
    , m_listeningForPreferenceChanges(false)
#endif
{
}

CaptionUserPreferencesMediaAF::~CaptionUserPreferencesMediaAF()
{
#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)
    if (kMAXCaptionAppearanceSettingsChangedNotification)
        CFNotificationCenterRemoveObserver(CFNotificationCenterGetLocalCenter(), this, kMAXCaptionAppearanceSettingsChangedNotification, 0);
#endif
}

#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)

CaptionUserPreferences::CaptionDisplayMode CaptionUserPreferencesMediaAF::captionDisplayMode() const
{
    if (testingMode() || !MediaAccessibilityLibrary())
        return CaptionUserPreferences::captionDisplayMode();

    MACaptionAppearanceDisplayType displayType = MACaptionAppearanceGetDisplayType(kMACaptionAppearanceDomainUser);
    switch (displayType) {
    case kMACaptionAppearanceDisplayTypeForcedOnly:
        return ForcedOnly;
        break;

    case kMACaptionAppearanceDisplayTypeAutomatic:
        return Automatic;
        break;

    case kMACaptionAppearanceDisplayTypeAlwaysOn:
        return AlwaysOn;
        break;
    }

    ASSERT_NOT_REACHED();
    return ForcedOnly;
}
    
void CaptionUserPreferencesMediaAF::setCaptionDisplayMode(CaptionUserPreferences::CaptionDisplayMode mode)
{
    if (testingMode() || !MediaAccessibilityLibrary()) {
        CaptionUserPreferences::setCaptionDisplayMode(mode);
        return;
    }

    MACaptionAppearanceDisplayType displayType = kMACaptionAppearanceDisplayTypeForcedOnly;
    switch (mode) {
    case Automatic:
        displayType = kMACaptionAppearanceDisplayTypeAutomatic;
        break;
    case ForcedOnly:
        displayType = kMACaptionAppearanceDisplayTypeForcedOnly;
        break;
    case AlwaysOn:
        displayType = kMACaptionAppearanceDisplayTypeAlwaysOn;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    MACaptionAppearanceSetDisplayType(kMACaptionAppearanceDomainUser, displayType);
}

bool CaptionUserPreferencesMediaAF::userPrefersCaptions() const
{
    bool captionSetting = CaptionUserPreferences::userPrefersCaptions();
    if (captionSetting || testingMode() || !MediaAccessibilityLibrary())
        return captionSetting;
    
    RetainPtr<CFArrayRef> captioningMediaCharacteristics = adoptCF(MACaptionAppearanceCopyPreferredCaptioningMediaCharacteristics(kMACaptionAppearanceDomainUser));
    return captioningMediaCharacteristics && CFArrayGetCount(captioningMediaCharacteristics.get());
}

bool CaptionUserPreferencesMediaAF::userPrefersSubtitles() const
{
    bool subtitlesSetting = CaptionUserPreferences::userPrefersSubtitles();
    if (subtitlesSetting || testingMode() || !MediaAccessibilityLibrary())
        return subtitlesSetting;
    
    RetainPtr<CFArrayRef> captioningMediaCharacteristics = adoptCF(MACaptionAppearanceCopyPreferredCaptioningMediaCharacteristics(kMACaptionAppearanceDomainUser));
    return !(captioningMediaCharacteristics && CFArrayGetCount(captioningMediaCharacteristics.get()));
}

void CaptionUserPreferencesMediaAF::setInterestedInCaptionPreferenceChanges()
{
    if (!MediaAccessibilityLibrary())
        return;

    if (!kMAXCaptionAppearanceSettingsChangedNotification)
        return;

    if (!m_listeningForPreferenceChanges) {
        m_listeningForPreferenceChanges = true;
        CFNotificationCenterAddObserver(CFNotificationCenterGetLocalCenter(), this, userCaptionPreferencesChangedNotificationCallback, kMAXCaptionAppearanceSettingsChangedNotification, 0, CFNotificationSuspensionBehaviorCoalesce);
    }
    
    updateCaptionStyleSheetOveride();
}

void CaptionUserPreferencesMediaAF::captionPreferencesChanged()
{
    if (m_listeningForPreferenceChanges)
        updateCaptionStyleSheetOveride();

    CaptionUserPreferences::captionPreferencesChanged();
}

String CaptionUserPreferencesMediaAF::captionsWindowCSS() const
{
    MACaptionAppearanceBehavior behavior;
    RetainPtr<CGColorRef> color = adoptCF(MACaptionAppearanceCopyWindowColor(kMACaptionAppearanceDomainUser, &behavior));

    Color windowColor(color.get());
    if (!windowColor.isValid())
        windowColor = Color::transparent;

    bool important = behavior == kMACaptionAppearanceBehaviorUseValue;
    CGFloat opacity = MACaptionAppearanceGetWindowOpacity(kMACaptionAppearanceDomainUser, &behavior);
    if (!important)
        important = behavior == kMACaptionAppearanceBehaviorUseValue;
    String windowStyle = colorPropertyCSS(CSSPropertyBackgroundColor, Color(windowColor.red(), windowColor.green(), windowColor.blue(), static_cast<int>(opacity * 255)), important);

    if (!opacity)
        return windowStyle;

    StringBuilder builder;
    builder.append(windowStyle);
    builder.append(getPropertyNameString(CSSPropertyPadding));
    builder.append(": .4em !important;");

    return builder.toString();
}

String CaptionUserPreferencesMediaAF::captionsBackgroundCSS() const
{
    // This default value must be the same as the one specified in mediaControls.css for -webkit-media-text-track-past-nodes
    // and webkit-media-text-track-future-nodes.
    DEFINE_STATIC_LOCAL(Color, defaultBackgroundColor, (Color(0, 0, 0, 0.8 * 255)));

    MACaptionAppearanceBehavior behavior;

    RetainPtr<CGColorRef> color = adoptCF(MACaptionAppearanceCopyBackgroundColor(kMACaptionAppearanceDomainUser, &behavior));
    Color backgroundColor(color.get());
    if (!backgroundColor.isValid())
        backgroundColor = defaultBackgroundColor;

    bool important = behavior == kMACaptionAppearanceBehaviorUseValue;
    CGFloat opacity = MACaptionAppearanceGetBackgroundOpacity(kMACaptionAppearanceDomainUser, &behavior);
    if (!important)
        important = behavior == kMACaptionAppearanceBehaviorUseValue;
    return colorPropertyCSS(CSSPropertyBackgroundColor, Color(backgroundColor.red(), backgroundColor.green(), backgroundColor.blue(), static_cast<int>(opacity * 255)), important);
}

Color CaptionUserPreferencesMediaAF::captionsTextColor(bool& important) const
{
    MACaptionAppearanceBehavior behavior;
    RetainPtr<CGColorRef> color = adoptCF(MACaptionAppearanceCopyForegroundColor(kMACaptionAppearanceDomainUser, &behavior));
    Color textColor(color.get());
    if (!textColor.isValid())
        // This default value must be the same as the one specified in mediaControls.css for -webkit-media-text-track-container.
        textColor = Color::white;
    
    important = behavior == kMACaptionAppearanceBehaviorUseValue;
    CGFloat opacity = MACaptionAppearanceGetForegroundOpacity(kMACaptionAppearanceDomainUser, &behavior);
    if (!important)
        important = behavior == kMACaptionAppearanceBehaviorUseValue;
    return Color(textColor.red(), textColor.green(), textColor.blue(), static_cast<int>(opacity * 255));
}
    
String CaptionUserPreferencesMediaAF::captionsTextColorCSS() const
{
    bool important;
    Color textColor = captionsTextColor(important);

    if (!textColor.isValid())
        return emptyString();

    return colorPropertyCSS(CSSPropertyColor, textColor, important);
}
    
String CaptionUserPreferencesMediaAF::windowRoundedCornerRadiusCSS() const
{
    MACaptionAppearanceBehavior behavior;
    CGFloat radius = MACaptionAppearanceGetWindowRoundedCornerRadius(kMACaptionAppearanceDomainUser, &behavior);
    if (!radius)
        return emptyString();

    StringBuilder builder;
    builder.append(getPropertyNameString(CSSPropertyBorderRadius));
    builder.append(String::format(":%.02fpx", radius));
    if (behavior == kMACaptionAppearanceBehaviorUseValue)
        builder.append(" !important");
    builder.append(';');

    return builder.toString();
}
    
Color CaptionUserPreferencesMediaAF::captionsEdgeColorForTextColor(const Color& textColor) const
{
    int distanceFromWhite = differenceSquared(textColor, Color::white);
    int distanceFromBlack = differenceSquared(textColor, Color::black);
    
    if (distanceFromWhite < distanceFromBlack)
        return textColor.dark();
    
    return textColor.light();
}

String CaptionUserPreferencesMediaAF::cssPropertyWithTextEdgeColor(CSSPropertyID id, const String& value, const Color& textColor, bool important) const
{
    StringBuilder builder;
    
    builder.append(getPropertyNameString(id));
    builder.append(':');
    builder.append(value);
    builder.append(' ');
    builder.append(captionsEdgeColorForTextColor(textColor).serialized());
    if (important)
        builder.append(" !important");
    builder.append(';');
    
    return builder.toString();
}

String CaptionUserPreferencesMediaAF::colorPropertyCSS(CSSPropertyID id, const Color& color, bool important) const
{
    StringBuilder builder;
    
    builder.append(getPropertyNameString(id));
    builder.append(':');
    builder.append(color.serialized());
    if (important)
        builder.append(" !important");
    builder.append(';');
    
    return builder.toString();
}

String CaptionUserPreferencesMediaAF::captionsTextEdgeCSS() const
{
    DEFINE_STATIC_LOCAL(const String, edgeStyleRaised, (" -.05em -.05em 0 ", String::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const String, edgeStyleDepressed, (" .05em .05em 0 ", String::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const String, edgeStyleDropShadow, (" .075em .075em 0 ", String::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const String, edgeStyleUniform, (" .03em ", String::ConstructFromLiteral));

    bool unused;
    Color color = captionsTextColor(unused);
    if (!color.isValid())
        color.setNamedColor("black");
    color = captionsEdgeColorForTextColor(color);

    MACaptionAppearanceBehavior behavior;
    MACaptionAppearanceTextEdgeStyle textEdgeStyle = MACaptionAppearanceGetTextEdgeStyle(kMACaptionAppearanceDomainUser, &behavior);
    switch (textEdgeStyle) {
    case kMACaptionAppearanceTextEdgeStyleUndefined:
    case kMACaptionAppearanceTextEdgeStyleNone:
        return emptyString();
            
    case kMACaptionAppearanceTextEdgeStyleRaised:
        return cssPropertyWithTextEdgeColor(CSSPropertyTextShadow, edgeStyleRaised, color, behavior == kMACaptionAppearanceBehaviorUseValue);
    case kMACaptionAppearanceTextEdgeStyleDepressed:
        return cssPropertyWithTextEdgeColor(CSSPropertyTextShadow, edgeStyleDepressed, color, behavior == kMACaptionAppearanceBehaviorUseValue);
    case kMACaptionAppearanceTextEdgeStyleDropShadow:
        return cssPropertyWithTextEdgeColor(CSSPropertyTextShadow, edgeStyleDropShadow, color, behavior == kMACaptionAppearanceBehaviorUseValue);
    case kMACaptionAppearanceTextEdgeStyleUniform:
        return cssPropertyWithTextEdgeColor(CSSPropertyWebkitTextStroke, edgeStyleUniform, color, behavior == kMACaptionAppearanceBehaviorUseValue);
            
    default:
        ASSERT_NOT_REACHED();
        break;
    }
    
    return emptyString();
}

String CaptionUserPreferencesMediaAF::captionsDefaultFontCSS() const
{
    MACaptionAppearanceBehavior behavior;
    
    RetainPtr<CTFontDescriptorRef> font = adoptCF(MACaptionAppearanceCopyFontDescriptorForStyle(kMACaptionAppearanceDomainUser, &behavior, kMACaptionAppearanceFontStyleDefault));
    if (!font)
        return emptyString();

    RetainPtr<CFTypeRef> name = adoptCF(CTFontDescriptorCopyAttribute(font.get(), kCTFontNameAttribute));
    if (!name)
        return emptyString();
    
    StringBuilder builder;
    
    builder.append(getPropertyNameString(CSSPropertyFontFamily));
    builder.append(": \"");
    builder.append(static_cast<CFStringRef>(name.get()));
    builder.append('"');
    if (behavior == kMACaptionAppearanceBehaviorUseValue)
        builder.append(" !important");
    builder.append(';');
    
    return builder.toString();
}

float CaptionUserPreferencesMediaAF::captionFontSizeScaleAndImportance(bool& important) const
{
    if (testingMode() || !MediaAccessibilityLibrary())
        return CaptionUserPreferences::captionFontSizeScaleAndImportance(important);

    MACaptionAppearanceBehavior behavior;
    CGFloat characterScale = CaptionUserPreferences::captionFontSizeScaleAndImportance(important);
    CGFloat scaleAdjustment = MACaptionAppearanceGetRelativeCharacterSize(kMACaptionAppearanceDomainUser, &behavior);

    if (!scaleAdjustment)
        return characterScale;

    important = behavior == kMACaptionAppearanceBehaviorUseValue;
#if defined(__LP64__) && __LP64__
    return narrowPrecisionToFloat(scaleAdjustment * characterScale);
#else
    return scaleAdjustment * characterScale;
#endif
}

void CaptionUserPreferencesMediaAF::setPreferredLanguage(const String& language)
{
    if (testingMode() || !MediaAccessibilityLibrary()) {
        CaptionUserPreferences::setPreferredLanguage(language);
        return;
    }

    MACaptionAppearanceAddSelectedLanguage(kMACaptionAppearanceDomainUser, language.createCFString().get());
}

Vector<String> CaptionUserPreferencesMediaAF::preferredLanguages() const
{
    if (testingMode() || !MediaAccessibilityLibrary())
        return CaptionUserPreferences::preferredLanguages();

    Vector<String> platformLanguages = platformUserPreferredLanguages();
    Vector<String> override = userPreferredLanguagesOverride();
    if (!override.isEmpty()) {
        if (platformLanguages.size() != override.size())
            return override;
        for (size_t i = 0; i < override.size(); i++) {
            if (override[i] != platformLanguages[i])
                return override;
        }
    }

    CFIndex languageCount = 0;
    RetainPtr<CFArrayRef> languages = adoptCF(MACaptionAppearanceCopySelectedLanguages(kMACaptionAppearanceDomainUser));
    if (languages)
        languageCount = CFArrayGetCount(languages.get());

    if (!languageCount)
        return CaptionUserPreferences::preferredLanguages();

    Vector<String> userPreferredLanguages;
    userPreferredLanguages.reserveCapacity(languageCount + platformLanguages.size());
    for (CFIndex i = 0; i < languageCount; i++)
        userPreferredLanguages.append(static_cast<CFStringRef>(CFArrayGetValueAtIndex(languages.get(), i)));

    userPreferredLanguages.appendVector(platformLanguages);

    return userPreferredLanguages;
}
#endif // HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)

String CaptionUserPreferencesMediaAF::captionsStyleSheetOverride() const
{
    if (testingMode())
        return CaptionUserPreferences::captionsStyleSheetOverride();
    
    StringBuilder captionsOverrideStyleSheet;

#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)
    if (!MediaAccessibilityLibrary())
        return CaptionUserPreferences::captionsStyleSheetOverride();
    
    String captionsColor = captionsTextColorCSS();
    String edgeStyle = captionsTextEdgeCSS();
    String fontName = captionsDefaultFontCSS();
    String background = captionsBackgroundCSS();
    if (!background.isEmpty() || !captionsColor.isEmpty() || !edgeStyle.isEmpty() || !fontName.isEmpty()) {
        captionsOverrideStyleSheet.append(" video::");
        captionsOverrideStyleSheet.append(TextTrackCue::cueShadowPseudoId());
        captionsOverrideStyleSheet.append('{');
        
        if (!background.isEmpty())
            captionsOverrideStyleSheet.append(background);
        if (!captionsColor.isEmpty())
            captionsOverrideStyleSheet.append(captionsColor);
        if (!edgeStyle.isEmpty())
            captionsOverrideStyleSheet.append(edgeStyle);
        if (!fontName.isEmpty())
            captionsOverrideStyleSheet.append(fontName);
        
        captionsOverrideStyleSheet.append('}');
    }
    
    String windowColor = captionsWindowCSS();
    String windowCornerRadius = windowRoundedCornerRadiusCSS();
    if (!windowColor.isEmpty() || !windowCornerRadius.isEmpty()) {
        captionsOverrideStyleSheet.append(" video::");
        captionsOverrideStyleSheet.append(TextTrackCueBox::textTrackCueBoxShadowPseudoId());
        captionsOverrideStyleSheet.append('{');
        
        if (!windowColor.isEmpty())
            captionsOverrideStyleSheet.append(windowColor);
        if (!windowCornerRadius.isEmpty())
            captionsOverrideStyleSheet.append(windowCornerRadius);
        
        captionsOverrideStyleSheet.append('}');
    }
#endif // HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)

    LOG(Media, "CaptionUserPreferencesMediaAF::captionsStyleSheetOverrideSetting sytle to:\n%s", captionsOverrideStyleSheet.toString().utf8().data());

    return captionsOverrideStyleSheet.toString();
}

static String languageIdentifier(const String& languageCode)
{
    if (languageCode.isEmpty())
        return languageCode;

    String lowercaseLanguageCode = languageCode.lower();

    // Need 2U here to disambiguate String::operator[] from operator(NSString*, int)[] in a production build.
    if (lowercaseLanguageCode.length() >= 3 && (lowercaseLanguageCode[2U] == '_' || lowercaseLanguageCode[2U] == '-'))
        lowercaseLanguageCode.truncate(2);

    return lowercaseLanguageCode;
}

static String trackDisplayName(TextTrack* track)
{
    if (track == TextTrack::captionMenuOffItem())
        return textTrackOffMenuItemText();
    if (track == TextTrack::captionMenuAutomaticItem())
        return textTrackAutomaticMenuItemText();

    StringBuilder displayName;
    String label = track->label();
    String trackLanguageIdentifier = track->language();

    RetainPtr<CFLocaleRef> currentLocale = adoptCF(CFLocaleCopyCurrent());
    RetainPtr<CFStringRef> localeIdentifier = adoptCF(CFLocaleCreateCanonicalLocaleIdentifierFromString(kCFAllocatorDefault, trackLanguageIdentifier.createCFString().get()));
    RetainPtr<CFStringRef> languageCF = adoptCF(CFLocaleCopyDisplayNameForPropertyValue(currentLocale.get(), kCFLocaleLanguageCode, localeIdentifier.get()));
    String language = languageCF.get();
    if (!label.isEmpty()) {
        if (language.isEmpty() || label.contains(language))
            displayName.append(label);
        else {
            RetainPtr<CFDictionaryRef> localeDict = adoptCF(CFLocaleCreateComponentsFromLocaleIdentifier(kCFAllocatorDefault, localeIdentifier.get()));
            if (localeDict) {
                CFStringRef countryCode = 0;
                String countryName;
                
                CFDictionaryGetValueIfPresent(localeDict.get(), kCFLocaleCountryCode, (const void **)&countryCode);
                if (countryCode) {
                    RetainPtr<CFStringRef> countryNameCF = adoptCF(CFLocaleCopyDisplayNameForPropertyValue(currentLocale.get(), kCFLocaleCountryCode, countryCode));
                    countryName = countryNameCF.get();
                }
                
                if (!countryName.isEmpty())
                    displayName.append(textTrackCountryAndLanguageMenuItemText(label, countryName, language));
                else
                    displayName.append(textTrackLanguageMenuItemText(label, language));
            }
        }
    } else {
        String languageAndLocale = CFLocaleCopyDisplayNameForPropertyValue(currentLocale.get(), kCFLocaleIdentifier, trackLanguageIdentifier.createCFString().get());
        if (!languageAndLocale.isEmpty())
            displayName.append(languageAndLocale);
        else if (!language.isEmpty())
            displayName.append(language);
        else
            displayName.append(localeIdentifier.get());
    }
    
    if (displayName.isEmpty())
        displayName.append(textTrackNoLabelText());
    
    if (track->isEasyToRead())
        return easyReaderTrackMenuItemText(displayName.toString());
    
    if (track->isClosedCaptions())
        return closedCaptionTrackMenuItemText(displayName.toString());

    if (track->isSDH())
        return sdhTrackMenuItemText(displayName.toString());

    return displayName.toString();
}

String CaptionUserPreferencesMediaAF::displayNameForTrack(TextTrack* track) const
{
    return trackDisplayName(track);
}

int CaptionUserPreferencesMediaAF::textTrackSelectionScore(TextTrack* track, HTMLMediaElement* mediaElement) const
{
    CaptionDisplayMode displayMode = captionDisplayMode();
    bool legacyOverride = mediaElement->webkitClosedCaptionsVisible();
    if (displayMode == AlwaysOn && (!userPrefersSubtitles() && !userPrefersCaptions() && !legacyOverride))
        return 0;
    if (track->kind() != TextTrack::captionsKeyword() && track->kind() != TextTrack::subtitlesKeyword() && track->kind() != TextTrack::forcedKeyword())
        return 0;
    if (!track->isMainProgramContent())
        return 0;

    bool trackHasOnlyForcedSubtitles = track->containsOnlyForcedSubtitles();
    if (!legacyOverride && ((trackHasOnlyForcedSubtitles && displayMode != ForcedOnly) || (!trackHasOnlyForcedSubtitles && displayMode == ForcedOnly)))
        return 0;

    Vector<String> userPreferredCaptionLanguages = preferredLanguages();

    if ((displayMode == Automatic && !legacyOverride) || trackHasOnlyForcedSubtitles) {

        if (!mediaElement || !mediaElement->player())
            return 0;

        String textTrackLanguage = track->language();
        if (textTrackLanguage.isEmpty())
            return 0;

        Vector<String> languageList;
        languageList.reserveCapacity(1);

        String audioTrackLanguage;
        if (testingMode())
            audioTrackLanguage = primaryAudioTrackLanguageOverride();
        else
            audioTrackLanguage = mediaElement->player()->languageOfPrimaryAudioTrack();

        if (audioTrackLanguage.isEmpty())
            return 0;

        if (trackHasOnlyForcedSubtitles) {
            languageList.append(audioTrackLanguage);
            size_t offset = indexOfBestMatchingLanguageInList(textTrackLanguage, languageList);

            // Only consider a forced-only track if it IS in the same language as the primary audio track.
            if (offset)
                return 0;
        } else {
            languageList.append(defaultLanguage());

            // Only enable a text track if the current audio track is NOT in the user's preferred language ...
            size_t offset = indexOfBestMatchingLanguageInList(audioTrackLanguage, languageList);
            if (!offset)
                return 0;

            // and the text track matches the user's preferred language.
            offset = indexOfBestMatchingLanguageInList(textTrackLanguage, languageList);
            if (offset)
                return 0;
        }

        userPreferredCaptionLanguages = languageList;
    }

    int trackScore = 0;

    if (userPrefersCaptions()) {
        // When the user prefers accessiblity tracks, rank is SDH, then CC, then subtitles.
        if (track->kind() == track->subtitlesKeyword())
            trackScore = 1;
        else if (track->isClosedCaptions())
            trackScore = 2;
        else
            trackScore = 3;
    } else {
        // When the user prefers translation tracks, rank is subtitles, then SDH, then CC tracks.
        if (track->kind() == track->subtitlesKeyword())
            trackScore = 3;
        else if (!track->isClosedCaptions())
            trackScore = 2;
        else
            trackScore = 1;
    }

    return trackScore + textTrackLanguageSelectionScore(track, userPreferredCaptionLanguages);
}

static bool textTrackCompare(const RefPtr<TextTrack>& a, const RefPtr<TextTrack>& b)
{
    String preferredLanguageDisplayName = displayNameForLanguageLocale(languageIdentifier(defaultLanguage()));
    String aLanguageDisplayName = displayNameForLanguageLocale(languageIdentifier(a->language()));
    String bLanguageDisplayName = displayNameForLanguageLocale(languageIdentifier(b->language()));

    // Tracks in the user's preferred language are always at the top of the menu.
    bool aIsPreferredLanguage = !codePointCompare(aLanguageDisplayName, preferredLanguageDisplayName);
    bool bIsPreferredLanguage = !codePointCompare(bLanguageDisplayName, preferredLanguageDisplayName);
    if ((aIsPreferredLanguage || bIsPreferredLanguage) && (aIsPreferredLanguage != bIsPreferredLanguage))
        return aIsPreferredLanguage;

    // Tracks not in the user's preferred language sort first by language ...
    if (codePointCompare(aLanguageDisplayName, bLanguageDisplayName))
        return codePointCompare(aLanguageDisplayName, bLanguageDisplayName) < 0;

    // ... but when tracks have the same language, main program content sorts next highest ...
    bool aIsMainContent = a->isMainProgramContent();
    bool bIsMainContent = b->isMainProgramContent();
    if ((aIsMainContent || bIsMainContent) && (aIsMainContent != bIsMainContent))
        return aIsMainContent;

    // ... and main program trakcs sort higher than CC tracks ...
    bool aIsCC = a->isClosedCaptions();
    bool bIsCC = b->isClosedCaptions();
    if ((aIsCC || bIsCC) && (aIsCC != bIsCC)) {
        if (aIsCC)
            return aIsMainContent;
        return bIsMainContent;
    }

    // ... and tracks of the same type and language sort by the menu item text.
    return codePointCompare(trackDisplayName(a.get()), trackDisplayName(b.get())) < 0;
}

Vector<RefPtr<TextTrack> > CaptionUserPreferencesMediaAF::sortedTrackListForMenu(TextTrackList* trackList)
{
    ASSERT(trackList);

    Vector<RefPtr<TextTrack> > tracksForMenu;
    HashSet<String> languagesIncluded;
    bool prefersAccessibilityTracks = userPrefersCaptions();
    bool filterTrackList = shouldFilterTrackMenu();

    for (unsigned i = 0, length = trackList->length(); i < length; ++i) {
        TextTrack* track = trackList->item(i);
        String language = displayNameForLanguageLocale(track->language());

        if (track->containsOnlyForcedSubtitles()) {
            LOG(Media, "CaptionUserPreferencesMac::sortedTrackListForMenu - skipping '%s' track with language '%s' because it contains only forced subtitles", track->kind().string().utf8().data(), language.utf8().data());
            continue;
        }
        
        if (track->isEasyToRead()) {
            LOG(Media, "CaptionUserPreferencesMac::sortedTrackListForMenu - adding '%s' track with language '%s' because it is 'easy to read'", track->kind().string().utf8().data(), language.utf8().data());
            if (!language.isEmpty())
                languagesIncluded.add(language);
            tracksForMenu.append(track);
            continue;
        }

        if (track->mode() == TextTrack::showingKeyword()) {
            LOG(Media, "CaptionUserPreferencesMac::sortedTrackListForMenu - adding '%s' track with language '%s' because it is already visible", track->kind().string().utf8().data(), language.utf8().data());
            if (!language.isEmpty())
                languagesIncluded.add(language);
            tracksForMenu.append(track);
            continue;
        }

        if (!language.isEmpty() && track->isMainProgramContent()) {
            bool isAccessibilityTrack = track->kind() == track->captionsKeyword();
            if (prefersAccessibilityTracks) {
                // In the first pass, include only caption tracks if the user prefers accessibility tracks.
                if (!isAccessibilityTrack && filterTrackList) {
                    LOG(Media, "CaptionUserPreferencesMediaAF::sortedTrackListForMenu - skipping '%s' track with language '%s' because it is NOT an accessibility track", track->kind().string().utf8().data(), language.utf8().data());
                    continue;
                }
            } else {
                // In the first pass, only include the first non-CC or SDH track with each language if the user prefers translation tracks.
                if (isAccessibilityTrack && filterTrackList) {
                    LOG(Media, "CaptionUserPreferencesMediaAF::sortedTrackListForMenu - skipping '%s' track with language '%s' because it is an accessibility track", track->kind().string().utf8().data(), language.utf8().data());
                    continue;
                }
                if (languagesIncluded.contains(language)  && filterTrackList) {
                    LOG(Media, "CaptionUserPreferencesMediaAF::sortedTrackListForMenu - skipping '%s' track with language '%s' because it is not the first with this language", track->kind().string().utf8().data(), language.utf8().data());
                    continue;
                }
            }
        }

        if (!language.isEmpty())
            languagesIncluded.add(language);
        tracksForMenu.append(track);

        LOG(Media, "CaptionUserPreferencesMac::sortedTrackListForMenu - adding '%s' track with language '%s', is%s main program content", track->kind().string().utf8().data(), language.utf8().data(), track->isMainProgramContent() ? "" : " NOT");
    }

    // Now that we have filtered for the user's accessibility/translation preference, add  all tracks with a unique language without regard to track type.
    for (unsigned i = 0, length = trackList->length(); i < length; ++i) {
        TextTrack* track = trackList->item(i);
        String language = displayNameForLanguageLocale(track->language());

        // All candidates with no languge were added the first time through.
        if (language.isEmpty())
            continue;

        if (track->containsOnlyForcedSubtitles())
            continue;

        if (!languagesIncluded.contains(language) && track->isMainProgramContent()) {
            languagesIncluded.add(language);
            tracksForMenu.append(track);
            LOG(Media, "CaptionUserPreferencesMediaAF::sortedTrackListForMenu - adding '%s' track with language '%s' because it is the only track with this language", track->kind().string().utf8().data(), language.utf8().data());
        }
    }

    nonCopyingSort(tracksForMenu.begin(), tracksForMenu.end(), textTrackCompare);

    tracksForMenu.insert(0, TextTrack::captionMenuOffItem());
    tracksForMenu.insert(1, TextTrack::captionMenuAutomaticItem());

    return tracksForMenu;
}
    
}

#endif // ENABLE(VIDEO_TRACK)
