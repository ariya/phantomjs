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

#include "config.h"
#include "DefaultLocalizationStrategy.h"

#if USE(PLATFORM_STRATEGIES)

#include "IntSize.h"
#include "LocalizedStrings.h"
#include "NotImplemented.h"
#include "PlatformString.h"
#include <wtf/MathExtras.h>
#include <wtf/text/CString.h>
#include <wtf/unicode/CharacterNames.h>
#include <wtf/UnusedParam.h>

#if USE(CF)
#include <wtf/RetainPtr.h>
#endif

namespace WebCore {

// We can't use String::format for two reasons:
//  1) It doesn't handle non-ASCII characters in the format string.
//  2) It doesn't handle the %2$d syntax.
// Note that because |format| is used as the second parameter to va_start, it cannot be a reference
// type according to section 18.7/3 of the C++ N1905 standard.
static String formatLocalizedString(String format, ...)
{
#if USE(CF)
    va_list arguments;
    va_start(arguments, format);
    RetainPtr<CFStringRef> formatCFString(AdoptCF, format.createCFString());
    RetainPtr<CFStringRef> result(AdoptCF, CFStringCreateWithFormatAndArguments(0, 0, formatCFString.get(), arguments));
    va_end(arguments);
    return result.get();
#elif PLATFORM(QT)
    va_list arguments;
    va_start(arguments, format);
    QString result;
    result.vsprintf(format.latin1().data(), arguments);
    va_end(arguments);
    return result;
#else
    notImplemented();
    return format;
#endif
}

#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
static String truncatedStringForLookupMenuItem(const String& original)
{
    if (original.isEmpty())
        return original;
    
    // Truncate the string if it's too long. This is in consistency with AppKit.
    unsigned maxNumberOfGraphemeClustersInLookupMenuItem = 24;
    DEFINE_STATIC_LOCAL(String, ellipsis, (&horizontalEllipsis, 1));
    
    String trimmed = original.stripWhiteSpace();
    unsigned numberOfCharacters = numCharactersInGraphemeClusters(trimmed, maxNumberOfGraphemeClustersInLookupMenuItem);
    return numberOfCharacters == trimmed.length() ? trimmed : trimmed.left(numberOfCharacters) + ellipsis;
}
#endif

DefaultLocalizationStrategy::DefaultLocalizationStrategy()
{
}

String DefaultLocalizationStrategy::inputElementAltText()
{
    return WEB_UI_STRING_KEY("Submit", "Submit (input element)", "alt text for <input> elements with no alt, title, or value");
}

String DefaultLocalizationStrategy::resetButtonDefaultLabel()
{
    return WEB_UI_STRING("Reset", "default label for Reset buttons in forms on web pages");
}

String DefaultLocalizationStrategy::searchableIndexIntroduction()
{
    return WEB_UI_STRING("This is a searchable index. Enter search keywords: ",
        "text that appears at the start of nearly-obsolete web pages in the form of a 'searchable index'");
}

String DefaultLocalizationStrategy::submitButtonDefaultLabel()
{
    return WEB_UI_STRING("Submit", "default label for Submit buttons in forms on web pages");
}

String DefaultLocalizationStrategy::fileButtonChooseFileLabel()
{
    return WEB_UI_STRING("Choose File", "title for file button used in HTML forms");
}

String DefaultLocalizationStrategy::fileButtonNoFileSelectedLabel()
{
    return WEB_UI_STRING("no file selected", "text to display in file button used in HTML forms when no file is selected");
}

String DefaultLocalizationStrategy::defaultDetailsSummaryText()
{
    return WEB_UI_STRING("Details", "text to display in <details> tag when it has no <summary> child");
}

#if PLATFORM(MAC)
String DefaultLocalizationStrategy::copyImageUnknownFileLabel()
{
    return WEB_UI_STRING("unknown", "Unknown filename");
}
#endif

#if ENABLE(CONTEXT_MENUS)

String DefaultLocalizationStrategy::contextMenuItemTagOpenLinkInNewWindow()
{
    return WEB_UI_STRING("Open Link in New Window", "Open in New Window context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagDownloadLinkToDisk()
{
    return WEB_UI_STRING("Download Linked File", "Download Linked File context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagCopyLinkToClipboard()
{
    return WEB_UI_STRING("Copy Link", "Copy Link context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagOpenImageInNewWindow()
{
    return WEB_UI_STRING("Open Image in New Window", "Open Image in New Window context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagDownloadImageToDisk()
{
    return WEB_UI_STRING("Download Image", "Download Image context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagCopyImageToClipboard()
{
    return WEB_UI_STRING("Copy Image", "Copy Image context menu item");
}

#if PLATFORM(QT)
String DefaultLocalizationStrategy::contextMenuItemTagCopyImageUrlToClipboard()
{
    return WEB_UI_STRING("Copy Image Address", "Copy Image Address menu item");
}
#endif

String DefaultLocalizationStrategy::contextMenuItemTagOpenVideoInNewWindow()
{
    return WEB_UI_STRING("Open Video in New Window", "Open Video in New Window context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagOpenAudioInNewWindow()
{
    return WEB_UI_STRING("Open Audio in New Window", "Open Audio in New Window context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagCopyVideoLinkToClipboard()
{
    return WEB_UI_STRING("Copy Video Address", "Copy Video Address Location context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagCopyAudioLinkToClipboard()
{
    return WEB_UI_STRING("Copy Audio Address", "Copy Audio Address Location context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagToggleMediaControls()
{
    return WEB_UI_STRING("Controls", "Media Controls context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagToggleMediaLoop()
{
    return WEB_UI_STRING("Loop", "Media Loop context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagEnterVideoFullscreen()
{
    return WEB_UI_STRING("Enter Fullscreen", "Video Enter Fullscreen context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagMediaPlay()
{
    return WEB_UI_STRING("Play", "Media Play context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagMediaPause()
{
    return WEB_UI_STRING("Pause", "Media Pause context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagMediaMute()
{
    return WEB_UI_STRING("Mute", "Media Mute context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagOpenFrameInNewWindow()
{
    return WEB_UI_STRING("Open Frame in New Window", "Open Frame in New Window context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagCopy()
{
    return WEB_UI_STRING("Copy", "Copy context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagGoBack()
{
    return WEB_UI_STRING("Back", "Back context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagGoForward()
{
    return WEB_UI_STRING("Forward", "Forward context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagStop()
{
    return WEB_UI_STRING("Stop", "Stop context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagReload()
{
    return WEB_UI_STRING("Reload", "Reload context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagCut()
{
    return WEB_UI_STRING("Cut", "Cut context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagPaste()
{
    return WEB_UI_STRING("Paste", "Paste context menu item");
}

#if PLATFORM(GTK)

String DefaultLocalizationStrategy::contextMenuItemTagDelete()
{
    notImplemented();
    return "Delete";
}

String DefaultLocalizationStrategy::contextMenuItemTagInputMethods()
{
    notImplemented();
    return "Input Methods";
}

String DefaultLocalizationStrategy::contextMenuItemTagUnicode()
{
    notImplemented();
    return "Unicode";
}

#endif

#if PLATFORM(GTK) || PLATFORM(QT)

String DefaultLocalizationStrategy::contextMenuItemTagSelectAll()
{
    notImplemented();
    return "Select All";
}

#endif

String DefaultLocalizationStrategy::contextMenuItemTagNoGuessesFound()
{
    return WEB_UI_STRING("No Guesses Found", "No Guesses Found context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagIgnoreSpelling()
{
    return WEB_UI_STRING("Ignore Spelling", "Ignore Spelling context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagLearnSpelling()
{
    return WEB_UI_STRING("Learn Spelling", "Learn Spelling context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagSearchWeb()
{
    return WEB_UI_STRING("Search in Google", "Search in Google context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagLookUpInDictionary(const String& selectedString)
{
#if defined(BUILDING_ON_LEOPARD) || defined(BUILDING_ON_SNOW_LEOPARD)
    UNUSED_PARAM(selectedString);
    return WEB_UI_STRING("Look Up in Dictionary", "Look Up in Dictionary context menu item");
#else
#if USE(CF)
    RetainPtr<CFStringRef> selectedCFString(AdoptCF, truncatedStringForLookupMenuItem(selectedString).createCFString());
    return formatLocalizedString(WEB_UI_STRING("Look Up “%@”", "Look Up context menu item with selected word"), selectedCFString.get());
#else
    return WEB_UI_STRING("Look Up “<selection>”", "Look Up context menu item with selected word").replace("<selection>", truncatedStringForLookupMenuItem(selectedString));
#endif
#endif
}

String DefaultLocalizationStrategy::contextMenuItemTagOpenLink()
{
    return WEB_UI_STRING("Open Link", "Open Link context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagIgnoreGrammar()
{
    return WEB_UI_STRING("Ignore Grammar", "Ignore Grammar context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagSpellingMenu()
{
    return WEB_UI_STRING("Spelling and Grammar", "Spelling and Grammar context sub-menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagShowSpellingPanel(bool show)
{
    if (show)
        return WEB_UI_STRING("Show Spelling and Grammar", "menu item title");
    return WEB_UI_STRING("Hide Spelling and Grammar", "menu item title");
}

String DefaultLocalizationStrategy::contextMenuItemTagCheckSpelling()
{
    return WEB_UI_STRING("Check Document Now", "Check spelling context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagCheckSpellingWhileTyping()
{
    return WEB_UI_STRING("Check Spelling While Typing", "Check spelling while typing context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagCheckGrammarWithSpelling()
{
    return WEB_UI_STRING("Check Grammar With Spelling", "Check grammar with spelling context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagFontMenu()
{
    return WEB_UI_STRING("Font", "Font context sub-menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagBold()
{
    return WEB_UI_STRING("Bold", "Bold context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagItalic()
{
    return WEB_UI_STRING("Italic", "Italic context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagUnderline()
{
    return WEB_UI_STRING("Underline", "Underline context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagOutline()
{
    return WEB_UI_STRING("Outline", "Outline context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagWritingDirectionMenu()
{
    return WEB_UI_STRING("Paragraph Direction", "Paragraph direction context sub-menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagTextDirectionMenu()
{
    return WEB_UI_STRING("Selection Direction", "Selection direction context sub-menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagDefaultDirection()
{
    return WEB_UI_STRING("Default", "Default writing direction context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagLeftToRight()
{
    return WEB_UI_STRING("Left to Right", "Left to Right context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagRightToLeft()
{
    return WEB_UI_STRING("Right to Left", "Right to Left context menu item");
}

#if PLATFORM(MAC)

String DefaultLocalizationStrategy::contextMenuItemTagSearchInSpotlight()
{
    return WEB_UI_STRING("Search in Spotlight", "Search in Spotlight context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagShowFonts()
{
    return WEB_UI_STRING("Show Fonts", "Show fonts context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagStyles()
{
    return WEB_UI_STRING("Styles...", "Styles context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagShowColors()
{
    return WEB_UI_STRING("Show Colors", "Show colors context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagSpeechMenu()
{
    return WEB_UI_STRING("Speech", "Speech context sub-menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagStartSpeaking()
{
    return WEB_UI_STRING("Start Speaking", "Start speaking context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagStopSpeaking()
{
    return WEB_UI_STRING("Stop Speaking", "Stop speaking context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagCorrectSpellingAutomatically()
{
    return WEB_UI_STRING("Correct Spelling Automatically", "Correct Spelling Automatically context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagSubstitutionsMenu()
{
    return WEB_UI_STRING("Substitutions", "Substitutions context sub-menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagShowSubstitutions(bool show)
{
    if (show) 
        return WEB_UI_STRING("Show Substitutions", "menu item title");
    return WEB_UI_STRING("Hide Substitutions", "menu item title");
}

String DefaultLocalizationStrategy::contextMenuItemTagSmartCopyPaste()
{
    return WEB_UI_STRING("Smart Copy/Paste", "Smart Copy/Paste context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagSmartQuotes()
{
    return WEB_UI_STRING("Smart Quotes", "Smart Quotes context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagSmartDashes()
{
    return WEB_UI_STRING("Smart Dashes", "Smart Dashes context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagSmartLinks()
{
    return WEB_UI_STRING("Smart Links", "Smart Links context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagTextReplacement()
{
    return WEB_UI_STRING("Text Replacement", "Text Replacement context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagTransformationsMenu()
{
    return WEB_UI_STRING("Transformations", "Transformations context sub-menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagMakeUpperCase()
{
    return WEB_UI_STRING("Make Upper Case", "Make Upper Case context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagMakeLowerCase()
{
    return WEB_UI_STRING("Make Lower Case", "Make Lower Case context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagCapitalize()
{
    return WEB_UI_STRING("Capitalize", "Capitalize context menu item");
}

String DefaultLocalizationStrategy::contextMenuItemTagChangeBack(const String& replacedString)
{
    notImplemented();
    return replacedString;
}

#endif

String DefaultLocalizationStrategy::contextMenuItemTagInspectElement()
{
    return WEB_UI_STRING("Inspect Element", "Inspect Element context menu item");
}

#endif // ENABLE(CONTEXT_MENUS)

String DefaultLocalizationStrategy::searchMenuNoRecentSearchesText()
{
    return WEB_UI_STRING("No recent searches", "Label for only item in menu that appears when clicking on the search field image, when no searches have been performed");
}

String DefaultLocalizationStrategy::searchMenuRecentSearchesText()
{
    return WEB_UI_STRING("Recent Searches", "label for first item in the menu that appears when clicking on the search field image, used as embedded menu title");
}

String DefaultLocalizationStrategy::searchMenuClearRecentSearchesText()
{
    return WEB_UI_STRING("Clear Recent Searches", "menu item in Recent Searches menu that empties menu's contents");
}

String DefaultLocalizationStrategy::AXWebAreaText()
{
    return WEB_UI_STRING("HTML content", "accessibility role description for web area");
}

String DefaultLocalizationStrategy::AXLinkText()
{
    return WEB_UI_STRING("link", "accessibility role description for link");
}

String DefaultLocalizationStrategy::AXListMarkerText()
{
    return WEB_UI_STRING("list marker", "accessibility role description for list marker");
}

String DefaultLocalizationStrategy::AXImageMapText()
{
    return WEB_UI_STRING("image map", "accessibility role description for image map");
}

String DefaultLocalizationStrategy::AXHeadingText()
{
    return WEB_UI_STRING("heading", "accessibility role description for headings");
}

String DefaultLocalizationStrategy::AXDefinitionListTermText()
{
    return WEB_UI_STRING("term", "term word of a definition");
}

String DefaultLocalizationStrategy::AXDefinitionListDefinitionText()
{
    return WEB_UI_STRING("definition", "definition phrase");
}

#if PLATFORM(MAC)
String DefaultLocalizationStrategy::AXARIAContentGroupText(const String& ariaType)
{
    if (ariaType == "ARIAApplicationAlert")
        return WEB_UI_STRING("alert", "An ARIA accessibility group that acts as an alert.");
    if (ariaType == "ARIAApplicationAlertDialog")
        return WEB_UI_STRING("alert dialog", "An ARIA accessibility group that acts as an alert dialog.");
    if (ariaType == "ARIAApplicationDialog")
        return WEB_UI_STRING("dialog", "An ARIA accessibility group that acts as an dialog.");
    if (ariaType == "ARIAApplicationLog")
        return WEB_UI_STRING("log", "An ARIA accessibility group that acts as a console log.");
    if (ariaType == "ARIAApplicationMarquee")
        return WEB_UI_STRING("marquee", "An ARIA accessibility group that acts as a marquee.");    
    if (ariaType == "ARIAApplicationStatus")
        return WEB_UI_STRING("application status", "An ARIA accessibility group that acts as a status update.");    
    if (ariaType == "ARIAApplicationTimer")
        return WEB_UI_STRING("timer", "An ARIA accessibility group that acts as an updating timer.");    
    if (ariaType == "ARIADocument")
        return WEB_UI_STRING("document", "An ARIA accessibility group that acts as a document.");    
    if (ariaType == "ARIADocumentArticle")
        return WEB_UI_STRING("article", "An ARIA accessibility group that acts as an article.");    
    if (ariaType == "ARIADocumentNote")
        return WEB_UI_STRING("note", "An ARIA accessibility group that acts as a note in a document.");    
    if (ariaType == "ARIADocumentRegion")
        return WEB_UI_STRING("region", "An ARIA accessibility group that acts as a distinct region in a document.");    
    if (ariaType == "ARIALandmarkApplication")
        return WEB_UI_STRING("application", "An ARIA accessibility group that acts as an application.");    
    if (ariaType == "ARIALandmarkBanner")
        return WEB_UI_STRING("banner", "An ARIA accessibility group that acts as a banner.");    
    if (ariaType == "ARIALandmarkComplementary")
        return WEB_UI_STRING("complementary", "An ARIA accessibility group that acts as a region of complementary information.");    
    if (ariaType == "ARIALandmarkContentInfo")
        return WEB_UI_STRING("content", "An ARIA accessibility group that contains content.");    
    if (ariaType == "ARIALandmarkMain")
        return WEB_UI_STRING("main", "An ARIA accessibility group that is the main portion of the website.");    
    if (ariaType == "ARIALandmarkNavigation")
        return WEB_UI_STRING("navigation", "An ARIA accessibility group that contains the main navigation elements of a website.");    
    if (ariaType == "ARIALandmarkSearch")
        return WEB_UI_STRING("search", "An ARIA accessibility group that contains a search feature of a website.");    
    if (ariaType == "ARIAUserInterfaceTooltip")
        return WEB_UI_STRING("tooltip", "An ARIA accessibility group that acts as a tooltip.");    
    if (ariaType == "ARIATabPanel")
        return WEB_UI_STRING("tab panel", "An ARIA accessibility group that contains the content of a tab.");
    if (ariaType == "ARIADocumentMath")
        return WEB_UI_STRING("math", "An ARIA accessibility group that contains mathematical symbols.");
    return String();
}
#endif

String DefaultLocalizationStrategy::AXButtonActionVerb()
{
    return WEB_UI_STRING("press", "Verb stating the action that will occur when a button is pressed, as used by accessibility");
}

String DefaultLocalizationStrategy::AXRadioButtonActionVerb() 
{
    return WEB_UI_STRING("select", "Verb stating the action that will occur when a radio button is clicked, as used by accessibility");
}

String DefaultLocalizationStrategy::AXTextFieldActionVerb()
{
    return WEB_UI_STRING("activate", "Verb stating the action that will occur when a text field is selected, as used by accessibility");
}

String DefaultLocalizationStrategy::AXCheckedCheckBoxActionVerb()
{
    return WEB_UI_STRING("uncheck", "Verb stating the action that will occur when a checked checkbox is clicked, as used by accessibility");
}

String DefaultLocalizationStrategy::AXUncheckedCheckBoxActionVerb()
{
    return WEB_UI_STRING("check", "Verb stating the action that will occur when an unchecked checkbox is clicked, as used by accessibility");
}

String DefaultLocalizationStrategy::AXMenuListActionVerb()
{
    notImplemented();
    return "select";
}

String DefaultLocalizationStrategy::AXMenuListPopupActionVerb()
{
    notImplemented();
    return "select";
}

String DefaultLocalizationStrategy::AXLinkActionVerb()
{
    return WEB_UI_STRING("jump", "Verb stating the action that will occur when a link is clicked, as used by accessibility");
}

String DefaultLocalizationStrategy::missingPluginText()
{
    return WEB_UI_STRING("Missing Plug-in", "Label text to be used when a plugin is missing");
}

String DefaultLocalizationStrategy::crashedPluginText()
{
    return WEB_UI_STRING("Plug-in Failure", "Label text to be used if plugin host process has crashed");
}

String DefaultLocalizationStrategy::multipleFileUploadText(unsigned numberOfFiles)
{
    return formatLocalizedString(WEB_UI_STRING("%d files", "Label to describe the number of files selected in a file upload control that allows multiple files"), numberOfFiles);
}

String DefaultLocalizationStrategy::unknownFileSizeText()
{
    return WEB_UI_STRING("Unknown", "Unknown filesize FTP directory listing item");
}

#if PLATFORM(WIN)

String DefaultLocalizationStrategy::uploadFileText()
{
    notImplemented();
    return "upload";
}

String DefaultLocalizationStrategy::allFilesText()
{
    notImplemented();
    return "all files";
}

#endif

#if PLATFORM(MAC)

String DefaultLocalizationStrategy::keygenMenuItem512()
{
    return WEB_UI_STRING("512 (Low Grade)", "Menu item title for KEYGEN pop-up menu");
}

String DefaultLocalizationStrategy::keygenMenuItem1024()
{
    return WEB_UI_STRING("1024 (Medium Grade)", "Menu item title for KEYGEN pop-up menu");
}

String DefaultLocalizationStrategy::keygenMenuItem2048()
{
    return WEB_UI_STRING("2048 (High Grade)", "Menu item title for KEYGEN pop-up menu");
}

String DefaultLocalizationStrategy::keygenKeychainItemName(const String& host)
{
    RetainPtr<CFStringRef> hostCFString(AdoptCF, host.createCFString());
    return formatLocalizedString(WEB_UI_STRING("Key from %@", "Name of keychain key generated by the KEYGEN tag"), hostCFString.get());
}

#endif

String DefaultLocalizationStrategy::imageTitle(const String& filename, const IntSize& size)
{
#if USE(CF)
#if !defined(BUILDING_ON_LEOPARD)
    RetainPtr<CFStringRef> filenameCFString(AdoptCF, filename.createCFString());
    RetainPtr<CFLocaleRef> locale(AdoptCF, CFLocaleCopyCurrent());
    RetainPtr<CFNumberFormatterRef> formatter(AdoptCF, CFNumberFormatterCreate(0, locale.get(), kCFNumberFormatterDecimalStyle));

    int widthInt = size.width();
    RetainPtr<CFNumberRef> width(AdoptCF, CFNumberCreate(0, kCFNumberIntType, &widthInt));
    RetainPtr<CFStringRef> widthString(AdoptCF, CFNumberFormatterCreateStringWithNumber(0, formatter.get(), width.get()));

    int heightInt = size.height();
    RetainPtr<CFNumberRef> height(AdoptCF, CFNumberCreate(0, kCFNumberIntType, &heightInt));
    RetainPtr<CFStringRef> heightString(AdoptCF, CFNumberFormatterCreateStringWithNumber(0, formatter.get(), height.get()));

    return formatLocalizedString(WEB_UI_STRING("%@ %@×%@ pixels", "window title for a standalone image (uses multiplication symbol, not x)"), filenameCFString.get(), widthString.get(), heightString.get());
#else
    RetainPtr<CFStringRef> filenameCFString(AdoptCF, filename.createCFString());
    return formatLocalizedString(WEB_UI_STRING("%@ %d×%d pixels", "window title for a standalone image (uses multiplication symbol, not x)"), filenameCFString.get(), size.width(), size.height());
#endif
#else
    return formatLocalizedString(WEB_UI_STRING("<filename> %d×%d pixels", "window title for a standalone image (uses multiplication symbol, not x)"), size.width(), size.height()).replace("<filename>", filename);
#endif
}

String DefaultLocalizationStrategy::mediaElementLoadingStateText()
{
    return WEB_UI_STRING("Loading...", "Media controller status message when the media is loading");
}

String DefaultLocalizationStrategy::mediaElementLiveBroadcastStateText()
{
    return WEB_UI_STRING("Live Broadcast", "Media controller status message when watching a live broadcast");
}

String DefaultLocalizationStrategy::localizedMediaControlElementString(const String& name)
{
    if (name == "AudioElement")
        return WEB_UI_STRING("audio element controller", "accessibility role description for audio element controller");
    if (name == "VideoElement")
        return WEB_UI_STRING("video element controller", "accessibility role description for video element controller");
    if (name == "MuteButton")
        return WEB_UI_STRING("mute", "accessibility role description for mute button");
    if (name == "UnMuteButton")
        return WEB_UI_STRING("unmute", "accessibility role description for turn mute off button");
    if (name == "PlayButton")
        return WEB_UI_STRING("play", "accessibility role description for play button");
    if (name == "PauseButton")
        return WEB_UI_STRING("pause", "accessibility role description for pause button");
    if (name == "Slider")
        return WEB_UI_STRING("movie time", "accessibility role description for timeline slider");
    if (name == "SliderThumb")
        return WEB_UI_STRING("timeline slider thumb", "accessibility role description for timeline thumb");
    if (name == "RewindButton")
        return WEB_UI_STRING("back 30 seconds", "accessibility role description for seek back 30 seconds button");
    if (name == "ReturnToRealtimeButton")
        return WEB_UI_STRING("return to realtime", "accessibility role description for return to real time button");
    if (name == "CurrentTimeDisplay")
        return WEB_UI_STRING("elapsed time", "accessibility role description for elapsed time display");
    if (name == "TimeRemainingDisplay")
        return WEB_UI_STRING("remaining time", "accessibility role description for time remaining display");
    if (name == "StatusDisplay")
        return WEB_UI_STRING("status", "accessibility role description for movie status");
    if (name == "FullscreenButton")
        return WEB_UI_STRING("fullscreen", "accessibility role description for enter fullscreen button");
    if (name == "SeekForwardButton")
        return WEB_UI_STRING("fast forward", "accessibility role description for fast forward button");
    if (name == "SeekBackButton")
        return WEB_UI_STRING("fast reverse", "accessibility role description for fast reverse button");
    if (name == "ShowClosedCaptionsButton")
        return WEB_UI_STRING("show closed captions", "accessibility role description for show closed captions button");
    if (name == "HideClosedCaptionsButton")
        return WEB_UI_STRING("hide closed captions", "accessibility role description for hide closed captions button");

    // FIXME: the ControlsPanel container should never be visible in the accessibility hierarchy.
    if (name == "ControlsPanel")
        return String();

    ASSERT_NOT_REACHED();
    return String();
}

String DefaultLocalizationStrategy::localizedMediaControlElementHelpText(const String& name)
{
    if (name == "AudioElement")
        return WEB_UI_STRING("audio element playback controls and status display", "accessibility role description for audio element controller");
    if (name == "VideoElement")
        return WEB_UI_STRING("video element playback controls and status display", "accessibility role description for video element controller");
    if (name == "MuteButton")
        return WEB_UI_STRING("mute audio tracks", "accessibility help text for mute button");
    if (name == "UnMuteButton")
        return WEB_UI_STRING("unmute audio tracks", "accessibility help text for un mute button");
    if (name == "PlayButton")
        return WEB_UI_STRING("begin playback", "accessibility help text for play button");
    if (name == "PauseButton")
        return WEB_UI_STRING("pause playback", "accessibility help text for pause button");
    if (name == "Slider")
        return WEB_UI_STRING("movie time scrubber", "accessibility help text for timeline slider");
    if (name == "SliderThumb")
        return WEB_UI_STRING("movie time scrubber thumb", "accessibility help text for timeline slider thumb");
    if (name == "RewindButton")
        return WEB_UI_STRING("seek movie back 30 seconds", "accessibility help text for jump back 30 seconds button");
    if (name == "ReturnToRealtimeButton")
        return WEB_UI_STRING("return streaming movie to real time", "accessibility help text for return streaming movie to real time button");
    if (name == "CurrentTimeDisplay")
        return WEB_UI_STRING("current movie time in seconds", "accessibility help text for elapsed time display");
    if (name == "TimeRemainingDisplay")
        return WEB_UI_STRING("number of seconds of movie remaining", "accessibility help text for remaining time display");
    if (name == "StatusDisplay")
        return WEB_UI_STRING("current movie status", "accessibility help text for movie status display");
    if (name == "SeekBackButton")
        return WEB_UI_STRING("seek quickly back", "accessibility help text for fast rewind button");
    if (name == "SeekForwardButton")
        return WEB_UI_STRING("seek quickly forward", "accessibility help text for fast forward button");
    if (name == "FullscreenButton")
        return WEB_UI_STRING("Play movie in fullscreen mode", "accessibility help text for enter fullscreen button");
    if (name == "ShowClosedCaptionsButton")
        return WEB_UI_STRING("start displaying closed captions", "accessibility help text for show closed captions button");
    if (name == "HideClosedCaptionsButton")
        return WEB_UI_STRING("stop displaying closed captions", "accessibility help text for hide closed captions button");

    ASSERT_NOT_REACHED();
    return String();
}

String DefaultLocalizationStrategy::localizedMediaTimeDescription(float time)
{
    if (!isfinite(time))
        return WEB_UI_STRING("indefinite time", "accessibility help text for an indefinite media controller time value");

    int seconds = static_cast<int>(fabsf(time)); 
    int days = seconds / (60 * 60 * 24);
    int hours = seconds / (60 * 60);
    int minutes = (seconds / 60) % 60;
    seconds %= 60;

    if (days)
        return formatLocalizedString(WEB_UI_STRING("%1$d days %2$d hours %3$d minutes %4$d seconds", "accessibility help text for media controller time value >= 1 day"), days, hours, minutes, seconds);
    if (hours)
        return formatLocalizedString(WEB_UI_STRING("%1$d hours %2$d minutes %3$d seconds", "accessibility help text for media controller time value >= 60 minutes"), hours, minutes, seconds);
    if (minutes)
        return formatLocalizedString(WEB_UI_STRING("%1$d minutes %2$d seconds", "accessibility help text for media controller time value >= 60 seconds"), minutes, seconds);
    return formatLocalizedString(WEB_UI_STRING("%1$d seconds", "accessibility help text for media controller time value < 60 seconds"), seconds);
}

String DefaultLocalizationStrategy::validationMessageValueMissingText()
{
    return WEB_UI_STRING("value missing", "Validation message for required form control elements that have no value");
}

String DefaultLocalizationStrategy::validationMessageTypeMismatchText()
{
    return WEB_UI_STRING("type mismatch", "Validation message for input form controls with a value not matching type");
}

String DefaultLocalizationStrategy::validationMessagePatternMismatchText()
{
    return WEB_UI_STRING("pattern mismatch", "Validation message for input form controls requiring a constrained value according to pattern");
}

String DefaultLocalizationStrategy::validationMessageTooLongText()
{
    return WEB_UI_STRING("too long", "Validation message for form control elements with a value longer than maximum allowed length");
}

String DefaultLocalizationStrategy::validationMessageRangeUnderflowText()
{
    return WEB_UI_STRING("range underflow", "Validation message for input form controls with value lower than allowed minimum");
}

String DefaultLocalizationStrategy::validationMessageRangeOverflowText()
{
    return WEB_UI_STRING("range overflow", "Validation message for input form controls with value higher than allowed maximum");
}

String DefaultLocalizationStrategy::validationMessageStepMismatchText()
{
    return WEB_UI_STRING("step mismatch", "Validation message for input form controls with value not respecting the step attribute");
}

} // namespace WebCore

#endif // USE(PLATFORM_STRATEGIES)
