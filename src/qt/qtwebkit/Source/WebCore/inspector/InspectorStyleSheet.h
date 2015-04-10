/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorStyleSheet_h
#define InspectorStyleSheet_h

#include "CSSPropertySourceData.h"
#include "CSSStyleDeclaration.h"
#include "ExceptionCode.h"
#include "InspectorStyleTextEditor.h"
#include "InspectorTypeBuilder.h"
#include "InspectorValues.h"

#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

class ParsedStyleSheet;

namespace WebCore {

class CSSRuleList;
class CSSStyleDeclaration;
class CSSStyleRule;
class CSSStyleSheet;
class Document;
class Element;
class InspectorPageAgent;
class InspectorStyleSheet;
class Node;

#if ENABLE(INSPECTOR)

typedef String ErrorString;

class InspectorCSSId {
public:
    InspectorCSSId()
        : m_ordinal(0)
    {
    }

    explicit InspectorCSSId(RefPtr<InspectorObject> value)
    {
        if (!value->getString("styleSheetId", &m_styleSheetId))
            return;
        
        RefPtr<InspectorValue> ordinalValue = value->get("ordinal");
        if (!ordinalValue || !ordinalValue->asNumber(&m_ordinal))
            m_styleSheetId = "";
    }

    InspectorCSSId(const String& styleSheetId, unsigned ordinal)
        : m_styleSheetId(styleSheetId)
        , m_ordinal(ordinal)
    {
    }

    bool isEmpty() const { return m_styleSheetId.isEmpty(); }

    const String& styleSheetId() const { return m_styleSheetId; }
    unsigned ordinal() const { return m_ordinal; }

    // ID type is either TypeBuilder::CSS::CSSStyleId or TypeBuilder::CSS::CSSRuleId.
    template<typename ID>
    PassRefPtr<ID> asProtocolValue() const
    {
        if (isEmpty())
            return 0;

        RefPtr<ID> result = ID::create()
            .setStyleSheetId(m_styleSheetId)
            .setOrdinal(m_ordinal);
        return result.release();
    }

private:
    String m_styleSheetId;
    unsigned m_ordinal;
};

struct InspectorStyleProperty {
    InspectorStyleProperty()
        : hasSource(false)
        , disabled(false)
    {
    }

    InspectorStyleProperty(CSSPropertySourceData sourceData, bool hasSource, bool disabled)
        : sourceData(sourceData)
        , hasSource(hasSource)
        , disabled(disabled)
    {
    }

    void setRawTextFromStyleDeclaration(const String& styleDeclaration)
    {
        unsigned start = sourceData.range.start;
        unsigned end = sourceData.range.end;
        ASSERT(start < end);
        ASSERT(end <= styleDeclaration.length());
        rawText = styleDeclaration.substring(start, end - start);
    }

    bool hasRawText() const { return !rawText.isEmpty(); }

    CSSPropertySourceData sourceData;
    bool hasSource;
    bool disabled;
    String rawText;
};

class InspectorStyle : public RefCounted<InspectorStyle> {
public:
    static PassRefPtr<InspectorStyle> create(const InspectorCSSId& styleId, PassRefPtr<CSSStyleDeclaration> style, InspectorStyleSheet* parentStyleSheet);
    virtual ~InspectorStyle();

    CSSStyleDeclaration* cssStyle() const { return m_style.get(); }
    PassRefPtr<TypeBuilder::CSS::CSSStyle> buildObjectForStyle() const;
    PassRefPtr<TypeBuilder::Array<TypeBuilder::CSS::CSSComputedStyleProperty> > buildArrayForComputedStyle() const;
    bool hasDisabledProperties() const { return !m_disabledProperties.isEmpty(); }
    bool setPropertyText(unsigned index, const String& text, bool overwrite, String* oldText, ExceptionCode&);
    bool toggleProperty(unsigned index, bool disable, ExceptionCode&);

    bool getText(String* result) const;
    bool setText(const String&, ExceptionCode&);

private:
    InspectorStyle(const InspectorCSSId& styleId, PassRefPtr<CSSStyleDeclaration> style, InspectorStyleSheet* parentStyleSheet);

    // FIXME: Remove these aliases and update all the current call sites to use the new public methods.
    bool styleText(String* result) const { return getText(result); }
    bool applyStyleText(const String& text) { ExceptionCode ec = 0; return setText(text, ec); }

    bool populateAllProperties(Vector<InspectorStyleProperty>* result) const;
    PassRefPtr<TypeBuilder::CSS::CSSStyle> styleWithProperties() const;
    PassRefPtr<CSSRuleSourceData> extractSourceData() const;
    String shorthandValue(const String& shorthandProperty) const;
    String shorthandPriority(const String& shorthandProperty) const;
    Vector<String> longhandProperties(const String& shorthandProperty) const;
    NewLineAndWhitespace& newLineAndWhitespaceDelimiters() const;

    InspectorCSSId m_styleId;
    RefPtr<CSSStyleDeclaration> m_style;
    InspectorStyleSheet* m_parentStyleSheet;
    Vector<InspectorStyleProperty> m_disabledProperties;
    mutable std::pair<String, String> m_format;
    mutable bool m_formatAcquired;
};

class InspectorStyleSheet : public RefCounted<InspectorStyleSheet> {
public:
    class Listener {
    public:
        Listener() { }
        virtual ~Listener() { }
        virtual void styleSheetChanged(InspectorStyleSheet*) = 0;
    };

    typedef HashMap<CSSStyleDeclaration*, RefPtr<InspectorStyle> > InspectorStyleMap;
    static PassRefPtr<InspectorStyleSheet> create(InspectorPageAgent*, const String& id, PassRefPtr<CSSStyleSheet> pageStyleSheet, TypeBuilder::CSS::StyleSheetOrigin::Enum, const String& documentURL, Listener*);
    static String styleSheetURL(CSSStyleSheet* pageStyleSheet);

    virtual ~InspectorStyleSheet();

    String id() const { return m_id; }
    String finalURL() const;
    CSSStyleSheet* pageStyleSheet() const { return m_pageStyleSheet.get(); }
    void reparseStyleSheet(const String&);
    bool setText(const String&, ExceptionCode&);
    String ruleSelector(const InspectorCSSId&, ExceptionCode&);
    bool setRuleSelector(const InspectorCSSId&, const String& selector, ExceptionCode&);
    CSSStyleRule* addRule(const String& selector, ExceptionCode&);
    bool deleteRule(const InspectorCSSId&, ExceptionCode&);
    CSSStyleRule* ruleForId(const InspectorCSSId&) const;
    PassRefPtr<TypeBuilder::CSS::CSSStyleSheetBody> buildObjectForStyleSheet();
    PassRefPtr<TypeBuilder::CSS::CSSStyleSheetHeader> buildObjectForStyleSheetInfo();
    PassRefPtr<TypeBuilder::CSS::CSSRule> buildObjectForRule(CSSStyleRule*);
    PassRefPtr<TypeBuilder::CSS::CSSStyle> buildObjectForStyle(CSSStyleDeclaration*);
    bool setStyleText(const InspectorCSSId&, const String& text, String* oldText, ExceptionCode&);
    bool setPropertyText(const InspectorCSSId&, unsigned propertyIndex, const String& text, bool overwrite, String* oldPropertyText, ExceptionCode&);
    bool toggleProperty(const InspectorCSSId&, unsigned propertyIndex, bool disable, ExceptionCode&);

    virtual bool getText(String* result) const;
    virtual CSSStyleDeclaration* styleForId(const InspectorCSSId&) const;
    void fireStyleSheetChanged();

    InspectorCSSId ruleId(CSSStyleRule*) const;
    InspectorCSSId styleId(CSSStyleDeclaration* style) const { return ruleOrStyleId(style); }

protected:
    InspectorStyleSheet(InspectorPageAgent*, const String& id, PassRefPtr<CSSStyleSheet> pageStyleSheet, TypeBuilder::CSS::StyleSheetOrigin::Enum, const String& documentURL, Listener*);

    bool canBind() const { return m_origin != TypeBuilder::CSS::StyleSheetOrigin::User_agent && m_origin != TypeBuilder::CSS::StyleSheetOrigin::User; }
    InspectorCSSId ruleOrStyleId(CSSStyleDeclaration* style) const;
    virtual Document* ownerDocument() const;
    virtual RefPtr<CSSRuleSourceData> ruleSourceDataFor(CSSStyleDeclaration* style) const;
    virtual unsigned ruleIndexByStyle(CSSStyleDeclaration*) const;
    virtual bool ensureParsedDataReady();
    virtual PassRefPtr<InspectorStyle> inspectorStyleForId(const InspectorCSSId&);
    virtual void rememberInspectorStyle(RefPtr<InspectorStyle> inspectorStyle);
    virtual void forgetInspectorStyle(CSSStyleDeclaration* style);

    // Also accessed by friend class InspectorStyle.
    virtual bool setStyleText(CSSStyleDeclaration*, const String&, ExceptionCode&);
    virtual PassOwnPtr<Vector<size_t> > lineEndings() const;

private:
    typedef Vector<RefPtr<CSSStyleRule> > CSSStyleRuleVector;
    friend class InspectorStyle;

    static void collectFlatRules(PassRefPtr<CSSRuleList>, CSSStyleRuleVector* result);
    bool checkPageStyleSheet(ExceptionCode&) const;
    bool ensureText() const;
    bool ensureSourceData();
    void ensureFlatRules() const;
    bool styleSheetTextWithChangedStyle(CSSStyleDeclaration*, const String& newStyleText, String* result);
    void revalidateStyle(CSSStyleDeclaration*);
    bool originalStyleSheetText(String* result) const;
    bool resourceStyleSheetText(String* result) const;
    bool inlineStyleSheetText(String* result) const;
    PassRefPtr<TypeBuilder::Array<TypeBuilder::CSS::CSSRule> > buildArrayForRuleList(CSSRuleList*);
    PassRefPtr<TypeBuilder::CSS::SelectorList> buildObjectForSelectorList(CSSStyleRule*);

    InspectorPageAgent* m_pageAgent;
    String m_id;
    RefPtr<CSSStyleSheet> m_pageStyleSheet;
    TypeBuilder::CSS::StyleSheetOrigin::Enum m_origin;
    String m_documentURL;
    bool m_isRevalidating;
    ParsedStyleSheet* m_parsedStyleSheet;
    InspectorStyleMap m_inspectorStyles;
    mutable CSSStyleRuleVector m_flatRules;
    Listener* m_listener;
};

class InspectorStyleSheetForInlineStyle : public InspectorStyleSheet {
public:
    static PassRefPtr<InspectorStyleSheetForInlineStyle> create(InspectorPageAgent*, const String& id, PassRefPtr<Element>, TypeBuilder::CSS::StyleSheetOrigin::Enum, Listener*);

    void didModifyElementAttribute();
    virtual bool getText(String* result) const;
    virtual CSSStyleDeclaration* styleForId(const InspectorCSSId& id) const { ASSERT_UNUSED(id, !id.ordinal()); return inlineStyle(); }

protected:
    InspectorStyleSheetForInlineStyle(InspectorPageAgent*, const String& id, PassRefPtr<Element>, TypeBuilder::CSS::StyleSheetOrigin::Enum, Listener*);

    virtual Document* ownerDocument() const;
    virtual RefPtr<CSSRuleSourceData> ruleSourceDataFor(CSSStyleDeclaration* style) const { ASSERT_UNUSED(style, style == inlineStyle()); return m_ruleSourceData; }
    virtual unsigned ruleIndexByStyle(CSSStyleDeclaration*) const { return 0; }
    virtual bool ensureParsedDataReady();
    virtual PassRefPtr<InspectorStyle> inspectorStyleForId(const InspectorCSSId&);
    virtual void rememberInspectorStyle(RefPtr<InspectorStyle>) { }
    virtual void forgetInspectorStyle(CSSStyleDeclaration*) { }

    // Also accessed by friend class InspectorStyle.
    virtual bool setStyleText(CSSStyleDeclaration*, const String&, ExceptionCode&);
    virtual PassOwnPtr<Vector<size_t> > lineEndings() const;

private:
    CSSStyleDeclaration* inlineStyle() const;
    const String& elementStyleText() const;
    bool getStyleAttributeRanges(CSSRuleSourceData* result) const;

    RefPtr<Element> m_element;
    RefPtr<CSSRuleSourceData> m_ruleSourceData;
    RefPtr<InspectorStyle> m_inspectorStyle;

    // Contains "style" attribute value.
    mutable String m_styleText;
    mutable bool m_isStyleTextValid;
};

#endif

} // namespace WebCore

#endif // !defined(InspectorStyleSheet_h)
