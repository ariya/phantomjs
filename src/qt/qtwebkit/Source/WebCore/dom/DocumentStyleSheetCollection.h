/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 *
 */

#ifndef DocumentStyleSheetCollection_h
#define DocumentStyleSheetCollection_h

#include <wtf/FastAllocBase.h>
#include <wtf/ListHashSet.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CSSStyleSheet;
class Document;
class Node;
class StyleSheet;
class StyleSheetContents;
class StyleSheetList;

class DocumentStyleSheetCollection {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<DocumentStyleSheetCollection> create(Document* document) { return adoptPtr(new DocumentStyleSheetCollection(document)); }

    ~DocumentStyleSheetCollection();

    const Vector<RefPtr<StyleSheet> >& styleSheetsForStyleSheetList() const { return m_styleSheetsForStyleSheetList; }

    const Vector<RefPtr<CSSStyleSheet> >& activeAuthorStyleSheets() const { return m_activeAuthorStyleSheets; }

    CSSStyleSheet* pageUserSheet();
    const Vector<RefPtr<CSSStyleSheet> >& documentUserStyleSheets() const { return m_userStyleSheets; }
    const Vector<RefPtr<CSSStyleSheet> >& documentAuthorStyleSheets() const { return m_authorStyleSheets; }
    const Vector<RefPtr<CSSStyleSheet> >& injectedUserStyleSheets() const;
    const Vector<RefPtr<CSSStyleSheet> >& injectedAuthorStyleSheets() const;

    void addStyleSheetCandidateNode(Node*, bool createdByParser);
    void removeStyleSheetCandidateNode(Node*);

    void clearPageUserSheet();
    void updatePageUserSheet();
    void invalidateInjectedStyleSheetCache();
    void updateInjectedStyleSheetCache() const;

    void addAuthorSheet(PassRefPtr<StyleSheetContents> authorSheet);
    void addUserSheet(PassRefPtr<StyleSheetContents> userSheet);

    bool needsUpdateActiveStylesheetsOnStyleRecalc() const { return m_needsUpdateActiveStylesheetsOnStyleRecalc; }

    enum UpdateFlag { FullUpdate, OptimizedUpdate };
    bool updateActiveStyleSheets(UpdateFlag);

    String preferredStylesheetSetName() const { return m_preferredStylesheetSetName; }
    String selectedStylesheetSetName() const { return m_selectedStylesheetSetName; }
    void setPreferredStylesheetSetName(const String& name) { m_preferredStylesheetSetName = name; }
    void setSelectedStylesheetSetName(const String& name) { m_selectedStylesheetSetName = name; }

    void addPendingSheet() { m_pendingStylesheets++; }
    enum RemovePendingSheetNotificationType {
        RemovePendingSheetNotifyImmediately,
        RemovePendingSheetNotifyLater
    };
    void removePendingSheet(RemovePendingSheetNotificationType = RemovePendingSheetNotifyImmediately);

    bool hasPendingSheets() const { return m_pendingStylesheets > 0; }

    bool usesSiblingRules() const { return m_usesSiblingRules || m_usesSiblingRulesOverride; }
    void setUsesSiblingRulesOverride(bool b) { m_usesSiblingRulesOverride = b; }
    bool usesFirstLineRules() const { return m_usesFirstLineRules; }
    bool usesFirstLetterRules() const { return m_usesFirstLetterRules; }
    void setUsesFirstLetterRules(bool b) { m_usesFirstLetterRules = b; }
    bool usesBeforeAfterRules() const { return m_usesBeforeAfterRules || m_usesBeforeAfterRulesOverride; }
    void setUsesBeforeAfterRulesOverride(bool b) { m_usesBeforeAfterRulesOverride = b; }
    bool usesRemUnits() const { return m_usesRemUnits; }
    void setUsesRemUnit(bool b) { m_usesRemUnits = b; }

    void combineCSSFeatureFlags();
    void resetCSSFeatureFlags();

private:
    DocumentStyleSheetCollection(Document*);

    void collectActiveStyleSheets(Vector<RefPtr<StyleSheet> >&);
    enum StyleResolverUpdateType {
        Reconstruct,
        Reset,
        Additive
    };
    void analyzeStyleSheetChange(UpdateFlag, const Vector<RefPtr<CSSStyleSheet> >& newStylesheets, StyleResolverUpdateType&, bool& requiresFullStyleRecalc);

    Document* m_document;

    Vector<RefPtr<StyleSheet> > m_styleSheetsForStyleSheetList;
    Vector<RefPtr<CSSStyleSheet> > m_activeAuthorStyleSheets;

    // Track the number of currently loading top-level stylesheets needed for rendering.
    // Sheets loaded using the @import directive are not included in this count.
    // We use this count of pending sheets to detect when we can begin attaching
    // elements and when it is safe to execute scripts.
    int m_pendingStylesheets;

    RefPtr<CSSStyleSheet> m_pageUserSheet;

    mutable Vector<RefPtr<CSSStyleSheet> > m_injectedUserStyleSheets;
    mutable Vector<RefPtr<CSSStyleSheet> > m_injectedAuthorStyleSheets;
    mutable bool m_injectedStyleSheetCacheValid;

    Vector<RefPtr<CSSStyleSheet> > m_userStyleSheets;
    Vector<RefPtr<CSSStyleSheet> > m_authorStyleSheets;

    bool m_hadActiveLoadingStylesheet;
    bool m_needsUpdateActiveStylesheetsOnStyleRecalc;

    typedef ListHashSet<Node*, 32> StyleSheetCandidateListHashSet;
    StyleSheetCandidateListHashSet m_styleSheetCandidateNodes;

    String m_preferredStylesheetSetName;
    String m_selectedStylesheetSetName;

    bool m_usesSiblingRules;
    bool m_usesSiblingRulesOverride;
    bool m_usesFirstLineRules;
    bool m_usesFirstLetterRules;
    bool m_usesBeforeAfterRules;
    bool m_usesBeforeAfterRulesOverride;
    bool m_usesRemUnits;
};

}

#endif

