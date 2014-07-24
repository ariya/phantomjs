/*
 * Copyright (C) 2010, 2012 Google Inc. All rights reserved.
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
#include "ValidationMessage.h"

#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "ElementShadow.h"
#include "ExceptionCodePlaceholder.h"
#include "HTMLBRElement.h"
#include "HTMLDivElement.h"
#include "HTMLFormControlElement.h"
#include "HTMLNames.h"
#include "Page.h"
#include "RenderBlock.h"
#include "RenderObject.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "StyleResolver.h"
#include "Text.h"
#include "ValidationMessageClient.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

using namespace HTMLNames;

ALWAYS_INLINE ValidationMessage::ValidationMessage(HTMLFormControlElement* element)
    : m_element(element)
{
    ASSERT(m_element);
}

ValidationMessage::~ValidationMessage()
{
    if (ValidationMessageClient* client = validationMessageClient()) {
        client->hideValidationMessage(*m_element);
        return;
    }

    deleteBubbleTree();
}

PassOwnPtr<ValidationMessage> ValidationMessage::create(HTMLFormControlElement* element)
{
    return adoptPtr(new ValidationMessage(element));
}

ValidationMessageClient* ValidationMessage::validationMessageClient() const
{
    if (Page* page = m_element->document()->page())
        return page->validationMessageClient();
    return 0;
}

void ValidationMessage::updateValidationMessage(const String& message)
{
    String updatedMessage = message;
    if (!validationMessageClient()) {
        // HTML5 specification doesn't ask UA to show the title attribute value
        // with the validationMessage. However, this behavior is same as Opera
        // and the specification describes such behavior as an example.
        const AtomicString& title = m_element->fastGetAttribute(titleAttr);
        if (!updatedMessage.isEmpty() && !title.isEmpty()) {
            updatedMessage.append('\n');
            updatedMessage.append(title);
        }
    }

    if (updatedMessage.isEmpty()) {
        requestToHideMessage();
        return;
    }
    setMessage(updatedMessage);
}

void ValidationMessage::setMessage(const String& message)
{
    if (ValidationMessageClient* client = validationMessageClient()) {
        client->showValidationMessage(*m_element, message);
        return;
    }

    // Don't modify the DOM tree in this context.
    // If so, an assertion in Element::isFocusable() fails.
    ASSERT(!message.isEmpty());
    m_message = message;
    if (!m_bubble)
        m_timer = adoptPtr(new Timer<ValidationMessage>(this, &ValidationMessage::buildBubbleTree));
    else
        m_timer = adoptPtr(new Timer<ValidationMessage>(this, &ValidationMessage::setMessageDOMAndStartTimer));
    m_timer->startOneShot(0);
}

void ValidationMessage::setMessageDOMAndStartTimer(Timer<ValidationMessage>*)
{
    ASSERT(!validationMessageClient());
    ASSERT(m_messageHeading);
    ASSERT(m_messageBody);
    m_messageHeading->removeChildren();
    m_messageBody->removeChildren();
    Vector<String> lines;
    m_message.split('\n', lines);
    Document* doc = m_messageHeading->document();
    for (unsigned i = 0; i < lines.size(); ++i) {
        if (i) {
            m_messageBody->appendChild(Text::create(doc, lines[i]), ASSERT_NO_EXCEPTION);
            if (i < lines.size() - 1)
                m_messageBody->appendChild(HTMLBRElement::create(doc), ASSERT_NO_EXCEPTION);
        } else
            m_messageHeading->setInnerText(lines[i], ASSERT_NO_EXCEPTION);
    }

    int magnification = doc->page() ? doc->page()->settings()->validationMessageTimerMagnification() : -1;
    if (magnification <= 0)
        m_timer.clear();
    else {
        m_timer = adoptPtr(new Timer<ValidationMessage>(this, &ValidationMessage::deleteBubbleTree));
        m_timer->startOneShot(max(5.0, static_cast<double>(m_message.length()) * magnification / 1000));
    }
}

static void adjustBubblePosition(const LayoutRect& hostRect, HTMLElement* bubble)
{
    ASSERT(bubble);
    if (hostRect.isEmpty())
        return;
    double hostX = hostRect.x();
    double hostY = hostRect.y();
    if (RenderObject* renderer = bubble->renderer()) {
        if (RenderBox* container = renderer->containingBlock()) {
            FloatPoint containerLocation = container->localToAbsolute();
            hostX -= containerLocation.x() + container->borderLeft();
            hostY -= containerLocation.y() + container->borderTop();
        }
    }

    bubble->setInlineStyleProperty(CSSPropertyTop, hostY + hostRect.height(), CSSPrimitiveValue::CSS_PX);
    // The 'left' value of ::-webkit-validation-bubble-arrow.
    const int bubbleArrowTopOffset = 32;
    double bubbleX = hostX;
    if (hostRect.width() / 2 < bubbleArrowTopOffset)
        bubbleX = max(hostX + hostRect.width() / 2 - bubbleArrowTopOffset, 0.0);
    bubble->setInlineStyleProperty(CSSPropertyLeft, bubbleX, CSSPrimitiveValue::CSS_PX);
}

void ValidationMessage::buildBubbleTree(Timer<ValidationMessage>*)
{
    ASSERT(!validationMessageClient());
    ShadowRoot* shadowRoot = m_element->ensureUserAgentShadowRoot();

    Document* doc = m_element->document();
    m_bubble = HTMLDivElement::create(doc);
    m_bubble->setPseudo(AtomicString("-webkit-validation-bubble", AtomicString::ConstructFromLiteral));
    // Need to force position:absolute because RenderMenuList doesn't assume it
    // contains non-absolute or non-fixed renderers as children.
    m_bubble->setInlineStyleProperty(CSSPropertyPosition, CSSValueAbsolute);
    shadowRoot->appendChild(m_bubble.get(), ASSERT_NO_EXCEPTION);
    m_element->document()->updateLayout();
    adjustBubblePosition(m_element->boundingBox(), m_bubble.get());

    RefPtr<HTMLDivElement> clipper = HTMLDivElement::create(doc);
    clipper->setPseudo(AtomicString("-webkit-validation-bubble-arrow-clipper", AtomicString::ConstructFromLiteral));
    RefPtr<HTMLDivElement> bubbleArrow = HTMLDivElement::create(doc);
    bubbleArrow->setPseudo(AtomicString("-webkit-validation-bubble-arrow", AtomicString::ConstructFromLiteral));
    clipper->appendChild(bubbleArrow.release(), ASSERT_NO_EXCEPTION);
    m_bubble->appendChild(clipper.release(), ASSERT_NO_EXCEPTION);

    RefPtr<HTMLElement> message = HTMLDivElement::create(doc);
    message->setPseudo(AtomicString("-webkit-validation-bubble-message", AtomicString::ConstructFromLiteral));
    RefPtr<HTMLElement> icon = HTMLDivElement::create(doc);
    icon->setPseudo(AtomicString("-webkit-validation-bubble-icon", AtomicString::ConstructFromLiteral));
    message->appendChild(icon.release(), ASSERT_NO_EXCEPTION);
    RefPtr<HTMLElement> textBlock = HTMLDivElement::create(doc);
    textBlock->setPseudo(AtomicString("-webkit-validation-bubble-text-block", AtomicString::ConstructFromLiteral));
    m_messageHeading = HTMLDivElement::create(doc);
    m_messageHeading->setPseudo(AtomicString("-webkit-validation-bubble-heading", AtomicString::ConstructFromLiteral));
    textBlock->appendChild(m_messageHeading, ASSERT_NO_EXCEPTION);
    m_messageBody = HTMLDivElement::create(doc);
    m_messageBody->setPseudo(AtomicString("-webkit-validation-bubble-body", AtomicString::ConstructFromLiteral));
    textBlock->appendChild(m_messageBody, ASSERT_NO_EXCEPTION);
    message->appendChild(textBlock.release(), ASSERT_NO_EXCEPTION);
    m_bubble->appendChild(message.release(), ASSERT_NO_EXCEPTION);

    setMessageDOMAndStartTimer();

    // FIXME: Use transition to show the bubble.
}

void ValidationMessage::requestToHideMessage()
{
    if (ValidationMessageClient* client = validationMessageClient()) {
        client->hideValidationMessage(*m_element);
        return;
    }

    // We must not modify the DOM tree in this context by the same reason as setMessage().
    m_timer = adoptPtr(new Timer<ValidationMessage>(this, &ValidationMessage::deleteBubbleTree));
    m_timer->startOneShot(0);
}

bool ValidationMessage::shadowTreeContains(Node* node) const
{
    if (validationMessageClient() || !m_bubble)
        return false;
    return m_bubble->treeScope() == node->treeScope();
}

void ValidationMessage::deleteBubbleTree(Timer<ValidationMessage>*)
{
    ASSERT(!validationMessageClient());
    if (m_bubble) {
        m_messageHeading = 0;
        m_messageBody = 0;
        m_element->userAgentShadowRoot()->removeChild(m_bubble.get(), ASSERT_NO_EXCEPTION);
        m_bubble = 0;
    }
    m_message = String();
}

bool ValidationMessage::isVisible() const
{
    if (ValidationMessageClient* client = validationMessageClient())
        return client->isValidationMessageVisible(*m_element);
    return !m_message.isEmpty();
}

} // namespace WebCore
