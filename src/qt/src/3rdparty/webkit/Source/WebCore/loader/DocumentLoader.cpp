/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "DocumentLoader.h"

#include "ApplicationCacheHost.h"
#include "ArchiveResourceCollection.h"
#include "CachedPage.h"
#include "CachedResourceLoader.h"
#include "DOMWindow.h"
#include "Document.h"
#include "DocumentParser.h"
#include "DocumentWriter.h"
#include "Event.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameTree.h"
#include "HistoryItem.h"
#include "Logging.h"
#include "MainResourceLoader.h"
#include "Page.h"
#include "PlatformString.h"
#include "Settings.h"
#include "SharedBuffer.h"
#include "TextResourceDecoder.h"
#include <wtf/Assertions.h>
#include <wtf/text/CString.h>
#include <wtf/unicode/Unicode.h>

#if ENABLE(WEB_ARCHIVE)
#include "ArchiveFactory.h"
#endif

namespace WebCore {

static void cancelAll(const ResourceLoaderSet& loaders)
{
    Vector<RefPtr<ResourceLoader> > loadersCopy;
    copyToVector(loaders, loadersCopy);
    size_t size = loadersCopy.size();
    for (size_t i = 0; i < size; ++i)
        loadersCopy[i]->cancel();
}

static void setAllDefersLoading(const ResourceLoaderSet& loaders, bool defers)
{
    Vector<RefPtr<ResourceLoader> > loadersCopy;
    copyToVector(loaders, loadersCopy);
    size_t size = loadersCopy.size();
    for (size_t i = 0; i < size; ++i)
        loadersCopy[i]->setDefersLoading(defers);
}

DocumentLoader::DocumentLoader(const ResourceRequest& req, const SubstituteData& substituteData)
    : m_deferMainResourceDataLoad(true)
    , m_frame(0)
    , m_writer(m_frame)
    , m_originalRequest(req)
    , m_substituteData(substituteData)
    , m_originalRequestCopy(req)
    , m_request(req)
    , m_committed(false)
    , m_isStopping(false)
    , m_loading(false)
    , m_gotFirstByte(false)
    , m_primaryLoadComplete(false)
    , m_isClientRedirect(false)
    , m_wasOnloadHandled(false)
    , m_stopRecordingResponses(false)
    , m_substituteResourceDeliveryTimer(this, &DocumentLoader::substituteResourceDeliveryTimerFired)
    , m_didCreateGlobalHistoryEntry(false)
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    , m_applicationCacheHost(adoptPtr(new ApplicationCacheHost(this)))
#endif
{
}

FrameLoader* DocumentLoader::frameLoader() const
{
    if (!m_frame)
        return 0;
    return m_frame->loader();
}

DocumentLoader::~DocumentLoader()
{
    ASSERT(!m_frame || frameLoader()->activeDocumentLoader() != this || !frameLoader()->isLoading());
    if (m_iconLoadDecisionCallback)
        m_iconLoadDecisionCallback->invalidate();
    if (m_iconDataCallback)
        m_iconDataCallback->invalidate();
}

PassRefPtr<SharedBuffer> DocumentLoader::mainResourceData() const
{
    if (m_mainResourceData)
        return m_mainResourceData;
    if (m_mainResourceLoader)
        return m_mainResourceLoader->resourceData();
    return 0;
}

const ResourceRequest& DocumentLoader::originalRequest() const
{
    return m_originalRequest;
}

const ResourceRequest& DocumentLoader::originalRequestCopy() const
{
    return m_originalRequestCopy;
}

const ResourceRequest& DocumentLoader::request() const
{
    return m_request;
}

ResourceRequest& DocumentLoader::request()
{
    return m_request;
}

const KURL& DocumentLoader::url() const
{
    return request().url();
}

void DocumentLoader::replaceRequestURLForSameDocumentNavigation(const KURL& url)
{
    m_originalRequestCopy.setURL(url);
    m_request.setURL(url);
}

void DocumentLoader::setRequest(const ResourceRequest& req)
{
    // Replacing an unreachable URL with alternate content looks like a server-side
    // redirect at this point, but we can replace a committed dataSource.
    bool handlingUnreachableURL = false;

    handlingUnreachableURL = m_substituteData.isValid() && !m_substituteData.failingURL().isEmpty();

    if (handlingUnreachableURL)
        m_committed = false;

    // We should never be getting a redirect callback after the data
    // source is committed, except in the unreachable URL case. It 
    // would be a WebFoundation bug if it sent a redirect callback after commit.
    ASSERT(!m_committed);

    KURL oldURL = m_request.url();
    m_request = req;

    // Only send webView:didReceiveServerRedirectForProvisionalLoadForFrame: if URL changed (and is non-null).
    // Also, don't send it when replacing unreachable URLs with alternate content.
    if (!handlingUnreachableURL && !req.url().isNull() && oldURL != req.url())
        frameLoader()->didReceiveServerRedirectForProvisionalLoadForFrame();
}

void DocumentLoader::setMainDocumentError(const ResourceError& error)
{
    m_mainDocumentError = error;    
    frameLoader()->setMainDocumentError(this, error);
 }

void DocumentLoader::clearErrors()
{
    m_mainDocumentError = ResourceError();
}

void DocumentLoader::mainReceivedError(const ResourceError& error, bool isComplete)
{
    ASSERT(!error.isNull());

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    m_applicationCacheHost->failedLoadingMainResource();
#endif
    
    if (!frameLoader())
        return;
    setMainDocumentError(error);
    if (isComplete)
        frameLoader()->mainReceivedCompleteError(this, error);
}

// Cancels the data source's pending loads.  Conceptually, a data source only loads
// one document at a time, but one document may have many related resources. 
// stopLoading will stop all loads initiated by the data source, 
// but not loads initiated by child frames' data sources -- that's the WebFrame's job.
void DocumentLoader::stopLoading()
{
    // In some rare cases, calling FrameLoader::stopLoading could set m_loading to false.
    // (This can happen when there's a single XMLHttpRequest currently loading and stopLoading causes it
    // to stop loading. Because of this, we need to save it so we don't return early.
    bool loading = m_loading;
    
    if (m_committed) {
        // Attempt to stop the frame if the document loader is loading, or if it is done loading but
        // still  parsing. Failure to do so can cause a world leak.
        Document* doc = m_frame->document();
        
        if (loading || doc->parsing())
            m_frame->loader()->stopLoading(UnloadEventPolicyNone);
    }

    // Always cancel multipart loaders
    cancelAll(m_multipartSubresourceLoaders);

    // Appcache uses ResourceHandle directly, DocumentLoader doesn't count these loads.
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    m_applicationCacheHost->stopLoadingInFrame(m_frame);
#endif

    if (!loading)
        return;
    
    RefPtr<Frame> protectFrame(m_frame);
    RefPtr<DocumentLoader> protectLoader(this);

    m_isStopping = true;

    FrameLoader* frameLoader = DocumentLoader::frameLoader();
    
    if (m_mainResourceLoader)
        // Stop the main resource loader and let it send the cancelled message.
        m_mainResourceLoader->cancel();
    else if (!m_subresourceLoaders.isEmpty())
        // The main resource loader already finished loading. Set the cancelled error on the 
        // document and let the subresourceLoaders send individual cancelled messages below.
        setMainDocumentError(frameLoader->cancelledError(m_request));
    else
        // If there are no resource loaders, we need to manufacture a cancelled message.
        // (A back/forward navigation has no resource loaders because its resources are cached.)
        mainReceivedError(frameLoader->cancelledError(m_request), true);
    
    stopLoadingSubresources();
    stopLoadingPlugIns();
    
    m_isStopping = false;
}

void DocumentLoader::setupForReplace()
{
    frameLoader()->setupForReplace();
    m_committed = false;
}

void DocumentLoader::commitIfReady()
{
    if (m_gotFirstByte && !m_committed) {
        m_committed = true;
        frameLoader()->commitProvisionalLoad();
    }
}

void DocumentLoader::finishedLoading()
{
    m_gotFirstByte = true;   
    commitIfReady();
    if (FrameLoader* loader = frameLoader()) {
        loader->finishedLoadingDocument(this);
        m_writer.end();
    }
}

void DocumentLoader::commitLoad(const char* data, int length)
{
    // Both unloading the old page and parsing the new page may execute JavaScript which destroys the datasource
    // by starting a new load, so retain temporarily.
    RefPtr<Frame> protectFrame(m_frame);
    RefPtr<DocumentLoader> protectLoader(this);

    commitIfReady();
    FrameLoader* frameLoader = DocumentLoader::frameLoader();
    if (!frameLoader)
        return;
#if ENABLE(WEB_ARCHIVE)
    if (ArchiveFactory::isArchiveMimeType(response().mimeType()))
        return;
#endif
    frameLoader->client()->committedLoad(this, data, length);
}

void DocumentLoader::commitData(const char* bytes, int length)
{
    // Set the text encoding.  This is safe to call multiple times.
    bool userChosen = true;
    String encoding = overrideEncoding();
    if (encoding.isNull()) {
        userChosen = false;
        encoding = response().textEncodingName();
    }
    m_writer.setEncoding(encoding, userChosen);
    ASSERT(m_frame->document()->parsing());
    m_writer.addData(bytes, length);
}

bool DocumentLoader::doesProgressiveLoad(const String& MIMEType) const
{
    return !frameLoader()->isReplacing() || MIMEType == "text/html";
}

void DocumentLoader::receivedData(const char* data, int length)
{    
    m_gotFirstByte = true;
    if (doesProgressiveLoad(m_response.mimeType()))
        commitLoad(data, length);
}

void DocumentLoader::setupForReplaceByMIMEType(const String& newMIMEType)
{
    if (!m_gotFirstByte)
        return;
    
    String oldMIMEType = m_response.mimeType();
    
    if (!doesProgressiveLoad(oldMIMEType)) {
        frameLoader()->revertToProvisional(this);
        setupForReplace();
        RefPtr<SharedBuffer> resourceData = mainResourceData();
        commitLoad(resourceData->data(), resourceData->size());
    }
    
    frameLoader()->finishedLoadingDocument(this);
    m_writer.end();
    
    frameLoader()->setReplacing();
    m_gotFirstByte = false;
    
    if (doesProgressiveLoad(newMIMEType)) {
        frameLoader()->revertToProvisional(this);
        setupForReplace();
    }
    
    stopLoadingSubresources();
    stopLoadingPlugIns();
#if ENABLE(WEB_ARCHIVE)
    clearArchiveResources();
#endif
}

void DocumentLoader::updateLoading()
{
    if (!m_frame) {
        setLoading(false);
        return;
    }
    ASSERT(this == frameLoader()->activeDocumentLoader());
    bool wasLoading = m_loading;
    setLoading(frameLoader()->isLoading());

    if (wasLoading && !m_loading) {
        if (DOMWindow* window = m_frame->existingDOMWindow())
            window->finishedLoading();
    }
}

void DocumentLoader::setFrame(Frame* frame)
{
    if (m_frame == frame)
        return;
    ASSERT(frame && !m_frame);
    m_frame = frame;
    m_writer.setFrame(frame);
    attachToFrame();
}

void DocumentLoader::attachToFrame()
{
    ASSERT(m_frame);
}

void DocumentLoader::detachFromFrame()
{
    ASSERT(m_frame);

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    m_applicationCacheHost->setDOMApplicationCache(0);
#endif
    m_frame = 0;
}

void DocumentLoader::prepareForLoadStart()
{
    ASSERT(!m_isStopping);
    setPrimaryLoadComplete(false);
    ASSERT(frameLoader());
    clearErrors();
    
    setLoading(true);
    
    frameLoader()->prepareForLoadStart();
}

void DocumentLoader::setPrimaryLoadComplete(bool flag)
{
    m_primaryLoadComplete = flag;
    if (flag) {
        if (m_mainResourceLoader) {
            m_mainResourceData = m_mainResourceLoader->resourceData();
            m_mainResourceLoader = 0;
        }

        if (this == frameLoader()->activeDocumentLoader())
            updateLoading();
    }
}

bool DocumentLoader::isLoadingInAPISense() const
{
    // Once a frame has loaded, we no longer need to consider subresources,
    // but we still need to consider subframes.
    if (frameLoader()->state() != FrameStateComplete) {
        if (!m_primaryLoadComplete && isLoading())
            return true;
        if (!m_subresourceLoaders.isEmpty())
            return true;
        Document* doc = m_frame->document();
        if (doc->cachedResourceLoader()->requestCount())
            return true;
        if (DocumentParser* parser = doc->parser())
            if (parser->processingData())
                return true;
    }
    return frameLoader()->subframeIsLoading();
}

#if ENABLE(WEB_ARCHIVE)
void DocumentLoader::addAllArchiveResources(Archive* archive)
{
    if (!m_archiveResourceCollection)
        m_archiveResourceCollection = adoptPtr(new ArchiveResourceCollection);
        
    ASSERT(archive);
    if (!archive)
        return;
        
    m_archiveResourceCollection->addAllResources(archive);
}

// FIXME: Adding a resource directly to a DocumentLoader/ArchiveResourceCollection seems like bad design, but is API some apps rely on.
// Can we change the design in a manner that will let us deprecate that API without reducing functionality of those apps?
void DocumentLoader::addArchiveResource(PassRefPtr<ArchiveResource> resource)
{
    if (!m_archiveResourceCollection)
        m_archiveResourceCollection = adoptPtr(new ArchiveResourceCollection);
        
    ASSERT(resource);
    if (!resource)
        return;
        
    m_archiveResourceCollection->addResource(resource);
}

PassRefPtr<Archive> DocumentLoader::popArchiveForSubframe(const String& frameName)
{
    return m_archiveResourceCollection ? m_archiveResourceCollection->popSubframeArchive(frameName) : PassRefPtr<Archive>(0);
}

void DocumentLoader::clearArchiveResources()
{
    m_archiveResourceCollection.clear();
    m_substituteResourceDeliveryTimer.stop();
}

void DocumentLoader::setParsedArchiveData(PassRefPtr<SharedBuffer> data)
{
    m_parsedArchiveData = data;
}

SharedBuffer* DocumentLoader::parsedArchiveData() const
{
    return m_parsedArchiveData.get();
}
#endif // ENABLE(WEB_ARCHIVE)

ArchiveResource* DocumentLoader::archiveResourceForURL(const KURL& url) const
{
    if (!m_archiveResourceCollection)
        return 0;
        
    ArchiveResource* resource = m_archiveResourceCollection->archiveResourceForURL(url);

    return resource && !resource->shouldIgnoreWhenUnarchiving() ? resource : 0;
}

PassRefPtr<ArchiveResource> DocumentLoader::mainResource() const
{
    const ResourceResponse& r = response();
    RefPtr<SharedBuffer> mainResourceBuffer = mainResourceData();
    if (!mainResourceBuffer)
        mainResourceBuffer = SharedBuffer::create();
        
    return ArchiveResource::create(mainResourceBuffer, r.url(), r.mimeType(), r.textEncodingName(), frame()->tree()->uniqueName());
}

PassRefPtr<ArchiveResource> DocumentLoader::subresource(const KURL& url) const
{
    if (!isCommitted())
        return 0;
    
    CachedResource* resource = m_frame->document()->cachedResourceLoader()->cachedResource(url);
    if (!resource || !resource->isLoaded())
        return archiveResourceForURL(url);

    // FIXME: This has the side effect of making the resource non-purgeable.
    // It would be better if it didn't have this permanent effect.
    if (!resource->makePurgeable(false))
        return 0;

    RefPtr<SharedBuffer> data = resource->data();
    if (!data)
        return 0;

    return ArchiveResource::create(data.release(), url, resource->response());
}

void DocumentLoader::getSubresources(Vector<PassRefPtr<ArchiveResource> >& subresources) const
{
    if (!isCommitted())
        return;

    Document* document = m_frame->document();

    const CachedResourceLoader::DocumentResourceMap& allResources = document->cachedResourceLoader()->allCachedResources();
    CachedResourceLoader::DocumentResourceMap::const_iterator end = allResources.end();
    for (CachedResourceLoader::DocumentResourceMap::const_iterator it = allResources.begin(); it != end; ++it) {
        RefPtr<ArchiveResource> subresource = this->subresource(KURL(ParsedURLString, it->second->url()));
        if (subresource)
            subresources.append(subresource.release());
    }

    return;
}

void DocumentLoader::deliverSubstituteResourcesAfterDelay()
{
    if (m_pendingSubstituteResources.isEmpty())
        return;
    ASSERT(m_frame && m_frame->page());
    if (m_frame->page()->defersLoading())
        return;
    if (!m_substituteResourceDeliveryTimer.isActive())
        m_substituteResourceDeliveryTimer.startOneShot(0);
}

void DocumentLoader::substituteResourceDeliveryTimerFired(Timer<DocumentLoader>*)
{
    if (m_pendingSubstituteResources.isEmpty())
        return;
    ASSERT(m_frame && m_frame->page());
    if (m_frame->page()->defersLoading())
        return;

    SubstituteResourceMap copy;
    copy.swap(m_pendingSubstituteResources);

    SubstituteResourceMap::const_iterator end = copy.end();
    for (SubstituteResourceMap::const_iterator it = copy.begin(); it != end; ++it) {
        RefPtr<ResourceLoader> loader = it->first;
        SubstituteResource* resource = it->second.get();
        
        if (resource) {
            SharedBuffer* data = resource->data();
        
            loader->didReceiveResponse(resource->response());
            loader->didReceiveData(data->data(), data->size(), data->size(), true);
            loader->didFinishLoading(0);
        } else {
            // A null resource means that we should fail the load.
            // FIXME: Maybe we should use another error here - something like "not in cache".
            loader->didFail(loader->cannotShowURLError());
        }
    }
}

#ifndef NDEBUG
bool DocumentLoader::isSubstituteLoadPending(ResourceLoader* loader) const
{
    return m_pendingSubstituteResources.contains(loader);
}
#endif

void DocumentLoader::cancelPendingSubstituteLoad(ResourceLoader* loader)
{
    if (m_pendingSubstituteResources.isEmpty())
        return;
    m_pendingSubstituteResources.remove(loader);
    if (m_pendingSubstituteResources.isEmpty())
        m_substituteResourceDeliveryTimer.stop();
}

#if ENABLE(WEB_ARCHIVE)
bool DocumentLoader::scheduleArchiveLoad(ResourceLoader* loader, const ResourceRequest& request, const KURL& originalURL)
{
    ArchiveResource* resource = 0;
    
    if (request.url() == originalURL)
        resource = archiveResourceForURL(originalURL);

    if (!resource) {
        // WebArchiveDebugMode means we fail loads instead of trying to fetch them from the network if they're not in the archive.
        bool shouldFailLoad = m_frame->settings()->webArchiveDebugModeEnabled() && ArchiveFactory::isArchiveMimeType(responseMIMEType());

        if (!shouldFailLoad)
            return false;
    }
    
    m_pendingSubstituteResources.set(loader, resource);
    deliverSubstituteResourcesAfterDelay();
    
    return true;
}
#endif // ENABLE(WEB_ARCHIVE)

void DocumentLoader::addResponse(const ResourceResponse& r)
{
    if (!m_stopRecordingResponses)
        m_responses.append(r);
}

void DocumentLoader::stopRecordingResponses()
{
    m_stopRecordingResponses = true;
}

void DocumentLoader::setTitle(const StringWithDirection& title)
{
    if (title.isEmpty())
        return;

    if (m_pageTitle != title) {
        frameLoader()->willChangeTitle(this);
        m_pageTitle = title;
        frameLoader()->didChangeTitle(this);
    }
}

IconURL DocumentLoader::iconURL(IconType iconType) const
{
    return m_iconURLs[toIconIndex(iconType)];
}

void DocumentLoader::setIconURL(const IconURL& url)
{
    if (url.m_iconURL.isEmpty())
        return;

    if (iconURL(url.m_iconType).m_iconURL != url.m_iconURL) {
        m_iconURLs[toIconIndex(url.m_iconType)] = url;
        frameLoader()->didChangeIcons(this, url.m_iconType);
    }
}

KURL DocumentLoader::urlForHistory() const
{
    // Return the URL to be used for history and B/F list.
    // Returns nil for WebDataProtocol URLs that aren't alternates 
    // for unreachable URLs, because these can't be stored in history.
    if (m_substituteData.isValid())
        return unreachableURL();

    return m_originalRequestCopy.url();
}

bool DocumentLoader::urlForHistoryReflectsFailure() const
{
    return m_substituteData.isValid() || m_response.httpStatusCode() >= 400;
}

const KURL& DocumentLoader::originalURL() const
{
    return m_originalRequestCopy.url();
}

const KURL& DocumentLoader::requestURL() const
{
    return request().url();
}

const KURL& DocumentLoader::responseURL() const
{
    return m_response.url();
}

const String& DocumentLoader::responseMIMEType() const
{
    return m_response.mimeType();
}

const KURL& DocumentLoader::unreachableURL() const
{
    return m_substituteData.failingURL();
}

void DocumentLoader::setDefersLoading(bool defers)
{
    if (m_mainResourceLoader)
        m_mainResourceLoader->setDefersLoading(defers);
    setAllDefersLoading(m_subresourceLoaders, defers);
    setAllDefersLoading(m_plugInStreamLoaders, defers);
    if (!defers)
        deliverSubstituteResourcesAfterDelay();
}

void DocumentLoader::stopLoadingPlugIns()
{
    cancelAll(m_plugInStreamLoaders);
}

void DocumentLoader::stopLoadingSubresources()
{
    cancelAll(m_subresourceLoaders);
}

void DocumentLoader::addSubresourceLoader(ResourceLoader* loader)
{
    m_subresourceLoaders.add(loader);
    setLoading(true);
}

void DocumentLoader::removeSubresourceLoader(ResourceLoader* loader)
{
    m_subresourceLoaders.remove(loader);
    updateLoading();
    if (Frame* frame = m_frame)
        frame->loader()->checkLoadComplete();
}

void DocumentLoader::addPlugInStreamLoader(ResourceLoader* loader)
{
    m_plugInStreamLoaders.add(loader);
    setLoading(true);
}

void DocumentLoader::removePlugInStreamLoader(ResourceLoader* loader)
{
    m_plugInStreamLoaders.remove(loader);
    updateLoading();
}

bool DocumentLoader::isLoadingMainResource() const
{
    return !!m_mainResourceLoader;
}

bool DocumentLoader::isLoadingSubresources() const
{
    return !m_subresourceLoaders.isEmpty();
}

bool DocumentLoader::isLoadingPlugIns() const
{
    return !m_plugInStreamLoaders.isEmpty();
}

bool DocumentLoader::isLoadingMultipartContent() const
{
    return m_mainResourceLoader && m_mainResourceLoader->isLoadingMultipartContent();
}

bool DocumentLoader::startLoadingMainResource(unsigned long identifier)
{
    ASSERT(!m_mainResourceLoader);
    m_mainResourceLoader = MainResourceLoader::create(m_frame);
    m_mainResourceLoader->setIdentifier(identifier);

    // FIXME: Is there any way the extra fields could have not been added by now?
    // If not, it would be great to remove this line of code.
    frameLoader()->addExtraFieldsToMainResourceRequest(m_request);

    if (!m_mainResourceLoader->load(m_request, m_substituteData)) {
        // FIXME: If this should really be caught, we should just ASSERT this doesn't happen;
        // should it be caught by other parts of WebKit or other parts of the app?
        LOG_ERROR("could not create WebResourceHandle for URL %s -- should be caught by policy handler level", m_request.url().string().ascii().data());
        m_mainResourceLoader = 0;
        return false;
    }

    return true;
}

void DocumentLoader::cancelMainResourceLoad(const ResourceError& error)
{
    m_mainResourceLoader->cancel(error);
}

void DocumentLoader::subresourceLoaderFinishedLoadingOnePart(ResourceLoader* loader)
{
    m_multipartSubresourceLoaders.add(loader);
    m_subresourceLoaders.remove(loader);
    updateLoading();
    if (Frame* frame = m_frame)
        frame->loader()->checkLoadComplete();    
}

void DocumentLoader::transferLoadingResourcesFromPage(Page* oldPage)
{
    ASSERT(oldPage != m_frame->page());

    FrameLoader* loader = frameLoader();
    ASSERT(loader);

    const ResourceRequest& request = originalRequest();
    if (isLoadingMainResource()) {
        loader->dispatchTransferLoadingResourceFromPage(
            m_mainResourceLoader->identifier(), this, request, oldPage);
    }

    if (isLoadingSubresources()) {
        ResourceLoaderSet::const_iterator it = m_subresourceLoaders.begin();
        ResourceLoaderSet::const_iterator end = m_subresourceLoaders.end();
        for (; it != end; ++it) {
            loader->dispatchTransferLoadingResourceFromPage(
                (*it)->identifier(), this, request, oldPage);
        }
    }
}

void DocumentLoader::iconLoadDecisionAvailable()
{
    if (m_frame)
        m_frame->loader()->iconLoadDecisionReceived(iconDatabase().synchronousLoadDecisionForIconURL(KURL(frameLoader()->iconURL()), this));
}

static void iconLoadDecisionCallback(IconLoadDecision decision, void* context)
{
    static_cast<DocumentLoader*>(context)->continueIconLoadWithDecision(decision);
}

void DocumentLoader::getIconLoadDecisionForIconURL(const String& urlString)
{
    if (m_iconLoadDecisionCallback)
        m_iconLoadDecisionCallback->invalidate();
    m_iconLoadDecisionCallback = IconLoadDecisionCallback::create(this, iconLoadDecisionCallback);
    iconDatabase().loadDecisionForIconURL(urlString, m_iconLoadDecisionCallback);
}

void DocumentLoader::continueIconLoadWithDecision(IconLoadDecision decision)
{
    ASSERT(m_iconLoadDecisionCallback);
    m_iconLoadDecisionCallback = 0;
    if (m_frame)
        m_frame->loader()->continueIconLoadWithDecision(decision);
}

static void iconDataCallback(SharedBuffer*, void*)
{
    // FIXME: Implement this once we know what parts of WebCore actually need the icon data returned.
}

void DocumentLoader::getIconDataForIconURL(const String& urlString)
{   
    if (m_iconDataCallback)
        m_iconDataCallback->invalidate();
    m_iconDataCallback = IconDataCallback::create(this, iconDataCallback);
    iconDatabase().iconDataForIconURL(urlString, m_iconDataCallback);
}

} // namespace WebCore
