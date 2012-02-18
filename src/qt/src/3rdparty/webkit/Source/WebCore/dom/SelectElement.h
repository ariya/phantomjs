/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http//www.torchmobile.com/)
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

#ifndef SelectElement_h
#define SelectElement_h

#include "Event.h"
#include <wtf/Forward.h>
#include <wtf/Vector.h>

namespace WebCore {

class Attribute;
class Element;
class Event;
class FormDataList;
class HTMLFormElement;
class KeyboardEvent;
class SelectElementData;

class SelectElement {
public:
    virtual bool multiple() const = 0;

    virtual int size() const = 0;
    virtual const Vector<Element*>& listItems() const = 0;

    virtual void listBoxOnChange() = 0;
    virtual void updateListBoxSelection(bool deselectOtherOptions) = 0;

    virtual void menuListOnChange() = 0;

    virtual int activeSelectionStartListIndex() const = 0;
    virtual int activeSelectionEndListIndex() const = 0;

    virtual void setActiveSelectionAnchorIndex(int index) = 0;
    virtual void setActiveSelectionEndIndex(int index) = 0;

    virtual int listToOptionIndex(int listIndex) const = 0;
    virtual int optionToListIndex(int optionIndex) const = 0;

    virtual int selectedIndex() const = 0;
    virtual void setSelectedIndex(int index, bool deselect = true) = 0;
    virtual void setSelectedIndexByUser(int index, bool deselect = true, bool fireOnChangeNow = false, bool allowMultipleSelection = false) = 0;

    virtual void listBoxSelectItem(int listIndex, bool allowMultiplySelections, bool shift, bool fireOnChangeNow = true) = 0;

    virtual void updateValidity() = 0;

protected:
    virtual ~SelectElement() { }

    friend class SelectElementData;

    static void selectAll(SelectElementData&, Element*);
    static void saveLastSelection(SelectElementData&, Element*);
    static void setActiveSelectionAnchorIndex(SelectElementData&, Element*, int index);
    static void setActiveSelectionEndIndex(SelectElementData&, int index);
    static void updateListBoxSelection(SelectElementData&, Element*, bool deselectOtherOptions);
    static void listBoxOnChange(SelectElementData&, Element*);
    static void menuListOnChange(SelectElementData&, Element*);
    static void scrollToSelection(SelectElementData&, Element*);
    static void setRecalcListItems(SelectElementData&, Element*);
    static void recalcListItems(SelectElementData&, const Element*, bool updateSelectedStates = true);
    static int selectedIndex(const SelectElementData&, const Element*);
    static void setSelectedIndex(SelectElementData&, Element*, int optionIndex, bool deselect = true, bool fireOnChangeNow = false, bool userDrivenChange = true);
    static int optionToListIndex(const SelectElementData&, const Element*, int optionIndex);
    static int listToOptionIndex(const SelectElementData&, const Element*, int listIndex);
    static void dispatchFocusEvent(SelectElementData&, Element*);
    static void dispatchBlurEvent(SelectElementData&, Element*);
    static void deselectItems(SelectElementData&, Element*, Element* excludeElement = 0);
    static bool saveFormControlState(const SelectElementData&, const Element*, String& state);
    static void restoreFormControlState(SelectElementData&, Element*, const String& state);
    static void parseMultipleAttribute(SelectElementData&, Element*, Attribute*);
    static bool appendFormData(SelectElementData&, Element*, FormDataList&);
    static void reset(SelectElementData&, Element*);
    static void defaultEventHandler(SelectElementData&, Element*, Event*, HTMLFormElement*);
    static int lastSelectedListIndex(const SelectElementData&, const Element*);
    static void typeAheadFind(SelectElementData&, Element*, KeyboardEvent*);
    static void insertedIntoTree(SelectElementData&, Element*);
    static void accessKeySetSelectedIndex(SelectElementData&, Element*, int index);
    static unsigned optionCount(const SelectElementData&, const Element*);

    static void updateSelectedState(SelectElementData& data, Element* element, int listIndex,
                                    bool multi, bool shift);
 
private:
    static void menuListDefaultEventHandler(SelectElementData&, Element*, Event*, HTMLFormElement*);
    static void listBoxDefaultEventHandler(SelectElementData&, Element*, Event*, HTMLFormElement*);
    static void setOptionsChangedOnRenderer(SelectElementData&, Element*);
};

// HTMLSelectElement hold this struct as member variable
// and pass it to the static helper functions in SelectElement
class SelectElementData {
public:
    SelectElementData();
    ~SelectElementData();

    bool multiple() const { return m_multiple; }
    void setMultiple(bool value) { m_multiple = value; }

    int size() const { return m_size; }
    void setSize(int value) { m_size = value; }

    bool usesMenuList() const
    {
#if ENABLE(NO_LISTBOX_RENDERING)
        return true;
#else
        return !m_multiple && m_size <= 1;
#endif
    }

    int lastOnChangeIndex() const { return m_lastOnChangeIndex; }
    void setLastOnChangeIndex(int value) { m_lastOnChangeIndex = value; }

    bool userDrivenChange() const { return m_userDrivenChange; }
    void setUserDrivenChange(bool value) { m_userDrivenChange = value; }

    Vector<bool>& lastOnChangeSelection() { return m_lastOnChangeSelection; }

    bool activeSelectionState() const { return m_activeSelectionState; }
    void setActiveSelectionState(bool value) { m_activeSelectionState = value; }

    int activeSelectionAnchorIndex() const { return m_activeSelectionAnchorIndex; }
    void setActiveSelectionAnchorIndex(int value) { m_activeSelectionAnchorIndex = value; }

    int activeSelectionEndIndex() const { return m_activeSelectionEndIndex; }
    void setActiveSelectionEndIndex(int value) { m_activeSelectionEndIndex = value; }

    Vector<bool>& cachedStateForActiveSelection() { return m_cachedStateForActiveSelection; }

    bool shouldRecalcListItems() const { return m_recalcListItems; }
    void setShouldRecalcListItems(bool value) { m_recalcListItems = value; }

    Vector<Element*>& rawListItems() { return m_listItems; }
    Vector<Element*>& listItems(const Element*);
    const Vector<Element*>& listItems(const Element*) const;

    UChar repeatingChar() const { return m_repeatingChar; }
    void setRepeatingChar(const UChar& value) { m_repeatingChar = value; }

    DOMTimeStamp lastCharTime() const { return m_lastCharTime; }
    void setLastCharTime(const DOMTimeStamp& value) { m_lastCharTime = value; }

    String& typedString() { return m_typedString; }
    void setTypedString(const String& value) { m_typedString = value; }

private:
    void checkListItems(const Element*) const;

    bool m_multiple;
    int m_size;

    int m_lastOnChangeIndex;
    Vector<bool> m_lastOnChangeSelection;
    bool m_userDrivenChange;

    bool m_activeSelectionState;
    int m_activeSelectionAnchorIndex;
    int m_activeSelectionEndIndex;
    Vector<bool> m_cachedStateForActiveSelection;

    bool m_recalcListItems;
    Vector<Element*> m_listItems;

    // Instance variables for type-ahead find
    UChar m_repeatingChar;
    DOMTimeStamp m_lastCharTime;
    String m_typedString;
};

SelectElement* toSelectElement(Element*);

}

#endif
