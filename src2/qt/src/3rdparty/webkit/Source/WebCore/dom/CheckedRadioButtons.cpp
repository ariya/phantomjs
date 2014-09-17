/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#include "CheckedRadioButtons.h"

#include "HTMLInputElement.h"

namespace WebCore {

void CheckedRadioButtons::addButton(HTMLFormControlElement* element)
{
    // We only want to add radio buttons.
    if (!element->isRadioButton())
        return;

    // Without a name, there is no group.
    if (element->name().isEmpty())
        return;

    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(element);

    // We only track checked buttons.
    if (!inputElement->checked())
        return;

    if (!m_nameToCheckedRadioButtonMap)
        m_nameToCheckedRadioButtonMap = adoptPtr(new NameToInputMap);

    pair<NameToInputMap::iterator, bool> result = m_nameToCheckedRadioButtonMap->add(element->name().impl(), inputElement);
    if (result.second)
        return;
    
    HTMLInputElement* oldCheckedButton = result.first->second;
    if (oldCheckedButton == inputElement)
        return;

    result.first->second = inputElement;
    oldCheckedButton->setChecked(false);
}

HTMLInputElement* CheckedRadioButtons::checkedButtonForGroup(const AtomicString& name) const
{
    if (!m_nameToCheckedRadioButtonMap)
        return 0;

    m_nameToCheckedRadioButtonMap->checkConsistency();
    
    return m_nameToCheckedRadioButtonMap->get(name.impl());
}

void CheckedRadioButtons::removeButton(HTMLFormControlElement* element)
{
    if (element->name().isEmpty() || !m_nameToCheckedRadioButtonMap)
        return;
    
    m_nameToCheckedRadioButtonMap->checkConsistency();

    NameToInputMap::iterator it = m_nameToCheckedRadioButtonMap->find(element->name().impl());
    if (it == m_nameToCheckedRadioButtonMap->end() || it->second != element)
        return;
    
    InputElement* inputElement = element->toInputElement();
    ASSERT_UNUSED(inputElement, inputElement);
    ASSERT(inputElement->isChecked());
    ASSERT(element->isRadioButton());

    m_nameToCheckedRadioButtonMap->remove(it);
    if (m_nameToCheckedRadioButtonMap->isEmpty())
        m_nameToCheckedRadioButtonMap.clear();
}

} // namespace
