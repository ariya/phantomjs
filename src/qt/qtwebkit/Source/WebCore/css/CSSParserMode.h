/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef CSSParserMode_h
#define CSSParserMode_h

#include "KURL.h"

namespace WebCore {

class Document;

enum CSSParserMode {
    CSSQuirksMode,
    CSSStrictMode,
    // SVG should always be in strict mode. For SVG attributes, the rules differ to strict sometimes.
    SVGAttributeMode
};

inline CSSParserMode strictToCSSParserMode(bool inStrictMode)
{
    return inStrictMode ? CSSStrictMode : CSSQuirksMode;
}

inline bool isStrictParserMode(CSSParserMode cssParserMode)
{
    return cssParserMode == CSSStrictMode || cssParserMode == SVGAttributeMode;
}

struct CSSParserContext {
    WTF_MAKE_FAST_ALLOCATED;
public:
    CSSParserContext(CSSParserMode, const KURL& baseURL = KURL());
    CSSParserContext(Document*, const KURL& baseURL = KURL(), const String& charset = emptyString());

    KURL baseURL;
    String charset;
    CSSParserMode mode;
    bool isHTMLDocument;
    bool isCSSCustomFilterEnabled;
    bool isCSSStickyPositionEnabled;
    bool isCSSRegionsEnabled;
    bool isCSSCompositingEnabled;
    bool isCSSGridLayoutEnabled;
#if ENABLE(CSS_VARIABLES)
    bool isCSSVariablesEnabled;
#endif
    bool needsSiteSpecificQuirks;
    bool enforcesCSSMIMETypeInNoQuirksMode;
    bool useLegacyBackgroundSizeShorthandBehavior;
};

bool operator==(const CSSParserContext&, const CSSParserContext&);
inline bool operator!=(const CSSParserContext& a, const CSSParserContext& b) { return !(a == b); }

const CSSParserContext& strictCSSParserContext();

};

#endif // CSSParserMode_h
