/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef CachedRawResource_h
#define CachedRawResource_h

#include "CachedResource.h"

namespace WebCore {

class CachedResourceClient;
class SubresourceLoader;

class CachedRawResource FINAL : public CachedResource {
public:
    CachedRawResource(ResourceRequest&, Type);

    // FIXME: AssociatedURLLoader shouldn't be a DocumentThreadableLoader and therefore shouldn't
    // use CachedRawResource. However, it is, and it needs to be able to defer loading.
    // This can be fixed by splitting CORS preflighting out of DocumentThreacableLoader.
    virtual void setDefersLoading(bool);

    virtual void setDataBufferingPolicy(DataBufferingPolicy);
    
    // FIXME: This is exposed for the InpsectorInstrumentation for preflights in DocumentThreadableLoader. It's also really lame.
    unsigned long identifier() const { return m_identifier; }

    void clear();

private:
    virtual void didAddClient(CachedResourceClient*) OVERRIDE;
    virtual void addDataBuffer(ResourceBuffer*) OVERRIDE;
    virtual void addData(const char* data, unsigned length) OVERRIDE;
    virtual void finishLoading(ResourceBuffer*) OVERRIDE;

    virtual bool shouldIgnoreHTTPStatusCodeErrors() const OVERRIDE { return true; }
    virtual void allClientsRemoved() OVERRIDE;

    virtual void willSendRequest(ResourceRequest&, const ResourceResponse&) OVERRIDE;
    virtual void responseReceived(const ResourceResponse&) OVERRIDE;
    virtual void didSendData(unsigned long long bytesSent, unsigned long long totalBytesToBeSent) OVERRIDE;

    virtual void switchClientsToRevalidatedResource() OVERRIDE;
    virtual bool mayTryReplaceEncodedData() const OVERRIDE { return true; }

    virtual bool canReuse(const ResourceRequest&) const OVERRIDE;

    const char* calculateIncrementalDataChunk(ResourceBuffer*, unsigned& incrementalDataLength);
    void notifyClientsDataWasReceived(const char* data, unsigned length);

#if USE(SOUP)
    virtual char* getOrCreateReadBuffer(size_t requestedSize, size_t& actualSize);
#endif

    unsigned long m_identifier;

    struct RedirectPair {
    public:
        explicit RedirectPair(const ResourceRequest& request, const ResourceResponse& redirectResponse)
            : m_request(request)
            , m_redirectResponse(redirectResponse)
        {
        }

        const ResourceRequest m_request;
        const ResourceResponse m_redirectResponse;
    };

    Vector<RedirectPair> m_redirectChain;
};

}

#endif // CachedRawResource_h
