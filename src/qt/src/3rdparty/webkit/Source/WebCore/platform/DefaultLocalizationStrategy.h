/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef DefaultLocalizationStrategy_h
#define DefaultLocalizationStrategy_h

#if USE(PLATFORM_STRATEGIES)

#include "LocalizationStrategy.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

class DefaultLocalizationStrategy : public LocalizationStrategy {
public:
    DefaultLocalizationStrategy();

private:
    virtual String inputElementAltText();
    virtual String resetButtonDefaultLabel();
    virtual String searchableIndexIntroduction();
    virtual String submitButtonDefaultLabel();
    virtual String fileButtonChooseFileLabel();
    virtual String fileButtonNoFileSelectedLabel();
    virtual String defaultDetailsSummaryText();
#if PLATFORM(MAC)
    virtual String copyImageUnknownFileLabel();
#endif
#if ENABLE(CONTEXT_MENUS)
    virtual String contextMenuItemTagOpenLinkInNewWindow();
    virtual String contextMenuItemTagDownloadLinkToDisk();
    virtual String contextMenuItemTagCopyLinkToClipboard();
    virtual String contextMenuItemTagOpenImageInNewWindow();
    virtual String contextMenuItemTagDownloadImageToDisk();
    virtual String contextMenuItemTagCopyImageToClipboard();
#if PLATFORM(QT)
    virtual String contextMenuItemTagCopyImageUrlToClipboard();
#endif
    virtual String contextMenuItemTagOpenFrameInNewWindow();
    virtual String contextMenuItemTagCopy();
    virtual String contextMenuItemTagGoBack();
    virtual String contextMenuItemTagGoForward();
    virtual String contextMenuItemTagStop();
    virtual String contextMenuItemTagReload();
    virtual String contextMenuItemTagCut();
    virtual String contextMenuItemTagPaste();
#if PLATFORM(GTK)
    virtual String contextMenuItemTagDelete();
    virtual String contextMenuItemTagInputMethods();
    virtual String contextMenuItemTagUnicode();
#endif
#if PLATFORM(GTK) || PLATFORM(QT)
    virtual String contextMenuItemTagSelectAll();
#endif
    virtual String contextMenuItemTagNoGuessesFound();
    virtual String contextMenuItemTagIgnoreSpelling();
    virtual String contextMenuItemTagLearnSpelling();
    virtual String contextMenuItemTagSearchWeb();
    virtual String contextMenuItemTagLookUpInDictionary(const String& selectedString);
    virtual String contextMenuItemTagOpenLink();
    virtual String contextMenuItemTagIgnoreGrammar();
    virtual String contextMenuItemTagSpellingMenu();
    virtual String contextMenuItemTagShowSpellingPanel(bool show);
    virtual String contextMenuItemTagCheckSpelling();
    virtual String contextMenuItemTagCheckSpellingWhileTyping();
    virtual String contextMenuItemTagCheckGrammarWithSpelling();
    virtual String contextMenuItemTagFontMenu();
    virtual String contextMenuItemTagBold();
    virtual String contextMenuItemTagItalic();
    virtual String contextMenuItemTagUnderline();
    virtual String contextMenuItemTagOutline();
    virtual String contextMenuItemTagWritingDirectionMenu();
    virtual String contextMenuItemTagTextDirectionMenu();
    virtual String contextMenuItemTagDefaultDirection();
    virtual String contextMenuItemTagLeftToRight();
    virtual String contextMenuItemTagRightToLeft();
#if PLATFORM(MAC)
    virtual String contextMenuItemTagSearchInSpotlight();
    virtual String contextMenuItemTagShowFonts();
    virtual String contextMenuItemTagStyles();
    virtual String contextMenuItemTagShowColors();
    virtual String contextMenuItemTagSpeechMenu();
    virtual String contextMenuItemTagStartSpeaking();
    virtual String contextMenuItemTagStopSpeaking();
    virtual String contextMenuItemTagCorrectSpellingAutomatically();
    virtual String contextMenuItemTagSubstitutionsMenu();
    virtual String contextMenuItemTagShowSubstitutions(bool show);
    virtual String contextMenuItemTagSmartCopyPaste();
    virtual String contextMenuItemTagSmartQuotes();
    virtual String contextMenuItemTagSmartDashes();
    virtual String contextMenuItemTagSmartLinks();
    virtual String contextMenuItemTagTextReplacement();
    virtual String contextMenuItemTagTransformationsMenu();
    virtual String contextMenuItemTagMakeUpperCase();
    virtual String contextMenuItemTagMakeLowerCase();
    virtual String contextMenuItemTagCapitalize();
    virtual String contextMenuItemTagChangeBack(const String& replacedString);
#endif
    virtual String contextMenuItemTagInspectElement();
    virtual String contextMenuItemTagOpenVideoInNewWindow();
    virtual String contextMenuItemTagOpenAudioInNewWindow();
    virtual String contextMenuItemTagCopyVideoLinkToClipboard();
    virtual String contextMenuItemTagCopyAudioLinkToClipboard();
    virtual String contextMenuItemTagToggleMediaControls();
    virtual String contextMenuItemTagToggleMediaLoop();
    virtual String contextMenuItemTagEnterVideoFullscreen();
    virtual String contextMenuItemTagMediaPlay();
    virtual String contextMenuItemTagMediaPause();
    virtual String contextMenuItemTagMediaMute();
#endif // ENABLE(CONTEXT_MENUS)
    virtual String searchMenuNoRecentSearchesText();
    virtual String searchMenuRecentSearchesText();
    virtual String searchMenuClearRecentSearchesText();
    virtual String AXWebAreaText();
    virtual String AXLinkText();
    virtual String AXListMarkerText();
    virtual String AXImageMapText();
    virtual String AXHeadingText();
    virtual String AXDefinitionListTermText();
    virtual String AXDefinitionListDefinitionText();
#if PLATFORM(MAC)
    virtual String AXARIAContentGroupText(const String& ariaType);
#endif
    virtual String AXButtonActionVerb();
    virtual String AXRadioButtonActionVerb();
    virtual String AXTextFieldActionVerb();
    virtual String AXCheckedCheckBoxActionVerb();
    virtual String AXUncheckedCheckBoxActionVerb();
    virtual String AXMenuListActionVerb();
    virtual String AXMenuListPopupActionVerb();
    virtual String AXLinkActionVerb();
    virtual String missingPluginText();
    virtual String crashedPluginText();
    virtual String multipleFileUploadText(unsigned numberOfFiles);
    virtual String unknownFileSizeText();
#if PLATFORM(WIN)
    virtual String uploadFileText();
    virtual String allFilesText();
#endif
#if PLATFORM(MAC)
    virtual String keygenMenuItem512();
    virtual String keygenMenuItem1024();
    virtual String keygenMenuItem2048();
    virtual String keygenKeychainItemName(const String& host);
#endif
    virtual String imageTitle(const String& filename, const IntSize&);
    virtual String mediaElementLoadingStateText();
    virtual String mediaElementLiveBroadcastStateText();
    virtual String localizedMediaControlElementString(const String&);
    virtual String localizedMediaControlElementHelpText(const String&);
    virtual String localizedMediaTimeDescription(float);
    virtual String validationMessageValueMissingText();
    virtual String validationMessageTypeMismatchText();
    virtual String validationMessagePatternMismatchText();
    virtual String validationMessageTooLongText();
    virtual String validationMessageRangeUnderflowText();
    virtual String validationMessageRangeOverflowText();
    virtual String validationMessageStepMismatchText();
};

} // namespace WebCore

#endif // USE(PLATFORM_STRATEGIES)

#endif // DefaultLocalizationStrategy_h
