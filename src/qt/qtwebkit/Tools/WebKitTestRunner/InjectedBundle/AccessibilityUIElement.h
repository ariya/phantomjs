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

#ifndef AccessibilityUIElement_h
#define AccessibilityUIElement_h

#include "AccessibilityTextMarker.h"
#include "AccessibilityTextMarkerRange.h"
#include "JSWrappable.h"

#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <wtf/Platform.h>
#include <wtf/Vector.h>

#if PLATFORM(MAC)
#ifdef __OBJC__
typedef id PlatformUIElement;
#else
typedef struct objc_object* PlatformUIElement;
#endif
#elif PLATFORM(WIN)
#undef _WINSOCKAPI_
#define _WINSOCKAPI_ // Prevent inclusion of winsock.h in windows.h

#include <WebCore/COMPtr.h>
#include <oleacc.h>

typedef COMPtr<IAccessible> PlatformUIElement;
#elif PLATFORM(GTK) || (PLATFORM(EFL) && HAVE(ACCESSIBILITY))
#include <atk/atk.h>
#include <wtf/gobject/GRefPtr.h>
typedef GRefPtr<AtkObject> PlatformUIElement;
#else
typedef void* PlatformUIElement;
#endif

#if PLATFORM(MAC)
#ifdef __OBJC__
typedef id NotificationHandler;
#else
typedef struct objc_object* NotificationHandler;
#endif
#endif

namespace WTR {

class AccessibilityUIElement : public JSWrappable {
public:
    static PassRefPtr<AccessibilityUIElement> create(PlatformUIElement);
    static PassRefPtr<AccessibilityUIElement> create(const AccessibilityUIElement&);

    ~AccessibilityUIElement();

    PlatformUIElement platformUIElement() { return m_element; }
    virtual JSClassRef wrapperClass();

    static JSObjectRef makeJSAccessibilityUIElement(JSContextRef, const AccessibilityUIElement&);

    bool isEqual(AccessibilityUIElement* otherElement);
    
    PassRefPtr<AccessibilityUIElement> elementAtPoint(int x, int y);
    PassRefPtr<AccessibilityUIElement> childAtIndex(unsigned);
    unsigned indexOfChild(AccessibilityUIElement*);
    int childrenCount();
    PassRefPtr<AccessibilityUIElement> titleUIElement();
    PassRefPtr<AccessibilityUIElement> parentElement();

    void takeFocus();
    void takeSelection();
    void addSelection();
    void removeSelection();

    // Methods - platform-independent implementations
    JSRetainPtr<JSStringRef> allAttributes();
    JSRetainPtr<JSStringRef> attributesOfLinkedUIElements();
    PassRefPtr<AccessibilityUIElement> linkedUIElementAtIndex(unsigned);
    
    JSRetainPtr<JSStringRef> attributesOfDocumentLinks();
    JSRetainPtr<JSStringRef> attributesOfChildren();
    JSRetainPtr<JSStringRef> parameterizedAttributeNames();
    void increment();
    void decrement();
    void showMenu();
    void press();

    // Attributes - platform-independent implementations
    JSRetainPtr<JSStringRef> stringAttributeValue(JSStringRef attribute);
    double numberAttributeValue(JSStringRef attribute);
    PassRefPtr<AccessibilityUIElement> uiElementAttributeValue(JSStringRef attribute) const;
    bool boolAttributeValue(JSStringRef attribute);
    bool isAttributeSupported(JSStringRef attribute);
    bool isAttributeSettable(JSStringRef attribute);
    bool isPressActionSupported();
    bool isIncrementActionSupported();
    bool isDecrementActionSupported();
    JSRetainPtr<JSStringRef> role();
    JSRetainPtr<JSStringRef> subrole();
    JSRetainPtr<JSStringRef> roleDescription();
    JSRetainPtr<JSStringRef> title();
    JSRetainPtr<JSStringRef> description();
    JSRetainPtr<JSStringRef> language();
    JSRetainPtr<JSStringRef> stringValue();
    JSRetainPtr<JSStringRef> accessibilityValue() const;
    JSRetainPtr<JSStringRef> helpText() const;
    JSRetainPtr<JSStringRef> orientation() const;
    double x();
    double y();
    double width();
    double height();
    double intValue() const;
    double minValue();
    double maxValue();
    JSRetainPtr<JSStringRef> valueDescription();
    int insertionPointLineNumber();
    JSRetainPtr<JSStringRef> selectedTextRange();
    bool isEnabled();
    bool isRequired() const;
    
    bool isFocused() const;
    bool isFocusable() const;
    bool isSelected() const;
    bool isSelectable() const;
    bool isMultiSelectable() const;
    void setSelectedChild(AccessibilityUIElement*) const;
    unsigned selectedChildrenCount() const;
    PassRefPtr<AccessibilityUIElement> selectedChildAtIndex(unsigned) const;
    
    bool isValid() const;
    bool isExpanded() const;
    bool isChecked() const;
    bool isVisible() const;
    bool isOffScreen() const;
    bool isCollapsed() const;
    bool isIgnored() const;
    bool hasPopup() const;
    int hierarchicalLevel() const;
    double clickPointX();
    double clickPointY();
    JSRetainPtr<JSStringRef> documentEncoding();
    JSRetainPtr<JSStringRef> documentURI();
    JSRetainPtr<JSStringRef> url();

    // CSS3-speech properties.
    JSRetainPtr<JSStringRef> speak();
    
    // Table-specific attributes
    JSRetainPtr<JSStringRef> attributesOfColumnHeaders();
    JSRetainPtr<JSStringRef> attributesOfRowHeaders();
    JSRetainPtr<JSStringRef> attributesOfColumns();
    JSRetainPtr<JSStringRef> attributesOfRows();
    JSRetainPtr<JSStringRef> attributesOfVisibleCells();
    JSRetainPtr<JSStringRef> attributesOfHeader();
    int indexInTable();
    JSRetainPtr<JSStringRef> rowIndexRange();
    JSRetainPtr<JSStringRef> columnIndexRange();
    int rowCount();
    int columnCount();
    
    // Tree/Outline specific attributes
    PassRefPtr<AccessibilityUIElement> selectedRowAtIndex(unsigned);
    PassRefPtr<AccessibilityUIElement> disclosedByRow();
    PassRefPtr<AccessibilityUIElement> disclosedRowAtIndex(unsigned);
    PassRefPtr<AccessibilityUIElement> rowAtIndex(unsigned);

    // ARIA specific
    PassRefPtr<AccessibilityUIElement> ariaOwnsElementAtIndex(unsigned);
    PassRefPtr<AccessibilityUIElement> ariaFlowToElementAtIndex(unsigned);

    // ARIA Drag and Drop
    bool ariaIsGrabbed() const;
    // A space concatentated string of all the drop effects.
    JSRetainPtr<JSStringRef> ariaDropEffects() const;
    
    // Parameterized attributes
    int lineForIndex(int);
    JSRetainPtr<JSStringRef> rangeForLine(int);
    JSRetainPtr<JSStringRef> rangeForPosition(int x, int y);
    JSRetainPtr<JSStringRef> boundsForRange(unsigned location, unsigned length);
    void setSelectedTextRange(unsigned location, unsigned length);
    JSRetainPtr<JSStringRef> stringForRange(unsigned location, unsigned length);
    JSRetainPtr<JSStringRef> attributedStringForRange(unsigned location, unsigned length);
    bool attributedStringRangeIsMisspelled(unsigned location, unsigned length);
    PassRefPtr<AccessibilityUIElement> uiElementForSearchPredicate(JSContextRef, AccessibilityUIElement* startElement, bool isDirectionNext, JSValueRef searchKey, JSStringRef searchText, bool visibleOnly);
    
    // Table-specific
    PassRefPtr<AccessibilityUIElement> cellForColumnAndRow(unsigned column, unsigned row);

    // Scrollarea-specific
    PassRefPtr<AccessibilityUIElement> horizontalScrollbar() const;
    PassRefPtr<AccessibilityUIElement> verticalScrollbar() const;

    void scrollToMakeVisible();
    
    // Text markers.
    PassRefPtr<AccessibilityTextMarkerRange> textMarkerRangeForElement(AccessibilityUIElement*);    
    PassRefPtr<AccessibilityTextMarkerRange> textMarkerRangeForMarkers(AccessibilityTextMarker* startMarker, AccessibilityTextMarker* endMarker);
    PassRefPtr<AccessibilityTextMarker> startTextMarkerForTextMarkerRange(AccessibilityTextMarkerRange*);
    PassRefPtr<AccessibilityTextMarker> endTextMarkerForTextMarkerRange(AccessibilityTextMarkerRange*);
    PassRefPtr<AccessibilityTextMarker> textMarkerForPoint(int x, int y);
    PassRefPtr<AccessibilityTextMarker> previousTextMarker(AccessibilityTextMarker*);
    PassRefPtr<AccessibilityTextMarker> nextTextMarker(AccessibilityTextMarker*);
    PassRefPtr<AccessibilityUIElement> accessibilityElementForTextMarker(AccessibilityTextMarker*);
    JSRetainPtr<JSStringRef> stringForTextMarkerRange(AccessibilityTextMarkerRange*);
    int textMarkerRangeLength(AccessibilityTextMarkerRange*);
    bool attributedStringForTextMarkerRangeContainsAttribute(JSStringRef, AccessibilityTextMarkerRange*);
    int indexForTextMarker(AccessibilityTextMarker*);
    bool isTextMarkerValid(AccessibilityTextMarker*);
    PassRefPtr<AccessibilityTextMarker> textMarkerForIndex(int);

    // Returns an ordered list of supported actions for an element.
    JSRetainPtr<JSStringRef> supportedActions() const;
    JSRetainPtr<JSStringRef> mathPostscriptsDescription() const;
    JSRetainPtr<JSStringRef> mathPrescriptsDescription() const;

    JSRetainPtr<JSStringRef> pathDescription() const;
    
    // Notifications
    // Function callback should take one argument, the name of the notification.
    bool addNotificationListener(JSValueRef functionCallback);
    // Make sure you call remove, because you can't rely on objects being deallocated in a timely fashion.
    bool removeNotificationListener();
    
private:
    AccessibilityUIElement(PlatformUIElement);
    AccessibilityUIElement(const AccessibilityUIElement&);

    PlatformUIElement m_element;
    
    // A retained, platform specific object used to help manage notifications for this object.
#if PLATFORM(MAC)
    NotificationHandler m_notificationHandler;

    void getLinkedUIElements(Vector<RefPtr<AccessibilityUIElement> >&);
    void getDocumentLinks(Vector<RefPtr<AccessibilityUIElement> >&);
#endif

#if PLATFORM(MAC) || PLATFORM(GTK) || (PLATFORM(EFL) && HAVE(ACCESSIBILITY))
    void getChildren(Vector<RefPtr<AccessibilityUIElement> >&);
    void getChildrenWithRange(Vector<RefPtr<AccessibilityUIElement> >&, unsigned location, unsigned length);
#endif
};
    
} // namespace WTR
    
#endif // AccessibilityUIElement_h
