/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#import "NetscapePluginHostProxy.h"

#import <mach/mach.h>
#import <wtf/StdLibExtras.h>

#import "HostedNetscapePluginStream.h"
#import "NetscapePluginHostManager.h"
#import "NetscapePluginInstanceProxy.h"
#import "WebFrameInternal.h"
#import "WebHostedNetscapePluginView.h"
#import "WebKitSystemInterface.h"
#import <WebCore/Frame.h>
#import <WebCore/IdentifierRep.h>
#import <WebCore/ScriptController.h>

extern "C" {
#import "WebKitPluginHost.h"
#import "WebKitPluginClientServer.h"
}

using namespace JSC;
using namespace WebCore;

@interface WebPlaceholderModalWindow : NSWindow 
@end

@implementation WebPlaceholderModalWindow
// Prevent NSApp from calling requestUserAttention: when the window is shown 
// modally, even if the app is inactive. See 6823049.
- (BOOL)_wantsUserAttention
{
    return NO;   
}
@end

namespace WebKit {

class PluginDestroyDeferrer {
public:
    PluginDestroyDeferrer(NetscapePluginInstanceProxy* proxy)
        : m_proxy(proxy)
    {
        m_proxy->willCallPluginFunction();
    }
    
    ~PluginDestroyDeferrer()
    {
        bool stopped;
        m_proxy->didCallPluginFunction(stopped);
    }

private:
    RefPtr<NetscapePluginInstanceProxy> m_proxy;
};

typedef HashMap<mach_port_t, NetscapePluginHostProxy*> PluginProxyMap;
static PluginProxyMap& pluginProxyMap()
{
    DEFINE_STATIC_LOCAL(PluginProxyMap, pluginProxyMap, ());
    
    return pluginProxyMap;
}

unsigned NetscapePluginHostProxy::s_processingRequests;

NetscapePluginHostProxy::NetscapePluginHostProxy(mach_port_t clientPort, mach_port_t pluginHostPort, const ProcessSerialNumber& pluginHostPSN, bool shouldCacheMissingPropertiesAndMethods)
    : m_clientPort(clientPort)
    , m_portSet(MACH_PORT_NULL)
    , m_pluginHostPort(pluginHostPort)
    , m_isModal(false)
    , m_menuBarIsVisible(true)
    , m_fullscreenWindowIsShowing(false)
    , m_pluginHostPSN(pluginHostPSN)
    , m_shouldCacheMissingPropertiesAndMethods(shouldCacheMissingPropertiesAndMethods)
{
    pluginProxyMap().add(m_clientPort, this);
    
    // FIXME: We should use libdispatch for this.
    CFMachPortContext context = { 0, this, 0, 0, 0 };
    m_deadNameNotificationPort = adoptCF(CFMachPortCreate(0, deadNameNotificationCallback, &context, 0));

    mach_port_t previous;
    mach_port_request_notification(mach_task_self(), pluginHostPort, MACH_NOTIFY_DEAD_NAME, 0, 
                                   CFMachPortGetPort(m_deadNameNotificationPort.get()), MACH_MSG_TYPE_MAKE_SEND_ONCE, &previous);
    ASSERT(previous == MACH_PORT_NULL);
    
    RetainPtr<CFRunLoopSourceRef> deathPortSource = adoptCF(CFMachPortCreateRunLoopSource(0, m_deadNameNotificationPort.get(), 0));
    
    CFRunLoopAddSource(CFRunLoopGetCurrent(), deathPortSource.get(), kCFRunLoopDefaultMode);
    
    m_clientPortSource = adoptCF(WKCreateMIGServerSource((mig_subsystem_t)&WKWebKitPluginClient_subsystem, m_clientPort));
    CFRunLoopAddSource(CFRunLoopGetCurrent(), m_clientPortSource.get(), kCFRunLoopDefaultMode);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), m_clientPortSource.get(), (CFStringRef)NSEventTrackingRunLoopMode);
}

NetscapePluginHostProxy::~NetscapePluginHostProxy()
{
    pluginProxyMap().remove(m_clientPort);

    // Free the port set
    if (m_portSet) {
        mach_port_extract_member(mach_task_self(), m_clientPort, m_portSet);
        mach_port_extract_member(mach_task_self(), CFMachPortGetPort(m_deadNameNotificationPort.get()), m_portSet);
        mach_port_destroy(mach_task_self(), m_portSet);
    }
    
    ASSERT(m_clientPortSource);
    CFRunLoopSourceInvalidate(m_clientPortSource.get());
    m_clientPortSource = 0;
}

void NetscapePluginHostProxy::pluginHostDied()
{
    PluginInstanceMap instances;    
    m_instances.swap(instances);
  
    PluginInstanceMap::const_iterator end = instances.end();
    for (PluginInstanceMap::const_iterator it = instances.begin(); it != end; ++it)
        it->value->pluginHostDied();
    
    NetscapePluginHostManager::shared().pluginHostDied(this);
    
    // The plug-in crashed while its menu bar was hidden. Make sure to show it.
    if (!m_menuBarIsVisible)
        setMenuBarVisible(true);

    // The plug-in crashed while it had a modal dialog up.
    if (m_isModal)
        endModal();
    
    delete this;
}
    
void NetscapePluginHostProxy::addPluginInstance(NetscapePluginInstanceProxy* instance)
{
    ASSERT(!m_instances.contains(instance->pluginID()));
    
    m_instances.set(instance->pluginID(), instance);
}
    
void NetscapePluginHostProxy::removePluginInstance(NetscapePluginInstanceProxy* instance)
{
    ASSERT(m_instances.get(instance->pluginID()) == instance);

    m_instances.remove(instance->pluginID());
}

NetscapePluginInstanceProxy* NetscapePluginHostProxy::pluginInstance(uint32_t pluginID)
{
    NetscapePluginInstanceProxy* result = m_instances.get(pluginID);
    ASSERT(!result || result->hostProxy() == this);
    return result;
}

void NetscapePluginHostProxy::deadNameNotificationCallback(CFMachPortRef port, void *msg, CFIndex size, void *info)
{
    ASSERT(msg);
    ASSERT(static_cast<mach_msg_header_t*>(msg)->msgh_id == MACH_NOTIFY_DEAD_NAME);
    
    static_cast<NetscapePluginHostProxy*>(info)->pluginHostDied();
}

void NetscapePluginHostProxy::setMenuBarVisible(bool visible)
{
    m_menuBarIsVisible = visible;

    [NSMenu setMenuBarVisible:visible];
}

void NetscapePluginHostProxy::didEnterFullscreen() const
{
    makePluginHostProcessFrontProcess();
}

void NetscapePluginHostProxy::didExitFullscreen() const
{
    // If the plug-in host is the current application then we should bring ourselves to the front when it exits full-screen mode.
    if (isPluginHostProcessFrontProcess())
        makeCurrentProcessFrontProcess();
}

void NetscapePluginHostProxy::setFullscreenWindowIsShowing(bool isShowing)
{
    if (m_fullscreenWindowIsShowing == isShowing)
        return;

    m_fullscreenWindowIsShowing = isShowing;
    if (m_fullscreenWindowIsShowing)
        didEnterFullscreen();
    else
        didExitFullscreen();

}

void NetscapePluginHostProxy::applicationDidBecomeActive()
{
    makePluginHostProcessFrontProcess();
}

void NetscapePluginHostProxy::beginModal()
{
    ASSERT(!m_placeholderWindow);
    ASSERT(!m_activationObserver);
    
    m_placeholderWindow = adoptNS([[WebPlaceholderModalWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1, 1) styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES]);
    
    m_activationObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationWillBecomeActiveNotification object:NSApp queue:nil
                                                                         usingBlock:^(NSNotification *){ applicationDidBecomeActive(); }];
    
    // We need to be able to get the setModal(false) call from the plug-in host.
    CFRunLoopAddSource(CFRunLoopGetCurrent(), m_clientPortSource.get(), (CFStringRef)NSModalPanelRunLoopMode);
    
    [NSApp runModalForWindow:m_placeholderWindow.get()];
    
    [m_placeholderWindow.get() orderOut:nil];
    m_placeholderWindow = 0;
}
    
void NetscapePluginHostProxy::endModal()
{
    ASSERT(m_placeholderWindow);
    ASSERT(m_activationObserver);
    
    [[NSNotificationCenter defaultCenter] removeObserver:m_activationObserver.get()];
    m_activationObserver = nil;
    
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), m_clientPortSource.get(), (CFStringRef)NSModalPanelRunLoopMode);
    
    [NSApp stopModal];

    makeCurrentProcessFrontProcess();
}


void NetscapePluginHostProxy::setModal(bool modal)
{
    if (modal == m_isModal) 
        return;
    
    m_isModal = modal;
    
    if (m_isModal)
        beginModal();
    else
        endModal();
}
    
bool NetscapePluginHostProxy::processRequests()
{
    s_processingRequests++;

   if (!m_portSet) {
        mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_PORT_SET, &m_portSet);
        mach_port_insert_member(mach_task_self(), m_clientPort, m_portSet);
        mach_port_insert_member(mach_task_self(), CFMachPortGetPort(m_deadNameNotificationPort.get()), m_portSet);
    }
    
    char buffer[4096];
    
    mach_msg_header_t* msg = reinterpret_cast<mach_msg_header_t*>(buffer);
    
    kern_return_t kr = mach_msg(msg, MACH_RCV_MSG, 0, sizeof(buffer), m_portSet, 0, MACH_PORT_NULL);
    
    if (kr != KERN_SUCCESS) {
        LOG_ERROR("Could not receive mach message, error %x", kr);
        s_processingRequests--;
        return false;
    }
    
    if (msg->msgh_local_port == m_clientPort) {
        __ReplyUnion__WKWebKitPluginClient_subsystem reply;
        mach_msg_header_t* replyHeader = reinterpret_cast<mach_msg_header_t*>(&reply);
        
        if (WebKitPluginClient_server(msg, replyHeader) && replyHeader->msgh_remote_port != MACH_PORT_NULL) {
            kr = mach_msg(replyHeader, MACH_SEND_MSG, replyHeader->msgh_size, 0, MACH_PORT_NULL, 0, MACH_PORT_NULL);
            
            if (kr != KERN_SUCCESS) {
                LOG_ERROR("Could not send mach message, error %x", kr);
                s_processingRequests--;
                return false;
            }
        }
        
        s_processingRequests--;
        return true;
    }
    
    if (msg->msgh_local_port == CFMachPortGetPort(m_deadNameNotificationPort.get())) {
        ASSERT(msg->msgh_id == MACH_NOTIFY_DEAD_NAME);
        pluginHostDied();
        s_processingRequests--;
        return false;
    }
    
    ASSERT_NOT_REACHED();
    s_processingRequests--;
    return false;
}

void NetscapePluginHostProxy::makeCurrentProcessFrontProcess()
{
#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    ProcessSerialNumber psn;
    GetCurrentProcess(&psn);
    SetFrontProcess(&psn);
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif
}

void NetscapePluginHostProxy::makePluginHostProcessFrontProcess() const
{
#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    SetFrontProcess(&m_pluginHostPSN);
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif
}

bool NetscapePluginHostProxy::isPluginHostProcessFrontProcess() const
{
#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    ProcessSerialNumber frontProcess;
    GetFrontProcess(&frontProcess);

    Boolean isSameProcess = 0;
    SameProcess(&frontProcess, &m_pluginHostPSN, &isSameProcess);
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

    return isSameProcess;
}

} // namespace WebKit

using namespace WebKit;

// Helper class for deallocating data
class DataDeallocator {
public:
    DataDeallocator(data_t data, mach_msg_type_number_t dataLength)
        : m_data(reinterpret_cast<vm_address_t>(data))
        , m_dataLength(dataLength)
    {
    }
    
    ~DataDeallocator()
    {
        if (!m_data)
            return;
        
        vm_deallocate(mach_task_self(), m_data, m_dataLength);
    }
    
private:
    vm_address_t m_data;
    vm_size_t m_dataLength;
};

// MiG callbacks
kern_return_t WKPCStatusText(mach_port_t clientPort, uint32_t pluginID, data_t text, mach_msg_type_number_t textCnt)
{
    DataDeallocator deallocator(text, textCnt);
    
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;
    
    instanceProxy->status(text);
    return KERN_SUCCESS;
}

kern_return_t WKPCLoadURL(mach_port_t clientPort, uint32_t pluginID, data_t url, mach_msg_type_number_t urlLength, data_t target, mach_msg_type_number_t targetLength, 
                          data_t postData, mach_msg_type_number_t postDataLength, uint32_t flags,
                          uint16_t* outResult, uint32_t* outStreamID)
{
    DataDeallocator urlDeallocator(url, urlLength);
    DataDeallocator targetDeallocator(target, targetLength);
    DataDeallocator postDataDeallocator(postData, postDataLength);

    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    uint32_t streamID = 0;
    NPError result = instanceProxy->loadURL(url, target, postData, postDataLength, static_cast<LoadURLFlags>(flags), streamID);
    
    *outResult = result;
    *outStreamID = streamID;
    return KERN_SUCCESS;
}

kern_return_t WKPCCancelLoadURL(mach_port_t clientPort, uint32_t pluginID, uint32_t streamID, int16_t reason)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;
    
    if (!instanceProxy->cancelStreamLoad(streamID, reason))
        return KERN_FAILURE;
    
    return KERN_SUCCESS;
}

kern_return_t WKPCInvalidateRect(mach_port_t clientPort, uint32_t pluginID, double x, double y, double width, double height)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_SUCCESS;

    if (!hostProxy->isProcessingRequests()) {
        if (NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID))
            instanceProxy->invalidateRect(x, y, width, height);
        return KERN_SUCCESS;
    }

    // Defer the work
    CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopDefaultMode, ^{
        if (NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort)) {
            if (NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID))
                instanceProxy->invalidateRect(x, y, width, height);
        }
    });

    return KERN_SUCCESS;
}

kern_return_t WKPCGetScriptableNPObjectReply(mach_port_t clientPort, uint32_t pluginID, uint32_t requestID, uint32_t objectID)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    instanceProxy->setCurrentReply(requestID, new NetscapePluginInstanceProxy::GetScriptableNPObjectReply(objectID));
    return KERN_SUCCESS;
}

kern_return_t WKPCBooleanReply(mach_port_t clientPort, uint32_t pluginID, uint32_t requestID, boolean_t result)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;
    
    instanceProxy->setCurrentReply(requestID, new NetscapePluginInstanceProxy::BooleanReply(result));
    return KERN_SUCCESS;
}

kern_return_t WKPCBooleanAndDataReply(mach_port_t clientPort, uint32_t pluginID, uint32_t requestID, boolean_t returnValue, data_t resultData, mach_msg_type_number_t resultLength)
{
    DataDeallocator deallocator(resultData, resultLength);

    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    RetainPtr<CFDataRef> result = adoptCF(CFDataCreate(0, reinterpret_cast<UInt8*>(resultData), resultLength));
    instanceProxy->setCurrentReply(requestID, new NetscapePluginInstanceProxy::BooleanAndDataReply(returnValue, result));
    
    return KERN_SUCCESS;
}

kern_return_t WKPCInstantiatePluginReply(mach_port_t clientPort, uint32_t pluginID, uint32_t requestID, kern_return_t result, uint32_t renderContextID, uint32_t rendererType)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    instanceProxy->setCurrentReply(requestID, new NetscapePluginInstanceProxy::InstantiatePluginReply(result, renderContextID, static_cast<RendererType>(rendererType)));
    return KERN_SUCCESS;
}

kern_return_t WKPCGetWindowNPObject(mach_port_t clientPort, uint32_t pluginID, uint32_t* outObjectID)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    uint32_t objectID;
    if (!instanceProxy->getWindowNPObject(objectID))
        return KERN_FAILURE;
    
    *outObjectID = objectID;    
    return KERN_SUCCESS;
}

kern_return_t WKPCGetPluginElementNPObject(mach_port_t clientPort, uint32_t pluginID, uint32_t* outObjectID)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;
    
    uint32_t objectID;
    if (!instanceProxy->getPluginElementNPObject(objectID))
        return KERN_FAILURE;
    
    *outObjectID = objectID;    
    return KERN_SUCCESS;
}

kern_return_t WKPCForgetBrowserObject(mach_port_t clientPort, uint32_t pluginID, uint32_t objectID)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    return instanceProxy->forgetBrowserObjectID(objectID) ? KERN_SUCCESS : KERN_FAILURE;
}

kern_return_t WKPCEvaluate(mach_port_t clientPort, uint32_t pluginID, uint32_t requestID, uint32_t objectID, data_t scriptData, mach_msg_type_number_t scriptLength, boolean_t allowPopups)
{
    DataDeallocator deallocator(scriptData, scriptLength);

    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    PluginDestroyDeferrer deferrer(instanceProxy);
    
    String script = scriptLength ? String::fromUTF8WithLatin1Fallback(scriptData, scriptLength) : emptyString();
    
    data_t resultData = 0;
    mach_msg_type_number_t resultLength = 0;
    boolean_t returnValue = instanceProxy->evaluate(objectID, script, resultData, resultLength, allowPopups);

    hostProxy = instanceProxy->hostProxy();
    if (!hostProxy)
        return KERN_FAILURE;

    _WKPHBooleanAndDataReply(hostProxy->port(), instanceProxy->pluginID(), requestID, returnValue, resultData, resultLength);
    if (resultData)
        mig_deallocate(reinterpret_cast<vm_address_t>(resultData), resultLength);
        
    return KERN_SUCCESS;
}

kern_return_t WKPCGetStringIdentifier(mach_port_t clientPort, data_t name, mach_msg_type_number_t nameCnt, uint64_t* identifier)
{
    DataDeallocator deallocator(name, nameCnt);

    COMPILE_ASSERT(sizeof(*identifier) == sizeof(IdentifierRep*), identifier_sizes);
    
    *identifier = reinterpret_cast<uint64_t>(IdentifierRep::get(name));
    return KERN_SUCCESS;
}

kern_return_t WKPCGetIntIdentifier(mach_port_t clientPort, int32_t value, uint64_t* identifier)
{
    COMPILE_ASSERT(sizeof(*identifier) == sizeof(NPIdentifier), identifier_sizes);
    
    *identifier = reinterpret_cast<uint64_t>(IdentifierRep::get(value));
    return KERN_SUCCESS;
}

static Identifier identifierFromIdentifierRep(IdentifierRep* identifier)
{
    ASSERT(IdentifierRep::isValid(identifier));
    ASSERT(identifier->isString());
  
    const char* str = identifier->string();    
    return Identifier(JSDOMWindow::commonVM(), String::fromUTF8WithLatin1Fallback(str, strlen(str)));
}

kern_return_t WKPCInvoke(mach_port_t clientPort, uint32_t pluginID, uint32_t requestID, uint32_t objectID, uint64_t serverIdentifier,
                         data_t argumentsData, mach_msg_type_number_t argumentsLength) 
{
    DataDeallocator deallocator(argumentsData, argumentsLength);

    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    PluginDestroyDeferrer deferrer(instanceProxy);
    
    IdentifierRep* identifier = reinterpret_cast<IdentifierRep*>(serverIdentifier);
    if (!IdentifierRep::isValid(identifier))
        return KERN_FAILURE;

    Identifier methodNameIdentifier = identifierFromIdentifierRep(identifier);

    data_t resultData = 0;
    mach_msg_type_number_t resultLength = 0;
    boolean_t returnValue = instanceProxy->invoke(objectID, methodNameIdentifier, argumentsData, argumentsLength, resultData, resultLength);

    hostProxy = instanceProxy->hostProxy();
    if (!hostProxy)
        return KERN_FAILURE;

    _WKPHBooleanAndDataReply(hostProxy->port(), instanceProxy->pluginID(), requestID, returnValue, resultData, resultLength);
    if (resultData)
        mig_deallocate(reinterpret_cast<vm_address_t>(resultData), resultLength);
    
    return KERN_SUCCESS;
}

kern_return_t WKPCInvokeDefault(mach_port_t clientPort, uint32_t pluginID, uint32_t requestID, uint32_t objectID,
                                data_t argumentsData, mach_msg_type_number_t argumentsLength)
{
    DataDeallocator deallocator(argumentsData, argumentsLength);

    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    PluginDestroyDeferrer deferrer(instanceProxy);

    data_t resultData = 0;
    mach_msg_type_number_t resultLength = 0;
    boolean_t returnValue = instanceProxy->invokeDefault(objectID, argumentsData, argumentsLength, resultData, resultLength);

    hostProxy = instanceProxy->hostProxy();
    if (!hostProxy)
        return KERN_FAILURE;

    _WKPHBooleanAndDataReply(hostProxy->port(), instanceProxy->pluginID(), requestID, returnValue, resultData, resultLength);
    if (resultData)
        mig_deallocate(reinterpret_cast<vm_address_t>(resultData), resultLength);
    
    return KERN_SUCCESS;
}

kern_return_t WKPCConstruct(mach_port_t clientPort, uint32_t pluginID, uint32_t objectID,
                            data_t argumentsData, mach_msg_type_number_t argumentsLength, 
                            boolean_t* returnValue, data_t* resultData, mach_msg_type_number_t* resultLength)
{
    DataDeallocator deallocator(argumentsData, argumentsLength);

    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    PluginDestroyDeferrer deferrer(instanceProxy);

    *returnValue = instanceProxy->construct(objectID, argumentsData, argumentsLength, *resultData, *resultLength);
    
    return KERN_SUCCESS;
}

kern_return_t WKPCGetProperty(mach_port_t clientPort, uint32_t pluginID, uint32_t requestID, uint32_t objectID, uint64_t serverIdentifier)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    IdentifierRep* identifier = reinterpret_cast<IdentifierRep*>(serverIdentifier);
    if (!IdentifierRep::isValid(identifier))
        return KERN_FAILURE;
    
    PluginDestroyDeferrer deferrer(instanceProxy);

    data_t resultData = 0;
    mach_msg_type_number_t resultLength = 0;
    boolean_t returnValue;
    
    if (identifier->isString()) {
        Identifier propertyNameIdentifier = identifierFromIdentifierRep(identifier);        
        returnValue = instanceProxy->getProperty(objectID, propertyNameIdentifier, resultData, resultLength);
    } else 
        returnValue = instanceProxy->setProperty(objectID, identifier->number(), resultData, resultLength);

    hostProxy = instanceProxy->hostProxy();
    if (!hostProxy)
        return KERN_FAILURE;

    _WKPHBooleanAndDataReply(hostProxy->port(), instanceProxy->pluginID(), requestID, returnValue, resultData, resultLength);
    if (resultData)
        mig_deallocate(reinterpret_cast<vm_address_t>(resultData), resultLength);
    
    return KERN_SUCCESS;
}

kern_return_t WKPCSetProperty(mach_port_t clientPort, uint32_t pluginID, uint32_t requestID, uint32_t objectID, uint64_t serverIdentifier, data_t valueData, mach_msg_type_number_t valueLength)
{
    DataDeallocator deallocator(valueData, valueLength);

    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    PluginDestroyDeferrer deferrer(instanceProxy);

    IdentifierRep* identifier = reinterpret_cast<IdentifierRep*>(serverIdentifier);
    if (!IdentifierRep::isValid(identifier))
        return KERN_FAILURE;

    bool result;
    if (identifier->isString()) {
        Identifier propertyNameIdentifier = identifierFromIdentifierRep(identifier);        
        result = instanceProxy->setProperty(objectID, propertyNameIdentifier, valueData, valueLength);
    } else 
        result = instanceProxy->setProperty(objectID, identifier->number(), valueData, valueLength);

    hostProxy = instanceProxy->hostProxy();
    if (!hostProxy)
        return KERN_FAILURE;

    _WKPHBooleanReply(hostProxy->port(), instanceProxy->pluginID(), requestID, result);

    return KERN_SUCCESS;
}

kern_return_t WKPCRemoveProperty(mach_port_t clientPort, uint32_t pluginID, uint32_t requestID, uint32_t objectID, uint64_t serverIdentifier)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;
    
    PluginDestroyDeferrer deferrer(instanceProxy);

    IdentifierRep* identifier = reinterpret_cast<IdentifierRep*>(serverIdentifier);
    if (!IdentifierRep::isValid(identifier))
        return KERN_FAILURE;

    bool result;
    if (identifier->isString()) {
        Identifier propertyNameIdentifier = identifierFromIdentifierRep(identifier);        
        result = instanceProxy->removeProperty(objectID, propertyNameIdentifier);
    } else 
        result = instanceProxy->removeProperty(objectID, identifier->number());

    hostProxy = instanceProxy->hostProxy();
    if (!hostProxy)
        return KERN_FAILURE;

    _WKPHBooleanReply(hostProxy->port(), instanceProxy->pluginID(), requestID, result);

    return KERN_SUCCESS;
}

kern_return_t WKPCHasProperty(mach_port_t clientPort, uint32_t pluginID, uint32_t requestID, uint32_t objectID, uint64_t serverIdentifier)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;
    
    PluginDestroyDeferrer deferrer(instanceProxy);

    IdentifierRep* identifier = reinterpret_cast<IdentifierRep*>(serverIdentifier);
    if (!IdentifierRep::isValid(identifier))
        return KERN_FAILURE;
    
    boolean_t returnValue;
    if (identifier->isString()) {
        Identifier propertyNameIdentifier = identifierFromIdentifierRep(identifier);        
        returnValue = instanceProxy->hasProperty(objectID, propertyNameIdentifier);
    } else 
        returnValue = instanceProxy->hasProperty(objectID, identifier->number());

    hostProxy = instanceProxy->hostProxy();
    if (!hostProxy)
        return KERN_FAILURE;

    _WKPHBooleanReply(hostProxy->port(), instanceProxy->pluginID(), requestID, returnValue);
    
    return KERN_SUCCESS;
}

kern_return_t WKPCHasMethod(mach_port_t clientPort, uint32_t pluginID, uint32_t requestID, uint32_t objectID, uint64_t serverIdentifier)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;
    
    PluginDestroyDeferrer deferrer(instanceProxy);

    IdentifierRep* identifier = reinterpret_cast<IdentifierRep*>(serverIdentifier);
    if (!IdentifierRep::isValid(identifier))
        return KERN_FAILURE;
    
    Identifier methodNameIdentifier = identifierFromIdentifierRep(identifier);        
    boolean_t returnValue = instanceProxy->hasMethod(objectID, methodNameIdentifier);

    hostProxy = instanceProxy->hostProxy();
    if (!hostProxy)
        return KERN_FAILURE;

    _WKPHBooleanReply(hostProxy->port(), instanceProxy->pluginID(), requestID, returnValue);

    return KERN_SUCCESS;
}

kern_return_t WKPCIdentifierInfo(mach_port_t clientPort, uint64_t serverIdentifier, data_t* infoData, mach_msg_type_number_t* infoLength)
{
    IdentifierRep* identifier = reinterpret_cast<IdentifierRep*>(serverIdentifier);
    if (!IdentifierRep::isValid(identifier))
        return KERN_FAILURE;
    
    id info;
    if (identifier->isString()) {
        const char* str = identifier->string();
        info = [NSData dataWithBytesNoCopy:(void*)str length:strlen(str) freeWhenDone:NO];
    } else 
        info = [NSNumber numberWithInt:identifier->number()];

    RetainPtr<NSData*> data = [NSPropertyListSerialization dataFromPropertyList:info format:NSPropertyListBinaryFormat_v1_0 errorDescription:0];
    ASSERT(data);
    
    *infoLength = [data.get() length];
    mig_allocate(reinterpret_cast<vm_address_t*>(infoData), *infoLength);
    
    memcpy(*infoData, [data.get() bytes], *infoLength);
    
    return KERN_SUCCESS;
}

kern_return_t WKPCEnumerate(mach_port_t clientPort, uint32_t pluginID, uint32_t requestID, uint32_t objectID)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;
    
    data_t resultData = 0;
    mach_msg_type_number_t resultLength = 0;
    boolean_t returnValue = instanceProxy->enumerate(objectID, resultData, resultLength);

    hostProxy = instanceProxy->hostProxy();
    if (!hostProxy)
        return KERN_FAILURE;

    _WKPHBooleanAndDataReply(hostProxy->port(), instanceProxy->pluginID(), requestID, returnValue, resultData, resultLength);
    
    if (resultData)
        mig_deallocate(reinterpret_cast<vm_address_t>(resultData), resultLength);
    
    return KERN_SUCCESS;
}

kern_return_t WKPCSetMenuBarVisible(mach_port_t clientPort, boolean_t menuBarVisible)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;

    hostProxy->setMenuBarVisible(menuBarVisible);

    return KERN_SUCCESS;
}

kern_return_t WKPCSetFullscreenWindowIsShowing(mach_port_t clientPort, boolean_t fullscreenWindowIsShowing)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;

    hostProxy->setFullscreenWindowIsShowing(fullscreenWindowIsShowing);

    return KERN_SUCCESS;
}

kern_return_t WKPCSetModal(mach_port_t clientPort, boolean_t modal)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;

    if (!hostProxy->isProcessingRequests()) {
        hostProxy->setModal(modal);
        return KERN_SUCCESS;
    }

    // Defer the work
    CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopDefaultMode, ^{
        if (NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort))
            hostProxy->setModal(modal);
    });

    return KERN_SUCCESS;
}

kern_return_t WKPCGetCookies(mach_port_t clientPort, uint32_t pluginID,
                             data_t urlData, mach_msg_type_number_t urlLength,
                             boolean_t* returnValue, data_t* cookiesData, mach_msg_type_number_t* cookiesLength)
{
    *cookiesData = 0;
    *cookiesLength = 0;
    
    DataDeallocator deallocator(urlData, urlLength);
    
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;
    
    *returnValue = instanceProxy->getCookies(urlData, urlLength, *cookiesData, *cookiesLength);
    
    return KERN_SUCCESS;
}

kern_return_t WKPCGetProxy(mach_port_t clientPort, uint32_t pluginID,
                           data_t urlData, mach_msg_type_number_t urlLength,
                           boolean_t* returnValue, data_t* proxyData, mach_msg_type_number_t* proxyLength)
{
    *proxyData = 0;
    *proxyLength = 0;
    
    DataDeallocator deallocator(urlData, urlLength);
    
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;
    
    *returnValue = instanceProxy->getProxy(urlData, urlLength, *proxyData, *proxyLength);
    
    return KERN_SUCCESS;
}

kern_return_t WKPCSetCookies(mach_port_t clientPort, uint32_t pluginID,
                             data_t urlData, mach_msg_type_number_t urlLength,
                             data_t cookiesData, mach_msg_type_number_t cookiesLength,
                             boolean_t* returnValue)
{
    DataDeallocator urlDeallocator(urlData, urlLength);
    DataDeallocator cookiesDeallocator(cookiesData, cookiesLength);
 
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    *returnValue = instanceProxy->setCookies(urlData, urlLength, cookiesData, cookiesLength);
    return KERN_SUCCESS;
}

kern_return_t WKPCGetAuthenticationInfo(mach_port_t clientPort, uint32_t pluginID,
                                        data_t protocolData, mach_msg_type_number_t protocolLength,
                                        data_t hostData, mach_msg_type_number_t hostLength,
                                        uint32_t port,
                                        data_t schemeData, mach_msg_type_number_t schemeLength,
                                        data_t realmData, mach_msg_type_number_t realmLength,
                                        boolean_t* returnValue,
                                        data_t* usernameData, mach_msg_type_number_t *usernameLength,
                                        data_t* passwordData, mach_msg_type_number_t *passwordLength)
{
    DataDeallocator protocolDeallocator(protocolData, protocolLength);
    DataDeallocator hostDeallocator(hostData, hostLength);
    DataDeallocator schemeDeallocator(schemeData, schemeLength);
    DataDeallocator realmDeallocator(realmData, realmLength);

    *usernameData = 0;
    *usernameLength = 0;
    *passwordData = 0;
    *passwordLength = 0;
    
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;
    
    *returnValue = instanceProxy->getAuthenticationInfo(protocolData, hostData, port, schemeData, realmData, *usernameData, *usernameLength, *passwordData, *passwordLength);
    
    return KERN_SUCCESS;
}

kern_return_t WKPCConvertPoint(mach_port_t clientPort, uint32_t pluginID, 
                               double sourceX, double sourceY, uint32_t sourceSpace, 
                               uint32_t destSpace, boolean_t *returnValue, double *destX, double *destY)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    *returnValue = instanceProxy->convertPoint(sourceX, sourceY, static_cast<NPCoordinateSpace>(sourceSpace), 
                                               *destX, *destY, static_cast<NPCoordinateSpace>(destSpace));
    return KERN_SUCCESS;
}

kern_return_t WKPCCheckIfAllowedToLoadURL(mach_port_t clientPort, uint32_t pluginID, data_t urlData, mach_msg_type_number_t urlLength,
                                          data_t targetData, mach_msg_type_number_t targetLength, uint32_t *checkID)
{
    DataDeallocator urlDeallocator(urlData, urlLength);
    DataDeallocator targetDeallocator(targetData, targetLength);

    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;
    
    *checkID = instanceProxy->checkIfAllowedToLoadURL(urlData, targetData);
    return KERN_SUCCESS;
}

kern_return_t WKPCCancelCheckIfAllowedToLoadURL(mach_port_t clientPort, uint32_t pluginID, uint32_t checkID)
{
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;

    instanceProxy->cancelCheckIfAllowedToLoadURL(checkID);
    return KERN_SUCCESS;
}

kern_return_t WKPCResolveURL(mach_port_t clientPort, uint32_t pluginID, data_t urlData, mach_msg_type_number_t urlLength,
                             data_t targetData, mach_msg_type_number_t targetLength,
                             data_t *resolvedURLData, mach_msg_type_number_t *resolvedURLLength)
{
    DataDeallocator urlDeallocator(urlData, urlLength);
    DataDeallocator targetDeallocator(targetData, targetLength);
    
    *resolvedURLData = 0;
    *resolvedURLLength = 0;
    
    NetscapePluginHostProxy* hostProxy = pluginProxyMap().get(clientPort);
    if (!hostProxy)
        return KERN_FAILURE;
    
    NetscapePluginInstanceProxy* instanceProxy = hostProxy->pluginInstance(pluginID);
    if (!instanceProxy)
        return KERN_FAILURE;
    
    instanceProxy->resolveURL(urlData, targetData, *resolvedURLData, *resolvedURLLength);
    return KERN_SUCCESS;
}

kern_return_t WKPCSetException(mach_port_t clientPort, data_t message, mach_msg_type_number_t messageCnt)
{
    DataDeallocator deallocator(message, messageCnt);

    NetscapePluginInstanceProxy::setGlobalException(String::fromUTF8WithLatin1Fallback(message, messageCnt));

    return KERN_SUCCESS;
}

#endif // USE(PLUGIN_HOST_PROCESS) && ENABLE(NETSCAPE_PLUGIN_API)
