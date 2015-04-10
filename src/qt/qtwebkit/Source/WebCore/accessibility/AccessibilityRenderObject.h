/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef AccessibilityRenderObject_h
#define AccessibilityRenderObject_h

#include "AccessibilityNodeObject.h"
#include "LayoutRect.h"
#include <wtf/Forward.h>

namespace WebCore {
    
class AccessibilitySVGRoot;
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
    
class AccessibilityRenderObject : public AccessibilityNodeObject {
protected:
    explicit AccessibilityRenderObject(RenderObject*);
public:
    static PassRefPtr<AccessibilityRenderObject> create(RenderObject*);
    virtual ~AccessibilityRenderObject();
    
    virtual bool isAccessibilityRenderObject() const { return true; }

    virtual void init();
    
    virtual bool isAttachment() const;
    virtual bool isFileUploadButton() const;

    virtual bool isSelected() const;
    virtual bool isFocused() const;
    virtual bool isLoaded() const;
    virtual bool isOffScreen() const;
    virtual bool isReadOnly() const;
    virtual bool isUnvisited() const;
    virtual bool isVisited() const;        
    virtual bool isLinked() const;
    virtual bool hasBoldFont() const;
    virtual bool hasItalicFont() const;
    virtual bool hasPlainText() const;
    virtual bool hasSameFont(RenderObject*) const;
    virtual bool hasSameFontColor(RenderObject*) const;
    virtual bool hasSameStyle(RenderObject*) const;
    virtual bool hasUnderline() const;

    virtual bool canSetTextRangeAttributes() const;
    virtual bool canSetValueAttribute() const;
    virtual bool canSetExpandedAttribute() const;

    virtual void setAccessibleName(const AtomicString&);
    
    // Provides common logic used by all elements when determining isIgnored.
    virtual AccessibilityObjectInclusion defaultObjectInclusion() const;
    
    virtual int layoutCount() const;
    virtual double estimatedLoadingProgress() const;
    
    virtual AccessibilityObject* firstChild() const;
    virtual AccessibilityObject* lastChild() const;
    virtual AccessibilityObject* previousSibling() const;
    virtual AccessibilityObject* nextSibling() const;
    virtual AccessibilityObject* parentObject() const;
    virtual AccessibilityObject* parentObjectIfExists() const;
    virtual AccessibilityObject* observableObject() const;
    virtual void linkedUIElements(AccessibilityChildrenVector&) const;
    virtual bool exposesTitleUIElement() const;
    virtual AccessibilityObject* titleUIElement() const;
    virtual AccessibilityObject* correspondingControlForLabelElement() const;
    virtual AccessibilityObject* correspondingLabelForControlElement() const;

    virtual void ariaOwnsElements(AccessibilityChildrenVector&) const;
    virtual bool supportsARIAOwns() const;
    virtual bool isPresentationalChildOfAriaRole() const;
    virtual bool ariaRoleHasPresentationalChildren() const;
    
    // Should be called on the root accessibility object to kick off a hit test.
    virtual AccessibilityObject* accessibilityHitTest(const IntPoint&) const;

    FrameView* frameViewIfRenderView() const;
    virtual Element* anchorElement() const;
    
    virtual LayoutRect boundingBoxRect() const;
    virtual LayoutRect elementRect() const;
    virtual IntPoint clickPoint();
    
    void setRenderer(RenderObject*);
    virtual RenderObject* renderer() const { return m_renderer; }
    RenderBoxModelObject* renderBoxModelObject() const;
    virtual Node* node() const;

    virtual Document* document() const;

    RenderView* topRenderer() const;
    RenderTextControl* textControl() const;
    FrameView* topDocumentFrameView() const;  
    Document* topDocument() const;
    HTMLLabelElement* labelElementContainer() const;
    
    virtual KURL url() const;
    virtual PlainTextRange selectedTextRange() const;
    virtual VisibleSelection selection() const;
    virtual String stringValue() const;
    virtual String helpText() const;
    virtual String textUnderElement(AccessibilityTextUnderElementMode = TextUnderElementModeSkipIgnoredChildren) const;
    virtual String text() const;
    virtual int textLength() const;
    virtual String selectedText() const;
    virtual const AtomicString& accessKey() const;
    virtual const String& actionVerb() const;
    virtual Widget* widget() const;
    virtual Widget* widgetForAttachmentView() const;
    virtual void getDocumentLinks(AccessibilityChildrenVector&);
    virtual FrameView* documentFrameView() const;

    virtual void clearChildren();
    virtual void updateChildrenIfNecessary();
    
    virtual void setFocused(bool);
    virtual void setSelectedTextRange(const PlainTextRange&);
    virtual void setValue(const String&);
    virtual void setSelectedRows(AccessibilityChildrenVector&);
    virtual AccessibilityOrientation orientation() const;
    
    virtual void detach();
    virtual void textChanged();
    virtual void addChildren();
    virtual bool canHaveChildren() const;
    virtual void selectedChildren(AccessibilityChildrenVector&);
    virtual void visibleChildren(AccessibilityChildrenVector&);
    virtual void tabChildren(AccessibilityChildrenVector&);
    virtual bool shouldFocusActiveDescendant() const;
    bool shouldNotifyActiveDescendant() const;
    virtual AccessibilityObject* activeDescendant() const;
    virtual void handleActiveDescendantChanged();
    virtual void handleAriaExpandedChanged();
    
    virtual VisiblePositionRange visiblePositionRange() const;
    virtual VisiblePositionRange visiblePositionRangeForLine(unsigned) const;
    virtual IntRect boundsForVisiblePositionRange(const VisiblePositionRange&) const;
    virtual void setSelectedVisiblePositionRange(const VisiblePositionRange&) const;
    virtual bool supportsARIAFlowTo() const;
    virtual void ariaFlowToElements(AccessibilityChildrenVector&) const;
    virtual bool ariaHasPopup() const;

    virtual bool supportsARIADropping() const;
    virtual bool supportsARIADragging() const;
    virtual bool isARIAGrabbed();
    virtual void determineARIADropEffects(Vector<String>&);
    
    virtual VisiblePosition visiblePositionForPoint(const IntPoint&) const;
    virtual VisiblePosition visiblePositionForIndex(unsigned indexValue, bool lastIndexOK) const;    
    virtual int index(const VisiblePosition&) const;

    virtual VisiblePosition visiblePositionForIndex(int) const;
    virtual int indexForVisiblePosition(const VisiblePosition&) const;

    virtual void lineBreaks(Vector<int>&) const;
    virtual PlainTextRange doAXRangeForLine(unsigned) const;
    virtual PlainTextRange doAXRangeForIndex(unsigned) const;
    
    virtual String doAXStringForRange(const PlainTextRange&) const;
    virtual IntRect doAXBoundsForRange(const PlainTextRange&) const;
    
    virtual String stringValueForMSAA() const;
    virtual String stringRoleForMSAA() const;
    virtual String nameForMSAA() const;
    virtual String descriptionForMSAA() const;
    virtual AccessibilityRole roleValueForMSAA() const;

    virtual String passwordFieldValue() const;

protected:
    RenderObject* m_renderer;
    
    void setRenderObject(RenderObject* renderer) { m_renderer = renderer; }
    bool needsToUpdateChildren() const { return m_childrenDirty; }
    ScrollableArea* getScrollableAreaIfScrollable() const;
    void scrollTo(const IntPoint&) const;
    
    virtual bool isDetached() const { return !m_renderer; }

    virtual AccessibilityRole determineAccessibilityRole();
    virtual bool computeAccessibilityIsIgnored() const;

private:
    void ariaListboxSelectedChildren(AccessibilityChildrenVector&);
    void ariaListboxVisibleChildren(AccessibilityChildrenVector&);
    bool isAllowedChildOfTree() const;
    bool hasTextAlternative() const;
    String positionalDescriptionForMSAA() const;
    PlainTextRange ariaSelectedTextRange() const;
    Element* rootEditableElementForPosition(const Position&) const;
    bool nodeIsTextControl(const Node*) const;
    virtual void setNeedsToUpdateChildren() { m_childrenDirty = true; }
    virtual Path elementPath() const;
    
    bool isTabItemSelected() const;
    LayoutRect checkboxOrRadioRect() const;
    void addRadioButtonGroupMembers(AccessibilityChildrenVector& linkedUIElements) const;
    AccessibilityObject* internalLinkElement() const;
    AccessibilityObject* accessibilityImageMapHitTest(HTMLAreaElement*, const IntPoint&) const;
    AccessibilityObject* accessibilityParentForImageMap(HTMLMapElement*) const;
    virtual AccessibilityObject* elementAccessibilityHitTest(const IntPoint&) const;

    bool renderObjectIsObservable(RenderObject*) const;
    RenderObject* renderParentObject() const;
    bool isDescendantOfElementType(const QualifiedName& tagName) const;
    
    bool isSVGImage() const;
    void detachRemoteSVGRoot();
    AccessibilitySVGRoot* remoteSVGRootElement() const;
    AccessibilityObject* remoteSVGElementHitTest(const IntPoint&) const;
    void offsetBoundingBoxForRemoteSVGElement(LayoutRect&) const;
    virtual bool supportsPath() const;

    void addHiddenChildren();
    void addTextFieldChildren();
    void addImageMapChildren();
    void addCanvasChildren();
    void addAttachmentChildren();
    void addRemoteSVGChildren();
#if PLATFORM(MAC)
    void updateAttachmentViewParents();
#endif

    void ariaSelectedRows(AccessibilityChildrenVector&);
    
    bool elementAttributeValue(const QualifiedName&) const;
    void setElementAttributeValue(const QualifiedName&, bool);
    
    virtual ESpeak speakProperty() const;
    
    virtual const AtomicString& ariaLiveRegionStatus() const;
    virtual const AtomicString& ariaLiveRegionRelevant() const;
    virtual bool ariaLiveRegionAtomic() const;
    virtual bool ariaLiveRegionBusy() const;    

    bool inheritsPresentationalRole() const;

#if ENABLE(MATHML)
    // All math elements return true for isMathElement().
    virtual bool isMathElement() const;
    virtual bool isMathFraction() const;
    virtual bool isMathFenced() const;
    virtual bool isMathSubscriptSuperscript() const;
    virtual bool isMathRow() const;
    virtual bool isMathUnderOver() const;
    virtual bool isMathRoot() const;
    virtual bool isMathSquareRoot() const;
    virtual bool isMathText() const;
    virtual bool isMathNumber() const;
    virtual bool isMathOperator() const;
    virtual bool isMathFenceOperator() const;
    virtual bool isMathSeparatorOperator() const;
    virtual bool isMathIdentifier() const;
    virtual bool isMathTable() const;
    virtual bool isMathTableRow() const;
    virtual bool isMathTableCell() const;
    virtual bool isMathMultiscript() const;
    
    // Generic components.
    virtual AccessibilityObject* mathBaseObject();
    
    // Root components.
    virtual AccessibilityObject* mathRadicandObject();
    virtual AccessibilityObject* mathRootIndexObject();
    
    // Fraction components.
    virtual AccessibilityObject* mathNumeratorObject();
    virtual AccessibilityObject* mathDenominatorObject();

    // Under over components.
    virtual AccessibilityObject* mathUnderObject();
    virtual AccessibilityObject* mathOverObject();
    
    // Subscript/superscript components.
    virtual AccessibilityObject* mathSubscriptObject();
    virtual AccessibilityObject* mathSuperscriptObject();
    
    // Fenced components.
    virtual String mathFencedOpenString() const;
    virtual String mathFencedCloseString() const;
    virtual int mathLineThickness() const;

    // Multiscripts components.
    virtual void mathPrescripts(AccessibilityMathMultiscriptPairs&);
    virtual void mathPostscripts(AccessibilityMathMultiscriptPairs&);
    
    bool isIgnoredElementWithinMathTree() const;
#endif
};

inline AccessibilityRenderObject* toAccessibilityRenderObject(AccessibilityObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isAccessibilityRenderObject());
    return static_cast<AccessibilityRenderObject*>(object);
}

inline const AccessibilityRenderObject* toAccessibilityRenderObject(const AccessibilityObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isAccessibilityRenderObject());
    return static_cast<const AccessibilityRenderObject*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toAccessibilityRenderObject(const AccessibilityRenderObject*);

} // namespace WebCore

#endif // AccessibilityRenderObject_h
