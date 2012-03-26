/*
 * Copyright (c) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FontTranscoder_h
#define FontTranscoder_h

#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class FontDescription;
class TextEncoding;

class FontTranscoder {
    WTF_MAKE_NONCOPYABLE(FontTranscoder); WTF_MAKE_FAST_ALLOCATED;
public:
    void convert(String& text, const FontDescription&, const TextEncoding* = 0) const;
    bool needsTranscoding(const FontDescription&, const TextEncoding* = 0) const;

private:
    FontTranscoder();
    ~FontTranscoder(); // Not implemented to make sure nobody accidentally calls delete -- WebCore does not delete singletons.

    enum ConverterType {
        NoConversion, BackslashToYenSign,
    };

    ConverterType converterType(const FontDescription&, const TextEncoding*) const;

    HashMap<AtomicString, ConverterType> m_converterTypes;

    friend FontTranscoder& fontTranscoder();
};

FontTranscoder& fontTranscoder();

} // namespace WebCore

#endif // FontTranscoder_h
