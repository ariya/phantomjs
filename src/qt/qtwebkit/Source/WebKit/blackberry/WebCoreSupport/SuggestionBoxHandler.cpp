/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
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
#include "SuggestionBoxHandler.h"

#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "DOMTokenList.h"
#include "ElementShadow.h"
#include "ExceptionCodePlaceholder.h"
#include "HTMLCollection.h"
#include "HTMLDataListElement.h"
#include "HTMLDivElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLOptionElement.h"
#include "HTMLSpanElement.h"
#include "RenderBlock.h"
#include "RenderObject.h"
#include "ShadowRoot.h"
#include "StyleResolver.h"
#include "SuggestionBoxElement.h"
#include "Text.h"

#include <BlackBerryPlatformLog.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

using namespace HTMLNames;

SuggestionBoxHandler::SuggestionBoxHandler(HTMLInputElement* element)
    : m_inputElement(element)
{
}

SuggestionBoxHandler::~SuggestionBoxHandler()
{
    hideDropdownBox();
}

PassOwnPtr<SuggestionBoxHandler> SuggestionBoxHandler::create(HTMLInputElement* element)
{
    return adoptPtr(new SuggestionBoxHandler(element));
}

void SuggestionBoxHandler::setInputElementAndUpdateDisplay(HTMLInputElement* element, bool allowEmptyPrefix)
{
    hideDropdownBox();
    m_inputElement = element;
    showDropdownBox(allowEmptyPrefix);
}

void SuggestionBoxHandler::showDropdownBox(bool allowEmptyPrefix)
{
    // Clearing suggestions and parse again
    m_suggestions.clear();
    parseSuggestions(allowEmptyPrefix);

    // Don't bother building the tree if suggestion size is 0.
    // We need to call insertSuggestions to clear previous suggestions
    // in the DOM if suggestion size is 0.
    if (!m_dropdownBox) {
        if (!m_suggestions.size())
            return;
        buildDropdownBoxTree();
    }

    insertSuggestionsToDropdownBox();
}

void SuggestionBoxHandler::hideDropdownBox()
{
    if (m_dropdownBox) {
        if (ShadowRoot* shadowRoot = m_inputElement->userAgentShadowRoot()) {
            ExceptionCode ec;
            shadowRoot->removeChild(m_dropdownBox.get(), ec);
        }
        m_dropdownBox = 0;
    }
    m_suggestions.clear();
}

void SuggestionBoxHandler::changeInputElementInnerTextValue(String& text)
{
    m_inputElement->setInnerTextValue(text);
}

void SuggestionBoxHandler::parseSuggestions(bool allowEmptyPrefix)
{
    // Return if this handler doesn't have an input element to attach to
    if (!m_inputElement)
        return;

    // List attribute exists, check if datalist exists.
    HTMLDataListElement* dList = m_inputElement->dataList();
    if (!dList)
        return;

    RefPtr<HTMLCollection> optionsList = dList->options();
    String prefix = m_inputElement->innerTextValue();

    // Exit if:
    // 1) If length of nodes in datalist is empty, return.
    // 2) If we do not allow empty prefixes and prefix is null
    if (!optionsList->length() || (!allowEmptyPrefix && prefix.isEmpty())) {
        hideDropdownBox();
        return;
    }

    // Prefix match the inner text value of the input against the candidate suggestions.
    // Make sure the length of the suggestion is smaller than max length
    for (unsigned i = 0; i < optionsList->length(); i++) {
        HTMLOptionElement* target = toHTMLOptionElement((optionsList->item(i)));
        if (!m_inputElement->isValidValue(target->value()))
            continue;
        String candidateString = m_inputElement->sanitizeValue(target->value());
        if (!candidateString.findIgnoringCase(prefix) && candidateString.length() <= (unsigned) m_inputElement->maxLength())
            m_suggestions.append(candidateString);
    }
}

void SuggestionBoxHandler::insertSuggestionsToDropdownBox()
{
    // In case someone is calling this function without successfully building the dropdownBox tree
    ASSERT(m_dropdownBox);

    Document* document = m_dropdownBox->document();

    // Clear the dropdown box's previous values
    m_dropdownBox->removeChildren();

    int prefixIndex = m_inputElement->innerTextValue().length();

    unsigned total = m_suggestions.size();
    for (unsigned i = 0; i < total; ++i) {
        RefPtr<HTMLElement> suggestionItem = SuggestionBoxElement::create(this, m_suggestions[i], document);
        if (i < total - 1)
            suggestionItem->setPseudo("-webkit-suggestion-dropdown-box-item");
        else
            suggestionItem->setPseudo("-webkit-suggestion-dropdown-box-item-last");

        // Append the part where it prefix matched
        RefPtr<HTMLSpanElement> prefixElement = HTMLSpanElement::create(HTMLNames::spanTag, document);
        prefixElement->appendChild(Text::create(document, m_suggestions[i].substring(0, prefixIndex)));
        prefixElement->setPseudo("-webkit-suggestion-prefix-text");
        suggestionItem->appendChild(prefixElement.release(), ASSERT_NO_EXCEPTION);

        // Append the rest of the text in the suggestion item
        suggestionItem->appendChild(Text::create(document, m_suggestions[i].substring(prefixIndex)), ASSERT_NO_EXCEPTION);
        m_dropdownBox->appendChild(suggestionItem.release(), ASSERT_NO_EXCEPTION);
    }
}

static void adjustDropdownBoxPosition(const LayoutRect& hostRect, HTMLElement* dropdownBox)
{
    // FIXME: reverse positioning and scroll
    ASSERT(dropdownBox);
    if (hostRect.isEmpty())
        return;
    double hostX = hostRect.x();
    double hostY = hostRect.y();
    if (RenderObject* renderer = dropdownBox->renderer()) {
        if (RenderBox* container = renderer->containingBlock()) {
            FloatPoint containerLocation = container->localToAbsolute();
            hostX -= containerLocation.x() + container->borderLeft();
            hostY -= containerLocation.y() + container->borderTop();
        }
    }

    dropdownBox->setInlineStyleProperty(CSSPropertyTop, hostY + hostRect.height(), CSSPrimitiveValue::CSS_PX);
    // The 'left' value of ::-webkit-validation-bubble-arrow.
    const int bubbleArrowTopOffset = 32;
    double bubbleX = hostX;
    if (hostRect.width() / 2 < bubbleArrowTopOffset)
        bubbleX = max(hostX + hostRect.width() / 2 - bubbleArrowTopOffset, 0.0);
    dropdownBox->setInlineStyleProperty(CSSPropertyLeft, bubbleX, CSSPrimitiveValue::CSS_PX);
}

void SuggestionBoxHandler::buildDropdownBoxTree()
{
    ShadowRoot* shadowRoot = m_inputElement->ensureUserAgentShadowRoot();

    // Return if shadowRoot is null
    if (!shadowRoot)
        return;

    Document* document = m_inputElement->document();
    m_dropdownBox = HTMLDivElement::create(document);
    m_dropdownBox->setPseudo("-webkit-suggestion-dropdown-box");
    // Need to force position:absolute because RenderMenuList doesn't assume it
    // contains non-absolute or non-fixed renderers as children.
    m_dropdownBox->setInlineStyleProperty(CSSPropertyPosition, CSSValueAbsolute);

    // Minus 2 pixels because of the 1px borders on the suggestion box.
    m_dropdownBox->setInlineStyleProperty(CSSPropertyWidth, String::number(m_inputElement->boundingBox().width().rawValue() - 2) + "px", false);
    ExceptionCode ec = 0;
    shadowRoot->appendChild(m_dropdownBox.get(), ec);
    ASSERT(!ec);
    document->updateLayout();
    adjustDropdownBoxPosition(m_inputElement->boundingBox(), m_dropdownBox.get());
}

} // namespace WebCore
