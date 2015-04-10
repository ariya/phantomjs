/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SharedBuffer.h"
#include <wtf/text/CString.h>

// INVALID_FILE_SIZE is not defined on WinCE.
#ifndef INVALID_FILE_SIZE
#define INVALID_FILE_SIZE 0xffffffff
#endif

namespace WebCore {

PassRefPtr<SharedBuffer> SharedBuffer::createWithContentsOfFile(const String& filePath)
{
    if (filePath.isEmpty())
        return 0;

    String nullifiedPath = filePath;
    HANDLE fileHandle = CreateFileW(nullifiedPath.charactersWithNullTermination().data(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        LOG_ERROR("Failed to open file %s to create shared buffer, GetLastError() = %u", filePath.ascii().data(), GetLastError());
        return 0;
    }

    RefPtr<SharedBuffer> result;
    DWORD bytesToRead = GetFileSize(fileHandle, 0);
    DWORD lastError = GetLastError();

    if (bytesToRead != INVALID_FILE_SIZE || lastError == NO_ERROR) {
        Vector<char> buffer(bytesToRead);
        DWORD bytesRead;
        if (ReadFile(fileHandle, buffer.data(), bytesToRead, &bytesRead, 0) && bytesToRead == bytesRead)
            result = SharedBuffer::adoptVector(buffer);
        else
            LOG_ERROR("Failed to fully read contents of file %s, GetLastError() = %u", filePath.ascii().data(), GetLastError());
    } else
        LOG_ERROR("Failed to get filesize of file %s, GetLastError() = %u", filePath.ascii().data(), lastError);

    CloseHandle(fileHandle);
    return result.release();
}

} // namespace WebCore
