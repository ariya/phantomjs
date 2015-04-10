/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
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

#ifndef TextCodecMac_h
#define TextCodecMac_h

#include "TextCodec.h"
#include <CoreServices/CoreServices.h>

namespace WebCore {

    typedef ::TextEncoding TECTextEncodingID;
    const TECTextEncodingID invalidEncoding = kCFStringEncodingInvalidId;

    class TextCodecMac : public TextCodec {
    public:
        static void registerEncodingNames(EncodingNameRegistrar);
        static void registerCodecs(TextCodecRegistrar);

        explicit TextCodecMac(TECTextEncodingID);
        virtual ~TextCodecMac();

        virtual String decode(const char*, size_t length, bool flush, bool stopOnError, bool& sawError);
        virtual CString encode(const UChar*, size_t length, UnencodableHandling);

    private:
        OSStatus decode(const unsigned char* inputBuffer, int inputBufferLength, int& inputLength,
            void* outputBuffer, int outputBufferLength, int& outputLength);

        OSStatus createTECConverter() const;
        void releaseTECConverter() const;

        TECTextEncodingID m_encoding;
        UChar m_backslashAsCurrencySymbol;
        unsigned m_numBufferedBytes;
        unsigned char m_bufferedBytes[16]; // bigger than any single multi-byte character
        mutable TECObjectRef m_converterTEC;
    };

    struct TECConverterWrapper {
        TECConverterWrapper() : converter(0), encoding(invalidEncoding) { }
        ~TECConverterWrapper() { if (converter) TECDisposeConverter(converter); }

        TECObjectRef converter;
        TECTextEncodingID encoding;
    };

} // namespace WebCore

#endif // TextCodecMac_h
