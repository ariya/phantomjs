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
#include <wtf/HashSet.h>

namespace WebCore {

class RadioButtonGroup {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<RadioButtonGroup> create();
    bool isEmpty() const { return m_members.isEmpty(); }
    bool isRequired() const { return m_requiredCount; }
    HTMLInputElement* checkedButton() const { return m_checkedButton; }
    void add(HTMLInputElement*);
    void updateCheckedState(HTMLInputElement*);
    void requiredAttributeChanged(HTMLInputElement*);
    void remove(HTMLInputElement*);
    bool contains(HTMLInputElement*) const;

private:
    RadioButtonGroup();
    void setNeedsValidityCheckForAllButtons();
    bool isValid() const;
    void setCheckedButton(HTMLInputElement*);

    HashSet<HTMLInputElement*> m_members;
    HTMLInputElement* m_checkedButton;
    size_t m_requiredCount;
};

RadioButtonGroup::RadioButtonGroup()
    : m_checkedButton(0)
    , m_requiredCount(0)
{
}

PassOwnPtr<RadioButtonGroup> RadioButtonGroup::create()
{
    return adoptPtr(new RadioButtonGroup);
}

inline bool RadioButtonGroup::isValid() const
{
    return !isRequired() || m_checkedButton;
}

void RadioButtonGroup::setCheckedButton(HTMLInputElement* button)
{
    HTMLInputElement* oldCheckedButton = m_checkedButton;
    if (oldCheckedButton == button)
        return;
    m_checkedButton = button;
    if (oldCheckedButton)
        oldCheckedButton->setChecked(false);
}

void RadioButtonGroup::add(HTMLInputElement* button)
{
    ASSERT(button->isRadioButton());
    if (!m_members.add(button).isNewEntry)
        return;
    bool groupWasValid = isValid();
    if (button->isRequired())
        ++m_requiredCount;
    if (button->checked())
        setCheckedButton(button);

    bool groupIsValid = isValid();
    if (groupWasValid != groupIsValid)
        setNeedsValidityCheckForAllButtons();
    else if (!groupIsValid) {
        // A radio button not in a group is always valid. We need to make it
        // invalid only if the group is invalid.
        button->setNeedsValidityCheck();
    }
}

void RadioButtonGroup::updateCheckedState(HTMLInputElement* button)
{
    ASSERT(button->isRadioButton());
    ASSERT(m_members.contains(button));
    bool wasValid = isValid();
    if (button->checked())
        setCheckedButton(button);
    else {
        if (m_checkedButton == button)
            m_checkedButton = 0;
    }
    if (wasValid != isValid())
        setNeedsValidityCheckForAllButtons();
}

void RadioButtonGroup::requiredAttributeChanged(HTMLInputElement* button)
{
    ASSERT(button->isRadioButton());
    ASSERT(m_members.contains(button));
    bool wasValid = isValid();
    if (button->isRequired())
        ++m_requiredCount;
    else {
        ASSERT(m_requiredCount);
        --m_requiredCount;
    }
    if (wasValid != isValid())
        setNeedsValidityCheckForAllButtons();
}

void RadioButtonGroup::remove(HTMLInputElement* button)
{
    ASSERT(button->isRadioButton());
    HashSet<HTMLInputElement*>::iterator it = m_members.find(button);
    if (it == m_members.end())
        return;
    bool wasValid = isValid();
    m_members.remove(it);
    if (button->isRequired()) {
        ASSERT(m_requiredCount);
        --m_requiredCount;
    }
    if (m_checkedButton == button)
        m_checkedButton = 0;

    if (m_members.isEmpty()) {
        ASSERT(!m_requiredCount);
        ASSERT(!m_checkedButton);
    } else if (wasValid != isValid())
        setNeedsValidityCheckForAllButtons();
    if (!wasValid) {
        // A radio button not in a group is always valid. We need to make it
        // valid only if the group was invalid.
        button->setNeedsValidityCheck();
    }
}

void RadioButtonGroup::setNeedsValidityCheckForAllButtons()
{
    typedef HashSet<HTMLInputElement*>::const_iterator Iterator;
    Iterator end = m_members.end();
    for (Iterator it = m_members.begin(); it != end; ++it) {
        HTMLInputElement* button = *it;
        ASSERT(button->isRadioButton());
        button->setNeedsValidityCheck();
    }
}

bool RadioButtonGroup::contains(HTMLInputElement* button) const
{
    return m_members.contains(button);
}

// ----------------------------------------------------------------

// Explicity define empty constructor and destructor in order to prevent the
// compiler from generating them as inlines. So we don't need to to define
// RadioButtonGroup in the header.
CheckedRadioButtons::CheckedRadioButtons()
{
}

CheckedRadioButtons::~CheckedRadioButtons()
{
}

void CheckedRadioButtons::addButton(HTMLInputElement* element)
{
    ASSERT(element->isRadioButton());
    if (element->name().isEmpty())
        return;

    if (!m_nameToGroupMap)
        m_nameToGroupMap = adoptPtr(new NameToGroupMap);

    OwnPtr<RadioButtonGroup>& group = m_nameToGroupMap->add(element->name().impl(), PassOwnPtr<RadioButtonGroup>()).iterator->value;
    if (!group)
        group = RadioButtonGroup::create();
    group->add(element);
}

void CheckedRadioButtons::updateCheckedState(HTMLInputElement* element)
{
    ASSERT(element->isRadioButton());
    if (element->name().isEmpty())
        return;
    ASSERT(m_nameToGroupMap);
    if (!m_nameToGroupMap)
        return;
    RadioButtonGroup* group = m_nameToGroupMap->get(element->name().impl());
    ASSERT(group);
    group->updateCheckedState(element);
}

void CheckedRadioButtons::requiredAttributeChanged(HTMLInputElement* element)
{
    ASSERT(element->isRadioButton());
    if (element->name().isEmpty())
        return;
    ASSERT(m_nameToGroupMap);
    if (!m_nameToGroupMap)
        return;
    RadioButtonGroup* group = m_nameToGroupMap->get(element->name().impl());
    ASSERT(group);
    group->requiredAttributeChanged(element);
}

HTMLInputElement* CheckedRadioButtons::checkedButtonForGroup(const AtomicString& name) const
{
    if (!m_nameToGroupMap)
        return 0;
    m_nameToGroupMap->checkConsistency();
    RadioButtonGroup* group = m_nameToGroupMap->get(name.impl());
    return group ? group->checkedButton() : 0;
}

bool CheckedRadioButtons::isInRequiredGroup(HTMLInputElement* element) const
{
    ASSERT(element->isRadioButton());
    if (element->name().isEmpty())
        return false;
    if (!m_nameToGroupMap)
        return false;
    RadioButtonGroup* group = m_nameToGroupMap->get(element->name().impl());
    return group && group->isRequired() && group->contains(element);
}

void CheckedRadioButtons::removeButton(HTMLInputElement* element)
{
    ASSERT(element->isRadioButton());
    if (element->name().isEmpty())
        return;
    if (!m_nameToGroupMap)
        return;

    m_nameToGroupMap->checkConsistency();
    NameToGroupMap::iterator it = m_nameToGroupMap->find(element->name().impl());
    if (it == m_nameToGroupMap->end())
        return;
    it->value->remove(element);
    if (it->value->isEmpty()) {
        // FIXME: We may skip deallocating the empty RadioButtonGroup for
        // performance improvement. If we do so, we need to change the key type
        // of m_nameToGroupMap from AtomicStringImpl* to RefPtr<AtomicStringImpl>.
        m_nameToGroupMap->remove(it);
        if (m_nameToGroupMap->isEmpty())
            m_nameToGroupMap.clear();
    }
}

} // namespace
