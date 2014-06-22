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

#import "config.h"
#import "WebPageProxy.h"

#import "AttributedString.h"
#import "ColorSpaceData.h"
#import "DataReference.h"
#import "DictionaryPopupInfo.h"
#import "EditorState.h"
#import "NativeWebKeyboardEvent.h"
#import "PluginComplexTextInputState.h"
#import "PageClient.h"
#import "PageClientImpl.h"
#import "StringUtilities.h"
#import "TextChecker.h"
#import "WebPageMessages.h"
#import "WebProcessProxy.h"
#import <WebCore/DictationAlternative.h>
#import <WebCore/GraphicsLayer.h>
#import <WebCore/SharedBuffer.h>
#import <WebCore/SystemVersionMac.h>
#import <WebCore/TextAlternativeWithRange.h>
#import <WebKitSystemInterface.h>
#import <wtf/text/StringConcatenate.h>

@interface NSApplication (Details)
- (void)speakString:(NSString *)string;
@end

#define MESSAGE_CHECK(assertion) MESSAGE_CHECK_BASE(assertion, process()->connection())

using namespace WebCore;

namespace WebKit {

#if defined(__ppc__) || defined(__ppc64__)
#define PROCESSOR "PPC"
#elif defined(__i386__) || defined(__x86_64__)
#define PROCESSOR "Intel"
#else
#error Unknown architecture
#endif

static NSString *systemMarketingVersionForUserAgentString()
{
    // Use underscores instead of dots because when we first added the Mac OS X version to the user agent string
    // we were concerned about old DHTML libraries interpreting "4." as Netscape 4. That's no longer a concern for us
    // but we're sticking with the underscores for compatibility with the format used by older versions of Safari.
    return [systemMarketingVersion() stringByReplacingOccurrencesOfString:@"." withString:@"_"];
}

static String userVisibleWebKitVersionString()
{
    // If the version is 4 digits long or longer, then the first digit represents
    // the version of the OS. Our user agent string should not include this first digit,
    // so strip it off and report the rest as the version. <rdar://problem/4997547>
    NSString *fullVersion = [[NSBundle bundleForClass:NSClassFromString(@"WKView")] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey];
    NSRange nonDigitRange = [fullVersion rangeOfCharacterFromSet:[[NSCharacterSet decimalDigitCharacterSet] invertedSet]];
    if (nonDigitRange.location == NSNotFound && [fullVersion length] >= 4)
        return [fullVersion substringFromIndex:1];
    if (nonDigitRange.location != NSNotFound && nonDigitRange.location >= 4)
        return [fullVersion substringFromIndex:1];
    return fullVersion;
}

String WebPageProxy::standardUserAgent(const String& applicationNameForUserAgent)
{
    DEFINE_STATIC_LOCAL(String, osVersion, (systemMarketingVersionForUserAgentString()));
    DEFINE_STATIC_LOCAL(String, webKitVersion, (userVisibleWebKitVersionString()));

    if (applicationNameForUserAgent.isEmpty())
        return makeString("Mozilla/5.0 (Macintosh; " PROCESSOR " Mac OS X ", osVersion, ") AppleWebKit/", webKitVersion, " (KHTML, like Gecko)");
    return makeString("Mozilla/5.0 (Macintosh; " PROCESSOR " Mac OS X ", osVersion, ") AppleWebKit/", webKitVersion, " (KHTML, like Gecko) ", applicationNameForUserAgent);
}

void WebPageProxy::getIsSpeaking(bool& isSpeaking)
{
    isSpeaking = [NSApp isSpeaking];
}

void WebPageProxy::speak(const String& string)
{
    [NSApp speakString:nsStringFromWebCoreString(string)];
}

void WebPageProxy::stopSpeaking()
{
    [NSApp stopSpeaking:nil];
}

void WebPageProxy::searchWithSpotlight(const String& string)
{
    [[NSWorkspace sharedWorkspace] showSearchResultsForQueryString:nsStringFromWebCoreString(string)];
}
    
void WebPageProxy::searchTheWeb(const String& string)
{
    NSPasteboard *pasteboard = [NSPasteboard pasteboardWithUniqueName];
    [pasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
    [pasteboard setString:string forType:NSStringPboardType];
    
    NSPerformService(@"Search With %WebSearchProvider@", pasteboard);
}
    
CGContextRef WebPageProxy::containingWindowGraphicsContext()
{
    return m_pageClient->containingWindowGraphicsContext();
}

void WebPageProxy::updateWindowIsVisible(bool windowIsVisible)
{
    if (!isValid())
        return;
    process()->send(Messages::WebPage::SetWindowIsVisible(windowIsVisible), m_pageID);
}

void WebPageProxy::windowAndViewFramesChanged(const FloatRect& viewFrameInWindowCoordinates, const FloatPoint& accessibilityViewCoordinates)
{
    if (!isValid())
        return;

    // In case the UI client overrides getWindowFrame(), we call it here to make sure we send the appropriate window frame.
    FloatRect windowFrameInScreenCoordinates = m_uiClient.windowFrame(this);
    FloatRect windowFrameInUnflippedScreenCoordinates = m_pageClient->convertToUserSpace(windowFrameInScreenCoordinates);

    process()->send(Messages::WebPage::WindowAndViewFramesChanged(windowFrameInScreenCoordinates, windowFrameInUnflippedScreenCoordinates, viewFrameInWindowCoordinates, accessibilityViewCoordinates), m_pageID);
}

void WebPageProxy::viewExposedRectChanged(const FloatRect& exposedRect, bool clipsToExposedRect)
{
    if (!isValid())
        return;

    m_exposedRect = exposedRect;
    m_clipsToExposedRect = clipsToExposedRect;

    if (!m_exposedRectChangedTimer.isActive())
        m_exposedRectChangedTimer.startOneShot(0);
}

void WebPageProxy::exposedRectChangedTimerFired(Timer<WebPageProxy>*)
{
    if (!isValid())
        return;

    if (m_exposedRect == m_lastSentExposedRect && m_clipsToExposedRect == m_lastSentClipsToExposedRect)
        return;

    process()->send(Messages::WebPage::ViewExposedRectChanged(m_exposedRect, m_clipsToExposedRect), m_pageID);
    m_lastSentExposedRect = m_exposedRect;
    m_lastSentClipsToExposedRect = m_clipsToExposedRect;
}

void WebPageProxy::setMainFrameIsScrollable(bool isScrollable)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::SetMainFrameIsScrollable(isScrollable), m_pageID);
}

void WebPageProxy::setComposition(const String& text, Vector<CompositionUnderline> underlines, uint64_t selectionStart, uint64_t selectionEnd, uint64_t replacementRangeStart, uint64_t replacementRangeEnd)
{
    if (!isValid()) {
        // If this fails, we should call -discardMarkedText on input context to notify the input method.
        // This will happen naturally later, as part of reloading the page.
        return;
    }

    process()->sendSync(Messages::WebPage::SetComposition(text, underlines, selectionStart, selectionEnd, replacementRangeStart, replacementRangeEnd), Messages::WebPage::SetComposition::Reply(m_editorState), m_pageID);
}

void WebPageProxy::confirmComposition()
{
    if (!isValid())
        return;

    process()->sendSync(Messages::WebPage::ConfirmComposition(), Messages::WebPage::ConfirmComposition::Reply(m_editorState), m_pageID);
}

void WebPageProxy::cancelComposition()
{
    if (!isValid())
        return;

    process()->sendSync(Messages::WebPage::CancelComposition(), Messages::WebPage::ConfirmComposition::Reply(m_editorState), m_pageID);
}

bool WebPageProxy::insertText(const String& text, uint64_t replacementRangeStart, uint64_t replacementRangeEnd)
{
    if (!isValid())
        return true;

    bool handled = true;
    process()->sendSync(Messages::WebPage::InsertText(text, replacementRangeStart, replacementRangeEnd), Messages::WebPage::InsertText::Reply(handled, m_editorState), m_pageID);
    m_temporarilyClosedComposition = false;

    return handled;
}

bool WebPageProxy::insertDictatedText(const String& text, uint64_t replacementRangeStart, uint64_t replacementRangeEnd, const Vector<TextAlternativeWithRange>& dictationAlternativesWithRange)
{
#if USE(DICTATION_ALTERNATIVES)
    if (dictationAlternativesWithRange.isEmpty())
        return insertText(text, replacementRangeStart, replacementRangeEnd);

    if (!isValid())
        return true;

    Vector<DictationAlternative> dictationAlternatives;

    for (size_t i = 0; i < dictationAlternativesWithRange.size(); ++i) {
        const TextAlternativeWithRange& alternativeWithRange = dictationAlternativesWithRange[i];
        uint64_t dictationContext = m_pageClient->addDictationAlternatives(alternativeWithRange.alternatives);
        if (dictationContext)
            dictationAlternatives.append(DictationAlternative(alternativeWithRange.range.location, alternativeWithRange.range.length, dictationContext));
    }

    if (dictationAlternatives.isEmpty())
        return insertText(text, replacementRangeStart, replacementRangeEnd);

    bool handled = true;
    process()->sendSync(Messages::WebPage::InsertDictatedText(text, replacementRangeStart, replacementRangeEnd, dictationAlternatives), Messages::WebPage::InsertDictatedText::Reply(handled, m_editorState), m_pageID);
    return handled;
#else
    return insertText(text, replacementRangeStart, replacementRangeEnd);
#endif
}

void WebPageProxy::getMarkedRange(uint64_t& location, uint64_t& length)
{
    location = NSNotFound;
    length = 0;

    if (!isValid())
        return;

    process()->sendSync(Messages::WebPage::GetMarkedRange(), Messages::WebPage::GetMarkedRange::Reply(location, length), m_pageID);
}

void WebPageProxy::getSelectedRange(uint64_t& location, uint64_t& length)
{
    location = NSNotFound;
    length = 0;

    if (!isValid())
        return;

    process()->sendSync(Messages::WebPage::GetSelectedRange(), Messages::WebPage::GetSelectedRange::Reply(location, length), m_pageID);
}

void WebPageProxy::getAttributedSubstringFromRange(uint64_t location, uint64_t length, AttributedString& result)
{
    if (!isValid())
        return;
    process()->sendSync(Messages::WebPage::GetAttributedSubstringFromRange(location, length), Messages::WebPage::GetAttributedSubstringFromRange::Reply(result), m_pageID);
}

uint64_t WebPageProxy::characterIndexForPoint(const IntPoint point)
{
    if (!isValid())
        return 0;

    uint64_t result = 0;
    process()->sendSync(Messages::WebPage::CharacterIndexForPoint(point), Messages::WebPage::CharacterIndexForPoint::Reply(result), m_pageID);
    return result;
}

IntRect WebPageProxy::firstRectForCharacterRange(uint64_t location, uint64_t length)
{
    if (!isValid())
        return IntRect();

    IntRect resultRect;
    process()->sendSync(Messages::WebPage::FirstRectForCharacterRange(location, length), Messages::WebPage::FirstRectForCharacterRange::Reply(resultRect), m_pageID);
    return resultRect;
}

bool WebPageProxy::executeKeypressCommands(const Vector<WebCore::KeypressCommand>& commands)
{
    if (!isValid())
        return false;

    bool result = false;
    process()->sendSync(Messages::WebPage::ExecuteKeypressCommands(commands), Messages::WebPage::ExecuteKeypressCommands::Reply(result, m_editorState), m_pageID);
    return result;
}

String WebPageProxy::stringSelectionForPasteboard()
{
    String value;
    if (!isValid())
        return value;
    
    const double messageTimeout = 20;
    process()->sendSync(Messages::WebPage::GetStringSelectionForPasteboard(), Messages::WebPage::GetStringSelectionForPasteboard::Reply(value), m_pageID, messageTimeout);
    return value;
}

PassRefPtr<WebCore::SharedBuffer> WebPageProxy::dataSelectionForPasteboard(const String& pasteboardType)
{
    if (!isValid())
        return 0;
    SharedMemory::Handle handle;
    uint64_t size = 0;
    const double messageTimeout = 20;
    process()->sendSync(Messages::WebPage::GetDataSelectionForPasteboard(pasteboardType),
                                                Messages::WebPage::GetDataSelectionForPasteboard::Reply(handle, size), m_pageID, messageTimeout);
    if (handle.isNull())
        return 0;
    RefPtr<SharedMemory> sharedMemoryBuffer = SharedMemory::create(handle, SharedMemory::ReadOnly);
    return SharedBuffer::create(static_cast<unsigned char *>(sharedMemoryBuffer->data()), size);
}

bool WebPageProxy::readSelectionFromPasteboard(const String& pasteboardName)
{
    if (!isValid())
        return false;

    bool result = false;
    const double messageTimeout = 20;
    process()->sendSync(Messages::WebPage::ReadSelectionFromPasteboard(pasteboardName), Messages::WebPage::ReadSelectionFromPasteboard::Reply(result), m_pageID, messageTimeout);
    return result;
}

#if ENABLE(DRAG_SUPPORT)
void WebPageProxy::setDragImage(const WebCore::IntPoint& clientPosition, const ShareableBitmap::Handle& dragImageHandle, bool isLinkDrag)
{
    RefPtr<ShareableBitmap> dragImage = ShareableBitmap::create(dragImageHandle);
    if (!dragImage)
        return;
    
    m_pageClient->setDragImage(clientPosition, dragImage.release(), isLinkDrag);
}

void WebPageProxy::setPromisedData(const String& pasteboardName, const SharedMemory::Handle& imageHandle, uint64_t imageSize, const String& filename, const String& extension,
                                   const String& title, const String& url, const String& visibleURL, const SharedMemory::Handle& archiveHandle, uint64_t archiveSize)
{
    RefPtr<SharedMemory> sharedMemoryImage = SharedMemory::create(imageHandle, SharedMemory::ReadOnly);
    RefPtr<SharedBuffer> imageBuffer = SharedBuffer::create(static_cast<unsigned char*>(sharedMemoryImage->data()), imageSize);
    RefPtr<SharedBuffer> archiveBuffer;
    
    if (!archiveHandle.isNull()) {
        RefPtr<SharedMemory> sharedMemoryArchive = SharedMemory::create(archiveHandle, SharedMemory::ReadOnly);;
        archiveBuffer = SharedBuffer::create(static_cast<unsigned char*>(sharedMemoryArchive->data()), archiveSize);
    }
    m_pageClient->setPromisedData(pasteboardName, imageBuffer, filename, extension, title, url, visibleURL, archiveBuffer);
}
#endif

void WebPageProxy::performDictionaryLookupAtLocation(const WebCore::FloatPoint& point)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::PerformDictionaryLookupAtLocation(point), m_pageID); 
}

void WebPageProxy::interpretQueuedKeyEvent(const EditorState& state, bool& handled, Vector<WebCore::KeypressCommand>& commands)
{
    m_editorState = state;
    handled = m_pageClient->interpretKeyEvent(m_keyEventQueue.first(), commands);
}

// Complex text input support for plug-ins.
void WebPageProxy::sendComplexTextInputToPlugin(uint64_t pluginComplexTextInputIdentifier, const String& textInput)
{
    if (!isValid())
        return;
    
    process()->send(Messages::WebPage::SendComplexTextInputToPlugin(pluginComplexTextInputIdentifier, textInput), m_pageID);
}

void WebPageProxy::uppercaseWord()
{
    process()->send(Messages::WebPage::UppercaseWord(), m_pageID);
}

void WebPageProxy::lowercaseWord()
{
    process()->send(Messages::WebPage::LowercaseWord(), m_pageID);
}

void WebPageProxy::capitalizeWord()
{
    process()->send(Messages::WebPage::CapitalizeWord(), m_pageID);
}

void WebPageProxy::setSmartInsertDeleteEnabled(bool isSmartInsertDeleteEnabled)
{
    if (m_isSmartInsertDeleteEnabled == isSmartInsertDeleteEnabled)
        return;

    TextChecker::setSmartInsertDeleteEnabled(isSmartInsertDeleteEnabled);
    m_isSmartInsertDeleteEnabled = isSmartInsertDeleteEnabled;
    process()->send(Messages::WebPage::SetSmartInsertDeleteEnabled(isSmartInsertDeleteEnabled), m_pageID);
}

void WebPageProxy::didPerformDictionaryLookup(const AttributedString& text, const DictionaryPopupInfo& dictionaryPopupInfo)
{
    m_pageClient->didPerformDictionaryLookup(text, dictionaryPopupInfo);
}
    
void WebPageProxy::registerWebProcessAccessibilityToken(const CoreIPC::DataReference& data)
{
    m_pageClient->accessibilityWebProcessTokenReceived(data);
}    
    
void WebPageProxy::makeFirstResponder()
{
    m_pageClient->makeFirstResponder();
}

ColorSpaceData WebPageProxy::colorSpace()
{
    return m_pageClient->colorSpace();
}

void WebPageProxy::registerUIProcessAccessibilityTokens(const CoreIPC::DataReference& elementToken, const CoreIPC::DataReference& windowToken)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::RegisterUIProcessAccessibilityTokens(elementToken, windowToken), m_pageID);
}

void WebPageProxy::pluginFocusOrWindowFocusChanged(uint64_t pluginComplexTextInputIdentifier, bool pluginHasFocusAndWindowHasFocus)
{
    m_pageClient->pluginFocusOrWindowFocusChanged(pluginComplexTextInputIdentifier, pluginHasFocusAndWindowHasFocus);
}

void WebPageProxy::setPluginComplexTextInputState(uint64_t pluginComplexTextInputIdentifier, uint64_t pluginComplexTextInputState)
{
    MESSAGE_CHECK(isValidPluginComplexTextInputState(pluginComplexTextInputState));

    m_pageClient->setPluginComplexTextInputState(pluginComplexTextInputIdentifier, static_cast<PluginComplexTextInputState>(pluginComplexTextInputState));
}

void WebPageProxy::executeSavedCommandBySelector(const String& selector, bool& handled)
{
    MESSAGE_CHECK(isValidKeypressCommandName(selector));

    handled = m_pageClient->executeSavedCommandBySelector(selector);
}

bool WebPageProxy::shouldDelayWindowOrderingForEvent(const WebKit::WebMouseEvent& event)
{
    if (!process()->isValid())
        return false;

    bool result = false;
    const double messageTimeout = 3;
    process()->sendSync(Messages::WebPage::ShouldDelayWindowOrderingEvent(event), Messages::WebPage::ShouldDelayWindowOrderingEvent::Reply(result), m_pageID, messageTimeout);
    return result;
}

bool WebPageProxy::acceptsFirstMouse(int eventNumber, const WebKit::WebMouseEvent& event)
{
    if (!isValid())
        return false;

    bool result = false;
    const double messageTimeout = 3;
    process()->sendSync(Messages::WebPage::AcceptsFirstMouse(eventNumber, event), Messages::WebPage::AcceptsFirstMouse::Reply(result), m_pageID, messageTimeout);
    return result;
}

WKView* WebPageProxy::wkView() const
{
    return m_pageClient->wkView();
}

void WebPageProxy::intrinsicContentSizeDidChange(const IntSize& intrinsicContentSize)
{
    m_pageClient->intrinsicContentSizeDidChange(intrinsicContentSize);
}

void WebPageProxy::setAcceleratedCompositingRootLayer(const GraphicsLayer* rootLayer)
{
    m_pageClient->setAcceleratedCompositingRootLayer(rootLayer->platformLayer());
}

static NSString *temporaryPDFDirectoryPath()
{
    static NSString *temporaryPDFDirectoryPath;

    if (!temporaryPDFDirectoryPath) {
        NSString *temporaryDirectoryTemplate = [NSTemporaryDirectory() stringByAppendingPathComponent:@"WebKitPDFs-XXXXXX"];
        CString templateRepresentation = [temporaryDirectoryTemplate fileSystemRepresentation];

        if (mkdtemp(templateRepresentation.mutableData()))
            temporaryPDFDirectoryPath = [[[NSFileManager defaultManager] stringWithFileSystemRepresentation:templateRepresentation.data() length:templateRepresentation.length()] copy];
    }

    return temporaryPDFDirectoryPath;
}

static NSString *pathToPDFOnDisk(const String& suggestedFilename)
{
    NSString *pdfDirectoryPath = temporaryPDFDirectoryPath();
    if (!pdfDirectoryPath) {
        WTFLogAlways("Cannot create temporary PDF download directory.");
        return nil;
    }

    NSString *path = [pdfDirectoryPath stringByAppendingPathComponent:suggestedFilename];

    NSFileManager *fileManager = [NSFileManager defaultManager];
    if ([fileManager fileExistsAtPath:path]) {
        NSString *pathTemplatePrefix = [pdfDirectoryPath stringByAppendingPathComponent:@"XXXXXX-"];
        NSString *pathTemplate = [pathTemplatePrefix stringByAppendingString:suggestedFilename];
        CString pathTemplateRepresentation = [pathTemplate fileSystemRepresentation];

        int fd = mkstemps(pathTemplateRepresentation.mutableData(), pathTemplateRepresentation.length() - strlen([pathTemplatePrefix fileSystemRepresentation]) + 1);
        if (fd < 0) {
            WTFLogAlways("Cannot create PDF file in the temporary directory (%s).", suggestedFilename.utf8().data());
            return nil;
        }

        close(fd);
        path = [fileManager stringWithFileSystemRepresentation:pathTemplateRepresentation.data() length:pathTemplateRepresentation.length()];
    }

    return path;
}

void WebPageProxy::savePDFToTemporaryFolderAndOpenWithNativeApplicationRaw(const String& suggestedFilename, const String& originatingURLString, const uint8_t* data, unsigned long size, const String& pdfUUID)
{
    // FIXME: Write originatingURLString to the file's originating URL metadata (perhaps WKSetMetadataURL?).
    UNUSED_PARAM(originatingURLString);

    if (!suggestedFilename.endsWith(".pdf", false)) {
        WTFLogAlways("Cannot save file without .pdf extension to the temporary directory.");
        return;
    }

    if (!size) {
        WTFLogAlways("Cannot save empty PDF file to the temporary directory.");
        return;
    }

    NSString *nsPath = pathToPDFOnDisk(suggestedFilename);

    if (!nsPath)
        return;

    RetainPtr<NSNumber> permissions = adoptNS([[NSNumber alloc] initWithInt:S_IRUSR]);
    RetainPtr<NSDictionary> fileAttributes = adoptNS([[NSDictionary alloc] initWithObjectsAndKeys:permissions.get(), NSFilePosixPermissions, nil]);
    RetainPtr<NSData> nsData = adoptNS([[NSData alloc] initWithBytesNoCopy:(void*)data length:size freeWhenDone:NO]);

    if (![[NSFileManager defaultManager] createFileAtPath:nsPath contents:nsData.get() attributes:fileAttributes.get()]) {
        WTFLogAlways("Cannot create PDF file in the temporary directory (%s).", suggestedFilename.utf8().data());
        return;
    }

    m_temporaryPDFFiles.add(pdfUUID, nsPath);

    [[NSWorkspace sharedWorkspace] openFile:nsPath];
}

void WebPageProxy::savePDFToTemporaryFolderAndOpenWithNativeApplication(const String& suggestedFilename, const String& originatingURLString, const CoreIPC::DataReference& data, const String& pdfUUID)
{
    if (data.isEmpty()) {
        WTFLogAlways("Cannot save empty PDF file to the temporary directory.");
        return;
    }

    savePDFToTemporaryFolderAndOpenWithNativeApplicationRaw(suggestedFilename, originatingURLString, data.data(), data.size(), pdfUUID);
}

void WebPageProxy::openPDFFromTemporaryFolderWithNativeApplication(const String& pdfUUID)
{
    String pdfFilename = m_temporaryPDFFiles.get(pdfUUID);

    if (!pdfFilename.endsWith(".pdf", false))
        return;

    [[NSWorkspace sharedWorkspace] openFile:pdfFilename];
}

} // namespace WebKit
