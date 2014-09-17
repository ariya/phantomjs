/*
 * Copyright (C) 2010 Company 100, Inc. All rights reserved.
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

#include "FileSystem.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wtf/text/CString.h>

namespace WebCore {

PassRefPtr<SharedBuffer> SharedBuffer::createWithContentsOfFile(const String& filePath)
{
    if (filePath.isEmpty())
        return 0;

    CString filename = fileSystemRepresentation(filePath);
    int fd = open(filename.data(), O_RDONLY);
    if (fd == -1)
        return 0;

    struct stat fileStat;
    if (fstat(fd, &fileStat)) {
        close(fd);
        return 0;
    }

    size_t bytesToRead = fileStat.st_size;
    if (bytesToRead != fileStat.st_size) {
        close(fd);
        return 0;
    }

    RefPtr<SharedBuffer> result = create();
    result->m_buffer.grow(bytesToRead);

    size_t totalBytesRead = 0;
    ssize_t bytesRead;
    while ((bytesRead = read(fd, result->m_buffer.data() + totalBytesRead, bytesToRead - totalBytesRead)) > 0)
        totalBytesRead += bytesRead;

    close(fd);

    return totalBytesRead == bytesToRead ? result.release() : 0;
}

} // namespace WebCore
