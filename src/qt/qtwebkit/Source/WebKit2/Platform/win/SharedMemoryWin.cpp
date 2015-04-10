/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SharedMemory.h"

#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"
#include <wtf/RefPtr.h>

namespace WebKit {

SharedMemory::Handle::Handle()
    : m_handle(0)
    , m_size(0)
{
}

SharedMemory::Handle::~Handle()
{
    if (!m_handle)
        return;

    ::CloseHandle(m_handle);
}

bool SharedMemory::Handle::isNull() const
{
    return !m_handle;
}

void SharedMemory::Handle::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << static_cast<uint64_t>(m_size);

    // Hand off ownership of our HANDLE to the receiving process. It will close it for us.
    // FIXME: If the receiving process crashes before it receives the memory, the memory will be
    // leaked. See <http://webkit.org/b/47502>.
    encoder << reinterpret_cast<uint64_t>(m_handle);
    m_handle = 0;

    // Send along our PID so that the receiving process can duplicate the HANDLE for its own use.
    encoder << static_cast<uint32_t>(::GetCurrentProcessId());
}

static bool getDuplicatedHandle(HANDLE sourceHandle, DWORD sourcePID, HANDLE& duplicatedHandle)
{
    duplicatedHandle = 0;
    if (!sourceHandle)
        return true;

    HANDLE sourceProcess = ::OpenProcess(PROCESS_DUP_HANDLE, FALSE, sourcePID);
    if (!sourceProcess)
        return false;

    // Copy the handle into our process and close the handle that the sending process created for us.
    BOOL success = ::DuplicateHandle(sourceProcess, sourceHandle, ::GetCurrentProcess(), &duplicatedHandle, 0, FALSE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    ASSERT_WITH_MESSAGE(success, "::DuplicateHandle failed with error %lu", ::GetLastError());

    ::CloseHandle(sourceProcess);

    return success;
}

bool SharedMemory::Handle::decode(CoreIPC::ArgumentDecoder& decoder, Handle& handle)
{
    ASSERT_ARG(handle, !handle.m_handle);
    ASSERT_ARG(handle, !handle.m_size);

    uint64_t size;
    if (!decoder.decode(size))
        return false;

    uint64_t sourceHandle;
    if (!decoder.decode(sourceHandle))
        return false;

    uint32_t sourcePID;
    if (!decoder.decode(sourcePID))
        return false;

    HANDLE duplicatedHandle;
    if (!getDuplicatedHandle(reinterpret_cast<HANDLE>(sourceHandle), sourcePID, duplicatedHandle))
        return false;

    handle.m_handle = duplicatedHandle;
    handle.m_size = size;
    return true;
}

PassRefPtr<SharedMemory> SharedMemory::create(size_t size)
{
    HANDLE handle = ::CreateFileMappingW(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, size, 0);
    if (!handle)
        return 0;

    void* baseAddress = ::MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (!baseAddress) {
        ::CloseHandle(handle);
        return 0;
    }

    RefPtr<SharedMemory> memory = adoptRef(new SharedMemory);
    memory->m_size = size;
    memory->m_data = baseAddress;
    memory->m_handle = handle;

    return memory.release();
}

static DWORD accessRights(SharedMemory::Protection protection)
{
    switch (protection) {
    case SharedMemory::ReadOnly:
        return FILE_MAP_READ;
    case SharedMemory::ReadWrite:
        return FILE_MAP_READ | FILE_MAP_WRITE;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

PassRefPtr<SharedMemory> SharedMemory::create(const Handle& handle, Protection protection)
{
    RefPtr<SharedMemory> memory = adopt(handle.m_handle, handle.m_size, protection);
    if (!memory)
        return 0;

    // The SharedMemory object now owns the HANDLE.
    handle.m_handle = 0;

    return memory.release();
}

PassRefPtr<SharedMemory> SharedMemory::adopt(HANDLE handle, size_t size, Protection protection)
{
    if (!handle)
        return 0;

    DWORD desiredAccess = accessRights(protection);

    void* baseAddress = ::MapViewOfFile(handle, desiredAccess, 0, 0, size);
    ASSERT_WITH_MESSAGE(baseAddress, "::MapViewOfFile failed with error %lu", ::GetLastError());
    if (!baseAddress)
        return 0;

    RefPtr<SharedMemory> memory = adoptRef(new SharedMemory);
    memory->m_size = size;
    memory->m_data = baseAddress;
    memory->m_handle = handle;

    return memory.release();
}

SharedMemory::~SharedMemory()
{
    ASSERT(m_data);
    ASSERT(m_handle);

    ::UnmapViewOfFile(m_data);
    ::CloseHandle(m_handle);
}

bool SharedMemory::createHandle(Handle& handle, Protection protection)
{
    ASSERT_ARG(handle, !handle.m_handle);
    ASSERT_ARG(handle, !handle.m_size);

    HANDLE processHandle = ::GetCurrentProcess();

    HANDLE duplicatedHandle;
    if (!::DuplicateHandle(processHandle, m_handle, processHandle, &duplicatedHandle, accessRights(protection), FALSE, 0))
        return false;

    handle.m_handle = duplicatedHandle;
    handle.m_size = m_size;
    return true;
}

PassRefPtr<SharedMemory> SharedMemory::createCopyOnWriteCopy(size_t size) const
{
    ASSERT_ARG(size, size <= this->size());

    HANDLE duplicatedHandle;
    BOOL result = ::DuplicateHandle(::GetCurrentProcess(), m_handle, ::GetCurrentProcess(), &duplicatedHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);
    ASSERT_WITH_MESSAGE(result, "::DuplicateHandle failed with error %lu", ::GetLastError());
    if (!result)
        return 0;

    void* newMapping = ::MapViewOfFile(duplicatedHandle, FILE_MAP_COPY, 0, 0, size);
    ASSERT_WITH_MESSAGE(newMapping, "::MapViewOfFile failed with error %lu", ::GetLastError());
    if (!newMapping) {
        ::CloseHandle(duplicatedHandle);
        return 0;
    }

    RefPtr<SharedMemory> memory = adoptRef(new SharedMemory);
    memory->m_size = size;
    memory->m_data = newMapping;
    memory->m_handle = duplicatedHandle;

    return memory.release();
}

unsigned SharedMemory::systemPageSize()
{
    static unsigned pageSize = 0;

    if (!pageSize) {
        SYSTEM_INFO systemInfo;
        ::GetSystemInfo(&systemInfo);
        pageSize = systemInfo.dwPageSize;
    }

    return pageSize;
}

} // namespace WebKit
