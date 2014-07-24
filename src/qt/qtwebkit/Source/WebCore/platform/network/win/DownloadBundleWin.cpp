/*
 * Copyright (C) 2011 Apple Inc.  All rights reserved.
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
#include "DownloadBundle.h"

#include <CoreFoundation/CoreFoundation.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

namespace DownloadBundle {

static UInt32 magicNumber()
{
    return 0xDECAF4EA;
}

const String& fileExtension()
{
    DEFINE_STATIC_LOCAL(const String, extension, (ASCIILiteral(".download")));
    return extension;
}

bool appendResumeData(CFDataRef resumeData, const String& bundlePath)
{
    if (!resumeData) {
        LOG_ERROR("Invalid resume data to write to bundle path");
        return false;
    }
    if (bundlePath.isEmpty()) {
        LOG_ERROR("Cannot write resume data to empty download bundle path");
        return false;
    }

    String nullifiedPath = bundlePath;
    FILE* bundle = 0;
    if (_wfopen_s(&bundle, nullifiedPath.charactersWithNullTermination().data(), TEXT("ab")) || !bundle) {
        LOG_ERROR("Failed to open file %s to append resume data", bundlePath.ascii().data());
        return false;
    }

    bool result = false;

    const UInt8* resumeBytes = CFDataGetBytePtr(resumeData);
    ASSERT(resumeBytes);
    if (!resumeBytes)
        goto exit;

    CFIndex resumeLength = CFDataGetLength(resumeData);
    ASSERT(resumeLength > 0);
    if (resumeLength < 1)
        goto exit;

    if (fwrite(resumeBytes, 1, resumeLength, bundle) != resumeLength) {
        LOG_ERROR("Failed to write resume data to the bundle - errno(%i)", errno);
        goto exit;
    }

    if (fwrite(&resumeLength, 4, 1, bundle) != 1) {
        LOG_ERROR("Failed to write footer length to the bundle - errno(%i)", errno);
        goto exit;
    }

    const UInt32& magic = magicNumber();
    if (fwrite(&magic, 4, 1, bundle) != 1) {
        LOG_ERROR("Failed to write footer magic number to the bundle - errno(%i)", errno);
        goto exit;
    }

    result = true;
exit:
    fclose(bundle);
    return result;
}

CFDataRef extractResumeData(const String& bundlePath)
{
    if (bundlePath.isEmpty()) {
        LOG_ERROR("Cannot create resume data from empty download bundle path");
        return 0;
    }

    // Open a handle to the bundle file
    String nullifiedPath = bundlePath;
    FILE* bundle = 0;
    if (_wfopen_s(&bundle, nullifiedPath.charactersWithNullTermination().data(), TEXT("r+b")) || !bundle) {
        LOG_ERROR("Failed to open file %s to get resume data", bundlePath.ascii().data());
        return 0;
    }

    CFDataRef result = 0;
    Vector<UInt8> footerBuffer;

    // Stat the file to get its size
    struct _stat64 fileStat;
    if (_fstat64(_fileno(bundle), &fileStat))
        goto exit;

    // Check for the bundle magic number at the end of the file
    fpos_t footerMagicNumberPosition = fileStat.st_size - 4;
    ASSERT(footerMagicNumberPosition >= 0);
    if (footerMagicNumberPosition < 0)
        goto exit;
    if (fsetpos(bundle, &footerMagicNumberPosition))
        goto exit;

    UInt32 footerMagicNumber = 0;
    if (fread(&footerMagicNumber, 4, 1, bundle) != 1) {
        LOG_ERROR("Failed to read footer magic number from the bundle - errno(%i)", errno);
        goto exit;
    }

    if (footerMagicNumber != magicNumber()) {
        LOG_ERROR("Footer's magic number does not match 0x%X - errno(%i)", magicNumber(), errno);
        goto exit;
    }

    // Now we're *reasonably* sure this is a .download bundle we actually wrote.
    // Get the length of the resume data
    fpos_t footerLengthPosition = fileStat.st_size - 8;
    ASSERT(footerLengthPosition >= 0);
    if (footerLengthPosition < 0)
        goto exit;

    if (fsetpos(bundle, &footerLengthPosition))
        goto exit;

    UInt32 footerLength = 0;
    if (fread(&footerLength, 4, 1, bundle) != 1) {
        LOG_ERROR("Failed to read ResumeData length from the bundle - errno(%i)", errno);
        goto exit;
    }

    // Make sure theres enough bytes to read in for the resume data, and perform the read
    fpos_t footerStartPosition = fileStat.st_size - 8 - footerLength;
    ASSERT(footerStartPosition >= 0);
    if (footerStartPosition < 0)
        goto exit;
    if (fsetpos(bundle, &footerStartPosition))
        goto exit;

    footerBuffer.resize(footerLength);
    if (fread(footerBuffer.data(), 1, footerLength, bundle) != footerLength) {
        LOG_ERROR("Failed to read ResumeData from the bundle - errno(%i)", errno);
        goto exit;
    }

    // CFURLDownload will seek to the appropriate place in the file (before our footer) and start overwriting from there
    // However, say we were within a few hundred bytes of the end of a download when it was paused -
    // The additional footer extended the length of the file beyond its final length, and there will be junk data leftover
    // at the end.  Therefore, now that we've retrieved the footer data, we need to truncate it.
    if (errno_t resizeError = _chsize_s(_fileno(bundle), footerStartPosition)) {
        LOG_ERROR("Failed to truncate the resume footer off the end of the file - errno(%i)", resizeError);
        goto exit;
    }

    // Finally, make the resume data.  Now, it is possible by some twist of fate the bundle magic number
    // was naturally at the end of the file and its not actually a valid bundle.  That, or someone engineered
    // it that way to try to attack us.  In that cause, this CFData will successfully create but when we 
    // actually try to start the CFURLDownload using this bogus data, it will fail and we will handle that gracefully
    result = CFDataCreate(0, footerBuffer.data(), footerLength);
exit:
    fclose(bundle);
    return result;
}

} // namespace DownloadBundle

} // namespace WebCore
