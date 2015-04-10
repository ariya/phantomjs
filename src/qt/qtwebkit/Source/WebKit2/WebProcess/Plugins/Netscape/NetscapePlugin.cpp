/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "NetscapePlugin.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "NPRuntimeObjectMap.h"
#include "NPRuntimeUtilities.h"
#include "NetscapePluginStream.h"
#include "PluginController.h"
#include "ShareableBitmap.h"
#include <WebCore/GraphicsContext.h>
#include <WebCore/HTTPHeaderMap.h>
#include <WebCore/IntRect.h>
#include <WebCore/KURL.h>
#include <WebCore/SharedBuffer.h>
#include <runtime/JSObject.h>
#include <utility>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

// The plug-in that we're currently calling NPP_New for.
static NetscapePlugin* currentNPPNewPlugin;

PassRefPtr<NetscapePlugin> NetscapePlugin::create(PassRefPtr<NetscapePluginModule> pluginModule)
{
    if (!pluginModule)
        return 0;

    return adoptRef(new NetscapePlugin(pluginModule));
}
    
NetscapePlugin::NetscapePlugin(PassRefPtr<NetscapePluginModule> pluginModule)
    : m_nextRequestID(0)
    , m_pluginModule(pluginModule)
    , m_npWindow()
    , m_isStarted(false)
#if PLATFORM(MAC)
    , m_isWindowed(false)
#else
    , m_isWindowed(true)
#endif
    , m_isTransparent(false)
    , m_inNPPNew(false)
    , m_shouldUseManualLoader(false)
    , m_hasCalledSetWindow(false)
    , m_nextTimerID(0)
#if PLATFORM(MAC)
    , m_drawingModel(static_cast<NPDrawingModel>(-1))
    , m_eventModel(static_cast<NPEventModel>(-1))
    , m_pluginReturnsNonretainedLayer(!m_pluginModule->pluginQuirks().contains(PluginQuirks::ReturnsRetainedCoreAnimationLayer))
    , m_layerHostingMode(LayerHostingModeDefault)
    , m_currentMouseEvent(0)
    , m_pluginHasFocus(false)
    , m_windowHasFocus(false)
    , m_pluginWantsLegacyCocoaTextInput(true)
    , m_isComplexTextInputEnabled(false)
    , m_hasHandledAKeyDownEvent(false)
    , m_ignoreNextKeyUpEventCounter(0)
#ifndef NP_NO_CARBON
    , m_nullEventTimer(RunLoop::main(), this, &NetscapePlugin::nullEventTimerFired)
    , m_npCGContext()
#endif
#elif PLUGIN_ARCHITECTURE(X11)
    , m_drawable(0)
    , m_pluginDisplay(0)
#if PLATFORM(GTK)
    , m_platformPluginWidget(0)
#endif
#endif
{
    m_npp.ndata = this;
    m_npp.pdata = 0;
    
    m_pluginModule->incrementLoadCount();
}

NetscapePlugin::~NetscapePlugin()
{
    ASSERT(!m_isStarted);
    ASSERT(m_timers.isEmpty());

    m_pluginModule->decrementLoadCount();
}

PassRefPtr<NetscapePlugin> NetscapePlugin::fromNPP(NPP npp)
{
    if (!npp)
        return 0;

    return static_cast<NetscapePlugin*>(npp->ndata);
}

void NetscapePlugin::invalidate(const NPRect* invalidRect)
{
    IntRect rect;
    
    if (!invalidRect)
        rect = IntRect(0, 0, m_pluginSize.width(), m_pluginSize.height());
    else
        rect = IntRect(invalidRect->left, invalidRect->top, invalidRect->right - invalidRect->left, invalidRect->bottom - invalidRect->top);
    
    if (platformInvalidate(rect))
        return;

    controller()->invalidate(rect);
}

const char* NetscapePlugin::userAgent(NPP npp)
{
    if (npp)
        return fromNPP(npp)->userAgent();

    if (currentNPPNewPlugin)
        return currentNPPNewPlugin->userAgent();

    return 0;
}

const char* NetscapePlugin::userAgent()
{
#if PLUGIN_ARCHITECTURE(WIN)
    static const char* MozillaUserAgent = "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1) Gecko/20061010 Firefox/2.0";
    
    if (quirks().contains(PluginQuirks::WantsMozillaUserAgent))
        return MozillaUserAgent;
#endif

    if (m_userAgent.isNull()) {
        String userAgent = controller()->userAgent();
        ASSERT(!userAgent.isNull());

#if PLUGIN_ARCHITECTURE(MAC)
        if (quirks().contains(PluginQuirks::AppendVersion3UserAgent))
            userAgent.append(" Version/3.2.1");
#endif

        m_userAgent = userAgent.utf8();
    }
    return m_userAgent.data();
}

void NetscapePlugin::loadURL(const String& method, const String& urlString, const String& target, const HTTPHeaderMap& headerFields, const Vector<uint8_t>& httpBody,
                             bool sendNotification, void* notificationData)
{
    uint64_t requestID = ++m_nextRequestID;
    
    controller()->loadURL(requestID, method, urlString, target, headerFields, httpBody, allowPopups());

    if (target.isNull()) {
        // The browser is going to send the data in a stream, create a plug-in stream.
        RefPtr<NetscapePluginStream> pluginStream = NetscapePluginStream::create(this, requestID, urlString, sendNotification, notificationData);
        ASSERT(!m_streams.contains(requestID));

        m_streams.set(requestID, pluginStream.release());
        return;
    }

    if (sendNotification) {
        // Eventually we are going to get a frameDidFinishLoading or frameDidFail call for this request.
        // Keep track of the notification data so we can call NPP_URLNotify.
        ASSERT(!m_pendingURLNotifications.contains(requestID));
        m_pendingURLNotifications.set(requestID, std::make_pair(urlString, notificationData));
    }
}

NPError NetscapePlugin::destroyStream(NPStream* stream, NPReason reason)
{
    NetscapePluginStream* pluginStream = 0;

    for (StreamsMap::const_iterator it = m_streams.begin(), end = m_streams.end(); it != end; ++it) {
        if (it->value->npStream() == stream) {
            pluginStream = it->value.get();
            break;
        }
    }

    if (!pluginStream)
        return NPERR_INVALID_INSTANCE_ERROR;

    return pluginStream->destroy(reason);
}

void NetscapePlugin::setIsWindowed(bool isWindowed)
{
    // Once the plugin has started, it's too late to change whether the plugin is windowed or not.
    // (This is true in Firefox and Chrome, too.) Disallow setting m_isWindowed in that case to
    // keep our internal state consistent.
    if (m_isStarted)
        return;

    m_isWindowed = isWindowed;
}

void NetscapePlugin::setIsTransparent(bool isTransparent)
{
    m_isTransparent = isTransparent;
}

void NetscapePlugin::setStatusbarText(const String& statusbarText)
{
    controller()->setStatusbarText(statusbarText);
}

static void (*setExceptionFunction)(const String&);

void NetscapePlugin::setSetExceptionFunction(void (*function)(const String&))
{
    ASSERT(!setExceptionFunction || setExceptionFunction == function);
    setExceptionFunction = function;
}

void NetscapePlugin::setException(const String& exceptionString)
{
    ASSERT(setExceptionFunction);
    setExceptionFunction(exceptionString);
}

bool NetscapePlugin::evaluate(NPObject* npObject, const String& scriptString, NPVariant* result)
{
    return controller()->evaluate(npObject, scriptString, result, allowPopups());
}

bool NetscapePlugin::isPrivateBrowsingEnabled()
{
    return controller()->isPrivateBrowsingEnabled();
}

NPObject* NetscapePlugin::windowScriptNPObject()
{
    return controller()->windowScriptNPObject();
}

NPObject* NetscapePlugin::pluginElementNPObject()
{
    return controller()->pluginElementNPObject();
}

void NetscapePlugin::cancelStreamLoad(NetscapePluginStream* pluginStream)
{
    if (pluginStream == m_manualStream) {
        controller()->cancelManualStreamLoad();
        return;
    }

    // Ask the plug-in controller to cancel this stream load.
    controller()->cancelStreamLoad(pluginStream->streamID());
}

void NetscapePlugin::removePluginStream(NetscapePluginStream* pluginStream)
{
    if (pluginStream == m_manualStream) {
        m_manualStream = 0;
        return;
    }

    ASSERT(m_streams.get(pluginStream->streamID()) == pluginStream);
    m_streams.remove(pluginStream->streamID());
}

bool NetscapePlugin::isAcceleratedCompositingEnabled()
{
#if USE(ACCELERATED_COMPOSITING)
    return controller()->isAcceleratedCompositingEnabled();
#else
    return false;
#endif
}

void NetscapePlugin::pushPopupsEnabledState(bool state)
{
    m_popupEnabledStates.append(state);
}
 
void NetscapePlugin::popPopupsEnabledState()
{
    ASSERT(!m_popupEnabledStates.isEmpty());

    m_popupEnabledStates.removeLast();
}

void NetscapePlugin::pluginThreadAsyncCall(void (*function)(void*), void* userData)
{
    RunLoop::main()->dispatch(WTF::bind(&NetscapePlugin::handlePluginThreadAsyncCall, this, function, userData));
}
    
void NetscapePlugin::handlePluginThreadAsyncCall(void (*function)(void*), void* userData)
{
    if (!m_isStarted)
        return;

    function(userData);
}

PassOwnPtr<NetscapePlugin::Timer> NetscapePlugin::Timer::create(NetscapePlugin* netscapePlugin, unsigned timerID, unsigned interval, bool repeat, TimerFunc timerFunc)
{
    return adoptPtr(new Timer(netscapePlugin, timerID, interval, repeat, timerFunc));
}

NetscapePlugin::Timer::Timer(NetscapePlugin* netscapePlugin, unsigned timerID, unsigned interval, bool repeat, TimerFunc timerFunc)
    : m_netscapePlugin(netscapePlugin)
    , m_timerID(timerID)
    , m_interval(interval)
    , m_repeat(repeat)
    , m_timerFunc(timerFunc)
    , m_timer(RunLoop::main(), this, &Timer::timerFired)
{
}

NetscapePlugin::Timer::~Timer()
{
}

void NetscapePlugin::Timer::start()
{
    double timeInterval = m_interval / 1000.0;

    if (m_repeat)
        m_timer.startRepeating(timeInterval);
    else
        m_timer.startOneShot(timeInterval);
}

void NetscapePlugin::Timer::stop()
{
    m_timer.stop();
}

void NetscapePlugin::Timer::timerFired()
{
    m_timerFunc(&m_netscapePlugin->m_npp, m_timerID);

    if (!m_repeat)
        m_netscapePlugin->unscheduleTimer(m_timerID);
}

uint32_t NetscapePlugin::scheduleTimer(unsigned interval, bool repeat, void (*timerFunc)(NPP, unsigned timerID))
{
    if (!timerFunc)
        return 0;

    // FIXME: Handle wrapping around.
    unsigned timerID = ++m_nextTimerID;

    OwnPtr<Timer> timer = Timer::create(this, timerID, interval, repeat, timerFunc);
    
    // FIXME: Based on the plug-in visibility, figure out if we should throttle the timer, or if we should start it at all.
    timer->start();
    m_timers.set(timerID, timer.release());

    return timerID;
}

void NetscapePlugin::unscheduleTimer(unsigned timerID)
{
    if (OwnPtr<Timer> timer = m_timers.take(timerID))
        timer->stop();
}

double NetscapePlugin::contentsScaleFactor()
{
    return controller()->contentsScaleFactor();
}

String NetscapePlugin::proxiesForURL(const String& urlString)
{
    return controller()->proxiesForURL(urlString);
}
    
String NetscapePlugin::cookiesForURL(const String& urlString)
{
    return controller()->cookiesForURL(urlString);
}

void NetscapePlugin::setCookiesForURL(const String& urlString, const String& cookieString)
{
    controller()->setCookiesForURL(urlString, cookieString);
}

bool NetscapePlugin::getAuthenticationInfo(const ProtectionSpace& protectionSpace, String& username, String& password)
{
    return controller()->getAuthenticationInfo(protectionSpace, username, password);
}    

NPError NetscapePlugin::NPP_New(NPMIMEType pluginType, uint16_t mode, int16_t argc, char* argn[], char* argv[], NPSavedData* savedData)
{
    return m_pluginModule->pluginFuncs().newp(pluginType, &m_npp, mode, argc, argn, argv, savedData);
}
    
NPError NetscapePlugin::NPP_Destroy(NPSavedData** savedData)
{
    return m_pluginModule->pluginFuncs().destroy(&m_npp, savedData);
}

NPError NetscapePlugin::NPP_SetWindow(NPWindow* npWindow)
{
    return m_pluginModule->pluginFuncs().setwindow(&m_npp, npWindow);
}

NPError NetscapePlugin::NPP_NewStream(NPMIMEType mimeType, NPStream* stream, NPBool seekable, uint16_t* streamType)
{
    return m_pluginModule->pluginFuncs().newstream(&m_npp, mimeType, stream, seekable, streamType);
}

NPError NetscapePlugin::NPP_DestroyStream(NPStream* stream, NPReason reason)
{
    return m_pluginModule->pluginFuncs().destroystream(&m_npp, stream, reason);
}

void NetscapePlugin::NPP_StreamAsFile(NPStream* stream, const char* filename)
{
    return m_pluginModule->pluginFuncs().asfile(&m_npp, stream, filename);
}

int32_t NetscapePlugin::NPP_WriteReady(NPStream* stream)
{
    return m_pluginModule->pluginFuncs().writeready(&m_npp, stream);
}

int32_t NetscapePlugin::NPP_Write(NPStream* stream, int32_t offset, int32_t len, void* buffer)
{
    return m_pluginModule->pluginFuncs().write(&m_npp, stream, offset, len, buffer);
}

int16_t NetscapePlugin::NPP_HandleEvent(void* event)
{
    return m_pluginModule->pluginFuncs().event(&m_npp, event);
}

void NetscapePlugin::NPP_URLNotify(const char* url, NPReason reason, void* notifyData)
{
    m_pluginModule->pluginFuncs().urlnotify(&m_npp, url, reason, notifyData);
}

NPError NetscapePlugin::NPP_GetValue(NPPVariable variable, void *value)
{
    if (!m_pluginModule->pluginFuncs().getvalue)
        return NPERR_GENERIC_ERROR;

    return m_pluginModule->pluginFuncs().getvalue(&m_npp, variable, value);
}

NPError NetscapePlugin::NPP_SetValue(NPNVariable variable, void *value)
{
    if (!m_pluginModule->pluginFuncs().setvalue)
        return NPERR_GENERIC_ERROR;

    return m_pluginModule->pluginFuncs().setvalue(&m_npp, variable, value);
}

void NetscapePlugin::callSetWindow()
{
    if (wantsPluginRelativeNPWindowCoordinates()) {
        m_npWindow.x = 0;
        m_npWindow.y = 0;
        m_npWindow.clipRect.top = m_clipRect.y();
        m_npWindow.clipRect.left = m_clipRect.x();
    } else {
        IntPoint pluginLocationInRootViewCoordinates = convertToRootView(IntPoint());
        IntPoint clipRectInRootViewCoordinates = convertToRootView(m_clipRect.location());

        m_npWindow.x = pluginLocationInRootViewCoordinates.x();
        m_npWindow.y = pluginLocationInRootViewCoordinates.y();
        m_npWindow.clipRect.top = clipRectInRootViewCoordinates.y();
        m_npWindow.clipRect.left = clipRectInRootViewCoordinates.x();
    }

    m_npWindow.width = m_pluginSize.width();
    m_npWindow.height = m_pluginSize.height();
    m_npWindow.clipRect.right = m_npWindow.clipRect.left + m_clipRect.width();
    m_npWindow.clipRect.bottom = m_npWindow.clipRect.top + m_clipRect.height();

    NPP_SetWindow(&m_npWindow);
    m_hasCalledSetWindow = true;
}

void NetscapePlugin::callSetWindowInvisible()
{
    NPWindow invisibleWindow = m_npWindow;
    
    invisibleWindow.window = 0;
    invisibleWindow.clipRect.top = 0;
    invisibleWindow.clipRect.left = 0;
    invisibleWindow.clipRect.bottom = 0;
    invisibleWindow.clipRect.right = 0;
    
    NPP_SetWindow(&invisibleWindow);
    m_hasCalledSetWindow = true;
}

bool NetscapePlugin::shouldLoadSrcURL()
{
    // Check if we should cancel the load
    NPBool cancelSrcStream = false;

    if (NPP_GetValue(NPPVpluginCancelSrcStream, &cancelSrcStream) != NPERR_NO_ERROR)
        return true;

    return !cancelSrcStream;
}

NetscapePluginStream* NetscapePlugin::streamFromID(uint64_t streamID)
{
    return m_streams.get(streamID);
}

void NetscapePlugin::stopAllStreams()
{
    Vector<RefPtr<NetscapePluginStream> > streams;
    copyValuesToVector(m_streams, streams);

    for (size_t i = 0; i < streams.size(); ++i)
        streams[i]->stop(NPRES_USER_BREAK);
}

bool NetscapePlugin::allowPopups() const
{
    if (m_pluginModule->pluginFuncs().version >= NPVERS_HAS_POPUPS_ENABLED_STATE) {
        if (!m_popupEnabledStates.isEmpty())
            return m_popupEnabledStates.last();
    }

    // FIXME: Check if the current event is a user gesture.
    // Really old versions of Flash required this for popups to work, but all newer versions
    // support NPN_PushPopupEnabledState/NPN_PopPopupEnabledState.
    return false;
}

#if PLUGIN_ARCHITECTURE(MAC)
static bool isTransparentSilverlightBackgroundValue(const String& lowercaseBackgroundValue)
{
    // This checks if the background color value is transparent, according to
    // the format documented at http://msdn.microsoft.com/en-us/library/cc838148(VS.95).aspx
    if (lowercaseBackgroundValue.startsWith('#')) {
        if (lowercaseBackgroundValue.length() == 5 && lowercaseBackgroundValue[1] != 'f') {
            // An 8-bit RGB value with alpha transparency, in the form #ARGB.
            return true;
        }

        if (lowercaseBackgroundValue.length() == 9 && !(lowercaseBackgroundValue[1] == 'f' && lowercaseBackgroundValue[2] == 'f')) {
            // A 16-bit RGB value with alpha transparency, in the form #AARRGGBB.
            return true;
        }
    } else if (lowercaseBackgroundValue.startsWith("sc#")) {
        Vector<String> components;
        lowercaseBackgroundValue.substring(3).split(",", components);

        // An ScRGB value with alpha transparency, in the form sc#A,R,G,B.
        if (components.size() == 4) {
            if (components[0].toDouble() < 1)
                return true;
        }
    } else if (lowercaseBackgroundValue == "transparent")
        return true;

    // This is an opaque color.
    return false;
}
#endif

bool NetscapePlugin::initialize(const Parameters& parameters)
{
    uint16_t mode = parameters.isFullFramePlugin ? NP_FULL : NP_EMBED;
    
    m_shouldUseManualLoader = parameters.shouldUseManualLoader;

    CString mimeTypeCString = parameters.mimeType.utf8();

    ASSERT(parameters.names.size() == parameters.values.size());

    Vector<CString> paramNames;
    Vector<CString> paramValues;
    for (size_t i = 0; i < parameters.names.size(); ++i) {
        String parameterName = parameters.names[i];

#if PLUGIN_ARCHITECTURE(MAC)
        if (m_pluginModule->pluginQuirks().contains(PluginQuirks::WantsLowercaseParameterNames))
            parameterName = parameterName.lower();
#endif

        paramNames.append(parameterName.utf8());
        paramValues.append(parameters.values[i].utf8());
    }

    // The strings that these pointers point to are kept alive by paramNames and paramValues.
    Vector<const char*> names;
    Vector<const char*> values;
    for (size_t i = 0; i < paramNames.size(); ++i) {
        names.append(paramNames[i].data());
        values.append(paramValues[i].data());
    }

#if PLUGIN_ARCHITECTURE(MAC)
    if (m_pluginModule->pluginQuirks().contains(PluginQuirks::MakeOpaqueUnlessTransparentSilverlightBackgroundAttributeExists)) {
        for (size_t i = 0; i < parameters.names.size(); ++i) {
            if (equalIgnoringCase(parameters.names[i], "background")) {
                setIsTransparent(isTransparentSilverlightBackgroundValue(parameters.values[i].lower()));
                break;
            }
        }
    }

    m_layerHostingMode = parameters.layerHostingMode;
#endif

    platformPreInitialize();

    NetscapePlugin* previousNPPNewPlugin = currentNPPNewPlugin;
    
    m_inNPPNew = true;
    currentNPPNewPlugin = this;

    NPError error = NPP_New(const_cast<char*>(mimeTypeCString.data()), mode, names.size(),
                            const_cast<char**>(names.data()), const_cast<char**>(values.data()), 0);

    m_inNPPNew = false;
    currentNPPNewPlugin = previousNPPNewPlugin;

    if (error != NPERR_NO_ERROR)
        return false;

    m_isStarted = true;

    // FIXME: This is not correct in all cases.
    m_npWindow.type = NPWindowTypeDrawable;

    if (!platformPostInitialize()) {
        destroy();
        return false;
    }

    // Load the src URL if needed.
    if (!parameters.shouldUseManualLoader && !parameters.url.isEmpty() && shouldLoadSrcURL())
        loadURL("GET", parameters.url.string(), String(), HTTPHeaderMap(), Vector<uint8_t>(), false, 0);
    
    return true;
}
    
void NetscapePlugin::destroy()
{
    ASSERT(m_isStarted);

    // Stop all streams.
    stopAllStreams();

#if !PLUGIN_ARCHITECTURE(MAC) && !PLUGIN_ARCHITECTURE(X11)
    m_npWindow.window = 0;
    callSetWindow();
#endif

    NPP_Destroy(0);

    m_isStarted = false;

    platformDestroy();

    m_timers.clear();
}
    
void NetscapePlugin::paint(GraphicsContext* context, const IntRect& dirtyRect)
{
    ASSERT(m_isStarted);
    
    platformPaint(context, dirtyRect);
}

PassRefPtr<ShareableBitmap> NetscapePlugin::snapshot()
{
    if (!supportsSnapshotting() || m_pluginSize.isEmpty())
        return 0;

    ASSERT(m_isStarted);

    IntSize backingStoreSize = m_pluginSize;
    backingStoreSize.scale(contentsScaleFactor());

    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::createShareable(backingStoreSize, ShareableBitmap::SupportsAlpha);
    OwnPtr<GraphicsContext> context = bitmap->createGraphicsContext();

    // FIXME: We should really call applyDeviceScaleFactor instead of scale, but that ends up calling into WKSI
    // which we currently don't have initiated in the plug-in process.
    context->scale(FloatSize(contentsScaleFactor(), contentsScaleFactor()));

    platformPaint(context.get(), IntRect(IntPoint(), m_pluginSize), true);

    return bitmap.release();
}

bool NetscapePlugin::isTransparent()
{
    return m_isTransparent;
}

bool NetscapePlugin::wantsWheelEvents()
{
    return m_pluginModule->pluginQuirks().contains(PluginQuirks::WantsWheelEvents);
}

void NetscapePlugin::geometryDidChange(const IntSize& pluginSize, const IntRect& clipRect, const AffineTransform& pluginToRootViewTransform)
{
    ASSERT(m_isStarted);

    if (pluginSize == m_pluginSize && m_clipRect == clipRect && m_pluginToRootViewTransform == pluginToRootViewTransform) {
        // Nothing to do.
        return;
    }

    bool shouldCallSetWindow = true;

    // If the plug-in doesn't want window relative coordinates, we don't need to call setWindow unless its size or clip rect changes.
    if (m_hasCalledSetWindow && wantsPluginRelativeNPWindowCoordinates() && m_pluginSize == pluginSize && m_clipRect == clipRect)
        shouldCallSetWindow = false;

    m_pluginSize = pluginSize;
    m_clipRect = clipRect;
    m_pluginToRootViewTransform = pluginToRootViewTransform;

    IntPoint frameRectLocationInWindowCoordinates = m_pluginToRootViewTransform.mapPoint(IntPoint());
    m_frameRectInWindowCoordinates = IntRect(frameRectLocationInWindowCoordinates, m_pluginSize);

    platformGeometryDidChange();

    if (!shouldCallSetWindow)
        return;

    callSetWindow();
}

void NetscapePlugin::visibilityDidChange()
{
    ASSERT(m_isStarted);

    platformVisibilityDidChange();
}

void NetscapePlugin::frameDidFinishLoading(uint64_t requestID)
{
    ASSERT(m_isStarted);
    
    PendingURLNotifyMap::iterator it = m_pendingURLNotifications.find(requestID);
    if (it == m_pendingURLNotifications.end())
        return;

    String url = it->value.first;
    void* notificationData = it->value.second;

    m_pendingURLNotifications.remove(it);
    
    NPP_URLNotify(url.utf8().data(), NPRES_DONE, notificationData);
}

void NetscapePlugin::frameDidFail(uint64_t requestID, bool wasCancelled)
{
    ASSERT(m_isStarted);
    
    PendingURLNotifyMap::iterator it = m_pendingURLNotifications.find(requestID);
    if (it == m_pendingURLNotifications.end())
        return;

    String url = it->value.first;
    void* notificationData = it->value.second;

    m_pendingURLNotifications.remove(it);
    
    NPP_URLNotify(url.utf8().data(), wasCancelled ? NPRES_USER_BREAK : NPRES_NETWORK_ERR, notificationData);
}

void NetscapePlugin::didEvaluateJavaScript(uint64_t requestID, const String& result)
{
    ASSERT(m_isStarted);
    
    if (NetscapePluginStream* pluginStream = streamFromID(requestID))
        pluginStream->sendJavaScriptStream(result);
}

void NetscapePlugin::streamDidReceiveResponse(uint64_t streamID, const KURL& responseURL, uint32_t streamLength, 
                                              uint32_t lastModifiedTime, const String& mimeType, const String& headers, const String& /* suggestedFileName */)
{
    ASSERT(m_isStarted);
    
    if (NetscapePluginStream* pluginStream = streamFromID(streamID))
        pluginStream->didReceiveResponse(responseURL, streamLength, lastModifiedTime, mimeType, headers);
}

void NetscapePlugin::streamDidReceiveData(uint64_t streamID, const char* bytes, int length)
{
    ASSERT(m_isStarted);
    
    if (NetscapePluginStream* pluginStream = streamFromID(streamID))
        pluginStream->didReceiveData(bytes, length);
}

void NetscapePlugin::streamDidFinishLoading(uint64_t streamID)
{
    ASSERT(m_isStarted);
    
    if (NetscapePluginStream* pluginStream = streamFromID(streamID))
        pluginStream->didFinishLoading();
}

void NetscapePlugin::streamDidFail(uint64_t streamID, bool wasCancelled)
{
    ASSERT(m_isStarted);
    
    if (NetscapePluginStream* pluginStream = streamFromID(streamID))
        pluginStream->didFail(wasCancelled);
}

void NetscapePlugin::manualStreamDidReceiveResponse(const KURL& responseURL, uint32_t streamLength, uint32_t lastModifiedTime, 
                                                    const String& mimeType, const String& headers, const String& /* suggestedFileName */)
{
    ASSERT(m_isStarted);
    ASSERT(m_shouldUseManualLoader);
    ASSERT(!m_manualStream);
    
    m_manualStream = NetscapePluginStream::create(this, 0, responseURL.string(), false, 0);
    m_manualStream->didReceiveResponse(responseURL, streamLength, lastModifiedTime, mimeType, headers);
}

void NetscapePlugin::manualStreamDidReceiveData(const char* bytes, int length)
{
    ASSERT(m_isStarted);
    ASSERT(m_shouldUseManualLoader);
    ASSERT(m_manualStream);

    m_manualStream->didReceiveData(bytes, length);
}

void NetscapePlugin::manualStreamDidFinishLoading()
{
    ASSERT(m_isStarted);
    ASSERT(m_shouldUseManualLoader);
    ASSERT(m_manualStream);

    m_manualStream->didFinishLoading();
}

void NetscapePlugin::manualStreamDidFail(bool wasCancelled)
{
    ASSERT(m_isStarted);
    ASSERT(m_shouldUseManualLoader);

    if (!m_manualStream)
        return;
    m_manualStream->didFail(wasCancelled);
}

bool NetscapePlugin::handleMouseEvent(const WebMouseEvent& mouseEvent)
{
    ASSERT(m_isStarted);
    
    return platformHandleMouseEvent(mouseEvent);
}
    
bool NetscapePlugin::handleWheelEvent(const WebWheelEvent& wheelEvent)
{
    ASSERT(m_isStarted);

    return platformHandleWheelEvent(wheelEvent);
}

bool NetscapePlugin::handleMouseEnterEvent(const WebMouseEvent& mouseEvent)
{
    ASSERT(m_isStarted);

    return platformHandleMouseEnterEvent(mouseEvent);
}

bool NetscapePlugin::handleMouseLeaveEvent(const WebMouseEvent& mouseEvent)
{
    ASSERT(m_isStarted);

    return platformHandleMouseLeaveEvent(mouseEvent);
}

bool NetscapePlugin::handleContextMenuEvent(const WebMouseEvent&)
{
    // We don't know if the plug-in has handled mousedown event by displaying a context menu, so we never want WebKit to show a default one.
    return true;
}

bool NetscapePlugin::handleKeyboardEvent(const WebKeyboardEvent& keyboardEvent)
{
    ASSERT(m_isStarted);

    return platformHandleKeyboardEvent(keyboardEvent);
}

bool NetscapePlugin::handleEditingCommand(const String& /* commandName */, const String& /* argument */)
{
    return false;
}

bool NetscapePlugin::isEditingCommandEnabled(const String& /* commandName */)
{
    return false;
}

bool NetscapePlugin::shouldAllowScripting()
{
    return true;
}

bool NetscapePlugin::shouldAllowNavigationFromDrags()
{
    return false;
}

bool NetscapePlugin::handlesPageScaleFactor()
{
    return false;
}

void NetscapePlugin::setFocus(bool hasFocus)
{
    ASSERT(m_isStarted);

    platformSetFocus(hasFocus);
}

NPObject* NetscapePlugin::pluginScriptableNPObject()
{
    ASSERT(m_isStarted);
    NPObject* scriptableNPObject = 0;
    
    if (NPP_GetValue(NPPVpluginScriptableNPObject, &scriptableNPObject) != NPERR_NO_ERROR)
        return 0;

#if PLUGIN_ARCHITECTURE(MAC)
    if (m_pluginModule->pluginQuirks().contains(PluginQuirks::ReturnsNonRetainedScriptableNPObject))
        retainNPObject(scriptableNPObject);        
#endif    

    return scriptableNPObject;
}
    
unsigned NetscapePlugin::countFindMatches(const String&, WebCore::FindOptions, unsigned)
{
    return 0;
}

bool NetscapePlugin::findString(const String&, WebCore::FindOptions, unsigned)
{
    return false;
}

void NetscapePlugin::contentsScaleFactorChanged(float scaleFactor)
{
    ASSERT(m_isStarted);

#if PLUGIN_ARCHITECTURE(MAC)
    double contentsScaleFactor = scaleFactor;
    NPP_SetValue(NPNVcontentsScaleFactor, &contentsScaleFactor);
#else
    UNUSED_PARAM(scaleFactor);
#endif
}

void NetscapePlugin::storageBlockingStateChanged(bool storageBlockingEnabled)
{
    if (m_storageBlockingState != storageBlockingEnabled) {
        m_storageBlockingState = storageBlockingEnabled;
        updateNPNPrivateMode();
    }
}

void NetscapePlugin::privateBrowsingStateChanged(bool privateBrowsingEnabled)
{
    if (m_privateBrowsingState != privateBrowsingEnabled) {
        m_privateBrowsingState = privateBrowsingEnabled;
        updateNPNPrivateMode();
    }
}

void NetscapePlugin::updateNPNPrivateMode()
{
    ASSERT(m_isStarted);

    // From https://wiki.mozilla.org/Plugins:PrivateMode
    //   When the browser turns private mode on or off it will call NPP_SetValue for "NPNVprivateModeBool" 
    //   (assigned enum value 18) with a pointer to an NPBool value on all applicable instances.
    //   Plugins should check the boolean value pointed to, not the pointer itself. 
    //   The value will be true when private mode is on.
    NPBool value = m_privateBrowsingState || m_storageBlockingState;
    NPP_SetValue(NPNVprivateModeBool, &value);
}

bool NetscapePlugin::getFormValue(String& formValue)
{
    ASSERT(m_isStarted);

    char* formValueString = 0;
    if (NPP_GetValue(NPPVformValue, &formValueString) != NPERR_NO_ERROR)
        return false;

    formValue = String::fromUTF8(formValueString);

    // The plug-in allocates the form value string with NPN_MemAlloc so it needs to be freed with NPN_MemFree.
    npnMemFree(formValueString);
    return true;
}

bool NetscapePlugin::handleScroll(ScrollDirection, ScrollGranularity)
{
    return false;
}

Scrollbar* NetscapePlugin::horizontalScrollbar()
{
    return 0;
}

Scrollbar* NetscapePlugin::verticalScrollbar()
{
    return 0;
}

bool NetscapePlugin::supportsSnapshotting() const
{
#if PLATFORM(MAC)
    return m_pluginModule && m_pluginModule->pluginQuirks().contains(PluginQuirks::SupportsSnapshotting);
#endif
    return false;
}

PassRefPtr<WebCore::SharedBuffer> NetscapePlugin::liveResourceData() const
{
    return 0;
}

IntPoint NetscapePlugin::convertToRootView(const IntPoint& pointInPluginCoordinates) const
{
    return m_pluginToRootViewTransform.mapPoint(pointInPluginCoordinates);
}

bool NetscapePlugin::convertFromRootView(const IntPoint& pointInRootViewCoordinates, IntPoint& pointInPluginCoordinates)
{
    if (!m_pluginToRootViewTransform.isInvertible())
        return false;

    pointInPluginCoordinates = m_pluginToRootViewTransform.inverse().mapPoint(pointInRootViewCoordinates);
    return true;
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
