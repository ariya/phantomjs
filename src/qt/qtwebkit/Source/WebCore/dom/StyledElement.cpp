/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008, 2010 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "StyledElement.h"

#include "Attribute.h"
#include "CSSImageValue.h"
#include "CSSParser.h"
#include "CSSPropertyNames.h"
#include "CSSStyleSheet.h"
#include "CSSValueKeywords.h"
#include "CSSValuePool.h"
#include "Color.h"
#include "ClassList.h"
#include "ContentSecurityPolicy.h"
#include "DOMTokenList.h"
#include "Document.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "PropertySetCSSStyleDeclaration.h"
#include "ScriptableDocumentParser.h"
#include "StylePropertySet.h"
#include "StyleResolver.h"
#include <wtf/HashFunctions.h>
#include <wtf/text/TextPosition.h>

using namespace std;

namespace WebCore {

COMPILE_ASSERT(sizeof(StyledElement) == sizeof(Element), styledelement_should_remain_same_size_as_element);

using namespace HTMLNames;

struct PresentationAttributeCacheKey {
    PresentationAttributeCacheKey() : tagName(0) { }
    AtomicStringImpl* tagName;
    // Only the values need refcounting.
    Vector<pair<AtomicStringImpl*, AtomicString>, 3> attributesAndValues;
};

struct PresentationAttributeCacheEntry {
    WTF_MAKE_FAST_ALLOCATED;
public:
    PresentationAttributeCacheKey key;
    RefPtr<StylePropertySet> value;
};

typedef HashMap<unsigned, OwnPtr<PresentationAttributeCacheEntry>, AlreadyHashed> PresentationAttributeCache;
    
static bool operator!=(const PresentationAttributeCacheKey& a, const PresentationAttributeCacheKey& b)
{
    if (a.tagName != b.tagName)
        return true;
    return a.attributesAndValues != b.attributesAndValues;
}

static PresentationAttributeCache& presentationAttributeCache()
{
    DEFINE_STATIC_LOCAL(PresentationAttributeCache, cache, ());
    return cache;
}

class PresentationAttributeCacheCleaner {
    WTF_MAKE_NONCOPYABLE(PresentationAttributeCacheCleaner); WTF_MAKE_FAST_ALLOCATED;
public:
    PresentationAttributeCacheCleaner()
        : m_hitCount(0)
        , m_cleanTimer(this, &PresentationAttributeCacheCleaner::cleanCache)
    {
    }

    void didHitPresentationAttributeCache()
    {
        if (presentationAttributeCache().size() < minimumPresentationAttributeCacheSizeForCleaning)
            return;

        m_hitCount++;

        if (!m_cleanTimer.isActive())
            m_cleanTimer.startOneShot(presentationAttributeCacheCleanTimeInSeconds);
     }

private:
    static const unsigned presentationAttributeCacheCleanTimeInSeconds = 60;
    static const int minimumPresentationAttributeCacheSizeForCleaning = 100;
    static const unsigned minimumPresentationAttributeCacheHitCountPerMinute = (100 * presentationAttributeCacheCleanTimeInSeconds) / 60;

    void cleanCache(Timer<PresentationAttributeCacheCleaner>* timer)
    {
        ASSERT_UNUSED(timer, timer == &m_cleanTimer);
        unsigned hitCount = m_hitCount;
        m_hitCount = 0;
        if (hitCount > minimumPresentationAttributeCacheHitCountPerMinute)
            return;
        presentationAttributeCache().clear();
    }

    unsigned m_hitCount;
    Timer<PresentationAttributeCacheCleaner> m_cleanTimer;
};

static PresentationAttributeCacheCleaner& presentationAttributeCacheCleaner()
{
    DEFINE_STATIC_LOCAL(PresentationAttributeCacheCleaner, cleaner, ());
    return cleaner;
}

void StyledElement::synchronizeStyleAttributeInternal() const
{
    ASSERT(elementData());
    ASSERT(elementData()->m_styleAttributeIsDirty);
    elementData()->m_styleAttributeIsDirty = false;
    if (const StylePropertySet* inlineStyle = this->inlineStyle())
        const_cast<StyledElement*>(this)->setSynchronizedLazyAttribute(styleAttr, inlineStyle->asText());
}

StyledElement::~StyledElement()
{
    if (PropertySetCSSStyleDeclaration* cssomWrapper = inlineStyleCSSOMWrapper())
        cssomWrapper->clearParentElement();
}

CSSStyleDeclaration* StyledElement::style()
{
    return ensureMutableInlineStyle()->ensureInlineCSSStyleDeclaration(this);
}

MutableStylePropertySet* StyledElement::ensureMutableInlineStyle()
{
    RefPtr<StylePropertySet>& inlineStyle = ensureUniqueElementData()->m_inlineStyle;
    if (!inlineStyle)
        inlineStyle = MutableStylePropertySet::create(strictToCSSParserMode(isHTMLElement() && !document()->inQuirksMode()));
    else if (!inlineStyle->isMutable())
        inlineStyle = inlineStyle->mutableCopy();
    ASSERT(inlineStyle->isMutable());
    return static_cast<MutableStylePropertySet*>(inlineStyle.get());
}

void StyledElement::attributeChanged(const QualifiedName& name, const AtomicString& newValue, AttributeModificationReason reason)
{
    if (name == styleAttr)
        styleAttributeChanged(newValue, reason);
    else if (isPresentationAttribute(name)) {
        elementData()->m_presentationAttributeStyleIsDirty = true;
        setNeedsStyleRecalc(InlineStyleChange);
    }

    Element::attributeChanged(name, newValue, reason);
}

PropertySetCSSStyleDeclaration* StyledElement::inlineStyleCSSOMWrapper()
{
    if (!inlineStyle() || !inlineStyle()->hasCSSOMWrapper())
        return 0;
    PropertySetCSSStyleDeclaration* cssomWrapper = ensureMutableInlineStyle()->cssStyleDeclaration();
    ASSERT(cssomWrapper && cssomWrapper->parentElement() == this);
    return cssomWrapper;
}

inline void StyledElement::setInlineStyleFromString(const AtomicString& newStyleString)
{
    RefPtr<StylePropertySet>& inlineStyle = elementData()->m_inlineStyle;

    // Avoid redundant work if we're using shared attribute data with already parsed inline style.
    if (inlineStyle && !elementData()->isUnique())
        return;

    // We reconstruct the property set instead of mutating if there is no CSSOM wrapper.
    // This makes wrapperless property sets immutable and so cacheable.
    if (inlineStyle && !inlineStyle->isMutable())
        inlineStyle.clear();

    if (!inlineStyle)
        inlineStyle = CSSParser::parseInlineStyleDeclaration(newStyleString, this);
    else {
        ASSERT(inlineStyle->isMutable());
        static_pointer_cast<MutableStylePropertySet>(inlineStyle)->parseDeclaration(newStyleString, document()->elementSheet()->contents());
    }
}

void StyledElement::styleAttributeChanged(const AtomicString& newStyleString, AttributeModificationReason reason)
{
    WTF::OrdinalNumber startLineNumber = WTF::OrdinalNumber::beforeFirst();
    if (document() && document()->scriptableDocumentParser() && !document()->isInDocumentWrite())
        startLineNumber = document()->scriptableDocumentParser()->lineNumber();

    if (newStyleString.isNull()) {
        if (PropertySetCSSStyleDeclaration* cssomWrapper = inlineStyleCSSOMWrapper())
            cssomWrapper->clearParentElement();
        ensureUniqueElementData()->m_inlineStyle.clear();
    } else if (reason == ModifiedByCloning || document()->contentSecurityPolicy()->allowInlineStyle(document()->url(), startLineNumber))
        setInlineStyleFromString(newStyleString);

    elementData()->m_styleAttributeIsDirty = false;

    setNeedsStyleRecalc(InlineStyleChange);
    InspectorInstrumentation::didInvalidateStyleAttr(document(), this);
}

void StyledElement::inlineStyleChanged()
{
    setNeedsStyleRecalc(InlineStyleChange);
    ASSERT(elementData());
    elementData()->m_styleAttributeIsDirty = true;
    InspectorInstrumentation::didInvalidateStyleAttr(document(), this);
}
    
bool StyledElement::setInlineStyleProperty(CSSPropertyID propertyID, CSSValueID identifier, bool important)
{
    ensureMutableInlineStyle()->setProperty(propertyID, cssValuePool().createIdentifierValue(identifier), important);
    inlineStyleChanged();
    return true;
}

bool StyledElement::setInlineStyleProperty(CSSPropertyID propertyID, CSSPropertyID identifier, bool important)
{
    ensureMutableInlineStyle()->setProperty(propertyID, cssValuePool().createIdentifierValue(identifier), important);
    inlineStyleChanged();
    return true;
}

bool StyledElement::setInlineStyleProperty(CSSPropertyID propertyID, double value, CSSPrimitiveValue::UnitTypes unit, bool important)
{
    ensureMutableInlineStyle()->setProperty(propertyID, cssValuePool().createValue(value, unit), important);
    inlineStyleChanged();
    return true;
}

bool StyledElement::setInlineStyleProperty(CSSPropertyID propertyID, const String& value, bool important)
{
    bool changes = ensureMutableInlineStyle()->setProperty(propertyID, value, important, document()->elementSheet()->contents());
    if (changes)
        inlineStyleChanged();
    return changes;
}

bool StyledElement::removeInlineStyleProperty(CSSPropertyID propertyID)
{
    if (!inlineStyle())
        return false;
    bool changes = ensureMutableInlineStyle()->removeProperty(propertyID);
    if (changes)
        inlineStyleChanged();
    return changes;
}

void StyledElement::removeAllInlineStyleProperties()
{
    if (!inlineStyle() || inlineStyle()->isEmpty())
        return;
    ensureMutableInlineStyle()->clear();
    inlineStyleChanged();
}

void StyledElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    if (const StylePropertySet* inlineStyle = elementData() ? elementData()->inlineStyle() : 0)
        inlineStyle->addSubresourceStyleURLs(urls, document()->elementSheet()->contents());
}

static inline bool attributeNameSort(const pair<AtomicStringImpl*, AtomicString>& p1, const pair<AtomicStringImpl*, AtomicString>& p2)
{
    // Sort based on the attribute name pointers. It doesn't matter what the order is as long as it is always the same. 
    return p1.first < p2.first;
}

void StyledElement::makePresentationAttributeCacheKey(PresentationAttributeCacheKey& result) const
{    
    // FIXME: Enable for SVG.
    if (namespaceURI() != xhtmlNamespaceURI)
        return;
    // Interpretation of the size attributes on <input> depends on the type attribute.
    if (hasTagName(inputTag))
        return;
    unsigned size = attributeCount();
    for (unsigned i = 0; i < size; ++i) {
        const Attribute* attribute = attributeItem(i);
        if (!isPresentationAttribute(attribute->name()))
            continue;
        if (!attribute->namespaceURI().isNull())
            return;
        // FIXME: Background URL may depend on the base URL and can't be shared. Disallow caching.
        if (attribute->name() == backgroundAttr)
            return;
        result.attributesAndValues.append(make_pair(attribute->localName().impl(), attribute->value()));
    }
    if (result.attributesAndValues.isEmpty())
        return;
    // Attribute order doesn't matter. Sort for easy equality comparison.
    std::sort(result.attributesAndValues.begin(), result.attributesAndValues.end(), attributeNameSort);
    // The cache key is non-null when the tagName is set.
    result.tagName = localName().impl();
}

static unsigned computePresentationAttributeCacheHash(const PresentationAttributeCacheKey& key)
{
    if (!key.tagName)
        return 0;
    ASSERT(key.attributesAndValues.size());
    unsigned attributeHash = StringHasher::hashMemory(key.attributesAndValues.data(), key.attributesAndValues.size() * sizeof(key.attributesAndValues[0]));
    return WTF::pairIntHash(key.tagName->existingHash(), attributeHash);
}

void StyledElement::rebuildPresentationAttributeStyle()
{
    PresentationAttributeCacheKey cacheKey;
    makePresentationAttributeCacheKey(cacheKey);

    unsigned cacheHash = computePresentationAttributeCacheHash(cacheKey);

    PresentationAttributeCache::iterator cacheIterator;
    if (cacheHash) {
        cacheIterator = presentationAttributeCache().add(cacheHash, nullptr).iterator;
        if (cacheIterator->value && cacheIterator->value->key != cacheKey)
            cacheHash = 0;
    } else
        cacheIterator = presentationAttributeCache().end();

    RefPtr<StylePropertySet> style;
    if (cacheHash && cacheIterator->value) {
        style = cacheIterator->value->value;
        presentationAttributeCacheCleaner().didHitPresentationAttributeCache();
    } else {
        style = MutableStylePropertySet::create(isSVGElement() ? SVGAttributeMode : CSSQuirksMode);
        unsigned size = attributeCount();
        for (unsigned i = 0; i < size; ++i) {
            const Attribute* attribute = attributeItem(i);
            collectStyleForPresentationAttribute(attribute->name(), attribute->value(), static_cast<MutableStylePropertySet*>(style.get()));
        }
    }

    // ShareableElementData doesn't store presentation attribute style, so make sure we have a UniqueElementData.
    UniqueElementData* elementData = ensureUniqueElementData();

    elementData->m_presentationAttributeStyleIsDirty = false;
    elementData->m_presentationAttributeStyle = style->isEmpty() ? 0 : style;

    if (!cacheHash || cacheIterator->value)
        return;

    OwnPtr<PresentationAttributeCacheEntry> newEntry = adoptPtr(new PresentationAttributeCacheEntry);
    newEntry->key = cacheKey;
    newEntry->value = style.release();

    static const int presentationAttributeCacheMaximumSize = 4096;
    if (presentationAttributeCache().size() > presentationAttributeCacheMaximumSize) {
        // Start building from scratch if the cache ever gets big.
        presentationAttributeCache().clear();
        presentationAttributeCache().set(cacheHash, newEntry.release());
    } else
        cacheIterator->value = newEntry.release();
}

void StyledElement::addPropertyToPresentationAttributeStyle(MutableStylePropertySet* style, CSSPropertyID propertyID, CSSValueID identifier)
{
    style->setProperty(propertyID, cssValuePool().createIdentifierValue(identifier));
}

void StyledElement::addPropertyToPresentationAttributeStyle(MutableStylePropertySet* style, CSSPropertyID propertyID, double value, CSSPrimitiveValue::UnitTypes unit)
{
    style->setProperty(propertyID, cssValuePool().createValue(value, unit));
}
    
void StyledElement::addPropertyToPresentationAttributeStyle(MutableStylePropertySet* style, CSSPropertyID propertyID, const String& value)
{
    style->setProperty(propertyID, value, false, document()->elementSheet()->contents());
}

}
