/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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

#if HAVE(ACCESSIBILITY)

#include "AccessibilityUIElement.h"

#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <limits.h>

// Static Functions

static inline AccessibilityUIElement* toAXElement(JSObjectRef object)
{
    // FIXME: We should ASSERT that it is the right class here.
    return static_cast<AccessibilityUIElement*>(JSObjectGetPrivate(object));
}

static JSValueRef allAttributesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> attributes(Adopt, toAXElement(thisObject)->allAttributes());
    return JSValueMakeString(context, attributes.get());
}

static JSValueRef attributesOfLinkedUIElementsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> linkedUIDescription(Adopt, toAXElement(thisObject)->attributesOfLinkedUIElements());
    return JSValueMakeString(context, linkedUIDescription.get());
}

static JSValueRef attributesOfDocumentLinksCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> linkedUIDescription(Adopt, toAXElement(thisObject)->attributesOfDocumentLinks());
    return JSValueMakeString(context, linkedUIDescription.get());
}

static JSValueRef attributesOfChildrenCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> childrenDescription(Adopt, toAXElement(thisObject)->attributesOfChildren());
    return JSValueMakeString(context, childrenDescription.get());
}

static JSValueRef parameterizedAttributeNamesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> parameterizedAttributeNames(Adopt, toAXElement(thisObject)->parameterizedAttributeNames());
    return JSValueMakeString(context, parameterizedAttributeNames.get());
}

static JSValueRef attributesOfColumnHeadersCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> attributesOfColumnHeaders(Adopt, toAXElement(thisObject)->attributesOfColumnHeaders());
    return JSValueMakeString(context, attributesOfColumnHeaders.get());
}

static JSValueRef attributesOfRowHeadersCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> attributesOfRowHeaders(Adopt, toAXElement(thisObject)->attributesOfRowHeaders());
    return JSValueMakeString(context, attributesOfRowHeaders.get());
}

static JSValueRef attributesOfColumnsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> attributesOfColumns(Adopt, toAXElement(thisObject)->attributesOfColumns());
    return JSValueMakeString(context, attributesOfColumns.get());
}

static JSValueRef attributesOfRowsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> attributesOfRows(Adopt, toAXElement(thisObject)->attributesOfRows());
    return JSValueMakeString(context, attributesOfRows.get());
}

static JSValueRef attributesOfVisibleCellsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> attributesOfVisibleCells(Adopt, toAXElement(thisObject)->attributesOfVisibleCells());
    return JSValueMakeString(context, attributesOfVisibleCells.get());
}

static JSValueRef attributesOfHeaderCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> attributesOfHeader(Adopt, toAXElement(thisObject)->attributesOfHeader());
    return JSValueMakeString(context, attributesOfHeader.get());
}

static JSValueRef indexInTableCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->indexInTable());
}

static JSValueRef rowIndexRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> rowIndexRange(Adopt, toAXElement(thisObject)->rowIndexRange());
    return JSValueMakeString(context, rowIndexRange.get());
}

static JSValueRef columnIndexRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> columnIndexRange(Adopt, toAXElement(thisObject)->columnIndexRange());
    return JSValueMakeString(context, columnIndexRange.get());
}

static JSValueRef lineForIndexCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int indexNumber = -1;
    if (argumentCount == 1)
        indexNumber = JSValueToNumber(context, arguments[0], exception);
    
    return JSValueMakeNumber(context, toAXElement(thisObject)->lineForIndex(indexNumber));
}

static JSValueRef rangeForLineCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int indexNumber = -1;
    if (argumentCount == 1)
        indexNumber = JSValueToNumber(context, arguments[0], exception);
    
    JSRetainPtr<JSStringRef> rangeLine(Adopt, toAXElement(thisObject)->rangeForLine(indexNumber));
    return JSValueMakeString(context, rangeLine.get());
}

static JSValueRef boundsForRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    unsigned location = UINT_MAX, length = 0;
    if (argumentCount == 2) {
        location = JSValueToNumber(context, arguments[0], exception);
        length = JSValueToNumber(context, arguments[1], exception);
    }

    JSRetainPtr<JSStringRef> boundsDescription(Adopt, toAXElement(thisObject)->boundsForRange(location, length));
    return JSValueMakeString(context, boundsDescription.get());    
}

static JSValueRef rangeForPositionCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int x = 0, y = 0;
    if (argumentCount == 2) {
        x = JSValueToNumber(context, arguments[0], exception);
        y = JSValueToNumber(context, arguments[1], exception);
    }
    
    JSRetainPtr<JSStringRef> rangeDescription(Adopt, toAXElement(thisObject)->rangeForPosition(x, y));
    return JSValueMakeString(context, rangeDescription.get());    
}

static JSValueRef stringForRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    unsigned location = UINT_MAX, length = 0;
    if (argumentCount == 2) {
        location = JSValueToNumber(context, arguments[0], exception);
        length = JSValueToNumber(context, arguments[1], exception);
    }
    
    JSRetainPtr<JSStringRef> stringDescription(Adopt, toAXElement(thisObject)->stringForRange(location, length));
    return JSValueMakeString(context, stringDescription.get());    
}

static JSValueRef attributedStringForRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    unsigned location = UINT_MAX, length = 0;
    if (argumentCount == 2) {
        location = JSValueToNumber(context, arguments[0], exception);
        length = JSValueToNumber(context, arguments[1], exception);
    }
    
    JSRetainPtr<JSStringRef> stringDescription(Adopt, toAXElement(thisObject)->attributedStringForRange(location, length));
    return JSValueMakeString(context, stringDescription.get());    
}

static JSValueRef attributedStringRangeIsMisspelledCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    unsigned location = UINT_MAX, length = 0;
    if (argumentCount == 2) {
        location = JSValueToNumber(context, arguments[0], exception);
        length = JSValueToNumber(context, arguments[1], exception);
    }
    
    return JSValueMakeBoolean(context, toAXElement(thisObject)->attributedStringRangeIsMisspelled(location, length));
}

static JSValueRef uiElementForSearchPredicateCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    AccessibilityUIElement* startElement = 0;
    bool isDirectionNext = true;
    bool visibleOnly = false;
    JSValueRef searchKey = 0;
    JSStringRef searchText = 0;
    if (argumentCount == 5) {
        JSObjectRef startElementObject = JSValueToObject(context, arguments[0], exception);
        if (startElementObject)
            startElement = toAXElement(startElementObject);
        isDirectionNext = JSValueToBoolean(context, arguments[1]);      
        
        searchKey = arguments[2];
        
        if (JSValueIsString(context, arguments[3]))
            searchText = JSValueToStringCopy(context, arguments[3], exception);
        
        visibleOnly = JSValueToBoolean(context, arguments[4]);
    }
    JSObjectRef resultObject = AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->uiElementForSearchPredicate(context, startElement, isDirectionNext, searchKey, searchText, visibleOnly));

    if (searchText)
        JSStringRelease(searchText);
    
    return resultObject;
}

static JSValueRef indexOfChildCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 1)
        return 0;
    
    JSObjectRef otherElement = JSValueToObject(context, arguments[0], exception);
    AccessibilityUIElement* childElement = toAXElement(otherElement);
    return JSValueMakeNumber(context, (double)toAXElement(thisObject)->indexOfChild(childElement));
}

#if PLATFORM(IOS)

static JSValueRef headerElementAtIndexCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 1)
        return 0;
    
    unsigned index = JSValueToNumber(context, arguments[0], exception);
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->headerElementAtIndex(index));
}

static JSValueRef linkedElementCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->linkedElement());
}

static JSValueRef elementsForRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 2)
        return 0;
    
    unsigned location = JSValueToNumber(context, arguments[0], exception);
    unsigned length = JSValueToNumber(context, arguments[1], exception);
    
    Vector<AccessibilityUIElement> elements;
    toAXElement(thisObject)->elementsForRange(location, length, elements);
    
    unsigned elementsSize = elements.size();
    JSValueRef valueElements[elementsSize];
    for (unsigned k = 0; k < elementsSize; ++k)
        valueElements[k] = AccessibilityUIElement::makeJSAccessibilityUIElement(context, elements[k]);
    
    return JSObjectMakeArray(context, elementsSize, valueElements, 0);
}

static JSValueRef increaseTextSelectionCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    toAXElement(thisObject)->increaseTextSelection();
    return JSValueMakeUndefined(context);
}

static JSValueRef decreaseTextSelectionCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    toAXElement(thisObject)->decreaseTextSelection();
    return JSValueMakeUndefined(context);
}

static JSValueRef assistiveTechnologySimulatedFocusCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    toAXElement(thisObject)->assistiveTechnologySimulatedFocus();
    return JSValueMakeUndefined(context);
}

#endif

static JSValueRef childAtIndexCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int indexNumber = -1;
    if (argumentCount == 1)
        indexNumber = JSValueToNumber(context, arguments[0], exception);
    
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->getChildAtIndex(indexNumber));
}

static JSValueRef selectedChildAtIndexCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int indexNumber = -1;
    if (argumentCount == 1)
        indexNumber = JSValueToNumber(context, arguments[0], exception);
    
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->selectedChildAtIndex(indexNumber));
}

static JSValueRef linkedUIElementAtIndexCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int indexNumber = -1;
    if (argumentCount == 1)
        indexNumber = JSValueToNumber(context, arguments[0], exception);
    
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->linkedUIElementAtIndex(indexNumber));
}

static JSValueRef disclosedRowAtIndexCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int indexNumber = 0;
    if (argumentCount == 1)
        indexNumber = JSValueToNumber(context, arguments[0], exception);
    
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->disclosedRowAtIndex(indexNumber));
}

static JSValueRef ariaOwnsElementAtIndexCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int indexNumber = 0;
    if (argumentCount == 1)
        indexNumber = JSValueToNumber(context, arguments[0], exception);
    
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->ariaOwnsElementAtIndex(indexNumber));
}

static JSValueRef ariaFlowToElementAtIndexCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int indexNumber = 0;
    if (argumentCount == 1)
        indexNumber = JSValueToNumber(context, arguments[0], exception);
    
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->ariaFlowToElementAtIndex(indexNumber));
}

static JSValueRef selectedRowAtIndexCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int indexNumber = 0;
    if (argumentCount == 1)
        indexNumber = JSValueToNumber(context, arguments[0], exception);
    
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->selectedRowAtIndex(indexNumber));
}

static JSValueRef rowAtIndexCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int indexNumber = 0;
    if (argumentCount == 1)
        indexNumber = JSValueToNumber(context, arguments[0], exception);
    
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->rowAtIndex(indexNumber));
}

static JSValueRef isEqualCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSObjectRef otherElement = 0;
    if (argumentCount == 1)
        otherElement = JSValueToObject(context, arguments[0], exception);
    else
        return JSValueMakeBoolean(context, false);
    
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isEqual(toAXElement(otherElement)));
}

static JSValueRef setSelectedChildCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSObjectRef element = 0;
    if (argumentCount == 1)
        element = JSValueToObject(context, arguments[0], exception);

    toAXElement(thisObject)->setSelectedChild(toAXElement(element));

    return JSValueMakeUndefined(context);
}

static JSValueRef elementAtPointCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int x = 0;
    int y = 0;
    if (argumentCount == 2) {
        x = JSValueToNumber(context, arguments[0], exception);
        y = JSValueToNumber(context, arguments[1], exception);
    }
    
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->elementAtPoint(x, y));
}

static JSValueRef isAttributeSupportedCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSStringRef attribute = 0;
    if (argumentCount == 1)
        attribute = JSValueToStringCopy(context, arguments[0], exception);    
    JSValueRef result = JSValueMakeBoolean(context, toAXElement(thisObject)->isAttributeSupported(attribute));
    if (attribute)
        JSStringRelease(attribute);
    return result;
}

static JSValueRef isAttributeSettableCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSStringRef attribute = 0;
    if (argumentCount == 1)
        attribute = JSValueToStringCopy(context, arguments[0], exception);    
    JSValueRef result = JSValueMakeBoolean(context, toAXElement(thisObject)->isAttributeSettable(attribute));
    if (attribute)
        JSStringRelease(attribute);
    return result;
}

static JSValueRef isPressActionSupportedCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isPressActionSupported());
}

static JSValueRef isIncrementActionSupportedCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isIncrementActionSupported());
}

static JSValueRef isDecrementActionSupportedCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isDecrementActionSupported());
}

static JSValueRef boolAttributeValueCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSStringRef attribute = 0;
    if (argumentCount == 1)
        attribute = JSValueToStringCopy(context, arguments[0], exception);
    bool val = toAXElement(thisObject)->boolAttributeValue(attribute);
    JSValueRef result = JSValueMakeBoolean(context, val);
    if (attribute)
        JSStringRelease(attribute);
    return result;
}

static JSValueRef stringAttributeValueCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSStringRef attribute = 0;
    if (argumentCount == 1)
        attribute = JSValueToStringCopy(context, arguments[0], exception);
    JSRetainPtr<JSStringRef> stringAttributeValue(Adopt, toAXElement(thisObject)->stringAttributeValue(attribute));
    JSValueRef result = JSValueMakeString(context, stringAttributeValue.get());
    if (attribute)
        JSStringRelease(attribute);
    return result;
}

static JSValueRef uiElementAttributeValueCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> attribute;
    if (argumentCount == 1)
        attribute.adopt(JSValueToStringCopy(context, arguments[0], exception));
    
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->uiElementAttributeValue(attribute.get()));
}

static JSValueRef numberAttributeValueCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSStringRef attribute = 0;
    if (argumentCount == 1)
        attribute = JSValueToStringCopy(context, arguments[0], exception);
    double val = toAXElement(thisObject)->numberAttributeValue(attribute);
    JSValueRef result = JSValueMakeNumber(context, val);
    if (attribute)
        JSStringRelease(attribute);
    return result;
}

static JSValueRef cellForColumnAndRowCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    unsigned column = 0, row = 0;
    if (argumentCount == 2) {
        column = JSValueToNumber(context, arguments[0], exception);
        row = JSValueToNumber(context, arguments[1], exception);
    }
    
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->cellForColumnAndRow(column, row));
}

static JSValueRef titleUIElementCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->titleUIElement());
}

static JSValueRef parentElementCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->parentElement());
}

static JSValueRef disclosedByRowCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->disclosedByRow());
}

static JSValueRef setSelectedTextRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    unsigned location = UINT_MAX, length = 0;
    if (argumentCount == 2) {
        location = JSValueToNumber(context, arguments[0], exception);
        length = JSValueToNumber(context, arguments[1], exception);
    }
    
    toAXElement(thisObject)->setSelectedTextRange(location, length);
    return JSValueMakeUndefined(context);
}

static JSValueRef incrementCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    toAXElement(thisObject)->increment();
    return JSValueMakeUndefined(context);
}

static JSValueRef decrementCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    toAXElement(thisObject)->decrement();
    return JSValueMakeUndefined(context);
}

static JSValueRef showMenuCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    toAXElement(thisObject)->showMenu();
    return JSValueMakeUndefined(context);
}

static JSValueRef pressCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    toAXElement(thisObject)->press();
    return JSValueMakeUndefined(context);
}

static JSValueRef scrollToMakeVisibleCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    toAXElement(thisObject)->scrollToMakeVisible();
    return JSValueMakeUndefined(context);
}

static JSValueRef takeFocusCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    toAXElement(thisObject)->takeFocus();
    return JSValueMakeUndefined(context);
}

static JSValueRef takeSelectionCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    toAXElement(thisObject)->takeSelection();
    return JSValueMakeUndefined(context);
}

static JSValueRef addSelectionCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    toAXElement(thisObject)->addSelection();
    return JSValueMakeUndefined(context);
}

static JSValueRef removeSelectionCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    toAXElement(thisObject)->removeSelection();
    return JSValueMakeUndefined(context);
}

static JSValueRef textMarkerRangeForElementCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    AccessibilityUIElement* uiElement = 0;
    if (argumentCount == 1)
        uiElement = toAXElement(JSValueToObject(context, arguments[0], exception));
    
    return AccessibilityTextMarkerRange::makeJSAccessibilityTextMarkerRange(context, toAXElement(thisObject)->textMarkerRangeForElement(uiElement));
}

static JSValueRef attributedStringForTextMarkerRangeContainsAttributeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    AccessibilityTextMarkerRange* markerRange = 0;
    JSStringRef attribute = 0;
    if (argumentCount == 2) {
        attribute = JSValueToStringCopy(context, arguments[0], exception);
        markerRange = toTextMarkerRange(JSValueToObject(context, arguments[1], exception));
    }
    
    JSValueRef result = JSValueMakeBoolean(context, toAXElement(thisObject)->attributedStringForTextMarkerRangeContainsAttribute(attribute, markerRange));
    if (attribute)
        JSStringRelease(attribute);
    
    return result;    
}

static JSValueRef indexForTextMarkerCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    AccessibilityTextMarker* marker = 0;
    if (argumentCount == 1)
        marker = toTextMarker(JSValueToObject(context, arguments[0], exception));
    
    return JSValueMakeNumber(context, toAXElement(thisObject)->indexForTextMarker(marker));
}

static JSValueRef isTextMarkerValidCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    AccessibilityTextMarker* marker = 0;
    if (argumentCount == 1)
        marker = toTextMarker(JSValueToObject(context, arguments[0], exception));
    
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isTextMarkerValid(marker));
}

static JSValueRef textMarkerForIndexCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int textIndex = 0;
    if (argumentCount == 1)
        textIndex = JSValueToNumber(context, arguments[0], exception);
    
    return AccessibilityTextMarker::makeJSAccessibilityTextMarker(context, toAXElement(thisObject)->textMarkerForIndex(textIndex));
}

static JSValueRef textMarkerRangeLengthCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    AccessibilityTextMarkerRange* range = 0;
    if (argumentCount == 1)
        range = toTextMarkerRange(JSValueToObject(context, arguments[0], exception));
    
    return JSValueMakeNumber(context, (int)toAXElement(thisObject)->textMarkerRangeLength(range));
}

static JSValueRef nextTextMarkerCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    AccessibilityTextMarker* marker = 0;
    if (argumentCount == 1)
        marker = toTextMarker(JSValueToObject(context, arguments[0], exception));
    
    return AccessibilityTextMarker::makeJSAccessibilityTextMarker(context, toAXElement(thisObject)->nextTextMarker(marker));
}

static JSValueRef previousTextMarkerCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    AccessibilityTextMarker* marker = 0;
    if (argumentCount == 1)
        marker = toTextMarker(JSValueToObject(context, arguments[0], exception));
    
    return AccessibilityTextMarker::makeJSAccessibilityTextMarker(context, toAXElement(thisObject)->previousTextMarker(marker));
}

static JSValueRef stringForTextMarkerRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    AccessibilityTextMarkerRange* markerRange = 0;
    if (argumentCount == 1)
        markerRange = toTextMarkerRange(JSValueToObject(context, arguments[0], exception));
    
    JSRetainPtr<JSStringRef> markerRangeString(Adopt, toAXElement(thisObject)->stringForTextMarkerRange(markerRange));
    return JSValueMakeString(context, markerRangeString.get());    
}

static JSValueRef textMarkerForPointCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int x = 0;
    int y = 0;
    if (argumentCount == 2) {
        x = JSValueToNumber(context, arguments[0], exception);
        y = JSValueToNumber(context, arguments[1], exception);
    }
    
    return AccessibilityTextMarker::makeJSAccessibilityTextMarker(context, toAXElement(thisObject)->textMarkerForPoint(x, y));
}

static JSValueRef textMarkerRangeForMarkersCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    AccessibilityTextMarker* startMarker = 0;
    AccessibilityTextMarker* endMarker = 0;
    if (argumentCount == 2) {
        startMarker = toTextMarker(JSValueToObject(context, arguments[0], exception));
        endMarker = toTextMarker(JSValueToObject(context, arguments[1], exception));
    }
    
    return AccessibilityTextMarkerRange::makeJSAccessibilityTextMarkerRange(context, toAXElement(thisObject)->textMarkerRangeForMarkers(startMarker, endMarker));
}

static JSValueRef startTextMarkerForTextMarkerRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    AccessibilityTextMarkerRange* markerRange = 0;
    if (argumentCount == 1)
        markerRange = toTextMarkerRange(JSValueToObject(context, arguments[0], exception));
    
    return AccessibilityTextMarker::makeJSAccessibilityTextMarker(context, toAXElement(thisObject)->startTextMarkerForTextMarkerRange(markerRange));
}

static JSValueRef endTextMarkerForTextMarkerRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    AccessibilityTextMarkerRange* markerRange = 0;
    if (argumentCount == 1)
        markerRange = toTextMarkerRange(JSValueToObject(context, arguments[0], exception));
    
    return AccessibilityTextMarker::makeJSAccessibilityTextMarker(context, toAXElement(thisObject)->endTextMarkerForTextMarkerRange(markerRange));
}

static JSValueRef accessibilityElementForTextMarkerCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    AccessibilityTextMarker* marker = 0;
    if (argumentCount == 1)
        marker = toTextMarker(JSValueToObject(context, arguments[0], exception));
    
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->accessibilityElementForTextMarker(marker));
}

// Static Value Getters

static JSValueRef getARIADropEffectsCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> dropEffects(Adopt, toAXElement(thisObject)->ariaDropEffects());
    return JSValueMakeString(context, dropEffects.get());
}

static JSValueRef getARIAIsGrabbedCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->ariaIsGrabbed());
}

static JSValueRef getIsValidCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    AccessibilityUIElement* uiElement = toAXElement(thisObject);
    if (!uiElement->platformUIElement())
        return JSValueMakeBoolean(context, false);
    
    // There might be other platform logic that one could check here...
    
    return JSValueMakeBoolean(context, true);
}

static JSValueRef getRoleCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> role(Adopt, toAXElement(thisObject)->role());
    return JSValueMakeString(context, role.get());
}

static JSValueRef getSubroleCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> role(Adopt, toAXElement(thisObject)->subrole());
    return JSValueMakeString(context, role.get());
}

static JSValueRef getRoleDescriptionCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> roleDesc(Adopt, toAXElement(thisObject)->roleDescription());
    return JSValueMakeString(context, roleDesc.get());
}

static JSValueRef getTitleCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> title(Adopt, toAXElement(thisObject)->title());
    return JSValueMakeString(context, title.get());
}

static JSValueRef getDescriptionCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> description(Adopt, toAXElement(thisObject)->description());
    return JSValueMakeString(context, description.get());
}

static JSValueRef getStringValueCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> stringValue(Adopt, toAXElement(thisObject)->stringValue());
    return JSValueMakeString(context, stringValue.get());
}

static JSValueRef getLanguageCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> language(Adopt, toAXElement(thisObject)->language());
    return JSValueMakeString(context, language.get());
}

static JSValueRef getHelpTextCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> language(Adopt, toAXElement(thisObject)->helpText());
    return JSValueMakeString(context, language.get());
}

static JSValueRef getOrientationCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> orientation(Adopt, toAXElement(thisObject)->orientation());
    return JSValueMakeString(context, orientation.get());
}

static JSValueRef getChildrenCountCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->childrenCount());
}

static JSValueRef rowCountCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->rowCount());
}

static JSValueRef columnCountCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->columnCount());
}

static JSValueRef getXCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->x());
}

static JSValueRef getYCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->y());
}

static JSValueRef getWidthCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->width());
}

static JSValueRef getHeightCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->height());
}

static JSValueRef getClickPointXCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->clickPointX());
}

static JSValueRef getClickPointYCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->clickPointY());
}

static JSValueRef getIntValueCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->intValue());
}

static JSValueRef getMinValueCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->minValue());
}

static JSValueRef getMaxValueCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->maxValue());
}

static JSValueRef getInsertionPointLineNumberCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->insertionPointLineNumber());
}

static JSValueRef getPathDescriptionCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> pathDescription(Adopt, toAXElement(thisObject)->pathDescription());
    return JSValueMakeString(context, pathDescription.get());
}

static JSValueRef getSelectedTextRangeCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> selectedTextRange(Adopt, toAXElement(thisObject)->selectedTextRange());
    return JSValueMakeString(context, selectedTextRange.get());
}

static JSValueRef getIsEnabledCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isEnabled());
}

static JSValueRef getIsRequiredCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isRequired());
}

static JSValueRef getIsFocusedCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isFocused());
}

static JSValueRef getIsFocusableCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isFocusable());
}

static JSValueRef getIsSelectedCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isSelected());
}

static JSValueRef getIsSelectableCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isSelectable());
}

static JSValueRef getIsMultiSelectableCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isMultiSelectable());
}

static JSValueRef getIsSelectedOptionActiveCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isSelectedOptionActive());
}

static JSValueRef getIsExpandedCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isExpanded());
}

static JSValueRef getIsCheckedCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isChecked());
}

static JSValueRef getIsVisibleCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isVisible());
}

static JSValueRef getIsOffScreenCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isOffScreen());
}

static JSValueRef getIsCollapsedCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isCollapsed());
}

static JSValueRef isIgnoredCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->isIgnored());
}

static JSValueRef speakCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    JSRetainPtr<JSStringRef> speakString(Adopt, toAXElement(thisObject)->speak());
    return JSValueMakeString(context, speakString.get());
}

static JSValueRef selectedChildrenCountCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->selectedChildrenCount());
}

static JSValueRef horizontalScrollbarCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->horizontalScrollbar());
}

static JSValueRef verticalScrollbarCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, toAXElement(thisObject)->verticalScrollbar());
}

static JSValueRef getHasPopupCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->hasPopup());
}

static JSValueRef hierarchicalLevelCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef, JSValueRef*)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->hierarchicalLevel());
}

static JSValueRef getValueDescriptionCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> valueDescription(Adopt, toAXElement(thisObject)->valueDescription());
    return JSValueMakeString(context, valueDescription.get());
}

static JSValueRef getAccessibilityValueCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> accessibilityValue(Adopt, toAXElement(thisObject)->accessibilityValue());
    return JSValueMakeString(context, accessibilityValue.get());
}

static JSValueRef getDocumentEncodingCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> documentEncoding(Adopt, toAXElement(thisObject)->documentEncoding());
    return JSValueMakeString(context, documentEncoding.get());
}

static JSValueRef getDocumentURICallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> documentURI(Adopt, toAXElement(thisObject)->documentURI());
    return JSValueMakeString(context, documentURI.get());
}

static JSValueRef getURLCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> url(Adopt, toAXElement(thisObject)->url());
    return JSValueMakeString(context, url.get());
}

static JSValueRef addNotificationListenerCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 1)
        return JSValueMakeBoolean(context, false);
    
    JSObjectRef callback = JSValueToObject(context, arguments[0], exception);
    bool succeeded = toAXElement(thisObject)->addNotificationListener(callback);
    return JSValueMakeBoolean(context, succeeded);
}

static JSValueRef removeNotificationListenerCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    toAXElement(thisObject)->removeNotificationListener();
    return JSValueMakeUndefined(context);
}

#if PLATFORM(IOS)

static JSValueRef stringForSelectionCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> labelString(Adopt, toAXElement(thisObject)->stringForSelection());
    return JSValueMakeString(context, labelString.get());
}

static JSValueRef getIPhoneLabelCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> labelString(Adopt, toAXElement(thisObject)->iphoneLabel());
    return JSValueMakeString(context, labelString.get());
}

static JSValueRef getIPhoneHintCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> hintString(Adopt, toAXElement(thisObject)->iphoneHint());
    return JSValueMakeString(context, hintString.get());
}

static JSValueRef getIPhoneValueCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> valueString(Adopt, toAXElement(thisObject)->iphoneValue());
    return JSValueMakeString(context, valueString.get());
}

static JSValueRef getIPhoneIdentifierCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> valueString(Adopt, toAXElement(thisObject)->iphoneIdentifier());
    return JSValueMakeString(context, valueString.get());
}


static JSValueRef getIPhoneTraitsCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> valueString(Adopt, toAXElement(thisObject)->iphoneTraits());
    return JSValueMakeString(context, valueString.get());
}

static JSValueRef getIPhoneIsElementCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeBoolean(context, toAXElement(thisObject)->iphoneIsElement());
}

static JSValueRef getIPhoneElementTextPositionCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->iphoneElementTextPosition());
}

static JSValueRef getIPhoneElementTextLengthCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeNumber(context, toAXElement(thisObject)->iphoneElementTextLength());
}

#endif // PLATFORM(IOS)

#if PLATFORM(MAC) && !PLATFORM(IOS)
static JSValueRef supportedActionsCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> valueString(Adopt, toAXElement(thisObject)->supportedActions());
    return JSValueMakeString(context, valueString.get());
}

static JSValueRef mathPostscriptsDescriptionCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> valueString(Adopt, toAXElement(thisObject)->mathPostscriptsDescription());
    return JSValueMakeString(context, valueString.get());
}

static JSValueRef mathPrescriptsDescriptionCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> valueString(Adopt, toAXElement(thisObject)->mathPrescriptsDescription());
    return JSValueMakeString(context, valueString.get());
}

#endif

// Implementation

// Unsupported methods on various platforms.
#if !PLATFORM(MAC) || PLATFORM(IOS)
JSStringRef AccessibilityUIElement::speak() { return 0; }
JSStringRef AccessibilityUIElement::rangeForLine(int line) { return 0; }
JSStringRef AccessibilityUIElement::rangeForPosition(int, int) { return 0; }
void AccessibilityUIElement::setSelectedChild(AccessibilityUIElement*) const { }
unsigned AccessibilityUIElement::selectedChildrenCount() const { return 0; }
AccessibilityUIElement AccessibilityUIElement::selectedChildAtIndex(unsigned) const { return 0; }
AccessibilityUIElement AccessibilityUIElement::horizontalScrollbar() const { return 0; }
AccessibilityUIElement AccessibilityUIElement::verticalScrollbar() const { return 0; }
AccessibilityUIElement AccessibilityUIElement::uiElementAttributeValue(JSStringRef) const { return 0; }
#endif

#if !PLATFORM(MAC) && !PLATFORM(IOS)
JSStringRef AccessibilityUIElement::pathDescription() const { return 0; }
#endif

#if !PLATFORM(WIN)
bool AccessibilityUIElement::isEqual(AccessibilityUIElement* otherElement)
{
    if (!otherElement)
        return false;
    return platformUIElement() == otherElement->platformUIElement();
}
#endif

#if !SUPPORTS_AX_TEXTMARKERS

AccessibilityTextMarkerRange AccessibilityUIElement::textMarkerRangeForElement(AccessibilityUIElement*)
{
    return 0;
}

int AccessibilityUIElement::textMarkerRangeLength(AccessibilityTextMarkerRange*)
{
    return 0;
}

AccessibilityTextMarkerRange AccessibilityUIElement::textMarkerRangeForMarkers(AccessibilityTextMarker*, AccessibilityTextMarker*)
{
    return 0;
}

AccessibilityTextMarker AccessibilityUIElement::startTextMarkerForTextMarkerRange(AccessibilityTextMarkerRange*)
{
    return 0;
}

AccessibilityTextMarker AccessibilityUIElement::endTextMarkerForTextMarkerRange(AccessibilityTextMarkerRange*)
{
    return 0;   
}

AccessibilityUIElement AccessibilityUIElement::accessibilityElementForTextMarker(AccessibilityTextMarker*)
{
    return 0;
}

AccessibilityTextMarker AccessibilityUIElement::textMarkerForPoint(int x, int y)
{
    return 0;
}

AccessibilityTextMarker AccessibilityUIElement::previousTextMarker(AccessibilityTextMarker*)
{
    return 0;    
}

AccessibilityTextMarker AccessibilityUIElement::nextTextMarker(AccessibilityTextMarker*)
{
    return 0;
}

JSStringRef AccessibilityUIElement::stringForTextMarkerRange(AccessibilityTextMarkerRange*)
{
    return 0;
}

bool AccessibilityUIElement::attributedStringForTextMarkerRangeContainsAttribute(JSStringRef, AccessibilityTextMarkerRange*)
{
    return false;
}

int AccessibilityUIElement::indexForTextMarker(AccessibilityTextMarker*)
{
    return -1;
}

bool AccessibilityUIElement::isTextMarkerValid(AccessibilityTextMarker*)
{
    return false;
}

AccessibilityTextMarker AccessibilityUIElement::textMarkerForIndex(int)
{
    return 0;
}

#endif

// Destruction

static void finalize(JSObjectRef thisObject)
{
    delete toAXElement(thisObject);
}

// Object Creation

JSObjectRef AccessibilityUIElement::makeJSAccessibilityUIElement(JSContextRef context, const AccessibilityUIElement& element)
{
    return JSObjectMake(context, AccessibilityUIElement::getJSClass(), new AccessibilityUIElement(element));
}

JSClassRef AccessibilityUIElement::getJSClass()
{
    static JSStaticValue staticValues[] = {
        { "accessibilityValue", getAccessibilityValueCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "role", getRoleCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "subrole", getSubroleCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "roleDescription", getRoleDescriptionCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "title", getTitleCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "description", getDescriptionCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "language", getLanguageCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "helpText", getHelpTextCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "stringValue", getStringValueCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "x", getXCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "y", getYCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "width", getWidthCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "height", getHeightCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "clickPointX", getClickPointXCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "clickPointY", getClickPointYCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "intValue", getIntValueCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "minValue", getMinValueCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "maxValue", getMaxValueCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "pathDescription", getPathDescriptionCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "childrenCount", getChildrenCountCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "rowCount", rowCountCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "columnCount", columnCountCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "insertionPointLineNumber", getInsertionPointLineNumberCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "selectedTextRange", getSelectedTextRangeCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isEnabled", getIsEnabledCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isRequired", getIsRequiredCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isFocused", getIsFocusedCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isFocusable", getIsFocusableCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isSelected", getIsSelectedCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isSelectable", getIsSelectableCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isMultiSelectable", getIsMultiSelectableCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isSelectedOptionActive", getIsSelectedOptionActiveCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isExpanded", getIsExpandedCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isChecked", getIsCheckedCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isVisible", getIsVisibleCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isOffScreen", getIsOffScreenCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isCollapsed", getIsCollapsedCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "hasPopup", getHasPopupCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "valueDescription", getValueDescriptionCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "hierarchicalLevel", hierarchicalLevelCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "documentEncoding", getDocumentEncodingCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "documentURI", getDocumentURICallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "url", getURLCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isValid", getIsValidCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "orientation", getOrientationCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "ariaIsGrabbed", getARIAIsGrabbedCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "ariaDropEffects", getARIADropEffectsCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isIgnored", isIgnoredCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "speak", speakCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "selectedChildrenCount", selectedChildrenCountCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "horizontalScrollbar", horizontalScrollbarCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "verticalScrollbar", verticalScrollbarCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
#if PLATFORM(IOS)
        { "iphoneLabel", getIPhoneLabelCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "iphoneHint", getIPhoneHintCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "iphoneValue", getIPhoneValueCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "iphoneIdentifier", getIPhoneIdentifierCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "iphoneTraits", getIPhoneTraitsCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "iphoneIsElement", getIPhoneIsElementCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "iphoneElementTextPosition", getIPhoneElementTextPositionCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "iphoneElementTextLength", getIPhoneElementTextLengthCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "stringForSelection", stringForSelectionCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
#endif // PLATFORM(IOS)
#if PLATFORM(MAC) && !PLATFORM(IOS)
        { "supportedActions", supportedActionsCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "mathPostscriptsDescription", mathPostscriptsDescriptionCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "mathPrescriptsDescription", mathPrescriptsDescriptionCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
#endif
        { 0, 0, 0, 0 }
    };

    static JSStaticFunction staticFunctions[] = {
        { "allAttributes", allAttributesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "attributesOfLinkedUIElements", attributesOfLinkedUIElementsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "attributesOfDocumentLinks", attributesOfDocumentLinksCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "attributesOfChildren", attributesOfChildrenCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "parameterizedAttributeNames", parameterizedAttributeNamesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "lineForIndex", lineForIndexCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "rangeForLine", rangeForLineCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "boundsForRange", boundsForRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "rangeForPosition", rangeForPositionCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "stringForRange", stringForRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "attributedStringForRange", attributedStringForRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "attributedStringRangeIsMisspelled", attributedStringRangeIsMisspelledCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "uiElementForSearchPredicate", uiElementForSearchPredicateCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "childAtIndex", childAtIndexCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "linkedUIElementAtIndex", linkedUIElementAtIndexCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "indexOfChild", indexOfChildCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "elementAtPoint", elementAtPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "attributesOfColumnHeaders", attributesOfColumnHeadersCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "attributesOfRowHeaders", attributesOfRowHeadersCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "attributesOfColumns", attributesOfColumnsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "attributesOfRows", attributesOfRowsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "attributesOfVisibleCells", attributesOfVisibleCellsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "attributesOfHeader", attributesOfHeaderCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "indexInTable", indexInTableCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "rowIndexRange", rowIndexRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "columnIndexRange", columnIndexRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "cellForColumnAndRow", cellForColumnAndRowCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "titleUIElement", titleUIElementCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setSelectedTextRange", setSelectedTextRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "stringAttributeValue", stringAttributeValueCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "uiElementAttributeValue", uiElementAttributeValueCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "numberAttributeValue", numberAttributeValueCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "boolAttributeValue", boolAttributeValueCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isAttributeSupported", isAttributeSupportedCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isAttributeSettable", isAttributeSettableCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isPressActionSupported", isPressActionSupportedCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isIncrementActionSupported", isIncrementActionSupportedCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isDecrementActionSupported", isDecrementActionSupportedCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "parentElement", parentElementCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "disclosedByRow", disclosedByRowCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "increment", incrementCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "decrement", decrementCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "showMenu", showMenuCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "press", pressCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "disclosedRowAtIndex", disclosedRowAtIndexCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "ariaOwnsElementAtIndex", ariaOwnsElementAtIndexCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "ariaFlowToElementAtIndex", ariaFlowToElementAtIndexCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "selectedRowAtIndex", selectedRowAtIndexCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "rowAtIndex", rowAtIndexCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isEqual", isEqualCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "addNotificationListener", addNotificationListenerCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "removeNotificationListener", removeNotificationListenerCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "takeFocus", takeFocusCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "takeSelection", takeSelectionCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "addSelection", addSelectionCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "removeSelection", removeSelectionCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "textMarkerRangeForElement", textMarkerRangeForElementCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "attributedStringForTextMarkerRangeContainsAttribute", attributedStringForTextMarkerRangeContainsAttributeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "indexForTextMarker", indexForTextMarkerCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isTextMarkerValid", isTextMarkerValidCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "textMarkerRangeForMarkers", textMarkerRangeForMarkersCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "textMarkerForIndex", textMarkerForIndexCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "startTextMarkerForTextMarkerRange", startTextMarkerForTextMarkerRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "endTextMarkerForTextMarkerRange", endTextMarkerForTextMarkerRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "accessibilityElementForTextMarker", accessibilityElementForTextMarkerCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "textMarkerRangeLength", textMarkerRangeLengthCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "textMarkerForPoint", textMarkerForPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "nextTextMarker", nextTextMarkerCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "previousTextMarker", previousTextMarkerCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "stringForTextMarkerRange", stringForTextMarkerRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setSelectedChild", setSelectedChildCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "selectedChildAtIndex", selectedChildAtIndexCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "scrollToMakeVisible", scrollToMakeVisibleCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
#if PLATFORM(IOS)
        { "linkedElement", linkedElementCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "headerElementAtIndex", headerElementAtIndexCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "elementsForRange", elementsForRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "increaseTextSelection", increaseTextSelectionCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "decreaseTextSelection", decreaseTextSelectionCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "assistiveTechnologySimulatedFocus", assistiveTechnologySimulatedFocusCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        
#endif
        { 0, 0, 0 }
    };

    static JSClassDefinition classDefinition = {
        0, kJSClassAttributeNone, "AccessibilityUIElement", 0, staticValues, staticFunctions,
        0, finalize, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    static JSClassRef accessibilityUIElementClass = JSClassCreate(&classDefinition);
    return accessibilityUIElementClass;
}
#endif
