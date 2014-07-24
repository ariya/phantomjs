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

#ifndef WebPageProxy_h
#define WebPageProxy_h

#include "APIObject.h"
#include "Connection.h"
#include "DragControllerAction.h"
#include "DrawingAreaProxy.h"
#include "EditorState.h"
#include "GeolocationPermissionRequestManagerProxy.h"
#include "LayerTreeContext.h"
#include "NotificationPermissionRequestManagerProxy.h"
#include "PlatformProcessIdentifier.h"
#include "SandboxExtension.h"
#include "ShareableBitmap.h"
#include "WKBase.h"
#include "WKPagePrivate.h"
#include "WebColorPicker.h"
#include "WebContextMenuItemData.h"
#include "WebCoreArgumentCoders.h"
#include "WebFindClient.h"
#include "WebFormClient.h"
#include "WebFrameProxy.h"
#include "WebHistoryClient.h"
#include "WebHitTestResult.h"
#include "WebLoaderClient.h"
#include "WebPageContextMenuClient.h"
#include <WebCore/AlternativeTextClient.h> // FIXME: Needed by WebPageProxyMessages.h for DICTATION_ALTERNATIVES.
#include "WebPageProxyMessages.h"
#include "WebPolicyClient.h"
#include "WebPopupMenuProxy.h"
#include "WebUIClient.h"
#include <WebCore/Color.h>
#include <WebCore/DragActions.h>
#include <WebCore/DragSession.h>
#include <WebCore/HitTestResult.h>
#include <WebCore/Page.h>
#include <WebCore/PlatformScreen.h>
#include <WebCore/ScrollTypes.h>
#include <WebCore/TextChecking.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

#if ENABLE(DRAG_SUPPORT)
#include <WebCore/DragActions.h>
#include <WebCore/DragSession.h>
#endif

#if ENABLE(TOUCH_EVENTS)
#include "NativeWebTouchEvent.h"
#endif

#if PLATFORM(EFL)
#include "WKPageEfl.h"
#include "WebUIPopupMenuClient.h"
#include <Evas.h>
#endif

#if PLATFORM(QT)
#include "QtNetworkRequestData.h"
#endif

namespace CoreIPC {
    class ArgumentDecoder;
    class Connection;
}

namespace WebCore {
    class AuthenticationChallenge;
    class Cursor;
    class DragData;
    class FloatRect;
    class GraphicsLayer;
    class IntSize;
    class ProtectionSpace;
    class SharedBuffer;
    struct FileChooserSettings;
    struct TextAlternativeWithRange;
    struct TextCheckingResult;
    struct ViewportAttributes;
    struct WindowFeatures;
}

#if PLATFORM(QT)
class QQuickNetworkReply;
#endif

#if USE(APPKIT)
#ifdef __OBJC__
@class WKView;
#else
class WKView;
#endif
#endif

#if PLATFORM(GTK)
typedef GtkWidget* PlatformWidget;
#endif

namespace WebKit {

class NativeWebKeyboardEvent;
class NativeWebMouseEvent;
class NativeWebWheelEvent;
class PageClient;
class PlatformCertificateInfo;
class StringPairVector;
class WebBackForwardList;
class WebBackForwardListItem;
class WebColorPickerResultListenerProxy;
class WebContextMenuProxy;
class WebData;
class WebEditCommandProxy;
class WebFullScreenManagerProxy;
class WebKeyboardEvent;
class WebMouseEvent;
class WebOpenPanelResultListenerProxy;
class WebPageGroup;
class WebProcessProxy;
class WebURLRequest;
class WebWheelEvent;
struct AttributedString;
struct ColorSpaceData;
struct DictionaryPopupInfo;
struct EditorState;
struct PlatformPopupMenuData;
struct PrintInfo;
struct WebPageCreationParameters;
struct WebPopupItem;

#if ENABLE(GESTURE_EVENTS)
class WebGestureEvent;
#endif

#if ENABLE(VIBRATION)
class WebVibrationProxy;
#endif

typedef GenericCallback<WKStringRef, StringImpl*> StringCallback;
typedef GenericCallback<WKSerializedScriptValueRef, WebSerializedScriptValue*> ScriptValueCallback;

#if PLATFORM(GTK)
typedef GenericCallback<WKErrorRef> PrintFinishedCallback;
#endif

#if ENABLE(TOUCH_EVENTS)
struct QueuedTouchEvents {
    QueuedTouchEvents(const NativeWebTouchEvent& event)
        : forwardedEvent(event)
    {
    }
    NativeWebTouchEvent forwardedEvent;
    Vector<NativeWebTouchEvent> deferredTouchEvents;
};
#endif

// FIXME: Make a version of CallbackBase with three arguments, and define ValidateCommandCallback as a specialization.
class ValidateCommandCallback : public CallbackBase {
public:
    typedef void (*CallbackFunction)(WKStringRef, bool, int32_t, WKErrorRef, void*);

    static PassRefPtr<ValidateCommandCallback> create(void* context, CallbackFunction callback)
    {
        return adoptRef(new ValidateCommandCallback(context, callback));
    }

    virtual ~ValidateCommandCallback()
    {
        ASSERT(!m_callback);
    }

    void performCallbackWithReturnValue(StringImpl* returnValue1, bool returnValue2, int returnValue3)
    {
        ASSERT(m_callback);

        m_callback(toAPI(returnValue1), returnValue2, returnValue3, 0, context());

        m_callback = 0;
    }
    
    void invalidate()
    {
        ASSERT(m_callback);

        RefPtr<WebError> error = WebError::create();
        m_callback(0, 0, 0, toAPI(error.get()), context());
        
        m_callback = 0;
    }

private:

    ValidateCommandCallback(void* context, CallbackFunction callback)
        : CallbackBase(context)
        , m_callback(callback)
    {
    }

    CallbackFunction m_callback;
};

class WebPageProxy
    : public TypedAPIObject<APIObject::TypePage>
#if ENABLE(INPUT_TYPE_COLOR)
    , public WebColorPicker::Client
#endif
    , public WebPopupMenuProxy::Client
    , public CoreIPC::MessageReceiver {
public:

    static PassRefPtr<WebPageProxy> create(PageClient*, PassRefPtr<WebProcessProxy>, WebPageGroup*, uint64_t pageID);
    virtual ~WebPageProxy();

    uint64_t pageID() const { return m_pageID; }

    WebFrameProxy* mainFrame() const { return m_mainFrame.get(); }
    WebFrameProxy* focusedFrame() const { return m_focusedFrame.get(); }
    WebFrameProxy* frameSetLargestFrame() const { return m_frameSetLargestFrame.get(); }

    DrawingAreaProxy* drawingArea() const { return m_drawingArea.get(); }

    WebBackForwardList* backForwardList() const { return m_backForwardList.get(); }

#if ENABLE(INSPECTOR)
    WebInspectorProxy* inspector();
#endif

#if ENABLE(VIBRATION)
    WebVibrationProxy* vibration() { return m_vibration.get(); }
#endif

#if ENABLE(FULLSCREEN_API)
    WebFullScreenManagerProxy* fullScreenManager();
#endif

#if ENABLE(CONTEXT_MENUS)
    void initializeContextMenuClient(const WKPageContextMenuClient*);
#endif
    void initializeFindClient(const WKPageFindClient*);
    void initializeFindMatchesClient(const WKPageFindMatchesClient*);
    void initializeFormClient(const WKPageFormClient*);
    void initializeLoaderClient(const WKPageLoaderClient*);
    void initializePolicyClient(const WKPagePolicyClient*);
    void initializeUIClient(const WKPageUIClient*);
#if PLATFORM(EFL)
    void initializeUIPopupMenuClient(const WKPageUIPopupMenuClient*);
#endif

    void initializeWebPage();

    void close();
    bool tryClose();
    bool isClosed() const { return m_isClosed; }

    void loadURL(const String&, APIObject* userData = 0);
    void loadURLRequest(WebURLRequest*, APIObject* userData = 0);
    void loadFile(const String& fileURL, const String& resourceDirectoryURL, APIObject* userData = 0);
    void loadData(WebData*, const String& MIMEType, const String& encoding, const String& baseURL, APIObject* userData = 0);
    void loadHTMLString(const String& htmlString, const String& baseURL, APIObject* userData = 0);
    void loadAlternateHTMLString(const String& htmlString, const String& baseURL, const String& unreachableURL, APIObject* userData = 0);
    void loadPlainTextString(const String& string, APIObject* userData = 0);
    void loadWebArchiveData(const WebData*, APIObject* userData = 0);

    void stopLoading();
    void reload(bool reloadFromOrigin);

    void goForward();
    bool canGoForward() const;
    void goBack();
    bool canGoBack() const;

    void goToBackForwardItem(WebBackForwardListItem*);
    void tryRestoreScrollPosition();
    void didChangeBackForwardList(WebBackForwardListItem* addedItem, Vector<RefPtr<APIObject> >* removedItems);
    void shouldGoToBackForwardListItem(uint64_t itemID, bool& shouldGoToBackForwardListItem);
    void willGoToBackForwardListItem(uint64_t itemID, CoreIPC::MessageDecoder&);

    String activeURL() const;
    String provisionalURL() const;
    String committedURL() const;

    bool willHandleHorizontalScrollEvents() const;

    bool canShowMIMEType(const String& mimeType) const;

    bool drawsBackground() const { return m_drawsBackground; }
    void setDrawsBackground(bool);

    bool drawsTransparentBackground() const { return m_drawsTransparentBackground; }
    void setDrawsTransparentBackground(bool);

    WebCore::Color underlayColor() const { return m_underlayColor; }
    void setUnderlayColor(const WebCore::Color&);

    void viewWillStartLiveResize();
    void viewWillEndLiveResize();

    void setInitialFocus(bool forward, bool isKeyboardEventValid, const WebKeyboardEvent&);
    void setWindowResizerSize(const WebCore::IntSize&);
    
    void clearSelection();

    void setViewNeedsDisplay(const WebCore::IntRect&);
    void displayView();
    bool canScrollView();
    void scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset);

    enum {
        ViewWindowIsActive = 1 << 0,
        ViewIsFocused = 1 << 1,
        ViewIsVisible = 1 << 2,
        ViewIsInWindow = 1 << 3,
    };
    typedef unsigned ViewStateFlags;
    void viewStateDidChange(ViewStateFlags flags);
    bool isInWindow() const { return m_isInWindow; }
    void waitForDidUpdateInWindowState();

    WebCore::IntSize viewSize() const;
    bool isViewVisible() const { return m_isVisible; }
    bool isViewWindowActive() const;

    void executeEditCommand(const String& commandName);
    void validateCommand(const String& commandName, PassRefPtr<ValidateCommandCallback>);

    const EditorState& editorState() const { return m_editorState; }
    bool canDelete() const { return hasSelectedRange() && isContentEditable(); }
    bool hasSelectedRange() const { return m_editorState.selectionIsRange; }
    bool isContentEditable() const { return m_editorState.isContentEditable; }
    
    bool maintainsInactiveSelection() const { return m_maintainsInactiveSelection; }
    void setMaintainsInactiveSelection(bool);
#if USE(TILED_BACKING_STORE) 
    void didRenderFrame(const WebCore::IntSize& contentsSize, const WebCore::IntRect& coveredRect);
#endif
#if PLATFORM(QT)
    void registerApplicationScheme(const String& scheme);
    void resolveApplicationSchemeRequest(QtNetworkRequestData);
    void sendApplicationSchemeReply(const QQuickNetworkReply*);
    void authenticationRequiredRequest(const String& hostname, const String& realm, const String& prefilledUsername, String& username, String& password);
    void certificateVerificationRequest(const String& hostname, bool& ignoreErrors);
    void proxyAuthenticationRequiredRequest(const String& hostname, uint16_t port, const String& prefilledUsername, String& username, String& password);
#endif // PLATFORM(QT).
#if PLATFORM(EFL)
    void setThemePath(const String&);
#endif

#if PLATFORM(QT) || PLATFORM(GTK)
    void setComposition(const String& text, Vector<WebCore::CompositionUnderline> underlines, uint64_t selectionStart, uint64_t selectionEnd, uint64_t replacementRangeStart, uint64_t replacementRangeEnd);
    void confirmComposition(const String& compositionString, int64_t selectionStart, int64_t selectionLength);
    void cancelComposition();
#endif

#if PLATFORM(GTK)
    void setInputMethodState(bool enabled);
#endif

#if PLATFORM(MAC)
    void updateWindowIsVisible(bool windowIsVisible);
    void windowAndViewFramesChanged(const WebCore::FloatRect& viewFrameInWindowCoordinates, const WebCore::FloatPoint& accessibilityViewCoordinates);
    void viewExposedRectChanged(const WebCore::FloatRect& exposedRect, bool);
    void exposedRectChangedTimerFired(WebCore::Timer<WebPageProxy>*);
    void setMainFrameIsScrollable(bool);

    void setComposition(const String& text, Vector<WebCore::CompositionUnderline> underlines, uint64_t selectionStart, uint64_t selectionEnd, uint64_t replacementRangeStart, uint64_t replacementRangeEnd);
    void confirmComposition();
    void cancelComposition();
    bool insertText(const String& text, uint64_t replacementRangeStart, uint64_t replacementRangeEnd);
    bool insertDictatedText(const String& text, uint64_t replacementRangeStart, uint64_t replacementRangeEnd, const Vector<WebCore::TextAlternativeWithRange>& dictationAlternatives);
    void getMarkedRange(uint64_t& location, uint64_t& length);
    void getSelectedRange(uint64_t& location, uint64_t& length);
    void getAttributedSubstringFromRange(uint64_t location, uint64_t length, AttributedString&);
    uint64_t characterIndexForPoint(const WebCore::IntPoint);
    WebCore::IntRect firstRectForCharacterRange(uint64_t, uint64_t);
    bool executeKeypressCommands(const Vector<WebCore::KeypressCommand>&);

    void sendComplexTextInputToPlugin(uint64_t pluginComplexTextInputIdentifier, const String& textInput);
    CGContextRef containingWindowGraphicsContext();
    bool shouldDelayWindowOrderingForEvent(const WebMouseEvent&);
    bool acceptsFirstMouse(int eventNumber, const WebMouseEvent&);

    void setAcceleratedCompositingRootLayer(const WebCore::GraphicsLayer*);

#if USE(APPKIT)
    WKView* wkView() const;
    void intrinsicContentSizeDidChange(const WebCore::IntSize& intrinsicContentSize);
#endif
#endif
#if PLATFORM(EFL)
    void handleInputMethodKeydown(bool& handled);
    void confirmComposition(const String&);
    void setComposition(const String&, Vector<WebCore::CompositionUnderline>&, int);
    void cancelComposition();
#endif
#if PLATFORM(GTK)
    PlatformWidget viewWidget();
#endif
#if USE(TILED_BACKING_STORE)
    void commitPageTransitionViewport();
#endif

    void handleMouseEvent(const NativeWebMouseEvent&);
    void handleWheelEvent(const NativeWebWheelEvent&);
    void handleKeyboardEvent(const NativeWebKeyboardEvent&);
#if ENABLE(GESTURE_EVENTS)
    void handleGestureEvent(const WebGestureEvent&);
#endif
#if ENABLE(TOUCH_EVENTS)
    void handleTouchEvent(const NativeWebTouchEvent&);
#if PLATFORM(QT)
    void handlePotentialActivation(const WebCore::IntPoint& touchPoint, const WebCore::IntSize& touchArea);
#endif
#endif

    void scrollBy(WebCore::ScrollDirection, WebCore::ScrollGranularity);
    void centerSelectionInVisibleArea();

    String pageTitle() const;
    const String& toolTip() const { return m_toolTip; }

    void setUserAgent(const String&);
    const String& userAgent() const { return m_userAgent; }
    void setApplicationNameForUserAgent(const String&);
    const String& applicationNameForUserAgent() const { return m_applicationNameForUserAgent; }
    void setCustomUserAgent(const String&);
    const String& customUserAgent() const { return m_customUserAgent; }
    static String standardUserAgent(const String& applicationName = String());

    bool supportsTextEncoding() const;
    void setCustomTextEncodingName(const String&);
    String customTextEncodingName() const { return m_customTextEncodingName; }

    bool areActiveDOMObjectsAndAnimationsSuspended() const { return m_isPageSuspended; }
    void resumeActiveDOMObjectsAndAnimations();
    void suspendActiveDOMObjectsAndAnimations();

    double estimatedProgress() const;

    void terminateProcess();

    typedef bool (*WebPageProxySessionStateFilterCallback)(WKPageRef, WKStringRef type, WKTypeRef object, void* context);
    PassRefPtr<WebData> sessionStateData(WebPageProxySessionStateFilterCallback, void* context) const;
    void restoreFromSessionStateData(WebData*);

    bool supportsTextZoom() const;
    double textZoomFactor() const { return m_textZoomFactor; }
    void setTextZoomFactor(double);
    double pageZoomFactor() const { return m_pageZoomFactor; }
    void setPageZoomFactor(double);
    void setPageAndTextZoomFactors(double pageZoomFactor, double textZoomFactor);

    void scalePage(double scale, const WebCore::IntPoint& origin);
    double pageScaleFactor() const { return m_pageScaleFactor; }

    float deviceScaleFactor() const;
    void setIntrinsicDeviceScaleFactor(float);
    void setCustomDeviceScaleFactor(float);
    void windowScreenDidChange(PlatformDisplayID);

    LayerHostingMode layerHostingMode() const { return m_layerHostingMode; }

    void setUseFixedLayout(bool);
    void setFixedLayoutSize(const WebCore::IntSize&);
    bool useFixedLayout() const { return m_useFixedLayout; };
    const WebCore::IntSize& fixedLayoutSize() const { return m_fixedLayoutSize; };

    void listenForLayoutMilestones(WebCore::LayoutMilestones);

    void setVisibilityState(WebCore::PageVisibilityState, bool isInitialState);
    void didUpdateInWindowState() { m_waitingForDidUpdateInWindowState = false; }

    bool hasHorizontalScrollbar() const { return m_mainFrameHasHorizontalScrollbar; }
    bool hasVerticalScrollbar() const { return m_mainFrameHasVerticalScrollbar; }

    void setSuppressScrollbarAnimations(bool);
    bool areScrollbarAnimationsSuppressed() const { return m_suppressScrollbarAnimations; }

    bool isPinnedToLeftSide() const { return m_mainFrameIsPinnedToLeftSide; }
    bool isPinnedToRightSide() const { return m_mainFrameIsPinnedToRightSide; }
    bool isPinnedToTopSide() const { return m_mainFrameIsPinnedToTopSide; }
    bool isPinnedToBottomSide() const { return m_mainFrameIsPinnedToBottomSide; }

    bool rubberBandsAtBottom() const { return m_rubberBandsAtBottom; }
    void setRubberBandsAtBottom(bool);
    bool rubberBandsAtTop() const { return m_rubberBandsAtTop; }
    void setRubberBandsAtTop(bool);

    void setPaginationMode(WebCore::Pagination::Mode);
    WebCore::Pagination::Mode paginationMode() const { return m_paginationMode; }
    void setPaginationBehavesLikeColumns(bool);
    bool paginationBehavesLikeColumns() const { return m_paginationBehavesLikeColumns; }
    void setPageLength(double);
    double pageLength() const { return m_pageLength; }
    void setGapBetweenPages(double);
    double gapBetweenPages() const { return m_gapBetweenPages; }
    unsigned pageCount() const { return m_pageCount; }

#if PLATFORM(MAC)
    // Called by the web process through a message.
    void registerWebProcessAccessibilityToken(const CoreIPC::DataReference&);
    // Called by the UI process when it is ready to send its tokens to the web process.
    void registerUIProcessAccessibilityTokens(const CoreIPC::DataReference& elemenToken, const CoreIPC::DataReference& windowToken);
    bool readSelectionFromPasteboard(const String& pasteboardName);
    String stringSelectionForPasteboard();
    PassRefPtr<WebCore::SharedBuffer> dataSelectionForPasteboard(const String& pasteboardType);
    void makeFirstResponder();

    ColorSpaceData colorSpace();
#endif

    void pageScaleFactorDidChange(double);
    void pageZoomFactorDidChange(double);

    void setMemoryCacheClientCallsEnabled(bool);

    // Find.
    void findString(const String&, FindOptions, unsigned maxMatchCount);
    void findStringMatches(const String&, FindOptions, unsigned maxMatchCount);
    void getImageForFindMatch(int32_t matchIndex);
    void selectFindMatch(int32_t matchIndex);
    void didGetImageForFindMatch(const ShareableBitmap::Handle& contentImageHandle, uint32_t matchIndex);
    void hideFindUI();
    void countStringMatches(const String&, FindOptions, unsigned maxMatchCount);
    void didCountStringMatches(const String&, uint32_t matchCount);
    void setFindIndicator(const WebCore::FloatRect& selectionRectInWindowCoordinates, const Vector<WebCore::FloatRect>& textRectsInSelectionRectCoordinates, float contentImageScaleFactor, const ShareableBitmap::Handle& contentImageHandle, bool fadeOut, bool animate);
    void didFindString(const String&, uint32_t matchCount);
    void didFailToFindString(const String&);
    void didFindStringMatches(const String&, Vector<Vector<WebCore::IntRect> > matchRects, int32_t firstIndexAfterSelection);

    void getContentsAsString(PassRefPtr<StringCallback>);
#if ENABLE(MHTML)
    void getContentsAsMHTMLData(PassRefPtr<DataCallback>, bool useBinaryEncoding);
#endif
    void getMainResourceDataOfFrame(WebFrameProxy*, PassRefPtr<DataCallback>);
    void getResourceDataFromFrame(WebFrameProxy*, WebURL*, PassRefPtr<DataCallback>);
    void getRenderTreeExternalRepresentation(PassRefPtr<StringCallback>);
    void getSelectionOrContentsAsString(PassRefPtr<StringCallback>);
    void getSelectionAsWebArchiveData(PassRefPtr<DataCallback>);
    void getSourceForFrame(WebFrameProxy*, PassRefPtr<StringCallback>);
    void getWebArchiveOfFrame(WebFrameProxy*, PassRefPtr<DataCallback>);
    void runJavaScriptInMainFrame(const String&, PassRefPtr<ScriptValueCallback>);
    void forceRepaint(PassRefPtr<VoidCallback>);

    float headerHeight(WebFrameProxy*);
    float footerHeight(WebFrameProxy*);
    void drawHeader(WebFrameProxy*, const WebCore::FloatRect&);
    void drawFooter(WebFrameProxy*, const WebCore::FloatRect&);

#if PLATFORM(MAC)
    // Dictionary.
    void performDictionaryLookupAtLocation(const WebCore::FloatPoint&);
#endif

    void receivedPolicyDecision(WebCore::PolicyAction, WebFrameProxy*, uint64_t listenerID);

    void backForwardRemovedItem(uint64_t itemID);

#if ENABLE(DRAG_SUPPORT)    
    // Drag and drop support.
    void dragEntered(WebCore::DragData*, const String& dragStorageName = String());
    void dragUpdated(WebCore::DragData*, const String& dragStorageName = String());
    void dragExited(WebCore::DragData*, const String& dragStorageName = String());
    void performDrag(WebCore::DragData*, const String& dragStorageName, const SandboxExtension::Handle&, const SandboxExtension::HandleArray&);

    void didPerformDragControllerAction(WebCore::DragSession);
    void dragEnded(const WebCore::IntPoint& clientPosition, const WebCore::IntPoint& globalPosition, uint64_t operation);
#if PLATFORM(MAC)
    void setDragImage(const WebCore::IntPoint& clientPosition, const ShareableBitmap::Handle& dragImageHandle, bool isLinkDrag);
    void setPromisedData(const String& pasteboardName, const SharedMemory::Handle& imageHandle, uint64_t imageSize, const String& filename, const String& extension,
                         const String& title, const String& url, const String& visibleURL, const SharedMemory::Handle& archiveHandle, uint64_t archiveSize);
#endif
#if PLATFORM(QT) || PLATFORM(GTK)
    void startDrag(const WebCore::DragData&, const ShareableBitmap::Handle& dragImage);
#endif
#endif

    void processDidBecomeUnresponsive();
    void interactionOccurredWhileProcessUnresponsive();
    void processDidBecomeResponsive();
    void processDidCrash();

#if USE(ACCELERATED_COMPOSITING)
    virtual void enterAcceleratedCompositingMode(const LayerTreeContext&);
    virtual void exitAcceleratedCompositingMode();
    virtual void updateAcceleratedCompositingMode(const LayerTreeContext&);
#endif
    
    void didDraw();

    enum UndoOrRedo { Undo, Redo };
    void addEditCommand(WebEditCommandProxy*);
    void removeEditCommand(WebEditCommandProxy*);
    bool isValidEditCommand(WebEditCommandProxy*);
    void registerEditCommand(PassRefPtr<WebEditCommandProxy>, UndoOrRedo);

#if PLATFORM(MAC)
    void registerKeypressCommandName(const String& name) { m_knownKeypressCommandNames.add(name); }
    bool isValidKeypressCommandName(const String& name) const { return m_knownKeypressCommandNames.contains(name); }
#endif

    WebProcessProxy* process() const;
    PlatformProcessIdentifier processIdentifier() const;

    WebPageGroup* pageGroup() const { return m_pageGroup.get(); }

    bool isValid() const;

    PassRefPtr<ImmutableArray> relatedPages() const;

    const String& urlAtProcessExit() const { return m_urlAtProcessExit; }
    WebFrameProxy::LoadState loadStateAtProcessExit() const { return m_loadStateAtProcessExit; }

#if ENABLE(DRAG_SUPPORT)
    WebCore::DragSession dragSession() const { return m_currentDragSession; }
    void resetDragOperation() { m_currentDragSession = WebCore::DragSession(); }
#endif

    void preferencesDidChange();

#if ENABLE(CONTEXT_MENUS)
    // Called by the WebContextMenuProxy.
    void contextMenuItemSelected(const WebContextMenuItemData&);
#endif

    // Called by the WebOpenPanelResultListenerProxy.
    void didChooseFilesForOpenPanel(const Vector<String>&);
    void didCancelForOpenPanel();

    WebPageCreationParameters creationParameters() const;

#if USE(COORDINATED_GRAPHICS)
    void findZoomableAreaForPoint(const WebCore::IntPoint&, const WebCore::IntSize&);
#endif

#if PLATFORM(QT) || PLATFORM(EFL) || PLATFORM(GTK)
    void handleDownloadRequest(DownloadProxy*);
#endif

    void advanceToNextMisspelling(bool startBeforeSelection) const;
    void changeSpellingToWord(const String& word) const;
#if USE(APPKIT)
    void uppercaseWord();
    void lowercaseWord();
    void capitalizeWord();
#endif

#if PLATFORM(MAC)
    bool isSmartInsertDeleteEnabled() const { return m_isSmartInsertDeleteEnabled; }
    void setSmartInsertDeleteEnabled(bool);
#endif

#if PLATFORM(GTK)
    String accessibilityPlugID() const { return m_accessibilityPlugID; }
#endif

    void setCanRunModal(bool);
    bool canRunModal();

    void beginPrinting(WebFrameProxy*, const PrintInfo&);
    void endPrinting();
    void computePagesForPrinting(WebFrameProxy*, const PrintInfo&, PassRefPtr<ComputedPagesCallback>);
#if PLATFORM(MAC)
    void drawRectToImage(WebFrameProxy*, const PrintInfo&, const WebCore::IntRect&, const WebCore::IntSize&, PassRefPtr<ImageCallback>);
    void drawPagesToPDF(WebFrameProxy*, const PrintInfo&, uint32_t first, uint32_t count, PassRefPtr<DataCallback>);
#elif PLATFORM(GTK)
    void drawPagesForPrinting(WebFrameProxy*, const PrintInfo&, PassRefPtr<PrintFinishedCallback>);
#endif

    const String& pendingAPIRequestURL() const { return m_pendingAPIRequestURL; }

    void flashBackingStoreUpdates(const Vector<WebCore::IntRect>& updateRects);

#if PLATFORM(MAC)
    void handleAlternativeTextUIResult(const String& result);
#endif

    static void setDebugPaintFlags(WKPageDebugPaintFlags flags) { s_debugPaintFlags = flags; }
    static WKPageDebugPaintFlags debugPaintFlags() { return s_debugPaintFlags; }

    // Color to be used with kWKDebugFlashViewUpdates.
    static WebCore::Color viewUpdatesFlashColor();

    // Color to be used with kWKDebugFlashBackingStoreUpdates.
    static WebCore::Color backingStoreUpdatesFlashColor();

    void saveDataToFileInDownloadsFolder(const String& suggestedFilename, const String& mimeType, const String& originatingURLString, WebData*);
    void savePDFToFileInDownloadsFolder(const String& suggestedFilename, const String& originatingURLString, const CoreIPC::DataReference&);
#if PLATFORM(MAC)
    void savePDFToTemporaryFolderAndOpenWithNativeApplicationRaw(const String& suggestedFilename, const String& originatingURLString, const uint8_t* data, unsigned long size, const String& pdfUUID);
    void savePDFToTemporaryFolderAndOpenWithNativeApplication(const String& suggestedFilename, const String& originatingURLString, const CoreIPC::DataReference&, const String& pdfUUID);
    void openPDFFromTemporaryFolderWithNativeApplication(const String& pdfUUID);
#endif

    void linkClicked(const String&, const WebMouseEvent&);

    WebCore::IntRect visibleScrollerThumbRect() const { return m_visibleScrollerThumbRect; }

    uint64_t renderTreeSize() const { return m_renderTreeSize; }

    void setShouldSendEventsSynchronously(bool sync) { m_shouldSendEventsSynchronously = sync; };

    void printMainFrame();
    
    void setMediaVolume(float);
    void setMayStartMediaWhenInWindow(bool);

    // WebPopupMenuProxy::Client
    virtual NativeWebMouseEvent* currentlyProcessedMouseDownEvent();

#if PLATFORM(GTK) && USE(TEXTURE_MAPPER_GL)
    void setAcceleratedCompositingWindowId(uint64_t nativeWindowId);
#endif

    void setSuppressVisibilityUpdates(bool flag) { m_suppressVisibilityUpdates = flag; }
    bool suppressVisibilityUpdates() { return m_suppressVisibilityUpdates; }

    void postMessageToInjectedBundle(const String& messageName, APIObject* messageBody);

#if ENABLE(INPUT_TYPE_COLOR)
    void setColorChooserColor(const WebCore::Color&);
    void endColorChooser();
#endif

    const WebLoaderClient& loaderClient() { return m_loaderClient; }

    WebCore::IntSize minimumLayoutSize() const { return m_minimumLayoutSize; }
    void setMinimumLayoutSize(const WebCore::IntSize&);

    bool mainFrameInViewSourceMode() const { return m_mainFrameInViewSourceMode; }
    void setMainFrameInViewSourceMode(bool);

    void didReceiveAuthenticationChallengeProxy(uint64_t frameID, PassRefPtr<AuthenticationChallengeProxy>);

    int64_t spellDocumentTag();
    void didFinishCheckingText(uint64_t requestID, const Vector<WebCore::TextCheckingResult>&) const;
    void didCancelCheckingText(uint64_t requestID) const;

    void connectionWillOpen(CoreIPC::Connection*);
    void connectionWillClose(CoreIPC::Connection*);

    void didSaveToPageCache();
        
    void setScrollPinningBehavior(WebCore::ScrollPinningBehavior);
    WebCore::ScrollPinningBehavior scrollPinningBehavior() { return m_scrollPinningBehavior; }
        
private:
    WebPageProxy(PageClient*, PassRefPtr<WebProcessProxy>, WebPageGroup*, uint64_t pageID);

    void resetStateAfterProcessExited();

    // CoreIPC::MessageReceiver
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;
    virtual void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&) OVERRIDE;

    // WebPopupMenuProxy::Client
    virtual void valueChangedForPopupMenu(WebPopupMenuProxy*, int32_t newSelectedIndex);
    virtual void setTextFromItemForPopupMenu(WebPopupMenuProxy*, int32_t index);
#if PLATFORM(GTK)
    virtual void failedToShowPopupMenu();
#endif
#if PLATFORM(QT)
    virtual void changeSelectedIndex(int32_t newSelectedIndex);
    virtual void closePopupMenu();
#endif

    // Implemented in generated WebPageProxyMessageReceiver.cpp
    void didReceiveWebPageProxyMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    void didReceiveSyncWebPageProxyMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&);

    void didCreateMainFrame(uint64_t frameID);
    void didCreateSubframe(uint64_t frameID);

    void didStartProvisionalLoadForFrame(uint64_t frameID, const String& url, const String& unreachableURL, CoreIPC::MessageDecoder&);
    void didReceiveServerRedirectForProvisionalLoadForFrame(uint64_t frameID, const String&, CoreIPC::MessageDecoder&);
    void didFailProvisionalLoadForFrame(uint64_t frameID, const WebCore::ResourceError&, CoreIPC::MessageDecoder&);
    void didCommitLoadForFrame(uint64_t frameID, const String& mimeType, uint32_t frameLoadType, const PlatformCertificateInfo&, CoreIPC::MessageDecoder&);
    void didFinishDocumentLoadForFrame(uint64_t frameID, CoreIPC::MessageDecoder&);
    void didFinishLoadForFrame(uint64_t frameID, CoreIPC::MessageDecoder&);
    void didFailLoadForFrame(uint64_t frameID, const WebCore::ResourceError&, CoreIPC::MessageDecoder&);
    void didSameDocumentNavigationForFrame(uint64_t frameID, uint32_t sameDocumentNavigationType, const String&, CoreIPC::MessageDecoder&);
    void didReceiveTitleForFrame(uint64_t frameID, const String&, CoreIPC::MessageDecoder&);
    void didFirstLayoutForFrame(uint64_t frameID, CoreIPC::MessageDecoder&);
    void didFirstVisuallyNonEmptyLayoutForFrame(uint64_t frameID, CoreIPC::MessageDecoder&);
    void didNewFirstVisuallyNonEmptyLayout(CoreIPC::MessageDecoder&);
    void didLayout(uint32_t layoutMilestones, CoreIPC::MessageDecoder&);
    void didRemoveFrameFromHierarchy(uint64_t frameID, CoreIPC::MessageDecoder&);
    void didDisplayInsecureContentForFrame(uint64_t frameID, CoreIPC::MessageDecoder&);
    void didRunInsecureContentForFrame(uint64_t frameID, CoreIPC::MessageDecoder&);
    void didDetectXSSForFrame(uint64_t frameID, CoreIPC::MessageDecoder&);
    void frameDidBecomeFrameSet(uint64_t frameID, bool);
    void didStartProgress();
    void didChangeProgress(double);
    void didFinishProgress();

    void decidePolicyForNavigationAction(uint64_t frameID, uint32_t navigationType, uint32_t modifiers, int32_t mouseButton, const WebCore::ResourceRequest&, uint64_t listenerID, CoreIPC::MessageDecoder&, bool& receivedPolicyAction, uint64_t& policyAction, uint64_t& downloadID);
    void decidePolicyForNewWindowAction(uint64_t frameID, uint32_t navigationType, uint32_t modifiers, int32_t mouseButton, const WebCore::ResourceRequest&, const String& frameName, uint64_t listenerID, CoreIPC::MessageDecoder&);
    void decidePolicyForResponse(uint64_t frameID, const WebCore::ResourceResponse&, const WebCore::ResourceRequest&, uint64_t listenerID, CoreIPC::MessageDecoder&);
    void decidePolicyForResponseSync(uint64_t frameID, const WebCore::ResourceResponse&, const WebCore::ResourceRequest&, uint64_t listenerID, CoreIPC::MessageDecoder&, bool& receivedPolicyAction, uint64_t& policyAction, uint64_t& downloadID);
    void unableToImplementPolicy(uint64_t frameID, const WebCore::ResourceError&, CoreIPC::MessageDecoder&);

    void willSubmitForm(uint64_t frameID, uint64_t sourceFrameID, const Vector<std::pair<String, String> >& textFieldValues, uint64_t listenerID, CoreIPC::MessageDecoder&);

    // UI client
    void createNewPage(const WebCore::ResourceRequest&, const WebCore::WindowFeatures&, uint32_t modifiers, int32_t mouseButton, uint64_t& newPageID, WebPageCreationParameters&);
    void showPage();
    void closePage(bool stopResponsivenessTimer);
    void runJavaScriptAlert(uint64_t frameID, const String&);
    void runJavaScriptConfirm(uint64_t frameID, const String&, bool& result);
    void runJavaScriptPrompt(uint64_t frameID, const String&, const String&, String& result);
    void shouldInterruptJavaScript(bool& result);
    void setStatusText(const String&);
    void mouseDidMoveOverElement(const WebHitTestResult::Data& hitTestResultData, uint32_t modifiers, CoreIPC::MessageDecoder&);
    void unavailablePluginButtonClicked(uint32_t opaquePluginUnavailabilityReason, const String& mimeType, const String& pluginURLString, const String& pluginsPageURLString, const String& frameURLString, const String& pageURLString);
    void setToolbarsAreVisible(bool toolbarsAreVisible);
    void getToolbarsAreVisible(bool& toolbarsAreVisible);
    void setMenuBarIsVisible(bool menuBarIsVisible);
    void getMenuBarIsVisible(bool& menuBarIsVisible);
    void setStatusBarIsVisible(bool statusBarIsVisible);
    void getStatusBarIsVisible(bool& statusBarIsVisible);
    void setIsResizable(bool isResizable);
    void getIsResizable(bool& isResizable);
    void setWindowFrame(const WebCore::FloatRect&);
    void getWindowFrame(WebCore::FloatRect&);
    void screenToWindow(const WebCore::IntPoint& screenPoint, WebCore::IntPoint& windowPoint);
    void windowToScreen(const WebCore::IntRect& viewRect, WebCore::IntRect& result);
    void runBeforeUnloadConfirmPanel(const String& message, uint64_t frameID, bool& shouldClose);
    void didChangeViewportProperties(const WebCore::ViewportAttributes&);
    void pageDidScroll();
    void runOpenPanel(uint64_t frameID, const WebCore::FileChooserSettings&);
    void printFrame(uint64_t frameID);
    void exceededDatabaseQuota(uint64_t frameID, const String& originIdentifier, const String& databaseName, const String& displayName, uint64_t currentQuota, uint64_t currentOriginUsage, uint64_t currentDatabaseUsage, uint64_t expectedUsage, PassRefPtr<Messages::WebPageProxy::ExceededDatabaseQuota::DelayedReply>);
    void requestGeolocationPermissionForFrame(uint64_t geolocationID, uint64_t frameID, String originIdentifier);
    void runModal();
    void notifyScrollerThumbIsVisibleInRect(const WebCore::IntRect&);
    void recommendedScrollbarStyleDidChange(int32_t newStyle);
    void didChangeScrollbarsForMainFrame(bool hasHorizontalScrollbar, bool hasVerticalScrollbar);
    void didChangeScrollOffsetPinningForMainFrame(bool pinnedToLeftSide, bool pinnedToRightSide, bool pinnedToTopSide, bool pinnedToBottomSide);
    void didChangePageCount(unsigned);
    void didFailToInitializePlugin(const String& mimeType, const String& frameURLString, const String& pageURLString);
    void didBlockInsecurePluginVersion(const String& mimeType, const String& pluginURLString, const String& frameURLString, const String& pageURLString, bool replacementObscured);
    void setCanShortCircuitHorizontalWheelEvents(bool canShortCircuitHorizontalWheelEvents) { m_canShortCircuitHorizontalWheelEvents = canShortCircuitHorizontalWheelEvents; }

    void reattachToWebProcess();
    void reattachToWebProcessWithItem(WebBackForwardListItem*);

    void requestNotificationPermission(uint64_t notificationID, const String& originString);
    void showNotification(const String& title, const String& body, const String& iconURL, const String& tag, const String& lang, const String& dir, const String& originString, uint64_t notificationID);
    void cancelNotification(uint64_t notificationID);
    void clearNotifications(const Vector<uint64_t>& notificationIDs);
    void didDestroyNotification(uint64_t notificationID);

#if USE(TILED_BACKING_STORE)
    void pageDidRequestScroll(const WebCore::IntPoint&);
    void pageTransitionViewportReady();
#endif
#if USE(COORDINATED_GRAPHICS)
    void didFindZoomableArea(const WebCore::IntPoint&, const WebCore::IntRect&);
#endif
#if PLATFORM(QT) || PLATFORM(EFL)
    void didChangeContentsSize(const WebCore::IntSize&);
#endif

#if ENABLE(TOUCH_EVENTS)
    void needTouchEvents(bool);
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    void showColorChooser(const WebCore::Color& initialColor, const WebCore::IntRect&);
    void didChooseColor(const WebCore::Color&);
    void didEndColorChooser();
#endif

    void editorStateChanged(const EditorState&);
#if PLATFORM(QT)
    void willSetInputMethodState();
#endif

    // Back/Forward list management
    void backForwardAddItem(uint64_t itemID);
    void backForwardGoToItem(uint64_t itemID, SandboxExtension::Handle&);
    void backForwardItemAtIndex(int32_t index, uint64_t& itemID);
    void backForwardBackListCount(int32_t& count);
    void backForwardForwardListCount(int32_t& count);
    void backForwardClear();

    // Undo management
    void registerEditCommandForUndo(uint64_t commandID, uint32_t editAction);
    void clearAllEditCommands();
    void canUndoRedo(uint32_t action, bool& result);
    void executeUndoRedo(uint32_t action, bool& result);

    // Keyboard handling
#if PLATFORM(MAC)
    void interpretQueuedKeyEvent(const EditorState&, bool& handled, Vector<WebCore::KeypressCommand>&);
    void executeSavedCommandBySelector(const String& selector, bool& handled);
#endif

#if PLATFORM(GTK)
    void getEditorCommandsForKeyEvent(const AtomicString&, Vector<String>&);
    void bindAccessibilityTree(const String&);
#endif
#if PLATFORM(EFL)
    void getEditorCommandsForKeyEvent(Vector<String>&);
#endif

    // Popup Menu.
    void showPopupMenu(const WebCore::IntRect& rect, uint64_t textDirection, const Vector<WebPopupItem>& items, int32_t selectedIndex, const PlatformPopupMenuData&);
    void hidePopupMenu();

#if ENABLE(CONTEXT_MENUS)
    // Context Menu.
    void showContextMenu(const WebCore::IntPoint& menuLocation, const WebHitTestResult::Data&, const Vector<WebContextMenuItemData>&, CoreIPC::MessageDecoder&);
    void internalShowContextMenu(const WebCore::IntPoint& menuLocation, const WebHitTestResult::Data&, const Vector<WebContextMenuItemData>&, CoreIPC::MessageDecoder&);
#endif

    // Search popup results
    void saveRecentSearches(const String&, const Vector<String>&);
    void loadRecentSearches(const String&, Vector<String>&);

#if PLATFORM(MAC)
    // Speech.
    void getIsSpeaking(bool&);
    void speak(const String&);
    void stopSpeaking();

    // Spotlight.
    void searchWithSpotlight(const String&);
        
    void searchTheWeb(const String&);

    // Dictionary.
    void didPerformDictionaryLookup(const AttributedString&, const DictionaryPopupInfo&);
#endif

    // Spelling and grammar.
#if USE(UNIFIED_TEXT_CHECKING)
    void checkTextOfParagraph(const String& text, uint64_t checkingTypes, Vector<WebCore::TextCheckingResult>& results);
#endif
    void checkSpellingOfString(const String& text, int32_t& misspellingLocation, int32_t& misspellingLength);
    void checkGrammarOfString(const String& text, Vector<WebCore::GrammarDetail>&, int32_t& badGrammarLocation, int32_t& badGrammarLength);
    void spellingUIIsShowing(bool&);
    void updateSpellingUIWithMisspelledWord(const String& misspelledWord);
    void updateSpellingUIWithGrammarString(const String& badGrammarPhrase, const WebCore::GrammarDetail&);
    void getGuessesForWord(const String& word, const String& context, Vector<String>& guesses);
    void learnWord(const String& word);
    void ignoreWord(const String& word);
    void requestCheckingOfString(uint64_t requestID, const WebCore::TextCheckingRequestData&);

    void setFocus(bool focused);
    void takeFocus(uint32_t direction);
    void setToolTip(const String&);
    void setCursor(const WebCore::Cursor&);
    void setCursorHiddenUntilMouseMoves(bool);

    void didReceiveEvent(uint32_t opaqueType, bool handled);
    void stopResponsivenessTimer();

    void voidCallback(uint64_t);
    void dataCallback(const CoreIPC::DataReference&, uint64_t);
    void imageCallback(const ShareableBitmap::Handle&, uint64_t);
    void stringCallback(const String&, uint64_t);
    void scriptValueCallback(const CoreIPC::DataReference&, uint64_t);
    void computedPagesCallback(const Vector<WebCore::IntRect>&, double totalScaleFactorForPrinting, uint64_t);
    void validateCommandCallback(const String&, bool, int, uint64_t);
#if PLATFORM(GTK)
    void printFinishedCallback(const WebCore::ResourceError&, uint64_t);
#endif

    void focusedFrameChanged(uint64_t frameID);
    void frameSetLargestFrameChanged(uint64_t frameID);

    void canAuthenticateAgainstProtectionSpaceInFrame(uint64_t frameID, const WebCore::ProtectionSpace&, bool& canAuthenticate);
    void didReceiveAuthenticationChallenge(uint64_t frameID, const WebCore::AuthenticationChallenge&, uint64_t challengeID);

#if PLATFORM(MAC)
    void pluginFocusOrWindowFocusChanged(uint64_t pluginComplexTextInputIdentifier, bool pluginHasFocusAndWindowHasFocus);
    void setPluginComplexTextInputState(uint64_t pluginComplexTextInputIdentifier, uint64_t complexTextInputState);
#endif

    void clearPendingAPIRequestURL() { m_pendingAPIRequestURL = String(); }
    void setPendingAPIRequestURL(const String& pendingAPIRequestURL) { m_pendingAPIRequestURL = pendingAPIRequestURL; }

    bool maybeInitializeSandboxExtensionHandle(const WebCore::KURL&, SandboxExtension::Handle&);

#if PLATFORM(MAC)
    void substitutionsPanelIsShowing(bool&);
    void showCorrectionPanel(int32_t panelType, const WebCore::FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings);
    void dismissCorrectionPanel(int32_t reason);
    void dismissCorrectionPanelSoon(int32_t reason, String& result);
    void recordAutocorrectionResponse(int32_t responseType, const String& replacedString, const String& replacementString);

#if USE(DICTATION_ALTERNATIVES)
    void showDictationAlternativeUI(const WebCore::FloatRect& boundingBoxOfDictatedText, uint64_t dictationContext);
    void removeDictationAlternatives(uint64_t dictationContext);
    void dictationAlternatives(uint64_t dictationContext, Vector<String>& result);
#endif
#endif // PLATFORM(MAC)

#if USE(SOUP)
    void didReceiveURIRequest(String uriString, uint64_t requestID);
#endif

    void clearLoadDependentCallbacks();

    void performDragControllerAction(DragControllerAction, WebCore::DragData*, const String& dragStorageName, const SandboxExtension::Handle&, const SandboxExtension::HandleArray&);

    void updateBackingStoreDiscardableState();

    void setRenderTreeSize(uint64_t treeSize) { m_renderTreeSize = treeSize; }

#if PLUGIN_ARCHITECTURE(X11)
    void createPluginContainer(uint64_t& windowID);
    void windowedPluginGeometryDidChange(const WebCore::IntRect& frameRect, const WebCore::IntRect& clipRect, uint64_t windowID);
#endif

    void processNextQueuedWheelEvent();
    void sendWheelEvent(const WebWheelEvent&);

#if ENABLE(NETSCAPE_PLUGIN_API)
    void findPlugin(const String& mimeType, uint32_t processType, const String& urlString, const String& frameURLString, const String& pageURLString, bool allowOnlyApplicationPlugins, uint64_t& pluginProcessToken, String& newMIMEType, uint32_t& pluginLoadPolicy, String& unavailabilityDescription);
#endif

    PageClient* m_pageClient;
    WebLoaderClient m_loaderClient;
    WebPolicyClient m_policyClient;
    WebFormClient m_formClient;
    WebUIClient m_uiClient;
#if PLATFORM(EFL)
    WebUIPopupMenuClient m_uiPopupMenuClient;
#endif
    WebFindClient m_findClient;
    WebFindMatchesClient m_findMatchesClient;
#if ENABLE(CONTEXT_MENUS)
    WebPageContextMenuClient m_contextMenuClient;
#endif

    OwnPtr<DrawingAreaProxy> m_drawingArea;
    RefPtr<WebProcessProxy> m_process;
    RefPtr<WebPageGroup> m_pageGroup;
    RefPtr<WebFrameProxy> m_mainFrame;
    RefPtr<WebFrameProxy> m_focusedFrame;
    RefPtr<WebFrameProxy> m_frameSetLargestFrame;

    String m_userAgent;
    String m_applicationNameForUserAgent;
    String m_customUserAgent;
    String m_customTextEncodingName;

#if ENABLE(INSPECTOR)
    RefPtr<WebInspectorProxy> m_inspector;
#endif

#if ENABLE(FULLSCREEN_API)
    RefPtr<WebFullScreenManagerProxy> m_fullScreenManager;
#endif

#if ENABLE(VIBRATION)
    RefPtr<WebVibrationProxy> m_vibration;
#endif

    HashMap<uint64_t, RefPtr<VoidCallback> > m_voidCallbacks;
    HashMap<uint64_t, RefPtr<DataCallback> > m_dataCallbacks;
    HashMap<uint64_t, RefPtr<ImageCallback> > m_imageCallbacks;
    HashMap<uint64_t, RefPtr<StringCallback> > m_stringCallbacks;
    HashSet<uint64_t> m_loadDependentStringCallbackIDs;
    HashMap<uint64_t, RefPtr<ScriptValueCallback> > m_scriptValueCallbacks;
    HashMap<uint64_t, RefPtr<ComputedPagesCallback> > m_computedPagesCallbacks;
    HashMap<uint64_t, RefPtr<ValidateCommandCallback> > m_validateCommandCallbacks;
#if PLATFORM(GTK)
    HashMap<uint64_t, RefPtr<PrintFinishedCallback> > m_printFinishedCallbacks;
#endif

    HashSet<WebEditCommandProxy*> m_editCommandSet;

#if PLATFORM(MAC)
    HashSet<String> m_knownKeypressCommandNames;
#endif

    RefPtr<WebPopupMenuProxy> m_activePopupMenu;
    RefPtr<WebContextMenuProxy> m_activeContextMenu;
    WebHitTestResult::Data m_activeContextMenuHitTestResultData;
    RefPtr<WebOpenPanelResultListenerProxy> m_openPanelResultListener;
    GeolocationPermissionRequestManagerProxy m_geolocationPermissionRequestManager;
    NotificationPermissionRequestManagerProxy m_notificationPermissionRequestManager;

    double m_estimatedProgress;

    // Whether the web page is contained in a top-level window.
    bool m_isInWindow;

    // Whether the page is visible; if the backing view is visible and inserted into a window.
    bool m_isVisible;

    bool m_canGoBack;
    bool m_canGoForward;
    RefPtr<WebBackForwardList> m_backForwardList;
    
    bool m_maintainsInactiveSelection;

    String m_toolTip;

    String m_urlAtProcessExit;
    WebFrameProxy::LoadState m_loadStateAtProcessExit;

    EditorState m_editorState;
    bool m_temporarilyClosedComposition; // Editor state changed from hasComposition to !hasComposition, but that was only with shouldIgnoreCompositionSelectionChange yet.

    double m_textZoomFactor;
    double m_pageZoomFactor;
    double m_pageScaleFactor;
    float m_intrinsicDeviceScaleFactor;
    float m_customDeviceScaleFactor;

    LayerHostingMode m_layerHostingMode;

    bool m_drawsBackground;
    bool m_drawsTransparentBackground;

    WebCore::Color m_underlayColor;

    bool m_areMemoryCacheClientCallsEnabled;

    bool m_useFixedLayout;
    WebCore::IntSize m_fixedLayoutSize;

    bool m_suppressScrollbarAnimations;

    WebCore::Pagination::Mode m_paginationMode;
    bool m_paginationBehavesLikeColumns;
    double m_pageLength;
    double m_gapBetweenPages;

    // If the process backing the web page is alive and kicking.
    bool m_isValid;

    // Whether WebPageProxy::close() has been called on this page.
    bool m_isClosed;

    // Whether it can run modal child web pages.
    bool m_canRunModal;

    bool m_isInPrintingMode;
    bool m_isPerformingDOMPrintOperation;

    bool m_inDecidePolicyForResponseSync;
    const WebCore::ResourceRequest* m_decidePolicyForResponseRequest;
    bool m_syncMimeTypePolicyActionIsValid;
    WebCore::PolicyAction m_syncMimeTypePolicyAction;
    uint64_t m_syncMimeTypePolicyDownloadID;

    bool m_inDecidePolicyForNavigationAction;
    bool m_syncNavigationActionPolicyActionIsValid;
    WebCore::PolicyAction m_syncNavigationActionPolicyAction;
    uint64_t m_syncNavigationActionPolicyDownloadID;

#if ENABLE(GESTURE_EVENTS)
    Deque<WebGestureEvent> m_gestureEventQueue;
#endif
    Deque<NativeWebKeyboardEvent> m_keyEventQueue;
    Deque<NativeWebWheelEvent> m_wheelEventQueue;
    Deque<OwnPtr<Vector<NativeWebWheelEvent> > > m_currentlyProcessedWheelEvents;

    bool m_processingMouseMoveEvent;
    OwnPtr<NativeWebMouseEvent> m_nextMouseMoveEvent;
    OwnPtr<NativeWebMouseEvent> m_currentlyProcessedMouseDownEvent;

#if ENABLE(TOUCH_EVENTS)
    bool m_needTouchEvents;
    Deque<QueuedTouchEvents> m_touchEventQueue;
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    RefPtr<WebColorPicker> m_colorPicker;
    RefPtr<WebColorPickerResultListenerProxy> m_colorPickerResultListener;
#endif

    uint64_t m_pageID;

    bool m_isPageSuspended;

#if PLATFORM(MAC)
    bool m_isSmartInsertDeleteEnabled;
#endif

#if PLATFORM(GTK)
    String m_accessibilityPlugID;
#endif

    int64_t m_spellDocumentTag;
    bool m_hasSpellDocumentTag;
    unsigned m_pendingLearnOrIgnoreWordMessageCount;

#if ENABLE(DRAG_SUPPORT)
    WebCore::DragSession m_currentDragSession;
#endif

    String m_pendingAPIRequestURL;

    bool m_mainFrameHasHorizontalScrollbar;
    bool m_mainFrameHasVerticalScrollbar;

    // Whether horizontal wheel events can be handled directly for swiping purposes.
    bool m_canShortCircuitHorizontalWheelEvents;

    bool m_mainFrameIsPinnedToLeftSide;
    bool m_mainFrameIsPinnedToRightSide;
    bool m_mainFrameIsPinnedToTopSide;
    bool m_mainFrameIsPinnedToBottomSide;

    bool m_rubberBandsAtBottom;
    bool m_rubberBandsAtTop;

    bool m_mainFrameInViewSourceMode;
        
    unsigned m_pageCount;

    WebCore::IntRect m_visibleScrollerThumbRect;

    uint64_t m_renderTreeSize;

    static WKPageDebugPaintFlags s_debugPaintFlags;

    bool m_shouldSendEventsSynchronously;

    bool m_suppressVisibilityUpdates;
    WebCore::IntSize m_minimumLayoutSize;

    float m_mediaVolume;
    bool m_mayStartMediaWhenInWindow;

    bool m_waitingForDidUpdateInWindowState;

#if PLATFORM(MAC)
    WebCore::Timer<WebPageProxy> m_exposedRectChangedTimer;
    WebCore::FloatRect m_exposedRect;
    WebCore::FloatRect m_lastSentExposedRect;
    bool m_clipsToExposedRect;
    bool m_lastSentClipsToExposedRect;
#endif

#if PLATFORM(QT)
    WTF::HashSet<RefPtr<QtRefCountedNetworkRequestData> > m_applicationSchemeRequests;
#endif

#if ENABLE(PAGE_VISIBILITY_API)
    WebCore::PageVisibilityState m_visibilityState;
#endif

#if PLATFORM(MAC)
    HashMap<String, String> m_temporaryPDFFiles;
#endif
        
    WebCore::ScrollPinningBehavior m_scrollPinningBehavior;
};

} // namespace WebKit

#endif // WebPageProxy_h
