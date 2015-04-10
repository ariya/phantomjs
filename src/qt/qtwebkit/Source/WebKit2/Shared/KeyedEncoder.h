/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef KeyedEncoder_h
#define KeyedEncoder_h

#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

// FIXME: I believe this class can be moved to either WTF or WebCore
// and replace the Encoder/Decoder classes there. Currently the Encoder/Decoder
// classes are used to serialize history trees and using a keyed encoder would be
// less fragile from a binary compatibility perspective.

namespace WebKit {

struct KeyedCodingValue;

class KeyedEncoder {
public:
    KeyedEncoder();
    ~KeyedEncoder();

    void encode(const String& key, const String& value);

private:
#if 0
    struct Value {
        enum Type {
            StringValue,
            ObjectValue,
        };

        String string;
        HashMap<String, Value> object;
    };

    HashMap<String, Value> root;
#endif

    HashMap<String, KeyedCodingValue> m_rootObject;
    HashMap<String, KeyedCodingValue>* m_currentObject;
};

} // namespace WebKit

#endif // KeyedEncoder_h
