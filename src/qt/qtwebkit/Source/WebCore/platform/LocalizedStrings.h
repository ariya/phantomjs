/*
 * Copyright (C) 2003, 2006, 2009, 2011, 2012, 2013 Apple Inc.  All rights reserved.
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

#ifndef LocalizedStrings_h
#define LocalizedStrings_h

#include <wtf/Forward.h>

namespace WebCore {

    class IntSize;
    
    String inputElementAltText();
    String resetButtonDefaultLabel();
    String searchableIndexIntroduction();
    String submitButtonDefaultLabel();
    String fileButtonChooseFileLabel();
    String fileButtonChooseMultipleFilesLabel();
    String fileButtonNoFileSelectedLabel();
    String fileButtonNoFilesSelectedLabel();
    String defaultDetailsSummaryText();

#if PLATFORM(MAC)
    String copyImageUnknownFileLabel();
#endif

#if ENABLE(CONTEXT_MENUS)
    String contextMenuItemTagOpenLinkInNewWindow();
#if PLATFORM(QT)
    String contextMenuItemTagOpenLinkInThisWindow();
#endif
    String contextMenuItemTagDownloadLinkToDisk();
    String contextMenuItemTagCopyLinkToClipboard();
    String contextMenuItemTagOpenImageInNewWindow();
    String contextMenuItemTagDownloadImageToDisk();
    String contextMenuItemTagCopyImageToClipboard();
#if PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
    String contextMenuItemTagCopyImageUrlToClipboard();
#endif
    String contextMenuItemTagOpenFrameInNewWindow();
    String contextMenuItemTagCopy();
    String contextMenuItemTagGoBack();
    String contextMenuItemTagGoForward();
    String contextMenuItemTagStop();
    String contextMenuItemTagReload();
    String contextMenuItemTagCut();
    String contextMenuItemTagPaste();
#if PLATFORM(GTK)
    String contextMenuItemTagDelete();
    String contextMenuItemTagInputMethods();
    String contextMenuItemTagUnicode();
    String contextMenuItemTagUnicodeInsertLRMMark();
    String contextMenuItemTagUnicodeInsertRLMMark();
    String contextMenuItemTagUnicodeInsertLREMark();
    String contextMenuItemTagUnicodeInsertRLEMark();
    String contextMenuItemTagUnicodeInsertLROMark();
    String contextMenuItemTagUnicodeInsertRLOMark();
    String contextMenuItemTagUnicodeInsertPDFMark();
    String contextMenuItemTagUnicodeInsertZWSMark();
    String contextMenuItemTagUnicodeInsertZWJMark();
    String contextMenuItemTagUnicodeInsertZWNJMark();
#endif
#if PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(EFL)
    String contextMenuItemTagSelectAll();
#endif
    String contextMenuItemTagNoGuessesFound();
    String contextMenuItemTagIgnoreSpelling();
    String contextMenuItemTagLearnSpelling();
    String contextMenuItemTagSearchWeb();
    String contextMenuItemTagLookUpInDictionary(const String& selectedString);
    String contextMenuItemTagOpenLink();
    String contextMenuItemTagIgnoreGrammar();
    String contextMenuItemTagSpellingMenu();
    String contextMenuItemTagShowSpellingPanel(bool show);
    String contextMenuItemTagCheckSpelling();
    String contextMenuItemTagCheckSpellingWhileTyping();
    String contextMenuItemTagCheckGrammarWithSpelling();
    String contextMenuItemTagFontMenu();
    String contextMenuItemTagBold();
    String contextMenuItemTagItalic();
    String contextMenuItemTagUnderline();
    String contextMenuItemTagOutline();
    String contextMenuItemTagWritingDirectionMenu();
    String contextMenuItemTagTextDirectionMenu();
    String contextMenuItemTagDefaultDirection();
    String contextMenuItemTagLeftToRight();
    String contextMenuItemTagRightToLeft();
#if PLATFORM(MAC)
    String contextMenuItemTagSearchInSpotlight();
    String contextMenuItemTagShowFonts();
    String contextMenuItemTagStyles();
    String contextMenuItemTagShowColors();
    String contextMenuItemTagSpeechMenu();
    String contextMenuItemTagStartSpeaking();
    String contextMenuItemTagStopSpeaking();
    String contextMenuItemTagCorrectSpellingAutomatically();
    String contextMenuItemTagSubstitutionsMenu();
    String contextMenuItemTagShowSubstitutions(bool show);
    String contextMenuItemTagSmartCopyPaste();
    String contextMenuItemTagSmartQuotes();
    String contextMenuItemTagSmartDashes();
    String contextMenuItemTagSmartLinks();
    String contextMenuItemTagTextReplacement();
    String contextMenuItemTagTransformationsMenu();
    String contextMenuItemTagMakeUpperCase();
    String contextMenuItemTagMakeLowerCase();
    String contextMenuItemTagCapitalize();
    String contextMenuItemTagChangeBack(const String& replacedString);
#endif
    String contextMenuItemTagOpenVideoInNewWindow();
    String contextMenuItemTagOpenAudioInNewWindow();
    String contextMenuItemTagOpenMediaInNewWindow();
    String contextMenuItemTagDownloadVideoToDisk();
    String contextMenuItemTagDownloadAudioToDisk();
    String contextMenuItemTagDownloadMediaToDisk();
    String contextMenuItemTagCopyVideoLinkToClipboard();
    String contextMenuItemTagCopyAudioLinkToClipboard();
    String contextMenuItemTagCopyMediaLinkToClipboard();
    String contextMenuItemTagToggleMediaControls();
    String contextMenuItemTagShowMediaControls();
    String contextMenuItemTagHideMediaControls();
    String contextMenuItemTagToggleMediaLoop();
    String contextMenuItemTagEnterVideoFullscreen();
    String contextMenuItemTagExitVideoFullscreen();
    String contextMenuItemTagToggleVideoFullscreen();
    String contextMenuItemTagMediaPlayPause();
    String contextMenuItemTagMediaPlay();
    String contextMenuItemTagMediaPause();
    String contextMenuItemTagMediaMute();
    String contextMenuItemTagMediaUnmute();
    String contextMenuItemTagInspectElement();
#endif // ENABLE(CONTEXT_MENUS)

    String searchMenuNoRecentSearchesText();
    String searchMenuRecentSearchesText();
    String searchMenuClearRecentSearchesText();

    String AXWebAreaText();
    String AXLinkText();
    String AXListMarkerText();
    String AXImageMapText();
    String AXHeadingText();
    String AXDefinitionText();
    String AXDescriptionListText();
    String AXDescriptionListTermText();
    String AXDescriptionListDetailText();
    String AXFooterRoleDescriptionText();
    String AXFileUploadButtonText();
    
    String AXButtonActionVerb();
    String AXRadioButtonActionVerb();
    String AXTextFieldActionVerb();
    String AXCheckedCheckBoxActionVerb();
    String AXUncheckedCheckBoxActionVerb();
    String AXMenuListActionVerb();
    String AXMenuListPopupActionVerb();
    String AXLinkActionVerb();
    String AXListItemActionVerb();

#if ENABLE(INPUT_TYPE_WEEK)
    // weekFormatInLDML() returns week and year format in LDML, Unicode
    // technical standard 35, Locale Data Markup Language, e.g. "'Week' ww, yyyy"
    String weekFormatInLDML();
#endif
#if PLATFORM(MAC) || PLATFORM(IOS)
    String AXARIAContentGroupText(const String& ariaType);
#endif

    String missingPluginText();
    String crashedPluginText();
    String blockedPluginByContentSecurityPolicyText();
    String insecurePluginVersionText();

    String multipleFileUploadText(unsigned numberOfFiles);
    String unknownFileSizeText();

#if PLATFORM(WIN)
    String uploadFileText();
    String allFilesText();
#endif

#if PLATFORM(MAC)
    String builtInPDFPluginName();
    String pdfDocumentTypeDescription();
    String postScriptDocumentTypeDescription();
    String keygenMenuItem512();
    String keygenMenuItem1024();
    String keygenMenuItem2048();
    String keygenKeychainItemName(const String& host);
#endif

#if PLATFORM(IOS)
    String htmlSelectMultipleItems(size_t num);
#endif

    String imageTitle(const String& filename, const IntSize& size);

    String mediaElementLoadingStateText();
    String mediaElementLiveBroadcastStateText();
    String localizedMediaControlElementString(const String&);
    String localizedMediaControlElementHelpText(const String&);
    String localizedMediaTimeDescription(float);

    String validationMessageValueMissingText();
    String validationMessageValueMissingForCheckboxText();
    String validationMessageValueMissingForFileText();
    String validationMessageValueMissingForMultipleFileText();
    String validationMessageValueMissingForRadioText();
    String validationMessageValueMissingForSelectText();
    String validationMessageTypeMismatchText();
    String validationMessageTypeMismatchForEmailText();
    String validationMessageTypeMismatchForMultipleEmailText();
    String validationMessageTypeMismatchForURLText();
    String validationMessagePatternMismatchText();
    String validationMessageTooLongText(int valueLength, int maxLength);
    String validationMessageRangeUnderflowText(const String& minimum);
    String validationMessageRangeOverflowText(const String& maximum);
    String validationMessageStepMismatchText(const String& base, const String& step);
    String validationMessageBadInputForNumberText();
#if USE(SOUP)
    String unacceptableTLSCertificate();
#endif

    String clickToExitFullScreenText();

#if ENABLE(VIDEO_TRACK)
    String textTrackSubtitlesText();
    String textTrackOffMenuItemText();
    String textTrackAutomaticMenuItemText();
    String textTrackNoLabelText();
#if PLATFORM(MAC) || PLATFORM(WIN)
    String textTrackCountryAndLanguageMenuItemText(const String& title, const String& country, const String& language);
    String textTrackLanguageMenuItemText(const String& title, const String& language);
    String closedCaptionTrackMenuItemText(const String&);
    String sdhTrackMenuItemText(const String&);
    String easyReaderTrackMenuItemText(const String&);
#endif
#endif

    String snapshottedPlugInLabelTitle();
    String snapshottedPlugInLabelSubtitle();

#define WEB_UI_STRING(string, description) WebCore::localizedString(string)
#define WEB_UI_STRING_KEY(string, key, description) WebCore::localizedString(key)

    String localizedString(const char* key);

} // namespace WebCore

#endif // LocalizedStrings_h
