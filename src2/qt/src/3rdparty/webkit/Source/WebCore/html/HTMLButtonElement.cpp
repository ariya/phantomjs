/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2007 Samuel Weinig (sam@webkit.org)
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

#include "config.h"
#include "HTMLButtonElement.h"

#include "Attribute.h"
#include "EventNames.h"
#include "FormDataList.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"
#include "KeyboardEvent.h"
#include "RenderButton.h"
#include "ScriptEventListener.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

using namespace HTMLNames;

inline HTMLButtonElement::HTMLButtonElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
    : HTMLFormControlElement(tagName, document, form)
    , m_type(SUBMIT)
    , m_isActivatedSubmit(false)
{
    ASSERT(hasTagName(buttonTag));
}

PassRefPtr<HTMLButtonElement> HTMLButtonElement::create(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
{
    return adoptRef(new HTMLButtonElement(tagName, document, form));
}

RenderObject* HTMLButtonElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderButton(this);
}

const AtomicString& HTMLButtonElement::formControlType() const
{
    switch (m_type) {
        case SUBMIT: {
            DEFINE_STATIC_LOCAL(const AtomicString, submit, ("submit"));
            return submit;
        }
        case BUTTON: {
            DEFINE_STATIC_LOCAL(const AtomicString, button, ("button"));
            return button;
        }
        case RESET: {
            DEFINE_STATIC_LOCAL(const AtomicString, reset, ("reset"));
            return reset;
        }
    }

    ASSERT_NOT_REACHED();
    return emptyAtom;
}

void HTMLButtonElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == typeAttr) {
        if (equalIgnoringCase(attr->value(), "reset"))
            m_type = RESET;
        else if (equalIgnoringCase(attr->value(), "button"))
            m_type = BUTTON;
        else
            m_type = SUBMIT;
        setNeedsWillValidateCheck();
    } else if (attr->name() == alignAttr) {
        // Don't map 'align' attribute.  This matches what Firefox and IE do, but not Opera.
        // See http://bugs.webkit.org/show_bug.cgi?id=12071
    } else
        HTMLFormControlElement::parseMappedAttribute(attr);
}

void HTMLButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().DOMActivateEvent && !disabled()) {
        if (form() && m_type == SUBMIT) {
            m_isActivatedSubmit = true;
            form()->prepareForSubmission(event);
            m_isActivatedSubmit = false; // Do this in case submission was canceled.
        }
        if (form() && m_type == RESET)
            form()->reset();
    }

    if (event->isKeyboardEvent()) {
        if (event->type() == eventNames().keydownEvent && static_cast<KeyboardEvent*>(event)->keyIdentifier() == "U+0020") {
            setActive(true, true);
            // No setDefaultHandled() - IE dispatches a keypress in this case.
            return;
        }
        if (event->type() == eventNames().keypressEvent) {
            switch (static_cast<KeyboardEvent*>(event)->charCode()) {
                case '\r':
                    dispatchSimulatedClick(event);
                    event->setDefaultHandled();
                    return;
                case ' ':
                    // Prevent scrolling down the page.
                    event->setDefaultHandled();
                    return;
            }
        }
        if (event->type() == eventNames().keyupEvent && static_cast<KeyboardEvent*>(event)->keyIdentifier() == "U+0020") {
            if (active())
                dispatchSimulatedClick(event);
            event->setDefaultHandled();
            return;
        }
    }

    HTMLFormControlElement::defaultEventHandler(event);
}

bool HTMLButtonElement::isSuccessfulSubmitButton() const
{
    // HTML spec says that buttons must have names to be considered successful.
    // However, other browsers do not impose this constraint.
    return m_type == SUBMIT && !disabled();
}

bool HTMLButtonElement::isActivatedSubmit() const
{
    return m_isActivatedSubmit;
}

void HTMLButtonElement::setActivatedSubmit(bool flag)
{
    m_isActivatedSubmit = flag;
}

bool HTMLButtonElement::appendFormData(FormDataList& formData, bool)
{
    if (m_type != SUBMIT || name().isEmpty() || !m_isActivatedSubmit)
        return false;
    formData.appendData(name(), value());
    return true;
}

void HTMLButtonElement::accessKeyAction(bool sendToAnyElement)
{
    focus();
    // send the mouse button events iff the caller specified sendToAnyElement
    dispatchSimulatedClick(0, sendToAnyElement);
}

bool HTMLButtonElement::isURLAttribute(Attribute* attr) const
{
    return attr->name() == formactionAttr;
}

String HTMLButtonElement::value() const
{
    return getAttribute(valueAttr);
}

bool HTMLButtonElement::recalcWillValidate() const
{
    return m_type == SUBMIT && HTMLFormControlElement::recalcWillValidate();
}

} // namespace
