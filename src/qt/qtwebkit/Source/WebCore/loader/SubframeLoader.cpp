/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) Research In Motion Limited 2009. All rights reserved.
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

#include "config.h"
#include "SubframeLoader.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "ContentSecurityPolicy.h"
#include "DiagnosticLoggingKeys.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "HTMLAppletElement.h"
#include "HTMLFrameElementBase.h"
#include "HTMLNames.h"
#include "HTMLPlugInImageElement.h"
#include "MIMETypeRegistry.h"
#include "Page.h"
#include "PluginData.h"
#include "PluginDocument.h"
#include "RenderEmbeddedObject.h"
#include "RenderView.h"
#include "ScriptController.h"
#include "SecurityOrigin.h"
#include "SecurityPolicy.h"
#include "Settings.h"

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
#include "HTMLMediaElement.h"
#include "RenderVideo.h"
#endif

namespace WebCore {
    
using namespace HTMLNames;

SubframeLoader::SubframeLoader(Frame* frame)
    : m_containsPlugins(false)
    , m_frame(frame)
{
}

void SubframeLoader::clear()
{
    m_containsPlugins = false;
}

bool SubframeLoader::requestFrame(HTMLFrameOwnerElement* ownerElement, const String& urlString, const AtomicString& frameName, bool lockHistory, bool lockBackForwardList)
{
    // Support for <frame src="javascript:string">
    KURL scriptURL;
    KURL url;
    if (protocolIsJavaScript(urlString)) {
        scriptURL = completeURL(urlString); // completeURL() encodes the URL.
        url = blankURL();
    } else
        url = completeURL(urlString);

    Frame* frame = loadOrRedirectSubframe(ownerElement, url, frameName, lockHistory, lockBackForwardList);
    if (!frame)
        return false;

    if (!scriptURL.isEmpty())
        frame->script()->executeIfJavaScriptURL(scriptURL);

    return true;
}
    
bool SubframeLoader::resourceWillUsePlugin(const String& url, const String& mimeType, bool shouldPreferPlugInsForImages)
{
    KURL completedURL;
    if (!url.isEmpty())
        completedURL = completeURL(url);

    bool useFallback;
    return shouldUsePlugin(completedURL, mimeType, shouldPreferPlugInsForImages, false, useFallback);
}

bool SubframeLoader::pluginIsLoadable(HTMLPlugInImageElement* pluginElement, const KURL& url, const String& mimeType)
{
    Settings* settings = m_frame->settings();
    if (!settings)
        return false;

    if (MIMETypeRegistry::isJavaAppletMIMEType(mimeType)) {
        if (!settings->isJavaEnabled())
            return false;
        if (document() && document()->securityOrigin()->isLocal() && !settings->isJavaEnabledForLocalFiles())
            return false;
    }

    if (document()) {
        if (document()->isSandboxed(SandboxPlugins))
            return false;

        if (!document()->securityOrigin()->canDisplay(url)) {
            FrameLoader::reportLocalLoadFailed(m_frame, url.string());
            return false;
        }

        String declaredMimeType = document()->isPluginDocument() && document()->ownerElement() ?
            document()->ownerElement()->fastGetAttribute(HTMLNames::typeAttr) :
            pluginElement->fastGetAttribute(HTMLNames::typeAttr);
        if (!document()->contentSecurityPolicy()->allowObjectFromSource(url)
            || !document()->contentSecurityPolicy()->allowPluginType(mimeType, declaredMimeType, url)) {
            RenderEmbeddedObject* renderer = pluginElement->renderEmbeddedObject();
            renderer->setPluginUnavailabilityReason(RenderEmbeddedObject::PluginBlockedByContentSecurityPolicy);
            return false;
        }

        if (m_frame->loader() && !m_frame->loader()->mixedContentChecker()->canRunInsecureContent(document()->securityOrigin(), url))
            return false;
    }

    return true;
}

bool SubframeLoader::requestPlugin(HTMLPlugInImageElement* ownerElement, const KURL& url, const String& mimeType, const Vector<String>& paramNames, const Vector<String>& paramValues, bool useFallback)
{
    // Application plug-ins are plug-ins implemented by the user agent, for example Qt plug-ins,
    // as opposed to third-party code such as Flash. The user agent decides whether or not they are
    // permitted, rather than WebKit.
    if ((!allowPlugins(AboutToInstantiatePlugin) && !MIMETypeRegistry::isApplicationPluginMIMEType(mimeType)))
        return false;

    if (!pluginIsLoadable(ownerElement, url, mimeType))
        return false;

    ASSERT(ownerElement->hasTagName(objectTag) || ownerElement->hasTagName(embedTag));
    return loadPlugin(ownerElement, url, mimeType, paramNames, paramValues, useFallback);
}

static String findPluginMIMETypeFromURL(Page* page, const String& url)
{
    if (!url)
        return String();

    size_t dotIndex = url.reverseFind('.');
    if (dotIndex == notFound)
        return String();

    String extension = url.substring(dotIndex + 1);

    PluginData* pluginData = page->pluginData();
    if (!pluginData)
        return String();

    for (size_t i = 0; i < pluginData->mimes().size(); ++i) {
        const MimeClassInfo& mimeClassInfo = pluginData->mimes()[i];
        for (size_t j = 0; j < mimeClassInfo.extensions.size(); ++j) {
            if (equalIgnoringCase(extension, mimeClassInfo.extensions[j]))
                return mimeClassInfo.type;
        }
    }

    return String();
}

static void logPluginRequest(Page* page, const String& mimeType, const String& url, bool success)
{
    if (!page || !page->settings()->diagnosticLoggingEnabled())
        return;

    String newMIMEType = mimeType;
    if (!newMIMEType) {
        // Try to figure out the MIME type from the URL extension.
        newMIMEType = findPluginMIMETypeFromURL(page, url);
        if (!newMIMEType)
            return;
    }

    PluginData* pluginData = page->pluginData();
    String pluginFile = pluginData ? pluginData->pluginFileForMimeType(newMIMEType) : String();
    String description = !pluginFile ? newMIMEType : pluginFile;

    ChromeClient* client = page->chrome().client();
    client->logDiagnosticMessage(success ? DiagnosticLoggingKeys::pluginLoadedKey() : DiagnosticLoggingKeys::pluginLoadingFailedKey(), description, DiagnosticLoggingKeys::noopKey());

    if (!page->hasSeenAnyPlugin())
        client->logDiagnosticMessage(DiagnosticLoggingKeys::pageContainsAtLeastOnePluginKey(), emptyString(), DiagnosticLoggingKeys::noopKey());
    
    if (!page->hasSeenPlugin(description))
        client->logDiagnosticMessage(DiagnosticLoggingKeys::pageContainsPluginKey(), description, DiagnosticLoggingKeys::noopKey());

    page->sawPlugin(description);
}

bool SubframeLoader::requestObject(HTMLPlugInImageElement* ownerElement, const String& url, const AtomicString& frameName, const String& mimeType, const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    if (url.isEmpty() && mimeType.isEmpty())
        return false;

    // FIXME: None of this code should use renderers!
    RenderEmbeddedObject* renderer = ownerElement->renderEmbeddedObject();
    ASSERT(renderer);
    if (!renderer)
        return false;

    KURL completedURL;
    if (!url.isEmpty())
        completedURL = completeURL(url);

    bool useFallback;
    if (shouldUsePlugin(completedURL, mimeType, ownerElement->shouldPreferPlugInsForImages(), renderer->hasFallbackContent(), useFallback)) {
        bool success = requestPlugin(ownerElement, completedURL, mimeType, paramNames, paramValues, useFallback);
        logPluginRequest(document()->page(), mimeType, completedURL, success);
        return success;
    }

    // If the plug-in element already contains a subframe, loadOrRedirectSubframe will re-use it. Otherwise,
    // it will create a new frame and set it as the RenderPart's widget, causing what was previously 
    // in the widget to be torn down.
    return loadOrRedirectSubframe(ownerElement, completedURL, frameName, true, true);
}

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
PassRefPtr<Widget> SubframeLoader::loadMediaPlayerProxyPlugin(Node* node, const KURL& url,
    const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    ASSERT(node->hasTagName(videoTag) || isHTMLAudioElement(node));

    KURL completedURL;
    if (!url.isEmpty())
        completedURL = completeURL(url);

    if (!m_frame->document()->securityOrigin()->canDisplay(completedURL)) {
        FrameLoader::reportLocalLoadFailed(m_frame, completedURL.string());
        return 0;
    }

    if (!m_frame->document()->contentSecurityPolicy()->allowMediaFromSource(completedURL))
        return 0;

    HTMLMediaElement* mediaElement = toHTMLMediaElement(node);
    RenderPart* renderer = toRenderPart(node->renderer());
    IntSize size;

    if (renderer)
        size = roundedIntSize(LayoutSize(renderer->contentWidth(), renderer->contentHeight()));
    else if (mediaElement->isVideo())
        size = RenderVideo::defaultSize();

    if (!m_frame->loader()->mixedContentChecker()->canRunInsecureContent(m_frame->document()->securityOrigin(), completedURL))
        return 0;

    RefPtr<Widget> widget = m_frame->loader()->client()->createMediaPlayerProxyPlugin(size, mediaElement, completedURL,
                                         paramNames, paramValues, "application/x-media-element-proxy-plugin");

    if (widget && renderer) {
        renderer->setWidget(widget);
        renderer->node()->setNeedsStyleRecalc(SyntheticStyleChange);
    }
    m_containsPlugins = true;

    return widget ? widget.release() : 0;
}
#endif // ENABLE(PLUGIN_PROXY_FOR_VIDEO)

PassRefPtr<Widget> SubframeLoader::createJavaAppletWidget(const IntSize& size, HTMLAppletElement* element, const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    String baseURLString;
    String codeBaseURLString;

    for (size_t i = 0; i < paramNames.size(); ++i) {
        if (equalIgnoringCase(paramNames[i], "baseurl"))
            baseURLString = paramValues[i];
        else if (equalIgnoringCase(paramNames[i], "codebase"))
            codeBaseURLString = paramValues[i];
    }

    if (!codeBaseURLString.isEmpty()) {
        KURL codeBaseURL = completeURL(codeBaseURLString);
        if (!element->document()->securityOrigin()->canDisplay(codeBaseURL)) {
            FrameLoader::reportLocalLoadFailed(m_frame, codeBaseURL.string());
            return 0;
        }

        const char javaAppletMimeType[] = "application/x-java-applet";
        if (!element->document()->contentSecurityPolicy()->allowObjectFromSource(codeBaseURL)
            || !element->document()->contentSecurityPolicy()->allowPluginType(javaAppletMimeType, javaAppletMimeType, codeBaseURL))
            return 0;
    }

    if (baseURLString.isEmpty())
        baseURLString = m_frame->document()->baseURL().string();
    KURL baseURL = completeURL(baseURLString);

    RefPtr<Widget> widget;
    if (allowPlugins(AboutToInstantiatePlugin))
        widget = m_frame->loader()->client()->createJavaAppletWidget(size, element, baseURL, paramNames, paramValues);

    logPluginRequest(document()->page(), element->serviceType(), String(), widget);

    if (!widget) {
        RenderEmbeddedObject* renderer = element->renderEmbeddedObject();

        if (!renderer->isPluginUnavailable())
            renderer->setPluginUnavailabilityReason(RenderEmbeddedObject::PluginMissing);
        return 0;
    }

    m_containsPlugins = true;
    return widget;
}

Frame* SubframeLoader::loadOrRedirectSubframe(HTMLFrameOwnerElement* ownerElement, const KURL& url, const AtomicString& frameName, bool lockHistory, bool lockBackForwardList)
{
    Frame* frame = ownerElement->contentFrame();
    if (frame)
        frame->navigationScheduler()->scheduleLocationChange(m_frame->document()->securityOrigin(), url.string(), m_frame->loader()->outgoingReferrer(), lockHistory, lockBackForwardList);
    else
        frame = loadSubframe(ownerElement, url, frameName, m_frame->loader()->outgoingReferrer());

    ASSERT(ownerElement->contentFrame() == frame || !ownerElement->contentFrame());
    return ownerElement->contentFrame();
}

Frame* SubframeLoader::loadSubframe(HTMLFrameOwnerElement* ownerElement, const KURL& url, const String& name, const String& referrer)
{
    RefPtr<Frame> protect(m_frame);

    bool allowsScrolling = true;
    int marginWidth = -1;
    int marginHeight = -1;
    if (ownerElement->hasTagName(frameTag) || ownerElement->hasTagName(iframeTag)) {
        HTMLFrameElementBase* frameElementBase = toHTMLFrameElementBase(ownerElement);
        allowsScrolling = frameElementBase->scrollingMode() != ScrollbarAlwaysOff;
        marginWidth = frameElementBase->marginWidth();
        marginHeight = frameElementBase->marginHeight();
    }

    if (!ownerElement->document()->securityOrigin()->canDisplay(url)) {
        FrameLoader::reportLocalLoadFailed(m_frame, url.string());
        return 0;
    }

    String referrerToUse = SecurityPolicy::generateReferrerHeader(ownerElement->document()->referrerPolicy(), url, referrer);
    RefPtr<Frame> frame = m_frame->loader()->client()->createFrame(url, name, ownerElement, referrerToUse, allowsScrolling, marginWidth, marginHeight);

    if (!frame)  {
        m_frame->loader()->checkCallImplicitClose();
        return 0;
    }
    
    // All new frames will have m_isComplete set to true at this point due to synchronously loading
    // an empty document in FrameLoader::init(). But many frames will now be starting an
    // asynchronous load of url, so we set m_isComplete to false and then check if the load is
    // actually completed below. (Note that we set m_isComplete to false even for synchronous
    // loads, so that checkCompleted() below won't bail early.)
    // FIXME: Can we remove this entirely? m_isComplete normally gets set to false when a load is committed.
    frame->loader()->started();
   
    RenderObject* renderer = ownerElement->renderer();
    FrameView* view = frame->view();
    if (renderer && renderer->isWidget() && view)
        toRenderWidget(renderer)->setWidget(view);
    
    m_frame->loader()->checkCallImplicitClose();
    
    // Some loads are performed synchronously (e.g., about:blank and loads
    // cancelled by returning a null ResourceRequest from requestFromDelegate).
    // In these cases, the synchronous load would have finished
    // before we could connect the signals, so make sure to send the 
    // completed() signal for the child by hand and mark the load as being
    // complete.
    // FIXME: In this case the Frame will have finished loading before 
    // it's being added to the child list. It would be a good idea to
    // create the child first, then invoke the loader separately.
    if (frame->loader()->state() == FrameStateComplete && !frame->loader()->policyDocumentLoader())
        frame->loader()->checkCompleted();

    return frame.get();
}

bool SubframeLoader::allowPlugins(ReasonForCallingAllowPlugins reason)
{
    Settings* settings = m_frame->settings();
    bool allowed = m_frame->loader()->client()->allowPlugins(settings && settings->arePluginsEnabled());
    if (!allowed && reason == AboutToInstantiatePlugin)
        m_frame->loader()->client()->didNotAllowPlugins();
    return allowed;
}

bool SubframeLoader::shouldUsePlugin(const KURL& url, const String& mimeType, bool shouldPreferPlugInsForImages, bool hasFallback, bool& useFallback)
{
    if (m_frame->loader()->client()->shouldAlwaysUsePluginDocument(mimeType)) {
        useFallback = false;
        return true;
    }

    // Allow other plug-ins to win over QuickTime because if the user has installed a plug-in that
    // can handle TIFF (which QuickTime can also handle) they probably intended to override QT.
    if (m_frame->page() && (mimeType == "image/tiff" || mimeType == "image/tif" || mimeType == "image/x-tiff")) {
        const PluginData* pluginData = m_frame->page()->pluginData();
        String pluginName = pluginData ? pluginData->pluginNameForMimeType(mimeType) : String();
        if (!pluginName.isEmpty() && !pluginName.contains("QuickTime", false)) 
            return true;
    }
        
    ObjectContentType objectType = m_frame->loader()->client()->objectContentType(url, mimeType, shouldPreferPlugInsForImages);
    // If an object's content can't be handled and it has no fallback, let
    // it be handled as a plugin to show the broken plugin icon.
    useFallback = objectType == ObjectContentNone && hasFallback;
    return objectType == ObjectContentNone || objectType == ObjectContentNetscapePlugin || objectType == ObjectContentOtherPlugin;
}

Document* SubframeLoader::document() const
{
    return m_frame->document();
}

bool SubframeLoader::loadPlugin(HTMLPlugInImageElement* pluginElement, const KURL& url, const String& mimeType,
    const Vector<String>& paramNames, const Vector<String>& paramValues, bool useFallback)
{
    RenderEmbeddedObject* renderer = pluginElement->renderEmbeddedObject();

    // FIXME: This code should not depend on renderer!
    if (!renderer || useFallback)
        return false;

    pluginElement->subframeLoaderWillCreatePlugIn(url);

    IntSize contentSize = roundedIntSize(LayoutSize(renderer->contentWidth(), renderer->contentHeight()));
    bool loadManually = document()->isPluginDocument() && !m_containsPlugins && toPluginDocument(document())->shouldLoadPluginManually();
    RefPtr<Widget> widget = m_frame->loader()->client()->createPlugin(contentSize,
        pluginElement, url, paramNames, paramValues, mimeType, loadManually);

    if (!widget) {
        if (!renderer->isPluginUnavailable())
            renderer->setPluginUnavailabilityReason(RenderEmbeddedObject::PluginMissing);
        return false;
    }

    pluginElement->subframeLoaderDidCreatePlugIn(widget.get());
    renderer->setWidget(widget);
    m_containsPlugins = true;
 
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    pluginElement->setNeedsStyleRecalc(SyntheticStyleChange);
#endif
    return true;
}

KURL SubframeLoader::completeURL(const String& url) const
{
    ASSERT(m_frame->document());
    return m_frame->document()->completeURL(url);
}

} // namespace WebCore
