/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "FileSystemIOS.h"

#import "FileSystem.h"

namespace WebCore {

NSString *createTemporaryDirectory(NSString *directoryPrefix)
{
    NSString *tempDirectory = NSTemporaryDirectory();
    if (!tempDirectory || ![tempDirectory length])
        return nil;

    if (!directoryPrefix || ![directoryPrefix length])
        return nil;

    NSString *tempDirectoryComponent = [directoryPrefix stringByAppendingString:@"-XXXXXXXX"];
    const char* tempDirectoryCString = [[tempDirectory stringByAppendingPathComponent:tempDirectoryComponent] fileSystemRepresentation];
    if (!tempDirectoryCString)
        return nil;

    const size_t length = strlen(tempDirectoryCString);
    ASSERT(length <= MAXPATHLEN);
    if (length > MAXPATHLEN)
        return nil;

    const size_t lengthPlusNullTerminator = length + 1;
    Vector<char, MAXPATHLEN + 1> path(lengthPlusNullTerminator);
    memcpy(path.data(), tempDirectoryCString, lengthPlusNullTerminator);

    if (!mkdtemp(path.data()))
        return nil;

    return [[NSFileManager defaultManager] stringWithFileSystemRepresentation:path.data() length:length];
}

NSString *createTemporaryFile(NSString *directoryPath, NSString *filePrefix)
{
    if (!directoryPath || ![directoryPath length] || !filePrefix || ![filePrefix length])
        return nil;

    NSString *tempFileComponent = [filePrefix stringByAppendingString:@"-XXXXXXXX"];
    const char* templatePathCString = [[directoryPath stringByAppendingPathComponent:tempFileComponent] fileSystemRepresentation];
    if (!templatePathCString)
        return nil;

    const size_t length = strlen(templatePathCString);
    ASSERT(length <= MAXPATHLEN);
    if (length > MAXPATHLEN)
        return nil;

    const size_t lengthPlusNullTerminator = length + 1;
    Vector<char, MAXPATHLEN + 1> path(lengthPlusNullTerminator);
    memcpy(path.data(), templatePathCString, lengthPlusNullTerminator);

    int fd = mkstemp(path.data());
    if (fd < 0)
        return nil;

    int err = fchmod(fd, S_IRUSR | S_IWUSR);
    if (err < 0) {
        close(fd);
        unlink(path.data());
        return nil;
    }

    close(fd);
    return [[NSFileManager defaultManager] stringWithFileSystemRepresentation:path.data() length:length];
}

} // namespace WebCore

