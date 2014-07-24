/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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

#ifndef MarshallingHelpers_H
#define MarshallingHelpers_H

#include <wtf/Forward.h>
#include <CoreFoundation/CoreFoundation.h>

namespace WebCore {
    class IntRect;
    class KURL;
}

class MarshallingHelpers
{
public:
    static WebCore::KURL BSTRToKURL(BSTR);
    static BSTR KURLToBSTR(const WebCore::KURL&);
    static CFURLRef PathStringToFileCFURLRef(const WTF::String&);
    static WTF::String FileCFURLRefToPathString(CFURLRef fileURL);
    static CFURLRef BSTRToCFURLRef(BSTR);
    static CFStringRef BSTRToCFStringRef(BSTR);
    static CFStringRef LPCOLESTRToCFStringRef(LPCOLESTR);
    static BSTR CFStringRefToBSTR(CFStringRef);
    static int CFNumberRefToInt(CFNumberRef);
    static CFNumberRef intToCFNumberRef(int);
    static CFAbsoluteTime DATEToCFAbsoluteTime(DATE);
    static DATE CFAbsoluteTimeToDATE(CFAbsoluteTime);
    static SAFEARRAY* stringArrayToSafeArray(CFArrayRef);
    static SAFEARRAY* intArrayToSafeArray(CFArrayRef);
    static SAFEARRAY* intRectToSafeArray(const WebCore::IntRect&);
    static SAFEARRAY* iunknownArrayToSafeArray(CFArrayRef);
    static CFArrayRef safeArrayToStringArray(SAFEARRAY*);
    static CFArrayRef safeArrayToIntArray(SAFEARRAY*);
    static CFArrayRef safeArrayToIUnknownArray(SAFEARRAY*);
    static const void* IUnknownRetainCallback(CFAllocatorRef, const void*);
    static void IUnknownReleaseCallback(CFAllocatorRef, const void*);
    static CFArrayCallBacks kIUnknownArrayCallBacks;
    static CFDictionaryValueCallBacks kIUnknownDictionaryValueCallBacks;

private:
    static CFAbsoluteTime MarshallingHelpers::windowsEpochAbsoluteTime();

private:
    MarshallingHelpers();
    ~MarshallingHelpers();
};

#endif
