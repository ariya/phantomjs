/*
 * Copyright (C) 2007 Staikos Computing Services Inc. <info@staikos.net>
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * Copyright (C) 2010, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2010 INdT - Instituto Nokia de Tecnologia
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

#include "config.h"
#include "LocalizedStrings.h"

#include "IntSize.h"
#include "NotImplemented.h"
#include <QCoreApplication>
#include <wtf/MathExtras.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

String inputElementAltText()
{
    return QCoreApplication::translate("QWebPage", "Submit", "Submit (input element) alt text for <input> elements with no alt, title, or value");
}

String resetButtonDefaultLabel()
{
    return QCoreApplication::translate("QWebPage", "Reset", "default label for Reset buttons in forms on web pages");
}

String searchableIndexIntroduction()
{
    return QCoreApplication::translate("QWebPage", "This is a searchable index. Enter search keywords: ", "text that appears at the start of nearly-obsolete web pages in the form of a 'searchable index'");
}

String submitButtonDefaultLabel()
{
    return QCoreApplication::translate("QWebPage", "Submit", "default label for Submit buttons in forms on web pages");
}

String fileButtonChooseFileLabel()
{
    return QCoreApplication::translate("QWebPage", "Choose File", "title for a single file chooser button used in HTML forms");
}

String fileButtonChooseMultipleFilesLabel()
{
    return QCoreApplication::translate("QWebPage", "Choose Files", "title for a multiple file chooser button used in HTML forms. This title should be as short as possible.");
}

String fileButtonNoFileSelectedLabel()
{
    return QCoreApplication::translate("QWebPage", "No file selected", "text to display in file button used in HTML forms when no file is selected");
}

String fileButtonNoFilesSelectedLabel()
{
    return QCoreApplication::translate("QWebPage", "No files selected", "text to display in file button used in HTML forms when no files are selected and the button allows multiple files to be selected");
}

String defaultDetailsSummaryText()
{
    return QCoreApplication::translate("QWebPage", "Details", "text to display in <details> tag when it has no <summary> child");
}

String contextMenuItemTagOpenLinkInNewWindow()
{
    return QCoreApplication::translate("QWebPage", "Open in New Window", "Open in New Window context menu item");
}

String contextMenuItemTagOpenLinkInThisWindow()
{
    return QCoreApplication::translate("QWebPage", "Open in This Window", "Open in This Window context menu item");
}

String contextMenuItemTagDownloadLinkToDisk()
{
    return QCoreApplication::translate("QWebPage", "Save Link...", "Download Linked File context menu item");
}

String contextMenuItemTagCopyLinkToClipboard()
{
    return QCoreApplication::translate("QWebPage", "Copy Link", "Copy Link context menu item");
}

String contextMenuItemTagOpenImageInNewWindow()
{
    return QCoreApplication::translate("QWebPage", "Open Image", "Open Image in New Window context menu item");
}

String contextMenuItemTagDownloadImageToDisk()
{
    return QCoreApplication::translate("QWebPage", "Save Image", "Download Image context menu item");
}

String contextMenuItemTagCopyImageToClipboard()
{
    return QCoreApplication::translate("QWebPage", "Copy Image", "Copy Link context menu item");
}

String contextMenuItemTagCopyImageUrlToClipboard()
{
    return QCoreApplication::translate("QWebPage", "Copy Image Address", "Copy Image Address menu item");
}

String contextMenuItemTagOpenVideoInNewWindow()
{
    return QCoreApplication::translate("QWebPage", "Open Video", "Open Video in New Window");
}

String contextMenuItemTagOpenAudioInNewWindow()
{
    return QCoreApplication::translate("QWebPage", "Open Audio", "Open Audio in New Window");
}

String contextMenuItemTagOpenMediaInNewWindow()
{
    return QCoreApplication::translate("QWebPage", "Open Media", "Open Media in New Window context menu item");
}

String contextMenuItemTagDownloadVideoToDisk()
{
    return QCoreApplication::translate("QWebPage", "Download Video", "Download Video context menu item");
}

String contextMenuItemTagDownloadAudioToDisk()
{
    return QCoreApplication::translate("QWebPage", "Download Audio", "Download Audio context menu item");
}

String contextMenuItemTagDownloadMediaToDisk()
{
    return QCoreApplication::translate("QWebPage", "Download Media", "Download Media context menu item");
}

String contextMenuItemTagCopyVideoLinkToClipboard()
{
    return QCoreApplication::translate("QWebPage", "Copy Video Address", "Copy Video Link to Clipboard");
}

String contextMenuItemTagCopyAudioLinkToClipboard()
{
    return QCoreApplication::translate("QWebPage", "Copy Audio Address", "Copy Audio Link to Clipboard");
}

String contextMenuItemTagCopyMediaLinkToClipboard()
{
    return QCoreApplication::translate("QWebPage", "Copy Media Address", "Copy Media Link to Clipboard");
}

String contextMenuItemTagToggleMediaControls()
{
    return QCoreApplication::translate("QWebPage", "Show Controls", "Toggle Media Controls checkable context menu item");
}

String contextMenuItemTagShowMediaControls()
{
    return QCoreApplication::translate("QWebPage", "Show Controls", "Show Media Controls");
}

String contextMenuItemTagHideMediaControls()
{
    return QCoreApplication::translate("QWebPage", "Hide Controls", "Hide Media Controls");
}

String contextMenuItemTagToggleMediaLoop()
{
    return QCoreApplication::translate("QWebPage", "Looping", "Toggle Media Loop Playback");
}

String contextMenuItemTagToggleVideoFullscreen()
{
    return QCoreApplication::translate("QWebPage", "Toggle Fullscreen", "Toggle Fullscreen Mode of Video context menu item");
}

String contextMenuItemTagEnterVideoFullscreen()
{
    return QCoreApplication::translate("QWebPage", "Enter Fullscreen", "Switch Video to Fullscreen context menu item");
}

String contextMenuItemTagExitVideoFullscreen()
{
    return QCoreApplication::translate("QWebPage", "Exit Fullscreen", "Switch Video out of Fullscreen context menu item");
}

String contextMenuItemTagMediaPlay()
{
    return QCoreApplication::translate("QWebPage", "Play", "Play");
}

String contextMenuItemTagMediaPause()
{
    return QCoreApplication::translate("QWebPage", "Pause", "Pause");
}

String contextMenuItemTagMediaPlayPause()
{
    return QCoreApplication::translate("QWebPage", "Play/Pause", "Toggle Play and Pause state");
}

String contextMenuItemTagMediaMute()
{
    return QCoreApplication::translate("QWebPage", "Mute", "Media Mute context menu item");
}

String contextMenuItemTagMediaUnmute()
{
    return QCoreApplication::translate("QWebPage", "Unmute", "Media Unmute context menu item");
}

String contextMenuItemTagOpenFrameInNewWindow()
{
    return QCoreApplication::translate("QWebPage", "Open Frame", "Open Frame in New Window context menu item");
}

String contextMenuItemTagCopy()
{
    return QCoreApplication::translate("QWebPage", "Copy", "Copy context menu item");
}

String contextMenuItemTagGoBack()
{
    return QCoreApplication::translate("QWebPage", "Go Back", "Back context menu item");
}

String contextMenuItemTagGoForward()
{
    return QCoreApplication::translate("QWebPage", "Go Forward", "Forward context menu item");
}

String contextMenuItemTagStop()
{
    return QCoreApplication::translate("QWebPage", "Stop", "Stop context menu item");
}

String contextMenuItemTagReload()
{
    return QCoreApplication::translate("QWebPage", "Reload", "Reload context menu item");
}

String contextMenuItemTagCut()
{
    return QCoreApplication::translate("QWebPage", "Cut", "Cut context menu item");
}

String contextMenuItemTagPaste()
{
    return QCoreApplication::translate("QWebPage", "Paste", "Paste context menu item");
}

String contextMenuItemTagSelectAll()
{
    return QCoreApplication::translate("QWebPage", "Select All", "Select All context menu item");
}

String contextMenuItemTagNoGuessesFound()
{
    return QCoreApplication::translate("QWebPage", "No Guesses Found", "No Guesses Found context menu item");
}

String contextMenuItemTagIgnoreSpelling()
{
    return QCoreApplication::translate("QWebPage", "Ignore", "Ignore Spelling context menu item");
}

String contextMenuItemTagLearnSpelling()
{
    return QCoreApplication::translate("QWebPage", "Add To Dictionary", "Learn Spelling context menu item");
}

String contextMenuItemTagSearchWeb()
{
    return QCoreApplication::translate("QWebPage", "Search The Web", "Search The Web context menu item");
}

String contextMenuItemTagLookUpInDictionary(const String&)
{
    return QCoreApplication::translate("QWebPage", "Look Up In Dictionary", "Look Up in Dictionary context menu item");
}

String contextMenuItemTagOpenLink()
{
    return QCoreApplication::translate("QWebPage", "Open Link", "Open Link context menu item");
}

String contextMenuItemTagIgnoreGrammar()
{
    return QCoreApplication::translate("QWebPage", "Ignore", "Ignore Grammar context menu item");
}

String contextMenuItemTagSpellingMenu()
{
    return QCoreApplication::translate("QWebPage", "Spelling", "Spelling and Grammar context sub-menu item");
}

String contextMenuItemTagShowSpellingPanel(bool show)
{
    return show ? QCoreApplication::translate("QWebPage", "Show Spelling and Grammar", "menu item title") :
                  QCoreApplication::translate("QWebPage", "Hide Spelling and Grammar", "menu item title");
}

String contextMenuItemTagCheckSpelling()
{
    return QCoreApplication::translate("QWebPage", "Check Spelling", "Check spelling context menu item");
}

String contextMenuItemTagCheckSpellingWhileTyping()
{
    return QCoreApplication::translate("QWebPage", "Check Spelling While Typing", "Check spelling while typing context menu item");
}

String contextMenuItemTagCheckGrammarWithSpelling()
{
    return QCoreApplication::translate("QWebPage", "Check Grammar With Spelling", "Check grammar with spelling context menu item");
}

String contextMenuItemTagFontMenu()
{
    return QCoreApplication::translate("QWebPage", "Fonts", "Font context sub-menu item");
}

String contextMenuItemTagBold()
{
    return QCoreApplication::translate("QWebPage", "Bold", "Bold context menu item");
}

String contextMenuItemTagItalic()
{
    return QCoreApplication::translate("QWebPage", "Italic", "Italic context menu item");
}

String contextMenuItemTagUnderline()
{
    return QCoreApplication::translate("QWebPage", "Underline", "Underline context menu item");
}

String contextMenuItemTagOutline()
{
    return QCoreApplication::translate("QWebPage", "Outline", "Outline context menu item");
}

String contextMenuItemTagWritingDirectionMenu()
{
    return QCoreApplication::translate("QWebPage", "Direction", "Writing direction context sub-menu item");
}

String contextMenuItemTagTextDirectionMenu()
{
    return QCoreApplication::translate("QWebPage", "Text Direction", "Text direction context sub-menu item");
}

String contextMenuItemTagDefaultDirection()
{
    return QCoreApplication::translate("QWebPage", "Default", "Default writing direction context menu item");
}

String contextMenuItemTagLeftToRight()
{
    return QCoreApplication::translate("QWebPage", "Left to Right", "Left to Right context menu item");
}

String contextMenuItemTagRightToLeft()
{
    return QCoreApplication::translate("QWebPage", "Right to Left", "Right to Left context menu item");
}

String contextMenuItemTagInspectElement()
{
    return QCoreApplication::translate("QWebPage", "Inspect", "Inspect Element context menu item");
}

String searchMenuNoRecentSearchesText()
{
    return QCoreApplication::translate("QWebPage", "No recent searches", "Label for only item in menu that appears when clicking on the search field image, when no searches have been performed");
}

String searchMenuRecentSearchesText()
{
    return QCoreApplication::translate("QWebPage", "Recent searches", "label for first item in the menu that appears when clicking on the search field image, used as embedded menu title");
}

String searchMenuClearRecentSearchesText()
{
    return QCoreApplication::translate("QWebPage", "Clear recent searches", "menu item in Recent Searches menu that empties menu's contents");
}

String AXWebAreaText()
{
    notImplemented();
    return String();
}

String AXLinkText()
{
    notImplemented();
    return String();
}

String AXListMarkerText()
{
    notImplemented();
    return String();
}

String AXImageMapText()
{
    notImplemented();
    return String();
}

String AXHeadingText()
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

String AXDescriptionListTermText()
{
    notImplemented();
    return String();
}

String AXDescriptionListDetailText()
{
    notImplemented();
    return String();
}

String AXButtonActionVerb()
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

String AXCheckedCheckBoxActionVerb()
{
    notImplemented();
    return String();
}

String AXUncheckedCheckBoxActionVerb()
{
    notImplemented();
    return String();
}

String AXMenuListActionVerb()
{
    notImplemented();
    return String();
}

String AXMenuListPopupActionVerb()
{
    notImplemented();
    return String();
}

String AXLinkActionVerb()
{
    notImplemented();
    return String();
}

String missingPluginText()
{
    return QCoreApplication::translate("QWebPage", "Missing Plug-in", "Label text to be used when a plug-in is missing");
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
    notImplemented();
    return String();
}

String unknownFileSizeText()
{
    return QCoreApplication::translate("QWebPage", "Unknown", "Unknown filesize FTP directory listing item");
}

String imageTitle(const String& filename, const IntSize& size)
{
    return QCoreApplication::translate("QWebPage", "%1 (%2x%3 pixels)", "Title string for images").arg(filename).arg(size.width()).arg(size.height());
}

String mediaElementLoadingStateText()
{
    return QCoreApplication::translate("QWebPage", "Loading...", "Media controller status message when the media is loading");
}

String mediaElementLiveBroadcastStateText()
{
    return QCoreApplication::translate("QWebPage", "Live Broadcast", "Media controller status message when watching a live broadcast");
}

#if ENABLE(VIDEO)

String localizedMediaControlElementString(const String& name)
{
    if (name == "AudioElement")
        return QCoreApplication::translate("QWebPage", "Audio Element", "Media controller element");
    if (name == "VideoElement")
        return QCoreApplication::translate("QWebPage", "Video Element", "Media controller element");
    if (name == "MuteButton")
        return QCoreApplication::translate("QWebPage", "Mute Button", "Media controller element");
    if (name == "UnMuteButton")
        return QCoreApplication::translate("QWebPage", "Unmute Button", "Media controller element");
    if (name == "PlayButton")
        return QCoreApplication::translate("QWebPage", "Play Button", "Media controller element");
    if (name == "PauseButton")
        return QCoreApplication::translate("QWebPage", "Pause Button", "Media controller element");
    if (name == "Slider")
        return QCoreApplication::translate("QWebPage", "Slider", "Media controller element");
    if (name == "SliderThumb")
        return QCoreApplication::translate("QWebPage", "Slider Thumb", "Media controller element");
    if (name == "RewindButton")
        return QCoreApplication::translate("QWebPage", "Rewind Button", "Media controller element");
    if (name == "ReturnToRealtimeButton")
        return QCoreApplication::translate("QWebPage", "Return to Real-time Button", "Media controller element");
    if (name == "CurrentTimeDisplay")
        return QCoreApplication::translate("QWebPage", "Elapsed Time", "Media controller element");
    if (name == "TimeRemainingDisplay")
        return QCoreApplication::translate("QWebPage", "Remaining Time", "Media controller element");
    if (name == "StatusDisplay")
        return QCoreApplication::translate("QWebPage", "Status Display", "Media controller element");
    if (name == "EnterFullscreenButton")
        return QCoreApplication::translate("QWebPage", "EnterFullscreen Button", "Media controller element");
    if (name == "ExitFullscreenButton")
        return QCoreApplication::translate("QWebPage", "ExitFullscreen Button", "Media controller element");
    if (name == "SeekForwardButton")
        return QCoreApplication::translate("QWebPage", "Seek Forward Button", "Media controller element");
    if (name == "SeekBackButton")
        return QCoreApplication::translate("QWebPage", "Seek Back Button", "Media controller element");

    return String();
}

String localizedMediaControlElementHelpText(const String& name)
{
    if (name == "AudioElement")
        return QCoreApplication::translate("QWebPage", "Audio element playback controls and status display", "Media controller element");
    if (name == "VideoElement")
        return QCoreApplication::translate("QWebPage", "Video element playback controls and status display", "Media controller element");
    if (name == "MuteButton")
        return QCoreApplication::translate("QWebPage", "Mute audio tracks", "Media controller element");
    if (name == "UnMuteButton")
        return QCoreApplication::translate("QWebPage", "Unmute audio tracks", "Media controller element");
    if (name == "PlayButton")
        return QCoreApplication::translate("QWebPage", "Begin playback", "Media controller element");
    if (name == "PauseButton")
        return QCoreApplication::translate("QWebPage", "Pause playback", "Media controller element");
    if (name == "Slider")
        return QCoreApplication::translate("QWebPage", "Movie time scrubber", "Media controller element");
    if (name == "SliderThumb")
        return QCoreApplication::translate("QWebPage", "Movie time scrubber thumb", "Media controller element");
    if (name == "RewindButton")
        return QCoreApplication::translate("QWebPage", "Rewind movie", "Media controller element");
    if (name == "ReturnToRealtimeButton")
        return QCoreApplication::translate("QWebPage", "Return streaming movie to real-time", "Media controller element");
    if (name == "CurrentTimeDisplay")
        return QCoreApplication::translate("QWebPage", "Current movie time", "Media controller element");
    if (name == "TimeRemainingDisplay")
        return QCoreApplication::translate("QWebPage", "Remaining movie time", "Media controller element");
    if (name == "StatusDisplay")
        return QCoreApplication::translate("QWebPage", "Current movie status", "Media controller element");
    if (name == "EnterFullscreenButton")
        return QCoreApplication::translate("QWebPage", "Play movie in full-screen mode", "Media controller element");
    if (name == "ExitFullscreenButton")
        return QCoreApplication::translate("QWebPage", "Exit full-screen mode", "Media controller element");
    if (name == "SeekForwardButton")
        return QCoreApplication::translate("QWebPage", "Seek quickly back", "Media controller element");
    if (name == "SeekBackButton")
        return QCoreApplication::translate("QWebPage", "Seek quickly forward", "Media controller element");

    ASSERT_NOT_REACHED();
    return String();
}

String localizedMediaTimeDescription(float time)
{
    if (!std::isfinite(time))
        return QCoreApplication::translate("QWebPage", "Indefinite time", "Media time description");

    int seconds = (int)fabsf(time);
    int days = seconds / (60 * 60 * 24);
    int hours = seconds / (60 * 60);
    int minutes = (seconds / 60) % 60;
    seconds %= 60;

    if (days)
        return QCoreApplication::translate("QWebPage", "%1 days %2 hours %3 minutes %4 seconds", "Media time description").arg(days).arg(hours).arg(minutes).arg(seconds);

    if (hours)
        return QCoreApplication::translate("QWebPage", "%1 hours %2 minutes %3 seconds", "Media time description").arg(hours).arg(minutes).arg(seconds);

    if (minutes)
        return QCoreApplication::translate("QWebPage", "%1 minutes %2 seconds", "Media time description").arg(minutes).arg(seconds);

    return QCoreApplication::translate("QWebPage", "%1 seconds", "Media time description").arg(seconds);
}

#else // ENABLE(VIDEO)
// FIXME: #if ENABLE(VIDEO) should be in the base class

String localizedMediaControlElementString(const String& name)
{
    return String();
}

String localizedMediaControlElementHelpText(const String& name)
{
    return String();
}

String localizedMediaTimeDescription(float time)
{
    return String();
}

#endif // ENABLE(VIDEO)


String validationMessageValueMissingText()
{
    notImplemented();
    return String();
}

String validationMessageValueMissingForCheckboxText()
{
    notImplemented();
    return String();
}

String validationMessageValueMissingForFileText()
{
    notImplemented();
    return String();
}

String validationMessageValueMissingForMultipleFileText()
{
    notImplemented();
    return String();
}

String validationMessageValueMissingForRadioText()
{
    notImplemented();
    return String();
}

String validationMessageValueMissingForSelectText()
{
    notImplemented();
    return String();
}

String validationMessageTypeMismatchText()
{
    notImplemented();
    return String();
}

String validationMessageTypeMismatchForEmailText()
{
    notImplemented();
    return String();
}

String validationMessageTypeMismatchForMultipleEmailText()
{
    notImplemented();
    return String();
}

String validationMessageTypeMismatchForURLText()
{
    notImplemented();
    return String();
}

String validationMessagePatternMismatchText()
{
    notImplemented();
    return String();
}

String validationMessageTooLongText(int valueLength, int maxLength)
{
    notImplemented();
    return String();
}

String validationMessageRangeUnderflowText(const String& minimum)
{
    notImplemented();
    return String();
}

String validationMessageRangeOverflowText(const String& maximum)
{
    notImplemented();
    return String();
}

String validationMessageStepMismatchText(const String& base, const String& step)
{
    notImplemented();
    return String();
}

String validationMessageBadInputForNumberText()
{
    notImplemented();
    return validationMessageTypeMismatchText();
}

#if ENABLE(VIDEO_TRACK)
String textTrackSubtitlesText()
{
    return QCoreApplication::translate("QWebPage", "Subtitles", "Menu section heading for subtitles");
}

String textTrackOffMenuItemText()
{
    return QCoreApplication::translate("QWebPage", "Off", "Menu item label for the track that represents disabling closed captions");
}

String textTrackAutomaticMenuItemText()
{
    return QCoreApplication::translate("QWebPage", "Auto", "Menu item label for the track that represents automatic closed captions selection");
}

String textTrackNoLabelText()
{
    return QCoreApplication::translate("QWebPage", "No label", "Menu item label for a closed captions track that has no other name");
}
#endif

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

String localizedString(const char* key)
{
    return String::fromUTF8(key, strlen(key));
}

}
