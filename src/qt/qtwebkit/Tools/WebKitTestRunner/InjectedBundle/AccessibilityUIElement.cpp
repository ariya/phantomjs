/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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

#include "JSAccessibilityUIElement.h"
#include <JavaScriptCore/JSRetainPtr.h>

namespace WTR {

PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::create(PlatformUIElement uiElement)
{
    return adoptRef(new AccessibilityUIElement(uiElement));
}
    
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::create(const AccessibilityUIElement& uiElement)
{
    return adoptRef(new AccessibilityUIElement(uiElement));
}

JSClassRef AccessibilityUIElement::wrapperClass()
{
    return JSAccessibilityUIElement::accessibilityUIElementClass();
}
    
// Implementation

bool AccessibilityUIElement::isValid() const
{
    return m_element;            
}

// Unsupported methods on various platforms. As they're implemented on other platforms this list should be modified.
#if !PLATFORM(MAC) && !PLATFORM(GTK) && !PLATFORM(EFL)
AccessibilityUIElement::AccessibilityUIElement(PlatformUIElement) { }
AccessibilityUIElement::AccessibilityUIElement(const AccessibilityUIElement&) { }
AccessibilityUIElement::~AccessibilityUIElement() { }
bool AccessibilityUIElement::isEqual(AccessibilityUIElement*) { return false; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::elementAtPoint(int, int) { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::childAtIndex(unsigned) { return 0; }
unsigned AccessibilityUIElement::indexOfChild(AccessibilityUIElement*) { return 0; }
int AccessibilityUIElement::childrenCount() { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::titleUIElement() { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::parentElement() { return 0; }
void AccessibilityUIElement::takeFocus() { }
void AccessibilityUIElement::takeSelection() { }
void AccessibilityUIElement::addSelection() { }
void AccessibilityUIElement::removeSelection() { }
JSRetainPtr<JSStringRef> AccessibilityUIElement::allAttributes() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfLinkedUIElements() { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::linkedUIElementAtIndex(unsigned) { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfDocumentLinks() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfChildren() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::parameterizedAttributeNames() { return 0; }
void AccessibilityUIElement::increment() { }
void AccessibilityUIElement::decrement() { }
void AccessibilityUIElement::showMenu() { }
void AccessibilityUIElement::press() { }
JSRetainPtr<JSStringRef> AccessibilityUIElement::stringAttributeValue(JSStringRef) { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::uiElementAttributeValue(JSStringRef) const { return 0; }
double AccessibilityUIElement::numberAttributeValue(JSStringRef) { return 0; }
bool AccessibilityUIElement::boolAttributeValue(JSStringRef) { return false; }
bool AccessibilityUIElement::isAttributeSupported(JSStringRef) { return false; }
bool AccessibilityUIElement::isAttributeSettable(JSStringRef) { return false; }
bool AccessibilityUIElement::isPressActionSupported() { return false; }
bool AccessibilityUIElement::isIncrementActionSupported() { return false; }
bool AccessibilityUIElement::isDecrementActionSupported() { return false; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::role() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::subrole() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::roleDescription() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::title() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::description() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::language() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::stringValue() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::accessibilityValue() const { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::helpText() const { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::orientation() const { return 0; }
double AccessibilityUIElement::x() { return 0; }
double AccessibilityUIElement::y() { return 0; }
double AccessibilityUIElement::width() { return 0; }
double AccessibilityUIElement::height() { return 0; }
double AccessibilityUIElement::intValue() const { return 0; }
double AccessibilityUIElement::minValue() { return 0; }
double AccessibilityUIElement::maxValue() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::valueDescription() { return 0; }
int AccessibilityUIElement::insertionPointLineNumber() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::selectedTextRange() { return 0; }
bool AccessibilityUIElement::isEnabled() { return false; }
bool AccessibilityUIElement::isRequired() const { return false; }
bool AccessibilityUIElement::isFocused() const { return false; }
bool AccessibilityUIElement::isFocusable() const { return false; }
bool AccessibilityUIElement::isSelected() const { return false; }
bool AccessibilityUIElement::isSelectable() const { return false; }
bool AccessibilityUIElement::isMultiSelectable() const { return false; }
void AccessibilityUIElement::setSelectedChild(AccessibilityUIElement*) const { }
unsigned AccessibilityUIElement::selectedChildrenCount() const { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::selectedChildAtIndex(unsigned) const { return 0; }
bool AccessibilityUIElement::isExpanded() const { return false; }
bool AccessibilityUIElement::isChecked() const { return false; }
bool AccessibilityUIElement::isVisible() const { return false; }
bool AccessibilityUIElement::isOffScreen() const { return false; }
bool AccessibilityUIElement::isCollapsed() const { return false; }
bool AccessibilityUIElement::isIgnored() const { return false; }
bool AccessibilityUIElement::hasPopup() const { return false; }
int AccessibilityUIElement::hierarchicalLevel() const { return 0; }
double AccessibilityUIElement::clickPointX() { return 0; }
double AccessibilityUIElement::clickPointY() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::documentEncoding() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::documentURI() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::url() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::speak() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfColumnHeaders() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfRowHeaders() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfColumns() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfRows() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfVisibleCells() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::attributesOfHeader() { return 0; }
int AccessibilityUIElement::indexInTable() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::rowIndexRange() { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::columnIndexRange() { return 0; }
int AccessibilityUIElement::rowCount() { return 0; }
int AccessibilityUIElement::columnCount() { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::selectedRowAtIndex(unsigned) { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::disclosedByRow() { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::disclosedRowAtIndex(unsigned) { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::rowAtIndex(unsigned) { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::ariaOwnsElementAtIndex(unsigned) { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::ariaFlowToElementAtIndex(unsigned) { return 0; }
bool AccessibilityUIElement::ariaIsGrabbed() const { return false; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::ariaDropEffects() const { return 0; }
int AccessibilityUIElement::lineForIndex(int) { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::rangeForLine(int) { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::rangeForPosition(int, int) { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::boundsForRange(unsigned, unsigned) { return 0; }
void AccessibilityUIElement::setSelectedTextRange(unsigned, unsigned) { }
JSRetainPtr<JSStringRef> AccessibilityUIElement::stringForRange(unsigned, unsigned) { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::attributedStringForRange(unsigned, unsigned) { return 0; }
bool AccessibilityUIElement::attributedStringRangeIsMisspelled(unsigned, unsigned) { return false; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::uiElementForSearchPredicate(JSContextRef, AccessibilityUIElement*, bool, JSValueRef, JSStringRef, bool) { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::cellForColumnAndRow(unsigned, unsigned) { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::horizontalScrollbar() const { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::verticalScrollbar() const { return 0; }
bool AccessibilityUIElement::addNotificationListener(JSValueRef) { return false; }
bool AccessibilityUIElement::removeNotificationListener() { return false; }
PassRefPtr<AccessibilityTextMarkerRange> AccessibilityUIElement::textMarkerRangeForElement(AccessibilityUIElement*) { return 0; }
int AccessibilityUIElement::textMarkerRangeLength(AccessibilityTextMarkerRange*) { return 0; }
PassRefPtr<AccessibilityTextMarkerRange> AccessibilityUIElement::textMarkerRangeForMarkers(AccessibilityTextMarker*, AccessibilityTextMarker*) { return 0; }
PassRefPtr<AccessibilityTextMarker> AccessibilityUIElement::startTextMarkerForTextMarkerRange(AccessibilityTextMarkerRange*) { return 0; }
PassRefPtr<AccessibilityTextMarker> AccessibilityUIElement::endTextMarkerForTextMarkerRange(AccessibilityTextMarkerRange*) { return 0; }
PassRefPtr<AccessibilityUIElement> AccessibilityUIElement::accessibilityElementForTextMarker(AccessibilityTextMarker*) { return 0; }
PassRefPtr<AccessibilityTextMarker> AccessibilityUIElement::textMarkerForPoint(int, int) { return 0; }
PassRefPtr<AccessibilityTextMarker> AccessibilityUIElement::previousTextMarker(AccessibilityTextMarker*) { return 0; }
PassRefPtr<AccessibilityTextMarker> AccessibilityUIElement::nextTextMarker(AccessibilityTextMarker*) { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::stringForTextMarkerRange(AccessibilityTextMarkerRange*) { return 0; }
bool AccessibilityUIElement::attributedStringForTextMarkerRangeContainsAttribute(JSStringRef, AccessibilityTextMarkerRange*) { return false; }
int AccessibilityUIElement::indexForTextMarker(AccessibilityTextMarker*) { return -1; }
bool AccessibilityUIElement::isTextMarkerValid(AccessibilityTextMarker*) { return false; }
PassRefPtr<AccessibilityTextMarker> AccessibilityUIElement::textMarkerForIndex(int) { return 0; }
void AccessibilityUIElement::scrollToMakeVisible() { }
JSRetainPtr<JSStringRef> AccessibilityUIElement::supportedActions() const { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::mathPostscriptsDescription() const { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::mathPrescriptsDescription() const { return 0; }
JSRetainPtr<JSStringRef> AccessibilityUIElement::pathDescription() const { return 0; }

#endif

} // namespace WTR

