/*
 * Copyright (C) 2010, 2012 Apple Inc. All rights reserved.
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

#ifndef PluginView_h
#define PluginView_h

#include "NPRuntimeObjectMap.h"
#include "Plugin.h"
#include "PluginController.h"
#include "WebFrame.h"
#include <WebCore/FindOptions.h>
#include <WebCore/Image.h>
#include <WebCore/MediaCanStartListener.h>
#include <WebCore/PluginViewBase.h>
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceResponse.h>
#include <WebCore/RunLoop.h>
#include <WebCore/Timer.h>
#include <wtf/Deque.h>

// FIXME: Eventually this should move to WebCore.

namespace WebCore {
class Frame;
class HTMLPlugInElement;
class MouseEvent;
class RenderBoxModelObject;
}

namespace WebKit {

class WebEvent;

class PluginView : public WebCore::PluginViewBase, public PluginController, private WebCore::MediaCanStartListener, private WebFrame::LoadListener {
public:
    static PassRefPtr<PluginView> create(PassRefPtr<WebCore::HTMLPlugInElement>, PassRefPtr<Plugin>, const Plugin::Parameters&);

    void recreateAndInitialize(PassRefPtr<Plugin>);

    WebCore::Frame* frame() const;

    bool isBeingDestroyed() const { return m_isBeingDestroyed; }

    void manualLoadDidReceiveResponse(const WebCore::ResourceResponse&);
    void manualLoadDidReceiveData(const char* bytes, int length);
    void manualLoadDidFinishLoading();
    void manualLoadDidFail(const WebCore::ResourceError&);

#if PLATFORM(MAC)
    void setWindowIsVisible(bool);
    void setWindowIsFocused(bool);
    void setDeviceScaleFactor(float);
    void windowAndViewFramesChanged(const WebCore::FloatRect& windowFrameInScreenCoordinates, const WebCore::FloatRect& viewFrameInWindowCoordinates);
    bool sendComplexTextInput(uint64_t pluginComplexTextInputIdentifier, const String& textInput);
    void setLayerHostingMode(LayerHostingMode);
    RetainPtr<PDFDocument> pdfDocumentForPrinting() const { return m_plugin->pdfDocumentForPrinting(); }
    NSObject *accessibilityObject() const;
#endif

    WebCore::HTMLPlugInElement* pluginElement() const { return m_pluginElement.get(); }
    const Plugin::Parameters& initialParameters() const { return m_parameters; }

    // FIXME: Remove this; nobody should have to know about the plug-in view's renderer except the plug-in view itself.
    WebCore::RenderBoxModelObject* renderer() const;
    
    void setPageScaleFactor(double scaleFactor, WebCore::IntPoint origin);
    double pageScaleFactor() const;
    bool handlesPageScaleFactor() const;

    void pageScaleFactorDidChange();
    void webPageDestroyed();

    bool handleEditingCommand(const String& commandName, const String& argument);
    bool isEditingCommandEnabled(const String& commandName);
    
    unsigned countFindMatches(const String& target, WebCore::FindOptions, unsigned maxMatchCount);
    bool findString(const String& target, WebCore::FindOptions, unsigned maxMatchCount);

    String getSelectionString() const;

    bool shouldAllowScripting();

    PassRefPtr<WebCore::SharedBuffer> liveResourceData() const;
    bool performDictionaryLookupAtLocation(const WebCore::FloatPoint&);

private:
    PluginView(PassRefPtr<WebCore::HTMLPlugInElement>, PassRefPtr<Plugin>, const Plugin::Parameters& parameters);
    virtual ~PluginView();

    void initializePlugin();

    void viewGeometryDidChange();
    void viewVisibilityDidChange();
    WebCore::IntRect clipRectInWindowCoordinates() const;
    void focusPluginElement();
    
    void pendingURLRequestsTimerFired();
    class URLRequest;
    void performURLRequest(URLRequest*);

    // Perform a URL request where the frame target is not null.
    void performFrameLoadURLRequest(URLRequest*);

    // Perform a URL request where the URL protocol is "javascript:".
    void performJavaScriptURLRequest(URLRequest*);

    class Stream;
    void addStream(Stream*);
    void removeStream(Stream*);
    void cancelAllStreams();

    void redeliverManualStream();

    void pluginSnapshotTimerFired(WebCore::DeferrableOneShotTimer<PluginView>*);
    void pluginDidReceiveUserInteraction();

    bool shouldCreateTransientPaintingSnapshot() const;

    // WebCore::PluginViewBase
#if PLATFORM(MAC)
    virtual PlatformLayer* platformLayer() const;
#endif
    virtual JSC::JSObject* scriptObject(JSC::JSGlobalObject*);
    virtual void storageBlockingStateChanged();
    virtual void privateBrowsingStateChanged(bool);
    virtual bool getFormValue(String&);
    virtual bool scroll(WebCore::ScrollDirection, WebCore::ScrollGranularity);
    virtual WebCore::Scrollbar* horizontalScrollbar();
    virtual WebCore::Scrollbar* verticalScrollbar();
    virtual bool wantsWheelEvents();
    virtual bool shouldAlwaysAutoStart() const OVERRIDE;
    virtual void beginSnapshottingRunningPlugin() OVERRIDE;
    virtual bool shouldAllowNavigationFromDrags() const OVERRIDE;
    virtual bool shouldNotAddLayer() const OVERRIDE;

    // WebCore::Widget
    virtual void setFrameRect(const WebCore::IntRect&);
    virtual void paint(WebCore::GraphicsContext*, const WebCore::IntRect&);
    virtual void invalidateRect(const WebCore::IntRect&);
    virtual void setFocus(bool);
    virtual void frameRectsChanged();
    virtual void setParent(WebCore::ScrollView*);
    virtual void handleEvent(WebCore::Event*);
    virtual void notifyWidget(WebCore::WidgetNotification);
    virtual void show();
    virtual void hide();
    virtual bool transformsAffectFrameRect();
    virtual void clipRectChanged() OVERRIDE;

    // WebCore::MediaCanStartListener
    virtual void mediaCanStart();

    // PluginController
    virtual bool isPluginVisible();
    virtual void invalidate(const WebCore::IntRect&);
    virtual String userAgent();
    virtual void loadURL(uint64_t requestID, const String& method, const String& urlString, const String& target, 
                         const WebCore::HTTPHeaderMap& headerFields, const Vector<uint8_t>& httpBody, bool allowPopups);
    virtual void cancelStreamLoad(uint64_t streamID);
    virtual void cancelManualStreamLoad();
#if ENABLE(NETSCAPE_PLUGIN_API)
    virtual NPObject* windowScriptNPObject();
    virtual NPObject* pluginElementNPObject();
    virtual bool evaluate(NPObject*, const String&scriptString, NPVariant* result, bool allowPopups);
#endif
    virtual void setStatusbarText(const String&);
    virtual bool isAcceleratedCompositingEnabled();
    virtual void pluginProcessCrashed();
    virtual void willSendEventToPlugin();
#if PLATFORM(MAC)
    virtual void pluginFocusOrWindowFocusChanged(bool pluginHasFocusAndWindowHasFocus);
    virtual void setComplexTextInputState(PluginComplexTextInputState);
    virtual mach_port_t compositingRenderServerPort();
    virtual void openPluginPreferencePane() OVERRIDE;
#endif
    virtual float contentsScaleFactor();
    virtual String proxiesForURL(const String&);
    virtual String cookiesForURL(const String&);
    virtual void setCookiesForURL(const String& urlString, const String& cookieString);
    virtual bool getAuthenticationInfo(const WebCore::ProtectionSpace&, String& username, String& password);
    virtual bool isPrivateBrowsingEnabled();
    virtual bool asynchronousPluginInitializationEnabled() const;
    virtual bool asynchronousPluginInitializationEnabledForAllPlugins() const;
    virtual bool artificialPluginInitializationDelayEnabled() const;
    virtual void protectPluginFromDestruction();
    virtual void unprotectPluginFromDestruction();
#if PLUGIN_ARCHITECTURE(X11)
    virtual uint64_t createPluginContainer();
    virtual void windowedPluginGeometryDidChange(const WebCore::IntRect& frameRect, const WebCore::IntRect& clipRect, uint64_t windowID);
#endif

    virtual void didInitializePlugin();
    virtual void didFailToInitializePlugin();
    void destroyPluginAndReset();

    // WebFrame::LoadListener
    virtual void didFinishLoad(WebFrame*);
    virtual void didFailLoad(WebFrame*, bool wasCancelled);

    PassOwnPtr<WebEvent> createWebEvent(WebCore::MouseEvent*) const;

    RefPtr<WebCore::HTMLPlugInElement> m_pluginElement;
    RefPtr<Plugin> m_plugin;
    WebPage* m_webPage;
    Plugin::Parameters m_parameters;
    
    bool m_isInitialized;
    bool m_isWaitingForSynchronousInitialization;
    bool m_isWaitingUntilMediaCanStart;
    bool m_isBeingDestroyed;
    bool m_pluginProcessHasCrashed;

    // Pending URLRequests that the plug-in has made.
    Deque<RefPtr<URLRequest> > m_pendingURLRequests;
    WebCore::RunLoop::Timer<PluginView> m_pendingURLRequestsTimer;

    // Pending frame loads that the plug-in has made.
    typedef HashMap<RefPtr<WebFrame>, RefPtr<URLRequest> > FrameLoadMap;
    FrameLoadMap m_pendingFrameLoads;

    // Streams that the plug-in has requested to load. 
    HashMap<uint64_t, RefPtr<Stream> > m_streams;

#if ENABLE(NETSCAPE_PLUGIN_API)
    // A map of all related NPObjects for this plug-in view.
    NPRuntimeObjectMap m_npRuntimeObjectMap;
#endif

    // The manual stream state. This is used so we can deliver a manual stream to a plug-in
    // when it is initialized.
    enum ManualStreamState {
        StreamStateInitial,
        StreamStateHasReceivedResponse,
        StreamStateFinished,
        StreamStateFailed
    };
    ManualStreamState m_manualStreamState;

    WebCore::ResourceResponse m_manualStreamResponse;
    WebCore::ResourceError m_manualStreamError;
    RefPtr<WebCore::SharedBuffer> m_manualStreamData;
    
    // This snapshot is used to avoid side effects should the plugin run JS during painting.
    RefPtr<ShareableBitmap> m_transientPaintingSnapshot;
    // This timer is used when plugin snapshotting is enabled, to capture a plugin placeholder.
    WebCore::DeferrableOneShotTimer<PluginView> m_pluginSnapshotTimer;
    unsigned m_countSnapshotRetries;
    bool m_didReceiveUserInteraction;

    double m_pageScaleFactor;
};

} // namespace WebKit

#endif // PluginView_h
