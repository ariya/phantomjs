/*
 * Copyright (C) 2006, 2008, 2009 Apple Inc. All rights reserved.
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
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef HTMLViewSourceDocument_h
#define HTMLViewSourceDocument_h

#include "HTMLDocument.h"

namespace WebCore {

class HTMLTableCellElement;
class HTMLTableSectionElement;
class HTMLToken;

class HTMLViewSourceDocument : public HTMLDocument {
public:
    static PassRefPtr<HTMLViewSourceDocument> create(Frame* frame, const KURL& url, const String& mimeType)
    {
        return adoptRef(new HTMLViewSourceDocument(frame, url, mimeType));
    }

    void addSource(const String&, HTMLToken&);

private:
    HTMLViewSourceDocument(Frame*, const KURL&, const String& mimeType);

    // Returns HTMLViewSourceParser or TextDocumentParser based on m_type.
    virtual PassRefPtr<DocumentParser> createParser();

    void processDoctypeToken(const String& source, HTMLToken&);
    void processTagToken(const String& source, HTMLToken&);
    void processCommentToken(const String& source, HTMLToken&);
    void processCharacterToken(const String& source, HTMLToken&);

    void createContainingTable();
    PassRefPtr<Element> addSpanWithClassName(const AtomicString&);
    void addLine(const AtomicString& className);
    void addText(const String& text, const AtomicString& className);
    int addRange(const String& source, int start, int end, const String& className, bool isLink = false, bool isAnchor = false);
    PassRefPtr<Element> addLink(const AtomicString& url, bool isAnchor);
    PassRefPtr<Element> addBase(const AtomicString& href);

    String m_type;
    RefPtr<Element> m_current;
    RefPtr<HTMLTableSectionElement> m_tbody;
    RefPtr<HTMLTableCellElement> m_td;
};

}

#endif // HTMLViewSourceDocument_h
