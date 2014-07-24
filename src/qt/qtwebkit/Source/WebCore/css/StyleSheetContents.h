/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2006, 2007, 2008, 2009, 2010, 2012 Apple Inc. All rights reserved.
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

#ifndef StyleSheetContents_h
#define StyleSheetContents_h

#include "CSSParserMode.h"
#include "KURL.h"
#include <wtf/HashMap.h>
#include <wtf/ListHashSet.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class CSSStyleSheet;
class CachedCSSStyleSheet;
class Document;
class Node;
class SecurityOrigin;
class StyleRuleBase;
class StyleRuleImport;

class StyleSheetContents : public RefCounted<StyleSheetContents> {
public:
    static PassRefPtr<StyleSheetContents> create(const CSSParserContext& context = CSSParserContext(CSSStrictMode))
    {
        return adoptRef(new StyleSheetContents(0, String(), context));
    }
    static PassRefPtr<StyleSheetContents> create(const String& originalURL, const CSSParserContext& context)
    {
        return adoptRef(new StyleSheetContents(0, originalURL, context));
    }
    static PassRefPtr<StyleSheetContents> create(StyleRuleImport* ownerRule, const String& originalURL, const CSSParserContext& context)
    {
        return adoptRef(new StyleSheetContents(ownerRule, originalURL, context));
    }

    ~StyleSheetContents();
    
    const CSSParserContext& parserContext() const { return m_parserContext; }

    const AtomicString& determineNamespace(const AtomicString& prefix);

    void parseAuthorStyleSheet(const CachedCSSStyleSheet*, const SecurityOrigin*);
    bool parseString(const String&);
    bool parseStringAtLine(const String&, int startLineNumber, bool);

    bool isCacheable() const;

    bool isLoading() const;

    void checkLoaded();
    void startLoadingDynamicSheet();

    StyleSheetContents* rootStyleSheet() const;
    Node* singleOwnerNode() const;
    Document* singleOwnerDocument() const;

    const String& charset() const { return m_parserContext.charset; }

    bool loadCompleted() const { return m_loadCompleted; }
    bool hasFailedOrCanceledSubresources() const;

    KURL completeURL(const String& url) const;
    void addSubresourceStyleURLs(ListHashSet<KURL>&);

    void setIsUserStyleSheet(bool b) { m_isUserStyleSheet = b; }
    bool isUserStyleSheet() const { return m_isUserStyleSheet; }
    void setHasSyntacticallyValidCSSHeader(bool b) { m_hasSyntacticallyValidCSSHeader = b; }
    bool hasSyntacticallyValidCSSHeader() const { return m_hasSyntacticallyValidCSSHeader; }

    void parserAddNamespace(const AtomicString& prefix, const AtomicString& uri);
    void parserAppendRule(PassRefPtr<StyleRuleBase>);
    void parserSetEncodingFromCharsetRule(const String& encoding); 
    void parserSetUsesRemUnits(bool b) { m_usesRemUnits = b; }

    void clearRules();

    bool hasCharsetRule() const { return !m_encodingFromCharsetRule.isNull(); }
    String encodingFromCharsetRule() const { return m_encodingFromCharsetRule; }
    // Rules other than @charset and @import.
    const Vector<RefPtr<StyleRuleBase> >& childRules() const { return m_childRules; }
    const Vector<RefPtr<StyleRuleImport> >& importRules() const { return m_importRules; }

    void notifyLoadedSheet(const CachedCSSStyleSheet*);
    
    StyleSheetContents* parentStyleSheet() const;
    StyleRuleImport* ownerRule() const { return m_ownerRule; }
    void clearOwnerRule() { m_ownerRule = 0; }
    
    // Note that href is the URL that started the redirect chain that led to
    // this style sheet. This property probably isn't useful for much except
    // the JavaScript binding (which needs to use this value for security).
    String originalURL() const { return m_originalURL; }
    const KURL& baseURL() const { return m_parserContext.baseURL; }

    unsigned ruleCount() const;
    StyleRuleBase* ruleAt(unsigned index) const;

    bool usesRemUnits() const { return m_usesRemUnits; }

    unsigned estimatedSizeInBytes() const;
    
    bool wrapperInsertRule(PassRefPtr<StyleRuleBase>, unsigned index);
    void wrapperDeleteRule(unsigned index);

    PassRefPtr<StyleSheetContents> copy() const { return adoptRef(new StyleSheetContents(*this)); }

    void registerClient(CSSStyleSheet*);
    void unregisterClient(CSSStyleSheet*);
    bool hasOneClient() { return m_clients.size() == 1; }

    bool isMutable() const { return m_isMutable; }
    void setMutable() { m_isMutable = true; }

    bool isInMemoryCache() const { return m_isInMemoryCache; }
    void addedToMemoryCache();
    void removedFromMemoryCache();

    void shrinkToFit();

private:
    StyleSheetContents(StyleRuleImport* ownerRule, const String& originalURL, const CSSParserContext&);
    StyleSheetContents(const StyleSheetContents&);

    void clearCharsetRule();

    StyleRuleImport* m_ownerRule;

    String m_originalURL;

    String m_encodingFromCharsetRule;
    Vector<RefPtr<StyleRuleImport> > m_importRules;
    Vector<RefPtr<StyleRuleBase> > m_childRules;
    typedef HashMap<AtomicString, AtomicString> PrefixNamespaceURIMap;
    PrefixNamespaceURIMap m_namespaces;

    bool m_loadCompleted : 1;
    bool m_isUserStyleSheet : 1;
    bool m_hasSyntacticallyValidCSSHeader : 1;
    bool m_didLoadErrorOccur : 1;
    bool m_usesRemUnits : 1;
    bool m_isMutable : 1;
    bool m_isInMemoryCache : 1;
    
    CSSParserContext m_parserContext;

    Vector<CSSStyleSheet*> m_clients;
};

} // namespace

#endif
