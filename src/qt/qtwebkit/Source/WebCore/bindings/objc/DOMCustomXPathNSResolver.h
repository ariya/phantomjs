/*
 * Copyright (C) 2007 Alexey Proskuryakov (ap@nypop.com)
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

#ifndef DOMCustomXPathNSResolver_h
#define DOMCustomXPathNSResolver_h

#include "XPathNSResolver.h"

#include "DOMXPathNSResolver.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

    class Frame;

    class DOMCustomXPathNSResolver : public XPathNSResolver {
    public:
        static PassRefPtr<DOMCustomXPathNSResolver> create(id <DOMXPathNSResolver> customResolver) { return adoptRef(new DOMCustomXPathNSResolver(customResolver)); }
        virtual ~DOMCustomXPathNSResolver();

        virtual String lookupNamespaceURI(const String& prefix);

    private:
        DOMCustomXPathNSResolver(id <DOMXPathNSResolver>);
        id <DOMXPathNSResolver> m_customResolver; // DOMCustomXPathNSResolvers are always temporary, thus no need to GC protect the object.
    };

} // namespace WebCore

#endif // DOMCustomXPathNSResolver_h
