/*
 * Copyright (c) 2010 Google Inc. All rights reserved.
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

#ifndef CSSPropertySourceData_h
#define CSSPropertySourceData_h

#include <utility>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class StyleRuleBase;

struct SourceRange {
    SourceRange();
    SourceRange(unsigned start, unsigned end);
    unsigned length() const;

    unsigned start;
    unsigned end;
};

struct CSSPropertySourceData {
    CSSPropertySourceData(const String& name, const String& value, bool important, bool parsedOk, const SourceRange& range);
    CSSPropertySourceData(const CSSPropertySourceData& other);
    CSSPropertySourceData();

    String toString() const;
    unsigned hash() const;

    String name;
    String value;
    bool important;
    bool parsedOk;
    SourceRange range;
};

struct CSSStyleSourceData : public RefCounted<CSSStyleSourceData> {
    static PassRefPtr<CSSStyleSourceData> create()
    {
        return adoptRef(new CSSStyleSourceData());
    }

    Vector<CSSPropertySourceData> propertyData;
};

struct CSSRuleSourceData;
typedef Vector<RefPtr<CSSRuleSourceData> > RuleSourceDataList;
typedef Vector<SourceRange> SelectorRangeList;

struct CSSRuleSourceData : public RefCounted<CSSRuleSourceData> {
    enum Type {
        UNKNOWN_RULE,
        STYLE_RULE,
        CHARSET_RULE,
        IMPORT_RULE,
        MEDIA_RULE,
        FONT_FACE_RULE,
        PAGE_RULE,
        KEYFRAMES_RULE,
        REGION_RULE,
        HOST_RULE,
        VIEWPORT_RULE,
        SUPPORTS_RULE,
#if ENABLE(CSS_SHADERS)
        FILTER_RULE
#endif
    };

    static PassRefPtr<CSSRuleSourceData> create(Type type)
    {
        return adoptRef(new CSSRuleSourceData(type));
    }

    static PassRefPtr<CSSRuleSourceData> createUnknown()
    {
        return adoptRef(new CSSRuleSourceData(UNKNOWN_RULE));
    }

    CSSRuleSourceData(Type type)
        : type(type)
    {
        if (type == STYLE_RULE || type == FONT_FACE_RULE || type == PAGE_RULE)
            styleSourceData = CSSStyleSourceData::create();
    }

    Type type;

    // Range of the selector list in the enclosing source.
    SourceRange ruleHeaderRange;

    // Range of the rule body (e.g. style text for style rules) in the enclosing source.
    SourceRange ruleBodyRange;

    // Only for CSSStyleRules.
    SelectorRangeList selectorRanges;

    // Only for CSSStyleRules, CSSFontFaceRules, and CSSPageRules.
    RefPtr<CSSStyleSourceData> styleSourceData;

    // Only for CSSMediaRules.
    RuleSourceDataList childRules;
};

} // namespace WebCore

#endif // CSSPropertySourceData_h
