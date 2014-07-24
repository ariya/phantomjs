/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple, Inc.  All rights reserved.
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

#include "QTCFDictionary.h"

#include <CFData.h>
#include <windows.h>

CFDataRef QTCFPropertyListCreateXMLData(CFAllocatorRef allocator, CFPropertyListRef propertyList)
{

    typedef CFDataRef (* pfnCFPropertyListCreateXMLData)(CFAllocatorRef allocator, CFPropertyListRef propertyList);
    static pfnCFPropertyListCreateXMLData pCFPropertyListCreateXMLData = 0;
    if (!pCFPropertyListCreateXMLData) {
        HMODULE qtcfDLL = LoadLibraryW(L"QTCF.dll");
        if (qtcfDLL)
            pCFPropertyListCreateXMLData = reinterpret_cast<pfnCFPropertyListCreateXMLData>(GetProcAddress(qtcfDLL, "QTCF_CFPropertyListCreateXMLData"));
    }

    if (pCFPropertyListCreateXMLData)
        return pCFPropertyListCreateXMLData(allocator, propertyList);
    return 0;
}

CFDictionaryRef QTCFDictionaryCreateCopyWithDataCallback(CFAllocatorRef allocator, CFDictionaryRef dictionary, QTCFDictonaryCreateFromDataCallback callback) 
{
    ASSERT(dictionary);
    ASSERT(callback);

    CFDataRef data = QTCFPropertyListCreateXMLData(kCFAllocatorDefault, dictionary);
    if (!data)
        return 0;
    CFDictionaryRef outputDictionary = callback(allocator, CFDataGetBytePtr(data), CFDataGetLength(data));
    CFRelease(data);

    return outputDictionary;
}
