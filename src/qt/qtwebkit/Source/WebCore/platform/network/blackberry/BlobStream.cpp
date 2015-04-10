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

#include "config.h"
#include "BlobStream.h"

#include "ResourceHandle.h"

using BlackBerry::Platform::FilterStream;

namespace WebCore {

BlobStream::BlobStream(const ResourceResponse& response, ResourceHandle* handle)
    : FilterStream()
    , m_response(response)
{
    registerStreamId();
    handle->setClient(this);
}

BlobStream::~BlobStream()
{
}

void BlobStream::didReceiveData(ResourceHandle*, const char* data, int len, int)
{
    notifyDataReceived(BlackBerry::Platform::createNetworkBufferByWrappingData(data, len));
}

void BlobStream::didFinishLoading(ResourceHandle*, double)
{
    notifyClose(FilterStream::StatusSuccess);
}

void BlobStream::didFail(ResourceHandle*, const ResourceError&)
{
    notifyClose(FilterStream::StatusErrorIO);
}

BlackBerry::Platform::String BlobStream::url() const
{
    return m_response.url().string();
}

const BlackBerry::Platform::String BlobStream::mimeType() const
{
    return m_response.mimeType();
}

} // namespace WebCore
