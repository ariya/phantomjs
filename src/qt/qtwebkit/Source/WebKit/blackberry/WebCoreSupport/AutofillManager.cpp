/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "AutofillManager.h"

#include "AutofillBackingStore.h"
#include "HTMLCollection.h"
#include "HTMLFormElement.h"
#include "HTMLInputElement.h"
#include "WebPage_p.h"

namespace WebCore {

static bool isAutofillable(HTMLInputElement* element)
{
    return element && element->isTextField() && !element->isPasswordField()
        && !element->isAutofilled() && element->shouldAutocomplete();
}

PassRefPtr<AutofillManager> AutofillManager::create(BlackBerry::WebKit::WebPagePrivate* page)
{
    return adoptRef(new AutofillManager(page));
}

void AutofillManager::didChangeInTextField(HTMLInputElement* element)
{
    if (!isAutofillable(element))
        return;

    if (m_element != element)
        m_element = element;

    Vector<String> candidates = autofillBackingStore().get(element->getAttribute(HTMLNames::nameAttr).string(), element->value());
    m_webPagePrivate->notifyPopupAutofillDialog(candidates);
}

void AutofillManager::textFieldDidEndEditing(HTMLInputElement*)
{
    m_webPagePrivate->notifyDismissAutofillDialog();
}

void AutofillManager::autofillTextField(const String& value)
{
    if (!m_element)
        return;

    m_element->setValue(value);
    m_element->setAutofilled();
}

void AutofillManager::saveTextFields(HTMLFormElement* form)
{
    RefPtr<HTMLCollection> elements = form->elements();
    size_t itemCount = elements->length();
    for (size_t i = 0; i < itemCount; ++i) {
        HTMLInputElement* element = form->item(i)->toInputElement();
        if (!isAutofillable(element))
            continue;
        autofillBackingStore().add(element->getAttribute(HTMLNames::nameAttr).string(), element->value());
    }
}

void AutofillManager::clear()
{
    autofillBackingStore().clear();
}

} // namespace WebCore
