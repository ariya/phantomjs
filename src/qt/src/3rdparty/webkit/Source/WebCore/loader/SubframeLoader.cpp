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

#include "ContentSecurityPolicy.h"
#include "Frame.h"
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
#include "SecurityOrigin.h"
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

bool SubframeLoader::requestPlugin(HTMLPlugInImageElement* ownerElement, const KURL& url, const String& mimeType, const Vector<String>& paramNames, const Vector<String>& paramValues, bool useFallback)
{
    Settings* settings = m_frame->settings();
    if ((!allowPlugins(AboutToInstantiatePlugin)
         // Application plug-ins are plug-ins implemented by the user agent, for example Qt plug-ins,
         // as opposed to third-party code such as Flash. The user agent decides whether or not they are
         // permitted, rather than WebKit.
         && !MIMETypeRegistry::isApplicationPluginMIMEType(mimeType))
        || (!settings->isJavaEnabled() && MIMETypeRegistry::isJavaAppletMIMEType(mimeType)))
        return false;

    if (m_frame->document()) {
        if (m_frame->document()->securityOrigin()->isSandboxed(SandboxPlugins))
            return false;
        if (!m_frame->document()->contentSecurityPolicy()->allowObjectFromSource(url))
            return false;
    }

    ASSERT(ownerElement->hasTagName(objectTag) || ownerElement->hasTagName(embedTag));
    return loadPlugin(ownerElement, url, mimeType, paramNames, paramValues, useFallback);
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
    if (shouldUsePlugin(completedURL, mimeType, ownerElement->shouldPreferPlugInsForImages(), renderer->hasFallbackContent(), useFallback))
        return requestPlugin(ownerElement, completedURL, mimeType, paramNames, paramValues, useFallback);

    // If the plug-in element already contains a subframe, loadOrRedirectSubframe will re-use it. Otherwise,
    // it will create a new frame and set it as the RenderPart's widget, causing what was previously 
    // in the widget to be torn down.
    return loadOrRedirectSubframe(ownerElement, completedURL, frameName, true, true);
}

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
PassRefPtr<Widget> SubframeLoader::loadMediaPlayerProxyPlugin(Node* node, const KURL& url,
    const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    ASSERT(node->hasTagName(videoTag) || node->hasTagName(audioTag));

    KURL completedURL;
    if (!url.isEmpty())
        completedURL = completeURL(url);

    if (!m_frame->document()->securityOrigin()->canDisplay(completedURL)) {
        FrameLoader::reportLocalLoadFailed(m_frame, completedURL.string());
        return 0;
    }

    if (!m_frame->document()->contentSecurityPolicy()->allowMediaFromSource(completedURL))
        return 0;

    HTMLMediaElement* mediaElement = static_cast<HTMLMediaElement*>(node);
    RenderPart* renderer = toRenderPart(node->renderer());
    IntSize size;

    if (renderer)
        size = IntSize(renderer->contentWidth(), renderer->contentHeight());
    else if (mediaElement->isVideo())
        size = RenderVideo::defaultSize();

    if (!m_frame->loader()->checkIfRunInsecureContent(m_frame->document()->securityOrigin(), completedURL))
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

PassRefPtr<Widget> SubframeLoader::createJavaAppletWidget(const IntSize& size, HTMLAppletElement* element, const HashMap<String, String>& args)
{
    String baseURLString;
    String codeBaseURLString;
    Vector<String> paramNames;
    Vector<String> paramValues;
    HashMap<String, String>::const_iterator end = args.end();
    for (HashMap<String, String>::const_iterator it = args.begin(); it != end; ++it) {
        if (equalIgnoringCase(it->first, "baseurl"))
            baseURLString = it->second;
        else if (equalIgnoringCase(it->first, "codebase"))
            codeBaseURLString = it->second;
        paramNames.append(it->first);
        paramValues.append(it->second);
    }

    if (!codeBaseURLString.isEmpty()) {
        KURL codeBaseURL = completeURL(codeBaseURLString);
        if (!element->document()->securityOrigin()->canDisplay(codeBaseURL)) {
            FrameLoader::reportLocalLoadFailed(m_frame, codeBaseURL.string());
            return 0;
        }

        if (!element->document()->contentSecurityPolicy()->allowObjectFromSource(codeBaseURL))
            return 0;
    }

    if (baseURLString.isEmpty())
        baseURLString = m_frame->document()->baseURL().string();
    KURL baseURL = completeURL(baseURLString);

    RefPtr<Widget> widget;
    if (allowPlugins(AboutToInstantiatePlugin))
        widget = m_frame->loader()->client()->createJavaAppletWidget(size, element, baseURL, paramNames, paramValues);
    if (!widget)
        return 0;

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
    return frame;
}

Frame* SubframeLoader::loadSubframe(HTMLFrameOwnerElement* ownerElement, const KURL& url, const String& name, const String& referrer)
{
    bool allowsScrolling = true;
    int marginWidth = -1;
    int marginHeight = -1;
    if (ownerElement->hasTagName(frameTag) || ownerElement->hasTagName(iframeTag)) {
        HTMLFrameElementBase* o = static_cast<HTMLFrameElementBase*>(ownerElement);
        allowsScrolling = o->scrollingMode() != ScrollbarAlwaysOff;
        marginWidth = o->marginWidth();
        marginHeight = o->marginHeight();
    }

    if (!ownerElement->document()->securityOrigin()->canDisplay(url)) {
        FrameLoader::reportLocalLoadFailed(m_frame, url.string());
        return 0;
    }

    if (!ownerElement->document()->contentSecurityPolicy()->allowChildFrameFromSource(url))
        return 0;

    bool hideReferrer = SecurityOrigin::shouldHideReferrer(url, referrer);
    RefPtr<Frame> frame = m_frame->loader()->client()->createFrame(url, name, ownerElement, hideReferrer ? String() : referrer, allowsScrolling, marginWidth, marginHeight);

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
    if (m_frame->loader()->client()->shouldUsePluginDocument(mimeType)) {
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

    if (!document()->securityOrigin()->canDisplay(url)) {
        FrameLoader::reportLocalLoadFailed(m_frame, url.string());
        return false;
    }

    if (!document()->contentSecurityPolicy()->allowObjectFromSource(url))
        return false;

    FrameLoader* frameLoader = m_frame->loader();
    if (!frameLoader->checkIfRunInsecureContent(document()->securityOrigin(), url))
        return false;

    IntSize contentSize(renderer->contentWidth(), renderer->contentHeight());
    bool loadManually = document()->isPluginDocument() && !m_containsPlugins && toPluginDocument(document())->shouldLoadPluginManually();
    RefPtr<Widget> widget = frameLoader->client()->createPlugin(contentSize,
        pluginElement, url, paramNames, paramValues, mimeType, loadManually);

    if (!widget) {
        renderer->setShowsMissingPluginIndicator();
        return false;
    }

    renderer->setWidget(widget);
    m_containsPlugins = true;
 
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO) || ENABLE(3D_PLUGIN)
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
