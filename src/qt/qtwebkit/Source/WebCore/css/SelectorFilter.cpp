/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
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
#include "SelectorFilter.h"

#include "CSSSelector.h"
#include "StyledElement.h"

namespace WebCore {

// Salt to separate otherwise identical string hashes so a class-selector like .article won't match <article> elements.
enum { TagNameSalt = 13, IdAttributeSalt = 17, ClassAttributeSalt = 19 };

static inline void collectElementIdentifierHashes(const Element* element, Vector<unsigned, 4>& identifierHashes)
{
    identifierHashes.append(element->localName().impl()->existingHash() * TagNameSalt);
    if (element->hasID())
        identifierHashes.append(element->idForStyleResolution().impl()->existingHash() * IdAttributeSalt);
    const StyledElement* styledElement = element->isStyledElement() ? static_cast<const StyledElement*>(element) : 0;
    if (styledElement && styledElement->hasClass()) {
        const SpaceSplitString& classNames = styledElement->classNames();
        size_t count = classNames.size();
        for (size_t i = 0; i < count; ++i)
            identifierHashes.append(classNames[i].impl()->existingHash() * ClassAttributeSalt);
    }
}

void SelectorFilter::pushParentStackFrame(Element* parent)
{
    ASSERT(m_ancestorIdentifierFilter);
    ASSERT(m_parentStack.isEmpty() || m_parentStack.last().element == parent->parentOrShadowHostElement());
    ASSERT(!m_parentStack.isEmpty() || !parent->parentOrShadowHostElement());
    m_parentStack.append(ParentStackFrame(parent));
    ParentStackFrame& parentFrame = m_parentStack.last();
    // Mix tags, class names and ids into some sort of weird bouillabaisse.
    // The filter is used for fast rejection of child and descendant selectors.
    collectElementIdentifierHashes(parent, parentFrame.identifierHashes);
    size_t count = parentFrame.identifierHashes.size();
    for (size_t i = 0; i < count; ++i)
        m_ancestorIdentifierFilter->add(parentFrame.identifierHashes[i]);
}

void SelectorFilter::popParentStackFrame()
{
    ASSERT(!m_parentStack.isEmpty());
    ASSERT(m_ancestorIdentifierFilter);
    const ParentStackFrame& parentFrame = m_parentStack.last();
    size_t count = parentFrame.identifierHashes.size();
    for (size_t i = 0; i < count; ++i)
        m_ancestorIdentifierFilter->remove(parentFrame.identifierHashes[i]);
    m_parentStack.removeLast();
    if (m_parentStack.isEmpty()) {
        ASSERT(m_ancestorIdentifierFilter->likelyEmpty());
        m_ancestorIdentifierFilter.clear();
    }
}

void SelectorFilter::setupParentStack(Element* parent)
{
    ASSERT(m_parentStack.isEmpty() == !m_ancestorIdentifierFilter);
    // Kill whatever we stored before.
    m_parentStack.shrink(0);
    m_ancestorIdentifierFilter = adoptPtr(new BloomFilter<bloomFilterKeyBits>);
    // Fast version if parent is a root element:
    if (!parent->parentOrShadowHostNode()) {
        pushParentStackFrame(parent);
        return;
    }
    // Otherwise climb up the tree.
    Vector<Element*, 30> ancestors;
    for (Element* ancestor = parent; ancestor; ancestor = ancestor->parentOrShadowHostElement())
        ancestors.append(ancestor);
    for (size_t n = ancestors.size(); n; --n)
        pushParentStackFrame(ancestors[n - 1]);
}

void SelectorFilter::pushParent(Element* parent)
{
    ASSERT(m_ancestorIdentifierFilter);
    // We may get invoked for some random elements in some wacky cases during style resolve.
    // Pause maintaining the stack in this case.
    if (m_parentStack.last().element != parent->parentOrShadowHostElement())
        return;
    pushParentStackFrame(parent);
}

static inline void collectDescendantSelectorIdentifierHashes(const CSSSelector* selector, unsigned*& hash)
{
    switch (selector->m_match) {
    case CSSSelector::Id:
        if (!selector->value().isEmpty())
            (*hash++) = selector->value().impl()->existingHash() * IdAttributeSalt;
        break;
    case CSSSelector::Class:
        if (!selector->value().isEmpty())
            (*hash++) = selector->value().impl()->existingHash() * ClassAttributeSalt;
        break;
    case CSSSelector::Tag:
        if (selector->tagQName().localName() != starAtom)
            (*hash++) = selector->tagQName().localName().impl()->existingHash() * TagNameSalt;
        break;
    default:
        break;
    }
}

void SelectorFilter::collectIdentifierHashes(const CSSSelector* selector, unsigned* identifierHashes, unsigned maximumIdentifierCount)
{
    unsigned* hash = identifierHashes;
    unsigned* end = identifierHashes + maximumIdentifierCount;
    CSSSelector::Relation relation = selector->relation();

    // Skip the topmost selector. It is handled quickly by the rule hashes.
    bool skipOverSubselectors = true;
    for (selector = selector->tagHistory(); selector; selector = selector->tagHistory()) {
        // Only collect identifiers that match ancestors.
        switch (relation) {
        case CSSSelector::SubSelector:
            if (!skipOverSubselectors)
                collectDescendantSelectorIdentifierHashes(selector, hash);
            break;
        case CSSSelector::DirectAdjacent:
        case CSSSelector::IndirectAdjacent:
        case CSSSelector::ShadowDescendant:
            skipOverSubselectors = true;
            break;
        case CSSSelector::Descendant:
        case CSSSelector::Child:
            skipOverSubselectors = false;
            collectDescendantSelectorIdentifierHashes(selector, hash);
            break;
        }
        if (hash == end)
            return;
        relation = selector->relation();
    }
    *hash = 0;
}

}

