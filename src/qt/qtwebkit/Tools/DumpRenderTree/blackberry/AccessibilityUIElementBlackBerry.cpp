/*
 * Copyright (C) 2010, 2012 Research In Motion Limited. All rights reserved.
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
#include "AccessibilityUIElement.h"

#include "NotImplemented.h"

AccessibilityUIElement::AccessibilityUIElement(PlatformUIElement element)
    : m_element(element)
{
}

AccessibilityUIElement::AccessibilityUIElement(const AccessibilityUIElement& other)
    : m_element(other.m_element)
{
}

AccessibilityUIElement::~AccessibilityUIElement()
{
}

void AccessibilityUIElement::getLinkedUIElements(Vector<AccessibilityUIElement>&)
{
    notImplemented();
}

void AccessibilityUIElement::getDocumentLinks(Vector<AccessibilityUIElement>&)
{
    notImplemented();
}

void AccessibilityUIElement::getChildren(Vector<AccessibilityUIElement>&)
{
    notImplemented();
}

void AccessibilityUIElement::getChildrenWithRange(Vector<AccessibilityUIElement>&, unsigned, unsigned)
{
    notImplemented();
}

int AccessibilityUIElement::childrenCount()
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::elementAtPoint(int, int)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::getChildAtIndex(unsigned)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::linkedUIElementAtIndex(unsigned)
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::allAttributes()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfLinkedUIElements()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfDocumentLinks()
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::titleUIElement()
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::parentElement()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfChildren()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::parameterizedAttributeNames()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::role()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::subrole()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::roleDescription()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::title()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::description()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::stringValue()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::language()
{
    notImplemented();
    return 0;
}

double AccessibilityUIElement::x()
{
    notImplemented();
    return 0;
}

double AccessibilityUIElement::y()
{
    notImplemented();
    return 0;
}

double AccessibilityUIElement::width()
{
    notImplemented();
    return 0;
}

double AccessibilityUIElement::height()
{
    notImplemented();
    return 0;
}

double AccessibilityUIElement::clickPointX()
{
    notImplemented();
    return 0;
}

double AccessibilityUIElement::clickPointY()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::orientation() const
{
    notImplemented();
    return 0;
}

double AccessibilityUIElement::minValue()
{
    notImplemented();
    return 0;
}

double AccessibilityUIElement::maxValue()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::valueDescription()
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isEnabled()
{
    notImplemented();
    return 0;
}

int AccessibilityUIElement::insertionPointLineNumber()
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isPressActionSupported()
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isIncrementActionSupported()
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isDecrementActionSupported()
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isRequired() const
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isSelected() const
{
    notImplemented();
    return 0;
}

int AccessibilityUIElement::hierarchicalLevel() const
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::ariaIsGrabbed() const
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::ariaDropEffects() const
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isExpanded() const
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfColumnHeaders()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfRowHeaders()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfColumns()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfRows()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfVisibleCells()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfHeader()
{
    notImplemented();
    return 0;
}

int AccessibilityUIElement::indexInTable()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::rowIndexRange()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::columnIndexRange()
{
    notImplemented();
    return 0;
}

int AccessibilityUIElement::lineForIndex(int)
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::boundsForRange(unsigned, unsigned)
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::stringForRange(unsigned, unsigned)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::uiElementForSearchPredicate(JSContextRef, AccessibilityUIElement*, bool, JSValueRef, JSStringRef)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::cellForColumnAndRow(unsigned, unsigned)
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::selectedTextRange()
{
    notImplemented();
    return 0;
}

void AccessibilityUIElement::setSelectedTextRange(unsigned, unsigned)
{
    notImplemented();
}

bool AccessibilityUIElement::isAttributeSettable(JSStringRef)
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isAttributeSupported(JSStringRef)
{
    notImplemented();
    return 0;
}

void AccessibilityUIElement::increment()
{
    notImplemented();
}

void AccessibilityUIElement::decrement()
{
    notImplemented();
}

void AccessibilityUIElement::showMenu()
{
    notImplemented();
}

AccessibilityUIElement AccessibilityUIElement::disclosedRowAtIndex(unsigned)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::ariaOwnsElementAtIndex(unsigned)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::ariaFlowToElementAtIndex(unsigned)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::selectedRowAtIndex(unsigned)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::rowAtIndex(unsigned)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::disclosedByRow()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::accessibilityValue() const
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::documentEncoding()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::documentURI()
{
    notImplemented();
    return 0;
}

unsigned AccessibilityUIElement::indexOfChild(AccessibilityUIElement*)
{
    notImplemented();
    return 0;
}

double AccessibilityUIElement::numberAttributeValue(JSStringRef)
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::boolAttributeValue(JSStringRef)
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::stringAttributeValue(JSStringRef)
{
    notImplemented();
    return 0;
}

double AccessibilityUIElement::intValue() const
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isChecked() const
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::url()
{
    notImplemented();
    return 0;
}


bool AccessibilityUIElement::addNotificationListener(JSObjectRef)
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isSelectable() const
{
    notImplemented();
    return false;
}

bool AccessibilityUIElement::isMultiSelectable() const
{
    notImplemented();
    return false;
}

bool AccessibilityUIElement::isSelectedOptionActive() const
{
    notImplemented();
    return false;
}

bool AccessibilityUIElement::isVisible() const
{
    notImplemented();
    return false;
}

bool AccessibilityUIElement::isOffScreen() const
{
    notImplemented();
    return false;
}

bool AccessibilityUIElement::isCollapsed() const
{
    notImplemented();
    return false;
}

bool AccessibilityUIElement::hasPopup() const
{
    notImplemented();
    return false;
}

void AccessibilityUIElement::scrollToMakeVisible()
{
    notImplemented();
}

void AccessibilityUIElement::scrollToMakeVisibleWithSubFocus(int, int, int, int)
{
    notImplemented();
}

void AccessibilityUIElement::scrollToGlobalPoint(int, int)
{
    notImplemented();
}

void AccessibilityUIElement::takeFocus()
{
    notImplemented();
}

void AccessibilityUIElement::takeSelection()
{
    notImplemented();
}

void AccessibilityUIElement::addSelection()
{
    notImplemented();
}

void AccessibilityUIElement::removeSelection()
{
    notImplemented();
}

int AccessibilityUIElement::columnCount()
{
    notImplemented();
    return 0;
}

void AccessibilityUIElement::removeNotificationListener()
{
    notImplemented();
}

void AccessibilityUIElement::press()
{
    notImplemented();
}

int AccessibilityUIElement::rowCount()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::helpText() const
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributedStringForRange(unsigned, unsigned)
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::attributedStringRangeIsMisspelled(unsigned, unsigned)
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isIgnored() const
{
    notImplemented();
    return false;
}

bool AccessibilityUIElement::isFocused() const
{
    notImplemented();
    return false;
}

bool AccessibilityUIElement::isFocusable() const
{
    notImplemented();
    return false;
}

