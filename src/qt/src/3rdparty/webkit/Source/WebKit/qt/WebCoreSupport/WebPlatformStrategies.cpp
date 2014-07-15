/*
 * Copyright (C) 2007 Staikos Computing Services Inc. <info@staikos.net>
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "WebPlatformStrategies.h"

#include "Chrome.h"
#include "ChromeClientQt.h"
#include <IntSize.h>
#include "NotImplemented.h"
#include <Page.h>
#include <PageGroup.h>
#include <PluginDatabase.h>
#include <QCoreApplication>
#include <QLocale>
#include <qwebpage.h>
#include <qwebpluginfactory.h>
#include <wtf/MathExtras.h>

using namespace WebCore;

void WebPlatformStrategies::initialize()
{
    DEFINE_STATIC_LOCAL(WebPlatformStrategies, platformStrategies, ());
    Q_UNUSED(platformStrategies);
}

WebPlatformStrategies::WebPlatformStrategies()
{
    setPlatformStrategies(this);
}


CookiesStrategy* WebPlatformStrategies::createCookiesStrategy()
{
    return this;
}

PluginStrategy* WebPlatformStrategies::createPluginStrategy()
{
    return this;
}

LocalizationStrategy* WebPlatformStrategies::createLocalizationStrategy()
{
    return this;
}

VisitedLinkStrategy* WebPlatformStrategies::createVisitedLinkStrategy()
{
    return this;
}

void WebPlatformStrategies::notifyCookiesChanged()
{
}

void WebPlatformStrategies::refreshPlugins()
{
    PluginDatabase::installedPlugins()->refresh();
}

void WebPlatformStrategies::getPluginInfo(const WebCore::Page* page, Vector<WebCore::PluginInfo>& outPlugins)
{
    QWebPage* qPage = static_cast<ChromeClientQt*>(page->chrome()->client())->m_webPage;
    QWebPluginFactory* factory;
    if (qPage && (factory = qPage->pluginFactory())) {

        QList<QWebPluginFactory::Plugin> qplugins = factory->plugins();
        for (int i = 0; i < qplugins.count(); ++i) {
            const QWebPluginFactory::Plugin& qplugin = qplugins.at(i);
            PluginInfo info;
            info.name = qplugin.name;
            info.desc = qplugin.description;

            for (int j = 0; j < qplugin.mimeTypes.count(); ++j) {
                const QWebPluginFactory::MimeType& mimeType = qplugin.mimeTypes.at(j);

                MimeClassInfo mimeInfo;
                mimeInfo.type = mimeType.name;
                mimeInfo.desc = mimeType.description;
                for (int k = 0; k < mimeType.fileExtensions.count(); ++k)
                  mimeInfo.extensions.append(mimeType.fileExtensions.at(k));

                info.mimes.append(mimeInfo);
            }
            outPlugins.append(info);
        }
    }

    PluginDatabase* db = PluginDatabase::installedPlugins();
    const Vector<PluginPackage*> &plugins = db->plugins();

    for (unsigned int i = 0; i < plugins.size(); ++i) {
        PluginInfo info;
        PluginPackage* package = plugins[i];

        info.name = package->name();
        info.file = package->fileName();
        info.desc = package->description();

        const MIMEToDescriptionsMap& mimeToDescriptions = package->mimeToDescriptions();
        MIMEToDescriptionsMap::const_iterator end = mimeToDescriptions.end();
        for (MIMEToDescriptionsMap::const_iterator it = mimeToDescriptions.begin(); it != end; ++it) {
            MimeClassInfo mime;

            mime.type = it->first;
            mime.desc = it->second;
            mime.extensions = package->mimeToExtensions().get(mime.type);

            info.mimes.append(mime);
        }

        outPlugins.append(info);
    }

}


// LocalizationStrategy

String WebPlatformStrategies::inputElementAltText()
{
    return QCoreApplication::translate("QWebPage", "Submit", "Submit (input element) alt text for <input> elements with no alt, title, or value");
}

String WebPlatformStrategies::resetButtonDefaultLabel()
{
    return QCoreApplication::translate("QWebPage", "Reset", "default label for Reset buttons in forms on web pages");
}

String WebPlatformStrategies::searchableIndexIntroduction()
{
    return QCoreApplication::translate("QWebPage", "This is a searchable index. Enter search keywords: ", "text that appears at the start of nearly-obsolete web pages in the form of a 'searchable index'");
}

String WebPlatformStrategies::submitButtonDefaultLabel()
{
    return QCoreApplication::translate("QWebPage", "Submit", "default label for Submit buttons in forms on web pages");
}

String WebPlatformStrategies::fileButtonChooseFileLabel()
{
    return QCoreApplication::translate("QWebPage", "Choose File", "title for file button used in HTML forms");
}

String WebPlatformStrategies::fileButtonNoFileSelectedLabel()
{
    return QCoreApplication::translate("QWebPage", "No file selected", "text to display in file button used in HTML forms when no file is selected");
}

String WebPlatformStrategies::defaultDetailsSummaryText()
{
    return QCoreApplication::translate("QWebPage", "Details", "text to display in <details> tag when it has no <summary> child");
}

String WebPlatformStrategies::contextMenuItemTagOpenLinkInNewWindow()
{
    return QCoreApplication::translate("QWebPage", "Open in New Window", "Open in New Window context menu item");
}

String WebPlatformStrategies::contextMenuItemTagDownloadLinkToDisk()
{
    return QCoreApplication::translate("QWebPage", "Save Link...", "Download Linked File context menu item");
}

String WebPlatformStrategies::contextMenuItemTagCopyLinkToClipboard()
{
    return QCoreApplication::translate("QWebPage", "Copy Link", "Copy Link context menu item");
}

String WebPlatformStrategies::contextMenuItemTagOpenImageInNewWindow()
{
    return QCoreApplication::translate("QWebPage", "Open Image", "Open Image in New Window context menu item");
}

String WebPlatformStrategies::contextMenuItemTagDownloadImageToDisk()
{
    return QCoreApplication::translate("QWebPage", "Save Image", "Download Image context menu item");
}

String WebPlatformStrategies::contextMenuItemTagCopyImageToClipboard()
{
    return QCoreApplication::translate("QWebPage", "Copy Image", "Copy Link context menu item");
}

String WebPlatformStrategies::contextMenuItemTagCopyImageUrlToClipboard()
{
    return QCoreApplication::translate("QWebPage", "Copy Image Address", "Copy Image Address menu item");
}

String WebPlatformStrategies::contextMenuItemTagOpenVideoInNewWindow()
{
    return QCoreApplication::translate("QWebPage", "Open Video", "Open Video in New Window");
}

String WebPlatformStrategies::contextMenuItemTagOpenAudioInNewWindow()
{
    return QCoreApplication::translate("QWebPage", "Open Audio", "Open Audio in New Window");
}

String WebPlatformStrategies::contextMenuItemTagCopyVideoLinkToClipboard()
{
    return QCoreApplication::translate("QWebPage", "Copy Video", "Copy Video Link Location");
}

String WebPlatformStrategies::contextMenuItemTagCopyAudioLinkToClipboard()
{
    return QCoreApplication::translate("QWebPage", "Copy Audio", "Copy Audio Link Location");
}

String WebPlatformStrategies::contextMenuItemTagToggleMediaControls()
{
    return QCoreApplication::translate("QWebPage", "Toggle Controls", "Toggle Media Controls");
}

String WebPlatformStrategies::contextMenuItemTagToggleMediaLoop()
{
    return QCoreApplication::translate("QWebPage", "Toggle Loop", "Toggle Media Loop Playback");
}

String WebPlatformStrategies::contextMenuItemTagEnterVideoFullscreen()
{
    return QCoreApplication::translate("QWebPage", "Enter Fullscreen", "Switch Video to Fullscreen");
}

String WebPlatformStrategies::contextMenuItemTagMediaPlay()
{
    return QCoreApplication::translate("QWebPage", "Play", "Play");
}

String WebPlatformStrategies::contextMenuItemTagMediaPause()
{
    return QCoreApplication::translate("QWebPage", "Pause", "Pause");
}

String WebPlatformStrategies::contextMenuItemTagMediaMute()
{
    return QCoreApplication::translate("QWebPage", "Mute", "Mute");
}

String WebPlatformStrategies::contextMenuItemTagOpenFrameInNewWindow()
{
    return QCoreApplication::translate("QWebPage", "Open Frame", "Open Frame in New Window context menu item");
}

String WebPlatformStrategies::contextMenuItemTagCopy()
{
    return QCoreApplication::translate("QWebPage", "Copy", "Copy context menu item");
}

String WebPlatformStrategies::contextMenuItemTagGoBack()
{
    return QCoreApplication::translate("QWebPage", "Go Back", "Back context menu item");
}

String WebPlatformStrategies::contextMenuItemTagGoForward()
{
    return QCoreApplication::translate("QWebPage", "Go Forward", "Forward context menu item");
}

String WebPlatformStrategies::contextMenuItemTagStop()
{
    return QCoreApplication::translate("QWebPage", "Stop", "Stop context menu item");
}

String WebPlatformStrategies::contextMenuItemTagReload()
{
    return QCoreApplication::translate("QWebPage", "Reload", "Reload context menu item");
}

String WebPlatformStrategies::contextMenuItemTagCut()
{
    return QCoreApplication::translate("QWebPage", "Cut", "Cut context menu item");
}

String WebPlatformStrategies::contextMenuItemTagPaste()
{
    return QCoreApplication::translate("QWebPage", "Paste", "Paste context menu item");
}

String WebPlatformStrategies::contextMenuItemTagSelectAll()
{
    return QCoreApplication::translate("QWebPage", "Select All", "Select All context menu item");
}

String WebPlatformStrategies::contextMenuItemTagNoGuessesFound()
{
    return QCoreApplication::translate("QWebPage", "No Guesses Found", "No Guesses Found context menu item");
}

String WebPlatformStrategies::contextMenuItemTagIgnoreSpelling()
{
    return QCoreApplication::translate("QWebPage", "Ignore", "Ignore Spelling context menu item");
}

String WebPlatformStrategies::contextMenuItemTagLearnSpelling()
{
    return QCoreApplication::translate("QWebPage", "Add To Dictionary", "Learn Spelling context menu item");
}

String WebPlatformStrategies::contextMenuItemTagSearchWeb()
{
    return QCoreApplication::translate("QWebPage", "Search The Web", "Search The Web context menu item");
}

String WebPlatformStrategies::contextMenuItemTagLookUpInDictionary(const String&)
{
    return QCoreApplication::translate("QWebPage", "Look Up In Dictionary", "Look Up in Dictionary context menu item");
}

String WebPlatformStrategies::contextMenuItemTagOpenLink()
{
    return QCoreApplication::translate("QWebPage", "Open Link", "Open Link context menu item");
}

String WebPlatformStrategies::contextMenuItemTagIgnoreGrammar()
{
    return QCoreApplication::translate("QWebPage", "Ignore", "Ignore Grammar context menu item");
}

String WebPlatformStrategies::contextMenuItemTagSpellingMenu()
{
    return QCoreApplication::translate("QWebPage", "Spelling", "Spelling and Grammar context sub-menu item");
}

String WebPlatformStrategies::contextMenuItemTagShowSpellingPanel(bool show)
{
    return show ? QCoreApplication::translate("QWebPage", "Show Spelling and Grammar", "menu item title") :
                  QCoreApplication::translate("QWebPage", "Hide Spelling and Grammar", "menu item title");
}

String WebPlatformStrategies::contextMenuItemTagCheckSpelling()
{
    return QCoreApplication::translate("QWebPage", "Check Spelling", "Check spelling context menu item");
}

String WebPlatformStrategies::contextMenuItemTagCheckSpellingWhileTyping()
{
    return QCoreApplication::translate("QWebPage", "Check Spelling While Typing", "Check spelling while typing context menu item");
}

String WebPlatformStrategies::contextMenuItemTagCheckGrammarWithSpelling()
{
    return QCoreApplication::translate("QWebPage", "Check Grammar With Spelling", "Check grammar with spelling context menu item");
}

String WebPlatformStrategies::contextMenuItemTagFontMenu()
{
    return QCoreApplication::translate("QWebPage", "Fonts", "Font context sub-menu item");
}

String WebPlatformStrategies::contextMenuItemTagBold()
{
    return QCoreApplication::translate("QWebPage", "Bold", "Bold context menu item");
}

String WebPlatformStrategies::contextMenuItemTagItalic()
{
    return QCoreApplication::translate("QWebPage", "Italic", "Italic context menu item");
}

String WebPlatformStrategies::contextMenuItemTagUnderline()
{
    return QCoreApplication::translate("QWebPage", "Underline", "Underline context menu item");
}

String WebPlatformStrategies::contextMenuItemTagOutline()
{
    return QCoreApplication::translate("QWebPage", "Outline", "Outline context menu item");
}

String WebPlatformStrategies::contextMenuItemTagWritingDirectionMenu()
{
    return QCoreApplication::translate("QWebPage", "Direction", "Writing direction context sub-menu item");
}

String WebPlatformStrategies::contextMenuItemTagTextDirectionMenu()
{
    return QCoreApplication::translate("QWebPage", "Text Direction", "Text direction context sub-menu item");
}

String WebPlatformStrategies::contextMenuItemTagDefaultDirection()
{
    return QCoreApplication::translate("QWebPage", "Default", "Default writing direction context menu item");
}

String WebPlatformStrategies::contextMenuItemTagLeftToRight()
{
    return QCoreApplication::translate("QWebPage", "Left to Right", "Left to Right context menu item");
}

String WebPlatformStrategies::contextMenuItemTagRightToLeft()
{
    return QCoreApplication::translate("QWebPage", "Right to Left", "Right to Left context menu item");
}

String WebPlatformStrategies::contextMenuItemTagInspectElement()
{
    return QCoreApplication::translate("QWebPage", "Inspect", "Inspect Element context menu item");
}

String WebPlatformStrategies::searchMenuNoRecentSearchesText()
{
    return QCoreApplication::translate("QWebPage", "No recent searches", "Label for only item in menu that appears when clicking on the search field image, when no searches have been performed");
}

String WebPlatformStrategies::searchMenuRecentSearchesText()
{
    return QCoreApplication::translate("QWebPage", "Recent searches", "label for first item in the menu that appears when clicking on the search field image, used as embedded menu title");
}

String WebPlatformStrategies::searchMenuClearRecentSearchesText()
{
    return QCoreApplication::translate("QWebPage", "Clear recent searches", "menu item in Recent Searches menu that empties menu's contents");
}

String WebPlatformStrategies::AXWebAreaText()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::AXLinkText()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::AXListMarkerText()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::AXImageMapText()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::AXHeadingText()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::AXDefinitionListTermText()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::AXDefinitionListDefinitionText()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::AXButtonActionVerb()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::AXRadioButtonActionVerb()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::AXTextFieldActionVerb()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::AXCheckedCheckBoxActionVerb()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::AXUncheckedCheckBoxActionVerb()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::AXMenuListActionVerb()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::AXMenuListPopupActionVerb()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::AXLinkActionVerb()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::missingPluginText()
{
    return QCoreApplication::translate("QWebPage", "Missing Plug-in", "Label text to be used when a plug-in is missing");
}

String WebPlatformStrategies::crashedPluginText()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::multipleFileUploadText(unsigned)
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::unknownFileSizeText()
{
    return QCoreApplication::translate("QWebPage", "Unknown", "Unknown filesize FTP directory listing item");
}

String WebPlatformStrategies::imageTitle(const String& filename, const IntSize& size)
{
    return QCoreApplication::translate("QWebPage", "%1 (%2x%3 pixels)", "Title string for images").arg(filename).arg(size.width()).arg(size.height());
}

String WebPlatformStrategies::mediaElementLoadingStateText()
{
    return QCoreApplication::translate("QWebPage", "Loading...", "Media controller status message when the media is loading");
}

String WebPlatformStrategies::mediaElementLiveBroadcastStateText()
{
    return QCoreApplication::translate("QWebPage", "Live Broadcast", "Media controller status message when watching a live broadcast");
}

#if ENABLE(VIDEO)

String WebPlatformStrategies::localizedMediaControlElementString(const String& name)
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
    if (name == "FullscreenButton")
        return QCoreApplication::translate("QWebPage", "Fullscreen Button", "Media controller element");
    if (name == "SeekForwardButton")
        return QCoreApplication::translate("QWebPage", "Seek Forward Button", "Media controller element");
    if (name == "SeekBackButton")
        return QCoreApplication::translate("QWebPage", "Seek Back Button", "Media controller element");

    return String();
}

String WebPlatformStrategies::localizedMediaControlElementHelpText(const String& name)
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
    if (name == "FullscreenButton")
        return QCoreApplication::translate("QWebPage", "Play movie in full-screen mode", "Media controller element");
    if (name == "SeekForwardButton")
        return QCoreApplication::translate("QWebPage", "Seek quickly back", "Media controller element");
    if (name == "SeekBackButton")
        return QCoreApplication::translate("QWebPage", "Seek quickly forward", "Media controller element");

    ASSERT_NOT_REACHED();
    return String();
}

String WebPlatformStrategies::localizedMediaTimeDescription(float time)
{
    if (!isfinite(time))
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

String WebPlatformStrategies::localizedMediaControlElementString(const String& name)
{
    return String();
}

String WebPlatformStrategies::localizedMediaControlElementHelpText(const String& name)
{
    return String();
}

String WebPlatformStrategies::localizedMediaTimeDescription(float time)
{
    return String();
}

#endif // ENABLE(VIDEO)


String WebPlatformStrategies::validationMessageValueMissingText()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::validationMessageTypeMismatchText()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::validationMessagePatternMismatchText()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::validationMessageTooLongText()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::validationMessageRangeUnderflowText()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::validationMessageRangeOverflowText()
{
    notImplemented();
    return String();
}

String WebPlatformStrategies::validationMessageStepMismatchText()
{
    notImplemented();
    return String();
}


// VisitedLinkStrategy

bool WebPlatformStrategies::isLinkVisited(Page* page, LinkHash hash)
{
    return page->group().isLinkVisited(hash);
}

void WebPlatformStrategies::addVisitedLink(Page* page, LinkHash hash)
{
    page->group().addVisitedLinkHash(hash);
}
