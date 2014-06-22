/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AccessibilityUIElement.h"

#if HAVE(ACCESSIBILITY)

#include "InjectedBundle.h"
#include "InjectedBundlePage.h"
#include "NotImplemented.h"
#include <JavaScriptCore/JSStringRef.h>
#include <atk/atk.h>
#include <wtf/Assertions.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <wtf/unicode/CharacterNames.h>

namespace WTR {

static String coreAttributeToAtkAttribute(JSStringRef attribute)
{
    size_t bufferSize = JSStringGetMaximumUTF8CStringSize(attribute);
    GOwnPtr<gchar> buffer(static_cast<gchar*>(g_malloc(bufferSize)));
    JSStringGetUTF8CString(attribute, buffer.get(), bufferSize);

    String attributeString = String::fromUTF8(buffer.get());
    return attributeString == "AXPlaceholderValue" ? "placeholder-text" : String();
}

static String getAttributeSetValueForId(AtkObject* accessible, const char* id)
{
    const char* attributeValue = 0;
    AtkAttributeSet* attributeSet = atk_object_get_attributes(accessible);
    for (AtkAttributeSet* attributes = attributeSet; attributes; attributes = attributes->next) {
        AtkAttribute* atkAttribute = static_cast<AtkAttribute*>(attributes->data);
        if (!strcmp(atkAttribute->name, id)) {
            attributeValue = atkAttribute->value;
            break;
        }
    }

    String atkAttributeValue = String::fromUTF8(attributeValue);
    atk_attribute_set_free(attributeSet);

    return atkAttributeValue;
}

static char* getAtkAttributeSetAsString(AtkObject* accessible)
{
    GString* str = g_string_new(0);

    AtkAttributeSet* attributeSet = atk_object_get_attributes(accessible);
    for (AtkAttributeSet* attributes = attributeSet; attributes; attributes = attributes->next) {
        AtkAttribute* attribute = static_cast<AtkAttribute*>(attributes->data);
        GOwnPtr<gchar> attributeData(g_strconcat(attribute->name, ":", attribute->value, NULL));
        g_string_append(str, attributeData.get());
        if (attributes->next)
            g_string_append(str, ", ");
    }
    atk_attribute_set_free(attributeSet);

    return g_string_free(str, FALSE);
}

static bool checkElementState(PlatformUIElement element, AtkStateType stateType)
{
    if (!ATK_IS_OBJECT(element.get()))
        return false;

    GRefPtr<AtkStateSet> stateSet = adoptGRef(atk_object_ref_state_set(ATK_OBJECT(element.get())));
    return atk_state_set_contains_state(stateSet.get(), stateType);
}

static JSStringRef indexRangeInTable(PlatformUIElement element, bool isRowRange)
{
    GOwnPtr<gchar> rangeString(g_strdup("{0, 0}"));

    if (!element || !ATK_IS_OBJECT(element.get()))
        return JSStringCreateWithUTF8CString(rangeString.get());

    AtkObject* axTable = atk_object_get_parent(ATK_OBJECT(element.get()));
    if (!axTable || !ATK_IS_TABLE(axTable))
        return JSStringCreateWithUTF8CString(rangeString.get());

    // Look for the cell in the table.
    gint indexInParent = atk_object_get_index_in_parent(ATK_OBJECT(element.get()));
    if (indexInParent == -1)
        return JSStringCreateWithUTF8CString(rangeString.get());

    int row = -1;
    int column = -1;
    row = atk_table_get_row_at_index(ATK_TABLE(axTable), indexInParent);
    column = atk_table_get_column_at_index(ATK_TABLE(axTable), indexInParent);

    // Get the actual values, if row and columns are valid values.
    if (row != -1 && column != -1) {
        int base = 0;
        int length = 0;
        if (isRowRange) {
            base = row;
            length = atk_table_get_row_extent_at(ATK_TABLE(axTable), row, column);
        } else {
            base = column;
            length = atk_table_get_column_extent_at(ATK_TABLE(axTable), row, column);
        }
        rangeString.set(g_strdup_printf("{%d, %d}", base, length));
    }

    return JSStringCreateWithUTF8CString(rangeString.get());
}

static void alterCurrentValue(PlatformUIElement element, int factor)
{
    if (!element || !ATK_IS_VALUE(element.get()))
        return;

    GValue currentValue = G_VALUE_INIT;
    atk_value_get_current_value(ATK_VALUE(element.get()), &currentValue);

    GValue increment = G_VALUE_INIT;
    atk_value_get_minimum_increment(ATK_VALUE(element.get()), &increment);

    GValue newValue = G_VALUE_INIT;
    g_value_init(&newValue, G_TYPE_DOUBLE);

    g_value_set_float(&newValue, g_value_get_float(&currentValue) + factor * g_value_get_float(&increment));
    atk_value_set_current_value(ATK_VALUE(element.get()), &newValue);

    g_value_unset(&newValue);
    g_value_unset(&increment);
    g_value_unset(&currentValue);
}

static gchar* replaceCharactersForResults(gchar* str)
{
    WTF::String uString = WTF::String::fromUTF8(str);

    // The object replacement character is passed along to ATs so we need to be
    // able to test for their presence and do so without causing test failures.
    uString.replace(objectReplacementCharacter, "<obj>");

    // The presence of newline characters in accessible text of a single object
    // is appropriate, but it makes test results (especially the accessible tree)
    // harder to read.
    uString.replace("\n", "<\\n>");

    return g_strdup(uString.utf8().data());
}

static const gchar* roleToString(AtkRole role)
{
    switch (role) {
    case ATK_ROLE_ALERT:
        return "AXRole: AXAlert";
    case ATK_ROLE_CANVAS:
        return "AXRole: AXCanvas";
    case ATK_ROLE_CHECK_BOX:
        return "AXRole: AXCheckBox";
    case ATK_ROLE_COLUMN_HEADER:
        return "AXRole: AXColumnHeader";
    case ATK_ROLE_COMBO_BOX:
        return "AXRole: AXComboBox";
    case ATK_ROLE_DOCUMENT_FRAME:
        return "AXRole: AXWebArea";
    case ATK_ROLE_ENTRY:
        return "AXRole: AXTextField";
    case ATK_ROLE_FOOTER:
        return "AXRole: AXFooter";
    case ATK_ROLE_FORM:
        return "AXRole: AXForm";
    case ATK_ROLE_GROUPING:
        return "AXRole: AXGroup";
    case ATK_ROLE_HEADING:
        return "AXRole: AXHeading";
    case ATK_ROLE_IMAGE:
        return "AXRole: AXImage";
    case ATK_ROLE_IMAGE_MAP:
        return "AXRole: AXImageMap";
    case ATK_ROLE_LABEL:
        return "AXRole: AXLabel";
    case ATK_ROLE_LINK:
        return "AXRole: AXLink";
    case ATK_ROLE_LIST:
        return "AXRole: AXList";
    case ATK_ROLE_LIST_BOX:
        return "AXRole: AXListBox";
    case ATK_ROLE_LIST_ITEM:
        return "AXRole: AXListItem";
    case ATK_ROLE_MENU:
        return "AXRole: AXMenu";
    case ATK_ROLE_MENU_BAR:
        return "AXRole: AXMenuBar";
    case ATK_ROLE_MENU_ITEM:
        return "AXRole: AXMenuItem";
    case ATK_ROLE_PAGE_TAB:
        return "AXRole: AXTab";
    case ATK_ROLE_PAGE_TAB_LIST:
        return "AXRole: AXTabGroup";
    case ATK_ROLE_PANEL:
        return "AXRole: AXGroup";
    case ATK_ROLE_PARAGRAPH:
        return "AXRole: AXParagraph";
    case ATK_ROLE_PASSWORD_TEXT:
        return "AXRole: AXPasswordField";
    case ATK_ROLE_PUSH_BUTTON:
        return "AXRole: AXButton";
    case ATK_ROLE_RADIO_BUTTON:
        return "AXRole: AXRadioButton";
    case ATK_ROLE_ROW_HEADER:
        return "AXRole: AXRowHeader";
    case ATK_ROLE_RULER:
        return "AXRole: AXRuler";
    case ATK_ROLE_SCROLL_BAR:
        return "AXRole: AXScrollBar";
    case ATK_ROLE_SCROLL_PANE:
        return "AXRole: AXScrollArea";
    case ATK_ROLE_SECTION:
        return "AXRole: AXDiv";
    case ATK_ROLE_SEPARATOR:
        return "AXRole: AXHorizontalRule";
    case ATK_ROLE_SLIDER:
        return "AXRole: AXSlider";
    case ATK_ROLE_SPIN_BUTTON:
        return "AXRole: AXSpinButton";
    case ATK_ROLE_TABLE:
        return "AXRole: AXTable";
    case ATK_ROLE_TABLE_CELL:
        return "AXRole: AXCell";
    case ATK_ROLE_TABLE_COLUMN_HEADER:
        return "AXRole: AXColumnHeader";
    case ATK_ROLE_TABLE_ROW:
        return "AXRole: AXRow";
    case ATK_ROLE_TABLE_ROW_HEADER:
        return "AXRole: AXRowHeader";
    case ATK_ROLE_TOGGLE_BUTTON:
        return "AXRole: AXToggleButton";
    case ATK_ROLE_TOOL_BAR:
        return "AXRole: AXToolbar";
    case ATK_ROLE_TOOL_TIP:
        return "AXRole: AXUserInterfaceTooltip";
    case ATK_ROLE_TREE:
        return "AXRole: AXTree";
    case ATK_ROLE_TREE_TABLE:
        return "AXRole: AXTreeGrid";
    case ATK_ROLE_TREE_ITEM:
        return "AXRole: AXTreeItem";
    case ATK_ROLE_WINDOW:
        return "AXRole: AXWindow";
    case ATK_ROLE_UNKNOWN:
        return "AXRole: AXUnknown";
    default:
        // We want to distinguish ATK_ROLE_UNKNOWN from a known AtkRole which
        // our DRT isn't properly handling.
        return "AXRole: FIXME not identified";
    }
}

AccessibilityUIElement::AccessibilityUIElement(PlatformUIElement element)
    : m_element(element)
{
}

AccessibilityUIElement::AccessibilityUIElement(const AccessibilityUIElement& other)
    : JSWrappable()
    , m_element(other.m_element)
{
}

AccessibilityUIElement::~AccessibilityUIElement()
{
}

bool AccessibilityUIElement::isEqual(AccessibilityUIElement* otherElement)
{
    return m_element == otherElement->platformUIElement();
}

void AccessibilityUIElement::getChildren(Vector<RefPtr<AccessibilityUIElement> >& children)
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return;

    int count = childrenCount();
    for (int i = 0; i < count; i++) {
        GRefPtr<AtkObject> child = adoptGRef(atk_object_ref_accessible_child(ATK_OBJECT(m_element.get()), i));
        children.append(AccessibilityUIElement::create(child.get()));
    }
}

void AccessibilityUIElement::getChildrenWithRange(Vector<RefPtr<AccessibilityUIElement> >& children, unsigned location, unsigned length)
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return;
    unsigned end = location + length;
    for (unsigned i = location; i < end; i++) {
        GRefPtr<AtkObject> child = adoptGRef(atk_object_ref_accessible_child(ATK_OBJECT(m_element.get()), i));
        children.append(AccessibilityUIElement::create(child.get()));
    }
}

int AccessibilityUIElement::childrenCount()
{
    if (!m_element)
        return 0;

    return atk_object_get_n_accessible_children(ATK_OBJECT(m_element.get()));
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::elementAtPoint(int x, int y)
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return 0;

    GRefPtr<AtkObject> objectAtPoint = adoptGRef(atk_component_ref_accessible_at_point(ATK_COMPONENT(m_element.get()), x, y, ATK_XY_WINDOW));
    return objectAtPoint ? AccessibilityUIElement::create(objectAtPoint.get()) : 0;
}

unsigned AccessibilityUIElement::indexOfChild(AccessibilityUIElement* element)
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return 0;

    Vector<RefPtr<AccessibilityUIElement> > children;
    getChildren(children);

    unsigned count = children.size();
    for (unsigned i = 0; i < count; i++)
        if (children[i]->isEqual(element))
            return i;

    return 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::childAtIndex(unsigned index)
{
    Vector<RefPtr<AccessibilityUIElement> > children;
    getChildrenWithRange(children, index, 1);

    if (children.size() == 1)
        return children[0];

    return 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::linkedUIElementAtIndex(unsigned index)
{
    // FIXME: implement
    return 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::ariaOwnsElementAtIndex(unsigned index)
{
    // FIXME: implement
    return 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::ariaFlowToElementAtIndex(unsigned index)
{
    // FIXME: implement
    return 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::disclosedRowAtIndex(unsigned index)
{
    // FIXME: implement
    return 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::rowAtIndex(unsigned index)
{
    // FIXME: implement
    return 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::selectedChildAtIndex(unsigned index) const
{
    // FIXME: implement
    return 0;
}

unsigned AccessibilityUIElement::selectedChildrenCount() const
{
    // FIXME: implement
    return 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::selectedRowAtIndex(unsigned index)
{
    // FIXME: implement
    return 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::titleUIElement()
{
    if (!m_element)
        return 0;

    AtkRelationSet* set = atk_object_ref_relation_set(ATK_OBJECT(m_element.get()));
    if (!set)
        return 0;

    AtkObject* target = 0;
    int count = atk_relation_set_get_n_relations(set);
    for (int i = 0; i < count; i++) {
        AtkRelation* relation = atk_relation_set_get_relation(set, i);
        if (atk_relation_get_relation_type(relation) == ATK_RELATION_LABELLED_BY) {
            GPtrArray* targetList = atk_relation_get_target(relation);
            if (targetList->len)
                target = static_cast<AtkObject*>(g_ptr_array_index(targetList, 0));
        }
    }

    g_object_unref(set);
    return target ? AccessibilityUIElement::create(target) : 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::parentElement()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return 0;

    AtkObject* parent = atk_object_get_parent(ATK_OBJECT(m_element.get()));
    return parent ? AccessibilityUIElement::create(parent) : 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::disclosedByRow()
{
    // FIXME: implement
    return 0;
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfLinkedUIElements()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfDocumentLinks()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfChildren()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::allAttributes()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return JSStringCreateWithCharacters(0, 0);

    GOwnPtr<char> attributeData(getAtkAttributeSetAsString(ATK_OBJECT(m_element.get())));
    return JSStringCreateWithUTF8CString(attributeData.get());
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::stringAttributeValue(JSStringRef attribute)
{
    if (!m_element)
        return JSStringCreateWithCharacters(0, 0);

    String atkAttributeName = coreAttributeToAtkAttribute(attribute);
    if (atkAttributeName.isNull())
        return JSStringCreateWithCharacters(0, 0);

    String attributeValue = getAttributeSetValueForId(ATK_OBJECT(m_element.get()), atkAttributeName.utf8().data());
    return JSStringCreateWithUTF8CString(attributeValue.utf8().data());
}

double AccessibilityUIElement::numberAttributeValue(JSStringRef attribute)
{
    // FIXME: implement
    return 0.0f;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::uiElementAttributeValue(JSStringRef attribute) const
{
    // FIXME: implement
    return 0;
}

bool AccessibilityUIElement::boolAttributeValue(JSStringRef attribute)
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isAttributeSettable(JSStringRef attribute)
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isAttributeSupported(JSStringRef attribute)
{
    // FIXME: implement
    return false;
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::parameterizedAttributeNames()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::role()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return JSStringCreateWithCharacters(0, 0);

    AtkRole role = atk_object_get_role(ATK_OBJECT(m_element.get()));
    if (!role)
        return JSStringCreateWithCharacters(0, 0);

    GOwnPtr<gchar> axRole(g_strdup(roleToString(role)));
    return JSStringCreateWithUTF8CString(axRole.get());
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::subrole()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::roleDescription()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::title()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return JSStringCreateWithCharacters(0, 0);

    const gchar* name = atk_object_get_name(ATK_OBJECT(m_element.get()));
    GOwnPtr<gchar> axTitle(g_strdup_printf("AXTitle: %s", name ? name : ""));

    return JSStringCreateWithUTF8CString(axTitle.get());
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::description()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return JSStringCreateWithCharacters(0, 0);

    const gchar* description = atk_object_get_description(ATK_OBJECT(m_element.get()));
    if (!description)
        return JSStringCreateWithCharacters(0, 0);

    GOwnPtr<gchar> axDesc(g_strdup_printf("AXDescription: %s", description));

    return JSStringCreateWithUTF8CString(axDesc.get());
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::orientation() const
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return JSStringCreateWithCharacters(0, 0);

    const gchar* axOrientation = 0;
    if (checkElementState(m_element.get(), ATK_STATE_HORIZONTAL))
        axOrientation = "AXOrientation: AXHorizontalOrientation";
    else if (checkElementState(m_element.get(), ATK_STATE_VERTICAL))
        axOrientation = "AXOrientation: AXVerticalOrientation";

    if (!axOrientation)
        return JSStringCreateWithCharacters(0, 0);

    return JSStringCreateWithUTF8CString(axOrientation);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::stringValue()
{
    if (!m_element || !ATK_IS_TEXT(m_element.get()))
        return JSStringCreateWithCharacters(0, 0);

    GOwnPtr<gchar> text(atk_text_get_text(ATK_TEXT(m_element.get()), 0, -1));
    GOwnPtr<gchar> textWithReplacedCharacters(replaceCharactersForResults(text.get()));
    GOwnPtr<gchar> axValue(g_strdup_printf("AXValue: %s", textWithReplacedCharacters.get()));

    return JSStringCreateWithUTF8CString(axValue.get());
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::language()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return JSStringCreateWithCharacters(0, 0);

    const gchar* locale = atk_object_get_object_locale(ATK_OBJECT(m_element.get()));
    if (!locale)
        return JSStringCreateWithCharacters(0, 0);

    GOwnPtr<char> axValue(g_strdup_printf("AXLanguage: %s", locale));
    return JSStringCreateWithUTF8CString(axValue.get());
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::helpText() const
{
    // FIXME: implement
    // We need a way to call WebCore::AccessibilityObject::helpText()
    // from here, probably a new helper class in WebProcess/WebCoreSupport.
    return JSStringCreateWithCharacters(0, 0);
}

double AccessibilityUIElement::x()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return 0.0f;

    int x, y;
    atk_component_get_position(ATK_COMPONENT(m_element.get()), &x, &y, ATK_XY_SCREEN);
    return x;
}

double AccessibilityUIElement::y()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return 0.0f;

    int x, y;
    atk_component_get_position(ATK_COMPONENT(m_element.get()), &x, &y, ATK_XY_SCREEN);
    return y;
}

double AccessibilityUIElement::width()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return 0.0f;

    int width, height;
    atk_component_get_size(ATK_COMPONENT(m_element.get()), &width, &height);
    return width;
}

double AccessibilityUIElement::height()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return 0.0f;

    int width, height;
    atk_component_get_size(ATK_COMPONENT(m_element.get()), &width, &height);
    return height;
}

double AccessibilityUIElement::clickPointX()
{
    // FIXME: implement
    return 0.0f;
}

double AccessibilityUIElement::clickPointY()
{
    // FIXME: implement
    return 0.0f;
}

double AccessibilityUIElement::intValue() const
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return 0.0f;

    GValue value = G_VALUE_INIT;
    atk_value_get_current_value(ATK_VALUE(m_element.get()), &value);
    if (!G_VALUE_HOLDS_FLOAT(&value))
        return 0.0f;

    return g_value_get_float(&value);
}

double AccessibilityUIElement::minValue()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return 0.0f;

    GValue value = G_VALUE_INIT;
    atk_value_get_minimum_value(ATK_VALUE(m_element.get()), &value);
    if (!G_VALUE_HOLDS_FLOAT(&value))
        return 0.0f;

    return g_value_get_float(&value);
}

double AccessibilityUIElement::maxValue()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return 0.0f;

    GValue value = G_VALUE_INIT;
    atk_value_get_maximum_value(ATK_VALUE(m_element.get()), &value);
    if (!G_VALUE_HOLDS_FLOAT(&value))
        return 0.0f;

    return g_value_get_float(&value);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::valueDescription()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

int AccessibilityUIElement::insertionPointLineNumber()
{
    // FIXME: implement
    return -1;
}

bool AccessibilityUIElement::isPressActionSupported()
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isIncrementActionSupported()
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isDecrementActionSupported()
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isEnabled()
{
    return checkElementState(m_element.get(), ATK_STATE_ENABLED);
}

bool AccessibilityUIElement::isRequired() const
{
    return checkElementState(m_element.get(), ATK_STATE_REQUIRED);
}

bool AccessibilityUIElement::isFocused() const
{
    return checkElementState(m_element.get(), ATK_STATE_FOCUSED);
}

bool AccessibilityUIElement::isSelected() const
{
    return checkElementState(m_element.get(), ATK_STATE_SELECTED);
}

bool AccessibilityUIElement::isExpanded() const
{
    return checkElementState(m_element.get(), ATK_STATE_EXPANDED);
}

bool AccessibilityUIElement::isChecked() const
{
    return checkElementState(m_element.get(), ATK_STATE_CHECKED);
}

int AccessibilityUIElement::hierarchicalLevel() const
{
    // FIXME: implement
    return 0;
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::speak()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

bool AccessibilityUIElement::ariaIsGrabbed() const
{
    // FIXME: implement
    return false;
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::ariaDropEffects() const
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

// parameterized attributes
int AccessibilityUIElement::lineForIndex(int index)
{
    // FIXME: implement
    return 0;
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::rangeForLine(int line)
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::rangeForPosition(int x, int y)
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::boundsForRange(unsigned location, unsigned length)
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::stringForRange(unsigned location, unsigned length)
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::attributedStringForRange(unsigned location, unsigned length)
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

bool AccessibilityUIElement::attributedStringRangeIsMisspelled(unsigned location, unsigned length)
{
    // FIXME: implement
    return false;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::uiElementForSearchPredicate(JSContextRef context, AccessibilityUIElement* startElement, bool isDirectionNext, JSValueRef searchKey, JSStringRef searchText, bool visibleOnly)
{
    // FIXME: implement
    return 0;
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfColumnHeaders()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfRowHeaders()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfColumns()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfRows()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfVisibleCells()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfHeader()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

int AccessibilityUIElement::rowCount()
{
    if (!m_element || !ATK_IS_TABLE(m_element.get()))
        return 0;

    return atk_table_get_n_rows(ATK_TABLE(m_element.get()));
}

int AccessibilityUIElement::columnCount()
{
    if (!m_element || !ATK_IS_TABLE(m_element.get()))
        return 0;

    return atk_table_get_n_columns(ATK_TABLE(m_element.get()));
}

int AccessibilityUIElement::indexInTable()
{
    // FIXME: implement
    return -1;
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::rowIndexRange()
{
    // Range in table for rows.
    return indexRangeInTable(m_element.get(), true);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::columnIndexRange()
{
    // Range in table for columns.
    return indexRangeInTable(m_element.get(), false);
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::cellForColumnAndRow(unsigned col, unsigned row)
{
    if (!m_element || !ATK_IS_TABLE(m_element.get()))
        return 0;

    // Adopt the AtkObject representing the cell because
    // at_table_ref_at() transfers full ownership.
    GRefPtr<AtkObject> foundCell = adoptGRef(atk_table_ref_at(ATK_TABLE(m_element.get()), row, col));
    return foundCell ? AccessibilityUIElement::create(foundCell.get()) : 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::horizontalScrollbar() const
{
    // FIXME: implement
    return 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::verticalScrollbar() const
{
    // FIXME: implement
    return 0;
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::selectedTextRange()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

void AccessibilityUIElement::setSelectedTextRange(unsigned location, unsigned length)
{
    // FIXME: implement
}

void AccessibilityUIElement::increment()
{
    alterCurrentValue(m_element.get(), 1);
}

void AccessibilityUIElement::decrement()
{
    alterCurrentValue(m_element.get(), -1);
}

void AccessibilityUIElement::showMenu()
{
    // FIXME: implement
}

void AccessibilityUIElement::press()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return;

    if (!ATK_IS_ACTION(m_element.get()))
        return;

    // Only one action per object is supported so far.
    atk_action_do_action(ATK_ACTION(m_element.get()), 0);
}

void AccessibilityUIElement::setSelectedChild(AccessibilityUIElement* element) const
{
    // FIXME: implement
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::accessibilityValue() const
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::documentEncoding()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return JSStringCreateWithCharacters(0, 0);

    AtkRole role = atk_object_get_role(ATK_OBJECT(m_element.get()));
    if (role != ATK_ROLE_DOCUMENT_FRAME)
        return JSStringCreateWithCharacters(0, 0);

    return JSStringCreateWithUTF8CString(atk_document_get_attribute_value(ATK_DOCUMENT(m_element.get()), "Encoding"));
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::documentURI()
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return JSStringCreateWithCharacters(0, 0);

    AtkRole role = atk_object_get_role(ATK_OBJECT(m_element.get()));
    if (role != ATK_ROLE_DOCUMENT_FRAME)
        return JSStringCreateWithCharacters(0, 0);

    return JSStringCreateWithUTF8CString(atk_document_get_attribute_value(ATK_DOCUMENT(m_element.get()), "URI"));
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::url()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

bool AccessibilityUIElement::addNotificationListener(JSValueRef functionCallback)
{
    // FIXME: implement
    return true;
}

bool AccessibilityUIElement::removeNotificationListener()
{
    // FIXME: implement
    return true;
}

bool AccessibilityUIElement::isFocusable() const
{
    return checkElementState(m_element.get(), ATK_STATE_FOCUSABLE);
}

bool AccessibilityUIElement::isSelectable() const
{
    return checkElementState(m_element.get(), ATK_STATE_SELECTABLE);
}

bool AccessibilityUIElement::isMultiSelectable() const
{
    return checkElementState(m_element.get(), ATK_STATE_MULTISELECTABLE);
}

bool AccessibilityUIElement::isVisible() const
{
    return checkElementState(m_element.get(), ATK_STATE_VISIBLE);
}

bool AccessibilityUIElement::isOffScreen() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isCollapsed() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isIgnored() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::hasPopup() const
{
    if (!m_element || !ATK_IS_OBJECT(m_element.get()))
        return false;

    return equalIgnoringCase(getAttributeSetValueForId(ATK_OBJECT(m_element.get()), "aria-haspopup"), "true");
}

void AccessibilityUIElement::takeFocus()
{
    // FIXME: implement
}

void AccessibilityUIElement::takeSelection()
{
    // FIXME: implement
}

void AccessibilityUIElement::addSelection()
{
    // FIXME: implement
}

void AccessibilityUIElement::removeSelection()
{
    // FIXME: implement
}

// Text markers
PassRefPtr<AccessibilityTextMarkerRange> AccessibilityUIElement::textMarkerRangeForElement(AccessibilityUIElement* element)
{
    // FIXME: implement
    return 0;
}

int AccessibilityUIElement::textMarkerRangeLength(AccessibilityTextMarkerRange* range)
{
    // FIXME: implement
    return 0;
}

PassRefPtr<AccessibilityTextMarker> AccessibilityUIElement::previousTextMarker(AccessibilityTextMarker* textMarker)
{
    // FIXME: implement
    return 0;
}

PassRefPtr<AccessibilityTextMarker> AccessibilityUIElement::nextTextMarker(AccessibilityTextMarker* textMarker)
{
    // FIXME: implement
    return 0;
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::stringForTextMarkerRange(AccessibilityTextMarkerRange* markerRange)
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

PassRefPtr<AccessibilityTextMarkerRange> AccessibilityUIElement::textMarkerRangeForMarkers(AccessibilityTextMarker* startMarker, AccessibilityTextMarker* endMarker)
{
    // FIXME: implement
    return 0;
}

PassRefPtr<AccessibilityTextMarker> AccessibilityUIElement::startTextMarkerForTextMarkerRange(AccessibilityTextMarkerRange* range)
{
    // FIXME: implement
    return 0;
}

PassRefPtr<AccessibilityTextMarker> AccessibilityUIElement::endTextMarkerForTextMarkerRange(AccessibilityTextMarkerRange* range)
{
    // FIXME: implement
    return 0;
}

PassRefPtr<AccessibilityTextMarker> AccessibilityUIElement::textMarkerForPoint(int x, int y)
{
    // FIXME: implement
    return 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::accessibilityElementForTextMarker(AccessibilityTextMarker* marker)
{
    // FIXME: implement
    return 0;
}

bool AccessibilityUIElement::attributedStringForTextMarkerRangeContainsAttribute(JSStringRef attribute, AccessibilityTextMarkerRange* range)
{
    // FIXME: implement
    return false;
}

int AccessibilityUIElement::indexForTextMarker(AccessibilityTextMarker* marker)
{
    // FIXME: implement
    return -1;
}

bool AccessibilityUIElement::isTextMarkerValid(AccessibilityTextMarker* textMarker)
{
    // FIXME: implement
    return false;
}

PassRefPtr<AccessibilityTextMarker> AccessibilityUIElement::textMarkerForIndex(int textIndex)
{
    // FIXME: implement
    return 0;
}

void AccessibilityUIElement::scrollToMakeVisible()
{
    // FIXME: implement
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::supportedActions() const
{
    // FIXME: implement
    return 0;
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::pathDescription() const
{
    notImplemented();
    return 0;
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::mathPostscriptsDescription() const
{
    notImplemented();
    return 0;
}

JSRetainPtr<JSStringRef> AccessibilityUIElement::mathPrescriptsDescription() const
{
    notImplemented();
    return 0;
}

} // namespace WTR

#endif
