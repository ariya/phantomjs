/*
 * Copyright (C) 2011 Google, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
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

#ifndef ContentSecurityPolicy_h
#define ContentSecurityPolicy_h

#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CSPDirective;
class Document;
class KURL;

class ContentSecurityPolicy : public RefCounted<ContentSecurityPolicy> {
public:
    static PassRefPtr<ContentSecurityPolicy> create(Document* document)
    {
        return adoptRef(new ContentSecurityPolicy(document));
    }
    ~ContentSecurityPolicy();

    void didReceiveHeader(const String&);

    bool allowJavaScriptURLs() const;
    bool allowInlineEventHandlers() const;
    bool allowInlineScript() const;
    bool allowInlineStyle() const;
    bool allowEval() const;

    bool allowScriptFromSource(const KURL&) const;
    bool allowObjectFromSource(const KURL&) const;
    bool allowChildFrameFromSource(const KURL&) const;
    bool allowImageFromSource(const KURL&) const;
    bool allowStyleFromSource(const KURL&) const;
    bool allowFontFromSource(const KURL&) const;
    bool allowMediaFromSource(const KURL&) const;

private:
    explicit ContentSecurityPolicy(Document*);

    void parse(const String&);
    bool parseDirective(const UChar* begin, const UChar* end, String& name, String& value);
    void parseReportURI(const String&);
    void addDirective(const String& name, const String& value);

    PassOwnPtr<CSPDirective> createCSPDirective(const String& name, const String& value);

    CSPDirective* operativeDirective(CSPDirective*) const;
    void reportViolation(const String& directiveText, const String& consoleMessage) const;
    bool checkEval(CSPDirective*) const;

    bool checkInlineAndReportViolation(CSPDirective*, const String& consoleMessage) const;
    bool checkEvalAndReportViolation(CSPDirective*, const String& consoleMessage) const;
    bool checkSourceAndReportViolation(CSPDirective*, const KURL&, const String& type) const;

    bool m_havePolicy;
    Document* m_document;

    OwnPtr<CSPDirective> m_defaultSrc;
    OwnPtr<CSPDirective> m_scriptSrc;
    OwnPtr<CSPDirective> m_objectSrc;
    OwnPtr<CSPDirective> m_frameSrc;
    OwnPtr<CSPDirective> m_imgSrc;
    OwnPtr<CSPDirective> m_styleSrc;
    OwnPtr<CSPDirective> m_fontSrc;
    OwnPtr<CSPDirective> m_mediaSrc;

    Vector<KURL> m_reportURLs;
};

}

#endif
