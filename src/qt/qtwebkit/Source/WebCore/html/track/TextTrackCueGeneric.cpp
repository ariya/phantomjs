/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#if ENABLE(VIDEO_TRACK)

#include "TextTrackCueGeneric.h"

#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "HTMLNames.h"
#include "HTMLSpanElement.h"
#include "InbandTextTrackPrivateClient.h"
#include "Logging.h"
#include "RenderObject.h"
#include "RenderTextTrackCue.h"
#include "ScriptExecutionContext.h"
#include "TextTrackCue.h"

namespace WebCore {

class TextTrackCueGenericBoxElement FINAL : public TextTrackCueBox {
public:
    static PassRefPtr<TextTrackCueGenericBoxElement> create(Document* document, TextTrackCueGeneric* cue)
    {
        return adoptRef(new TextTrackCueGenericBoxElement(document, cue));
    }
    
    virtual void applyCSSProperties(const IntSize&) OVERRIDE;
    
private:
    TextTrackCueGenericBoxElement(Document*, TextTrackCue*);
};

TextTrackCueGenericBoxElement::TextTrackCueGenericBoxElement(Document* document, TextTrackCue* cue)
    : TextTrackCueBox(document, cue)
{
}

void TextTrackCueGenericBoxElement::applyCSSProperties(const IntSize& videoSize)
{
    setInlineStyleProperty(CSSPropertyPosition, CSSValueAbsolute);
    setInlineStyleProperty(CSSPropertyUnicodeBidi, CSSValueWebkitPlaintext);
    
    TextTrackCueGeneric* cue = static_cast<TextTrackCueGeneric*>(getCue());
    RefPtr<HTMLSpanElement> cueElement = cue->element();

    float size = static_cast<float>(cue->getCSSSize());
    if (cue->useDefaultPosition()) {
        setInlineStyleProperty(CSSPropertyBottom, "0");
        setInlineStyleProperty(CSSPropertyMarginBottom, 1.0, CSSPrimitiveValue::CSS_PERCENTAGE);
    } else {
        setInlineStyleProperty(CSSPropertyLeft, static_cast<float>(cue->position()), CSSPrimitiveValue::CSS_PERCENTAGE);
        setInlineStyleProperty(CSSPropertyTop, static_cast<float>(cue->line()), CSSPrimitiveValue::CSS_PERCENTAGE);

        if (cue->getWritingDirection() == TextTrackCue::Horizontal)
            setInlineStyleProperty(CSSPropertyWidth, size, CSSPrimitiveValue::CSS_PERCENTAGE);
        else
            setInlineStyleProperty(CSSPropertyHeight, size,  CSSPrimitiveValue::CSS_PERCENTAGE);
    }

    if (cue->foregroundColor().isValid())
        cueElement->setInlineStyleProperty(CSSPropertyColor, cue->foregroundColor().serialized());
    if (cue->highlightColor().isValid())
        cueElement->setInlineStyleProperty(CSSPropertyBackgroundColor, cue->highlightColor().serialized());

    if (cue->getWritingDirection() == TextTrackCue::Horizontal)
        setInlineStyleProperty(CSSPropertyHeight, CSSValueAuto);
    else
        setInlineStyleProperty(CSSPropertyWidth, CSSValueAuto);

    if (cue->baseFontSizeRelativeToVideoHeight())
        cue->setFontSize(cue->baseFontSizeRelativeToVideoHeight(), videoSize, false);

    if (cue->getAlignment() == TextTrackCue::Middle)
        setInlineStyleProperty(CSSPropertyTextAlign, CSSValueCenter);
    else if (cue->getAlignment() == TextTrackCue::End)
        setInlineStyleProperty(CSSPropertyTextAlign, CSSValueEnd);
    else
        setInlineStyleProperty(CSSPropertyTextAlign, CSSValueStart);

    if (cue->backgroundColor().isValid())
        setInlineStyleProperty(CSSPropertyBackgroundColor, cue->backgroundColor().serialized());
    setInlineStyleProperty(CSSPropertyWebkitWritingMode, cue->getCSSWritingMode(), false);
    setInlineStyleProperty(CSSPropertyWhiteSpace, CSSValuePreWrap);
    setInlineStyleProperty(CSSPropertyWordBreak, CSSValueNormal);
}

TextTrackCueGeneric::TextTrackCueGeneric(ScriptExecutionContext* context, double start, double end, const String& content)
    : TextTrackCue(context, start, end, content)
    , m_baseFontSizeRelativeToVideoHeight(0)
    , m_fontSizeMultiplier(0)
    , m_defaultPosition(true)
{
}

PassRefPtr<TextTrackCueBox> TextTrackCueGeneric::createDisplayTree()
{
    return TextTrackCueGenericBoxElement::create(ownerDocument(), this);
}

void TextTrackCueGeneric::setLine(int line, ExceptionCode& ec)
{
    m_defaultPosition = false;
    TextTrackCue::setLine(line, ec);
}

void TextTrackCueGeneric::setPosition(int position, ExceptionCode& ec)
{
    m_defaultPosition = false;
    TextTrackCue::setPosition(position, ec);
}

void TextTrackCueGeneric::setFontSize(int fontSize, const IntSize& videoSize, bool important)
{
    if (!hasDisplayTree() || !fontSize)
        return;
    
    if (important || !baseFontSizeRelativeToVideoHeight()) {
        TextTrackCue::setFontSize(fontSize, videoSize, important);
        return;
    }

    double size = videoSize.height() * baseFontSizeRelativeToVideoHeight() / 100;
    if (fontSizeMultiplier())
        size *= fontSizeMultiplier() / 100;
    displayTreeInternal()->setInlineStyleProperty(CSSPropertyFontSize, String::number(lround(size)) + "px");

    LOG(Media, "TextTrackCueGeneric::setFontSize - setting cue font size to %li", lround(size));
}
    
bool TextTrackCueGeneric::isEqual(const TextTrackCue& cue, TextTrackCue::CueMatchRules match) const
{
    if (cue.cueType() != TextTrackCue::Generic)
        return false;

    const TextTrackCueGeneric* other = static_cast<const TextTrackCueGeneric*>(&cue);

    if (m_baseFontSizeRelativeToVideoHeight != other->baseFontSizeRelativeToVideoHeight())
        return false;
    if (m_fontSizeMultiplier != other->fontSizeMultiplier())
        return false;
    if (m_fontName != other->fontName())
        return false;
    if (m_foregroundColor != other->foregroundColor())
        return false;
    if (m_backgroundColor != other->backgroundColor())
        return false;

    return TextTrackCue::isEqual(cue, match);
}

bool TextTrackCueGeneric::isOrderedBefore(const TextTrackCue* that) const
{
    if (TextTrackCue::isOrderedBefore(that))
        return true;

    if (that->cueType() == Generic && startTime() == that->startTime() && endTime() == that->endTime()) {
        // Further order generic cues by their calculated line value.
        std::pair<double, double> thisPosition = getPositionCoordinates();
        std::pair<double, double> thatPosition = that->getPositionCoordinates();
        return thisPosition.second > thatPosition.second || (thisPosition.second == thatPosition.second && thisPosition.first < thatPosition.first);
    }

    return false;
}
    
} // namespace WebCore

#endif
