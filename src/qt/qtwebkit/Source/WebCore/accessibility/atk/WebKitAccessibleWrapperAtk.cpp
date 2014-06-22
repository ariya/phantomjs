/*
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Alonzo
 * Copyright (C) 2009, 2010, 2011, 2012 Igalia S.L.
 * Copyright (C) 2013 Samsung Electronics
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
#include "WebKitAccessibleWrapperAtk.h"

#if HAVE(ACCESSIBILITY)

#include "AXObjectCache.h"
#include "Document.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLNames.h"
#include "HTMLTableElement.h"
#include "HostWindow.h"
#include "RenderObject.h"
#include "Settings.h"
#include "TextIterator.h"
#include "VisibleUnits.h"
#include "WebKitAccessibleHyperlink.h"
#include "WebKitAccessibleInterfaceAction.h"
#include "WebKitAccessibleInterfaceComponent.h"
#include "WebKitAccessibleInterfaceDocument.h"
#include "WebKitAccessibleInterfaceEditableText.h"
#include "WebKitAccessibleInterfaceHyperlinkImpl.h"
#include "WebKitAccessibleInterfaceHypertext.h"
#include "WebKitAccessibleInterfaceImage.h"
#include "WebKitAccessibleInterfaceSelection.h"
#include "WebKitAccessibleInterfaceTable.h"
#include "WebKitAccessibleInterfaceText.h"
#include "WebKitAccessibleInterfaceValue.h"
#include "WebKitAccessibleUtil.h"
#include "htmlediting.h"
#include <glib/gprintf.h>
#include <wtf/text/CString.h>

#if PLATFORM(GTK)
#include <gtk/gtk.h>
#endif

using namespace WebCore;

struct _WebKitAccessiblePrivate {
    // Cached data for AtkObject.
    CString accessibleName;
    CString accessibleDescription;

    // Cached data for AtkAction.
    CString actionName;
    CString actionKeyBinding;

    // Cached data for AtkDocument.
    CString documentLocale;
    CString documentType;
    CString documentEncoding;
    CString documentURI;

    // Cached data for AtkImage.
    CString imageDescription;
};

#define WEBKIT_ACCESSIBLE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_ACCESSIBLE, WebKitAccessiblePrivate))

static AccessibilityObject* fallbackObject()
{
    // FIXME: An AXObjectCache with a Document is meaningless.
    static AXObjectCache* fallbackCache = new AXObjectCache(0);
    static AccessibilityObject* object = 0;
    if (!object) {
        // FIXME: using fallbackCache->getOrCreate(ListBoxOptionRole) is a hack
        object = fallbackCache->getOrCreate(ListBoxOptionRole);
        object->ref();
    }

    return object;
}

static AccessibilityObject* core(WebKitAccessible* accessible)
{
    if (!accessible)
        return 0;

    return accessible->m_object;
}

static AccessibilityObject* core(AtkObject* object)
{
    if (!WEBKIT_IS_ACCESSIBLE(object))
        return 0;

    return core(WEBKIT_ACCESSIBLE(object));
}

static const gchar* webkitAccessibleGetName(AtkObject* object)
{
    AccessibilityObject* coreObject = core(object);
    if (!coreObject->isAccessibilityRenderObject())
        return cacheAndReturnAtkProperty(object, AtkCachedAccessibleName, coreObject->stringValue());

    if (coreObject->isFieldset()) {
        AccessibilityObject* label = coreObject->titleUIElement();
        if (label) {
            AtkObject* atkObject = label->wrapper();
            if (ATK_IS_TEXT(atkObject))
                return atk_text_get_text(ATK_TEXT(atkObject), 0, -1);
        }
    }

    if (coreObject->isControl()) {
        AccessibilityObject* label = coreObject->correspondingLabelForControlElement();
        if (label) {
            AtkObject* atkObject = label->wrapper();
            if (ATK_IS_TEXT(atkObject))
                return atk_text_get_text(ATK_TEXT(atkObject), 0, -1);
        }

        // Try text under the node.
        String textUnder = coreObject->textUnderElement();
        if (textUnder.length())
            return cacheAndReturnAtkProperty(object, AtkCachedAccessibleName, textUnder);
    }

    if (coreObject->isImage() || coreObject->isInputImage()) {
        Node* node = coreObject->node();
        if (node && node->isHTMLElement()) {
            // Get the attribute rather than altText String so as not to fall back on title.
            String alt = toHTMLElement(node)->getAttribute(HTMLNames::altAttr);
            if (!alt.isEmpty())
                return cacheAndReturnAtkProperty(object, AtkCachedAccessibleName, alt);
        }
    }

    // Fallback for the webArea object: just return the document's title.
    if (coreObject->isWebArea()) {
        Document* document = coreObject->document();
        if (document)
            return cacheAndReturnAtkProperty(object, AtkCachedAccessibleName, document->title());
    }

    // Nothing worked so far, try with the AccessibilityObject's
    // title() before going ahead with stringValue().
    String axTitle = accessibilityTitle(coreObject);
    if (!axTitle.isEmpty())
        return cacheAndReturnAtkProperty(object, AtkCachedAccessibleName, axTitle);

    return cacheAndReturnAtkProperty(object, AtkCachedAccessibleName, coreObject->stringValue());
}

static const gchar* webkitAccessibleGetDescription(AtkObject* object)
{
    AccessibilityObject* coreObject = core(object);
    Node* node = 0;
    if (coreObject->isAccessibilityRenderObject())
        node = coreObject->node();
    if (!node || !node->isHTMLElement() || coreObject->ariaRoleAttribute() != UnknownRole)
        return cacheAndReturnAtkProperty(object, AtkCachedAccessibleDescription, accessibilityDescription(coreObject));

    // atk_table_get_summary returns an AtkObject. We have no summary object, so expose summary here.
    if (coreObject->roleValue() == TableRole) {
        String summary = toHTMLTableElement(node)->summary();
        if (!summary.isEmpty())
            return cacheAndReturnAtkProperty(object, AtkCachedAccessibleDescription, summary);
    }

    // The title attribute should be reliably available as the object's descripton.
    // We do not want to fall back on other attributes in its absence. See bug 25524.
    String title = toHTMLElement(node)->title();
    if (!title.isEmpty())
        return cacheAndReturnAtkProperty(object, AtkCachedAccessibleDescription, title);

    return cacheAndReturnAtkProperty(object, AtkCachedAccessibleDescription, accessibilityDescription(coreObject));
}

static void setAtkRelationSetFromCoreObject(AccessibilityObject* coreObject, AtkRelationSet* relationSet)
{
    if (coreObject->isFieldset()) {
        AccessibilityObject* label = coreObject->titleUIElement();
        if (label)
            atk_relation_set_add_relation_by_type(relationSet, ATK_RELATION_LABELLED_BY, label->wrapper());
        return;
    }

    if (coreObject->roleValue() == LegendRole) {
        for (AccessibilityObject* parent = coreObject->parentObjectUnignored(); parent; parent = parent->parentObjectUnignored()) {
            if (parent->isFieldset()) {
                atk_relation_set_add_relation_by_type(relationSet, ATK_RELATION_LABEL_FOR, parent->wrapper());
                break;
            }
        }
        return;
    }

    if (coreObject->isControl()) {
        AccessibilityObject* label = coreObject->correspondingLabelForControlElement();
        if (label)
            atk_relation_set_add_relation_by_type(relationSet, ATK_RELATION_LABELLED_BY, label->wrapper());
    } else {
        AccessibilityObject* control = coreObject->correspondingControlForLabelElement();
        if (control)
            atk_relation_set_add_relation_by_type(relationSet, ATK_RELATION_LABEL_FOR, control->wrapper());
    }
}

static gpointer webkitAccessibleParentClass = 0;

static bool isRootObject(AccessibilityObject* coreObject)
{
    // The root accessible object in WebCore is always an object with
    // the ScrolledArea role with one child with the WebArea role.
    if (!coreObject || !coreObject->isScrollView())
        return false;

    AccessibilityObject* firstChild = coreObject->firstChild();
    if (!firstChild || !firstChild->isWebArea())
        return false;

    return true;
}

static AtkObject* atkParentOfRootObject(AtkObject* object)
{
    AccessibilityObject* coreObject = core(object);
    AccessibilityObject* coreParent = coreObject->parentObjectUnignored();

    // The top level object claims to not have a parent. This makes it
    // impossible for assistive technologies to ascend the accessible
    // hierarchy all the way to the application. (Bug 30489)
    if (!coreParent && isRootObject(coreObject)) {
        Document* document = coreObject->document();
        if (!document)
            return 0;

#if PLATFORM(GTK)
        HostWindow* hostWindow = document->view()->hostWindow();
        if (hostWindow) {
            PlatformPageClient scrollView = hostWindow->platformPageClient();
            if (scrollView) {
                GtkWidget* scrollViewParent = gtk_widget_get_parent(scrollView);
                if (scrollViewParent)
                    return gtk_widget_get_accessible(scrollViewParent);
            }
        }
#endif // PLATFORM(GTK)
    }

    if (!coreParent)
        return 0;

    return coreParent->wrapper();
}

static AtkObject* webkitAccessibleGetParent(AtkObject* object)
{
    // Check first if the parent has been already set.
    AtkObject* accessibleParent = ATK_OBJECT_CLASS(webkitAccessibleParentClass)->get_parent(object);
    if (accessibleParent)
        return accessibleParent;

    // Parent not set yet, so try to find it in the hierarchy.
    AccessibilityObject* coreObject = core(object);
    if (!coreObject)
        return 0;

    AccessibilityObject* coreParent = coreObject->parentObjectUnignored();

    if (!coreParent && isRootObject(coreObject))
        return atkParentOfRootObject(object);

    if (!coreParent)
        return 0;

    // We don't expose table rows to Assistive technologies, but we
    // need to have them anyway in the hierarchy from WebCore to
    // properly perform coordinates calculations when requested.
    if (coreParent->isTableRow() && coreObject->isTableCell())
        coreParent = coreParent->parentObjectUnignored();

    return coreParent->wrapper();
}

static gint getNChildrenForTable(AccessibilityObject* coreObject)
{
    AccessibilityObject::AccessibilityChildrenVector tableChildren = coreObject->children();
    size_t tableChildrenCount = tableChildren.size();
    size_t cellsCount = 0;

    // Look for the actual index of the cell inside the table.
    for (unsigned i = 0; i < tableChildrenCount; ++i) {
        if (tableChildren[i]->isTableRow()) {
            AccessibilityObject::AccessibilityChildrenVector rowChildren = tableChildren[i]->children();
            cellsCount += rowChildren.size();
        } else
            cellsCount++;
    }

    return cellsCount;
}

static gint webkitAccessibleGetNChildren(AtkObject* object)
{
    AccessibilityObject* coreObject = core(object);

    // Tables should be treated in a different way because rows should
    // be bypassed when exposing the accessible hierarchy.
    if (coreObject->isAccessibilityTable())
        return getNChildrenForTable(coreObject);

    return coreObject->children().size();
}

static AccessibilityObject* getChildForTable(AccessibilityObject* coreObject, gint index)
{
    AccessibilityObject::AccessibilityChildrenVector tableChildren = coreObject->children();
    size_t tableChildrenCount = tableChildren.size();
    size_t cellsCount = 0;

    // Look for the actual index of the cell inside the table.
    size_t current = static_cast<size_t>(index);
    for (unsigned i = 0; i < tableChildrenCount; ++i) {
        if (tableChildren[i]->isTableRow()) {
            AccessibilityObject::AccessibilityChildrenVector rowChildren = tableChildren[i]->children();
            size_t rowChildrenCount = rowChildren.size();
            if (current < cellsCount + rowChildrenCount)
                return rowChildren.at(current - cellsCount).get();
            cellsCount += rowChildrenCount;
        } else if (cellsCount == current)
            return tableChildren[i].get();
        else
            cellsCount++;
    }

    // Shouldn't reach if the child was found.
    return 0;
}

static AtkObject* webkitAccessibleRefChild(AtkObject* object, gint index)
{
    if (index < 0)
        return 0;

    AccessibilityObject* coreObject = core(object);
    AccessibilityObject* coreChild = 0;

    // Tables are special cases because rows should be bypassed, but
    // still taking their cells into account.
    if (coreObject->isAccessibilityTable())
        coreChild = getChildForTable(coreObject, index);
    else {
        AccessibilityObject::AccessibilityChildrenVector children = coreObject->children();
        if (static_cast<unsigned>(index) >= children.size())
            return 0;
        coreChild = children.at(index).get();
    }

    if (!coreChild)
        return 0;

    AtkObject* child = coreChild->wrapper();
    atk_object_set_parent(child, object);
    g_object_ref(child);

    return child;
}

static gint getIndexInParentForCellInRow(AccessibilityObject* coreObject)
{
    AccessibilityObject* parent = coreObject->parentObjectUnignored();
    if (!parent)
        return -1;

    AccessibilityObject* grandParent = parent->parentObjectUnignored();
    if (!grandParent)
        return -1;

    AccessibilityObject::AccessibilityChildrenVector rows = grandParent->children();
    size_t rowsCount = rows.size();
    size_t previousCellsCount = 0;

    // Look for the actual index of the cell inside the table.
    for (unsigned i = 0; i < rowsCount; ++i) {
        if (!rows[i]->isTableRow())
            continue;

        AccessibilityObject::AccessibilityChildrenVector cells = rows[i]->children();
        size_t cellsCount = cells.size();

        if (rows[i] == parent) {
            for (unsigned j = 0; j < cellsCount; ++j) {
                if (cells[j] == coreObject)
                    return previousCellsCount + j;
            }
        }

        previousCellsCount += cellsCount;
    }

    return -1;
}

static gint webkitAccessibleGetIndexInParent(AtkObject* object)
{
    AccessibilityObject* coreObject = core(object);
    AccessibilityObject* parent = coreObject->parentObjectUnignored();

    if (!parent && isRootObject(coreObject)) {
        AtkObject* atkParent = atkParentOfRootObject(object);
        if (!atkParent)
            return -1;

        unsigned count = atk_object_get_n_accessible_children(atkParent);
        for (unsigned i = 0; i < count; ++i) {
            AtkObject* child = atk_object_ref_accessible_child(atkParent, i);
            bool childIsObject = child == object;
            g_object_unref(child);
            if (childIsObject)
                return i;
        }
    }

    // Need to calculate the index of the cell in the table, as
    // rows won't be exposed to assistive technologies.
    if (parent && parent->isTableRow() && coreObject->isTableCell())
        return getIndexInParentForCellInRow(coreObject);

    if (!parent)
        return -1;

    size_t index = parent->children().find(coreObject);
    return (index == WTF::notFound) ? -1 : index;
}

static AtkAttributeSet* webkitAccessibleGetAttributes(AtkObject* object)
{
    AtkAttributeSet* attributeSet = 0;
#if PLATFORM(GTK)
    attributeSet = addToAtkAttributeSet(attributeSet, "toolkit", "WebKitGtk");
#elif PLATFORM(EFL)
    attributeSet = addToAtkAttributeSet(attributeSet, "toolkit", "WebKitEfl");
#endif

    AccessibilityObject* coreObject = core(object);
    if (!coreObject)
        return attributeSet;

    // Hack needed for WebKit2 tests because obtaining an element by its ID
    // cannot be done from the UIProcess. Assistive technologies have no need
    // for this information.
    Node* node = coreObject->node();
    if (node && node->isElementNode()) {
        String id = toElement(node)->getIdAttribute().string();
        if (!id.isEmpty())
            attributeSet = addToAtkAttributeSet(attributeSet, "html-id", id.utf8().data());
    }

    int headingLevel = coreObject->headingLevel();
    if (headingLevel) {
        String value = String::number(headingLevel);
        attributeSet = addToAtkAttributeSet(attributeSet, "level", value.utf8().data());
    }

    // Set the 'layout-guess' attribute to help Assistive
    // Technologies know when an exposed table is not data table.
    if (coreObject->isAccessibilityTable() && !coreObject->isDataTable())
        attributeSet = addToAtkAttributeSet(attributeSet, "layout-guess", "true");

    String placeholder = coreObject->placeholderValue();
    if (!placeholder.isEmpty())
        attributeSet = addToAtkAttributeSet(attributeSet, "placeholder-text", placeholder.utf8().data());

    if (coreObject->ariaHasPopup())
        attributeSet = addToAtkAttributeSet(attributeSet, "aria-haspopup", "true");

    return attributeSet;
}

static AtkRole atkRole(AccessibilityRole role)
{
    switch (role) {
    case UnknownRole:
        return ATK_ROLE_UNKNOWN;
    case ButtonRole:
        return ATK_ROLE_PUSH_BUTTON;
    case ToggleButtonRole:
        return ATK_ROLE_TOGGLE_BUTTON;
    case RadioButtonRole:
        return ATK_ROLE_RADIO_BUTTON;
    case CheckBoxRole:
        return ATK_ROLE_CHECK_BOX;
    case SliderRole:
        return ATK_ROLE_SLIDER;
    case TabGroupRole:
    case TabListRole:
        return ATK_ROLE_PAGE_TAB_LIST;
    case TextFieldRole:
    case TextAreaRole:
        return ATK_ROLE_ENTRY;
    case StaticTextRole:
        return ATK_ROLE_TEXT;
    case OutlineRole:
        return ATK_ROLE_TREE;
    case MenuBarRole:
        return ATK_ROLE_MENU_BAR;
    case MenuListPopupRole:
    case MenuRole:
        return ATK_ROLE_MENU;
    case MenuListOptionRole:
    case MenuItemRole:
        return ATK_ROLE_MENU_ITEM;
    case ColumnRole:
        // return ATK_ROLE_TABLE_COLUMN_HEADER; // Is this right?
        return ATK_ROLE_UNKNOWN; // Matches Mozilla
    case RowRole:
        // return ATK_ROLE_TABLE_ROW_HEADER; // Is this right?
        return ATK_ROLE_LIST_ITEM; // Matches Mozilla
    case ToolbarRole:
        return ATK_ROLE_TOOL_BAR;
    case BusyIndicatorRole:
        return ATK_ROLE_PROGRESS_BAR; // Is this right?
    case ProgressIndicatorRole:
        // return ATK_ROLE_SPIN_BUTTON; // Some confusion about this role in AccessibilityRenderObject.cpp
        return ATK_ROLE_PROGRESS_BAR;
    case WindowRole:
        return ATK_ROLE_WINDOW;
    case PopUpButtonRole:
    case ComboBoxRole:
        return ATK_ROLE_COMBO_BOX;
    case SplitGroupRole:
        return ATK_ROLE_SPLIT_PANE;
    case SplitterRole:
        return ATK_ROLE_UNKNOWN;
    case ColorWellRole:
        return ATK_ROLE_COLOR_CHOOSER;
    case ListRole:
        return ATK_ROLE_LIST;
    case ScrollBarRole:
        return ATK_ROLE_SCROLL_BAR;
    case ScrollAreaRole:
        return ATK_ROLE_SCROLL_PANE;
    case GridRole: // Is this right?
    case TableRole:
        return ATK_ROLE_TABLE;
    case ApplicationRole:
        return ATK_ROLE_APPLICATION;
    case GroupRole:
    case RadioGroupRole:
    case TabPanelRole:
        return ATK_ROLE_PANEL;
    case RowHeaderRole: // Row headers are cells after all.
    case ColumnHeaderRole: // Column headers are cells after all.
    case CellRole:
        return ATK_ROLE_TABLE_CELL;
    case LinkRole:
    case WebCoreLinkRole:
    case ImageMapLinkRole:
        return ATK_ROLE_LINK;
    case ImageMapRole:
    case ImageRole:
        return ATK_ROLE_IMAGE;
    case ListMarkerRole:
        return ATK_ROLE_TEXT;
    case WebAreaRole:
        // return ATK_ROLE_HTML_CONTAINER; // Is this right?
        return ATK_ROLE_DOCUMENT_FRAME;
    case HeadingRole:
        return ATK_ROLE_HEADING;
    case ListBoxRole:
        return ATK_ROLE_LIST;
    case ListItemRole:
    case ListBoxOptionRole:
        return ATK_ROLE_LIST_ITEM;
    case ParagraphRole:
        return ATK_ROLE_PARAGRAPH;
    case LabelRole:
    case LegendRole:
        return ATK_ROLE_LABEL;
    case DivRole:
        return ATK_ROLE_SECTION;
    case FormRole:
        return ATK_ROLE_FORM;
    case CanvasRole:
        return ATK_ROLE_CANVAS;
    case HorizontalRuleRole:
        return ATK_ROLE_SEPARATOR;
    case SpinButtonRole:
        return ATK_ROLE_SPIN_BUTTON;
    case TabRole:
        return ATK_ROLE_PAGE_TAB;
    default:
        return ATK_ROLE_UNKNOWN;
    }
}

static AtkRole webkitAccessibleGetRole(AtkObject* object)
{
    AccessibilityObject* coreObject = core(object);

    if (!coreObject)
        return ATK_ROLE_UNKNOWN;

    // Note: Why doesn't WebCore have a password field for this
    if (coreObject->isPasswordField())
        return ATK_ROLE_PASSWORD_TEXT;

    return atkRole(coreObject->roleValue());
}

static bool isTextWithCaret(AccessibilityObject* coreObject)
{
    if (!coreObject || !coreObject->isAccessibilityRenderObject())
        return false;

    Document* document = coreObject->document();
    if (!document)
        return false;

    Frame* frame = document->frame();
    if (!frame)
        return false;

    Settings* settings = frame->settings();
    if (!settings || !settings->caretBrowsingEnabled())
        return false;

    // Check text objects and paragraphs only.
    AtkObject* axObject = coreObject->wrapper();
    AtkRole role = axObject ? atk_object_get_role(axObject) : ATK_ROLE_INVALID;
    if (role != ATK_ROLE_TEXT && role != ATK_ROLE_PARAGRAPH)
        return false;

    // Finally, check whether the caret is set in the current object.
    VisibleSelection selection = coreObject->selection();
    if (!selection.isCaret())
        return false;

    return selectionBelongsToObject(coreObject, selection);
}

static void setAtkStateSetFromCoreObject(AccessibilityObject* coreObject, AtkStateSet* stateSet)
{
    AccessibilityObject* parent = coreObject->parentObject();
    bool isListBoxOption = parent && parent->isListBox();

    // Please keep the state list in alphabetical order
    if (coreObject->isChecked())
        atk_state_set_add_state(stateSet, ATK_STATE_CHECKED);

    // FIXME: isReadOnly does not seem to do the right thing for
    // controls, so check explicitly for them. In addition, because
    // isReadOnly is false for listBoxOptions, we need to add one
    // more check so that we do not present them as being "editable".
    if ((!coreObject->isReadOnly()
        || (coreObject->isControl() && coreObject->canSetValueAttribute()))
        && !isListBoxOption)
        atk_state_set_add_state(stateSet, ATK_STATE_EDITABLE);

    // FIXME: Put both ENABLED and SENSITIVE together here for now
    if (coreObject->isEnabled()) {
        atk_state_set_add_state(stateSet, ATK_STATE_ENABLED);
        atk_state_set_add_state(stateSet, ATK_STATE_SENSITIVE);
    }

    if (coreObject->canSetExpandedAttribute())
        atk_state_set_add_state(stateSet, ATK_STATE_EXPANDABLE);

    if (coreObject->isExpanded())
        atk_state_set_add_state(stateSet, ATK_STATE_EXPANDED);

    if (coreObject->canSetFocusAttribute())
        atk_state_set_add_state(stateSet, ATK_STATE_FOCUSABLE);

    if (coreObject->isFocused() || isTextWithCaret(coreObject))
        atk_state_set_add_state(stateSet, ATK_STATE_FOCUSED);

    if (coreObject->orientation() == AccessibilityOrientationHorizontal)
        atk_state_set_add_state(stateSet, ATK_STATE_HORIZONTAL);
    else if (coreObject->orientation() == AccessibilityOrientationVertical)
        atk_state_set_add_state(stateSet, ATK_STATE_VERTICAL);

    if (coreObject->isIndeterminate())
        atk_state_set_add_state(stateSet, ATK_STATE_INDETERMINATE);

    if (coreObject->isMultiSelectable())
        atk_state_set_add_state(stateSet, ATK_STATE_MULTISELECTABLE);

    // TODO: ATK_STATE_OPAQUE

    if (coreObject->isPressed())
        atk_state_set_add_state(stateSet, ATK_STATE_PRESSED);

    if (coreObject->isRequired())
        atk_state_set_add_state(stateSet, ATK_STATE_REQUIRED);

    // TODO: ATK_STATE_SELECTABLE_TEXT

    if (coreObject->canSetSelectedAttribute()) {
        atk_state_set_add_state(stateSet, ATK_STATE_SELECTABLE);
        // Items in focusable lists have both STATE_SELECT{ABLE,ED}
        // and STATE_FOCUS{ABLE,ED}. We'll fake the latter based on
        // the former.
        if (isListBoxOption)
            atk_state_set_add_state(stateSet, ATK_STATE_FOCUSABLE);
    }

    if (coreObject->isSelected()) {
        atk_state_set_add_state(stateSet, ATK_STATE_SELECTED);
        // Items in focusable lists have both STATE_SELECT{ABLE,ED}
        // and STATE_FOCUS{ABLE,ED}. We'll fake the latter based on the
        // former.
        if (isListBoxOption)
            atk_state_set_add_state(stateSet, ATK_STATE_FOCUSED);
    }

    // FIXME: Group both SHOWING and VISIBLE here for now
    // Not sure how to handle this in WebKit, see bug
    // http://bugzilla.gnome.org/show_bug.cgi?id=509650 for other
    // issues with SHOWING vs VISIBLE.
    if (!coreObject->isOffScreen()) {
        atk_state_set_add_state(stateSet, ATK_STATE_SHOWING);
        atk_state_set_add_state(stateSet, ATK_STATE_VISIBLE);
    }

    // Mutually exclusive, so we group these two
    if (coreObject->roleValue() == TextFieldRole)
        atk_state_set_add_state(stateSet, ATK_STATE_SINGLE_LINE);
    else if (coreObject->roleValue() == TextAreaRole)
        atk_state_set_add_state(stateSet, ATK_STATE_MULTI_LINE);

    // TODO: ATK_STATE_SENSITIVE

    if (coreObject->isVisited())
        atk_state_set_add_state(stateSet, ATK_STATE_VISITED);
}

static AtkStateSet* webkitAccessibleRefStateSet(AtkObject* object)
{
    AtkStateSet* stateSet = ATK_OBJECT_CLASS(webkitAccessibleParentClass)->ref_state_set(object);
    AccessibilityObject* coreObject = core(object);

    if (coreObject == fallbackObject()) {
        atk_state_set_add_state(stateSet, ATK_STATE_DEFUNCT);
        return stateSet;
    }

    // Text objects must be focusable.
    AtkRole role = atk_object_get_role(object);
    if (role == ATK_ROLE_TEXT || role == ATK_ROLE_PARAGRAPH)
        atk_state_set_add_state(stateSet, ATK_STATE_FOCUSABLE);

    setAtkStateSetFromCoreObject(coreObject, stateSet);
    return stateSet;
}

static AtkRelationSet* webkitAccessibleRefRelationSet(AtkObject* object)
{
    AtkRelationSet* relationSet = ATK_OBJECT_CLASS(webkitAccessibleParentClass)->ref_relation_set(object);
    AccessibilityObject* coreObject = core(object);

    setAtkRelationSetFromCoreObject(coreObject, relationSet);

    return relationSet;
}

static void webkitAccessibleInit(AtkObject* object, gpointer data)
{
    if (ATK_OBJECT_CLASS(webkitAccessibleParentClass)->initialize)
        ATK_OBJECT_CLASS(webkitAccessibleParentClass)->initialize(object, data);

    WebKitAccessible* accessible = WEBKIT_ACCESSIBLE(object);
    accessible->m_object = reinterpret_cast<AccessibilityObject*>(data);
    accessible->priv = WEBKIT_ACCESSIBLE_GET_PRIVATE(accessible);
}

static const gchar* webkitAccessibleGetObjectLocale(AtkObject* object)
{
    if (ATK_IS_DOCUMENT(object)) {
        AccessibilityObject* coreObject = core(object);
        if (!coreObject)
            return 0;

        // TODO: Should we fall back on lang xml:lang when the following comes up empty?
        String language = coreObject->language();
        if (!language.isEmpty())
            return cacheAndReturnAtkProperty(object, AtkCachedDocumentLocale, language);

    } else if (ATK_IS_TEXT(object)) {
        const gchar* locale = 0;

        AtkAttributeSet* textAttributes = atk_text_get_default_attributes(ATK_TEXT(object));
        for (AtkAttributeSet* attributes = textAttributes; attributes; attributes = attributes->next) {
            AtkAttribute* atkAttribute = static_cast<AtkAttribute*>(attributes->data);
            if (!strcmp(atkAttribute->name, atk_text_attribute_get_name(ATK_TEXT_ATTR_LANGUAGE))) {
                locale = cacheAndReturnAtkProperty(object, AtkCachedDocumentLocale, String::fromUTF8(atkAttribute->value));
                break;
            }
        }
        atk_attribute_set_free(textAttributes);

        return locale;
    }

    return 0;
}

static void webkitAccessibleFinalize(GObject* object)
{
    G_OBJECT_CLASS(webkitAccessibleParentClass)->finalize(object);
}

static void webkitAccessibleClassInit(AtkObjectClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);

    webkitAccessibleParentClass = g_type_class_peek_parent(klass);

    gobjectClass->finalize = webkitAccessibleFinalize;

    klass->initialize = webkitAccessibleInit;
    klass->get_name = webkitAccessibleGetName;
    klass->get_description = webkitAccessibleGetDescription;
    klass->get_parent = webkitAccessibleGetParent;
    klass->get_n_children = webkitAccessibleGetNChildren;
    klass->ref_child = webkitAccessibleRefChild;
    klass->get_role = webkitAccessibleGetRole;
    klass->ref_state_set = webkitAccessibleRefStateSet;
    klass->get_index_in_parent = webkitAccessibleGetIndexInParent;
    klass->get_attributes = webkitAccessibleGetAttributes;
    klass->ref_relation_set = webkitAccessibleRefRelationSet;
    klass->get_object_locale = webkitAccessibleGetObjectLocale;

    g_type_class_add_private(klass, sizeof(WebKitAccessiblePrivate));
}

GType
webkitAccessibleGetType(void)
{
    static volatile gsize typeVolatile = 0;

    if (g_once_init_enter(&typeVolatile)) {
        static const GTypeInfo tinfo = {
            sizeof(WebKitAccessibleClass),
            (GBaseInitFunc) 0,
            (GBaseFinalizeFunc) 0,
            (GClassInitFunc) webkitAccessibleClassInit,
            (GClassFinalizeFunc) 0,
            0, /* class data */
            sizeof(WebKitAccessible), /* instance size */
            0, /* nb preallocs */
            (GInstanceInitFunc) 0,
            0 /* value table */
        };

        GType type = g_type_register_static(ATK_TYPE_OBJECT, "WebKitAccessible", &tinfo, GTypeFlags(0));
        g_once_init_leave(&typeVolatile, type);
    }

    return typeVolatile;
}

static const GInterfaceInfo AtkInterfacesInitFunctions[] = {
    {reinterpret_cast<GInterfaceInitFunc>(webkitAccessibleActionInterfaceInit), 0, 0},
    {reinterpret_cast<GInterfaceInitFunc>(webkitAccessibleSelectionInterfaceInit), 0, 0},
    {reinterpret_cast<GInterfaceInitFunc>(webkitAccessibleEditableTextInterfaceInit), 0, 0},
    {reinterpret_cast<GInterfaceInitFunc>(webkitAccessibleTextInterfaceInit), 0, 0},
    {reinterpret_cast<GInterfaceInitFunc>(webkitAccessibleComponentInterfaceInit), 0, 0},
    {reinterpret_cast<GInterfaceInitFunc>(webkitAccessibleImageInterfaceInit), 0, 0},
    {reinterpret_cast<GInterfaceInitFunc>(webkitAccessibleTableInterfaceInit), 0, 0},
    {reinterpret_cast<GInterfaceInitFunc>(webkitAccessibleHypertextInterfaceInit), 0, 0},
    {reinterpret_cast<GInterfaceInitFunc>(webkitAccessibleHyperlinkImplInterfaceInit), 0, 0},
    {reinterpret_cast<GInterfaceInitFunc>(webkitAccessibleDocumentInterfaceInit), 0, 0},
    {reinterpret_cast<GInterfaceInitFunc>(webkitAccessibleValueInterfaceInit), 0, 0}
};

enum WAIType {
    WAI_ACTION,
    WAI_SELECTION,
    WAI_EDITABLE_TEXT,
    WAI_TEXT,
    WAI_COMPONENT,
    WAI_IMAGE,
    WAI_TABLE,
    WAI_HYPERTEXT,
    WAI_HYPERLINK,
    WAI_DOCUMENT,
    WAI_VALUE,
};

static GType GetAtkInterfaceTypeFromWAIType(WAIType type)
{
    switch (type) {
    case WAI_ACTION:
        return ATK_TYPE_ACTION;
    case WAI_SELECTION:
        return ATK_TYPE_SELECTION;
    case WAI_EDITABLE_TEXT:
        return ATK_TYPE_EDITABLE_TEXT;
    case WAI_TEXT:
        return ATK_TYPE_TEXT;
    case WAI_COMPONENT:
        return ATK_TYPE_COMPONENT;
    case WAI_IMAGE:
        return ATK_TYPE_IMAGE;
    case WAI_TABLE:
        return ATK_TYPE_TABLE;
    case WAI_HYPERTEXT:
        return ATK_TYPE_HYPERTEXT;
    case WAI_HYPERLINK:
        return ATK_TYPE_HYPERLINK_IMPL;
    case WAI_DOCUMENT:
        return ATK_TYPE_DOCUMENT;
    case WAI_VALUE:
        return ATK_TYPE_VALUE;
    }

    return G_TYPE_INVALID;
}

static bool roleIsTextType(AccessibilityRole role)
{
    return role == ParagraphRole || role == HeadingRole || role == DivRole || role == CellRole || role == ListItemRole;
}

static guint16 getInterfaceMaskFromObject(AccessibilityObject* coreObject)
{
    guint16 interfaceMask = 0;

    // Component interface is always supported
    interfaceMask |= 1 << WAI_COMPONENT;

    AccessibilityRole role = coreObject->roleValue();

    // Action
    // As the implementation of the AtkAction interface is a very
    // basic one (just relays in executing the default action for each
    // object, and only supports having one action per object), it is
    // better just to implement this interface for every instance of
    // the WebKitAccessible class and let WebCore decide what to do.
    interfaceMask |= 1 << WAI_ACTION;

    // Selection
    if (coreObject->isListBox() || coreObject->isMenuList())
        interfaceMask |= 1 << WAI_SELECTION;

    // Get renderer if available.
    RenderObject* renderer = 0;
    if (coreObject->isAccessibilityRenderObject())
        renderer = coreObject->renderer();

    // Hyperlink (links and embedded objects).
    if (coreObject->isLink() || (renderer && renderer->isReplaced()))
        interfaceMask |= 1 << WAI_HYPERLINK;

    // Text & Editable Text
    if (role == StaticTextRole || coreObject->isMenuListOption())
        interfaceMask |= 1 << WAI_TEXT;
    else {
        if (coreObject->isTextControl()) {
            interfaceMask |= 1 << WAI_TEXT;
            if (!coreObject->isReadOnly())
                interfaceMask |= 1 << WAI_EDITABLE_TEXT;
        } else {
            if (role != TableRole) {
                interfaceMask |= 1 << WAI_HYPERTEXT;
                if ((renderer && renderer->childrenInline()) || roleIsTextType(role))
                    interfaceMask |= 1 << WAI_TEXT;
            }

            // Add the TEXT interface for list items whose
            // first accessible child has a text renderer
            if (role == ListItemRole) {
                AccessibilityObject::AccessibilityChildrenVector children = coreObject->children();
                if (children.size()) {
                    AccessibilityObject* axRenderChild = children.at(0).get();
                    interfaceMask |= getInterfaceMaskFromObject(axRenderChild);
                }
            }
        }
    }

    // Image
    if (coreObject->isImage())
        interfaceMask |= 1 << WAI_IMAGE;

    // Table
    if (role == TableRole)
        interfaceMask |= 1 << WAI_TABLE;

    // Document
    if (role == WebAreaRole)
        interfaceMask |= 1 << WAI_DOCUMENT;

    // Value
    if (role == SliderRole || role == SpinButtonRole || role == ScrollBarRole)
        interfaceMask |= 1 << WAI_VALUE;

    return interfaceMask;
}

static const char* getUniqueAccessibilityTypeName(guint16 interfaceMask)
{
#define WAI_TYPE_NAME_LEN (30) /* Enough for prefix + 5 hex characters (max) */
    static char name[WAI_TYPE_NAME_LEN + 1];

    g_sprintf(name, "WAIType%x", interfaceMask);
    name[WAI_TYPE_NAME_LEN] = '\0';

    return name;
}

static GType getAccessibilityTypeFromObject(AccessibilityObject* coreObject)
{
    static const GTypeInfo typeInfo = {
        sizeof(WebKitAccessibleClass),
        (GBaseInitFunc) 0,
        (GBaseFinalizeFunc) 0,
        (GClassInitFunc) 0,
        (GClassFinalizeFunc) 0,
        0, /* class data */
        sizeof(WebKitAccessible), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) 0,
        0 /* value table */
    };

    guint16 interfaceMask = getInterfaceMaskFromObject(coreObject);
    const char* atkTypeName = getUniqueAccessibilityTypeName(interfaceMask);
    GType type = g_type_from_name(atkTypeName);
    if (type)
        return type;

    type = g_type_register_static(WEBKIT_TYPE_ACCESSIBLE, atkTypeName, &typeInfo, GTypeFlags(0));
    for (guint i = 0; i < G_N_ELEMENTS(AtkInterfacesInitFunctions); i++) {
        if (interfaceMask & (1 << i))
            g_type_add_interface_static(type,
                GetAtkInterfaceTypeFromWAIType(static_cast<WAIType>(i)),
                &AtkInterfacesInitFunctions[i]);
    }

    return type;
}

WebKitAccessible* webkitAccessibleNew(AccessibilityObject* coreObject)
{
    GType type = getAccessibilityTypeFromObject(coreObject);
    AtkObject* object = static_cast<AtkObject*>(g_object_new(type, 0));

    atk_object_initialize(object, coreObject);

    return WEBKIT_ACCESSIBLE(object);
}

AccessibilityObject* webkitAccessibleGetAccessibilityObject(WebKitAccessible* accessible)
{
    return accessible->m_object;
}

void webkitAccessibleDetach(WebKitAccessible* accessible)
{
    ASSERT(accessible->m_object);

    if (core(accessible)->roleValue() == WebAreaRole)
        g_signal_emit_by_name(accessible, "state-change", "defunct", true);

    // We replace the WebCore AccessibilityObject with a fallback object that
    // provides default implementations to avoid repetitive null-checking after
    // detachment.
    accessible->m_object = fallbackObject();
}

AtkObject* webkitAccessibleGetFocusedElement(WebKitAccessible* accessible)
{
    if (!accessible->m_object)
        return 0;

    RefPtr<AccessibilityObject> focusedObj = accessible->m_object->focusedUIElement();
    if (!focusedObj)
        return 0;

    return focusedObj->wrapper();
}

AccessibilityObject* objectFocusedAndCaretOffsetUnignored(AccessibilityObject* referenceObject, int& offset)
{
    // Indication that something bogus has transpired.
    offset = -1;

    Document* document = referenceObject->document();
    if (!document)
        return 0;

    Node* focusedNode = referenceObject->selection().end().containerNode();
    if (!focusedNode)
        return 0;

    RenderObject* focusedRenderer = focusedNode->renderer();
    if (!focusedRenderer)
        return 0;

    AccessibilityObject* focusedObject = document->axObjectCache()->getOrCreate(focusedRenderer);
    if (!focusedObject)
        return 0;

    // Look for the actual (not ignoring accessibility) selected object.
    AccessibilityObject* firstUnignoredParent = focusedObject;
    if (firstUnignoredParent->accessibilityIsIgnored())
        firstUnignoredParent = firstUnignoredParent->parentObjectUnignored();
    if (!firstUnignoredParent)
        return 0;

    // Don't ignore links if the offset is being requested for a link.
    if (!referenceObject->isLink() && firstUnignoredParent->isLink())
        firstUnignoredParent = firstUnignoredParent->parentObjectUnignored();
    if (!firstUnignoredParent)
        return 0;

    // The reference object must either coincide with the focused
    // object being considered, or be a descendant of it.
    if (referenceObject->isDescendantOfObject(firstUnignoredParent))
        referenceObject = firstUnignoredParent;

    Node* startNode = 0;
    if (firstUnignoredParent != referenceObject || firstUnignoredParent->isTextControl()) {
        // We need to use the first child's node of the reference
        // object as the start point to calculate the caret offset
        // because we want it to be relative to the object of
        // reference, not just to the focused object (which could have
        // previous siblings which should be taken into account too).
        AccessibilityObject* axFirstChild = referenceObject->firstChild();
        if (axFirstChild)
            startNode = axFirstChild->node();
    }
    // Getting the Position of a PseudoElement now triggers an assertion.
    // This can occur when clicking on empty space in a render block.
    if (!startNode || startNode->isPseudoElement())
        startNode = firstUnignoredParent->node();

    // Check if the node for the first parent object not ignoring
    // accessibility is null again before using it. This might happen
    // with certain kind of accessibility objects, such as the root
    // one (the scroller containing the webArea object).
    if (!startNode)
        return 0;

    VisiblePosition startPosition = VisiblePosition(positionBeforeNode(startNode), DOWNSTREAM);
    VisiblePosition endPosition = firstUnignoredParent->selection().visibleEnd();

    if (startPosition == endPosition)
        offset = 0;
    else if (!isStartOfLine(endPosition)) {
        RefPtr<Range> range = makeRange(startPosition, endPosition.previous());
        offset = TextIterator::rangeLength(range.get(), true) + 1;
    } else {
        RefPtr<Range> range = makeRange(startPosition, endPosition);
        offset = TextIterator::rangeLength(range.get(), true);
    }

    return firstUnignoredParent;
}

const char* cacheAndReturnAtkProperty(AtkObject* object, AtkCachedProperty property, String value)
{
    WebKitAccessiblePrivate* priv = WEBKIT_ACCESSIBLE(object)->priv;
    CString* propertyPtr = 0;

    switch (property) {
    case AtkCachedAccessibleName:
        propertyPtr = &priv->accessibleName;
        break;

    case AtkCachedAccessibleDescription:
        propertyPtr = &priv->accessibleDescription;
        break;

    case AtkCachedActionName:
        propertyPtr = &priv->actionName;
        break;

    case AtkCachedActionKeyBinding:
        propertyPtr = &priv->actionKeyBinding;
        break;

    case AtkCachedDocumentLocale:
        propertyPtr = &priv->documentLocale;
        break;

    case AtkCachedDocumentType:
        propertyPtr = &priv->documentType;
        break;

    case AtkCachedDocumentEncoding:
        propertyPtr = &priv->documentEncoding;
        break;

    case AtkCachedDocumentURI:
        propertyPtr = &priv->documentURI;
        break;

    case AtkCachedImageDescription:
        propertyPtr = &priv->imageDescription;
        break;

    default:
        ASSERT_NOT_REACHED();
    }

    // Don't invalidate old memory if not stricly needed, since other
    // callers might be still holding on to it.
    if (*propertyPtr != value.utf8())
        *propertyPtr = value.utf8();

    return (*propertyPtr).data();
}

#endif // HAVE(ACCESSIBILITY)
