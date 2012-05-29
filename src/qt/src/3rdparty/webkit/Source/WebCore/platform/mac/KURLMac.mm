/*
 * Copyright (C) 2004, 2008 Apple Inc. All rights reserved.
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

#import "config.h"
#import "KURL.h"

#import "FoundationExtras.h"
#import <CoreFoundation/CFURL.h>

namespace WebCore {

KURL::KURL(NSURL *url)
{
    if (!url) {
        parse(0, 0);
        return;
    }

    CFIndex bytesLength = CFURLGetBytes(reinterpret_cast<CFURLRef>(url), 0, 0);
    Vector<char, 512> buffer(bytesLength + 6); // 5 for "file:", 1 for null character to end C string
    char* bytes = &buffer[5];
    CFURLGetBytes(reinterpret_cast<CFURLRef>(url), reinterpret_cast<UInt8*>(bytes), bytesLength);
    bytes[bytesLength] = '\0';
    if (bytes[0] != '/') {
        parse(bytes, 0);
        return;
    }

    buffer[0] = 'f';
    buffer[1] = 'i';
    buffer[2] = 'l';
    buffer[3] = 'e';
    buffer[4] = ':';

    parse(buffer.data(), 0);
}

KURL::operator NSURL *() const
{
    if (isNull())
        return nil;

    // CFURL can't hold an empty URL, unlike NSURL.
    if (isEmpty())
        return [NSURL URLWithString:@""];

    return HardAutorelease(createCFURL());
}

}
