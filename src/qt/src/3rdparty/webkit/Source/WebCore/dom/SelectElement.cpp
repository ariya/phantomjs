/*
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
#include "SelectElement.h"

#include "Attribute.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Element.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "FormDataList.h"
#include "Frame.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"
#include "HTMLSelectElement.h"
#include "KeyboardEvent.h"
#include "MouseEvent.h"
#include "OptionElement.h"
#include "OptionGroupElement.h"
#include "Page.h"
#include "RenderListBox.h"
#include "RenderMenuList.h"
#include "SpatialNavigation.h"
#include <wtf/Assertions.h>
#include <wtf/unicode/CharacterNames.h>

// Configure platform-specific behavior when focused pop-up receives arrow/space/return keystroke.
// (PLATFORM(MAC) and PLATFORM(GTK) are always false in Chromium, hence the extra tests.)
#if PLATFORM(MAC) || (PLATFORM(CHROMIUM) && OS(DARWIN))
#define ARROW_KEYS_POP_MENU 1
#define SPACE_OR_RETURN_POP_MENU 0
#elif PLATFORM(GTK) || (PLATFORM(CHROMIUM) && OS(UNIX))
#define ARROW_KEYS_POP_MENU 0
#define SPACE_OR_RETURN_POP_MENU 1
#else
#define ARROW_KEYS_POP_MENU 0
#define SPACE_OR_RETURN_POP_MENU 0
#endif

using std::min;
using std::max;
using namespace WTF;
using namespace Unicode;

namespace WebCore {

static const DOMTimeStamp typeAheadTimeout = 1000;

enum SkipDirection {
    SkipBackwards = -1,
    SkipForwards = 1
};

// Returns the 1st valid item |skip| items from |listIndex| in direction |direction| if there is one.
// Otherwise, it returns the valid item closest to that boundary which is past |listIndex| if there is one.
// Otherwise, it returns |listIndex|.
// Valid means that it is enabled and an option element.
static int nextValidIndex(const Vector<Element*>& listItems, int listIndex, SkipDirection direction, int skip)
{
    ASSERT(direction == -1 || direction == 1);
    int lastGoodIndex = listIndex;
    int size = listItems.size();
    for (listIndex += direction; listIndex >= 0 && listIndex < size; listIndex += direction) {
        --skip;
        if (!listItems[listIndex]->disabled() && isOptionElement(listItems[listIndex])) {
            lastGoodIndex = listIndex;
            if (skip <= 0)
                break;
        }
    }
    return lastGoodIndex;
}

static int nextSelectableListIndex(SelectElementData& data, Element* element, int startIndex)
{
    return nextValidIndex(data.listItems(element), startIndex, SkipForwards, 1);
}

static int previousSelectableListIndex(SelectElementData& data, Element* element, int startIndex)
{
    if (startIndex == -1)
        startIndex = data.listItems(element).size();
    return nextValidIndex(data.listItems(element), startIndex, SkipBackwards, 1);
}

static int firstSelectableListIndex(SelectElementData& data, Element* element)
{
    const Vector<Element*>& items = data.listItems(element);
    int index = nextValidIndex(items, items.size(), SkipBackwards, INT_MAX);
    if (static_cast<unsigned>(index) == items.size())
        return -1;
    return index;
}

static int lastSelectableListIndex(SelectElementData& data, Element* element)
{
    return nextValidIndex(data.listItems(element), -1, SkipForwards, INT_MAX);
}

// Returns the index of the next valid item one page away from |startIndex| in direction |direction|.
static int nextSelectableListIndexPageAway(SelectElementData& data, Element* element, int startIndex, SkipDirection direction)
{
    const Vector<Element*>& items = data.listItems(element);
    // Can't use data->size() because renderer forces a minimum size.
    int pageSize = 0;
    if (element->renderer()->isListBox())
        pageSize = toRenderListBox(element->renderer())->size() - 1; // -1 so we still show context

    // One page away, but not outside valid bounds.
    // If there is a valid option item one page away, the index is chosen.
    // If there is no exact one page away valid option, returns startIndex or the most far index.
    int edgeIndex = (direction == SkipForwards) ? 0 : (items.size() - 1);
    int skipAmount = pageSize + ((direction == SkipForwards) ? startIndex : (edgeIndex - startIndex));
    return nextValidIndex(items, edgeIndex, direction, skipAmount);
}

void SelectElement::selectAll(SelectElementData& data, Element* element)
{
    ASSERT(!data.usesMenuList());
    if (!element->renderer() || !data.multiple())
        return;

    // Save the selection so it can be compared to the new selectAll selection when dispatching change events
    saveLastSelection(data, element);

    data.setActiveSelectionState(true);
    setActiveSelectionAnchorIndex(data, element, nextSelectableListIndex(data, element, -1));
    setActiveSelectionEndIndex(data, previousSelectableListIndex(data, element, -1));

    updateListBoxSelection(data, element, false);
    listBoxOnChange(data, element);
}

void SelectElement::saveLastSelection(SelectElementData& data, Element* element)
{
    if (data.usesMenuList()) {
        data.setLastOnChangeIndex(selectedIndex(data, element));
        return;
    }

    Vector<bool>& lastOnChangeSelection = data.lastOnChangeSelection(); 
    lastOnChangeSelection.clear();

    const Vector<Element*>& items = data.listItems(element);
    for (unsigned i = 0; i < items.size(); ++i) {
        OptionElement* optionElement = toOptionElement(items[i]);
        lastOnChangeSelection.append(optionElement && optionElement->selected());
    }
}

void SelectElement::setActiveSelectionAnchorIndex(SelectElementData& data, Element* element, int index)
{
    data.setActiveSelectionAnchorIndex(index);

    // Cache the selection state so we can restore the old selection as the new selection pivots around this anchor index
    Vector<bool>& cachedStateForActiveSelection = data.cachedStateForActiveSelection(); 
    cachedStateForActiveSelection.clear();

    const Vector<Element*>& items = data.listItems(element);
    for (unsigned i = 0; i < items.size(); ++i) {
        OptionElement* optionElement = toOptionElement(items[i]);
        cachedStateForActiveSelection.append(optionElement && optionElement->selected());
    }
}

void SelectElement::setActiveSelectionEndIndex(SelectElementData& data, int index)
{
    data.setActiveSelectionEndIndex(index);
}

void SelectElement::updateListBoxSelection(SelectElementData& data, Element* element, bool deselectOtherOptions)
{
    ASSERT(element->renderer() && (element->renderer()->isListBox() || data.multiple()));
    ASSERT(!data.listItems(element).size() || data.activeSelectionAnchorIndex() >= 0);

    unsigned start = min(data.activeSelectionAnchorIndex(), data.activeSelectionEndIndex());
    unsigned end = max(data.activeSelectionAnchorIndex(), data.activeSelectionEndIndex());
    Vector<bool>& cachedStateForActiveSelection = data.cachedStateForActiveSelection();

    const Vector<Element*>& items = data.listItems(element);
    for (unsigned i = 0; i < items.size(); ++i) {
        OptionElement* optionElement = toOptionElement(items[i]);
        if (!optionElement || items[i]->disabled())
            continue;

        if (i >= start && i <= end)
            optionElement->setSelectedState(data.activeSelectionState());
        else if (deselectOtherOptions || i >= cachedStateForActiveSelection.size())
            optionElement->setSelectedState(false);
        else
            optionElement->setSelectedState(cachedStateForActiveSelection[i]);
    }

    toSelectElement(element)->updateValidity();
    scrollToSelection(data, element);
}

void SelectElement::listBoxOnChange(SelectElementData& data, Element* element)
{
    ASSERT(!data.usesMenuList() || data.multiple());

    Vector<bool>& lastOnChangeSelection = data.lastOnChangeSelection(); 
    const Vector<Element*>& items = data.listItems(element);

    // If the cached selection list is empty, or the size has changed, then fire dispatchFormControlChangeEvent, and return early.
    if (lastOnChangeSelection.isEmpty() || lastOnChangeSelection.size() != items.size()) {
        element->dispatchFormControlChangeEvent();
        return;
    }

    // Update lastOnChangeSelection and fire dispatchFormControlChangeEvent
    bool fireOnChange = false;
    for (unsigned i = 0; i < items.size(); ++i) {
        OptionElement* optionElement = toOptionElement(items[i]);
        bool selected = optionElement &&  optionElement->selected();
        if (selected != lastOnChangeSelection[i])
            fireOnChange = true;
        lastOnChangeSelection[i] = selected;
    }

    if (fireOnChange)
        element->dispatchFormControlChangeEvent();
}

void SelectElement::menuListOnChange(SelectElementData& data, Element* element)
{
    ASSERT(data.usesMenuList());

    int selected = selectedIndex(data, element);
    if (data.lastOnChangeIndex() != selected && data.userDrivenChange()) {
        data.setLastOnChangeIndex(selected);
        data.setUserDrivenChange(false);
        element->dispatchFormControlChangeEvent();
    }
}

void SelectElement::scrollToSelection(SelectElementData& data, Element* element)
{
    if (data.usesMenuList())
        return;

    if (RenderObject* renderer = element->renderer())
        toRenderListBox(renderer)->selectionChanged();
}

void SelectElement::setOptionsChangedOnRenderer(SelectElementData& data, Element* element)
{
    if (RenderObject* renderer = element->renderer()) {
        if (data.usesMenuList())
            toRenderMenuList(renderer)->setOptionsChanged(true);
        else
            toRenderListBox(renderer)->setOptionsChanged(true);
    }
}

void SelectElement::setRecalcListItems(SelectElementData& data, Element* element)
{
    data.setShouldRecalcListItems(true);
    data.setActiveSelectionAnchorIndex(-1); // Manual selection anchor is reset when manipulating the select programmatically.
    setOptionsChangedOnRenderer(data, element);
    element->setNeedsStyleRecalc();
}

void SelectElement::recalcListItems(SelectElementData& data, const Element* element, bool updateSelectedStates)
{
    Vector<Element*>& listItems = data.rawListItems();
    listItems.clear();

    data.setShouldRecalcListItems(false);

    OptionElement* foundSelected = 0;
    for (Node* currentNode = element->firstChild(); currentNode;) {
        if (!currentNode->isElementNode()) {
            currentNode = currentNode->traverseNextSibling(element);
            continue;
        }

        Element* current = static_cast<Element*>(currentNode);

        // optgroup tags may not nest. However, both FireFox and IE will
        // flatten the tree automatically, so we follow suit.
        // (http://www.w3.org/TR/html401/interact/forms.html#h-17.6)
        if (isOptionGroupElement(current)) {
            listItems.append(current);
            if (current->firstChild()) {
                currentNode = current->firstChild();
                continue;
            }
        }

        if (OptionElement* optionElement = toOptionElement(current)) {
            listItems.append(current);

            if (updateSelectedStates && !data.multiple()) {
                if (!foundSelected && (data.size() <= 1 || optionElement->selected())) {
                    foundSelected = optionElement;
                    foundSelected->setSelectedState(true);
                } else if (foundSelected && optionElement->selected()) {
                    foundSelected->setSelectedState(false);
                    foundSelected = optionElement;
                }
            }
        }

        if (current->hasTagName(HTMLNames::hrTag))
            listItems.append(current);

        // In conforming HTML code, only <optgroup> and <option> will be found
        // within a <select>. We call traverseNextSibling so that we only step
        // into those tags that we choose to. For web-compat, we should cope
        // with the case where odd tags like a <div> have been added but we
        // handle this because such tags have already been removed from the
        // <select>'s subtree at this point.
        currentNode = currentNode->traverseNextSibling(element);
    }
}

int SelectElement::selectedIndex(const SelectElementData& data, const Element* element)
{
    unsigned index = 0;

    // return the number of the first option selected
    const Vector<Element*>& items = data.listItems(element);
    for (size_t i = 0; i < items.size(); ++i) {
        if (OptionElement* optionElement = toOptionElement(items[i])) {
            if (optionElement->selected())
                return index;
            ++index;
        }
    }

    return -1;
}

void SelectElement::setSelectedIndex(SelectElementData& data, Element* element, int optionIndex, bool deselect, bool fireOnChangeNow, bool userDrivenChange)
{
    if (optionIndex == -1 && !deselect && !data.multiple())
        optionIndex = nextSelectableListIndex(data, element, -1);
    if (!data.multiple())
        deselect = true;

    const Vector<Element*>& items = data.listItems(element);
    int listIndex = optionToListIndex(data, element, optionIndex);

    Element* excludeElement = 0;
    if (OptionElement* optionElement = (listIndex >= 0 ? toOptionElement(items[listIndex]) : 0)) {
        excludeElement = items[listIndex];
        if (data.activeSelectionAnchorIndex() < 0 || deselect)
            setActiveSelectionAnchorIndex(data, element, listIndex);
        if (data.activeSelectionEndIndex() < 0 || deselect)
            setActiveSelectionEndIndex(data, listIndex);
        optionElement->setSelectedState(true);
    }

    if (deselect)
        deselectItems(data, element, excludeElement);

    // For the menu list case, this is what makes the selected element appear.
    if (RenderObject* renderer = element->renderer())
        renderer->updateFromElement();

    scrollToSelection(data, element);

    // This only gets called with fireOnChangeNow for menu lists. 
    if (data.usesMenuList()) {
        data.setUserDrivenChange(userDrivenChange);
        if (fireOnChangeNow)
            menuListOnChange(data, element);
        RenderObject* renderer = element->renderer();
        if (renderer) {
            if (data.usesMenuList())
                toRenderMenuList(renderer)->didSetSelectedIndex();
            else if (renderer->isListBox())
                toRenderListBox(renderer)->selectionChanged();
        }
    }

    if (Frame* frame = element->document()->frame())
        frame->page()->chrome()->client()->formStateDidChange(element);
}

int SelectElement::optionToListIndex(const SelectElementData& data, const Element* element, int optionIndex)
{
    const Vector<Element*>& items = data.listItems(element);
    int listSize = (int) items.size();
    if (optionIndex < 0 || optionIndex >= listSize)
        return -1;

    int optionIndex2 = -1;
    for (int listIndex = 0; listIndex < listSize; ++listIndex) {
        if (isOptionElement(items[listIndex])) {
            ++optionIndex2;
            if (optionIndex2 == optionIndex)
                return listIndex;
        }
    }

    return -1;
}

int SelectElement::listToOptionIndex(const SelectElementData& data, const Element* element, int listIndex)
{
    const Vector<Element*>& items = data.listItems(element);
    if (listIndex < 0 || listIndex >= int(items.size()) ||
        !isOptionElement(items[listIndex]))
        return -1;

    int optionIndex = 0; // actual index of option not counting OPTGROUP entries that may be in list
    for (int i = 0; i < listIndex; ++i)
        if (isOptionElement(items[i]))
            ++optionIndex;

    return optionIndex;
}

void SelectElement::dispatchFocusEvent(SelectElementData& data, Element* element)
{
    // Save the selection so it can be compared to the new selection when dispatching change events during blur event dispatchal
    if (data.usesMenuList())
        saveLastSelection(data, element);
}

void SelectElement::dispatchBlurEvent(SelectElementData& data, Element* element)
{
    // We only need to fire change events here for menu lists, because we fire change events for list boxes whenever the selection change is actually made.
    // This matches other browsers' behavior.
    if (data.usesMenuList())
        menuListOnChange(data, element);
}

void SelectElement::deselectItems(SelectElementData& data, Element* element, Element* excludeElement)
{
    const Vector<Element*>& items = data.listItems(element);
    for (unsigned i = 0; i < items.size(); ++i) {
        if (items[i] == excludeElement)
            continue;

        if (OptionElement* optionElement = toOptionElement(items[i]))
            optionElement->setSelectedState(false);
    }
}

bool SelectElement::saveFormControlState(const SelectElementData& data, const Element* element, String& value)
{
    const Vector<Element*>& items = data.listItems(element);
    int length = items.size();

    // FIXME: Change this code to use the new StringImpl::createUninitialized code path.
    Vector<char, 1024> characters(length);
    for (int i = 0; i < length; ++i) {
        OptionElement* optionElement = toOptionElement(items[i]);
        bool selected = optionElement && optionElement->selected();
        characters[i] = selected ? 'X' : '.';
    }

    value = String(characters.data(), length);
    return true;
}

void SelectElement::restoreFormControlState(SelectElementData& data, Element* element, const String& state)
{
    recalcListItems(data, element);

    const Vector<Element*>& items = data.listItems(element);
    int length = items.size();

    for (int i = 0; i < length; ++i) {
        if (OptionElement* optionElement = toOptionElement(items[i]))
            optionElement->setSelectedState(state[i] == 'X');
    }

    setOptionsChangedOnRenderer(data, element);
}

void SelectElement::parseMultipleAttribute(SelectElementData& data, Element* element, Attribute* attribute)
{
    bool oldUsesMenuList = data.usesMenuList();
    data.setMultiple(!attribute->isNull());
    toSelectElement(element)->updateValidity();
    if (oldUsesMenuList != data.usesMenuList() && element->attached()) {
        element->detach();
        element->attach();
    }
}

bool SelectElement::appendFormData(SelectElementData& data, Element* element, FormDataList& list)
{
    const AtomicString& name = element->formControlName();
    if (name.isEmpty())
        return false;

    bool successful = false;
    const Vector<Element*>& items = data.listItems(element);

    for (unsigned i = 0; i < items.size(); ++i) {
        OptionElement* optionElement = toOptionElement(items[i]);
        if (optionElement && optionElement->selected() && !optionElement->disabled()) {
            list.appendData(name, optionElement->value());
            successful = true;
        }
    }

    // It's possible that this is a menulist with multiple options and nothing
    // will be submitted (!successful). We won't send a unselected non-disabled
    // option as fallback. This behavior matches to other browsers.
    return successful;
} 

void SelectElement::reset(SelectElementData& data, Element* element)
{
    OptionElement* firstOption = 0;
    OptionElement* selectedOption = 0;

    const Vector<Element*>& items = data.listItems(element);
    for (unsigned i = 0; i < items.size(); ++i) {
        OptionElement* optionElement = toOptionElement(items[i]);
        if (!optionElement)
            continue;

        if (items[i]->fastHasAttribute(HTMLNames::selectedAttr)) {
            if (selectedOption && !data.multiple())
                selectedOption->setSelectedState(false);
            optionElement->setSelectedState(true);
            selectedOption = optionElement;
        } else
            optionElement->setSelectedState(false);

        if (!firstOption)
            firstOption = optionElement;
    }

    if (!selectedOption && firstOption && !data.multiple() && data.size() <= 1)
        firstOption->setSelectedState(true);

    setOptionsChangedOnRenderer(data, element);
    element->setNeedsStyleRecalc();
}

void SelectElement::menuListDefaultEventHandler(SelectElementData& data, Element* element, Event* event, HTMLFormElement* htmlForm)
{
    if (event->type() == eventNames().keydownEvent) {
        if (!element->renderer() || !event->isKeyboardEvent())
            return;

        const String& keyIdentifier = static_cast<KeyboardEvent*>(event)->keyIdentifier();
        bool handled = false;

#if ARROW_KEYS_POP_MENU
        if (!isSpatialNavigationEnabled(element->document()->frame())) {
            if (keyIdentifier == "Down" || keyIdentifier == "Up") {
                element->focus();

                if (!element->renderer()) // Calling focus() may cause us to lose our renderer, in which case do not want to handle the event.
                    return;

                // Save the selection so it can be compared to the new selection when dispatching change events during setSelectedIndex,
                // which gets called from RenderMenuList::valueChanged, which gets called after the user makes a selection from the menu.
                saveLastSelection(data, element);
                if (RenderMenuList* menuList = toRenderMenuList(element->renderer()))
                    menuList->showPopup();

                event->setDefaultHandled();
            }
            return;
        }
#endif
        // When using spatial navigation, we want to be able to navigate away from the select element
        // when the user hits any of the arrow keys, instead of changing the selection.
        if (isSpatialNavigationEnabled(element->document()->frame()))
            if (!data.activeSelectionState())
                return;

        UNUSED_PARAM(htmlForm);
        const Vector<Element*>& listItems = data.listItems(element);

        int listIndex = optionToListIndex(data, element, selectedIndex(data, element));
        if (keyIdentifier == "Down" || keyIdentifier == "Right") {
            listIndex = nextValidIndex(listItems, listIndex, SkipForwards, 1);
            handled = true;
        } else if (keyIdentifier == "Up" || keyIdentifier == "Left") {
            listIndex = nextValidIndex(listItems, listIndex, SkipBackwards, 1);
            handled = true;
        } else if (keyIdentifier == "PageDown") {
            listIndex = nextValidIndex(listItems, listIndex, SkipForwards, 3);
            handled = true;
        } else if (keyIdentifier == "PageUp") {
            listIndex = nextValidIndex(listItems, listIndex, SkipBackwards, 3);
            handled = true;
        } else if (keyIdentifier == "Home") {
            listIndex = nextValidIndex(listItems, -1, SkipForwards, 1);
            handled = true;
        } else if (keyIdentifier == "End") {
            listIndex = nextValidIndex(listItems, listItems.size(), SkipBackwards, 1);
            handled = true;
        }

        if (handled && listIndex >= 0 && static_cast<unsigned>(listIndex) < listItems.size())
            setSelectedIndex(data, element, listToOptionIndex(data, element, listIndex));

        if (handled)
            event->setDefaultHandled();
    }

    // Use key press event here since sending simulated mouse events
    // on key down blocks the proper sending of the key press event.
    if (event->type() == eventNames().keypressEvent) {
        if (!element->renderer() || !event->isKeyboardEvent())
            return;

        int keyCode = static_cast<KeyboardEvent*>(event)->keyCode();
        bool handled = false;

        if (keyCode == ' ' && isSpatialNavigationEnabled(element->document()->frame())) {
            // Use space to toggle arrow key handling for selection change or spatial navigation.
            data.setActiveSelectionState(!data.activeSelectionState());
            event->setDefaultHandled();
            return;
        }

#if SPACE_OR_RETURN_POP_MENU
        if (keyCode == ' ' || keyCode == '\r') {
            element->focus();

            if (!element->renderer()) // Calling focus() may cause us to lose our renderer, in which case do not want to handle the event.
                return;

            // Save the selection so it can be compared to the new selection when dispatching change events during setSelectedIndex,
            // which gets called from RenderMenuList::valueChanged, which gets called after the user makes a selection from the menu.
            saveLastSelection(data, element);
            if (RenderMenuList* menuList = toRenderMenuList(element->renderer()))
                menuList->showPopup();
            handled = true;
        }
#elif ARROW_KEYS_POP_MENU
        if (keyCode == ' ') {
            element->focus();

            if (!element->renderer()) // Calling focus() may cause us to lose our renderer, in which case do not want to handle the event.
                return;

            // Save the selection so it can be compared to the new selection when dispatching change events during setSelectedIndex,
            // which gets called from RenderMenuList::valueChanged, which gets called after the user makes a selection from the menu.
            saveLastSelection(data, element);
            if (RenderMenuList* menuList = toRenderMenuList(element->renderer()))
                menuList->showPopup();
            handled = true;
        } else if (keyCode == '\r') {
            if (htmlForm)
                htmlForm->submitImplicitly(event, false);
            menuListOnChange(data, element);
            handled = true;
        }
#else
        int listIndex = optionToListIndex(data, element, selectedIndex(data, element));
        if (keyCode == '\r') {
            // listIndex should already be selected, but this will fire the onchange handler.
            setSelectedIndex(data, element, listToOptionIndex(data, element, listIndex), true, true);
            handled = true;
        }
#endif
        if (handled)
            event->setDefaultHandled();
    }

    if (event->type() == eventNames().mousedownEvent && event->isMouseEvent() && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        element->focus();
        if (element->renderer() && element->renderer()->isMenuList()) {
            if (RenderMenuList* menuList = toRenderMenuList(element->renderer())) {
                if (menuList->popupIsVisible())
                    menuList->hidePopup();
                else {
                    // Save the selection so it can be compared to the new selection when we call onChange during setSelectedIndex,
                    // which gets called from RenderMenuList::valueChanged, which gets called after the user makes a selection from the menu.
                    saveLastSelection(data, element);
                    menuList->showPopup();
                }
            }
        }
        event->setDefaultHandled();
    }
}

void SelectElement::updateSelectedState(SelectElementData& data, Element* element, int listIndex,
                                        bool multi, bool shift)
{
    ASSERT(listIndex >= 0);

    // Save the selection so it can be compared to the new selection when dispatching change events during mouseup, or after autoscroll finishes.
    saveLastSelection(data, element);

    data.setActiveSelectionState(true);

    bool shiftSelect = data.multiple() && shift;
    bool multiSelect = data.multiple() && multi && !shift;

    Element* clickedElement = data.listItems(element)[listIndex];
    OptionElement* option = toOptionElement(clickedElement);
    if (option) {
        // Keep track of whether an active selection (like during drag selection), should select or deselect
        if (option->selected() && multi)
            data.setActiveSelectionState(false);

        if (!data.activeSelectionState())
            option->setSelectedState(false);
    }

    // If we're not in any special multiple selection mode, then deselect all other items, excluding the clicked option.
    // If no option was clicked, then this will deselect all items in the list.
    if (!shiftSelect && !multiSelect)
        deselectItems(data, element, clickedElement);

    // If the anchor hasn't been set, and we're doing a single selection or a shift selection, then initialize the anchor to the first selected index.
    if (data.activeSelectionAnchorIndex() < 0 && !multiSelect)
        setActiveSelectionAnchorIndex(data, element, selectedIndex(data, element));

    // Set the selection state of the clicked option
    if (option && !clickedElement->disabled())
        option->setSelectedState(true);

    // If there was no selectedIndex() for the previous initialization, or
    // If we're doing a single selection, or a multiple selection (using cmd or ctrl), then initialize the anchor index to the listIndex that just got clicked.
    if (data.activeSelectionAnchorIndex() < 0 || !shiftSelect)
        setActiveSelectionAnchorIndex(data, element, listIndex);

    setActiveSelectionEndIndex(data, listIndex);
    updateListBoxSelection(data, element, !multiSelect);
}

void SelectElement::listBoxDefaultEventHandler(SelectElementData& data, Element* element, Event* event, HTMLFormElement* htmlForm)
{
    const Vector<Element*>& listItems = data.listItems(element);

    if (event->type() == eventNames().mousedownEvent && event->isMouseEvent() && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        element->focus();

        if (!element->renderer()) // Calling focus() may cause us to lose our renderer, in which case do not want to handle the event.
            return;

        // Convert to coords relative to the list box if needed.
        MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
        IntPoint localOffset = roundedIntPoint(element->renderer()->absoluteToLocal(mouseEvent->absoluteLocation(), false, true));
        int listIndex = toRenderListBox(element->renderer())->listIndexAtOffset(localOffset.x(), localOffset.y());
        if (listIndex >= 0) {
#if PLATFORM(MAC) || (PLATFORM(CHROMIUM) && OS(DARWIN))
            updateSelectedState(data, element, listIndex, mouseEvent->metaKey(), mouseEvent->shiftKey());
#else
            updateSelectedState(data, element, listIndex, mouseEvent->ctrlKey(), mouseEvent->shiftKey());
#endif
            if (Frame* frame = element->document()->frame())
                frame->eventHandler()->setMouseDownMayStartAutoscroll();

            event->setDefaultHandled();
        }
    } else if (event->type() == eventNames().mouseupEvent && event->isMouseEvent() && static_cast<MouseEvent*>(event)->button() == LeftButton && element->document()->frame()->eventHandler()->autoscrollRenderer() != element->renderer()) {
        // This makes sure we fire dispatchFormControlChangeEvent for a single click.  For drag selection, onChange will fire when the autoscroll timer stops.
        listBoxOnChange(data, element);
    } else if (event->type() == eventNames().keydownEvent) {
        if (!event->isKeyboardEvent())
            return;
        const String& keyIdentifier = static_cast<KeyboardEvent*>(event)->keyIdentifier();

        bool handled = false;
        int endIndex = 0;
        if (data.activeSelectionEndIndex() < 0) {
            // Initialize the end index
            if (keyIdentifier == "Down" || keyIdentifier == "PageDown") {
                int startIndex = lastSelectedListIndex(data, element);
                handled = true;
                if (keyIdentifier == "Down")
                    endIndex = nextSelectableListIndex(data, element, startIndex);
                else
                    endIndex = nextSelectableListIndexPageAway(data, element, startIndex, SkipForwards);
            } else if (keyIdentifier == "Up" || keyIdentifier == "PageUp") {
                int startIndex = optionToListIndex(data, element, selectedIndex(data, element));
                handled = true;
                if (keyIdentifier == "Up")
                    endIndex = previousSelectableListIndex(data, element, startIndex);
                else
                    endIndex = nextSelectableListIndexPageAway(data, element, startIndex, SkipBackwards);
            }
        } else {
            // Set the end index based on the current end index
            if (keyIdentifier == "Down") {
                endIndex = nextSelectableListIndex(data, element, data.activeSelectionEndIndex());
                handled = true;
            } else if (keyIdentifier == "Up") {
                endIndex = previousSelectableListIndex(data, element, data.activeSelectionEndIndex());
                handled = true;
            } else if (keyIdentifier == "PageDown") {
                endIndex = nextSelectableListIndexPageAway(data, element, data.activeSelectionEndIndex(), SkipForwards);
                handled = true;
            } else if (keyIdentifier == "PageUp") {
                endIndex = nextSelectableListIndexPageAway(data, element, data.activeSelectionEndIndex(), SkipBackwards);
                handled = true;
            }
        }
        if (keyIdentifier == "Home") {
            endIndex = firstSelectableListIndex(data, element);
            handled = true;
        } else if (keyIdentifier == "End") {
            endIndex = lastSelectableListIndex(data, element);
            handled = true;
        }

        if (isSpatialNavigationEnabled(element->document()->frame()))
            // Check if the selection moves to the boundary.
            if (keyIdentifier == "Left" || keyIdentifier == "Right" || ((keyIdentifier == "Down" || keyIdentifier == "Up") && endIndex == data.activeSelectionEndIndex()))
                return;

        if (endIndex >= 0 && handled) {
            // Save the selection so it can be compared to the new selection when dispatching change events immediately after making the new selection.
            saveLastSelection(data, element);

            ASSERT_UNUSED(listItems, !listItems.size() || (endIndex >= 0 && static_cast<unsigned>(endIndex) < listItems.size()));
            setActiveSelectionEndIndex(data, endIndex);

            bool selectNewItem = !data.multiple() || static_cast<KeyboardEvent*>(event)->shiftKey() || !isSpatialNavigationEnabled(element->document()->frame());
            if (selectNewItem)
                data.setActiveSelectionState(true);
            // If the anchor is unitialized, or if we're going to deselect all other options, then set the anchor index equal to the end index.
            bool deselectOthers = !data.multiple() || (!static_cast<KeyboardEvent*>(event)->shiftKey() && selectNewItem);
            if (data.activeSelectionAnchorIndex() < 0 || deselectOthers) {
                if (deselectOthers)
                    deselectItems(data, element);
                setActiveSelectionAnchorIndex(data, element, data.activeSelectionEndIndex());
            }

            toRenderListBox(element->renderer())->scrollToRevealElementAtListIndex(endIndex);
            if (selectNewItem) {
                updateListBoxSelection(data, element, deselectOthers);
                listBoxOnChange(data, element);
            } else
                scrollToSelection(data, element);

            event->setDefaultHandled();
        }
    } else if (event->type() == eventNames().keypressEvent) {
        if (!event->isKeyboardEvent())
            return;
        int keyCode = static_cast<KeyboardEvent*>(event)->keyCode();

        if (keyCode == '\r') {
            if (htmlForm)
                htmlForm->submitImplicitly(event, false);
            event->setDefaultHandled();
        } else if (data.multiple() && keyCode == ' ' && isSpatialNavigationEnabled(element->document()->frame())) {
            // Use space to toggle selection change.
            data.setActiveSelectionState(!data.activeSelectionState());
            updateSelectedState(data, element, listToOptionIndex(data, element, data.activeSelectionEndIndex()), true /*multi*/, false /*shift*/);
            listBoxOnChange(data, element);
            event->setDefaultHandled();
        }
    }
}

void SelectElement::defaultEventHandler(SelectElementData& data, Element* element, Event* event, HTMLFormElement* htmlForm)
{
    if (!element->renderer())
        return;

    if (data.usesMenuList())
        menuListDefaultEventHandler(data, element, event, htmlForm);
    else 
        listBoxDefaultEventHandler(data, element, event, htmlForm);

    if (event->defaultHandled())
        return;

    if (event->type() == eventNames().keypressEvent && event->isKeyboardEvent()) {
        KeyboardEvent* keyboardEvent = static_cast<KeyboardEvent*>(event);
        if (!keyboardEvent->ctrlKey() && !keyboardEvent->altKey() && !keyboardEvent->metaKey() && isPrintableChar(keyboardEvent->charCode())) {
            typeAheadFind(data, element, keyboardEvent);
            event->setDefaultHandled();
            return;
        }
    }
}

int SelectElement::lastSelectedListIndex(const SelectElementData& data, const Element* element)
{
    // return the number of the last option selected
    unsigned index = 0;
    bool found = false;
    const Vector<Element*>& items = data.listItems(element);
    for (size_t i = 0; i < items.size(); ++i) {
        if (OptionElement* optionElement = toOptionElement(items[i])) {
            if (optionElement->selected()) {
                index = i;
                found = true;
            }
        }
    }

    return found ? (int) index : -1;
}

static String stripLeadingWhiteSpace(const String& string)
{
    int length = string.length();

    int i;
    for (i = 0; i < length; ++i) {
        if (string[i] != noBreakSpace && (string[i] <= 0x7F ? !isASCIISpace(string[i]) : (direction(string[i]) != WhiteSpaceNeutral)))
            break;
    }

    return string.substring(i, length - i);
}

void SelectElement::typeAheadFind(SelectElementData& data, Element* element, KeyboardEvent* event)
{
    if (event->timeStamp() < data.lastCharTime())
        return;

    DOMTimeStamp delta = event->timeStamp() - data.lastCharTime();
    data.setLastCharTime(event->timeStamp());

    UChar c = event->charCode();

    String prefix;
    int searchStartOffset = 1;
    if (delta > typeAheadTimeout) {
        prefix = String(&c, 1);
        data.setTypedString(prefix);
        data.setRepeatingChar(c);
    } else {
        data.typedString().append(c);

        if (c == data.repeatingChar())
            // The user is likely trying to cycle through all the items starting with this character, so just search on the character
            prefix = String(&c, 1);
        else {
            data.setRepeatingChar(0);
            prefix = data.typedString();
            searchStartOffset = 0;
        }
    }

    const Vector<Element*>& items = data.listItems(element);
    int itemCount = items.size();
    if (itemCount < 1)
        return;

    int selected = selectedIndex(data, element);
    int index = (optionToListIndex(data, element, selected >= 0 ? selected : 0) + searchStartOffset) % itemCount;
    ASSERT(index >= 0);

    // Compute a case-folded copy of the prefix string before beginning the search for
    // a matching element. This code uses foldCase to work around the fact that
    // String::startWith does not fold non-ASCII characters. This code can be changed
    // to use startWith once that is fixed.
    String prefixWithCaseFolded(prefix.foldCase());
    for (int i = 0; i < itemCount; ++i, index = (index + 1) % itemCount) {
        OptionElement* optionElement = toOptionElement(items[index]);
        if (!optionElement || items[index]->disabled())
            continue;

        // Fold the option string and check if its prefix is equal to the folded prefix.
        String text = optionElement->textIndentedToRespectGroupLabel();
        if (stripLeadingWhiteSpace(text).foldCase().startsWith(prefixWithCaseFolded)) {
            setSelectedIndex(data, element, listToOptionIndex(data, element, index));
            if (!data.usesMenuList())
                listBoxOnChange(data, element);

            setOptionsChangedOnRenderer(data, element);
            element->setNeedsStyleRecalc();
            return;
        }
    }
}

void SelectElement::insertedIntoTree(SelectElementData& data, Element* element)
{
    // When the element is created during document parsing, it won't have any items yet - but for innerHTML
    // and related methods, this method is called after the whole subtree is constructed.
    recalcListItems(data, element, true);
}

void SelectElement::accessKeySetSelectedIndex(SelectElementData& data, Element* element, int index)
{    
    // first bring into focus the list box
    if (!element->focused())
        element->accessKeyAction(false);
    
    // if this index is already selected, unselect. otherwise update the selected index
    const Vector<Element*>& items = data.listItems(element);
    int listIndex = optionToListIndex(data, element, index);
    if (OptionElement* optionElement = (listIndex >= 0 ? toOptionElement(items[listIndex]) : 0)) {
        if (optionElement->selected())
            optionElement->setSelectedState(false);
        else
            setSelectedIndex(data, element, index, false, true);
    }

    if (data.usesMenuList())
        menuListOnChange(data, element);
    else
        listBoxOnChange(data, element);

    scrollToSelection(data, element);
}

unsigned SelectElement::optionCount(const SelectElementData& data, const Element* element)
{
    unsigned options = 0;

    const Vector<Element*>& items = data.listItems(element);
    for (unsigned i = 0; i < items.size(); ++i) {
        if (isOptionElement(items[i]))
            ++options;
    }

    return options;
}

// SelectElementData
SelectElementData::SelectElementData()
    : m_multiple(false) 
    , m_size(0)
    , m_lastOnChangeIndex(-1)
    , m_activeSelectionState(false)
    , m_activeSelectionAnchorIndex(-1)
    , m_activeSelectionEndIndex(-1)
    , m_recalcListItems(false)
    , m_repeatingChar(0)
    , m_lastCharTime(0)
{
}

SelectElementData::~SelectElementData()
{
}

void SelectElementData::checkListItems(const Element* element) const
{
#if !ASSERT_DISABLED
    Vector<Element*> items = m_listItems;
    SelectElement::recalcListItems(*const_cast<SelectElementData*>(this), element, false);
    ASSERT(items == m_listItems);
#else
    UNUSED_PARAM(element);
#endif
}

Vector<Element*>& SelectElementData::listItems(const Element* element)
{
    if (m_recalcListItems)
        SelectElement::recalcListItems(*this, element);
    else
        checkListItems(element);

    return m_listItems;
}

const Vector<Element*>& SelectElementData::listItems(const Element* element) const
{
    if (m_recalcListItems)
        SelectElement::recalcListItems(*const_cast<SelectElementData*>(this), element);
    else
        checkListItems(element);

    return m_listItems;
}

SelectElement* toSelectElement(Element* element)
{
    if (element->isHTMLElement() && element->hasTagName(HTMLNames::selectTag))
        return static_cast<HTMLSelectElement*>(element);
    return 0;
}

}
