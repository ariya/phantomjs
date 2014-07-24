/*
 * Copyright (C) 2010, 2011, 2013 Apple Inc. All rights reserved.
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

#ifndef WebPage_h
#define WebPage_h

#include "APIObject.h"
#include "DrawingArea.h"
#include "FindController.h"
#include "GeolocationPermissionRequestManager.h"
#include "ImageOptions.h"
#include "ImmutableArray.h"
#if ENABLE(CONTEXT_MENUS)
#include "InjectedBundlePageContextMenuClient.h"
#endif
#include "InjectedBundlePageDiagnosticLoggingClient.h"
#include "InjectedBundlePageEditorClient.h"
#include "InjectedBundlePageFormClient.h"
#include "InjectedBundlePageFullScreenClient.h"
#include "InjectedBundlePageLoaderClient.h"
#include "InjectedBundlePagePolicyClient.h"
#include "InjectedBundlePageResourceLoadClient.h"
#include "InjectedBundlePageUIClient.h"
#include "MessageReceiver.h"
#include "MessageSender.h"
#include "TapHighlightController.h"
#include "Plugin.h"
#include "SandboxExtension.h"
#include "ShareableBitmap.h"
#include "WebUndoStep.h"
#include <WebCore/DictationAlternative.h>
#include <WebCore/DragData.h>
#include <WebCore/Editor.h>
#include <WebCore/FrameLoaderTypes.h>
#include <WebCore/IntRect.h>
#include <WebCore/Page.h>
#include <WebCore/PageVisibilityState.h>
#include <WebCore/PlatformScreen.h>
#include <WebCore/ScrollTypes.h>
#include <WebCore/TextChecking.h>
#include <WebCore/WebCoreKeyboardUIMode.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(QT)
#include "ArgumentCodersQt.h"
#include "QtNetworkAccessManager.h"
#include "QtNetworkReply.h"
#include "QtNetworkReplyData.h"
#include "QtNetworkRequestData.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#endif

#if HAVE(ACCESSIBILITY) && (PLATFORM(GTK) || PLATFORM(EFL))
#include "WebPageAccessibilityObject.h"
#include <wtf/gobject/GRefPtr.h>
#endif

#if PLATFORM(GTK)
#include "ArgumentCodersGtk.h"
#include "WebPrintOperationGtk.h"
#endif

#if ENABLE(TOUCH_EVENTS)
#include <WebCore/PlatformTouchEvent.h>
#endif

#if PLATFORM(MAC)
#include "DictionaryPopupInfo.h"
#include "LayerHostingContext.h"
#include <wtf/RetainPtr.h>
OBJC_CLASS CALayer;
OBJC_CLASS NSDictionary;
OBJC_CLASS NSObject;
OBJC_CLASS WKAccessibilityWebPageObject;

#define ENABLE_PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC 1
#endif

namespace CoreIPC {
    class ArgumentDecoder;
    class Connection;
}

namespace WebCore {
    class GraphicsContext;
    class Frame;
    class FrameView;
    class HTMLPlugInElement;
    class KeyboardEvent;
    class Page;
    class PrintContext;
    class Range;
    class ResourceResponse;
    class ResourceRequest;
    class SharedBuffer;
    class TextCheckingRequest;
    class VisibleSelection;
    struct KeypressCommand;
    struct TextCheckingResult;
}

namespace WebKit {

class DrawingArea;
class InjectedBundleBackForwardList;
class NotificationPermissionRequestManager;
class PageBanner;
class PageOverlay;
class PluginView;
class SessionState;
class WebColorChooser;
class WebContextMenu;
class WebContextMenuItemData;
class WebEvent;
class WebFrame;
class WebFullScreenManager;
class WebImage;
class WebInspector;
class WebInspectorClient;
class WebKeyboardEvent;
class WebMouseEvent;
class WebNotificationClient;
class WebOpenPanelResultListener;
class WebPageGroupProxy;
class WebPopupMenu;
class WebWheelEvent;
struct AttributedString;
struct EditorState;
struct PrintInfo;
struct WebPageCreationParameters;
struct WebPreferencesStore;

#if ENABLE(GESTURE_EVENTS)
class WebGestureEvent;
#endif

#if ENABLE(TOUCH_EVENTS)
class WebTouchEvent;
#endif

typedef Vector<RefPtr<PageOverlay> > PageOverlayList;

class WebPage : public TypedAPIObject<APIObject::TypeBundlePage>, public CoreIPC::MessageReceiver, public CoreIPC::MessageSender {
public:
    static PassRefPtr<WebPage> create(uint64_t pageID, const WebPageCreationParameters&);
    virtual ~WebPage();

    void close();

    static WebPage* fromCorePage(WebCore::Page*);

    WebCore::Page* corePage() const { return m_page.get(); }
    uint64_t pageID() const { return m_pageID; }

    void setSize(const WebCore::IntSize&);
    const WebCore::IntSize& size() const { return m_viewSize; }
    WebCore::IntRect bounds() const { return WebCore::IntRect(WebCore::IntPoint(), size()); }
    
    InjectedBundleBackForwardList* backForwardList();
    DrawingArea* drawingArea() const { return m_drawingArea.get(); }

    WebPageGroupProxy* pageGroup() const { return m_pageGroup.get(); }

    void scrollMainFrameIfNotAtMaxScrollPosition(const WebCore::IntSize& scrollOffset);

    void scrollBy(uint32_t scrollDirection, uint32_t scrollGranularity);

    void centerSelectionInVisibleArea();

#if ENABLE(INSPECTOR)
    WebInspector* inspector();
#endif

#if ENABLE(FULLSCREEN_API)
    WebFullScreenManager* fullScreenManager();
#endif

    // -- Called by the DrawingArea.
    // FIXME: We could genericize these into a DrawingArea client interface. Would that be beneficial?
    void drawRect(WebCore::GraphicsContext&, const WebCore::IntRect&);
    void drawPageOverlay(PageOverlay*, WebCore::GraphicsContext&, const WebCore::IntRect&);
    void layoutIfNeeded();

    // -- Called from WebCore clients.
#if PLATFORM(MAC)
    bool handleEditingKeyboardEvent(WebCore::KeyboardEvent*, bool saveCommands);
#elif !PLATFORM(GTK)
    bool handleEditingKeyboardEvent(WebCore::KeyboardEvent*);
#endif

    void didStartPageTransition();
    void didCompletePageTransition();
    void didCommitLoad(WebFrame*);
    void didFinishLoad(WebFrame*);
    void show();
    String userAgent() const { return m_userAgent; }
    WebCore::IntRect windowResizerRect() const;
    WebCore::KeyboardUIMode keyboardUIMode();

    WebUndoStep* webUndoStep(uint64_t);
    void addWebUndoStep(uint64_t, WebUndoStep*);
    void removeWebEditCommand(uint64_t);
    bool isInRedo() const { return m_isInRedo; }

    void setActivePopupMenu(WebPopupMenu*);

#if ENABLE(INPUT_TYPE_COLOR)
    WebColorChooser* activeColorChooser() const { return m_activeColorChooser; }
    void setActiveColorChooser(WebColorChooser*);
    void didChooseColor(const WebCore::Color&);
    void didEndColorChooser();
#endif

    WebOpenPanelResultListener* activeOpenPanelResultListener() const { return m_activeOpenPanelResultListener.get(); }
    void setActiveOpenPanelResultListener(PassRefPtr<WebOpenPanelResultListener>);

    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;
    void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&) OVERRIDE;

    // -- InjectedBundle methods
#if ENABLE(CONTEXT_MENUS)
    void initializeInjectedBundleContextMenuClient(WKBundlePageContextMenuClient*);
#endif
    void initializeInjectedBundleEditorClient(WKBundlePageEditorClient*);
    void initializeInjectedBundleFormClient(WKBundlePageFormClient*);
    void initializeInjectedBundleLoaderClient(WKBundlePageLoaderClient*);
    void initializeInjectedBundlePolicyClient(WKBundlePagePolicyClient*);
    void initializeInjectedBundleResourceLoadClient(WKBundlePageResourceLoadClient*);
    void initializeInjectedBundleUIClient(WKBundlePageUIClient*);
#if ENABLE(FULLSCREEN_API)
    void initializeInjectedBundleFullScreenClient(WKBundlePageFullScreenClient*);
#endif
    void initializeInjectedBundleDiagnosticLoggingClient(WKBundlePageDiagnosticLoggingClient*);

#if ENABLE(CONTEXT_MENUS)
    InjectedBundlePageContextMenuClient& injectedBundleContextMenuClient() { return m_contextMenuClient; }
#endif
    InjectedBundlePageEditorClient& injectedBundleEditorClient() { return m_editorClient; }
    InjectedBundlePageFormClient& injectedBundleFormClient() { return m_formClient; }
    InjectedBundlePageLoaderClient& injectedBundleLoaderClient() { return m_loaderClient; }
    InjectedBundlePagePolicyClient& injectedBundlePolicyClient() { return m_policyClient; }
    InjectedBundlePageResourceLoadClient& injectedBundleResourceLoadClient() { return m_resourceLoadClient; }
    InjectedBundlePageUIClient& injectedBundleUIClient() { return m_uiClient; }
    InjectedBundlePageDiagnosticLoggingClient& injectedBundleDiagnosticLoggingClient() { return m_logDiagnosticMessageClient; }
#if ENABLE(FULLSCREEN_API)
    InjectedBundlePageFullScreenClient& injectedBundleFullScreenClient() { return m_fullScreenClient; }
#endif

    void setUnderlayPage(PassRefPtr<WebPage> underlayPage) { m_underlayPage = underlayPage; }

    bool findStringFromInjectedBundle(const String&, FindOptions);

    WebFrame* mainWebFrame() const { return m_mainFrame.get(); }

    WebCore::Frame* mainFrame() const; // May return 0.
    WebCore::FrameView* mainFrameView() const; // May return 0.

    PassRefPtr<WebCore::Range> currentSelectionAsRange();

#if ENABLE(NETSCAPE_PLUGIN_API)
    PassRefPtr<Plugin> createPlugin(WebFrame*, WebCore::HTMLPlugInElement*, const Plugin::Parameters&, String& newMIMEType);
#endif

    EditorState editorState() const;

    String renderTreeExternalRepresentation() const;
    String renderTreeExternalRepresentationForPrinting() const;
    uint64_t renderTreeSize() const;

    void setTracksRepaints(bool);
    bool isTrackingRepaints() const;
    void resetTrackedRepaints();
    PassRefPtr<ImmutableArray> trackedRepaintRects();

    void executeEditingCommand(const String& commandName, const String& argument);
    bool isEditingCommandEnabled(const String& commandName);
    void clearMainFrameName();
    void sendClose();

    void sendSetWindowFrame(const WebCore::FloatRect&);

    double textZoomFactor() const;
    void setTextZoomFactor(double);
    double pageZoomFactor() const;
    void setPageZoomFactor(double);
    void setPageAndTextZoomFactors(double pageZoomFactor, double textZoomFactor);
    void windowScreenDidChange(uint64_t);

    void scalePage(double scale, const WebCore::IntPoint& origin);
    double pageScaleFactor() const;

    void setUseFixedLayout(bool);
    bool useFixedLayout() const { return m_useFixedLayout; }
    void setFixedLayoutSize(const WebCore::IntSize&);

    void listenForLayoutMilestones(uint32_t /* LayoutMilestones */);

    void setSuppressScrollbarAnimations(bool);

    void setRubberBandsAtBottom(bool);
    void setRubberBandsAtTop(bool);

    void setPaginationMode(uint32_t /* WebCore::Pagination::Mode */);
    void setPaginationBehavesLikeColumns(bool);
    void setPageLength(double);
    void setGapBetweenPages(double);

    void postInjectedBundleMessage(const String& messageName, CoreIPC::MessageDecoder&);

    bool drawsBackground() const { return m_drawsBackground; }
    bool drawsTransparentBackground() const { return m_drawsTransparentBackground; }

    void setUnderlayColor(const WebCore::Color& color) { m_underlayColor = color; }
    WebCore::Color underlayColor() const { return m_underlayColor; }

    void stopLoading();
    void stopLoadingFrame(uint64_t frameID);
    void setDefersLoading(bool deferLoading);

#if USE(ACCELERATED_COMPOSITING)
    void enterAcceleratedCompositingMode(WebCore::GraphicsLayer*);
    void exitAcceleratedCompositingMode();
#endif

    void addPluginView(PluginView*);
    void removePluginView(PluginView*);

#if PLATFORM(MAC)
    LayerHostingMode layerHostingMode() const { return m_layerHostingMode; }
    void setLayerHostingMode(LayerHostingMode);

    bool windowIsVisible() const { return m_windowIsVisible; }
    void updatePluginsActiveAndFocusedState();
    const WebCore::FloatRect& windowFrameInScreenCoordinates() const { return m_windowFrameInScreenCoordinates; }
    const WebCore::FloatRect& windowFrameInUnflippedScreenCoordinates() const { return m_windowFrameInUnflippedScreenCoordinates; }
    const WebCore::FloatRect& viewFrameInWindowCoordinates() const { return m_viewFrameInWindowCoordinates; }

    bool hasCachedWindowFrame() const { return m_hasCachedWindowFrame; }

    void setTopOverhangImage(PassRefPtr<WebImage>);
    void setBottomOverhangImage(PassRefPtr<WebImage>);

    void updateHeaderAndFooterLayersForDeviceScaleChange(float scaleFactor);
#endif // PLATFORM(MAC)

    bool windowIsFocused() const;
    bool windowAndWebPageAreFocused() const;
    void installPageOverlay(PassRefPtr<PageOverlay>, bool shouldFadeIn = false);
    void uninstallPageOverlay(PageOverlay*, bool shouldFadeOut = false);
    bool hasPageOverlay() const { return m_pageOverlays.size(); }
    PageOverlayList& pageOverlays() { return m_pageOverlays; }

    void setHeaderPageBanner(PassRefPtr<PageBanner>);
    PageBanner* headerPageBanner();
    void setFooterPageBanner(PassRefPtr<PageBanner>);
    PageBanner* footerPageBanner();

    void hidePageBanners();
    void showPageBanners();

    WebCore::IntPoint screenToWindow(const WebCore::IntPoint&);
    WebCore::IntRect windowToScreen(const WebCore::IntRect&);

    PassRefPtr<WebImage> scaledSnapshotWithOptions(const WebCore::IntRect&, double scaleFactor, SnapshotOptions);

    static const WebEvent* currentEvent();

    FindController& findController() { return m_findController; }
#if ENABLE(TOUCH_EVENTS) && PLATFORM(QT)
    TapHighlightController& tapHighlightController() { return m_tapHighlightController; }
#endif

#if ENABLE(GEOLOCATION)
    GeolocationPermissionRequestManager& geolocationPermissionRequestManager() { return m_geolocationPermissionRequestManager; }
#endif

    NotificationPermissionRequestManager* notificationPermissionRequestManager();

    void pageDidScroll();
#if USE(TILED_BACKING_STORE)
    void pageDidRequestScroll(const WebCore::IntPoint&);
    void setFixedVisibleContentRect(const WebCore::IntRect&);
    void sendViewportAttributesChanged();
#endif

#if ENABLE(CONTEXT_MENUS)
    WebContextMenu* contextMenu();
    WebContextMenu* contextMenuAtPointInWindow(const WebCore::IntPoint&);
#endif
    
    bool hasLocalDataForURL(const WebCore::KURL&);
    String cachedResponseMIMETypeForURL(const WebCore::KURL&);
    String cachedSuggestedFilenameForURL(const WebCore::KURL&);
    PassRefPtr<WebCore::SharedBuffer> cachedResponseDataForURL(const WebCore::KURL&);

    static bool canHandleRequest(const WebCore::ResourceRequest&);

    class SandboxExtensionTracker {
    public:
        ~SandboxExtensionTracker();

        void invalidate();

        void beginLoad(WebFrame*, const SandboxExtension::Handle& handle);
        void willPerformLoadDragDestinationAction(PassRefPtr<SandboxExtension> pendingDropSandboxExtension);
        void didStartProvisionalLoad(WebFrame*);
        void didCommitProvisionalLoad(WebFrame*);
        void didFailProvisionalLoad(WebFrame*);

    private:
        void setPendingProvisionalSandboxExtension(PassRefPtr<SandboxExtension>);

        RefPtr<SandboxExtension> m_pendingProvisionalSandboxExtension;
        RefPtr<SandboxExtension> m_provisionalSandboxExtension;
        RefPtr<SandboxExtension> m_committedSandboxExtension;
    };

    SandboxExtensionTracker& sandboxExtensionTracker() { return m_sandboxExtensionTracker; }

#if PLATFORM(EFL)
    void setThemePath(const String&);
#endif

#if USE(TILED_BACKING_STORE)
    void commitPageTransitionViewport();
#endif

#if PLATFORM(QT) || PLATFORM(GTK)
    void setComposition(const String& text, Vector<WebCore::CompositionUnderline> underlines, uint64_t selectionStart, uint64_t selectionEnd, uint64_t replacementRangeStart, uint64_t replacementRangeEnd);
    void confirmComposition(const String& text, int64_t selectionStart, int64_t selectionLength);
    void cancelComposition();
#endif

    void didChangeSelection();

#if PLATFORM(MAC)
    void registerUIProcessAccessibilityTokens(const CoreIPC::DataReference& elemenToken, const CoreIPC::DataReference& windowToken);
    WKAccessibilityWebPageObject* accessibilityRemoteObject();
    NSObject *accessibilityObjectForMainFramePlugin();
    const WebCore::FloatPoint& accessibilityPosition() const { return m_accessibilityPosition; }
    
    void sendComplexTextInputToPlugin(uint64_t pluginComplexTextInputIdentifier, const String& textInput);

    void setComposition(const String& text, Vector<WebCore::CompositionUnderline> underlines, uint64_t selectionStart, uint64_t selectionEnd, uint64_t replacementRangeStart, uint64_t replacementRangeEnd, EditorState& newState);
    void confirmComposition(EditorState& newState);
    void cancelComposition(EditorState& newState);
    void insertText(const String& text, uint64_t replacementRangeStart, uint64_t replacementRangeEnd, bool& handled, EditorState& newState);
    void getMarkedRange(uint64_t& location, uint64_t& length);
    void getSelectedRange(uint64_t& location, uint64_t& length);
    void getAttributedSubstringFromRange(uint64_t location, uint64_t length, AttributedString&);
    void characterIndexForPoint(const WebCore::IntPoint point, uint64_t& result);
    void firstRectForCharacterRange(uint64_t location, uint64_t length, WebCore::IntRect& resultRect);
    void executeKeypressCommands(const Vector<WebCore::KeypressCommand>&, bool& handled, EditorState& newState);
    void readSelectionFromPasteboard(const WTF::String& pasteboardName, bool& result);
    void getStringSelectionForPasteboard(WTF::String& stringValue);
    void getDataSelectionForPasteboard(const WTF::String pasteboardType, SharedMemory::Handle& handle, uint64_t& size);
    void shouldDelayWindowOrderingEvent(const WebKit::WebMouseEvent&, bool& result);
    void acceptsFirstMouse(int eventNumber, const WebKit::WebMouseEvent&, bool& result);
    bool performNonEditingBehaviorForSelector(const String&);
    void insertDictatedText(const String& text, uint64_t replacementRangeStart, uint64_t replacementRangeEnd, const Vector<WebCore::DictationAlternative>& dictationAlternativeLocations, bool& handled, EditorState& newState);
#elif PLATFORM(EFL)
    void confirmComposition(const String& compositionString);
    void setComposition(const WTF::String& compositionString, const WTF::Vector<WebCore::CompositionUnderline>& underlines, uint64_t cursorPosition);
    void cancelComposition();
#elif PLATFORM(GTK)
#if USE(TEXTURE_MAPPER_GL)
    void setAcceleratedCompositingWindowId(int64_t nativeWindowHandle);
#endif
#endif

#if HAVE(ACCESSIBILITY) && (PLATFORM(GTK) || PLATFORM(EFL))
    void updateAccessibilityTree();
#endif

    void setCompositionForTesting(const String& compositionString, uint64_t from, uint64_t length);
    bool hasCompositionForTesting();
    void confirmCompositionForTesting(const String& compositionString);

    // FIXME: This a dummy message, to avoid breaking the build for platforms that don't require
    // any synchronous messages, and should be removed when <rdar://problem/8775115> is fixed.
    void dummy(bool&);

#if PLATFORM(MAC)
    void performDictionaryLookupForSelection(WebCore::Frame*, const WebCore::VisibleSelection&);

    bool isSpeaking();
    void speak(const String&);
    void stopSpeaking();

#endif

    bool isSmartInsertDeleteEnabled();
    void setSmartInsertDeleteEnabled(bool);

    bool isSelectTrailingWhitespaceEnabled();
    void setSelectTrailingWhitespaceEnabled(bool);

    void replaceSelectionWithText(WebCore::Frame*, const String&);
    void clearSelection();

#if ENABLE(DRAG_SUPPORT)
#if PLATFORM(QT) || PLATFORM(GTK)
    void performDragControllerAction(uint64_t action, WebCore::DragData);
#else
    void performDragControllerAction(uint64_t action, WebCore::IntPoint clientPosition, WebCore::IntPoint globalPosition, uint64_t draggingSourceOperationMask, const WTF::String& dragStorageName, uint32_t flags, const SandboxExtension::Handle&, const SandboxExtension::HandleArray&);
#endif
    void dragEnded(WebCore::IntPoint clientPosition, WebCore::IntPoint globalPosition, uint64_t operation);

    void willPerformLoadDragDestinationAction();
    void mayPerformUploadDragDestinationAction();
#endif // ENABLE(DRAG_SUPPORT)

    void beginPrinting(uint64_t frameID, const PrintInfo&);
    void endPrinting();
    void computePagesForPrinting(uint64_t frameID, const PrintInfo&, uint64_t callbackID);
#if PLATFORM(MAC)
    void drawRectToImage(uint64_t frameID, const PrintInfo&, const WebCore::IntRect&, const WebCore::IntSize&, uint64_t callbackID);
    void drawPagesToPDF(uint64_t frameID, const PrintInfo&, uint32_t first, uint32_t count, uint64_t callbackID);
#elif PLATFORM(GTK)
    void drawPagesForPrinting(uint64_t frameID, const PrintInfo&, uint64_t callbackID);
#endif

    void setMediaVolume(float);
    void setMayStartMediaWhenInWindow(bool);

    void didChangeScrollOffsetForMainFrame();

    void mainFrameDidLayout();

    bool canRunBeforeUnloadConfirmPanel() const { return m_canRunBeforeUnloadConfirmPanel; }
    void setCanRunBeforeUnloadConfirmPanel(bool canRunBeforeUnloadConfirmPanel) { m_canRunBeforeUnloadConfirmPanel = canRunBeforeUnloadConfirmPanel; }

    bool canRunModal() const { return m_canRunModal; }
    void setCanRunModal(bool canRunModal) { m_canRunModal = canRunModal; }

    void runModal();

    void setDeviceScaleFactor(float);
    float deviceScaleFactor() const;

    void setMemoryCacheMessagesEnabled(bool);

    void forceRepaintWithoutCallback();

    void unmarkAllMisspellings();
    void unmarkAllBadGrammar();
#if PLATFORM(MAC)
    void handleAlternativeTextUIResult(const String&);
#endif

    // For testing purpose.
    void simulateMouseDown(int button, WebCore::IntPoint, int clickCount, WKEventModifiers, double time);
    void simulateMouseUp(int button, WebCore::IntPoint, int clickCount, WKEventModifiers, double time);
    void simulateMouseMotion(WebCore::IntPoint, double time);

#if ENABLE(CONTEXT_MENUS)
    void contextMenuShowing() { m_isShowingContextMenu = true; }
#endif

#if PLATFORM(QT)
    void registerApplicationScheme(const String& scheme);
    void applicationSchemeReply(const QtNetworkReplyData&);
    void receivedApplicationSchemeRequest(const QNetworkRequest&, QtNetworkReply*);
    void setUserScripts(const Vector<String>&);
#endif
    void wheelEvent(const WebWheelEvent&);
#if ENABLE(GESTURE_EVENTS)
    void gestureEvent(const WebGestureEvent&);
#endif

    void numWheelEventHandlersChanged(unsigned);
    void recomputeShortCircuitHorizontalWheelEventsState();

    bool willGoToBackForwardItemCallbackEnabled() const { return m_willGoToBackForwardItemCallbackEnabled; }

#if ENABLE(PAGE_VISIBILITY_API) || ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
    void setVisibilityState(uint32_t /* WebCore::PageVisibilityState */, bool isInitialState);
#endif
    void setThrottled(bool isThrottled);

#if PLATFORM(GTK) && USE(TEXTURE_MAPPER_GL)
    uint64_t nativeWindowHandle() { return m_nativeWindowHandle; }
#endif

    bool canPluginHandleResponse(const WebCore::ResourceResponse& response);

    bool asynchronousPluginInitializationEnabled() const { return m_asynchronousPluginInitializationEnabled; }
    void setAsynchronousPluginInitializationEnabled(bool enabled) { m_asynchronousPluginInitializationEnabled = enabled; }
    bool asynchronousPluginInitializationEnabledForAllPlugins() const { return m_asynchronousPluginInitializationEnabledForAllPlugins; }
    void setAsynchronousPluginInitializationEnabledForAllPlugins(bool enabled) { m_asynchronousPluginInitializationEnabledForAllPlugins = enabled; }
    bool artificialPluginInitializationDelayEnabled() const { return m_artificialPluginInitializationDelayEnabled; }
    void setArtificialPluginInitializationDelayEnabled(bool enabled) { m_artificialPluginInitializationDelayEnabled = enabled; }
    void setTabToLinksEnabled(bool enabled) { m_tabToLinks = enabled; }
    bool tabToLinksEnabled() const { return m_tabToLinks; }

    bool scrollingPerformanceLoggingEnabled() const { return m_scrollingPerformanceLoggingEnabled; }
    void setScrollingPerformanceLoggingEnabled(bool);

#if PLATFORM(MAC)
    bool shouldUsePDFPlugin() const;
    bool pdfPluginEnabled() const { return m_pdfPluginEnabled; }
    void setPDFPluginEnabled(bool enabled) { m_pdfPluginEnabled = enabled; }
#endif

    void savePDFToFileInDownloadsFolder(const String& suggestedFilename, const String& originatingURLString, const uint8_t* data, unsigned long size);
#if PLATFORM(MAC)
    void savePDFToTemporaryFolderAndOpenWithNativeApplication(const String& suggestedFilename, const String& originatingURLString, const uint8_t* data, unsigned long size, const String& pdfUUID);
#endif

    bool mainFrameIsScrollable() const { return m_mainFrameIsScrollable; }

    void setMinimumLayoutSize(const WebCore::IntSize&);
    WebCore::IntSize minimumLayoutSize() const { return m_minimumLayoutSize; }

    bool canShowMIMEType(const String& MIMEType) const;

    void addTextCheckingRequest(uint64_t requestID, PassRefPtr<WebCore::TextCheckingRequest>);
    void didFinishCheckingText(uint64_t requestID, const Vector<WebCore::TextCheckingResult>&);
    void didCancelCheckingText(uint64_t requestID);

#if ENABLE(PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC)
    void determinePrimarySnapshottedPlugIn();
    void determinePrimarySnapshottedPlugInTimerFired();
    void resetPrimarySnapshottedPlugIn();
    bool matchesPrimaryPlugIn(const String& pageOrigin, const String& pluginOrigin, const String& mimeType) const;
#endif

    unsigned extendIncrementalRenderingSuppression();
    void stopExtendingIncrementalRenderingSuppression(unsigned token);
    bool shouldExtendIncrementalRenderingSuppression() { return !m_activeRenderingSuppressionTokens.isEmpty(); }

    WebCore::ScrollPinningBehavior scrollPinningBehavior() { return m_scrollPinningBehavior; }
    void setScrollPinningBehavior(uint32_t /* WebCore::ScrollPinningBehavior */ pinning);

private:
    WebPage(uint64_t pageID, const WebPageCreationParameters&);

    // CoreIPC::MessageSender
    virtual CoreIPC::Connection* messageSenderConnection() OVERRIDE;
    virtual uint64_t messageSenderDestinationID() OVERRIDE;

    void platformInitialize();

    void didReceiveWebPageMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    void didReceiveSyncWebPageMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&);

#if !PLATFORM(MAC)
    static const char* interpretKeyEvent(const WebCore::KeyboardEvent*);
#endif
    bool performDefaultBehaviorForKeyEvent(const WebKeyboardEvent&);

#if PLATFORM(MAC)
    bool executeKeypressCommandsInternal(const Vector<WebCore::KeypressCommand>&, WebCore::KeyboardEvent*);
#endif

    String sourceForFrame(WebFrame*);

    void loadDataImpl(PassRefPtr<WebCore::SharedBuffer>, const String& MIMEType, const String& encodingName, const WebCore::KURL& baseURL, const WebCore::KURL& failingURL, CoreIPC::MessageDecoder&);

    bool platformHasLocalDataForURL(const WebCore::KURL&);

    // Actions
    void tryClose();
    void loadURL(const String&, const SandboxExtension::Handle&, CoreIPC::MessageDecoder&);
    void loadURLRequest(const WebCore::ResourceRequest&, const SandboxExtension::Handle&, CoreIPC::MessageDecoder&);
    void loadData(const CoreIPC::DataReference&, const String& MIMEType, const String& encodingName, const String& baseURL, CoreIPC::MessageDecoder&);
    void loadHTMLString(const String& htmlString, const String& baseURL, CoreIPC::MessageDecoder&);
    void loadAlternateHTMLString(const String& htmlString, const String& baseURL, const String& unreachableURL, CoreIPC::MessageDecoder&);
    void loadPlainTextString(const String&, CoreIPC::MessageDecoder&);
    void loadWebArchiveData(const CoreIPC::DataReference&, CoreIPC::MessageDecoder&);
    void linkClicked(const String& url, const WebMouseEvent&);
    void reload(bool reloadFromOrigin, const SandboxExtension::Handle&);
    void goForward(uint64_t);
    void goBack(uint64_t);
    void goToBackForwardItem(uint64_t);
    void tryRestoreScrollPosition();
    void setActive(bool);
    void setFocused(bool);
    void setInitialFocus(bool forward, bool isKeyboardEventValid, const WebKeyboardEvent&);
    void setWindowResizerSize(const WebCore::IntSize&);
    void setIsInWindow(bool);
    void validateCommand(const String&, uint64_t);
    void executeEditCommand(const String&);

    void mouseEvent(const WebMouseEvent&);
    void mouseEventSyncForTesting(const WebMouseEvent&, bool&);
    void wheelEventSyncForTesting(const WebWheelEvent&, bool&);
    void keyEvent(const WebKeyboardEvent&);
    void keyEventSyncForTesting(const WebKeyboardEvent&, bool&);
#if ENABLE(TOUCH_EVENTS)
    void touchEvent(const WebTouchEvent&);
    void touchEventSyncForTesting(const WebTouchEvent&, bool& handled);
#if PLATFORM(QT)
    void highlightPotentialActivation(const WebCore::IntPoint&, const WebCore::IntSize& area);
#endif
#endif
#if ENABLE(CONTEXT_MENUS)
    void contextMenuHidden() { m_isShowingContextMenu = false; }
#endif

    static void scroll(WebCore::Page*, WebCore::ScrollDirection, WebCore::ScrollGranularity);
    static void logicalScroll(WebCore::Page*, WebCore::ScrollLogicalDirection, WebCore::ScrollGranularity);

    uint64_t restoreSession(const SessionState&);
    void restoreSessionAndNavigateToCurrentItem(const SessionState&);

    void didRemoveBackForwardItem(uint64_t);

    void setWillGoToBackForwardItemCallbackEnabled(bool enabled) { m_willGoToBackForwardItemCallbackEnabled = enabled; }
    
    void setDrawsBackground(bool);
    void setDrawsTransparentBackground(bool);

    void viewWillStartLiveResize();
    void viewWillEndLiveResize();

    void getContentsAsString(uint64_t callbackID);
#if ENABLE(MHTML)
    void getContentsAsMHTMLData(uint64_t callbackID, bool useBinaryEncoding);
#endif
    void getMainResourceDataOfFrame(uint64_t frameID, uint64_t callbackID);
    void getResourceDataFromFrame(uint64_t frameID, const String& resourceURL, uint64_t callbackID);
    void getRenderTreeExternalRepresentation(uint64_t callbackID);
    void getSelectionOrContentsAsString(uint64_t callbackID);
    void getSelectionAsWebArchiveData(uint64_t callbackID);
    void getSourceForFrame(uint64_t frameID, uint64_t callbackID);
    void getWebArchiveOfFrame(uint64_t frameID, uint64_t callbackID);
    void runJavaScriptInMainFrame(const String&, uint64_t callbackID);
    void forceRepaint(uint64_t callbackID);

    void preferencesDidChange(const WebPreferencesStore&);
    void platformPreferencesDidChange(const WebPreferencesStore&);
    void updatePreferences(const WebPreferencesStore&);

    void didReceivePolicyDecision(uint64_t frameID, uint64_t listenerID, uint32_t policyAction, uint64_t downloadID);
    void setUserAgent(const String&);
    void setCustomTextEncodingName(const String&);
    void suspendActiveDOMObjectsAndAnimations();
    void resumeActiveDOMObjectsAndAnimations();

#if PLATFORM(MAC)
    void performDictionaryLookupAtLocation(const WebCore::FloatPoint&);
    void performDictionaryLookupForRange(WebCore::Frame*, WebCore::Range*, NSDictionary *options);

    void setWindowIsVisible(bool windowIsVisible);
    void windowAndViewFramesChanged(const WebCore::FloatRect& windowFrameInScreenCoordinates, const WebCore::FloatRect& windowFrameInUnflippedScreenCoordinates, const WebCore::FloatRect& viewFrameInWindowCoordinates, const WebCore::FloatPoint& accessibilityViewCoordinates);

    RetainPtr<PDFDocument> pdfDocumentForPrintingFrame(WebCore::Frame*);
    void computePagesForPrintingPDFDocument(uint64_t frameID, const PrintInfo&, Vector<WebCore::IntRect>& resultPageRects);
    void drawPDFDocument(CGContextRef, PDFDocument *, const PrintInfo&, const WebCore::IntRect&);
    void drawPagesToPDFFromPDFDocument(CGContextRef, PDFDocument *, const PrintInfo&, uint32_t first, uint32_t count);
#endif

    void viewExposedRectChanged(const WebCore::FloatRect& exposedRect, bool clipsToExposedRect);
    void setMainFrameIsScrollable(bool);

    void unapplyEditCommand(uint64_t commandID);
    void reapplyEditCommand(uint64_t commandID);
    void didRemoveEditCommand(uint64_t commandID);

    void findString(const String&, uint32_t findOptions, uint32_t maxMatchCount);
    void findStringMatches(const String&, uint32_t findOptions, uint32_t maxMatchCount);
    void getImageForFindMatch(uint32_t matchIndex);
    void selectFindMatch(uint32_t matchIndex);
    void hideFindUI();
    void countStringMatches(const String&, uint32_t findOptions, uint32_t maxMatchCount);

#if USE(COORDINATED_GRAPHICS)
    void findZoomableAreaForPoint(const WebCore::IntPoint&, const WebCore::IntSize& area);
#endif

    void didChangeSelectedIndexForActivePopupMenu(int32_t newIndex);
    void setTextForActivePopupMenu(int32_t index);

#if PLATFORM(GTK)
    void failedToShowPopupMenu();
#endif
#if PLATFORM(QT)
    void hidePopupMenu();
    void selectedIndex(int32_t newIndex);
#endif

    void didChooseFilesForOpenPanel(const Vector<String>&);
    void didCancelForOpenPanel();
#if ENABLE(WEB_PROCESS_SANDBOX)
    void extendSandboxForFileFromOpenPanel(const SandboxExtension::Handle&);
#endif

    void didReceiveGeolocationPermissionDecision(uint64_t geolocationID, bool allowed);

    void didReceiveNotificationPermissionDecision(uint64_t notificationID, bool allowed);

    void advanceToNextMisspelling(bool startBeforeSelection);
    void changeSpellingToWord(const String& word);
#if USE(APPKIT)
    void uppercaseWord();
    void lowercaseWord();
    void capitalizeWord();
#endif

#if ENABLE(CONTEXT_MENUS)
    void didSelectItemFromActiveContextMenu(const WebContextMenuItemData&);
#endif

    void changeSelectedIndex(int32_t index);
    void setCanStartMediaTimerFired();
    void didUpdateInWindowStateTimerFired();

    bool canHandleUserEvents() const;

    void setMainFrameInViewSourceMode(bool);

    static bool platformCanHandleRequest(const WebCore::ResourceRequest&);

    static PluginView* focusedPluginViewForFrame(WebCore::Frame*);
    static PluginView* pluginViewForFrame(WebCore::Frame*);

    void reportUsedFeatures();

    OwnPtr<WebCore::Page> m_page;
    RefPtr<WebFrame> m_mainFrame;
    RefPtr<InjectedBundleBackForwardList> m_backForwardList;

    RefPtr<WebPageGroupProxy> m_pageGroup;

    String m_userAgent;

    WebCore::IntSize m_viewSize;
    OwnPtr<DrawingArea> m_drawingArea;

    HashSet<PluginView*> m_pluginViews;

    HashMap<uint64_t, RefPtr<WebCore::TextCheckingRequest> > m_pendingTextCheckingRequestMap;

    bool m_useFixedLayout;

    bool m_drawsBackground;
    bool m_drawsTransparentBackground;

    WebCore::Color m_underlayColor;

    bool m_isInRedo;
    bool m_isClosed;

    bool m_tabToLinks;
    
    bool m_asynchronousPluginInitializationEnabled;
    bool m_asynchronousPluginInitializationEnabledForAllPlugins;
    bool m_artificialPluginInitializationDelayEnabled;

    bool m_scrollingPerformanceLoggingEnabled;

    bool m_mainFrameIsScrollable;

#if ENABLE(PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC)
    bool m_readyToFindPrimarySnapshottedPlugin;
    bool m_didFindPrimarySnapshottedPlugin;
    String m_primaryPlugInPageOrigin;
    String m_primaryPlugInOrigin;
    String m_primaryPlugInMimeType;
    WebCore::RunLoop::Timer<WebPage> m_determinePrimarySnapshottedPlugInTimer;
#endif

#if PLATFORM(MAC)
    bool m_pdfPluginEnabled;

    bool m_hasCachedWindowFrame;

    // Whether the containing window is visible or not.
    bool m_windowIsVisible;

    // The frame of the containing window in screen coordinates.
    WebCore::FloatRect m_windowFrameInScreenCoordinates;

    // The frame of the containing window in unflipped screen coordinates.
    WebCore::FloatRect m_windowFrameInUnflippedScreenCoordinates;

    // The frame of the view in window coordinates.
    WebCore::FloatRect m_viewFrameInWindowCoordinates;

    // The accessibility position of the view.
    WebCore::FloatPoint m_accessibilityPosition;
    
    // The layer hosting mode.
    LayerHostingMode m_layerHostingMode;

    RetainPtr<WKAccessibilityWebPageObject> m_mockAccessibilityElement;

    WebCore::KeyboardEvent* m_keyboardEventBeingInterpreted;

#elif HAVE(ACCESSIBILITY) && (PLATFORM(GTK) || PLATFORM(EFL))
    GRefPtr<WebPageAccessibilityObject> m_accessibilityObject;

#if USE(TEXTURE_MAPPER_GL)
    // Our view's window in the UI process.
    uint64_t m_nativeWindowHandle;
#endif
#endif

    RefPtr<PageBanner> m_headerBanner;
    RefPtr<PageBanner> m_footerBanner;

    WebCore::RunLoop::Timer<WebPage> m_setCanStartMediaTimer;
    WebCore::RunLoop::Timer<WebPage> m_sendDidUpdateInWindowStateTimer;
    bool m_mayStartMediaWhenInWindow;

    HashMap<uint64_t, RefPtr<WebUndoStep> > m_undoStepMap;

    WebCore::IntSize m_windowResizerSize;

#if ENABLE(CONTEXT_MENUS)
    InjectedBundlePageContextMenuClient m_contextMenuClient;
#endif
    InjectedBundlePageEditorClient m_editorClient;
    InjectedBundlePageFormClient m_formClient;
    InjectedBundlePageLoaderClient m_loaderClient;
    InjectedBundlePagePolicyClient m_policyClient;
    InjectedBundlePageResourceLoadClient m_resourceLoadClient;
    InjectedBundlePageUIClient m_uiClient;
#if ENABLE(FULLSCREEN_API)
    InjectedBundlePageFullScreenClient m_fullScreenClient;
#endif
    InjectedBundlePageDiagnosticLoggingClient m_logDiagnosticMessageClient;

    FindController m_findController;
#if ENABLE(TOUCH_EVENTS) && PLATFORM(QT)
    TapHighlightController m_tapHighlightController;
#endif
    PageOverlayList m_pageOverlays;

    RefPtr<WebPage> m_underlayPage;

#if ENABLE(INSPECTOR)
    RefPtr<WebInspector> m_inspector;
#endif
#if ENABLE(FULLSCREEN_API)
    RefPtr<WebFullScreenManager> m_fullScreenManager;
#endif
    RefPtr<WebPopupMenu> m_activePopupMenu;
#if ENABLE(CONTEXT_MENUS)
    RefPtr<WebContextMenu> m_contextMenu;
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    WebColorChooser* m_activeColorChooser;
#endif
    RefPtr<WebOpenPanelResultListener> m_activeOpenPanelResultListener;
    RefPtr<NotificationPermissionRequestManager> m_notificationPermissionRequestManager;

#if ENABLE(GEOLOCATION)
    GeolocationPermissionRequestManager m_geolocationPermissionRequestManager;
#endif

    OwnPtr<WebCore::PrintContext> m_printContext;
#if PLATFORM(GTK)
    RefPtr<WebPrintOperationGtk> m_printOperation;
#endif

    SandboxExtensionTracker m_sandboxExtensionTracker;
    uint64_t m_pageID;

    RefPtr<SandboxExtension> m_pendingDropSandboxExtension;
    Vector<RefPtr<SandboxExtension> > m_pendingDropExtensionsForFileUpload;

    bool m_canRunBeforeUnloadConfirmPanel;

    bool m_canRunModal;
    bool m_isRunningModal;

    bool m_cachedMainFrameIsPinnedToLeftSide;
    bool m_cachedMainFrameIsPinnedToRightSide;
    bool m_cachedMainFrameIsPinnedToTopSide;
    bool m_cachedMainFrameIsPinnedToBottomSide;
    bool m_canShortCircuitHorizontalWheelEvents;
    unsigned m_numWheelEventHandlers;

    unsigned m_cachedPageCount;

    WebCore::IntSize m_minimumLayoutSize;

#if ENABLE(CONTEXT_MENUS)
    bool m_isShowingContextMenu;
#endif
    
    bool m_willGoToBackForwardItemCallbackEnabled;
    
#if PLATFORM(QT)
    HashMap<String, QtNetworkReply*> m_applicationSchemeReplies;
#endif
#if ENABLE(PAGE_VISIBILITY_API)
    WebCore::PageVisibilityState m_visibilityState;
#endif
    WebInspectorClient* m_inspectorClient;

    WebCore::Color m_backgroundColor;

    HashSet<unsigned> m_activeRenderingSuppressionTokens;
    unsigned m_maximumRenderingSuppressionToken;
    
    WebCore::ScrollPinningBehavior m_scrollPinningBehavior;
};

} // namespace WebKit

#endif // WebPage_h
