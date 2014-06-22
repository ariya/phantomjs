/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef DocumentRuleSets_h
#define DocumentRuleSets_h

#include "RuleFeature.h"
#include "RuleSet.h"

#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class CSSStyleRule;
class CSSStyleSheet;
class DocumentStyleSheetCollection;
class InspectorCSSOMWrappers;
class MatchRequest;
class MediaQueryEvaluator;
class RuleSet;
class StyleScopeResolver;

class DocumentRuleSets {
public:
    DocumentRuleSets();
    ~DocumentRuleSets();
    RuleSet* authorStyle() const { return m_authorStyle.get(); }
    RuleSet* userStyle() const { return m_userStyle.get(); }
    RuleFeatureSet& features() { return m_features; }
    const RuleFeatureSet& features() const { return m_features; }
    RuleSet* sibling() const { return m_siblingRuleSet.get(); }
    RuleSet* uncommonAttribute() const { return m_uncommonAttributeRuleSet.get(); }

    void initUserStyle(DocumentStyleSheetCollection*, const MediaQueryEvaluator&, StyleResolver&);
    void resetAuthorStyle();
    void appendAuthorStyleSheets(unsigned firstNew, const Vector<RefPtr<CSSStyleSheet> >&, MediaQueryEvaluator*, InspectorCSSOMWrappers&, bool isViewSource, StyleResolver*);

    void collectFeatures(bool isViewSource, StyleScopeResolver*);

private:
    void collectRulesFromUserStyleSheets(const Vector<RefPtr<CSSStyleSheet> >&, RuleSet& userStyle, const MediaQueryEvaluator&, StyleResolver&);
    OwnPtr<RuleSet> m_authorStyle;
    OwnPtr<RuleSet> m_userStyle;
    RuleFeatureSet m_features;
    OwnPtr<RuleSet> m_siblingRuleSet;
    OwnPtr<RuleSet> m_uncommonAttributeRuleSet;
};

} // namespace WebCore

#endif // DocumentRuleSets_h
