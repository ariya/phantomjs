/*
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Alonzo
 * Copyright (C) 2010, 2011, 2012 Igalia S.L.
 *
 * Portions from Mozilla a11y, copyright as follows:
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Sun Microsystems, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
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
 */

#include "config.h"
#include "WebKitAccessibleInterfaceSelection.h"

#if HAVE(ACCESSIBILITY)

#include "AccessibilityListBox.h"
#include "AccessibilityObject.h"
#include "HTMLSelectElement.h"
#include "RenderObject.h"
#include "WebKitAccessibleWrapperAtk.h"

using namespace WebCore;

static AccessibilityObject* core(AtkSelection* selection)
{
    if (!WEBKIT_IS_ACCESSIBLE(selection))
        return 0;

    return webkitAccessibleGetAccessibilityObject(WEBKIT_ACCESSIBLE(selection));
}

static AccessibilityObject* listObjectForSelection(AtkSelection* selection)
{
    AccessibilityObject* coreSelection = core(selection);

    // Only list boxes and menu lists supported so far.
    if (!coreSelection->isListBox() && !coreSelection->isMenuList())
        return 0;

    // For list boxes the list object is just itself.
    if (coreSelection->isListBox())
        return coreSelection;

    // For menu lists we need to return the first accessible child,
    // with role MenuListPopupRole, since that's the one holding the list
    // of items with role MenuListOptionRole.
    AccessibilityObject::AccessibilityChildrenVector children = coreSelection->children();
    if (!children.size())
        return 0;

    AccessibilityObject* listObject = children.at(0).get();
    if (!listObject->isMenuListPopup())
        return 0;

    return listObject;
}

static AccessibilityObject* optionFromList(AtkSelection* selection, gint index)
{
    AccessibilityObject* coreSelection = core(selection);
    if (!coreSelection || index < 0)
        return 0;

    // Need to select the proper list object depending on the type.
    AccessibilityObject* listObject = listObjectForSelection(selection);
    if (!listObject)
        return 0;

    AccessibilityObject::AccessibilityChildrenVector options = listObject->children();
    if (index < static_cast<gint>(options.size()))
        return options.at(index).get();

    return 0;
}

static AccessibilityObject* optionFromSelection(AtkSelection* selection, gint index)
{
    // i is the ith selection as opposed to the ith child.

    AccessibilityObject* coreSelection = core(selection);
    if (!coreSelection || !coreSelection->isAccessibilityRenderObject() || index < 0)
        return 0;

    AccessibilityObject::AccessibilityChildrenVector selectedItems;
    if (coreSelection->isListBox())
        coreSelection->selectedChildren(selectedItems);
    else if (coreSelection->isMenuList()) {
        RenderObject* renderer = coreSelection->renderer();
        if (!renderer)
            return 0;

        HTMLSelectElement* selectNode = toHTMLSelectElement(renderer->node());
        int selectedIndex = selectNode->selectedIndex();
        const Vector<HTMLElement*> listItems = selectNode->listItems();

        if (selectedIndex < 0 || selectedIndex >= static_cast<int>(listItems.size()))
            return 0;

        return optionFromList(selection, selectedIndex);
    }

    if (index < static_cast<gint>(selectedItems.size()))
        return selectedItems.at(index).get();

    return 0;
}

static gboolean webkitAccessibleSelectionAddSelection(AtkSelection* selection, gint index)
{
    AccessibilityObject* coreSelection = core(selection);
    if (!coreSelection)
        return FALSE;

    AccessibilityObject* option = optionFromList(selection, index);
    if (option && (coreSelection->isListBox() || coreSelection->isMenuList())) {
        option->setSelected(true);
        return option->isSelected();
    }

    return FALSE;
}

static gboolean webkitAccessibleSelectionClearSelection(AtkSelection* selection)
{
    AccessibilityObject* coreSelection = core(selection);
    if (!coreSelection)
        return FALSE;

    AccessibilityObject::AccessibilityChildrenVector selectedItems;
    if (coreSelection->isListBox() || coreSelection->isMenuList()) {
        // Set the list of selected items to an empty list; then verify that it worked.
        AccessibilityListBox* listBox = static_cast<AccessibilityListBox*>(coreSelection);
        listBox->setSelectedChildren(selectedItems);
        listBox->selectedChildren(selectedItems);
        return !selectedItems.size();
    }
    return FALSE;
}

static AtkObject* webkitAccessibleSelectionRefSelection(AtkSelection* selection, gint index)
{
    AccessibilityObject* option = optionFromSelection(selection, index);
    if (option) {
        AtkObject* child = option->wrapper();
        g_object_ref(child);
        return child;
    }

    return 0;
}

static gint webkitAccessibleSelectionGetSelectionCount(AtkSelection* selection)
{
    AccessibilityObject* coreSelection = core(selection);
    if (!coreSelection || !coreSelection->isAccessibilityRenderObject())
        return 0;

    if (coreSelection->isListBox()) {
        AccessibilityObject::AccessibilityChildrenVector selectedItems;
        coreSelection->selectedChildren(selectedItems);
        return static_cast<gint>(selectedItems.size());
    }

    if (coreSelection->isMenuList()) {
        RenderObject* renderer = coreSelection->renderer();
        if (!renderer)
            return 0;

        int selectedIndex = toHTMLSelectElement(renderer->node())->selectedIndex();
        return selectedIndex >= 0 && selectedIndex < static_cast<int>(toHTMLSelectElement(renderer->node())->listItems().size());
    }

    return 0;
}

static gboolean webkitAccessibleSelectionIsChildSelected(AtkSelection* selection, gint index)
{
    AccessibilityObject* coreSelection = core(selection);
    if (!coreSelection)
        return 0;

    AccessibilityObject* option = optionFromList(selection, index);
    if (option && (coreSelection->isListBox() || coreSelection->isMenuList()))
        return option->isSelected();

    return FALSE;
}

static gboolean webkitAccessibleSelectionRemoveSelection(AtkSelection* selection, gint index)
{
    AccessibilityObject* coreSelection = core(selection);
    if (!coreSelection)
        return 0;

    // TODO: This is only getting called if i == 0. What is preventing the rest?
    AccessibilityObject* option = optionFromSelection(selection, index);
    if (option && (coreSelection->isListBox() || coreSelection->isMenuList())) {
        option->setSelected(false);
        return !option->isSelected();
    }

    return FALSE;
}

static gboolean webkitAccessibleSelectionSelectAllSelection(AtkSelection* selection)
{
    AccessibilityObject* coreSelection = core(selection);
    if (!coreSelection || !coreSelection->isMultiSelectable())
        return FALSE;

    AccessibilityObject::AccessibilityChildrenVector children = coreSelection->children();
    if (coreSelection->isListBox()) {
        AccessibilityListBox* listBox = static_cast<AccessibilityListBox*>(coreSelection);
        listBox->setSelectedChildren(children);
        AccessibilityObject::AccessibilityChildrenVector selectedItems;
        listBox->selectedChildren(selectedItems);
        return selectedItems.size() == children.size();
    }

    return FALSE;
}

void webkitAccessibleSelectionInterfaceInit(AtkSelectionIface* iface)
{
    iface->add_selection = webkitAccessibleSelectionAddSelection;
    iface->clear_selection = webkitAccessibleSelectionClearSelection;
    iface->ref_selection = webkitAccessibleSelectionRefSelection;
    iface->get_selection_count = webkitAccessibleSelectionGetSelectionCount;
    iface->is_child_selected = webkitAccessibleSelectionIsChildSelected;
    iface->remove_selection = webkitAccessibleSelectionRemoveSelection;
    iface->select_all_selection = webkitAccessibleSelectionSelectAllSelection;
}

#endif
