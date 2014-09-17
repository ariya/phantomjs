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
#include "InspectorValues.h"
#include "PlatformString.h"

#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

class ParsedStyleSheet;

namespace WebCore {

class CSSRuleList;
class CSSStyleDeclaration;
class CSSStyleSheet;
class Document;
class Element;
class InspectorStyleSheet;
class Node;

#if ENABLE(INSPECTOR)

typedef String ErrorString;

class InspectorCSSId {
public:
    InspectorCSSId() { }

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

    PassRefPtr<InspectorValue> asInspectorValue() const
    {
        if (isEmpty())
            return InspectorValue::null();

        RefPtr<InspectorObject> result = InspectorObject::create();
        result->setString("styleSheetId", m_styleSheetId);
        result->setNumber("ordinal", m_ordinal);
        return result.release();
    }

private:
    String m_styleSheetId;
    unsigned m_ordinal;
};

struct InspectorStyleProperty {
    InspectorStyleProperty()
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
    PassRefPtr<InspectorObject> buildObjectForStyle() const;
    bool hasDisabledProperties() const { return !m_disabledProperties.isEmpty(); }
    bool setPropertyText(ErrorString*, unsigned index, const String& text, bool overwrite);
    bool toggleProperty(ErrorString*, unsigned index, bool disable);

private:
    InspectorStyle(const InspectorCSSId& styleId, PassRefPtr<CSSStyleDeclaration> style, InspectorStyleSheet* parentStyleSheet);

    static unsigned disabledIndexByOrdinal(unsigned ordinal, bool canUseSubsequent, Vector<InspectorStyleProperty>& allProperties);

    bool styleText(String* result) const;
    bool disableProperty(unsigned indexToDisable, Vector<InspectorStyleProperty>& allProperties);
    bool enableProperty(unsigned indexToEnable, Vector<InspectorStyleProperty>& allProperties);
    bool populateAllProperties(Vector<InspectorStyleProperty>* result) const;
    void populateObjectWithStyleProperties(InspectorObject* result) const;
    void shiftDisabledProperties(unsigned fromIndex, long offset);
    bool replacePropertyInStyleText(const InspectorStyleProperty& property, const String& newText);
    String shorthandValue(const String& shorthandProperty) const;
    String shorthandPriority(const String& shorthandProperty) const;
    Vector<String> longhandProperties(const String& shorthandProperty) const;

    InspectorCSSId m_styleId;
    RefPtr<CSSStyleDeclaration> m_style;
    InspectorStyleSheet* m_parentStyleSheet;
    Vector<InspectorStyleProperty> m_disabledProperties;
};

class InspectorStyleSheet : public RefCounted<InspectorStyleSheet> {
public:
    typedef HashMap<CSSStyleDeclaration*, RefPtr<InspectorStyle> > InspectorStyleMap;
    static PassRefPtr<InspectorStyleSheet> create(const String& id, PassRefPtr<CSSStyleSheet> pageStyleSheet, const String& origin, const String& documentURL);

    virtual ~InspectorStyleSheet();

    String id() const { return m_id; }
    String finalURL() const;
    CSSStyleSheet* pageStyleSheet() const { return m_pageStyleSheet.get(); }
    void reparseStyleSheet(const String&);
    bool setText(const String&);
    bool setRuleSelector(const InspectorCSSId&, const String& selector);
    CSSStyleRule* addRule(const String& selector);
    CSSStyleRule* ruleForId(const InspectorCSSId&) const;
    PassRefPtr<InspectorObject> buildObjectForStyleSheet();
    PassRefPtr<InspectorObject> buildObjectForStyleSheetInfo();
    PassRefPtr<InspectorObject> buildObjectForRule(CSSStyleRule*);
    PassRefPtr<InspectorObject> buildObjectForStyle(CSSStyleDeclaration*);
    bool setPropertyText(ErrorString*, const InspectorCSSId&, unsigned propertyIndex, const String& text, bool overwrite);
    bool toggleProperty(ErrorString*, const InspectorCSSId&, unsigned propertyIndex, bool disable);

    virtual bool text(String* result) const;
    virtual CSSStyleDeclaration* styleForId(const InspectorCSSId&) const;

protected:
    InspectorStyleSheet(const String& id, PassRefPtr<CSSStyleSheet> pageStyleSheet, const String& origin, const String& documentURL);

    bool canBind() const { return m_origin != "userAgent" && m_origin != "user"; }
    InspectorCSSId ruleOrStyleId(CSSStyleDeclaration* style) const;
    virtual Document* ownerDocument() const;
    virtual RefPtr<CSSRuleSourceData> ruleSourceDataFor(CSSStyleDeclaration* style) const;
    virtual unsigned ruleIndexByStyle(CSSStyleDeclaration*) const;
    virtual bool ensureParsedDataReady();
    virtual PassRefPtr<InspectorStyle> inspectorStyleForId(const InspectorCSSId&);
    virtual void rememberInspectorStyle(RefPtr<InspectorStyle> inspectorStyle);
    virtual void forgetInspectorStyle(CSSStyleDeclaration* style);

    // Also accessed by friend class InspectorStyle.
    virtual bool setStyleText(CSSStyleDeclaration*, const String&);

private:
    static void fixUnparsedPropertyRanges(CSSRuleSourceData* ruleData, const String& styleSheetText);
    static void collectFlatRules(PassRefPtr<CSSRuleList>, Vector<CSSStyleRule*>* result);
    bool ensureText() const;
    bool ensureSourceData();
    void ensureFlatRules() const;
    bool styleSheetTextWithChangedStyle(CSSStyleDeclaration*, const String& newStyleText, String* result);
    InspectorCSSId ruleId(CSSStyleRule* rule) const;
    InspectorCSSId styleId(CSSStyleDeclaration* style) const { return ruleOrStyleId(style); }
    void revalidateStyle(CSSStyleDeclaration*);
    bool originalStyleSheetText(String* result) const;
    bool resourceStyleSheetText(String* result) const;
    bool inlineStyleSheetText(String* result) const;
    PassRefPtr<InspectorArray> buildArrayForRuleList(CSSRuleList*);

    String m_id;
    RefPtr<CSSStyleSheet> m_pageStyleSheet;
    String m_origin;
    String m_documentURL;
    bool m_isRevalidating;
    ParsedStyleSheet* m_parsedStyleSheet;
    InspectorStyleMap m_inspectorStyles;
    mutable Vector<CSSStyleRule*> m_flatRules;

    friend class InspectorStyle;
};

class InspectorStyleSheetForInlineStyle : public InspectorStyleSheet {
public:
    static PassRefPtr<InspectorStyleSheetForInlineStyle> create(const String& id, PassRefPtr<Element> element, const String& origin);

    void didModifyElementAttribute();
    virtual bool text(String* result) const;
    virtual CSSStyleDeclaration* styleForId(const InspectorCSSId& id) const { ASSERT_UNUSED(id, !id.ordinal()); return inlineStyle(); }

protected:
    InspectorStyleSheetForInlineStyle(const String& id, PassRefPtr<Element> element, const String& origin);

    virtual Document* ownerDocument() const;
    virtual RefPtr<CSSRuleSourceData> ruleSourceDataFor(CSSStyleDeclaration* style) const { ASSERT_UNUSED(style, style == inlineStyle()); return m_ruleSourceData; }
    virtual unsigned ruleIndexByStyle(CSSStyleDeclaration*) const { return 0; }
    virtual bool ensureParsedDataReady();
    virtual PassRefPtr<InspectorStyle> inspectorStyleForId(const InspectorCSSId&);
    virtual void rememberInspectorStyle(RefPtr<InspectorStyle>) { }
    virtual void forgetInspectorStyle(CSSStyleDeclaration*) { }

    // Also accessed by friend class InspectorStyle.
    virtual bool setStyleText(CSSStyleDeclaration*, const String&);

private:
    CSSStyleDeclaration* inlineStyle() const;
    const String& elementStyleText() const;
    bool getStyleAttributeRanges(RefPtr<CSSStyleSourceData>* result);

    RefPtr<Element> m_element;
    RefPtr<CSSRuleSourceData> m_ruleSourceData;
    RefPtr<InspectorStyle> m_inspectorStyle;

    // Contains "style" attribute value and should always be up-to-date.
    String m_styleText;
};

#endif

} // namespace WebCore

#endif // !defined(InspectorStyleSheet_h)
