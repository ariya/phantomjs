/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SliderThumbElement_h
#define SliderThumbElement_h

#include "HTMLDivElement.h"
#include "HTMLNames.h"
#include "RenderBlock.h"
#include <wtf/Forward.h>

namespace WebCore {

class HTMLElement;
class HTMLInputElement;
class Event;
class FloatPoint;

class SliderThumbElement FINAL : public HTMLDivElement {
public:
    static PassRefPtr<SliderThumbElement> create(Document*);

    void setPositionFromValue();

    void dragFrom(const LayoutPoint&);
    virtual void defaultEventHandler(Event*);
    virtual bool willRespondToMouseMoveEvents() OVERRIDE;
    virtual bool willRespondToMouseClickEvents() OVERRIDE;
    virtual void detach(const AttachContext& = AttachContext()) OVERRIDE;
    virtual const AtomicString& shadowPseudoId() const;
    HTMLInputElement* hostInput() const;
    void setPositionFromPoint(const LayoutPoint&);

private:
    SliderThumbElement(Document*);
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual PassRefPtr<Element> cloneElementWithoutAttributesAndChildren();
    virtual bool isDisabledFormControl() const OVERRIDE;
    virtual bool matchesReadOnlyPseudoClass() const OVERRIDE;
    virtual bool matchesReadWritePseudoClass() const OVERRIDE;
    virtual Element* focusDelegate() OVERRIDE;
    void startDragging();
    void stopDragging();

    bool m_inDragMode;
};

inline SliderThumbElement::SliderThumbElement(Document* document)
    : HTMLDivElement(HTMLNames::divTag, document)
    , m_inDragMode(false)
{
}

inline PassRefPtr<SliderThumbElement> SliderThumbElement::create(Document* document)
{
    return adoptRef(new SliderThumbElement(document));
}

inline PassRefPtr<Element> SliderThumbElement::cloneElementWithoutAttributesAndChildren()
{
    return create(document());
}

inline SliderThumbElement* toSliderThumbElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || node->isHTMLElement());
    return static_cast<SliderThumbElement*>(node);
}

// This always return a valid pointer.
// An assertion fails if the specified node is not a range input.
SliderThumbElement* sliderThumbElementOf(Node*);
HTMLElement* sliderTrackElementOf(Node*);

// --------------------------------

class RenderSliderThumb FINAL : public RenderBlock {
public:
    RenderSliderThumb(SliderThumbElement*);
    void updateAppearance(RenderStyle* parentStyle);

private:
    virtual bool isSliderThumb() const;
};

// --------------------------------

class SliderContainerElement FINAL : public HTMLDivElement {
public:
    static PassRefPtr<SliderContainerElement> create(Document*);

private:
    SliderContainerElement(Document*);
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual const AtomicString& shadowPseudoId() const;
};

}

#endif
