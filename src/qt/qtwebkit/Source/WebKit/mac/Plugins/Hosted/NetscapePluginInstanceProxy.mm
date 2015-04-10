/*
 * Copyright (C) 2008, 2009, 2010 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#if USE(PLUGIN_HOST_PROCESS) && ENABLE(NETSCAPE_PLUGIN_API)

#import "NetscapePluginInstanceProxy.h"

#import "HostedNetscapePluginStream.h"
#import "NetscapePluginHostProxy.h"
#import "ProxyInstance.h"
#import "ProxyRuntimeObject.h"
#import "WebDataSourceInternal.h"
#import "WebFrameInternal.h"
#import "WebHostedNetscapePluginView.h"
#import "WebKitNSStringExtras.h"
#import "WebNSDataExtras.h"
#import "WebNSURLExtras.h"
#import "WebPluginRequest.h"
#import "WebUIDelegate.h"
#import "WebUIDelegatePrivate.h"
#import "WebViewInternal.h"
#import <JavaScriptCore/Completion.h>
#import <JavaScriptCore/Error.h>
#import <JavaScriptCore/JSLock.h>
#import <JavaScriptCore/PropertyNameArray.h>
#import <JavaScriptCore/SourceCode.h>
#import <JavaScriptCore/StrongInlines.h>
#import <WebCore/CookieJar.h>
#import <WebCore/DocumentLoader.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/FrameTree.h>
#import <WebCore/KURL.h>
#import <WebCore/ProxyServer.h>
#import <WebCore/SecurityOrigin.h>
#import <WebCore/ScriptController.h>
#import <WebCore/ScriptValue.h>
#import <WebCore/UserGestureIndicator.h>
#import <WebCore/npruntime_impl.h>
#import <WebCore/runtime_object.h>
#import <WebKitSystemInterface.h>
#import <mach/mach.h>
#import <utility>
#import <wtf/RefCountedLeakCounter.h>
#import <wtf/text/CString.h>

extern "C" {
#import "WebKitPluginClientServer.h"
#import "WebKitPluginHost.h"
}

using namespace JSC;
using namespace JSC::Bindings;
using namespace WebCore;

namespace WebKit {

class NetscapePluginInstanceProxy::PluginRequest : public RefCounted<NetscapePluginInstanceProxy::PluginRequest> {
public:
    static PassRefPtr<PluginRequest> create(uint32_t requestID, NSURLRequest* request, NSString* frameName, bool allowPopups)
    {
        return adoptRef(new PluginRequest(requestID, request, frameName, allowPopups));
    }

    uint32_t requestID() const { return m_requestID; }
    NSURLRequest* request() const { return m_request.get(); }
    NSString* frameName() const { return m_frameName.get(); }
    bool allowPopups() const { return m_allowPopups; }
    
private:
    PluginRequest(uint32_t requestID, NSURLRequest* request, NSString* frameName, bool allowPopups)
        : m_requestID(requestID)
        , m_request(request)
        , m_frameName(frameName)
        , m_allowPopups(allowPopups)
    {
    }
    
    uint32_t m_requestID;
    RetainPtr<NSURLRequest*> m_request;
    RetainPtr<NSString*> m_frameName;
    bool m_allowPopups;
};

NetscapePluginInstanceProxy::LocalObjectMap::LocalObjectMap()
    : m_objectIDCounter(0)
{
}

NetscapePluginInstanceProxy::LocalObjectMap::~LocalObjectMap()
{
}

inline bool NetscapePluginInstanceProxy::LocalObjectMap::contains(uint32_t objectID) const
{
    return m_idToJSObjectMap.contains(objectID);
}

inline JSC::JSObject* NetscapePluginInstanceProxy::LocalObjectMap::get(uint32_t objectID) const
{
    if (objectID == HashTraits<uint32_t>::emptyValue() || HashTraits<uint32_t>::isDeletedValue(objectID))
        return 0;

    return m_idToJSObjectMap.get(objectID).get();
}

uint32_t NetscapePluginInstanceProxy::LocalObjectMap::idForObject(VM& vm, JSObject* object)
{
    // This method creates objects with refcount of 1, but doesn't increase refcount when returning
    // found objects. This extra count accounts for the main "reference" kept by plugin process.

    // To avoid excessive IPC, plugin process doesn't send each NPObject release/retain call to
    // Safari. It only sends one when the last reference is removed, and it can destroy the proxy
    // NPObject.

    // However, the browser may be sending the same object out to plug-in as a function call
    // argument at the same time - neither side can know what the other one is doing. So,
    // is to make PCForgetBrowserObject call return a boolean result, making it possible for 
    // the browser to make plugin host keep the proxy with zero refcount for a little longer.

    uint32_t objectID = 0;
    
    HashMap<JSC::JSObject*, pair<uint32_t, uint32_t> >::iterator iter = m_jsObjectToIDMap.find(object);
    if (iter != m_jsObjectToIDMap.end())
        return iter->value.first;
    
    do {
        objectID = ++m_objectIDCounter;
    } while (!m_objectIDCounter || m_objectIDCounter == static_cast<uint32_t>(-1) || m_idToJSObjectMap.contains(objectID));

    m_idToJSObjectMap.set(objectID, Strong<JSObject>(vm, object));
    m_jsObjectToIDMap.set(object, std::make_pair(objectID, 1));

    return objectID;
}

void NetscapePluginInstanceProxy::LocalObjectMap::retain(JSC::JSObject* object)
{
    HashMap<JSC::JSObject*, pair<uint32_t, uint32_t> >::iterator iter = m_jsObjectToIDMap.find(object);
    ASSERT(iter != m_jsObjectToIDMap.end());

    iter->value.second = iter->value.second + 1;
}

void NetscapePluginInstanceProxy::LocalObjectMap::release(JSC::JSObject* object)
{
    HashMap<JSC::JSObject*, pair<uint32_t, uint32_t> >::iterator iter = m_jsObjectToIDMap.find(object);
    ASSERT(iter != m_jsObjectToIDMap.end());

    ASSERT(iter->value.second > 0);
    iter->value.second = iter->value.second - 1;
    if (!iter->value.second) {
        m_idToJSObjectMap.remove(iter->value.first);
        m_jsObjectToIDMap.remove(iter);
    }
}

void NetscapePluginInstanceProxy::LocalObjectMap::clear()
{
    m_idToJSObjectMap.clear();
    m_jsObjectToIDMap.clear();
}

bool NetscapePluginInstanceProxy::LocalObjectMap::forget(uint32_t objectID)
{
    if (objectID == HashTraits<uint32_t>::emptyValue() || HashTraits<uint32_t>::isDeletedValue(objectID)) {
        LOG_ERROR("NetscapePluginInstanceProxy::LocalObjectMap::forget: local object id %u is not valid.", objectID);
        return true;
    }

    HashMap<uint32_t, JSC::Strong<JSC::JSObject> >::iterator iter = m_idToJSObjectMap.find(objectID);
    if (iter == m_idToJSObjectMap.end()) {
        LOG_ERROR("NetscapePluginInstanceProxy::LocalObjectMap::forget: local object %u doesn't exist.", objectID);
        return true;
    }

    HashMap<JSC::JSObject*, pair<uint32_t, uint32_t> >::iterator rIter = m_jsObjectToIDMap.find(iter->value.get());

    // If the object is being sent to plug-in right now, then it's not the time to forget.
    if (rIter->value.second != 1)
        return false;

    m_jsObjectToIDMap.remove(rIter);
    m_idToJSObjectMap.remove(iter);
    return true;
}

static uint32_t pluginIDCounter;

bool NetscapePluginInstanceProxy::m_inDestroy;

DEFINE_DEBUG_ONLY_GLOBAL(WTF::RefCountedLeakCounter, netscapePluginInstanceProxyCounter, ("NetscapePluginInstanceProxy"));

NetscapePluginInstanceProxy::NetscapePluginInstanceProxy(NetscapePluginHostProxy* pluginHostProxy, WebHostedNetscapePluginView *pluginView, bool fullFramePlugin)
    : m_pluginHostProxy(pluginHostProxy)
    , m_pluginView(pluginView)
    , m_requestTimer(this, &NetscapePluginInstanceProxy::requestTimerFired)
    , m_currentURLRequestID(0)
    , m_renderContextID(0)
    , m_rendererType(UseSoftwareRenderer)
    , m_waitingForReply(false)
    , m_urlCheckCounter(0)
    , m_pluginFunctionCallDepth(0)
    , m_shouldStopSoon(false)
    , m_currentRequestID(0)
    , m_pluginIsWaitingForDraw(false)
{
    ASSERT(m_pluginView);
    
    if (fullFramePlugin) {
        // For full frame plug-ins, the first requestID will always be the one for the already
        // open stream.
        ++m_currentURLRequestID;
    }
    
    // Assign a plug-in ID.
    do {
        m_pluginID = ++pluginIDCounter;
    } while (pluginHostProxy->pluginInstance(m_pluginID) || !m_pluginID);

#ifndef NDEBUG
    netscapePluginInstanceProxyCounter.increment();
#endif
}

PassRefPtr<NetscapePluginInstanceProxy> NetscapePluginInstanceProxy::create(NetscapePluginHostProxy* pluginHostProxy, WebHostedNetscapePluginView *pluginView, bool fullFramePlugin)
{
    RefPtr<NetscapePluginInstanceProxy> proxy = adoptRef(new NetscapePluginInstanceProxy(pluginHostProxy, pluginView, fullFramePlugin));
    pluginHostProxy->addPluginInstance(proxy.get());
    return proxy.release();
}

NetscapePluginInstanceProxy::~NetscapePluginInstanceProxy()
{
    ASSERT(!m_pluginHostProxy);
    
    m_pluginID = 0;
    deleteAllValues(m_replies);

#ifndef NDEBUG
    netscapePluginInstanceProxyCounter.decrement();
#endif
}

void NetscapePluginInstanceProxy::resize(NSRect size, NSRect clipRect)
{
    uint32_t requestID = 0;
    
    requestID = nextRequestID();

    _WKPHResizePluginInstance(m_pluginHostProxy->port(), m_pluginID, requestID,
                              size.origin.x, size.origin.y, size.size.width, size.size.height,
                              clipRect.origin.x, clipRect.origin.y, clipRect.size.width, clipRect.size.height);
    
    waitForReply<NetscapePluginInstanceProxy::BooleanReply>(requestID);
}

void NetscapePluginInstanceProxy::stopAllStreams()
{
    Vector<RefPtr<HostedNetscapePluginStream> > streamsCopy;
    copyValuesToVector(m_streams, streamsCopy);
    for (size_t i = 0; i < streamsCopy.size(); i++)
        streamsCopy[i]->stop();
}

void NetscapePluginInstanceProxy::cleanup()
{
    stopAllStreams();
    
    m_requestTimer.stop();
    
    // Clear the object map, this will cause any outstanding JS objects that the plug-in had a reference to 
    // to go away when the next garbage collection takes place.
    m_localObjects.clear();
    
    if (Frame* frame = core([m_pluginView webFrame]))
        frame->script()->cleanupScriptObjectsForPlugin(m_pluginView);
    
    ProxyInstanceSet instances;
    instances.swap(m_instances);
    
    // Invalidate all proxy instances.
    ProxyInstanceSet::const_iterator end = instances.end();
    for (ProxyInstanceSet::const_iterator it = instances.begin(); it != end; ++it)
        (*it)->invalidate();
    
    m_pluginView = nil;
    m_manualStream = 0;
}

void NetscapePluginInstanceProxy::invalidate()
{
    // If the plug-in host has died, the proxy will be null.
    if (!m_pluginHostProxy)
        return;
    
    m_pluginHostProxy->removePluginInstance(this);
    m_pluginHostProxy = 0;
}

void NetscapePluginInstanceProxy::destroy()
{
    uint32_t requestID = nextRequestID();

    ASSERT(!m_inDestroy);
    m_inDestroy = true;
    
    FrameLoadMap::iterator end = m_pendingFrameLoads.end();
    for (FrameLoadMap::iterator it = m_pendingFrameLoads.begin(); it != end; ++it)
        [(it->key) _setInternalLoadDelegate:nil];

    _WKPHDestroyPluginInstance(m_pluginHostProxy->port(), m_pluginID, requestID);
 
    // If the plug-in host crashes while we're waiting for a reply, the last reference to the instance proxy
    // will go away. Prevent this by protecting it here.
    RefPtr<NetscapePluginInstanceProxy> protect(this);
    
    // We don't care about the reply here - we just want to block until the plug-in instance has been torn down.
    waitForReply<NetscapePluginInstanceProxy::BooleanReply>(requestID);

    m_inDestroy = false;
    
    cleanup();
    invalidate();
}

void NetscapePluginInstanceProxy::setManualStream(PassRefPtr<HostedNetscapePluginStream> manualStream) 
{
    ASSERT(!m_manualStream);
    
    m_manualStream = manualStream;
}

bool NetscapePluginInstanceProxy::cancelStreamLoad(uint32_t streamID, NPReason reason) 
{
    HostedNetscapePluginStream* stream = 0;
    
    if (m_manualStream && streamID == 1)
        stream = m_manualStream.get();
    else
        stream = m_streams.get(streamID);
    
    if (!stream)
        return false;
    
    stream->cancelLoad(reason);
    return true;
}

void NetscapePluginInstanceProxy::disconnectStream(HostedNetscapePluginStream* stream)
{
    if (stream == m_manualStream) {
        m_manualStream = 0;
        return;
    }

    ASSERT(m_streams.get(stream->streamID()) == stream);
    m_streams.remove(stream->streamID());
}
    
void NetscapePluginInstanceProxy::pluginHostDied()
{
    m_pluginHostProxy = 0;

    [m_pluginView pluginHostDied];

    cleanup();
}

void NetscapePluginInstanceProxy::focusChanged(bool hasFocus)
{
    _WKPHPluginInstanceFocusChanged(m_pluginHostProxy->port(), m_pluginID, hasFocus);
}

void NetscapePluginInstanceProxy::windowFocusChanged(bool hasFocus)
{
    _WKPHPluginInstanceWindowFocusChanged(m_pluginHostProxy->port(), m_pluginID, hasFocus);
}

void NetscapePluginInstanceProxy::windowFrameChanged(NSRect frame)
{
    _WKPHPluginInstanceWindowFrameChanged(m_pluginHostProxy->port(), m_pluginID, frame.origin.x, frame.origin.y, frame.size.width, frame.size.height,
                                          NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]));
}
    
void NetscapePluginInstanceProxy::startTimers(bool throttleTimers)
{
    _WKPHPluginInstanceStartTimers(m_pluginHostProxy->port(), m_pluginID, throttleTimers);
}
    
void NetscapePluginInstanceProxy::mouseEvent(NSView *pluginView, NSEvent *event, NPCocoaEventType type)
{
    NSPoint screenPoint = [[event window] convertBaseToScreen:[event locationInWindow]];
    NSPoint pluginPoint = [pluginView convertPoint:[event locationInWindow] fromView:nil];
    
    int clickCount;
    if (type == NPCocoaEventMouseEntered || type == NPCocoaEventMouseExited)
        clickCount = 0;
    else
        clickCount = [event clickCount];
    

    _WKPHPluginInstanceMouseEvent(m_pluginHostProxy->port(), m_pluginID,
                                  [event timestamp],
                                  type, [event modifierFlags],
                                  pluginPoint.x, pluginPoint.y,
                                  screenPoint.x, screenPoint.y,
                                  NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]),
                                  [event buttonNumber], clickCount, 
                                  [event deltaX], [event deltaY], [event deltaZ]);
}
    
void NetscapePluginInstanceProxy::keyEvent(NSView *pluginView, NSEvent *event, NPCocoaEventType type)
{
    NSData *charactersData = [[event characters] dataUsingEncoding:NSUTF8StringEncoding];
    NSData *charactersIgnoringModifiersData = [[event charactersIgnoringModifiers] dataUsingEncoding:NSUTF8StringEncoding];
    
    _WKPHPluginInstanceKeyboardEvent(m_pluginHostProxy->port(), m_pluginID,
                                     [event timestamp], 
                                     type, [event modifierFlags], 
                                     const_cast<char*>(reinterpret_cast<const char*>([charactersData bytes])), [charactersData length], 
                                     const_cast<char*>(reinterpret_cast<const char*>([charactersIgnoringModifiersData bytes])), [charactersIgnoringModifiersData length], 
                                     [event isARepeat], [event keyCode], WKGetNSEventKeyChar(event));
}

void NetscapePluginInstanceProxy::syntheticKeyDownWithCommandModifier(int keyCode, char character)
{
    NSData *charactersData = [NSData dataWithBytes:&character length:1];

    _WKPHPluginInstanceKeyboardEvent(m_pluginHostProxy->port(), m_pluginID, 
                                     [NSDate timeIntervalSinceReferenceDate], 
                                     NPCocoaEventKeyDown, NSCommandKeyMask,
                                     const_cast<char*>(reinterpret_cast<const char*>([charactersData bytes])), [charactersData length], 
                                     const_cast<char*>(reinterpret_cast<const char*>([charactersData bytes])), [charactersData length], 
                                     false, keyCode, character);
}

void NetscapePluginInstanceProxy::flagsChanged(NSEvent *event)
{
    _WKPHPluginInstanceKeyboardEvent(m_pluginHostProxy->port(), m_pluginID, 
                                     [event timestamp], NPCocoaEventFlagsChanged, 
                                     [event modifierFlags], 0, 0, 0, 0, false, [event keyCode], 0);
}

void NetscapePluginInstanceProxy::insertText(NSString *text)
{
    NSData *textData = [text dataUsingEncoding:NSUTF8StringEncoding];
    
    _WKPHPluginInstanceInsertText(m_pluginHostProxy->port(), m_pluginID,
                                  const_cast<char*>(reinterpret_cast<const char*>([textData bytes])), [textData length]);
}

bool NetscapePluginInstanceProxy::wheelEvent(NSView *pluginView, NSEvent *event)
{
    NSPoint pluginPoint = [pluginView convertPoint:[event locationInWindow] fromView:nil];

    uint32_t requestID = nextRequestID();
    _WKPHPluginInstanceWheelEvent(m_pluginHostProxy->port(), m_pluginID, requestID,
                                  [event timestamp], [event modifierFlags], 
                                  pluginPoint.x, pluginPoint.y, [event buttonNumber], 
                                  [event deltaX], [event deltaY], [event deltaZ]);
    
    std::auto_ptr<NetscapePluginInstanceProxy::BooleanReply> reply = waitForReply<NetscapePluginInstanceProxy::BooleanReply>(requestID);
    if (!reply.get() || !reply->m_result)
        return false;
    
    return true;
}

void NetscapePluginInstanceProxy::print(CGContextRef context, unsigned width, unsigned height)
{
    uint32_t requestID = nextRequestID();
    _WKPHPluginInstancePrint(m_pluginHostProxy->port(), m_pluginID, requestID, width, height);
    
    std::auto_ptr<NetscapePluginInstanceProxy::BooleanAndDataReply> reply = waitForReply<NetscapePluginInstanceProxy::BooleanAndDataReply>(requestID);
    if (!reply.get() || !reply->m_returnValue)
        return;

    RetainPtr<CGDataProvider> dataProvider = adoptCF(CGDataProviderCreateWithCFData(reply->m_result.get()));
    RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());
    RetainPtr<CGImageRef> image = adoptCF(CGImageCreate(width, height, 8, 32, width * 4, colorSpace.get(), kCGImageAlphaFirst, dataProvider.get(), 0, false, kCGRenderingIntentDefault));

    // Flip the context and draw the image.
    CGContextSaveGState(context);
    CGContextTranslateCTM(context, 0.0, height);
    CGContextScaleCTM(context, 1.0, -1.0);
    
    CGContextDrawImage(context, CGRectMake(0, 0, width, height), image.get());

    CGContextRestoreGState(context);
}

void NetscapePluginInstanceProxy::snapshot(CGContextRef context, unsigned width, unsigned height)
{
    uint32_t requestID = nextRequestID();
    _WKPHPluginInstanceSnapshot(m_pluginHostProxy->port(), m_pluginID, requestID, width, height);
    
    std::auto_ptr<NetscapePluginInstanceProxy::BooleanAndDataReply> reply = waitForReply<NetscapePluginInstanceProxy::BooleanAndDataReply>(requestID);
    if (!reply.get() || !reply->m_returnValue)
        return;

    RetainPtr<CGDataProvider> dataProvider = adoptCF(CGDataProviderCreateWithCFData(reply->m_result.get()));
    RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());
    RetainPtr<CGImageRef> image = adoptCF(CGImageCreate(width, height, 8, 32, width * 4, colorSpace.get(), kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host, dataProvider.get(), 0, false, kCGRenderingIntentDefault));

    CGContextDrawImage(context, CGRectMake(0, 0, width, height), image.get());
}

void NetscapePluginInstanceProxy::stopTimers()
{
    _WKPHPluginInstanceStopTimers(m_pluginHostProxy->port(), m_pluginID);
}

void NetscapePluginInstanceProxy::status(const char* message)
{
    RetainPtr<CFStringRef> status = adoptCF(CFStringCreateWithCString(0, message ? message : "", kCFStringEncodingUTF8));
    if (!status)
        return;

    WebView *wv = [m_pluginView webView];
    [[wv _UIDelegateForwarder] webView:wv setStatusText:(NSString *)status.get()];
}

NPError NetscapePluginInstanceProxy::loadURL(const char* url, const char* target, const char* postData, uint32_t postLen, LoadURLFlags flags, uint32_t& streamID)
{
    if (!url)
        return NPERR_INVALID_PARAM;
 
    NSMutableURLRequest *request = [m_pluginView requestWithURLCString:url];

    if (flags & IsPost) {
        NSData *httpBody = nil;

        if (flags & PostDataIsFile) {
            // If we're posting a file, buf is either a file URL or a path to the file.
            if (!postData)
                return NPERR_INVALID_PARAM;
            RetainPtr<CFStringRef> bufString = adoptCF(CFStringCreateWithCString(kCFAllocatorDefault, postData, kCFStringEncodingWindowsLatin1));
            if (!bufString)
                return NPERR_INVALID_PARAM;
            
            NSURL *fileURL = [NSURL _web_URLWithDataAsString:(NSString *)bufString.get()];
            NSString *path;
            if ([fileURL isFileURL])
                path = [fileURL path];
            else
                path = (NSString *)bufString.get();
            httpBody = [NSData dataWithContentsOfFile:[path _webkit_fixedCarbonPOSIXPath]];
            if (!httpBody)
                return NPERR_FILE_NOT_FOUND;
        } else
            httpBody = [NSData dataWithBytes:postData length:postLen];

        if (![httpBody length])
            return NPERR_INVALID_PARAM;

        [request setHTTPMethod:@"POST"];
        
        if (flags & AllowHeadersInPostData) {
            if ([httpBody _web_startsWithBlankLine])
                httpBody = [httpBody subdataWithRange:NSMakeRange(1, [httpBody length] - 1)];
            else {
                NSInteger location = [httpBody _web_locationAfterFirstBlankLine];
                if (location != NSNotFound) {
                    // If the blank line is somewhere in the middle of postData, everything before is the header.
                    NSData *headerData = [httpBody subdataWithRange:NSMakeRange(0, location)];
                    NSMutableDictionary *header = [headerData _webkit_parseRFC822HeaderFields];
                    unsigned dataLength = [httpBody length] - location;

                    // Sometimes plugins like to set Content-Length themselves when they post,
                    // but CFNetwork does not like that. So we will remove the header
                    // and instead truncate the data to the requested length.
                    NSString *contentLength = [header objectForKey:@"Content-Length"];

                    if (contentLength)
                        dataLength = std::min(static_cast<unsigned>([contentLength intValue]), dataLength);
                    [header removeObjectForKey:@"Content-Length"];

                    if ([header count] > 0)
                        [request setAllHTTPHeaderFields:header];

                    // Everything after the blank line is the actual content of the POST.
                    httpBody = [httpBody subdataWithRange:NSMakeRange(location, dataLength)];
                }
            }
        }

        if (![httpBody length])
            return NPERR_INVALID_PARAM;

        // Plug-ins expect to receive uncached data when doing a POST (3347134).
        [request setCachePolicy:NSURLRequestReloadIgnoringCacheData];
        [request setHTTPBody:httpBody];
    }
    
    return loadRequest(request, target, flags & AllowPopups, streamID);
}

void NetscapePluginInstanceProxy::performRequest(PluginRequest* pluginRequest)
{
    // Loading the request can cause the instance proxy to go away, so protect it.
    RefPtr<NetscapePluginInstanceProxy> protect(this);

    ASSERT(m_pluginView);
    
    NSURLRequest *request = pluginRequest->request();
    NSString *frameName = pluginRequest->frameName();
    WebFrame *frame = nil;
    
    NSURL *URL = [request URL];
    NSString *JSString = [URL _webkit_scriptIfJavaScriptURL];
    
    ASSERT(frameName || JSString);
    if (frameName) {
        // FIXME - need to get rid of this window creation which
        // bypasses normal targeted link handling
        frame = kit(core([m_pluginView webFrame])->loader()->findFrameForNavigation(frameName));
        if (!frame) {
            WebView *currentWebView = [m_pluginView webView];
            NSDictionary *features = [[NSDictionary alloc] init];
            WebView *newWebView = [[currentWebView _UIDelegateForwarder] webView:currentWebView
                                                        createWebViewWithRequest:nil
                                                                  windowFeatures:features];
            [features release];

            if (!newWebView) {
                _WKPHLoadURLNotify(m_pluginHostProxy->port(), m_pluginID, pluginRequest->requestID(), NPERR_GENERIC_ERROR);
                return;
            }
            
            frame = [newWebView mainFrame];
            core(frame)->tree()->setName(frameName);
            [[newWebView _UIDelegateForwarder] webViewShow:newWebView];
        }
    }

    if (JSString) {
        ASSERT(!frame || [m_pluginView webFrame] == frame);
        evaluateJavaScript(pluginRequest);
    } else {
        [frame loadRequest:request];

        // Check if another plug-in view or even this view is waiting for the frame to load.
        // If it is, tell it that the load was cancelled because it will be anyway.
        WebHostedNetscapePluginView *view = [frame _internalLoadDelegate];
        if (view != nil) {
            ASSERT([view isKindOfClass:[WebHostedNetscapePluginView class]]);
            [view webFrame:frame didFinishLoadWithReason:NPRES_USER_BREAK];
        }
        m_pendingFrameLoads.set(frame, pluginRequest);
        [frame _setInternalLoadDelegate:m_pluginView];
    }

}

void NetscapePluginInstanceProxy::webFrameDidFinishLoadWithReason(WebFrame* webFrame, NPReason reason)
{
    FrameLoadMap::iterator it = m_pendingFrameLoads.find(webFrame);
    ASSERT(it != m_pendingFrameLoads.end());
        
    PluginRequest* pluginRequest = it->value.get();
    _WKPHLoadURLNotify(m_pluginHostProxy->port(), m_pluginID, pluginRequest->requestID(), reason);
 
    m_pendingFrameLoads.remove(it);

    [webFrame _setInternalLoadDelegate:nil];
}

void NetscapePluginInstanceProxy::evaluateJavaScript(PluginRequest* pluginRequest)
{
    NSURL *URL = [pluginRequest->request() URL];
    NSString *JSString = [URL _webkit_scriptIfJavaScriptURL];
    ASSERT(JSString);

    RefPtr<NetscapePluginInstanceProxy> protect(this); // Executing arbitrary JavaScript can destroy the proxy.

    NSString *result = [[m_pluginView webFrame] _stringByEvaluatingJavaScriptFromString:JSString forceUserGesture:pluginRequest->allowPopups()];
    
    // Don't continue if stringByEvaluatingJavaScriptFromString caused the plug-in to stop.
    if (!m_pluginHostProxy)
        return;

    if (pluginRequest->frameName() != nil)
        return;
        
    if ([result length] > 0) {
        // Don't call NPP_NewStream and other stream methods if there is no JS result to deliver. This is what Mozilla does.
        NSData *JSData = [result dataUsingEncoding:NSUTF8StringEncoding];
        
        RefPtr<HostedNetscapePluginStream> stream = HostedNetscapePluginStream::create(this, pluginRequest->requestID(), pluginRequest->request());
        m_streams.add(stream->streamID(), stream);
        
        RetainPtr<NSURLResponse> response = adoptNS([[NSURLResponse alloc] initWithURL:URL 
                                                                             MIMEType:@"text/plain" 
                                                                expectedContentLength:[JSData length]
                                                                     textEncodingName:nil]);
        stream->startStreamWithResponse(response.get());
        stream->didReceiveData(0, static_cast<const char*>([JSData bytes]), [JSData length]);
        stream->didFinishLoading(0);
    }
}

void NetscapePluginInstanceProxy::requestTimerFired(Timer<NetscapePluginInstanceProxy>*)
{
    ASSERT(!m_pluginRequests.isEmpty());
    ASSERT(m_pluginView);
    
    RefPtr<PluginRequest> request = m_pluginRequests.first();
    m_pluginRequests.removeFirst();
    
    if (!m_pluginRequests.isEmpty())
        m_requestTimer.startOneShot(0);
    
    performRequest(request.get());
}
    
NPError NetscapePluginInstanceProxy::loadRequest(NSURLRequest *request, const char* cTarget, bool allowPopups, uint32_t& requestID)
{
    NSURL *URL = [request URL];

    if (!URL) 
        return NPERR_INVALID_URL;

    // Don't allow requests to be loaded when the document loader is stopping all loaders.
    DocumentLoader* documentLoader = [[m_pluginView dataSource] _documentLoader];
    if (!documentLoader || documentLoader->isStopping())
        return NPERR_GENERIC_ERROR;

    NSString *target = nil;
    if (cTarget) {
        // Find the frame given the target string.
        target = [NSString stringWithCString:cTarget encoding:NSISOLatin1StringEncoding];
    }
    WebFrame *frame = [m_pluginView webFrame];

    // don't let a plugin start any loads if it is no longer part of a document that is being 
    // displayed unless the loads are in the same frame as the plugin.
    if (documentLoader != core([m_pluginView webFrame])->loader()->activeDocumentLoader() &&
        (!cTarget || [frame findFrameNamed:target] != frame)) {
        return NPERR_GENERIC_ERROR; 
    }
    
    NSString *JSString = [URL _webkit_scriptIfJavaScriptURL];
    if (JSString != nil) {
        if (![[[m_pluginView webView] preferences] isJavaScriptEnabled]) {
            // Return NPERR_GENERIC_ERROR if JS is disabled. This is what Mozilla does.
            return NPERR_GENERIC_ERROR;
        }
    } else {
        if (!core([m_pluginView webFrame])->document()->securityOrigin()->canDisplay(URL))
            return NPERR_GENERIC_ERROR;
    }
    
    // FIXME: Handle wraparound
    requestID = ++m_currentURLRequestID;
        
    if (cTarget || JSString) {
        // Make when targetting a frame or evaluating a JS string, perform the request after a delay because we don't
        // want to potentially kill the plug-in inside of its URL request.
        
        if (JSString && target && [frame findFrameNamed:target] != frame) {
            // For security reasons, only allow JS requests to be made on the frame that contains the plug-in.
            return NPERR_INVALID_PARAM;
        }

        RefPtr<PluginRequest> pluginRequest = PluginRequest::create(requestID, request, target, allowPopups);
        m_pluginRequests.append(pluginRequest.release());
        m_requestTimer.startOneShot(0);
    } else {
        RefPtr<HostedNetscapePluginStream> stream = HostedNetscapePluginStream::create(this, requestID, request);

        ASSERT(!m_streams.contains(requestID));
        m_streams.add(requestID, stream);
        stream->start();
    }
    
    return NPERR_NO_ERROR;
}

NetscapePluginInstanceProxy::Reply* NetscapePluginInstanceProxy::processRequestsAndWaitForReply(uint32_t requestID)
{
    Reply* reply = 0;

    ASSERT(m_pluginHostProxy);
    while (!(reply = m_replies.take(requestID))) {
        if (!m_pluginHostProxy->processRequests())
            return 0;

        // The host proxy can be destroyed while executing a nested processRequests() call, in which case it's normal
        // to get a success result, but be unable to keep looping.
        if (!m_pluginHostProxy)
            return 0;
    }
    
    ASSERT(reply);
    return reply;
}
    
// NPRuntime support
bool NetscapePluginInstanceProxy::getWindowNPObject(uint32_t& objectID)
{
    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;
    
    if (!frame->script()->canExecuteScripts(NotAboutToExecuteScript))
        objectID = 0;
    else
        objectID = m_localObjects.idForObject(*pluginWorld()->vm(), frame->script()->windowShell(pluginWorld())->window());
        
    return true;
}

bool NetscapePluginInstanceProxy::getPluginElementNPObject(uint32_t& objectID)
{
    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;
    
    if (JSObject* object = frame->script()->jsObjectForPluginElement([m_pluginView element]))
        objectID = m_localObjects.idForObject(*pluginWorld()->vm(), object);
    else
        objectID = 0;
    
    return true;
}

bool NetscapePluginInstanceProxy::forgetBrowserObjectID(uint32_t objectID)
{
    return m_localObjects.forget(objectID);
}
 
bool NetscapePluginInstanceProxy::evaluate(uint32_t objectID, const String& script, data_t& resultData, mach_msg_type_number_t& resultLength, bool allowPopups)
{
    resultData = 0;
    resultLength = 0;

    if (m_inDestroy)
        return false;

    if (!m_localObjects.contains(objectID)) {
        LOG_ERROR("NetscapePluginInstanceProxy::evaluate: local object %u doesn't exist.", objectID);
        return false;
    }

    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;

    JSLockHolder lock(pluginWorld()->vm());
    Strong<JSGlobalObject> globalObject(*pluginWorld()->vm(), frame->script()->globalObject(pluginWorld()));
    ExecState* exec = globalObject->globalExec();

    UserGestureIndicator gestureIndicator(allowPopups ? DefinitelyProcessingUserGesture : PossiblyProcessingUserGesture);
    
    JSValue result = JSC::evaluate(exec, makeSource(script));
    
    marshalValue(exec, result, resultData, resultLength);
    exec->clearException();
    return true;
}

bool NetscapePluginInstanceProxy::invoke(uint32_t objectID, const Identifier& methodName, data_t argumentsData, mach_msg_type_number_t argumentsLength, data_t& resultData, mach_msg_type_number_t& resultLength)
{
    resultData = 0;
    resultLength = 0;
    
    if (m_inDestroy)
        return false;
    
    JSObject* object = m_localObjects.get(objectID);
    if (!object) {
        LOG_ERROR("NetscapePluginInstanceProxy::invoke: local object %u doesn't exist.", objectID);
        return false;
    }
    
    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;
    
    ExecState* exec = frame->script()->globalObject(pluginWorld())->globalExec();
    JSLockHolder lock(exec);
    JSValue function = object->get(exec, methodName);
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return false;

    MarkedArgumentBuffer argList;
    demarshalValues(exec, argumentsData, argumentsLength, argList);

    JSValue value = call(exec, function, callType, callData, object->methodTable()->toThisObject(object, exec), argList);
        
    marshalValue(exec, value, resultData, resultLength);
    exec->clearException();
    return true;
}

bool NetscapePluginInstanceProxy::invokeDefault(uint32_t objectID, data_t argumentsData, mach_msg_type_number_t argumentsLength, data_t& resultData, mach_msg_type_number_t& resultLength)
{
    if (m_inDestroy)
        return false;

    JSObject* object = m_localObjects.get(objectID);
    if (!object) {
        LOG_ERROR("NetscapePluginInstanceProxy::invokeDefault: local object %u doesn't exist.", objectID);
        return false;
    }
    
    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;
    
    ExecState* exec = frame->script()->globalObject(pluginWorld())->globalExec();
    JSLockHolder lock(exec);    
    CallData callData;
    CallType callType = object->methodTable()->getCallData(object, callData);
    if (callType == CallTypeNone)
        return false;

    MarkedArgumentBuffer argList;
    demarshalValues(exec, argumentsData, argumentsLength, argList);

    JSValue value = call(exec, object, callType, callData, object->methodTable()->toThisObject(object, exec), argList);
    
    marshalValue(exec, value, resultData, resultLength);
    exec->clearException();
    return true;
}

bool NetscapePluginInstanceProxy::construct(uint32_t objectID, data_t argumentsData, mach_msg_type_number_t argumentsLength, data_t& resultData, mach_msg_type_number_t& resultLength)
{
    if (m_inDestroy)
        return false;

    JSObject* object = m_localObjects.get(objectID);
    if (!object) {
        LOG_ERROR("NetscapePluginInstanceProxy::construct: local object %u doesn't exist.", objectID);
        return false;
    }
    
    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;
    
    ExecState* exec = frame->script()->globalObject(pluginWorld())->globalExec();
    JSLockHolder lock(exec);

    ConstructData constructData;
    ConstructType constructType = object->methodTable()->getConstructData(object, constructData);
    if (constructType == ConstructTypeNone)
        return false;

    MarkedArgumentBuffer argList;
    demarshalValues(exec, argumentsData, argumentsLength, argList);

    JSValue value = JSC::construct(exec, object, constructType, constructData, argList);
    
    marshalValue(exec, value, resultData, resultLength);
    exec->clearException();
    return true;
}

bool NetscapePluginInstanceProxy::getProperty(uint32_t objectID, const Identifier& propertyName, data_t& resultData, mach_msg_type_number_t& resultLength)
{
    if (m_inDestroy)
        return false;

    JSObject* object = m_localObjects.get(objectID);
    if (!object) {
        LOG_ERROR("NetscapePluginInstanceProxy::getProperty: local object %u doesn't exist.", objectID);
        return false;
    }
    
    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;
    
    ExecState* exec = frame->script()->globalObject(pluginWorld())->globalExec();
    JSLockHolder lock(exec);    
    JSValue value = object->get(exec, propertyName);
    
    marshalValue(exec, value, resultData, resultLength);
    exec->clearException();
    return true;
}
    
bool NetscapePluginInstanceProxy::getProperty(uint32_t objectID, unsigned propertyName, data_t& resultData, mach_msg_type_number_t& resultLength)
{
    JSObject* object = m_localObjects.get(objectID);
    if (!object) {
        LOG_ERROR("NetscapePluginInstanceProxy::getProperty: local object %u doesn't exist.", objectID);
        return false;
    }
    
    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;
    
    ExecState* exec = frame->script()->globalObject(pluginWorld())->globalExec();
    JSLockHolder lock(exec);    
    JSValue value = object->get(exec, propertyName);
    
    marshalValue(exec, value, resultData, resultLength);
    exec->clearException();
    return true;
}

bool NetscapePluginInstanceProxy::setProperty(uint32_t objectID, const Identifier& propertyName, data_t valueData, mach_msg_type_number_t valueLength)
{
    if (m_inDestroy)
        return false;

    JSObject* object = m_localObjects.get(objectID);
    if (!object) {
        LOG_ERROR("NetscapePluginInstanceProxy::setProperty: local object %u doesn't exist.", objectID);
        return false;
    }
    
    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;
    
    ExecState* exec = frame->script()->globalObject(pluginWorld())->globalExec();
    JSLockHolder lock(exec);    

    JSValue value = demarshalValue(exec, valueData, valueLength);
    PutPropertySlot slot;
    object->methodTable()->put(object, exec, propertyName, value, slot);
    
    exec->clearException();
    return true;
}

bool NetscapePluginInstanceProxy::setProperty(uint32_t objectID, unsigned propertyName, data_t valueData, mach_msg_type_number_t valueLength)
{
    if (m_inDestroy)
        return false;

    JSObject* object = m_localObjects.get(objectID);
    if (!object) {
        LOG_ERROR("NetscapePluginInstanceProxy::setProperty: local object %u doesn't exist.", objectID);
        return false;
    }
    
    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;
    
    ExecState* exec = frame->script()->globalObject(pluginWorld())->globalExec();
    JSLockHolder lock(exec);    
    
    JSValue value = demarshalValue(exec, valueData, valueLength);
    object->methodTable()->putByIndex(object, exec, propertyName, value, false);
    
    exec->clearException();
    return true;
}

bool NetscapePluginInstanceProxy::removeProperty(uint32_t objectID, const Identifier& propertyName)
{
    if (m_inDestroy)
        return false;

    JSObject* object = m_localObjects.get(objectID);
    if (!object) {
        LOG_ERROR("NetscapePluginInstanceProxy::removeProperty: local object %u doesn't exist.", objectID);
        return false;
    }
    
    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;

    ExecState* exec = frame->script()->globalObject(pluginWorld())->globalExec();
    JSLockHolder lock(exec);
    if (!object->hasProperty(exec, propertyName)) {
        exec->clearException();
        return false;
    }
    
    object->methodTable()->deleteProperty(object, exec, propertyName);
    exec->clearException();    
    return true;
}
    
bool NetscapePluginInstanceProxy::removeProperty(uint32_t objectID, unsigned propertyName)
{
    if (m_inDestroy)
        return false;

    JSObject* object = m_localObjects.get(objectID);
    if (!object) {
        LOG_ERROR("NetscapePluginInstanceProxy::removeProperty: local object %u doesn't exist.", objectID);
        return false;
    }
    
    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;
    
    ExecState* exec = frame->script()->globalObject(pluginWorld())->globalExec();
    JSLockHolder lock(exec);
    if (!object->hasProperty(exec, propertyName)) {
        exec->clearException();
        return false;
    }
    
    object->methodTable()->deletePropertyByIndex(object, exec, propertyName);
    exec->clearException();    
    return true;
}

bool NetscapePluginInstanceProxy::hasProperty(uint32_t objectID, const Identifier& propertyName)
{
    if (m_inDestroy)
        return false;

    JSObject* object = m_localObjects.get(objectID);
    if (!object) {
        LOG_ERROR("NetscapePluginInstanceProxy::hasProperty: local object %u doesn't exist.", objectID);
        return false;
    }
    
    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;
    
    ExecState* exec = frame->script()->globalObject(pluginWorld())->globalExec();
    bool result = object->hasProperty(exec, propertyName);
    exec->clearException();
    
    return result;
}

bool NetscapePluginInstanceProxy::hasProperty(uint32_t objectID, unsigned propertyName)
{
    if (m_inDestroy)
        return false;

    JSObject* object = m_localObjects.get(objectID);
    if (!object) {
        LOG_ERROR("NetscapePluginInstanceProxy::hasProperty: local object %u doesn't exist.", objectID);
        return false;
    }
    
    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;
    
    ExecState* exec = frame->script()->globalObject(pluginWorld())->globalExec();
    bool result = object->hasProperty(exec, propertyName);
    exec->clearException();
    
    return result;
}
    
bool NetscapePluginInstanceProxy::hasMethod(uint32_t objectID, const Identifier& methodName)
{
    if (m_inDestroy)
        return false;

    JSObject* object = m_localObjects.get(objectID);
    if (!object) {
        LOG_ERROR("NetscapePluginInstanceProxy::hasMethod: local object %u doesn't exist.", objectID);
        return false;
    }

    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;
    
    ExecState* exec = frame->script()->globalObject(pluginWorld())->globalExec();
    JSLockHolder lock(exec);
    JSValue func = object->get(exec, methodName);
    exec->clearException();
    return !func.isUndefined();
}

bool NetscapePluginInstanceProxy::enumerate(uint32_t objectID, data_t& resultData, mach_msg_type_number_t& resultLength)
{
    if (m_inDestroy)
        return false;

    JSObject* object = m_localObjects.get(objectID);
    if (!object) {
        LOG_ERROR("NetscapePluginInstanceProxy::enumerate: local object %u doesn't exist.", objectID);
        return false;
    }
    
    Frame* frame = core([m_pluginView webFrame]);
    if (!frame)
        return false;
    
    ExecState* exec = frame->script()->globalObject(pluginWorld())->globalExec();
    JSLockHolder lock(exec);
 
    PropertyNameArray propertyNames(exec);
    object->methodTable()->getPropertyNames(object, exec, propertyNames, ExcludeDontEnumProperties);

    RetainPtr<NSMutableArray*> array = adoptNS([[NSMutableArray alloc] init]);
    for (unsigned i = 0; i < propertyNames.size(); i++) {
        uint64_t methodName = reinterpret_cast<uint64_t>(_NPN_GetStringIdentifier(propertyNames[i].string().utf8().data()));

        [array.get() addObject:[NSNumber numberWithLongLong:methodName]];
    }

    NSData *data = [NSPropertyListSerialization dataFromPropertyList:array.get() format:NSPropertyListBinaryFormat_v1_0 errorDescription:0];
    ASSERT(data);

    resultLength = [data length];
    mig_allocate(reinterpret_cast<vm_address_t*>(&resultData), resultLength);

    memcpy(resultData, [data bytes], resultLength);

    exec->clearException();

    return true;
}

static bool getObjectID(NetscapePluginInstanceProxy* pluginInstanceProxy, JSObject* object, uint64_t& objectID)
{
    if (object->classInfo() != &ProxyRuntimeObject::s_info)
        return false;

    ProxyRuntimeObject* runtimeObject = static_cast<ProxyRuntimeObject*>(object);
    ProxyInstance* instance = runtimeObject->getInternalProxyInstance();
    if (!instance)
        return false;

    if (instance->instanceProxy() != pluginInstanceProxy)
        return false;

    objectID = instance->objectID();
    return true;
}
    
void NetscapePluginInstanceProxy::addValueToArray(NSMutableArray *array, ExecState* exec, JSValue value)
{
    JSLockHolder lock(exec);

    if (value.isString()) {
        [array addObject:[NSNumber numberWithInt:StringValueType]];
        [array addObject:value.toWTFString(exec)];
    } else if (value.isNumber()) {
        [array addObject:[NSNumber numberWithInt:DoubleValueType]];
        [array addObject:[NSNumber numberWithDouble:value.toNumber(exec)]];
    } else if (value.isBoolean()) {
        [array addObject:[NSNumber numberWithInt:BoolValueType]];
        [array addObject:[NSNumber numberWithBool:value.toBoolean(exec)]];
    } else if (value.isNull())
        [array addObject:[NSNumber numberWithInt:NullValueType]];
    else if (value.isObject()) {
        JSObject* object = asObject(value);
        uint64_t objectID;
        if (getObjectID(this, object, objectID)) {
            [array addObject:[NSNumber numberWithInt:NPObjectValueType]];
            [array addObject:[NSNumber numberWithInt:objectID]];
        } else {
            [array addObject:[NSNumber numberWithInt:JSObjectValueType]];
            [array addObject:[NSNumber numberWithInt:m_localObjects.idForObject(exec->vm(), object)]];
        }
    } else
        [array addObject:[NSNumber numberWithInt:VoidValueType]];
}

void NetscapePluginInstanceProxy::marshalValue(ExecState* exec, JSValue value, data_t& resultData, mach_msg_type_number_t& resultLength)
{
    RetainPtr<NSMutableArray*> array = adoptNS([[NSMutableArray alloc] init]);
    
    addValueToArray(array.get(), exec, value);

    RetainPtr<NSData *> data = [NSPropertyListSerialization dataFromPropertyList:array.get() format:NSPropertyListBinaryFormat_v1_0 errorDescription:0];
    ASSERT(data);
    
    resultLength = [data.get() length];
    mig_allocate(reinterpret_cast<vm_address_t*>(&resultData), resultLength);
    
    memcpy(resultData, [data.get() bytes], resultLength);
}

RetainPtr<NSData *> NetscapePluginInstanceProxy::marshalValues(ExecState* exec, const ArgList& args)
{
    RetainPtr<NSMutableArray*> array = adoptNS([[NSMutableArray alloc] init]);

    for (unsigned i = 0; i < args.size(); i++)
        addValueToArray(array.get(), exec, args.at(i));

    RetainPtr<NSData *> data = [NSPropertyListSerialization dataFromPropertyList:array.get() format:NSPropertyListBinaryFormat_v1_0 errorDescription:0];
    ASSERT(data);

    return data;
}    

bool NetscapePluginInstanceProxy::demarshalValueFromArray(ExecState* exec, NSArray *array, NSUInteger& index, JSValue& result)
{
    if (index == [array count])
        return false;
                  
    int type = [[array objectAtIndex:index++] intValue];
    switch (type) {
        case VoidValueType:
            result = jsUndefined();
            return true;
        case NullValueType:
            result = jsNull();
            return true;
        case BoolValueType:
            result = jsBoolean([[array objectAtIndex:index++] boolValue]);
            return true;
        case DoubleValueType:
            result = jsNumber([[array objectAtIndex:index++] doubleValue]);
            return true;
        case StringValueType: {
            NSString *string = [array objectAtIndex:index++];
            
            result = jsString(exec, String(string));
            return true;
        }
        case JSObjectValueType: {
            uint32_t objectID = [[array objectAtIndex:index++] intValue];
            
            result = m_localObjects.get(objectID);
            ASSERT(result);
            return true;
        }
        case NPObjectValueType: {
            uint32_t objectID = [[array objectAtIndex:index++] intValue];

            Frame* frame = core([m_pluginView webFrame]);
            if (!frame)
                return false;
            
            if (!frame->script()->canExecuteScripts(NotAboutToExecuteScript))
                return false;

            RefPtr<RootObject> rootObject = frame->script()->createRootObject(m_pluginView);
            if (!rootObject)
                return false;
            
            result = ProxyInstance::create(rootObject.release(), this, objectID)->createRuntimeObject(exec);
            return true;
        }
        default:
            ASSERT_NOT_REACHED();
            return false;
    }
}

JSValue NetscapePluginInstanceProxy::demarshalValue(ExecState* exec, const char* valueData, mach_msg_type_number_t valueLength)
{
    RetainPtr<NSData*> data = adoptNS([[NSData alloc] initWithBytesNoCopy:(void*)valueData length:valueLength freeWhenDone:NO]);

    RetainPtr<NSArray*> array = [NSPropertyListSerialization propertyListFromData:data.get()
                                                                 mutabilityOption:NSPropertyListImmutable
                                                                           format:0
                                                                 errorDescription:0];
    NSUInteger position = 0;
    JSValue value;
    bool result = demarshalValueFromArray(exec, array.get(), position, value);
    ASSERT_UNUSED(result, result);

    return value;
}

void NetscapePluginInstanceProxy::demarshalValues(ExecState* exec, data_t valuesData, mach_msg_type_number_t valuesLength, MarkedArgumentBuffer& result)
{
    RetainPtr<NSData*> data = adoptNS([[NSData alloc] initWithBytesNoCopy:valuesData length:valuesLength freeWhenDone:NO]);

    RetainPtr<NSArray*> array = [NSPropertyListSerialization propertyListFromData:data.get()
                                                                 mutabilityOption:NSPropertyListImmutable
                                                                           format:0
                                                                 errorDescription:0];
    NSUInteger position = 0;
    JSValue value;
    while (demarshalValueFromArray(exec, array.get(), position, value))
        result.append(value);
}

void NetscapePluginInstanceProxy::retainLocalObject(JSC::JSValue value)
{
    if (!value.isObject() || value.inherits(&ProxyRuntimeObject::s_info))
        return;

    m_localObjects.retain(asObject(value));
}

void NetscapePluginInstanceProxy::releaseLocalObject(JSC::JSValue value)
{
    if (!value.isObject() || value.inherits(&ProxyRuntimeObject::s_info))
        return;

    m_localObjects.release(asObject(value));
}

PassRefPtr<Instance> NetscapePluginInstanceProxy::createBindingsInstance(PassRefPtr<RootObject> rootObject)
{
    uint32_t requestID = nextRequestID();
    
    if (_WKPHGetScriptableNPObject(m_pluginHostProxy->port(), m_pluginID, requestID) != KERN_SUCCESS)
        return 0;

    std::auto_ptr<GetScriptableNPObjectReply> reply = waitForReply<GetScriptableNPObjectReply>(requestID);
    if (!reply.get())
        return 0;

    if (!reply->m_objectID)
        return 0;

    // Since the reply was non-null, "this" is still a valid pointer.
    return ProxyInstance::create(rootObject, this, reply->m_objectID);
}

void NetscapePluginInstanceProxy::addInstance(ProxyInstance* instance)
{
    ASSERT(!m_instances.contains(instance));
    
    m_instances.add(instance);
}
    
void NetscapePluginInstanceProxy::removeInstance(ProxyInstance* instance)
{
    ASSERT(m_instances.contains(instance));
    
    m_instances.remove(instance);
}
 
void NetscapePluginInstanceProxy::willCallPluginFunction()
{
    m_pluginFunctionCallDepth++;
}
    
void NetscapePluginInstanceProxy::didCallPluginFunction(bool& stopped)
{
    ASSERT(m_pluginFunctionCallDepth > 0);
    m_pluginFunctionCallDepth--;
    
    // If -stop was called while we were calling into a plug-in function, and we're no longer
    // inside a plug-in function, stop now.
    if (!m_pluginFunctionCallDepth && m_shouldStopSoon) {
        m_shouldStopSoon = false;
        [m_pluginView stop];
        stopped = true;
    }
}
    
bool NetscapePluginInstanceProxy::shouldStop()
{
    if (m_pluginFunctionCallDepth) {
        m_shouldStopSoon = true;
        return false;
    }
    
    return true;
}

uint32_t NetscapePluginInstanceProxy::nextRequestID()
{
    uint32_t requestID = ++m_currentRequestID;
    
    // We don't want to return the HashMap empty/deleted "special keys"
    if (requestID == 0 || requestID == static_cast<uint32_t>(-1))
        return nextRequestID();
    
    return requestID;
}

void NetscapePluginInstanceProxy::invalidateRect(double x, double y, double width, double height)
{
    ASSERT(m_pluginView);
    
    m_pluginIsWaitingForDraw = true;
    [m_pluginView invalidatePluginContentRect:NSMakeRect(x, y, width, height)];
}

void NetscapePluginInstanceProxy::didDraw()
{
    if (!m_pluginIsWaitingForDraw)
        return;
    
    m_pluginIsWaitingForDraw = false;
    _WKPHPluginInstanceDidDraw(m_pluginHostProxy->port(), m_pluginID);
}
    
bool NetscapePluginInstanceProxy::getCookies(data_t urlData, mach_msg_type_number_t urlLength, data_t& cookiesData, mach_msg_type_number_t& cookiesLength)
{
    ASSERT(m_pluginView);
    
    NSURL *url = [m_pluginView URLWithCString:urlData];
    if (!url)
        return false;
    
    if (Frame* frame = core([m_pluginView webFrame])) {
        String cookieString = cookies(frame->document(), url); 
        WTF::CString cookieStringUTF8 = cookieString.utf8();
        if (cookieStringUTF8.isNull())
            return false;
        
        cookiesLength = cookieStringUTF8.length();
        mig_allocate(reinterpret_cast<vm_address_t*>(&cookiesData), cookiesLength);
        memcpy(cookiesData, cookieStringUTF8.data(), cookiesLength);
        
        return true;
    }

    return false;
}
    
bool NetscapePluginInstanceProxy::setCookies(data_t urlData, mach_msg_type_number_t urlLength, data_t cookiesData, mach_msg_type_number_t cookiesLength)
{
    ASSERT(m_pluginView);
    
    NSURL *url = [m_pluginView URLWithCString:urlData];
    if (!url)
        return false;

    if (Frame* frame = core([m_pluginView webFrame])) {
        String cookieString = String::fromUTF8(cookiesData, cookiesLength);
        if (!cookieString)
            return false;
        
        WebCore::setCookies(frame->document(), url, cookieString);
        return true;
    }

    return false;
}

bool NetscapePluginInstanceProxy::getProxy(data_t urlData, mach_msg_type_number_t urlLength, data_t& proxyData, mach_msg_type_number_t& proxyLength)
{
    ASSERT(m_pluginView);
    
    NSURL *url = [m_pluginView URLWithCString:urlData];
    if (!url)
        return false;

    Vector<ProxyServer> proxyServers = proxyServersForURL(url, 0);
    WTF::CString proxyStringUTF8 = toString(proxyServers).utf8();

    proxyLength = proxyStringUTF8.length();
    mig_allocate(reinterpret_cast<vm_address_t*>(&proxyData), proxyLength);
    memcpy(proxyData, proxyStringUTF8.data(), proxyLength);
    
    return true;
}
    
bool NetscapePluginInstanceProxy::getAuthenticationInfo(data_t protocolData, data_t hostData, uint32_t port, data_t schemeData, data_t realmData, 
                                                        data_t& usernameData, mach_msg_type_number_t& usernameLength, data_t& passwordData, mach_msg_type_number_t& passwordLength)
{
    WTF::CString username;
    WTF::CString password;
    
    if (!WebKit::getAuthenticationInfo(protocolData, hostData, port, schemeData, realmData, username, password))
        return false;
    
    usernameLength = username.length();
    mig_allocate(reinterpret_cast<vm_address_t*>(&usernameData), usernameLength);
    memcpy(usernameData, username.data(), usernameLength);
    
    passwordLength = password.length();
    mig_allocate(reinterpret_cast<vm_address_t*>(&passwordData), passwordLength);
    memcpy(passwordData, password.data(), passwordLength);
    
    return true;
}

bool NetscapePluginInstanceProxy::convertPoint(double sourceX, double sourceY, NPCoordinateSpace sourceSpace, 
                                               double& destX, double& destY, NPCoordinateSpace destSpace)
{
    ASSERT(m_pluginView);

    return [m_pluginView convertFromX:sourceX andY:sourceY space:sourceSpace toX:&destX andY:&destY space:destSpace];
}

uint32_t NetscapePluginInstanceProxy::checkIfAllowedToLoadURL(const char* url, const char* target)
{
    uint32_t checkID;
    
    // Assign a check ID
    do {
        checkID = ++m_urlCheckCounter;
    } while (m_urlChecks.contains(checkID) || !m_urlCheckCounter);

    NSString *frameName = target ? [NSString stringWithCString:target encoding:NSISOLatin1StringEncoding] : nil;

    NSNumber *contextInfo = [[NSNumber alloc] initWithUnsignedInt:checkID];
    WebPluginContainerCheck *check = [WebPluginContainerCheck checkWithRequest:[m_pluginView requestWithURLCString:url]
                                                                        target:frameName
                                                                  resultObject:m_pluginView
                                                                      selector:@selector(_containerCheckResult:contextInfo:)
                                                                    controller:m_pluginView 
                                                                   contextInfo:contextInfo];
    
    [contextInfo release];
    m_urlChecks.set(checkID, check);
    [check start];
    
    return checkID;
}

void NetscapePluginInstanceProxy::cancelCheckIfAllowedToLoadURL(uint32_t checkID)
{
    URLCheckMap::iterator it = m_urlChecks.find(checkID);
    if (it == m_urlChecks.end())
        return;
    
    WebPluginContainerCheck *check = it->value.get();
    [check cancel];
    m_urlChecks.remove(it);
}

void NetscapePluginInstanceProxy::checkIfAllowedToLoadURLResult(uint32_t checkID, bool allowed)
{
    _WKPHCheckIfAllowedToLoadURLResult(m_pluginHostProxy->port(), m_pluginID, checkID, allowed);
}

void NetscapePluginInstanceProxy::resolveURL(const char* url, const char* target, data_t& resolvedURLData, mach_msg_type_number_t& resolvedURLLength)
{
    ASSERT(m_pluginView);
    
    WTF::CString resolvedURL = [m_pluginView resolvedURLStringForURL:url target:target];
    
    resolvedURLLength = resolvedURL.length();
    mig_allocate(reinterpret_cast<vm_address_t*>(&resolvedURLData), resolvedURLLength);
    memcpy(resolvedURLData, resolvedURL.data(), resolvedURLLength);
}

void NetscapePluginInstanceProxy::privateBrowsingModeDidChange(bool isPrivateBrowsingEnabled)
{
    _WKPHPluginInstancePrivateBrowsingModeDidChange(m_pluginHostProxy->port(), m_pluginID, isPrivateBrowsingEnabled);
}

static String& globalExceptionString()
{
    DEFINE_STATIC_LOCAL(String, exceptionString, ());
    return exceptionString;
}

void NetscapePluginInstanceProxy::setGlobalException(const String& exception)
{
    globalExceptionString() = exception;
}

void NetscapePluginInstanceProxy::moveGlobalExceptionToExecState(ExecState* exec)
{
    if (globalExceptionString().isNull())
        return;

    {
        JSLockHolder lock(exec);
        throwError(exec, createError(exec, globalExceptionString()));
    }

    globalExceptionString() = String();
}

} // namespace WebKit

#endif // USE(PLUGIN_HOST_PROCESS) && ENABLE(NETSCAPE_PLUGIN_API)
