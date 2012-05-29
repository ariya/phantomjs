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

#include "config.h"
#include "AccessibilityRenderObject.h"

#include "AXObjectCache.h"
#include "AccessibilityImageMapLink.h"
#include "AccessibilityListBox.h"
#include "EventNames.h"
#include "FloatRect.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTMLAreaElement.h"
#include "HTMLFormElement.h"
#include "HTMLFrameElementBase.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLLabelElement.h"
#include "HTMLMapElement.h"
#include "HTMLOptGroupElement.h"
#include "HTMLOptionElement.h"
#include "HTMLOptionsCollection.h"
#include "HTMLSelectElement.h"
#include "HTMLTextAreaElement.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "LocalizedStrings.h"
#include "MathMLNames.h"
#include "NodeList.h"
#include "ProgressTracker.h"
#include "RenderButton.h"
#include "RenderFieldset.h"
#include "RenderFileUploadControl.h"
#include "RenderHTMLCanvas.h"
#include "RenderImage.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderListBox.h"
#include "RenderListMarker.h"
#include "RenderMenuList.h"
#include "RenderText.h"
#include "RenderTextControl.h"
#include "RenderTextFragment.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "RenderWidget.h"
#include "SelectElement.h"
#include "SelectionController.h"
#include "Text.h"
#include "TextIterator.h"
#include "htmlediting.h"
#include "visible_units.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/unicode/CharacterNames.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

AccessibilityRenderObject::AccessibilityRenderObject(RenderObject* renderer)
    : AccessibilityObject()
    , m_renderer(renderer)
    , m_ariaRole(UnknownRole)
    , m_childrenDirty(false)
    , m_roleForMSAA(UnknownRole)
{
    m_role = determineAccessibilityRole();
    
#ifndef NDEBUG
    m_renderer->setHasAXObject(true);
#endif
}

AccessibilityRenderObject::~AccessibilityRenderObject()
{
    ASSERT(isDetached());
}

PassRefPtr<AccessibilityRenderObject> AccessibilityRenderObject::create(RenderObject* renderer)
{
    return adoptRef(new AccessibilityRenderObject(renderer));
}

void AccessibilityRenderObject::detach()
{
    clearChildren();
    AccessibilityObject::detach();
    
#ifndef NDEBUG
    if (m_renderer)
        m_renderer->setHasAXObject(false);
#endif
    m_renderer = 0;    
}

RenderBoxModelObject* AccessibilityRenderObject::renderBoxModelObject() const
{
    if (!m_renderer || !m_renderer->isBoxModelObject())
        return 0;
    return toRenderBoxModelObject(m_renderer);
}

static inline bool isInlineWithContinuation(RenderObject* object)
{
    if (!object->isBoxModelObject())
        return false;

    RenderBoxModelObject* renderer = toRenderBoxModelObject(object);
    if (!renderer->isRenderInline())
        return false;

    return toRenderInline(renderer)->continuation();
}

static inline RenderObject* firstChildInContinuation(RenderObject* renderer)
{
    RenderObject* r = toRenderInline(renderer)->continuation();

    while (r) {
        if (r->isRenderBlock())
            return r;
        if (RenderObject* child = r->firstChild())
            return child;
        r = toRenderInline(r)->continuation(); 
    }

    return 0;
}

static inline RenderObject* firstChildConsideringContinuation(RenderObject* renderer)
{
    RenderObject* firstChild = renderer->firstChild();

    if (!firstChild && isInlineWithContinuation(renderer))
        firstChild = firstChildInContinuation(renderer);

    return firstChild;
}


static inline RenderObject* lastChildConsideringContinuation(RenderObject* renderer)
{
    RenderObject* lastChild = renderer->lastChild();
    RenderObject* prev;
    RenderObject* cur = renderer;

    if (!cur->isRenderInline() && !cur->isRenderBlock())
        return renderer;

    while (cur) {
        prev = cur;

        if (RenderObject* lc = cur->lastChild())
            lastChild = lc;

        if (cur->isRenderInline()) {
            cur = toRenderInline(cur)->inlineElementContinuation();
            ASSERT_UNUSED(prev, cur || !toRenderInline(prev)->continuation());
        } else
            cur = toRenderBlock(cur)->inlineElementContinuation();
    }

    return lastChild;
}

AccessibilityObject* AccessibilityRenderObject::firstChild() const
{
    if (!m_renderer)
        return 0;
    
    RenderObject* firstChild = firstChildConsideringContinuation(m_renderer);

    if (!firstChild)
        return 0;
    
    return axObjectCache()->getOrCreate(firstChild);
}

AccessibilityObject* AccessibilityRenderObject::lastChild() const
{
    if (!m_renderer)
        return 0;

    RenderObject* lastChild = lastChildConsideringContinuation(m_renderer);

    if (!lastChild)
        return 0;
    
    return axObjectCache()->getOrCreate(lastChild);
}

static inline RenderInline* startOfContinuations(RenderObject* r)
{
    if (r->isInlineElementContinuation())
        return toRenderInline(r->node()->renderer());

    // Blocks with a previous continuation always have a next continuation
    if (r->isRenderBlock() && toRenderBlock(r)->inlineElementContinuation())
        return toRenderInline(toRenderBlock(r)->inlineElementContinuation()->node()->renderer());

    return 0;
}

static inline RenderObject* endOfContinuations(RenderObject* renderer)
{
    RenderObject* prev = renderer;
    RenderObject* cur = renderer;

    if (!cur->isRenderInline() && !cur->isRenderBlock())
        return renderer;

    while (cur) {
        prev = cur;
        if (cur->isRenderInline()) {
            cur = toRenderInline(cur)->inlineElementContinuation();
            ASSERT(cur || !toRenderInline(prev)->continuation());
        } else 
            cur = toRenderBlock(cur)->inlineElementContinuation();
    }

    return prev;
}


static inline RenderObject* childBeforeConsideringContinuations(RenderInline* r, RenderObject* child)
{
    RenderBoxModelObject* curContainer = r;
    RenderObject* cur = 0;
    RenderObject* prev = 0;

    while (curContainer) {
        if (curContainer->isRenderInline()) {
            cur = curContainer->firstChild();
            while (cur) {
                if (cur == child)
                    return prev;
                prev = cur;
                cur = cur->nextSibling();
            }

            curContainer = toRenderInline(curContainer)->continuation();
        } else if (curContainer->isRenderBlock()) {
            if (curContainer == child)
                return prev;

            prev = curContainer;
            curContainer = toRenderBlock(curContainer)->inlineElementContinuation();
        }
    }

    ASSERT_NOT_REACHED();

    return 0;
}

static inline bool firstChildIsInlineContinuation(RenderObject* renderer)
{
    return renderer->firstChild() && renderer->firstChild()->isInlineElementContinuation();
}

AccessibilityObject* AccessibilityRenderObject::previousSibling() const
{
    if (!m_renderer)
        return 0;

    RenderObject* previousSibling = 0;

    // Case 1: The node is a block and is an inline's continuation. In that case, the inline's
    // last child is our previous sibling (or further back in the continuation chain)
    RenderInline* startOfConts;
    if (m_renderer->isRenderBlock() && (startOfConts = startOfContinuations(m_renderer)))
        previousSibling = childBeforeConsideringContinuations(startOfConts, m_renderer);

    // Case 2: Anonymous block parent of the end of a continuation - skip all the way to before
    // the parent of the start, since everything in between will be linked up via the continuation.
    else if (m_renderer->isAnonymousBlock() && firstChildIsInlineContinuation(m_renderer)) {
        RenderObject* firstParent = startOfContinuations(m_renderer->firstChild())->parent();
        while (firstChildIsInlineContinuation(firstParent))
            firstParent = startOfContinuations(firstParent->firstChild())->parent();
        previousSibling = firstParent->previousSibling();
    }

    // Case 3: The node has an actual previous sibling
    else if (RenderObject* ps = m_renderer->previousSibling())
        previousSibling = ps;

    // Case 4: This node has no previous siblings, but its parent is an inline,
    // and is another node's inline continutation. Follow the continuation chain.
    else if (m_renderer->parent()->isRenderInline() && (startOfConts = startOfContinuations(m_renderer->parent())))
        previousSibling = childBeforeConsideringContinuations(startOfConts, m_renderer->parent()->firstChild());

    if (!previousSibling)
        return 0;
    
    return axObjectCache()->getOrCreate(previousSibling);
}

static inline bool lastChildHasContinuation(RenderObject* renderer)
{
    return renderer->lastChild() && isInlineWithContinuation(renderer->lastChild());
}

AccessibilityObject* AccessibilityRenderObject::nextSibling() const
{
    if (!m_renderer)
        return 0;

    RenderObject* nextSibling = 0;

    // Case 1: node is a block and has an inline continuation. Next sibling is the inline continuation's
    // first child.
    RenderInline* inlineContinuation;
    if (m_renderer->isRenderBlock() && (inlineContinuation = toRenderBlock(m_renderer)->inlineElementContinuation()))
        nextSibling = firstChildConsideringContinuation(inlineContinuation);

    // Case 2: Anonymous block parent of the start of a continuation - skip all the way to
    // after the parent of the end, since everything in between will be linked up via the continuation.
    else if (m_renderer->isAnonymousBlock() && lastChildHasContinuation(m_renderer)) {
        RenderObject* lastParent = endOfContinuations(m_renderer->lastChild())->parent();
        while (lastChildHasContinuation(lastParent))
            lastParent = endOfContinuations(lastParent->lastChild())->parent();
        nextSibling = lastParent->nextSibling();
    }

    // Case 3: node has an actual next sibling
    else if (RenderObject* ns = m_renderer->nextSibling())
        nextSibling = ns;

    // Case 4: node is an inline with a continuation. Next sibling is the next sibling of the end 
    // of the continuation chain.
    else if (isInlineWithContinuation(m_renderer))
        nextSibling = endOfContinuations(m_renderer)->nextSibling();

    // Case 5: node has no next sibling, and its parent is an inline with a continuation.
    else if (isInlineWithContinuation(m_renderer->parent())) {
        RenderObject* continuation = toRenderInline(m_renderer->parent())->continuation();
        
        // Case 5a: continuation is a block - in this case the block itself is the next sibling.
        if (continuation->isRenderBlock())
            nextSibling = continuation;
        // Case 5b: continuation is an inline - in this case the inline's first child is the next sibling
        else
            nextSibling = firstChildConsideringContinuation(continuation);
    }

    if (!nextSibling)
        return 0;
    
    return axObjectCache()->getOrCreate(nextSibling);
}

static RenderBoxModelObject* nextContinuation(RenderObject* renderer)
{
    ASSERT(renderer);
    if (renderer->isRenderInline() && !renderer->isReplaced())
        return toRenderInline(renderer)->continuation();
    if (renderer->isRenderBlock())
        return toRenderBlock(renderer)->inlineElementContinuation();
    return 0;
}
    
RenderObject* AccessibilityRenderObject::renderParentObject() const
{
    if (!m_renderer)
        return 0;

    RenderObject* parent = m_renderer->parent();

    // Case 1: node is a block and is an inline's continuation. Parent
    // is the start of the continuation chain.
    RenderObject* startOfConts = 0;
    RenderObject* firstChild = 0;
    if (m_renderer->isRenderBlock() && (startOfConts = startOfContinuations(m_renderer)))
        parent = startOfConts;

    // Case 2: node's parent is an inline which is some node's continuation; parent is 
    // the earliest node in the continuation chain.
    else if (parent && parent->isRenderInline() && (startOfConts = startOfContinuations(parent)))
        parent = startOfConts;
    
    // Case 3: The first sibling is the beginning of a continuation chain. Find the origin of that continuation.
    else if (parent && (firstChild = parent->firstChild()) && firstChild->node()) {
        // Get the node's renderer and follow that continuation chain until the first child is found
        RenderObject* nodeRenderFirstChild = firstChild->node()->renderer();
        if (nodeRenderFirstChild != firstChild) {
            for (RenderObject* contsTest = nodeRenderFirstChild; contsTest; contsTest = nextContinuation(contsTest)) {
                if (contsTest == firstChild) {
                    parent = nodeRenderFirstChild->parent();
                    break;
                }
            }
        }
    }
        
    return parent;
}
    
AccessibilityObject* AccessibilityRenderObject::parentObjectIfExists() const
{
    return axObjectCache()->get(renderParentObject());
}
    
AccessibilityObject* AccessibilityRenderObject::parentObject() const
{
    if (!m_renderer)
        return 0;
    
    if (ariaRoleAttribute() == MenuBarRole)
        return axObjectCache()->getOrCreate(m_renderer->parent());

    // menuButton and its corresponding menu are DOM siblings, but Accessibility needs them to be parent/child
    if (ariaRoleAttribute() == MenuRole) {
        AccessibilityObject* parent = menuButtonForMenu();
        if (parent)
            return parent;
    }
    
    RenderObject* parentObj = renderParentObject();
    if (parentObj)
        return axObjectCache()->getOrCreate(parentObj);
    
    // WebArea's parent should be the scroll view containing it.
    if (isWebArea())
        return axObjectCache()->getOrCreate(m_renderer->frame()->view());
    
    return 0;
}

bool AccessibilityRenderObject::isWebArea() const
{
    return roleValue() == WebAreaRole;
}

bool AccessibilityRenderObject::isImageButton() const
{
    return isNativeImage() && roleValue() == ButtonRole;
}

bool AccessibilityRenderObject::isAnchor() const
{
    return !isNativeImage() && isLink();
}

bool AccessibilityRenderObject::isNativeTextControl() const
{
    return m_renderer->isTextControl();
}
    
bool AccessibilityRenderObject::isNativeImage() const
{
    return m_renderer->isBoxModelObject() && toRenderBoxModelObject(m_renderer)->isImage();
}    
    
bool AccessibilityRenderObject::isImage() const
{
    return roleValue() == ImageRole;
}

bool AccessibilityRenderObject::isAttachment() const
{
    RenderBoxModelObject* renderer = renderBoxModelObject();
    if (!renderer)
        return false;
    // Widgets are the replaced elements that we represent to AX as attachments
    bool isWidget = renderer->isWidget();
    ASSERT(!isWidget || (renderer->isReplaced() && !isImage()));
    return isWidget && ariaRoleAttribute() == UnknownRole;
}

bool AccessibilityRenderObject::isPasswordField() const
{
    ASSERT(m_renderer);
    if (!m_renderer->node() || !m_renderer->node()->isHTMLElement())
        return false;
    if (ariaRoleAttribute() != UnknownRole)
        return false;

    InputElement* inputElement = m_renderer->node()->toInputElement();
    if (!inputElement)
        return false;

    return inputElement->isPasswordField();
}
    
bool AccessibilityRenderObject::isFileUploadButton() const
{
    if (m_renderer && m_renderer->node() && m_renderer->node()->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(m_renderer->node());
        return input->isFileUpload();
    }
    
    return false;
}
    
bool AccessibilityRenderObject::isInputImage() const
{
    Node* elementNode = node();
    if (roleValue() == ButtonRole && elementNode && elementNode->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(elementNode);
        return input->isImageButton();
    }
    
    return false;
}

bool AccessibilityRenderObject::isProgressIndicator() const
{
    return roleValue() == ProgressIndicatorRole;
}

bool AccessibilityRenderObject::isSlider() const
{
    return roleValue() == SliderRole;
}

bool AccessibilityRenderObject::isMenuRelated() const
{
    AccessibilityRole role = roleValue();
    return role == MenuRole 
        || role == MenuBarRole
        || role == MenuButtonRole
        || role == MenuItemRole;
}    

bool AccessibilityRenderObject::isMenu() const
{
    return roleValue() == MenuRole;
}

bool AccessibilityRenderObject::isMenuBar() const
{
    return roleValue() == MenuBarRole;
}

bool AccessibilityRenderObject::isMenuButton() const
{
    return roleValue() == MenuButtonRole;
}

bool AccessibilityRenderObject::isMenuItem() const
{
    return roleValue() == MenuItemRole;
}
     
bool AccessibilityRenderObject::isPressed() const
{
    ASSERT(m_renderer);
    if (roleValue() != ButtonRole)
        return false;

    Node* node = m_renderer->node();
    if (!node)
        return false;

    // If this is an ARIA button, check the aria-pressed attribute rather than node()->active()
    if (ariaRoleAttribute() == ButtonRole) {
        if (equalIgnoringCase(getAttribute(aria_pressedAttr), "true"))
            return true;
        return false;
    }

    return node->active();
}

bool AccessibilityRenderObject::isIndeterminate() const
{
    ASSERT(m_renderer);
    if (!m_renderer->node())
        return false;

    InputElement* inputElement = m_renderer->node()->toInputElement();
    if (!inputElement)
        return false;

    return inputElement->isIndeterminate();
}

bool AccessibilityRenderObject::isNativeCheckboxOrRadio() const
{
    Node* elementNode = node();
    if (elementNode) {
        InputElement* input = elementNode->toInputElement();
        if (input)
            return input->isCheckbox() || input->isRadioButton();
    }
    
    return false;
}
    
bool AccessibilityRenderObject::isChecked() const
{
    ASSERT(m_renderer);
    if (!m_renderer->node())
        return false;

    // First test for native checkedness semantics
    InputElement* inputElement = m_renderer->node()->toInputElement();
    if (inputElement)
        return inputElement->isChecked();

    // Else, if this is an ARIA checkbox or radio, respect the aria-checked attribute
    AccessibilityRole ariaRole = ariaRoleAttribute();
    if (ariaRole == RadioButtonRole || ariaRole == CheckBoxRole) {
        if (equalIgnoringCase(getAttribute(aria_checkedAttr), "true"))
            return true;
        return false;
    }

    // Otherwise it's not checked
    return false;
}

bool AccessibilityRenderObject::isHovered() const
{
    ASSERT(m_renderer);
    return m_renderer->node() && m_renderer->node()->hovered();
}

bool AccessibilityRenderObject::isMultiSelectable() const
{
    ASSERT(m_renderer);
    
    const AtomicString& ariaMultiSelectable = getAttribute(aria_multiselectableAttr);
    if (equalIgnoringCase(ariaMultiSelectable, "true"))
        return true;
    if (equalIgnoringCase(ariaMultiSelectable, "false"))
        return false;
    
    if (!m_renderer->isBoxModelObject() || !toRenderBoxModelObject(m_renderer)->isListBox())
        return false;
    return m_renderer->node() && static_cast<HTMLSelectElement*>(m_renderer->node())->multiple();
}
    
bool AccessibilityRenderObject::isReadOnly() const
{
    ASSERT(m_renderer);
    
    if (isWebArea()) {
        Document* document = m_renderer->document();
        if (!document)
            return true;
        
        HTMLElement* body = document->body();
        if (body && body->isContentEditable())
            return false;

        return !document->rendererIsEditable();
    }

    if (m_renderer->isBoxModelObject()) {
        RenderBoxModelObject* box = toRenderBoxModelObject(m_renderer);
        if (box->isTextField())
            return static_cast<HTMLInputElement*>(box->node())->readOnly();
        if (box->isTextArea())
            return static_cast<HTMLTextAreaElement*>(box->node())->readOnly();
    }

    return !m_renderer->node() || !m_renderer->node()->rendererIsEditable();
}

bool AccessibilityRenderObject::isOffScreen() const
{
    ASSERT(m_renderer);
    IntRect contentRect = m_renderer->absoluteClippedOverflowRect();
    FrameView* view = m_renderer->frame()->view();
    FloatRect viewRect = view->visibleContentRect();
    viewRect.intersect(contentRect);
    return viewRect.isEmpty();
}

int AccessibilityRenderObject::headingLevel() const
{
    // headings can be in block flow and non-block flow
    Node* element = node();
    if (!element)
        return 0;

    if (ariaRoleAttribute() == HeadingRole)
        return getAttribute(aria_levelAttr).toInt();

    if (element->hasTagName(h1Tag))
        return 1;
    
    if (element->hasTagName(h2Tag))
        return 2;
    
    if (element->hasTagName(h3Tag))
        return 3;
    
    if (element->hasTagName(h4Tag))
        return 4;
    
    if (element->hasTagName(h5Tag))
        return 5;
    
    if (element->hasTagName(h6Tag))
        return 6;
    
    return 0;
}

bool AccessibilityRenderObject::isHeading() const
{
    return roleValue() == HeadingRole;
}
    
bool AccessibilityRenderObject::isLink() const
{
    return roleValue() == WebCoreLinkRole;
}    
    
bool AccessibilityRenderObject::isControl() const
{
    if (!m_renderer)
        return false;
    
    Node* node = m_renderer->node();
    return node && ((node->isElementNode() && static_cast<Element*>(node)->isFormControlElement())
                    || AccessibilityObject::isARIAControl(ariaRoleAttribute()));
}

bool AccessibilityRenderObject::isFieldset() const
{
    RenderBoxModelObject* renderer = renderBoxModelObject();
    if (!renderer)
        return false;
    return renderer->isFieldset();
}
  
bool AccessibilityRenderObject::isGroup() const
{
    return roleValue() == GroupRole;
}
    
AccessibilityObject* AccessibilityRenderObject::selectedRadioButton()
{
    if (!isRadioGroup())
        return 0;
    
    // Find the child radio button that is selected (ie. the intValue == 1).
    int count = m_children.size();
    for (int i = 0; i < count; ++i) {
        AccessibilityObject* object = m_children[i].get();
        if (object->roleValue() == RadioButtonRole && object->checkboxOrRadioValue() == ButtonStateOn)
            return object;
    }
    return 0;
}

AccessibilityObject* AccessibilityRenderObject::selectedTabItem()
{
    if (!isTabList())
        return 0;
    
    // Find the child tab item that is selected (ie. the intValue == 1).
    AccessibilityObject::AccessibilityChildrenVector tabs;
    tabChildren(tabs);
    
    int count = tabs.size();
    for (int i = 0; i < count; ++i) {
        AccessibilityObject* object = m_children[i].get();
        if (object->isTabItem() && object->isChecked())
            return object;
    }
    return 0;
}

Element* AccessibilityRenderObject::anchorElement() const
{
    if (!m_renderer)
        return 0;
    
    AXObjectCache* cache = axObjectCache();
    RenderObject* currRenderer;
    
    // Search up the render tree for a RenderObject with a DOM node.  Defer to an earlier continuation, though.
    for (currRenderer = m_renderer; currRenderer && !currRenderer->node(); currRenderer = currRenderer->parent()) {
        if (currRenderer->isAnonymousBlock()) {
            RenderObject* continuation = toRenderBlock(currRenderer)->continuation();
            if (continuation)
                return cache->getOrCreate(continuation)->anchorElement();
        }
    }
    
    // bail if none found
    if (!currRenderer)
        return 0;
    
    // search up the DOM tree for an anchor element
    // NOTE: this assumes that any non-image with an anchor is an HTMLAnchorElement
    Node* node = currRenderer->node();
    for ( ; node; node = node->parentNode()) {
        if (node->hasTagName(aTag) || (node->renderer() && cache->getOrCreate(node->renderer())->isAnchor()))
            return static_cast<Element*>(node);
    }
    
    return 0;
}

Element* AccessibilityRenderObject::actionElement() const
{
    if (!m_renderer)
        return 0;
    
    Node* node = m_renderer->node();
    if (node) {
        if (node->hasTagName(inputTag)) {
            HTMLInputElement* input = static_cast<HTMLInputElement*>(node);
            if (!input->disabled() && (isCheckboxOrRadio() || input->isTextButton()))
                return input;
        } else if (node->hasTagName(buttonTag))
            return static_cast<Element*>(node);
    }

    if (isFileUploadButton())
        return static_cast<Element*>(m_renderer->node());
            
    if (AccessibilityObject::isARIAInput(ariaRoleAttribute()))
        return static_cast<Element*>(m_renderer->node());

    if (isImageButton())
        return static_cast<Element*>(m_renderer->node());
    
    if (m_renderer->isBoxModelObject() && toRenderBoxModelObject(m_renderer)->isMenuList())
        return static_cast<Element*>(m_renderer->node());

    AccessibilityRole role = roleValue();
    if (role == ButtonRole || role == PopUpButtonRole)
        return static_cast<Element*>(m_renderer->node()); 
    
    Element* elt = anchorElement();
    if (!elt)
        elt = mouseButtonListener();
    return elt;
}

Element* AccessibilityRenderObject::mouseButtonListener() const
{
    Node* node = m_renderer->node();
    if (!node)
        return 0;
    
    // check if our parent is a mouse button listener
    while (node && !node->isElementNode())
        node = node->parentNode();

    if (!node)
        return 0;

    // FIXME: Do the continuation search like anchorElement does
    for (Element* element = static_cast<Element*>(node); element; element = element->parentElement()) {
        if (element->getAttributeEventListener(eventNames().clickEvent) || element->getAttributeEventListener(eventNames().mousedownEvent) || element->getAttributeEventListener(eventNames().mouseupEvent))
            return element;
    }

    return 0;
}

void AccessibilityRenderObject::increment()
{
    if (roleValue() != SliderRole)
        return;
    
    changeValueByPercent(5);
}

void AccessibilityRenderObject::decrement()
{
    if (roleValue() != SliderRole)
        return;
    
    changeValueByPercent(-5);
}

static Element* siblingWithAriaRole(String role, Node* node)
{
    Node* sibling = node->parentNode()->firstChild();
    while (sibling) {
        if (sibling->isElementNode()) {
            const AtomicString& siblingAriaRole = static_cast<Element*>(sibling)->getAttribute(roleAttr);
            if (equalIgnoringCase(siblingAriaRole, role))
                return static_cast<Element*>(sibling);
        }
        sibling = sibling->nextSibling();
    }
    
    return 0;
}

Element* AccessibilityRenderObject::menuElementForMenuButton() const
{
    if (ariaRoleAttribute() != MenuButtonRole)
        return 0;

    return siblingWithAriaRole("menu", renderer()->node());
}

AccessibilityObject* AccessibilityRenderObject::menuForMenuButton() const
{
    Element* menu = menuElementForMenuButton();
    if (menu && menu->renderer())
        return axObjectCache()->getOrCreate(menu->renderer());
    return 0;
}

Element* AccessibilityRenderObject::menuItemElementForMenu() const
{
    if (ariaRoleAttribute() != MenuRole)
        return 0;
    
    return siblingWithAriaRole("menuitem", renderer()->node());    
}

AccessibilityObject* AccessibilityRenderObject::menuButtonForMenu() const
{
    Element* menuItem = menuItemElementForMenu();

    if (menuItem && menuItem->renderer()) {
        // ARIA just has generic menu items.  AppKit needs to know if this is a top level items like MenuBarButton or MenuBarItem
        AccessibilityObject* menuItemAX = axObjectCache()->getOrCreate(menuItem->renderer());
        if (menuItemAX->isMenuButton())
            return menuItemAX;
    }
    return 0;
}

String AccessibilityRenderObject::helpText() const
{
    if (!m_renderer)
        return String();
    
    const AtomicString& ariaHelp = getAttribute(aria_helpAttr);
    if (!ariaHelp.isEmpty())
        return ariaHelp;
    
    for (RenderObject* curr = m_renderer; curr; curr = curr->parent()) {
        if (curr->node() && curr->node()->isHTMLElement()) {
            const AtomicString& summary = static_cast<Element*>(curr->node())->getAttribute(summaryAttr);
            if (!summary.isEmpty())
                return summary;
            const AtomicString& title = static_cast<Element*>(curr->node())->getAttribute(titleAttr);
            if (!title.isEmpty())
                return title;
        }
        
        // Only take help text from an ancestor element if its a group or an unknown role. If help was 
        // added to those kinds of elements, it is likely it was meant for a child element.
        AccessibilityObject* axObj = axObjectCache()->getOrCreate(curr);
        if (axObj) {
            AccessibilityRole role = axObj->roleValue();
            if (role != GroupRole && role != UnknownRole)
                break;
        }
    }
    
    return String();
}
    
unsigned AccessibilityRenderObject::hierarchicalLevel() const
{
    if (!m_renderer)
        return 0;

    Node* node = m_renderer->node();
    if (!node || !node->isElementNode())
        return 0;
    Element* element = static_cast<Element*>(node);
    String ariaLevel = element->getAttribute(aria_levelAttr);
    if (!ariaLevel.isEmpty())
        return ariaLevel.toInt();
    
    // Only tree item will calculate its level through the DOM currently.
    if (roleValue() != TreeItemRole)
        return 0;
    
    // Hierarchy leveling starts at 0.
    // We measure tree hierarchy by the number of groups that the item is within.
    unsigned level = 0;
    AccessibilityObject* parent = parentObject();
    while (parent) {
        AccessibilityRole parentRole = parent->roleValue();
        if (parentRole == GroupRole)
            level++;
        else if (parentRole == TreeRole)
            break;
        
        parent = parent->parentObject();
    }
    
    return level;
}

static TextIteratorBehavior textIteratorBehaviorForTextRange()
{
    TextIteratorBehavior behavior = TextIteratorIgnoresStyleVisibility;

#if PLATFORM(GTK)
    // We need to emit replaced elements for GTK, and present
    // them with the 'object replacement character' (0xFFFC).
    behavior = static_cast<TextIteratorBehavior>(behavior | TextIteratorEmitsObjectReplacementCharacters);
#endif

    return behavior;
}

String AccessibilityRenderObject::textUnderElement() const
{
    if (!m_renderer)
        return String();
    
    if (isFileUploadButton())
        return toRenderFileUploadControl(m_renderer)->buttonValue();
    
    Node* node = m_renderer->node();
    if (node) {
        if (Frame* frame = node->document()->frame()) {
            // catch stale WebCoreAXObject (see <rdar://problem/3960196>)
            if (frame->document() != node->document())
                return String();

            return plainText(rangeOfContents(node).get(), textIteratorBehaviorForTextRange());
        }
    }
    
    // Sometimes text fragments don't have Node's associated with them (like when
    // CSS content is used to insert text).
    if (m_renderer->isText()) {
        RenderText* renderTextObject = toRenderText(m_renderer);
        if (renderTextObject->isTextFragment())
            return String(static_cast<RenderTextFragment*>(m_renderer)->contentString());
    }
    
    // return the null string for anonymous text because it is non-trivial to get
    // the actual text and, so far, that is not needed
    return String();
}

Node* AccessibilityRenderObject::node() const
{ 
    return m_renderer ? m_renderer->node() : 0; 
}    
    
AccessibilityButtonState AccessibilityRenderObject::checkboxOrRadioValue() const
{
    if (isNativeCheckboxOrRadio())
        return isChecked() ? ButtonStateOn : ButtonStateOff;
    
    return AccessibilityObject::checkboxOrRadioValue();
}

String AccessibilityRenderObject::valueDescription() const
{
    // Only sliders and progress bars support value descriptions currently.
    if (!isProgressIndicator() && !isSlider())
        return String();
    
    return getAttribute(aria_valuetextAttr).string();
}
    
float AccessibilityRenderObject::valueForRange() const
{
    if (!isProgressIndicator() && !isSlider() && !isScrollbar())
        return 0.0f;

    return getAttribute(aria_valuenowAttr).toFloat();
}

float AccessibilityRenderObject::maxValueForRange() const
{
    if (!isProgressIndicator() && !isSlider())
        return 0.0f;

    return getAttribute(aria_valuemaxAttr).toFloat();
}

float AccessibilityRenderObject::minValueForRange() const
{
    if (!isProgressIndicator() && !isSlider())
        return 0.0f;

    return getAttribute(aria_valueminAttr).toFloat();
}

String AccessibilityRenderObject::stringValue() const
{
    if (!m_renderer || isPasswordField())
        return String();

    RenderBoxModelObject* cssBox = renderBoxModelObject();

    if (ariaRoleAttribute() == StaticTextRole) {
        String staticText = text();
        if (!staticText.length())
            staticText = textUnderElement();
        return staticText;
    }
        
    if (m_renderer->isText())
        return textUnderElement();
    
    if (cssBox && cssBox->isMenuList()) {
        // RenderMenuList will go straight to the text() of its selected item.
        // This has to be overriden in the case where the selected item has an aria label
        SelectElement* selectNode = toSelectElement(static_cast<Element*>(m_renderer->node()));
        int selectedIndex = selectNode->selectedIndex();
        const Vector<Element*> listItems = selectNode->listItems();
        
        Element* selectedOption = 0;
        if (selectedIndex >= 0 && selectedIndex < (int)listItems.size()) 
            selectedOption = listItems[selectedIndex];
        if (selectedOption) {
            String overridenDescription = selectedOption->getAttribute(aria_labelAttr);
            if (!overridenDescription.isNull())
                return overridenDescription;
        }
        
        return toRenderMenuList(m_renderer)->text();
    }
    
    if (m_renderer->isListMarker())
        return toRenderListMarker(m_renderer)->text();
    
    if (cssBox && cssBox->isRenderButton())
        return toRenderButton(m_renderer)->text();

    if (isWebArea()) {
        if (m_renderer->frame())
            return String();
        
        // FIXME: should use startOfDocument and endOfDocument (or rangeForDocument?) here
        VisiblePosition startVisiblePosition = m_renderer->positionForCoordinates(0, 0);
        VisiblePosition endVisiblePosition = m_renderer->positionForCoordinates(INT_MAX, INT_MAX);
        if (startVisiblePosition.isNull() || endVisiblePosition.isNull())
            return String();

        return plainText(makeRange(startVisiblePosition, endVisiblePosition).get(),
                         textIteratorBehaviorForTextRange());
    }
    
    if (isTextControl())
        return text();
    
    if (isFileUploadButton())
        return toRenderFileUploadControl(m_renderer)->fileTextValue();
    
    // FIXME: We might need to implement a value here for more types
    // FIXME: It would be better not to advertise a value at all for the types for which we don't implement one;
    // this would require subclassing or making accessibilityAttributeNames do something other than return a
    // single static array.
    return String();
}

// This function implements the ARIA accessible name as described by the Mozilla
// ARIA Implementer's Guide.
static String accessibleNameForNode(Node* node)
{
    if (node->isTextNode())
        return static_cast<Text*>(node)->data();

    if (node->hasTagName(inputTag))
        return static_cast<HTMLInputElement*>(node)->value();

    if (node->isHTMLElement()) {
        const AtomicString& alt = toHTMLElement(node)->getAttribute(altAttr);
        if (!alt.isEmpty())
            return alt;
    }

    return String();
}

String AccessibilityRenderObject::accessibilityDescriptionForElements(Vector<Element*> &elements) const
{
    StringBuilder builder;
    unsigned size = elements.size();
    for (unsigned i = 0; i < size; ++i) {
        Element* idElement = elements[i];

        builder.append(accessibleNameForNode(idElement));
        for (Node* n = idElement->firstChild(); n; n = n->traverseNextNode(idElement))
            builder.append(accessibleNameForNode(n));

        if (i != size - 1)
            builder.append(' ');
    }
    return builder.toString();
}

void AccessibilityRenderObject::elementsFromAttribute(Vector<Element*>& elements, const QualifiedName& attribute) const
{
    Node* node = m_renderer->node();
    if (!node || !node->isElementNode())
        return;

    TreeScope* scope = node->treeScope();
    if (!scope)
        return;
    
    String idList = getAttribute(attribute).string();
    if (idList.isEmpty())
        return;
    
    idList.replace('\n', ' ');
    Vector<String> idVector;
    idList.split(' ', idVector);
    
    unsigned size = idVector.size();
    for (unsigned i = 0; i < size; ++i) {
        String idName = idVector[i];
        Element* idElement = scope->getElementById(idName);
        if (idElement)
            elements.append(idElement);
    }
}
    
void AccessibilityRenderObject::ariaLabeledByElements(Vector<Element*>& elements) const
{
    elementsFromAttribute(elements, aria_labeledbyAttr);
    if (!elements.size())
        elementsFromAttribute(elements, aria_labelledbyAttr);
}
   
String AccessibilityRenderObject::ariaLabeledByAttribute() const
{
    Vector<Element*> elements;
    ariaLabeledByElements(elements);
    
    return accessibilityDescriptionForElements(elements);
}

static HTMLLabelElement* labelForElement(Element* element)
{
    RefPtr<NodeList> list = element->document()->getElementsByTagName("label");
    unsigned len = list->length();
    for (unsigned i = 0; i < len; i++) {
        if (list->item(i)->hasTagName(labelTag)) {
            HTMLLabelElement* label = static_cast<HTMLLabelElement*>(list->item(i));
            if (label->control() == element)
                return label;
        }
    }
    
    return 0;
}
    
HTMLLabelElement* AccessibilityRenderObject::labelElementContainer() const
{
    if (!m_renderer)
        return 0;

    // the control element should not be considered part of the label
    if (isControl())
        return 0;
    
    // find if this has a parent that is a label
    for (Node* parentNode = m_renderer->node(); parentNode; parentNode = parentNode->parentNode()) {
        if (parentNode->hasTagName(labelTag))
            return static_cast<HTMLLabelElement*>(parentNode);
    }
    
    return 0;
}

String AccessibilityRenderObject::title() const
{
    AccessibilityRole ariaRole = ariaRoleAttribute();
    
    if (!m_renderer)
        return String();

    Node* node = m_renderer->node();
    if (!node)
        return String();
    
    String ariaLabel = ariaLabeledByAttribute();
    if (!ariaLabel.isEmpty())
        return ariaLabel;
    
    const AtomicString& title = getAttribute(titleAttr);
    if (!title.isEmpty())
        return title;
    
    bool isInputTag = node->hasTagName(inputTag);
    if (isInputTag) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(node);
        if (input->isTextButton())
            return input->value();
    }
    
    if (isInputTag || AccessibilityObject::isARIAInput(ariaRole) || isControl()) {
        HTMLLabelElement* label = labelForElement(static_cast<Element*>(node));
        if (label && !titleUIElement())
            return label->innerText();
    }
    
    if (roleValue() == ButtonRole
        || ariaRole == ListBoxOptionRole
        || ariaRole == MenuItemRole
        || ariaRole == MenuButtonRole
        || ariaRole == RadioButtonRole
        || ariaRole == CheckBoxRole
        || ariaRole == TabRole
        || ariaRole == PopUpButtonRole
        || isHeading()
        || isLink())
        return textUnderElement();
    
    return String();
}

String AccessibilityRenderObject::ariaDescribedByAttribute() const
{
    Vector<Element*> elements;
    elementsFromAttribute(elements, aria_describedbyAttr);
    
    return accessibilityDescriptionForElements(elements);
}
    
String AccessibilityRenderObject::ariaAccessibilityDescription() const
{
    const AtomicString& ariaLabel = getAttribute(aria_labelAttr);
    if (!ariaLabel.isEmpty())
        return ariaLabel;
    
    String ariaDescription = ariaDescribedByAttribute();
    if (!ariaDescription.isEmpty())
        return ariaDescription;
    
    return String();
}

String AccessibilityRenderObject::accessibilityDescription() const
{
    if (!m_renderer)
        return String();

    // Static text should not have a description, it should only have a stringValue.
    if (roleValue() == StaticTextRole)
        return String();
    
    String ariaDescription = ariaAccessibilityDescription();
    if (!ariaDescription.isEmpty())
        return ariaDescription;
    
    Node* node = m_renderer->node();
    if (isImage() || isInputImage() || isNativeImage()) {
        if (node && node->isHTMLElement()) {
            const AtomicString& alt = toHTMLElement(node)->getAttribute(altAttr);
            if (alt.isEmpty())
                return String();
            return alt;
        }
    }
    
#if ENABLE(MATHML)
    if (node && node->isElementNode() && static_cast<Element*>(node)->isMathMLElement())
        return getAttribute(MathMLNames::alttextAttr);
#endif
    
    if (isWebArea()) {
        Document* document = m_renderer->document();
        
        // Check if the HTML element has an aria-label for the webpage.
        Element* documentElement = document->documentElement();
        if (documentElement) {
            const AtomicString& ariaLabel = documentElement->getAttribute(aria_labelAttr);
            if (!ariaLabel.isEmpty())
                return ariaLabel;
        }
        
        Node* owner = document->ownerElement();
        if (owner) {
            if (owner->hasTagName(frameTag) || owner->hasTagName(iframeTag)) {
                const AtomicString& title = static_cast<HTMLFrameElementBase*>(owner)->getAttribute(titleAttr);
                if (!title.isEmpty())
                    return title;
                return static_cast<HTMLFrameElementBase*>(owner)->getAttribute(nameAttr);
            }
            if (owner->isHTMLElement())
                return toHTMLElement(owner)->getAttribute(nameAttr);
        }
        owner = document->body();
        if (owner && owner->isHTMLElement())
            return toHTMLElement(owner)->getAttribute(nameAttr);
    }

    return String();
}

IntRect AccessibilityRenderObject::boundingBoxRect() const
{
    RenderObject* obj = m_renderer;
    
    if (!obj)
        return IntRect();
    
    if (obj->node()) // If we are a continuation, we want to make sure to use the primary renderer.
        obj = obj->node()->renderer();
    
    // absoluteFocusRingQuads will query the hierarchy below this element, which for large webpages can be very slow.
    // For a web area, which will have the most elements of any element, absoluteQuads should be used.
    Vector<FloatQuad> quads;
    if (obj->isText())
        toRenderText(obj)->absoluteQuads(quads, RenderText::ClipToEllipsis);
    else if (isWebArea())
        obj->absoluteQuads(quads);
    else
        obj->absoluteFocusRingQuads(quads);
    const size_t n = quads.size();
    if (!n)
        return IntRect();

    IntRect result;
    for (size_t i = 0; i < n; ++i) {
        IntRect r = quads[i].enclosingBoundingBox();
        if (!r.isEmpty()) {
            if (obj->style()->hasAppearance())
                obj->theme()->adjustRepaintRect(obj, r);
            result.unite(r);
        }
    }

    // The size of the web area should be the content size, not the clipped size.
    if (isWebArea() && obj->frame()->view())
        result.setSize(obj->frame()->view()->contentsSize());
    
    return result;
}
    
IntRect AccessibilityRenderObject::checkboxOrRadioRect() const
{
    if (!m_renderer)
        return IntRect();
    
    HTMLLabelElement* label = labelForElement(static_cast<Element*>(m_renderer->node()));
    if (!label || !label->renderer())
        return boundingBoxRect();
    
    IntRect labelRect = axObjectCache()->getOrCreate(label->renderer())->elementRect();
    labelRect.unite(boundingBoxRect());
    return labelRect;
}

IntRect AccessibilityRenderObject::elementRect() const
{
    // a checkbox or radio button should encompass its label
    if (isCheckboxOrRadio())
        return checkboxOrRadioRect();
    
    return boundingBoxRect();
}

IntSize AccessibilityRenderObject::size() const
{
    IntRect rect = elementRect();
    return rect.size();
}

IntPoint AccessibilityRenderObject::clickPoint() const
{
    // use the default position unless this is an editable web area, in which case we use the selection bounds.
    if (!isWebArea() || isReadOnly())
        return AccessibilityObject::clickPoint();
    
    VisibleSelection visSelection = selection();
    VisiblePositionRange range = VisiblePositionRange(visSelection.visibleStart(), visSelection.visibleEnd());
    IntRect bounds = boundsForVisiblePositionRange(range);
#if PLATFORM(MAC)
    bounds.setLocation(m_renderer->document()->view()->screenToContents(bounds.location()));
#endif        
    return IntPoint(bounds.x() + (bounds.width() / 2), bounds.y() - (bounds.height() / 2));
}
    
AccessibilityObject* AccessibilityRenderObject::internalLinkElement() const
{
    Element* element = anchorElement();
    if (!element)
        return 0;
    
    // Right now, we do not support ARIA links as internal link elements
    if (!element->hasTagName(aTag))
        return 0;
    HTMLAnchorElement* anchor = static_cast<HTMLAnchorElement*>(element);
    
    KURL linkURL = anchor->href();
    String fragmentIdentifier = linkURL.fragmentIdentifier();
    if (fragmentIdentifier.isEmpty())
        return 0;
    
    // check if URL is the same as current URL
    KURL documentURL = m_renderer->document()->url();
    if (!equalIgnoringFragmentIdentifier(documentURL, linkURL))
        return 0;
    
    Node* linkedNode = m_renderer->document()->findAnchor(fragmentIdentifier);
    if (!linkedNode)
        return 0;
    
    // The element we find may not be accessible, so find the first accessible object.
    return firstAccessibleObjectFromNode(linkedNode);
}

ESpeak AccessibilityRenderObject::speakProperty() const
{
    if (!m_renderer)
        return AccessibilityObject::speakProperty();
    
    return m_renderer->style()->speak();
}
    
void AccessibilityRenderObject::addRadioButtonGroupMembers(AccessibilityChildrenVector& linkedUIElements) const
{
    if (!m_renderer || roleValue() != RadioButtonRole)
        return;
    
    Node* node = m_renderer->node();
    if (!node || !node->hasTagName(inputTag))
        return;
    
    HTMLInputElement* input = static_cast<HTMLInputElement*>(node);
    // if there's a form, then this is easy
    if (input->form()) {
        Vector<RefPtr<Node> > formElements;
        input->form()->getNamedElements(input->name(), formElements);
        
        unsigned len = formElements.size();
        for (unsigned i = 0; i < len; ++i) {
            Node* associateElement = formElements[i].get();
            if (AccessibilityObject* object = axObjectCache()->getOrCreate(associateElement->renderer()))
                linkedUIElements.append(object);        
        } 
    } else {
        RefPtr<NodeList> list = node->document()->getElementsByTagName("input");
        unsigned len = list->length();
        for (unsigned i = 0; i < len; ++i) {
            if (list->item(i)->hasTagName(inputTag)) {
                HTMLInputElement* associateElement = static_cast<HTMLInputElement*>(list->item(i));
                if (associateElement->isRadioButton() && associateElement->name() == input->name()) {
                    if (AccessibilityObject* object = axObjectCache()->getOrCreate(associateElement->renderer()))
                        linkedUIElements.append(object);
                }
            }
        }
    }
}
    
// linked ui elements could be all the related radio buttons in a group
// or an internal anchor connection
void AccessibilityRenderObject::linkedUIElements(AccessibilityChildrenVector& linkedUIElements) const
{
    ariaFlowToElements(linkedUIElements);

    if (isAnchor()) {
        AccessibilityObject* linkedAXElement = internalLinkElement();
        if (linkedAXElement)
            linkedUIElements.append(linkedAXElement);
    }

    if (roleValue() == RadioButtonRole)
        addRadioButtonGroupMembers(linkedUIElements);
}

bool AccessibilityRenderObject::hasTextAlternative() const
{
    // ARIA: section 2A, bullet #3 says if aria-labeledby or aria-label appears, it should
    // override the "label" element association.
    if (!ariaLabeledByAttribute().isEmpty() || !getAttribute(aria_labelAttr).isEmpty())
        return true;
        
    return false;   
}
    
bool AccessibilityRenderObject::ariaHasPopup() const
{
    return elementAttributeValue(aria_haspopupAttr);
}
    
bool AccessibilityRenderObject::supportsARIAFlowTo() const
{
    return !getAttribute(aria_flowtoAttr).isEmpty();
}
    
void AccessibilityRenderObject::ariaFlowToElements(AccessibilityChildrenVector& flowTo) const
{
    Vector<Element*> elements;
    elementsFromAttribute(elements, aria_flowtoAttr);
    
    AXObjectCache* cache = axObjectCache();
    unsigned count = elements.size();
    for (unsigned k = 0; k < count; ++k) {
        Element* element = elements[k];
        AccessibilityObject* flowToElement = cache->getOrCreate(element->renderer());
        if (flowToElement)
            flowTo.append(flowToElement);
    }
        
}
    
bool AccessibilityRenderObject::supportsARIADropping() const 
{
    const AtomicString& dropEffect = getAttribute(aria_dropeffectAttr);
    return !dropEffect.isEmpty();
}

bool AccessibilityRenderObject::supportsARIADragging() const
{
    const AtomicString& grabbed = getAttribute(aria_grabbedAttr);
    return equalIgnoringCase(grabbed, "true") || equalIgnoringCase(grabbed, "false");   
}

bool AccessibilityRenderObject::isARIAGrabbed()
{
    return elementAttributeValue(aria_grabbedAttr);
}

void AccessibilityRenderObject::determineARIADropEffects(Vector<String>& effects)
{
    const AtomicString& dropEffects = getAttribute(aria_dropeffectAttr);
    if (dropEffects.isEmpty()) {
        effects.clear();
        return;
    }
    
    String dropEffectsString = dropEffects.string();
    dropEffectsString.replace('\n', ' ');
    dropEffectsString.split(' ', effects);
}
    
bool AccessibilityRenderObject::exposesTitleUIElement() const
{
    if (!isControl())
        return false;

    // checkbox or radio buttons don't expose the title ui element unless it has a title already
    if (isCheckboxOrRadio() && getAttribute(titleAttr).isEmpty())
        return false;
    
    if (hasTextAlternative())
        return false;
    
    return true;
}
    
AccessibilityObject* AccessibilityRenderObject::titleUIElement() const
{
    if (!m_renderer)
        return 0;
    
    // if isFieldset is true, the renderer is guaranteed to be a RenderFieldset
    if (isFieldset())
        return axObjectCache()->getOrCreate(toRenderFieldset(m_renderer)->findLegend());
    
    if (!exposesTitleUIElement())
        return 0;
    
    Node* element = m_renderer->node();
    HTMLLabelElement* label = labelForElement(static_cast<Element*>(element));
    if (label && label->renderer())
        return axObjectCache()->getOrCreate(label->renderer());

    return 0;   
}
    
bool AccessibilityRenderObject::ariaIsHidden() const
{
    if (equalIgnoringCase(getAttribute(aria_hiddenAttr), "true"))
        return true;
    
    // aria-hidden hides this object and any children
    AccessibilityObject* object = parentObject();
    while (object) {
        if (object->isAccessibilityRenderObject() && equalIgnoringCase(static_cast<AccessibilityRenderObject*>(object)->getAttribute(aria_hiddenAttr), "true"))
            return true;
        object = object->parentObject();
    }

    return false;
}

bool AccessibilityRenderObject::isDescendantOfBarrenParent() const
{
    for (AccessibilityObject* object = parentObject(); object; object = object->parentObject()) {
        if (!object->canHaveChildren())
            return true;
    }
    
    return false;
}
    
bool AccessibilityRenderObject::isAllowedChildOfTree() const
{
    // Determine if this is in a tree. If so, we apply special behavior to make it work like an AXOutline.
    AccessibilityObject* axObj = parentObject();
    bool isInTree = false;
    while (axObj) {
        if (axObj->isTree()) {
            isInTree = true;
            break;
        }
        axObj = axObj->parentObject();
    }
    
    // If the object is in a tree, only tree items should be exposed (and the children of tree items).
    if (isInTree) {
        AccessibilityRole role = roleValue();
        if (role != TreeItemRole && role != StaticTextRole)
            return false;
    }
    return true;
}
    
AccessibilityObjectInclusion AccessibilityRenderObject::accessibilityIsIgnoredBase() const
{
    // The following cases can apply to any element that's a subclass of AccessibilityRenderObject.
    
    // Ignore invisible elements.
    if (!m_renderer || m_renderer->style()->visibility() != VISIBLE)
        return IgnoreObject;

    // Anything marked as aria-hidden or a child of something aria-hidden must be hidden.
    if (ariaIsHidden())
        return IgnoreObject;
    
    // Anything that is a presentational role must be hidden.
    if (isPresentationalChildOfAriaRole())
        return IgnoreObject;

    // Allow the platform to make a decision.
    AccessibilityObjectInclusion decision = accessibilityPlatformIncludesObject();
    if (decision == IncludeObject)
        return IncludeObject;
    if (decision == IgnoreObject)
        return IgnoreObject;
        
    return DefaultBehavior;
}  
 
bool AccessibilityRenderObject::accessibilityIsIgnored() const
{
    // Check first if any of the common reasons cause this element to be ignored.
    // Then process other use cases that need to be applied to all the various roles
    // that AccessibilityRenderObjects take on.
    AccessibilityObjectInclusion decision = accessibilityIsIgnoredBase();
    if (decision == IncludeObject)
        return false;
    if (decision == IgnoreObject)
        return true;
    
    // If this element is within a parent that cannot have children, it should not be exposed.
    if (isDescendantOfBarrenParent())
        return true;    
    
    if (roleValue() == IgnoredRole)
        return true;
    
    if (roleValue() == PresentationalRole || inheritsPresentationalRole())
        return true;
    
    // An ARIA tree can only have tree items and static text as children.
    if (!isAllowedChildOfTree())
        return true;

    // Allow the platform to decide if the attachment is ignored or not.
    if (isAttachment())
        return accessibilityIgnoreAttachment();
    
    // ignore popup menu items because AppKit does
    for (RenderObject* parent = m_renderer->parent(); parent; parent = parent->parent()) {
        if (parent->isBoxModelObject() && toRenderBoxModelObject(parent)->isMenuList())
            return true;
    }

    // find out if this element is inside of a label element.
    // if so, it may be ignored because it's the label for a checkbox or radio button
    AccessibilityObject* controlObject = correspondingControlForLabelElement();
    if (controlObject && !controlObject->exposesTitleUIElement() && controlObject->isCheckboxOrRadio())
        return true;
        
    // NOTE: BRs always have text boxes now, so the text box check here can be removed
    if (m_renderer->isText()) {
        // static text beneath MenuItems and MenuButtons are just reported along with the menu item, so it's ignored on an individual level
        if (parentObjectUnignored()->ariaRoleAttribute() == MenuItemRole
            || parentObjectUnignored()->ariaRoleAttribute() == MenuButtonRole)
            return true;
        RenderText* renderText = toRenderText(m_renderer);
        if (m_renderer->isBR() || !renderText->firstTextBox())
            return true;

        // static text beneath TextControls is reported along with the text control text so it's ignored.
        for (AccessibilityObject* parent = parentObject(); parent; parent = parent->parentObject()) { 
            if (parent->roleValue() == TextFieldRole)
                return true;
        }

        // text elements that are just empty whitespace should not be returned
        return renderText->text()->containsOnlyWhitespace();
    }
    
    if (isHeading())
        return false;
    
    if (isLink())
        return false;
    
    // all controls are accessible
    if (isControl())
        return false;
    
    if (ariaRoleAttribute() != UnknownRole)
        return false;

    // don't ignore labels, because they serve as TitleUIElements
    Node* node = m_renderer->node();
    if (node && node->hasTagName(labelTag))
        return false;
    
    // Anything that is content editable should not be ignored.
    // However, one cannot just call node->rendererIsEditable() since that will ask if its parents
    // are also editable. Only the top level content editable region should be exposed.
    if (node && node->isElementNode()) {
        Element* element = static_cast<Element*>(node);
        const AtomicString& contentEditable = element->getAttribute(contenteditableAttr);
        if (equalIgnoringCase(contentEditable, "true"))
            return false;
    }
    
    // List items play an important role in defining the structure of lists. They should not be ignored.
    if (roleValue() == ListItemRole)
        return false;
    
    // if this element has aria attributes on it, it should not be ignored.
    if (supportsARIAAttributes())
        return false;
    
    if (m_renderer->isBlockFlow() && m_renderer->childrenInline())
        return !toRenderBlock(m_renderer)->firstLineBox() && !mouseButtonListener();
    
    // ignore images seemingly used as spacers
    if (isImage()) {
        if (node && node->isElementNode()) {
            Element* elt = static_cast<Element*>(node);
            const AtomicString& alt = elt->getAttribute(altAttr);
            // don't ignore an image that has an alt tag
            if (!alt.isEmpty())
                return false;
            // informal standard is to ignore images with zero-length alt strings
            if (!alt.isNull())
                return true;
        }
        
        if (node && node->hasTagName(canvasTag)) {
            RenderHTMLCanvas* canvas = toRenderHTMLCanvas(m_renderer);
            if (canvas->height() <= 1 || canvas->width() <= 1)
                return true;
            return false;
        }
        
        if (isNativeImage()) {
            // check for one-dimensional image
            RenderImage* image = toRenderImage(m_renderer);
            if (image->height() <= 1 || image->width() <= 1)
                return true;
            
            // check whether rendered image was stretched from one-dimensional file image
            if (image->cachedImage()) {
                IntSize imageSize = image->cachedImage()->imageSize(image->view()->zoomFactor());
                return imageSize.height() <= 1 || imageSize.width() <= 1;
            }
        }
        return false;
    }
    
    if (isWebArea() || m_renderer->isListMarker())
        return false;
    
    // Using the help text to decide an element's visibility is not as definitive
    // as previous checks, so this should remain as one of the last.
    if (!helpText().isEmpty())
        return false;
    
    // By default, objects should be ignored so that the AX hierarchy is not 
    // filled with unnecessary items.
    return true;
}

bool AccessibilityRenderObject::isLoaded() const
{
    return !m_renderer->document()->parser();
}

double AccessibilityRenderObject::estimatedLoadingProgress() const
{
    if (!m_renderer)
        return 0;
    
    if (isLoaded())
        return 1.0;
    
    Page* page = m_renderer->document()->page();
    if (!page)
        return 0;
    
    return page->progress()->estimatedProgress();
}
    
int AccessibilityRenderObject::layoutCount() const
{
    if (!m_renderer->isRenderView())
        return 0;
    return toRenderView(m_renderer)->frameView()->layoutCount();
}

String AccessibilityRenderObject::text() const
{
    // If this is a user defined static text, use the accessible name computation.
    if (ariaRoleAttribute() == StaticTextRole)
        return ariaAccessibilityDescription();
    
    if (!isTextControl() || isPasswordField())
        return String();
    
    if (isNativeTextControl())
        return toRenderTextControl(m_renderer)->text();
    
    Node* node = m_renderer->node();
    if (!node)
        return String();
    if (!node->isElementNode())
        return String();
    
    return static_cast<Element*>(node)->innerText();
}
    
int AccessibilityRenderObject::textLength() const
{
    ASSERT(isTextControl());
    
    if (isPasswordField())
        return -1; // need to return something distinct from 0
    
    return text().length();
}

PlainTextRange AccessibilityRenderObject::ariaSelectedTextRange() const
{
    Node* node = m_renderer->node();
    if (!node)
        return PlainTextRange();
    
    ExceptionCode ec = 0;
    VisibleSelection visibleSelection = selection();
    RefPtr<Range> currentSelectionRange = visibleSelection.toNormalizedRange();
    if (!currentSelectionRange || !currentSelectionRange->intersectsNode(node, ec))
        return PlainTextRange();
    
    int start = indexForVisiblePosition(visibleSelection.start());
    int end = indexForVisiblePosition(visibleSelection.end());
    
    return PlainTextRange(start, end - start);
}

String AccessibilityRenderObject::selectedText() const
{
    ASSERT(isTextControl());
    
    if (isPasswordField())
        return String(); // need to return something distinct from empty string
    
    if (isNativeTextControl()) {
        RenderTextControl* textControl = toRenderTextControl(m_renderer);
        return textControl->text().substring(textControl->selectionStart(), textControl->selectionEnd() - textControl->selectionStart());
    }
    
    if (ariaRoleAttribute() == UnknownRole)
        return String();
    
    return doAXStringForRange(ariaSelectedTextRange());
}

const AtomicString& AccessibilityRenderObject::accessKey() const
{
    Node* node = m_renderer->node();
    if (!node)
        return nullAtom;
    if (!node->isElementNode())
        return nullAtom;
    return static_cast<Element*>(node)->getAttribute(accesskeyAttr);
}

VisibleSelection AccessibilityRenderObject::selection() const
{
    return m_renderer->frame()->selection()->selection();
}

PlainTextRange AccessibilityRenderObject::selectedTextRange() const
{
    ASSERT(isTextControl());
    
    if (isPasswordField())
        return PlainTextRange();
    
    AccessibilityRole ariaRole = ariaRoleAttribute();
    if (isNativeTextControl() && ariaRole == UnknownRole) {
        RenderTextControl* textControl = toRenderTextControl(m_renderer);
        return PlainTextRange(textControl->selectionStart(), textControl->selectionEnd() - textControl->selectionStart());
    }
    
    if (ariaRole == UnknownRole)
        return PlainTextRange();
    
    return ariaSelectedTextRange();
}

void AccessibilityRenderObject::setSelectedTextRange(const PlainTextRange& range)
{
    if (isNativeTextControl()) {
        setSelectionRange(m_renderer->node(), range.start, range.start + range.length);
        return;
    }

    Document* document = m_renderer->document();
    if (!document)
        return;
    Frame* frame = document->frame();
    if (!frame)
        return;
    Node* node = m_renderer->node();
    frame->selection()->setSelection(VisibleSelection(Position(node, range.start, Position::PositionIsOffsetInAnchor),
        Position(node, range.start + range.length, Position::PositionIsOffsetInAnchor), DOWNSTREAM));
}

KURL AccessibilityRenderObject::url() const
{
    if (isAnchor() && m_renderer->node()->hasTagName(aTag)) {
        if (HTMLAnchorElement* anchor = static_cast<HTMLAnchorElement*>(anchorElement()))
            return anchor->href();
    }
    
    if (isWebArea())
        return m_renderer->document()->url();
    
    if (isImage() && m_renderer->node() && m_renderer->node()->hasTagName(imgTag))
        return static_cast<HTMLImageElement*>(m_renderer->node())->src();
    
    if (isInputImage())
        return static_cast<HTMLInputElement*>(m_renderer->node())->src();
    
    return KURL();
}

bool AccessibilityRenderObject::isVisited() const
{
    // FIXME: Is it a privacy violation to expose visited information to accessibility APIs?
    return m_renderer->style()->isLink() && m_renderer->style()->insideLink() == InsideVisitedLink;
}

void AccessibilityRenderObject::setElementAttributeValue(const QualifiedName& attributeName, bool value)
{
    if (!m_renderer)
        return;
    
    Node* node = m_renderer->node();
    if (!node || !node->isElementNode())
        return;
    
    Element* element = static_cast<Element*>(node);
    element->setAttribute(attributeName, (value) ? "true" : "false");        
}
    
bool AccessibilityRenderObject::elementAttributeValue(const QualifiedName& attributeName) const
{
    if (!m_renderer)
        return false;
    
    return equalIgnoringCase(getAttribute(attributeName), "true");
}
    
bool AccessibilityRenderObject::isRequired() const
{
    if (equalIgnoringCase(getAttribute(aria_requiredAttr), "true"))
        return true;
    
    Node* n = node();
    if (n && (n->isElementNode() && static_cast<Element*>(n)->isFormControlElement()))
        return static_cast<HTMLFormControlElement*>(n)->required();
    
    return false;
}

bool AccessibilityRenderObject::isSelected() const
{
    if (!m_renderer)
        return false;
    
    Node* node = m_renderer->node();
    if (!node)
        return false;
    
    const AtomicString& ariaSelected = getAttribute(aria_selectedAttr);
    if (equalIgnoringCase(ariaSelected, "true"))
        return true;    
    
    if (isTabItem() && isTabItemSelected())
        return true;

    return false;
}

bool AccessibilityRenderObject::isTabItemSelected() const
{
    if (!isTabItem() || !m_renderer)
        return false;
    
    Node* node = m_renderer->node();
    if (!node || !node->isElementNode())
        return false;
    
    // The ARIA spec says a tab item can also be selected if it is aria-labeled by a tabpanel
    // that has keyboard focus inside of it, or if a tabpanel in its aria-controls list has KB
    // focus inside of it.
    AccessibilityObject* focusedElement = focusedUIElement();
    if (!focusedElement)
        return false;
    
    Vector<Element*> elements;
    elementsFromAttribute(elements, aria_controlsAttr);
    
    unsigned count = elements.size();
    for (unsigned k = 0; k < count; ++k) {
        Element* element = elements[k];
        AccessibilityObject* tabPanel = axObjectCache()->getOrCreate(element->renderer());

        // A tab item should only control tab panels.
        if (!tabPanel || tabPanel->roleValue() != TabPanelRole)
            continue;
        
        AccessibilityObject* checkFocusElement = focusedElement;
        // Check if the focused element is a descendant of the element controlled by the tab item.
        while (checkFocusElement) {
            if (tabPanel == checkFocusElement)
                return true;
            checkFocusElement = checkFocusElement->parentObject();
        }
    }
    
    return false;
}
    
bool AccessibilityRenderObject::isFocused() const
{
    if (!m_renderer)
        return false;
    
    Document* document = m_renderer->document();
    if (!document)
        return false;
    
    Node* focusedNode = document->focusedNode();
    if (!focusedNode)
        return false;
    
    // A web area is represented by the Document node in the DOM tree, which isn't focusable.
    // Check instead if the frame's selection controller is focused
    if (focusedNode == m_renderer->node()
        || (roleValue() == WebAreaRole && document->frame()->selection()->isFocusedAndActive()))
        return true;
    
    return false;
}

void AccessibilityRenderObject::setFocused(bool on)
{
    if (!canSetFocusAttribute())
        return;
    
    if (!on)
        m_renderer->document()->setFocusedNode(0);
    else {
        if (m_renderer->node()->isElementNode())
            static_cast<Element*>(m_renderer->node())->focus();
        else
            m_renderer->document()->setFocusedNode(m_renderer->node());
    }
}

void AccessibilityRenderObject::changeValueByPercent(float percentChange)
{
    float range = maxValueForRange() - minValueForRange();
    float value = valueForRange();
    
    value += range * (percentChange / 100);
    setValue(String::number(value));
    
    axObjectCache()->postNotification(m_renderer, AXObjectCache::AXValueChanged, true);
}

void AccessibilityRenderObject::setSelectedRows(AccessibilityChildrenVector& selectedRows)
{
    // Setting selected only makes sense in trees and tables (and tree-tables).
    AccessibilityRole role = roleValue();
    if (role != TreeRole && role != TreeGridRole && role != TableRole)
        return;
    
    bool isMulti = isMultiSelectable();
    unsigned count = selectedRows.size();
    if (count > 1 && !isMulti)
        count = 1;
    
    for (unsigned k = 0; k < count; ++k)
        selectedRows[k]->setSelected(true);
}
    
void AccessibilityRenderObject::setValue(const String& string)
{
    if (!m_renderer || !m_renderer->node() || !m_renderer->node()->isElementNode())
        return;
    Element* element = static_cast<Element*>(m_renderer->node());

    if (!m_renderer->isBoxModelObject())
        return;
    RenderBoxModelObject* renderer = toRenderBoxModelObject(m_renderer);

    // FIXME: Do we want to do anything here for ARIA textboxes?
    if (renderer->isTextField()) {
        // FIXME: This is not safe!  Other elements could have a TextField renderer.
        static_cast<HTMLInputElement*>(element)->setValue(string);
    } else if (renderer->isTextArea()) {
        // FIXME: This is not safe!  Other elements could have a TextArea renderer.
        static_cast<HTMLTextAreaElement*>(element)->setValue(string);
    }
}

void AccessibilityRenderObject::ariaOwnsElements(AccessibilityChildrenVector& axObjects) const
{
    Vector<Element*> elements;
    elementsFromAttribute(elements, aria_ownsAttr);
    
    unsigned count = elements.size();
    for (unsigned k = 0; k < count; ++k) {
        RenderObject* render = elements[k]->renderer();
        AccessibilityObject* obj = axObjectCache()->getOrCreate(render);
        if (obj)
            axObjects.append(obj);
    }
}

bool AccessibilityRenderObject::supportsARIAOwns() const
{
    if (!m_renderer)
        return false;
    const AtomicString& ariaOwns = getAttribute(aria_ownsAttr);

    return !ariaOwns.isEmpty();
}
    
bool AccessibilityRenderObject::isEnabled() const
{
    ASSERT(m_renderer);
    
    if (equalIgnoringCase(getAttribute(aria_disabledAttr), "true"))
        return false;
    
    Node* node = m_renderer->node();
    if (!node || !node->isElementNode())
        return true;

    return static_cast<Element*>(node)->isEnabledFormControl();
}

RenderView* AccessibilityRenderObject::topRenderer() const
{
    Document* topDoc = topDocument();
    if (!topDoc)
        return 0;
    
    return topDoc->renderView();
}

Document* AccessibilityRenderObject::document() const
{
    if (!m_renderer)
        return 0;
    return m_renderer->document();
}

Document* AccessibilityRenderObject::topDocument() const
{
    if (!document())
        return 0;
    return document()->topDocument();
}
    
FrameView* AccessibilityRenderObject::topDocumentFrameView() const
{
    RenderView* renderView = topRenderer();
    if (!renderView || !renderView->view())
        return 0;
    return renderView->view()->frameView();
}

Widget* AccessibilityRenderObject::widget() const
{
    if (!m_renderer->isBoxModelObject() || !toRenderBoxModelObject(m_renderer)->isWidget())
        return 0;
    return toRenderWidget(m_renderer)->widget();
}

AccessibilityObject* AccessibilityRenderObject::accessibilityParentForImageMap(HTMLMapElement* map) const
{
    // find an image that is using this map
    if (!map)
        return 0;

    HTMLImageElement* imageElement = map->imageElement();
    if (!imageElement)
        return 0;
    
    return axObjectCache()->getOrCreate(imageElement->renderer());
}
    
void AccessibilityRenderObject::getDocumentLinks(AccessibilityChildrenVector& result)
{
    Document* document = m_renderer->document();
    RefPtr<HTMLCollection> coll = document->links();
    Node* curr = coll->firstItem();
    while (curr) {
        RenderObject* obj = curr->renderer();
        if (obj) {
            RefPtr<AccessibilityObject> axobj = document->axObjectCache()->getOrCreate(obj);
            ASSERT(axobj);
            if (!axobj->accessibilityIsIgnored() && axobj->isLink())
                result.append(axobj);
        } else {
            Node* parent = curr->parentNode();
            if (parent && curr->hasTagName(areaTag) && parent->hasTagName(mapTag)) {
                AccessibilityImageMapLink* areaObject = static_cast<AccessibilityImageMapLink*>(axObjectCache()->getOrCreate(ImageMapLinkRole));
                areaObject->setHTMLAreaElement(static_cast<HTMLAreaElement*>(curr));
                areaObject->setHTMLMapElement(static_cast<HTMLMapElement*>(parent));
                areaObject->setParent(accessibilityParentForImageMap(static_cast<HTMLMapElement*>(parent)));

                result.append(areaObject);
            }
        }
        curr = coll->nextItem();
    }
}

FrameView* AccessibilityRenderObject::documentFrameView() const 
{ 
    if (!m_renderer || !m_renderer->document()) 
        return 0; 

    // this is the RenderObject's Document's Frame's FrameView 
    return m_renderer->document()->view();
}

Widget* AccessibilityRenderObject::widgetForAttachmentView() const
{
    if (!isAttachment())
        return 0;
    return toRenderWidget(m_renderer)->widget();
}

FrameView* AccessibilityRenderObject::frameViewIfRenderView() const
{
    if (!m_renderer->isRenderView())
        return 0;
    // this is the RenderObject's Document's renderer's FrameView
    return m_renderer->view()->frameView();
}

// This function is like a cross-platform version of - (WebCoreTextMarkerRange*)textMarkerRange. It returns
// a Range that we can convert to a WebCoreTextMarkerRange in the Obj-C file
VisiblePositionRange AccessibilityRenderObject::visiblePositionRange() const
{
    if (!m_renderer)
        return VisiblePositionRange();
    
    // construct VisiblePositions for start and end
    Node* node = m_renderer->node();
    if (!node)
        return VisiblePositionRange();

    VisiblePosition startPos = firstPositionInOrBeforeNode(node);
    VisiblePosition endPos = lastPositionInOrAfterNode(node);

    // the VisiblePositions are equal for nodes like buttons, so adjust for that
    // FIXME: Really?  [button, 0] and [button, 1] are distinct (before and after the button)
    // I expect this code is only hit for things like empty divs?  In which case I don't think
    // the behavior is correct here -- eseidel
    if (startPos == endPos) {
        endPos = endPos.next();
        if (endPos.isNull())
            endPos = startPos;
    }

    return VisiblePositionRange(startPos, endPos);
}

VisiblePositionRange AccessibilityRenderObject::visiblePositionRangeForLine(unsigned lineCount) const
{
    if (!lineCount || !m_renderer)
        return VisiblePositionRange();
    
    // iterate over the lines
    // FIXME: this is wrong when lineNumber is lineCount+1,  because nextLinePosition takes you to the
    // last offset of the last line
    VisiblePosition visiblePos = m_renderer->document()->renderer()->positionForCoordinates(0, 0);
    VisiblePosition savedVisiblePos;
    while (--lineCount) {
        savedVisiblePos = visiblePos;
        visiblePos = nextLinePosition(visiblePos, 0);
        if (visiblePos.isNull() || visiblePos == savedVisiblePos)
            return VisiblePositionRange();
    }
    
    // make a caret selection for the marker position, then extend it to the line
    // NOTE: ignores results of sel.modify because it returns false when
    // starting at an empty line.  The resulting selection in that case
    // will be a caret at visiblePos.
    SelectionController selection;
    selection.setSelection(VisibleSelection(visiblePos));
    selection.modify(SelectionController::AlterationExtend, DirectionRight, LineBoundary);
    
    return VisiblePositionRange(selection.selection().visibleStart(), selection.selection().visibleEnd());
}
    
VisiblePosition AccessibilityRenderObject::visiblePositionForIndex(int index) const
{
    if (!m_renderer)
        return VisiblePosition();
    
    if (isNativeTextControl())
        return toRenderTextControl(m_renderer)->visiblePositionForIndex(index);

    if (!allowsTextRanges() && !m_renderer->isText())
        return VisiblePosition();
    
    Node* node = m_renderer->node();
    if (!node)
        return VisiblePosition();
    
    if (index <= 0)
        return VisiblePosition(firstPositionInOrBeforeNode(node), DOWNSTREAM);
    
    ExceptionCode ec = 0;
    RefPtr<Range> range = Range::create(m_renderer->document());
    range->selectNodeContents(node, ec);
    CharacterIterator it(range.get());
    it.advance(index - 1);
    return VisiblePosition(Position(it.range()->endContainer(ec), it.range()->endOffset(ec), Position::PositionIsOffsetInAnchor), UPSTREAM);
}
    
int AccessibilityRenderObject::indexForVisiblePosition(const VisiblePosition& pos) const
{
    if (isNativeTextControl())
        return toRenderTextControl(m_renderer)->indexForVisiblePosition(pos);
    
    if (!isTextControl())
        return 0;
    
    Node* node = m_renderer->node();
    if (!node)
        return 0;
    
    Position indexPosition = pos.deepEquivalent();
    if (!indexPosition.anchorNode() || indexPosition.anchorNode()->rootEditableElement() != node)
        return 0;
    
    ExceptionCode ec = 0;
    RefPtr<Range> range = Range::create(m_renderer->document());
    range->setStart(node, 0, ec);
    range->setEnd(indexPosition.anchorNode(), indexPosition.deprecatedEditingOffset(), ec);

#if PLATFORM(GTK)
    // We need to consider replaced elements for GTK, as they will be
    // presented with the 'object replacement character' (0xFFFC).
    return TextIterator::rangeLength(range.get(), true);
#else
    return TextIterator::rangeLength(range.get());
#endif
}

IntRect AccessibilityRenderObject::boundsForVisiblePositionRange(const VisiblePositionRange& visiblePositionRange) const
{
    if (visiblePositionRange.isNull())
        return IntRect();
    
    // Create a mutable VisiblePositionRange.
    VisiblePositionRange range(visiblePositionRange);
    IntRect rect1 = range.start.absoluteCaretBounds();
    IntRect rect2 = range.end.absoluteCaretBounds();
    
    // readjust for position at the edge of a line.  This is to exclude line rect that doesn't need to be accounted in the range bounds
    if (rect2.y() != rect1.y()) {
        VisiblePosition endOfFirstLine = endOfLine(range.start);
        if (range.start == endOfFirstLine) {
            range.start.setAffinity(DOWNSTREAM);
            rect1 = range.start.absoluteCaretBounds();
        }
        if (range.end == endOfFirstLine) {
            range.end.setAffinity(UPSTREAM);
            rect2 = range.end.absoluteCaretBounds();
        }
    }
    
    IntRect ourrect = rect1;
    ourrect.unite(rect2);
    
    // if the rectangle spans lines and contains multiple text chars, use the range's bounding box intead
    if (rect1.maxY() != rect2.maxY()) {
        RefPtr<Range> dataRange = makeRange(range.start, range.end);
        IntRect boundingBox = dataRange->boundingBox();
        String rangeString = plainText(dataRange.get());
        if (rangeString.length() > 1 && !boundingBox.isEmpty())
            ourrect = boundingBox;
    }
    
#if PLATFORM(MAC)
    return m_renderer->document()->view()->contentsToScreen(ourrect);
#else
    return ourrect;
#endif
}
    
void AccessibilityRenderObject::setSelectedVisiblePositionRange(const VisiblePositionRange& range) const
{
    if (range.start.isNull() || range.end.isNull())
        return;
    
    // make selection and tell the document to use it. if it's zero length, then move to that position
    if (range.start == range.end)
        m_renderer->frame()->selection()->moveTo(range.start, true);
    else {
        VisibleSelection newSelection = VisibleSelection(range.start, range.end);
        m_renderer->frame()->selection()->setSelection(newSelection);
    }    
}

VisiblePosition AccessibilityRenderObject::visiblePositionForPoint(const IntPoint& point) const
{
    if (!m_renderer)
        return VisiblePosition();
    
    // convert absolute point to view coordinates
    Document* topDoc = topDocument();
    if (!topDoc || !topDoc->renderer() || !topDoc->renderer()->view())
        return VisiblePosition();
    
    FrameView* frameView = topDoc->renderer()->view()->frameView();
    if (!frameView)
        return VisiblePosition();
    
    RenderView* renderView = topRenderer();
    if (!renderView)
        return VisiblePosition();
    
    Node* innerNode = 0;
    
    // locate the node containing the point
    IntPoint pointResult;
    while (1) {
        IntPoint ourpoint;
#if PLATFORM(MAC)
        ourpoint = frameView->screenToContents(point);
#else
        ourpoint = point;
#endif
        HitTestRequest request(HitTestRequest::ReadOnly |
                               HitTestRequest::Active);
        HitTestResult result(ourpoint);
        renderView->layer()->hitTest(request, result);
        innerNode = result.innerNode();
        if (!innerNode)
            return VisiblePosition();
        
        RenderObject* renderer = innerNode->renderer();
        if (!renderer)
            return VisiblePosition();
        
        pointResult = result.localPoint();

        // done if hit something other than a widget
        if (!renderer->isWidget())
            break;

        // descend into widget (FRAME, IFRAME, OBJECT...)
        Widget* widget = toRenderWidget(renderer)->widget();
        if (!widget || !widget->isFrameView())
            break;
        Frame* frame = static_cast<FrameView*>(widget)->frame();
        if (!frame)
            break;
        renderView = frame->document()->renderView();
        frameView = static_cast<FrameView*>(widget);
    }
    
    return innerNode->renderer()->positionForPoint(pointResult);
}

// NOTE: Consider providing this utility method as AX API
VisiblePosition AccessibilityRenderObject::visiblePositionForIndex(unsigned indexValue, bool lastIndexOK) const
{
    if (!isTextControl())
        return VisiblePosition();
    
    // lastIndexOK specifies whether the position after the last character is acceptable
    if (indexValue >= text().length()) {
        if (!lastIndexOK || indexValue > text().length())
            return VisiblePosition();
    }
    VisiblePosition position = visiblePositionForIndex(indexValue);
    position.setAffinity(DOWNSTREAM);
    return position;
}

// NOTE: Consider providing this utility method as AX API
int AccessibilityRenderObject::index(const VisiblePosition& position) const
{
    if (!isTextControl())
        return -1;
    
    Node* node = position.deepEquivalent().deprecatedNode();
    if (!node)
        return -1;
    
    for (RenderObject* renderer = node->renderer(); renderer && renderer->node(); renderer = renderer->parent()) {
        if (renderer == m_renderer)
            return indexForVisiblePosition(position);
    }
    
    return -1;
}

// Given a line number, the range of characters of the text associated with this accessibility
// object that contains the line number.
PlainTextRange AccessibilityRenderObject::doAXRangeForLine(unsigned lineNumber) const
{
    if (!isTextControl())
        return PlainTextRange();
    
    // iterate to the specified line
    VisiblePosition visiblePos = visiblePositionForIndex(0);
    VisiblePosition savedVisiblePos;
    for (unsigned lineCount = lineNumber; lineCount; lineCount -= 1) {
        savedVisiblePos = visiblePos;
        visiblePos = nextLinePosition(visiblePos, 0);
        if (visiblePos.isNull() || visiblePos == savedVisiblePos)
            return PlainTextRange();
    }

    // Get the end of the line based on the starting position.
    VisiblePosition endPosition = endOfLine(visiblePos);

    int index1 = indexForVisiblePosition(visiblePos);
    int index2 = indexForVisiblePosition(endPosition);
    
    // add one to the end index for a line break not caused by soft line wrap (to match AppKit)
    if (endPosition.affinity() == DOWNSTREAM && endPosition.next().isNotNull())
        index2 += 1;
    
    // return nil rather than an zero-length range (to match AppKit)
    if (index1 == index2)
        return PlainTextRange();
    
    return PlainTextRange(index1, index2 - index1);
}

// The composed character range in the text associated with this accessibility object that
// is specified by the given index value. This parameterized attribute returns the complete
// range of characters (including surrogate pairs of multi-byte glyphs) at the given index.
PlainTextRange AccessibilityRenderObject::doAXRangeForIndex(unsigned index) const
{
    if (!isTextControl())
        return PlainTextRange();
    
    String elementText = text();
    if (!elementText.length() || index > elementText.length() - 1)
        return PlainTextRange();
    
    return PlainTextRange(index, 1);
}

// A substring of the text associated with this accessibility object that is
// specified by the given character range.
String AccessibilityRenderObject::doAXStringForRange(const PlainTextRange& range) const
{
    if (isPasswordField())
        return String();
    
    if (!range.length)
        return String();
    
    if (!isTextControl())
        return String();
    
    String elementText = text();
    if (range.start + range.length > elementText.length())
        return String();
    
    return elementText.substring(range.start, range.length);
}

// The bounding rectangle of the text associated with this accessibility object that is
// specified by the given range. This is the bounding rectangle a sighted user would see
// on the display screen, in pixels.
IntRect AccessibilityRenderObject::doAXBoundsForRange(const PlainTextRange& range) const
{
    if (allowsTextRanges())
        return boundsForVisiblePositionRange(visiblePositionRangeForRange(range));
    return IntRect();
}

AccessibilityObject* AccessibilityRenderObject::accessibilityImageMapHitTest(HTMLAreaElement* area, const IntPoint& point) const
{
    if (!area)
        return 0;
    
    HTMLMapElement* map = static_cast<HTMLMapElement*>(area->parentNode());
    AccessibilityObject* parent = accessibilityParentForImageMap(map);
    if (!parent)
        return 0;
    
    AccessibilityObject::AccessibilityChildrenVector children = parent->children();
    
    unsigned count = children.size();
    for (unsigned k = 0; k < count; ++k) {
        if (children[k]->elementRect().contains(point))
            return children[k].get();
    }
    
    return 0;
}
    
AccessibilityObject* AccessibilityRenderObject::accessibilityHitTest(const IntPoint& point) const
{
    if (!m_renderer || !m_renderer->hasLayer())
        return 0;
    
    RenderLayer* layer = toRenderBox(m_renderer)->layer();
     
    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active);
    HitTestResult hitTestResult = HitTestResult(point);
    layer->hitTest(request, hitTestResult);
    if (!hitTestResult.innerNode())
        return 0;
    Node* node = hitTestResult.innerNode()->shadowAncestorNode();

    if (node->hasTagName(areaTag)) 
        return accessibilityImageMapHitTest(static_cast<HTMLAreaElement*>(node), point);
    
    if (node->hasTagName(optionTag))
        node = static_cast<HTMLOptionElement*>(node)->ownerSelectElement();
    
    RenderObject* obj = node->renderer();
    if (!obj)
        return 0;
    
    AccessibilityObject* result = obj->document()->axObjectCache()->getOrCreate(obj);
    result->updateChildrenIfNecessary();

    // Allow the element to perform any hit-testing it might need to do to reach non-render children.
    result = result->elementAccessibilityHitTest(point);
    
    if (result && result->accessibilityIsIgnored()) {
        // If this element is the label of a control, a hit test should return the control.
        AccessibilityObject* controlObject = result->correspondingControlForLabelElement();
        if (controlObject && !controlObject->exposesTitleUIElement())
            return controlObject;

        result = result->parentObjectUnignored();
    }

    return result;
}

bool AccessibilityRenderObject::shouldFocusActiveDescendant() const
{
    switch (ariaRoleAttribute()) {
    case GroupRole:
    case ComboBoxRole:
    case ListBoxRole:
    case MenuRole:
    case MenuBarRole:
    case RadioGroupRole:
    case RowRole:
    case PopUpButtonRole:
    case ProgressIndicatorRole:
    case ToolbarRole:
    case OutlineRole:
    case TreeRole:
    case GridRole:
    /* FIXME: replace these with actual roles when they are added to AccessibilityRole
    composite
    alert
    alertdialog
    status
    timer
    */
        return true;
    default:
        return false;
    }
}

AccessibilityObject* AccessibilityRenderObject::activeDescendant() const
{
    if (!m_renderer)
        return 0;
    
    if (m_renderer->node() && !m_renderer->node()->isElementNode())
        return 0;
    Element* element = static_cast<Element*>(m_renderer->node());
        
    const AtomicString& activeDescendantAttrStr = element->getAttribute(aria_activedescendantAttr);
    if (activeDescendantAttrStr.isNull() || activeDescendantAttrStr.isEmpty())
        return 0;
    
    Element* target = element->treeScope()->getElementById(activeDescendantAttrStr);
    if (!target)
        return 0;
    
    AccessibilityObject* obj = axObjectCache()->getOrCreate(target->renderer());
    if (obj && obj->isAccessibilityRenderObject())
    // an activedescendant is only useful if it has a renderer, because that's what's needed to post the notification
        return obj;
    return 0;
}

void AccessibilityRenderObject::handleAriaExpandedChanged()
{
    // Find if a parent of this object should handle aria-expanded changes.
    AccessibilityObject* containerParent = this->parentObject();
    while (containerParent) {
        bool foundParent = false;
        
        switch (containerParent->roleValue()) {
        case TreeRole:
        case TreeGridRole:
        case GridRole:
        case TableRole:
        case BrowserRole:
            foundParent = true;
            break;
        default:
            break;
        }
        
        if (foundParent)
            break;
        
        containerParent = containerParent->parentObject();
    }
    
    // Post that the row count changed.
    if (containerParent)
        axObjectCache()->postNotification(containerParent, document(), AXObjectCache::AXRowCountChanged, true);

    // Post that the specific row either collapsed or expanded.
    if (roleValue() == RowRole || roleValue() == TreeItemRole)
        axObjectCache()->postNotification(this, document(), isExpanded() ? AXObjectCache::AXRowExpanded : AXObjectCache::AXRowCollapsed, true);
}

void AccessibilityRenderObject::handleActiveDescendantChanged()
{
    Element* element = static_cast<Element*>(renderer()->node());
    if (!element)
        return;
    Document* doc = renderer()->document();
    if (!doc->frame()->selection()->isFocusedAndActive() || doc->focusedNode() != element)
        return; 
    AccessibilityRenderObject* activedescendant = static_cast<AccessibilityRenderObject*>(activeDescendant());
    
    if (activedescendant && shouldFocusActiveDescendant())
        doc->axObjectCache()->postNotification(m_renderer, AXObjectCache::AXActiveDescendantChanged, true);
}

AccessibilityObject* AccessibilityRenderObject::correspondingControlForLabelElement() const
{
    HTMLLabelElement* labelElement = labelElementContainer();
    if (!labelElement)
        return 0;
    
    HTMLElement* correspondingControl = labelElement->control();
    if (!correspondingControl)
        return 0;
    
    return axObjectCache()->getOrCreate(correspondingControl->renderer());     
}

AccessibilityObject* AccessibilityRenderObject::correspondingLabelForControlElement() const
{
    if (!m_renderer)
        return 0;

    Node* node = m_renderer->node();
    if (node && node->isHTMLElement()) {
        HTMLLabelElement* label = labelForElement(static_cast<Element*>(node));
        if (label)
            return axObjectCache()->getOrCreate(label->renderer());
    }

    return 0;
}

bool AccessibilityRenderObject::renderObjectIsObservable(RenderObject* renderer) const
{
    // AX clients will listen for AXValueChange on a text control.
    if (renderer->isTextControl())
        return true;
    
    // AX clients will listen for AXSelectedChildrenChanged on listboxes.
    Node* node = renderer->node();
    if (nodeHasRole(node, "listbox") || (renderer->isBoxModelObject() && toRenderBoxModelObject(renderer)->isListBox()))
        return true;

    // Textboxes should send out notifications.
    if (nodeHasRole(node, "textbox"))
        return true;
    
    return false;
}
    
AccessibilityObject* AccessibilityRenderObject::observableObject() const
{
    // Find the object going up the parent chain that is used in accessibility to monitor certain notifications.
    for (RenderObject* renderer = m_renderer; renderer && renderer->node(); renderer = renderer->parent()) {
        if (renderObjectIsObservable(renderer))
            return axObjectCache()->getOrCreate(renderer);
    }
    
    return 0;
}

AccessibilityRole AccessibilityRenderObject::determineAriaRoleAttribute() const
{
    const AtomicString& ariaRole = getAttribute(roleAttr);
    if (ariaRole.isNull() || ariaRole.isEmpty())
        return UnknownRole;
    
    AccessibilityRole role = ariaRoleToWebCoreRole(ariaRole);

    if (role == ButtonRole && ariaHasPopup())
        role = PopUpButtonRole;

    if (role == TextAreaRole && ariaIsMultiline())
        role = TextFieldRole;
    
    if (role)
        return role;

    AccessibilityObject* parentObject = parentObjectUnignored();
    if (!parentObject)
        return UnknownRole;

    AccessibilityRole parentAriaRole = parentObject->ariaRoleAttribute();

    // selects and listboxes both have options as child roles, but they map to different roles within WebCore
    if (equalIgnoringCase(ariaRole, "option")) {
        if (parentAriaRole == MenuRole)
            return MenuItemRole;
        if (parentAriaRole == ListBoxRole)
            return ListBoxOptionRole;
    }
    // an aria "menuitem" may map to MenuButton or MenuItem depending on its parent
    if (equalIgnoringCase(ariaRole, "menuitem")) {
        if (parentAriaRole == GroupRole)
            return MenuButtonRole;
        if (parentAriaRole == MenuRole || parentAriaRole == MenuBarRole)
            return MenuItemRole;
    }
    
    return UnknownRole;
}

AccessibilityRole AccessibilityRenderObject::ariaRoleAttribute() const
{
    return m_ariaRole;
}
    
void AccessibilityRenderObject::updateAccessibilityRole()
{
    bool ignoredStatus = accessibilityIsIgnored();
    m_role = determineAccessibilityRole();
    
    // The AX hierarchy only needs to be updated if the ignored status of an element has changed.
    if (ignoredStatus != accessibilityIsIgnored())
        childrenChanged();
}
    
AccessibilityRole AccessibilityRenderObject::determineAccessibilityRole()
{
    if (!m_renderer)
        return UnknownRole;

    m_ariaRole = determineAriaRoleAttribute();
    
    Node* node = m_renderer->node();
    AccessibilityRole ariaRole = ariaRoleAttribute();
    if (ariaRole != UnknownRole)
        return ariaRole;

    RenderBoxModelObject* cssBox = renderBoxModelObject();

    if (node && node->isLink()) {
        if (cssBox && cssBox->isImage())
            return ImageMapRole;
        return WebCoreLinkRole;
    }
    if (cssBox && cssBox->isListItem())
        return ListItemRole;
    if (m_renderer->isListMarker())
        return ListMarkerRole;
    if (node && node->hasTagName(buttonTag))
        return ButtonRole;
    if (m_renderer->isText())
        return StaticTextRole;
    if (cssBox && cssBox->isImage()) {
        if (node && node->hasTagName(inputTag))
            return ButtonRole;
        return ImageRole;
    }
    if (node && node->hasTagName(canvasTag))
        return ImageRole;

    if (cssBox && cssBox->isRenderView())
        return WebAreaRole;
    
    if (cssBox && cssBox->isTextField())
        return TextFieldRole;
    
    if (cssBox && cssBox->isTextArea())
        return TextAreaRole;

    if (node && node->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(node);
        if (input->isCheckbox())
            return CheckBoxRole;
        if (input->isRadioButton())
            return RadioButtonRole;
        if (input->isTextButton())
            return ButtonRole;
    }

    if (node && node->hasTagName(buttonTag))
        return ButtonRole;

    if (isFileUploadButton())
        return ButtonRole;
    
    if (cssBox && cssBox->isMenuList())
        return PopUpButtonRole;
    
    if (headingLevel())
        return HeadingRole;
    
#if ENABLE(MATHML)
    if (node && node->hasTagName(MathMLNames::mathTag))
        return DocumentMathRole;
#endif
    
    if (node && node->hasTagName(ddTag))
        return DefinitionListDefinitionRole;
    
    if (node && node->hasTagName(dtTag))
        return DefinitionListTermRole;

    if (node && (node->hasTagName(rpTag) || node->hasTagName(rtTag)))
        return AnnotationRole;

#if PLATFORM(GTK)
    // Gtk ATs expect all tables, data and layout, to be exposed as tables.
    if (node && (node->hasTagName(tdTag) || node->hasTagName(thTag)))
        return CellRole;

    if (node && node->hasTagName(trTag))
        return RowRole;

    if (node && node->hasTagName(tableTag))
        return TableRole;
#endif

    // Table sections should be ignored.
    if (m_renderer->isTableSection())
        return IgnoredRole;
    
#if PLATFORM(GTK)
    if (m_renderer->isHR())
        return SplitterRole;

    if (node && node->hasTagName(pTag))
        return ParagraphRole;

    if (node && node->hasTagName(labelTag))
        return LabelRole;

    if (node && node->hasTagName(divTag))
        return DivRole;

    if (node && node->hasTagName(formTag))
        return FormRole;
#else
    if (node && node->hasTagName(labelTag))
        return GroupRole;
#endif

    if (m_renderer->isBlockFlow())
        return GroupRole;
    
    // If the element does not have role, but it has ARIA attributes, accessibility should fallback to exposing it as a group.
    if (supportsARIAAttributes())
        return GroupRole;
    
    return UnknownRole;
}

AccessibilityOrientation AccessibilityRenderObject::orientation() const
{
    const AtomicString& ariaOrientation = getAttribute(aria_orientationAttr);
    if (equalIgnoringCase(ariaOrientation, "horizontal"))
        return AccessibilityOrientationHorizontal;
    if (equalIgnoringCase(ariaOrientation, "vertical"))
        return AccessibilityOrientationVertical;
    
    return AccessibilityObject::orientation();
}
    
bool AccessibilityRenderObject::inheritsPresentationalRole() const
{
    // ARIA spec says that when a parent object is presentational, and it has required child elements,
    // those child elements are also presentational. For example, <li> becomes presentational from <ul>.
    // http://www.w3.org/WAI/PF/aria/complete#presentation
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, listItemParents, ());

    HashSet<QualifiedName>* possibleParentTagNames = 0;
    switch (roleValue()) {
    case ListItemRole:
    case ListMarkerRole:
        if (listItemParents.isEmpty()) {
            listItemParents.add(ulTag);
            listItemParents.add(olTag);
            listItemParents.add(dlTag);
        }
        possibleParentTagNames = &listItemParents;
        break;
    default:
        break;
    }
    
    // Not all elements need to check for this, only ones that are required children.
    if (!possibleParentTagNames)
        return false;
    
    for (AccessibilityObject* parent = parentObject(); parent; parent = parent->parentObject()) { 
        if (!parent->isAccessibilityRenderObject())
            continue;
        
        Node* elementNode = static_cast<AccessibilityRenderObject*>(parent)->node();
        if (!elementNode || !elementNode->isElementNode())
            continue;
        
        // If native tag of the parent element matches an acceptable name, then return
        // based on its presentational status.
        if (possibleParentTagNames->contains(static_cast<Element*>(elementNode)->tagQName()))
            return parent->roleValue() == PresentationalRole;
    }
    
    return false;
}
    
bool AccessibilityRenderObject::isPresentationalChildOfAriaRole() const
{
    // Walk the parent chain looking for a parent that has presentational children
    AccessibilityObject* parent;
    for (parent = parentObject(); parent && !parent->ariaRoleHasPresentationalChildren(); parent = parent->parentObject())
    { }
    
    return parent;
}
    
bool AccessibilityRenderObject::ariaRoleHasPresentationalChildren() const
{
    switch (m_ariaRole) {
    case ButtonRole:
    case SliderRole:
    case ImageRole:
    case ProgressIndicatorRole:
    // case SeparatorRole:
        return true;
    default:
        return false;
    }
}

bool AccessibilityRenderObject::canSetFocusAttribute() const
{
    ASSERT(m_renderer);
    Node* node = m_renderer->node();

    // NOTE: It would be more accurate to ask the document whether setFocusedNode() would
    // do anything.  For example, setFocusedNode() will do nothing if the current focused
    // node will not relinquish the focus.
    if (!node || !node->isElementNode())
        return false;

    if (!static_cast<Element*>(node)->isEnabledFormControl())
        return false;

    switch (roleValue()) {
    case WebCoreLinkRole:
    case ImageMapLinkRole:
    case TextFieldRole:
    case TextAreaRole:
    case ButtonRole:
    case PopUpButtonRole:
    case CheckBoxRole:
    case RadioButtonRole:
    case SliderRole:
        return true;
    default:
        return node->supportsFocus();
    }
}
    
bool AccessibilityRenderObject::canSetExpandedAttribute() const
{
    // An object can be expanded if it aria-expanded is true or false.
    const AtomicString& ariaExpanded = getAttribute(aria_expandedAttr);
    return equalIgnoringCase(ariaExpanded, "true") || equalIgnoringCase(ariaExpanded, "false");
}

bool AccessibilityRenderObject::canSetValueAttribute() const
{
    if (equalIgnoringCase(getAttribute(aria_readonlyAttr), "true"))
        return false;

    // Any node could be contenteditable, so isReadOnly should be relied upon
    // for this information for all elements.
    return isProgressIndicator() || isSlider() || !isReadOnly();
}

bool AccessibilityRenderObject::canSetTextRangeAttributes() const
{
    return isTextControl();
}

void AccessibilityRenderObject::contentChanged()
{
    // If this element supports ARIA live regions, then notify the AT of changes.
    AXObjectCache* cache = axObjectCache();
    for (RenderObject* renderParent = m_renderer; renderParent; renderParent = renderParent->parent()) {
        AccessibilityObject* parent = cache->get(renderParent);
        if (!parent)
            continue;
        
        // If we find a parent that has ARIA live region on, send the notification and stop processing.
        // The spec does not talk about nested live regions.
        if (parent->supportsARIALiveRegion()) {
            axObjectCache()->postNotification(renderParent, AXObjectCache::AXLiveRegionChanged, true);
            break;
        }
    }
}
    
void AccessibilityRenderObject::childrenChanged()
{
    // This method is meant as a quick way of marking a portion of the accessibility tree dirty.
    if (!m_renderer)
        return;
    
    bool sentChildrenChanged = false;
    
    // Go up the accessibility parent chain, but only if the element already exists. This method is
    // called during render layouts, minimal work should be done. 
    // If AX elements are created now, they could interrogate the render tree while it's in a funky state.
    // At the same time, process ARIA live region changes.
    for (AccessibilityObject* parent = this; parent; parent = parent->parentObjectIfExists()) {
        if (!parent->isAccessibilityRenderObject())
            continue;
        
        AccessibilityRenderObject* axParent = static_cast<AccessibilityRenderObject*>(parent);
        
        // Send the children changed notification on the first accessibility render object ancestor.
        if (!sentChildrenChanged) {
            axObjectCache()->postNotification(axParent->renderer(), AXObjectCache::AXChildrenChanged, true);
            sentChildrenChanged = true;
        }
        
        // Only do work if the children haven't been marked dirty. This has the effect of blocking
        // future live region change notifications until the AX tree has been accessed again. This
        // is a good performance win for all parties.
        if (!axParent->needsToUpdateChildren()) {
            axParent->setNeedsToUpdateChildren();
            
            // If this element supports ARIA live regions, then notify the AT of changes.
            if (axParent->supportsARIALiveRegion())
                axObjectCache()->postNotification(axParent->renderer(), AXObjectCache::AXLiveRegionChanged, true);
        }
    }
}
    
bool AccessibilityRenderObject::canHaveChildren() const
{
    if (!m_renderer)
        return false;
    
    // Elements that should not have children
    switch (roleValue()) {
    case ImageRole:
    case ButtonRole:
    case PopUpButtonRole:
    case CheckBoxRole:
    case RadioButtonRole:
    case TabRole:
    case StaticTextRole:
    case ListBoxOptionRole:
    case ScrollBarRole:
        return false;
    default:
        return true;
    }
}

void AccessibilityRenderObject::clearChildren()
{
    AccessibilityObject::clearChildren();
    m_childrenDirty = false;
}
    
void AccessibilityRenderObject::updateChildrenIfNecessary()
{
    if (needsToUpdateChildren())
        clearChildren();        
    
    AccessibilityObject::updateChildrenIfNecessary();
}
    
const AccessibilityObject::AccessibilityChildrenVector& AccessibilityRenderObject::children()
{
    updateChildrenIfNecessary();
    
    return m_children;
}

void AccessibilityRenderObject::addChildren()
{
    // If the need to add more children in addition to existing children arises, 
    // childrenChanged should have been called, leaving the object with no children.
    ASSERT(!m_haveChildren); 
    
    // nothing to add if there is no RenderObject
    if (!m_renderer)
        return;
    
    m_haveChildren = true;
    
    if (!canHaveChildren())
        return;
    
    // add all unignored acc children
    for (RefPtr<AccessibilityObject> obj = firstChild(); obj; obj = obj->nextSibling()) {
        if (obj->accessibilityIsIgnored()) {
            obj->updateChildrenIfNecessary();
            AccessibilityChildrenVector children = obj->children();
            unsigned length = children.size();
            for (unsigned i = 0; i < length; ++i)
                m_children.append(children[i]);
        } else {
            ASSERT(obj->parentObject() == this);
            m_children.append(obj);
        }
    }
    
    // FrameView's need to be inserted into the AX hierarchy when encountered.
    if (isAttachment()) {
        Widget* widget = widgetForAttachmentView();
        if (widget && widget->isFrameView())
            m_children.append(axObjectCache()->getOrCreate(widget));
    }
    
    // for a RenderImage, add the <area> elements as individual accessibility objects
    RenderBoxModelObject* cssBox = renderBoxModelObject();
    if (cssBox && cssBox->isRenderImage()) {
        HTMLMapElement* map = toRenderImage(cssBox)->imageMap();
        if (map) {
            for (Node* current = map->firstChild(); current; current = current->traverseNextNode(map)) {

                // add an <area> element for this child if it has a link
                if (current->hasTagName(areaTag) && current->isLink()) {
                    AccessibilityImageMapLink* areaObject = static_cast<AccessibilityImageMapLink*>(axObjectCache()->getOrCreate(ImageMapLinkRole));
                    areaObject->setHTMLAreaElement(static_cast<HTMLAreaElement*>(current));
                    areaObject->setHTMLMapElement(map);
                    areaObject->setParent(this);

                    m_children.append(areaObject);
                }
            }
        }
    }
}
        
const AtomicString& AccessibilityRenderObject::ariaLiveRegionStatus() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, liveRegionStatusAssertive, ("assertive"));
    DEFINE_STATIC_LOCAL(const AtomicString, liveRegionStatusPolite, ("polite"));
    DEFINE_STATIC_LOCAL(const AtomicString, liveRegionStatusOff, ("off"));
    
    const AtomicString& liveRegionStatus = getAttribute(aria_liveAttr);
    // These roles have implicit live region status.
    if (liveRegionStatus.isEmpty()) {
        switch (roleValue()) {
        case ApplicationAlertDialogRole:
        case ApplicationAlertRole:
            return liveRegionStatusAssertive;
        case ApplicationLogRole:
        case ApplicationStatusRole:
            return liveRegionStatusPolite;
        case ApplicationTimerRole:
        case ApplicationMarqueeRole:
            return liveRegionStatusOff;
        default:
            break;
        }
    }

    return liveRegionStatus;
}

const AtomicString& AccessibilityRenderObject::ariaLiveRegionRelevant() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, defaultLiveRegionRelevant, ("additions text"));
    const AtomicString& relevant = getAttribute(aria_relevantAttr);

    // Default aria-relevant = "additions text".
    if (relevant.isEmpty())
        return defaultLiveRegionRelevant;
    
    return relevant;
}

bool AccessibilityRenderObject::ariaLiveRegionAtomic() const
{
    return elementAttributeValue(aria_atomicAttr);    
}

bool AccessibilityRenderObject::ariaLiveRegionBusy() const
{
    return elementAttributeValue(aria_busyAttr);    
}
    
void AccessibilityRenderObject::ariaSelectedRows(AccessibilityChildrenVector& result)
{
    // Get all the rows. 
    AccessibilityChildrenVector allRows;
    ariaTreeRows(allRows);

    // Determine which rows are selected.
    bool isMulti = isMultiSelectable();

    // Prefer active descendant over aria-selected.
    AccessibilityObject* activeDesc = activeDescendant();
    if (activeDesc && (activeDesc->isTreeItem() || activeDesc->isTableRow())) {
        result.append(activeDesc);    
        if (!isMulti)
            return;
    }

    unsigned count = allRows.size();
    for (unsigned k = 0; k < count; ++k) {
        if (allRows[k]->isSelected()) {
            result.append(allRows[k]);
            if (!isMulti)
                break;
        }
    }
}
    
void AccessibilityRenderObject::ariaListboxSelectedChildren(AccessibilityChildrenVector& result)
{
    bool isMulti = isMultiSelectable();

    AccessibilityChildrenVector childObjects = children();
    unsigned childrenSize = childObjects.size();
    for (unsigned k = 0; k < childrenSize; ++k) {
        // Every child should have aria-role option, and if so, check for selected attribute/state.
        AccessibilityObject* child = childObjects[k].get();
        if (child->isSelected() && child->ariaRoleAttribute() == ListBoxOptionRole) {
            result.append(child);
            if (!isMulti)
                return;
        }
    }
}

void AccessibilityRenderObject::selectedChildren(AccessibilityChildrenVector& result)
{
    ASSERT(result.isEmpty());

    // only listboxes should be asked for their selected children. 
    AccessibilityRole role = roleValue();
    if (role == ListBoxRole) // native list boxes would be AccessibilityListBoxes, so only check for aria list boxes
        ariaListboxSelectedChildren(result);
    else if (role == TreeRole || role == TreeGridRole || role == TableRole)
        ariaSelectedRows(result);
}

void AccessibilityRenderObject::ariaListboxVisibleChildren(AccessibilityChildrenVector& result)      
{
    if (!hasChildren())
        addChildren();
    
    unsigned length = m_children.size();
    for (unsigned i = 0; i < length; i++) {
        if (!m_children[i]->isOffScreen())
            result.append(m_children[i]);
    }
}

void AccessibilityRenderObject::visibleChildren(AccessibilityChildrenVector& result)
{
    ASSERT(result.isEmpty());
        
    // only listboxes are asked for their visible children. 
    if (ariaRoleAttribute() != ListBoxRole) { // native list boxes would be AccessibilityListBoxes, so only check for aria list boxes
        ASSERT_NOT_REACHED();
        return;
    }
    return ariaListboxVisibleChildren(result);
}
 
void AccessibilityRenderObject::tabChildren(AccessibilityChildrenVector& result)
{
    ASSERT(roleValue() == TabListRole);
    
    unsigned length = m_children.size();
    for (unsigned i = 0; i < length; ++i) {
        if (m_children[i]->isTabItem())
            result.append(m_children[i]);
    }
}
    
const String& AccessibilityRenderObject::actionVerb() const
{
    // FIXME: Need to add verbs for select elements.
    DEFINE_STATIC_LOCAL(const String, buttonAction, (AXButtonActionVerb()));
    DEFINE_STATIC_LOCAL(const String, textFieldAction, (AXTextFieldActionVerb()));
    DEFINE_STATIC_LOCAL(const String, radioButtonAction, (AXRadioButtonActionVerb()));
    DEFINE_STATIC_LOCAL(const String, checkedCheckBoxAction, (AXCheckedCheckBoxActionVerb()));
    DEFINE_STATIC_LOCAL(const String, uncheckedCheckBoxAction, (AXUncheckedCheckBoxActionVerb()));
    DEFINE_STATIC_LOCAL(const String, linkAction, (AXLinkActionVerb()));
    DEFINE_STATIC_LOCAL(const String, noAction, ());
    
    switch (roleValue()) {
    case ButtonRole:
        return buttonAction;
    case TextFieldRole:
    case TextAreaRole:
        return textFieldAction;
    case RadioButtonRole:
        return radioButtonAction;
    case CheckBoxRole:
        return isChecked() ? checkedCheckBoxAction : uncheckedCheckBoxAction;
    case LinkRole:
    case WebCoreLinkRole:
        return linkAction;
    default:
        return noAction;
    }
}
    
void AccessibilityRenderObject::setAccessibleName(String& name)
{
    // Setting the accessible name can store the value in the DOM
    if (!m_renderer)
        return;

    Node* domNode = 0;
    // For web areas, set the aria-label on the HTML element.
    if (isWebArea())
        domNode = m_renderer->document()->documentElement();
    else
        domNode = m_renderer->node();

    if (domNode && domNode->isElementNode())
        static_cast<Element*>(domNode)->setAttribute(aria_labelAttr, name);
}
    
void AccessibilityRenderObject::updateBackingStore()
{
    if (!m_renderer)
        return;

    // Updating layout may delete m_renderer and this object.
    m_renderer->document()->updateLayoutIgnorePendingStylesheets();
}

static bool isLinkable(const AccessibilityRenderObject& object)
{
    if (!object.renderer())
        return false;

    // See https://wiki.mozilla.org/Accessibility/AT-Windows-API for the elements
    // Mozilla considers linkable.
    return object.isLink() || object.isImage() || object.renderer()->isText();
}

String AccessibilityRenderObject::stringValueForMSAA() const
{
    if (isLinkable(*this)) {
        Element* anchor = anchorElement();
        if (anchor && anchor->hasTagName(aTag))
            return static_cast<HTMLAnchorElement*>(anchor)->href();
    }

    return stringValue();
}

bool AccessibilityRenderObject::isLinked() const
{
    if (!isLinkable(*this))
        return false;

    Element* anchor = anchorElement();
    if (!anchor || !anchor->hasTagName(aTag))
        return false;

    return !static_cast<HTMLAnchorElement*>(anchor)->href().isEmpty();
}

String AccessibilityRenderObject::nameForMSAA() const
{
    if (m_renderer && m_renderer->isText())
        return textUnderElement();

    return title();
}

static bool shouldReturnTagNameAsRoleForMSAA(const Element& element)
{
    // See "document structure",
    // https://wiki.mozilla.org/Accessibility/AT-Windows-API
    // FIXME: Add the other tag names that should be returned as the role.
    return element.hasTagName(h1Tag) || element.hasTagName(h2Tag) 
        || element.hasTagName(h3Tag) || element.hasTagName(h4Tag)
        || element.hasTagName(h5Tag) || element.hasTagName(h6Tag);
}

String AccessibilityRenderObject::stringRoleForMSAA() const
{
    if (!m_renderer)
        return String();

    Node* node = m_renderer->node();
    if (!node || !node->isElementNode())
        return String();

    Element* element = static_cast<Element*>(node);
    if (!shouldReturnTagNameAsRoleForMSAA(*element))
        return String();

    return element->tagName();
}

String AccessibilityRenderObject::positionalDescriptionForMSAA() const
{
    // See "positional descriptions",
    // https://wiki.mozilla.org/Accessibility/AT-Windows-API
    if (isHeading())
        return "L" + String::number(headingLevel());

    // FIXME: Add positional descriptions for other elements.
    return String();
}

String AccessibilityRenderObject::descriptionForMSAA() const
{
    String description = positionalDescriptionForMSAA();
    if (!description.isEmpty())
        return description;

    description = accessibilityDescription();
    if (!description.isEmpty()) {
        // From the Mozilla MSAA implementation:
        // "Signal to screen readers that this description is speakable and is not
        // a formatted positional information description. Don't localize the
        // 'Description: ' part of this string, it will be parsed out by assistive
        // technologies."
        return "Description: " + description;
    }

    return String();
}

static AccessibilityRole msaaRoleForRenderer(const RenderObject* renderer)
{
    if (!renderer)
        return UnknownRole;

    if (renderer->isText())
        return EditableTextRole;

    if (renderer->isBoxModelObject() && toRenderBoxModelObject(renderer)->isListItem())
        return ListItemRole;

    return UnknownRole;
}

AccessibilityRole AccessibilityRenderObject::roleValueForMSAA() const
{
    if (m_roleForMSAA != UnknownRole)
        return m_roleForMSAA;

    m_roleForMSAA = msaaRoleForRenderer(m_renderer);

    if (m_roleForMSAA == UnknownRole)
        m_roleForMSAA = roleValue();

    return m_roleForMSAA;
}

} // namespace WebCore
