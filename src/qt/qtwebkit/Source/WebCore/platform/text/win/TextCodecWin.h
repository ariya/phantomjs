/*
 * Copyright (C) 2004, 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2010-2012 Patrick Gansterer <paroga@paroga.com>
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

#ifndef TextCodecWin_h
#define TextCodecWin_h

#include "TextCodec.h"
#include "TextEncoding.h"
#include <windows.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class TextCodecWin : public TextCodec {
public:
    static void registerExtendedEncodingNames(EncodingNameRegistrar);
    static void registerExtendedCodecs(TextCodecRegistrar);

    TextCodecWin(UINT codePage);
    virtual ~TextCodecWin();

    virtual String decode(const char*, size_t length, bool flush, bool stopOnError, bool& sawError);
    virtual CString encode(const UChar*, size_t length, UnencodableHandling);

    struct EncodingInfo {
        String m_encoding;
        String m_friendlyName;
    };

    struct EncodingReceiver {
        // Return false to stop enumerating.
        virtual bool receive(const char* encoding, const wchar_t* friendlyName, unsigned int codePage) = 0;
    };

    static void enumerateSupportedEncodings(EncodingReceiver&);

private:
    UINT m_codePage;
    Vector<char> m_decodeBuffer;
};

} // namespace WebCore

#endif // TextCodecWin_h
