/*
 * Copyright (C) 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef MediaControlElementTypes_h
#define MediaControlElementTypes_h

#if ENABLE(VIDEO)

#include "HTMLDivElement.h"
#include "HTMLInputElement.h"
#include "HTMLMediaElement.h"
#include "MediaControllerInterface.h"
#include "RenderBlock.h"

namespace WebCore {

// Must match WebKitSystemInterface.h
enum MediaControlElementType {
    MediaEnterFullscreenButton = 0,
    MediaMuteButton,
    MediaPlayButton,
    MediaSeekBackButton,
    MediaSeekForwardButton,
    MediaSlider,
    MediaSliderThumb,
    MediaRewindButton,
    MediaReturnToRealtimeButton,
    MediaShowClosedCaptionsButton,
    MediaHideClosedCaptionsButton,
    MediaUnMuteButton,
    MediaPauseButton,
    MediaTimelineContainer,
    MediaCurrentTimeDisplay,
    MediaTimeRemainingDisplay,
    MediaStatusDisplay,
    MediaControlsPanel,
    MediaVolumeSliderContainer,
    MediaVolumeSlider,
    MediaVolumeSliderThumb,
    MediaFullScreenVolumeSlider,
    MediaFullScreenVolumeSliderThumb,
    MediaVolumeSliderMuteButton,
    MediaTextTrackDisplayContainer,
    MediaTextTrackDisplay,
    MediaExitFullscreenButton,
    MediaOverlayPlayButton,
    MediaClosedCaptionsContainer,
    MediaClosedCaptionsTrackList,
};

HTMLMediaElement* toParentMediaElement(Node*);
inline HTMLMediaElement* toParentMediaElement(RenderObject* renderer) { return toParentMediaElement(renderer->node()); }

MediaControlElementType mediaControlElementType(Node*);

// ----------------------------

class MediaControlElement {
public:
    virtual void hide();
    virtual void show();
    virtual bool isShowing() const;

    virtual MediaControlElementType displayType() { return m_displayType; }
    virtual const AtomicString& shadowPseudoId() const = 0;

    virtual void setMediaController(MediaControllerInterface* controller) { m_mediaController = controller; }
    virtual MediaControllerInterface* mediaController() const { return m_mediaController; }

protected:
    explicit MediaControlElement(MediaControlElementType, HTMLElement*);
    ~MediaControlElement() { }

    virtual void setDisplayType(MediaControlElementType);
    virtual bool isMediaControlElement() const { return true; }

private:
    MediaControllerInterface* m_mediaController;
    MediaControlElementType m_displayType;
    HTMLElement* m_element;
};

// ----------------------------

class MediaControlDivElement : public HTMLDivElement, public MediaControlElement {
protected:
    virtual bool isMediaControlElement() const OVERRIDE { return MediaControlElement::isMediaControlElement(); }
    explicit MediaControlDivElement(Document*, MediaControlElementType);
};

// ----------------------------

class MediaControlInputElement : public HTMLInputElement, public MediaControlElement {
protected:
    virtual bool isMediaControlElement() const OVERRIDE { return MediaControlElement::isMediaControlElement(); }
    explicit MediaControlInputElement(Document*, MediaControlElementType);

private:
    virtual void updateDisplayType() { }
};

// ----------------------------

class MediaControlTimeDisplayElement : public MediaControlDivElement {
public:
    void setCurrentValue(double);
    double currentValue() const { return m_currentValue; }

protected:
    explicit MediaControlTimeDisplayElement(Document*, MediaControlElementType);

private:
    double m_currentValue;
};

// ----------------------------

class MediaControlMuteButtonElement : public MediaControlInputElement {
public:
    void changedMute();

    virtual bool willRespondToMouseClickEvents() OVERRIDE { return true; }

protected:
    explicit MediaControlMuteButtonElement(Document*, MediaControlElementType);

    virtual void defaultEventHandler(Event*) OVERRIDE;

private:
    virtual void updateDisplayType() OVERRIDE;
};

// ----------------------------

class MediaControlSeekButtonElement : public MediaControlInputElement {
public:
    virtual bool willRespondToMouseClickEvents() OVERRIDE { return true; }

protected:
    explicit MediaControlSeekButtonElement(Document*, MediaControlElementType);

    virtual void defaultEventHandler(Event*) OVERRIDE;
    virtual bool isForwardButton() const = 0;

private:
    void setActive(bool /*flag*/ = true, bool /*pause*/ = false) OVERRIDE FINAL;

    void startTimer();
    void stopTimer();
    double nextRate() const;
    void seekTimerFired(Timer<MediaControlSeekButtonElement>*);

    enum ActionType { Nothing, Play, Pause };
    ActionType m_actionOnStop;
    enum SeekType { Skip, Scan };
    SeekType m_seekType;
    Timer<MediaControlSeekButtonElement> m_seekTimer;
};

// ----------------------------

class MediaControlVolumeSliderElement : public MediaControlInputElement {
public:
    virtual bool willRespondToMouseMoveEvents() OVERRIDE;
    virtual bool willRespondToMouseClickEvents() OVERRIDE;
    void setVolume(double);
    void setClearMutedOnUserInteraction(bool);

protected:
    explicit MediaControlVolumeSliderElement(Document*);

    virtual void defaultEventHandler(Event*) OVERRIDE;

private:
    bool m_clearMutedOnUserInteraction;
};

} // namespace WebCore

#endif // ENABLE(VIDEO)

#endif // MediaControlElementTypes_h
