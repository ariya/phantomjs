/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef BlobURL_h
#define BlobURL_h

#include "KURL.h"

namespace WebCore {

class SecurityOrigin;

// Blob URLs are of the form
//     blob:%escaped_origin%/%UUID%
// For public urls, the origin of the host page is encoded in the URL value to
// allow easy lookup of the origin when security checks need to be performed.
// When loading blobs via ResourceHandle or when reading blobs via FileReader
// the loader conducts security checks that examine the origin of host page
// encoded in the public blob url. The origin baked into internal blob urls
// is a simple constant value, "blobinternal://", internal urls should not
// be used with ResourceHandle or FileReader.
class BlobURL {
public:
    static KURL createPublicURL(SecurityOrigin*);
    static KURL createInternalURL();
    static String getOrigin(const KURL&);
    static String getIdentifier(const KURL&);
    static const char* blobProtocol() { return kBlobProtocol; }

private:
    static KURL createBlobURL(const String& originString);
    static const char kBlobProtocol[];
    BlobURL() { }
};

}

#endif // BlobURL_h
