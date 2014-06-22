/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
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

#ifndef TemplateContentDocumentFragment_h
#define TemplateContentDocumentFragment_h

#if ENABLE(TEMPLATE_ELEMENT)

#include "DocumentFragment.h"

namespace WebCore {

class TemplateContentDocumentFragment FINAL : public DocumentFragment {
public:
    static PassRefPtr<TemplateContentDocumentFragment> create(Document* document, const Element* host)
    {
        return adoptRef(new TemplateContentDocumentFragment(document, host));
    }

    const Element* host() const { return m_host; }

private:
    TemplateContentDocumentFragment(Document* document, const Element* host)
        : DocumentFragment(document, CreateDocumentFragment)
        , m_host(host)
    {
    }

    virtual bool isTemplateContent() const OVERRIDE { return true; }

    const Element* m_host;
};

} // namespace WebCore

#endif // ENABLE(TEMPLATE_ELEMENT)

#endif // TemplateContentDocumentFragment_h
