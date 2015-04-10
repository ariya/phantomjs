/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef MIMEHeader_h
#define MIMEHeader_h

#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class SharedBufferChunkReader;

// FIXME: This class is a limited MIME parser used to parse the MIME headers of MHTML files.
class MIMEHeader : public RefCounted<MIMEHeader> {
public:
    enum Encoding {
        QuotedPrintable,
        Base64,
        SevenBit,
        Binary,
        Unknown
    };

    static PassRefPtr<MIMEHeader> parseHeader(SharedBufferChunkReader* crLFLineReader);

    bool isMultipart() const { return m_contentType.startsWith("multipart/"); }

    String contentType() const { return m_contentType; }
    String charset() const { return m_charset; }
    Encoding contentTransferEncoding() const { return m_contentTransferEncoding; }
    String contentLocation() const { return m_contentLocation; }

    // Multi-part type and boundaries are only valid for multipart MIME headers.
    String multiPartType() const { return m_multipartType; }
    String endOfPartBoundary() const { return m_endOfPartBoundary; }
    String endOfDocumentBoundary() const { return m_endOfDocumentBoundary; }

private:
    MIMEHeader();

    static Encoding parseContentTransferEncoding(const String&);

    String m_contentType;
    String m_charset;
    Encoding m_contentTransferEncoding;
    String m_contentLocation;
    String m_multipartType;
    String m_endOfPartBoundary;
    String m_endOfDocumentBoundary;
};

}

#endif
