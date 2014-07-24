/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "HTMLContentElement.h"

#include "CSSParser.h"
#include "ContentDistributor.h"
#include "HTMLNames.h"
#include "QualifiedName.h"
#include "RuntimeEnabledFeatures.h"
#include "ShadowRoot.h"

namespace WebCore {

using HTMLNames::selectAttr;

#if ENABLE(SHADOW_DOM)

const QualifiedName& HTMLContentElement::contentTagName(Document*)
{
    if (!RuntimeEnabledFeatures::shadowDOMEnabled())
        return HTMLNames::webkitShadowContentTag;
    return HTMLNames::contentTag;
}

PassRefPtr<HTMLContentElement> HTMLContentElement::create(Document* document)
{
    return adoptRef(new HTMLContentElement(contentTagName(document), document));
}

PassRefPtr<HTMLContentElement> HTMLContentElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLContentElement(tagName, document));
}

HTMLContentElement::HTMLContentElement(const QualifiedName& name, Document* document)
    : InsertionPoint(name, document)
    , m_shouldParseSelectorList(false)
    , m_isValidSelector(true)
{
}

HTMLContentElement::~HTMLContentElement()
{
}

InsertionPoint::MatchType HTMLContentElement::matchTypeFor(Node*)
{
    if (select().isNull() || select().isEmpty())
        return AlwaysMatches;
    if (!isSelectValid())
        return NeverMatches;
    return HasToMatchSelector;
}

const AtomicString& HTMLContentElement::select() const
{
    return getAttribute(selectAttr);
}

bool HTMLContentElement::isSelectValid()
{
    ensureSelectParsed();
    return m_isValidSelector;
}

void HTMLContentElement::ensureSelectParsed()
{
    if (!m_shouldParseSelectorList)
        return;

    CSSParser parser(document());
    parser.parseSelector(select(), m_selectorList);
    m_shouldParseSelectorList = false;
    m_isValidSelector = validateSelect();
    if (!m_isValidSelector)
        m_selectorList = CSSSelectorList();
}

void HTMLContentElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == selectAttr) {
        if (ShadowRoot* root = containingShadowRoot())
            root->owner()->willAffectSelector();
        m_shouldParseSelectorList = true;
    } else
        InsertionPoint::parseAttribute(name, value);
}

static bool validateSubSelector(const CSSSelector* selector)
{
    switch (selector->m_match) {
    case CSSSelector::Tag:
    case CSSSelector::Id:
    case CSSSelector::Class:
    case CSSSelector::Exact:
    case CSSSelector::Set:
    case CSSSelector::List:
    case CSSSelector::Hyphen:
    case CSSSelector::Contain:
    case CSSSelector::Begin:
    case CSSSelector::End:
        return true;
    case CSSSelector::PseudoElement:
        return false;
    case CSSSelector::PagePseudoClass:
    case CSSSelector::PseudoClass:
        break;
    }

    switch (selector->pseudoType()) {
    case CSSSelector::PseudoEmpty:
    case CSSSelector::PseudoLink:
    case CSSSelector::PseudoVisited:
    case CSSSelector::PseudoTarget:
    case CSSSelector::PseudoEnabled:
    case CSSSelector::PseudoDisabled:
    case CSSSelector::PseudoChecked:
    case CSSSelector::PseudoIndeterminate:
    case CSSSelector::PseudoNthChild:
    case CSSSelector::PseudoNthLastChild:
    case CSSSelector::PseudoNthOfType:
    case CSSSelector::PseudoNthLastOfType:
    case CSSSelector::PseudoFirstChild:
    case CSSSelector::PseudoLastChild:
    case CSSSelector::PseudoFirstOfType:
    case CSSSelector::PseudoLastOfType:
    case CSSSelector::PseudoOnlyOfType:
        return true;
    default:
        return false;
    }
}

static bool validateSelector(const CSSSelector* selector)
{
    ASSERT(selector);

    if (!validateSubSelector(selector))
        return false;

    const CSSSelector* prevSubSelector = selector;
    const CSSSelector* subSelector = selector->tagHistory();

    while (subSelector) {
        if (prevSubSelector->relation() != CSSSelector::SubSelector)
            return false;
        if (!validateSubSelector(subSelector))
            return false;

        prevSubSelector = subSelector;
        subSelector = subSelector->tagHistory();
    }

    return true;
}

bool HTMLContentElement::validateSelect() const
{
    ASSERT(!m_shouldParseSelectorList);

    if (select().isNull() || select().isEmpty())
        return true;

    if (!m_selectorList.isValid())
        return false;

    for (const CSSSelector* selector = m_selectorList.first(); selector; selector = m_selectorList.next(selector)) {
        if (!validateSelector(selector))
            return false;
    }

    return true;
}

#endif // if ENABLE(SHADOW_DOM)

}

