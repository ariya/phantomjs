/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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

#ifndef KeyframeAnimation_h
#define KeyframeAnimation_h

#include "AnimationBase.h"
#include "Document.h"
#include "KeyframeList.h"

namespace WebCore {

class RenderStyle;

// A KeyframeAnimation tracks the state of an explicit animation
// for a single RenderObject.
class KeyframeAnimation : public AnimationBase {
public:
    static PassRefPtr<KeyframeAnimation> create(const Animation* animation, RenderObject* renderer, int index, CompositeAnimation* compositeAnimation, RenderStyle* unanimatedStyle)
    {
        return adoptRef(new KeyframeAnimation(animation, renderer, index, compositeAnimation, unanimatedStyle));
    };

    virtual void animate(CompositeAnimation*, RenderObject*, const RenderStyle* currentStyle, RenderStyle* targetStyle, RefPtr<RenderStyle>& animatedStyle);
    virtual void getAnimatedStyle(RefPtr<RenderStyle>& animatedStyle);

    const AtomicString& name() const { return m_keyframes.animationName(); }
    int index() const { return m_index; }
    void setIndex(int i) { m_index = i; }

    bool hasAnimationForProperty(int property) const;
    
    void setUnanimatedStyle(PassRefPtr<RenderStyle> style) { m_unanimatedStyle = style; }
    RenderStyle* unanimatedStyle() const { return m_unanimatedStyle.get(); }

    virtual double timeToNextService();

protected:
    virtual void onAnimationStart(double elapsedTime);
    virtual void onAnimationIteration(double elapsedTime);
    virtual void onAnimationEnd(double elapsedTime);
    virtual bool startAnimation(double timeOffset);
    virtual void pauseAnimation(double timeOffset);
    virtual void endAnimation();

    virtual void overrideAnimations();
    virtual void resumeOverriddenAnimations();

    bool shouldSendEventForListener(Document::ListenerType inListenerType) const;
    bool sendAnimationEvent(const AtomicString&, double elapsedTime);

    virtual bool affectsProperty(int) const;

    void validateTransformFunctionList();

private:
    KeyframeAnimation(const Animation* animation, RenderObject*, int index, CompositeAnimation*, RenderStyle* unanimatedStyle);
    virtual ~KeyframeAnimation();
    
    // Get the styles for the given property surrounding the current animation time and the progress between them.
    void fetchIntervalEndpointsForProperty(int property, const RenderStyle*& fromStyle, const RenderStyle*& toStyle, double& progress) const;

    // The keyframes that we are blending.
    KeyframeList m_keyframes;

    // The order in which this animation appears in the animation-name style.
    int m_index;
    bool m_startEventDispatched;

    // The style just before we started animation
    RefPtr<RenderStyle> m_unanimatedStyle;
};

} // namespace WebCore

#endif // KeyframeAnimation_h
