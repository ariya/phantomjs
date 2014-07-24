/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#ifndef DecodedDataDocumentParser_h
#define DecodedDataDocumentParser_h

#include "DocumentParser.h"

namespace WebCore {

class DecodedDataDocumentParser : public DocumentParser {
public:
    // Only used by the XMLDocumentParser to communicate back to
    // XMLHttpRequest if the responseXML was well formed.
    virtual bool wellFormed() const { return true; }

protected:
    explicit DecodedDataDocumentParser(Document*);

private:
    // append is used by DocumentWriter::replaceDocument.
    virtual void append(PassRefPtr<StringImpl>) = 0;

    // appendBytes and flush are used by DocumentWriter (the loader).
    virtual void appendBytes(DocumentWriter*, const char* bytes, size_t length);
    virtual void flush(DocumentWriter*);
};

}

#endif // DecodedDataDocumentParser_h
