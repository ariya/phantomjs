/*
 * Copyright (C) 2006 Eric Seidel (eric@webkit.org)
 * Copyright (C) 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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

#ifndef EmptyClients_h
#define EmptyClients_h

#include "ChromeClient.h"
#include "ContextMenuClient.h"
#include "DeviceMotionClient.h"
#include "DeviceOrientationClient.h"
#include "DragClient.h"
#include "EditorClient.h"
#include "TextCheckerClient.h"
#include "FloatRect.h"
#include "FocusDirection.h"
#include "FrameLoaderClient.h"
#include "InspectorClient.h"
#include "Page.h"
#include "ResourceError.h"

/*
 This file holds empty Client stubs for use by WebCore.
 Viewless element needs to create a dummy Page->Frame->FrameView tree for use in parsing or executing JavaScript.
 This tree depends heavily on Clients (usually provided by WebKit classes).

 This file was first created for SVGImage as it had no way to access the current Page (nor should it,
 since Images are not tied to a page).
 See http://bugs.webkit.org/show_bug.cgi?id=5971 for the original discussion about this file.

 Ideally, whenever you change a Client class, you should add a stub here.
 Brittle, yes.  Unfortunate, yes.  Hopefully temporary.
*/

namespace WebCore {

class GraphicsContext3D;

class EmptyChromeClient : public ChromeClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~EmptyChromeClient() { }
    virtual void chromeDestroyed() { }

    virtual void setWindowRect(const FloatRect&) { }
    virtual FloatRect windowRect() { return FloatRect(); }

    virtual FloatRect pageRect() { return FloatRect(); }

    virtual void focus() { }
    virtual void unfocus() { }

    virtual bool canTakeFocus(FocusDirection) { return false; }
    virtual void takeFocus(FocusDirection) { }

    virtual void focusedNodeChanged(Node*) { }
    virtual void focusedFrameChanged(Frame*) { }

    virtual Page* createWindow(Frame*, const FrameLoadRequest&, const WindowFeatures&, const NavigationAction&) { return 0; }
    virtual void show() { }

    virtual bool canRunModal() { return false; }
    virtual void runModal() { }

    virtual void setToolbarsVisible(bool) { }
    virtual bool toolbarsVisible() { return false; }

    virtual void setStatusbarVisible(bool) { }
    virtual bool statusbarVisible() { return false; }

    virtual void setScrollbarsVisible(bool) { }
    virtual bool scrollbarsVisible() { return false; }

    virtual void setMenubarVisible(bool) { }
    virtual bool menubarVisible() { return false; }

    virtual void setResizable(bool) { }

    virtual void addMessageToConsole(MessageSource, MessageLevel, const String&, unsigned, unsigned, const String&, const String&) { }

    virtual bool canRunBeforeUnloadConfirmPanel() { return false; }
    virtual bool runBeforeUnloadConfirmPanel(const String&, Frame*) { return true; }

    virtual void closeWindowSoon() { }

    virtual void runJavaScriptAlert(Frame*, const String&) { }
    virtual bool runJavaScriptConfirm(Frame*, const String&) { return false; }
    virtual bool runJavaScriptPrompt(Frame*, const String&, const String&, String&) { return false; }
    virtual bool shouldInterruptJavaScript() { return false; }

    virtual bool selectItemWritingDirectionIsNatural() { return false; }
    virtual bool selectItemAlignmentFollowsMenuWritingDirection() { return false; }
    virtual bool hasOpenedPopup() const OVERRIDE { return false; }
    virtual PassRefPtr<PopupMenu> createPopupMenu(PopupMenuClient*) const OVERRIDE;
    virtual PassRefPtr<SearchPopupMenu> createSearchPopupMenu(PopupMenuClient*) const OVERRIDE;

    virtual void setStatusbarText(const String&) { }

    virtual KeyboardUIMode keyboardUIMode() { return KeyboardAccessDefault; }

    virtual IntRect windowResizerRect() const { return IntRect(); }

    virtual void invalidateRootView(const IntRect&, bool) OVERRIDE { }
    virtual void invalidateContentsAndRootView(const IntRect&, bool) OVERRIDE { }
    virtual void invalidateContentsForSlowScroll(const IntRect&, bool) OVERRIDE { }
    virtual void scroll(const IntSize&, const IntRect&, const IntRect&) { }
#if USE(TILED_BACKING_STORE)
    virtual void delegatedScrollRequested(const IntPoint&) { }
#endif
#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
    virtual void scheduleAnimation() { }
#endif

    virtual IntPoint screenToRootView(const IntPoint& p) const OVERRIDE { return p; }
    virtual IntRect rootViewToScreen(const IntRect& r) const OVERRIDE { return r; }
    virtual PlatformPageClient platformPageClient() const { return 0; }
    virtual void contentsSizeChanged(Frame*, const IntSize&) const { }

    virtual void scrollbarsModeDidChange() const { }
    virtual void mouseDidMoveOverElement(const HitTestResult&, unsigned) { }

    virtual void setToolTip(const String&, TextDirection) { }

    virtual void print(Frame*) { }

#if ENABLE(SQL_DATABASE)
    virtual void exceededDatabaseQuota(Frame*, const String&, DatabaseDetails) { }
#endif

    virtual void reachedMaxAppCacheSize(int64_t) { }
    virtual void reachedApplicationCacheOriginQuota(SecurityOrigin*, int64_t) { }

#if ENABLE(DIRECTORY_UPLOAD)
    virtual void enumerateChosenDirectory(FileChooser*) { }
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    virtual PassOwnPtr<ColorChooser> createColorChooser(ColorChooserClient*, const Color&) OVERRIDE;
#endif

#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
    virtual PassRefPtr<DateTimeChooser> openDateTimeChooser(DateTimeChooserClient*, const DateTimeChooserParameters&) OVERRIDE;
#endif

    virtual void runOpenPanel(Frame*, PassRefPtr<FileChooser>) OVERRIDE;
    virtual void loadIconForFiles(const Vector<String>&, FileIconLoader*) { }

    virtual void formStateDidChange(const Node*) { }

    virtual void elementDidFocus(const Node*) { }
    virtual void elementDidBlur(const Node*) { }

    virtual void setCursor(const Cursor&) { }
    virtual void setCursorHiddenUntilMouseMoves(bool) { }

    virtual void scrollRectIntoView(const IntRect&) const { }

#if USE(ACCELERATED_COMPOSITING)
    virtual void attachRootGraphicsLayer(Frame*, GraphicsLayer*) { }
    virtual void setNeedsOneShotDrawingSynchronization() { }
    virtual void scheduleCompositingLayerFlush() { }
#endif

#if PLATFORM(WIN)
    virtual void setLastSetCursorToCurrentCursor() { }
    virtual void AXStartFrameLoad() { }
    virtual void AXFinishFrameLoad() { }
#endif
#if ENABLE(TOUCH_EVENTS)
    virtual void needTouchEvents(bool) { }
#endif
    
    virtual void numWheelEventHandlersChanged(unsigned) OVERRIDE { }
    
    virtual bool shouldRubberBandInDirection(WebCore::ScrollDirection) const { return false; }
    
    virtual bool isEmptyChromeClient() const { return true; }

    virtual void didAssociateFormControls(const Vector<RefPtr<Element> >&) { }
    virtual bool shouldNotifyOnFormChanges() { return false; }
};

// FIXME (bug 116233): Get rid of EmptyFrameLoaderClient. It is a travesty.

class EmptyFrameLoaderClient : public FrameLoaderClient {
    WTF_MAKE_NONCOPYABLE(EmptyFrameLoaderClient); WTF_MAKE_FAST_ALLOCATED;
public:
    EmptyFrameLoaderClient() { }
    virtual ~EmptyFrameLoaderClient() {  }
    virtual void frameLoaderDestroyed() { }

    virtual bool hasWebView() const { return true; } // mainly for assertions

    virtual void makeRepresentation(DocumentLoader*) { }
    virtual void forceLayout() { }
    virtual void forceLayoutForNonHTML() { }

    virtual void setCopiesOnScroll() { }

    virtual void detachedFromParent2() { }
    virtual void detachedFromParent3() { }

    virtual void convertMainResourceLoadToDownload(DocumentLoader*, const ResourceRequest&, const ResourceResponse&) OVERRIDE { }

    virtual void assignIdentifierToInitialRequest(unsigned long, DocumentLoader*, const ResourceRequest&) { }
    virtual bool shouldUseCredentialStorage(DocumentLoader*, unsigned long) { return false; }
    virtual void dispatchWillSendRequest(DocumentLoader*, unsigned long, ResourceRequest&, const ResourceResponse&) { }
    virtual void dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, unsigned long, const AuthenticationChallenge&) { }
    virtual void dispatchDidCancelAuthenticationChallenge(DocumentLoader*, unsigned long, const AuthenticationChallenge&) { }
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    virtual bool canAuthenticateAgainstProtectionSpace(DocumentLoader*, unsigned long, const ProtectionSpace&) { return false; }
#endif
    virtual void dispatchDidReceiveResponse(DocumentLoader*, unsigned long, const ResourceResponse&) { }
    virtual void dispatchDidReceiveContentLength(DocumentLoader*, unsigned long, int) { }
    virtual void dispatchDidFinishLoading(DocumentLoader*, unsigned long) { }
    virtual void dispatchDidFailLoading(DocumentLoader*, unsigned long, const ResourceError&) { }
    virtual bool dispatchDidLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int) { return false; }

    virtual void dispatchDidHandleOnloadEvents() { }
    virtual void dispatchDidReceiveServerRedirectForProvisionalLoad() { }
    virtual void dispatchDidCancelClientRedirect() { }
    virtual void dispatchWillPerformClientRedirect(const KURL&, double, double) { }
    virtual void dispatchDidChangeLocationWithinPage() { }
    virtual void dispatchDidPushStateWithinPage() { }
    virtual void dispatchDidReplaceStateWithinPage() { }
    virtual void dispatchDidPopStateWithinPage() { }
    virtual void dispatchWillClose() { }
    virtual void dispatchDidReceiveIcon() { }
    virtual void dispatchDidStartProvisionalLoad() { }
    virtual void dispatchDidReceiveTitle(const StringWithDirection&) { }
    virtual void dispatchDidChangeIcons(IconType) { }
    virtual void dispatchDidCommitLoad() { }
    virtual void dispatchDidFailProvisionalLoad(const ResourceError&) { }
    virtual void dispatchDidFailLoad(const ResourceError&) { }
    virtual void dispatchDidFinishDocumentLoad() { }
    virtual void dispatchDidFinishLoad() { }
    virtual void dispatchDidLayout(LayoutMilestones) { }

    virtual Frame* dispatchCreatePage(const NavigationAction&) { return 0; }
    virtual void dispatchShow() { }

    virtual void dispatchDecidePolicyForResponse(FramePolicyFunction, const ResourceResponse&, const ResourceRequest&) { }
    virtual void dispatchDecidePolicyForNewWindowAction(FramePolicyFunction, const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>, const String&) OVERRIDE;
    virtual void dispatchDecidePolicyForNavigationAction(FramePolicyFunction, const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>) OVERRIDE;
    virtual void cancelPolicyCheck() { }

    virtual void dispatchUnableToImplementPolicy(const ResourceError&) { }

    virtual void dispatchWillSendSubmitEvent(PassRefPtr<FormState>) OVERRIDE;
    virtual void dispatchWillSubmitForm(FramePolicyFunction, PassRefPtr<FormState>) OVERRIDE;

    virtual void revertToProvisionalState(DocumentLoader*) { }
    virtual void setMainDocumentError(DocumentLoader*, const ResourceError&) { }

    virtual void willChangeEstimatedProgress() { }
    virtual void didChangeEstimatedProgress() { }
    virtual void postProgressStartedNotification() { }
    virtual void postProgressEstimateChangedNotification() { }
    virtual void postProgressFinishedNotification() { }

    virtual void setMainFrameDocumentReady(bool) { }

    virtual void startDownload(const ResourceRequest&, const String& suggestedName = String()) { UNUSED_PARAM(suggestedName); }

    virtual void willChangeTitle(DocumentLoader*) { }
    virtual void didChangeTitle(DocumentLoader*) { }

    virtual void committedLoad(DocumentLoader*, const char*, int) { }
    virtual void finishedLoading(DocumentLoader*) { }

    virtual ResourceError cancelledError(const ResourceRequest&) { ResourceError error("", 0, "", ""); error.setIsCancellation(true); return error; }
    virtual ResourceError blockedError(const ResourceRequest&) { return ResourceError("", 0, "", ""); }
    virtual ResourceError cannotShowURLError(const ResourceRequest&) { return ResourceError("", 0, "", ""); }
    virtual ResourceError interruptedForPolicyChangeError(const ResourceRequest&) { return ResourceError("", 0, "", ""); }

    virtual ResourceError cannotShowMIMETypeError(const ResourceResponse&) { return ResourceError("", 0, "", ""); }
    virtual ResourceError fileDoesNotExistError(const ResourceResponse&) { return ResourceError("", 0, "", ""); }
    virtual ResourceError pluginWillHandleLoadError(const ResourceResponse&) { return ResourceError("", 0, "", ""); }

    virtual bool shouldFallBack(const ResourceError&) { return false; }

    virtual bool canHandleRequest(const ResourceRequest&) const { return false; }
    virtual bool canShowMIMEType(const String&) const { return false; }
    virtual bool canShowMIMETypeAsHTML(const String&) const { return false; }
    virtual bool representationExistsForURLScheme(const String&) const { return false; }
    virtual String generatedMIMETypeForURLScheme(const String&) const { return ""; }

    virtual void frameLoadCompleted() { }
    virtual void restoreViewState() { }
    virtual void provisionalLoadStarted() { }
    virtual bool shouldTreatURLAsSameAsCurrent(const KURL&) const { return false; }
    virtual void didFinishLoad() { }
    virtual void prepareForDataSourceReplacement() { }

    virtual PassRefPtr<DocumentLoader> createDocumentLoader(const ResourceRequest&, const SubstituteData&) OVERRIDE;
    virtual void setTitle(const StringWithDirection&, const KURL&) { }

    virtual String userAgent(const KURL&) { return ""; }

    virtual void savePlatformDataToCachedFrame(CachedFrame*) { }
    virtual void transitionToCommittedFromCachedFrame(CachedFrame*) { }
    virtual void transitionToCommittedForNewPage() { }    

    virtual void didSaveToPageCache() { }
    virtual void didRestoreFromPageCache() { }

    virtual void dispatchDidBecomeFrameset(bool) { }

    virtual void updateGlobalHistory() { }
    virtual void updateGlobalHistoryRedirectLinks() { }
    virtual bool shouldGoToHistoryItem(HistoryItem*) const { return false; }
    virtual bool shouldStopLoadingForHistoryItem(HistoryItem*) const { return false; }
    virtual void updateGlobalHistoryItemForPage() { }
    virtual void saveViewStateToItem(HistoryItem*) { }
    virtual bool canCachePage() const { return false; }
    virtual void didDisplayInsecureContent() { }
    virtual void didRunInsecureContent(SecurityOrigin*, const KURL&) { }
    virtual void didDetectXSS(const KURL&, bool) { }
    virtual PassRefPtr<Frame> createFrame(const KURL&, const String&, HTMLFrameOwnerElement*, const String&, bool, int, int) OVERRIDE;
    virtual PassRefPtr<Widget> createPlugin(const IntSize&, HTMLPlugInElement*, const KURL&, const Vector<String>&, const Vector<String>&, const String&, bool) OVERRIDE;
    virtual void recreatePlugin(Widget*) OVERRIDE;
    virtual PassRefPtr<Widget> createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const KURL&, const Vector<String>&, const Vector<String>&) OVERRIDE;
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    virtual PassRefPtr<Widget> createMediaPlayerProxyPlugin(const IntSize&, HTMLMediaElement*, const KURL&, const Vector<String>&, const Vector<String>&, const String&) OVERRIDE;
    virtual void hideMediaPlayerProxyPlugin(Widget*) { }
    virtual void showMediaPlayerProxyPlugin(Widget*) { }
#endif

    virtual ObjectContentType objectContentType(const KURL&, const String&, bool) { return ObjectContentType(); }
    virtual String overrideMediaType() const { return String(); }

    virtual void redirectDataToPlugin(Widget*) { }
    virtual void dispatchDidClearWindowObjectInWorld(DOMWrapperWorld*) { }
    virtual void documentElementAvailable() { }
    virtual void didPerformFirstNavigation() const { }

    virtual void registerForIconNotification(bool) { }

#if PLATFORM(MAC)
    virtual RemoteAXObjectRef accessibilityRemoteObject() { return 0; }
    virtual NSCachedURLResponse* willCacheResponse(DocumentLoader*, unsigned long, NSCachedURLResponse* response) const { return response; }
#endif
#if PLATFORM(WIN) && USE(CFNETWORK)
    // FIXME: Windows should use willCacheResponse - <https://bugs.webkit.org/show_bug.cgi?id=57257>.
    virtual bool shouldCacheResponse(DocumentLoader*, unsigned long, const ResourceResponse&, const unsigned char*, unsigned long long) { return true; }
#endif

    virtual PassRefPtr<FrameNetworkingContext> createNetworkingContext() OVERRIDE;

    virtual bool isEmptyFrameLoaderClient() OVERRIDE { return true; }
};

class EmptyTextCheckerClient : public TextCheckerClient {
public:
    virtual bool shouldEraseMarkersAfterChangeSelection(TextCheckingType) const { return true; }
    virtual void ignoreWordInSpellDocument(const String&) { }
    virtual void learnWord(const String&) { }
    virtual void checkSpellingOfString(const UChar*, int, int*, int*) { }
    virtual String getAutoCorrectSuggestionForMisspelledWord(const String&) { return String(); }
    virtual void checkGrammarOfString(const UChar*, int, Vector<GrammarDetail>&, int*, int*) { }

#if USE(UNIFIED_TEXT_CHECKING)
    virtual void checkTextOfParagraph(const UChar*, int, TextCheckingTypeMask, Vector<TextCheckingResult>&) { };
#endif

    virtual void getGuessesForWord(const String&, const String&, Vector<String>&) { }
    virtual void requestCheckingOfString(PassRefPtr<TextCheckingRequest>) OVERRIDE;
};

class EmptyEditorClient : public EditorClient {
    WTF_MAKE_NONCOPYABLE(EmptyEditorClient); WTF_MAKE_FAST_ALLOCATED;
public:
    EmptyEditorClient() { }
    virtual ~EmptyEditorClient() { }
    virtual void pageDestroyed() { }
    virtual void frameWillDetachPage(Frame*) { }

    virtual bool shouldDeleteRange(Range*) { return false; }
    virtual bool smartInsertDeleteEnabled() { return false; }
    virtual bool isSelectTrailingWhitespaceEnabled() { return false; }
    virtual bool isContinuousSpellCheckingEnabled() { return false; }
    virtual void toggleContinuousSpellChecking() { }
    virtual bool isGrammarCheckingEnabled() { return false; }
    virtual void toggleGrammarChecking() { }
    virtual int spellCheckerDocumentTag() { return -1; }

    virtual bool selectWordBeforeMenuEvent() { return false; }
    virtual bool isEditable() { return false; }

    virtual bool shouldBeginEditing(Range*) { return false; }
    virtual bool shouldEndEditing(Range*) { return false; }
    virtual bool shouldInsertNode(Node*, Range*, EditorInsertAction) { return false; }
    virtual bool shouldInsertText(const String&, Range*, EditorInsertAction) { return false; }
    virtual bool shouldChangeSelectedRange(Range*, Range*, EAffinity, bool) { return false; }

    virtual bool shouldApplyStyle(StylePropertySet*, Range*) { return false; }
    virtual bool shouldMoveRangeAfterDelete(Range*, Range*) { return false; }

    virtual void didBeginEditing() { }
    virtual void respondToChangedContents() { }
    virtual void respondToChangedSelection(Frame*) { }
    virtual void didEndEditing() { }
    virtual void willWriteSelectionToPasteboard(Range*) { }
    virtual void didWriteSelectionToPasteboard() { }
    virtual void getClientPasteboardDataForRange(Range*, Vector<String>&, Vector<RefPtr<SharedBuffer> >&) { }
    virtual void didSetSelectionTypesForPasteboard() { }

    virtual void registerUndoStep(PassRefPtr<UndoStep>) OVERRIDE;
    virtual void registerRedoStep(PassRefPtr<UndoStep>) OVERRIDE;
    virtual void clearUndoRedoOperations() { }

    virtual bool canCopyCut(Frame*, bool defaultValue) const { return defaultValue; }
    virtual bool canPaste(Frame*, bool defaultValue) const { return defaultValue; }
    virtual bool canUndo() const { return false; }
    virtual bool canRedo() const { return false; }

    virtual void undo() { }
    virtual void redo() { }

    virtual void handleKeyboardEvent(KeyboardEvent*) { }
    virtual void handleInputMethodKeydown(KeyboardEvent*) { }

    virtual void textFieldDidBeginEditing(Element*) { }
    virtual void textFieldDidEndEditing(Element*) { }
    virtual void textDidChangeInTextField(Element*) { }
    virtual bool doTextFieldCommandFromEvent(Element*, KeyboardEvent*) { return false; }
    virtual void textWillBeDeletedInTextField(Element*) { }
    virtual void textDidChangeInTextArea(Element*) { }

#if PLATFORM(MAC)
    virtual void markedTextAbandoned(Frame*) { }

    virtual NSString* userVisibleString(NSURL*) { return 0; }
    virtual DocumentFragment* documentFragmentFromAttributedString(NSAttributedString*, Vector<RefPtr<ArchiveResource> >&) { return 0; };
    virtual void setInsertionPasteboard(const String&) { };
    virtual NSURL* canonicalizeURL(NSURL*) { return 0; }
    virtual NSURL* canonicalizeURLString(NSString*) { return 0; }
#endif

#if USE(APPKIT)
    virtual void uppercaseWord() { }
    virtual void lowercaseWord() { }
    virtual void capitalizeWord() { }
#endif

#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    virtual void showSubstitutionsPanel(bool) { }
    virtual bool substitutionsPanelIsShowing() { return false; }
    virtual void toggleSmartInsertDelete() { }
    virtual bool isAutomaticQuoteSubstitutionEnabled() { return false; }
    virtual void toggleAutomaticQuoteSubstitution() { }
    virtual bool isAutomaticLinkDetectionEnabled() { return false; }
    virtual void toggleAutomaticLinkDetection() { }
    virtual bool isAutomaticDashSubstitutionEnabled() { return false; }
    virtual void toggleAutomaticDashSubstitution() { }
    virtual bool isAutomaticTextReplacementEnabled() { return false; }
    virtual void toggleAutomaticTextReplacement() { }
    virtual bool isAutomaticSpellingCorrectionEnabled() { return false; }
    virtual void toggleAutomaticSpellingCorrection() { }
#endif

#if ENABLE(DELETION_UI)
    virtual bool shouldShowDeleteInterface(HTMLElement*) { return false; }
#endif

#if PLATFORM(GTK)
    virtual bool shouldShowUnicodeMenu() { return false; }
#endif
    TextCheckerClient* textChecker() { return &m_textCheckerClient; }

    virtual void updateSpellingUIWithGrammarString(const String&, const GrammarDetail&) { }
    virtual void updateSpellingUIWithMisspelledWord(const String&) { }
    virtual void showSpellingUI(bool) { }
    virtual bool spellingUIIsShowing() { return false; }

    virtual void willSetInputMethodState() { }
    virtual void setInputMethodState(bool) { }

private:
    EmptyTextCheckerClient m_textCheckerClient;
};

#if ENABLE(CONTEXT_MENUS)
class EmptyContextMenuClient : public ContextMenuClient {
    WTF_MAKE_NONCOPYABLE(EmptyContextMenuClient); WTF_MAKE_FAST_ALLOCATED;
public:
    EmptyContextMenuClient() { }
    virtual ~EmptyContextMenuClient() {  }
    virtual void contextMenuDestroyed() { }

#if USE(CROSS_PLATFORM_CONTEXT_MENUS)
    virtual PassOwnPtr<ContextMenu> customizeMenu(PassOwnPtr<ContextMenu>) OVERRIDE;
#else
    virtual PlatformMenuDescription getCustomMenuFromDefaultItems(ContextMenu*) { return 0; }
#endif
    virtual void contextMenuItemSelected(ContextMenuItem*, const ContextMenu*) { }

    virtual void downloadURL(const KURL&) { }
    virtual void copyImageToClipboard(const HitTestResult&) { }
    virtual void searchWithGoogle(const Frame*) { }
    virtual void lookUpInDictionary(Frame*) { }
    virtual bool isSpeaking() { return false; }
    virtual void speak(const String&) { }
    virtual void stopSpeaking() { }

#if PLATFORM(MAC)
    virtual void searchWithSpotlight() { }
#endif

#if USE(ACCESSIBILITY_CONTEXT_MENUS)
    virtual void showContextMenu() { }
#endif
};
#endif // ENABLE(CONTEXT_MENUS)

#if ENABLE(DRAG_SUPPORT)
class EmptyDragClient : public DragClient {
    WTF_MAKE_NONCOPYABLE(EmptyDragClient); WTF_MAKE_FAST_ALLOCATED;
public:
    EmptyDragClient() { }
    virtual ~EmptyDragClient() {}
    virtual void willPerformDragDestinationAction(DragDestinationAction, DragData*) { }
    virtual void willPerformDragSourceAction(DragSourceAction, const IntPoint&, Clipboard*) { }
    virtual DragDestinationAction actionMaskForDrag(DragData*) { return DragDestinationActionNone; }
    virtual DragSourceAction dragSourceActionMaskForPoint(const IntPoint&) { return DragSourceActionNone; }
    virtual void startDrag(DragImageRef, const IntPoint&, const IntPoint&, Clipboard*, Frame*, bool) { }
    virtual void dragControllerDestroyed() { }
};
#endif // ENABLE(DRAG_SUPPORT)

class EmptyInspectorClient : public InspectorClient {
    WTF_MAKE_NONCOPYABLE(EmptyInspectorClient); WTF_MAKE_FAST_ALLOCATED;
public:
    EmptyInspectorClient() { }
    virtual ~EmptyInspectorClient() { }

    virtual void inspectorDestroyed() { }
    
    virtual InspectorFrontendChannel* openInspectorFrontend(InspectorController*) { return 0; }
    virtual void closeInspectorFrontend() { }
    virtual void bringFrontendToFront() { }

    virtual void highlight() { }
    virtual void hideHighlight() { }
};

class EmptyDeviceClient : public DeviceClient {
public:
    virtual void startUpdating() OVERRIDE { }
    virtual void stopUpdating() OVERRIDE { }
};

class EmptyDeviceMotionClient : public DeviceMotionClient {
public:
    virtual void setController(DeviceMotionController*) { }
    virtual DeviceMotionData* lastMotion() const { return 0; }
    virtual void deviceMotionControllerDestroyed() { }
};

class EmptyDeviceOrientationClient : public DeviceOrientationClient {
public:
    virtual void setController(DeviceOrientationController*) { }
    virtual DeviceOrientationData* lastOrientation() const { return 0; }
    virtual void deviceOrientationControllerDestroyed() { }
};

void fillWithEmptyClients(Page::PageClients&);

}

#endif // EmptyClients_h
