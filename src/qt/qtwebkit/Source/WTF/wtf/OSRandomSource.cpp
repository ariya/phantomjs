/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE, INC. ``AS IS'' AND ANY
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
#include "OSRandomSource.h"

#include <stdint.h>
#include <stdlib.h>

#if OS(UNIX)
#include <fcntl.h>
#include <unistd.h>
#endif

#if OS(WINDOWS)
#include <windows.h>
#include <wincrypt.h> // windows.h must be included before wincrypt.h.
#endif

namespace WTF {

void cryptographicallyRandomValuesFromOS(unsigned char* buffer, size_t length)
{
#if OS(UNIX)
    int fd = open("/dev/urandom", O_RDONLY, 0);
    if (fd < 0)
        CRASH(); // We need /dev/urandom for this API to work...

    if (read(fd, buffer, length) != static_cast<ssize_t>(length))
        CRASH();

    close(fd);
#elif OS(WINDOWS)
    HCRYPTPROV hCryptProv = 0;
    if (!CryptAcquireContext(&hCryptProv, 0, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        CRASH();
    if (!CryptGenRandom(hCryptProv, length, buffer))
        CRASH();
    CryptReleaseContext(hCryptProv, 0);
#else
    #error "This configuration doesn't have a strong source of randomness."
    // WARNING: When adding new sources of OS randomness, the randomness must
    //          be of cryptographic quality!
#endif
}

}
