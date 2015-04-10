/*
 * Copyright (C) 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef BlobStream_h
#define BlobStream_h

#include "ResourceHandleClient.h"
#include "ResourceResponse.h"

#include <network/FilterStream.h>

namespace WebCore {

class ResourceHandle;

class BlobStream : public ResourceHandleClient, public BlackBerry::Platform::FilterStream {
public:
    BlobStream(const ResourceResponse&, ResourceHandle*);
    virtual ~BlobStream();

    // From class BlackBerry::Platform::FilterStream.
    virtual BlackBerry::Platform::String url() const OVERRIDE;
    virtual const BlackBerry::Platform::String mimeType() const OVERRIDE;

    // From class ResourceHandleClient.
    virtual void didReceiveData(ResourceHandle*, const char*, int, int) OVERRIDE;
    virtual void didFinishLoading(ResourceHandle*, double) OVERRIDE;
    virtual void didFail(ResourceHandle*, const ResourceError&) OVERRIDE;

private:
    ResourceResponse m_response;
};

} // namespace WebCore

#endif // BlobStream_h
