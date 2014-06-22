/*
 * Copyright (C) 2008, 2013 Apple Inc. All Rights Reserved.
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

#include "AccessibilityController.h"
#include "DumpRenderTree.h"
#include "FrameLoadDelegate.h"
#include <JavaScriptCore/JSStringRef.h>
#include <wtf/text/WTFString.h>
#include <comutil.h>
#include <tchar.h>
#include <string>

using std::wstring;

static COMPtr<IAccessibleComparable> comparableObject(IAccessible* accessible)
{
    COMPtr<IServiceProvider> serviceProvider(Query, accessible);
    if (!serviceProvider)
        return 0;
    COMPtr<IAccessibleComparable> comparable;
    serviceProvider->QueryService(SID_AccessibleComparable, __uuidof(IAccessibleComparable), reinterpret_cast<void**>(&comparable));
    return comparable;
}

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

bool AccessibilityUIElement::isEqual(AccessibilityUIElement* otherElement)
{
    COMPtr<IAccessibleComparable> comparable = comparableObject(m_element.get());
    COMPtr<IAccessibleComparable> otherComparable = comparableObject(otherElement->m_element.get());
    if (!comparable || !otherComparable)
        return false;
    BOOL isSame = FALSE;
    if (FAILED(comparable->isSameObject(otherComparable.get(), &isSame)))
        return false;
    return isSame;
}

void AccessibilityUIElement::getLinkedUIElements(Vector<AccessibilityUIElement>&)
{
}

void AccessibilityUIElement::getDocumentLinks(Vector<AccessibilityUIElement>&)
{
}

void AccessibilityUIElement::getChildren(Vector<AccessibilityUIElement>& children)
{
    if (!m_element)
        return;

    long childCount;
    if (FAILED(m_element->get_accChildCount(&childCount)))
        return;
    for (long i = 0; i < childCount; ++i)
        children.append(getChildAtIndex(i));
}

void AccessibilityUIElement::getChildrenWithRange(Vector<AccessibilityUIElement>& elementVector, unsigned location, unsigned length)
{
    if (!m_element)
        return;

    long childCount;
    unsigned appendedCount = 0;
    if (FAILED(m_element->get_accChildCount(&childCount)))
        return;
    for (long i = location; i < childCount && appendedCount < length; ++i, ++appendedCount)
        elementVector.append(getChildAtIndex(i));
}

int AccessibilityUIElement::childrenCount()
{
    if (!m_element)
        return 0;

    long childCount;
    m_element->get_accChildCount(&childCount);
    return childCount;
}

int AccessibilityUIElement::rowCount()
{
    // FIXME: implement
    return 0;
}
 
int AccessibilityUIElement::columnCount()
{
    // FIXME: implement
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::elementAtPoint(int x, int y)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::linkedUIElementAtIndex(unsigned index)
{
    // FIXME: implement
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::getChildAtIndex(unsigned index)
{
    if (!m_element)
        return 0;

    COMPtr<IDispatch> child;
    VARIANT vChild;
    ::VariantInit(&vChild);
    V_VT(&vChild) = VT_I4;
    // In MSAA, index 0 is the object itself.
    V_I4(&vChild) = index + 1;
    if (FAILED(m_element->get_accChild(vChild, &child)))
        return 0;
    return COMPtr<IAccessible>(Query, child);
}

unsigned AccessibilityUIElement::indexOfChild(AccessibilityUIElement* element)
{ 
    // FIXME: implement
    return 0;
}

JSStringRef AccessibilityUIElement::allAttributes()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfLinkedUIElements()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfDocumentLinks()
{
    return JSStringCreateWithCharacters(0, 0);
}

AccessibilityUIElement AccessibilityUIElement::titleUIElement()
{
    COMPtr<IAccessible> platformElement = platformUIElement();

    COMPtr<IAccessibleComparable> comparable = comparableObject(platformElement.get());
    if (!comparable)
        return 0;

    VARIANT value;
    ::VariantInit(&value);

    _bstr_t titleUIElementAttributeKey(L"AXTitleUIElementAttribute");
    if (FAILED(comparable->get_attribute(titleUIElementAttributeKey, &value))) {
        ::VariantClear(&value);
        return 0;
    }

    if (V_VT(&value) == VT_EMPTY) {
        ::VariantClear(&value);
        return 0;
    }

    ASSERT(V_VT(&value) == VT_UNKNOWN);

    if (V_VT(&value) != VT_UNKNOWN) {
        ::VariantClear(&value);
        return 0;
    }

    COMPtr<IAccessible> titleElement(Query, value.punkVal);
    if (value.punkVal)
        value.punkVal->Release();
    ::VariantClear(&value);

    return titleElement;
}

AccessibilityUIElement AccessibilityUIElement::parentElement()
{
    if (!m_element)
        return 0;

    COMPtr<IDispatch> parent;
    m_element->get_accParent(&parent);

    COMPtr<IAccessible> parentAccessible(Query, parent);
    return parentAccessible;
}

JSStringRef AccessibilityUIElement::attributesOfChildren()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::parameterizedAttributeNames()
{
    return JSStringCreateWithCharacters(0, 0);
}

static VARIANT& self()
{
    static VARIANT vSelf;
    static bool haveInitialized;

    if (!haveInitialized) {
        ::VariantInit(&vSelf);
        V_VT(&vSelf) = VT_I4;
        V_I4(&vSelf) = CHILDID_SELF;
    }
    return vSelf;
}

JSStringRef AccessibilityUIElement::role()
{
    if (!m_element)
        return JSStringCreateWithCharacters(0, 0);

    VARIANT vRole;
    if (FAILED(m_element->get_accRole(self(), &vRole)))
        return JSStringCreateWithCharacters(0, 0);

    ASSERT(V_VT(&vRole) == VT_I4 || V_VT(&vRole) == VT_BSTR);

    wstring result;
    if (V_VT(&vRole) == VT_I4) {
        unsigned roleTextLength = ::GetRoleText(V_I4(&vRole), 0, 0) + 1;

        Vector<TCHAR> roleText(roleTextLength);

        ::GetRoleText(V_I4(&vRole), roleText.data(), roleTextLength);

        result = roleText.data();
    } else if (V_VT(&vRole) == VT_BSTR)
        result = wstring(V_BSTR(&vRole), ::SysStringLen(V_BSTR(&vRole)));

    ::VariantClear(&vRole);

    return JSStringCreateWithCharacters(result.data(), result.length());
}

JSStringRef AccessibilityUIElement::subrole()
{
    return 0;
}

JSStringRef AccessibilityUIElement::roleDescription()
{
    return 0;
}

JSStringRef AccessibilityUIElement::title()
{
    if (!m_element)
        return JSStringCreateWithCharacters(0, 0);

    BSTR titleBSTR;
    if (FAILED(m_element->get_accName(self(), &titleBSTR)) || !titleBSTR)
        return JSStringCreateWithCharacters(0, 0);
    wstring title(titleBSTR, SysStringLen(titleBSTR));
    ::SysFreeString(titleBSTR);
    return JSStringCreateWithCharacters(title.data(), title.length());
}

JSStringRef AccessibilityUIElement::description()
{
    if (!m_element)
        return JSStringCreateWithCharacters(0, 0);

    BSTR descriptionBSTR;
    if (FAILED(m_element->get_accDescription(self(), &descriptionBSTR)) || !descriptionBSTR)
        return JSStringCreateWithCharacters(0, 0);
    wstring description(descriptionBSTR, SysStringLen(descriptionBSTR));
    ::SysFreeString(descriptionBSTR);
    return JSStringCreateWithCharacters(description.data(), description.length());
}

JSStringRef AccessibilityUIElement::stringValue()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::language()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::helpText() const
{
    return 0;
}

double AccessibilityUIElement::x()
{
    if (!m_element)
        return 0;

    long x, y, width, height;
    if (FAILED(m_element->accLocation(&x, &y, &width, &height, self())))
        return 0;
    return x;
}

double AccessibilityUIElement::y()
{
    if (!m_element)
        return 0;

    long x, y, width, height;
    if (FAILED(m_element->accLocation(&x, &y, &width, &height, self())))
        return 0;
    return y;
}

double AccessibilityUIElement::width()
{
    if (!m_element)
        return 0;

    long x, y, width, height;
    if (FAILED(m_element->accLocation(&x, &y, &width, &height, self())))
        return 0;
    return width;
}

double AccessibilityUIElement::height()
{
    if (!m_element)
        return 0;

    long x, y, width, height;
    if (FAILED(m_element->accLocation(&x, &y, &width, &height, self())))
        return 0;
    return height;
}

double AccessibilityUIElement::clickPointX()
{
    return 0;
}

double AccessibilityUIElement::clickPointY()
{
    return 0;
}

JSStringRef AccessibilityUIElement::valueDescription()
{
    return 0;
}

static DWORD accessibilityState(COMPtr<IAccessible> element)
{
    VARIANT state;
    element->get_accState(self(), &state);

    ASSERT(V_VT(&state) == VT_I4);

    DWORD result = state.lVal;
    VariantClear(&state);

    return result;
}

bool AccessibilityUIElement::isFocused() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isSelected() const
{
    DWORD state = accessibilityState(m_element);
    return (state & STATE_SYSTEM_SELECTED) == STATE_SYSTEM_SELECTED;
}

int AccessibilityUIElement::hierarchicalLevel() const
{
    return 0;
}

bool AccessibilityUIElement::ariaIsGrabbed() const
{
    return false;
}
 
JSStringRef AccessibilityUIElement::ariaDropEffects() const
{
    return 0;
}

bool AccessibilityUIElement::isExpanded() const
{
    return false;
}

bool AccessibilityUIElement::isChecked() const
{
    if (!m_element)
        return false;

    VARIANT vState;
    if (FAILED(m_element->get_accState(self(), &vState)))
        return false;

    return vState.lVal & STATE_SYSTEM_CHECKED;
}

JSStringRef AccessibilityUIElement::orientation() const
{
    return 0;
}

double AccessibilityUIElement::intValue() const
{
    if (!m_element)
        return 0;

    BSTR valueBSTR;
    if (FAILED(m_element->get_accValue(self(), &valueBSTR)) || !valueBSTR)
        return 0;
    wstring value(valueBSTR, SysStringLen(valueBSTR));
    ::SysFreeString(valueBSTR);
    TCHAR* ignored;
    return _tcstod(value.data(), &ignored);
}

double AccessibilityUIElement::minValue()
{
    return 0;
}

double AccessibilityUIElement::maxValue()
{
    return 0;
}

bool AccessibilityUIElement::isPressActionSupported()
{
    if (!m_element)
        return 0;

    BSTR valueBSTR;
    if (FAILED(m_element->get_accDefaultAction(self(), &valueBSTR) || !valueBSTR))
        return false;

    if (!::SysStringLen(valueBSTR))
        return false;

    return true;
}

bool AccessibilityUIElement::isIncrementActionSupported()
{
    return false;
}

bool AccessibilityUIElement::isDecrementActionSupported()
{
    return false;
}

bool AccessibilityUIElement::isEnabled()
{
    DWORD state = accessibilityState(m_element);
    return (state & STATE_SYSTEM_UNAVAILABLE) != STATE_SYSTEM_UNAVAILABLE;
}

bool AccessibilityUIElement::isRequired() const
{
    return false;
}


int AccessibilityUIElement::insertionPointLineNumber()
{
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfColumnHeaders()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfRowHeaders()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfColumns()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfRows()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfVisibleCells()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfHeader()
{
    return JSStringCreateWithCharacters(0, 0);
}

int AccessibilityUIElement::indexInTable()
{
    return 0;
}

JSStringRef AccessibilityUIElement::rowIndexRange()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::columnIndexRange()
{
    return JSStringCreateWithCharacters(0, 0);
}

int AccessibilityUIElement::lineForIndex(int)
{
    return 0;
}

JSStringRef AccessibilityUIElement::boundsForRange(unsigned location, unsigned length)
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::stringForRange(unsigned, unsigned)
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributedStringForRange(unsigned, unsigned)
{
    return JSStringCreateWithCharacters(0, 0);
}

bool AccessibilityUIElement::attributedStringRangeIsMisspelled(unsigned, unsigned)
{
    return false;
}

AccessibilityUIElement AccessibilityUIElement::uiElementForSearchPredicate(JSContextRef context, AccessibilityUIElement* startElement, bool isDirectionNext, JSValueRef searchKey, JSStringRef searchText, bool visibleOnly)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::cellForColumnAndRow(unsigned column, unsigned row)
{
    return 0;
}

JSStringRef AccessibilityUIElement::selectedTextRange()
{
    return JSStringCreateWithCharacters(0, 0);    
}

void AccessibilityUIElement::setSelectedTextRange(unsigned location, unsigned length)
{
}

JSStringRef AccessibilityUIElement::stringAttributeValue(JSStringRef attribute)
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

double AccessibilityUIElement::numberAttributeValue(JSStringRef attribute)
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
    return false;
}

bool AccessibilityUIElement::isAttributeSupported(JSStringRef attribute)
{
    return false;
}

void AccessibilityUIElement::increment()
{
}

void AccessibilityUIElement::decrement()
{
}

void AccessibilityUIElement::showMenu()
{
    if (!m_element)
        return;

    ASSERT(hasPopup());
    m_element->accDoDefaultAction(self());
}

void AccessibilityUIElement::press()
{
    if (!m_element)
        return;

    m_element->accDoDefaultAction(self());
}

AccessibilityUIElement AccessibilityUIElement::disclosedRowAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::ariaOwnsElementAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::ariaFlowToElementAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::selectedRowAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::rowAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::disclosedByRow()
{
    return 0;
}

JSStringRef AccessibilityUIElement::accessibilityValue() const
{
    if (!m_element)
        return JSStringCreateWithCharacters(0, 0);

    BSTR valueBSTR;
    if (FAILED(m_element->get_accValue(self(), &valueBSTR)) || !valueBSTR)
        return JSStringCreateWithCharacters(0, 0);

    wstring value(valueBSTR, SysStringLen(valueBSTR));
    ::SysFreeString(valueBSTR);

    return JSStringCreateWithCharacters(value.data(), value.length());
}


JSStringRef AccessibilityUIElement::documentEncoding()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::documentURI()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::url()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

bool AccessibilityUIElement::addNotificationListener(JSObjectRef functionCallback)
{
    if (!functionCallback)
        return false;

    sharedFrameLoadDelegate->accessibilityController()->winAddNotificationListener(m_element, functionCallback);
    return true;
}

void AccessibilityUIElement::removeNotificationListener()
{
    // FIXME: implement
}

bool AccessibilityUIElement::isFocusable() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isSelectable() const
{
    DWORD state = accessibilityState(m_element);
    return (state & STATE_SYSTEM_SELECTABLE) == STATE_SYSTEM_SELECTABLE;
}

bool AccessibilityUIElement::isMultiSelectable() const
{
    DWORD multiSelectable = STATE_SYSTEM_EXTSELECTABLE | STATE_SYSTEM_MULTISELECTABLE;
    DWORD state = accessibilityState(m_element);
    return (state & multiSelectable) == multiSelectable;
}

bool AccessibilityUIElement::isSelectedOptionActive() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isVisible() const
{
    DWORD state = accessibilityState(m_element);
    return (state & STATE_SYSTEM_INVISIBLE) != STATE_SYSTEM_INVISIBLE;
}

bool AccessibilityUIElement::isOffScreen() const
{
    DWORD state = accessibilityState(m_element);
    return (state & STATE_SYSTEM_OFFSCREEN) == STATE_SYSTEM_OFFSCREEN;
}

bool AccessibilityUIElement::isCollapsed() const
{
    DWORD state = accessibilityState(m_element);
    return (state & STATE_SYSTEM_COLLAPSED) == STATE_SYSTEM_COLLAPSED;
}

bool AccessibilityUIElement::isIgnored() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::hasPopup() const
{
    DWORD state = accessibilityState(m_element);
    return (state & STATE_SYSTEM_HASPOPUP) == STATE_SYSTEM_HASPOPUP;
}

void AccessibilityUIElement::takeFocus()
{
    if (!m_element)
        return;

    m_element->accSelect(SELFLAG_TAKEFOCUS, self());
}

void AccessibilityUIElement::takeSelection()
{
    if (!m_element)
        return;

    m_element->accSelect(SELFLAG_TAKESELECTION, self());
}

void AccessibilityUIElement::addSelection()
{
    if (!m_element)
        return;

    m_element->accSelect(SELFLAG_ADDSELECTION, self());
}

void AccessibilityUIElement::removeSelection()
{
    if (!m_element)
        return;

    m_element->accSelect(SELFLAG_REMOVESELECTION, self());
}

void AccessibilityUIElement::scrollToMakeVisible()
{
    // FIXME: implement
}

void AccessibilityUIElement::scrollToMakeVisibleWithSubFocus(int x, int y, int width, int height)
{
    // FIXME: implement
}

void AccessibilityUIElement::scrollToGlobalPoint(int x, int y)
{
    // FIXME: implement
}
