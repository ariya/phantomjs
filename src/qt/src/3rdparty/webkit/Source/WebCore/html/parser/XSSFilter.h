/*
 * Copyright (C) 2011 Adam Barth. All Rights Reserved.
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

#ifndef XSSFilter_h
#define XSSFilter_h

#include "HTMLToken.h"
#include "HTTPParsers.h"
#include "SuffixTree.h"

namespace WebCore {

class HTMLDocumentParser;

class XSSFilter {
    WTF_MAKE_NONCOPYABLE(XSSFilter);
public:
    explicit XSSFilter(HTMLDocumentParser*);

    void filterToken(HTMLToken&);

private:
    enum State {
        Uninitialized,
        Initial,
        AfterScriptStartTag,
    };

    void init();

    bool filterTokenInitial(HTMLToken&);
    bool filterTokenAfterScriptStartTag(HTMLToken&);

    bool filterScriptToken(HTMLToken&);
    bool filterObjectToken(HTMLToken&);
    bool filterParamToken(HTMLToken&);
    bool filterEmbedToken(HTMLToken&);
    bool filterAppletToken(HTMLToken&);
    bool filterIframeToken(HTMLToken&);
    bool filterMetaToken(HTMLToken&);
    bool filterBaseToken(HTMLToken&);
    bool filterFormToken(HTMLToken&);

    bool eraseDangerousAttributesIfInjected(HTMLToken&);
    bool eraseAttributeIfInjected(HTMLToken&, const QualifiedName&, const String& replacementValue = String());

    String snippetForRange(const HTMLToken&, int start, int end);
    String snippetForAttribute(const HTMLToken&, const HTMLToken::Attribute&);

    bool isContainedInRequest(const String&);
    bool isSameOriginResource(const String& url);

    HTMLDocumentParser* m_parser;
    bool m_isEnabled;
    XSSProtectionDisposition m_xssProtection;

    String m_decodedURL;
    String m_decodedHTTPBody;
    OwnPtr<SuffixTree<ASCIICodebook> > m_decodedHTTPBodySuffixTree;

    State m_state;
    String m_cachedSnippet;
};

}

#endif
