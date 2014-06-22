/*
 * Copyright (C) 2010, 2011, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
#include "WebPage.h"

#include "Arguments.h"
#include "DataReference.h"
#include "DecoderAdapter.h"
#include "DrawingArea.h"
#include "DrawingAreaMessages.h"
#include "InjectedBundle.h"
#include "InjectedBundleBackForwardList.h"
#include "InjectedBundleUserMessageCoders.h"
#include "LayerTreeHost.h"
#include "NetscapePlugin.h"
#include "NotificationPermissionRequestManager.h"
#include "PageBanner.h"
#include "PageOverlay.h"
#include "PluginProcessAttributes.h"
#include "PluginProxy.h"
#include "PluginView.h"
#include "PrintInfo.h"
#include "SessionState.h"
#include "ShareableBitmap.h"
#include "WebAlternativeTextClient.h"
#include "WebBackForwardList.h"
#include "WebBackForwardListItem.h"
#include "WebBackForwardListProxy.h"
#include "WebChromeClient.h"
#include "WebColorChooser.h"
#include "WebContextMenu.h"
#include "WebContextMenuClient.h"
#include "WebContextMessages.h"
#include "WebCoreArgumentCoders.h"
#include "WebDragClient.h"
#include "WebEditorClient.h"
#include "WebEvent.h"
#include "WebEventConversion.h"
#include "WebFrame.h"
#include "WebFullScreenManager.h"
#include "WebFullScreenManagerMessages.h"
#include "WebGeolocationClient.h"
#include "WebGeometry.h"
#include "WebImage.h"
#include "WebInspector.h"
#include "WebInspectorClient.h"
#include "WebInspectorMessages.h"
#include "WebNotificationClient.h"
#include "WebOpenPanelResultListener.h"
#include "WebPageCreationParameters.h"
#include "WebPageGroupProxy.h"
#include "WebPageMessages.h"
#include "WebPageProxyMessages.h"
#include "WebPlugInClient.h"
#include "WebPopupMenu.h"
#include "WebPreferencesStore.h"
#include "WebProcess.h"
#include "WebProcessProxyMessages.h"
#include <JavaScriptCore/APICast.h>
#include <WebCore/ArchiveResource.h>
#include <WebCore/Chrome.h>
#include <WebCore/ContextMenuController.h>
#include <WebCore/DatabaseManager.h>
#include <WebCore/DocumentFragment.h>
#include <WebCore/DocumentLoader.h>
#include <WebCore/DocumentMarkerController.h>
#include <WebCore/DragController.h>
#include <WebCore/DragData.h>
#include <WebCore/DragSession.h>
#include <WebCore/EventHandler.h>
#include <WebCore/FocusController.h>
#include <WebCore/FormState.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoadRequest.h>
#include <WebCore/FrameLoaderTypes.h>
#include <WebCore/FrameView.h>
#include <WebCore/HTMLFormElement.h>
#include <WebCore/HTMLInputElement.h>
#include <WebCore/HTMLPlugInElement.h>
#include <WebCore/HTMLPlugInImageElement.h>
#include <WebCore/HistoryController.h>
#include <WebCore/HistoryItem.h>
#include <WebCore/JSDOMWindow.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/MIMETypeRegistry.h>
#include <WebCore/MouseEvent.h>
#include <WebCore/Page.h>
#include <WebCore/PlatformKeyboardEvent.h>
#include <WebCore/PluginDocument.h>
#include <WebCore/PrintContext.h>
#include <WebCore/Range.h>
#include <WebCore/RenderLayer.h>
#include <WebCore/RenderTreeAsText.h>
#include <WebCore/RenderView.h>
#include <WebCore/ResourceBuffer.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>
#include <WebCore/RunLoop.h>
#include <WebCore/RuntimeEnabledFeatures.h>
#include <WebCore/SchemeRegistry.h>
#include <WebCore/ScriptController.h>
#include <WebCore/ScriptValue.h>
#include <WebCore/SerializedScriptValue.h>
#include <WebCore/Settings.h>
#include <WebCore/SharedBuffer.h>
#include <WebCore/SubstituteData.h>
#include <WebCore/TextIterator.h>
#include <WebCore/VisiblePosition.h>
#include <WebCore/markup.h>
#include <runtime/JSCJSValue.h>
#include <runtime/JSLock.h>
#include <runtime/Operations.h>

#if ENABLE(MHTML)
#include <WebCore/MHTMLArchive.h>
#endif

#if ENABLE(PLUGIN_PROCESS)
#if PLATFORM(MAC)
#include "MachPort.h"
#endif
#endif

#if ENABLE(BATTERY_STATUS)
#include "WebBatteryClient.h"
#endif

#if ENABLE(NETWORK_INFO)
#include "WebNetworkInfoClient.h"
#endif

#if ENABLE(VIBRATION)
#include "WebVibrationClient.h"
#endif

#if ENABLE(PROXIMITY_EVENTS)
#include "WebDeviceProximityClient.h"
#endif

#if PLATFORM(MAC)
#include "SimplePDFPlugin.h"
#if ENABLE(PDFKIT_PLUGIN)
#include "PDFPlugin.h"
#endif
#include <WebCore/LegacyWebArchive.h>
#endif

#if PLATFORM(QT)
#if ENABLE(DEVICE_ORIENTATION) && HAVE(QTSENSORS)
#include "DeviceMotionClientQt.h"
#include "DeviceOrientationClientQt.h"
#endif
#include "HitTestResult.h"
#include <QMimeData>
#endif

#if PLATFORM(GTK)
#include <gtk/gtk.h>
#include "DataObjectGtk.h"
#include "WebPrintOperationGtk.h"
#endif

#ifndef NDEBUG
#include <wtf/RefCountedLeakCounter.h>
#endif

#if USE(COORDINATED_GRAPHICS)
#include "CoordinatedLayerTreeHostMessages.h"
#endif

using namespace JSC;
using namespace WebCore;

namespace WebKit {

class SendStopResponsivenessTimer {
public:
    SendStopResponsivenessTimer(WebPage* page)
        : m_page(page)
    {
    }
    
    ~SendStopResponsivenessTimer()
    {
        m_page->send(Messages::WebPageProxy::StopResponsivenessTimer());
    }

private:
    WebPage* m_page;
};

DEFINE_DEBUG_ONLY_GLOBAL(WTF::RefCountedLeakCounter, webPageCounter, ("WebPage"));

PassRefPtr<WebPage> WebPage::create(uint64_t pageID, const WebPageCreationParameters& parameters)
{
    RefPtr<WebPage> page = adoptRef(new WebPage(pageID, parameters));

    if (page->pageGroup()->isVisibleToInjectedBundle() && WebProcess::shared().injectedBundle())
        WebProcess::shared().injectedBundle()->didCreatePage(page.get());

    return page.release();
}

WebPage::WebPage(uint64_t pageID, const WebPageCreationParameters& parameters)
    : m_viewSize(parameters.viewSize)
    , m_useFixedLayout(false)
    , m_drawsBackground(true)
    , m_drawsTransparentBackground(false)
    , m_isInRedo(false)
    , m_isClosed(false)
    , m_tabToLinks(false)
    , m_asynchronousPluginInitializationEnabled(false)
    , m_asynchronousPluginInitializationEnabledForAllPlugins(false)
    , m_artificialPluginInitializationDelayEnabled(false)
    , m_scrollingPerformanceLoggingEnabled(false)
    , m_mainFrameIsScrollable(true)
#if ENABLE(PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC)
    , m_readyToFindPrimarySnapshottedPlugin(false)
    , m_didFindPrimarySnapshottedPlugin(false)
    , m_determinePrimarySnapshottedPlugInTimer(RunLoop::main(), this, &WebPage::determinePrimarySnapshottedPlugInTimerFired)
#endif
#if PLATFORM(MAC)
    , m_pdfPluginEnabled(false)
    , m_hasCachedWindowFrame(false)
    , m_windowIsVisible(false)
    , m_layerHostingMode(parameters.layerHostingMode)
    , m_keyboardEventBeingInterpreted(0)
#elif PLATFORM(GTK)
    , m_accessibilityObject(0)
#endif
    , m_setCanStartMediaTimer(RunLoop::main(), this, &WebPage::setCanStartMediaTimerFired)
    , m_sendDidUpdateInWindowStateTimer(RunLoop::main(), this, &WebPage::didUpdateInWindowStateTimerFired)
    , m_findController(this)
#if ENABLE(TOUCH_EVENTS)
#if PLATFORM(QT)
    , m_tapHighlightController(this)
#endif
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    , m_activeColorChooser(0)
#endif
#if ENABLE(GEOLOCATION)
    , m_geolocationPermissionRequestManager(this)
#endif
    , m_pageID(pageID)
    , m_canRunBeforeUnloadConfirmPanel(parameters.canRunBeforeUnloadConfirmPanel)
    , m_canRunModal(parameters.canRunModal)
    , m_isRunningModal(false)
    , m_cachedMainFrameIsPinnedToLeftSide(false)
    , m_cachedMainFrameIsPinnedToRightSide(false)
    , m_cachedMainFrameIsPinnedToTopSide(false)
    , m_cachedMainFrameIsPinnedToBottomSide(false)
    , m_canShortCircuitHorizontalWheelEvents(false)
    , m_numWheelEventHandlers(0)
    , m_cachedPageCount(0)
#if ENABLE(CONTEXT_MENUS)
    , m_isShowingContextMenu(false)
#endif
    , m_willGoToBackForwardItemCallbackEnabled(true)
#if ENABLE(PAGE_VISIBILITY_API)
    , m_visibilityState(WebCore::PageVisibilityStateVisible)
#endif
    , m_inspectorClient(0)
    , m_backgroundColor(Color::white)
    , m_maximumRenderingSuppressionToken(0)
    , m_scrollPinningBehavior(DoNotPin)
{
    ASSERT(m_pageID);
    // FIXME: This is a non-ideal location for this Setting and
    // 4ms should be adopted project-wide now, https://bugs.webkit.org/show_bug.cgi?id=61214
    Settings::setDefaultMinDOMTimerInterval(0.004);

    Page::PageClients pageClients;
    pageClients.chromeClient = new WebChromeClient(this);
#if ENABLE(CONTEXT_MENUS)
    pageClients.contextMenuClient = new WebContextMenuClient(this);
#endif
    pageClients.editorClient = new WebEditorClient(this);
#if ENABLE(DRAG_SUPPORT)
    pageClients.dragClient = new WebDragClient(this);
#endif
    pageClients.backForwardClient = WebBackForwardListProxy::create(this);
#if ENABLE(INSPECTOR)
    m_inspectorClient = new WebInspectorClient(this);
    pageClients.inspectorClient = m_inspectorClient;
#endif
#if USE(AUTOCORRECTION_PANEL)
    pageClients.alternativeTextClient = new WebAlternativeTextClient(this);
#endif
    pageClients.plugInClient = new WebPlugInClient(this);

    m_page = adoptPtr(new Page(pageClients));

#if ENABLE(BATTERY_STATUS)
    WebCore::provideBatteryTo(m_page.get(), new WebBatteryClient(this));
#endif
#if ENABLE(GEOLOCATION)
    WebCore::provideGeolocationTo(m_page.get(), new WebGeolocationClient(this));
#endif
#if ENABLE(DEVICE_ORIENTATION) && PLATFORM(QT) && HAVE(QTSENSORS)
    WebCore::provideDeviceMotionTo(m_page.get(), new DeviceMotionClientQt);
    WebCore::provideDeviceOrientationTo(m_page.get(), new DeviceOrientationClientQt);
#endif
#if ENABLE(NETWORK_INFO)
    WebCore::provideNetworkInfoTo(m_page.get(), new WebNetworkInfoClient(this));
#endif
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    WebCore::provideNotification(m_page.get(), new WebNotificationClient(this));
#endif
#if ENABLE(VIBRATION)
    WebCore::provideVibrationTo(m_page.get(), new WebVibrationClient(this));
#endif
#if ENABLE(PROXIMITY_EVENTS)
    WebCore::provideDeviceProximityTo(m_page.get(), new WebDeviceProximityClient(this));
#endif

    m_page->setCanStartMedia(false);
    m_mayStartMediaWhenInWindow = parameters.mayStartMediaWhenInWindow;

    m_pageGroup = WebProcess::shared().webPageGroup(parameters.pageGroupData);
    m_page->setGroupName(m_pageGroup->identifier());
    m_page->setDeviceScaleFactor(parameters.deviceScaleFactor);

    m_drawingArea = DrawingArea::create(this, parameters);
    m_drawingArea->setPaintingEnabled(false);

    updatePreferences(parameters.store);
    platformInitialize();

    m_mainFrame = WebFrame::createMainFrame(this);

    setUseFixedLayout(parameters.useFixedLayout);

    setDrawsBackground(parameters.drawsBackground);
    setDrawsTransparentBackground(parameters.drawsTransparentBackground);

    setUnderlayColor(parameters.underlayColor);

    setPaginationMode(parameters.paginationMode);
    setPaginationBehavesLikeColumns(parameters.paginationBehavesLikeColumns);
    setPageLength(parameters.pageLength);
    setGapBetweenPages(parameters.gapBetweenPages);

    setMemoryCacheMessagesEnabled(parameters.areMemoryCacheClientCallsEnabled);

    setActive(parameters.isActive);
    setFocused(parameters.isFocused);

    // Page defaults to in-window, but setIsInWindow depends on it being a valid indicator of actually having been put into a window.
    if (!parameters.isInWindow)
        m_page->setIsInWindow(false);
    else
        WebProcess::shared().pageDidEnterWindow(m_pageID);

    setIsInWindow(parameters.isInWindow);

    setMinimumLayoutSize(parameters.minimumLayoutSize);
    
    setScrollPinningBehavior(parameters.scrollPinningBehavior);

    m_userAgent = parameters.userAgent;

    WebBackForwardListProxy::setHighestItemIDFromUIProcess(parameters.highestUsedBackForwardItemID);
    
    if (!parameters.sessionState.isEmpty())
        restoreSession(parameters.sessionState);

    m_drawingArea->setPaintingEnabled(true);
    
    setMediaVolume(parameters.mediaVolume);

    // We use the DidFirstLayout milestone to determine when to unfreeze the layer tree.
    m_page->addLayoutMilestones(DidFirstLayout);

    WebProcess::shared().addMessageReceiver(Messages::WebPage::messageReceiverName(), m_pageID, this);

    // FIXME: This should be done in the object constructors, and the objects themselves should be message receivers.
    WebProcess::shared().addMessageReceiver(Messages::DrawingArea::messageReceiverName(), m_pageID, this);
#if USE(COORDINATED_GRAPHICS)
    WebProcess::shared().addMessageReceiver(Messages::CoordinatedLayerTreeHost::messageReceiverName(), m_pageID, this);
#endif
#if ENABLE(INSPECTOR)
    WebProcess::shared().addMessageReceiver(Messages::WebInspector::messageReceiverName(), m_pageID, this);
#endif
#if ENABLE(FULLSCREEN_API)
    WebProcess::shared().addMessageReceiver(Messages::WebFullScreenManager::messageReceiverName(), m_pageID, this);
#endif

#ifndef NDEBUG
    webPageCounter.increment();
#endif
}

WebPage::~WebPage()
{
    if (m_backForwardList)
        m_backForwardList->detach();

    ASSERT(!m_page);

    m_sandboxExtensionTracker.invalidate();

    for (HashSet<PluginView*>::const_iterator it = m_pluginViews.begin(), end = m_pluginViews.end(); it != end; ++it)
        (*it)->webPageDestroyed();

    if (m_headerBanner)
        m_headerBanner->detachFromPage();
    if (m_footerBanner)
        m_footerBanner->detachFromPage();

    WebProcess::shared().removeMessageReceiver(Messages::WebPage::messageReceiverName(), m_pageID);

    // FIXME: This should be done in the object destructors, and the objects themselves should be message receivers.
    WebProcess::shared().removeMessageReceiver(Messages::DrawingArea::messageReceiverName(), m_pageID);
#if USE(COORDINATED_GRAPHICS)
    WebProcess::shared().removeMessageReceiver(Messages::CoordinatedLayerTreeHost::messageReceiverName(), m_pageID);
#endif
#if ENABLE(INSPECTOR)
    WebProcess::shared().removeMessageReceiver(Messages::WebInspector::messageReceiverName(), m_pageID);
#endif
#if ENABLE(FULLSCREEN_API)
    WebProcess::shared().removeMessageReceiver(Messages::WebFullScreenManager::messageReceiverName(), m_pageID);
#endif

#ifndef NDEBUG
    webPageCounter.decrement();
#endif
}

void WebPage::dummy(bool&)
{
}

CoreIPC::Connection* WebPage::messageSenderConnection()
{
    return WebProcess::shared().parentProcessConnection();
}

uint64_t WebPage::messageSenderDestinationID()
{
    return pageID();
}

#if ENABLE(CONTEXT_MENUS)
void WebPage::initializeInjectedBundleContextMenuClient(WKBundlePageContextMenuClient* client)
{
    m_contextMenuClient.initialize(client);
}
#endif

void WebPage::initializeInjectedBundleEditorClient(WKBundlePageEditorClient* client)
{
    m_editorClient.initialize(client);
}

void WebPage::initializeInjectedBundleFormClient(WKBundlePageFormClient* client)
{
    m_formClient.initialize(client);
}

void WebPage::initializeInjectedBundleLoaderClient(WKBundlePageLoaderClient* client)
{
    // It would be nice to get rid of this code and transition all clients to using didLayout instead of
    // didFirstLayoutInFrame and didFirstVisuallyNonEmptyLayoutInFrame. In the meantime, this is required
    // for backwards compatibility.
    LayoutMilestones milestones = 0;
    if (client) {
        if (client->didFirstLayoutForFrame)
            milestones |= WebCore::DidFirstLayout;
        if (client->didFirstVisuallyNonEmptyLayoutForFrame)
            milestones |= WebCore::DidFirstVisuallyNonEmptyLayout;
        if (client->didNewFirstVisuallyNonEmptyLayout)
            milestones |= WebCore::DidHitRelevantRepaintedObjectsAreaThreshold;
    }

    if (milestones)
        listenForLayoutMilestones(milestones);

    m_loaderClient.initialize(client);
}

void WebPage::initializeInjectedBundlePolicyClient(WKBundlePagePolicyClient* client)
{
    m_policyClient.initialize(client);
}

void WebPage::initializeInjectedBundleResourceLoadClient(WKBundlePageResourceLoadClient* client)
{
    m_resourceLoadClient.initialize(client);
}

void WebPage::initializeInjectedBundleUIClient(WKBundlePageUIClient* client)
{
    m_uiClient.initialize(client);
}

#if ENABLE(FULLSCREEN_API)
void WebPage::initializeInjectedBundleFullScreenClient(WKBundlePageFullScreenClient* client)
{
    m_fullScreenClient.initialize(client);
}
#endif

void WebPage::initializeInjectedBundleDiagnosticLoggingClient(WKBundlePageDiagnosticLoggingClient* client)
{
    m_logDiagnosticMessageClient.initialize(client);
}

#if ENABLE(NETSCAPE_PLUGIN_API)
PassRefPtr<Plugin> WebPage::createPlugin(WebFrame* frame, HTMLPlugInElement* pluginElement, const Plugin::Parameters& parameters, String& newMIMEType)
{
    String frameURLString = frame->coreFrame()->loader()->documentLoader()->responseURL().string();
    String pageURLString = m_page->mainFrame()->loader()->documentLoader()->responseURL().string();
    PluginProcessType processType = pluginElement->displayState() == HTMLPlugInElement::WaitingForSnapshot ? PluginProcessTypeSnapshot : PluginProcessTypeNormal;

    bool allowOnlyApplicationPlugins = !frame->coreFrame()->loader()->subframeLoader()->allowPlugins(NotAboutToInstantiatePlugin);

    uint64_t pluginProcessToken;
    uint32_t pluginLoadPolicy;
    String unavailabilityDescription;
    if (!sendSync(Messages::WebPageProxy::FindPlugin(parameters.mimeType, static_cast<uint32_t>(processType), parameters.url.string(), frameURLString, pageURLString, allowOnlyApplicationPlugins), Messages::WebPageProxy::FindPlugin::Reply(pluginProcessToken, newMIMEType, pluginLoadPolicy, unavailabilityDescription))) {
        return 0;
    }

    switch (static_cast<PluginModuleLoadPolicy>(pluginLoadPolicy)) {
    case PluginModuleLoadNormally:
    case PluginModuleLoadUnsandboxed:
        break;

    case PluginModuleBlocked:
        bool replacementObscured = false;
        if (pluginElement->renderer()->isEmbeddedObject()) {
            RenderEmbeddedObject* renderObject = toRenderEmbeddedObject(pluginElement->renderer());
            renderObject->setPluginUnavailabilityReasonWithDescription(RenderEmbeddedObject::InsecurePluginVersion, unavailabilityDescription);
            replacementObscured = renderObject->isReplacementObscured();
            renderObject->setUnavailablePluginIndicatorIsHidden(replacementObscured);
        }

        send(Messages::WebPageProxy::DidBlockInsecurePluginVersion(parameters.mimeType, parameters.url.string(), frameURLString, pageURLString, replacementObscured));
        return 0;
    }

    if (!pluginProcessToken) {
#if PLATFORM(MAC)
        String path = parameters.url.path();
        if (MIMETypeRegistry::isPDFOrPostScriptMIMEType(parameters.mimeType) || (parameters.mimeType.isEmpty() && (path.endsWith(".pdf", false) || path.endsWith(".ps", false)))) {
#if ENABLE(PDFKIT_PLUGIN)
            if (shouldUsePDFPlugin())
                return PDFPlugin::create(frame);
#endif
            return SimplePDFPlugin::create(frame);
        }
#else
        UNUSED_PARAM(frame);
#endif
        return 0;
    }

    bool isRestartedProcess = (pluginElement->displayState() == HTMLPlugInElement::Restarting || pluginElement->displayState() == HTMLPlugInElement::RestartingWithPendingMouseClick);
    return PluginProxy::create(pluginProcessToken, isRestartedProcess);
}

#endif // ENABLE(NETSCAPE_PLUGIN_API)

EditorState WebPage::editorState() const
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    ASSERT(frame);

    EditorState result;

    if (PluginView* pluginView = focusedPluginViewForFrame(frame)) {
        if (!pluginView->getSelectionString().isNull()) {
            result.selectionIsNone = false;
            result.selectionIsRange = true;
            result.isInPlugin = true;
            return result;
        }
    }

    result.selectionIsNone = frame->selection()->isNone();
    result.selectionIsRange = frame->selection()->isRange();
    result.isContentEditable = frame->selection()->isContentEditable();
    result.isContentRichlyEditable = frame->selection()->isContentRichlyEditable();
    result.isInPasswordField = frame->selection()->isInPasswordField();
    result.hasComposition = frame->editor().hasComposition();
    result.shouldIgnoreCompositionSelectionChange = frame->editor().ignoreCompositionSelectionChange();

#if PLATFORM(QT)
    size_t location = 0;
    size_t length = 0;

    Element* selectionRoot = frame->selection()->rootEditableElementRespectingShadowTree();
    Element* scope = selectionRoot ? selectionRoot : frame->document()->documentElement();

    if (!scope)
        return result;

    if (isHTMLInputElement(scope)) {
        HTMLInputElement* input = toHTMLInputElement(scope);
        if (input->isTelephoneField())
            result.inputMethodHints |= Qt::ImhDialableCharactersOnly;
        else if (input->isNumberField())
            result.inputMethodHints |= Qt::ImhDigitsOnly;
        else if (input->isEmailField()) {
            result.inputMethodHints |= Qt::ImhEmailCharactersOnly;
            result.inputMethodHints |= Qt::ImhNoAutoUppercase;
        } else if (input->isURLField()) {
            result.inputMethodHints |= Qt::ImhUrlCharactersOnly;
            result.inputMethodHints |= Qt::ImhNoAutoUppercase;
        } else if (input->isPasswordField()) {
            // Set ImhHiddenText flag for password fields. The Qt platform
            // is responsible for determining which widget will receive input
            // method events for password fields.
            result.inputMethodHints |= Qt::ImhHiddenText;
            result.inputMethodHints |= Qt::ImhNoAutoUppercase;
            result.inputMethodHints |= Qt::ImhNoPredictiveText;
            result.inputMethodHints |= Qt::ImhSensitiveData;
        }
    }

    if (selectionRoot)
        result.editorRect = frame->view()->contentsToWindow(selectionRoot->pixelSnappedBoundingBox());

    RefPtr<Range> range;
    if (result.hasComposition && (range = frame->editor().compositionRange())) {
        frame->editor().getCompositionSelection(result.anchorPosition, result.cursorPosition);

        result.compositionRect = frame->view()->contentsToWindow(range->boundingBox());
    }

    if (!result.hasComposition && !result.selectionIsNone && (range = frame->selection()->selection().firstRange())) {
        TextIterator::getLocationAndLengthFromRange(scope, range.get(), location, length);
        bool baseIsFirst = frame->selection()->selection().isBaseFirst();

        result.cursorPosition = (baseIsFirst) ? location + length : location;
        result.anchorPosition = (baseIsFirst) ? location : location + length;
        result.selectedText = range->text();
    }

    if (range) {
        result.cursorRect = frame->view()->contentsToWindow(frame->editor().firstRectForRange(range.get()));
        // Check that at least one dimension is valid
        if (result.cursorRect.width() != 0 || result.cursorRect.height() != 0)
        {
            if (result.cursorRect.width() == 0)
                result.cursorRect.setWidth(1);
            if (result.cursorRect.height() == 0)
                result.cursorRect.setHeight(1);
        }
        // now adjust the cursor coordinates to take scrolling into account
        result.cursorRect.moveBy(-frame->view()->visibleContentRect().location());
    }


    // FIXME: We should only transfer innerText when it changes and do this on the UI side.
    if (result.isContentEditable && !result.isInPasswordField) {
        result.surroundingText = scope->innerText();
        if (result.hasComposition) {
            // The anchor is always the left position when they represent a composition.
            result.surroundingText.remove(result.anchorPosition, result.cursorPosition - result.anchorPosition);
        }
    }
#endif

#if PLATFORM(GTK)
    result.cursorRect = frame->selection()->absoluteCaretBounds();
#endif

    return result;
}

String WebPage::renderTreeExternalRepresentation() const
{
    return externalRepresentation(m_mainFrame->coreFrame(), RenderAsTextBehaviorNormal);
}

String WebPage::renderTreeExternalRepresentationForPrinting() const
{
    return externalRepresentation(m_mainFrame->coreFrame(), RenderAsTextPrintingMode);
}

uint64_t WebPage::renderTreeSize() const
{
    if (!m_page)
        return 0;
    return m_page->renderTreeSize().treeSize;
}

void WebPage::setTracksRepaints(bool trackRepaints)
{
    if (FrameView* view = mainFrameView())
        view->setTracksRepaints(trackRepaints);
}

bool WebPage::isTrackingRepaints() const
{
    if (FrameView* view = mainFrameView())
        return view->isTrackingRepaints();

    return false;
}

void WebPage::resetTrackedRepaints()
{
    if (FrameView* view = mainFrameView())
        view->resetTrackedRepaints();
}

PassRefPtr<ImmutableArray> WebPage::trackedRepaintRects()
{
    FrameView* view = mainFrameView();
    if (!view)
        return ImmutableArray::create();

    const Vector<IntRect>& rects = view->trackedRepaintRects();
    size_t size = rects.size();
    if (!size)
        return ImmutableArray::create();

    Vector<RefPtr<APIObject> > vector;
    vector.reserveInitialCapacity(size);

    for (size_t i = 0; i < size; ++i)
        vector.uncheckedAppend(WebRect::create(toAPI(rects[i])));

    return ImmutableArray::adopt(vector);
}

PluginView* WebPage::focusedPluginViewForFrame(Frame* frame)
{
    if (!frame->document()->isPluginDocument())
        return 0;

    PluginDocument* pluginDocument = static_cast<PluginDocument*>(frame->document());

    if (pluginDocument->focusedElement() != pluginDocument->pluginElement())
        return 0;

    PluginView* pluginView = static_cast<PluginView*>(pluginDocument->pluginWidget());
    return pluginView;
}

PluginView* WebPage::pluginViewForFrame(Frame* frame)
{
    if (!frame->document()->isPluginDocument())
        return 0;

    PluginDocument* pluginDocument = static_cast<PluginDocument*>(frame->document());
    PluginView* pluginView = static_cast<PluginView*>(pluginDocument->pluginWidget());
    return pluginView;
}

void WebPage::executeEditingCommand(const String& commandName, const String& argument)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    if (PluginView* pluginView = focusedPluginViewForFrame(frame)) {
        pluginView->handleEditingCommand(commandName, argument);
        return;
    }
    
    frame->editor().command(commandName).execute(argument);
}

bool WebPage::isEditingCommandEnabled(const String& commandName)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return false;

    if (PluginView* pluginView = focusedPluginViewForFrame(frame))
        return pluginView->isEditingCommandEnabled(commandName);
    
    Editor::Command command = frame->editor().command(commandName);
    return command.isSupported() && command.isEnabled();
}
    
void WebPage::clearMainFrameName()
{
    if (Frame* frame = mainFrame())
        frame->tree()->clearName();
}

#if USE(ACCELERATED_COMPOSITING)
void WebPage::enterAcceleratedCompositingMode(GraphicsLayer* layer)
{
    m_drawingArea->setRootCompositingLayer(layer);
}

void WebPage::exitAcceleratedCompositingMode()
{
    m_drawingArea->setRootCompositingLayer(0);
}
#endif

void WebPage::close()
{
    if (m_isClosed)
        return;

    m_isClosed = true;

    // If there is still no URL, then we never loaded anything in this page, so nothing to report.
    if (!mainWebFrame()->url().isEmpty())
        reportUsedFeatures();

    if (pageGroup()->isVisibleToInjectedBundle() && WebProcess::shared().injectedBundle())
        WebProcess::shared().injectedBundle()->willDestroyPage(this);

#if ENABLE(INSPECTOR)
    m_inspector = 0;
#endif
#if ENABLE(FULLSCREEN_API)
    m_fullScreenManager = 0;
#endif

    if (m_activePopupMenu) {
        m_activePopupMenu->disconnectFromPage();
        m_activePopupMenu = 0;
    }

    if (m_activeOpenPanelResultListener) {
        m_activeOpenPanelResultListener->disconnectFromPage();
        m_activeOpenPanelResultListener = 0;
    }

#if ENABLE(INPUT_TYPE_COLOR)
    if (m_activeColorChooser) {
        m_activeColorChooser->disconnectFromPage();
        m_activeColorChooser = 0;
    }
#endif

    m_sandboxExtensionTracker.invalidate();

#if ENABLE(PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC)
    m_determinePrimarySnapshottedPlugInTimer.stop();
#endif

#if ENABLE(CONTEXT_MENUS)
    m_contextMenuClient.initialize(0);
#endif
    m_editorClient.initialize(0);
    m_formClient.initialize(0);
    m_loaderClient.initialize(0);
    m_policyClient.initialize(0);
    m_resourceLoadClient.initialize(0);
    m_uiClient.initialize(0);
#if ENABLE(FULLSCREEN_API)
    m_fullScreenClient.initialize(0);
#endif
    m_logDiagnosticMessageClient.initialize(0);

    m_underlayPage = nullptr;
    m_printContext = nullptr;
    m_mainFrame->coreFrame()->loader()->detachFromParent();
    m_page = nullptr;
    m_drawingArea = nullptr;

    bool isRunningModal = m_isRunningModal;
    m_isRunningModal = false;

    // The WebPage can be destroyed by this call.
    WebProcess::shared().removeWebPage(m_pageID);

    if (isRunningModal)
        RunLoop::main()->stop();
}

void WebPage::tryClose()
{
    SendStopResponsivenessTimer stopper(this);

    if (!m_mainFrame->coreFrame()->loader()->shouldClose()) {
        send(Messages::WebPageProxy::StopResponsivenessTimer());
        return;
    }

    send(Messages::WebPageProxy::ClosePage(true));
}

void WebPage::sendClose()
{
    send(Messages::WebPageProxy::ClosePage(false));
}

void WebPage::loadURL(const String& url, const SandboxExtension::Handle& sandboxExtensionHandle, CoreIPC::MessageDecoder& decoder)
{
    loadURLRequest(ResourceRequest(KURL(KURL(), url)), sandboxExtensionHandle, decoder);
}

void WebPage::loadURLRequest(const ResourceRequest& request, const SandboxExtension::Handle& sandboxExtensionHandle, CoreIPC::MessageDecoder& decoder)
{
    SendStopResponsivenessTimer stopper(this);

    RefPtr<APIObject> userData;
    InjectedBundleUserMessageDecoder userMessageDecoder(userData);
    if (!decoder.decode(userMessageDecoder))
        return;

    m_sandboxExtensionTracker.beginLoad(m_mainFrame.get(), sandboxExtensionHandle);

    // Let the InjectedBundle know we are about to start the load, passing the user data from the UIProcess
    // to all the client to set up any needed state.
    m_loaderClient.willLoadURLRequest(this, request, userData.get());

    // Initate the load in WebCore.
    m_mainFrame->coreFrame()->loader()->load(FrameLoadRequest(m_mainFrame->coreFrame(), request));
}

void WebPage::loadDataImpl(PassRefPtr<SharedBuffer> sharedBuffer, const String& MIMEType, const String& encodingName, const KURL& baseURL, const KURL& unreachableURL, CoreIPC::MessageDecoder& decoder)
{
    SendStopResponsivenessTimer stopper(this);

    RefPtr<APIObject> userData;
    InjectedBundleUserMessageDecoder userMessageDecoder(userData);
    if (!decoder.decode(userMessageDecoder))
        return;

    ResourceRequest request(baseURL);
    SubstituteData substituteData(sharedBuffer, MIMEType, encodingName, unreachableURL);

    // Let the InjectedBundle know we are about to start the load, passing the user data from the UIProcess
    // to all the client to set up any needed state.
    m_loaderClient.willLoadDataRequest(this, request, substituteData.content(), substituteData.mimeType(), substituteData.textEncoding(), substituteData.failingURL(), userData.get());

    // Initate the load in WebCore.
    m_mainFrame->coreFrame()->loader()->load(FrameLoadRequest(m_mainFrame->coreFrame(), request, substituteData));
}

void WebPage::loadData(const CoreIPC::DataReference& data, const String& MIMEType, const String& encodingName, const String& baseURLString, CoreIPC::MessageDecoder& decoder)
{
    RefPtr<SharedBuffer> sharedBuffer = SharedBuffer::create(reinterpret_cast<const char*>(data.data()), data.size());
    KURL baseURL = baseURLString.isEmpty() ? blankURL() : KURL(KURL(), baseURLString);
    loadDataImpl(sharedBuffer, MIMEType, encodingName, baseURL, KURL(), decoder);
}

void WebPage::loadHTMLString(const String& htmlString, const String& baseURLString, CoreIPC::MessageDecoder& decoder)
{
    RefPtr<SharedBuffer> sharedBuffer = SharedBuffer::create(reinterpret_cast<const char*>(htmlString.characters()), htmlString.length() * sizeof(UChar));
    KURL baseURL = baseURLString.isEmpty() ? blankURL() : KURL(KURL(), baseURLString);
    loadDataImpl(sharedBuffer, "text/html", "utf-16", baseURL, KURL(), decoder);
}

void WebPage::loadAlternateHTMLString(const String& htmlString, const String& baseURLString, const String& unreachableURLString, CoreIPC::MessageDecoder& decoder)
{
    RefPtr<SharedBuffer> sharedBuffer = SharedBuffer::create(reinterpret_cast<const char*>(htmlString.characters()), htmlString.length() * sizeof(UChar));
    KURL baseURL = baseURLString.isEmpty() ? blankURL() : KURL(KURL(), baseURLString);
    KURL unreachableURL = unreachableURLString.isEmpty() ? KURL() : KURL(KURL(), unreachableURLString);
    loadDataImpl(sharedBuffer, "text/html", "utf-16", baseURL, unreachableURL, decoder);
}

void WebPage::loadPlainTextString(const String& string, CoreIPC::MessageDecoder& decoder)
{
    RefPtr<SharedBuffer> sharedBuffer = SharedBuffer::create(reinterpret_cast<const char*>(string.characters()), string.length() * sizeof(UChar));
    loadDataImpl(sharedBuffer, "text/plain", "utf-16", blankURL(), KURL(), decoder);
}

void WebPage::loadWebArchiveData(const CoreIPC::DataReference& webArchiveData, CoreIPC::MessageDecoder& decoder)
{
    RefPtr<SharedBuffer> sharedBuffer = SharedBuffer::create(reinterpret_cast<const char*>(webArchiveData.data()), webArchiveData.size() * sizeof(uint8_t));
    loadDataImpl(sharedBuffer, "application/x-webarchive", "utf-16", blankURL(), KURL(), decoder);
}

void WebPage::linkClicked(const String& url, const WebMouseEvent& event)
{
    Frame* frame = m_page->mainFrame();
    if (!frame)
        return;

    RefPtr<Event> coreEvent;
    if (event.type() != WebEvent::NoType)
        coreEvent = MouseEvent::create(eventNames().clickEvent, frame->document()->defaultView(), platform(event), 0, 0);

    frame->loader()->loadFrameRequest(FrameLoadRequest(frame, ResourceRequest(url)), false, false, coreEvent.get(), 0, MaybeSendReferrer);
}

void WebPage::stopLoadingFrame(uint64_t frameID)
{
    WebFrame* frame = WebProcess::shared().webFrame(frameID);
    if (!frame)
        return;

    frame->coreFrame()->loader()->stopForUserCancel();
}

void WebPage::stopLoading()
{
    SendStopResponsivenessTimer stopper(this);

    m_mainFrame->coreFrame()->loader()->stopForUserCancel();
}

void WebPage::setDefersLoading(bool defersLoading)
{
    m_page->setDefersLoading(defersLoading);
}

void WebPage::reload(bool reloadFromOrigin, const SandboxExtension::Handle& sandboxExtensionHandle)
{
    SendStopResponsivenessTimer stopper(this);

    m_sandboxExtensionTracker.beginLoad(m_mainFrame.get(), sandboxExtensionHandle);
    m_mainFrame->coreFrame()->loader()->reload(reloadFromOrigin);
}

void WebPage::goForward(uint64_t backForwardItemID)
{
    SendStopResponsivenessTimer stopper(this);

    HistoryItem* item = WebBackForwardListProxy::itemForID(backForwardItemID);
    ASSERT(item);
    if (!item)
        return;

    m_page->goToItem(item, FrameLoadTypeForward);
}

void WebPage::goBack(uint64_t backForwardItemID)
{
    SendStopResponsivenessTimer stopper(this);

    HistoryItem* item = WebBackForwardListProxy::itemForID(backForwardItemID);
    ASSERT(item);
    if (!item)
        return;

    m_page->goToItem(item, FrameLoadTypeBack);
}

void WebPage::goToBackForwardItem(uint64_t backForwardItemID)
{
    SendStopResponsivenessTimer stopper(this);

    HistoryItem* item = WebBackForwardListProxy::itemForID(backForwardItemID);
    ASSERT(item);
    if (!item)
        return;

    m_page->goToItem(item, FrameLoadTypeIndexedBackForward);
}

void WebPage::tryRestoreScrollPosition()
{
    m_page->mainFrame()->loader()->history()->restoreScrollPositionAndViewState();
}

void WebPage::layoutIfNeeded()
{
    if (m_mainFrame->coreFrame()->view())
        m_mainFrame->coreFrame()->view()->updateLayoutAndStyleIfNeededRecursive();

    if (m_underlayPage) {
        if (FrameView *frameView = m_underlayPage->mainFrameView())
            frameView->updateLayoutAndStyleIfNeededRecursive();
    }
}

WebPage* WebPage::fromCorePage(Page* page)
{
    return static_cast<WebChromeClient*>(page->chrome().client())->page();
}

void WebPage::setSize(const WebCore::IntSize& viewSize)
{
    FrameView* view = m_page->mainFrame()->view();

    if (m_viewSize == viewSize)
        return;

    view->resize(viewSize);
    view->setNeedsLayout();
    m_drawingArea->setNeedsDisplay();
    
    m_viewSize = viewSize;

#if USE(TILED_BACKING_STORE)
    if (view->useFixedLayout())
        sendViewportAttributesChanged();
#endif
}

#if USE(TILED_BACKING_STORE)
void WebPage::setFixedVisibleContentRect(const IntRect& rect)
{
    ASSERT(m_useFixedLayout);

    m_page->mainFrame()->view()->setFixedVisibleContentRect(rect);
}

void WebPage::sendViewportAttributesChanged()
{
    ASSERT(m_useFixedLayout);

    // Viewport properties have no impact on zero sized fixed viewports.
    if (m_viewSize.isEmpty())
        return;

    // Recalculate the recommended layout size, when the available size (device pixel) changes.
    Settings* settings = m_page->settings();

    int minimumLayoutFallbackWidth = std::max(settings->layoutFallbackWidth(), m_viewSize.width());

    // If unset  we use the viewport dimensions. This fits with the behavior of desktop browsers.
    int deviceWidth = (settings->deviceWidth() > 0) ? settings->deviceWidth() : m_viewSize.width();
    int deviceHeight = (settings->deviceHeight() > 0) ? settings->deviceHeight() : m_viewSize.height();

    ViewportAttributes attr = computeViewportAttributes(m_page->viewportArguments(), minimumLayoutFallbackWidth, deviceWidth, deviceHeight, 1, m_viewSize);

    FrameView* view = m_page->mainFrame()->view();

    // If no layout was done yet set contentFixedOrigin to (0,0).
    IntPoint contentFixedOrigin = view->didFirstLayout() ? view->fixedVisibleContentRect().location() : IntPoint();

    // Put the width and height to the viewport width and height. In css units however.
    // Use FloatSize to avoid truncated values during scale.
    FloatSize contentFixedSize = m_viewSize;

#if ENABLE(CSS_DEVICE_ADAPTATION)
    // CSS viewport descriptors might be applied to already affected viewport size
    // if the page enables/disables stylesheets, so need to keep initial viewport size.
    view->setInitialViewportSize(roundedIntSize(contentFixedSize));
#endif

    contentFixedSize.scale(1 / attr.initialScale);
    setFixedVisibleContentRect(IntRect(contentFixedOrigin, roundedIntSize(contentFixedSize)));

    attr.initialScale = m_page->viewportArguments().zoom; // Resets auto (-1) if no value was set by user.

    // This also takes care of the relayout.
    setFixedLayoutSize(roundedIntSize(attr.layoutSize));

    send(Messages::WebPageProxy::DidChangeViewportProperties(attr));
}
#endif

void WebPage::scrollMainFrameIfNotAtMaxScrollPosition(const IntSize& scrollOffset)
{
    Frame* frame = m_page->mainFrame();

    IntPoint scrollPosition = frame->view()->scrollPosition();
    IntPoint maximumScrollPosition = frame->view()->maximumScrollPosition();

    // If the current scroll position in a direction is the max scroll position 
    // we don't want to scroll at all.
    IntSize newScrollOffset;
    if (scrollPosition.x() < maximumScrollPosition.x())
        newScrollOffset.setWidth(scrollOffset.width());
    if (scrollPosition.y() < maximumScrollPosition.y())
        newScrollOffset.setHeight(scrollOffset.height());

    if (newScrollOffset.isZero())
        return;

    frame->view()->setScrollPosition(frame->view()->scrollPosition() + newScrollOffset);
}

void WebPage::drawRect(GraphicsContext& graphicsContext, const IntRect& rect)
{
    GraphicsContextStateSaver stateSaver(graphicsContext);
    graphicsContext.clip(rect);

    if (m_underlayPage) {
        m_underlayPage->drawRect(graphicsContext, rect);

        graphicsContext.beginTransparencyLayer(1);
        m_mainFrame->coreFrame()->view()->paint(&graphicsContext, rect);
        graphicsContext.endTransparencyLayer();
        return;
    }

    m_mainFrame->coreFrame()->view()->paint(&graphicsContext, rect);
}

void WebPage::drawPageOverlay(PageOverlay* pageOverlay, GraphicsContext& graphicsContext, const IntRect& rect)
{
    ASSERT(pageOverlay);

    GraphicsContextStateSaver stateSaver(graphicsContext);
    graphicsContext.clip(rect);
    pageOverlay->drawRect(graphicsContext, rect);
}

double WebPage::textZoomFactor() const
{
    Frame* frame = m_mainFrame->coreFrame();
    if (!frame)
        return 1;
    return frame->textZoomFactor();
}

void WebPage::setTextZoomFactor(double zoomFactor)
{
    PluginView* pluginView = pluginViewForFrame(m_page->mainFrame());
    if (pluginView && pluginView->handlesPageScaleFactor())
        return;

    Frame* frame = m_mainFrame->coreFrame();
    if (!frame)
        return;
    frame->setTextZoomFactor(static_cast<float>(zoomFactor));
}

double WebPage::pageZoomFactor() const
{
    PluginView* pluginView = pluginViewForFrame(m_page->mainFrame());
    if (pluginView && pluginView->handlesPageScaleFactor())
        return pluginView->pageScaleFactor();

    Frame* frame = m_mainFrame->coreFrame();
    if (!frame)
        return 1;
    return frame->pageZoomFactor();
}

void WebPage::setPageZoomFactor(double zoomFactor)
{
    PluginView* pluginView = pluginViewForFrame(m_page->mainFrame());
    if (pluginView && pluginView->handlesPageScaleFactor()) {
        pluginView->setPageScaleFactor(zoomFactor, IntPoint());
        return;
    }

    Frame* frame = m_mainFrame->coreFrame();
    if (!frame)
        return;
    frame->setPageZoomFactor(static_cast<float>(zoomFactor));
}

void WebPage::setPageAndTextZoomFactors(double pageZoomFactor, double textZoomFactor)
{
    PluginView* pluginView = pluginViewForFrame(m_page->mainFrame());
    if (pluginView && pluginView->handlesPageScaleFactor()) {
        pluginView->setPageScaleFactor(pageZoomFactor, IntPoint());
        return;
    }

    Frame* frame = m_mainFrame->coreFrame();
    if (!frame)
        return;
    return frame->setPageAndTextZoomFactors(static_cast<float>(pageZoomFactor), static_cast<float>(textZoomFactor));
}

void WebPage::windowScreenDidChange(uint64_t displayID)
{
    m_page->chrome().windowScreenDidChange(static_cast<PlatformDisplayID>(displayID));
}

void WebPage::scalePage(double scale, const IntPoint& origin)
{
    PluginView* pluginView = pluginViewForFrame(m_page->mainFrame());
    if (pluginView && pluginView->handlesPageScaleFactor()) {
        pluginView->setPageScaleFactor(scale, origin);
        return;
    }

    m_page->setPageScaleFactor(scale, origin);

    for (HashSet<PluginView*>::const_iterator it = m_pluginViews.begin(), end = m_pluginViews.end(); it != end; ++it)
        (*it)->pageScaleFactorDidChange();

    if (m_drawingArea->layerTreeHost())
        m_drawingArea->layerTreeHost()->deviceOrPageScaleFactorChanged();

    send(Messages::WebPageProxy::PageScaleFactorDidChange(scale));
}

double WebPage::pageScaleFactor() const
{
    PluginView* pluginView = pluginViewForFrame(m_page->mainFrame());
    if (pluginView && pluginView->handlesPageScaleFactor())
        return pluginView->pageScaleFactor();
    
    return m_page->pageScaleFactor();
}

void WebPage::setDeviceScaleFactor(float scaleFactor)
{
    if (scaleFactor == m_page->deviceScaleFactor())
        return;

    m_page->setDeviceScaleFactor(scaleFactor);

    // Tell all our plug-in views that the device scale factor changed.
#if PLATFORM(MAC)
    for (HashSet<PluginView*>::const_iterator it = m_pluginViews.begin(), end = m_pluginViews.end(); it != end; ++it)
        (*it)->setDeviceScaleFactor(scaleFactor);

    updateHeaderAndFooterLayersForDeviceScaleChange(scaleFactor);
#endif

    if (m_findController.isShowingOverlay()) {
        // We must have updated layout to get the selection rects right.
        layoutIfNeeded();
        m_findController.deviceScaleFactorDidChange();
    }

    if (m_drawingArea->layerTreeHost())
        m_drawingArea->layerTreeHost()->deviceOrPageScaleFactorChanged();
}

float WebPage::deviceScaleFactor() const
{
    return m_page->deviceScaleFactor();
}

void WebPage::setUseFixedLayout(bool fixed)
{
    // Do not overwrite current settings if initially setting it to false.
    if (m_useFixedLayout == fixed)
        return;
    m_useFixedLayout = fixed;

    m_page->settings()->setFixedElementsLayoutRelativeToFrame(fixed);
#if USE(COORDINATED_GRAPHICS)
    m_page->settings()->setAcceleratedCompositingForFixedPositionEnabled(fixed);
    m_page->settings()->setFixedPositionCreatesStackingContext(fixed);
    m_page->settings()->setApplyPageScaleFactorInCompositor(fixed);
    m_page->settings()->setScrollingCoordinatorEnabled(fixed);
#endif

#if USE(TILED_BACKING_STORE) && ENABLE(SMOOTH_SCROLLING)
    // Delegated scrolling will be enabled when the FrameView is created if fixed layout is enabled.
    // Ensure we don't do animated scrolling in the WebProcess in that case.
    m_page->settings()->setScrollAnimatorEnabled(!fixed);
#endif

    FrameView* view = mainFrameView();
    if (!view)
        return;

#if USE(TILED_BACKING_STORE)
    view->setDelegatesScrolling(fixed);
    view->setPaintsEntireContents(fixed);
#endif
    view->setUseFixedLayout(fixed);
    if (!fixed)
        setFixedLayoutSize(IntSize());
}

void WebPage::setFixedLayoutSize(const IntSize& size)
{
    FrameView* view = mainFrameView();
    if (!view || view->fixedLayoutSize() == size)
        return;

    view->setFixedLayoutSize(size);
    // Do not force it until the first layout, this would then become our first layout prematurely.
    if (view->didFirstLayout())
        view->forceLayout();
}

void WebPage::listenForLayoutMilestones(uint32_t milestones)
{
    if (!m_page)
        return;
    m_page->addLayoutMilestones(static_cast<LayoutMilestones>(milestones));
}

void WebPage::setSuppressScrollbarAnimations(bool suppressAnimations)
{
    m_page->setShouldSuppressScrollbarAnimations(suppressAnimations);
}

void WebPage::setRubberBandsAtBottom(bool rubberBandsAtBottom)
{
    m_page->setRubberBandsAtBottom(rubberBandsAtBottom);
}

void WebPage::setRubberBandsAtTop(bool rubberBandsAtTop)
{
    m_page->setRubberBandsAtTop(rubberBandsAtTop);
}

void WebPage::setPaginationMode(uint32_t mode)
{
    Pagination pagination = m_page->pagination();
    pagination.mode = static_cast<Pagination::Mode>(mode);
    m_page->setPagination(pagination);
}

void WebPage::setPaginationBehavesLikeColumns(bool behavesLikeColumns)
{
    Pagination pagination = m_page->pagination();
    pagination.behavesLikeColumns = behavesLikeColumns;
    m_page->setPagination(pagination);
}

void WebPage::setPageLength(double pageLength)
{
    Pagination pagination = m_page->pagination();
    pagination.pageLength = pageLength;
    m_page->setPagination(pagination);
}

void WebPage::setGapBetweenPages(double gap)
{
    Pagination pagination = m_page->pagination();
    pagination.gap = gap;
    m_page->setPagination(pagination);
}

void WebPage::postInjectedBundleMessage(const String& messageName, CoreIPC::MessageDecoder& decoder)
{
    InjectedBundle* injectedBundle = WebProcess::shared().injectedBundle();
    if (!injectedBundle)
        return;

    RefPtr<APIObject> messageBody;
    InjectedBundleUserMessageDecoder messageBodyDecoder(messageBody);
    if (!decoder.decode(messageBodyDecoder))
        return;

    injectedBundle->didReceiveMessageToPage(this, messageName, messageBody.get());
}

void WebPage::installPageOverlay(PassRefPtr<PageOverlay> pageOverlay, bool shouldFadeIn)
{
    RefPtr<PageOverlay> overlay = pageOverlay;
    
    if (m_pageOverlays.contains(overlay.get()))
        return;

    m_pageOverlays.append(overlay);
    overlay->setPage(this);

    if (shouldFadeIn)
        overlay->startFadeInAnimation();

    m_drawingArea->didInstallPageOverlay(overlay.get());
}

void WebPage::uninstallPageOverlay(PageOverlay* pageOverlay, bool shouldFadeOut)
{
    size_t existingOverlayIndex = m_pageOverlays.find(pageOverlay);
    if (existingOverlayIndex == notFound)
        return;

    if (shouldFadeOut) {
        pageOverlay->startFadeOutAnimation();
        return;
    }

    pageOverlay->setPage(0);
    m_pageOverlays.remove(existingOverlayIndex);

    m_drawingArea->didUninstallPageOverlay(pageOverlay);
}

void WebPage::setHeaderPageBanner(PassRefPtr<PageBanner> pageBanner)
{
    if (m_headerBanner)
        m_headerBanner->detachFromPage();

    m_headerBanner = pageBanner;

    if (m_headerBanner)
        m_headerBanner->addToPage(PageBanner::Header, this);
}

PageBanner* WebPage::headerPageBanner()
{
    return m_headerBanner.get();
}

void WebPage::setFooterPageBanner(PassRefPtr<PageBanner> pageBanner)
{
    if (m_footerBanner)
        m_footerBanner->detachFromPage();

    m_footerBanner = pageBanner;

    if (m_footerBanner)
        m_footerBanner->addToPage(PageBanner::Footer, this);
}

PageBanner* WebPage::footerPageBanner()
{
    return m_footerBanner.get();
}

void WebPage::hidePageBanners()
{
    if (m_headerBanner)
        m_headerBanner->hide();
    if (m_footerBanner)
        m_footerBanner->hide();
}

void WebPage::showPageBanners()
{
    if (m_headerBanner)
        m_headerBanner->showIfHidden();
    if (m_footerBanner)
        m_footerBanner->showIfHidden();
}

PassRefPtr<WebImage> WebPage::scaledSnapshotWithOptions(const IntRect& rect, double scaleFactor, SnapshotOptions options)
{
    Frame* coreFrame = m_mainFrame->coreFrame();
    if (!coreFrame)
        return 0;

    FrameView* frameView = coreFrame->view();
    if (!frameView)
        return 0;

    IntSize bitmapSize = rect.size();
    float combinedScaleFactor = scaleFactor * corePage()->deviceScaleFactor();
    bitmapSize.scale(combinedScaleFactor);

    RefPtr<WebImage> snapshot = WebImage::create(bitmapSize, snapshotOptionsToImageOptions(options));
    if (!snapshot->bitmap())
        return 0;

    OwnPtr<WebCore::GraphicsContext> graphicsContext = snapshot->bitmap()->createGraphicsContext();

    graphicsContext->clearRect(IntRect(IntPoint(), bitmapSize));

    graphicsContext->applyDeviceScaleFactor(combinedScaleFactor);
    graphicsContext->translate(-rect.x(), -rect.y());

    FrameView::SelectionInSnaphot shouldPaintSelection = FrameView::IncludeSelection;
    if (options & SnapshotOptionsExcludeSelectionHighlighting)
        shouldPaintSelection = FrameView::ExcludeSelection;

    FrameView::CoordinateSpaceForSnapshot coordinateSpace = FrameView::DocumentCoordinates;
    if (options & SnapshotOptionsInViewCoordinates)
        coordinateSpace = FrameView::ViewCoordinates;

    frameView->paintContentsForSnapshot(graphicsContext.get(), rect, shouldPaintSelection, coordinateSpace);

    if (options & SnapshotOptionsPaintSelectionRectangle) {
        FloatRect selectionRectangle = m_mainFrame->coreFrame()->selection()->bounds();
        graphicsContext->setStrokeColor(Color(0xFF, 0, 0), ColorSpaceDeviceRGB);
        graphicsContext->strokeRect(selectionRectangle, 1);
    }

    return snapshot.release();
}

void WebPage::pageDidScroll()
{
    m_uiClient.pageDidScroll(this);

    send(Messages::WebPageProxy::PageDidScroll());
}

#if USE(TILED_BACKING_STORE)
void WebPage::pageDidRequestScroll(const IntPoint& point)
{
    send(Messages::WebPageProxy::PageDidRequestScroll(point));
}
#endif

#if ENABLE(CONTEXT_MENUS)
WebContextMenu* WebPage::contextMenu()
{
    if (!m_contextMenu)
        m_contextMenu = WebContextMenu::create(this);
    return m_contextMenu.get();
}

WebContextMenu* WebPage::contextMenuAtPointInWindow(const IntPoint& point)
{
    corePage()->contextMenuController()->clearContextMenu();
    
    // Simulate a mouse click to generate the correct menu.
    PlatformMouseEvent mouseEvent(point, point, RightButton, PlatformEvent::MousePressed, 1, false, false, false, false, currentTime());
    bool handled = corePage()->mainFrame()->eventHandler()->sendContextMenuEvent(mouseEvent);
    if (!handled)
        return 0;

    return contextMenu();
}
#endif

// Events 

static const WebEvent* g_currentEvent = 0;

// FIXME: WebPage::currentEvent is used by the plug-in code to avoid having to convert from DOM events back to
// WebEvents. When we get the event handling sorted out, this should go away and the Widgets should get the correct
// platform events passed to the event handler code.
const WebEvent* WebPage::currentEvent()
{
    return g_currentEvent;
}

class CurrentEvent {
public:
    explicit CurrentEvent(const WebEvent& event)
        : m_previousCurrentEvent(g_currentEvent)
    {
        g_currentEvent = &event;
    }

    ~CurrentEvent()
    {
        g_currentEvent = m_previousCurrentEvent;
    }

private:
    const WebEvent* m_previousCurrentEvent;
};

#if ENABLE(CONTEXT_MENUS)
static bool isContextClick(const PlatformMouseEvent& event)
{
    if (event.button() == WebCore::RightButton)
        return true;

#if PLATFORM(MAC)
    // FIXME: this really should be about OSX-style UI, not about the Mac port
    if (event.button() == WebCore::LeftButton && event.ctrlKey())
        return true;
#endif

    return false;
}

static bool handleContextMenuEvent(const PlatformMouseEvent& platformMouseEvent, WebPage* page)
{
    IntPoint point = page->corePage()->mainFrame()->view()->windowToContents(platformMouseEvent.position());
    HitTestResult result = page->corePage()->mainFrame()->eventHandler()->hitTestResultAtPoint(point);

    Frame* frame = page->corePage()->mainFrame();
    if (result.innerNonSharedNode())
        frame = result.innerNonSharedNode()->document()->frame();
    
    bool handled = frame->eventHandler()->sendContextMenuEvent(platformMouseEvent);
    if (handled)
        page->contextMenu()->show();

    return handled;
}
#endif

static bool handleMouseEvent(const WebMouseEvent& mouseEvent, WebPage* page, bool onlyUpdateScrollbars)
{
    Frame* frame = page->corePage()->mainFrame();
    if (!frame->view())
        return false;

    PlatformMouseEvent platformMouseEvent = platform(mouseEvent);

    switch (platformMouseEvent.type()) {
        case PlatformEvent::MousePressed: {
#if ENABLE(CONTEXT_MENUS)
            if (isContextClick(platformMouseEvent))
                page->corePage()->contextMenuController()->clearContextMenu();
#endif

            bool handled = frame->eventHandler()->handleMousePressEvent(platformMouseEvent);
#if ENABLE(CONTEXT_MENUS)
            if (isContextClick(platformMouseEvent))
                handled = handleContextMenuEvent(platformMouseEvent, page);
#endif
            return handled;
        }
        case PlatformEvent::MouseReleased:
            return frame->eventHandler()->handleMouseReleaseEvent(platformMouseEvent);

        case PlatformEvent::MouseMoved:
            if (onlyUpdateScrollbars)
                return frame->eventHandler()->passMouseMovedEventToScrollbars(platformMouseEvent);
            return frame->eventHandler()->mouseMoved(platformMouseEvent);
        default:
            ASSERT_NOT_REACHED();
            return false;
    }
}

void WebPage::mouseEvent(const WebMouseEvent& mouseEvent)
{
#if ENABLE(CONTEXT_MENUS)
    // Don't try to handle any pending mouse events if a context menu is showing.
    if (m_isShowingContextMenu) {
        send(Messages::WebPageProxy::DidReceiveEvent(static_cast<uint32_t>(mouseEvent.type()), false));
        return;
    }
#endif
    bool handled = false;
    if (m_pageOverlays.size()) {
        // Let the page overlay handle the event.
        PageOverlayList::reverse_iterator end = m_pageOverlays.rend();
        for (PageOverlayList::reverse_iterator it = m_pageOverlays.rbegin(); it != end; ++it)
            if ((handled = (*it)->mouseEvent(mouseEvent)))
                break;
    }

    if (!handled && m_headerBanner)
        handled = m_headerBanner->mouseEvent(mouseEvent);
    if (!handled && m_footerBanner)
        handled = m_footerBanner->mouseEvent(mouseEvent);

    if (!handled && canHandleUserEvents()) {
        CurrentEvent currentEvent(mouseEvent);

        // We need to do a full, normal hit test during this mouse event if the page is active or if a mouse
        // button is currently pressed. It is possible that neither of those things will be true since on
        // Lion when legacy scrollbars are enabled, WebKit receives mouse events all the time. If it is one
        // of those cases where the page is not active and the mouse is not pressed, then we can fire a more
        // efficient scrollbars-only version of the event.
        bool onlyUpdateScrollbars = !(m_page->focusController()->isActive() || (mouseEvent.button() != WebMouseEvent::NoButton));
        handled = handleMouseEvent(mouseEvent, this, onlyUpdateScrollbars);
    }

    send(Messages::WebPageProxy::DidReceiveEvent(static_cast<uint32_t>(mouseEvent.type()), handled));
}

void WebPage::mouseEventSyncForTesting(const WebMouseEvent& mouseEvent, bool& handled)
{
    handled = false;

    if (m_pageOverlays.size()) {
        PageOverlayList::reverse_iterator end = m_pageOverlays.rend();
        for (PageOverlayList::reverse_iterator it = m_pageOverlays.rbegin(); it != end; ++it)
            if ((handled = (*it)->mouseEvent(mouseEvent)))
                break;
    }
    if (!handled && m_headerBanner)
        handled = m_headerBanner->mouseEvent(mouseEvent);
    if (!handled && m_footerBanner)
        handled = m_footerBanner->mouseEvent(mouseEvent);

    if (!handled) {
        CurrentEvent currentEvent(mouseEvent);

        // We need to do a full, normal hit test during this mouse event if the page is active or if a mouse
        // button is currently pressed. It is possible that neither of those things will be true since on 
        // Lion when legacy scrollbars are enabled, WebKit receives mouse events all the time. If it is one 
        // of those cases where the page is not active and the mouse is not pressed, then we can fire a more
        // efficient scrollbars-only version of the event.
        bool onlyUpdateScrollbars = !(m_page->focusController()->isActive() || (mouseEvent.button() != WebMouseEvent::NoButton));
        handled = handleMouseEvent(mouseEvent, this, onlyUpdateScrollbars);
    }
}

static bool handleWheelEvent(const WebWheelEvent& wheelEvent, Page* page)
{
    Frame* frame = page->mainFrame();
    if (!frame->view())
        return false;

    PlatformWheelEvent platformWheelEvent = platform(wheelEvent);
    return frame->eventHandler()->handleWheelEvent(platformWheelEvent);
}

void WebPage::wheelEvent(const WebWheelEvent& wheelEvent)
{
    bool handled = false;

    if (canHandleUserEvents()) {
        CurrentEvent currentEvent(wheelEvent);

        handled = handleWheelEvent(wheelEvent, m_page.get());
    }
    send(Messages::WebPageProxy::DidReceiveEvent(static_cast<uint32_t>(wheelEvent.type()), handled));
}

void WebPage::wheelEventSyncForTesting(const WebWheelEvent& wheelEvent, bool& handled)
{
    CurrentEvent currentEvent(wheelEvent);

    handled = handleWheelEvent(wheelEvent, m_page.get());
}

static bool handleKeyEvent(const WebKeyboardEvent& keyboardEvent, Page* page)
{
    if (!page->mainFrame()->view())
        return false;

    if (keyboardEvent.type() == WebEvent::Char && keyboardEvent.isSystemKey())
        return page->focusController()->focusedOrMainFrame()->eventHandler()->handleAccessKey(platform(keyboardEvent));
    return page->focusController()->focusedOrMainFrame()->eventHandler()->keyEvent(platform(keyboardEvent));
}

void WebPage::keyEvent(const WebKeyboardEvent& keyboardEvent)
{
    bool handled = false;

    if (canHandleUserEvents()) {
        CurrentEvent currentEvent(keyboardEvent);

        handled = handleKeyEvent(keyboardEvent, m_page.get());
        // FIXME: Platform default behaviors should be performed during normal DOM event dispatch (in most cases, in default keydown event handler).
        if (!handled)
            handled = performDefaultBehaviorForKeyEvent(keyboardEvent);
    }
    send(Messages::WebPageProxy::DidReceiveEvent(static_cast<uint32_t>(keyboardEvent.type()), handled));
}

void WebPage::keyEventSyncForTesting(const WebKeyboardEvent& keyboardEvent, bool& handled)
{
    CurrentEvent currentEvent(keyboardEvent);

    handled = handleKeyEvent(keyboardEvent, m_page.get());
    if (!handled)
        handled = performDefaultBehaviorForKeyEvent(keyboardEvent);
}

#if ENABLE(GESTURE_EVENTS)
static bool handleGestureEvent(const WebGestureEvent& gestureEvent, Page* page)
{
    Frame* frame = page->mainFrame();
    if (!frame->view())
        return false;

    PlatformGestureEvent platformGestureEvent = platform(gestureEvent);
    return frame->eventHandler()->handleGestureEvent(platformGestureEvent);
}

void WebPage::gestureEvent(const WebGestureEvent& gestureEvent)
{
    bool handled = false;

    if (canHandleUserEvents()) {
        CurrentEvent currentEvent(gestureEvent);

        handled = handleGestureEvent(gestureEvent, m_page.get());
    }
    send(Messages::WebPageProxy::DidReceiveEvent(static_cast<uint32_t>(gestureEvent.type()), handled));
}
#endif

void WebPage::validateCommand(const String& commandName, uint64_t callbackID)
{
    bool isEnabled = false;
    int32_t state = 0;
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (frame) {
        if (PluginView* pluginView = focusedPluginViewForFrame(frame))
            isEnabled = pluginView->isEditingCommandEnabled(commandName);
        else {
            Editor::Command command = frame->editor().command(commandName);
            state = command.state();
            isEnabled = command.isSupported() && command.isEnabled();
        }
    }

    send(Messages::WebPageProxy::ValidateCommandCallback(commandName, isEnabled, state, callbackID));
}

void WebPage::executeEditCommand(const String& commandName)
{
    executeEditingCommand(commandName, String());
}

uint64_t WebPage::restoreSession(const SessionState& sessionState)
{
    const BackForwardListItemVector& list = sessionState.list();
    size_t size = list.size();
    uint64_t currentItemID = 0;
    for (size_t i = 0; i < size; ++i) {
        WebBackForwardListItem* webItem = list[i].get();
        DecoderAdapter decoder(webItem->backForwardData().data(), webItem->backForwardData().size());
        
        RefPtr<HistoryItem> item = HistoryItem::decodeBackForwardTree(webItem->url(), webItem->title(), webItem->originalURL(), decoder);
        if (!item) {
            LOG_ERROR("Failed to decode a HistoryItem from session state data.");
            return 0;
        }
        
        if (i == sessionState.currentIndex())
            currentItemID = webItem->itemID();
        
        WebBackForwardListProxy::addItemFromUIProcess(list[i]->itemID(), item.release());
    }    
    ASSERT(currentItemID);
    return currentItemID;
}

void WebPage::restoreSessionAndNavigateToCurrentItem(const SessionState& sessionState)
{
    if (uint64_t currentItemID = restoreSession(sessionState))
        goToBackForwardItem(currentItemID);
}

#if ENABLE(TOUCH_EVENTS)
#if PLATFORM(QT)
void WebPage::highlightPotentialActivation(const IntPoint& point, const IntSize& area)
{
    if (point == IntPoint::zero()) {
        // An empty point deactivates the highlighting.
        tapHighlightController().hideHighlight();
    } else {
        Frame* mainframe = m_page->mainFrame();
        Node* activationNode = 0;
        Node* adjustedNode = 0;
        IntPoint adjustedPoint;

#if ENABLE(TOUCH_ADJUSTMENT)
        if (!mainframe->eventHandler()->bestClickableNodeForTouchPoint(point, IntSize(area.width() / 2, area.height() / 2), adjustedPoint, adjustedNode))
            return;

#else
        HitTestResult result = mainframe->eventHandler()->hitTestResultAtPoint(mainframe->view()->windowToContents(point), HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::IgnoreClipping | HitTestRequest::DisallowShadowContent);
        adjustedNode = result.innerNode();
#endif
        // Find the node to highlight. This is not the same as the node responding the tap gesture, because many
        // pages has a global click handler and we do not want to highlight the body.
        for (Node* node = adjustedNode; node; node = node->parentOrShadowHostNode()) {
            if (node->isDocumentNode() || node->isFrameOwnerElement())
                break;

            // We always highlight focusable (form-elements), image links or content-editable elements.
            if ((node->isElementNode() && toElement(node)->isMouseFocusable()) || node->isLink() || node->isContentEditable())
                activationNode = node;
            else if (node->willRespondToMouseClickEvents()) {
                // Highlight elements with default mouse-click handlers, but highlight only inline elements with
                // scripted event-handlers.
                if (!node->Node::willRespondToMouseClickEvents() || (node->renderer() && node->renderer()->isInline()))
                    activationNode = node;
            }

            if (activationNode)
                break;
        }

        if (activationNode)
            tapHighlightController().highlight(activationNode);
    }
}
#endif

static bool handleTouchEvent(const WebTouchEvent& touchEvent, Page* page)
{
    Frame* frame = page->mainFrame();
    if (!frame->view())
        return false;

    return frame->eventHandler()->handleTouchEvent(platform(touchEvent));
}

void WebPage::touchEvent(const WebTouchEvent& touchEvent)
{
    bool handled = false;

    if (canHandleUserEvents()) {
        CurrentEvent currentEvent(touchEvent);

        handled = handleTouchEvent(touchEvent, m_page.get());
    }
    send(Messages::WebPageProxy::DidReceiveEvent(static_cast<uint32_t>(touchEvent.type()), handled));
}

void WebPage::touchEventSyncForTesting(const WebTouchEvent& touchEvent, bool& handled)
{
    CurrentEvent currentEvent(touchEvent);
    handled = handleTouchEvent(touchEvent, m_page.get());
}
#endif

void WebPage::scroll(Page* page, ScrollDirection direction, ScrollGranularity granularity)
{
    page->focusController()->focusedOrMainFrame()->eventHandler()->scrollRecursively(direction, granularity);
}

void WebPage::logicalScroll(Page* page, ScrollLogicalDirection direction, ScrollGranularity granularity)
{
    page->focusController()->focusedOrMainFrame()->eventHandler()->logicalScrollRecursively(direction, granularity);
}

void WebPage::scrollBy(uint32_t scrollDirection, uint32_t scrollGranularity)
{
    scroll(m_page.get(), static_cast<ScrollDirection>(scrollDirection), static_cast<ScrollGranularity>(scrollGranularity));
}

void WebPage::centerSelectionInVisibleArea()
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;
    
    frame->selection()->revealSelection(ScrollAlignment::alignCenterAlways);
    m_findController.showFindIndicatorInSelection();
}

void WebPage::setActive(bool isActive)
{
    m_page->focusController()->setActive(isActive);

#if PLATFORM(MAC)    
    // Tell all our plug-in views that the window focus changed.
    for (HashSet<PluginView*>::const_iterator it = m_pluginViews.begin(), end = m_pluginViews.end(); it != end; ++it)
        (*it)->setWindowIsFocused(isActive);
#endif
}

void WebPage::setDrawsBackground(bool drawsBackground)
{
    if (m_drawsBackground == drawsBackground)
        return;

    m_drawsBackground = drawsBackground;

    for (Frame* coreFrame = m_mainFrame->coreFrame(); coreFrame; coreFrame = coreFrame->tree()->traverseNext()) {
        if (FrameView* view = coreFrame->view())
            view->setTransparent(!drawsBackground);
    }

    m_drawingArea->pageBackgroundTransparencyChanged();
    m_drawingArea->setNeedsDisplay();
}

void WebPage::setDrawsTransparentBackground(bool drawsTransparentBackground)
{
    if (m_drawsTransparentBackground == drawsTransparentBackground)
        return;

    m_drawsTransparentBackground = drawsTransparentBackground;

    Color backgroundColor = drawsTransparentBackground ? Color::transparent : Color::white;
    for (Frame* coreFrame = m_mainFrame->coreFrame(); coreFrame; coreFrame = coreFrame->tree()->traverseNext()) {
        if (FrameView* view = coreFrame->view())
            view->setBaseBackgroundColor(backgroundColor);
    }

    m_drawingArea->pageBackgroundTransparencyChanged();
    m_drawingArea->setNeedsDisplay();
}

void WebPage::viewWillStartLiveResize()
{
    if (!m_page)
        return;

    // FIXME: This should propagate to all ScrollableAreas.
    if (Frame* frame = m_page->focusController()->focusedOrMainFrame()) {
        if (FrameView* view = frame->view())
            view->willStartLiveResize();
    }
}

void WebPage::viewWillEndLiveResize()
{
    if (!m_page)
        return;

    // FIXME: This should propagate to all ScrollableAreas.
    if (Frame* frame = m_page->focusController()->focusedOrMainFrame()) {
        if (FrameView* view = frame->view())
            view->willEndLiveResize();
    }
}

void WebPage::setFocused(bool isFocused)
{
    m_page->focusController()->setFocused(isFocused);
}

void WebPage::setInitialFocus(bool forward, bool isKeyboardEventValid, const WebKeyboardEvent& event)
{
    if (!m_page || !m_page->focusController())
        return;

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    frame->document()->setFocusedElement(0);

    if (isKeyboardEventValid && event.type() == WebEvent::KeyDown) {
        PlatformKeyboardEvent platformEvent(platform(event));
        platformEvent.disambiguateKeyDownEvent(PlatformEvent::RawKeyDown);
        m_page->focusController()->setInitialFocus(forward ? FocusDirectionForward : FocusDirectionBackward, KeyboardEvent::create(platformEvent, frame->document()->defaultView()).get());
        return;
    }

    m_page->focusController()->setInitialFocus(forward ? FocusDirectionForward : FocusDirectionBackward, 0);
}

void WebPage::setWindowResizerSize(const IntSize& windowResizerSize)
{
    if (m_windowResizerSize == windowResizerSize)
        return;

    m_windowResizerSize = windowResizerSize;

    for (Frame* coreFrame = m_mainFrame->coreFrame(); coreFrame; coreFrame = coreFrame->tree()->traverseNext()) {
        FrameView* view = coreFrame->view();
        if (view)
            view->windowResizerRectChanged();
    }
}

void WebPage::setCanStartMediaTimerFired()
{
    if (m_page)
        m_page->setCanStartMedia(true);
}

void WebPage::didUpdateInWindowStateTimerFired()
{
    send(Messages::WebPageProxy::DidUpdateInWindowState());
}

inline bool WebPage::canHandleUserEvents() const
{
#if USE(TILED_BACKING_STORE)
    // Should apply only if the area was frozen by didStartPageTransition().
    return !m_drawingArea->layerTreeStateIsFrozen();
#endif
    return true;
}

void WebPage::setIsInWindow(bool isInWindow)
{
    bool pageWasInWindow = m_page->isInWindow();
    
    if (!isInWindow) {
        m_setCanStartMediaTimer.stop();
        m_page->setCanStartMedia(false);
        m_page->willMoveOffscreen();
        
        if (pageWasInWindow)
            WebProcess::shared().pageWillLeaveWindow(m_pageID);
    } else {
        // Defer the call to Page::setCanStartMedia() since it ends up sending a synchronous message to the UI process
        // in order to get plug-in connections, and the UI process will be waiting for the Web process to update the backing
        // store after moving the view into a window, until it times out and paints white. See <rdar://problem/9242771>.
        if (m_mayStartMediaWhenInWindow)
            m_setCanStartMediaTimer.startOneShot(0);

        m_page->didMoveOnscreen();
        
        if (!pageWasInWindow)
            WebProcess::shared().pageDidEnterWindow(m_pageID);
    }

    m_page->setIsInWindow(isInWindow);
    m_sendDidUpdateInWindowStateTimer.startOneShot(0);
}

void WebPage::didReceivePolicyDecision(uint64_t frameID, uint64_t listenerID, uint32_t policyAction, uint64_t downloadID)
{
    WebFrame* frame = WebProcess::shared().webFrame(frameID);
    if (!frame)
        return;
    frame->didReceivePolicyDecision(listenerID, static_cast<PolicyAction>(policyAction), downloadID);
}

void WebPage::didStartPageTransition()
{
    m_drawingArea->setLayerTreeStateIsFrozen(true);
}

void WebPage::didCompletePageTransition()
{
#if USE(TILED_BACKING_STORE)
    if (m_mainFrame->coreFrame()->view()->delegatesScrolling())
        // Wait until the UI process sent us the visible rect it wants rendered.
        send(Messages::WebPageProxy::PageTransitionViewportReady());
    else
#endif
        
    m_drawingArea->setLayerTreeStateIsFrozen(false);
}

void WebPage::show()
{
    send(Messages::WebPageProxy::ShowPage());
}

void WebPage::setUserAgent(const String& userAgent)
{
    m_userAgent = userAgent;
}

void WebPage::suspendActiveDOMObjectsAndAnimations()
{
    m_page->suspendActiveDOMObjectsAndAnimations();
}

void WebPage::resumeActiveDOMObjectsAndAnimations()
{
    m_page->resumeActiveDOMObjectsAndAnimations();
}

IntPoint WebPage::screenToWindow(const IntPoint& point)
{
    IntPoint windowPoint;
    sendSync(Messages::WebPageProxy::ScreenToWindow(point), Messages::WebPageProxy::ScreenToWindow::Reply(windowPoint));
    return windowPoint;
}
    
IntRect WebPage::windowToScreen(const IntRect& rect)
{
    IntRect screenRect;
    sendSync(Messages::WebPageProxy::WindowToScreen(rect), Messages::WebPageProxy::WindowToScreen::Reply(screenRect));
    return screenRect;
}

IntRect WebPage::windowResizerRect() const
{
    if (m_windowResizerSize.isEmpty())
        return IntRect();

    IntSize frameViewSize;
    if (Frame* coreFrame = m_mainFrame->coreFrame()) {
        if (FrameView* view = coreFrame->view())
            frameViewSize = view->size();
    }

    return IntRect(frameViewSize.width() - m_windowResizerSize.width(), frameViewSize.height() - m_windowResizerSize.height(), 
                   m_windowResizerSize.width(), m_windowResizerSize.height());
}

KeyboardUIMode WebPage::keyboardUIMode()
{
    bool fullKeyboardAccessEnabled = WebProcess::shared().fullKeyboardAccessEnabled();
    return static_cast<KeyboardUIMode>((fullKeyboardAccessEnabled ? KeyboardAccessFull : KeyboardAccessDefault) | (m_tabToLinks ? KeyboardAccessTabsToLinks : 0));
}

void WebPage::runJavaScriptInMainFrame(const String& script, uint64_t callbackID)
{
    // NOTE: We need to be careful when running scripts that the objects we depend on don't
    // disappear during script execution.

    // Retain the SerializedScriptValue at this level so it (and the internal data) lives
    // long enough for the DataReference to be encoded by the sent message.
    RefPtr<SerializedScriptValue> serializedResultValue;
    CoreIPC::DataReference dataReference;

    JSLockHolder lock(JSDOMWindow::commonVM());
    if (JSValue resultValue = m_mainFrame->coreFrame()->script()->executeScript(script, true).jsValue()) {
        if ((serializedResultValue = SerializedScriptValue::create(m_mainFrame->jsContext(), toRef(m_mainFrame->coreFrame()->script()->globalObject(mainThreadNormalWorld())->globalExec(), resultValue), 0)))
            dataReference = serializedResultValue->data();
    }

    send(Messages::WebPageProxy::ScriptValueCallback(dataReference, callbackID));
}

void WebPage::getContentsAsString(uint64_t callbackID)
{
    String resultString = m_mainFrame->contentsAsString();
    send(Messages::WebPageProxy::StringCallback(resultString, callbackID));
}

#if ENABLE(MHTML)
void WebPage::getContentsAsMHTMLData(uint64_t callbackID, bool useBinaryEncoding)
{
    CoreIPC::DataReference dataReference;

    RefPtr<SharedBuffer> buffer = useBinaryEncoding
        ? MHTMLArchive::generateMHTMLDataUsingBinaryEncoding(m_page.get())
        : MHTMLArchive::generateMHTMLData(m_page.get());

    if (buffer)
        dataReference = CoreIPC::DataReference(reinterpret_cast<const uint8_t*>(buffer->data()), buffer->size());

    send(Messages::WebPageProxy::DataCallback(dataReference, callbackID));
}
#endif

void WebPage::getRenderTreeExternalRepresentation(uint64_t callbackID)
{
    String resultString = renderTreeExternalRepresentation();
    send(Messages::WebPageProxy::StringCallback(resultString, callbackID));
}

static Frame* frameWithSelection(Page* page)
{
    for (Frame* frame = page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (frame->selection()->isRange())
            return frame;
    }

    return 0;
}

void WebPage::getSelectionAsWebArchiveData(uint64_t callbackID)
{
    CoreIPC::DataReference dataReference;

#if PLATFORM(MAC)
    RefPtr<LegacyWebArchive> archive;
    RetainPtr<CFDataRef> data;

    Frame* frame = frameWithSelection(m_page.get());
    if (frame) {
        archive = LegacyWebArchive::createFromSelection(frame);
        data = archive->rawDataRepresentation();
        dataReference = CoreIPC::DataReference(CFDataGetBytePtr(data.get()), CFDataGetLength(data.get()));
    }
#endif

    send(Messages::WebPageProxy::DataCallback(dataReference, callbackID));
}

void WebPage::getSelectionOrContentsAsString(uint64_t callbackID)
{
    String resultString = m_mainFrame->selectionAsString();
    if (resultString.isEmpty())
        resultString = m_mainFrame->contentsAsString();
    send(Messages::WebPageProxy::StringCallback(resultString, callbackID));
}

void WebPage::getSourceForFrame(uint64_t frameID, uint64_t callbackID)
{
    String resultString;
    if (WebFrame* frame = WebProcess::shared().webFrame(frameID))
       resultString = frame->source();

    send(Messages::WebPageProxy::StringCallback(resultString, callbackID));
}

void WebPage::getMainResourceDataOfFrame(uint64_t frameID, uint64_t callbackID)
{
    CoreIPC::DataReference dataReference;

    RefPtr<ResourceBuffer> buffer;
    RefPtr<SharedBuffer> pdfResource;
    if (WebFrame* frame = WebProcess::shared().webFrame(frameID)) {
        if (PluginView* pluginView = pluginViewForFrame(frame->coreFrame())) {
            if ((pdfResource = pluginView->liveResourceData()))
                dataReference = CoreIPC::DataReference(reinterpret_cast<const uint8_t*>(pdfResource->data()), pdfResource->size());
        }

        if (dataReference.isEmpty()) {
            if (DocumentLoader* loader = frame->coreFrame()->loader()->documentLoader()) {
                if ((buffer = loader->mainResourceData()))
                    dataReference = CoreIPC::DataReference(reinterpret_cast<const uint8_t*>(buffer->data()), buffer->size());
            }
        }
    }

    send(Messages::WebPageProxy::DataCallback(dataReference, callbackID));
}

static PassRefPtr<SharedBuffer> resourceDataForFrame(Frame* frame, const KURL& resourceURL)
{
    DocumentLoader* loader = frame->loader()->documentLoader();
    if (!loader)
        return 0;

    RefPtr<ArchiveResource> subresource = loader->subresource(resourceURL);
    if (!subresource)
        return 0;

    return subresource->data();
}

void WebPage::getResourceDataFromFrame(uint64_t frameID, const String& resourceURLString, uint64_t callbackID)
{
    CoreIPC::DataReference dataReference;
    KURL resourceURL(KURL(), resourceURLString);

    RefPtr<SharedBuffer> buffer;
    if (WebFrame* frame = WebProcess::shared().webFrame(frameID)) {
        buffer = resourceDataForFrame(frame->coreFrame(), resourceURL);
        if (!buffer) {
            // Try to get the resource data from the cache.
            buffer = cachedResponseDataForURL(resourceURL);
        }

        if (buffer)
            dataReference = CoreIPC::DataReference(reinterpret_cast<const uint8_t*>(buffer->data()), buffer->size());
    }

    send(Messages::WebPageProxy::DataCallback(dataReference, callbackID));
}

void WebPage::getWebArchiveOfFrame(uint64_t frameID, uint64_t callbackID)
{
    CoreIPC::DataReference dataReference;

#if PLATFORM(MAC)
    RetainPtr<CFDataRef> data;
    if (WebFrame* frame = WebProcess::shared().webFrame(frameID)) {
        if ((data = frame->webArchiveData(0, 0)))
            dataReference = CoreIPC::DataReference(CFDataGetBytePtr(data.get()), CFDataGetLength(data.get()));
    }
#else
    UNUSED_PARAM(frameID);
#endif

    send(Messages::WebPageProxy::DataCallback(dataReference, callbackID));
}

void WebPage::forceRepaintWithoutCallback()
{
    m_drawingArea->forceRepaint();
}

void WebPage::forceRepaint(uint64_t callbackID)
{
    if (m_drawingArea->forceRepaintAsync(callbackID))
        return;

    forceRepaintWithoutCallback();
    send(Messages::WebPageProxy::VoidCallback(callbackID));
}

void WebPage::preferencesDidChange(const WebPreferencesStore& store)
{
    WebPreferencesStore::removeTestRunnerOverrides();
    updatePreferences(store);
}

void WebPage::updatePreferences(const WebPreferencesStore& store)
{
    Settings* settings = m_page->settings();

    m_tabToLinks = store.getBoolValueForKey(WebPreferencesKey::tabsToLinksKey());
    m_asynchronousPluginInitializationEnabled = store.getBoolValueForKey(WebPreferencesKey::asynchronousPluginInitializationEnabledKey());
    m_asynchronousPluginInitializationEnabledForAllPlugins = store.getBoolValueForKey(WebPreferencesKey::asynchronousPluginInitializationEnabledForAllPluginsKey());
    m_artificialPluginInitializationDelayEnabled = store.getBoolValueForKey(WebPreferencesKey::artificialPluginInitializationDelayEnabledKey());

    m_scrollingPerformanceLoggingEnabled = store.getBoolValueForKey(WebPreferencesKey::scrollingPerformanceLoggingEnabledKey());

#if PLATFORM(MAC)
    m_pdfPluginEnabled = store.getBoolValueForKey(WebPreferencesKey::pdfPluginEnabledKey());
#endif

    // FIXME: This should be generated from macro expansion for all preferences,
    // but we currently don't match the naming of WebCore exactly so we are
    // handrolling the boolean and integer preferences until that is fixed.

#define INITIALIZE_SETTINGS(KeyUpper, KeyLower, TypeName, Type, DefaultValue) settings->set##KeyUpper(store.get##TypeName##ValueForKey(WebPreferencesKey::KeyLower##Key()));

    FOR_EACH_WEBKIT_STRING_PREFERENCE(INITIALIZE_SETTINGS)

#undef INITIALIZE_SETTINGS

    settings->setScriptEnabled(store.getBoolValueForKey(WebPreferencesKey::javaScriptEnabledKey()));
    settings->setScriptMarkupEnabled(store.getBoolValueForKey(WebPreferencesKey::javaScriptMarkupEnabledKey()));
    settings->setLoadsImagesAutomatically(store.getBoolValueForKey(WebPreferencesKey::loadsImagesAutomaticallyKey()));
    settings->setLoadsSiteIconsIgnoringImageLoadingSetting(store.getBoolValueForKey(WebPreferencesKey::loadsSiteIconsIgnoringImageLoadingPreferenceKey()));
    settings->setPluginsEnabled(store.getBoolValueForKey(WebPreferencesKey::pluginsEnabledKey()));
    settings->setJavaEnabled(store.getBoolValueForKey(WebPreferencesKey::javaEnabledKey()));
    settings->setJavaEnabledForLocalFiles(store.getBoolValueForKey(WebPreferencesKey::javaEnabledForLocalFilesKey()));    
    settings->setOfflineWebApplicationCacheEnabled(store.getBoolValueForKey(WebPreferencesKey::offlineWebApplicationCacheEnabledKey()));
    settings->setLocalStorageEnabled(store.getBoolValueForKey(WebPreferencesKey::localStorageEnabledKey()));
    settings->setXSSAuditorEnabled(store.getBoolValueForKey(WebPreferencesKey::xssAuditorEnabledKey()));
    settings->setFrameFlatteningEnabled(store.getBoolValueForKey(WebPreferencesKey::frameFlatteningEnabledKey()));
    settings->setPrivateBrowsingEnabled(store.getBoolValueForKey(WebPreferencesKey::privateBrowsingEnabledKey()));
    settings->setDeveloperExtrasEnabled(store.getBoolValueForKey(WebPreferencesKey::developerExtrasEnabledKey()));
    settings->setJavaScriptExperimentsEnabled(store.getBoolValueForKey(WebPreferencesKey::javaScriptExperimentsEnabledKey()));
    settings->setTextAreasAreResizable(store.getBoolValueForKey(WebPreferencesKey::textAreasAreResizableKey()));
    settings->setNeedsSiteSpecificQuirks(store.getBoolValueForKey(WebPreferencesKey::needsSiteSpecificQuirksKey()));
    settings->setJavaScriptCanOpenWindowsAutomatically(store.getBoolValueForKey(WebPreferencesKey::javaScriptCanOpenWindowsAutomaticallyKey()));
    settings->setForceFTPDirectoryListings(store.getBoolValueForKey(WebPreferencesKey::forceFTPDirectoryListingsKey()));
    settings->setDNSPrefetchingEnabled(store.getBoolValueForKey(WebPreferencesKey::dnsPrefetchingEnabledKey()));
#if ENABLE(WEB_ARCHIVE)
    settings->setWebArchiveDebugModeEnabled(store.getBoolValueForKey(WebPreferencesKey::webArchiveDebugModeEnabledKey()));
#endif
    settings->setLocalFileContentSniffingEnabled(store.getBoolValueForKey(WebPreferencesKey::localFileContentSniffingEnabledKey()));
    settings->setUsesPageCache(store.getBoolValueForKey(WebPreferencesKey::usesPageCacheKey()));
    settings->setPageCacheSupportsPlugins(store.getBoolValueForKey(WebPreferencesKey::pageCacheSupportsPluginsKey()));
    settings->setAuthorAndUserStylesEnabled(store.getBoolValueForKey(WebPreferencesKey::authorAndUserStylesEnabledKey()));
    settings->setPaginateDuringLayoutEnabled(store.getBoolValueForKey(WebPreferencesKey::paginateDuringLayoutEnabledKey()));
    settings->setDOMPasteAllowed(store.getBoolValueForKey(WebPreferencesKey::domPasteAllowedKey()));
    settings->setJavaScriptCanAccessClipboard(store.getBoolValueForKey(WebPreferencesKey::javaScriptCanAccessClipboardKey()));
    settings->setShouldPrintBackgrounds(store.getBoolValueForKey(WebPreferencesKey::shouldPrintBackgroundsKey()));
    settings->setWebSecurityEnabled(store.getBoolValueForKey(WebPreferencesKey::webSecurityEnabledKey()));
    settings->setAllowUniversalAccessFromFileURLs(store.getBoolValueForKey(WebPreferencesKey::allowUniversalAccessFromFileURLsKey()));
    settings->setAllowFileAccessFromFileURLs(store.getBoolValueForKey(WebPreferencesKey::allowFileAccessFromFileURLsKey()));

    settings->setMinimumFontSize(store.getUInt32ValueForKey(WebPreferencesKey::minimumFontSizeKey()));
    settings->setMinimumLogicalFontSize(store.getUInt32ValueForKey(WebPreferencesKey::minimumLogicalFontSizeKey()));
    settings->setDefaultFontSize(store.getUInt32ValueForKey(WebPreferencesKey::defaultFontSizeKey()));
    settings->setDefaultFixedFontSize(store.getUInt32ValueForKey(WebPreferencesKey::defaultFixedFontSizeKey()));
    settings->setScreenFontSubstitutionEnabled(store.getBoolValueForKey(WebPreferencesKey::screenFontSubstitutionEnabledKey())
#if PLATFORM(MAC)
        || WebProcess::shared().shouldForceScreenFontSubstitution()
#endif
    );
    settings->setLayoutFallbackWidth(store.getUInt32ValueForKey(WebPreferencesKey::layoutFallbackWidthKey()));
    settings->setDeviceWidth(store.getUInt32ValueForKey(WebPreferencesKey::deviceWidthKey()));
    settings->setDeviceHeight(store.getUInt32ValueForKey(WebPreferencesKey::deviceHeightKey()));
    settings->setEditableLinkBehavior(static_cast<WebCore::EditableLinkBehavior>(store.getUInt32ValueForKey(WebPreferencesKey::editableLinkBehaviorKey())));
    settings->setShowsToolTipOverTruncatedText(store.getBoolValueForKey(WebPreferencesKey::showsToolTipOverTruncatedTextKey()));

    settings->setAcceleratedCompositingForOverflowScrollEnabled(store.getBoolValueForKey(WebPreferencesKey::acceleratedCompositingForOverflowScrollEnabledKey()));
    settings->setAcceleratedCompositingEnabled(store.getBoolValueForKey(WebPreferencesKey::acceleratedCompositingEnabledKey()) && LayerTreeHost::supportsAcceleratedCompositing());
    settings->setAcceleratedDrawingEnabled(store.getBoolValueForKey(WebPreferencesKey::acceleratedDrawingEnabledKey()) && LayerTreeHost::supportsAcceleratedCompositing());
    settings->setCanvasUsesAcceleratedDrawing(store.getBoolValueForKey(WebPreferencesKey::canvasUsesAcceleratedDrawingKey()) && LayerTreeHost::supportsAcceleratedCompositing());
    settings->setShowDebugBorders(store.getBoolValueForKey(WebPreferencesKey::compositingBordersVisibleKey()));
    settings->setShowRepaintCounter(store.getBoolValueForKey(WebPreferencesKey::compositingRepaintCountersVisibleKey()));
    settings->setShowTiledScrollingIndicator(store.getBoolValueForKey(WebPreferencesKey::tiledScrollingIndicatorVisibleKey()));
    settings->setAggressiveTileRetentionEnabled(store.getBoolValueForKey(WebPreferencesKey::aggressiveTileRetentionEnabledKey()));
    settings->setCSSCustomFilterEnabled(store.getBoolValueForKey(WebPreferencesKey::cssCustomFilterEnabledKey()));
    RuntimeEnabledFeatures::setCSSRegionsEnabled(store.getBoolValueForKey(WebPreferencesKey::cssRegionsEnabledKey()));
    RuntimeEnabledFeatures::setCSSCompositingEnabled(store.getBoolValueForKey(WebPreferencesKey::cssCompositingEnabledKey()));
    settings->setCSSGridLayoutEnabled(store.getBoolValueForKey(WebPreferencesKey::cssGridLayoutEnabledKey()));
    settings->setRegionBasedColumnsEnabled(store.getBoolValueForKey(WebPreferencesKey::regionBasedColumnsEnabledKey()));
    settings->setWebGLEnabled(store.getBoolValueForKey(WebPreferencesKey::webGLEnabledKey()));
    settings->setAccelerated2dCanvasEnabled(store.getBoolValueForKey(WebPreferencesKey::accelerated2dCanvasEnabledKey()));
    settings->setMediaPlaybackRequiresUserGesture(store.getBoolValueForKey(WebPreferencesKey::mediaPlaybackRequiresUserGestureKey()));
    settings->setMediaPlaybackAllowsInline(store.getBoolValueForKey(WebPreferencesKey::mediaPlaybackAllowsInlineKey()));
    settings->setMockScrollbarsEnabled(store.getBoolValueForKey(WebPreferencesKey::mockScrollbarsEnabledKey()));
    settings->setHyperlinkAuditingEnabled(store.getBoolValueForKey(WebPreferencesKey::hyperlinkAuditingEnabledKey()));
    settings->setRequestAnimationFrameEnabled(store.getBoolValueForKey(WebPreferencesKey::requestAnimationFrameEnabledKey()));
#if ENABLE(SMOOTH_SCROLLING)
    settings->setScrollAnimatorEnabled(store.getBoolValueForKey(WebPreferencesKey::scrollAnimatorEnabledKey()));
#endif
    settings->setInteractiveFormValidationEnabled(store.getBoolValueForKey(WebPreferencesKey::interactiveFormValidationEnabledKey()));

#if ENABLE(SQL_DATABASE)
    DatabaseManager::manager().setIsAvailable(store.getBoolValueForKey(WebPreferencesKey::databasesEnabledKey()));
#endif

#if ENABLE(FULLSCREEN_API)
    settings->setFullScreenEnabled(store.getBoolValueForKey(WebPreferencesKey::fullScreenEnabledKey()));
#endif

#if USE(AVFOUNDATION)
    settings->setAVFoundationEnabled(store.getBoolValueForKey(WebPreferencesKey::isAVFoundationEnabledKey()));
#endif

#if PLATFORM(MAC)
    settings->setQTKitEnabled(store.getBoolValueForKey(WebPreferencesKey::isQTKitEnabledKey()));
#endif

#if ENABLE(WEB_AUDIO)
    settings->setWebAudioEnabled(store.getBoolValueForKey(WebPreferencesKey::webAudioEnabledKey()));
#endif

    settings->setApplicationChromeMode(store.getBoolValueForKey(WebPreferencesKey::applicationChromeModeKey()));
    settings->setSuppressesIncrementalRendering(store.getBoolValueForKey(WebPreferencesKey::suppressesIncrementalRenderingKey()));
    settings->setIncrementalRenderingSuppressionTimeoutInSeconds(store.getDoubleValueForKey(WebPreferencesKey::incrementalRenderingSuppressionTimeoutKey()));
    settings->setBackspaceKeyNavigationEnabled(store.getBoolValueForKey(WebPreferencesKey::backspaceKeyNavigationEnabledKey()));
    settings->setWantsBalancedSetDefersLoadingBehavior(store.getBoolValueForKey(WebPreferencesKey::wantsBalancedSetDefersLoadingBehaviorKey()));
    settings->setCaretBrowsingEnabled(store.getBoolValueForKey(WebPreferencesKey::caretBrowsingEnabledKey()));

#if ENABLE(VIDEO_TRACK)
    settings->setShouldDisplaySubtitles(store.getBoolValueForKey(WebPreferencesKey::shouldDisplaySubtitlesKey()));
    settings->setShouldDisplayCaptions(store.getBoolValueForKey(WebPreferencesKey::shouldDisplayCaptionsKey()));
    settings->setShouldDisplayTextDescriptions(store.getBoolValueForKey(WebPreferencesKey::shouldDisplayTextDescriptionsKey()));
#endif

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    settings->setNotificationsEnabled(store.getBoolValueForKey(WebPreferencesKey::notificationsEnabledKey()));
#endif

    settings->setShouldRespectImageOrientation(store.getBoolValueForKey(WebPreferencesKey::shouldRespectImageOrientationKey()));
    settings->setStorageBlockingPolicy(static_cast<SecurityOrigin::StorageBlockingPolicy>(store.getUInt32ValueForKey(WebPreferencesKey::storageBlockingPolicyKey())));
    settings->setCookieEnabled(store.getBoolValueForKey(WebPreferencesKey::cookieEnabledKey()));

    settings->setDiagnosticLoggingEnabled(store.getBoolValueForKey(WebPreferencesKey::diagnosticLoggingEnabledKey()));

    settings->setScrollingPerformanceLoggingEnabled(m_scrollingPerformanceLoggingEnabled);

    settings->setPlugInSnapshottingEnabled(store.getBoolValueForKey(WebPreferencesKey::plugInSnapshottingEnabledKey()));
    settings->setSnapshotAllPlugIns(store.getBoolValueForKey(WebPreferencesKey::snapshotAllPlugInsKey()));
    settings->setAutostartOriginPlugInSnapshottingEnabled(store.getBoolValueForKey(WebPreferencesKey::autostartOriginPlugInSnapshottingEnabledKey()));
    settings->setPrimaryPlugInSnapshotDetectionEnabled(store.getBoolValueForKey(WebPreferencesKey::primaryPlugInSnapshotDetectionEnabledKey()));
    settings->setUsesEncodingDetector(store.getBoolValueForKey(WebPreferencesKey::usesEncodingDetectorKey()));

#if ENABLE(TEXT_AUTOSIZING)
    settings->setTextAutosizingEnabled(store.getBoolValueForKey(WebPreferencesKey::textAutosizingEnabledKey()));
#endif

    settings->setLogsPageMessagesToSystemConsoleEnabled(store.getBoolValueForKey(WebPreferencesKey::logsPageMessagesToSystemConsoleEnabledKey()));
    settings->setAsynchronousSpellCheckingEnabled(store.getBoolValueForKey(WebPreferencesKey::asynchronousSpellCheckingEnabledKey()));

    settings->setSmartInsertDeleteEnabled(store.getBoolValueForKey(WebPreferencesKey::smartInsertDeleteEnabledKey()));
    settings->setSelectTrailingWhitespaceEnabled(store.getBoolValueForKey(WebPreferencesKey::selectTrailingWhitespaceEnabledKey()));
    settings->setShowsURLsInToolTips(store.getBoolValueForKey(WebPreferencesKey::showsURLsInToolTipsEnabledKey()));

#if ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
    settings->setHiddenPageDOMTimerThrottlingEnabled(store.getBoolValueForKey(WebPreferencesKey::hiddenPageDOMTimerThrottlingEnabledKey()));
#endif
#if ENABLE(PAGE_VISIBILITY_API)
    settings->setHiddenPageCSSAnimationSuspensionEnabled(store.getBoolValueForKey(WebPreferencesKey::hiddenPageCSSAnimationSuspensionEnabledKey()));
#endif

    settings->setLowPowerVideoAudioBufferSizeEnabled(store.getBoolValueForKey(WebPreferencesKey::lowPowerVideoAudioBufferSizeEnabledKey()));

    platformPreferencesDidChange(store);

    if (m_drawingArea)
        m_drawingArea->updatePreferences(store);
}

#if ENABLE(INSPECTOR)
WebInspector* WebPage::inspector()
{
    if (m_isClosed)
        return 0;
    if (!m_inspector)
        m_inspector = WebInspector::create(this, m_inspectorClient);
    return m_inspector.get();
}
#endif

#if ENABLE(FULLSCREEN_API)
WebFullScreenManager* WebPage::fullScreenManager()
{
    if (!m_fullScreenManager)
        m_fullScreenManager = WebFullScreenManager::create(this);
    return m_fullScreenManager.get();
}
#endif

NotificationPermissionRequestManager* WebPage::notificationPermissionRequestManager()
{
    if (m_notificationPermissionRequestManager)
        return m_notificationPermissionRequestManager.get();

    m_notificationPermissionRequestManager = NotificationPermissionRequestManager::create(this);
    return m_notificationPermissionRequestManager.get();
}

#if !PLATFORM(GTK) && !PLATFORM(MAC)
bool WebPage::handleEditingKeyboardEvent(KeyboardEvent* evt)
{
    Node* node = evt->target()->toNode();
    ASSERT(node);
    Frame* frame = node->document()->frame();
    ASSERT(frame);

    const PlatformKeyboardEvent* keyEvent = evt->keyEvent();
    if (!keyEvent)
        return false;

    Editor::Command command = frame->editor().command(interpretKeyEvent(evt));

    if (keyEvent->type() == PlatformEvent::RawKeyDown) {
        // WebKit doesn't have enough information about mode to decide how commands that just insert text if executed via Editor should be treated,
        // so we leave it upon WebCore to either handle them immediately (e.g. Tab that changes focus) or let a keypress event be generated
        // (e.g. Tab that inserts a Tab character, or Enter).
        return !command.isTextInsertion() && command.execute(evt);
    }

    if (command.execute(evt))
        return true;

    // Don't allow text insertion for nodes that cannot edit.
    if (!frame->editor().canEdit())
        return false;

    // Don't insert null or control characters as they can result in unexpected behaviour
    if (evt->charCode() < ' ')
        return false;

    return frame->editor().insertText(evt->keyEvent()->text(), evt);
}
#endif

#if ENABLE(DRAG_SUPPORT)

#if PLATFORM(QT) || PLATFORM(GTK)
void WebPage::performDragControllerAction(uint64_t action, WebCore::DragData dragData)
{
    if (!m_page) {
        send(Messages::WebPageProxy::DidPerformDragControllerAction(WebCore::DragSession()));
#if PLATFORM(QT)
        QMimeData* data = const_cast<QMimeData*>(dragData.platformData());
        delete data;
#elif PLATFORM(GTK)
        DataObjectGtk* data = const_cast<DataObjectGtk*>(dragData.platformData());
        data->deref();
#endif
        return;
    }

    switch (action) {
    case DragControllerActionEntered:
        send(Messages::WebPageProxy::DidPerformDragControllerAction(m_page->dragController()->dragEntered(&dragData)));
        break;

    case DragControllerActionUpdated:
        send(Messages::WebPageProxy::DidPerformDragControllerAction(m_page->dragController()->dragUpdated(&dragData)));
        break;

    case DragControllerActionExited:
        m_page->dragController()->dragExited(&dragData);
        break;

    case DragControllerActionPerformDrag: {
        m_page->dragController()->performDrag(&dragData);
        break;
    }

    default:
        ASSERT_NOT_REACHED();
    }
    // DragData does not delete its platformData so we need to do that here.
#if PLATFORM(QT)
    QMimeData* data = const_cast<QMimeData*>(dragData.platformData());
    delete data;
#elif PLATFORM(GTK)
    DataObjectGtk* data = const_cast<DataObjectGtk*>(dragData.platformData());
    data->deref();
#endif
}

#else
void WebPage::performDragControllerAction(uint64_t action, WebCore::IntPoint clientPosition, WebCore::IntPoint globalPosition, uint64_t draggingSourceOperationMask, const String& dragStorageName, uint32_t flags, const SandboxExtension::Handle& sandboxExtensionHandle, const SandboxExtension::HandleArray& sandboxExtensionsHandleArray)
{
    if (!m_page) {
        send(Messages::WebPageProxy::DidPerformDragControllerAction(WebCore::DragSession()));
        return;
    }

    DragData dragData(dragStorageName, clientPosition, globalPosition, static_cast<DragOperation>(draggingSourceOperationMask), static_cast<DragApplicationFlags>(flags));
    switch (action) {
    case DragControllerActionEntered:
        send(Messages::WebPageProxy::DidPerformDragControllerAction(m_page->dragController()->dragEntered(&dragData)));
        break;

    case DragControllerActionUpdated:
        send(Messages::WebPageProxy::DidPerformDragControllerAction(m_page->dragController()->dragUpdated(&dragData)));
        break;
        
    case DragControllerActionExited:
        m_page->dragController()->dragExited(&dragData);
        break;
        
    case DragControllerActionPerformDrag: {
        ASSERT(!m_pendingDropSandboxExtension);

        m_pendingDropSandboxExtension = SandboxExtension::create(sandboxExtensionHandle);
        for (size_t i = 0; i < sandboxExtensionsHandleArray.size(); i++) {
            if (RefPtr<SandboxExtension> extension = SandboxExtension::create(sandboxExtensionsHandleArray[i]))
                m_pendingDropExtensionsForFileUpload.append(extension);
        }

        m_page->dragController()->performDrag(&dragData);

        // If we started loading a local file, the sandbox extension tracker would have adopted this
        // pending drop sandbox extension. If not, we'll play it safe and clear it.
        m_pendingDropSandboxExtension = nullptr;

        m_pendingDropExtensionsForFileUpload.clear();
        break;
    }

    default:
        ASSERT_NOT_REACHED();
    }
}
#endif

void WebPage::dragEnded(WebCore::IntPoint clientPosition, WebCore::IntPoint globalPosition, uint64_t operation)
{
    IntPoint adjustedClientPosition(clientPosition.x() + m_page->dragController()->dragOffset().x(), clientPosition.y() + m_page->dragController()->dragOffset().y());
    IntPoint adjustedGlobalPosition(globalPosition.x() + m_page->dragController()->dragOffset().x(), globalPosition.y() + m_page->dragController()->dragOffset().y());

    m_page->dragController()->dragEnded();
    FrameView* view = m_page->mainFrame()->view();
    if (!view)
        return;
    // FIXME: These are fake modifier keys here, but they should be real ones instead.
    PlatformMouseEvent event(adjustedClientPosition, adjustedGlobalPosition, LeftButton, PlatformEvent::MouseMoved, 0, false, false, false, false, currentTime());
    m_page->mainFrame()->eventHandler()->dragSourceEndedAt(event, (DragOperation)operation);
}

void WebPage::willPerformLoadDragDestinationAction()
{
    m_sandboxExtensionTracker.willPerformLoadDragDestinationAction(m_pendingDropSandboxExtension.release());
}

void WebPage::mayPerformUploadDragDestinationAction()
{
    for (size_t i = 0; i < m_pendingDropExtensionsForFileUpload.size(); i++)
        m_pendingDropExtensionsForFileUpload[i]->consumePermanently();
    m_pendingDropExtensionsForFileUpload.clear();
}
    
#endif // ENABLE(DRAG_SUPPORT)

WebUndoStep* WebPage::webUndoStep(uint64_t stepID)
{
    return m_undoStepMap.get(stepID);
}

void WebPage::addWebUndoStep(uint64_t stepID, WebUndoStep* entry)
{
    m_undoStepMap.set(stepID, entry);
}

void WebPage::removeWebEditCommand(uint64_t stepID)
{
    m_undoStepMap.remove(stepID);
}

void WebPage::unapplyEditCommand(uint64_t stepID)
{
    WebUndoStep* step = webUndoStep(stepID);
    if (!step)
        return;

    step->step()->unapply();
}

void WebPage::reapplyEditCommand(uint64_t stepID)
{
    WebUndoStep* step = webUndoStep(stepID);
    if (!step)
        return;

    m_isInRedo = true;
    step->step()->reapply();
    m_isInRedo = false;
}

void WebPage::didRemoveEditCommand(uint64_t commandID)
{
    removeWebEditCommand(commandID);
}

void WebPage::setActivePopupMenu(WebPopupMenu* menu)
{
    m_activePopupMenu = menu;
}

#if ENABLE(INPUT_TYPE_COLOR)
void WebPage::setActiveColorChooser(WebColorChooser* colorChooser)
{
    m_activeColorChooser = colorChooser;
}

void WebPage::didEndColorChooser()
{
    m_activeColorChooser->didEndChooser();
}

void WebPage::didChooseColor(const WebCore::Color& color)
{
    m_activeColorChooser->didChooseColor(color);
}
#endif

void WebPage::setActiveOpenPanelResultListener(PassRefPtr<WebOpenPanelResultListener> openPanelResultListener)
{
    m_activeOpenPanelResultListener = openPanelResultListener;
}

bool WebPage::findStringFromInjectedBundle(const String& target, FindOptions options)
{
    return m_page->findString(target, options);
}

void WebPage::findString(const String& string, uint32_t options, uint32_t maxMatchCount)
{
    m_findController.findString(string, static_cast<FindOptions>(options), maxMatchCount);
}

void WebPage::findStringMatches(const String& string, uint32_t options, uint32_t maxMatchCount)
{
    m_findController.findStringMatches(string, static_cast<FindOptions>(options), maxMatchCount);
}

void WebPage::getImageForFindMatch(uint32_t matchIndex)
{
    m_findController.getImageForFindMatch(matchIndex);
}

void WebPage::selectFindMatch(uint32_t matchIndex)
{
    m_findController.selectFindMatch(matchIndex);
}

void WebPage::hideFindUI()
{
    m_findController.hideFindUI();
}

void WebPage::countStringMatches(const String& string, uint32_t options, uint32_t maxMatchCount)
{
    m_findController.countStringMatches(string, static_cast<FindOptions>(options), maxMatchCount);
}

void WebPage::didChangeSelectedIndexForActivePopupMenu(int32_t newIndex)
{
    changeSelectedIndex(newIndex);
    m_activePopupMenu = 0;
}

void WebPage::changeSelectedIndex(int32_t index)
{
    if (!m_activePopupMenu)
        return;

    m_activePopupMenu->didChangeSelectedIndex(index);
}

void WebPage::didChooseFilesForOpenPanel(const Vector<String>& files)
{
    if (!m_activeOpenPanelResultListener)
        return;

    m_activeOpenPanelResultListener->didChooseFiles(files);
    m_activeOpenPanelResultListener = 0;
}

void WebPage::didCancelForOpenPanel()
{
    m_activeOpenPanelResultListener = 0;
}

#if ENABLE(WEB_PROCESS_SANDBOX)
void WebPage::extendSandboxForFileFromOpenPanel(const SandboxExtension::Handle& handle)
{
    SandboxExtension::create(handle)->consumePermanently();
}
#endif

#if ENABLE(GEOLOCATION)
void WebPage::didReceiveGeolocationPermissionDecision(uint64_t geolocationID, bool allowed)
{
    m_geolocationPermissionRequestManager.didReceiveGeolocationPermissionDecision(geolocationID, allowed);
}
#endif

void WebPage::didReceiveNotificationPermissionDecision(uint64_t notificationID, bool allowed)
{
    notificationPermissionRequestManager()->didReceiveNotificationPermissionDecision(notificationID, allowed);
}

void WebPage::advanceToNextMisspelling(bool startBeforeSelection)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    frame->editor().advanceToNextMisspelling(startBeforeSelection);
}

void WebPage::changeSpellingToWord(const String& word)
{
    replaceSelectionWithText(m_page->focusController()->focusedOrMainFrame(), word);
}

void WebPage::unmarkAllMisspellings()
{
    for (Frame* frame = m_page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (Document* document = frame->document())
            document->markers()->removeMarkers(DocumentMarker::Spelling);
    }
}

void WebPage::unmarkAllBadGrammar()
{
    for (Frame* frame = m_page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (Document* document = frame->document())
            document->markers()->removeMarkers(DocumentMarker::Grammar);
    }
}

#if USE(APPKIT)
void WebPage::uppercaseWord()
{
    m_page->focusController()->focusedOrMainFrame()->editor().uppercaseWord();
}

void WebPage::lowercaseWord()
{
    m_page->focusController()->focusedOrMainFrame()->editor().lowercaseWord();
}

void WebPage::capitalizeWord()
{
    m_page->focusController()->focusedOrMainFrame()->editor().capitalizeWord();
}
#endif
    
void WebPage::setTextForActivePopupMenu(int32_t index)
{
    if (!m_activePopupMenu)
        return;

    m_activePopupMenu->setTextForIndex(index);
}

#if PLATFORM(GTK)
void WebPage::failedToShowPopupMenu()
{
    if (!m_activePopupMenu)
        return;

    m_activePopupMenu->client()->popupDidHide();
}
#endif

#if ENABLE(CONTEXT_MENUS)
void WebPage::didSelectItemFromActiveContextMenu(const WebContextMenuItemData& item)
{
    if (!m_contextMenu)
        return;

    m_contextMenu->itemSelected(item);
    m_contextMenu = 0;
}
#endif

void WebPage::replaceSelectionWithText(Frame* frame, const String& text)
{
    bool selectReplacement = true;
    bool smartReplace = false;
    return frame->editor().replaceSelectionWithText(text, selectReplacement, smartReplace);
}

void WebPage::clearSelection()
{
    m_page->focusController()->focusedOrMainFrame()->selection()->clear();
}

void WebPage::didChangeScrollOffsetForMainFrame()
{
    Frame* frame = m_page->mainFrame();
    IntPoint scrollPosition = frame->view()->scrollPosition();
    IntPoint maximumScrollPosition = frame->view()->maximumScrollPosition();
    IntPoint minimumScrollPosition = frame->view()->minimumScrollPosition();

    bool isPinnedToLeftSide = (scrollPosition.x() <= minimumScrollPosition.x());
    bool isPinnedToRightSide = (scrollPosition.x() >= maximumScrollPosition.x());
    bool isPinnedToTopSide = (scrollPosition.y() <= minimumScrollPosition.y());
    bool isPinnedToBottomSide = (scrollPosition.y() >= maximumScrollPosition.y());

    if (isPinnedToLeftSide != m_cachedMainFrameIsPinnedToLeftSide || isPinnedToRightSide != m_cachedMainFrameIsPinnedToRightSide || isPinnedToTopSide != m_cachedMainFrameIsPinnedToTopSide || isPinnedToBottomSide != m_cachedMainFrameIsPinnedToBottomSide) {
        send(Messages::WebPageProxy::DidChangeScrollOffsetPinningForMainFrame(isPinnedToLeftSide, isPinnedToRightSide, isPinnedToTopSide, isPinnedToBottomSide));
        
        m_cachedMainFrameIsPinnedToLeftSide = isPinnedToLeftSide;
        m_cachedMainFrameIsPinnedToRightSide = isPinnedToRightSide;
        m_cachedMainFrameIsPinnedToTopSide = isPinnedToTopSide;
        m_cachedMainFrameIsPinnedToBottomSide = isPinnedToBottomSide;
    }
}

void WebPage::mainFrameDidLayout()
{
    unsigned pageCount = m_page->pageCount();
    if (pageCount != m_cachedPageCount) {
        send(Messages::WebPageProxy::DidChangePageCount(pageCount));
        m_cachedPageCount = pageCount;
    }

#if USE(TILED_BACKING_STORE) && USE(ACCELERATED_COMPOSITING)
    if (m_drawingArea && m_drawingArea->layerTreeHost()) {
        double red, green, blue, alpha;
        m_mainFrame->getDocumentBackgroundColor(&red, &green, &blue, &alpha);
        RGBA32 rgba = makeRGBA32FromFloats(red, green, blue, alpha);
        if (m_backgroundColor.rgb() != rgba) {
            m_backgroundColor.setRGB(rgba);
            m_drawingArea->layerTreeHost()->setBackgroundColor(m_backgroundColor);
        }
    }
#endif
}

void WebPage::addPluginView(PluginView* pluginView)
{
    ASSERT(!m_pluginViews.contains(pluginView));

    m_pluginViews.add(pluginView);
#if ENABLE(PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC)
    m_determinePrimarySnapshottedPlugInTimer.startOneShot(0);
#endif
}

void WebPage::removePluginView(PluginView* pluginView)
{
    ASSERT(m_pluginViews.contains(pluginView));

    m_pluginViews.remove(pluginView);
}

void WebPage::sendSetWindowFrame(const FloatRect& windowFrame)
{
#if PLATFORM(MAC)
    m_hasCachedWindowFrame = false;
#endif
    send(Messages::WebPageProxy::SetWindowFrame(windowFrame));
}

#if PLATFORM(MAC)
void WebPage::setWindowIsVisible(bool windowIsVisible)
{
    m_windowIsVisible = windowIsVisible;

    corePage()->focusController()->setContainingWindowIsVisible(windowIsVisible);

    // Tell all our plug-in views that the window visibility changed.
    for (HashSet<PluginView*>::const_iterator it = m_pluginViews.begin(), end = m_pluginViews.end(); it != end; ++it)
        (*it)->setWindowIsVisible(windowIsVisible);
}

void WebPage::windowAndViewFramesChanged(const FloatRect& windowFrameInScreenCoordinates, const FloatRect& windowFrameInUnflippedScreenCoordinates, const FloatRect& viewFrameInWindowCoordinates, const FloatPoint& accessibilityViewCoordinates)
{
    m_windowFrameInScreenCoordinates = windowFrameInScreenCoordinates;
    m_windowFrameInUnflippedScreenCoordinates = windowFrameInUnflippedScreenCoordinates;
    m_viewFrameInWindowCoordinates = viewFrameInWindowCoordinates;
    m_accessibilityPosition = accessibilityViewCoordinates;
    
    // Tell all our plug-in views that the window and view frames have changed.
    for (HashSet<PluginView*>::const_iterator it = m_pluginViews.begin(), end = m_pluginViews.end(); it != end; ++it)
        (*it)->windowAndViewFramesChanged(enclosingIntRect(windowFrameInScreenCoordinates), enclosingIntRect(viewFrameInWindowCoordinates));

    m_hasCachedWindowFrame = !m_windowFrameInUnflippedScreenCoordinates.isEmpty();
}
#endif

void WebPage::viewExposedRectChanged(const FloatRect& exposedRect, bool clipsToExposedRect)
{
    m_drawingArea->setExposedRect(exposedRect);
    m_drawingArea->setClipsToExposedRect(clipsToExposedRect);
}

void WebPage::setMainFrameIsScrollable(bool isScrollable)
{
    m_mainFrameIsScrollable = isScrollable;
    m_drawingArea->mainFrameScrollabilityChanged(isScrollable);

    if (FrameView* frameView = m_mainFrame->coreFrame()->view()) {
        frameView->setCanHaveScrollbars(isScrollable);
        frameView->setProhibitsScrolling(!isScrollable);
    }
}

bool WebPage::windowIsFocused() const
{
    return m_page->focusController()->isActive();
}

bool WebPage::windowAndWebPageAreFocused() const
{
#if PLATFORM(MAC)
    if (!m_windowIsVisible)
        return false;
#endif
    return m_page->focusController()->isFocused() && m_page->focusController()->isActive();
}

void WebPage::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder)
{
    if (decoder.messageReceiverName() == Messages::DrawingArea::messageReceiverName()) {
        if (m_drawingArea)
            m_drawingArea->didReceiveDrawingAreaMessage(connection, decoder);
        return;
    }

#if USE(TILED_BACKING_STORE) && USE(ACCELERATED_COMPOSITING)
    if (decoder.messageReceiverName() == Messages::CoordinatedLayerTreeHost::messageReceiverName()) {
        if (m_drawingArea)
            m_drawingArea->didReceiveCoordinatedLayerTreeHostMessage(connection, decoder);
        return;
    }
#endif
    
#if ENABLE(INSPECTOR)
    if (decoder.messageReceiverName() == Messages::WebInspector::messageReceiverName()) {
        if (WebInspector* inspector = this->inspector())
            inspector->didReceiveWebInspectorMessage(connection, decoder);
        return;
    }
#endif

#if ENABLE(FULLSCREEN_API)
    if (decoder.messageReceiverName() == Messages::WebFullScreenManager::messageReceiverName()) {
        fullScreenManager()->didReceiveMessage(connection, decoder);
        return;
    }
#endif

    didReceiveWebPageMessage(connection, decoder);
}

void WebPage::didReceiveSyncMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder, OwnPtr<CoreIPC::MessageEncoder>& replyEncoder)
{   
    didReceiveSyncWebPageMessage(connection, decoder, replyEncoder);
}
    
InjectedBundleBackForwardList* WebPage::backForwardList()
{
    if (!m_backForwardList)
        m_backForwardList = InjectedBundleBackForwardList::create(this);
    return m_backForwardList.get();
}

WebPage::SandboxExtensionTracker::~SandboxExtensionTracker()
{
    invalidate();
}

void WebPage::SandboxExtensionTracker::invalidate()
{
    m_pendingProvisionalSandboxExtension = nullptr;

    if (m_provisionalSandboxExtension) {
        m_provisionalSandboxExtension->revoke();
        m_provisionalSandboxExtension = nullptr;
    }

    if (m_committedSandboxExtension) {
        m_committedSandboxExtension->revoke();
        m_committedSandboxExtension = nullptr;
    }
}

void WebPage::SandboxExtensionTracker::willPerformLoadDragDestinationAction(PassRefPtr<SandboxExtension> pendingDropSandboxExtension)
{
    setPendingProvisionalSandboxExtension(pendingDropSandboxExtension);
}

void WebPage::SandboxExtensionTracker::beginLoad(WebFrame* frame, const SandboxExtension::Handle& handle)
{
    ASSERT_UNUSED(frame, frame->isMainFrame());

    setPendingProvisionalSandboxExtension(SandboxExtension::create(handle));
}

void WebPage::SandboxExtensionTracker::setPendingProvisionalSandboxExtension(PassRefPtr<SandboxExtension> pendingProvisionalSandboxExtension)
{
    m_pendingProvisionalSandboxExtension = pendingProvisionalSandboxExtension;    
}

static bool shouldReuseCommittedSandboxExtension(WebFrame* frame)
{
    ASSERT(frame->isMainFrame());

    FrameLoader* frameLoader = frame->coreFrame()->loader();
    FrameLoadType frameLoadType = frameLoader->loadType();

    // If the page is being reloaded, it should reuse whatever extension is committed.
    if (frameLoadType == FrameLoadTypeReload || frameLoadType == FrameLoadTypeReloadFromOrigin)
        return true;

    DocumentLoader* documentLoader = frameLoader->documentLoader();
    DocumentLoader* provisionalDocumentLoader = frameLoader->provisionalDocumentLoader();
    if (!documentLoader || !provisionalDocumentLoader)
        return false;

    if (documentLoader->url().isLocalFile() && provisionalDocumentLoader->url().isLocalFile())
        return true;

    return false;
}

void WebPage::SandboxExtensionTracker::didStartProvisionalLoad(WebFrame* frame)
{
    if (!frame->isMainFrame())
        return;

    // We should only reuse the commited sandbox extension if it is not null. It can be
    // null if the last load was for an error page.
    if (m_committedSandboxExtension && shouldReuseCommittedSandboxExtension(frame))
        m_pendingProvisionalSandboxExtension = m_committedSandboxExtension;

    ASSERT(!m_provisionalSandboxExtension);

    m_provisionalSandboxExtension = m_pendingProvisionalSandboxExtension.release();
    if (!m_provisionalSandboxExtension)
        return;

    ASSERT(!m_provisionalSandboxExtension || frame->coreFrame()->loader()->provisionalDocumentLoader()->url().isLocalFile());

    m_provisionalSandboxExtension->consume();
}

void WebPage::SandboxExtensionTracker::didCommitProvisionalLoad(WebFrame* frame)
{
    if (!frame->isMainFrame())
        return;

    if (m_committedSandboxExtension)
        m_committedSandboxExtension->revoke();

    m_committedSandboxExtension = m_provisionalSandboxExtension.release();

    // We can also have a non-null m_pendingProvisionalSandboxExtension if a new load is being started.
    // This extension is not cleared, because it does not pertain to the failed load, and will be needed.
}

void WebPage::SandboxExtensionTracker::didFailProvisionalLoad(WebFrame* frame)
{
    if (!frame->isMainFrame())
        return;

    if (!m_provisionalSandboxExtension)
        return;

    m_provisionalSandboxExtension->revoke();
    m_provisionalSandboxExtension = nullptr;

    // We can also have a non-null m_pendingProvisionalSandboxExtension if a new load is being started
    // (notably, if the current one fails because the new one cancels it). This extension is not cleared,
    // because it does not pertain to the failed load, and will be needed.
}

bool WebPage::hasLocalDataForURL(const KURL& url)
{
    if (url.isLocalFile())
        return true;

    FrameLoader* frameLoader = m_page->mainFrame()->loader();
    DocumentLoader* documentLoader = frameLoader ? frameLoader->documentLoader() : 0;
    if (documentLoader && documentLoader->subresource(url))
        return true;

    return platformHasLocalDataForURL(url);
}

void WebPage::setCustomTextEncodingName(const String& encoding)
{
    m_page->mainFrame()->loader()->reloadWithOverrideEncoding(encoding);
}

void WebPage::didRemoveBackForwardItem(uint64_t itemID)
{
    WebBackForwardListProxy::removeItem(itemID);
}

#if PLATFORM(MAC)

bool WebPage::isSpeaking()
{
    bool result;
    return sendSync(Messages::WebPageProxy::GetIsSpeaking(), Messages::WebPageProxy::GetIsSpeaking::Reply(result)) && result;
}

void WebPage::speak(const String& string)
{
    send(Messages::WebPageProxy::Speak(string));
}

void WebPage::stopSpeaking()
{
    send(Messages::WebPageProxy::StopSpeaking());
}

#endif

#if PLATFORM(MAC)
RetainPtr<PDFDocument> WebPage::pdfDocumentForPrintingFrame(Frame* coreFrame)
{
    Document* document = coreFrame->document();
    if (!document)
        return 0;

    if (!document->isPluginDocument())
        return 0;

    PluginView* pluginView = static_cast<PluginView*>(toPluginDocument(document)->pluginWidget());
    if (!pluginView)
        return 0;

    return pluginView->pdfDocumentForPrinting();
}
#endif // PLATFORM(MAC)

void WebPage::beginPrinting(uint64_t frameID, const PrintInfo& printInfo)
{
    WebFrame* frame = WebProcess::shared().webFrame(frameID);
    if (!frame)
        return;

    Frame* coreFrame = frame->coreFrame();
    if (!coreFrame)
        return;

#if PLATFORM(MAC)
    if (pdfDocumentForPrintingFrame(coreFrame))
        return;
#endif // PLATFORM(MAC)

    if (!m_printContext)
        m_printContext = adoptPtr(new PrintContext(coreFrame));

    drawingArea()->setLayerTreeStateIsFrozen(true);
    m_printContext->begin(printInfo.availablePaperWidth, printInfo.availablePaperHeight);

    float fullPageHeight;
    m_printContext->computePageRects(FloatRect(0, 0, printInfo.availablePaperWidth, printInfo.availablePaperHeight), 0, 0, printInfo.pageSetupScaleFactor, fullPageHeight, true);

#if PLATFORM(GTK)
    if (!m_printOperation)
        m_printOperation = WebPrintOperationGtk::create(this, printInfo);
#endif
}

void WebPage::endPrinting()
{
    drawingArea()->setLayerTreeStateIsFrozen(false);
#if PLATFORM(GTK)
    m_printOperation = 0;
#endif
    m_printContext = nullptr;
}

void WebPage::computePagesForPrinting(uint64_t frameID, const PrintInfo& printInfo, uint64_t callbackID)
{
    Vector<IntRect> resultPageRects;
    double resultTotalScaleFactorForPrinting = 1;

    beginPrinting(frameID, printInfo);

    if (m_printContext) {
        resultPageRects = m_printContext->pageRects();
        resultTotalScaleFactorForPrinting = m_printContext->computeAutomaticScaleFactor(FloatSize(printInfo.availablePaperWidth, printInfo.availablePaperHeight)) * printInfo.pageSetupScaleFactor;
    }
#if PLATFORM(MAC)
    else
        computePagesForPrintingPDFDocument(frameID, printInfo, resultPageRects);
#endif // PLATFORM(MAC)

    // If we're asked to print, we should actually print at least a blank page.
    if (resultPageRects.isEmpty())
        resultPageRects.append(IntRect(0, 0, 1, 1));

    send(Messages::WebPageProxy::ComputedPagesCallback(resultPageRects, resultTotalScaleFactorForPrinting, callbackID));
}

#if PLATFORM(MAC)
void WebPage::drawRectToImage(uint64_t frameID, const PrintInfo& printInfo, const IntRect& rect, const WebCore::IntSize& imageSize, uint64_t callbackID)
{
    WebFrame* frame = WebProcess::shared().webFrame(frameID);
    Frame* coreFrame = frame ? frame->coreFrame() : 0;

    RefPtr<WebImage> image;

#if USE(CG)
    if (coreFrame) {
#if PLATFORM(MAC)
        ASSERT(coreFrame->document()->printing() || pdfDocumentForPrintingFrame(coreFrame));
#else
        ASSERT(coreFrame->document()->printing());
#endif

        RefPtr<ShareableBitmap> bitmap = ShareableBitmap::createShareable(imageSize, ShareableBitmap::SupportsAlpha);
        OwnPtr<GraphicsContext> graphicsContext = bitmap->createGraphicsContext();

        float printingScale = static_cast<float>(imageSize.width()) / rect.width();
        graphicsContext->scale(FloatSize(printingScale, printingScale));

#if PLATFORM(MAC)
        if (RetainPtr<PDFDocument> pdfDocument = pdfDocumentForPrintingFrame(coreFrame)) {
            ASSERT(!m_printContext);
            graphicsContext->scale(FloatSize(1, -1));
            graphicsContext->translate(0, -rect.height());
            drawPDFDocument(graphicsContext->platformContext(), pdfDocument.get(), printInfo, rect);
        } else
#endif
        {
            m_printContext->spoolRect(*graphicsContext, rect);
        }

        image = WebImage::create(bitmap.release());
    }
#endif

    ShareableBitmap::Handle handle;

    if (image)
        image->bitmap()->createHandle(handle, SharedMemory::ReadOnly);

    send(Messages::WebPageProxy::ImageCallback(handle, callbackID));
}

void WebPage::drawPagesToPDF(uint64_t frameID, const PrintInfo& printInfo, uint32_t first, uint32_t count, uint64_t callbackID)
{
    WebFrame* frame = WebProcess::shared().webFrame(frameID);
    Frame* coreFrame = frame ? frame->coreFrame() : 0;

    RetainPtr<CFMutableDataRef> pdfPageData = adoptCF(CFDataCreateMutable(0, 0));

#if USE(CG)
    if (coreFrame) {

#if PLATFORM(MAC)
        ASSERT(coreFrame->document()->printing() || pdfDocumentForPrintingFrame(coreFrame));
#else
        ASSERT(coreFrame->document()->printing());
#endif

        // FIXME: Use CGDataConsumerCreate with callbacks to avoid copying the data.
        RetainPtr<CGDataConsumerRef> pdfDataConsumer = adoptCF(CGDataConsumerCreateWithCFData(pdfPageData.get()));

        CGRect mediaBox = (m_printContext && m_printContext->pageCount()) ? m_printContext->pageRect(0) : CGRectMake(0, 0, printInfo.availablePaperWidth, printInfo.availablePaperHeight);
        RetainPtr<CGContextRef> context = adoptCF(CGPDFContextCreate(pdfDataConsumer.get(), &mediaBox, 0));

#if PLATFORM(MAC)
        if (RetainPtr<PDFDocument> pdfDocument = pdfDocumentForPrintingFrame(coreFrame)) {
            ASSERT(!m_printContext);
            drawPagesToPDFFromPDFDocument(context.get(), pdfDocument.get(), printInfo, first, count);
        } else
#endif
        {
            size_t pageCount = m_printContext->pageCount();
            for (uint32_t page = first; page < first + count; ++page) {
                if (page >= pageCount)
                    break;

                RetainPtr<CFDictionaryRef> pageInfo = adoptCF(CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
                CGPDFContextBeginPage(context.get(), pageInfo.get());

                GraphicsContext ctx(context.get());
                ctx.scale(FloatSize(1, -1));
                ctx.translate(0, -m_printContext->pageRect(page).height());
                m_printContext->spoolPage(ctx, page, m_printContext->pageRect(page).width());

                CGPDFContextEndPage(context.get());
            }
        }
        CGPDFContextClose(context.get());
    }
#endif

    send(Messages::WebPageProxy::DataCallback(CoreIPC::DataReference(CFDataGetBytePtr(pdfPageData.get()), CFDataGetLength(pdfPageData.get())), callbackID));
}

#elif PLATFORM(GTK)

void WebPage::drawPagesForPrinting(uint64_t frameID, const PrintInfo& printInfo, uint64_t callbackID)
{
    beginPrinting(frameID, printInfo);
    if (m_printContext && m_printOperation) {
        m_printOperation->startPrint(m_printContext.get(), callbackID);
        return;
    }

    send(Messages::WebPageProxy::VoidCallback(callbackID));
}
#endif

void WebPage::savePDFToFileInDownloadsFolder(const String& suggestedFilename, const String& originatingURLString, const uint8_t* data, unsigned long size)
{
    send(Messages::WebPageProxy::SavePDFToFileInDownloadsFolder(suggestedFilename, originatingURLString, CoreIPC::DataReference(data, size)));
}

#if PLATFORM(MAC)
void WebPage::savePDFToTemporaryFolderAndOpenWithNativeApplication(const String& suggestedFilename, const String& originatingURLString, const uint8_t* data, unsigned long size, const String& pdfUUID)
{
    send(Messages::WebPageProxy::SavePDFToTemporaryFolderAndOpenWithNativeApplication(suggestedFilename, originatingURLString, CoreIPC::DataReference(data, size), pdfUUID));
}
#endif

void WebPage::setMediaVolume(float volume)
{
    m_page->setMediaVolume(volume);
}

void WebPage::setMayStartMediaWhenInWindow(bool mayStartMedia)
{
    if (mayStartMedia == m_mayStartMediaWhenInWindow)
        return;

    m_mayStartMediaWhenInWindow = mayStartMedia;
    if (m_mayStartMediaWhenInWindow && m_page->isOnscreen())
        m_setCanStartMediaTimer.startOneShot(0);
}

void WebPage::runModal()
{
    if (m_isClosed)
        return;
    if (m_isRunningModal)
        return;

    m_isRunningModal = true;
    send(Messages::WebPageProxy::RunModal());
    RunLoop::run();
    ASSERT(!m_isRunningModal);
}

void WebPage::setMemoryCacheMessagesEnabled(bool memoryCacheMessagesEnabled)
{
    m_page->setMemoryCacheClientCallsEnabled(memoryCacheMessagesEnabled);
}

bool WebPage::canHandleRequest(const WebCore::ResourceRequest& request)
{
    if (SchemeRegistry::shouldLoadURLSchemeAsEmptyDocument(request.url().protocol()))
        return true;

#if ENABLE(BLOB)
    if (request.url().protocolIs("blob"))
        return true;
#endif

    return platformCanHandleRequest(request);
}

#if USE(TILED_BACKING_STORE)
void WebPage::commitPageTransitionViewport()
{
    m_drawingArea->setLayerTreeStateIsFrozen(false);
}
#endif

#if PLATFORM(MAC)
void WebPage::handleAlternativeTextUIResult(const String& result)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;
    frame->editor().handleAlternativeTextUIResult(result);
}
#endif

void WebPage::simulateMouseDown(int button, WebCore::IntPoint position, int clickCount, WKEventModifiers modifiers, double time)
{
    mouseEvent(WebMouseEvent(WebMouseEvent::MouseDown, static_cast<WebMouseEvent::Button>(button), position, position, 0, 0, 0, clickCount, static_cast<WebMouseEvent::Modifiers>(modifiers), time));
}

void WebPage::simulateMouseUp(int button, WebCore::IntPoint position, int clickCount, WKEventModifiers modifiers, double time)
{
    mouseEvent(WebMouseEvent(WebMouseEvent::MouseUp, static_cast<WebMouseEvent::Button>(button), position, position, 0, 0, 0, clickCount, static_cast<WebMouseEvent::Modifiers>(modifiers), time));
}

void WebPage::simulateMouseMotion(WebCore::IntPoint position, double time)
{
    mouseEvent(WebMouseEvent(WebMouseEvent::MouseMove, WebMouseEvent::NoButton, position, position, 0, 0, 0, 0, WebMouseEvent::Modifiers(), time));
}

void WebPage::setCompositionForTesting(const String& compositionString, uint64_t from, uint64_t length)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame || !frame->editor().canEdit())
        return;

    Vector<CompositionUnderline> underlines;
    underlines.append(CompositionUnderline(0, compositionString.length(), Color(Color::black), false));
    frame->editor().setComposition(compositionString, underlines, from, from + length);
}

bool WebPage::hasCompositionForTesting()
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    return frame && frame->editor().hasComposition();
}

void WebPage::confirmCompositionForTesting(const String& compositionString)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame || !frame->editor().canEdit())
        return;

    if (compositionString.isNull())
        frame->editor().confirmComposition();
    frame->editor().confirmComposition(compositionString);
}

void WebPage::numWheelEventHandlersChanged(unsigned numWheelEventHandlers)
{
    if (m_numWheelEventHandlers == numWheelEventHandlers)
        return;

    m_numWheelEventHandlers = numWheelEventHandlers;
    recomputeShortCircuitHorizontalWheelEventsState();
}

static bool hasEnabledHorizontalScrollbar(ScrollableArea* scrollableArea)
{
    if (Scrollbar* scrollbar = scrollableArea->horizontalScrollbar())
        return scrollbar->enabled();

    return false;
}

static bool pageContainsAnyHorizontalScrollbars(Frame* mainFrame)
{
    if (FrameView* frameView = mainFrame->view()) {
        if (hasEnabledHorizontalScrollbar(frameView))
            return true;
    }

    for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext()) {
        FrameView* frameView = frame->view();
        if (!frameView)
            continue;

        const HashSet<ScrollableArea*>* scrollableAreas = frameView->scrollableAreas();
        if (!scrollableAreas)
            continue;

        for (HashSet<ScrollableArea*>::const_iterator it = scrollableAreas->begin(), end = scrollableAreas->end(); it != end; ++it) {
            ScrollableArea* scrollableArea = *it;
            if (!scrollableArea->scrollbarsCanBeActive())
                continue;

            if (hasEnabledHorizontalScrollbar(scrollableArea))
                return true;
        }
    }

    return false;
}

void WebPage::recomputeShortCircuitHorizontalWheelEventsState()
{
    bool canShortCircuitHorizontalWheelEvents = !m_numWheelEventHandlers;

    if (canShortCircuitHorizontalWheelEvents) {
        // Check if we have any horizontal scroll bars on the page.
        if (pageContainsAnyHorizontalScrollbars(mainFrame()))
            canShortCircuitHorizontalWheelEvents = false;
    }

    if (m_canShortCircuitHorizontalWheelEvents == canShortCircuitHorizontalWheelEvents)
        return;

    m_canShortCircuitHorizontalWheelEvents = canShortCircuitHorizontalWheelEvents;
    send(Messages::WebPageProxy::SetCanShortCircuitHorizontalWheelEvents(m_canShortCircuitHorizontalWheelEvents));
}

Frame* WebPage::mainFrame() const
{
    return m_page ? m_page->mainFrame() : 0;
}

FrameView* WebPage::mainFrameView() const
{
    if (Frame* frame = mainFrame())
        return frame->view();
    
    return 0;
}

#if ENABLE(PAGE_VISIBILITY_API) || ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
void WebPage::setVisibilityState(uint32_t visibilityState, bool isInitialState)
{
    if (!m_page)
        return;

    WebCore::PageVisibilityState state = static_cast<WebCore::PageVisibilityState>(visibilityState);

#if ENABLE(PAGE_VISIBILITY_API)
    if (m_visibilityState == state)
        return;

    FrameView* view = m_page->mainFrame() ? m_page->mainFrame()->view() : 0;

    if (state == WebCore::PageVisibilityStateVisible) {
        m_page->didMoveOnscreen();
        if (view)
            view->show();
    }

    m_page->setVisibilityState(state, isInitialState);
    m_visibilityState = state;

    if (state == WebCore::PageVisibilityStateHidden) {
        m_page->willMoveOffscreen();
        if (view)
            view->hide();
    }
#endif

#if ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING) && !ENABLE(PAGE_VISIBILITY_API)
    m_page->setVisibilityState(state, isInitialState);
#endif
}
#endif

void WebPage::setThrottled(bool isThrottled)
{
    if (m_page)
        m_page->setThrottled(isThrottled);
}

void WebPage::setScrollingPerformanceLoggingEnabled(bool enabled)
{
    m_scrollingPerformanceLoggingEnabled = enabled;

    FrameView* frameView = m_mainFrame->coreFrame()->view();
    if (!frameView)
        return;

    frameView->setScrollingPerformanceLoggingEnabled(enabled);
}

bool WebPage::canPluginHandleResponse(const ResourceResponse& response)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    uint32_t pluginLoadPolicy;
    bool allowOnlyApplicationPlugins = !m_mainFrame->coreFrame()->loader()->subframeLoader()->allowPlugins(NotAboutToInstantiatePlugin);

    uint64_t pluginProcessToken;
    String newMIMEType;
    String unavailabilityDescription;
    if (!sendSync(Messages::WebPageProxy::FindPlugin(response.mimeType(), PluginProcessTypeNormal, response.url().string(), response.url().string(), response.url().string(), allowOnlyApplicationPlugins), Messages::WebPageProxy::FindPlugin::Reply(pluginProcessToken, newMIMEType, pluginLoadPolicy, unavailabilityDescription)))
        return false;

    return pluginLoadPolicy != PluginModuleBlocked && pluginProcessToken;
#else
    return false;
#endif
}

#if PLATFORM(QT) || PLATFORM(GTK)
static Frame* targetFrameForEditing(WebPage* page)
{
    Frame* targetFrame = page->corePage()->focusController()->focusedOrMainFrame();

    if (!targetFrame)
        return 0;

    Editor& editor = targetFrame->editor();
    if (!editor.canEdit())
        return 0;

    if (editor.hasComposition()) {
        // We should verify the parent node of this IME composition node are
        // editable because JavaScript may delete a parent node of the composition
        // node. In this case, WebKit crashes while deleting texts from the parent
        // node, which doesn't exist any longer.
        if (PassRefPtr<Range> range = editor.compositionRange()) {
            Node* node = range->startContainer();
            if (!node || !node->isContentEditable())
                return 0;
        }
    }
    return targetFrame;
}

void WebPage::confirmComposition(const String& compositionString, int64_t selectionStart, int64_t selectionLength)
{
    Frame* targetFrame = targetFrameForEditing(this);
    if (!targetFrame) {
        send(Messages::WebPageProxy::EditorStateChanged(editorState()));
        return;
    }

    targetFrame->editor().confirmComposition(compositionString);

    if (selectionStart == -1) {
        send(Messages::WebPageProxy::EditorStateChanged(editorState()));
        return;
    }

    Element* scope = targetFrame->selection()->rootEditableElement();
    RefPtr<Range> selectionRange = TextIterator::rangeFromLocationAndLength(scope, selectionStart, selectionLength);
    ASSERT_WITH_MESSAGE(selectionRange, "Invalid selection: [%lld:%lld] in text of length %d", static_cast<long long>(selectionStart), static_cast<long long>(selectionLength), scope->innerText().length());

    if (selectionRange) {
        VisibleSelection selection(selectionRange.get(), SEL_DEFAULT_AFFINITY);
        targetFrame->selection()->setSelection(selection);
    }
    send(Messages::WebPageProxy::EditorStateChanged(editorState()));
}

void WebPage::setComposition(const String& text, Vector<CompositionUnderline> underlines, uint64_t selectionStart, uint64_t selectionEnd, uint64_t replacementStart, uint64_t replacementLength)
{
    Frame* targetFrame = targetFrameForEditing(this);
    if (!targetFrame || !targetFrame->selection()->isContentEditable()) {
        send(Messages::WebPageProxy::EditorStateChanged(editorState()));
        return;
    }

    if (replacementLength > 0) {
        // The layout needs to be uptodate before setting a selection
        targetFrame->document()->updateLayout();

        Element* scope = targetFrame->selection()->rootEditableElement();
        RefPtr<Range> replacementRange = TextIterator::rangeFromLocationAndLength(scope, replacementStart, replacementLength);
        targetFrame->editor().setIgnoreCompositionSelectionChange(true);
        targetFrame->selection()->setSelection(VisibleSelection(replacementRange.get(), SEL_DEFAULT_AFFINITY));
        targetFrame->editor().setIgnoreCompositionSelectionChange(false);
    }

    targetFrame->editor().setComposition(text, underlines, selectionStart, selectionEnd);
    send(Messages::WebPageProxy::EditorStateChanged(editorState()));
}

void WebPage::cancelComposition()
{
    if (Frame* targetFrame = targetFrameForEditing(this))
        targetFrame->editor().cancelComposition();
    send(Messages::WebPageProxy::EditorStateChanged(editorState()));
}
#endif

void WebPage::didChangeSelection()
{
    send(Messages::WebPageProxy::EditorStateChanged(editorState()));
}

void WebPage::setMainFrameInViewSourceMode(bool inViewSourceMode)
{
    m_mainFrame->coreFrame()->setInViewSourceMode(inViewSourceMode);
}

void WebPage::setMinimumLayoutSize(const IntSize& minimumLayoutSize)
{
    if (m_minimumLayoutSize == minimumLayoutSize)
        return;

    m_minimumLayoutSize = minimumLayoutSize;
    if (minimumLayoutSize.width() <= 0) {
        corePage()->mainFrame()->view()->enableAutoSizeMode(false, IntSize(), IntSize());
        return;
    }

    int minimumLayoutWidth = minimumLayoutSize.width();
    int minimumLayoutHeight = std::max(minimumLayoutSize.height(), 1);

    int maximumSize = std::numeric_limits<int>::max();

    corePage()->mainFrame()->view()->enableAutoSizeMode(true, IntSize(minimumLayoutWidth, minimumLayoutHeight), IntSize(maximumSize, maximumSize));
}

bool WebPage::isSmartInsertDeleteEnabled()
{
    return m_page->settings()->smartInsertDeleteEnabled();
}

void WebPage::setSmartInsertDeleteEnabled(bool enabled)
{
    if (m_page->settings()->smartInsertDeleteEnabled() != enabled) {
        m_page->settings()->setSmartInsertDeleteEnabled(enabled);
        setSelectTrailingWhitespaceEnabled(!enabled);
    }
}

bool WebPage::isSelectTrailingWhitespaceEnabled()
{
    return m_page->settings()->selectTrailingWhitespaceEnabled();
}

void WebPage::setSelectTrailingWhitespaceEnabled(bool enabled)
{
    if (m_page->settings()->selectTrailingWhitespaceEnabled() != enabled) {
        m_page->settings()->setSelectTrailingWhitespaceEnabled(enabled);
        setSmartInsertDeleteEnabled(!enabled);
    }
}

bool WebPage::canShowMIMEType(const String& MIMEType) const
{
    if (MIMETypeRegistry::canShowMIMEType(MIMEType))
        return true;

    if (PluginData* pluginData = m_page->pluginData()) {
        if (pluginData->supportsMimeType(MIMEType, PluginData::AllPlugins) && corePage()->mainFrame()->loader()->subframeLoader()->allowPlugins(NotAboutToInstantiatePlugin))
            return true;

        // We can use application plugins even if plugins aren't enabled.
        if (pluginData->supportsMimeType(MIMEType, PluginData::OnlyApplicationPlugins))
            return true;
    }

    return false;
}

void WebPage::addTextCheckingRequest(uint64_t requestID, PassRefPtr<TextCheckingRequest> request)
{
    m_pendingTextCheckingRequestMap.add(requestID, request);
}

void WebPage::didFinishCheckingText(uint64_t requestID, const Vector<TextCheckingResult>& result)
{
    TextCheckingRequest* request = m_pendingTextCheckingRequestMap.get(requestID);
    if (!request)
        return;

    request->didSucceed(result);
    m_pendingTextCheckingRequestMap.remove(requestID);
}

void WebPage::didCancelCheckingText(uint64_t requestID)
{
    TextCheckingRequest* request = m_pendingTextCheckingRequestMap.get(requestID);
    if (!request)
        return;

    request->didCancel();
    m_pendingTextCheckingRequestMap.remove(requestID);
}

void WebPage::didCommitLoad(WebFrame* frame)
{
    if (!frame->isMainFrame())
        return;

    // If previous URL is invalid, then it's not a real page that's being navigated away from.
    // Most likely, this is actually the first load to be committed in this page.
    if (frame->coreFrame()->loader()->previousURL().isValid())
        reportUsedFeatures();

    // Only restore the scale factor for standard frame loads (of the main frame).
    if (frame->coreFrame()->loader()->loadType() == FrameLoadTypeStandard) {
        Page* page = frame->coreFrame()->page();
        if (page && page->pageScaleFactor() != 1)
            scalePage(1, IntPoint());
    }

#if ENABLE(PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC)
    resetPrimarySnapshottedPlugIn();
#endif

    WebProcess::shared().updateActivePages();
}

void WebPage::didFinishLoad(WebFrame* frame)
{
#if ENABLE(PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC)
    if (!frame->isMainFrame())
        return;

    m_readyToFindPrimarySnapshottedPlugin = true;
    m_determinePrimarySnapshottedPlugInTimer.startOneShot(0);
#else
    UNUSED_PARAM(frame);
#endif
}

#if ENABLE(PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC)
static int primarySnapshottedPlugInSearchLimit = 3000;
static int primarySnapshottedPlugInSearchGap = 200;
static float primarySnapshottedPlugInSearchBucketSize = 1.1;
static int primarySnapshottedPlugInMinimumWidth = 400;
static int primarySnapshottedPlugInMinimumHeight = 300;

#if ENABLE(PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC)
void WebPage::determinePrimarySnapshottedPlugInTimerFired()
{
    if (!m_page)
        return;
    
    Settings* settings = m_page->settings();
    if (!settings->snapshotAllPlugIns() && settings->primaryPlugInSnapshotDetectionEnabled())
        determinePrimarySnapshottedPlugIn();
}
#endif

void WebPage::determinePrimarySnapshottedPlugIn()
{
    if (!m_page->settings()->plugInSnapshottingEnabled())
        return;

    if (!m_readyToFindPrimarySnapshottedPlugin)
        return;

    if (m_pluginViews.isEmpty())
        return;

    if (m_didFindPrimarySnapshottedPlugin)
        return;

    RenderView* renderView = corePage()->mainFrame()->view()->renderView();

    IntRect searchRect = IntRect(IntPoint(), corePage()->mainFrame()->view()->contentsSize());
    searchRect.intersect(IntRect(IntPoint(), IntSize(primarySnapshottedPlugInSearchLimit, primarySnapshottedPlugInSearchLimit)));

    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::AllowChildFrameContent | HitTestRequest::IgnoreClipping | HitTestRequest::DisallowShadowContent);

    HashSet<RenderObject*> seenRenderers;
    HTMLPlugInImageElement* candidatePlugIn = 0;
    unsigned candidatePlugInArea = 0;

    for (int x = searchRect.x(); x <= searchRect.width(); x += primarySnapshottedPlugInSearchGap) {
        for (int y = searchRect.y(); y <= searchRect.height(); y += primarySnapshottedPlugInSearchGap) {
            HitTestResult hitTestResult = HitTestResult(LayoutPoint(x, y));
            renderView->hitTest(request, hitTestResult);

            Element* element = hitTestResult.innerElement();
            if (!element)
                continue;

            RenderObject* renderer = element->renderer();
            if (!renderer || !renderer->isBox())
                continue;

            RenderBox* renderBox = toRenderBox(renderer);

            if (!seenRenderers.add(renderer).isNewEntry)
                continue;

            if (!element->isPluginElement())
                continue;

            HTMLPlugInElement* plugInElement = toHTMLPlugInElement(element);
            if (!plugInElement->isPlugInImageElement())
                continue;

            HTMLPlugInImageElement* plugInImageElement = toHTMLPlugInImageElement(plugInElement);

            if (plugInElement->displayState() == HTMLPlugInElement::Playing)
                continue;

            if (renderBox->contentWidth() < primarySnapshottedPlugInMinimumWidth || renderBox->contentHeight() < primarySnapshottedPlugInMinimumHeight)
                continue;

            LayoutUnit contentArea = renderBox->contentWidth() * renderBox->contentHeight();

            if (contentArea > candidatePlugInArea * primarySnapshottedPlugInSearchBucketSize) {
                candidatePlugIn = plugInImageElement;
                candidatePlugInArea = contentArea;
            }
        }
    }

    if (!candidatePlugIn)
        return;

    m_didFindPrimarySnapshottedPlugin = true;
    m_primaryPlugInPageOrigin = m_page->mainFrame()->document()->baseURL().host();
    m_primaryPlugInOrigin = candidatePlugIn->loadedUrl().host();
    m_primaryPlugInMimeType = candidatePlugIn->loadedMimeType();

    candidatePlugIn->setIsPrimarySnapshottedPlugIn(true);
}

void WebPage::resetPrimarySnapshottedPlugIn()
{
    m_readyToFindPrimarySnapshottedPlugin = false;
    m_didFindPrimarySnapshottedPlugin = false;
}

bool WebPage::matchesPrimaryPlugIn(const String& pageOrigin, const String& pluginOrigin, const String& mimeType) const
{
    if (!m_didFindPrimarySnapshottedPlugin)
        return false;

    return (pageOrigin == m_primaryPlugInPageOrigin && pluginOrigin == m_primaryPlugInOrigin && mimeType == m_primaryPlugInMimeType);
}
#endif // ENABLE(PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC)

PassRefPtr<Range> WebPage::currentSelectionAsRange()
{
    Frame* frame = frameWithSelection(m_page.get());
    if (!frame)
        return 0;

    return frame->selection()->toNormalizedRange();
}

void WebPage::reportUsedFeatures()
{
    // FIXME: Feature names should not be hardcoded.
    const BitVector* features = m_page->featureObserver()->accumulatedFeatureBits();
    Vector<String> namedFeatures;
    if (features && features->quickGet(FeatureObserver::SharedWorkerStart))
        namedFeatures.append("SharedWorker");

    m_loaderClient.featuresUsedInPage(this, namedFeatures);
}

unsigned WebPage::extendIncrementalRenderingSuppression()
{
    unsigned token = m_maximumRenderingSuppressionToken + 1;
    while (!HashSet<unsigned>::isValidValue(token) || m_activeRenderingSuppressionTokens.contains(token))
        token++;

    m_activeRenderingSuppressionTokens.add(token);
    m_page->mainFrame()->view()->setVisualUpdatesAllowedByClient(false);

    m_maximumRenderingSuppressionToken = token;

    return token;
}

void WebPage::stopExtendingIncrementalRenderingSuppression(unsigned token)
{
    if (!m_activeRenderingSuppressionTokens.contains(token))
        return;

    m_activeRenderingSuppressionTokens.remove(token);
    m_page->mainFrame()->view()->setVisualUpdatesAllowedByClient(!shouldExtendIncrementalRenderingSuppression());
}
    
void WebPage::setScrollPinningBehavior(uint32_t pinning)
{
    m_scrollPinningBehavior = static_cast<ScrollPinningBehavior>(pinning);
    m_page->mainFrame()->view()->setScrollPinningBehavior(m_scrollPinningBehavior);
}

} // namespace WebKit
