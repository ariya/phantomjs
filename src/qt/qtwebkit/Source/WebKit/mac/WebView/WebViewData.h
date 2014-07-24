/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Igalia S.L
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "WebTypesInternal.h"
#import "WebDelegateImplementationCaching.h"
#import <WebCore/AlternativeTextClient.h>
#import <WebCore/LayerFlushScheduler.h>
#import <WebCore/LayerFlushSchedulerClient.h>
#import <WebCore/WebCoreKeyboardUIMode.h>
#import <wtf/HashMap.h>
#import <wtf/PassOwnPtr.h>
#import <wtf/RetainPtr.h>
#import <wtf/text/WTFString.h>

namespace WebCore {
class AlternativeTextUIController;
class HistoryItem;
class Page;
}

@class WebInspector;
@class WebNodeHighlight;
@class WebPluginDatabase;
@class WebPreferences;
@class WebTextCompletionController;
@protocol WebFormDelegate;
@protocol WebDeviceOrientationProvider;
@protocol WebGeolocationProvider;
@protocol WebNotificationProvider;
#if ENABLE(VIDEO)
@class WebVideoFullscreenController;
#endif
#if ENABLE(FULLSCREEN_API)
@class WebFullScreenController;
#endif

extern BOOL applicationIsTerminating;
extern int pluginDatabaseClientCount;

#if USE(ACCELERATED_COMPOSITING)
class LayerFlushController;

class WebViewLayerFlushScheduler : public WebCore::LayerFlushScheduler {
public:
    WebViewLayerFlushScheduler(LayerFlushController*);
    virtual ~WebViewLayerFlushScheduler() { }

private:
    virtual void runLoopObserverCallback() OVERRIDE
    {
        RefPtr<LayerFlushController> protector = m_flushController;
        WebCore::LayerFlushScheduler::runLoopObserverCallback();
    }
    
    LayerFlushController* m_flushController;
};

class LayerFlushController : public RefCounted<LayerFlushController>, public WebCore::LayerFlushSchedulerClient {
public:
    static PassRefPtr<LayerFlushController> create(WebView* webView)
    {
        return adoptRef(new LayerFlushController(webView));
    }
    
    virtual bool flushLayers();
    
    void scheduleLayerFlush();
    void invalidate();
    
private:
    LayerFlushController(WebView*);
    
    WebView* m_webView;
    WebViewLayerFlushScheduler m_layerFlushScheduler;
};
#endif

// FIXME: This should be renamed to WebViewData.
@interface WebViewPrivate : NSObject {
@public
    WebCore::Page* page;
    
    id UIDelegate;
    id UIDelegateForwarder;
    id resourceProgressDelegate;
    id downloadDelegate;
    id policyDelegate;
    id policyDelegateForwarder;
    id frameLoadDelegate;
    id frameLoadDelegateForwarder;
    id <WebFormDelegate> formDelegate;
    id editingDelegate;
    id editingDelegateForwarder;
    id scriptDebugDelegate;
    id historyDelegate;

    WebInspector *inspector;
    WebNodeHighlight *currentNodeHighlight;

    BOOL allowsUndo;
        
    float zoomMultiplier;
    BOOL zoomsTextOnly;

    NSString *applicationNameForUserAgent;
    WTF::String userAgent;
    BOOL userAgentOverridden;
    
    WebPreferences *preferences;
    BOOL useSiteSpecificSpoofing;

    NSWindow *hostWindow;

    int programmaticFocusCount;
    
    WebResourceDelegateImplementationCache resourceLoadDelegateImplementations;
    WebFrameLoadDelegateImplementationCache frameLoadDelegateImplementations;
    WebScriptDebugDelegateImplementationCache scriptDebugDelegateImplementations;
    WebHistoryDelegateImplementationCache historyDelegateImplementations;

    void *observationInfo;
    
    BOOL closed;
    BOOL shouldCloseWithWindow;
    BOOL mainFrameDocumentReady;
    BOOL drawsBackground;
    BOOL tabKeyCyclesThroughElementsChanged;
    BOOL becomingFirstResponder;
    BOOL becomingFirstResponderFromOutside;
    BOOL usesPageCache;

    NSColor *backgroundColor;

    NSString *mediaStyle;
    
    BOOL hasSpellCheckerDocumentTag;
    NSInteger spellCheckerDocumentTag;

#if ENABLE(DASHBOARD_SUPPORT)
    BOOL dashboardBehaviorAlwaysSendMouseEventsToAllWindows;
    BOOL dashboardBehaviorAlwaysSendActiveNullEventsToPlugIns;
    BOOL dashboardBehaviorAlwaysAcceptsFirstMouse;
    BOOL dashboardBehaviorAllowWheelScrolling;
#endif
    
    // WebKit has both a global plug-in database and a separate, per WebView plug-in database. Dashboard uses the per WebView database.
    WebPluginDatabase *pluginDatabase;
    
    HashMap<unsigned long, RetainPtr<id> > identifierMap;

    BOOL _keyboardUIModeAccessed;
    WebCore::KeyboardUIMode _keyboardUIMode;

    BOOL shouldUpdateWhileOffscreen;

    BOOL includesFlattenedCompositingLayersWhenDrawingToBitmap;
    
#if USE(ACCELERATED_COMPOSITING)
    // When this flag is set, next time a WebHTMLView draws, it needs to temporarily disable screen updates
    // so that the NSView drawing is visually synchronized with CALayer updates.
    BOOL needsOneShotDrawingSynchronization;
    BOOL postsAcceleratedCompositingNotifications;
    RefPtr<LayerFlushController> layerFlushController;
#endif

    NSPasteboard *insertionPasteboard;
            
    NSSize lastLayoutSize;

#if ENABLE(VIDEO)
    WebVideoFullscreenController *fullscreenController;
#endif
    
#if ENABLE(FULLSCREEN_API)
    WebFullScreenController *newFullscreenController;
#endif

#if USE(GLIB)
    CFRunLoopObserverRef glibRunLoopObserver;
#endif
    id<WebGeolocationProvider> _geolocationProvider;
    id<WebDeviceOrientationProvider> m_deviceOrientationProvider;
    id<WebNotificationProvider> _notificationProvider;

    RefPtr<WebCore::HistoryItem> _globalHistoryItem;

    BOOL interactiveFormValidationEnabled;
    int validationMessageTimerMagnification;

    float customDeviceScaleFactor;

#if USE(DICTATION_ALTERNATIVES)
    OwnPtr<WebCore::AlternativeTextUIController> m_alternativeTextUIController;
#endif

    RetainPtr<NSData> sourceApplicationAuditData;
}
@end
