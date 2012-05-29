/*
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTTPParsers_h
#define HTTPParsers_h

#include <wtf/Forward.h>

namespace WebCore {

class ResourceResponseBase;

enum XSSProtectionDisposition {
    XSSProtectionDisabled,
    XSSProtectionEnabled,
    XSSProtectionBlockEnabled
};

typedef enum {
    ContentDispositionNone,
    ContentDispositionInline,
    ContentDispositionAttachment,
    ContentDispositionOther
} ContentDispositionType;

ContentDispositionType contentDispositionType(const String&);
bool parseHTTPRefresh(const String& refresh, bool fromHttpEquivMeta, double& delay, String& url);
double parseDate(const String&);
String filenameFromHTTPContentDisposition(const String&); 
String extractMIMETypeFromMediaType(const String&);
String extractCharsetFromMediaType(const String&); 
void findCharsetInMediaType(const String& mediaType, unsigned int& charsetPos, unsigned int& charsetLen, unsigned int start = 0);
XSSProtectionDisposition parseXSSProtectionHeader(const String&);
String extractReasonPhraseFromHTTPStatusLine(const String&);

// -1 could be set to one of the return parameters to indicate the value is not specified.
bool parseRange(const String&, long long& rangeOffset, long long& rangeEnd, long long& rangeSuffixLength);

}

#endif
