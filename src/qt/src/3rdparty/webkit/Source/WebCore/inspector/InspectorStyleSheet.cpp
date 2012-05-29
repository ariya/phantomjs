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

#include "config.h"
#include "InspectorStyleSheet.h"

#if ENABLE(INSPECTOR)

#include "CSSImportRule.h"
#include "CSSMediaRule.h"
#include "CSSParser.h"
#include "CSSPropertySourceData.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "CSSStyleSelector.h"
#include "CSSStyleSheet.h"
#include "Document.h"
#include "Element.h"
#include "HTMLHeadElement.h"
#include "HTMLParserIdioms.h"
#include "InspectorCSSAgent.h"
#include "InspectorPageAgent.h"
#include "InspectorValues.h"
#include "Node.h"
#include "StyleSheetList.h"
#include "WebKitCSSKeyframesRule.h"

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

class ParsedStyleSheet {
public:
    typedef Vector<RefPtr<WebCore::CSSRuleSourceData> > SourceData;
    ParsedStyleSheet();

    WebCore::CSSStyleSheet* cssStyleSheet() const { return m_parserOutput; }
    const String& text() const { return m_text; }
    void setText(const String& text);
    bool hasText() const { return m_hasText; }
    SourceData* sourceData() const { return m_sourceData.get(); }
    void setSourceData(PassOwnPtr<SourceData> sourceData);
    bool hasSourceData() const { return m_sourceData; }
    RefPtr<WebCore::CSSRuleSourceData> ruleSourceDataAt(unsigned index) const;

private:

    // StyleSheet constructed while parsing m_text.
    WebCore::CSSStyleSheet* m_parserOutput;
    String m_text;
    bool m_hasText;
    OwnPtr<SourceData> m_sourceData;
};

ParsedStyleSheet::ParsedStyleSheet()
    : m_parserOutput(0)
    , m_hasText(false)
{
}

void ParsedStyleSheet::setText(const String& text)
{
    m_hasText = true;
    m_text = text;
    setSourceData(nullptr);
}

void ParsedStyleSheet::setSourceData(PassOwnPtr<SourceData> sourceData)
{
    m_sourceData = sourceData;
}

RefPtr<WebCore::CSSRuleSourceData> ParsedStyleSheet::ruleSourceDataAt(unsigned index) const
{
    if (!hasSourceData() || index >= m_sourceData->size())
        return 0;

    return m_sourceData->at(index);
}

namespace WebCore {

static PassRefPtr<InspectorObject> buildSourceRangeObject(const SourceRange& range)
{
    RefPtr<InspectorObject> result = InspectorObject::create();
    result->setNumber("start", range.start);
    result->setNumber("end", range.end);
    return result.release();
}

static PassRefPtr<CSSRuleList> asCSSRuleList(StyleBase* styleBase)
{
    if (!styleBase)
        return 0;

    if (styleBase->isCSSStyleSheet())
        return CSSRuleList::create(static_cast<CSSStyleSheet*>(styleBase), true);
    if (styleBase->isRule()) {
        unsigned ruleType = static_cast<CSSRule*>(styleBase)->type();
        RefPtr<CSSRuleList> result = 0;

        switch (ruleType) {
        case CSSRule::MEDIA_RULE:
            result = static_cast<CSSMediaRule*>(styleBase)->cssRules();
            break;
        case CSSRule::WEBKIT_KEYFRAMES_RULE:
            result = static_cast<WebKitCSSKeyframesRule*>(styleBase)->cssRules();
            break;
        case CSSRule::IMPORT_RULE:
        case CSSRule::PAGE_RULE:
        default:
            return 0;
        }

        return result.release();
    }
    return 0;
}

PassRefPtr<InspectorStyle> InspectorStyle::create(const InspectorCSSId& styleId, PassRefPtr<CSSStyleDeclaration> style, InspectorStyleSheet* parentStyleSheet)
{
    return adoptRef(new InspectorStyle(styleId, style, parentStyleSheet));
}

InspectorStyle::InspectorStyle(const InspectorCSSId& styleId, PassRefPtr<CSSStyleDeclaration> style, InspectorStyleSheet* parentStyleSheet)
    : m_styleId(styleId)
    , m_style(style)
    , m_parentStyleSheet(parentStyleSheet)
{
    ASSERT(m_style);
}

InspectorStyle::~InspectorStyle()
{
}

PassRefPtr<InspectorObject> InspectorStyle::buildObjectForStyle() const
{
    RefPtr<InspectorObject> result = InspectorObject::create();
    if (!m_styleId.isEmpty())
        result->setValue("styleId", m_styleId.asInspectorValue());

    result->setString("width", m_style->getPropertyValue("width"));
    result->setString("height", m_style->getPropertyValue("height"));

    RefPtr<CSSRuleSourceData> sourceData = m_parentStyleSheet ? m_parentStyleSheet->ruleSourceDataFor(m_style.get()) : 0;
    if (sourceData)
        result->setObject("range", buildSourceRangeObject(sourceData->styleSourceData->styleBodyRange));

    populateObjectWithStyleProperties(result.get());

    return result.release();
}

// This method does the following preprocessing of |propertyText| with |overwrite| == false and |index| past the last active property:
// - If the last property (if present) has no subsequent whitespace in the style declaration, a space is prepended to |propertyText|.
// - If the last property (if present) has no closing ";", the ";" is prepended to the current |propertyText| value.
//
// The propertyText (if not empty) is checked to be a valid style declaration (containing at least one property). If not,
// the method returns false (denoting an error).
bool InspectorStyle::setPropertyText(ErrorString* errorString, unsigned index, const String& propertyText, bool overwrite)
{
    ASSERT(m_parentStyleSheet);
    if (!m_parentStyleSheet->ensureParsedDataReady()) {
        *errorString = "Internal error: no stylesheet parsed data available";
        return false;
    }

    Vector<InspectorStyleProperty> allProperties;
    populateAllProperties(&allProperties);

    unsigned propertyStart = 0; // Need to initialize to make the compiler happy.
    long propertyLengthDelta;

    if (propertyText.stripWhiteSpace().length()) {
        RefPtr<CSSMutableStyleDeclaration> tempMutableStyle = CSSMutableStyleDeclaration::create();
        CSSParser p;
        RefPtr<CSSStyleSourceData> sourceData = CSSStyleSourceData::create();
        p.parseDeclaration(tempMutableStyle.get(), propertyText + " -webkit-boguz-propertee: none", &sourceData);
        Vector<CSSPropertySourceData>& propertyData = sourceData->propertyData;
        unsigned propertyCount = propertyData.size();

        // At least one property + the bogus property added just above should be present.
        if (propertyCount < 2) {
            *errorString = "Invalid property value";
            return false;
        }

        // Check for a proper propertyText termination (the parser could at least restore to the PROPERTY_NAME state).
        if (propertyData.at(propertyCount - 1).name != "-webkit-boguz-propertee") {
            *errorString = "Invalid property value";
            return false;
        }
    }

    if (overwrite) {
        ASSERT(index < allProperties.size());
        InspectorStyleProperty& property = allProperties.at(index);
        propertyStart = property.sourceData.range.start;
        unsigned propertyEnd = property.sourceData.range.end;
        unsigned oldLength = propertyEnd - propertyStart;
        unsigned newLength = propertyText.length();
        propertyLengthDelta = newLength - oldLength;

        if (!property.disabled) {
            bool success = replacePropertyInStyleText(property, propertyText);
            if (!success) {
                *errorString = "Internal error: could not replace property value";
                return false;
            }
        } else {
            unsigned textLength = propertyText.length();
            unsigned disabledIndex = disabledIndexByOrdinal(index, false, allProperties);
            if (!textLength) {
                // Delete disabled property.
                m_disabledProperties.remove(disabledIndex);
            } else {
                // Patch disabled property text.
                m_disabledProperties.at(disabledIndex).rawText = propertyText;
            }

            // We should not shift subsequent disabled properties when altering a disabled property.
            return true;
        }
    } else {
        // Insert at index.
        RefPtr<CSSRuleSourceData> sourceData = m_parentStyleSheet->ruleSourceDataFor(m_style.get());
        if (!sourceData) {
            *errorString = "Internal error: no CSS rule source found";
            return false;
        }
        String text;
        bool success = styleText(&text);
        if (!success) {
            *errorString = "Internal error: could not fetch style text";
            return false;
        }
        propertyLengthDelta = propertyText.length();

        bool insertLast = true;
        if (index < allProperties.size()) {
            InspectorStyleProperty& property = allProperties.at(index);
            if (property.hasSource) {
                propertyStart = property.sourceData.range.start;
                // If inserting before a disabled property, it should be shifted, too.
                insertLast = false;
            }
        }

        String textToSet = propertyText;
        if (insertLast) {
            propertyStart = sourceData->styleSourceData->styleBodyRange.end - sourceData->styleSourceData->styleBodyRange.start;
            if (propertyStart && propertyText.length()) {
                const UChar* characters = text.characters();

                unsigned curPos = propertyStart - 1; // The last position of style declaration, since propertyStart points past one.
                while (curPos && isHTMLSpace(characters[curPos]))
                    --curPos;
                if (curPos && characters[curPos] != ';') {
                    // Prepend a ";" to the property text if appending to a style declaration where
                    // the last property has no trailing ";".
                    textToSet.insert("; ", 0);
                } else if (!isHTMLSpace(characters[propertyStart - 1])) {
                    // Prepend a " " if the last declaration character is not an HTML space.
                    textToSet.insert(" ", 0);
                }
            }
        }

        text.insert(textToSet, propertyStart);
        m_parentStyleSheet->setStyleText(m_style.get(), text);
    }

    // Recompute subsequent disabled property ranges if acting on a non-disabled property.
    shiftDisabledProperties(disabledIndexByOrdinal(index, true, allProperties), propertyLengthDelta);

    return true;
}

bool InspectorStyle::toggleProperty(ErrorString* errorString, unsigned index, bool disable)
{
    ASSERT(m_parentStyleSheet);
    if (!m_parentStyleSheet->ensureParsedDataReady()) {
        *errorString = "Can toggle only source-based properties";
        return false;
    }
    RefPtr<CSSRuleSourceData> sourceData = m_parentStyleSheet->ruleSourceDataFor(m_style.get());
    if (!sourceData) {
        *errorString = "Internal error: No source data for the style found";
        return false;
    }

    Vector<InspectorStyleProperty> allProperties;
    populateAllProperties(&allProperties);
    if (index >= allProperties.size()) {
        *errorString = "Property index is outside of property range";
        return false;
    }

    InspectorStyleProperty& property = allProperties.at(index);
    if (property.disabled == disable)
        return true; // Idempotent operation.

    bool success;
    if (!disable)
        success = enableProperty(index, allProperties);
    else
        success = disableProperty(index, allProperties);

    return success;
}

// static
unsigned InspectorStyle::disabledIndexByOrdinal(unsigned ordinal, bool canUseSubsequent, Vector<InspectorStyleProperty>& allProperties)
{
    unsigned disabledIndex = 0;
    for (unsigned i = 0, size = allProperties.size(); i < size; ++i) {
        InspectorStyleProperty& property = allProperties.at(i);
        if (property.disabled) {
            if (i == ordinal || (canUseSubsequent && i > ordinal))
                return disabledIndex;
            ++disabledIndex;
        }
    }

    return UINT_MAX;
}

bool InspectorStyle::styleText(String* result) const
{
    // Precondition: m_parentStyleSheet->ensureParsedDataReady() has been called successfully.
    RefPtr<CSSRuleSourceData> sourceData = m_parentStyleSheet->ruleSourceDataFor(m_style.get());
    if (!sourceData)
        return false;

    String styleSheetText;
    bool success = m_parentStyleSheet->text(&styleSheetText);
    if (!success)
        return false;

    SourceRange& bodyRange = sourceData->styleSourceData->styleBodyRange;
    *result = styleSheetText.substring(bodyRange.start, bodyRange.end - bodyRange.start);
    return true;
}

bool InspectorStyle::disableProperty(unsigned indexToDisable, Vector<InspectorStyleProperty>& allProperties)
{
    // Precondition: |indexToEnable| points to an enabled property.
    const InspectorStyleProperty& property = allProperties.at(indexToDisable);
    unsigned propertyStart = property.sourceData.range.start;
    InspectorStyleProperty disabledProperty(property);
    String oldStyleText;
    bool success = styleText(&oldStyleText);
    if (!success)
        return false;
    disabledProperty.setRawTextFromStyleDeclaration(oldStyleText);
    disabledProperty.disabled = true;
    disabledProperty.sourceData.range.end = propertyStart;
    // This may have to be negated below.
    long propertyLength = property.sourceData.range.end - propertyStart;
    success = replacePropertyInStyleText(property, "");
    if (!success)
        return false;

    // Add disabled property at correct position.
    unsigned insertionIndex = disabledIndexByOrdinal(indexToDisable, true, allProperties);
    if (insertionIndex == UINT_MAX)
        m_disabledProperties.append(disabledProperty);
    else {
        m_disabledProperties.insert(insertionIndex, disabledProperty);
        shiftDisabledProperties(insertionIndex + 1, -propertyLength); // Property removed from text - shift these back.
    }
    return true;
}

bool InspectorStyle::enableProperty(unsigned indexToEnable, Vector<InspectorStyleProperty>& allProperties)
{
    // Precondition: |indexToEnable| points to a disabled property.
    unsigned disabledIndex = disabledIndexByOrdinal(indexToEnable, false, allProperties);
    if (disabledIndex == UINT_MAX)
        return false;

    InspectorStyleProperty disabledProperty = m_disabledProperties.at(disabledIndex);
    m_disabledProperties.remove(disabledIndex);
    bool success = replacePropertyInStyleText(disabledProperty, disabledProperty.rawText);
    if (success)
        shiftDisabledProperties(disabledIndex, disabledProperty.rawText.length());
    return success;
}

bool InspectorStyle::populateAllProperties(Vector<InspectorStyleProperty>* result) const
{
    HashSet<String> foundShorthands;
    HashSet<String> sourcePropertyNames;
    unsigned disabledIndex = 0;
    unsigned disabledLength = m_disabledProperties.size();
    InspectorStyleProperty disabledProperty;
    if (disabledIndex < disabledLength)
        disabledProperty = m_disabledProperties.at(disabledIndex);

    RefPtr<CSSRuleSourceData> sourceData = (m_parentStyleSheet && m_parentStyleSheet->ensureParsedDataReady()) ? m_parentStyleSheet->ruleSourceDataFor(m_style.get()) : 0;
    Vector<CSSPropertySourceData>* sourcePropertyData = sourceData ? &(sourceData->styleSourceData->propertyData) : 0;
    if (sourcePropertyData) {
        String styleDeclaration;
        bool isStyleTextKnown = styleText(&styleDeclaration);
        ASSERT_UNUSED(isStyleTextKnown, isStyleTextKnown);
        for (Vector<CSSPropertySourceData>::const_iterator it = sourcePropertyData->begin(); it != sourcePropertyData->end(); ++it) {
            while (disabledIndex < disabledLength && disabledProperty.sourceData.range.start <= it->range.start) {
                result->append(disabledProperty);
                if (++disabledIndex < disabledLength)
                    disabledProperty = m_disabledProperties.at(disabledIndex);
            }
            InspectorStyleProperty p(*it, true, false);
            p.setRawTextFromStyleDeclaration(styleDeclaration);
            result->append(p);
            sourcePropertyNames.add(it->name.lower());
        }
    }

    while (disabledIndex < disabledLength) {
        disabledProperty = m_disabledProperties.at(disabledIndex++);
        result->append(disabledProperty);
    }

    for (int i = 0, size = m_style->length(); i < size; ++i) {
        String name = m_style->item(i);
        if (sourcePropertyNames.contains(name.lower()))
            continue;

        sourcePropertyNames.add(name.lower());
        result->append(InspectorStyleProperty(CSSPropertySourceData(name, m_style->getPropertyValue(name), !m_style->getPropertyPriority(name).isEmpty(), true, SourceRange()), false, false));
    }

    return true;
}

void InspectorStyle::populateObjectWithStyleProperties(InspectorObject* result) const
{
    Vector<InspectorStyleProperty> properties;
    populateAllProperties(&properties);

    RefPtr<InspectorArray> propertiesObject = InspectorArray::create();
    RefPtr<InspectorArray> shorthandEntries = InspectorArray::create();
    HashMap<String, RefPtr<InspectorObject> > propertyNameToPreviousActiveProperty;
    HashSet<String> foundShorthands;

    for (Vector<InspectorStyleProperty>::iterator it = properties.begin(), itEnd = properties.end(); it != itEnd; ++it) {
        const CSSPropertySourceData& propertyEntry = it->sourceData;
        const String& name = propertyEntry.name;

        RefPtr<InspectorObject> property = InspectorObject::create();
        propertiesObject->pushObject(property);
        String status = it->disabled ? "disabled" : "active";

        // Default "parsedOk" == true.
        if (!propertyEntry.parsedOk)
            property->setBoolean("parsedOk", false);
        if (it->hasRawText())
            property->setString("text", it->rawText);
        property->setString("name", name);
        property->setString("value", propertyEntry.value);

        // Default "priority" == "".
        if (propertyEntry.important)
            property->setString("priority", "important");
        if (!it->disabled) {
            if (it->hasSource) {
                property->setBoolean("implicit", false);
                property->setObject("range", buildSourceRangeObject(propertyEntry.range));

                // Parsed property overrides any property with the same name. Non-parsed property overrides
                // previous non-parsed property with the same name (if any).
                bool shouldInactivate = false;
                HashMap<String, RefPtr<InspectorObject> >::iterator activeIt = propertyNameToPreviousActiveProperty.find(name);
                if (activeIt != propertyNameToPreviousActiveProperty.end()) {
                    if (propertyEntry.parsedOk)
                        shouldInactivate = true;
                    else {
                        bool previousParsedOk;
                        bool success = activeIt->second->getBoolean("parsedOk", &previousParsedOk);
                        if (success && !previousParsedOk)
                            shouldInactivate = true;
                    }
                } else
                    propertyNameToPreviousActiveProperty.set(name, property);

                if (shouldInactivate) {
                    activeIt->second->setString("status", "inactive");
                    activeIt->second->remove("shorthandName");
                    propertyNameToPreviousActiveProperty.set(name, property);
                }
            } else {
                bool implicit = m_style->isPropertyImplicit(name);
                // Default "implicit" == false.
                if (implicit)
                    property->setBoolean("implicit", true);
                status = "";
            }
        }

        // Default "status" == "style".
        if (!status.isEmpty())
            property->setString("status", status);

        if (propertyEntry.parsedOk) {
            // Both for style-originated and parsed source properties.
            String shorthand = m_style->getPropertyShorthand(name);
            if (!shorthand.isEmpty()) {
                // Default "shorthandName" == "".
                property->setString("shorthandName", shorthand);
                if (!foundShorthands.contains(shorthand)) {
                    foundShorthands.add(shorthand);
                    RefPtr<InspectorObject> shorthandEntry = InspectorObject::create();
                    shorthandEntry->setString("name", shorthand);
                    shorthandEntry->setString("value", shorthandValue(shorthand));
                    shorthandEntries->pushObject(shorthandEntry.release());
                }
            }
        }
        // else shorthandName is not set
    }

    result->setArray("cssProperties", propertiesObject);
    result->setArray("shorthandEntries", shorthandEntries);
}


void InspectorStyle::shiftDisabledProperties(unsigned fromIndex, long delta)
{
    for (unsigned i = fromIndex, size = m_disabledProperties.size(); i < size; ++i) {
        SourceRange& range = m_disabledProperties.at(i).sourceData.range;
        range.start += delta;
        range.end += delta;
    }
}

bool InspectorStyle::replacePropertyInStyleText(const InspectorStyleProperty& property, const String& newText)
{
    // Precondition: m_parentStyleSheet->ensureParsedDataReady() has been called successfully.
    String text;
    bool success = styleText(&text);
    if (!success)
        return false;
    const SourceRange& range = property.sourceData.range;
    text.replace(range.start, range.end - range.start, newText);
    success = m_parentStyleSheet->setStyleText(m_style.get(), text);
    return success;
}

String InspectorStyle::shorthandValue(const String& shorthandProperty) const
{
    String value = m_style->getPropertyValue(shorthandProperty);
    if (value.isEmpty()) {
        for (unsigned i = 0; i < m_style->length(); ++i) {
            String individualProperty = m_style->item(i);
            if (m_style->getPropertyShorthand(individualProperty) != shorthandProperty)
                continue;
            if (m_style->isPropertyImplicit(individualProperty))
                continue;
            String individualValue = m_style->getPropertyValue(individualProperty);
            if (individualValue == "initial")
                continue;
            if (value.length())
                value.append(" ");
            value.append(individualValue);
        }
    }
    return value;
}

String InspectorStyle::shorthandPriority(const String& shorthandProperty) const
{
    String priority = m_style->getPropertyPriority(shorthandProperty);
    if (priority.isEmpty()) {
        for (unsigned i = 0; i < m_style->length(); ++i) {
            String individualProperty = m_style->item(i);
            if (m_style->getPropertyShorthand(individualProperty) != shorthandProperty)
                continue;
            priority = m_style->getPropertyPriority(individualProperty);
            break;
        }
    }
    return priority;
}

Vector<String> InspectorStyle::longhandProperties(const String& shorthandProperty) const
{
    Vector<String> properties;
    HashSet<String> foundProperties;
    for (unsigned i = 0; i < m_style->length(); ++i) {
        String individualProperty = m_style->item(i);
        if (foundProperties.contains(individualProperty) || m_style->getPropertyShorthand(individualProperty) != shorthandProperty)
            continue;

        foundProperties.add(individualProperty);
        properties.append(individualProperty);
    }
    return properties;
}

PassRefPtr<InspectorStyleSheet> InspectorStyleSheet::create(const String& id, PassRefPtr<CSSStyleSheet> pageStyleSheet, const String& origin, const String& documentURL)
{
    return adoptRef(new InspectorStyleSheet(id, pageStyleSheet, origin, documentURL));
}

InspectorStyleSheet::InspectorStyleSheet(const String& id, PassRefPtr<CSSStyleSheet> pageStyleSheet, const String& origin, const String& documentURL)
    : m_id(id)
    , m_pageStyleSheet(pageStyleSheet)
    , m_origin(origin)
    , m_documentURL(documentURL)
    , m_isRevalidating(false)
{
    m_parsedStyleSheet = new ParsedStyleSheet();
}

InspectorStyleSheet::~InspectorStyleSheet()
{
    delete m_parsedStyleSheet;
}

String InspectorStyleSheet::finalURL() const
{
    if (m_pageStyleSheet && !m_pageStyleSheet->finalURL().isEmpty())
        return m_pageStyleSheet->finalURL().string();
    return m_documentURL;
}

void InspectorStyleSheet::reparseStyleSheet(const String& text)
{
    for (unsigned i = 0, size = m_pageStyleSheet->length(); i < size; ++i)
        m_pageStyleSheet->remove(0);
    m_pageStyleSheet->parseString(text, m_pageStyleSheet->useStrictParsing());
    m_pageStyleSheet->styleSheetChanged();
    m_inspectorStyles.clear();
}

bool InspectorStyleSheet::setText(const String& text)
{
    if (!m_parsedStyleSheet)
        return false;

    m_parsedStyleSheet->setText(text);
    m_flatRules.clear();

    return true;
}

bool InspectorStyleSheet::setRuleSelector(const InspectorCSSId& id, const String& selector)
{
    CSSStyleRule* rule = ruleForId(id);
    if (!rule)
        return false;
    CSSStyleSheet* styleSheet = InspectorCSSAgent::parentStyleSheet(rule);
    if (!styleSheet || !ensureParsedDataReady())
        return false;

    rule->setSelectorText(selector);
    RefPtr<CSSRuleSourceData> sourceData = ruleSourceDataFor(rule->style());
    if (!sourceData)
        return false;

    String sheetText = m_parsedStyleSheet->text();
    sheetText.replace(sourceData->selectorListRange.start, sourceData->selectorListRange.end - sourceData->selectorListRange.start, selector);
    m_parsedStyleSheet->setText(sheetText);
    return true;
}

CSSStyleRule* InspectorStyleSheet::addRule(const String& selector)
{
    String styleSheetText;
    bool success = text(&styleSheetText);
    if (!success)
        return 0;

    ExceptionCode ec = 0;
    m_pageStyleSheet->addRule(selector, "", ec);
    if (ec)
        return 0;
    RefPtr<CSSRuleList> rules = m_pageStyleSheet->cssRules();
    ASSERT(rules->length());
    CSSStyleRule* rule = InspectorCSSAgent::asCSSStyleRule(rules->item(rules->length() - 1));
    ASSERT(rule);

    if (styleSheetText.length())
        styleSheetText += "\n";

    styleSheetText += selector;
    styleSheetText += " {}";
    // Using setText() as this operation changes the style sheet rule set.
    setText(styleSheetText);

    return rule;
}

CSSStyleRule* InspectorStyleSheet::ruleForId(const InspectorCSSId& id) const
{
    if (!m_pageStyleSheet)
        return 0;

    ASSERT(!id.isEmpty());
    ensureFlatRules();
    return id.ordinal() >= m_flatRules.size() ? 0 : m_flatRules.at(id.ordinal());

}

PassRefPtr<InspectorObject> InspectorStyleSheet::buildObjectForStyleSheet()
{
    CSSStyleSheet* styleSheet = pageStyleSheet();
    if (!styleSheet)
        return 0;

    RefPtr<InspectorObject> result = InspectorObject::create();
    result->setString("styleSheetId", id());
    RefPtr<CSSRuleList> cssRuleList = CSSRuleList::create(styleSheet, true);
    RefPtr<InspectorArray> cssRules = buildArrayForRuleList(cssRuleList.get());
    result->setArray("rules", cssRules.release());

    String styleSheetText;
    bool success = text(&styleSheetText);
    if (success)
        result->setString("text", styleSheetText);

    return result.release();
}

PassRefPtr<InspectorObject> InspectorStyleSheet::buildObjectForStyleSheetInfo()
{
    CSSStyleSheet* styleSheet = pageStyleSheet();
    if (!styleSheet)
        return 0;

    RefPtr<InspectorObject> result = InspectorObject::create();
    result->setString("styleSheetId", id());
    result->setBoolean("disabled", styleSheet->disabled());
    result->setString("sourceURL", finalURL());
    result->setString("title", styleSheet->title());
    return result.release();
}

PassRefPtr<InspectorObject> InspectorStyleSheet::buildObjectForRule(CSSStyleRule* rule)
{
    CSSStyleSheet* styleSheet = pageStyleSheet();
    if (!styleSheet)
        return 0;

    RefPtr<InspectorObject> result = InspectorObject::create();
    result->setString("selectorText", rule->selectorText());
    // "sourceURL" is present only for regular rules, otherwise "origin" should be used in the frontend.
    if (!m_origin.length())
        result->setString("sourceURL", finalURL());
    result->setNumber("sourceLine", rule->sourceLine());
    result->setString("origin", m_origin);

    result->setObject("style", buildObjectForStyle(rule->style()));
    if (canBind()) {
        InspectorCSSId id(ruleId(rule));
        if (!id.isEmpty())
            result->setValue("ruleId", id.asInspectorValue());
    }

    RefPtr<CSSRuleSourceData> sourceData;
    if (ensureParsedDataReady())
        sourceData = ruleSourceDataFor(rule->style());
    if (sourceData) {
        RefPtr<InspectorObject> selectorRange = InspectorObject::create();
        selectorRange->setNumber("start", sourceData->selectorListRange.start);
        selectorRange->setNumber("end", sourceData->selectorListRange.end);
        result->setObject("selectorRange", selectorRange.release());
    }

    return result.release();
}

PassRefPtr<InspectorObject> InspectorStyleSheet::buildObjectForStyle(CSSStyleDeclaration* style)
{
    RefPtr<CSSRuleSourceData> sourceData;
    if (ensureParsedDataReady())
        sourceData = ruleSourceDataFor(style);

    InspectorCSSId id = ruleOrStyleId(style);
    if (id.isEmpty()) {
        RefPtr<InspectorObject> bogusStyle = InspectorObject::create();
        bogusStyle->setArray("cssProperties", InspectorArray::create());
        bogusStyle->setObject("shorthandValues", InspectorObject::create());
        bogusStyle->setObject("properties", InspectorObject::create());
        return bogusStyle.release();
    }
    RefPtr<InspectorStyle> inspectorStyle = inspectorStyleForId(id);
    RefPtr<InspectorObject> result = inspectorStyle->buildObjectForStyle();

    // Style text cannot be retrieved without stylesheet, so set cssText here.
    if (sourceData) {
        String sheetText;
        bool success = text(&sheetText);
        if (success) {
            const SourceRange& bodyRange = sourceData->styleSourceData->styleBodyRange;
            result->setString("cssText", sheetText.substring(bodyRange.start, bodyRange.end - bodyRange.start));
        }
    }

    return result.release();
}

bool InspectorStyleSheet::setPropertyText(ErrorString* errorString, const InspectorCSSId& id, unsigned propertyIndex, const String& text, bool overwrite)
{
    RefPtr<InspectorStyle> inspectorStyle = inspectorStyleForId(id);
    if (!inspectorStyle) {
        *errorString = "No style found for given id";
        return false;
    }

    return inspectorStyle->setPropertyText(errorString, propertyIndex, text, overwrite);
}

bool InspectorStyleSheet::toggleProperty(ErrorString* errorString, const InspectorCSSId& id, unsigned propertyIndex, bool disable)
{
    RefPtr<InspectorStyle> inspectorStyle = inspectorStyleForId(id);
    if (!inspectorStyle) {
        *errorString = "No style found for given id";
        return false;
    }

    bool success = inspectorStyle->toggleProperty(errorString, propertyIndex, disable);
    if (success) {
        if (disable)
            rememberInspectorStyle(inspectorStyle);
        else if (!inspectorStyle->hasDisabledProperties())
            forgetInspectorStyle(inspectorStyle->cssStyle());
    }
    return success;
}

bool InspectorStyleSheet::text(String* result) const
{
    if (!ensureText())
        return false;
    *result = m_parsedStyleSheet->text();
    return true;
}

CSSStyleDeclaration* InspectorStyleSheet::styleForId(const InspectorCSSId& id) const
{
    CSSStyleRule* rule = ruleForId(id);
    if (!rule)
        return 0;

    return rule->style();
}

PassRefPtr<InspectorStyle> InspectorStyleSheet::inspectorStyleForId(const InspectorCSSId& id)
{
    CSSStyleDeclaration* style = styleForId(id);
    if (!style)
        return 0;

    InspectorStyleMap::iterator it = m_inspectorStyles.find(style);
    if (it == m_inspectorStyles.end()) {
        RefPtr<InspectorStyle> inspectorStyle = InspectorStyle::create(id, style, this);
        return inspectorStyle.release();
    }
    return it->second;
}

void InspectorStyleSheet::rememberInspectorStyle(RefPtr<InspectorStyle> inspectorStyle)
{
    m_inspectorStyles.set(inspectorStyle->cssStyle(), inspectorStyle);
}

void InspectorStyleSheet::forgetInspectorStyle(CSSStyleDeclaration* style)
{
    m_inspectorStyles.remove(style);
}

InspectorCSSId InspectorStyleSheet::ruleOrStyleId(CSSStyleDeclaration* style) const
{
    unsigned index = ruleIndexByStyle(style);
    if (index != UINT_MAX)
        return InspectorCSSId(id(), index);
    return InspectorCSSId();
}

Document* InspectorStyleSheet::ownerDocument() const
{
    return m_pageStyleSheet->document();
}

RefPtr<CSSRuleSourceData> InspectorStyleSheet::ruleSourceDataFor(CSSStyleDeclaration* style) const
{
    return m_parsedStyleSheet->ruleSourceDataAt(ruleIndexByStyle(style));
}

unsigned InspectorStyleSheet::ruleIndexByStyle(CSSStyleDeclaration* pageStyle) const
{
    ensureFlatRules();
    unsigned index = 0;
    for (unsigned i = 0, size = m_flatRules.size(); i < size; ++i) {
        if (m_flatRules.at(i)->style() == pageStyle)
            return index;

        ++index;
    }
    return UINT_MAX;
}

bool InspectorStyleSheet::ensureParsedDataReady()
{
    return ensureText() && ensureSourceData();
}

bool InspectorStyleSheet::ensureText() const
{
    if (!m_parsedStyleSheet)
        return false;
    if (m_parsedStyleSheet->hasText())
        return true;

    String text;
    bool success = originalStyleSheetText(&text);
    if (success)
        m_parsedStyleSheet->setText(text);
    // No need to clear m_flatRules here - it's empty.

    return success;
}

bool InspectorStyleSheet::ensureSourceData()
{
    if (m_parsedStyleSheet->hasSourceData())
        return true;

    if (!m_parsedStyleSheet->hasText())
        return false;

    RefPtr<CSSStyleSheet> newStyleSheet = CSSStyleSheet::create();
    CSSParser p;
    StyleRuleRangeMap ruleRangeMap;
    p.parseSheet(newStyleSheet.get(), m_parsedStyleSheet->text(), 0, &ruleRangeMap);
    OwnPtr<ParsedStyleSheet::SourceData> rangesVector(adoptPtr(new ParsedStyleSheet::SourceData));

    Vector<CSSStyleRule*> rules;
    RefPtr<CSSRuleList> ruleList = asCSSRuleList(newStyleSheet.get());
    collectFlatRules(ruleList, &rules);
    for (unsigned i = 0, size = rules.size(); i < size; ++i) {
        StyleRuleRangeMap::iterator it = ruleRangeMap.find(rules.at(i));
        if (it != ruleRangeMap.end()) {
            fixUnparsedPropertyRanges(it->second.get(), m_parsedStyleSheet->text());
            rangesVector->append(it->second);
        }
    }

    m_parsedStyleSheet->setSourceData(rangesVector.release());
    return m_parsedStyleSheet->hasSourceData();
}

void InspectorStyleSheet::ensureFlatRules() const
{
    // We are fine with redoing this for empty stylesheets as this will run fast.
    if (m_flatRules.isEmpty())
        collectFlatRules(asCSSRuleList(pageStyleSheet()), &m_flatRules);
}

bool InspectorStyleSheet::setStyleText(CSSStyleDeclaration* style, const String& text)
{
    if (!pageStyleSheet())
        return false;
    if (!ensureParsedDataReady())
        return false;

    String patchedStyleSheetText;
    bool success = styleSheetTextWithChangedStyle(style, text, &patchedStyleSheetText);
    if (!success)
        return false;

    InspectorCSSId id = ruleOrStyleId(style);
    if (id.isEmpty())
        return false;

    ExceptionCode ec = 0;
    style->setCssText(text, ec);
    if (!ec)
        m_parsedStyleSheet->setText(patchedStyleSheetText);

    return !ec;
}

bool InspectorStyleSheet::styleSheetTextWithChangedStyle(CSSStyleDeclaration* style, const String& newStyleText, String* result)
{
    if (!style)
        return false;

    if (!ensureParsedDataReady())
        return false;

    RefPtr<CSSRuleSourceData> sourceData = ruleSourceDataFor(style);
    unsigned bodyStart = sourceData->styleSourceData->styleBodyRange.start;
    unsigned bodyEnd = sourceData->styleSourceData->styleBodyRange.end;
    ASSERT(bodyStart <= bodyEnd);

    String text = m_parsedStyleSheet->text();
    ASSERT(bodyEnd <= text.length()); // bodyEnd is exclusive

    text.replace(bodyStart, bodyEnd - bodyStart, newStyleText);
    *result = text;
    return true;
}

InspectorCSSId InspectorStyleSheet::ruleId(CSSStyleRule* rule) const
{
    return ruleOrStyleId(rule->style());
}

void InspectorStyleSheet::revalidateStyle(CSSStyleDeclaration* pageStyle)
{
    if (m_isRevalidating)
        return;

    m_isRevalidating = true;
    ensureFlatRules();
    for (unsigned i = 0, size = m_flatRules.size(); i < size; ++i) {
        CSSStyleRule* parsedRule = m_flatRules.at(i);
        if (parsedRule->style() == pageStyle) {
            if (parsedRule->style()->cssText() != pageStyle->cssText()) {
                // Clear the disabled properties for the invalid style here.
                m_inspectorStyles.remove(pageStyle);
                setStyleText(pageStyle, pageStyle->cssText());
            }
            break;
        }
    }
    m_isRevalidating = false;
}

bool InspectorStyleSheet::originalStyleSheetText(String* result) const
{
    bool success = inlineStyleSheetText(result);
    if (!success)
        success = resourceStyleSheetText(result);
    return success;
}

bool InspectorStyleSheet::resourceStyleSheetText(String* result) const
{
    if (m_origin == "user" || m_origin == "user-agent")
        return false;

    if (!m_pageStyleSheet || !ownerDocument())
        return false;

    String error;
    InspectorPageAgent::resourceContent(&error, ownerDocument()->frame(), m_pageStyleSheet->finalURL(), result);
    return error.isEmpty();
}

bool InspectorStyleSheet::inlineStyleSheetText(String* result) const
{
    if (!m_pageStyleSheet)
        return false;

    Node* ownerNode = m_pageStyleSheet->ownerNode();
    if (!ownerNode || ownerNode->nodeType() != Node::ELEMENT_NODE)
        return false;
    Element* ownerElement = static_cast<Element*>(ownerNode);
    if (ownerElement->tagName().lower() != "style")
        return false;
    *result = ownerElement->innerText();
    return true;
}

PassRefPtr<InspectorArray> InspectorStyleSheet::buildArrayForRuleList(CSSRuleList* ruleList)
{
    RefPtr<InspectorArray> result = InspectorArray::create();
    if (!ruleList)
        return result.release();

    RefPtr<CSSRuleList> refRuleList = ruleList;
    Vector<CSSStyleRule*> rules;
    collectFlatRules(refRuleList, &rules);

    for (unsigned i = 0, size = rules.size(); i < size; ++i)
        result->pushObject(buildObjectForRule(rules.at(i)));

    return result.release();
}

void InspectorStyleSheet::fixUnparsedPropertyRanges(CSSRuleSourceData* ruleData, const String& styleSheetText)
{
    Vector<CSSPropertySourceData>& propertyData = ruleData->styleSourceData->propertyData;
    unsigned size = propertyData.size();
    if (!size)
        return;

    unsigned styleStart = ruleData->styleSourceData->styleBodyRange.start;
    const UChar* characters = styleSheetText.characters();
    CSSPropertySourceData* nextData = &(propertyData.at(0));
    for (unsigned i = 0; i < size; ++i) {
        CSSPropertySourceData* currentData = nextData;
        nextData = i < size - 1 ? &(propertyData.at(i + 1)) : 0;

        if (currentData->parsedOk)
            continue;
        if (currentData->range.end > 0 && characters[styleStart + currentData->range.end - 1] == ';')
            continue;

        unsigned propertyEndInStyleSheet;
        if (!nextData)
            propertyEndInStyleSheet = ruleData->styleSourceData->styleBodyRange.end - 1;
        else
            propertyEndInStyleSheet = styleStart + nextData->range.start - 1;

        while (isHTMLSpace(characters[propertyEndInStyleSheet]))
            --propertyEndInStyleSheet;

        // propertyEndInStyleSheet points at the last property text character.
        unsigned newPropertyEnd = propertyEndInStyleSheet - styleStart + 1; // Exclusive of the last property text character.
        if (currentData->range.end != newPropertyEnd) {
            currentData->range.end = newPropertyEnd;
            unsigned valueStartInStyleSheet = styleStart + currentData->range.start + currentData->name.length();
            while (valueStartInStyleSheet < propertyEndInStyleSheet && characters[valueStartInStyleSheet] != ':')
                ++valueStartInStyleSheet;
            if (valueStartInStyleSheet < propertyEndInStyleSheet)
                ++valueStartInStyleSheet; // Shift past the ':'.
            while (valueStartInStyleSheet < propertyEndInStyleSheet && isHTMLSpace(characters[valueStartInStyleSheet]))
                ++valueStartInStyleSheet;
            // Need to exclude the trailing ';' from the property value.
            currentData->value = styleSheetText.substring(valueStartInStyleSheet, propertyEndInStyleSheet - valueStartInStyleSheet + (characters[propertyEndInStyleSheet] == ';' ? 0 : 1));
        }
    }
}

void InspectorStyleSheet::collectFlatRules(PassRefPtr<CSSRuleList> ruleList, Vector<CSSStyleRule*>* result)
{
    if (!ruleList)
        return;

    for (unsigned i = 0, size = ruleList->length(); i < size; ++i) {
        CSSRule* rule = ruleList->item(i);
        CSSStyleRule* styleRule = InspectorCSSAgent::asCSSStyleRule(rule);
        if (styleRule)
            result->append(styleRule);
        else {
            RefPtr<CSSRuleList> childRuleList = asCSSRuleList(rule);
            if (childRuleList)
                collectFlatRules(childRuleList, result);
        }
    }
}

PassRefPtr<InspectorStyleSheetForInlineStyle> InspectorStyleSheetForInlineStyle::create(const String& id, PassRefPtr<Element> element, const String& origin)
{
    return adoptRef(new InspectorStyleSheetForInlineStyle(id, element, origin));
}

InspectorStyleSheetForInlineStyle::InspectorStyleSheetForInlineStyle(const String& id, PassRefPtr<Element> element, const String& origin)
    : InspectorStyleSheet(id, 0, origin, "")
    , m_element(element)
    , m_ruleSourceData(0)
{
    ASSERT(m_element);
    m_inspectorStyle = InspectorStyle::create(InspectorCSSId(id, 0), inlineStyle(), this);
    m_styleText = m_element->isStyledElement() ? m_element->getAttribute("style").string() : String();
}

void InspectorStyleSheetForInlineStyle::didModifyElementAttribute()
{
    String newStyleText = elementStyleText();
    bool shouldDropSourceData = false;
    if (m_element->isStyledElement() && m_element->style() != m_inspectorStyle->cssStyle()) {
        m_inspectorStyle = InspectorStyle::create(InspectorCSSId(id(), 0), inlineStyle(), this);
        shouldDropSourceData = true;
    }
    if (newStyleText != m_styleText) {
        m_styleText = newStyleText;
        shouldDropSourceData = true;
    }
    if (shouldDropSourceData)
        m_ruleSourceData.clear();
}

bool InspectorStyleSheetForInlineStyle::text(String* result) const
{
    *result = m_styleText;
    return true;
}

bool InspectorStyleSheetForInlineStyle::setStyleText(CSSStyleDeclaration* style, const String& text)
{
    ASSERT_UNUSED(style, style == inlineStyle());
    ExceptionCode ec = 0;
    m_element->setAttribute("style", text, ec);
    m_styleText = text;
    m_ruleSourceData.clear();
    return !ec;
}

Document* InspectorStyleSheetForInlineStyle::ownerDocument() const
{
    return m_element->document();
}

bool InspectorStyleSheetForInlineStyle::ensureParsedDataReady()
{
    // The "style" property value can get changed indirectly, e.g. via element.style.borderWidth = "2px".
    const String& currentStyleText = elementStyleText();
    if (m_styleText != currentStyleText) {
        m_ruleSourceData.clear();
        m_styleText = currentStyleText;
    }

    if (m_ruleSourceData)
        return true;

    m_ruleSourceData = CSSRuleSourceData::create();
    RefPtr<CSSStyleSourceData> sourceData = CSSStyleSourceData::create();
    bool success = getStyleAttributeRanges(&sourceData);
    if (!success)
        return false;

    m_ruleSourceData->styleSourceData = sourceData.release();
    return true;
}

PassRefPtr<InspectorStyle> InspectorStyleSheetForInlineStyle::inspectorStyleForId(const InspectorCSSId& id)
{
    ASSERT_UNUSED(id, !id.ordinal());
    return m_inspectorStyle;
}

CSSStyleDeclaration* InspectorStyleSheetForInlineStyle::inlineStyle() const
{
    return m_element->style();
}

const String& InspectorStyleSheetForInlineStyle::elementStyleText() const
{
    return m_element->getAttribute("style").string();
}

bool InspectorStyleSheetForInlineStyle::getStyleAttributeRanges(RefPtr<CSSStyleSourceData>* result)
{
    if (!m_element->isStyledElement())
        return false;

    if (m_styleText.isEmpty()) {
        (*result)->styleBodyRange.start = 0;
        (*result)->styleBodyRange.end = 0;
        return true;
    }

    RefPtr<CSSMutableStyleDeclaration> tempDeclaration = CSSMutableStyleDeclaration::create();
    CSSParser p;
    p.parseDeclaration(tempDeclaration.get(), m_styleText, result);
    return true;
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
