/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2004, 2006, 2007, 2008 Apple Inc. All rights reserved.

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

#ifndef CachedResourceRequest_h
#define CachedResourceRequest_h

#include "FrameLoaderTypes.h"
#include "SubresourceLoader.h"
#include "SubresourceLoaderClient.h"
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    class CachedResource;
    class CachedResourceLoader;
    class Request;

    class CachedResourceRequest : public RefCounted<CachedResourceRequest>, private SubresourceLoaderClient {
    public:
        static PassRefPtr<CachedResourceRequest> load(CachedResourceLoader*, CachedResource*, bool incremental, SecurityCheckPolicy, bool sendResourceLoadCallbacks);
        ~CachedResourceRequest();
        void didFail(bool cancelled = false);

        CachedResourceLoader* cachedResourceLoader() const { return m_cachedResourceLoader; }

    private:
        CachedResourceRequest(CachedResourceLoader*, CachedResource*, bool incremental);
        virtual void willSendRequest(SubresourceLoader*, ResourceRequest&, const ResourceResponse&);
        virtual void didReceiveResponse(SubresourceLoader*, const ResourceResponse&);
        virtual void didReceiveData(SubresourceLoader*, const char*, int);
        virtual void didReceiveCachedMetadata(SubresourceLoader*, const char*, int);
        virtual void didFinishLoading(SubresourceLoader*, double);
        virtual void didFail(SubresourceLoader*, const ResourceError&);

        RefPtr<SubresourceLoader> m_loader;
        CachedResourceLoader* m_cachedResourceLoader;
        CachedResource* m_resource;
        bool m_incremental;
        bool m_multipart;
        bool m_finishing;
    };

}

#endif
