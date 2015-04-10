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

#if ENABLE(INSPECTOR)

#include "InspectorStyleSheet.h"

#include "CSSHostRule.h"
#include "CSSImportRule.h"
#include "CSSMediaRule.h"
#include "CSSParser.h"
#include "CSSPropertyNames.h"
#include "CSSPropertySourceData.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "CSSStyleSheet.h"
#include "CSSSupportsRule.h"
#include "ContentSearchUtils.h"
#include "ContentSecurityPolicy.h"
#include "Document.h"
#include "Element.h"
#include "HTMLHeadElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "HTMLStyleElement.h"
#include "InspectorCSSAgent.h"
#include "InspectorPageAgent.h"
#include "InspectorValues.h"
#include "Node.h"
#include "RegularExpression.h"
#include "SVGNames.h"
#include "StylePropertySet.h"
#include "StyleResolver.h"
#include "StyleRule.h"
#include "StyleRuleImport.h"
#include "StyleSheetContents.h"
#include "StyleSheetList.h"
#include "WebKitCSSKeyframesRule.h"

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/StringBuilder.h>

using WebCore::TypeBuilder::Array;
using WebCore::RuleSourceDataList;
using WebCore::CSSRuleSourceData;

class ParsedStyleSheet {
    WTF_MAKE_FAST_ALLOCATED;
public:
    ParsedStyleSheet();

    WebCore::CSSStyleSheet* cssStyleSheet() const { return m_parserOutput; }
    const String& text() const { ASSERT(m_hasText); return m_text; }
    void setText(const String& text);
    bool hasText() const { return m_hasText; }
    RuleSourceDataList* sourceData() const { return m_sourceData.get(); }
    void setSourceData(PassOwnPtr<RuleSourceDataList>);
    bool hasSourceData() const { return m_sourceData; }
    PassRefPtr<WebCore::CSSRuleSourceData> ruleSourceDataAt(unsigned) const;

private:

    // StyleSheet constructed while parsing m_text.
    WebCore::CSSStyleSheet* m_parserOutput;
    String m_text;
    bool m_hasText;
    OwnPtr<RuleSourceDataList> m_sourceData;
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

static void flattenSourceData(RuleSourceDataList* dataList, RuleSourceDataList* target)
{
    for (size_t i = 0; i < dataList->size(); ++i) {
        RefPtr<CSSRuleSourceData>& data = dataList->at(i);
        if (data->type == CSSRuleSourceData::STYLE_RULE)
            target->append(data);
        else if (data->type == CSSRuleSourceData::MEDIA_RULE)
            flattenSourceData(&data->childRules, target);
#if ENABLE(SHADOW_DOM)
        else if (data->type == CSSRuleSourceData::HOST_RULE)
            flattenSourceData(&data->childRules, target);
#endif
#if ENABLE(CSS3_CONDITIONAL_RULES)
        else if (data->type == CSSRuleSourceData::SUPPORTS_RULE)
            flattenSourceData(&data->childRules, target);
#endif
    }
}

void ParsedStyleSheet::setSourceData(PassOwnPtr<RuleSourceDataList> sourceData)
{
    if (!sourceData) {
        m_sourceData.clear();
        return;
    }

    m_sourceData = adoptPtr(new RuleSourceDataList());

    // FIXME: This is a temporary solution to retain the original flat sourceData structure
    // containing only style rules, even though CSSParser now provides the full rule source data tree.
    // Normally, we should just assign m_sourceData = sourceData;
    flattenSourceData(sourceData.get(), m_sourceData.get());
}

PassRefPtr<WebCore::CSSRuleSourceData> ParsedStyleSheet::ruleSourceDataAt(unsigned index) const
{
    if (!hasSourceData() || index >= m_sourceData->size())
        return 0;

    return m_sourceData->at(index);
}

namespace WebCore {

enum MediaListSource {
    MediaListSourceLinkedSheet,
    MediaListSourceInlineSheet,
    MediaListSourceMediaRule,
    MediaListSourceImportRule
};

static PassRefPtr<TypeBuilder::CSS::SourceRange> buildSourceRangeObject(const SourceRange& range, Vector<size_t>* lineEndings)
{
    if (!lineEndings)
        return 0;
    TextPosition start = ContentSearchUtils::textPositionFromOffset(range.start, *lineEndings);
    TextPosition end = ContentSearchUtils::textPositionFromOffset(range.end, *lineEndings);

    RefPtr<TypeBuilder::CSS::SourceRange> result = TypeBuilder::CSS::SourceRange::create()
        .setStartLine(start.m_line.zeroBasedInt())
        .setStartColumn(start.m_column.zeroBasedInt())
        .setEndLine(end.m_line.zeroBasedInt())
        .setEndColumn(end.m_column.zeroBasedInt());
    return result.release();
}

static PassRefPtr<TypeBuilder::CSS::CSSMedia> buildMediaObject(const MediaList* media, MediaListSource mediaListSource, const String& sourceURL)
{
    // Make certain compilers happy by initializing |source| up-front.
    TypeBuilder::CSS::CSSMedia::Source::Enum source = TypeBuilder::CSS::CSSMedia::Source::InlineSheet;
    switch (mediaListSource) {
    case MediaListSourceMediaRule:
        source = TypeBuilder::CSS::CSSMedia::Source::MediaRule;
        break;
    case MediaListSourceImportRule:
        source = TypeBuilder::CSS::CSSMedia::Source::ImportRule;
        break;
    case MediaListSourceLinkedSheet:
        source = TypeBuilder::CSS::CSSMedia::Source::LinkedSheet;
        break;
    case MediaListSourceInlineSheet:
        source = TypeBuilder::CSS::CSSMedia::Source::InlineSheet;
        break;
    }

    RefPtr<TypeBuilder::CSS::CSSMedia> mediaObject = TypeBuilder::CSS::CSSMedia::create()
        .setText(media->mediaText())
        .setSource(source);

    if (!sourceURL.isEmpty()) {
        mediaObject->setSourceURL(sourceURL);
        mediaObject->setSourceLine(media->queries()->lastLine());
    }
    return mediaObject.release();
}

static PassRefPtr<CSSRuleList> asCSSRuleList(CSSStyleSheet* styleSheet)
{
    if (!styleSheet)
        return 0;

    RefPtr<StaticCSSRuleList> list = StaticCSSRuleList::create();
    Vector<RefPtr<CSSRule> >& listRules = list->rules();
    for (unsigned i = 0, size = styleSheet->length(); i < size; ++i) {
        CSSRule* item = styleSheet->item(i);
        if (item->type() == CSSRule::CHARSET_RULE)
            continue;
        listRules.append(item);
    }
    return list.release();
}

static PassRefPtr<CSSRuleList> asCSSRuleList(CSSRule* rule)
{
    if (!rule)
        return 0;

    if (rule->type() == CSSRule::MEDIA_RULE)
        return static_cast<CSSMediaRule*>(rule)->cssRules();

    if (rule->type() == CSSRule::WEBKIT_KEYFRAMES_RULE)
        return static_cast<WebKitCSSKeyframesRule*>(rule)->cssRules();

#if ENABLE(SHADOW_DOM)
    if (rule->type() == CSSRule::HOST_RULE)
        return static_cast<CSSHostRule*>(rule)->cssRules();
#endif

#if ENABLE(CSS3_CONDITIONAL_RULES)
    if (rule->type() == CSSRule::SUPPORTS_RULE)
        return static_cast<CSSSupportsRule*>(rule)->cssRules();
#endif

    return 0;
}

static void fillMediaListChain(CSSRule* rule, Array<TypeBuilder::CSS::CSSMedia>* mediaArray)
{
    MediaList* mediaList;
    CSSRule* parentRule = rule;
    String sourceURL;
    while (parentRule) {
        CSSStyleSheet* parentStyleSheet = 0;
        bool isMediaRule = true;
        if (parentRule->type() == CSSRule::MEDIA_RULE) {
            CSSMediaRule* mediaRule = static_cast<CSSMediaRule*>(parentRule);
            mediaList = mediaRule->media();
            parentStyleSheet = mediaRule->parentStyleSheet();
        } else if (parentRule->type() == CSSRule::IMPORT_RULE) {
            CSSImportRule* importRule = static_cast<CSSImportRule*>(parentRule);
            mediaList = importRule->media();
            parentStyleSheet = importRule->parentStyleSheet();
            isMediaRule = false;
        } else
            mediaList = 0;

        if (parentStyleSheet) {
            sourceURL = parentStyleSheet->contents()->baseURL();
            if (sourceURL.isEmpty())
                sourceURL = InspectorDOMAgent::documentURLString(parentStyleSheet->ownerDocument());
        } else
            sourceURL = "";

        if (mediaList && mediaList->length())
            mediaArray->addItem(buildMediaObject(mediaList, isMediaRule ? MediaListSourceMediaRule : MediaListSourceImportRule, sourceURL));

        if (parentRule->parentRule())
            parentRule = parentRule->parentRule();
        else {
            CSSStyleSheet* styleSheet = parentRule->parentStyleSheet();
            while (styleSheet) {
                mediaList = styleSheet->media();
                if (mediaList && mediaList->length()) {
                    Document* doc = styleSheet->ownerDocument();
                    if (doc)
                        sourceURL = doc->url();
                    else if (!styleSheet->contents()->baseURL().isEmpty())
                        sourceURL = styleSheet->contents()->baseURL();
                    else
                        sourceURL = "";
                    mediaArray->addItem(buildMediaObject(mediaList, styleSheet->ownerNode() ? MediaListSourceLinkedSheet : MediaListSourceInlineSheet, sourceURL));
                }
                parentRule = styleSheet->ownerRule();
                if (parentRule)
                    break;
                styleSheet = styleSheet->parentStyleSheet();
            }
        }
    }
}

static PassOwnPtr<CSSParser> createCSSParser(Document* document)
{
    return adoptPtr(new CSSParser(document ? CSSParserContext(document) : strictCSSParserContext()));
}

PassRefPtr<InspectorStyle> InspectorStyle::create(const InspectorCSSId& styleId, PassRefPtr<CSSStyleDeclaration> style, InspectorStyleSheet* parentStyleSheet)
{
    return adoptRef(new InspectorStyle(styleId, style, parentStyleSheet));
}

InspectorStyle::InspectorStyle(const InspectorCSSId& styleId, PassRefPtr<CSSStyleDeclaration> style, InspectorStyleSheet* parentStyleSheet)
    : m_styleId(styleId)
    , m_style(style)
    , m_parentStyleSheet(parentStyleSheet)
    , m_formatAcquired(false)
{
    ASSERT(m_style);
}

InspectorStyle::~InspectorStyle()
{
}

PassRefPtr<TypeBuilder::CSS::CSSStyle> InspectorStyle::buildObjectForStyle() const
{
    RefPtr<TypeBuilder::CSS::CSSStyle> result = styleWithProperties();
    if (!m_styleId.isEmpty())
        result->setStyleId(m_styleId.asProtocolValue<TypeBuilder::CSS::CSSStyleId>());

    result->setWidth(m_style->getPropertyValue("width"));
    result->setHeight(m_style->getPropertyValue("height"));

    RefPtr<CSSRuleSourceData> sourceData = extractSourceData();
    if (sourceData)
        result->setRange(buildSourceRangeObject(sourceData->ruleBodyRange, m_parentStyleSheet->lineEndings().get()));

    return result.release();
}

PassRefPtr<TypeBuilder::Array<TypeBuilder::CSS::CSSComputedStyleProperty> > InspectorStyle::buildArrayForComputedStyle() const
{
    RefPtr<TypeBuilder::Array<TypeBuilder::CSS::CSSComputedStyleProperty> > result = TypeBuilder::Array<TypeBuilder::CSS::CSSComputedStyleProperty>::create();
    Vector<InspectorStyleProperty> properties;
    populateAllProperties(&properties);

    for (Vector<InspectorStyleProperty>::iterator it = properties.begin(), itEnd = properties.end(); it != itEnd; ++it) {
        const CSSPropertySourceData& propertyEntry = it->sourceData;
        RefPtr<TypeBuilder::CSS::CSSComputedStyleProperty> entry = TypeBuilder::CSS::CSSComputedStyleProperty::create()
            .setName(propertyEntry.name)
            .setValue(propertyEntry.value);
        result->addItem(entry);
    }

    return result.release();
}

// This method does the following preprocessing of |propertyText| with |overwrite| == false and |index| past the last active property:
// - If the last property (if present) has no closing ";", the ";" is prepended to the current |propertyText| value.
// - A heuristic formatting is attempted to retain the style structure.
//
// The propertyText (if not empty) is checked to be a valid style declaration (containing at least one property). If not,
// the method returns false (denoting an error).
bool InspectorStyle::setPropertyText(unsigned index, const String& propertyText, bool overwrite, String* oldText, ExceptionCode& ec)
{
    ASSERT(m_parentStyleSheet);
    DEFINE_STATIC_LOCAL(String, bogusPropertyName, (ASCIILiteral("-webkit-boguz-propertee")));

    if (!m_parentStyleSheet->ensureParsedDataReady()) {
        ec = NOT_FOUND_ERR;
        return false;
    }

    Vector<InspectorStyleProperty> allProperties;
    populateAllProperties(&allProperties);

    if (propertyText.stripWhiteSpace().length()) {
        RefPtr<MutableStylePropertySet> tempMutableStyle = MutableStylePropertySet::create();
        RefPtr<CSSRuleSourceData> sourceData = CSSRuleSourceData::create(CSSRuleSourceData::STYLE_RULE);
        Document* ownerDocument = m_parentStyleSheet->pageStyleSheet() ? m_parentStyleSheet->pageStyleSheet()->ownerDocument() : 0;
        createCSSParser(ownerDocument)->parseDeclaration(tempMutableStyle.get(), propertyText + " " + bogusPropertyName + ": none", sourceData, m_style->parentStyleSheet()->contents());
        Vector<CSSPropertySourceData>& propertyData = sourceData->styleSourceData->propertyData;
        unsigned propertyCount = propertyData.size();

        // At least one property + the bogus property added just above should be present.
        if (propertyCount < 2) {
            ec = SYNTAX_ERR;
            return false;
        }

        // Check for a proper propertyText termination (the parser could at least restore to the PROPERTY_NAME state).
        if (propertyData.at(propertyCount - 1).name != bogusPropertyName) {
            ec = SYNTAX_ERR;
            return false;
        }
    }

    RefPtr<CSSRuleSourceData> sourceData = extractSourceData();
    if (!sourceData) {
        ec = NOT_FOUND_ERR;
        return false;
    }

    String text;
    bool success = styleText(&text);
    if (!success) {
        ec = NOT_FOUND_ERR;
        return false;
    }

    InspectorStyleTextEditor editor(&allProperties, &m_disabledProperties, text, newLineAndWhitespaceDelimiters());
    if (overwrite) {
        if (index >= allProperties.size()) {
            ec = INDEX_SIZE_ERR;
            return false;
        }
        *oldText = allProperties.at(index).rawText;
        editor.replaceProperty(index, propertyText);
    } else
        editor.insertProperty(index, propertyText, sourceData->ruleBodyRange.length());

    return applyStyleText(editor.styleText());
}

bool InspectorStyle::toggleProperty(unsigned index, bool disable, ExceptionCode& ec)
{
    ASSERT(m_parentStyleSheet);
    if (!m_parentStyleSheet->ensureParsedDataReady()) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return false;
    }

    RefPtr<CSSRuleSourceData> sourceData = extractSourceData();
    if (!sourceData) {
        ec = NOT_FOUND_ERR;
        return false;
    }

    String text;
    bool success = styleText(&text);
    if (!success) {
        ec = NOT_FOUND_ERR;
        return false;
    }

    Vector<InspectorStyleProperty> allProperties;
    populateAllProperties(&allProperties);
    if (index >= allProperties.size()) {
        ec = INDEX_SIZE_ERR;
        return false;
    }

    InspectorStyleProperty& property = allProperties.at(index);
    if (property.disabled == disable)
        return true; // Idempotent operation.

    InspectorStyleTextEditor editor(&allProperties, &m_disabledProperties, text, newLineAndWhitespaceDelimiters());
    if (disable)
        editor.disableProperty(index);
    else
        editor.enableProperty(index);

    return applyStyleText(editor.styleText());
}

bool InspectorStyle::getText(String* result) const
{
    // Precondition: m_parentStyleSheet->ensureParsedDataReady() has been called successfully.
    RefPtr<CSSRuleSourceData> sourceData = extractSourceData();
    if (!sourceData)
        return false;

    String styleSheetText;
    bool success = m_parentStyleSheet->getText(&styleSheetText);
    if (!success)
        return false;

    SourceRange& bodyRange = sourceData->ruleBodyRange;
    *result = styleSheetText.substring(bodyRange.start, bodyRange.end - bodyRange.start);
    return true;
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

    RefPtr<CSSRuleSourceData> sourceData = extractSourceData();
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

PassRefPtr<TypeBuilder::CSS::CSSStyle> InspectorStyle::styleWithProperties() const
{
    Vector<InspectorStyleProperty> properties;
    populateAllProperties(&properties);

    RefPtr<Array<TypeBuilder::CSS::CSSProperty> > propertiesObject = Array<TypeBuilder::CSS::CSSProperty>::create();
    RefPtr<Array<TypeBuilder::CSS::ShorthandEntry> > shorthandEntries = Array<TypeBuilder::CSS::ShorthandEntry>::create();
    HashMap<String, RefPtr<TypeBuilder::CSS::CSSProperty> > propertyNameToPreviousActiveProperty;
    HashSet<String> foundShorthands;
    String previousPriority;
    String previousStatus;
    OwnPtr<Vector<size_t> > lineEndings(m_parentStyleSheet ? m_parentStyleSheet->lineEndings() : PassOwnPtr<Vector<size_t> >());
    RefPtr<CSSRuleSourceData> sourceData = extractSourceData();
    unsigned ruleBodyRangeStart = sourceData ? sourceData->ruleBodyRange.start : 0;

    for (Vector<InspectorStyleProperty>::iterator it = properties.begin(), itEnd = properties.end(); it != itEnd; ++it) {
        const CSSPropertySourceData& propertyEntry = it->sourceData;
        const String& name = propertyEntry.name;

        TypeBuilder::CSS::CSSProperty::Status::Enum status = it->disabled ? TypeBuilder::CSS::CSSProperty::Status::Disabled : TypeBuilder::CSS::CSSProperty::Status::Active;

        RefPtr<TypeBuilder::CSS::CSSProperty> property = TypeBuilder::CSS::CSSProperty::create()
            .setName(name)
            .setValue(propertyEntry.value);

        propertiesObject->addItem(property);

        // Default "parsedOk" == true.
        if (!propertyEntry.parsedOk)
            property->setParsedOk(false);
        if (it->hasRawText())
            property->setText(it->rawText);

        // Default "priority" == "".
        if (propertyEntry.important)
            property->setPriority("important");
        if (!it->disabled) {
            if (it->hasSource) {
                ASSERT(sourceData);
                property->setImplicit(false);
                // The property range is relative to the style body start.
                // Should be converted into an absolute range (relative to the stylesheet start)
                // for the proper conversion into line:column.
                SourceRange absolutePropertyRange = propertyEntry.range;
                absolutePropertyRange.start += ruleBodyRangeStart;
                absolutePropertyRange.end += ruleBodyRangeStart;
                property->setRange(buildSourceRangeObject(absolutePropertyRange, lineEndings.get()));

                // Parsed property overrides any property with the same name. Non-parsed property overrides
                // previous non-parsed property with the same name (if any).
                bool shouldInactivate = false;
                CSSPropertyID propertyId = cssPropertyID(name);
                // Canonicalize property names to treat non-prefixed and vendor-prefixed property names the same (opacity vs. -webkit-opacity).
                String canonicalPropertyName = propertyId ? getPropertyNameString(propertyId) : name;
                HashMap<String, RefPtr<TypeBuilder::CSS::CSSProperty> >::iterator activeIt = propertyNameToPreviousActiveProperty.find(canonicalPropertyName);
                if (activeIt != propertyNameToPreviousActiveProperty.end()) {
                    if (propertyEntry.parsedOk) {
                        bool successPriority = activeIt->value->getString(TypeBuilder::CSS::CSSProperty::Priority, &previousPriority);
                        bool successStatus = activeIt->value->getString(TypeBuilder::CSS::CSSProperty::Status, &previousStatus);
                        if (successStatus && previousStatus != "inactive") {
                            if (propertyEntry.important || !successPriority) // Priority not set == "not important".
                                shouldInactivate = true;
                            else if (status == TypeBuilder::CSS::CSSProperty::Status::Active) {
                                // Inactivate a non-important property following the same-named important property.
                                status = TypeBuilder::CSS::CSSProperty::Status::Inactive;
                            }
                        }
                    } else {
                        bool previousParsedOk;
                        bool success = activeIt->value->getBoolean(TypeBuilder::CSS::CSSProperty::ParsedOk, &previousParsedOk);
                        if (success && !previousParsedOk)
                            shouldInactivate = true;
                    }
                } else
                    propertyNameToPreviousActiveProperty.set(canonicalPropertyName, property);

                if (shouldInactivate) {
                    activeIt->value->setStatus(TypeBuilder::CSS::CSSProperty::Status::Inactive);
                    propertyNameToPreviousActiveProperty.set(canonicalPropertyName, property);
                }
            } else {
                bool implicit = m_style->isPropertyImplicit(name);
                // Default "implicit" == false.
                if (implicit)
                    property->setImplicit(true);
                status = TypeBuilder::CSS::CSSProperty::Status::Style;

                String shorthand = m_style->getPropertyShorthand(name);
                if (!shorthand.isEmpty()) {
                    if (!foundShorthands.contains(shorthand)) {
                        foundShorthands.add(shorthand);
                        RefPtr<TypeBuilder::CSS::ShorthandEntry> entry = TypeBuilder::CSS::ShorthandEntry::create()
                            .setName(shorthand)
                            .setValue(shorthandValue(shorthand));
                        shorthandEntries->addItem(entry);
                    }
                }
            }
        }

        // Default "status" == "style".
        if (status != TypeBuilder::CSS::CSSProperty::Status::Style)
            property->setStatus(status);
    }

    RefPtr<TypeBuilder::CSS::CSSStyle> result = TypeBuilder::CSS::CSSStyle::create()
        .setCssProperties(propertiesObject)
        .setShorthandEntries(shorthandEntries);
    return result.release();
}

PassRefPtr<CSSRuleSourceData> InspectorStyle::extractSourceData() const
{
    if (!m_parentStyleSheet || !m_parentStyleSheet->ensureParsedDataReady())
        return 0;
    return m_parentStyleSheet->ruleSourceDataFor(m_style.get());
}

bool InspectorStyle::setText(const String& text, ExceptionCode& ec)
{
    return m_parentStyleSheet->setStyleText(m_style.get(), text, ec);
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

NewLineAndWhitespace& InspectorStyle::newLineAndWhitespaceDelimiters() const
{
    DEFINE_STATIC_LOCAL(String, defaultPrefix, (ASCIILiteral("    ")));

    if (m_formatAcquired)
        return m_format;

    RefPtr<CSSRuleSourceData> sourceData = extractSourceData();
    Vector<CSSPropertySourceData>* sourcePropertyData = sourceData ? &(sourceData->styleSourceData->propertyData) : 0;
    int propertyCount;
    if (!sourcePropertyData || !(propertyCount = sourcePropertyData->size())) {
        m_format.first = "\n";
        m_format.second = defaultPrefix;
        return m_format; // Do not remember the default formatting and attempt to acquire it later.
    }

    String text;
    bool success = styleText(&text);
    ASSERT_UNUSED(success, success);

    m_formatAcquired = true;

    String candidatePrefix = defaultPrefix;
    StringBuilder formatLineFeed;
    StringBuilder prefix;
    int scanStart = 0;
    int propertyIndex = 0;
    bool isFullPrefixScanned = false;
    bool lineFeedTerminated = false;
    const UChar* characters = text.characters();
    while (propertyIndex < propertyCount) {
        const WebCore::CSSPropertySourceData& currentProperty = sourcePropertyData->at(propertyIndex++);

        bool processNextProperty = false;
        int scanEnd = currentProperty.range.start;
        for (int i = scanStart; i < scanEnd; ++i) {
            UChar ch = characters[i];
            bool isLineFeed = isHTMLLineBreak(ch);
            if (isLineFeed) {
                if (!lineFeedTerminated)
                    formatLineFeed.append(ch);
                prefix.clear();
            } else if (isHTMLSpace(ch))
                prefix.append(ch);
            else {
                candidatePrefix = prefix.toString();
                prefix.clear();
                scanStart = currentProperty.range.end;
                ++propertyIndex;
                processNextProperty = true;
                break;
            }
            if (!isLineFeed && formatLineFeed.length())
                lineFeedTerminated = true;
        }
        if (!processNextProperty) {
            isFullPrefixScanned = true;
            break;
        }
    }

    m_format.first = formatLineFeed.toString();
    m_format.second = isFullPrefixScanned ? prefix.toString() : candidatePrefix;
    return m_format;
}

PassRefPtr<InspectorStyleSheet> InspectorStyleSheet::create(InspectorPageAgent* pageAgent, const String& id, PassRefPtr<CSSStyleSheet> pageStyleSheet, TypeBuilder::CSS::StyleSheetOrigin::Enum origin, const String& documentURL, Listener* listener)
{
    return adoptRef(new InspectorStyleSheet(pageAgent, id, pageStyleSheet, origin, documentURL, listener));
}

// static
String InspectorStyleSheet::styleSheetURL(CSSStyleSheet* pageStyleSheet)
{
    if (pageStyleSheet && !pageStyleSheet->contents()->baseURL().isEmpty())
        return pageStyleSheet->contents()->baseURL().string();
    return emptyString();
}

InspectorStyleSheet::InspectorStyleSheet(InspectorPageAgent* pageAgent, const String& id, PassRefPtr<CSSStyleSheet> pageStyleSheet, TypeBuilder::CSS::StyleSheetOrigin::Enum origin, const String& documentURL, Listener* listener)
    : m_pageAgent(pageAgent)
    , m_id(id)
    , m_pageStyleSheet(pageStyleSheet)
    , m_origin(origin)
    , m_documentURL(documentURL)
    , m_isRevalidating(false)
    , m_listener(listener)
{
    m_parsedStyleSheet = new ParsedStyleSheet();
}

InspectorStyleSheet::~InspectorStyleSheet()
{
    delete m_parsedStyleSheet;
}

String InspectorStyleSheet::finalURL() const
{
    String url = styleSheetURL(m_pageStyleSheet.get());
    return url.isEmpty() ? m_documentURL : url;
}

void InspectorStyleSheet::reparseStyleSheet(const String& text)
{
    {
        // Have a separate scope for clearRules() (bug 95324).
        CSSStyleSheet::RuleMutationScope mutationScope(m_pageStyleSheet.get());
        m_pageStyleSheet->contents()->clearRules();
    }
    {
        CSSStyleSheet::RuleMutationScope mutationScope(m_pageStyleSheet.get());
        m_pageStyleSheet->contents()->parseString(text);
        m_pageStyleSheet->clearChildRuleCSSOMWrappers();
        m_inspectorStyles.clear();
        fireStyleSheetChanged();
    }
}

bool InspectorStyleSheet::setText(const String& text, ExceptionCode& ec)
{
    if (!checkPageStyleSheet(ec))
        return false;
    if (!m_parsedStyleSheet)
        return false;

    m_parsedStyleSheet->setText(text);
    m_flatRules.clear();

    return true;
}

String InspectorStyleSheet::ruleSelector(const InspectorCSSId& id, ExceptionCode& ec)
{
    CSSStyleRule* rule = ruleForId(id);
    if (!rule) {
        ec = NOT_FOUND_ERR;
        return "";
    }
    return rule->selectorText();
}

bool InspectorStyleSheet::setRuleSelector(const InspectorCSSId& id, const String& selector, ExceptionCode& ec)
{
    if (!checkPageStyleSheet(ec))
        return false;
    CSSStyleRule* rule = ruleForId(id);
    if (!rule) {
        ec = NOT_FOUND_ERR;
        return false;
    }
    CSSStyleSheet* styleSheet = rule->parentStyleSheet();
    if (!styleSheet || !ensureParsedDataReady()) {
        ec = NOT_FOUND_ERR;
        return false;
    }

    rule->setSelectorText(selector);
    RefPtr<CSSRuleSourceData> sourceData = ruleSourceDataFor(rule->style());
    if (!sourceData) {
        ec = NOT_FOUND_ERR;
        return false;
    }

    String sheetText = m_parsedStyleSheet->text();
    sheetText.replace(sourceData->ruleHeaderRange.start, sourceData->ruleHeaderRange.length(), selector);
    m_parsedStyleSheet->setText(sheetText);
    fireStyleSheetChanged();
    return true;
}

static bool checkStyleRuleSelector(Document* document, const String& selector)
{
    CSSSelectorList selectorList;
    createCSSParser(document)->parseSelector(selector, selectorList);
    return selectorList.isValid();
}

CSSStyleRule* InspectorStyleSheet::addRule(const String& selector, ExceptionCode& ec)
{
    if (!checkPageStyleSheet(ec))
        return 0;
    if (!checkStyleRuleSelector(m_pageStyleSheet->ownerDocument(), selector)) {
        ec = SYNTAX_ERR;
        return 0;
    }

    String text;
    bool success = getText(&text);
    if (!success) {
        ec = NOT_FOUND_ERR;
        return 0;
    }
    StringBuilder styleSheetText;
    styleSheetText.append(text);

    m_pageStyleSheet->addRule(selector, "", ec);
    if (ec)
        return 0;
    ASSERT(m_pageStyleSheet->length());
    unsigned lastRuleIndex = m_pageStyleSheet->length() - 1;
    CSSRule* rule = m_pageStyleSheet->item(lastRuleIndex);
    ASSERT(rule);

    CSSStyleRule* styleRule = InspectorCSSAgent::asCSSStyleRule(rule);
    if (!styleRule) {
        // What we just added has to be a CSSStyleRule - we cannot handle other types of rules yet.
        // If it is not a style rule, pretend we never touched the stylesheet.
        m_pageStyleSheet->deleteRule(lastRuleIndex, ASSERT_NO_EXCEPTION);
        ec = SYNTAX_ERR;
        return 0;
    }

    if (!styleSheetText.isEmpty())
        styleSheetText.append('\n');

    styleSheetText.append(selector);
    styleSheetText.appendLiteral(" {}");
    // Using setText() as this operation changes the style sheet rule set.
    setText(styleSheetText.toString(), ASSERT_NO_EXCEPTION);

    fireStyleSheetChanged();

    return styleRule;
}

bool InspectorStyleSheet::deleteRule(const InspectorCSSId& id, ExceptionCode& ec)
{
    if (!checkPageStyleSheet(ec))
        return false;
    RefPtr<CSSStyleRule> rule = ruleForId(id);
    if (!rule) {
        ec = NOT_FOUND_ERR;
        return false;
    }
    CSSStyleSheet* styleSheet = rule->parentStyleSheet();
    if (!styleSheet || !ensureParsedDataReady()) {
        ec = NOT_FOUND_ERR;
        return false;
    }

    RefPtr<CSSRuleSourceData> sourceData = ruleSourceDataFor(rule->style());
    if (!sourceData) {
        ec = NOT_FOUND_ERR;
        return false;
    }

    styleSheet->deleteRule(id.ordinal(), ec);
    // |rule| MAY NOT be addressed after this line!

    if (ec)
        return false;

    String sheetText = m_parsedStyleSheet->text();
    sheetText.remove(sourceData->ruleHeaderRange.start, sourceData->ruleBodyRange.end - sourceData->ruleHeaderRange.start + 1);
    setText(sheetText, ASSERT_NO_EXCEPTION);
    fireStyleSheetChanged();
    return true;
}

CSSStyleRule* InspectorStyleSheet::ruleForId(const InspectorCSSId& id) const
{
    if (!m_pageStyleSheet)
        return 0;

    ASSERT(!id.isEmpty());
    ensureFlatRules();
    return id.ordinal() >= m_flatRules.size() ? 0 : m_flatRules.at(id.ordinal()).get();

}

PassRefPtr<TypeBuilder::CSS::CSSStyleSheetBody> InspectorStyleSheet::buildObjectForStyleSheet()
{
    CSSStyleSheet* styleSheet = pageStyleSheet();
    if (!styleSheet)
        return 0;

    RefPtr<CSSRuleList> cssRuleList = asCSSRuleList(styleSheet);

    RefPtr<TypeBuilder::CSS::CSSStyleSheetBody> result = TypeBuilder::CSS::CSSStyleSheetBody::create()
        .setStyleSheetId(id())
        .setRules(buildArrayForRuleList(cssRuleList.get()));

    String styleSheetText;
    bool success = getText(&styleSheetText);
    if (success)
        result->setText(styleSheetText);

    return result.release();
}

PassRefPtr<TypeBuilder::CSS::CSSStyleSheetHeader> InspectorStyleSheet::buildObjectForStyleSheetInfo()
{
    CSSStyleSheet* styleSheet = pageStyleSheet();
    if (!styleSheet)
        return 0;

    Document* document = styleSheet->ownerDocument();
    Frame* frame = document ? document->frame() : 0;
    RefPtr<TypeBuilder::CSS::CSSStyleSheetHeader> result = TypeBuilder::CSS::CSSStyleSheetHeader::create()
        .setStyleSheetId(id())
        .setOrigin(m_origin)
        .setDisabled(styleSheet->disabled())
        .setSourceURL(finalURL())
        .setTitle(styleSheet->title())
        .setFrameId(m_pageAgent->frameId(frame));

    return result.release();
}

static PassRefPtr<TypeBuilder::Array<String> > selectorsFromSource(const CSSRuleSourceData* sourceData, const String& sheetText)
{
    DEFINE_STATIC_LOCAL(RegularExpression, comment, ("/\\*[^]*?\\*/", TextCaseSensitive, MultilineEnabled));
    RefPtr<TypeBuilder::Array<String> > result = TypeBuilder::Array<String>::create();
    const SelectorRangeList& ranges = sourceData->selectorRanges;
    for (size_t i = 0, size = ranges.size(); i < size; ++i) {
        const SourceRange& range = ranges.at(i);
        String selector = sheetText.substring(range.start, range.length());

        // We don't want to see any comments in the selector components, only the meaningful parts.
        replace(selector, comment, "");
        result->addItem(selector.stripWhiteSpace());
    }
    return result.release();
}

PassRefPtr<TypeBuilder::CSS::SelectorList> InspectorStyleSheet::buildObjectForSelectorList(CSSStyleRule* rule)
{
    RefPtr<CSSRuleSourceData> sourceData;
    if (ensureParsedDataReady())
        sourceData = ruleSourceDataFor(rule->style());
    RefPtr<TypeBuilder::Array<String> > selectors;

    // This intentionally does not rely on the source data to avoid catching the trailing comments (before the declaration starting '{').
    String selectorText = rule->selectorText();

    if (sourceData)
        selectors = selectorsFromSource(sourceData.get(), m_parsedStyleSheet->text());
    else {
        selectors = TypeBuilder::Array<String>::create();
        const CSSSelectorList& selectorList = rule->styleRule()->selectorList();
        for (const CSSSelector* selector = selectorList.first(); selector; selector = CSSSelectorList::next(selector))
            selectors->addItem(selector->selectorText());
    }
    RefPtr<TypeBuilder::CSS::SelectorList> result = TypeBuilder::CSS::SelectorList::create()
        .setSelectors(selectors)
        .setText(selectorText)
        .release();
    if (sourceData)
        result->setRange(buildSourceRangeObject(sourceData->ruleHeaderRange, lineEndings().get()));
    return result.release();
}

PassRefPtr<TypeBuilder::CSS::CSSRule> InspectorStyleSheet::buildObjectForRule(CSSStyleRule* rule)
{
    CSSStyleSheet* styleSheet = pageStyleSheet();
    if (!styleSheet)
        return 0;

    RefPtr<TypeBuilder::CSS::CSSRule> result = TypeBuilder::CSS::CSSRule::create()
        .setSelectorList(buildObjectForSelectorList(rule))
        .setSourceLine(rule->styleRule()->sourceLine())
        .setOrigin(m_origin)
        .setStyle(buildObjectForStyle(rule->style()));

    // "sourceURL" is present only for regular rules, otherwise "origin" should be used in the frontend.
    if (m_origin == TypeBuilder::CSS::StyleSheetOrigin::Regular)
        result->setSourceURL(finalURL());

    if (canBind()) {
        InspectorCSSId id(ruleId(rule));
        if (!id.isEmpty())
            result->setRuleId(id.asProtocolValue<TypeBuilder::CSS::CSSRuleId>());
    }

    RefPtr<Array<TypeBuilder::CSS::CSSMedia> > mediaArray = Array<TypeBuilder::CSS::CSSMedia>::create();

    fillMediaListChain(rule, mediaArray.get());
    if (mediaArray->length())
        result->setMedia(mediaArray.release());

    return result.release();
}

PassRefPtr<TypeBuilder::CSS::CSSStyle> InspectorStyleSheet::buildObjectForStyle(CSSStyleDeclaration* style)
{
    RefPtr<CSSRuleSourceData> sourceData;
    if (ensureParsedDataReady())
        sourceData = ruleSourceDataFor(style);

    InspectorCSSId id = ruleOrStyleId(style);
    if (id.isEmpty()) {
        RefPtr<TypeBuilder::CSS::CSSStyle> bogusStyle = TypeBuilder::CSS::CSSStyle::create()
            .setCssProperties(Array<TypeBuilder::CSS::CSSProperty>::create())
            .setShorthandEntries(Array<TypeBuilder::CSS::ShorthandEntry>::create());
        return bogusStyle.release();
    }
    RefPtr<InspectorStyle> inspectorStyle = inspectorStyleForId(id);
    RefPtr<TypeBuilder::CSS::CSSStyle> result = inspectorStyle->buildObjectForStyle();

    // Style text cannot be retrieved without stylesheet, so set cssText here.
    if (sourceData) {
        String sheetText;
        bool success = getText(&sheetText);
        if (success) {
            const SourceRange& bodyRange = sourceData->ruleBodyRange;
            result->setCssText(sheetText.substring(bodyRange.start, bodyRange.end - bodyRange.start));
        }
    }

    return result.release();
}

bool InspectorStyleSheet::setStyleText(const InspectorCSSId& id, const String& text, String* oldText, ExceptionCode& ec)
{
    RefPtr<InspectorStyle> inspectorStyle = inspectorStyleForId(id);
    if (!inspectorStyle) {
        ec = NOT_FOUND_ERR;
        return false;
    }

    if (oldText && !inspectorStyle->getText(oldText))
        return false;

    bool success = inspectorStyle->setText(text, ec);
    if (success)
        fireStyleSheetChanged();
    return success;
}

bool InspectorStyleSheet::setPropertyText(const InspectorCSSId& id, unsigned propertyIndex, const String& text, bool overwrite, String* oldText, ExceptionCode& ec)
{
    RefPtr<InspectorStyle> inspectorStyle = inspectorStyleForId(id);
    if (!inspectorStyle) {
        ec = NOT_FOUND_ERR;
        return false;
    }

    bool success = inspectorStyle->setPropertyText(propertyIndex, text, overwrite, oldText, ec);
    if (success)
        fireStyleSheetChanged();
    return success;
}

bool InspectorStyleSheet::toggleProperty(const InspectorCSSId& id, unsigned propertyIndex, bool disable, ExceptionCode& ec)
{
    RefPtr<InspectorStyle> inspectorStyle = inspectorStyleForId(id);
    if (!inspectorStyle) {
        ec = NOT_FOUND_ERR;
        return false;
    }

    bool success = inspectorStyle->toggleProperty(propertyIndex, disable, ec);
    if (success) {
        if (disable)
            rememberInspectorStyle(inspectorStyle);
        else if (!inspectorStyle->hasDisabledProperties())
            forgetInspectorStyle(inspectorStyle->cssStyle());
        fireStyleSheetChanged();
    }
    return success;
}

bool InspectorStyleSheet::getText(String* result) const
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

void InspectorStyleSheet::fireStyleSheetChanged()
{
    if (m_listener)
        m_listener->styleSheetChanged(this);
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
    return it->value;
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
    return m_pageStyleSheet->ownerDocument();
}

RefPtr<CSSRuleSourceData> InspectorStyleSheet::ruleSourceDataFor(CSSStyleDeclaration* style) const
{
    return m_parsedStyleSheet->ruleSourceDataAt(ruleIndexByStyle(style));
}

PassOwnPtr<Vector<size_t> > InspectorStyleSheet::lineEndings() const
{
    if (!m_parsedStyleSheet->hasText())
        return PassOwnPtr<Vector<size_t> >();
    return ContentSearchUtils::lineEndings(m_parsedStyleSheet->text());
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

bool InspectorStyleSheet::checkPageStyleSheet(ExceptionCode& ec) const
{
    if (!m_pageStyleSheet) {
        ec = NOT_SUPPORTED_ERR;
        return false;
    }
    return true;
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

    RefPtr<StyleSheetContents> newStyleSheet = StyleSheetContents::create();
    OwnPtr<RuleSourceDataList> ruleSourceDataResult = adoptPtr(new RuleSourceDataList());
    createCSSParser(m_pageStyleSheet->ownerDocument())->parseSheet(newStyleSheet.get(), m_parsedStyleSheet->text(), 0, ruleSourceDataResult.get());
    m_parsedStyleSheet->setSourceData(ruleSourceDataResult.release());
    return m_parsedStyleSheet->hasSourceData();
}

void InspectorStyleSheet::ensureFlatRules() const
{
    // We are fine with redoing this for empty stylesheets as this will run fast.
    if (m_flatRules.isEmpty())
        collectFlatRules(asCSSRuleList(pageStyleSheet()), &m_flatRules);
}

bool InspectorStyleSheet::setStyleText(CSSStyleDeclaration* style, const String& text, ExceptionCode& ec)
{
    if (!m_pageStyleSheet)
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
    unsigned bodyStart = sourceData->ruleBodyRange.start;
    unsigned bodyEnd = sourceData->ruleBodyRange.end;
    ASSERT(bodyStart <= bodyEnd);

    String text = m_parsedStyleSheet->text();
    ASSERT_WITH_SECURITY_IMPLICATION(bodyEnd <= text.length()); // bodyEnd is exclusive

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
        CSSStyleRule* parsedRule = m_flatRules.at(i).get();
        if (parsedRule->style() == pageStyle) {
            if (parsedRule->styleRule()->properties()->asText() != pageStyle->cssText()) {
                // Clear the disabled properties for the invalid style here.
                m_inspectorStyles.remove(pageStyle);

                ExceptionCode ec = 0;
                setStyleText(pageStyle, pageStyle->cssText(), ec);
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
    if (m_origin == TypeBuilder::CSS::StyleSheetOrigin::User || m_origin == TypeBuilder::CSS::StyleSheetOrigin::User_agent)
        return false;

    if (!m_pageStyleSheet || !ownerDocument() || !ownerDocument()->frame())
        return false;

    String error;
    bool base64Encoded;
    InspectorPageAgent::resourceContent(&error, ownerDocument()->frame(), KURL(ParsedURLString, m_pageStyleSheet->href()), result, &base64Encoded);
    return error.isEmpty() && !base64Encoded;
}

bool InspectorStyleSheet::inlineStyleSheetText(String* result) const
{
    if (!m_pageStyleSheet)
        return false;

    Node* ownerNode = m_pageStyleSheet->ownerNode();
    if (!ownerNode || ownerNode->nodeType() != Node::ELEMENT_NODE)
        return false;
    Element* ownerElement = toElement(ownerNode);

    if (!isHTMLStyleElement(ownerElement)
#if ENABLE(SVG)
        && !ownerElement->hasTagName(SVGNames::styleTag)
#endif
    )
        return false;
    *result = ownerElement->textContent();
    return true;
}

PassRefPtr<TypeBuilder::Array<TypeBuilder::CSS::CSSRule> > InspectorStyleSheet::buildArrayForRuleList(CSSRuleList* ruleList)
{
    RefPtr<TypeBuilder::Array<TypeBuilder::CSS::CSSRule> > result = TypeBuilder::Array<TypeBuilder::CSS::CSSRule>::create();
    if (!ruleList)
        return result.release();

    RefPtr<CSSRuleList> refRuleList = ruleList;
    CSSStyleRuleVector rules;
    collectFlatRules(refRuleList, &rules);

    for (unsigned i = 0, size = rules.size(); i < size; ++i)
        result->addItem(buildObjectForRule(rules.at(i).get()));

    return result.release();
}

void InspectorStyleSheet::collectFlatRules(PassRefPtr<CSSRuleList> ruleList, CSSStyleRuleVector* result)
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

PassRefPtr<InspectorStyleSheetForInlineStyle> InspectorStyleSheetForInlineStyle::create(InspectorPageAgent* pageAgent, const String& id, PassRefPtr<Element> element, TypeBuilder::CSS::StyleSheetOrigin::Enum origin, Listener* listener)
{
    return adoptRef(new InspectorStyleSheetForInlineStyle(pageAgent, id, element, origin, listener));
}

InspectorStyleSheetForInlineStyle::InspectorStyleSheetForInlineStyle(InspectorPageAgent* pageAgent, const String& id, PassRefPtr<Element> element, TypeBuilder::CSS::StyleSheetOrigin::Enum origin, Listener* listener)
    : InspectorStyleSheet(pageAgent, id, 0, origin, "", listener)
    , m_element(element)
    , m_ruleSourceData(0)
    , m_isStyleTextValid(false)
{
    ASSERT(m_element);
    m_inspectorStyle = InspectorStyle::create(InspectorCSSId(id, 0), inlineStyle(), this);
    m_styleText = m_element->isStyledElement() ? m_element->getAttribute("style").string() : String();
}

void InspectorStyleSheetForInlineStyle::didModifyElementAttribute()
{
    m_isStyleTextValid = false;
    if (m_element->isStyledElement() && m_element->style() != m_inspectorStyle->cssStyle())
        m_inspectorStyle = InspectorStyle::create(InspectorCSSId(id(), 0), inlineStyle(), this);
    m_ruleSourceData.clear();
}

bool InspectorStyleSheetForInlineStyle::getText(String* result) const
{
    if (!m_isStyleTextValid) {
        m_styleText = elementStyleText();
        m_isStyleTextValid = true;
    }
    *result = m_styleText;
    return true;
}

bool InspectorStyleSheetForInlineStyle::setStyleText(CSSStyleDeclaration* style, const String& text, ExceptionCode& ec)
{
    ASSERT_UNUSED(style, style == inlineStyle());

    {
        InspectorCSSAgent::InlineStyleOverrideScope overrideScope(m_element->ownerDocument());
        m_element->setAttribute("style", text, ec);
    }

    m_styleText = text;
    m_isStyleTextValid = true;
    m_ruleSourceData.clear();
    return !ec;
}

PassOwnPtr<Vector<size_t> > InspectorStyleSheetForInlineStyle::lineEndings() const
{
    return ContentSearchUtils::lineEndings(elementStyleText());
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
        m_isStyleTextValid = true;
    }

    if (m_ruleSourceData)
        return true;

    m_ruleSourceData = CSSRuleSourceData::create(CSSRuleSourceData::STYLE_RULE);
    bool success = getStyleAttributeRanges(m_ruleSourceData.get());
    if (!success)
        return false;

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

bool InspectorStyleSheetForInlineStyle::getStyleAttributeRanges(CSSRuleSourceData* result) const
{
    if (!m_element->isStyledElement())
        return false;

    if (m_styleText.isEmpty()) {
        result->ruleBodyRange.start = 0;
        result->ruleBodyRange.end = 0;
        return true;
    }

    RefPtr<MutableStylePropertySet> tempDeclaration = MutableStylePropertySet::create();
    createCSSParser(m_element->document())->parseDeclaration(tempDeclaration.get(), m_styleText, result, m_element->document()->elementSheet()->contents());
    return true;
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
