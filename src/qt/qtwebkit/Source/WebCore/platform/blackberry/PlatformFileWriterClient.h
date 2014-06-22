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

#ifndef PlatformFileWriterClient_h
#define PlatformFileWriterClient_h

#if ENABLE(FILE_SYSTEM)
#include <BlackBerryPlatformWebFileSystemFileWriter.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class AsyncFileWriterBlackBerry;
class AsyncFileWriterClient;

class PlatformFileWriterClient: public BlackBerry::Platform::WebFileWriterClient {
public:
    static PassOwnPtr<PlatformFileWriterClient> create(AsyncFileWriterClient* client, AsyncFileWriterBlackBerry* writer)
    {
        return adoptPtr(new PlatformFileWriterClient(client, writer));
    }

    virtual void notifyWrite(long long bytes, bool complete);
    virtual void notifyTruncate();
    virtual void notifyFail(BlackBerry::Platform::WebFileError);

protected:
    PlatformFileWriterClient(AsyncFileWriterClient* client, AsyncFileWriterBlackBerry* writer)
        : m_client(client)
        , m_writer(writer)
    { }

    ~PlatformFileWriterClient() { }

private:
    AsyncFileWriterClient* m_client;
    AsyncFileWriterBlackBerry* m_writer;
};

} // namespace WebCore

namespace WTF {

template <> inline void deleteOwnedPtr<WebCore::PlatformFileWriterClient>(WebCore::PlatformFileWriterClient* ptr)
{
    BlackBerry::Platform::deleteGuardedObject(ptr);
}

}

#endif
#endif
