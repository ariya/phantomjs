/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef MediaControlRootElementChromium_h
#define MediaControlRootElementChromium_h

#if ENABLE(VIDEO)

#include "MediaControls.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class HTMLInputElement;
class HTMLMediaElement;
class Event;
class MediaControlPanelMuteButtonElement;
class MediaControlPlayButtonElement;
class MediaControlCurrentTimeDisplayElement;
class MediaControlTimelineElement;
class MediaControlVolumeSliderElement;
class MediaControlFullscreenButtonElement;
class MediaControlTimeDisplayElement;
class MediaControlTimelineContainerElement;
class MediaControlMuteButtonElement;
class MediaControlVolumeSliderElement;
class MediaControlVolumeSliderContainerElement;
class MediaControlPanelElement;
class MediaPlayer;

class RenderBox;
class RenderMedia;

class MediaControlRootElementChromium : public MediaControls {
public:
    static PassRefPtr<MediaControlRootElementChromium> create(HTMLMediaElement*);

    // MediaControls implementation.
    void show();
    void hide();
    void makeOpaque();
    void makeTransparent();

    void reset();

    void playbackProgressed();
    void playbackStarted();
    void playbackStopped();

    void changedMute();
    void changedVolume();

    void enteredFullscreen();
    void exitedFullscreen();

    void reportedError();
    void loadedMetadata();
    void changedClosedCaptionsVisibility();

    void showVolumeSlider();
    void updateTimeDisplay();
    void updateStatusDisplay();

private:
    MediaControlRootElementChromium(HTMLMediaElement*);

    virtual const AtomicString& shadowPseudoId() const;

    HTMLMediaElement* m_mediaElement;

    MediaControlPlayButtonElement* m_playButton;
    MediaControlCurrentTimeDisplayElement* m_currentTimeDisplay;
    MediaControlTimelineElement* m_timeline;
    MediaControlTimelineContainerElement* m_timelineContainer;
    MediaControlPanelMuteButtonElement* m_panelMuteButton;
    MediaControlVolumeSliderElement* m_volumeSlider;
    MediaControlVolumeSliderContainerElement* m_volumeSliderContainer;
    MediaControlPanelElement* m_panel;

    bool m_opaque;
};

}

#endif

#endif
