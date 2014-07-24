/*
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

#include "config.h"
#if ENABLE(INPUT_SPEECH)
#include "RenderInputSpeech.h"

#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "PaintInfo.h"
#include "RenderBox.h"
#include "TextControlInnerElements.h"

namespace WebCore {

static const float defaultControlFontPixelSize = 13;
static const float defaultSpeechButtonSize = 16;
static const float minSpeechButtonSize = 8;
static const float maxSpeechButtonSize = 40;

void RenderInputSpeech::adjustInputFieldSpeechButtonStyle(StyleResolver*, RenderStyle* style, Element*)
{
    // Scale the button size based on the font size.
    float fontScale = style->fontSize() / defaultControlFontPixelSize;
    int speechButtonSize = lroundf(std::min(std::max(minSpeechButtonSize, defaultSpeechButtonSize * fontScale), maxSpeechButtonSize));
    style->setWidth(Length(speechButtonSize, Fixed));
    style->setHeight(Length(speechButtonSize, Fixed));
}

bool RenderInputSpeech::paintInputFieldSpeechButton(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
    Element* element = object->node()->isElementNode() ? toElement(object->node()) : 0;
    if (!element || !element->isInputFieldSpeechButtonElement())
        return false;

    // Get the renderer of <input> element.
    Node* input = object->node()->shadowHost();
    if (!input->renderer()->isBox())
        return false;
    RenderBox* inputRenderBox = toRenderBox(input->renderer());
    LayoutRect inputContentBox = inputRenderBox->contentBoxRect();

    // Make sure the scaled button stays square and will fit in its parent's box.
    LayoutUnit buttonSize = std::min(inputContentBox.width(), std::min<LayoutUnit>(inputContentBox.height(), rect.height()));
    // Calculate button's coordinates relative to the input element.
    // Center the button vertically.  Round up though, so if it has to be one pixel off-center, it will
    // be one pixel closer to the bottom of the field.  This tends to look better with the text.
    LayoutRect buttonRect(object->offsetFromAncestorContainer(inputRenderBox).width(),
                          inputContentBox.y() + (inputContentBox.height() - buttonSize + 1) / 2,
                          buttonSize, buttonSize);

    // Compute an offset between the part renderer and the input renderer.
    LayoutSize offsetFromInputRenderer = -(object->offsetFromAncestorContainer(inputRenderBox));
    // Move the rect into partRenderer's coords.
    buttonRect.move(offsetFromInputRenderer);
    // Account for the local drawing offset.
    buttonRect.moveBy(rect.location());

    DEFINE_STATIC_LOCAL(RefPtr<Image>, imageStateNormal, (Image::loadPlatformResource("inputSpeech")));
    DEFINE_STATIC_LOCAL(RefPtr<Image>, imageStateRecording, (Image::loadPlatformResource("inputSpeechRecording")));
    DEFINE_STATIC_LOCAL(RefPtr<Image>, imageStateWaiting, (Image::loadPlatformResource("inputSpeechWaiting")));

    InputFieldSpeechButtonElement* speechButton = toInputFieldSpeechButtonElement(element);
    Image* image = imageStateNormal.get();
    if (speechButton->state() == InputFieldSpeechButtonElement::Recording)
        image = imageStateRecording.get();
    else if (speechButton->state() == InputFieldSpeechButtonElement::Recognizing)
        image = imageStateWaiting.get();
    paintInfo.context->drawImage(image, object->style()->colorSpace(), pixelSnappedIntRect(buttonRect));

    return false;
}

} // namespace WebCore

#endif // ENABLE(INPUT_SPEECH)
