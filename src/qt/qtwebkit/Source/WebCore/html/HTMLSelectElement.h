/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef HTMLSelectElement_h
#define HTMLSelectElement_h

#include "Event.h"
#include "HTMLFormControlElementWithState.h"
#include "HTMLOptionsCollection.h"
#include "TypeAhead.h"
#include <wtf/Vector.h>

namespace WebCore {

class HTMLOptionElement;

class HTMLSelectElement : public HTMLFormControlElementWithState, public TypeAheadDataSource {
public:
    static PassRefPtr<HTMLSelectElement> create(const QualifiedName&, Document*, HTMLFormElement*);

    int selectedIndex() const;
    void setSelectedIndex(int);

    void optionSelectedByUser(int index, bool dispatchChangeEvent, bool allowMultipleSelection = false);

    // For ValidityState
    virtual String validationMessage() const OVERRIDE;
    virtual bool valueMissing() const OVERRIDE;

    unsigned length() const;

    int size() const { return m_size; }
    bool multiple() const { return m_multiple; }

    bool usesMenuList() const;

    void add(HTMLElement*, HTMLElement* beforeElement, ExceptionCode&);
    void remove(int index);
    void remove(HTMLOptionElement*);

    String value() const;
    void setValue(const String&);

    PassRefPtr<HTMLOptionsCollection> options();
    PassRefPtr<HTMLCollection> selectedOptions();

    void optionElementChildrenChanged();

    void setRecalcListItems();
    void invalidateSelectedItems();
    void updateListItemSelectedStates();

    const Vector<HTMLElement*>& listItems() const;

    virtual void accessKeyAction(bool sendMouseEvents);
    void accessKeySetSelectedIndex(int);

    void setMultiple(bool);

    void setSize(int);

    void setOption(unsigned index, HTMLOptionElement*, ExceptionCode&);
    void setLength(unsigned, ExceptionCode&);

    Node* namedItem(const AtomicString& name);
    Node* item(unsigned index);

    void scrollToSelection();

    void listBoxSelectItem(int listIndex, bool allowMultiplySelections, bool shift, bool fireOnChangeNow = true);

    bool canSelectAll() const;
    void selectAll();
    int listToOptionIndex(int listIndex) const;
    void listBoxOnChange();
    int optionToListIndex(int optionIndex) const;
    int activeSelectionStartListIndex() const;
    int activeSelectionEndListIndex() const;
    void setActiveSelectionAnchorIndex(int);
    void setActiveSelectionEndIndex(int);
    void updateListBoxSelection(bool deselectOtherOptions);
    
    // For use in the implementation of HTMLOptionElement.
    void optionSelectionStateChanged(HTMLOptionElement*, bool optionIsSelected);

protected:
    HTMLSelectElement(const QualifiedName&, Document*, HTMLFormElement*);

private:
    virtual const AtomicString& formControlType() const;
    
    virtual bool isKeyboardFocusable(KeyboardEvent*) const OVERRIDE;
    virtual bool isMouseFocusable() const OVERRIDE;

    virtual void dispatchFocusEvent(PassRefPtr<Element> oldFocusedElement, FocusDirection) OVERRIDE FINAL;
    virtual void dispatchBlurEvent(PassRefPtr<Element> newFocusedElement) OVERRIDE FINAL;
    
    virtual bool canStartSelection() const { return false; }

    virtual bool isEnumeratable() const { return true; }
    virtual bool supportLabels() const OVERRIDE { return true; }

    virtual FormControlState saveFormControlState() const OVERRIDE;
    virtual void restoreFormControlState(const FormControlState&) OVERRIDE;

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual bool isPresentationAttribute(const QualifiedName&) const OVERRIDE;

    virtual bool childShouldCreateRenderer(const NodeRenderingContext&) const OVERRIDE;
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle *);
    virtual bool appendFormData(FormDataList&, bool);

    virtual void reset();

    virtual void defaultEventHandler(Event*);

    void dispatchChangeEventForMenuList();
    
    void recalcListItems(bool updateSelectedStates = true) const;

    void deselectItems(HTMLOptionElement* excludeElement = 0);
    void typeAheadFind(KeyboardEvent*);
    void saveLastSelection();

    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;

    virtual bool isOptionalFormControl() const { return !isRequiredFormControl(); }
    virtual bool isRequiredFormControl() const;

    bool hasPlaceholderLabelOption() const;

    enum SelectOptionFlag {
        DeselectOtherOptions = 1 << 0,
        DispatchChangeEvent = 1 << 1,
        UserDriven = 1 << 2,
    };
    typedef unsigned SelectOptionFlags;
    void selectOption(int optionIndex, SelectOptionFlags = 0);
    void deselectItemsWithoutValidation(HTMLElement* elementToExclude = 0);
    void parseMultipleAttribute(const AtomicString&);
    int lastSelectedListIndex() const;
    void updateSelectedState(int listIndex, bool multi, bool shift);
    void menuListDefaultEventHandler(Event*);
    bool platformHandleKeydownEvent(KeyboardEvent*);
    void listBoxDefaultEventHandler(Event*);
    void setOptionsChangedOnRenderer();
    size_t searchOptionsForValue(const String&, size_t listIndexStart, size_t listIndexEnd) const;

    enum SkipDirection {
        SkipBackwards = -1,
        SkipForwards = 1
    };
    int nextValidIndex(int listIndex, SkipDirection, int skip) const;
    int nextSelectableListIndex(int startIndex) const;
    int previousSelectableListIndex(int startIndex) const;
    int firstSelectableListIndex() const;
    int lastSelectableListIndex() const;
    int nextSelectableListIndexPageAway(int startIndex, SkipDirection) const;

    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);
    virtual bool areAuthorShadowsAllowed() const OVERRIDE { return false; }

    // TypeAheadDataSource functions.
    virtual int indexOfSelectedOption() const OVERRIDE;
    virtual int optionCount() const OVERRIDE;
    virtual String optionAtIndex(int index) const OVERRIDE;

    // m_listItems contains HTMLOptionElement, HTMLOptGroupElement, and HTMLHRElement objects.
    mutable Vector<HTMLElement*> m_listItems;
    Vector<bool> m_lastOnChangeSelection;
    Vector<bool> m_cachedStateForActiveSelection;
    TypeAhead m_typeAhead;
    int m_size;
    int m_lastOnChangeIndex;
    int m_activeSelectionAnchorIndex;
    int m_activeSelectionEndIndex;
    bool m_isProcessingUserDrivenChange;
    bool m_multiple;
    bool m_activeSelectionState;
    mutable bool m_shouldRecalcListItems;
};

inline HTMLSelectElement* toHTMLSelectElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || node->hasTagName(HTMLNames::selectTag));
    return static_cast<HTMLSelectElement*>(node);
}

} // namespace

#endif
