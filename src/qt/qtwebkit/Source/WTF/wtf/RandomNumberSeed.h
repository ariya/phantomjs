/*
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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

#ifndef WTF_RandomNumberSeed_h
#define WTF_RandomNumberSeed_h

#include "RandomNumber.h"
#include <stdlib.h>
#include <time.h>

#if HAVE(SYS_TIME_H)
#include <sys/time.h>
#endif

#if OS(UNIX)
#include <sys/types.h>
#include <unistd.h>
#endif

namespace WTF {

inline void initializeRandomNumberGenerator()
{
#if OS(DARWIN)
    // On Darwin we use arc4random which initialises itself.
#elif OS(WINCE)
    // initialize rand()
    srand(GetTickCount());
#elif COMPILER(MSVC) && defined(_CRT_RAND_S)
    // On Windows we use rand_s which initialises itself
#elif OS(UNIX)
    // srandomdev is not guaranteed to exist on linux so we use this poor seed, this should be improved
    timeval time;
    gettimeofday(&time, 0);
    srandom(static_cast<unsigned>(time.tv_usec * getpid()));
#else
    srand(static_cast<unsigned>(time(0)));
#endif

}

}

#endif
