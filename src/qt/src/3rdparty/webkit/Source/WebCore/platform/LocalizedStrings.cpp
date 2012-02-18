/*
 * Copyright (C) 2003, 2006, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Igalia S.L
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

#include "config.h"
#include "LocalizedStrings.h"

#include "IntSize.h"
#include "LocalizationStrategy.h"
#include "PlatformStrategies.h"
#include "PlatformString.h"

namespace WebCore {

#if USE(PLATFORM_STRATEGIES)

String inputElementAltText()
{
    return platformStrategies()->localizationStrategy()->inputElementAltText();
}

String resetButtonDefaultLabel()
{
    return platformStrategies()->localizationStrategy()->resetButtonDefaultLabel();
}

String searchableIndexIntroduction()
{
    return platformStrategies()->localizationStrategy()->searchableIndexIntroduction();
}

String submitButtonDefaultLabel()
{
    return platformStrategies()->localizationStrategy()->submitButtonDefaultLabel();
}

String fileButtonChooseFileLabel()
{
    return platformStrategies()->localizationStrategy()->fileButtonChooseFileLabel();
}

String fileButtonNoFileSelectedLabel()
{
    return platformStrategies()->localizationStrategy()->fileButtonNoFileSelectedLabel();
}

String defaultDetailsSummaryText()
{
    return platformStrategies()->localizationStrategy()->defaultDetailsSummaryText();
}

#if PLATFORM(MAC)
String copyImageUnknownFileLabel()
{
    return platformStrategies()->localizationStrategy()->copyImageUnknownFileLabel();
}
#endif

#if ENABLE(CONTEXT_MENUS)
String contextMenuItemTagOpenLinkInNewWindow()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagOpenLinkInNewWindow();
}

String contextMenuItemTagDownloadLinkToDisk()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagDownloadLinkToDisk();
}

String contextMenuItemTagCopyLinkToClipboard()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagCopyLinkToClipboard();
}

String contextMenuItemTagOpenImageInNewWindow()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagOpenImageInNewWindow();
}

String contextMenuItemTagDownloadImageToDisk()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagDownloadImageToDisk();
}

String contextMenuItemTagCopyImageToClipboard()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagCopyImageToClipboard();
}

#if PLATFORM(QT) || PLATFORM(GTK)
String contextMenuItemTagCopyImageUrlToClipboard()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagCopyImageUrlToClipboard();
}
#endif

String contextMenuItemTagOpenFrameInNewWindow()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagOpenFrameInNewWindow();
}

String contextMenuItemTagCopy()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagCopy();
}

String contextMenuItemTagGoBack()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagGoBack();
}

String contextMenuItemTagGoForward()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagGoForward();
}

String contextMenuItemTagStop()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagStop();
}

String contextMenuItemTagReload()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagReload();
}

String contextMenuItemTagCut()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagCut();
}

String contextMenuItemTagPaste()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagPaste();
}

#if PLATFORM(QT)
String contextMenuItemTagSelectAll()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagSelectAll();
}
#endif

String contextMenuItemTagNoGuessesFound()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagNoGuessesFound();
}

String contextMenuItemTagIgnoreSpelling()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagIgnoreSpelling();
}

String contextMenuItemTagLearnSpelling()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagLearnSpelling();
}

#if PLATFORM(MAC)
String contextMenuItemTagSearchInSpotlight()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagSearchInSpotlight();
}
#endif

String contextMenuItemTagSearchWeb()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagSearchWeb();
}

String contextMenuItemTagLookUpInDictionary(const String& selectedString)
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagLookUpInDictionary(selectedString);
}

String contextMenuItemTagOpenLink()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagOpenLink();
}

String contextMenuItemTagIgnoreGrammar()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagIgnoreGrammar();
}

String contextMenuItemTagSpellingMenu()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagSpellingMenu();
}

String contextMenuItemTagShowSpellingPanel(bool show)
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagShowSpellingPanel(show);
}

String contextMenuItemTagCheckSpelling()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagCheckSpelling();
}

String contextMenuItemTagCheckSpellingWhileTyping()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagCheckSpellingWhileTyping();
}

String contextMenuItemTagCheckGrammarWithSpelling()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagCheckGrammarWithSpelling();
}

String contextMenuItemTagFontMenu()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagFontMenu();
}

#if PLATFORM(MAC)
String contextMenuItemTagShowFonts()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagShowFonts();
}
#endif

String contextMenuItemTagBold()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagBold();
}

String contextMenuItemTagItalic()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagItalic();
}

String contextMenuItemTagUnderline()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagUnderline();
}

String contextMenuItemTagOutline()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagOutline();
}

#if PLATFORM(MAC)
String contextMenuItemTagStyles()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagStyles();
}

String contextMenuItemTagShowColors()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagShowColors();
}

String contextMenuItemTagSpeechMenu()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagSpeechMenu();
}

String contextMenuItemTagStartSpeaking()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagStartSpeaking();
}

String contextMenuItemTagStopSpeaking()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagStopSpeaking();
}
#endif

String contextMenuItemTagWritingDirectionMenu()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagWritingDirectionMenu();
}

String contextMenuItemTagTextDirectionMenu()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagTextDirectionMenu();
}

String contextMenuItemTagDefaultDirection()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagDefaultDirection();
}

String contextMenuItemTagLeftToRight()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagLeftToRight();
}

String contextMenuItemTagRightToLeft()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagRightToLeft();
}

#if PLATFORM(MAC)

String contextMenuItemTagCorrectSpellingAutomatically()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagCorrectSpellingAutomatically();
}

String contextMenuItemTagSubstitutionsMenu()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagSubstitutionsMenu();
}

String contextMenuItemTagShowSubstitutions(bool show)
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagShowSubstitutions(show);
}

String contextMenuItemTagSmartCopyPaste()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagSmartCopyPaste();
}

String contextMenuItemTagSmartQuotes()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagSmartQuotes();
}

String contextMenuItemTagSmartDashes()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagSmartDashes();
}

String contextMenuItemTagSmartLinks()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagSmartLinks();
}

String contextMenuItemTagTextReplacement()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagTextReplacement();
}

String contextMenuItemTagTransformationsMenu()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagTransformationsMenu();
}

String contextMenuItemTagMakeUpperCase()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagMakeUpperCase();
}

String contextMenuItemTagMakeLowerCase()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagMakeLowerCase();
}

String contextMenuItemTagCapitalize()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagCapitalize();
}

String contextMenuItemTagChangeBack(const String& replacedString)
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagChangeBack(replacedString);
}

#endif // PLATFORM(MAC)

String contextMenuItemTagOpenVideoInNewWindow()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagOpenVideoInNewWindow();
}

String contextMenuItemTagOpenAudioInNewWindow()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagOpenAudioInNewWindow();
}

String contextMenuItemTagCopyVideoLinkToClipboard()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagCopyVideoLinkToClipboard();
}

String contextMenuItemTagCopyAudioLinkToClipboard()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagCopyAudioLinkToClipboard();
}

String contextMenuItemTagToggleMediaControls()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagToggleMediaControls();
}

String contextMenuItemTagToggleMediaLoop()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagToggleMediaLoop();
}

String contextMenuItemTagEnterVideoFullscreen()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagEnterVideoFullscreen();
}

String contextMenuItemTagMediaPlay()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagMediaPlay();
}

String contextMenuItemTagMediaPause()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagMediaPause();
}

String contextMenuItemTagMediaMute()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagMediaMute();
}
    
String contextMenuItemTagInspectElement()
{
    return platformStrategies()->localizationStrategy()->contextMenuItemTagInspectElement();
}

#endif // ENABLE(CONTEXT_MENUS)

String searchMenuNoRecentSearchesText()
{
    return platformStrategies()->localizationStrategy()->searchMenuNoRecentSearchesText();
}

String searchMenuRecentSearchesText ()
{
    return platformStrategies()->localizationStrategy()->searchMenuRecentSearchesText ();
}

String searchMenuClearRecentSearchesText()
{
    return platformStrategies()->localizationStrategy()->searchMenuClearRecentSearchesText();
}

String AXWebAreaText()
{
    return platformStrategies()->localizationStrategy()->AXWebAreaText();
}

String AXLinkText()
{
    return platformStrategies()->localizationStrategy()->AXLinkText();
}

String AXListMarkerText()
{
    return platformStrategies()->localizationStrategy()->AXListMarkerText();
}

String AXImageMapText()
{
    return platformStrategies()->localizationStrategy()->AXImageMapText();
}

String AXHeadingText()
{
    return platformStrategies()->localizationStrategy()->AXHeadingText();
}

String AXDefinitionListTermText()
{
    return platformStrategies()->localizationStrategy()->AXDefinitionListTermText();
}

String AXDefinitionListDefinitionText()
{
    return platformStrategies()->localizationStrategy()->AXDefinitionListDefinitionText();
}

#if PLATFORM(MAC)
String AXARIAContentGroupText(const String& ariaType)
{
    return platformStrategies()->localizationStrategy()->AXARIAContentGroupText(ariaType);
}
#endif
    
String AXButtonActionVerb()
{
    return platformStrategies()->localizationStrategy()->AXButtonActionVerb();
}

String AXRadioButtonActionVerb()
{
    return platformStrategies()->localizationStrategy()->AXRadioButtonActionVerb();
}

String AXTextFieldActionVerb()
{
    return platformStrategies()->localizationStrategy()->AXTextFieldActionVerb();
}

String AXCheckedCheckBoxActionVerb()
{
    return platformStrategies()->localizationStrategy()->AXCheckedCheckBoxActionVerb();
}

String AXUncheckedCheckBoxActionVerb()
{
    return platformStrategies()->localizationStrategy()->AXUncheckedCheckBoxActionVerb();
}

String AXLinkActionVerb()
{
    return platformStrategies()->localizationStrategy()->AXLinkActionVerb();
}

String AXMenuListPopupActionVerb()
{
    return platformStrategies()->localizationStrategy()->AXMenuListPopupActionVerb();
}

String AXMenuListActionVerb()
{
    return platformStrategies()->localizationStrategy()->AXMenuListActionVerb();
}

String missingPluginText()
{
    return platformStrategies()->localizationStrategy()->missingPluginText();
}

String crashedPluginText()
{
    return platformStrategies()->localizationStrategy()->crashedPluginText();
}

String multipleFileUploadText(unsigned numberOfFiles)
{
    return platformStrategies()->localizationStrategy()->multipleFileUploadText(numberOfFiles);
}

String unknownFileSizeText()
{
    return platformStrategies()->localizationStrategy()->unknownFileSizeText();
}

#if PLATFORM(WIN)
String uploadFileText()
{
    return platformStrategies()->localizationStrategy()->uploadFileText();
}

String allFilesText()
{
    return platformStrategies()->localizationStrategy()->allFilesText();
}
#endif

#if PLATFORM(MAC)
String keygenMenuItem512()
{
    return platformStrategies()->localizationStrategy()->keygenMenuItem512();
}

String keygenMenuItem1024()
{
    return platformStrategies()->localizationStrategy()->keygenMenuItem1024();
}

String keygenMenuItem2048()
{
    return platformStrategies()->localizationStrategy()->keygenMenuItem2048();
}

String keygenKeychainItemName(const String& host)
{
    return platformStrategies()->localizationStrategy()->keygenKeychainItemName(host);
}

#endif

String imageTitle(const String& filename, const IntSize& size)
{
    return platformStrategies()->localizationStrategy()->imageTitle(filename, size);
}

String mediaElementLoadingStateText()
{
    return platformStrategies()->localizationStrategy()->mediaElementLoadingStateText();
}

String mediaElementLiveBroadcastStateText()
{
    return platformStrategies()->localizationStrategy()->mediaElementLiveBroadcastStateText();
}

String localizedMediaControlElementString(const String& controlName)
{
    return platformStrategies()->localizationStrategy()->localizedMediaControlElementString(controlName);
}

String localizedMediaControlElementHelpText(const String& controlName)
{
    return platformStrategies()->localizationStrategy()->localizedMediaControlElementHelpText(controlName);
}

String localizedMediaTimeDescription(float time)
{
    return platformStrategies()->localizationStrategy()->localizedMediaTimeDescription(time);
}

String validationMessageValueMissingText()
{
    return platformStrategies()->localizationStrategy()->validationMessageValueMissingText();
}

String validationMessageValueMissingForCheckboxText()
{
    return platformStrategies()->localizationStrategy()->validationMessageValueMissingText();
}

String validationMessageValueMissingForFileText()
{
    return platformStrategies()->localizationStrategy()->validationMessageValueMissingText();
}

String validationMessageValueMissingForMultipleFileText()
{
    return platformStrategies()->localizationStrategy()->validationMessageValueMissingText();
}

String validationMessageValueMissingForRadioText()
{
    return platformStrategies()->localizationStrategy()->validationMessageValueMissingText();
}

String validationMessageValueMissingForSelectText()
{
    return platformStrategies()->localizationStrategy()->validationMessageValueMissingText();
}

String validationMessageTypeMismatchText()
{
    return platformStrategies()->localizationStrategy()->validationMessageTypeMismatchText();
}

String validationMessageTypeMismatchForEmailText()
{
    return platformStrategies()->localizationStrategy()->validationMessageTypeMismatchText();
}

String validationMessageTypeMismatchForMultipleEmailText()
{
    return platformStrategies()->localizationStrategy()->validationMessageTypeMismatchText();
}

String validationMessageTypeMismatchForURLText()
{
    return platformStrategies()->localizationStrategy()->validationMessageTypeMismatchText();
}

String validationMessagePatternMismatchText()
{
    return platformStrategies()->localizationStrategy()->validationMessagePatternMismatchText();
}

String validationMessageTooLongText(int, int)
{
    return platformStrategies()->localizationStrategy()->validationMessageTooLongText();
}

String validationMessageRangeUnderflowText(const String&)
{
    return platformStrategies()->localizationStrategy()->validationMessageRangeUnderflowText();
}

String validationMessageRangeOverflowText(const String&)
{
    return platformStrategies()->localizationStrategy()->validationMessageRangeOverflowText();
}

String validationMessageStepMismatchText(const String&, const String&)
{
    return platformStrategies()->localizationStrategy()->validationMessageStepMismatchText();
}

#endif // USE(PLATFORM_STRATEGIES)

#if !PLATFORM(MAC) && !PLATFORM(WIN)
String localizedString(const char* key)
{
    return String::fromUTF8(key, strlen(key));
}
#endif

} // namespace WebCore
