/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef InbandTextTrackPrivateClient_h
#define InbandTextTrackPrivateClient_h

#include "Color.h"
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

#if ENABLE(VIDEO_TRACK)

namespace WebCore {

class InbandTextTrackPrivate;

class GenericCueData : public RefCounted<GenericCueData> {
public:

    static PassRefPtr<GenericCueData> create() { return adoptRef(new GenericCueData()); }
    virtual ~GenericCueData() { }

    double startTime() const { return m_startTime; }
    void setStartTime(double startTime) { m_startTime = startTime; }

    double endTime() const { return m_endTime; }
    void setEndTime(double endTime) { m_endTime = endTime; }

    String id() const { return m_id; }
    void setId(String id) { m_id = id; }

    String content() const { return m_content; }
    void setContent(String content) { m_content = content; }

    double line() const { return m_line; }
    void setLine(double line) { m_line = line; }

    double position() const { return m_position; }
    void setPosition(double position) { m_position = position; }

    double size() const { return m_size; }
    void setSize(double size) { m_size = size; }

    enum Alignment {
        None,
        Start,
        Middle,
        End
    };
    Alignment align() const { return m_align; }
    void setAlign(Alignment align) { m_align = align; }

    String fontName() const { return m_fontName; }
    void setFontName(String fontName) { m_fontName = fontName; }

    double baseFontSize() const { return m_baseFontSize; }
    void setBaseFontSize(double baseFontSize) { m_baseFontSize = baseFontSize; }

    double relativeFontSize() const { return m_relativeFontSize; }
    void setRelativeFontSize(double relativeFontSize) { m_relativeFontSize = relativeFontSize; }

    Color foregroundColor() const { return m_foregroundColor; }
    void setForegroundColor(RGBA32 color) { m_foregroundColor.setRGB(color); }

    Color backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(RGBA32 color) { m_backgroundColor.setRGB(color); }
    
    Color highlightColor() const { return m_highlightColor; }
    void setHighlightColor(RGBA32 color) { m_highlightColor.setRGB(color); }
    
    enum Status {
        Uninitialized,
        Partial,
        Complete,
    };
    Status status() { return m_status; }
    void setStatus(Status status) { m_status = status; }
    
private:
    GenericCueData()
        : m_startTime(0)
        , m_endTime(0)
        , m_line(-1)
        , m_position(-1)
        , m_size(-1)
        , m_align(None)
        , m_baseFontSize(0)
        , m_relativeFontSize(0)
        , m_status(Uninitialized)
    {
    }

    double m_startTime;
    double m_endTime;
    String m_id;
    String m_content;
    double m_line;
    double m_position;
    double m_size;
    Alignment m_align;
    String m_fontName;
    double m_baseFontSize;
    double m_relativeFontSize;
    Color m_foregroundColor;
    Color m_backgroundColor;
    Color m_highlightColor;
    Status m_status;
};

class WebVTTCueData;

class InbandTextTrackPrivateClient {
public:
    virtual ~InbandTextTrackPrivateClient() { }

    virtual void addGenericCue(InbandTextTrackPrivate*, PassRefPtr<GenericCueData>) = 0;
    virtual void updateGenericCue(InbandTextTrackPrivate*, GenericCueData*) = 0;
    virtual void removeGenericCue(InbandTextTrackPrivate*, GenericCueData*) = 0;

    virtual void addWebVTTCue(InbandTextTrackPrivate*, PassRefPtr<WebVTTCueData>) = 0;
    virtual void removeWebVTTCue(InbandTextTrackPrivate*, WebVTTCueData*) = 0;

    virtual void willRemoveTextTrackPrivate(InbandTextTrackPrivate*) = 0;
};

} // namespace WebCore

#endif
#endif
