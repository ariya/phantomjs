/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Christian Dywan <christian@imendio.com>
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2008 INdT Instituto Nokia de Tecnologia
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2010 Samsung Electronics
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

#include "NotImplemented.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

String submitButtonDefaultLabel()
{
    return String::fromUTF8("Submit");
}

String inputElementAltText()
{
    return String::fromUTF8("Submit");
}

String resetButtonDefaultLabel()
{
    return String::fromUTF8("Reset");
}

String defaultDetailsSummaryText()
{
    return String::fromUTF8("Details");
}

String searchableIndexIntroduction()
{
    return String::fromUTF8("This is a searchable index. Enter search keywords: ");
}

String fileButtonChooseFileLabel()
{
    return String::fromUTF8("Choose File");
}

String fileButtonChooseMultipleFilesLabel()
{
    return String::fromUTF8("Choose Files");
}

String fileButtonNoFileSelectedLabel()
{
    return String::fromUTF8("No file selected");
}

String fileButtonNoFilesSelectedLabel()
{
    return String::fromUTF8("No files selected");
}

String contextMenuItemTagOpenLinkInNewWindow()
{
    return String::fromUTF8("Open Link in New Window");
}

String contextMenuItemTagDownloadLinkToDisk()
{
    return String::fromUTF8("Download Linked File");
}

String contextMenuItemTagCopyLinkToClipboard()
{
    return String::fromUTF8("Copy Link Location");
}

String contextMenuItemTagOpenImageInNewWindow()
{
    return String::fromUTF8("Open Image in New Window");
}

String contextMenuItemTagDownloadImageToDisk()
{
    return String::fromUTF8("Save Image As");
}

String contextMenuItemTagCopyImageToClipboard()
{
    return String::fromUTF8("Copy Image");
}

String contextMenuItemTagCopyImageUrlToClipboard()
{
    return String::fromUTF8("Copy Image Address");
}

String contextMenuItemTagOpenVideoInNewWindow()
{
    return String::fromUTF8("Open Video in New Window");
}

String contextMenuItemTagOpenAudioInNewWindow()
{
    return String::fromUTF8("Open Audio in New Window");
}

String contextMenuItemTagDownloadVideoToDisk()
{
    return String::fromUTF8("Download Video");
}

String contextMenuItemTagDownloadAudioToDisk()
{
    return String::fromUTF8("Download Audio");
}

String contextMenuItemTagCopyVideoLinkToClipboard()
{
    return String::fromUTF8("Copy Video Link Location");
}

String contextMenuItemTagCopyAudioLinkToClipboard()
{
    return String::fromUTF8("Copy Audio Link Location");
}

String contextMenuItemTagToggleMediaControls()
{
    return String::fromUTF8("Toggle Media Controls");
}

String contextMenuItemTagShowMediaControls()
{
    return String::fromUTF8("Show Media Controls");
}

String contextMenuitemTagHideMediaControls()
{
    return String::fromUTF8("Hide Media Controls");
}

String contextMenuItemTagToggleMediaLoop()
{
    return String::fromUTF8("Toggle Media Loop Playback");
}

String contextMenuItemTagEnterVideoFullscreen()
{
    return String::fromUTF8("Switch Video to Fullscreen");
}

String contextMenuItemTagMediaPlay()
{
    return String::fromUTF8("Play");
}

String contextMenuItemTagMediaPause()
{
    return String::fromUTF8("Pause");
}

String contextMenuItemTagMediaMute()
{
    return String::fromUTF8("Mute");
}

String contextMenuItemTagOpenFrameInNewWindow()
{
    return String::fromUTF8("Open Frame in New Window");
}

String contextMenuItemTagCopy()
{
    return String::fromUTF8("Copy");
}

String contextMenuItemTagDelete()
{
    return String::fromUTF8("Delete");
}

String contextMenuItemTagSelectAll()
{
    return String::fromUTF8("Select All");
}

String contextMenuItemTagUnicode()
{
    return String::fromUTF8("Insert Unicode Control Character");
}

String contextMenuItemTagInputMethods()
{
    return String::fromUTF8("Input Methods");
}

String contextMenuItemTagGoBack()
{
    return String::fromUTF8("Go Back");
}

String contextMenuItemTagGoForward()
{
    return String::fromUTF8("Go Forward");
}

String contextMenuItemTagStop()
{
    return String::fromUTF8("Stop");
}

String contextMenuItemTagReload()
{
    return String::fromUTF8("Reload");
}

String contextMenuItemTagCut()
{
    return String::fromUTF8("Cut");
}

String contextMenuItemTagPaste()
{
    return String::fromUTF8("Paste");
}

String contextMenuItemTagNoGuessesFound()
{
    return String::fromUTF8("No Guesses Found");
}

String contextMenuItemTagIgnoreSpelling()
{
    return String::fromUTF8("Ignore Spelling");
}

String contextMenuItemTagLearnSpelling()
{
    return String::fromUTF8("Learn Spelling");
}

String contextMenuItemTagSearchWeb()
{
    return String::fromUTF8("Search the Web");
}

String contextMenuItemTagLookUpInDictionary(const String&)
{
    return String::fromUTF8("Look Up in Dictionary");
}

String contextMenuItemTagOpenLink()
{
    return String::fromUTF8("Open Link");
}

String contextMenuItemTagIgnoreGrammar()
{
    return String::fromUTF8("Ignore Grammar");
}

String contextMenuItemTagSpellingMenu()
{
    return String::fromUTF8("Spelling and Grammar");
}

String contextMenuItemTagShowSpellingPanel(bool show)
{
    return String::fromUTF8(show ? "Show Spelling and Grammar" : "Hide Spelling and Grammar");
}

String contextMenuItemTagCheckSpelling()
{
    return String::fromUTF8("Check Document Now");
}

String contextMenuItemTagCheckSpellingWhileTyping()
{
    return String::fromUTF8("Check Spelling While Typing");
}

String contextMenuItemTagCheckGrammarWithSpelling()
{
    return String::fromUTF8("Check Grammar With Spelling");
}

String contextMenuItemTagFontMenu()
{
    return String::fromUTF8("Font");
}

String contextMenuItemTagBold()
{
    return String::fromUTF8("Bold");
}

String contextMenuItemTagItalic()
{
    return String::fromUTF8("Italic");
}

String contextMenuItemTagUnderline()
{
    return String::fromUTF8("Underline");
}

String contextMenuItemTagOutline()
{
    return String::fromUTF8("Outline");
}

String contextMenuItemTagInspectElement()
{
    return String::fromUTF8("Inspect Element");
}

String contextMenuItemTagRightToLeft()
{
    return String::fromUTF8("Right to Left");
}

String contextMenuItemTagLeftToRight()
{
    return String::fromUTF8("Left to Right");
}

String contextMenuItemTagWritingDirectionMenu()
{
    return String::fromUTF8("Writing Direction");
}

String contextMenuItemTagTextDirectionMenu()
{
    return String::fromUTF8("Text Direction");
}

String contextMenuItemTagDefaultDirection()
{
    return String::fromUTF8("Default");
}

String searchMenuNoRecentSearchesText()
{
    return String::fromUTF8("No recent searches");
}

String searchMenuRecentSearchesText()
{
    return String::fromUTF8("Recent searches");
}

String searchMenuClearRecentSearchesText()
{
    return String::fromUTF8("Clear recent searches");
}

String AXDefinitionText()
{
    return String::fromUTF8("definition");
}

String AXDescriptionListText()
{
    return String::fromUTF8("description list");
}

String AXDescriptionListTermText()
{
    return String::fromUTF8("term");
}

String AXDescriptionListDetailText()
{
    return String::fromUTF8("description");
}

String AXFooterRoleDescriptionText()
{
    return String::fromUTF8("footer");
}

String AXButtonActionVerb()
{
    return String::fromUTF8("press");
}

String AXRadioButtonActionVerb()
{
    return String::fromUTF8("select");
}

String AXTextFieldActionVerb()
{
    return String::fromUTF8("activate");
}

String AXCheckedCheckBoxActionVerb()
{
    return String::fromUTF8("uncheck");
}

String AXUncheckedCheckBoxActionVerb()
{
    return String::fromUTF8("check");
}

String AXLinkActionVerb()
{
    return String::fromUTF8("jump");
}

String unknownFileSizeText()
{
    return String::fromUTF8("Unknown");
}

String imageTitle(const String&, const IntSize&)
{
    notImplemented();
    return String();
}

String AXListItemActionVerb()
{
    notImplemented();
    return String();
}

#if ENABLE(VIDEO)
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

String localizedMediaTimeDescription(float)
{
    notImplemented();
    return String();
}
#endif

String mediaElementLoadingStateText()
{
    return String::fromUTF8("Loading...");
}

String mediaElementLiveBroadcastStateText()
{
    return String::fromUTF8("Live Broadcast");
}

String validationMessagePatternMismatchText()
{
    return String::fromUTF8("pattern mismatch");
}

String validationMessageRangeOverflowText(const String& maximum)
{
    return ASCIILiteral("Value must be less than or equal to ") + maximum;
}

String validationMessageRangeUnderflowText(const String& minimum)
{
    return ASCIILiteral("Value must be greater than or equal to ") + minimum;
}

String validationMessageStepMismatchText(const String&, const String&)
{
    return String::fromUTF8("step mismatch");
}

String validationMessageTooLongText(int, int)
{
    return String::fromUTF8("too long");
}

String validationMessageTypeMismatchText()
{
    return String::fromUTF8("type mismatch");
}

String validationMessageTypeMismatchForEmailText()
{
    return ASCIILiteral("Please enter an email address");
}

String validationMessageTypeMismatchForMultipleEmailText()
{
    return ASCIILiteral("Please enter an email address");
}

String validationMessageTypeMismatchForURLText()
{
    return ASCIILiteral("Please enter a URL");
}

String validationMessageValueMissingText()
{
    return String::fromUTF8("value missing");
}

String validationMessageValueMissingForCheckboxText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForFileText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForMultipleFileText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForRadioText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForSelectText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageBadInputForNumberText()
{
    notImplemented();
    return validationMessageTypeMismatchText();
}

String missingPluginText()
{
    return String::fromUTF8("missing plugin");
}

String AXMenuListPopupActionVerb()
{
    return String();
}

String AXMenuListActionVerb()
{
    return String();
}

String multipleFileUploadText(unsigned numberOfFiles)
{
    return String::number(numberOfFiles) + String::fromUTF8(" files");
}

String crashedPluginText()
{
    return String::fromUTF8("plugin crashed");
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

String unacceptableTLSCertificate()
{
    return String::fromUTF8("Unacceptable TLS certificate");
}

String localizedString(const char* key)
{
    return String::fromUTF8(key, strlen(key));
}

#if ENABLE(VIDEO_TRACK)
String textTrackClosedCaptionsText()
{
    return String::fromUTF8("Closed Captions");
}

String textTrackSubtitlesText()
{
    return String::fromUTF8("Subtitles");
}

String textTrackOffText()
{
    return String::fromUTF8("Off");
}

String textTrackNoLabelText()
{
    return String::fromUTF8("No label");
}
#endif

String snapshottedPlugInLabelTitle()
{
    return String("Snapshotted Plug-In");
}

String snapshottedPlugInLabelSubtitle()
{
    return String("Click to restart");
}


}
