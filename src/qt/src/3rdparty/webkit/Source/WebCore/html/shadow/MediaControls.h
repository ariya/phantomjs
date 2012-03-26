/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef MediaControls_h
#define MediaControls_h

#if ENABLE(VIDEO)

#include "HTMLDivElement.h"

namespace WebCore {

class HTMLMediaElement;

class MediaControls : public HTMLDivElement {
  public:
    virtual ~MediaControls() {}

    // This function is to be implemented in your port-specific media
    // controls implementation.
    static PassRefPtr<MediaControls> create(HTMLMediaElement*);

    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void makeOpaque() = 0;
    virtual void makeTransparent() = 0;

    virtual void reset() = 0;

    virtual void playbackProgressed() = 0;
    virtual void playbackStarted() = 0;
    virtual void playbackStopped() = 0;

    virtual void changedMute() = 0;
    virtual void changedVolume() = 0;

    virtual void enteredFullscreen() = 0;
    virtual void exitedFullscreen() = 0;

    virtual void reportedError() = 0;
    virtual void loadedMetadata() = 0;
    virtual void changedClosedCaptionsVisibility() = 0;

    virtual void showVolumeSlider() = 0;
    virtual void updateTimeDisplay() = 0;
    virtual void updateStatusDisplay() = 0;

    virtual bool shouldHideControls() = 0;

protected:
    MediaControls(HTMLMediaElement*);

private:
    MediaControls();

    virtual bool isMediaControls() const { return true; }
};

inline MediaControls* toMediaControls(Node* node)
{
    ASSERT(!node || node->isMediaControls());
    return static_cast<MediaControls*>(node);
}

// This will catch anyone doing an unneccessary cast.
void toMediaControls(const Node*);

}

#endif

#endif
