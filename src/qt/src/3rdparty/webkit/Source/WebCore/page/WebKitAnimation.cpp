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

#include "config.h"
#include "WebKitAnimation.h"

#include "Animation.h"
#include "AnimationBase.h"
#include "RenderStyle.h"

using namespace std;

namespace WebCore {

WebKitAnimation::WebKitAnimation(PassRefPtr<KeyframeAnimation> keyframeAnimation)
    : m_keyframeAnimation(keyframeAnimation)
{
}

String WebKitAnimation::name() const
{
    return m_keyframeAnimation->animation()->name();
}

double WebKitAnimation::duration() const
{
    return m_keyframeAnimation->duration();
}

double WebKitAnimation::elapsedTime() const
{
    return m_keyframeAnimation->getElapsedTime();
}

void WebKitAnimation::setElapsedTime(double time)
{
    m_keyframeAnimation->setElapsedTime(time);
}

double WebKitAnimation::delay() const
{
    return m_keyframeAnimation->animation()->delay();
}

int WebKitAnimation::iterationCount() const
{
    return m_keyframeAnimation->animation()->iterationCount();
}

bool WebKitAnimation::paused() const
{
    return m_keyframeAnimation->paused();
}

bool WebKitAnimation::ended() const
{
    int iterations = iterationCount();
    if (iterations == Animation::IterationCountInfinite)
        return false;
    return m_keyframeAnimation->getElapsedTime() > (m_keyframeAnimation->duration() * iterations);
}

WebKitAnimation::Direction WebKitAnimation::direction() const
{
    if (m_keyframeAnimation->animation()->direction() == Animation::AnimationDirectionNormal)
        return DIRECTION_NORMAL;
    return DIRECTION_ALTERNATE;
}

WebKitAnimation::FillMode WebKitAnimation::fillMode() const
{
    switch (m_keyframeAnimation->animation()->fillMode()) {
    case AnimationFillModeNone:
        return FILL_NONE;
        break;
    case AnimationFillModeForwards:
        return FILL_FORWARDS;
        break;
    case AnimationFillModeBackwards:
        return FILL_BACKWARDS;
        break;
    default:
        return FILL_BOTH;
        break;
    }
}

void WebKitAnimation::pause()
{
    m_keyframeAnimation->pause();
}

void WebKitAnimation::play()
{
    m_keyframeAnimation->play();
}

}
