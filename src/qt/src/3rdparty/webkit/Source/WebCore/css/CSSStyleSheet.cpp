/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2006, 2007 Apple Inc. All rights reserved.
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
#include "CSSStyleSheet.h"

#include "CSSImportRule.h"
#include "CSSNamespace.h"
#include "CSSParser.h"
#include "CSSRuleList.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "HTMLNames.h"
#include "Node.h"
#include "SVGNames.h"
#include "SecurityOrigin.h"
#include "TextEncoding.h"
#include <wtf/Deque.h>

namespace WebCore {

#if !ASSERT_DISABLED
static bool isAcceptableCSSStyleSheetParent(Node* parentNode)
{
    // Only these nodes can be parents of StyleSheets, and they need to call clearOwnerNode() when moved out of document.
    return !parentNode
        || parentNode->isDocumentNode()
        || parentNode->hasTagName(HTMLNames::linkTag)
        || parentNode->hasTagName(HTMLNames::styleTag)
#if ENABLE(SVG)
        || parentNode->hasTagName(SVGNames::styleTag)
#endif    
        || parentNode->nodeType() == Node::PROCESSING_INSTRUCTION_NODE;
}
#endif

CSSStyleSheet::CSSStyleSheet(CSSStyleSheet* parentSheet, const String& href, const KURL& baseURL, const String& charset)
    : StyleSheet(parentSheet, href, baseURL)
    , m_charset(charset)
    , m_loadCompleted(false)
    , m_strictParsing(!parentSheet || parentSheet->useStrictParsing())
    , m_isUserStyleSheet(parentSheet ? parentSheet->isUserStyleSheet() : false)
    , m_hasSyntacticallyValidCSSHeader(true)
{
}

CSSStyleSheet::CSSStyleSheet(Node* parentNode, const String& href, const KURL& baseURL, const String& charset)
    : StyleSheet(parentNode, href, baseURL)
    , m_charset(charset)
    , m_loadCompleted(false)
    , m_strictParsing(false)
    , m_isUserStyleSheet(false)
    , m_hasSyntacticallyValidCSSHeader(true)
{
    ASSERT(isAcceptableCSSStyleSheetParent(parentNode));
}

CSSStyleSheet::CSSStyleSheet(CSSRule* ownerRule, const String& href, const KURL& baseURL, const String& charset)
    : StyleSheet(ownerRule, href, baseURL)
    , m_charset(charset)
    , m_loadCompleted(false)
    , m_strictParsing(!ownerRule || ownerRule->useStrictParsing())
    , m_hasSyntacticallyValidCSSHeader(true)
{
    CSSStyleSheet* parentSheet = ownerRule ? ownerRule->parentStyleSheet() : 0;
    m_isUserStyleSheet = parentSheet ? parentSheet->isUserStyleSheet() : false;
}

CSSStyleSheet::~CSSStyleSheet()
{
}

CSSRule *CSSStyleSheet::ownerRule() const
{
    return (parent() && parent()->isRule()) ? static_cast<CSSRule*>(parent()) : 0;
}

unsigned CSSStyleSheet::insertRule(const String& rule, unsigned index, ExceptionCode& ec)
{
    ec = 0;
    if (index > length()) {
        ec = INDEX_SIZE_ERR;
        return 0;
    }
    CSSParser p(useStrictParsing());
    RefPtr<CSSRule> r = p.parseRule(this, rule);

    if (!r) {
        ec = SYNTAX_ERR;
        return 0;
    }

    // Throw a HIERARCHY_REQUEST_ERR exception if the rule cannot be inserted at the specified index.  The best
    // example of this is an @import rule inserted after regular rules.
    if (index > 0) {
        if (r->isImportRule()) {
            // Check all the rules that come before this one to make sure they are only @charset and @import rules.
            for (unsigned i = 0; i < index; ++i) {
                if (!item(i)->isCharsetRule() && !item(i)->isImportRule()) {
                    ec = HIERARCHY_REQUEST_ERR;
                    return 0;
                }
            }
        } else if (r->isCharsetRule()) {
            // The @charset rule has to come first and there can be only one.
            ec = HIERARCHY_REQUEST_ERR;
            return 0;
        }
    }

    insert(index, r.release());
    
    styleSheetChanged();
    
    return index;
}

int CSSStyleSheet::addRule(const String& selector, const String& style, int index, ExceptionCode& ec)
{
    insertRule(selector + " { " + style + " }", index, ec);

    // As per Microsoft documentation, always return -1.
    return -1;
}

int CSSStyleSheet::addRule(const String& selector, const String& style, ExceptionCode& ec)
{
    return addRule(selector, style, length(), ec);
}

PassRefPtr<CSSRuleList> CSSStyleSheet::cssRules(bool omitCharsetRules)
{
    KURL url = finalURL();
    if (!url.isEmpty() && document() && !document()->securityOrigin()->canRequest(url))
        return 0;
    return CSSRuleList::create(this, omitCharsetRules);
}

void CSSStyleSheet::deleteRule(unsigned index, ExceptionCode& ec)
{
    if (index >= length()) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    ec = 0;
    item(index)->setParent(0);
    remove(index);
    styleSheetChanged();
}

void CSSStyleSheet::addNamespace(CSSParser* p, const AtomicString& prefix, const AtomicString& uri)
{
    if (uri.isNull())
        return;

    m_namespaces = adoptPtr(new CSSNamespace(prefix, uri, m_namespaces.release()));
    
    if (prefix.isEmpty())
        // Set the default namespace on the parser so that selectors that omit namespace info will
        // be able to pick it up easily.
        p->m_defaultNamespace = uri;
}

const AtomicString& CSSStyleSheet::determineNamespace(const AtomicString& prefix)
{
    if (prefix.isNull())
        return nullAtom; // No namespace. If an element/attribute has a namespace, we won't match it.
    if (prefix == starAtom)
        return starAtom; // We'll match any namespace.
    if (m_namespaces) {
        if (CSSNamespace* namespaceForPrefix = m_namespaces->namespaceForPrefix(prefix))
            return namespaceForPrefix->uri;
    }
    return nullAtom; // Assume we won't match any namespaces.
}

bool CSSStyleSheet::parseString(const String &string, bool strict)
{
    return parseStringAtLine(string, strict, 0);
}

bool CSSStyleSheet::parseStringAtLine(const String& string, bool strict, int startLineNumber)
{
    setStrictParsing(strict);
    CSSParser p(strict);
    p.parseSheet(this, string, startLineNumber);
    return true;
}

bool CSSStyleSheet::isLoading()
{
    unsigned len = length();
    for (unsigned i = 0; i < len; ++i) {
        StyleBase* rule = item(i);
        if (rule->isImportRule() && static_cast<CSSImportRule*>(rule)->isLoading())
            return true;
    }
    return false;
}

void CSSStyleSheet::checkLoaded()
{
    if (isLoading())
        return;

    // Avoid |this| being deleted by scripts that run via
    // ScriptableDocumentParser::executeScriptsWaitingForStylesheets().
    // See <rdar://problem/6622300>.
    RefPtr<CSSStyleSheet> protector(this);
    if (parent())
        parent()->checkLoaded();
    m_loadCompleted = ownerNode() ? ownerNode()->sheetLoaded() : true;
}

Document* CSSStyleSheet::document()
{
    StyleBase* styleObject = this;
    while (styleObject) {
        if (styleObject->isCSSStyleSheet()) {
            Node* ownerNode = static_cast<CSSStyleSheet*>(styleObject)->ownerNode();
            if (ownerNode)
                return ownerNode->document();
        }
        if (styleObject->isRule())
            styleObject = static_cast<CSSRule*>(styleObject)->parentStyleSheet();
        else
            styleObject = styleObject->parent();
    }

    return 0;
}

void CSSStyleSheet::styleSheetChanged()
{
    StyleBase* root = this;
    while (StyleBase* parent = root->parent())
        root = parent;
    Document* documentToUpdate = root->isCSSStyleSheet() ? static_cast<CSSStyleSheet*>(root)->document() : 0;
    
    /* FIXME: We don't need to do everything updateStyleSelector does,
     * basically we just need to recreate the document's selector with the
     * already existing style sheets.
     */
    if (documentToUpdate)
        documentToUpdate->styleSelectorChanged(DeferRecalcStyle);
}

KURL CSSStyleSheet::completeURL(const String& url) const
{
    // Always return a null URL when passed a null string.
    // FIXME: Should we change the KURL constructor to have this behavior?
    // See also Document::completeURL(const String&)
    if (url.isNull() || m_charset.isEmpty())
        return StyleSheet::completeURL(url);
    const TextEncoding encoding = TextEncoding(m_charset);
    return KURL(baseURL(), url, encoding);
}

void CSSStyleSheet::addSubresourceStyleURLs(ListHashSet<KURL>& urls)
{
    Deque<CSSStyleSheet*> styleSheetQueue;
    styleSheetQueue.append(this);

    while (!styleSheetQueue.isEmpty()) {
        CSSStyleSheet* styleSheet = styleSheetQueue.takeFirst();

        for (unsigned i = 0; i < styleSheet->length(); ++i) {
            StyleBase* styleBase = styleSheet->item(i);
            if (!styleBase->isRule())
                continue;
            
            CSSRule* rule = static_cast<CSSRule*>(styleBase);
            if (rule->isImportRule()) {
                if (CSSStyleSheet* ruleStyleSheet = static_cast<CSSImportRule*>(rule)->styleSheet())
                    styleSheetQueue.append(ruleStyleSheet);
            }
            rule->addSubresourceStyleURLs(urls);
        }
    }
}

}
