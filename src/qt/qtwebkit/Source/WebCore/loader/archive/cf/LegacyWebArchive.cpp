/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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
#include "LegacyWebArchive.h"

#include "CachedResource.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameSelection.h"
#include "FrameTree.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLNames.h"
#include "IconDatabase.h"
#include "Image.h"
#include "KURLHash.h"
#include "Logging.h"
#include "MemoryCache.h"
#include "Page.h"
#include "Range.h"
#include "ResourceBuffer.h"
#include "Settings.h"
#include "markup.h"
#include <wtf/ListHashSet.h>
#include <wtf/RetainPtr.h>

namespace WebCore {

static const CFStringRef LegacyWebArchiveMainResourceKey = CFSTR("WebMainResource");
static const CFStringRef LegacyWebArchiveSubresourcesKey = CFSTR("WebSubresources");
static const CFStringRef LegacyWebArchiveSubframeArchivesKey = CFSTR("WebSubframeArchives");
static const CFStringRef LegacyWebArchiveResourceDataKey = CFSTR("WebResourceData");
static const CFStringRef LegacyWebArchiveResourceFrameNameKey = CFSTR("WebResourceFrameName");
static const CFStringRef LegacyWebArchiveResourceMIMETypeKey = CFSTR("WebResourceMIMEType");
static const CFStringRef LegacyWebArchiveResourceURLKey = CFSTR("WebResourceURL");
static const CFStringRef LegacyWebArchiveResourceTextEncodingNameKey = CFSTR("WebResourceTextEncodingName");
static const CFStringRef LegacyWebArchiveResourceResponseKey = CFSTR("WebResourceResponse");
static const CFStringRef LegacyWebArchiveResourceResponseVersionKey = CFSTR("WebResourceResponseVersion");

RetainPtr<CFDictionaryRef> LegacyWebArchive::createPropertyListRepresentation(ArchiveResource* resource, MainResourceStatus isMainResource)
{
    if (!resource) {
        // The property list representation of a null/empty WebResource has the following 3 objects stored as nil.
        // FIXME: 0 is not serializable. Presumably we need to use kCFNull here instead for compatibility.
        // FIXME: But why do we need to support a resource of 0? Who relies on that?
        RetainPtr<CFMutableDictionaryRef> propertyList = adoptCF(CFDictionaryCreateMutable(0, 3, 0, 0));
        CFDictionarySetValue(propertyList.get(), LegacyWebArchiveResourceDataKey, 0);
        CFDictionarySetValue(propertyList.get(), LegacyWebArchiveResourceURLKey, 0);
        CFDictionarySetValue(propertyList.get(), LegacyWebArchiveResourceMIMETypeKey, 0);
        return propertyList;
    }
    
    RetainPtr<CFMutableDictionaryRef> propertyList = adoptCF(CFDictionaryCreateMutable(0, 6, 0, &kCFTypeDictionaryValueCallBacks));
    
    // Resource data can be empty, but must be represented by an empty CFDataRef
    SharedBuffer* data = resource->data();
    RetainPtr<CFDataRef> cfData;
    if (data)
        cfData = adoptCF(data->createCFData());
    else
        cfData = adoptCF(CFDataCreate(0, 0, 0));
    CFDictionarySetValue(propertyList.get(), LegacyWebArchiveResourceDataKey, cfData.get());
    
    // Resource URL cannot be null
    if (RetainPtr<CFStringRef> cfURL = resource->url().string().createCFString())
        CFDictionarySetValue(propertyList.get(), LegacyWebArchiveResourceURLKey, cfURL.get());
    else {
        LOG(Archives, "LegacyWebArchive - NULL resource URL is invalid - returning null property list");
        return 0;
    }

    // FrameName should be left out if empty for subresources, but always included for main resources
    const String& frameName(resource->frameName());
    if (!frameName.isEmpty() || isMainResource)
        CFDictionarySetValue(propertyList.get(), LegacyWebArchiveResourceFrameNameKey, frameName.createCFString().get());
    
    // Set MIMEType, TextEncodingName, and ResourceResponse only if they actually exist
    const String& mimeType(resource->mimeType());
    if (!mimeType.isEmpty())
        CFDictionarySetValue(propertyList.get(), LegacyWebArchiveResourceMIMETypeKey, mimeType.createCFString().get());
    
    const String& textEncoding(resource->textEncoding());
    if (!textEncoding.isEmpty())
        CFDictionarySetValue(propertyList.get(), LegacyWebArchiveResourceTextEncodingNameKey, textEncoding.createCFString().get());

    // Don't include the resource response for the main resource
    if (!isMainResource) {
        RetainPtr<CFDataRef> resourceResponseData = createPropertyListRepresentation(resource->response());
        if (resourceResponseData)
            CFDictionarySetValue(propertyList.get(), LegacyWebArchiveResourceResponseKey, resourceResponseData.get());    
    }
    
    return propertyList;
}

RetainPtr<CFDictionaryRef> LegacyWebArchive::createPropertyListRepresentation(Archive* archive)
{
    RetainPtr<CFMutableDictionaryRef> propertyList = adoptCF(CFDictionaryCreateMutable(0, 3, 0, &kCFTypeDictionaryValueCallBacks));
    
    RetainPtr<CFDictionaryRef> mainResourceDict = createPropertyListRepresentation(archive->mainResource(), MainResource);
    ASSERT(mainResourceDict);
    if (!mainResourceDict)
        return 0;
    CFDictionarySetValue(propertyList.get(), LegacyWebArchiveMainResourceKey, mainResourceDict.get());

    RetainPtr<CFMutableArrayRef> subresourcesArray = adoptCF(CFArrayCreateMutable(0, archive->subresources().size(), &kCFTypeArrayCallBacks));
    const Vector<RefPtr<ArchiveResource> >& subresources(archive->subresources());
    for (unsigned i = 0; i < subresources.size(); ++i) {
        RetainPtr<CFDictionaryRef> subresource = createPropertyListRepresentation(subresources[i].get(), Subresource);
        if (subresource)
            CFArrayAppendValue(subresourcesArray.get(), subresource.get());
        else
            LOG(Archives, "LegacyWebArchive - Failed to create property list for subresource");
    }
    if (CFArrayGetCount(subresourcesArray.get()))
        CFDictionarySetValue(propertyList.get(), LegacyWebArchiveSubresourcesKey, subresourcesArray.get());

    RetainPtr<CFMutableArrayRef> subframesArray = adoptCF(CFArrayCreateMutable(0, archive->subframeArchives().size(), &kCFTypeArrayCallBacks));
    const Vector<RefPtr<Archive> >& subframeArchives(archive->subframeArchives());
    for (unsigned i = 0; i < subframeArchives.size(); ++i) {
        RetainPtr<CFDictionaryRef> subframeArchive = createPropertyListRepresentation(subframeArchives[i].get());
        if (subframeArchive)
            CFArrayAppendValue(subframesArray.get(), subframeArchive.get());
        else
            LOG(Archives, "LegacyWebArchive - Failed to create property list for subframe archive");
    }
    if (CFArrayGetCount(subframesArray.get()))
        CFDictionarySetValue(propertyList.get(), LegacyWebArchiveSubframeArchivesKey, subframesArray.get());

    return propertyList;
}

ResourceResponse LegacyWebArchive::createResourceResponseFromPropertyListData(CFDataRef data, CFStringRef responseDataType)
{
    ASSERT(data);
    if (!data)
        return ResourceResponse();
    
    // If the ResourceResponseVersion (passed in as responseDataType) exists at all, this is a "new" web archive that we
    // can parse well in a cross platform manner If it doesn't exist, we will assume this is an "old" web archive with,
    // NSURLResponse objects in it and parse the ResourceResponse as such.
    if (!responseDataType)
        return createResourceResponseFromMacArchivedData(data);
        
    // FIXME: Parse the "new" format that the above comment references here. This format doesn't exist yet.
    return ResourceResponse();
}

PassRefPtr<ArchiveResource> LegacyWebArchive::createResource(CFDictionaryRef dictionary)
{
    ASSERT(dictionary);
    if (!dictionary)
        return 0;
        
    CFDataRef resourceData = static_cast<CFDataRef>(CFDictionaryGetValue(dictionary, LegacyWebArchiveResourceDataKey));
    if (resourceData && CFGetTypeID(resourceData) != CFDataGetTypeID()) {
        LOG(Archives, "LegacyWebArchive - Resource data is not of type CFData, cannot create invalid resource");
        return 0;
    }
    
    CFStringRef frameName = static_cast<CFStringRef>(CFDictionaryGetValue(dictionary, LegacyWebArchiveResourceFrameNameKey));
    if (frameName && CFGetTypeID(frameName) != CFStringGetTypeID()) {
        LOG(Archives, "LegacyWebArchive - Frame name is not of type CFString, cannot create invalid resource");
        return 0;
    }
    
    CFStringRef mimeType = static_cast<CFStringRef>(CFDictionaryGetValue(dictionary, LegacyWebArchiveResourceMIMETypeKey));
    if (mimeType && CFGetTypeID(mimeType) != CFStringGetTypeID()) {
        LOG(Archives, "LegacyWebArchive - MIME type is not of type CFString, cannot create invalid resource");
        return 0;
    }
    
    CFStringRef url = static_cast<CFStringRef>(CFDictionaryGetValue(dictionary, LegacyWebArchiveResourceURLKey));
    if (url && CFGetTypeID(url) != CFStringGetTypeID()) {
        LOG(Archives, "LegacyWebArchive - URL is not of type CFString, cannot create invalid resource");
        return 0;
    }
    
    CFStringRef textEncoding = static_cast<CFStringRef>(CFDictionaryGetValue(dictionary, LegacyWebArchiveResourceTextEncodingNameKey));
    if (textEncoding && CFGetTypeID(textEncoding) != CFStringGetTypeID()) {
        LOG(Archives, "LegacyWebArchive - Text encoding is not of type CFString, cannot create invalid resource");
        return 0;
    }

    ResourceResponse response;
    
    CFDataRef resourceResponseData = static_cast<CFDataRef>(CFDictionaryGetValue(dictionary, LegacyWebArchiveResourceResponseKey));
    if (resourceResponseData) {
        if (CFGetTypeID(resourceResponseData) != CFDataGetTypeID()) {
            LOG(Archives, "LegacyWebArchive - Resource response data is not of type CFData, cannot create invalid resource");
            return 0;
        }
        
        CFStringRef resourceResponseVersion = static_cast<CFStringRef>(CFDictionaryGetValue(dictionary, LegacyWebArchiveResourceResponseVersionKey));
        if (resourceResponseVersion && CFGetTypeID(resourceResponseVersion) != CFStringGetTypeID()) {
            LOG(Archives, "LegacyWebArchive - Resource response version is not of type CFString, cannot create invalid resource");
            return 0;
        }
        
        response = createResourceResponseFromPropertyListData(resourceResponseData, resourceResponseVersion);
    }
    
    return ArchiveResource::create(SharedBuffer::wrapCFData(resourceData), KURL(KURL(), url), mimeType, textEncoding, frameName, response);
}

PassRefPtr<LegacyWebArchive> LegacyWebArchive::create()
{
    return adoptRef(new LegacyWebArchive);
}

PassRefPtr<LegacyWebArchive> LegacyWebArchive::create(PassRefPtr<ArchiveResource> mainResource, Vector<PassRefPtr<ArchiveResource> >& subresources, Vector<PassRefPtr<LegacyWebArchive> >& subframeArchives)
{
    ASSERT(mainResource);
    if (!mainResource)
        return 0;
    
    RefPtr<LegacyWebArchive> archive = create();
    archive->setMainResource(mainResource);
    
    for (unsigned i = 0; i < subresources.size(); ++i)
        archive->addSubresource(subresources[i]);
    
    for (unsigned i = 0; i < subframeArchives.size(); ++i)
        archive->addSubframeArchive(subframeArchives[i]);  
        
    return archive.release();
}

PassRefPtr<LegacyWebArchive> LegacyWebArchive::create(SharedBuffer* data)
{
    return create(KURL(), data);
}

PassRefPtr<LegacyWebArchive> LegacyWebArchive::create(const KURL&, SharedBuffer* data)
{
    LOG(Archives, "LegacyWebArchive - Creating from raw data");
    
    RefPtr<LegacyWebArchive> archive = create();
        
    ASSERT(data);
    if (!data)
        return 0;
        
    RetainPtr<CFDataRef> cfData = adoptCF(data->createCFData());
    if (!cfData)
        return 0;
        
    CFStringRef errorString = 0;
    
    RetainPtr<CFDictionaryRef> plist = adoptCF(static_cast<CFDictionaryRef>(CFPropertyListCreateFromXMLData(0, cfData.get(), kCFPropertyListImmutable, &errorString)));
    if (!plist) {
#ifndef NDEBUG
        const char* cError = errorString ? CFStringGetCStringPtr(errorString, kCFStringEncodingUTF8) : "unknown error";
        LOG(Archives, "LegacyWebArchive - Error parsing PropertyList from archive data - %s", cError);
#endif
        if (errorString)
            CFRelease(errorString);
        return 0;
    }
    
    if (CFGetTypeID(plist.get()) != CFDictionaryGetTypeID()) {
        LOG(Archives, "LegacyWebArchive - Archive property list is not the expected CFDictionary, aborting invalid WebArchive");
        return 0;
    }
    
    if (!archive->extract(plist.get()))
        return 0;

    return archive.release();
}

bool LegacyWebArchive::extract(CFDictionaryRef dictionary)
{
    ASSERT(dictionary);
    if (!dictionary) {
        LOG(Archives, "LegacyWebArchive - Null root CFDictionary, aborting invalid WebArchive");
        return false;
    }
    
    CFDictionaryRef mainResourceDict = static_cast<CFDictionaryRef>(CFDictionaryGetValue(dictionary, LegacyWebArchiveMainResourceKey));
    if (!mainResourceDict) {
        LOG(Archives, "LegacyWebArchive - No main resource in archive, aborting invalid WebArchive");
        return false;
    }
    if (CFGetTypeID(mainResourceDict) != CFDictionaryGetTypeID()) {
        LOG(Archives, "LegacyWebArchive - Main resource is not the expected CFDictionary, aborting invalid WebArchive");
        return false;
    }
    
    setMainResource(createResource(mainResourceDict));
    if (!mainResource()) {
        LOG(Archives, "LegacyWebArchive - Failed to parse main resource from CFDictionary or main resource does not exist, aborting invalid WebArchive");
        return false;
    }
    
    if (mainResource()->mimeType().isNull()) {
        LOG(Archives, "LegacyWebArchive - Main resource MIME type is required, but was null.");
        return false;
    }
    
    CFArrayRef subresourceArray = static_cast<CFArrayRef>(CFDictionaryGetValue(dictionary, LegacyWebArchiveSubresourcesKey));
    if (subresourceArray && CFGetTypeID(subresourceArray) != CFArrayGetTypeID()) {
        LOG(Archives, "LegacyWebArchive - Subresources is not the expected Array, aborting invalid WebArchive");
        return false;
    }
    
    if (subresourceArray) {
        CFIndex count = CFArrayGetCount(subresourceArray);
        for (CFIndex i = 0; i < count; ++i) {
            CFDictionaryRef subresourceDict = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(subresourceArray, i));
            if (CFGetTypeID(subresourceDict) != CFDictionaryGetTypeID()) {
                LOG(Archives, "LegacyWebArchive - Subresource is not expected CFDictionary, aborting invalid WebArchive");
                return false;
            }
            
            if (RefPtr<ArchiveResource> subresource = createResource(subresourceDict))
                addSubresource(subresource.release());
        }
    }
    
    CFArrayRef subframeArray = static_cast<CFArrayRef>(CFDictionaryGetValue(dictionary, LegacyWebArchiveSubframeArchivesKey));
    if (subframeArray && CFGetTypeID(subframeArray) != CFArrayGetTypeID()) {
        LOG(Archives, "LegacyWebArchive - Subframe archives is not the expected Array, aborting invalid WebArchive");
        return false;
    }
    
    if (subframeArray) {
        CFIndex count = CFArrayGetCount(subframeArray);
        for (CFIndex i = 0; i < count; ++i) {
            CFDictionaryRef subframeDict = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(subframeArray, i));
            if (CFGetTypeID(subframeDict) != CFDictionaryGetTypeID()) {
                LOG(Archives, "LegacyWebArchive - Subframe array is not expected CFDictionary, aborting invalid WebArchive");
                return false;
            }
            
            RefPtr<LegacyWebArchive> subframeArchive = create();
            if (subframeArchive->extract(subframeDict))
                addSubframeArchive(subframeArchive.release());
            else
                LOG(Archives, "LegacyWebArchive - Invalid subframe archive skipped");
        }
    }
    
    return true;
}

Archive::Type LegacyWebArchive::type() const
{
    return Archive::WebArchive;
}
    
RetainPtr<CFDataRef> LegacyWebArchive::rawDataRepresentation()
{
    RetainPtr<CFDictionaryRef> propertyList = createPropertyListRepresentation(this);
    ASSERT(propertyList);
    if (!propertyList) {
        LOG(Archives, "LegacyWebArchive - Failed to create property list for archive, returning no data");
        return 0;
    }

    RetainPtr<CFWriteStreamRef> stream = adoptCF(CFWriteStreamCreateWithAllocatedBuffers(0, 0));

    CFWriteStreamOpen(stream.get());
    CFPropertyListWriteToStream(propertyList.get(), stream.get(), kCFPropertyListBinaryFormat_v1_0, 0);

    RetainPtr<CFDataRef> plistData = adoptCF(static_cast<CFDataRef>(CFWriteStreamCopyProperty(stream.get(), kCFStreamPropertyDataWritten)));
    ASSERT(plistData);

    CFWriteStreamClose(stream.get());

    if (!plistData) {
        LOG(Archives, "LegacyWebArchive - Failed to convert property list into raw data, returning no data");
        return 0;
    }

    return plistData;
}

#if !PLATFORM(MAC)

ResourceResponse LegacyWebArchive::createResourceResponseFromMacArchivedData(CFDataRef responseData)
{
    // FIXME: If is is possible to parse in a serialized NSURLResponse manually, without using
    // NSKeyedUnarchiver, manipulating plists directly, then we want to do that here.
    // Until then, this can be done on Mac only.
    return ResourceResponse();
}

RetainPtr<CFDataRef> LegacyWebArchive::createPropertyListRepresentation(const ResourceResponse& response)
{
    // FIXME: Write out the "new" format described in createResourceResponseFromPropertyListData once we invent it.
    return 0;
}

#endif

PassRefPtr<LegacyWebArchive> LegacyWebArchive::create(Node* node, FrameFilter* filter)
{
    ASSERT(node);
    if (!node)
        return create();
        
    Document* document = node->document();
    Frame* frame = document ? document->frame() : 0;
    if (!frame)
        return create();

    // If the page was loaded with javascript enabled, we don't want to archive <noscript> tags
    // In practice we don't actually know whether scripting was enabled when the page was originally loaded
    // but we can approximate that by checking if scripting is enabled right now.
    OwnPtr<Vector<QualifiedName> > tagNamesToFilter;
    if (frame->page() && frame->page()->settings()->isScriptEnabled()) {
        tagNamesToFilter = adoptPtr(new Vector<QualifiedName>);
        tagNamesToFilter->append(HTMLNames::noscriptTag);
    }
        
    Vector<Node*> nodeList;
    String markupString = createMarkup(node, IncludeNode, &nodeList, DoNotResolveURLs, tagNamesToFilter.get());
    Node::NodeType nodeType = node->nodeType();
    if (nodeType != Node::DOCUMENT_NODE && nodeType != Node::DOCUMENT_TYPE_NODE)
        markupString = frame->documentTypeString() + markupString;

    return create(markupString, frame, nodeList, filter);
}

PassRefPtr<LegacyWebArchive> LegacyWebArchive::create(Frame* frame)
{
    ASSERT(frame);
    
    DocumentLoader* documentLoader = frame->loader()->documentLoader();

    if (!documentLoader)
        return 0;
        
    Vector<PassRefPtr<LegacyWebArchive> > subframeArchives;
    
    unsigned children = frame->tree()->childCount();
    for (unsigned i = 0; i < children; ++i) {
        RefPtr<LegacyWebArchive> childFrameArchive = create(frame->tree()->child(i));
        if (childFrameArchive)
            subframeArchives.append(childFrameArchive.release());
    }

    Vector<PassRefPtr<ArchiveResource> > subresources;
    documentLoader->getSubresources(subresources);

    return create(documentLoader->mainResource(), subresources, subframeArchives);
}

PassRefPtr<LegacyWebArchive> LegacyWebArchive::create(Range* range)
{
    if (!range)
        return 0;
    
    Node* startContainer = range->startContainer();
    if (!startContainer)
        return 0;
        
    Document* document = startContainer->document();
    if (!document)
        return 0;
        
    Frame* frame = document->frame();
    if (!frame)
        return 0;
    
    Vector<Node*> nodeList;
    
    // FIXME: This is always "for interchange". Is that right? See the previous method.
    String markupString = frame->documentTypeString() + createMarkup(range, &nodeList, AnnotateForInterchange);

    return create(markupString, frame, nodeList, 0);
}

PassRefPtr<LegacyWebArchive> LegacyWebArchive::create(const String& markupString, Frame* frame, const Vector<Node*>& nodes, FrameFilter* frameFilter)
{
    ASSERT(frame);
    
    const ResourceResponse& response = frame->loader()->documentLoader()->response();
    KURL responseURL = response.url();
    
    // it's possible to have a response without a URL here
    // <rdar://problem/5454935>
    if (responseURL.isNull())
        responseURL = KURL(ParsedURLString, emptyString());
        
    PassRefPtr<ArchiveResource> mainResource = ArchiveResource::create(utf8Buffer(markupString), responseURL, response.mimeType(), "UTF-8", frame->tree()->uniqueName());

    Vector<PassRefPtr<LegacyWebArchive> > subframeArchives;
    Vector<PassRefPtr<ArchiveResource> > subresources;
    HashSet<KURL> uniqueSubresources;

    size_t nodesSize = nodes.size();    
    for (size_t i = 0; i < nodesSize; ++i) {
        Node* node = nodes[i];
        Frame* childFrame;
        if ((node->hasTagName(HTMLNames::frameTag) || node->hasTagName(HTMLNames::iframeTag) || node->hasTagName(HTMLNames::objectTag)) &&
            (childFrame = toFrameOwnerElement(node)->contentFrame())) {
            if (frameFilter && !frameFilter->shouldIncludeSubframe(childFrame))
                continue;
                
            RefPtr<LegacyWebArchive> subframeArchive = create(childFrame->document(), frameFilter);
            
            if (subframeArchive)
                subframeArchives.append(subframeArchive);
            else
                LOG_ERROR("Unabled to archive subframe %s", childFrame->tree()->uniqueName().string().utf8().data());
        } else {
            ListHashSet<KURL> subresourceURLs;
            node->getSubresourceURLs(subresourceURLs);
            
            DocumentLoader* documentLoader = frame->loader()->documentLoader();
            ListHashSet<KURL>::iterator iterEnd = subresourceURLs.end();
            for (ListHashSet<KURL>::iterator iter = subresourceURLs.begin(); iter != iterEnd; ++iter) {
                const KURL& subresourceURL = *iter;
                if (uniqueSubresources.contains(subresourceURL))
                    continue;

                uniqueSubresources.add(subresourceURL);

                RefPtr<ArchiveResource> resource = documentLoader->subresource(subresourceURL);
                if (resource) {
                    subresources.append(resource.release());
                    continue;
                }

                ResourceRequest request(subresourceURL);
#if ENABLE(CACHE_PARTITIONING)
                request.setCachePartition(frame->document()->topOrigin()->cachePartition());
#endif
                CachedResource* cachedResource = memoryCache()->resourceForRequest(request);
                if (cachedResource) {
                    ResourceBuffer* data = cachedResource->resourceBuffer();
                    resource = ArchiveResource::create(data ? data->sharedBuffer() : 0, subresourceURL, cachedResource->response());
                    if (resource) {
                        subresources.append(resource.release());
                        continue;
                    }
                }

                // FIXME: should do something better than spew to console here
                LOG_ERROR("Failed to archive subresource for %s", subresourceURL.string().utf8().data());
            }
        }
    }

    // Add favicon if one exists for this page, if we are archiving the entire page.
    if (nodesSize && nodes[0]->isDocumentNode() && iconDatabase().isEnabled()) {
        const String& iconURL = iconDatabase().synchronousIconURLForPageURL(responseURL);
        if (!iconURL.isEmpty() && iconDatabase().synchronousIconDataKnownForIconURL(iconURL)) {
            if (Image* iconImage = iconDatabase().synchronousIconForPageURL(responseURL, IntSize(16, 16))) {
                if (RefPtr<ArchiveResource> resource = ArchiveResource::create(iconImage->data(), KURL(ParsedURLString, iconURL), "image/x-icon", "", ""))
                    subresources.append(resource.release());
            }
        }
    }

    return create(mainResource, subresources, subframeArchives);
}

PassRefPtr<LegacyWebArchive> LegacyWebArchive::createFromSelection(Frame* frame)
{
    if (!frame)
        return 0;
    
    RefPtr<Range> selectionRange = frame->selection()->toNormalizedRange();
    Vector<Node*> nodeList;
    String markupString = frame->documentTypeString() + createMarkup(selectionRange.get(), &nodeList, AnnotateForInterchange);
    
    RefPtr<LegacyWebArchive> archive = create(markupString, frame, nodeList, 0);
    
    if (!frame->document() || !frame->document()->isFrameSet())
        return archive.release();
        
    // Wrap the frameset document in an iframe so it can be pasted into
    // another document (which will have a body or frameset of its own). 
    String iframeMarkup = "<iframe frameborder=\"no\" marginwidth=\"0\" marginheight=\"0\" width=\"98%%\" height=\"98%%\" src=\"" +
                          frame->loader()->documentLoader()->response().url().string() + "\"></iframe>";
    RefPtr<ArchiveResource> iframeResource = ArchiveResource::create(utf8Buffer(iframeMarkup), blankURL(), "text/html", "UTF-8", String());

    Vector<PassRefPtr<ArchiveResource> > subresources;

    Vector<PassRefPtr<LegacyWebArchive> > subframeArchives;
    subframeArchives.append(archive);
    
    archive = create(iframeResource.release(), subresources, subframeArchives);
    
    return archive.release();
}

}
