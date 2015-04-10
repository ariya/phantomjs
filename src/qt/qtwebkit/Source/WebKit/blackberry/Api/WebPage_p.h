/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef WebPage_p_h
#define WebPage_p_h

#include "ChromeClient.h"
#include "HitTestResult.h"
#include "InRegionScroller.h"
#include "InspectorClientBlackBerry.h"
#include "InspectorOverlay.h"
#if USE(ACCELERATED_COMPOSITING)
#include "GLES2Context.h"
#include "GraphicsLayerClient.h"
#include "LayerRenderer.h"
#include <EGL/egl.h>
#endif
#include "KURL.h"
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#include "NotificationManager.h"
#endif
#include "PageClientBlackBerry.h"
#include "PlatformMouseEvent.h"
#include "ProximityDetector.h"
#include "ScriptSourceCode.h"
#include "SelectionOverlay.h"
#include "Timer.h"
#include "ViewportArguments.h"
#include "WebPage.h"
#include "WebSettings.h"
#include "WebTapHighlight.h"

#include <BlackBerryPlatformMessage.h>

#define DEFAULT_MAX_LAYOUT_WIDTH 1024
#define DEFAULT_MAX_LAYOUT_HEIGHT 768

namespace WebCore {
class AuthenticationChallengeClient;
class AutofillManager;
class DOMWrapperWorld;
class Document;
class Element;
class Frame;
class GeolocationClientBlackBerry;
class GraphicsLayerBlackBerry;
class LayerWebKitThread;
class NavigatorContentUtilsClientBlackBerry;
class Node;
class Page;
class PluginView;
class RenderLayer;
class RenderObject;
class ScrollView;
class TransformationMatrix;
template<typename T> class Timer;
}

namespace BlackBerry {
namespace WebKit {

class BackingStore;
class BackingStoreClient;
class DumpRenderTreeClient;
class InPageSearchManager;
class InputHandler;
class PagePopup;
class PagePopupClient;
class SelectionHandler;
class TouchEventHandler;
class WebCookieJar;
class WebPageClient;
class WebKitThreadViewportAccessor;

#if USE(ACCELERATED_COMPOSITING)
class FrameLayers;
class WebPageCompositorPrivate;
#endif

// In the client code, there is screen size and viewport.
// In WebPagePrivate, the screen size is called the transformedViewportSize,
// the viewport position is called the transformedScrollPosition,
// and the viewport size is called the transformedActualVisibleSize.
class WebPagePrivate : public PageClientBlackBerry
    , public WebSettingsDelegate
#if USE(ACCELERATED_COMPOSITING)
    , public WebCore::GraphicsLayerClient
#endif
#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
    , public BlackBerry::Platform::AnimationFrameRateClient
#endif
    , public Platform::GuardedPointerBase {
public:
    enum ViewMode { Desktop, FixedDesktop };
    enum LoadState { None /* on instantiation of page */, Provisional, Committed, Finished, Failed };

    WebPagePrivate(WebPage*, WebPageClient*, const WebCore::IntRect&);

    static WebCore::Page* core(const WebPage*);

    WebPageClient* client() const { return m_client; }

    void init(const BlackBerry::Platform::String& pageGroupName);
    bool handleMouseEvent(WebCore::PlatformMouseEvent&);
    bool handleWheelEvent(WebCore::PlatformWheelEvent&);

    void load(const Platform::NetworkRequest& platformRequest, bool needReferer = false);
    void loadString(const BlackBerry::Platform::String&, const BlackBerry::Platform::String& baseURL, const BlackBerry::Platform::String& mimeType, const BlackBerry::Platform::String& failingURL);
    bool executeJavaScript(const BlackBerry::Platform::String& script, JavaScriptDataType& returnType, BlackBerry::Platform::String& returnValue);
    bool executeJavaScriptInIsolatedWorld(const WebCore::ScriptSourceCode&, JavaScriptDataType& returnType, BlackBerry::Platform::String& returnValue);

    void stopCurrentLoad();
    void prepareToDestroy();

    void enableCrossSiteXHR();
    void addOriginAccessWhitelistEntry(const BlackBerry::Platform::String& sourceOrigin, const BlackBerry::Platform::String& destinationOrigin, bool allowDestinationSubdomains);
    void removeOriginAccessWhitelistEntry(const BlackBerry::Platform::String& sourceOrigin, const BlackBerry::Platform::String& destinationOrigin, bool allowDestinationSubdomains);

    LoadState loadState() const { return m_loadState; }
    bool isLoading() const { return m_loadState == WebPagePrivate::Provisional || m_loadState == WebPagePrivate::Committed; }

    // Called from within WebKit via FrameLoaderClientBlackBerry.
    void setLoadState(LoadState);

    // Clamp the scale.
    double clampedScale(double scale) const;

    // Determine if we should zoom, clamping the scale parameter if required.
    bool shouldZoomAboutPoint(double scale, const WebCore::FloatPoint& anchor, bool enforeScaleClamping, double* clampedScale);

    // Scale the page to the given scale and anchor about the point which is specified in untransformed content coordinates.
    bool zoomAboutPoint(double scale, const WebCore::FloatPoint& anchor, bool enforceScaleClamping = true, bool forceRendering = false, bool isRestoringZoomLevel = false);
    WebCore::IntPoint calculateReflowedScrollPosition(const WebCore::FloatPoint& anchorOffset, double inverseScale);
    void setTextReflowAnchorPoint(const Platform::IntPoint& focalPoint);

    void restoreHistoryViewState(const WebCore::IntPoint& scrollPosition, double scale, bool shouldReflowBlock);

    // Perform actual zoom for after zoom animation.
    void zoomAnimationFinished(double finalScale, const WebCore::FloatPoint& finalDocumentScrollPosition, bool shouldConstrainScrollingToContentEdge);

    // Called by the backing store as well as the method below.
    void updateLayoutAndStyleIfNeededRecursive() const;
    void layoutIfNeeded() const;
    void setNeedsLayout();

    WebCore::IntPoint scrollPosition() const;
    WebCore::IntPoint maximumScrollPosition() const;
    void setScrollPosition(const WebCore::IntPoint&);

    void notifyInRegionScrollStopped();
    void setScrollOriginPoint(const Platform::IntPoint&);
    void setHasInRegionScrollableAreas(bool);

    // The actual visible size as reported by the client, but in WebKit coordinates.
    WebCore::IntSize actualVisibleSize() const;

    // The viewport size is the same as the client's window size, but in webkit coordinates.
    WebCore::IntSize viewportSize() const;

    // Modifies the zoomToFit algorithm logic to construct a scale such that the viewportSize above is equal to this size.
    bool hasVirtualViewport() const;
    bool isUserScalable() const
    {
        if (!respectViewport())
            return true;
        return m_userScalable;
    }
    void setUserScalable(bool userScalable) { m_userScalable = userScalable; }

    // Sets default layout size without doing layout or marking as needing layout.
    void setDefaultLayoutSize(const WebCore::IntSize&);

    // Updates WebCore when the viewportSize() or actualVisibleSize() change.
    void updateViewportSize(bool setFixedReportedSize = true, bool sendResizeEvent = true);

    WebCore::FloatPoint centerOfVisibleContentsRect() const;
    WebCore::IntRect visibleContentsRect() const;
    WebCore::IntSize contentsSize() const;
    WebCore::IntSize absoluteVisibleOverflowSize() const;

    // Virtual functions inherited from PageClientBlackBerry.
    virtual int playerID() const;
    virtual void setCursor(WebCore::PlatformCursor);
    virtual Platform::NetworkStreamFactory* networkStreamFactory();
    virtual Platform::Graphics::Window* platformWindow() const;
    virtual void setPreventsScreenDimming(bool preventDimming);
    virtual void showVirtualKeyboard(bool showKeyboard);
    virtual void ensureContentVisible(bool centerInView = true);
    virtual void zoomToContentRect(const WebCore::IntRect&);
    virtual void registerPlugin(WebCore::PluginView*, bool);
    virtual void notifyPageOnLoad();
    virtual bool shouldPluginEnterFullScreen(WebCore::PluginView*, const char*);
    virtual void didPluginEnterFullScreen(WebCore::PluginView*, const char*);
    virtual void didPluginExitFullScreen(WebCore::PluginView*, const char*);
    virtual void onPluginStartBackgroundPlay(WebCore::PluginView*, const char*);
    virtual void onPluginStopBackgroundPlay(WebCore::PluginView*, const char*);
    virtual bool lockOrientation(bool landscape);
    virtual void unlockOrientation();
    virtual int orientation() const;
    virtual double currentZoomFactor() const;
    virtual int showAlertDialog(WebPageClient::AlertType atype);
    virtual bool isActive() const;
    virtual bool isVisible() const { return m_visible; }
    virtual void authenticationChallenge(const WebCore::KURL&, const WebCore::ProtectionSpace&, const WebCore::Credential&);
    virtual SaveCredentialType notifyShouldSaveCredential(bool);
    virtual void syncProxyCredential(const WebCore::Credential&);

    // Called from within WebKit via ChromeClientBlackBerry.
    void enterFullscreenForNode(WebCore::Node*);
    void exitFullscreenForNode(WebCore::Node*);
#if ENABLE(FULLSCREEN_API)
    void enterFullScreenForElement(WebCore::Element*);
    void exitFullScreenForElement(WebCore::Element*);
    void adjustFullScreenElementDimensionsIfNeeded();
#endif
    void contentsSizeChanged(const WebCore::IntSize&);
    void overflowExceedsContentsSize();
    void layoutFinished();
    void setNeedTouchEvents(bool);
    void notifyPopupAutofillDialog(const Vector<String>&);
    void notifyDismissAutofillDialog();

    bool shouldZoomToInitialScaleOnLoad() const { return loadState() == Committed || m_shouldZoomToInitialScaleAfterLoadFinished; }
    void setShouldZoomToInitialScaleAfterLoadFinished(bool shouldZoomToInitialScaleAfterLoadFinished)
    {
        m_shouldZoomToInitialScaleAfterLoadFinished = shouldZoomToInitialScaleAfterLoadFinished;
    }

    // Called according to our heuristic or from setLoadState depending on whether we have a virtual viewport.
    void zoomToInitialScaleOnLoad();

    // Various scale factors.
    double currentScale() const { return m_transformationMatrix->m11(); }
    double zoomToFitScale() const;
    bool respectViewport() const;
    double initialScale() const;
    void setInitialScale(double scale) { m_initialScale = scale; }
    double minimumScale() const
    {
        return (m_minimumScale > zoomToFitScale() && m_minimumScale <= maximumScale() && respectViewport()) ? m_minimumScale : zoomToFitScale();
    }

    void setMinimumScale(double scale) { m_minimumScale = scale; }
    double maximumScale() const;
    void setMaximumScale(double scale) { m_maximumScale = scale; }
    void resetScales();

    // Note: to make this reflow width transform invariant just use
    // transformedActualVisibleSize() here instead!
    int reflowWidth() const { return actualVisibleSize().width(); }

    // These methods give the real geometry of the device given the currently set transform.
    WebCore::IntPoint transformedScrollPosition() const;
    WebCore::IntPoint transformedMaximumScrollPosition() const;
    WebCore::IntSize transformedActualVisibleSize() const;
    WebCore::IntSize transformedViewportSize() const;

    // Notification methods that deliver changes to the real geometry of the device as specified above.
    void notifyTransformChanged();
    void notifyTransformedContentsSizeChanged();
    void notifyTransformedScrollChanged();

    void assignFocus(Platform::FocusDirection);
    Platform::IntRect focusNodeRect();
    WebCore::IntRect getRecursiveVisibleWindowRect(WebCore::ScrollView*, bool noClipOfMainFrame = false);

    WebCore::IntPoint frameOffset(const WebCore::Frame*) const;

    WebCore::Node* bestNodeForZoomUnderPoint(const WebCore::IntPoint&);
    WebCore::Node* bestChildNodeForClickRect(WebCore::Node* parentNode, const WebCore::IntRect& clickRect);
    WebCore::Node* nodeForZoomUnderPoint(const WebCore::IntPoint&);
    WebCore::Node* adjustedBlockZoomNodeForZoomAndExpandingRatioLimits(WebCore::Node*);
    WebCore::IntRect rectForNode(WebCore::Node*);
    WebCore::IntRect blockZoomRectForNode(WebCore::Node*);
    WebCore::IntRect adjustRectOffsetForFrameOffset(const WebCore::IntRect&, const WebCore::Node*);
    bool compareNodesForBlockZoom(WebCore::Node* n1, WebCore::Node* n2);
    double newScaleForBlockZoomRect(const WebCore::IntRect&, double oldScale, double margin);
    double maxBlockZoomScale() const;

    // Context Methods.
    Platform::WebContext webContext(TargetDetectionStrategy);
    PassRefPtr<WebCore::Node> contextNode(TargetDetectionStrategy);

#if ENABLE(VIEWPORT_REFLOW)
    void toggleTextReflowIfEnabledForBlockZoomOnly(bool shouldEnableTextReflow = false);
#endif

    void selectionChanged(WebCore::Frame*);
    void setOverlayExpansionPixelHeight(int);
    void updateSelectionScrollView(const WebCore::Node*);

    void updateDelegatedOverlays(bool dispatched = false);

    void updateCursor();

    ViewMode viewMode() const { return m_viewMode; }
    bool setViewMode(ViewMode); // Returns true if the change requires re-layout.

    void setShouldUseFixedDesktopMode(bool b) { m_shouldUseFixedDesktopMode = b; }

    bool useFixedLayout() const;
    WebCore::IntSize fixedLayoutSize(bool snapToIncrement = false) const;

    // ZoomToFitOnLoad can lead to a large recursion depth in FrameView::layout() as we attempt
    // to determine the zoom scale factor so as to have the content of the page fit within the
    // area of the frame. From observation, we can bail out after a recursion depth of 10 and
    // still have reasonable results.
    bool didLayoutExceedMaximumIterations() const { return m_nestedLayoutFinishedCount > 10; }

    void clearFocusNode();
    WebCore::Frame* focusedOrMainFrame() const;
    WebCore::Frame* mainFrame() const { return m_mainFrame; }

#if ENABLE(EVENT_MODE_METATAGS)
    void didReceiveCursorEventMode(WebCore::CursorEventMode);
    void didReceiveTouchEventMode(WebCore::TouchEventMode);
#endif

    void dispatchViewportPropertiesDidChange(const WebCore::ViewportArguments&);
    Platform::IntSize recomputeVirtualViewportFromViewportArguments();

    void resetBlockZoom();

    void zoomAboutPointTimerFired(WebCore::Timer<WebPagePrivate>*);
    bool shouldSendResizeEvent();
    void scrollEventTimerFired(WebCore::Timer<WebPagePrivate>*);
    void resizeEventTimerFired(WebCore::Timer<WebPagePrivate>*);

    // If this url should be handled as a pattern, returns the pattern
    // otherwise, returns an empty string.
    String findPatternStringForUrl(const WebCore::KURL&) const;

    void suspendBackingStore();
    void resumeBackingStore();

    void setShouldResetTilesWhenShown(bool flag) { m_shouldResetTilesWhenShown = flag; }
    bool shouldResetTilesWhenShown() const { return m_shouldResetTilesWhenShown; }

    void setScreenOrientation(int);

    // Scroll and/or zoom so that the WebPage fits the new actual visible size, a.k.a. visual viewport.
    // Also sets the default layout size, a.k.a. the layout viewport.
    bool setViewportSize(const WebCore::IntSize& transformedActualVisibleSize, const WebCore::IntSize& defaultLayoutSize, bool ensureFocusElementVisible);

    void scheduleDeferrableTimer(WebCore::Timer<WebPagePrivate>*, double timeOut);
    void unscheduleAllDeferrableTimers();
    void willDeferLoading();
    void didResumeLoading();

    WebCore::TransformationMatrix* transformationMatrix() const
    {
        return m_transformationMatrix;
    }

    bool compositorDrawsRootLayer() const; // Thread safe
    void setCompositorDrawsRootLayer(bool); // WebKit thread only

#if USE(ACCELERATED_COMPOSITING)
    // WebKit thread.
    bool needsOneShotDrawingSynchronization();
    void rootLayerCommitTimerFired(WebCore::Timer<WebPagePrivate>*);
    bool commitRootLayerIfNeeded();
    WebCore::LayerRenderingResults lastCompositingResults() const;
    WebCore::GraphicsLayer* overlayLayer();

    // Fallback GraphicsLayerClient implementation, used for various overlay layers.
    virtual void notifyAnimationStarted(const WebCore::GraphicsLayer*, double) { }
    virtual void notifyFlushRequired(const WebCore::GraphicsLayer*);
    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect&) { }

    // WebKit thread, plumbed through from ChromeClientBlackBerry.
    void setRootLayerWebKitThread(WebCore::Frame*, WebCore::LayerWebKitThread*);
    void setNeedsOneShotDrawingSynchronization();
    void scheduleRootLayerCommit();

    // Compositing thread.
    void setRootLayerCompositingThread(WebCore::LayerCompositingThread*);
    void commitRootLayer(const WebCore::IntRect& layoutRect, const WebCore::IntRect& documentRect, bool);
    bool isAcceleratedCompositingActive() const { return m_compositor; }
    WebPageCompositorPrivate* compositor() const { return m_compositor.get(); }
    void setCompositor(PassRefPtr<WebPageCompositorPrivate>);
    void setCompositorHelper(PassRefPtr<WebPageCompositorPrivate>);
    void setCompositorBackgroundColor(const WebCore::Color&);
    bool createCompositor();
    void destroyCompositor();
    void syncDestroyCompositorOnCompositingThread();
    void releaseLayerResources();
    void releaseLayerResourcesCompositingThread();
    void updateRootLayerCommitEnabled();

    void scheduleCompositingRun();
#endif

    bool dispatchTouchEventToFullScreenPlugin(WebCore::PluginView*, const Platform::TouchEvent&);
    bool dispatchTouchPointAsMouseEventToFullScreenPlugin(WebCore::PluginView*, const Platform::TouchPoint&);
    bool dispatchMouseEventToFullScreenPlugin(WebCore::PluginView*, const Platform::MouseEvent&);

    BackingStoreClient* backingStoreClient() const;

    bool openPagePopup(PagePopupClient*, const WebCore::IntRect& originBoundsInRootView);
    void closePagePopup();
    bool hasOpenedPopup() const;

    // Clean up any document related data we might be holding.
    void clearDocumentData(const WebCore::Document*);

    void frameUnloaded(const WebCore::Frame*);

    static WebCore::RenderLayer* enclosingPositionedAncestorOrSelfIfPositioned(WebCore::RenderLayer*);
    static WebCore::RenderLayer* enclosingFixedPositionedAncestorOrSelfIfFixedPositioned(WebCore::RenderLayer*);

    static const BlackBerry::Platform::String& defaultUserAgent();

    void setVisible(bool);
#if ENABLE(PAGE_VISIBILITY_API)
    void setPageVisibilityState();
#endif
    void notifyAppActivationStateChange(ActivationStateType);

    void deferredTasksTimerFired(WebCore::Timer<WebPagePrivate>*);

    void setInspectorOverlayClient(InspectorOverlay::InspectorOverlayClient*);

    void applySizeOverride(int overrideWidth, int overrideHeight);
    void setTextZoomFactor(float);

    void postponeDocumentStyleRecalc();
    void resumeDocumentStyleRecalc();

    const WebCore::HitTestResult& hitTestResult(const WebCore::IntPoint& contentPos);
    void clearCachedHitTestResult();

    WebCore::IntSize screenSize() const;

    void willComposite();
    void didComposite();

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    NotificationManager& notificationManager() { return m_notificationManager; };
#endif

    void animateToScaleAndDocumentScrollPosition(double destinationZoomScale, const WebCore::FloatPoint& destinationScrollPosition, bool shouldConstrainScrollingToContentEdge = true);

    void updateBackgroundColor(const WebCore::Color& backgroundColor);
    WebCore::Color documentBackgroundColor() const;

#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
    // BlackBerry::Platform::AnimationFrameRateClient.
    virtual void animationFrameChanged();
    void scheduleAnimation();
    void startRefreshAnimationClient();
    void stopRefreshAnimationClient();
    void serviceAnimations();
    static void handleServiceScriptedAnimationsOnMainThread(void*);
#endif

    WebPage* m_webPage;
    WebPageClient* m_client;
    WebCore::InspectorClientBlackBerry* m_inspectorClient;
    WebCore::Page* m_page;
    WebCore::Frame* m_mainFrame;
    RefPtr<WebCore::Node> m_currentContextNode;
    WebSettings* m_webSettings;
    WebCookieJar* m_cookieJar;
    OwnPtr<WebTapHighlight> m_tapHighlight;
    OwnPtr<WebTapHighlight> m_selectionHighlight;
    OwnPtr<SelectionOverlay> m_selectionOverlay;
#if ENABLE(NAVIGATOR_CONTENT_UTILS)
    OwnPtr<WebCore::NavigatorContentUtilsClientBlackBerry> m_navigatorContentUtilsClient;
#endif

    bool m_visible;
    ActivationStateType m_activationState;
    bool m_shouldResetTilesWhenShown;
    bool m_shouldZoomToInitialScaleAfterLoadFinished;
    bool m_userScalable;
    bool m_userPerformedManualZoom;
    bool m_userPerformedManualScroll;
    bool m_contentsSizeChanged;
    bool m_overflowExceedsContentsSize;
    bool m_resetVirtualViewportOnCommitted;
    bool m_shouldUseFixedDesktopMode;
    bool m_inspectorEnabled;
    int m_preventIdleDimmingCount;

#if ENABLE(TOUCH_EVENTS)
    bool m_preventDefaultOnTouchStart;
#endif
    unsigned m_nestedLayoutFinishedCount;
    WebCore::IntSize m_previousContentsSize;
    int m_actualVisibleWidth;
    int m_actualVisibleHeight;
    WebCore::IntSize m_virtualViewportSize;
    WebCore::IntSize m_defaultLayoutSize;
    WebCore::ViewportArguments m_viewportArguments; // We keep this around since we may need to re-evaluate the arguments on rotation.
    WebCore::ViewportArguments m_userViewportArguments; // A fallback set of Viewport Arguments supplied by the WebPageClient
    bool m_didRestoreFromPageCache;
    ViewMode m_viewMode;
    LoadState m_loadState;
    WebCore::TransformationMatrix* m_transformationMatrix;
    BackingStore* m_backingStore;
    BackingStoreClient* m_backingStoreClient;
    WebKitThreadViewportAccessor* m_webkitThreadViewportAccessor;
    InPageSearchManager* m_inPageSearchManager;
    InputHandler* m_inputHandler;
    SelectionHandler* m_selectionHandler;
    TouchEventHandler* m_touchEventHandler;
    ProximityDetector* m_proximityDetector;

#if ENABLE(EVENT_MODE_METATAGS)
    WebCore::CursorEventMode m_cursorEventMode;
    WebCore::TouchEventMode m_touchEventMode;
#endif

#if ENABLE(FULLSCREEN_API)
#if ENABLE(VIDEO)
    double m_scaleBeforeFullScreen;
    WebCore::IntPoint m_scrollPositionBeforeFullScreen;
    int m_orientationBeforeFullScreen;
#endif
#endif

    Platform::BlackBerryCursor m_currentCursor;

    DumpRenderTreeClient* m_dumpRenderTree;

    double m_initialScale;
    double m_minimumScale;
    double m_maximumScale;
    bool m_forceRespectViewportArguments;

    // Block zoom & zoom/scroll animation data.
    WebCore::FloatPoint m_finalAnimationDocumentScrollPositionReflowOffset;
    RefPtr<WebCore::Node> m_currentPinchZoomNode;
    WebCore::FloatPoint m_anchorInNodeRectRatio;
    RefPtr<WebCore::Node> m_currentBlockZoomNode;
    RefPtr<WebCore::Node> m_currentBlockZoomAdjustedNode;
    bool m_shouldReflowBlock;

    double m_lastUserEventTimestamp; // Used to detect user scrolling.

    WebCore::PlatformMouseEvent m_lastMouseEvent;
    bool m_pluginMouseButtonPressed; // Used to track mouse button for full screen plugins.
    bool m_pluginMayOpenNewTab;

    WebCore::GeolocationControllerClientBlackBerry* m_geolocationClient;

    HashSet<WebCore::PluginView*> m_pluginViews;

    OwnPtr<InRegionScroller> m_inRegionScroller;

#if USE(ACCELERATED_COMPOSITING)
    bool m_isAcceleratedCompositingActive;
    OwnPtr<FrameLayers> m_frameLayers; // WebKit thread only.
    OwnPtr<WebCore::GraphicsLayer> m_overlayLayer;

    // Compositing thread only, used only when the WebKit layer created the context.
    // If the API client created the context, this will be null.
    OwnPtr<GLES2Context> m_ownedContext;

    RefPtr<WebPageCompositorPrivate> m_compositor; // Compositing thread only.
    OwnPtr<WebCore::Timer<WebPagePrivate> > m_rootLayerCommitTimer;
    bool m_needsOneShotDrawingSynchronization;
    bool m_needsCommit;
    bool m_suspendRootLayerCommit;
#endif

    int m_pendingOrientation;

    RefPtr<WebCore::Node> m_fullscreenNode;
    RefPtr<WebCore::PluginView> m_fullScreenPluginView;

    typedef HashMap<const WebCore::Frame*, BackingStoreClient*> BackingStoreClientForFrameMap;
    BackingStoreClientForFrameMap m_backingStoreClientForFrameMap;

    // WebSettingsDelegate methods.
    virtual void didChangeSettings(WebSettings*);

    RefPtr<WebCore::DOMWrapperWorld> m_isolatedWorld;
    bool m_hasInRegionScrollableAreas;
    bool m_updateDelegatedOverlaysDispatched;
    OwnPtr<InspectorOverlay> m_inspectorOverlay;

    // There is no need to initialize the following members in WebPagePrivate's constructor,
    // because they are only used by WebPageTasks and the tasks will initialize them when
    // being constructed.
    bool m_wouldPopupListSelectMultiple;
    bool m_wouldPopupListSelectSingle;
    bool m_wouldSetDateTimeInput;
    bool m_wouldSetColorInput;
    bool m_wouldCancelSelection;
    bool m_wouldLoadManualScript;
    bool m_wouldSetFocused;
    bool m_wouldSetPageVisibilityState;
    bool m_cachedFocused;
    Vector<bool> m_cachedPopupListSelecteds;
    int m_cachedPopupListSelectedIndex;
    BlackBerry::Platform::String m_cachedDateTimeInput;
    BlackBerry::Platform::String m_cachedColorInput;
    WebCore::KURL m_cachedManualScript;

    class DeferredTaskBase {
    public:
        void perform(WebPagePrivate* webPagePrivate)
        {
            if (!(webPagePrivate->*m_isActive))
                return;
            performInternal(webPagePrivate);
        }
    protected:
        DeferredTaskBase(WebPagePrivate* webPagePrivate, bool WebPagePrivate::*isActive)
            : m_isActive(isActive)
        {
            webPagePrivate->*m_isActive = true;
        }

        virtual void performInternal(WebPagePrivate*) = 0;

        bool WebPagePrivate::*m_isActive;
    };

    Vector<OwnPtr<DeferredTaskBase> > m_deferredTasks;
    WebCore::Timer<WebPagePrivate> m_deferredTasksTimer;

    // The popup that opened in this webpage
    RefPtr<PagePopup> m_pagePopup;

    RefPtr<WebCore::AutofillManager> m_autofillManager;

    bool m_documentStyleRecalcPostponed;
    bool m_documentChildNeedsStyleRecalc;

    WebCore::IntPoint m_cachedHitTestContentPos;
    WebCore::HitTestResult m_cachedHitTestResult;

    typedef HashMap<RefPtr<WebCore::Document>, ListHashSet<RefPtr<WebCore::Node> > > CachedRectHitTestResults;
    CachedRectHitTestResults m_cachedRectHitTestResults;

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    NotificationManager m_notificationManager;
#endif

    bool m_didStartAnimations;
    double m_animationStartTime;

#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
    Mutex m_animationMutex;
    bool m_isRunningRefreshAnimationClient;
    bool m_animationScheduled;
    bool m_previousFrameDone;
    double m_monotonicAnimationStartTime;
#endif

protected:
    virtual ~WebPagePrivate();
};
}
}

#endif // WebPage_p_h
