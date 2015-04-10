/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */
#include "config.h"
#if ENABLE(FILE_SYSTEM)
#include "PlatformFileWriterClient.h"

#include "AsyncFileWriterBlackBerry.h"
#include "AsyncFileWriterClient.h"

namespace WebCore {

void PlatformFileWriterClient::notifyWrite(long long bytes, bool complete)
{
    if (m_writer->isAborting())
        return;

    if (complete)
        m_writer->finishOperation();
    m_client->didWrite(bytes, complete);
}

void PlatformFileWriterClient::notifyTruncate()
{
    if (m_writer->isAborting())
        return;

    m_writer->finishOperation();
    m_client->didTruncate();
}

void PlatformFileWriterClient::notifyFail(BlackBerry::Platform::WebFileError error)
{
    if (m_writer->isAborting() && error != BlackBerry::Platform::WebFileErrorAbort)
        return;

    // We don't reset m_isAborting unitl new operations (write, truncate) arrive.
    // notifyFail for abort may not be called because the platform writer may be
    // in successful state for a previous operation (write or truncate) but the
    // notification hasn't arrived in the thread we are running in. So if we reset
    // m_isAborting here and ASSERT(!m_isAborting) in other operations we could hit
    // the ASSERT because m_isAborting may not be reset.

    m_writer->finishOperation();
    m_client->didFail(static_cast<FileError::ErrorCode>(error));
}

}

#endif
