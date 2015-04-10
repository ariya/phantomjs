/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "LocalizedStrings.h"

#include "IntSize.h"
#include "NotImplemented.h"
#include <BlackBerryPlatformString.h>
#include <LocaleHandler.h>
#include <LocalizeResource.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

DEFINE_STATIC_LOCAL(BlackBerry::Platform::LocalizeResource, s_resource, ());

String fileButtonChooseFileLabel()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::FILE_CHOOSE_BUTTON_LABEL));
}

String fileButtonChooseMultipleFilesLabel()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::FILE_CHOOSE_MULTIPLE_BUTTON_LABEL));
}

String fileButtonNoFileSelectedLabel()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::FILE_BUTTON_NO_FILE_SELECTED_LABEL));
}

String resetButtonDefaultLabel()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::RESET_BUTTON_LABEL));
}

String submitButtonDefaultLabel()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::SUBMIT_BUTTON_LABEL));
}

String inputElementAltText()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::SUBMIT_BUTTON_LABEL));
}

static String platformLanguage()
{
    String lang = BlackBerry::Platform::LocaleHandler::instance()->language().c_str();
    // getLocale() returns a POSIX locale which uses '_' to separate language and country.
    // However, we use '-' instead of '_' in WebCore (e.g. en_us should read en-us)
    size_t underscorePosition = lang.find('_');
    String replaceWith = ASCIILiteral("-");
    if (underscorePosition != notFound)
        return lang.replace(underscorePosition, replaceWith.length(), replaceWith);
    return lang;
}

Vector<String> platformUserPreferredLanguages()
{
    Vector<String> userPreferredLanguages;
    userPreferredLanguages.append(platformLanguage());
    return userPreferredLanguages;
}

String searchableIndexIntroduction()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::SEARCHABLE_INDEX_INTRODUCTION));
}

String searchMenuNoRecentSearchesText()
{
    notImplemented();
    return String();
}

String searchMenuRecentSearchesText()
{
    notImplemented();
    return String();
}

String searchMenuClearRecentSearchesText()
{
    notImplemented();
    return String();
}

String imageTitle(String const& filename, IntSize const& size)
{
    return filename + " (" + String::number(size.width()) + "x" + String::number(size.height()) + ")";
}

String AXButtonActionVerb()
{
    notImplemented();
    return String();
}

String AXCheckedCheckBoxActionVerb()
{
    notImplemented();
    return String();
}

String AXDefinitionText()
{
    notImplemented();
    return String();
}

String AXDescriptionListText()
{
    notImplemented();
    return String();
}

String AXDescriptionListDetailText()
{
    notImplemented();
    return String();
}

String AXDescriptionListTermText()
{
    notImplemented();
    return String();
}

String AXFooterRoleDescriptionText()
{
    notImplemented();
    return String();
}

String AXLinkActionVerb()
{
    notImplemented();
    return String();
}

String AXRadioButtonActionVerb()
{
    notImplemented();
    return String();
}

String AXTextFieldActionVerb()
{
    notImplemented();
    return String();
}

String AXUncheckedCheckBoxActionVerb()
{
    notImplemented();
    return String();
}

String AXMenuListPopupActionVerb()
{
    notImplemented();
    return String();
}

String AXMenuListActionVerb()
{
    notImplemented();
    return String();
}

String AXListItemActionVerb()
{
    notImplemented();
    return String();
}

String unknownFileSizeText()
{
    notImplemented();
    return String();
}

String validationMessagePatternMismatchText()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::VALIDATION_PATTERN_MISMATCH));
}

String validationMessageTooLongText(int, int)
{
    notImplemented();
    return String();
}

String validationMessageRangeUnderflowText(const String& text)
{
    return String::format(s_resource.getString(BlackBerry::Platform::VALIDATION_RANGE_UNDERFLOW), text.utf8().data());
}

String validationMessageRangeOverflowText(const String& text)
{
    return String::format(s_resource.getString(BlackBerry::Platform::VALIDATION_RANGE_OVERFLOW), text.utf8().data());
}

String validationMessageStepMismatchText(const String&, const String&)
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::VALIDATION_STEP_MISMATCH));
}

String validationMessageTypeMismatchText()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::VALIDATION_TYPE_MISMATCH));
}

String validationMessageTypeMismatchForEmailText()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::VALIDATION_TYPE_MISMATCH_EMAIL));
}

String validationMessageTypeMismatchForMultipleEmailText()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::VALIDATION_TYPE_MISMATCH_MULTIPLE_EMAIL));
}

String validationMessageTypeMismatchForURLText()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::VALIDATION_TYPE_MISMATCH_URL));
}

String validationMessageValueMissingText()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::VALIDATION_VALUE_MISSING));
}

String validationMessageValueMissingForCheckboxText()
{
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForFileText()
{
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForMultipleFileText()
{
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForRadioText()
{
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForSelectText()
{
    return validationMessageValueMissingText();
}

String validationMessageBadInputForNumberText()
{
    notImplemented();
    return validationMessageTypeMismatchText();
}

String localizedMediaControlElementString(const String&)
{
    notImplemented();
    return String();
}

String localizedMediaControlElementHelpText(const String&)
{
    notImplemented();
    return String();
}

String localizedMediaTimeDescription(const String&)
{
    notImplemented();
    return String();
}

String localizedMediaTimeDescription(float)
{
    notImplemented();
    return String();
}

String mediaElementLoadingStateText()
{
    notImplemented();
    return String();
}

String mediaElementLiveBroadcastStateText()
{
    notImplemented();
    return String();
}

String missingPluginText()
{
    notImplemented();
    return String();
}

String crashedPluginText()
{
    notImplemented();
    return String();
}

String blockedPluginByContentSecurityPolicyText()
{
    notImplemented();
    return String();
}

String insecurePluginVersionText()
{
    notImplemented();
    return String();
}

String inactivePluginText()
{
    notImplemented();
    return String();
}

String multipleFileUploadText(unsigned)
{
    return String(", ...");
}

String defaultDetailsSummaryText()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::DETAILS_SUMMARY));
}

String fileButtonNoFilesSelectedLabel()
{
    return String::fromUTF8(s_resource.getString(BlackBerry::Platform::FILE_BUTTON_NO_FILE_SELECTED_LABEL));
}

String snapshottedPlugInLabelTitle()
{
    notImplemented();
    return String();
}

String snapshottedPlugInLabelSubtitle()
{
    notImplemented();
    return String();
}

String weekFormatInLDML()
{
    notImplemented();
    return String();
}

#if ENABLE(VIDEO_TRACK)
String textTrackClosedCaptionsText()
{
    notImplemented();
    return String();
}

String textTrackSubtitlesText()
{
    notImplemented();
    return String();
}

String textTrackOffText()
{
    notImplemented();
    return String();
}

String textTrackNoLabelText()
{
    notImplemented();
    return String();
}
#endif

} // namespace WebCore
