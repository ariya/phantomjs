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

#ifndef ArgumentCodersMac_h
#define ArgumentCodersMac_h

#include <wtf/RetainPtr.h>

namespace CoreIPC {

class ArgumentEncoder;
class ArgumentDecoder;

// id
void encode(ArgumentEncoder&, id);
bool decode(ArgumentDecoder&, RetainPtr<id>&);

// NSAttributedString
void encode(ArgumentEncoder&, NSAttributedString *);
bool decode(ArgumentDecoder&, RetainPtr<NSAttributedString>&);

#if USE(APPKIT)
// NSColor
void encode(ArgumentEncoder&, NSColor *);
bool decode(ArgumentDecoder&, RetainPtr<NSColor>&);
#endif

// NSDictionary
void encode(ArgumentEncoder&, NSDictionary *);
bool decode(ArgumentDecoder&, RetainPtr<NSDictionary>&);

// NSArray
void encode(ArgumentEncoder&, NSArray *);
bool decode(ArgumentDecoder&, RetainPtr<NSArray>&);

#if USE(APPKIT)
// NSFont
void encode(ArgumentEncoder&, NSFont *);
bool decode(ArgumentDecoder&, RetainPtr<NSFont>&);
#endif

// NSNumber
void encode(ArgumentEncoder&, NSNumber *);
bool decode(ArgumentDecoder&, RetainPtr<NSNumber>&);

// NSString
void encode(ArgumentEncoder&, NSString *);
bool decode(ArgumentDecoder&, RetainPtr<NSString>&);

// NSDate
void encode(ArgumentEncoder&, NSDate *);
bool decode(ArgumentDecoder&, RetainPtr<NSDate>&);

// NSData
void encode(ArgumentEncoder&, NSData *);
bool decode(ArgumentDecoder&, RetainPtr<NSData>&);

} // namespace CoreIPC

#endif // ArgumentCodersMac_h
