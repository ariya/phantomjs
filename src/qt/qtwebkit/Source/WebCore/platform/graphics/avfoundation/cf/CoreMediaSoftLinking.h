/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "SoftLinking.h"

// Soft-link against CoreMedia functions and variables required by MediaPlayerPrivateAVFoundationCF.cpp.

#ifdef DEBUG_ALL
SOFT_LINK_DEBUG_LIBRARY(CoreMedia)
#else
SOFT_LINK_LIBRARY(CoreMedia)
#endif

// Functions

SOFT_LINK_DLL_IMPORT(CoreMedia, CMTimeAdd, CMTime, __cdecl, (CMTime addend1, CMTime addend2), (addend1, addend2))
#define CMTimeAdd softLink_CMTimeAdd

SOFT_LINK_DLL_IMPORT(CoreMedia, CMTimeCompare, int32_t, __cdecl, (CMTime time1, CMTime time2), (time1, time2))
#define CMTimeCompare softLink_CMTimeCompare

SOFT_LINK_DLL_IMPORT(CoreMedia, CMTimeGetSeconds, Float64, __cdecl, (CMTime time), (time))
#define CMTimeGetSeconds softLink_CMTimeGetSeconds

SOFT_LINK_DLL_IMPORT(CoreMedia, CMTimeMake, CMTime, __cdecl, (int64_t value, int32_t timescale), (value, timescale))
#define CMTimeMake softLink_CMTimeMake

SOFT_LINK_DLL_IMPORT(CoreMedia, CMTimeMakeFromDictionary, CMTime, __cdecl, (CFDictionaryRef dict), (dict))
#define CMTimeMakeFromDictionary softLink_CMTimeMakeFromDictionary

SOFT_LINK_DLL_IMPORT(CoreMedia, CMTimeMakeWithSeconds, CMTime, __cdecl, (Float64 seconds, int32_t preferredTimeScale), (seconds, preferredTimeScale))
#define CMTimeMakeWithSeconds softLink_CMTimeMakeWithSeconds

SOFT_LINK_DLL_IMPORT(CoreMedia, CMTimeRangeGetEnd, CMTime, __cdecl, (CMTimeRange range), (range))
#define CMTimeRangeGetEnd softLink_CMTimeRangeGetEnd

// Variables

SOFT_LINK_VARIABLE_DLL_IMPORT(CoreMedia, kCMTimeZero, const CMTime);
#define kCMTimeZero getkCMTimeZero()
