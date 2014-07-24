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

#ifndef ApplicationCacheGroup_h
#define ApplicationCacheGroup_h

#include "DOMApplicationCache.h"
#include "KURL.h"
#include "ResourceHandleClient.h"
#include "SharedBuffer.h"
#include <wtf/Noncopyable.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ApplicationCache;
class ApplicationCacheResource;
class Document;
class DocumentLoader;
class Frame;
class ResourceHandle;
class SecurityOrigin;

enum ApplicationCacheUpdateOption {
    ApplicationCacheUpdateWithBrowsingContext,
    ApplicationCacheUpdateWithoutBrowsingContext
};

class ApplicationCacheGroup : ResourceHandleClient {
    WTF_MAKE_NONCOPYABLE(ApplicationCacheGroup); WTF_MAKE_FAST_ALLOCATED;
public:
    ApplicationCacheGroup(const KURL& manifestURL, bool isCopy = false);    
    ~ApplicationCacheGroup();
    
    enum UpdateStatus { Idle, Checking, Downloading };

    static ApplicationCache* cacheForMainRequest(const ResourceRequest&, DocumentLoader*);
    static ApplicationCache* fallbackCacheForMainRequest(const ResourceRequest&, DocumentLoader*);
    
    static void selectCache(Frame*, const KURL& manifestURL);
    static void selectCacheWithoutManifestURL(Frame*);
    
    const KURL& manifestURL() const { return m_manifestURL; }
    const SecurityOrigin* origin() const { return m_origin.get(); }
    UpdateStatus updateStatus() const { return m_updateStatus; }
    void setUpdateStatus(UpdateStatus status);

    void setStorageID(unsigned storageID) { m_storageID = storageID; }
    unsigned storageID() const { return m_storageID; }
    void clearStorageID();
    
    void update(Frame*, ApplicationCacheUpdateOption); // FIXME: Frame should not be needed when updating without browsing context.
    void cacheDestroyed(ApplicationCache*);
    
    void abort(Frame*);

    bool cacheIsBeingUpdated(const ApplicationCache* cache) const { return cache == m_cacheBeingUpdated; }

    void stopLoadingInFrame(Frame*);

    ApplicationCache* newestCache() const { return m_newestCache.get(); }
    void setNewestCache(PassRefPtr<ApplicationCache>);

    void makeObsolete();
    bool isObsolete() const { return m_isObsolete; }

    void finishedLoadingMainResource(DocumentLoader*);
    void failedLoadingMainResource(DocumentLoader*);

    void disassociateDocumentLoader(DocumentLoader*);

    bool isCopy() const { return m_isCopy; }

private:
    static void postListenerTask(ApplicationCacheHost::EventID id, const HashSet<DocumentLoader*>& set) { postListenerTask(id, 0, 0, set); }
    static void postListenerTask(ApplicationCacheHost::EventID id, DocumentLoader* loader)  { postListenerTask(id, 0, 0, loader); }
    static void postListenerTask(ApplicationCacheHost::EventID, int progressTotal, int progressDone, const HashSet<DocumentLoader*>&);
    static void postListenerTask(ApplicationCacheHost::EventID, int progressTotal, int progressDone, DocumentLoader*);

    void scheduleReachedMaxAppCacheSizeCallback();

    PassRefPtr<ResourceHandle> createResourceHandle(const KURL&, ApplicationCacheResource* newestCachedResource);

    // For normal resource loading, WebKit client is asked about each resource individually. Since application cache does not belong to any particular document,
    // the existing client callback cannot be used, so assume that any client that enables application cache also wants it to use credential storage.
    virtual bool shouldUseCredentialStorage(ResourceHandle*) { return true; }

    virtual void didReceiveResponse(ResourceHandle*, const ResourceResponse&);
    virtual void didReceiveData(ResourceHandle*, const char*, int length, int encodedDataLength);
    virtual void didFinishLoading(ResourceHandle*, double finishTime);
    virtual void didFail(ResourceHandle*, const ResourceError&);

    void didReceiveManifestResponse(const ResourceResponse&);
    void didReceiveManifestData(const char*, int);
    void didFinishLoadingManifest();
    void didReachMaxAppCacheSize();
    void didReachOriginQuota(int64_t totalSpaceNeeded);
    
    void startLoadingEntry();
    void deliverDelayedMainResources();
    void checkIfLoadIsComplete();
    void cacheUpdateFailed();
    void recalculateAvailableSpaceInQuota();
    void manifestNotFound();
    
    void addEntry(const String&, unsigned type);
    
    void associateDocumentLoaderWithCache(DocumentLoader*, ApplicationCache*);
    
    void stopLoading();
    
    KURL m_manifestURL;
    RefPtr<SecurityOrigin> m_origin;
    UpdateStatus m_updateStatus;
    
    // This is the newest complete cache in the group.
    RefPtr<ApplicationCache> m_newestCache;
    
    // All complete caches in this cache group.
    HashSet<ApplicationCache*> m_caches;
    
    // The cache being updated (if any). Note that cache updating does not immediately create a new
    // ApplicationCache object, so this may be null even when update status is not Idle.
    RefPtr<ApplicationCache> m_cacheBeingUpdated;

    // List of pending master entries, used during the update process to ensure that new master entries are cached.
    HashSet<DocumentLoader*> m_pendingMasterResourceLoaders;
    // How many of the above pending master entries have not yet finished downloading.
    int m_downloadingPendingMasterResourceLoadersCount;
    
    // These are all the document loaders that are associated with a cache in this group.
    HashSet<DocumentLoader*> m_associatedDocumentLoaders;

    // The URLs and types of pending cache entries.
    typedef HashMap<String, unsigned> EntryMap;
    EntryMap m_pendingEntries;
    
    // The total number of items to be processed to update the cache group and the number that have been done.
    int m_progressTotal;
    int m_progressDone;

    // Frame used for fetching resources when updating.
    // FIXME: An update started by a particular frame should not stop if it is destroyed, but there are other frames associated with the same cache group.
    Frame* m_frame;
  
    // An obsolete cache group is never stored, but the opposite is not true - storing may fail for multiple reasons, such as exceeding disk quota.
    unsigned m_storageID;
    bool m_isObsolete;

    // During update, this is used to handle asynchronously arriving results.
    enum CompletionType {
        None,
        NoUpdate,
        Failure,
        Completed
    };
    CompletionType m_completionType;

    // Whether this cache group is a copy that's only used for transferring the cache to another file.
    bool m_isCopy;

    // This flag is set immediately after the ChromeClient::reachedMaxAppCacheSize() callback is invoked as a result of the storage layer failing to save a cache
    // due to reaching the maximum size of the application cache database file. This flag is used by ApplicationCacheGroup::checkIfLoadIsComplete() to decide
    // the course of action in case of this failure (i.e. call the ChromeClient callback or run the failure steps).
    bool m_calledReachedMaxAppCacheSize;
    
    RefPtr<ResourceHandle> m_currentHandle;
    RefPtr<ApplicationCacheResource> m_currentResource;

#if ENABLE(INSPECTOR)
    unsigned long m_currentResourceIdentifier;
#endif

    RefPtr<ApplicationCacheResource> m_manifestResource;
    RefPtr<ResourceHandle> m_manifestHandle;

    int64_t m_availableSpaceInQuota;
    bool m_originQuotaExceededPreviously;

    friend class ChromeClientCallbackTimer;
};

} // namespace WebCore

#endif // ApplicationCacheGroup_h
