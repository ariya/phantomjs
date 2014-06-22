/*
 * Copyright (C) 2010, 2011, 2012 Apple Inc. All rights reserved.
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
#import "WebPage.h"

#import "AttributedString.h"
#import "DataReference.h"
#import "EditorState.h"
#import "PDFKitImports.h"
#import "PageBanner.h"
#import "PluginView.h"
#import "PrintInfo.h"
#import "WKAccessibilityWebPageObject.h"
#import "WebCoreArgumentCoders.h"
#import "WebEvent.h"
#import "WebEventConversion.h"
#import "WebFrame.h"
#import "WebImage.h"
#import "WebInspector.h"
#import "WebPageProxyMessages.h"
#import "WebPreferencesStore.h"
#import "WebProcess.h"
#import <PDFKit/PDFKit.h>
#import <QuartzCore/QuartzCore.h>
#import <WebCore/AXObjectCache.h>
#import <WebCore/EventHandler.h>
#import <WebCore/FocusController.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/FrameView.h>
#import <WebCore/HTMLConverter.h>
#import <WebCore/HitTestResult.h>
#import <WebCore/KeyboardEvent.h>
#import <WebCore/MIMETypeRegistry.h>
#import <WebCore/NetworkingContext.h>
#import <WebCore/Page.h>
#import <WebCore/PlatformKeyboardEvent.h>
#import <WebCore/PluginDocument.h>
#import <WebCore/RenderObject.h>
#import <WebCore/RenderStyle.h>
#import <WebCore/ResourceHandle.h>
#import <WebCore/ScrollView.h>
#import <WebCore/StyleInheritedData.h>
#import <WebCore/TextIterator.h>
#import <WebCore/VisibleUnits.h>
#import <WebCore/WindowsKeyboardCodes.h>
#import <WebKitSystemInterface.h>

using namespace WebCore;

namespace WebKit {

static PassRefPtr<Range> convertToRange(Frame*, NSRange);

void WebPage::platformInitialize()
{
#if USE(CFNETWORK)
    m_page->addSchedulePair(SchedulePair::create([[NSRunLoop currentRunLoop] getCFRunLoop], kCFRunLoopCommonModes));
#else
    m_page->addSchedulePair(SchedulePair::create([NSRunLoop currentRunLoop], kCFRunLoopCommonModes));
#endif

    WKAccessibilityWebPageObject* mockAccessibilityElement = [[[WKAccessibilityWebPageObject alloc] init] autorelease];

    // Get the pid for the starting process.
    pid_t pid = WebProcess::shared().presenterApplicationPid();    
    WKAXInitializeElementWithPresenterPid(mockAccessibilityElement, pid);
    [mockAccessibilityElement setWebPage:this];
    
    // send data back over
    NSData* remoteToken = (NSData *)WKAXRemoteTokenForElement(mockAccessibilityElement); 
    CoreIPC::DataReference dataToken = CoreIPC::DataReference(reinterpret_cast<const uint8_t*>([remoteToken bytes]), [remoteToken length]);
    send(Messages::WebPageProxy::RegisterWebProcessAccessibilityToken(dataToken));
    m_mockAccessibilityElement = mockAccessibilityElement;
}

NSObject *WebPage::accessibilityObjectForMainFramePlugin()
{
    Frame* frame = m_page->mainFrame();
    if (!frame)
        return 0;

    if (PluginView* pluginView = pluginViewForFrame(frame))
        return pluginView->accessibilityObject();

    return 0;
}

void WebPage::platformPreferencesDidChange(const WebPreferencesStore& store)
{
    if (WebInspector* inspector = this->inspector())
        inspector->setInspectorUsesWebKitUserInterface(store.getBoolValueForKey(WebPreferencesKey::inspectorUsesWebKitUserInterfaceKey()));
}

bool WebPage::shouldUsePDFPlugin() const
{
    return pdfPluginEnabled() && classFromPDFKit(@"PDFLayerController");
}

typedef HashMap<String, String> SelectorNameMap;

// Map selectors into Editor command names.
// This is not needed for any selectors that have the same name as the Editor command.
static const SelectorNameMap* createSelectorExceptionMap()
{
    SelectorNameMap* map = new HashMap<String, String>;

    map->add("insertNewlineIgnoringFieldEditor:", "InsertNewline");
    map->add("insertParagraphSeparator:", "InsertNewline");
    map->add("insertTabIgnoringFieldEditor:", "InsertTab");
    map->add("pageDown:", "MovePageDown");
    map->add("pageDownAndModifySelection:", "MovePageDownAndModifySelection");
    map->add("pageUp:", "MovePageUp");
    map->add("pageUpAndModifySelection:", "MovePageUpAndModifySelection");

    return map;
}

static String commandNameForSelectorName(const String& selectorName)
{
    // Check the exception map first.
    static const SelectorNameMap* exceptionMap = createSelectorExceptionMap();
    SelectorNameMap::const_iterator it = exceptionMap->find(selectorName);
    if (it != exceptionMap->end())
        return it->value;

    // Remove the trailing colon.
    // No need to capitalize the command name since Editor command names are not case sensitive.
    size_t selectorNameLength = selectorName.length();
    if (selectorNameLength < 2 || selectorName[selectorNameLength - 1] != ':')
        return String();
    return selectorName.left(selectorNameLength - 1);
}

static Frame* frameForEvent(KeyboardEvent* event)
{
    Node* node = event->target()->toNode();
    ASSERT(node);
    Frame* frame = node->document()->frame();
    ASSERT(frame);
    return frame;
}

bool WebPage::executeKeypressCommandsInternal(const Vector<WebCore::KeypressCommand>& commands, KeyboardEvent* event)
{
    Frame* frame = frameForEvent(event);
    ASSERT(frame->page() == corePage());

    bool eventWasHandled = false;
    for (size_t i = 0; i < commands.size(); ++i) {
        if (commands[i].commandName == "insertText:") {
            ASSERT(!frame->editor().hasComposition());

            if (!frame->editor().canEdit())
                continue;

            // An insertText: might be handled by other responders in the chain if we don't handle it.
            // One example is space bar that results in scrolling down the page.
            eventWasHandled |= frame->editor().insertText(commands[i].text, event);
        } else {
            Editor::Command command = frame->editor().command(commandNameForSelectorName(commands[i].commandName));
            if (command.isSupported()) {
                bool commandExecutedByEditor = command.execute(event);
                eventWasHandled |= commandExecutedByEditor;
                if (!commandExecutedByEditor) {
                    bool performedNonEditingBehavior = event->keyEvent()->type() == PlatformEvent::RawKeyDown && performNonEditingBehaviorForSelector(commands[i].commandName);
                    eventWasHandled |= performedNonEditingBehavior;
                }
            } else {
                bool commandWasHandledByUIProcess = false;
                WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebPageProxy::ExecuteSavedCommandBySelector(commands[i].commandName), 
                    Messages::WebPageProxy::ExecuteSavedCommandBySelector::Reply(commandWasHandledByUIProcess), m_pageID);
                eventWasHandled |= commandWasHandledByUIProcess;
            }
        }
    }
    return eventWasHandled;
}

bool WebPage::handleEditingKeyboardEvent(KeyboardEvent* event, bool saveCommands)
{
    ASSERT(!saveCommands || event->keypressCommands().isEmpty()); // Save commands once for each event.

    Frame* frame = frameForEvent(event);
    
    const PlatformKeyboardEvent* platformEvent = event->keyEvent();
    if (!platformEvent)
        return false;
    Vector<KeypressCommand>& commands = event->keypressCommands();

    if ([platformEvent->macEvent() type] == NSFlagsChanged)
        return false;

    bool eventWasHandled = false;
    
    if (saveCommands) {
        KeyboardEvent* oldEvent = m_keyboardEventBeingInterpreted;
        m_keyboardEventBeingInterpreted = event;
        bool sendResult = WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebPageProxy::InterpretQueuedKeyEvent(editorState()), 
            Messages::WebPageProxy::InterpretQueuedKeyEvent::Reply(eventWasHandled, commands), m_pageID);
        m_keyboardEventBeingInterpreted = oldEvent;
        if (!sendResult)
            return false;

        // An input method may make several actions per keypress. For example, pressing Return with Korean IM both confirms it and sends a newline.
        // IM-like actions are handled immediately (so the return value from UI process is true), but there are saved commands that
        // should be handled like normal text input after DOM event dispatch.
        if (!event->keypressCommands().isEmpty())
            return false;
    } else {
        // Are there commands that could just cause text insertion if executed via Editor?
        // WebKit doesn't have enough information about mode to decide how they should be treated, so we leave it upon WebCore
        // to either handle them immediately (e.g. Tab that changes focus) or let a keypress event be generated
        // (e.g. Tab that inserts a Tab character, or Enter).
        bool haveTextInsertionCommands = false;
        for (size_t i = 0; i < commands.size(); ++i) {
            if (frame->editor().command(commandNameForSelectorName(commands[i].commandName)).isTextInsertion())
                haveTextInsertionCommands = true;
        }
        // If there are no text insertion commands, default keydown handler is the right time to execute the commands.
        // Keypress (Char event) handler is the latest opportunity to execute.
        if (!haveTextInsertionCommands || platformEvent->type() == PlatformEvent::Char) {
            eventWasHandled = executeKeypressCommandsInternal(event->keypressCommands(), event);
            event->keypressCommands().clear();
        }
    }
    return eventWasHandled;
}

void WebPage::sendComplexTextInputToPlugin(uint64_t pluginComplexTextInputIdentifier, const String& textInput)
{
    for (HashSet<PluginView*>::const_iterator it = m_pluginViews.begin(), end = m_pluginViews.end(); it != end; ++it) {
        if ((*it)->sendComplexTextInput(pluginComplexTextInputIdentifier, textInput))
            break;
    }
}

void WebPage::setComposition(const String& text, Vector<CompositionUnderline> underlines, uint64_t selectionStart, uint64_t selectionEnd, uint64_t replacementRangeStart, uint64_t replacementRangeEnd, EditorState& newState)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();

    if (frame->selection()->isContentEditable()) {
        RefPtr<Range> replacementRange;
        if (replacementRangeStart != NSNotFound) {
            replacementRange = convertToRange(frame, NSMakeRange(replacementRangeStart, replacementRangeEnd - replacementRangeStart));
            frame->selection()->setSelection(VisibleSelection(replacementRange.get(), SEL_DEFAULT_AFFINITY));
        }

        frame->editor().setComposition(text, underlines, selectionStart, selectionEnd);
    }

    newState = editorState();
}

void WebPage::confirmComposition(EditorState& newState)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();

    frame->editor().confirmComposition();

    newState = editorState();
}

void WebPage::cancelComposition(EditorState& newState)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();

    frame->editor().cancelComposition();

    newState = editorState();
}

void WebPage::insertText(const String& text, uint64_t replacementRangeStart, uint64_t replacementRangeEnd, bool& handled, EditorState& newState)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();

    if (replacementRangeStart != NSNotFound) {
        RefPtr<Range> replacementRange = convertToRange(frame, NSMakeRange(replacementRangeStart, replacementRangeEnd - replacementRangeStart));
        if (replacementRange)
            frame->selection()->setSelection(VisibleSelection(replacementRange.get(), SEL_DEFAULT_AFFINITY));
    }

    if (!frame->editor().hasComposition()) {
        // An insertText: might be handled by other responders in the chain if we don't handle it.
        // One example is space bar that results in scrolling down the page.
        handled = frame->editor().insertText(text, m_keyboardEventBeingInterpreted);
    } else {
        handled = true;
        frame->editor().confirmComposition(text);
    }

    newState = editorState();
}

void WebPage::insertDictatedText(const String& text, uint64_t replacementRangeStart, uint64_t replacementRangeEnd, const Vector<WebCore::DictationAlternative>& dictationAlternativeLocations, bool& handled, EditorState& newState)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();

    if (replacementRangeStart != NSNotFound) {
        RefPtr<Range> replacementRange = convertToRange(frame, NSMakeRange(replacementRangeStart, replacementRangeEnd - replacementRangeStart));
        if (replacementRange)
            frame->selection()->setSelection(VisibleSelection(replacementRange.get(), SEL_DEFAULT_AFFINITY));
    }

    ASSERT(!frame->editor().hasComposition());
    handled = frame->editor().insertDictatedText(text, dictationAlternativeLocations, m_keyboardEventBeingInterpreted);
    newState = editorState();
}

void WebPage::getMarkedRange(uint64_t& location, uint64_t& length)
{
    location = NSNotFound;
    length = 0;
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    RefPtr<Range> range = frame->editor().compositionRange();
    size_t locationSize;
    size_t lengthSize;
    if (range && TextIterator::getLocationAndLengthFromRange(frame->selection()->rootEditableElementOrDocumentElement(), range.get(), locationSize, lengthSize)) {
        location = static_cast<uint64_t>(locationSize);
        length = static_cast<uint64_t>(lengthSize);
    }
}

void WebPage::getSelectedRange(uint64_t& location, uint64_t& length)
{
    location = NSNotFound;
    length = 0;
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    size_t locationSize;
    size_t lengthSize;
    RefPtr<Range> range = frame->selection()->toNormalizedRange();
    if (range && TextIterator::getLocationAndLengthFromRange(frame->selection()->rootEditableElementOrDocumentElement(), range.get(), locationSize, lengthSize)) {
        location = static_cast<uint64_t>(locationSize);
        length = static_cast<uint64_t>(lengthSize);
    }
}

void WebPage::getAttributedSubstringFromRange(uint64_t location, uint64_t length, AttributedString& result)
{
    NSRange nsRange = NSMakeRange(location, length - location);

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    if (frame->selection()->isNone() || !frame->selection()->isContentEditable() || frame->selection()->isInPasswordField())
        return;

    RefPtr<Range> range = convertToRange(frame, nsRange);
    if (!range)
        return;

    result.string = [WebHTMLConverter editingAttributedStringFromRange:range.get()];
    NSAttributedString* attributedString = result.string.get();
    
    // [WebHTMLConverter editingAttributedStringFromRange:] insists on inserting a trailing 
    // whitespace at the end of the string which breaks the ATOK input method.  <rdar://problem/5400551>
    // To work around this we truncate the resultant string to the correct length.
    if ([attributedString length] > nsRange.length) {
        ASSERT([attributedString length] == nsRange.length + 1);
        ASSERT([[attributedString string] characterAtIndex:nsRange.length] == '\n' || [[attributedString string] characterAtIndex:nsRange.length] == ' ');
        result.string = [attributedString attributedSubstringFromRange:NSMakeRange(0, nsRange.length)];
    }
}

void WebPage::characterIndexForPoint(IntPoint point, uint64_t& index)
{
    index = NSNotFound;
    Frame* frame = m_page->mainFrame();
    if (!frame)
        return;

    HitTestResult result = frame->eventHandler()->hitTestResultAtPoint(point);
    frame = result.innerNonSharedNode() ? result.innerNodeFrame() : m_page->focusController()->focusedOrMainFrame();
    
    RefPtr<Range> range = frame->rangeForPoint(result.roundedPointInInnerNodeFrame());
    if (!range)
        return;

    size_t location;
    size_t length;
    if (TextIterator::getLocationAndLengthFromRange(frame->selection()->rootEditableElementOrDocumentElement(), range.get(), location, length))
        index = static_cast<uint64_t>(location);
}

PassRefPtr<Range> convertToRange(Frame* frame, NSRange nsrange)
{
    if (nsrange.location > INT_MAX)
        return 0;
    if (nsrange.length > INT_MAX || nsrange.location + nsrange.length > INT_MAX)
        nsrange.length = INT_MAX - nsrange.location;
        
    // our critical assumption is that we are only called by input methods that
    // concentrate on a given area containing the selection
    // We have to do this because of text fields and textareas. The DOM for those is not
    // directly in the document DOM, so serialization is problematic. Our solution is
    // to use the root editable element of the selection start as the positional base.
    // That fits with AppKit's idea of an input context.
    return TextIterator::rangeFromLocationAndLength(frame->selection()->rootEditableElementOrDocumentElement(), nsrange.location, nsrange.length);
}
    
void WebPage::firstRectForCharacterRange(uint64_t location, uint64_t length, WebCore::IntRect& resultRect)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    resultRect.setLocation(IntPoint(0, 0));
    resultRect.setSize(IntSize(0, 0));
    
    RefPtr<Range> range = convertToRange(frame, NSMakeRange(location, length));
    if (!range)
        return;
    
    ASSERT(range->startContainer());
    ASSERT(range->endContainer());
     
    IntRect rect = frame->editor().firstRectForRange(range.get());
    resultRect = frame->view()->contentsToWindow(rect);
}

void WebPage::executeKeypressCommands(const Vector<WebCore::KeypressCommand>& commands, bool& handled, EditorState& newState)
{
    handled = executeKeypressCommandsInternal(commands, m_keyboardEventBeingInterpreted);
    newState = editorState();
}

static bool isPositionInRange(const VisiblePosition& position, Range* range)
{
    RefPtr<Range> positionRange = makeRange(position, position);

    ExceptionCode ec = 0;
    range->compareBoundaryPoints(Range::START_TO_START, positionRange.get(), ec);
    if (ec)
        return false;

    if (!range->isPointInRange(positionRange->startContainer(), positionRange->startOffset(), ec))
        return false;
    if (ec)
        return false;

    return true;
}

static bool shouldUseSelection(const VisiblePosition& position, const VisibleSelection& selection)
{
    if (!selection.isRange())
        return false;

    RefPtr<Range> selectedRange = selection.toNormalizedRange();
    if (!selectedRange)
        return false;

    return isPositionInRange(position, selectedRange.get());
}

void WebPage::performDictionaryLookupAtLocation(const FloatPoint& floatPoint)
{
    Frame* frame = m_page->mainFrame();
    if (!frame)
        return;

    if (PluginView* pluginView = pluginViewForFrame(frame)) {
        if (pluginView->performDictionaryLookupAtLocation(floatPoint))
            return;
    }

    // Find the frame the point is over.
    IntPoint point = roundedIntPoint(floatPoint);
    HitTestResult result = frame->eventHandler()->hitTestResultAtPoint(frame->view()->windowToContents(point));
    frame = result.innerNonSharedNode() ? result.innerNonSharedNode()->document()->frame() : m_page->focusController()->focusedOrMainFrame();

    IntPoint translatedPoint = frame->view()->windowToContents(point);

    // Don't do anything if there is no character at the point.
    if (!frame->rangeForPoint(translatedPoint))
        return;

    VisiblePosition position = frame->visiblePositionForPoint(translatedPoint);
    VisibleSelection selection = m_page->focusController()->focusedOrMainFrame()->selection()->selection();
    if (shouldUseSelection(position, selection)) {
        performDictionaryLookupForSelection(frame, selection);
        return;
    }

    NSDictionary *options = nil;

    // As context, we are going to use four lines of text before and after the point. (Dictionary can sometimes look up things that are four lines long)
    const int numberOfLinesOfContext = 4;
    VisiblePosition contextStart = position;
    VisiblePosition contextEnd = position;
    for (int i = 0; i < numberOfLinesOfContext; i++) {
        VisiblePosition n = previousLinePosition(contextStart, contextStart.lineDirectionPointForBlockDirectionNavigation());
        if (n.isNull() || n == contextStart)
            break;
        contextStart = n;
    }
    for (int i = 0; i < numberOfLinesOfContext; i++) {
        VisiblePosition n = nextLinePosition(contextEnd, contextEnd.lineDirectionPointForBlockDirectionNavigation());
        if (n.isNull() || n == contextEnd)
            break;
        contextEnd = n;
    }

    VisiblePosition lineStart = startOfLine(contextStart);
    if (!lineStart.isNull())
        contextStart = lineStart;

    VisiblePosition lineEnd = endOfLine(contextEnd);
    if (!lineEnd.isNull())
        contextEnd = lineEnd;

    NSRange rangeToPass = NSMakeRange(TextIterator::rangeLength(makeRange(contextStart, position).get()), 0);

    RefPtr<Range> fullCharacterRange = makeRange(contextStart, contextEnd);
    String fullPlainTextString = plainText(fullCharacterRange.get());

    NSRange extractedRange = WKExtractWordDefinitionTokenRangeFromContextualString(fullPlainTextString, rangeToPass, &options);

    // This function sometimes returns {NSNotFound, 0} if it was unable to determine a good string.
    if (extractedRange.location == NSNotFound)
        return;

    RefPtr<Range> finalRange = TextIterator::subrange(fullCharacterRange.get(), extractedRange.location, extractedRange.length);
    if (!finalRange)
        return;

    performDictionaryLookupForRange(frame, finalRange.get(), options);
}

void WebPage::performDictionaryLookupForSelection(Frame* frame, const VisibleSelection& selection)
{
    RefPtr<Range> selectedRange = selection.toNormalizedRange();
    if (!selectedRange)
        return;

    NSDictionary *options = nil;

    VisiblePosition selectionStart = selection.visibleStart();
    VisiblePosition selectionEnd = selection.visibleEnd();

    // As context, we are going to use the surrounding paragraphs of text.
    VisiblePosition paragraphStart = startOfParagraph(selectionStart);
    VisiblePosition paragraphEnd = endOfParagraph(selectionEnd);

    int lengthToSelectionStart = TextIterator::rangeLength(makeRange(paragraphStart, selectionStart).get());
    int lengthToSelectionEnd = TextIterator::rangeLength(makeRange(paragraphStart, selectionEnd).get());
    NSRange rangeToPass = NSMakeRange(lengthToSelectionStart, lengthToSelectionEnd - lengthToSelectionStart);

    String fullPlainTextString = plainText(makeRange(paragraphStart, paragraphEnd).get());

    // Since we already have the range we want, we just need to grab the returned options.
    WKExtractWordDefinitionTokenRangeFromContextualString(fullPlainTextString, rangeToPass, &options);

    performDictionaryLookupForRange(frame, selectedRange.get(), options);
}

void WebPage::performDictionaryLookupForRange(Frame* frame, Range* range, NSDictionary *options)
{
    if (range->text().stripWhiteSpace().isEmpty())
        return;
    
    RenderObject* renderer = range->startContainer()->renderer();
    RenderStyle* style = renderer->style();

    Vector<FloatQuad> quads;
    range->textQuads(quads);
    if (quads.isEmpty())
        return;

    IntRect rangeRect = frame->view()->contentsToWindow(quads[0].enclosingBoundingBox());
    
    DictionaryPopupInfo dictionaryPopupInfo;
    dictionaryPopupInfo.origin = FloatPoint(rangeRect.x(), rangeRect.y() + (style->fontMetrics().ascent() * pageScaleFactor()));
    dictionaryPopupInfo.options = (CFDictionaryRef)options;

    NSAttributedString *nsAttributedString = [WebHTMLConverter editingAttributedStringFromRange:range];

    RetainPtr<NSMutableAttributedString> scaledNSAttributedString = adoptNS([[NSMutableAttributedString alloc] initWithString:[nsAttributedString string]]);

    NSFontManager *fontManager = [NSFontManager sharedFontManager];

    [nsAttributedString enumerateAttributesInRange:NSMakeRange(0, [nsAttributedString length]) options:0 usingBlock:^(NSDictionary *attributes, NSRange range, BOOL *stop) {
        RetainPtr<NSMutableDictionary> scaledAttributes = adoptNS([attributes mutableCopy]);

        NSFont *font = [scaledAttributes objectForKey:NSFontAttributeName];
        if (font) {
            font = [fontManager convertFont:font toSize:[font pointSize] * pageScaleFactor()];
            [scaledAttributes setObject:font forKey:NSFontAttributeName];
        }

        [scaledNSAttributedString.get() addAttributes:scaledAttributes.get() range:range];
    }];

    AttributedString attributedString;
    attributedString.string = scaledNSAttributedString;

    send(Messages::WebPageProxy::DidPerformDictionaryLookup(attributedString, dictionaryPopupInfo));
}

bool WebPage::performNonEditingBehaviorForSelector(const String& selector)
{
    // FIXME: All these selectors have corresponding Editor commands, but the commands only work in editable content.
    // Should such non-editing behaviors be implemented in Editor or EventHandler::defaultArrowEventHandler() perhaps?
    if (selector == "moveUp:")
        scroll(m_page.get(), ScrollUp, ScrollByLine);
    else if (selector == "moveToBeginningOfParagraph:")
        scroll(m_page.get(), ScrollUp, ScrollByPage);
    else if (selector == "moveToBeginningOfDocument:") {
        scroll(m_page.get(), ScrollUp, ScrollByDocument);
        scroll(m_page.get(), ScrollLeft, ScrollByDocument);
    } else if (selector == "moveDown:")
        scroll(m_page.get(), ScrollDown, ScrollByLine);
    else if (selector == "moveToEndOfParagraph:")
        scroll(m_page.get(), ScrollDown, ScrollByPage);
    else if (selector == "moveToEndOfDocument:") {
        scroll(m_page.get(), ScrollDown, ScrollByDocument);
        scroll(m_page.get(), ScrollLeft, ScrollByDocument);
    } else if (selector == "moveLeft:")
        scroll(m_page.get(), ScrollLeft, ScrollByLine);
    else if (selector == "moveWordLeft:")
        scroll(m_page.get(), ScrollLeft, ScrollByPage);
    else if (selector == "moveToLeftEndOfLine:")
        m_page->goBack();
    else if (selector == "moveRight:")
        scroll(m_page.get(), ScrollRight, ScrollByLine);
    else if (selector == "moveWordRight:")
        scroll(m_page.get(), ScrollRight, ScrollByPage);
    else if (selector == "moveToRightEndOfLine:")
        m_page->goForward();
    else
        return false;

    return true;
}

bool WebPage::performDefaultBehaviorForKeyEvent(const WebKeyboardEvent&)
{
    return false;
}

void WebPage::registerUIProcessAccessibilityTokens(const CoreIPC::DataReference& elementToken, const CoreIPC::DataReference& windowToken)
{
    NSData* elementTokenData = [NSData dataWithBytes:elementToken.data() length:elementToken.size()];
    NSData* windowTokenData = [NSData dataWithBytes:windowToken.data() length:windowToken.size()];
    id remoteElement = WKAXRemoteElementForToken(elementTokenData);
    id remoteWindow = WKAXRemoteElementForToken(windowTokenData);
    WKAXSetWindowForRemoteElement(remoteWindow, remoteElement);
    
    [accessibilityRemoteObject() setRemoteParent:remoteElement];
}

void WebPage::readSelectionFromPasteboard(const String& pasteboardName, bool& result)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame || frame->selection()->isNone()) {
        result = false;
        return;
    }
    frame->editor().readSelectionFromPasteboard(pasteboardName);
    result = true;
}

void WebPage::getStringSelectionForPasteboard(String& stringValue)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();

    if (!frame)
        return;

    if (PluginView* pluginView = focusedPluginViewForFrame(frame)) {
        String selection = pluginView->getSelectionString();
        if (!selection.isNull()) {
            stringValue = selection;
            return;
        }
    }

    if (frame->selection()->isNone())
        return;

    stringValue = frame->editor().stringSelectionForPasteboard();
}

void WebPage::getDataSelectionForPasteboard(const String pasteboardType, SharedMemory::Handle& handle, uint64_t& size)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame || frame->selection()->isNone())
        return;

    RefPtr<SharedBuffer> buffer = frame->editor().dataSelectionForPasteboard(pasteboardType);
    if (!buffer) {
        size = 0;
        return;
    }
    size = buffer->size();
    RefPtr<SharedMemory> sharedMemoryBuffer = SharedMemory::create(size);
    memcpy(sharedMemoryBuffer->data(), buffer->data(), size);
    sharedMemoryBuffer->createHandle(handle, SharedMemory::ReadOnly);
}

WKAccessibilityWebPageObject* WebPage::accessibilityRemoteObject()
{
    return m_mockAccessibilityElement.get();
}
         
bool WebPage::platformHasLocalDataForURL(const WebCore::KURL& url)
{
    NSMutableURLRequest* request = [[NSMutableURLRequest alloc] initWithURL:url];
    [request setValue:(NSString*)userAgent() forHTTPHeaderField:@"User-Agent"];
    NSCachedURLResponse *cachedResponse;
    if (CFURLStorageSessionRef storageSession = corePage()->mainFrame()->loader()->networkingContext()->storageSession().platformSession())
        cachedResponse = WKCachedResponseForRequest(storageSession, request);
    else
        cachedResponse = [[NSURLCache sharedURLCache] cachedResponseForRequest:request];
    [request release];
    
    return cachedResponse;
}

static NSCachedURLResponse *cachedResponseForURL(WebPage* webPage, const KURL& url)
{
    RetainPtr<NSMutableURLRequest> request = adoptNS([[NSMutableURLRequest alloc] initWithURL:url]);
    [request.get() setValue:(NSString *)webPage->userAgent() forHTTPHeaderField:@"User-Agent"];

    if (CFURLStorageSessionRef storageSession = webPage->corePage()->mainFrame()->loader()->networkingContext()->storageSession().platformSession())
        return WKCachedResponseForRequest(storageSession, request.get());

    return [[NSURLCache sharedURLCache] cachedResponseForRequest:request.get()];
}

String WebPage::cachedSuggestedFilenameForURL(const KURL& url)
{
    return [[cachedResponseForURL(this, url) response] suggestedFilename];
}

String WebPage::cachedResponseMIMETypeForURL(const KURL& url)
{
    return [[cachedResponseForURL(this, url) response] MIMEType];
}

PassRefPtr<SharedBuffer> WebPage::cachedResponseDataForURL(const KURL& url)
{
    return SharedBuffer::wrapNSData([cachedResponseForURL(this, url) data]);
}

bool WebPage::platformCanHandleRequest(const WebCore::ResourceRequest& request)
{
    if ([NSURLConnection canHandleRequest:request.nsURLRequest(DoNotUpdateHTTPBody)])
        return true;

    // FIXME: Return true if this scheme is any one WebKit2 knows how to handle.
    return request.url().protocolIs("applewebdata");
}

void WebPage::shouldDelayWindowOrderingEvent(const WebKit::WebMouseEvent& event, bool& result)
{
    result = false;
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

#if ENABLE(DRAG_SUPPORT)
    HitTestResult hitResult = frame->eventHandler()->hitTestResultAtPoint(frame->view()->windowToContents(event.position()), HitTestRequest::ReadOnly | HitTestRequest::Active);
    if (hitResult.isSelected())
        result = frame->eventHandler()->eventMayStartDrag(platform(event));
#endif
}

void WebPage::acceptsFirstMouse(int eventNumber, const WebKit::WebMouseEvent& event, bool& result)
{
    result = false;
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;
    
    HitTestResult hitResult = frame->eventHandler()->hitTestResultAtPoint(frame->view()->windowToContents(event.position()), HitTestRequest::ReadOnly | HitTestRequest::Active);
    frame->eventHandler()->setActivationEventNumber(eventNumber);
#if ENABLE(DRAG_SUPPORT)
    if (hitResult.isSelected())
        result = frame->eventHandler()->eventMayStartDrag(platform(event));
    else
#endif
        result = !!hitResult.scrollbar();
}

void WebPage::setLayerHostingMode(LayerHostingMode layerHostingMode)
{
    m_layerHostingMode = layerHostingMode;

    for (HashSet<PluginView*>::const_iterator it = m_pluginViews.begin(), end = m_pluginViews.end(); it != end; ++it)
        (*it)->setLayerHostingMode(layerHostingMode);
}

void WebPage::setTopOverhangImage(PassRefPtr<WebImage> image)
{
    FrameView* frameView = m_mainFrame->coreFrame()->view();
    if (!frameView)
        return;

    GraphicsLayer* layer = frameView->setWantsLayerForTopOverHangArea(image.get());
    if (!layer)
        return;

    layer->setSize(image->size());
    layer->setPosition(FloatPoint(0, -image->size().height()));

    RetainPtr<CGImageRef> cgImage = image->bitmap()->makeCGImageCopy();
    layer->platformLayer().contents = (id)cgImage.get();
}

void WebPage::setBottomOverhangImage(PassRefPtr<WebImage> image)
{
    FrameView* frameView = m_mainFrame->coreFrame()->view();
    if (!frameView)
        return;

    GraphicsLayer* layer = frameView->setWantsLayerForBottomOverHangArea(image.get());
    if (!layer)
        return;

    layer->setSize(image->size());
    
    RetainPtr<CGImageRef> cgImage = image->bitmap()->makeCGImageCopy();
    layer->platformLayer().contents = (id)cgImage.get();
}

void WebPage::updateHeaderAndFooterLayersForDeviceScaleChange(float scaleFactor)
{    
    if (m_headerBanner)
        m_headerBanner->didChangeDeviceScaleFactor(scaleFactor);
    if (m_footerBanner)
        m_footerBanner->didChangeDeviceScaleFactor(scaleFactor);
}

void WebPage::computePagesForPrintingPDFDocument(uint64_t frameID, const PrintInfo& printInfo, Vector<IntRect>& resultPageRects)
{
    ASSERT(resultPageRects.isEmpty());
    WebFrame* frame = WebProcess::shared().webFrame(frameID);
    Frame* coreFrame = frame ? frame->coreFrame() : 0;
    RetainPtr<PDFDocument> pdfDocument = coreFrame ? pdfDocumentForPrintingFrame(coreFrame) : 0;
    if ([pdfDocument.get() allowsPrinting]) {
        NSUInteger pageCount = [pdfDocument.get() pageCount];
        IntRect pageRect(0, 0, ceilf(printInfo.availablePaperWidth), ceilf(printInfo.availablePaperHeight));
        for (NSUInteger i = 1; i <= pageCount; ++i) {
            resultPageRects.append(pageRect);
            pageRect.move(0, pageRect.height());
        }
    }
}

static inline CGFloat roundCGFloat(CGFloat f)
{
    if (sizeof(CGFloat) == sizeof(float))
        return roundf(static_cast<float>(f));
    return static_cast<CGFloat>(round(f));
}

static void drawPDFPage(PDFDocument *pdfDocument, CFIndex pageIndex, CGContextRef context, CGFloat pageSetupScaleFactor, CGSize paperSize)
{
    CGContextSaveGState(context);

    CGContextScaleCTM(context, pageSetupScaleFactor, pageSetupScaleFactor);

    PDFPage *pdfPage = [pdfDocument pageAtIndex:pageIndex];
    NSRect cropBox = [pdfPage boundsForBox:kPDFDisplayBoxCropBox];
    if (NSIsEmptyRect(cropBox))
        cropBox = [pdfPage boundsForBox:kPDFDisplayBoxMediaBox];
    else
        cropBox = NSIntersectionRect(cropBox, [pdfPage boundsForBox:kPDFDisplayBoxMediaBox]);

    // Always auto-rotate PDF content regardless of the paper orientation.
    NSInteger rotation = [pdfPage rotation];
    if (rotation == 90 || rotation == 270)
        std::swap(cropBox.size.width, cropBox.size.height);

    bool shouldRotate = (paperSize.width < paperSize.height) != (cropBox.size.width < cropBox.size.height);
    if (shouldRotate)
        std::swap(cropBox.size.width, cropBox.size.height);

    // Center.
    CGFloat widthDifference = paperSize.width / pageSetupScaleFactor - cropBox.size.width;
    CGFloat heightDifference = paperSize.height / pageSetupScaleFactor - cropBox.size.height;
    if (widthDifference || heightDifference)
        CGContextTranslateCTM(context, roundCGFloat(widthDifference / 2), roundCGFloat(heightDifference / 2));

    if (shouldRotate) {
        CGContextRotateCTM(context, static_cast<CGFloat>(piOverTwoDouble));
        CGContextTranslateCTM(context, 0, -cropBox.size.width);
    }

    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithGraphicsPort:context flipped:NO]];
    [pdfPage drawWithBox:kPDFDisplayBoxCropBox];
    [NSGraphicsContext restoreGraphicsState];

    CGAffineTransform transform = CGContextGetCTM(context);

    for (PDFAnnotation *annotation in [pdfPage annotations]) {
        if (![annotation isKindOfClass:pdfAnnotationLinkClass()])
            continue;

        PDFAnnotationLink *linkAnnotation = (PDFAnnotationLink *)annotation;
        NSURL *url = [linkAnnotation URL];
        if (!url)
            continue;

        CGRect urlRect = NSRectToCGRect([linkAnnotation bounds]);
        CGRect transformedRect = CGRectApplyAffineTransform(urlRect, transform);
        CGPDFContextSetURLForRect(context, (CFURLRef)url, transformedRect);
    }

    CGContextRestoreGState(context);
}

void WebPage::drawPDFDocument(CGContextRef context, PDFDocument *pdfDocument, const PrintInfo& printInfo, const WebCore::IntRect& rect)
{
    NSUInteger pageCount = [pdfDocument pageCount];
    IntSize paperSize(ceilf(printInfo.availablePaperWidth), ceilf(printInfo.availablePaperHeight));
    IntRect pageRect(IntPoint(), paperSize);
    for (NSUInteger i = 0; i < pageCount; ++i) {
        if (pageRect.intersects(rect)) {
            CGContextSaveGState(context);

            CGContextTranslateCTM(context, pageRect.x() - rect.x(), pageRect.y() - rect.y());
            drawPDFPage(pdfDocument, i, context, printInfo.pageSetupScaleFactor, paperSize);

            CGContextRestoreGState(context);
        }
        pageRect.move(0, pageRect.height());
    }
}

void WebPage::drawPagesToPDFFromPDFDocument(CGContextRef context, PDFDocument *pdfDocument, const PrintInfo& printInfo, uint32_t first, uint32_t count)
{
    NSUInteger pageCount = [pdfDocument pageCount];
    for (uint32_t page = first; page < first + count; ++page) {
        if (page >= pageCount)
            break;

        RetainPtr<CFDictionaryRef> pageInfo = adoptCF(CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

        CGPDFContextBeginPage(context, pageInfo.get());
        drawPDFPage(pdfDocument, page, context, printInfo.pageSetupScaleFactor, CGSizeMake(printInfo.availablePaperWidth, printInfo.availablePaperHeight));
        CGPDFContextEndPage(context);
    }
}

} // namespace WebKit
