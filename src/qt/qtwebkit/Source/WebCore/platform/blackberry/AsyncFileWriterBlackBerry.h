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

#ifndef AsyncFileWriterBlackBerry_h
#define AsyncFileWriterBlackBerry_h

#if ENABLE(FILE_SYSTEM)
#include "AsyncFileWriter.h"
#include "PlatformFileWriterClient.h"

#include <BlackBerryPlatformWebFileSystemFileWriter.h>
#include <wtf/Assertions.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

namespace WTF {

template <> inline void deleteOwnedPtr<BlackBerry::Platform::WebFileWriter>(BlackBerry::Platform::WebFileWriter* ptr)
{
    BlackBerry::Platform::deleteGuardedObject(ptr);
}

}

namespace WebCore {

class AsyncFileWriterBlackBerry: public AsyncFileWriter {
public:
    static PassOwnPtr<AsyncFileWriterBlackBerry> create(PassOwnPtr<BlackBerry::Platform::WebFileWriter> platformWriter, AsyncFileWriterClient* client)
    {
        return adoptPtr(new AsyncFileWriterBlackBerry(platformWriter, client));
    }

    virtual void write(long long position, Blob*);

    virtual void truncate(long long length)
    {
        beginWriteOrTruncate();
        m_platformWriter->truncate(length);
    }

    virtual void abort()
    {
        beginAbort();
        m_platformWriter->abort();
    }

    virtual bool waitForOperationToComplete()
    {
        return m_platformWriter->waitForOperationToComplete();
    }

    bool isAborting() const
    {
        return m_isAborting;
    }

    bool isOperationInProgress() const
    {
        return m_isOperationInProgress;
    }

    void finishOperation()
    {
        m_isOperationInProgress = false;
    }

    virtual PlatformFileWriterClient* platformWriterClient()
    {
        if (!m_platformWriterClient)
            m_platformWriterClient = PlatformFileWriterClient::create(m_client, this);
        return m_platformWriterClient.get();
    }

protected:
    AsyncFileWriterBlackBerry(PassOwnPtr<BlackBerry::Platform::WebFileWriter> platformWriter, AsyncFileWriterClient* client)
        : m_platformWriter(platformWriter)
        , m_client(client)
        , m_isAborting(false)
        , m_isOperationInProgress(false)
    {
    }

    void beginWriteOrTruncate()
    {
        m_isAborting = false;
        m_isOperationInProgress = true;
    }

    void beginAbort()
    {
        m_isAborting = true;
        m_isOperationInProgress = true;
    }

    OwnPtr<BlackBerry::Platform::WebFileWriter> m_platformWriter;
    OwnPtr<PlatformFileWriterClient> m_platformWriterClient;
    AsyncFileWriterClient* m_client;
    bool m_isAborting;
    bool m_isOperationInProgress;
};

} // namespace WebCore
#endif
#endif
