/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ArgumentCodersCF_h
#define ArgumentCodersCF_h

#include <wtf/RetainPtr.h>

#if USE(SECURITY_FRAMEWORK)
#include <Security/SecCertificate.h>
#include <Security/SecKeychainItem.h>
#endif

namespace CoreIPC {

class ArgumentEncoder;
class ArgumentDecoder;

// CFArrayRef
void encode(ArgumentEncoder&, CFArrayRef);
bool decode(ArgumentDecoder&, RetainPtr<CFArrayRef>& result);

// CFBooleanRef
void encode(ArgumentEncoder&, CFBooleanRef);
bool decode(ArgumentDecoder&, RetainPtr<CFBooleanRef>& result);

// CFDataRef
void encode(ArgumentEncoder&, CFDataRef);
bool decode(ArgumentDecoder&, RetainPtr<CFDataRef>& result);

// CFDateRef
void encode(ArgumentEncoder&, CFDateRef);
bool decode(ArgumentDecoder&, RetainPtr<CFDateRef>& result);

// CFDictionaryRef
void encode(ArgumentEncoder&, CFDictionaryRef);
bool decode(ArgumentDecoder&, RetainPtr<CFDictionaryRef>& result);

// CFNumberRef
void encode(ArgumentEncoder&, CFNumberRef);
bool decode(ArgumentDecoder&, RetainPtr<CFNumberRef>& result);

// CFStringRef
void encode(ArgumentEncoder&, CFStringRef);
bool decode(ArgumentDecoder&, RetainPtr<CFStringRef>& result);

// CFTypeRef
void encode(ArgumentEncoder&, CFTypeRef);
bool decode(ArgumentDecoder&, RetainPtr<CFTypeRef>& result);

// CFURLRef
void encode(ArgumentEncoder&, CFURLRef);
bool decode(ArgumentDecoder&, RetainPtr<CFURLRef>& result);

#if USE(SECURITY_FRAMEWORK)
// SecCertificateRef
void encode(ArgumentEncoder&, SecCertificateRef);
bool decode(ArgumentDecoder&, RetainPtr<SecCertificateRef>& result);

// SecKeychainItemRef
void encode(ArgumentEncoder&, SecKeychainItemRef);
bool decode(ArgumentDecoder&, RetainPtr<SecKeychainItemRef>& result);
#endif

CFTypeRef tokenNullTypeRef();

} // namespace CoreIPC

#endif // ArgumentCodersCF_h
