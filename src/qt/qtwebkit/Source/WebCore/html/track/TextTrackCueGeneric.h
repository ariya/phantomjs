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

#ifndef TextTrackCueGeneric_h
#define TextTrackCueGeneric_h

#if ENABLE(VIDEO_TRACK)

#include "Color.h"
#include "TextTrackCue.h"

namespace WebCore {

class GenericCueData;

// A "generic" cue is a non-WebVTT cue, so it is not positioned/sized with the WebVTT logic.
class TextTrackCueGeneric : public TextTrackCue {
public:
    static PassRefPtr<TextTrackCueGeneric> create(ScriptExecutionContext* context, double start, double end, const String& content)
    {
        return adoptRef(new TextTrackCueGeneric(context, start, end, content));
    }
    
    virtual ~TextTrackCueGeneric() { }

    virtual PassRefPtr<TextTrackCueBox> createDisplayTree() OVERRIDE;

    virtual void setLine(int, ExceptionCode&) OVERRIDE;
    virtual void setPosition(int, ExceptionCode&) OVERRIDE;

    bool useDefaultPosition() const { return m_defaultPosition; }
    
    double baseFontSizeRelativeToVideoHeight() const { return m_baseFontSizeRelativeToVideoHeight; }
    void setBaseFontSizeRelativeToVideoHeight(double size) { m_baseFontSizeRelativeToVideoHeight = size; }

    double fontSizeMultiplier() const { return m_fontSizeMultiplier; }
    void setFontSizeMultiplier(double size) { m_fontSizeMultiplier = size; }

    String fontName() const { return m_fontName; }
    void setFontName(String name) { m_fontName = name; }

    Color foregroundColor() const { return m_foregroundColor; }
    void setForegroundColor(RGBA32 color) { m_foregroundColor.setRGB(color); }
    
    Color backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(RGBA32 color) { m_backgroundColor.setRGB(color); }
    
    Color highlightColor() const { return m_highlightColor; }
    void setHighlightColor(RGBA32 color) { m_highlightColor.setRGB(color); }
    
    virtual void setFontSize(int, const IntSize&, bool important) OVERRIDE;

    virtual bool isEqual(const TextTrackCue&, CueMatchRules) const OVERRIDE;

    virtual TextTrackCue::CueType cueType() const OVERRIDE { return TextTrackCue::Generic; }

private:
    virtual bool isOrderedBefore(const TextTrackCue*) const OVERRIDE;

    TextTrackCueGeneric(ScriptExecutionContext*, double start, double end, const String&);
    
    Color m_foregroundColor;
    Color m_backgroundColor;
    Color m_highlightColor;
    double m_baseFontSizeRelativeToVideoHeight;
    double m_fontSizeMultiplier;
    String m_fontName;
    bool m_defaultPosition;
};

} // namespace WebCore

#endif
#endif
