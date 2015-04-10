/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
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
#include "ContextMenuController.h"

#if ENABLE(CONTEXT_MENUS)

#include "BackForwardController.h"
#include "Chrome.h"
#include "ContextMenu.h"
#include "ContextMenuClient.h"
#include "ContextMenuItem.h"
#include "ContextMenuProvider.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "DocumentLoader.h"
#include "Editor.h"
#include "EditorClient.h"
#include "Event.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "ExceptionCodePlaceholder.h"
#include "FormState.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameSelection.h"
#include "HTMLFormElement.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "InspectorController.h"
#include "LocalizedStrings.h"
#include "MouseEvent.h"
#include "NavigationAction.h"
#include "Node.h"
#include "Page.h"
#include "PlatformEvent.h"
#include "RenderObject.h"
#include "ReplaceSelectionCommand.h"
#include "ResourceRequest.h"
#include "Settings.h"
#include "TextIterator.h"
#include "TypingCommand.h"
#include "UserTypingGestureIndicator.h"
#include "WindowFeatures.h"
#include "markup.h"
#include <wtf/unicode/CharacterNames.h>
#include <wtf/unicode/Unicode.h>

#if PLATFORM(GTK)
#include <wtf/gobject/GOwnPtr.h>
#endif

using namespace WTF;
using namespace Unicode;

namespace WebCore {

ContextMenuController::ContextMenuController(Page* page, ContextMenuClient* client)
    : m_page(page)
    , m_client(client)
{
    ASSERT_ARG(page, page);
    ASSERT_ARG(client, client);
}

ContextMenuController::~ContextMenuController()
{
    m_client->contextMenuDestroyed();
}

PassOwnPtr<ContextMenuController> ContextMenuController::create(Page* page, ContextMenuClient* client)
{
    return adoptPtr(new ContextMenuController(page, client));
}

void ContextMenuController::clearContextMenu()
{
    m_contextMenu.clear();
    if (m_menuProvider)
        m_menuProvider->contextMenuCleared();
    m_menuProvider = 0;
}

void ContextMenuController::handleContextMenuEvent(Event* event)
{
    m_contextMenu = createContextMenu(event);
    if (!m_contextMenu)
        return;

    populate();

    showContextMenu(event);
}

static PassOwnPtr<ContextMenuItem> separatorItem()
{
    return adoptPtr(new ContextMenuItem(SeparatorType, ContextMenuItemTagNoAction, String()));
}

void ContextMenuController::showContextMenu(Event* event, PassRefPtr<ContextMenuProvider> menuProvider)
{
    m_menuProvider = menuProvider;

    m_contextMenu = createContextMenu(event);
    if (!m_contextMenu) {
        clearContextMenu();
        return;
    }

    m_menuProvider->populateContextMenu(m_contextMenu.get());
    if (m_hitTestResult.isSelected()) {
        appendItem(*separatorItem(), m_contextMenu.get());
        populate();
    }
    showContextMenu(event);
}

PassOwnPtr<ContextMenu> ContextMenuController::createContextMenu(Event* event)
{
    ASSERT(event);
    
    if (!event->isMouseEvent())
        return nullptr;

    MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
    HitTestResult result(mouseEvent->absoluteLocation());

    if (Frame* frame = event->target()->toNode()->document()->frame())
        result = frame->eventHandler()->hitTestResultAtPoint(mouseEvent->absoluteLocation());

    if (!result.innerNonSharedNode())
        return nullptr;

    m_hitTestResult = result;

    return adoptPtr(new ContextMenu);
}

void ContextMenuController::showContextMenu(Event* event)
{
#if ENABLE(INSPECTOR)
    if (m_page->inspectorController()->enabled())
        addInspectElementItem();
#endif

#if USE(CROSS_PLATFORM_CONTEXT_MENUS)
    m_contextMenu = m_client->customizeMenu(m_contextMenu.release());
#else
    PlatformMenuDescription customMenu = m_client->getCustomMenuFromDefaultItems(m_contextMenu.get());
    m_contextMenu->setPlatformDescription(customMenu);
#endif
    event->setDefaultHandled();
}

static void openNewWindow(const KURL& urlToLoad, Frame* frame)
{
    if (Page* oldPage = frame->page()) {
        FrameLoadRequest request(frame->document()->securityOrigin(), ResourceRequest(urlToLoad, frame->loader()->outgoingReferrer()));
        Page* newPage = oldPage;
        if (!frame->settings() || frame->settings()->supportsMultipleWindows()) {
            newPage = oldPage->chrome().createWindow(frame, request, WindowFeatures(), NavigationAction(request.resourceRequest()));
            if (!newPage)
                return;
            newPage->chrome().show();
        }
        newPage->mainFrame()->loader()->loadFrameRequest(request, false, false, 0, 0, MaybeSendReferrer);
    }
}

#if PLATFORM(GTK)
static void insertUnicodeCharacter(UChar character, Frame* frame)
{
    String text(&character, 1);
    if (!frame->editor().shouldInsertText(text, frame->selection()->toNormalizedRange().get(), EditorInsertActionTyped))
        return;

    TypingCommand::insertText(frame->document(), text, 0, TypingCommand::TextCompositionNone);
}
#endif

void ContextMenuController::contextMenuItemSelected(ContextMenuItem* item)
{
    ASSERT(item->type() == ActionType || item->type() == CheckableActionType);

    if (item->action() >= ContextMenuItemBaseApplicationTag) {
        m_client->contextMenuItemSelected(item, m_contextMenu.get());
        return;
    }

    if (item->action() >= ContextMenuItemBaseCustomTag) {
        ASSERT(m_menuProvider);
        m_menuProvider->contextMenuItemSelected(item);
        return;
    }

    Frame* frame = m_hitTestResult.innerNonSharedNode()->document()->frame();
    if (!frame)
        return;

    switch (item->action()) {
    case ContextMenuItemTagOpenLinkInNewWindow:
        openNewWindow(m_hitTestResult.absoluteLinkURL(), frame);
        break;
    case ContextMenuItemTagDownloadLinkToDisk:
        // FIXME: Some day we should be able to do this from within WebCore. (Bug 117709)
        m_client->downloadURL(m_hitTestResult.absoluteLinkURL());
        break;
    case ContextMenuItemTagCopyLinkToClipboard:
        frame->editor().copyURL(m_hitTestResult.absoluteLinkURL(), m_hitTestResult.textContent());
        break;
    case ContextMenuItemTagOpenImageInNewWindow:
        openNewWindow(m_hitTestResult.absoluteImageURL(), frame);
        break;
    case ContextMenuItemTagDownloadImageToDisk:
        // FIXME: Some day we should be able to do this from within WebCore. (Bug 117709)
        m_client->downloadURL(m_hitTestResult.absoluteImageURL());
        break;
    case ContextMenuItemTagCopyImageToClipboard:
        // FIXME: The Pasteboard class is not written yet
        // For now, call into the client. This is temporary!
        frame->editor().copyImage(m_hitTestResult);
        break;
#if PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
    case ContextMenuItemTagCopyImageUrlToClipboard:
        frame->editor().copyURL(m_hitTestResult.absoluteImageURL(), m_hitTestResult.textContent());
        break;
#endif
    case ContextMenuItemTagOpenMediaInNewWindow:
        openNewWindow(m_hitTestResult.absoluteMediaURL(), frame);
        break;
    case ContextMenuItemTagDownloadMediaToDisk:
        // FIXME: Some day we should be able to do this from within WebCore. (Bug 117709)
        m_client->downloadURL(m_hitTestResult.absoluteMediaURL());
        break;
    case ContextMenuItemTagCopyMediaLinkToClipboard:
        frame->editor().copyURL(m_hitTestResult.absoluteMediaURL(), m_hitTestResult.textContent());
        break;
    case ContextMenuItemTagToggleMediaControls:
        m_hitTestResult.toggleMediaControlsDisplay();
        break;
    case ContextMenuItemTagToggleMediaLoop:
        m_hitTestResult.toggleMediaLoopPlayback();
        break;
    case ContextMenuItemTagToggleVideoFullscreen:
        m_hitTestResult.toggleMediaFullscreenState();
        break;
    case ContextMenuItemTagEnterVideoFullscreen:
        m_hitTestResult.enterFullscreenForVideo();
        break;
    case ContextMenuItemTagMediaPlayPause:
        m_hitTestResult.toggleMediaPlayState();
        break;
    case ContextMenuItemTagMediaMute:
        m_hitTestResult.toggleMediaMuteState();
        break;
    case ContextMenuItemTagOpenFrameInNewWindow: {
        DocumentLoader* loader = frame->loader()->documentLoader();
        if (!loader->unreachableURL().isEmpty())
            openNewWindow(loader->unreachableURL(), frame);
        else
            openNewWindow(loader->url(), frame);
        break;
    }
    case ContextMenuItemTagCopy:
        frame->editor().copy();
        break;
    case ContextMenuItemTagGoBack:
        if (Page* page = frame->page())
            page->backForward()->goBackOrForward(-1);
        break;
    case ContextMenuItemTagGoForward:
        if (Page* page = frame->page())
            page->backForward()->goBackOrForward(1);
        break;
    case ContextMenuItemTagStop:
        frame->loader()->stop();
        break;
    case ContextMenuItemTagReload:
        frame->loader()->reload();
        break;
    case ContextMenuItemTagCut:
        frame->editor().command("Cut").execute();
        break;
    case ContextMenuItemTagPaste:
        frame->editor().command("Paste").execute();
        break;
#if PLATFORM(GTK)
    case ContextMenuItemTagDelete:
        frame->editor().performDelete();
        break;
    case ContextMenuItemTagUnicodeInsertLRMMark:
        insertUnicodeCharacter(leftToRightMark, frame);
        break;
    case ContextMenuItemTagUnicodeInsertRLMMark:
        insertUnicodeCharacter(rightToLeftMark, frame);
        break;
    case ContextMenuItemTagUnicodeInsertLREMark:
        insertUnicodeCharacter(leftToRightEmbed, frame);
        break;
    case ContextMenuItemTagUnicodeInsertRLEMark:
        insertUnicodeCharacter(rightToLeftEmbed, frame);
        break;
    case ContextMenuItemTagUnicodeInsertLROMark:
        insertUnicodeCharacter(leftToRightOverride, frame);
        break;
    case ContextMenuItemTagUnicodeInsertRLOMark:
        insertUnicodeCharacter(rightToLeftOverride, frame);
        break;
    case ContextMenuItemTagUnicodeInsertPDFMark:
        insertUnicodeCharacter(popDirectionalFormatting, frame);
        break;
    case ContextMenuItemTagUnicodeInsertZWSMark:
        insertUnicodeCharacter(zeroWidthSpace, frame);
        break;
    case ContextMenuItemTagUnicodeInsertZWJMark:
        insertUnicodeCharacter(zeroWidthJoiner, frame);
        break;
    case ContextMenuItemTagUnicodeInsertZWNJMark:
        insertUnicodeCharacter(zeroWidthNonJoiner, frame);
        break;
#endif
#if PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(EFL)
    case ContextMenuItemTagSelectAll:
        frame->editor().command("SelectAll").execute();
        break;
#endif
    case ContextMenuItemTagSpellingGuess: {
        FrameSelection* frameSelection = frame->selection();
        if (frame->editor().shouldInsertText(item->title(), frameSelection->toNormalizedRange().get(), EditorInsertActionPasted)) {
            Document* document = frame->document();
            ReplaceSelectionCommand::CommandOptions replaceOptions = ReplaceSelectionCommand::MatchStyle | ReplaceSelectionCommand::PreventNesting;

            if (frame->editor().behavior().shouldAllowSpellingSuggestionsWithoutSelection()) {
                ASSERT(frameSelection->isCaretOrRange());
                VisibleSelection wordSelection(frameSelection->base());
                wordSelection.expandUsingGranularity(WordGranularity);
                frameSelection->setSelection(wordSelection);
            } else {
                ASSERT(frame->editor().selectedText().length());
                replaceOptions |= ReplaceSelectionCommand::SelectReplacement;
            }

            RefPtr<ReplaceSelectionCommand> command = ReplaceSelectionCommand::create(document, createFragmentFromMarkup(document, item->title(), ""), replaceOptions);
            applyCommand(command);
            frameSelection->revealSelection(ScrollAlignment::alignToEdgeIfNeeded);
        }
        break;
    }
    case ContextMenuItemTagIgnoreSpelling:
        frame->editor().ignoreSpelling();
        break;
    case ContextMenuItemTagLearnSpelling:
        frame->editor().learnSpelling();
        break;
    case ContextMenuItemTagSearchWeb:
        m_client->searchWithGoogle(frame);
        break;
    case ContextMenuItemTagLookUpInDictionary:
        // FIXME: Some day we may be able to do this from within WebCore.
        m_client->lookUpInDictionary(frame);
        break;
    case ContextMenuItemTagOpenLink:
        if (Frame* targetFrame = m_hitTestResult.targetFrame())
            targetFrame->loader()->loadFrameRequest(FrameLoadRequest(frame->document()->securityOrigin(), ResourceRequest(m_hitTestResult.absoluteLinkURL(), frame->loader()->outgoingReferrer())), false, false, 0, 0, MaybeSendReferrer);
        else
            openNewWindow(m_hitTestResult.absoluteLinkURL(), frame);
        break;
    case ContextMenuItemTagOpenLinkInThisWindow:
        frame->loader()->loadFrameRequest(FrameLoadRequest(frame->document()->securityOrigin(), ResourceRequest(m_hitTestResult.absoluteLinkURL(), frame->loader()->outgoingReferrer())), false, false, 0, 0, MaybeSendReferrer);
        break;
    case ContextMenuItemTagBold:
        frame->editor().command("ToggleBold").execute();
        break;
    case ContextMenuItemTagItalic:
        frame->editor().command("ToggleItalic").execute();
        break;
    case ContextMenuItemTagUnderline:
        frame->editor().toggleUnderline();
        break;
    case ContextMenuItemTagOutline:
        // We actually never enable this because CSS does not have a way to specify an outline font,
        // which may make this difficult to implement. Maybe a special case of text-shadow?
        break;
    case ContextMenuItemTagStartSpeaking: {
        RefPtr<Range> selectedRange = frame->selection()->toNormalizedRange();
        if (!selectedRange || selectedRange->collapsed(IGNORE_EXCEPTION)) {
            Document* document = m_hitTestResult.innerNonSharedNode()->document();
            selectedRange = document->createRange();
            selectedRange->selectNode(document->documentElement(), IGNORE_EXCEPTION);
        }
        m_client->speak(plainText(selectedRange.get()));
        break;
    }
    case ContextMenuItemTagStopSpeaking:
        m_client->stopSpeaking();
        break;
    case ContextMenuItemTagDefaultDirection:
        frame->editor().setBaseWritingDirection(NaturalWritingDirection);
        break;
    case ContextMenuItemTagLeftToRight:
        frame->editor().setBaseWritingDirection(LeftToRightWritingDirection);
        break;
    case ContextMenuItemTagRightToLeft:
        frame->editor().setBaseWritingDirection(RightToLeftWritingDirection);
        break;
    case ContextMenuItemTagTextDirectionDefault:
        frame->editor().command("MakeTextWritingDirectionNatural").execute();
        break;
    case ContextMenuItemTagTextDirectionLeftToRight:
        frame->editor().command("MakeTextWritingDirectionLeftToRight").execute();
        break;
    case ContextMenuItemTagTextDirectionRightToLeft:
        frame->editor().command("MakeTextWritingDirectionRightToLeft").execute();
        break;
#if PLATFORM(MAC)
    case ContextMenuItemTagSearchInSpotlight:
        m_client->searchWithSpotlight();
        break;
#endif
    case ContextMenuItemTagShowSpellingPanel:
        frame->editor().showSpellingGuessPanel();
        break;
    case ContextMenuItemTagCheckSpelling:
        frame->editor().advanceToNextMisspelling();
        break;
    case ContextMenuItemTagCheckSpellingWhileTyping:
        frame->editor().toggleContinuousSpellChecking();
        break;
    case ContextMenuItemTagCheckGrammarWithSpelling:
        frame->editor().toggleGrammarChecking();
        break;
#if PLATFORM(MAC)
    case ContextMenuItemTagShowFonts:
        frame->editor().showFontPanel();
        break;
    case ContextMenuItemTagStyles:
        frame->editor().showStylesPanel();
        break;
    case ContextMenuItemTagShowColors:
        frame->editor().showColorPanel();
        break;
#endif
#if USE(APPKIT)
    case ContextMenuItemTagMakeUpperCase:
        frame->editor().uppercaseWord();
        break;
    case ContextMenuItemTagMakeLowerCase:
        frame->editor().lowercaseWord();
        break;
    case ContextMenuItemTagCapitalize:
        frame->editor().capitalizeWord();
        break;
#endif
#if PLATFORM(MAC)
    case ContextMenuItemTagChangeBack:
        frame->editor().changeBackToReplacedString(m_hitTestResult.replacedString());
        break;
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    case ContextMenuItemTagShowSubstitutions:
        frame->editor().showSubstitutionsPanel();
        break;
    case ContextMenuItemTagSmartCopyPaste:
        frame->editor().toggleSmartInsertDelete();
        break;
    case ContextMenuItemTagSmartQuotes:
        frame->editor().toggleAutomaticQuoteSubstitution();
        break;
    case ContextMenuItemTagSmartDashes:
        frame->editor().toggleAutomaticDashSubstitution();
        break;
    case ContextMenuItemTagSmartLinks:
        frame->editor().toggleAutomaticLinkDetection();
        break;
    case ContextMenuItemTagTextReplacement:
        frame->editor().toggleAutomaticTextReplacement();
        break;
    case ContextMenuItemTagCorrectSpellingAutomatically:
        frame->editor().toggleAutomaticSpellingCorrection();
        break;
#endif
#if ENABLE(INSPECTOR)
    case ContextMenuItemTagInspectElement:
        if (Page* page = frame->page())
            page->inspectorController()->inspect(m_hitTestResult.innerNonSharedNode());
        break;
#endif
    case ContextMenuItemTagDictationAlternative:
        frame->editor().applyDictationAlternativelternative(item->title());
        break;
    default:
        break;
    }
}

void ContextMenuController::appendItem(ContextMenuItem& menuItem, ContextMenu* parentMenu)
{
    checkOrEnableIfNeeded(menuItem);
    if (parentMenu)
        parentMenu->appendItem(menuItem);
}

void ContextMenuController::createAndAppendFontSubMenu(ContextMenuItem& fontMenuItem)
{
    ContextMenu fontMenu;

#if PLATFORM(MAC)
    ContextMenuItem showFonts(ActionType, ContextMenuItemTagShowFonts, contextMenuItemTagShowFonts());
#endif
    ContextMenuItem bold(CheckableActionType, ContextMenuItemTagBold, contextMenuItemTagBold());
    ContextMenuItem italic(CheckableActionType, ContextMenuItemTagItalic, contextMenuItemTagItalic());
    ContextMenuItem underline(CheckableActionType, ContextMenuItemTagUnderline, contextMenuItemTagUnderline());
    ContextMenuItem outline(ActionType, ContextMenuItemTagOutline, contextMenuItemTagOutline());
#if PLATFORM(MAC)
    ContextMenuItem styles(ActionType, ContextMenuItemTagStyles, contextMenuItemTagStyles());
    ContextMenuItem showColors(ActionType, ContextMenuItemTagShowColors, contextMenuItemTagShowColors());
#endif

#if PLATFORM(MAC)
    appendItem(showFonts, &fontMenu);
#endif
    appendItem(bold, &fontMenu);
    appendItem(italic, &fontMenu);
    appendItem(underline, &fontMenu);
    appendItem(outline, &fontMenu);
#if PLATFORM(MAC)
    appendItem(styles, &fontMenu);
    appendItem(*separatorItem(), &fontMenu);
    appendItem(showColors, &fontMenu);
#endif

    fontMenuItem.setSubMenu(&fontMenu);
}


#if !PLATFORM(GTK)

void ContextMenuController::createAndAppendSpellingAndGrammarSubMenu(ContextMenuItem& spellingAndGrammarMenuItem)
{
    ContextMenu spellingAndGrammarMenu;

    ContextMenuItem showSpellingPanel(ActionType, ContextMenuItemTagShowSpellingPanel, 
        contextMenuItemTagShowSpellingPanel(true));
    ContextMenuItem checkSpelling(ActionType, ContextMenuItemTagCheckSpelling, 
        contextMenuItemTagCheckSpelling());
    ContextMenuItem checkAsYouType(CheckableActionType, ContextMenuItemTagCheckSpellingWhileTyping, 
        contextMenuItemTagCheckSpellingWhileTyping());
    ContextMenuItem grammarWithSpelling(CheckableActionType, ContextMenuItemTagCheckGrammarWithSpelling, 
        contextMenuItemTagCheckGrammarWithSpelling());
#if PLATFORM(MAC)
    ContextMenuItem correctSpelling(CheckableActionType, ContextMenuItemTagCorrectSpellingAutomatically, 
        contextMenuItemTagCorrectSpellingAutomatically());
#endif

    appendItem(showSpellingPanel, &spellingAndGrammarMenu);
    appendItem(checkSpelling, &spellingAndGrammarMenu);
#if PLATFORM(MAC)
    appendItem(*separatorItem(), &spellingAndGrammarMenu);
#endif
    appendItem(checkAsYouType, &spellingAndGrammarMenu);
    appendItem(grammarWithSpelling, &spellingAndGrammarMenu);
#if PLATFORM(MAC)
    appendItem(correctSpelling, &spellingAndGrammarMenu);
#endif

    spellingAndGrammarMenuItem.setSubMenu(&spellingAndGrammarMenu);
}

#endif // !PLATFORM(GTK)


#if PLATFORM(MAC)

void ContextMenuController::createAndAppendSpeechSubMenu(ContextMenuItem& speechMenuItem)
{
    ContextMenu speechMenu;

    ContextMenuItem start(ActionType, ContextMenuItemTagStartSpeaking, contextMenuItemTagStartSpeaking());
    ContextMenuItem stop(ActionType, ContextMenuItemTagStopSpeaking, contextMenuItemTagStopSpeaking());

    appendItem(start, &speechMenu);
    appendItem(stop, &speechMenu);

    speechMenuItem.setSubMenu(&speechMenu);
}

#endif
 
#if PLATFORM(GTK)

void ContextMenuController::createAndAppendUnicodeSubMenu(ContextMenuItem& unicodeMenuItem)
{
    ContextMenu unicodeMenu;

    ContextMenuItem leftToRightMarkMenuItem(ActionType, ContextMenuItemTagUnicodeInsertLRMMark, contextMenuItemTagUnicodeInsertLRMMark());
    ContextMenuItem rightToLeftMarkMenuItem(ActionType, ContextMenuItemTagUnicodeInsertRLMMark, contextMenuItemTagUnicodeInsertRLMMark());
    ContextMenuItem leftToRightEmbedMenuItem(ActionType, ContextMenuItemTagUnicodeInsertLREMark, contextMenuItemTagUnicodeInsertLREMark());
    ContextMenuItem rightToLeftEmbedMenuItem(ActionType, ContextMenuItemTagUnicodeInsertRLEMark, contextMenuItemTagUnicodeInsertRLEMark());
    ContextMenuItem leftToRightOverrideMenuItem(ActionType, ContextMenuItemTagUnicodeInsertLROMark, contextMenuItemTagUnicodeInsertLROMark());
    ContextMenuItem rightToLeftOverrideMenuItem(ActionType, ContextMenuItemTagUnicodeInsertRLOMark, contextMenuItemTagUnicodeInsertRLOMark());
    ContextMenuItem popDirectionalFormattingMenuItem(ActionType, ContextMenuItemTagUnicodeInsertPDFMark, contextMenuItemTagUnicodeInsertPDFMark());
    ContextMenuItem zeroWidthSpaceMenuItem(ActionType, ContextMenuItemTagUnicodeInsertZWSMark, contextMenuItemTagUnicodeInsertZWSMark());
    ContextMenuItem zeroWidthJoinerMenuItem(ActionType, ContextMenuItemTagUnicodeInsertZWJMark, contextMenuItemTagUnicodeInsertZWJMark());
    ContextMenuItem zeroWidthNonJoinerMenuItem(ActionType, ContextMenuItemTagUnicodeInsertZWNJMark, contextMenuItemTagUnicodeInsertZWNJMark());

    appendItem(leftToRightMarkMenuItem, &unicodeMenu);
    appendItem(rightToLeftMarkMenuItem, &unicodeMenu);
    appendItem(leftToRightEmbedMenuItem, &unicodeMenu);
    appendItem(rightToLeftEmbedMenuItem, &unicodeMenu);
    appendItem(leftToRightOverrideMenuItem, &unicodeMenu);
    appendItem(rightToLeftOverrideMenuItem, &unicodeMenu);
    appendItem(popDirectionalFormattingMenuItem, &unicodeMenu);
    appendItem(zeroWidthSpaceMenuItem, &unicodeMenu);
    appendItem(zeroWidthJoinerMenuItem, &unicodeMenu);
    appendItem(zeroWidthNonJoinerMenuItem, &unicodeMenu);

    unicodeMenuItem.setSubMenu(&unicodeMenu);
}

#else

void ContextMenuController::createAndAppendWritingDirectionSubMenu(ContextMenuItem& writingDirectionMenuItem)
{
    ContextMenu writingDirectionMenu;

    ContextMenuItem defaultItem(ActionType, ContextMenuItemTagDefaultDirection, 
        contextMenuItemTagDefaultDirection());
    ContextMenuItem ltr(CheckableActionType, ContextMenuItemTagLeftToRight, contextMenuItemTagLeftToRight());
    ContextMenuItem rtl(CheckableActionType, ContextMenuItemTagRightToLeft, contextMenuItemTagRightToLeft());

    appendItem(defaultItem, &writingDirectionMenu);
    appendItem(ltr, &writingDirectionMenu);
    appendItem(rtl, &writingDirectionMenu);

    writingDirectionMenuItem.setSubMenu(&writingDirectionMenu);
}

void ContextMenuController::createAndAppendTextDirectionSubMenu(ContextMenuItem& textDirectionMenuItem)
{
    ContextMenu textDirectionMenu;

    ContextMenuItem defaultItem(ActionType, ContextMenuItemTagTextDirectionDefault, contextMenuItemTagDefaultDirection());
    ContextMenuItem ltr(CheckableActionType, ContextMenuItemTagTextDirectionLeftToRight, contextMenuItemTagLeftToRight());
    ContextMenuItem rtl(CheckableActionType, ContextMenuItemTagTextDirectionRightToLeft, contextMenuItemTagRightToLeft());

    appendItem(defaultItem, &textDirectionMenu);
    appendItem(ltr, &textDirectionMenu);
    appendItem(rtl, &textDirectionMenu);

    textDirectionMenuItem.setSubMenu(&textDirectionMenu);
}

#endif

#if PLATFORM(MAC)

void ContextMenuController::createAndAppendSubstitutionsSubMenu(ContextMenuItem& substitutionsMenuItem)
{
    ContextMenu substitutionsMenu;

    ContextMenuItem showSubstitutions(ActionType, ContextMenuItemTagShowSubstitutions, contextMenuItemTagShowSubstitutions(true));
    ContextMenuItem smartCopyPaste(CheckableActionType, ContextMenuItemTagSmartCopyPaste, contextMenuItemTagSmartCopyPaste());
    ContextMenuItem smartQuotes(CheckableActionType, ContextMenuItemTagSmartQuotes, contextMenuItemTagSmartQuotes());
    ContextMenuItem smartDashes(CheckableActionType, ContextMenuItemTagSmartDashes, contextMenuItemTagSmartDashes());
    ContextMenuItem smartLinks(CheckableActionType, ContextMenuItemTagSmartLinks, contextMenuItemTagSmartLinks());
    ContextMenuItem textReplacement(CheckableActionType, ContextMenuItemTagTextReplacement, contextMenuItemTagTextReplacement());

    appendItem(showSubstitutions, &substitutionsMenu);
    appendItem(*separatorItem(), &substitutionsMenu);
    appendItem(smartCopyPaste, &substitutionsMenu);
    appendItem(smartQuotes, &substitutionsMenu);
    appendItem(smartDashes, &substitutionsMenu);
    appendItem(smartLinks, &substitutionsMenu);
    appendItem(textReplacement, &substitutionsMenu);

    substitutionsMenuItem.setSubMenu(&substitutionsMenu);
}

void ContextMenuController::createAndAppendTransformationsSubMenu(ContextMenuItem& transformationsMenuItem)
{
    ContextMenu transformationsMenu;

    ContextMenuItem makeUpperCase(ActionType, ContextMenuItemTagMakeUpperCase, contextMenuItemTagMakeUpperCase());
    ContextMenuItem makeLowerCase(ActionType, ContextMenuItemTagMakeLowerCase, contextMenuItemTagMakeLowerCase());
    ContextMenuItem capitalize(ActionType, ContextMenuItemTagCapitalize, contextMenuItemTagCapitalize());

    appendItem(makeUpperCase, &transformationsMenu);
    appendItem(makeLowerCase, &transformationsMenu);
    appendItem(capitalize, &transformationsMenu);

    transformationsMenuItem.setSubMenu(&transformationsMenu);
}

#endif

static bool selectionContainsPossibleWord(Frame* frame)
{
    // Current algorithm: look for a character that's not just a separator.
    for (TextIterator it(frame->selection()->toNormalizedRange().get()); !it.atEnd(); it.advance()) {
        int length = it.length();
        for (int i = 0; i < length; ++i)
            if (!(category(it.characterAt(i)) & (Separator_Space | Separator_Line | Separator_Paragraph)))
                return true;
    }
    return false;
}

#if PLATFORM(MAC)
#if __MAC_OS_X_VERSION_MIN_REQUIRED == 1060
#define INCLUDE_SPOTLIGHT_CONTEXT_MENU_ITEM 1
#else
#define INCLUDE_SPOTLIGHT_CONTEXT_MENU_ITEM 0
#endif
#endif

#if PLATFORM(MAC) || PLATFORM(QT)
#define SUPPORTS_TOGGLE_VIDEO_FULLSCREEN 1
#else
#define SUPPORTS_TOGGLE_VIDEO_FULLSCREEN 0
#endif

#if PLATFORM(MAC)
#define SUPPORTS_TOGGLE_SHOW_HIDE_MEDIA_CONTROLS 1
#else
#define SUPPORTS_TOGGLE_SHOW_HIDE_MEDIA_CONTROLS 0
#endif

void ContextMenuController::populate()
{
    ContextMenuItem OpenLinkItem(ActionType, ContextMenuItemTagOpenLink, contextMenuItemTagOpenLink());
    ContextMenuItem OpenLinkInNewWindowItem(ActionType, ContextMenuItemTagOpenLinkInNewWindow, 
        contextMenuItemTagOpenLinkInNewWindow());
    ContextMenuItem DownloadFileItem(ActionType, ContextMenuItemTagDownloadLinkToDisk, 
        contextMenuItemTagDownloadLinkToDisk());
    ContextMenuItem CopyLinkItem(ActionType, ContextMenuItemTagCopyLinkToClipboard, 
        contextMenuItemTagCopyLinkToClipboard());
    ContextMenuItem OpenImageInNewWindowItem(ActionType, ContextMenuItemTagOpenImageInNewWindow, 
        contextMenuItemTagOpenImageInNewWindow());
    ContextMenuItem DownloadImageItem(ActionType, ContextMenuItemTagDownloadImageToDisk, 
        contextMenuItemTagDownloadImageToDisk());
    ContextMenuItem CopyImageItem(ActionType, ContextMenuItemTagCopyImageToClipboard, 
        contextMenuItemTagCopyImageToClipboard());
#if PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
    ContextMenuItem CopyImageUrlItem(ActionType, ContextMenuItemTagCopyImageUrlToClipboard, 
        contextMenuItemTagCopyImageUrlToClipboard());
#endif
    ContextMenuItem OpenMediaInNewWindowItem(ActionType, ContextMenuItemTagOpenMediaInNewWindow, String());
    ContextMenuItem DownloadMediaItem(ActionType, ContextMenuItemTagDownloadMediaToDisk, String());
    ContextMenuItem CopyMediaLinkItem(ActionType, ContextMenuItemTagCopyMediaLinkToClipboard, String());
    ContextMenuItem MediaPlayPause(ActionType, ContextMenuItemTagMediaPlayPause, 
        contextMenuItemTagMediaPlay());
    ContextMenuItem MediaMute(ActionType, ContextMenuItemTagMediaMute,
        contextMenuItemTagMediaMute());
#if SUPPORTS_TOGGLE_SHOW_HIDE_MEDIA_CONTROLS
    ContextMenuItem ToggleMediaControls(ActionType, ContextMenuItemTagToggleMediaControls,
        contextMenuItemTagHideMediaControls());
#else
    ContextMenuItem ToggleMediaControls(CheckableActionType, ContextMenuItemTagToggleMediaControls, 
        contextMenuItemTagToggleMediaControls());
#endif
    ContextMenuItem ToggleMediaLoop(CheckableActionType, ContextMenuItemTagToggleMediaLoop, 
        contextMenuItemTagToggleMediaLoop());
    ContextMenuItem EnterVideoFullscreen(ActionType, ContextMenuItemTagEnterVideoFullscreen,
        contextMenuItemTagEnterVideoFullscreen());
    ContextMenuItem ToggleVideoFullscreen(ActionType, ContextMenuItemTagToggleVideoFullscreen,
        contextMenuItemTagEnterVideoFullscreen());
#if PLATFORM(MAC)
    ContextMenuItem SearchSpotlightItem(ActionType, ContextMenuItemTagSearchInSpotlight, 
        contextMenuItemTagSearchInSpotlight());
#endif
#if !PLATFORM(GTK)
    ContextMenuItem SearchWebItem(ActionType, ContextMenuItemTagSearchWeb, contextMenuItemTagSearchWeb());
#endif
    ContextMenuItem CopyItem(ActionType, ContextMenuItemTagCopy, contextMenuItemTagCopy());
    ContextMenuItem BackItem(ActionType, ContextMenuItemTagGoBack, contextMenuItemTagGoBack());
    ContextMenuItem ForwardItem(ActionType, ContextMenuItemTagGoForward,  contextMenuItemTagGoForward());
    ContextMenuItem StopItem(ActionType, ContextMenuItemTagStop, contextMenuItemTagStop());
    ContextMenuItem ReloadItem(ActionType, ContextMenuItemTagReload, contextMenuItemTagReload());
    ContextMenuItem OpenFrameItem(ActionType, ContextMenuItemTagOpenFrameInNewWindow, 
        contextMenuItemTagOpenFrameInNewWindow());
    ContextMenuItem NoGuessesItem(ActionType, ContextMenuItemTagNoGuessesFound, 
        contextMenuItemTagNoGuessesFound());
    ContextMenuItem IgnoreSpellingItem(ActionType, ContextMenuItemTagIgnoreSpelling, 
        contextMenuItemTagIgnoreSpelling());
    ContextMenuItem LearnSpellingItem(ActionType, ContextMenuItemTagLearnSpelling, 
        contextMenuItemTagLearnSpelling());
    ContextMenuItem IgnoreGrammarItem(ActionType, ContextMenuItemTagIgnoreGrammar, 
        contextMenuItemTagIgnoreGrammar());
    ContextMenuItem CutItem(ActionType, ContextMenuItemTagCut, contextMenuItemTagCut());
    ContextMenuItem PasteItem(ActionType, ContextMenuItemTagPaste, contextMenuItemTagPaste());
#if PLATFORM(GTK)
    ContextMenuItem DeleteItem(ActionType, ContextMenuItemTagDelete, contextMenuItemTagDelete());
#endif
#if PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(EFL)
    ContextMenuItem SelectAllItem(ActionType, ContextMenuItemTagSelectAll, contextMenuItemTagSelectAll());
#endif

    Node* node = m_hitTestResult.innerNonSharedNode();
    if (!node)
        return;
#if PLATFORM(GTK)
    if (!m_hitTestResult.isContentEditable() && (node->isElementNode() && toElement(node)->isFormControlElement()))
        return;
#endif
    Frame* frame = node->document()->frame();
    if (!frame)
        return;

    if (!m_hitTestResult.isContentEditable()) {
        FrameLoader* loader = frame->loader();
        KURL linkURL = m_hitTestResult.absoluteLinkURL();
        if (!linkURL.isEmpty()) {
            if (loader->client()->canHandleRequest(ResourceRequest(linkURL))) {
                appendItem(OpenLinkItem, m_contextMenu.get());
                appendItem(OpenLinkInNewWindowItem, m_contextMenu.get());
                appendItem(DownloadFileItem, m_contextMenu.get());
            }
#if PLATFORM(QT)
            if (m_hitTestResult.isSelected()) 
                appendItem(CopyItem, m_contextMenu.get());
#endif
            appendItem(CopyLinkItem, m_contextMenu.get());
        }

        KURL imageURL = m_hitTestResult.absoluteImageURL();
        if (!imageURL.isEmpty()) {
            if (!linkURL.isEmpty())
                appendItem(*separatorItem(), m_contextMenu.get());

            appendItem(OpenImageInNewWindowItem, m_contextMenu.get());
            appendItem(DownloadImageItem, m_contextMenu.get());
            if (imageURL.isLocalFile() || m_hitTestResult.image())
                appendItem(CopyImageItem, m_contextMenu.get());
#if PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
            appendItem(CopyImageUrlItem, m_contextMenu.get());
#endif
        }

        KURL mediaURL = m_hitTestResult.absoluteMediaURL();
        if (!mediaURL.isEmpty()) {
            if (!linkURL.isEmpty() || !imageURL.isEmpty())
                appendItem(*separatorItem(), m_contextMenu.get());

            appendItem(MediaPlayPause, m_contextMenu.get());
            appendItem(MediaMute, m_contextMenu.get());
            appendItem(ToggleMediaControls, m_contextMenu.get());
            appendItem(ToggleMediaLoop, m_contextMenu.get());
#if SUPPORTS_TOGGLE_VIDEO_FULLSCREEN
            appendItem(ToggleVideoFullscreen, m_contextMenu.get());
#else
            appendItem(EnterVideoFullscreen, m_contextMenu.get());
#endif
            appendItem(*separatorItem(), m_contextMenu.get());
            appendItem(CopyMediaLinkItem, m_contextMenu.get());
            appendItem(OpenMediaInNewWindowItem, m_contextMenu.get());
            if (loader->client()->canHandleRequest(ResourceRequest(mediaURL)))
                appendItem(DownloadMediaItem, m_contextMenu.get());
        }

        if (imageURL.isEmpty() && linkURL.isEmpty() && mediaURL.isEmpty()) {
            if (m_hitTestResult.isSelected()) {
                if (selectionContainsPossibleWord(frame)) {
#if PLATFORM(MAC)
                    String selectedString = frame->displayStringModifiedByEncoding(frame->editor().selectedText());
                    ContextMenuItem LookUpInDictionaryItem(ActionType, ContextMenuItemTagLookUpInDictionary, contextMenuItemTagLookUpInDictionary(selectedString));

#if INCLUDE_SPOTLIGHT_CONTEXT_MENU_ITEM
                    appendItem(SearchSpotlightItem, m_contextMenu.get());
#else
                    appendItem(LookUpInDictionaryItem, m_contextMenu.get());
#endif
#endif

#if !PLATFORM(GTK)
                    appendItem(SearchWebItem, m_contextMenu.get());
                    appendItem(*separatorItem(), m_contextMenu.get());
#endif

#if PLATFORM(MAC) && INCLUDE_SPOTLIGHT_CONTEXT_MENU_ITEM
                    appendItem(LookUpInDictionaryItem, m_contextMenu.get());
                    appendItem(*separatorItem(), m_contextMenu.get());
#endif
                }

                appendItem(CopyItem, m_contextMenu.get());
#if PLATFORM(MAC)
                appendItem(*separatorItem(), m_contextMenu.get());

                ContextMenuItem SpeechMenuItem(SubmenuType, ContextMenuItemTagSpeechMenu, contextMenuItemTagSpeechMenu());
                createAndAppendSpeechSubMenu(SpeechMenuItem);
                appendItem(SpeechMenuItem, m_contextMenu.get());
#endif                
            } else {
#if ENABLE(INSPECTOR)
                if (!(frame->page() && frame->page()->inspectorController()->hasInspectorFrontendClient())) {
#endif

                // In GTK+ unavailable items are not hidden but insensitive.
#if PLATFORM(GTK)
                appendItem(BackItem, m_contextMenu.get());
                appendItem(ForwardItem, m_contextMenu.get());
                appendItem(StopItem, m_contextMenu.get());
                appendItem(ReloadItem, m_contextMenu.get());
#else
                if (frame->page() && frame->page()->backForward()->canGoBackOrForward(-1))
                    appendItem(BackItem, m_contextMenu.get());

                if (frame->page() && frame->page()->backForward()->canGoBackOrForward(1))
                    appendItem(ForwardItem, m_contextMenu.get());

                // use isLoadingInAPISense rather than isLoading because Stop/Reload are
                // intended to match WebKit's API, not WebCore's internal notion of loading status
                if (loader->documentLoader()->isLoadingInAPISense())
                    appendItem(StopItem, m_contextMenu.get());
                else
                    appendItem(ReloadItem, m_contextMenu.get());
#endif
#if ENABLE(INSPECTOR)
                }
#endif

                if (frame->page() && frame != frame->page()->mainFrame())
                    appendItem(OpenFrameItem, m_contextMenu.get());
            }
        }
    } else { // Make an editing context menu
        FrameSelection* selection = frame->selection();
        bool inPasswordField = selection->isInPasswordField();
        if (!inPasswordField) {
            bool haveContextMenuItemsForMisspellingOrGrammer = false;
            bool spellCheckingEnabled = frame->editor().isSpellCheckingEnabledFor(node);
            if (spellCheckingEnabled) {
                // Consider adding spelling-related or grammar-related context menu items (never both, since a single selected range
                // is never considered a misspelling and bad grammar at the same time)
                bool misspelling;
                bool badGrammar;
                Vector<String> guesses = frame->editor().guessesForMisspelledOrUngrammatical(misspelling, badGrammar);
                if (misspelling || badGrammar) {
                    size_t size = guesses.size();
                    if (!size) {
                        // If there's bad grammar but no suggestions (e.g., repeated word), just leave off the suggestions
                        // list and trailing separator rather than adding a "No Guesses Found" item (matches AppKit)
                        if (misspelling) {
                            appendItem(NoGuessesItem, m_contextMenu.get());
                            appendItem(*separatorItem(), m_contextMenu.get());
                        }
                    } else {
                        for (unsigned i = 0; i < size; i++) {
                            const String &guess = guesses[i];
                            if (!guess.isEmpty()) {
                                ContextMenuItem item(ActionType, ContextMenuItemTagSpellingGuess, guess);
                                appendItem(item, m_contextMenu.get());
                            }
                        }
                        appendItem(*separatorItem(), m_contextMenu.get());
                    }
                    if (misspelling) {
                        appendItem(IgnoreSpellingItem, m_contextMenu.get());
                        appendItem(LearnSpellingItem, m_contextMenu.get());
                    } else
                        appendItem(IgnoreGrammarItem, m_contextMenu.get());
                    appendItem(*separatorItem(), m_contextMenu.get());
                    haveContextMenuItemsForMisspellingOrGrammer = true;
#if PLATFORM(MAC)
                } else {
                    // If the string was autocorrected, generate a contextual menu item allowing it to be changed back.
                    String replacedString = m_hitTestResult.replacedString();
                    if (!replacedString.isEmpty()) {
                        ContextMenuItem item(ActionType, ContextMenuItemTagChangeBack, contextMenuItemTagChangeBack(replacedString));
                        appendItem(item, m_contextMenu.get());
                        appendItem(*separatorItem(), m_contextMenu.get());
                        haveContextMenuItemsForMisspellingOrGrammer = true;
                    }
#endif
                }
            }

            if (!haveContextMenuItemsForMisspellingOrGrammer) {
                // Spelling and grammar checking is mutually exclusive with dictation alternatives.
                Vector<String> dictationAlternatives = m_hitTestResult.dictationAlternatives();
                if (!dictationAlternatives.isEmpty()) {
                    for (size_t i = 0; i < dictationAlternatives.size(); ++i) {
                        ContextMenuItem item(ActionType, ContextMenuItemTagDictationAlternative, dictationAlternatives[i]);
                        appendItem(item, m_contextMenu.get());
                    }
                    appendItem(*separatorItem(), m_contextMenu.get());
                }
            }
        }

        FrameLoader* loader = frame->loader();
        KURL linkURL = m_hitTestResult.absoluteLinkURL();
        if (!linkURL.isEmpty()) {
            if (loader->client()->canHandleRequest(ResourceRequest(linkURL))) {
                appendItem(OpenLinkItem, m_contextMenu.get());
                appendItem(OpenLinkInNewWindowItem, m_contextMenu.get());
                appendItem(DownloadFileItem, m_contextMenu.get());
            }
            appendItem(CopyLinkItem, m_contextMenu.get());
            appendItem(*separatorItem(), m_contextMenu.get());
        }

        if (m_hitTestResult.isSelected() && !inPasswordField && selectionContainsPossibleWord(frame)) {
#if PLATFORM(MAC)
            String selectedString = frame->displayStringModifiedByEncoding(frame->editor().selectedText());
            ContextMenuItem LookUpInDictionaryItem(ActionType, ContextMenuItemTagLookUpInDictionary, contextMenuItemTagLookUpInDictionary(selectedString));

#if INCLUDE_SPOTLIGHT_CONTEXT_MENU_ITEM
            appendItem(SearchSpotlightItem, m_contextMenu.get());
#else
            appendItem(LookUpInDictionaryItem, m_contextMenu.get());
#endif
#endif

#if !PLATFORM(GTK)
            appendItem(SearchWebItem, m_contextMenu.get());
            appendItem(*separatorItem(), m_contextMenu.get());
#endif

#if PLATFORM(MAC) && INCLUDE_SPOTLIGHT_CONTEXT_MENU_ITEM
            appendItem(LookUpInDictionaryItem, m_contextMenu.get());
            appendItem(*separatorItem(), m_contextMenu.get());
#endif
        }

        appendItem(CutItem, m_contextMenu.get());
        appendItem(CopyItem, m_contextMenu.get());
        appendItem(PasteItem, m_contextMenu.get());
#if PLATFORM(GTK)
        appendItem(DeleteItem, m_contextMenu.get());
        appendItem(*separatorItem(), m_contextMenu.get());
#endif
#if PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(EFL)
        appendItem(SelectAllItem, m_contextMenu.get());
#endif

        if (!inPasswordField) {
#if !PLATFORM(GTK)
            appendItem(*separatorItem(), m_contextMenu.get());
            ContextMenuItem SpellingAndGrammarMenuItem(SubmenuType, ContextMenuItemTagSpellingMenu, 
                contextMenuItemTagSpellingMenu());
            createAndAppendSpellingAndGrammarSubMenu(SpellingAndGrammarMenuItem);
            appendItem(SpellingAndGrammarMenuItem, m_contextMenu.get());
#endif
#if PLATFORM(MAC)
            ContextMenuItem substitutionsMenuItem(SubmenuType, ContextMenuItemTagSubstitutionsMenu, 
                contextMenuItemTagSubstitutionsMenu());
            createAndAppendSubstitutionsSubMenu(substitutionsMenuItem);
            appendItem(substitutionsMenuItem, m_contextMenu.get());
            ContextMenuItem transformationsMenuItem(SubmenuType, ContextMenuItemTagTransformationsMenu, 
                contextMenuItemTagTransformationsMenu());
            createAndAppendTransformationsSubMenu(transformationsMenuItem);
            appendItem(transformationsMenuItem, m_contextMenu.get());
#endif
#if PLATFORM(GTK)
            bool shouldShowFontMenu = frame->editor().canEditRichly();
#else
            bool shouldShowFontMenu = true;
#endif
            if (shouldShowFontMenu) {
                ContextMenuItem FontMenuItem(SubmenuType, ContextMenuItemTagFontMenu, 
                    contextMenuItemTagFontMenu());
                createAndAppendFontSubMenu(FontMenuItem);
                appendItem(FontMenuItem, m_contextMenu.get());
            }
#if PLATFORM(MAC)
            ContextMenuItem SpeechMenuItem(SubmenuType, ContextMenuItemTagSpeechMenu, contextMenuItemTagSpeechMenu());
            createAndAppendSpeechSubMenu(SpeechMenuItem);
            appendItem(SpeechMenuItem, m_contextMenu.get());
#endif
#if PLATFORM(GTK)
            EditorClient* client = frame->editor().client();
            if (client && client->shouldShowUnicodeMenu()) {
                ContextMenuItem UnicodeMenuItem(SubmenuType, ContextMenuItemTagUnicode, contextMenuItemTagUnicode());
                createAndAppendUnicodeSubMenu(UnicodeMenuItem);
                appendItem(*separatorItem(), m_contextMenu.get());
                appendItem(UnicodeMenuItem, m_contextMenu.get());
            }
#else
            ContextMenuItem WritingDirectionMenuItem(SubmenuType, ContextMenuItemTagWritingDirectionMenu, 
                contextMenuItemTagWritingDirectionMenu());
            createAndAppendWritingDirectionSubMenu(WritingDirectionMenuItem);
            appendItem(WritingDirectionMenuItem, m_contextMenu.get());
            if (Page* page = frame->page()) {
                if (Settings* settings = page->settings()) {
                    bool includeTextDirectionSubmenu = settings->textDirectionSubmenuInclusionBehavior() == TextDirectionSubmenuAlwaysIncluded
                        || (settings->textDirectionSubmenuInclusionBehavior() == TextDirectionSubmenuAutomaticallyIncluded && frame->editor().hasBidiSelection());
                    if (includeTextDirectionSubmenu) {
                        ContextMenuItem TextDirectionMenuItem(SubmenuType, ContextMenuItemTagTextDirectionMenu, 
                            contextMenuItemTagTextDirectionMenu());
                        createAndAppendTextDirectionSubMenu(TextDirectionMenuItem);
                        appendItem(TextDirectionMenuItem, m_contextMenu.get());
                    }
                }
            }
#endif
        }
    }
}

#if ENABLE(INSPECTOR)
void ContextMenuController::addInspectElementItem()
{
    Node* node = m_hitTestResult.innerNonSharedNode();
    if (!node)
        return;

    Frame* frame = node->document()->frame();
    if (!frame)
        return;

    Page* page = frame->page();
    if (!page)
        return;

    if (!page->inspectorController())
        return;

    ContextMenuItem InspectElementItem(ActionType, ContextMenuItemTagInspectElement, contextMenuItemTagInspectElement());
#if USE(CROSS_PLATFORM_CONTEXT_MENUS)
    if (m_contextMenu && !m_contextMenu->items().isEmpty())
#else
    if (m_contextMenu && m_contextMenu->itemCount())
#endif
        appendItem(*separatorItem(), m_contextMenu.get());
    appendItem(InspectElementItem, m_contextMenu.get());
}
#endif // ENABLE(INSPECTOR)

void ContextMenuController::checkOrEnableIfNeeded(ContextMenuItem& item) const
{
    if (item.type() == SeparatorType)
        return;
    
    Frame* frame = m_hitTestResult.innerNonSharedNode()->document()->frame();
    if (!frame)
        return;

    // Custom items already have proper checked and enabled values.
    if (ContextMenuItemBaseCustomTag <= item.action() && item.action() <= ContextMenuItemLastCustomTag)
        return;

    bool shouldEnable = true;
    bool shouldCheck = false; 

    switch (item.action()) {
        case ContextMenuItemTagCheckSpelling:
            shouldEnable = frame->editor().canEdit();
            break;
        case ContextMenuItemTagDefaultDirection:
            shouldCheck = false;
            shouldEnable = false;
            break;
        case ContextMenuItemTagLeftToRight:
        case ContextMenuItemTagRightToLeft: {
            String direction = item.action() == ContextMenuItemTagLeftToRight ? "ltr" : "rtl";
            shouldCheck = frame->editor().selectionHasStyle(CSSPropertyDirection, direction) != FalseTriState;
            shouldEnable = true;
            break;
        }
        case ContextMenuItemTagTextDirectionDefault: {
            Editor::Command command = frame->editor().command("MakeTextWritingDirectionNatural");
            shouldCheck = command.state() == TrueTriState;
            shouldEnable = command.isEnabled();
            break;
        }
        case ContextMenuItemTagTextDirectionLeftToRight: {
            Editor::Command command = frame->editor().command("MakeTextWritingDirectionLeftToRight");
            shouldCheck = command.state() == TrueTriState;
            shouldEnable = command.isEnabled();
            break;
        }
        case ContextMenuItemTagTextDirectionRightToLeft: {
            Editor::Command command = frame->editor().command("MakeTextWritingDirectionRightToLeft");
            shouldCheck = command.state() == TrueTriState;
            shouldEnable = command.isEnabled();
            break;
        }
        case ContextMenuItemTagCopy:
            shouldEnable = frame->editor().canDHTMLCopy() || frame->editor().canCopy();
            break;
        case ContextMenuItemTagCut:
            shouldEnable = frame->editor().canDHTMLCut() || frame->editor().canCut();
            break;
        case ContextMenuItemTagIgnoreSpelling:
        case ContextMenuItemTagLearnSpelling:
            shouldEnable = frame->selection()->isRange();
            break;
        case ContextMenuItemTagPaste:
            shouldEnable = frame->editor().canDHTMLPaste() || frame->editor().canPaste();
            break;
#if PLATFORM(GTK)
        case ContextMenuItemTagDelete:
            shouldEnable = frame->editor().canDelete();
            break;
        case ContextMenuItemTagInputMethods:
        case ContextMenuItemTagUnicode:
        case ContextMenuItemTagUnicodeInsertLRMMark:
        case ContextMenuItemTagUnicodeInsertRLMMark:
        case ContextMenuItemTagUnicodeInsertLREMark:
        case ContextMenuItemTagUnicodeInsertRLEMark:
        case ContextMenuItemTagUnicodeInsertLROMark:
        case ContextMenuItemTagUnicodeInsertRLOMark:
        case ContextMenuItemTagUnicodeInsertPDFMark:
        case ContextMenuItemTagUnicodeInsertZWSMark:
        case ContextMenuItemTagUnicodeInsertZWJMark:
        case ContextMenuItemTagUnicodeInsertZWNJMark:
            shouldEnable = true;
            break;
#endif
#if PLATFORM(GTK) || PLATFORM(EFL)
        case ContextMenuItemTagSelectAll:
            shouldEnable = true;
            break;
#endif
        case ContextMenuItemTagUnderline: {
            shouldCheck = frame->editor().selectionHasStyle(CSSPropertyWebkitTextDecorationsInEffect, "underline") != FalseTriState;
            shouldEnable = frame->editor().canEditRichly();
            break;
        }
        case ContextMenuItemTagLookUpInDictionary:
            shouldEnable = frame->selection()->isRange();
            break;
        case ContextMenuItemTagCheckGrammarWithSpelling:
            if (frame->editor().isGrammarCheckingEnabled())
                shouldCheck = true;
            shouldEnable = true;
            break;
        case ContextMenuItemTagItalic: {
            shouldCheck = frame->editor().selectionHasStyle(CSSPropertyFontStyle, "italic") != FalseTriState;
            shouldEnable = frame->editor().canEditRichly();
            break;
        }
        case ContextMenuItemTagBold: {
            shouldCheck = frame->editor().selectionHasStyle(CSSPropertyFontWeight, "bold") != FalseTriState;
            shouldEnable = frame->editor().canEditRichly();
            break;
        }
        case ContextMenuItemTagOutline:
            shouldEnable = false;
            break;
        case ContextMenuItemTagShowSpellingPanel:
            if (frame->editor().spellingPanelIsShowing())
                item.setTitle(contextMenuItemTagShowSpellingPanel(false));
            else
                item.setTitle(contextMenuItemTagShowSpellingPanel(true));
            shouldEnable = frame->editor().canEdit();
            break;
        case ContextMenuItemTagNoGuessesFound:
            shouldEnable = false;
            break;
        case ContextMenuItemTagCheckSpellingWhileTyping:
            shouldCheck = frame->editor().isContinuousSpellCheckingEnabled();
            break;
#if PLATFORM(MAC)
        case ContextMenuItemTagSubstitutionsMenu:
        case ContextMenuItemTagTransformationsMenu:
            break;
        case ContextMenuItemTagShowSubstitutions:
            if (frame->editor().substitutionsPanelIsShowing())
                item.setTitle(contextMenuItemTagShowSubstitutions(false));
            else
                item.setTitle(contextMenuItemTagShowSubstitutions(true));
            shouldEnable = frame->editor().canEdit();
            break;
        case ContextMenuItemTagMakeUpperCase:
        case ContextMenuItemTagMakeLowerCase:
        case ContextMenuItemTagCapitalize:
        case ContextMenuItemTagChangeBack:
            shouldEnable = frame->editor().canEdit();
            break;
        case ContextMenuItemTagCorrectSpellingAutomatically:
            shouldCheck = frame->editor().isAutomaticSpellingCorrectionEnabled();
            break;
        case ContextMenuItemTagSmartCopyPaste:
            shouldCheck = frame->editor().smartInsertDeleteEnabled();
            break;
        case ContextMenuItemTagSmartQuotes:
            shouldCheck = frame->editor().isAutomaticQuoteSubstitutionEnabled();
            break;
        case ContextMenuItemTagSmartDashes:
            shouldCheck = frame->editor().isAutomaticDashSubstitutionEnabled();
            break;
        case ContextMenuItemTagSmartLinks:
            shouldCheck = frame->editor().isAutomaticLinkDetectionEnabled();
            break;
        case ContextMenuItemTagTextReplacement:
            shouldCheck = frame->editor().isAutomaticTextReplacementEnabled();
            break;
        case ContextMenuItemTagStopSpeaking:
            shouldEnable = client() && client()->isSpeaking();
            break;
#else // PLATFORM(MAC) ends here
        case ContextMenuItemTagStopSpeaking:
            break;
#endif
#if PLATFORM(GTK)
        case ContextMenuItemTagGoBack:
            shouldEnable = frame->page() && frame->page()->backForward()->canGoBackOrForward(-1);
            break;
        case ContextMenuItemTagGoForward:
            shouldEnable = frame->page() && frame->page()->backForward()->canGoBackOrForward(1);
            break;
        case ContextMenuItemTagStop:
            shouldEnable = frame->loader()->documentLoader()->isLoadingInAPISense();
            break;
        case ContextMenuItemTagReload:
            shouldEnable = !frame->loader()->documentLoader()->isLoadingInAPISense();
            break;
        case ContextMenuItemTagFontMenu:
            shouldEnable = frame->editor().canEditRichly();
            break;
#else
        case ContextMenuItemTagGoBack:
        case ContextMenuItemTagGoForward:
        case ContextMenuItemTagStop:
        case ContextMenuItemTagReload:
        case ContextMenuItemTagFontMenu:
#endif
        case ContextMenuItemTagNoAction:
        case ContextMenuItemTagOpenLinkInNewWindow:
        case ContextMenuItemTagOpenLinkInThisWindow:
        case ContextMenuItemTagDownloadLinkToDisk:
        case ContextMenuItemTagCopyLinkToClipboard:
        case ContextMenuItemTagOpenImageInNewWindow:
        case ContextMenuItemTagDownloadImageToDisk:
        case ContextMenuItemTagCopyImageToClipboard:
#if PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
        case ContextMenuItemTagCopyImageUrlToClipboard:
#endif
            break;
        case ContextMenuItemTagOpenMediaInNewWindow:
            if (m_hitTestResult.mediaIsVideo())
                item.setTitle(contextMenuItemTagOpenVideoInNewWindow());
            else
                item.setTitle(contextMenuItemTagOpenAudioInNewWindow());
            break;
        case ContextMenuItemTagDownloadMediaToDisk:
            if (m_hitTestResult.mediaIsVideo())
                item.setTitle(contextMenuItemTagDownloadVideoToDisk());
            else
                item.setTitle(contextMenuItemTagDownloadAudioToDisk());
            break;
        case ContextMenuItemTagCopyMediaLinkToClipboard:
            if (m_hitTestResult.mediaIsVideo())
                item.setTitle(contextMenuItemTagCopyVideoLinkToClipboard());
            else
                item.setTitle(contextMenuItemTagCopyAudioLinkToClipboard());
            break;
        case ContextMenuItemTagToggleMediaControls:
#if SUPPORTS_TOGGLE_SHOW_HIDE_MEDIA_CONTROLS
            item.setTitle(m_hitTestResult.mediaControlsEnabled() ? contextMenuItemTagHideMediaControls() : contextMenuItemTagShowMediaControls());
#else
            shouldCheck = m_hitTestResult.mediaControlsEnabled();
#endif
            break;
        case ContextMenuItemTagToggleMediaLoop:
            shouldCheck = m_hitTestResult.mediaLoopEnabled();
            break;
        case ContextMenuItemTagToggleVideoFullscreen:
#if SUPPORTS_TOGGLE_VIDEO_FULLSCREEN
            item.setTitle(m_hitTestResult.mediaIsInFullscreen() ? contextMenuItemTagExitVideoFullscreen() : contextMenuItemTagEnterVideoFullscreen());
            break;
#endif
        case ContextMenuItemTagEnterVideoFullscreen:
            shouldEnable = m_hitTestResult.mediaSupportsFullscreen();
            break;
        case ContextMenuItemTagOpenFrameInNewWindow:
        case ContextMenuItemTagSpellingGuess:
        case ContextMenuItemTagOther:
        case ContextMenuItemTagSearchInSpotlight:
        case ContextMenuItemTagSearchWeb:
        case ContextMenuItemTagOpenWithDefaultApplication:
        case ContextMenuItemPDFActualSize:
        case ContextMenuItemPDFZoomIn:
        case ContextMenuItemPDFZoomOut:
        case ContextMenuItemPDFAutoSize:
        case ContextMenuItemPDFSinglePage:
        case ContextMenuItemPDFFacingPages:
        case ContextMenuItemPDFContinuous:
        case ContextMenuItemPDFNextPage:
        case ContextMenuItemPDFPreviousPage:
        case ContextMenuItemTagOpenLink:
        case ContextMenuItemTagIgnoreGrammar:
        case ContextMenuItemTagSpellingMenu:
        case ContextMenuItemTagShowFonts:
        case ContextMenuItemTagStyles:
        case ContextMenuItemTagShowColors:
        case ContextMenuItemTagSpeechMenu:
        case ContextMenuItemTagStartSpeaking:
        case ContextMenuItemTagWritingDirectionMenu:
        case ContextMenuItemTagTextDirectionMenu:
        case ContextMenuItemTagPDFSinglePageScrolling:
        case ContextMenuItemTagPDFFacingPagesScrolling:
#if ENABLE(INSPECTOR)
        case ContextMenuItemTagInspectElement:
#endif
        case ContextMenuItemBaseCustomTag:
        case ContextMenuItemCustomTagNoAction:
        case ContextMenuItemLastCustomTag:
        case ContextMenuItemBaseApplicationTag:
        case ContextMenuItemTagDictationAlternative:
            break;
        case ContextMenuItemTagMediaPlayPause:
            if (m_hitTestResult.mediaPlaying())
                item.setTitle(contextMenuItemTagMediaPause());
            else
                item.setTitle(contextMenuItemTagMediaPlay());
            break;
        case ContextMenuItemTagMediaMute:
            if (m_hitTestResult.mediaMuted())
                item.setTitle(contextMenuItemTagMediaUnmute());
            else
                item.setTitle(contextMenuItemTagMediaMute());
            break;
            shouldEnable = m_hitTestResult.mediaHasAudio();
            shouldCheck = shouldEnable &&  m_hitTestResult.mediaMuted();
            break;
    }

    item.setChecked(shouldCheck);
    item.setEnabled(shouldEnable);
}

#if USE(ACCESSIBILITY_CONTEXT_MENUS)
void ContextMenuController::showContextMenuAt(Frame* frame, const IntPoint& clickPoint)
{
    // Simulate a click in the middle of the accessibility object.
    PlatformMouseEvent mouseEvent(clickPoint, clickPoint, RightButton, PlatformEvent::MousePressed, 1, false, false, false, false, currentTime());
    bool handled = frame->eventHandler()->sendContextMenuEvent(mouseEvent);
    if (handled && client())
        client()->showContextMenu();
}
#endif

} // namespace WebCore

#endif // ENABLE(CONTEXT_MENUS)
