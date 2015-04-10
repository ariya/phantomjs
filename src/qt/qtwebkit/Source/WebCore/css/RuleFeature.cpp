/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "RuleFeature.h"

#include "CSSSelector.h"

namespace WebCore {

void RuleFeatureSet::collectFeaturesFromSelector(const CSSSelector* selector)
{
    if (selector->m_match == CSSSelector::Id)
        idsInRules.add(selector->value().impl());
    else if (selector->m_match == CSSSelector::Class)
        classesInRules.add(selector->value().impl());
    else if (selector->isAttributeSelector())
        attrsInRules.add(selector->attribute().localName().impl());
    switch (selector->pseudoType()) {
    case CSSSelector::PseudoFirstLine:
        usesFirstLineRules = true;
        break;
    case CSSSelector::PseudoBefore:
    case CSSSelector::PseudoAfter:
        usesBeforeAfterRules = true;
        break;
    default:
        break;
    }
}

void RuleFeatureSet::add(const RuleFeatureSet& other)
{
    HashSet<AtomicStringImpl*>::const_iterator end = other.idsInRules.end();
    for (HashSet<AtomicStringImpl*>::const_iterator it = other.idsInRules.begin(); it != end; ++it)
        idsInRules.add(*it);
    end = other.classesInRules.end();
    for (HashSet<AtomicStringImpl*>::const_iterator it = other.classesInRules.begin(); it != end; ++it)
        classesInRules.add(*it);
    end = other.attrsInRules.end();
    for (HashSet<AtomicStringImpl*>::const_iterator it = other.attrsInRules.begin(); it != end; ++it)
        attrsInRules.add(*it);
    siblingRules.appendVector(other.siblingRules);
    uncommonAttributeRules.appendVector(other.uncommonAttributeRules);
    usesFirstLineRules = usesFirstLineRules || other.usesFirstLineRules;
    usesBeforeAfterRules = usesBeforeAfterRules || other.usesBeforeAfterRules;
}

void RuleFeatureSet::clear()
{
    idsInRules.clear();
    classesInRules.clear();
    attrsInRules.clear();
    siblingRules.clear();
    uncommonAttributeRules.clear();
    usesFirstLineRules = false;
    usesBeforeAfterRules = false;
}

} // namespace WebCore
