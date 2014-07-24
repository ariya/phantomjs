/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#include <JavaScriptCore/JSObjectRef.h>
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
#elif HAVE(ACCESSIBILITY) && (PLATFORM(GTK) || PLATFORM(EFL))
#include <atk/atk.h>
typedef AtkObject* PlatformUIElement;
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

class AccessibilityUIElement {
public:
    AccessibilityUIElement(PlatformUIElement);
    AccessibilityUIElement(const AccessibilityUIElement&);
    ~AccessibilityUIElement();

    PlatformUIElement platformUIElement() { return m_element; }

    static JSObjectRef makeJSAccessibilityUIElement(JSContextRef, const AccessibilityUIElement&);

    bool isEqual(AccessibilityUIElement* otherElement);

    void getLinkedUIElements(Vector<AccessibilityUIElement>&);
    void getDocumentLinks(Vector<AccessibilityUIElement>&);
    void getChildren(Vector<AccessibilityUIElement>&);
    void getChildrenWithRange(Vector<AccessibilityUIElement>&, unsigned location, unsigned length);
    
    AccessibilityUIElement elementAtPoint(int x, int y);
    AccessibilityUIElement getChildAtIndex(unsigned);
    unsigned indexOfChild(AccessibilityUIElement*);
    int childrenCount();
    AccessibilityUIElement titleUIElement();
    AccessibilityUIElement parentElement();

    void takeFocus();
    void takeSelection();
    void addSelection();
    void removeSelection();

    // Methods - platform-independent implementations
    JSStringRef allAttributes();
    JSStringRef attributesOfLinkedUIElements();
    AccessibilityUIElement linkedUIElementAtIndex(unsigned);
    
    JSStringRef attributesOfDocumentLinks();
    JSStringRef attributesOfChildren();
    JSStringRef parameterizedAttributeNames();
    void increment();
    void decrement();
    void showMenu();
    void press();

    // Attributes - platform-independent implementations
    JSStringRef stringAttributeValue(JSStringRef attribute);
    double numberAttributeValue(JSStringRef attribute);
    AccessibilityUIElement uiElementAttributeValue(JSStringRef attribute) const;    
    bool boolAttributeValue(JSStringRef attribute);
    bool isAttributeSupported(JSStringRef attribute);
    bool isAttributeSettable(JSStringRef attribute);
    bool isPressActionSupported();
    bool isIncrementActionSupported();
    bool isDecrementActionSupported();
    JSStringRef role();
    JSStringRef subrole();
    JSStringRef roleDescription();
    JSStringRef title();
    JSStringRef description();
    JSStringRef language();
    JSStringRef stringValue();
    JSStringRef accessibilityValue() const;
    JSStringRef helpText() const;
    JSStringRef orientation() const;
    double x();
    double y();
    double width();
    double height();
    double intValue() const;
    double minValue();
    double maxValue();
    JSStringRef pathDescription() const;
    JSStringRef valueDescription();
    int insertionPointLineNumber();
    JSStringRef selectedTextRange();
    bool isEnabled();
    bool isRequired() const;
    
    bool isFocused() const;
    bool isFocusable() const;
    bool isSelected() const;
    bool isSelectable() const;
    bool isMultiSelectable() const;
    bool isSelectedOptionActive() const;
    void setSelectedChild(AccessibilityUIElement*) const;
    unsigned selectedChildrenCount() const;
    AccessibilityUIElement selectedChildAtIndex(unsigned) const;
    
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
    JSStringRef documentEncoding();
    JSStringRef documentURI();
    JSStringRef url();

    // CSS3-speech properties.
    JSStringRef speak();
    
    // Table-specific attributes
    JSStringRef attributesOfColumnHeaders();
    JSStringRef attributesOfRowHeaders();
    JSStringRef attributesOfColumns();
    JSStringRef attributesOfRows();
    JSStringRef attributesOfVisibleCells();
    JSStringRef attributesOfHeader();
    int indexInTable();
    JSStringRef rowIndexRange();
    JSStringRef columnIndexRange();
    int rowCount();
    int columnCount();
    
    // Tree/Outline specific attributes
    AccessibilityUIElement selectedRowAtIndex(unsigned);
    AccessibilityUIElement disclosedByRow();
    AccessibilityUIElement disclosedRowAtIndex(unsigned);
    AccessibilityUIElement rowAtIndex(unsigned);

    // ARIA specific
    AccessibilityUIElement ariaOwnsElementAtIndex(unsigned);
    AccessibilityUIElement ariaFlowToElementAtIndex(unsigned);

    // ARIA Drag and Drop
    bool ariaIsGrabbed() const;
    // A space concatentated string of all the drop effects.
    JSStringRef ariaDropEffects() const;
    
    // Parameterized attributes
    int lineForIndex(int);
    JSStringRef rangeForLine(int);
    JSStringRef rangeForPosition(int x, int y);
    JSStringRef boundsForRange(unsigned location, unsigned length);
    void setSelectedTextRange(unsigned location, unsigned length);
    JSStringRef stringForRange(unsigned location, unsigned length);
    JSStringRef attributedStringForRange(unsigned location, unsigned length);
    bool attributedStringRangeIsMisspelled(unsigned location, unsigned length);
    AccessibilityUIElement uiElementForSearchPredicate(JSContextRef, AccessibilityUIElement* startElement, bool isDirectionNext, JSValueRef searchKey, JSStringRef searchText, bool visibleOnly);
#if PLATFORM(IOS)
    void elementsForRange(unsigned location, unsigned length, Vector<AccessibilityUIElement>& elements);
    JSStringRef stringForSelection();
    void increaseTextSelection();
    void decreaseTextSelection();
    AccessibilityUIElement linkedElement();
#endif

    // Table-specific
    AccessibilityUIElement cellForColumnAndRow(unsigned column, unsigned row);

    // Scrollarea-specific
    AccessibilityUIElement horizontalScrollbar() const;
    AccessibilityUIElement verticalScrollbar() const;

    // Text markers.
    AccessibilityTextMarkerRange textMarkerRangeForElement(AccessibilityUIElement*);    
    AccessibilityTextMarkerRange textMarkerRangeForMarkers(AccessibilityTextMarker* startMarker, AccessibilityTextMarker* endMarker);
    AccessibilityTextMarker startTextMarkerForTextMarkerRange(AccessibilityTextMarkerRange*);
    AccessibilityTextMarker endTextMarkerForTextMarkerRange(AccessibilityTextMarkerRange*);
    AccessibilityTextMarker textMarkerForPoint(int x, int y);
    AccessibilityTextMarker previousTextMarker(AccessibilityTextMarker*);
    AccessibilityTextMarker nextTextMarker(AccessibilityTextMarker*);
    AccessibilityUIElement accessibilityElementForTextMarker(AccessibilityTextMarker*);
    JSStringRef stringForTextMarkerRange(AccessibilityTextMarkerRange*);
    int textMarkerRangeLength(AccessibilityTextMarkerRange*);
    bool attributedStringForTextMarkerRangeContainsAttribute(JSStringRef, AccessibilityTextMarkerRange*);
    int indexForTextMarker(AccessibilityTextMarker*);
    bool isTextMarkerValid(AccessibilityTextMarker*);
    AccessibilityTextMarker textMarkerForIndex(int);
    
    void scrollToMakeVisible();
    void scrollToMakeVisibleWithSubFocus(int x, int y, int width, int height);
    void scrollToGlobalPoint(int x, int y);

    // Notifications
    // Function callback should take one argument, the name of the notification.
    bool addNotificationListener(JSObjectRef functionCallback);
    // Make sure you call remove, because you can't rely on objects being deallocated in a timely fashion.
    void removeNotificationListener();
    
#if PLATFORM(IOS)
    JSStringRef iphoneLabel();
    JSStringRef iphoneValue();
    JSStringRef iphoneTraits();
    JSStringRef iphoneHint();
    JSStringRef iphoneIdentifier();
    bool iphoneIsElement();
    int iphoneElementTextPosition();
    int iphoneElementTextLength();
    AccessibilityUIElement headerElementAtIndex(unsigned);
    // This will simulate the accessibilityDidBecomeFocused API in UIKit.
    void assistiveTechnologySimulatedFocus();
#endif // PLATFORM(IOS)

#if PLATFORM(MAC) && !PLATFORM(IOS)
    // Returns an ordered list of supported actions for an element.
    JSStringRef supportedActions();
    
    // A general description of the elements making up multiscript pre/post objects.
    JSStringRef mathPostscriptsDescription() const;
    JSStringRef mathPrescriptsDescription() const;
#endif
    
private:
    static JSClassRef getJSClass();
    PlatformUIElement m_element;
    
    // A retained, platform specific object used to help manage notifications for this object.
#if PLATFORM(MAC)
    NotificationHandler m_notificationHandler;
#endif
};

#endif // AccessibilityUIElement_h
