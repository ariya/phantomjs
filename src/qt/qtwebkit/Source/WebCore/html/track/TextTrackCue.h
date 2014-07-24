/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 * Copyright (C) 2012, 2013 Apple Inc.  All rights reserved.
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

#ifndef TextTrackCue_h
#define TextTrackCue_h

#if ENABLE(VIDEO_TRACK)

#include "EventTarget.h"
#include "HTMLElement.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class DocumentFragment;
class HTMLSpanElement;
class ScriptExecutionContext;
class TextTrack;
class TextTrackCue;

// ----------------------------

class TextTrackCueBox : public HTMLElement {
public:
    static PassRefPtr<TextTrackCueBox> create(Document* document, TextTrackCue* cue)
    {
        return adoptRef(new TextTrackCueBox(document, cue));
    }

    TextTrackCue* getCue() const;
    virtual void applyCSSProperties(const IntSize& videoSize);

    static const AtomicString& textTrackCueBoxShadowPseudoId();

protected:
    TextTrackCueBox(Document*, TextTrackCue*);

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*) OVERRIDE;

    TextTrackCue* m_cue;
};

// ----------------------------

class TextTrackCue : public RefCounted<TextTrackCue>, public EventTarget {
public:
    static PassRefPtr<TextTrackCue> create(ScriptExecutionContext* context, double start, double end, const String& content)
    {
        return adoptRef(new TextTrackCue(context, start, end, content));
    }

    static const AtomicString& cueShadowPseudoId()
    {
        DEFINE_STATIC_LOCAL(const AtomicString, cue, ("cue", AtomicString::ConstructFromLiteral));
        return cue;
    }

    virtual ~TextTrackCue();

    TextTrack* track() const;
    void setTrack(TextTrack*);

    const String& id() const { return m_id; }
    void setId(const String&);

    double startTime() const { return m_startTime; }
    void setStartTime(double, ExceptionCode&);

    double endTime() const { return m_endTime; }
    void setEndTime(double, ExceptionCode&);

    bool pauseOnExit() const { return m_pauseOnExit; }
    void setPauseOnExit(bool);

    const String& vertical() const;
    void setVertical(const String&, ExceptionCode&);

    bool snapToLines() const { return m_snapToLines; }
    void setSnapToLines(bool);

    int line() const { return m_linePosition; }
    virtual void setLine(int, ExceptionCode&);

    int position() const { return m_textPosition; }
    virtual void setPosition(int, ExceptionCode&);

    int size() const { return m_cueSize; }
    virtual void setSize(int, ExceptionCode&);

    const String& align() const;
    void setAlign(const String&, ExceptionCode&);

    const String& text() const { return m_content; }
    void setText(const String&);

    const String& cueSettings() const { return m_settings; }
    void setCueSettings(const String&);

    int cueIndex();
    void invalidateCueIndex();

    PassRefPtr<DocumentFragment> getCueAsHTML();
    PassRefPtr<DocumentFragment> createCueRenderingTree();

    using EventTarget::dispatchEvent;
    virtual bool dispatchEvent(PassRefPtr<Event>) OVERRIDE;

#if ENABLE(WEBVTT_REGIONS)
    const String& regionId() const { return m_regionId; }
    void setRegionId(const String&);
#endif

    bool isActive();
    void setIsActive(bool);

    bool hasDisplayTree() const { return m_displayTree; }
    TextTrackCueBox* getDisplayTree(const IntSize& videoSize);
    HTMLSpanElement* element() const { return m_cueBackgroundBox.get(); }

    void updateDisplayTree(double);
    void removeDisplayTree();
    void markFutureAndPastNodes(ContainerNode*, double, double);

    int calculateComputedLinePosition();
    std::pair<double, double> getPositionCoordinates() const;

    virtual const AtomicString& interfaceName() const;
    virtual ScriptExecutionContext* scriptExecutionContext() const;

    std::pair<double, double> getCSSPosition() const;

    int getCSSSize() const;
    CSSValueID getCSSWritingDirection() const;
    CSSValueID getCSSWritingMode() const;

    enum WritingDirection {
        Horizontal,
        VerticalGrowingLeft,
        VerticalGrowingRight,
        NumberOfWritingDirections
    };
    WritingDirection getWritingDirection() const { return m_writingDirection; }

    enum CueAlignment {
        Start,
        Middle,
        End
    };
    CueAlignment getAlignment() const { return m_cueAlignment; }

    virtual void setFontSize(int, const IntSize&, bool important);

    enum CueMatchRules {
        MatchAllFields,
        IgnoreDuration,
    };
    virtual bool isEqual(const TextTrackCue&, CueMatchRules) const;

    virtual bool isOrderedBefore(const TextTrackCue*) const;

    enum CueType {
        Generic,
        WebVTT
    };
    virtual CueType cueType() const { return WebVTT; }

    void willChange();
    void didChange();
    
    DEFINE_ATTRIBUTE_EVENT_LISTENER(enter);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(exit);

    using RefCounted<TextTrackCue>::ref;
    using RefCounted<TextTrackCue>::deref;

protected:
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();

    TextTrackCue(ScriptExecutionContext*, double start, double end, const String& content);

    Document* ownerDocument() { return toDocument(m_scriptExecutionContext); }

    virtual PassRefPtr<TextTrackCueBox> createDisplayTree();
    TextTrackCueBox* displayTreeInternal();

private:
    void createWebVTTNodeTree();
    void copyWebVTTNodeToDOMTree(ContainerNode* WebVTTNode, ContainerNode* root);

    void parseSettings(const String&);

    void determineTextDirection();
    void calculateDisplayParameters();

    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }

    enum CueSetting {
        None,
        Vertical,
        Line,
        Position,
        Size,
        Align,
#if ENABLE(WEBVTT_REGIONS)
        RegionId
#endif
    };
    CueSetting settingName(const String&);

    String m_id;
    double m_startTime;
    double m_endTime;
    String m_content;
    String m_settings;
    int m_linePosition;
    int m_computedLinePosition;
    int m_textPosition;
    int m_cueSize;
    int m_cueIndex;
    int m_processingCueChanges;

    WritingDirection m_writingDirection;

    CueAlignment m_cueAlignment;

    RefPtr<DocumentFragment> m_webVTTNodeTree;
    TextTrack* m_track;

    EventTargetData m_eventTargetData;
    ScriptExecutionContext* m_scriptExecutionContext;

    bool m_isActive;
    bool m_pauseOnExit;
    bool m_snapToLines;

    RefPtr<HTMLSpanElement> m_cueBackgroundBox;

    bool m_displayTreeShouldChange;
    RefPtr<TextTrackCueBox> m_displayTree;

    CSSValueID m_displayDirection;

    CSSValueID m_displayWritingModeMap[NumberOfWritingDirections];
    CSSValueID m_displayWritingMode;

    int m_displaySize;

    std::pair<float, float> m_displayPosition;
#if ENABLE(WEBVTT_REGIONS)
    String m_regionId;
#endif
};

} // namespace WebCore

#endif
#endif
