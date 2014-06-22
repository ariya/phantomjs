/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2006, 2007, 2012, 2013 Apple Inc. All rights reserved.
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
#include "StyleSheetContents.h"

#include "CSSImportRule.h"
#include "CSSParser.h"
#include "CSSStyleSheet.h"
#include "CachedCSSStyleSheet.h"
#include "Document.h"
#include "MediaList.h"
#include "Node.h"
#include "RuleSet.h"
#include "SecurityOrigin.h"
#include "StylePropertySet.h"
#include "StyleRule.h"
#include "StyleRuleImport.h"
#include <wtf/Deque.h>

namespace WebCore {

// Rough size estimate for the memory cache.
unsigned StyleSheetContents::estimatedSizeInBytes() const
{
    // Note that this does not take into account size of the strings hanging from various objects. 
    // The assumption is that nearly all of of them are atomic and would exist anyway.
    unsigned size = sizeof(*this);

    // FIXME: This ignores the children of media and region rules.
    // Most rules are StyleRules.
    size += ruleCount() * StyleRule::averageSizeInBytes();

    for (unsigned i = 0; i < m_importRules.size(); ++i) {
        if (StyleSheetContents* sheet = m_importRules[i]->styleSheet())
            size += sheet->estimatedSizeInBytes();
    }
    return size;
}

StyleSheetContents::StyleSheetContents(StyleRuleImport* ownerRule, const String& originalURL, const CSSParserContext& context)
    : m_ownerRule(ownerRule)
    , m_originalURL(originalURL)
    , m_loadCompleted(false)
    , m_isUserStyleSheet(ownerRule && ownerRule->parentStyleSheet() && ownerRule->parentStyleSheet()->isUserStyleSheet())
    , m_hasSyntacticallyValidCSSHeader(true)
    , m_didLoadErrorOccur(false)
    , m_usesRemUnits(false)
    , m_isMutable(false)
    , m_isInMemoryCache(false)
    , m_parserContext(context)
{
}

StyleSheetContents::StyleSheetContents(const StyleSheetContents& o)
    : RefCounted<StyleSheetContents>()
    , m_ownerRule(0)
    , m_originalURL(o.m_originalURL)
    , m_encodingFromCharsetRule(o.m_encodingFromCharsetRule)
    , m_importRules(o.m_importRules.size())
    , m_childRules(o.m_childRules.size())
    , m_namespaces(o.m_namespaces)
    , m_loadCompleted(true)
    , m_isUserStyleSheet(o.m_isUserStyleSheet)
    , m_hasSyntacticallyValidCSSHeader(o.m_hasSyntacticallyValidCSSHeader)
    , m_didLoadErrorOccur(false)
    , m_usesRemUnits(o.m_usesRemUnits)
    , m_isMutable(false)
    , m_isInMemoryCache(false)
    , m_parserContext(o.m_parserContext)
{
    ASSERT(o.isCacheable());

    // FIXME: Copy import rules.
    ASSERT(o.m_importRules.isEmpty());

    for (unsigned i = 0; i < m_childRules.size(); ++i)
        m_childRules[i] = o.m_childRules[i]->copy();
}

StyleSheetContents::~StyleSheetContents()
{
    clearRules();
}

bool StyleSheetContents::isCacheable() const
{
    // FIXME: Support copying import rules.
    if (!m_importRules.isEmpty())
        return false;
    // FIXME: Support cached stylesheets in import rules.
    if (m_ownerRule)
        return false;
    // This would require dealing with multiple clients for load callbacks.
    if (!m_loadCompleted)
        return false;
    if (m_didLoadErrorOccur)
        return false;
    // It is not the original sheet anymore.
    if (m_isMutable)
        return false;
    // If the header is valid we are not going to need to check the SecurityOrigin.
    // FIXME: Valid mime type avoids the check too.
    if (!m_hasSyntacticallyValidCSSHeader)
        return false;
    return true;
}

void StyleSheetContents::parserAppendRule(PassRefPtr<StyleRuleBase> rule)
{
    ASSERT(!rule->isCharsetRule());
    if (rule->isImportRule()) {
        // Parser enforces that @import rules come before anything else except @charset.
        ASSERT(m_childRules.isEmpty());
        m_importRules.append(static_cast<StyleRuleImport*>(rule.get()));
        m_importRules.last()->setParentStyleSheet(this);
        m_importRules.last()->requestStyleSheet();
        return;
    }

#if ENABLE(RESOLUTION_MEDIA_QUERY)
    // Add warning message to inspector if dpi/dpcm values are used for screen media.
    if (rule->isMediaRule())
        reportMediaQueryWarningIfNeeded(singleOwnerDocument(), static_cast<StyleRuleMedia*>(rule.get())->mediaQueries());
#endif

    // NOTE: The selector list has to fit into RuleData. <http://webkit.org/b/118369>
    // If we're adding a rule with a huge number of selectors, split it up into multiple rules
    if (rule->isStyleRule() && toStyleRule(rule.get())->selectorList().componentCount() > RuleData::maximumSelectorComponentCount) {
        Vector<RefPtr<StyleRule> > rules = toStyleRule(rule.get())->splitIntoMultipleRulesWithMaximumSelectorComponentCount(RuleData::maximumSelectorComponentCount);
        m_childRules.appendVector(rules);
        return;
    }

    m_childRules.append(rule);
}

StyleRuleBase* StyleSheetContents::ruleAt(unsigned index) const
{
    ASSERT_WITH_SECURITY_IMPLICATION(index < ruleCount());
    
    unsigned childVectorIndex = index;
    if (hasCharsetRule()) {
        if (index == 0)
            return 0;
        --childVectorIndex;
    }
    if (childVectorIndex < m_importRules.size())
        return m_importRules[childVectorIndex].get();

    childVectorIndex -= m_importRules.size();
    return m_childRules[childVectorIndex].get();
}

unsigned StyleSheetContents::ruleCount() const
{
    unsigned result = 0;
    result += hasCharsetRule() ? 1 : 0;
    result += m_importRules.size();
    result += m_childRules.size();
    return result;
}

void StyleSheetContents::clearCharsetRule()
{
    m_encodingFromCharsetRule = String();
}

void StyleSheetContents::clearRules()
{
    for (unsigned i = 0; i < m_importRules.size(); ++i) {
        ASSERT(m_importRules.at(i)->parentStyleSheet() == this);
        m_importRules[i]->clearParentStyleSheet();
    }
    m_importRules.clear();
    m_childRules.clear();
    clearCharsetRule();
}

void StyleSheetContents::parserSetEncodingFromCharsetRule(const String& encoding)
{
    // Parser enforces that there is ever only one @charset.
    ASSERT(m_encodingFromCharsetRule.isNull());
    m_encodingFromCharsetRule = encoding; 
}

bool StyleSheetContents::wrapperInsertRule(PassRefPtr<StyleRuleBase> rule, unsigned index)
{
    ASSERT(m_isMutable);
    ASSERT_WITH_SECURITY_IMPLICATION(index <= ruleCount());
    // Parser::parseRule doesn't currently allow @charset so we don't need to deal with it.
    ASSERT(!rule->isCharsetRule());
    
    unsigned childVectorIndex = index;
    // m_childRules does not contain @charset which is always in index 0 if it exists.
    if (hasCharsetRule()) {
        if (childVectorIndex == 0) {
            // Nothing can be inserted before @charset.
            return false;
        }
        --childVectorIndex;
    }
    
    if (childVectorIndex < m_importRules.size() || (childVectorIndex == m_importRules.size() && rule->isImportRule())) {
        // Inserting non-import rule before @import is not allowed.
        if (!rule->isImportRule())
            return false;
        m_importRules.insert(childVectorIndex, static_cast<StyleRuleImport*>(rule.get()));
        m_importRules[childVectorIndex]->setParentStyleSheet(this);
        m_importRules[childVectorIndex]->requestStyleSheet();
        // FIXME: Stylesheet doesn't actually change meaningfully before the imported sheets are loaded.
        return true;
    }
    // Inserting @import rule after a non-import rule is not allowed.
    if (rule->isImportRule())
        return false;
    childVectorIndex -= m_importRules.size();
 
    m_childRules.insert(childVectorIndex, rule);
    return true;
}

void StyleSheetContents::wrapperDeleteRule(unsigned index)
{
    ASSERT(m_isMutable);
    ASSERT_WITH_SECURITY_IMPLICATION(index < ruleCount());

    unsigned childVectorIndex = index;
    if (hasCharsetRule()) {
        if (childVectorIndex == 0) {
            clearCharsetRule();
            return;
        }
        --childVectorIndex;
    }
    if (childVectorIndex < m_importRules.size()) {
        m_importRules[childVectorIndex]->clearParentStyleSheet();
        m_importRules.remove(childVectorIndex);
        return;
    }
    childVectorIndex -= m_importRules.size();

    m_childRules.remove(childVectorIndex);
}

void StyleSheetContents::parserAddNamespace(const AtomicString& prefix, const AtomicString& uri)
{
    if (uri.isNull() || prefix.isNull())
        return;
    PrefixNamespaceURIMap::AddResult result = m_namespaces.add(prefix, uri);
    if (result.isNewEntry)
        return;
    result.iterator->value = uri;
}

const AtomicString& StyleSheetContents::determineNamespace(const AtomicString& prefix)
{
    if (prefix.isNull())
        return nullAtom; // No namespace. If an element/attribute has a namespace, we won't match it.
    if (prefix == starAtom)
        return starAtom; // We'll match any namespace.
    PrefixNamespaceURIMap::const_iterator it = m_namespaces.find(prefix);
    if (it == m_namespaces.end())
        return nullAtom;
    return it->value;
}

void StyleSheetContents::parseAuthorStyleSheet(const CachedCSSStyleSheet* cachedStyleSheet, const SecurityOrigin* securityOrigin)
{
    // Check to see if we should enforce the MIME type of the CSS resource in strict mode.
    // Running in iWeb 2 is one example of where we don't want to - <rdar://problem/6099748>
    bool enforceMIMEType = isStrictParserMode(m_parserContext.mode) && m_parserContext.enforcesCSSMIMETypeInNoQuirksMode;
    bool hasValidMIMEType = false;
    String sheetText = cachedStyleSheet->sheetText(enforceMIMEType, &hasValidMIMEType);

    CSSParser p(parserContext());
    p.parseSheet(this, sheetText, 0, 0, true);

    // If we're loading a stylesheet cross-origin, and the MIME type is not standard, require the CSS
    // to at least start with a syntactically valid CSS rule.
    // This prevents an attacker playing games by injecting CSS strings into HTML, XML, JSON, etc. etc.
    if (!hasValidMIMEType && !hasSyntacticallyValidCSSHeader()) {
        bool isCrossOriginCSS = !securityOrigin || !securityOrigin->canRequest(baseURL());
        if (isCrossOriginCSS) {
            clearRules();
            return;
        }
    }
    if (m_parserContext.needsSiteSpecificQuirks && isStrictParserMode(m_parserContext.mode)) {
        // Work around <https://bugs.webkit.org/show_bug.cgi?id=28350>.
        DEFINE_STATIC_LOCAL(const String, mediaWikiKHTMLFixesStyleSheet, (ASCIILiteral("/* KHTML fix stylesheet */\n/* work around the horizontal scrollbars */\n#column-content { margin-left: 0; }\n\n")));
        // There are two variants of KHTMLFixes.css. One is equal to mediaWikiKHTMLFixesStyleSheet,
        // while the other lacks the second trailing newline.
        if (baseURL().string().endsWith("/KHTMLFixes.css") && !sheetText.isNull() && mediaWikiKHTMLFixesStyleSheet.startsWith(sheetText)
            && sheetText.length() >= mediaWikiKHTMLFixesStyleSheet.length() - 1)
            clearRules();
    }
}

bool StyleSheetContents::parseString(const String& sheetText)
{
    return parseStringAtLine(sheetText, 0, false);
}

bool StyleSheetContents::parseStringAtLine(const String& sheetText, int startLineNumber, bool createdByParser)
{
    CSSParser p(parserContext());
    p.parseSheet(this, sheetText, startLineNumber, 0, createdByParser);

    return true;
}

bool StyleSheetContents::isLoading() const
{
    for (unsigned i = 0; i < m_importRules.size(); ++i) {
        if (m_importRules[i]->isLoading())
            return true;
    }
    return false;
}

void StyleSheetContents::checkLoaded()
{
    if (isLoading())
        return;

    RefPtr<StyleSheetContents> protect(this);

    // Avoid |this| being deleted by scripts that run via
    // ScriptableDocumentParser::executeScriptsWaitingForStylesheets().
    // See <rdar://problem/6622300>.
    RefPtr<StyleSheetContents> protector(this);
    StyleSheetContents* parentSheet = parentStyleSheet();
    if (parentSheet) {
        parentSheet->checkLoaded();
        m_loadCompleted = true;
        return;
    }
    RefPtr<Node> ownerNode = singleOwnerNode();
    if (!ownerNode) {
        m_loadCompleted = true;
        return;
    }
    m_loadCompleted = ownerNode->sheetLoaded();
    if (m_loadCompleted)
        ownerNode->notifyLoadedSheetAndAllCriticalSubresources(m_didLoadErrorOccur);
}

void StyleSheetContents::notifyLoadedSheet(const CachedCSSStyleSheet* sheet)
{
    ASSERT(sheet);
    m_didLoadErrorOccur |= sheet->errorOccurred();
}

void StyleSheetContents::startLoadingDynamicSheet()
{
    if (Node* owner = singleOwnerNode())
        owner->startLoadingDynamicSheet();
}

StyleSheetContents* StyleSheetContents::rootStyleSheet() const
{
    const StyleSheetContents* root = this;
    while (root->parentStyleSheet())
        root = root->parentStyleSheet();
    return const_cast<StyleSheetContents*>(root);
}

Node* StyleSheetContents::singleOwnerNode() const
{
    StyleSheetContents* root = rootStyleSheet();
    if (root->m_clients.isEmpty())
        return 0;
    ASSERT(root->m_clients.size() == 1);
    return root->m_clients[0]->ownerNode();
}

Document* StyleSheetContents::singleOwnerDocument() const
{
    Node* ownerNode = singleOwnerNode();
    return ownerNode ? ownerNode->document() : 0;
}

KURL StyleSheetContents::completeURL(const String& url) const
{
    return CSSParser::completeURL(m_parserContext, url);
}

void StyleSheetContents::addSubresourceStyleURLs(ListHashSet<KURL>& urls)
{
    Deque<StyleSheetContents*> styleSheetQueue;
    styleSheetQueue.append(this);

    while (!styleSheetQueue.isEmpty()) {
        StyleSheetContents* styleSheet = styleSheetQueue.takeFirst();
        
        for (unsigned i = 0; i < styleSheet->m_importRules.size(); ++i) {
            StyleRuleImport* importRule = styleSheet->m_importRules[i].get();
            if (importRule->styleSheet()) {
                styleSheetQueue.append(importRule->styleSheet());
                addSubresourceURL(urls, importRule->styleSheet()->baseURL());
            }
        }
        for (unsigned i = 0; i < styleSheet->m_childRules.size(); ++i) {
            StyleRuleBase* rule = styleSheet->m_childRules[i].get();
            if (rule->isStyleRule())
                static_cast<StyleRule*>(rule)->properties()->addSubresourceStyleURLs(urls, this);
            else if (rule->isFontFaceRule())
                static_cast<StyleRuleFontFace*>(rule)->properties()->addSubresourceStyleURLs(urls, this);
        }
    }
}

static bool childRulesHaveFailedOrCanceledSubresources(const Vector<RefPtr<StyleRuleBase> >& rules)
{
    for (unsigned i = 0; i < rules.size(); ++i) {
        const StyleRuleBase* rule = rules[i].get();
        switch (rule->type()) {
        case StyleRuleBase::Style:
            if (static_cast<const StyleRule*>(rule)->properties()->hasFailedOrCanceledSubresources())
                return true;
            break;
        case StyleRuleBase::FontFace:
            if (static_cast<const StyleRuleFontFace*>(rule)->properties()->hasFailedOrCanceledSubresources())
                return true;
            break;
        case StyleRuleBase::Media:
            if (childRulesHaveFailedOrCanceledSubresources(static_cast<const StyleRuleMedia*>(rule)->childRules()))
                return true;
            break;
        case StyleRuleBase::Region:
            if (childRulesHaveFailedOrCanceledSubresources(static_cast<const StyleRuleRegion*>(rule)->childRules()))
                return true;
            break;
#if ENABLE(SHADOW_DOM)
        case StyleRuleBase::HostInternal:
            if (childRulesHaveFailedOrCanceledSubresources(static_cast<const StyleRuleHost*>(rule)->childRules()))
                return true;
            break;
#endif
        case StyleRuleBase::Import:
            ASSERT_NOT_REACHED();
        case StyleRuleBase::Page:
        case StyleRuleBase::Keyframes:
        case StyleRuleBase::Unknown:
        case StyleRuleBase::Charset:
        case StyleRuleBase::Keyframe:
#if ENABLE(CSS3_CONDITIONAL_RULES)
        case StyleRuleBase::Supports:
#endif
#if ENABLE(CSS_DEVICE_ADAPTATION)
        case StyleRuleBase::Viewport:
#endif
#if ENABLE(CSS_SHADERS)
        case StyleRuleBase::Filter:
#endif
            break;
        }
    }
    return false;
}

bool StyleSheetContents::hasFailedOrCanceledSubresources() const
{
    ASSERT(isCacheable());
    return childRulesHaveFailedOrCanceledSubresources(m_childRules);
}

StyleSheetContents* StyleSheetContents::parentStyleSheet() const
{
    return m_ownerRule ? m_ownerRule->parentStyleSheet() : 0;
}

void StyleSheetContents::registerClient(CSSStyleSheet* sheet)
{
    ASSERT(!m_clients.contains(sheet));
    m_clients.append(sheet);
}

void StyleSheetContents::unregisterClient(CSSStyleSheet* sheet)
{
    size_t position = m_clients.find(sheet);
    ASSERT(position != notFound);
    m_clients.remove(position);
}

void StyleSheetContents::addedToMemoryCache()
{
    ASSERT(!m_isInMemoryCache);
    ASSERT(isCacheable());
    m_isInMemoryCache = true;
}

void StyleSheetContents::removedFromMemoryCache()
{
    ASSERT(m_isInMemoryCache);
    ASSERT(isCacheable());
    m_isInMemoryCache = false;
}

void StyleSheetContents::shrinkToFit()
{
    m_importRules.shrinkToFit();
    m_childRules.shrinkToFit();
}

}
