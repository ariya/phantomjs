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

#ifndef CachedRawResourceClient_h
#define CachedRawResourceClient_h

#include "CachedResourceClient.h"

namespace WebCore {

class CachedResource;
class ResourceRequest;
class ResourceResponse;

class CachedRawResourceClient : public CachedResourceClient {
public:
    virtual ~CachedRawResourceClient() { }
    static CachedResourceClientType expectedType() { return RawResourceType; }
    virtual CachedResourceClientType resourceClientType() const OVERRIDE { return expectedType(); }

    virtual void dataSent(CachedResource*, unsigned long long /* bytesSent */, unsigned long long /* totalBytesToBeSent */) { }
    virtual void responseReceived(CachedResource*, const ResourceResponse&) { }
    virtual void dataReceived(CachedResource*, const char* /* data */, int /* length */) { }
    virtual void redirectReceived(CachedResource*, ResourceRequest&, const ResourceResponse&) { }
#if USE(SOUP)
    virtual char* getOrCreateReadBuffer(CachedResource*, size_t /* requestedSize */, size_t& /* actualSize */) { return 0; }
#endif
};

}

#endif // CachedRawResourceClient_h
