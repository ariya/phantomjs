/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "InspectorPageAgent.h"

#if ENABLE(INSPECTOR)

#include "Base64.h"
#include "CachedCSSStyleSheet.h"
#include "CachedResource.h"
#include "CachedResourceLoader.h"
#include "CachedScript.h"
#include "Cookie.h"
#include "CookieJar.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLNames.h"
#include "InjectedScriptManager.h"
#include "InspectorFrontend.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "MemoryCache.h"
#include "Page.h"
#include "ScriptObject.h"
#include "SharedBuffer.h"
#include "TextEncoding.h"
#include "UserGestureIndicator.h"
#include "WindowFeatures.h"

#include <wtf/CurrentTime.h>
#include <wtf/ListHashSet.h>

namespace WebCore {

static bool decodeSharedBuffer(PassRefPtr<SharedBuffer> buffer, const String& textEncodingName, String* result)
{
    if (buffer) {
        TextEncoding encoding(textEncodingName);
        if (!encoding.isValid())
            encoding = WindowsLatin1Encoding();
        *result = encoding.decode(buffer->data(), buffer->size());
        return true;
    }
    return false;
}

static bool prepareCachedResourceBuffer(CachedResource* cachedResource, bool* hasZeroSize)
{
    *hasZeroSize = false;
    if (!cachedResource)
        return false;

    // Zero-sized resources don't have data at all -- so fake the empty buffer, instead of indicating error by returning 0.
    if (!cachedResource->encodedSize()) {
        *hasZeroSize = true;
        return true;
    }

    if (cachedResource->isPurgeable()) {
        // If the resource is purgeable then make it unpurgeable to get
        // get its data. This might fail, in which case we return an
        // empty String.
        // FIXME: should we do something else in the case of a purged
        // resource that informs the user why there is no data in the
        // inspector?
        if (!cachedResource->makePurgeable(false))
            return false;
    }

    return true;
}

static bool decodeCachedResource(CachedResource* cachedResource, String* result)
{
    bool hasZeroSize;
    bool prepared = prepareCachedResourceBuffer(cachedResource, &hasZeroSize);
    if (!prepared)
        return false;

    if (cachedResource) {
        switch (cachedResource->type()) {
        case CachedResource::CSSStyleSheet:
            *result = static_cast<CachedCSSStyleSheet*>(cachedResource)->sheetText();
            return true;
        case CachedResource::Script:
            *result = static_cast<CachedScript*>(cachedResource)->script();
            return true;
        default:
            if (hasZeroSize) {
                *result = "";
                return true;
            }
            return decodeSharedBuffer(cachedResource->data(), cachedResource->encoding(), result);
        }
    }
    return false;
}

PassOwnPtr<InspectorPageAgent> InspectorPageAgent::create(InstrumentingAgents* instrumentingAgents, Page* page, InjectedScriptManager* injectedScriptManager)
{
    return adoptPtr(new InspectorPageAgent(instrumentingAgents, page, injectedScriptManager));
}

void InspectorPageAgent::resourceContent(ErrorString* errorString, Frame* frame, const KURL& url, String* result)
{
    if (!frame) {
        *errorString = "No frame to get resource content for";
        return;
    }

    FrameLoader* frameLoader = frame->loader();
    DocumentLoader* loader = frameLoader->documentLoader();
    RefPtr<SharedBuffer> buffer;
    bool success = false;
    if (equalIgnoringFragmentIdentifier(url, loader->url())) {
        String textEncodingName = frame->document()->inputEncoding();
        buffer = frameLoader->documentLoader()->mainResourceData();
        success = decodeSharedBuffer(buffer, textEncodingName, result);
    }
    if (!success)
        success = decodeCachedResource(cachedResource(frame, url), result);

    if (!success)
        *errorString = "No resource with given URL found";
}

void InspectorPageAgent::resourceContentBase64(ErrorString* errorString, Frame* frame, const KURL& url, String* result)
{
    String textEncodingName;
    RefPtr<SharedBuffer> data = InspectorPageAgent::resourceData(frame, url, &textEncodingName);
    if (!data) {
        *result = String();
        *errorString = "No resource with given URL found";
        return;
    }

    *result = base64Encode(data->data(), data->size());
}

PassRefPtr<SharedBuffer> InspectorPageAgent::resourceData(Frame* frame, const KURL& url, String* textEncodingName)
{
    RefPtr<SharedBuffer> buffer;
    FrameLoader* frameLoader = frame->loader();
    DocumentLoader* loader = frameLoader->documentLoader();
    if (equalIgnoringFragmentIdentifier(url, loader->url())) {
        *textEncodingName = frame->document()->inputEncoding();
        buffer = frameLoader->documentLoader()->mainResourceData();
        if (buffer)
            return buffer;
    }

    CachedResource* cachedResource = InspectorPageAgent::cachedResource(frame, url);
    if (!cachedResource)
        return 0;

    bool hasZeroSize;
    bool prepared = prepareCachedResourceBuffer(cachedResource, &hasZeroSize);
    if (!prepared)
        return 0;

    *textEncodingName = cachedResource->encoding();
    return hasZeroSize ? SharedBuffer::create() : cachedResource->data();
}

CachedResource* InspectorPageAgent::cachedResource(Frame* frame, const KURL& url)
{
    CachedResource* cachedResource = frame->document()->cachedResourceLoader()->cachedResource(url);
    if (!cachedResource)
        cachedResource = memoryCache()->resourceForURL(url);
    return cachedResource;
}

String InspectorPageAgent::resourceTypeString(InspectorPageAgent::ResourceType resourceType)
{
    switch (resourceType) {
    case DocumentResource:
        return "Document";
    case ImageResource:
        return "Image";
    case FontResource:
        return "Font";
    case StylesheetResource:
        return "Stylesheet";
    case ScriptResource:
        return "Script";
    case XHRResource:
        return "XHR";
    case WebSocketResource:
        return "WebSocket";
    case OtherResource:
        return "Other";
    }
    return "Other";
}

InspectorPageAgent::ResourceType InspectorPageAgent::cachedResourceType(const CachedResource& cachedResource)
{
    switch (cachedResource.type()) {
    case CachedResource::ImageResource:
        return InspectorPageAgent::ImageResource;
    case CachedResource::FontResource:
        return InspectorPageAgent::FontResource;
    case CachedResource::CSSStyleSheet:
        // Fall through.
#if ENABLE(XSLT)
    case CachedResource::XSLStyleSheet:
#endif
        return InspectorPageAgent::StylesheetResource;
    case CachedResource::Script:
        return InspectorPageAgent::ScriptResource;
    default:
        break;
    }
    return InspectorPageAgent::OtherResource;
}

String InspectorPageAgent::cachedResourceTypeString(const CachedResource& cachedResource)
{
    return resourceTypeString(cachedResourceType(cachedResource));
}

InspectorPageAgent::InspectorPageAgent(InstrumentingAgents* instrumentingAgents, Page* page, InjectedScriptManager* injectedScriptManager)
    : m_instrumentingAgents(instrumentingAgents)
    , m_page(page)
    , m_injectedScriptManager(injectedScriptManager)
    , m_frontend(0)
{
}

void InspectorPageAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend->page();
    m_instrumentingAgents->setInspectorPageAgent(this);
}

void InspectorPageAgent::clearFrontend()
{
    m_instrumentingAgents->setInspectorPageAgent(0);
    m_frontend = 0;
}

void InspectorPageAgent::addScriptToEvaluateOnLoad(ErrorString*, const String& source)
{
    m_scriptsToEvaluateOnLoad.append(source);
}

void InspectorPageAgent::removeAllScriptsToEvaluateOnLoad(ErrorString*)
{
    m_scriptsToEvaluateOnLoad.clear();
}

void InspectorPageAgent::reload(ErrorString*, const bool* const optionalIgnoreCache)
{
    m_page->mainFrame()->loader()->reload(optionalIgnoreCache ? *optionalIgnoreCache : false);
}

void InspectorPageAgent::open(ErrorString*, const String& url, const bool* const inNewWindow)
{
    Frame* mainFrame = m_page->mainFrame();
    Frame* frame;
    if (inNewWindow && *inNewWindow) {
        FrameLoadRequest request(mainFrame->document()->securityOrigin(), ResourceRequest(), "_blank");

        bool created;
        WindowFeatures windowFeatures;
        frame = WebCore::createWindow(mainFrame, mainFrame, request, windowFeatures, created);
        if (!frame)
            return;

        frame->loader()->setOpener(mainFrame);
        frame->page()->setOpenedByDOM();
    } else
        frame = mainFrame;

    UserGestureIndicator indicator(DefinitelyProcessingUserGesture);
    frame->loader()->changeLocation(mainFrame->document()->securityOrigin(), frame->loader()->completeURL(url), "", false, false);
}

static PassRefPtr<InspectorObject> buildObjectForCookie(const Cookie& cookie)
{
    RefPtr<InspectorObject> value = InspectorObject::create();
    value->setString("name", cookie.name);
    value->setString("value", cookie.value);
    value->setString("domain", cookie.domain);
    value->setString("path", cookie.path);
    value->setNumber("expires", cookie.expires);
    value->setNumber("size", (cookie.name.length() + cookie.value.length()));
    value->setBoolean("httpOnly", cookie.httpOnly);
    value->setBoolean("secure", cookie.secure);
    value->setBoolean("session", cookie.session);
    return value;
}

static PassRefPtr<InspectorArray> buildArrayForCookies(ListHashSet<Cookie>& cookiesList)
{
    RefPtr<InspectorArray> cookies = InspectorArray::create();

    ListHashSet<Cookie>::iterator end = cookiesList.end();
    ListHashSet<Cookie>::iterator it = cookiesList.begin();
    for (int i = 0; it != end; ++it, i++)
        cookies->pushObject(buildObjectForCookie(*it));

    return cookies;
}

void InspectorPageAgent::getCookies(ErrorString*, RefPtr<InspectorArray>* cookies, WTF::String* cookiesString)
{
    // If we can get raw cookies.
    ListHashSet<Cookie> rawCookiesList;

    // If we can't get raw cookies - fall back to String representation
    String stringCookiesList;

    // Return value to getRawCookies should be the same for every call because
    // the return value is platform/network backend specific, and the call will
    // always return the same true/false value.
    bool rawCookiesImplemented = false;

    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext(mainFrame())) {
        Document* document = frame->document();
        const CachedResourceLoader::DocumentResourceMap& allResources = document->cachedResourceLoader()->allCachedResources();
        CachedResourceLoader::DocumentResourceMap::const_iterator end = allResources.end();
        for (CachedResourceLoader::DocumentResourceMap::const_iterator it = allResources.begin(); it != end; ++it) {
            Vector<Cookie> docCookiesList;
            rawCookiesImplemented = getRawCookies(document, KURL(ParsedURLString, it->second->url()), docCookiesList);

            if (!rawCookiesImplemented) {
                // FIXME: We need duplication checking for the String representation of cookies.
                ExceptionCode ec = 0;
                stringCookiesList += document->cookie(ec);
                // Exceptions are thrown by cookie() in sandboxed frames. That won't happen here
                // because "document" is the document of the main frame of the page.
                ASSERT(!ec);
            } else {
                int cookiesSize = docCookiesList.size();
                for (int i = 0; i < cookiesSize; i++) {
                    if (!rawCookiesList.contains(docCookiesList[i]))
                        rawCookiesList.add(docCookiesList[i]);
                }
            }
        }
    }

    if (rawCookiesImplemented)
        *cookies = buildArrayForCookies(rawCookiesList);
    else
        *cookiesString = stringCookiesList;
}

void InspectorPageAgent::deleteCookie(ErrorString*, const String& cookieName, const String& domain)
{
    for (Frame* frame = m_page->mainFrame(); frame; frame = frame->tree()->traverseNext(m_page->mainFrame())) {
        Document* document = frame->document();
        if (document->url().host() != domain)
            continue;
        const CachedResourceLoader::DocumentResourceMap& allResources = document->cachedResourceLoader()->allCachedResources();
        CachedResourceLoader::DocumentResourceMap::const_iterator end = allResources.end();
        for (CachedResourceLoader::DocumentResourceMap::const_iterator it = allResources.begin(); it != end; ++it)
            WebCore::deleteCookie(document, KURL(ParsedURLString, it->second->url()), cookieName);
    }
}

void InspectorPageAgent::getResourceTree(ErrorString*, RefPtr<InspectorObject>* object)
{
    *object = buildObjectForFrameTree(m_page->mainFrame());
}

void InspectorPageAgent::getResourceContent(ErrorString* errorString, const String& frameId, const String& url, const bool* const optionalBase64Encode, String* content)
{
    Frame* frame = frameForId(frameId);
    if (!frame) {
        *errorString = "No frame for given id found";
        return;
    }
    if (optionalBase64Encode ? *optionalBase64Encode : false)
        InspectorPageAgent::resourceContentBase64(errorString, frame, KURL(ParsedURLString, url), content);
    else
        InspectorPageAgent::resourceContent(errorString, frame, KURL(ParsedURLString, url), content);
}

void InspectorPageAgent::domContentEventFired()
{
     m_frontend->domContentEventFired(currentTime());
}

void InspectorPageAgent::loadEventFired()
{
     m_frontend->loadEventFired(currentTime());
}

void InspectorPageAgent::frameNavigated(DocumentLoader* loader)
{
    m_frontend->frameNavigated(buildObjectForFrame(loader->frame()), loaderId(loader));
}

void InspectorPageAgent::frameDetached(Frame* frame)
{
    m_frontend->frameDetached(frameId(frame));
}

void InspectorPageAgent::didClearWindowObjectInWorld(Frame* frame, DOMWrapperWorld* world)
{
    if (world != mainThreadNormalWorld())
        return;

    if (frame == m_page->mainFrame())
        m_injectedScriptManager->discardInjectedScripts();

    if (m_scriptsToEvaluateOnLoad.size()) {
        ScriptState* scriptState = mainWorldScriptState(frame);
        for (Vector<String>::iterator it = m_scriptsToEvaluateOnLoad.begin();
             it != m_scriptsToEvaluateOnLoad.end(); ++it) {
            m_injectedScriptManager->injectScript(*it, scriptState);
        }
    }
}

Frame* InspectorPageAgent::mainFrame()
{
    return m_page->mainFrame();
}

static String pointerAsId(void* pointer)
{
    unsigned long long address = reinterpret_cast<uintptr_t>(pointer);
    // We want 0 to be "", so that JavaScript checks for if (frameId) worked.
    return String::format("%.0llX", address);
}

Frame* InspectorPageAgent::frameForId(const String& frameId)
{
    Frame* mainFrame = m_page->mainFrame();
    for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext(mainFrame)) {
        if (pointerAsId(frame) == frameId)
            return frame;
    }
    return 0;
}

String InspectorPageAgent::frameId(Frame* frame)
{
    return pointerAsId(frame);
}

String InspectorPageAgent::loaderId(DocumentLoader* loader)
{
    return pointerAsId(loader);
}

PassRefPtr<InspectorObject> InspectorPageAgent::buildObjectForFrame(Frame* frame)
{
    RefPtr<InspectorObject> frameObject = InspectorObject::create();
    frameObject->setString("id", frameId(frame));
    frameObject->setString("parentId", frameId(frame->tree()->parent()));
    if (frame->ownerElement()) {
        String name = frame->ownerElement()->getAttribute(HTMLNames::nameAttr);
        if (name.isEmpty())
            name = frame->ownerElement()->getAttribute(HTMLNames::idAttr);
        frameObject->setString("name", name);
    }
    frameObject->setString("url", frame->document()->url().string());
    frameObject->setString("loaderId", loaderId(frame->loader()->documentLoader()));

    return frameObject;
}

PassRefPtr<InspectorObject> InspectorPageAgent::buildObjectForFrameTree(Frame* frame)
{
    RefPtr<InspectorObject> result = InspectorObject::create();
    RefPtr<InspectorObject> frameObject = buildObjectForFrame(frame);
    result->setObject("frame", frameObject);

    RefPtr<InspectorArray> subresources = InspectorArray::create();
    result->setArray("resources", subresources);
    const CachedResourceLoader::DocumentResourceMap& allResources = frame->document()->cachedResourceLoader()->allCachedResources();
    CachedResourceLoader::DocumentResourceMap::const_iterator end = allResources.end();
    for (CachedResourceLoader::DocumentResourceMap::const_iterator it = allResources.begin(); it != end; ++it) {
        CachedResource* cachedResource = it->second.get();
        RefPtr<InspectorObject> resourceObject = InspectorObject::create();
        resourceObject->setString("url", cachedResource->url());
        resourceObject->setString("type", cachedResourceTypeString(*cachedResource));
        subresources->pushValue(resourceObject);
    }

    RefPtr<InspectorArray> childrenArray;
    for (Frame* child = frame->tree()->firstChild(); child; child = child->tree()->nextSibling()) {
        if (!childrenArray) {
            childrenArray = InspectorArray::create();
            result->setArray("childFrames", childrenArray);
        }
        childrenArray->pushObject(buildObjectForFrameTree(child));
    }
    return result;
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
