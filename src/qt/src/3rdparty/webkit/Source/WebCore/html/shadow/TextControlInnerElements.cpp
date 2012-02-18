/*
 * Copyright (C) 2006, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TextControlInnerElements.h"

#include "BeforeTextInsertedEvent.h"
#include "Document.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "Frame.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLTextAreaElement.h"
#include "MouseEvent.h"
#include "Page.h"
#include "RenderLayer.h"
#include "RenderTextControlSingleLine.h"
#include "ScrollbarTheme.h"
#include "SpeechInput.h"
#include "SpeechInputEvent.h"

namespace WebCore {

using namespace HTMLNames;

TextControlInnerElement::TextControlInnerElement(Document* document, HTMLElement* shadowParent)
    : HTMLDivElement(divTag, document)
{
    setShadowHost(shadowParent);
}

PassRefPtr<TextControlInnerElement> TextControlInnerElement::create(HTMLElement* shadowParent)
{
    return adoptRef(new TextControlInnerElement(shadowParent->document(), shadowParent));
}

void TextControlInnerElement::attachInnerElement(Node* parent, PassRefPtr<RenderStyle> style, RenderArena* arena)
{
    // When adding these elements, create the renderer & style first before adding to the DOM.
    // Otherwise, the render tree will create some anonymous blocks that will mess up our layout.

    // Create the renderer with the specified style
    RenderObject* renderer = createRenderer(arena, style.get());
    if (renderer) {
        setRenderer(renderer);
        renderer->setStyle(style);
    }

    // Set these explicitly since this normally happens during an attach()
    setAttached();
    setInDocument();

    // For elements not yet in shadow DOM, add the node to the DOM normally.
    if (!isShadowRoot()) {
        // FIXME: This code seems very wrong.  Why are we magically adding |this| to the DOM here?
        //        We shouldn't be calling parser API methods outside of the parser!
        parent->deprecatedParserAddChild(this);
    }

    // Add the renderer to the render tree
    if (renderer)
        parent->renderer()->addChild(renderer);
}

void TextControlInnerElement::detach()
{
    HTMLDivElement::detach();
    // FIXME: Remove once shadow DOM uses Element::setShadowRoot().
    if (shadowHost())
        setShadowHost(0);
}

// ----------------------------

inline TextControlInnerTextElement::TextControlInnerTextElement(Document* document, HTMLElement* shadowParent)
    : TextControlInnerElement(document, shadowParent)
{
}

PassRefPtr<TextControlInnerTextElement> TextControlInnerTextElement::create(Document* document, HTMLElement* shadowParent)
{
    return adoptRef(new TextControlInnerTextElement(document, shadowParent));
}

void TextControlInnerTextElement::defaultEventHandler(Event* event)
{
    // FIXME: In the future, we should add a way to have default event listeners.
    // Then we would add one to the text field's inner div, and we wouldn't need this subclass.
    // Or possibly we could just use a normal event listener.
    if (event->isBeforeTextInsertedEvent() || event->type() == eventNames().webkitEditableContentChangedEvent) {
        Node* shadowAncestor = shadowAncestorNode();
        // A TextControlInnerTextElement can be its own shadow ancestor if its been detached, but kept alive by an EditCommand.
        // In this case, an undo/redo can cause events to be sent to the TextControlInnerTextElement.  
        // To prevent an infinite loop, we must check for this case before sending the event up the chain.
        if (shadowAncestor && shadowAncestor != this)
            shadowAncestor->defaultEventHandler(event);
    }
    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

RenderObject* TextControlInnerTextElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    bool multiLine = false;
    Node* shadowAncestor = shadowAncestorNode();
    if (shadowAncestor && shadowAncestor->renderer()) {
        ASSERT(shadowAncestor->renderer()->isTextField() || shadowAncestor->renderer()->isTextArea());
        multiLine = shadowAncestor->renderer()->isTextArea();
    }
    return new (arena) RenderTextControlInnerBlock(this, multiLine);
}

// ----------------------------

inline SearchFieldResultsButtonElement::SearchFieldResultsButtonElement(Document* document)
    : TextControlInnerElement(document)
{
}

PassRefPtr<SearchFieldResultsButtonElement> SearchFieldResultsButtonElement::create(Document* document)
{
    return adoptRef(new SearchFieldResultsButtonElement(document));
}

void SearchFieldResultsButtonElement::defaultEventHandler(Event* event)
{
    // On mousedown, bring up a menu, if needed
    HTMLInputElement* input = static_cast<HTMLInputElement*>(shadowAncestorNode());
    if (event->type() == eventNames().mousedownEvent && event->isMouseEvent() && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        input->focus();
        input->select();
        RenderTextControlSingleLine* renderer = toRenderTextControlSingleLine(input->renderer());
        if (renderer->popupIsVisible())
            renderer->hidePopup();
        else if (input->maxResults() > 0)
            renderer->showPopup();
        event->setDefaultHandled();
    }

    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

// ----------------------------

inline SearchFieldCancelButtonElement::SearchFieldCancelButtonElement(Document* document)
    : TextControlInnerElement(document)
    , m_capturing(false)
{
}

PassRefPtr<SearchFieldCancelButtonElement> SearchFieldCancelButtonElement::create(Document* document)
{
    return adoptRef(new SearchFieldCancelButtonElement(document));
}

void SearchFieldCancelButtonElement::detach()
{
    if (m_capturing) {
        if (Frame* frame = document()->frame())
            frame->eventHandler()->setCapturingMouseEventsNode(0);
    }
    TextControlInnerElement::detach();
}


void SearchFieldCancelButtonElement::defaultEventHandler(Event* event)
{
    // If the element is visible, on mouseup, clear the value, and set selection
    RefPtr<HTMLInputElement> input(static_cast<HTMLInputElement*>(shadowAncestorNode()));
    if (event->type() == eventNames().mousedownEvent && event->isMouseEvent() && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        if (renderer() && renderer()->visibleToHitTesting()) {
            if (Frame* frame = document()->frame()) {
                frame->eventHandler()->setCapturingMouseEventsNode(this);
                m_capturing = true;
            }
        }
        input->focus();
        input->select();
        event->setDefaultHandled();
    }
    if (event->type() == eventNames().mouseupEvent && event->isMouseEvent() && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        if (m_capturing) {
            if (Frame* frame = document()->frame()) {
                frame->eventHandler()->setCapturingMouseEventsNode(0);
                m_capturing = false;
            }
            if (hovered()) {
                String oldValue = input->value();
                input->setValueForUser("");
                input->onSearch();
                event->setDefaultHandled();
            }
        }
    }

    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

// ----------------------------

inline SpinButtonElement::SpinButtonElement(HTMLElement* shadowParent)
    : TextControlInnerElement(shadowParent->document(), shadowParent)
    , m_capturing(false)
    , m_upDownState(Indeterminate)
    , m_pressStartingState(Indeterminate)
    , m_repeatingTimer(this, &SpinButtonElement::repeatingTimerFired)
{
}

PassRefPtr<SpinButtonElement> SpinButtonElement::create(HTMLElement* shadowParent)
{
    return adoptRef(new SpinButtonElement(shadowParent));
}

void SpinButtonElement::detach()
{
    stopRepeatingTimer();
    if (m_capturing) {
        if (Frame* frame = document()->frame()) {
            frame->eventHandler()->setCapturingMouseEventsNode(0);
            m_capturing = false;
        }
    }
    TextControlInnerElement::detach();
}

void SpinButtonElement::defaultEventHandler(Event* event)
{
    if (!event->isMouseEvent()) {
        if (!event->defaultHandled())
            HTMLDivElement::defaultEventHandler(event);
        return;
    }

    RenderBox* box = renderBox();
    if (!box) {
        if (!event->defaultHandled())
            HTMLDivElement::defaultEventHandler(event);
        return;
    }

    RefPtr<HTMLInputElement> input(static_cast<HTMLInputElement*>(shadowAncestorNode()));
    if (input->disabled() || input->isReadOnlyFormControl()) {
        if (!event->defaultHandled())
            HTMLDivElement::defaultEventHandler(event);
        return;
    }

    MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
    IntPoint local = roundedIntPoint(box->absoluteToLocal(mouseEvent->absoluteLocation(), false, true));
    if (mouseEvent->type() == eventNames().mousedownEvent && mouseEvent->button() == LeftButton) {
        if (box->borderBoxRect().contains(local)) {
            // The following functions of HTMLInputElement may run JavaScript
            // code which detaches this shadow node. We need to take a reference
            // and check renderer() after such function calls.
            RefPtr<Node> protector(this);
            input->focus();
            input->select();
            if (renderer()) {
                input->stepUpFromRenderer(m_upDownState == Up ? 1 : -1);
                if (renderer())
                    startRepeatingTimer();
            }
            event->setDefaultHandled();
        }
    } else if (mouseEvent->type() == eventNames().mouseupEvent && mouseEvent->button() == LeftButton)
        stopRepeatingTimer();
    else if (event->type() == eventNames().mousemoveEvent) {
        if (box->borderBoxRect().contains(local)) {
            if (!m_capturing) {
                if (Frame* frame = document()->frame()) {
                    frame->eventHandler()->setCapturingMouseEventsNode(this);
                    m_capturing = true;
                }
            }
            UpDownState oldUpDownState = m_upDownState;
            m_upDownState = local.y() < box->height() / 2 ? Up : Down;
            if (m_upDownState != oldUpDownState)
                renderer()->repaint();
        } else {
            if (m_capturing) {
                stopRepeatingTimer();
                if (Frame* frame = document()->frame()) {
                    frame->eventHandler()->setCapturingMouseEventsNode(0);
                    m_capturing = false;
                }
            }
        }
    }

    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

void SpinButtonElement::startRepeatingTimer()
{
    m_pressStartingState = m_upDownState;
    ScrollbarTheme* theme = ScrollbarTheme::nativeTheme();
    m_repeatingTimer.start(theme->initialAutoscrollTimerDelay(), theme->autoscrollTimerDelay());
}

void SpinButtonElement::stopRepeatingTimer()
{
    m_repeatingTimer.stop();
}

void SpinButtonElement::repeatingTimerFired(Timer<SpinButtonElement>*)
{
    HTMLInputElement* input = static_cast<HTMLInputElement*>(shadowAncestorNode());
    if (input->disabled() || input->isReadOnlyFormControl())
        return;
    // On Mac OS, NSStepper updates the value for the button under the mouse
    // cursor regardless of the button pressed at the beginning. So the
    // following check is not needed for Mac OS.
#if !OS(MAC_OS_X)
    if (m_upDownState != m_pressStartingState)
        return;
#endif
    input->stepUpFromRenderer(m_upDownState == Up ? 1 : -1);
}

void SpinButtonElement::setHovered(bool flag)
{
    if (!hovered() && flag)
        m_upDownState = Indeterminate;
    TextControlInnerElement::setHovered(flag);
}


// ----------------------------

#if ENABLE(INPUT_SPEECH)

inline InputFieldSpeechButtonElement::InputFieldSpeechButtonElement(HTMLElement* shadowParent)
    : TextControlInnerElement(shadowParent->document(), shadowParent)
    , m_capturing(false)
    , m_state(Idle)
    , m_listenerId(document()->page()->speechInput()->registerListener(this))
{
}

InputFieldSpeechButtonElement::~InputFieldSpeechButtonElement()
{
    SpeechInput* speech = speechInput();
    if (speech && m_listenerId)  { // Could be null when page is unloading.
        if (m_state != Idle)
            speech->cancelRecognition(m_listenerId);
        speech->unregisterListener(m_listenerId);
    }
}

PassRefPtr<InputFieldSpeechButtonElement> InputFieldSpeechButtonElement::create(HTMLElement* shadowParent)
{
    return adoptRef(new InputFieldSpeechButtonElement(shadowParent));
}

void InputFieldSpeechButtonElement::defaultEventHandler(Event* event)
{
    // For privacy reasons, only allow clicks directly coming from the user.
    if (!event->fromUserGesture()) {
        HTMLDivElement::defaultEventHandler(event);
        return;
    }

    // The call to focus() below dispatches a focus event, and an event handler in the page might
    // remove the input element from DOM. To make sure it remains valid until we finish our work
    // here, we take a temporary reference.
    RefPtr<HTMLInputElement> input(static_cast<HTMLInputElement*>(shadowAncestorNode()));

    if (input->disabled() || input->isReadOnlyFormControl()) {
        if (!event->defaultHandled())
            HTMLDivElement::defaultEventHandler(event);
        return;
    }

    // On mouse down, select the text and set focus.
    if (event->type() == eventNames().mousedownEvent && event->isMouseEvent() && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        if (renderer() && renderer()->visibleToHitTesting()) {
            if (Frame* frame = document()->frame()) {
                frame->eventHandler()->setCapturingMouseEventsNode(this);
                m_capturing = true;
            }
        }
        RefPtr<InputFieldSpeechButtonElement> holdRefButton(this);
        input->focus();
        input->select();
        event->setDefaultHandled();
    }
    // On mouse up, release capture cleanly.
    if (event->type() == eventNames().mouseupEvent && event->isMouseEvent() && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        if (m_capturing && renderer() && renderer()->visibleToHitTesting()) {
            if (Frame* frame = document()->frame()) {
                frame->eventHandler()->setCapturingMouseEventsNode(0);
                m_capturing = false;
            }
        }
    }

    if (event->type() == eventNames().clickEvent) {
        switch (m_state) {
        case Idle: {
              AtomicString language = input->computeInheritedLanguage();
              String grammar = input->getAttribute(webkitgrammarAttr);
              IntRect rect = input->renderer()->absoluteBoundingBoxRect();
              if (speechInput()->startRecognition(m_listenerId, rect, language, grammar, document()->securityOrigin()))
                  setState(Recording);
            }
            break;
        case Recording:
            speechInput()->stopRecording(m_listenerId);
            break;
        case Recognizing:
            // Nothing to do here, we will continue to wait for results.
            break;
        }
        event->setDefaultHandled();
    }

    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

void InputFieldSpeechButtonElement::setState(SpeechInputState state)
{
    if (m_state != state) {
        m_state = state;
        shadowAncestorNode()->renderer()->repaint();
    }
}

SpeechInput* InputFieldSpeechButtonElement::speechInput()
{
    return document()->page() ? document()->page()->speechInput() : 0;
}

void InputFieldSpeechButtonElement::didCompleteRecording(int)
{
    setState(Recognizing);
}

void InputFieldSpeechButtonElement::didCompleteRecognition(int)
{
    setState(Idle);
}

void InputFieldSpeechButtonElement::setRecognitionResult(int, const SpeechInputResultArray& results)
{
    m_results = results;

    // The call to setValue() below dispatches an event, and an event handler in the page might
    // remove the input element from DOM. To make sure it remains valid until we finish our work
    // here, we take a temporary reference.
    RefPtr<HTMLInputElement> input(static_cast<HTMLInputElement*>(shadowAncestorNode()));
    if (input->disabled() || input->isReadOnlyFormControl())
        return;

    RefPtr<InputFieldSpeechButtonElement> holdRefButton(this);
    input->setValue(results.isEmpty() ? "" : results[0]->utterance());
    input->dispatchEvent(SpeechInputEvent::create(eventNames().webkitspeechchangeEvent, results));

    // Check before accessing the renderer as the above event could have potentially turned off
    // speech in the input element, hence removing this button and renderer from the hierarchy.
    if (renderer())
        renderer()->repaint();
}

void InputFieldSpeechButtonElement::detach()
{
    if (m_capturing) {
        if (Frame* frame = document()->frame())
            frame->eventHandler()->setCapturingMouseEventsNode(0);
    }

    if (m_listenerId) {
        if (m_state != Idle)
            speechInput()->cancelRecognition(m_listenerId);
        speechInput()->unregisterListener(m_listenerId);
        m_listenerId = 0;
    }

    TextControlInnerElement::detach();
}

#endif // ENABLE(INPUT_SPEECH)

}
