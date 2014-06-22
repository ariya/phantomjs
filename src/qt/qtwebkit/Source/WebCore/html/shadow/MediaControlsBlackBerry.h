/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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

#ifndef MediaControlsBlackBerry_h
#define MediaControlsBlackBerry_h

#if ENABLE(VIDEO)

#include "MediaControlElementTypes.h"
#include "MediaControlElements.h"
#include "MediaControls.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class HTMLInputElement;
class HTMLMediaElement;
class Event;
class MediaPlayer;

class RenderBox;
class RenderMedia;

#if ENABLE(VIDEO_TRACK)
class MediaControlTextTrackContainerElement;
class MediaControlTextTrackDisplayElement;
#endif

class MediaControlButtonGroupContainerElement : public MediaControlDivElement {
public:
    static PassRefPtr<MediaControlButtonGroupContainerElement> create(Document*);

protected:
    MediaControlButtonGroupContainerElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

class MediaControlTimeDisplayContainerElement : public MediaControlDivElement {
public:
    static PassRefPtr<MediaControlTimeDisplayContainerElement> create(Document*);

protected:
    MediaControlTimeDisplayContainerElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

class MediaControlEmbeddedPanelElement : public MediaControlDivElement {
public:
    static PassRefPtr<MediaControlEmbeddedPanelElement> create(Document*);

    void setCanBeDragged(bool);
    void setIsDisplayed(bool);

    void resetPosition();
    void makeOpaque();
    void makeTransparent();

    virtual bool willRespondToMouseMoveEvents() OVERRIDE { return true; }
    virtual bool willRespondToMouseClickEvents() OVERRIDE { return true; }

private:
    explicit MediaControlEmbeddedPanelElement(Document*);

    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
    virtual void defaultEventHandler(Event*) OVERRIDE;

    void startDrag(const LayoutPoint& eventLocation);
    void continueDrag(const LayoutPoint& eventLocation);
    void endDrag();

    void startTimer();
    void stopTimer();
    void transitionTimerFired(Timer<MediaControlEmbeddedPanelElement>*);

    void setPosition(const LayoutPoint&);

    bool m_canBeDragged;
    bool m_isBeingDragged;
    bool m_isDisplayed;
    bool m_opaque;
    LayoutPoint m_lastDragEventLocation;
    LayoutPoint m_cumulativeDragOffset;

    Timer<MediaControlEmbeddedPanelElement> m_transitionTimer;
};

class MediaControlFullscreenTimeDisplayContainerElement : public MediaControlDivElement {
public:
    static PassRefPtr<MediaControlFullscreenTimeDisplayContainerElement> create(Document*);

protected:
    MediaControlFullscreenTimeDisplayContainerElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

class MediaControlFullscreenButtonContainerElement : public MediaControlDivElement {
public:
    static PassRefPtr<MediaControlFullscreenButtonContainerElement> create(Document*);

private:
    MediaControlFullscreenButtonContainerElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

class MediaControlFullscreenButtonDividerElement : public MediaControlDivElement {
public:
    static PassRefPtr<MediaControlFullscreenButtonDividerElement> create(Document*);

private:
    MediaControlFullscreenButtonDividerElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

class MediaControlPlayButtonContainerElement : public MediaControlDivElement {
public:
    static PassRefPtr<MediaControlPlayButtonContainerElement> create(Document*);

private:
    MediaControlPlayButtonContainerElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

class MediaControlPlaceholderElement : public MediaControlDivElement {
public:
    static PassRefPtr<MediaControlPlaceholderElement> create(Document*);

private:
    MediaControlPlaceholderElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

class MediaControlFullscreenPlayButtonElement : public MediaControlInputElement {
public:
    static PassRefPtr<MediaControlFullscreenPlayButtonElement> create(Document*);

    virtual bool willRespondToMouseClickEvents() OVERRIDE { return true; }
    virtual void updateDisplayType() OVERRIDE;

private:
    MediaControlFullscreenPlayButtonElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
    virtual void defaultEventHandler(Event*) OVERRIDE;
};

class MediaControlFullscreenFullscreenButtonElement : public MediaControlInputElement {
public:
    static PassRefPtr<MediaControlFullscreenFullscreenButtonElement> create(Document*);

    virtual bool willRespondToMouseClickEvents() OVERRIDE { return true; }
    void setIsFullscreen(bool);

private:
    MediaControlFullscreenFullscreenButtonElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
    virtual void defaultEventHandler(Event*) OVERRIDE;
};

class MediaControlFullscreenTimelineContainerElement : public MediaControlDivElement {
public:
    static PassRefPtr<MediaControlFullscreenTimelineContainerElement> create(Document*);

private:
    MediaControlFullscreenTimelineContainerElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

class MediaControlFullscreenTimelineElement : public MediaControlInputElement {
public:
    static PassRefPtr<MediaControlFullscreenTimelineElement> create(Document*, MediaControls*);

    virtual bool willRespondToMouseClickEvents() OVERRIDE;
    void setPosition(double);
    void setDuration(double);

private:
    MediaControlFullscreenTimelineElement(Document*, MediaControls*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
    virtual void defaultEventHandler(Event*) OVERRIDE;

    MediaControls* m_controls;
};

class MediaControlFullscreenTimeRemainingDisplayElement : public MediaControlTimeDisplayElement {
public:
    static PassRefPtr<MediaControlFullscreenTimeRemainingDisplayElement> create(Document*);

private:
    MediaControlFullscreenTimeRemainingDisplayElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

class MediaControlFullscreenCurrentTimeDisplayElement : public MediaControlTimeDisplayElement {
public:
    static PassRefPtr<MediaControlFullscreenCurrentTimeDisplayElement> create(Document*);

private:
    MediaControlFullscreenCurrentTimeDisplayElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

class MediaControlAudioMuteButtonElement : public MediaControlMuteButtonElement {
public:
    static PassRefPtr<MediaControlAudioMuteButtonElement> create(Document*, MediaControls*);

    virtual bool willRespondToMouseMoveEvents() OVERRIDE { return true; }

private:
    explicit MediaControlAudioMuteButtonElement(Document*, MediaControls*);

    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
    virtual void defaultEventHandler(Event*) OVERRIDE;

    MediaControls* m_controls;
};

class MediaControlsBlackBerry : public MediaControls {
public:
    static PassRefPtr<MediaControlsBlackBerry> createControls(Document*);

    // MediaControls implementation.
    void setMediaController(MediaControllerInterface*) OVERRIDE;

    void show() OVERRIDE;
    void hide() OVERRIDE;
    void makeOpaque() OVERRIDE;
    void makeTransparent() OVERRIDE;

    void reset() OVERRIDE;

    void bufferingProgressed() OVERRIDE;
    void playbackProgressed() OVERRIDE;
    void playbackStarted() OVERRIDE;
    void playbackStopped() OVERRIDE;

    void changedMute() OVERRIDE;

    void enteredFullscreen() OVERRIDE;
    void exitedFullscreen() OVERRIDE;

    void reportedError() OVERRIDE;

    void showVolumeSlider() OVERRIDE;
    void toggleVolumeSlider();
    void updateCurrentTimeDisplay() OVERRIDE;

    virtual bool shouldHideControls() OVERRIDE;

private:
    MediaControlsBlackBerry(Document*);

    MediaControlButtonGroupContainerElement* m_buttonContainer;
    MediaControlTimeDisplayContainerElement* m_timeDisplayContainer;
    MediaControlFullscreenTimeDisplayContainerElement* m_fullscreenTimeDisplayContainer;
    MediaControlFullscreenPlayButtonElement* m_fullscreenPlayButton;
    MediaControlFullscreenCurrentTimeDisplayElement* m_fullscreenCurrentTimeDisplay;
    MediaControlFullscreenTimelineElement* m_fullscreenTimeline;
    MediaControlTimeRemainingDisplayElement* m_timeRemainingDisplay;
    MediaControlFullscreenTimeRemainingDisplayElement* m_fullscreenTimeRemainingDisplay;
    MediaControlTimelineContainerElement* m_timelineContainer;
    MediaControlFullscreenTimelineContainerElement* m_fullscreenTimelineContainer;
    MediaControlFullscreenButtonDividerElement* m_fullScreenDivider;
    MediaControlFullscreenFullscreenButtonElement* m_fullscreenFullScreenButton;
    MediaControlAudioMuteButtonElement* m_muteButton;
    MediaControlVolumeSliderContainerElement* m_volumeSliderContainer;
    MediaControlEmbeddedPanelElement* m_embeddedPanel;
    MediaControlFullscreenButtonContainerElement* m_fullScreenButtonContainer;
    MediaControlPlayButtonContainerElement* m_playButtonContainer;
    MediaControlPlaceholderElement* m_placeholder;
};

}

#endif
#endif
