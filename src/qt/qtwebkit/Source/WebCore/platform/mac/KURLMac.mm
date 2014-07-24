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

#import "CFURLExtras.h"
#import "FoundationExtras.h"
#import <wtf/text/CString.h>

namespace WebCore {

KURL::KURL(NSURL *url)
{
    if (!url) {
        invalidate();
        return;
    }

    // FIXME: Why is it OK to ignore base URL here?
    CString urlBytes;
    getURLBytes(reinterpret_cast<CFURLRef>(url), urlBytes);
    parse(urlBytes.data());
}

KURL::operator NSURL *() const
{
    // Creating a toll-free bridged CFURL, because a real NSURL would not preserve the original string.
    // We'll need fidelity when round-tripping via CFURLGetBytes().
    return HardAutorelease(createCFURL().leakRef());
}

RetainPtr<CFURLRef> KURL::createCFURL() const
{
    if (isNull())
        return 0;

    if (isEmpty()) {
        // We use the toll-free bridge between NSURL and CFURL to
        // create a CFURLRef supporting both empty and null values.
        RetainPtr<NSURL> emptyNSURL = adoptNS([[NSURL alloc] initWithString:@""]);
        return reinterpret_cast<CFURLRef>(emptyNSURL.get());
    }

    URLCharBuffer buffer;
    copyToBuffer(buffer);
    return createCFURLFromBuffer(buffer.data(), buffer.size());
}



}
