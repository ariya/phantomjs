/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef WebKitAnimation_h
#define WebKitAnimation_h

#include "Animation.h"
#include "AnimationBase.h"
#include "KeyframeAnimation.h"
#include "RenderStyle.h"

namespace WebCore {

class WebKitAnimation : public RefCounted<WebKitAnimation> {
public:

    static PassRefPtr<WebKitAnimation> create(PassRefPtr<KeyframeAnimation> keyframeAnimation)
    {
        return adoptRef(new WebKitAnimation(keyframeAnimation));
    }

    virtual ~WebKitAnimation() { }

    // DOM API

    String name() const;

    double duration() const;

    double elapsedTime() const;
    void setElapsedTime(double);

    double delay() const;
    int iterationCount() const;

    bool paused() const;
    bool ended() const;

    // direction
    enum Direction { DIRECTION_NORMAL, DIRECTION_ALTERNATE };
    Direction direction() const;

    // fill mode
    enum FillMode { FILL_NONE, FILL_BACKWARDS, FILL_FORWARDS, FILL_BOTH };
    FillMode fillMode() const;

    void play();
    void pause();

protected:
    WebKitAnimation(PassRefPtr<KeyframeAnimation>);

private:
    RefPtr<KeyframeAnimation> m_keyframeAnimation;
};

} // namespace

#endif
