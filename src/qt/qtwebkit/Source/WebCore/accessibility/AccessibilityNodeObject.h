/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AccessibilityNodeObject_h
#define AccessibilityNodeObject_h

#include "AccessibilityObject.h"
#include "LayoutRect.h"
#include <wtf/Forward.h>

namespace WebCore {
    
class AXObjectCache;
class Element;
class Frame;
class FrameView;
class HitTestResult;
class HTMLAnchorElement;
class HTMLAreaElement;
class HTMLElement;
class HTMLLabelElement;
class HTMLMapElement;
class HTMLSelectElement;
class IntPoint;
class IntSize;
class Node;
class RenderListBox;
class RenderTextControl;
class RenderView;
class VisibleSelection;
class Widget;
    
class AccessibilityNodeObject : public AccessibilityObject {
protected:
    explicit AccessibilityNodeObject(Node*);
public:
    static PassRefPtr<AccessibilityNodeObject> create(Node*);
    virtual ~AccessibilityNodeObject();

    virtual void init();
    
    virtual bool isAccessibilityNodeObject() const { return true; }

    virtual bool canvasHasFallbackContent() const;

    virtual bool isAnchor() const;
    virtual bool isControl() const;
    virtual bool isFieldset() const;
    virtual bool isGroup() const;
    virtual bool isHeading() const;
    virtual bool isHovered() const;
    virtual bool isImage() const;
    virtual bool isImageButton() const;
    virtual bool isInputImage() const;
    virtual bool isLink() const;
    virtual bool isMenu() const;
    virtual bool isMenuBar() const;
    virtual bool isMenuButton() const;
    virtual bool isMenuItem() const;
    virtual bool isMenuRelated() const;
    virtual bool isMultiSelectable() const;
    virtual bool isNativeCheckboxOrRadio() const;
    virtual bool isNativeImage() const;
    virtual bool isNativeTextControl() const;
    virtual bool isPasswordField() const;
    virtual bool isProgressIndicator() const;
    virtual bool isSearchField() const;
    virtual bool isSlider() const;

    virtual bool isChecked() const;
    virtual bool isEnabled() const;
    virtual bool isIndeterminate() const;
    virtual bool isPressed() const;
    virtual bool isReadOnly() const;
    virtual bool isRequired() const;
    virtual bool supportsRequiredAttribute() const;

    virtual bool canSetSelectedAttribute() const OVERRIDE;

    void setNode(Node*);
    virtual Node* node() const { return m_node; }
    virtual Document* document() const;

    virtual bool canSetFocusAttribute() const;
    virtual int headingLevel() const;

    virtual String valueDescription() const;
    virtual float valueForRange() const;
    virtual float maxValueForRange() const;
    virtual float minValueForRange() const;
    virtual float stepValueForRange() const;

    virtual AccessibilityObject* selectedRadioButton();
    virtual AccessibilityObject* selectedTabItem();
    virtual AccessibilityButtonState checkboxOrRadioValue() const;

    virtual unsigned hierarchicalLevel() const;
    virtual String textUnderElement(AccessibilityTextUnderElementMode = TextUnderElementModeSkipIgnoredChildren) const;
    virtual String accessibilityDescription() const;
    virtual String helpText() const;
    virtual String title() const;
    virtual String text() const;
    virtual String stringValue() const;
    virtual void colorValue(int& r, int& g, int& b) const;
    virtual String ariaLabeledByAttribute() const;

    virtual Element* actionElement() const;
    Element* mouseButtonListener() const;
    virtual Element* anchorElement() const;
    AccessibilityObject* menuForMenuButton() const;
   
    virtual void changeValueByPercent(float percentChange);
 
    virtual AccessibilityObject* firstChild() const;
    virtual AccessibilityObject* lastChild() const;
    virtual AccessibilityObject* previousSibling() const;
    virtual AccessibilityObject* nextSibling() const;
    virtual AccessibilityObject* parentObject() const;
    virtual AccessibilityObject* parentObjectIfExists() const;

    virtual void detach();
    virtual void childrenChanged();
    virtual void updateAccessibilityRole();

    virtual void increment();
    virtual void decrement();

    virtual LayoutRect elementRect() const;

protected:
    AccessibilityRole m_ariaRole;
    bool m_childrenDirty;
    mutable AccessibilityRole m_roleForMSAA;
#ifndef NDEBUG
    bool m_initialized;
#endif

    virtual bool isDetached() const { return !m_node; }

    virtual AccessibilityRole determineAccessibilityRole();
    virtual void addChildren();
    virtual void addChild(AccessibilityObject*);
    virtual void insertChild(AccessibilityObject*, unsigned index);

    virtual bool canHaveChildren() const;
    AccessibilityRole ariaRoleAttribute() const;
    AccessibilityRole determineAriaRoleAttribute() const;
    AccessibilityRole remapAriaRoleDueToParent(AccessibilityRole) const;
    bool hasContentEditableAttributeSet() const;
    virtual bool isDescendantOfBarrenParent() const;
    void alterSliderValue(bool increase);
    void changeValueByStep(bool increase);
    // This returns true if it's focusable but it's not content editable and it's not a control or ARIA control.
    bool isGenericFocusableElement() const;
    HTMLLabelElement* labelForElement(Element*) const;
    String ariaAccessibilityDescription() const;
    void ariaLabeledByElements(Vector<Element*>& elements) const;
    String accessibilityDescriptionForElements(Vector<Element*> &elements) const;
    void elementsFromAttribute(Vector<Element*>& elements, const QualifiedName&) const;
    virtual LayoutRect boundingBoxRect() const;
    String ariaDescribedByAttribute() const;
    
    Element* menuElementForMenuButton() const;
    Element* menuItemElementForMenu() const;
    AccessibilityObject* menuButtonForMenu() const;

private:
    Node* m_node;

    virtual void accessibilityText(Vector<AccessibilityText>&);
    void titleElementText(Vector<AccessibilityText>&);
    void alternativeText(Vector<AccessibilityText>&) const;
    void visibleText(Vector<AccessibilityText>&) const;
    void helpText(Vector<AccessibilityText>&) const;
    String alternativeTextForWebArea() const;
    void ariaLabeledByText(Vector<AccessibilityText>&) const;
    virtual bool computeAccessibilityIsIgnored() const;
};

inline AccessibilityNodeObject* toAccessibilityNodeObject(AccessibilityObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isAccessibilityNodeObject());
    return static_cast<AccessibilityNodeObject*>(object);
}

inline const AccessibilityNodeObject* toAccessibilityNodeObject(const AccessibilityObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isAccessibilityNodeObject());
    return static_cast<const AccessibilityNodeObject*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toAccessibilityNodeObject(const AccessibilityNodeObject*);

} // namespace WebCore

#endif // AccessibilityNodeObject_h
