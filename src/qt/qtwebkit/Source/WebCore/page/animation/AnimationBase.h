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

#ifndef AnimationBase_h
#define AnimationBase_h

#include "Animation.h"
#include "CSSPropertyNames.h"
#include "RenderStyleConstants.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/RefCounted.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class AnimationBase;
class AnimationController;
class CompositeAnimation;
class Element;
class Node;
class RenderObject;
class RenderStyle;
class TimingFunction;

class AnimationBase : public RefCounted<AnimationBase> {
    friend class CompositeAnimation;
    friend class CSSPropertyAnimation;

public:
    AnimationBase(const Animation* transition, RenderObject* renderer, CompositeAnimation* compAnim);
    virtual ~AnimationBase() { }

    RenderObject* renderer() const { return m_object; }
    void clear()
    {
      endAnimation();
      m_object = 0;
      m_compAnim = 0;
    }

    double duration() const;

    // Animations and Transitions go through the states below. When entering the STARTED state
    // the animation is started. This may or may not require deferred response from the animator.
    // If so, we stay in this state until that response is received (and it returns the start time).
    // Otherwise, we use the current time as the start time and go immediately to AnimationStateLooping
    // or AnimationStateEnding.
    enum AnimState { 
        AnimationStateNew,                  // animation just created, animation not running yet
        AnimationStateStartWaitTimer,       // start timer running, waiting for fire
        AnimationStateStartWaitStyleAvailable,   // waiting for style setup so we can start animations
        AnimationStateStartWaitResponse,    // animation started, waiting for response
        AnimationStateLooping,              // response received, animation running, loop timer running, waiting for fire
        AnimationStateEnding,               // received, animation running, end timer running, waiting for fire
        AnimationStatePausedNew,            // in pause mode when animation was created
        AnimationStatePausedWaitTimer,      // in pause mode when animation started
        AnimationStatePausedWaitStyleAvailable, // in pause mode when waiting for style setup
        AnimationStatePausedWaitResponse,   // animation paused when in STARTING state
        AnimationStatePausedRun,            // animation paused when in LOOPING or ENDING state
        AnimationStateDone,                 // end timer fired, animation finished and removed
        AnimationStateFillingForwards       // animation has ended and is retaining its final value
    };

    enum AnimStateInput {
        AnimationStateInputMakeNew,           // reset back to new from any state
        AnimationStateInputStartAnimation,    // animation requests a start
        AnimationStateInputRestartAnimation,  // force a restart from any state
        AnimationStateInputStartTimerFired,   // start timer fired
        AnimationStateInputStyleAvailable,    // style is setup, ready to start animating
        AnimationStateInputStartTimeSet,      // m_startTime was set
        AnimationStateInputLoopTimerFired,    // loop timer fired
        AnimationStateInputEndTimerFired,     // end timer fired
        AnimationStateInputPauseOverride,     // pause an animation due to override
        AnimationStateInputResumeOverride,    // resume an overridden animation
        AnimationStateInputPlayStateRunning,  // play state paused -> running
        AnimationStateInputPlayStatePaused,   // play state running -> paused
        AnimationStateInputEndAnimation       // force an end from any state
    };

    // Called when animation is in AnimationStateNew to start animation
    void updateStateMachine(AnimStateInput, double param);

    // Animation has actually started, at passed time
    void onAnimationStartResponse(double startTime)
    {
        updateStateMachine(AnimationBase::AnimationStateInputStartTimeSet, startTime);
    }

    // Called to change to or from paused state
    void updatePlayState(EAnimPlayState);
    bool playStatePlaying() const;

    bool waitingToStart() const { return m_animState == AnimationStateNew || m_animState == AnimationStateStartWaitTimer || m_animState == AnimationStatePausedNew; }
    bool preActive() const
    {
        return m_animState == AnimationStateNew || m_animState == AnimationStateStartWaitTimer || m_animState == AnimationStateStartWaitStyleAvailable || m_animState == AnimationStateStartWaitResponse;
    }

    bool postActive() const { return m_animState == AnimationStateDone; }
    bool active() const { return !postActive() && !preActive(); }
    bool running() const { return !isNew() && !postActive(); }
    bool paused() const { return m_pauseTime >= 0 || m_animState == AnimationStatePausedNew; }
    bool isNew() const { return m_animState == AnimationStateNew || m_animState == AnimationStatePausedNew; }
    bool waitingForStartTime() const { return m_animState == AnimationStateStartWaitResponse; }
    bool waitingForStyleAvailable() const { return m_animState == AnimationStateStartWaitStyleAvailable; }

    virtual double timeToNextService();

    double progress(double scale, double offset, const TimingFunction*) const;

    virtual void animate(CompositeAnimation*, RenderObject*, const RenderStyle* /*currentStyle*/, RenderStyle* /*targetStyle*/, RefPtr<RenderStyle>& /*animatedStyle*/) = 0;
    virtual void getAnimatedStyle(RefPtr<RenderStyle>& /*animatedStyle*/) = 0;

    virtual bool shouldFireEvents() const { return false; }

    void fireAnimationEventsIfNeeded();

    bool animationsMatch(const Animation*) const;

    void setAnimation(const Animation* anim) { m_animation = const_cast<Animation*>(anim); }

    // Return true if this animation is overridden. This will only be the case for
    // ImplicitAnimations and is used to determine whether or not we should force
    // set the start time. If an animation is overridden, it will probably not get
    // back the AnimationStateInputStartTimeSet input.
    virtual bool overridden() const { return false; }

    // Does this animation/transition involve the given property?
    virtual bool affectsProperty(CSSPropertyID /*property*/) const { return false; }

    bool isAnimatingProperty(CSSPropertyID property, bool acceleratedOnly, bool isRunningNow) const
    {
        if (acceleratedOnly && !m_isAccelerated)
            return false;
            
        if (isRunningNow)
            return (!waitingToStart() && !postActive()) && affectsProperty(property);

        return !postActive() && affectsProperty(property);
    }

    // FIXME: rename this using the "lists match" terminology.
    bool isTransformFunctionListValid() const { return m_transformFunctionListValid; }
#if ENABLE(CSS_FILTERS)
    bool filterFunctionListsMatch() const { return m_filterFunctionListsMatch; }
#endif

    // Freeze the animation; used by DumpRenderTree.
    void freezeAtTime(double t);

    // Play and pause API
    void play();
    void pause();
    
    double beginAnimationUpdateTime() const;
    
    double getElapsedTime() const;
    // Setting the elapsed time will adjust the start time and possibly pause time.
    void setElapsedTime(double);
    
    void styleAvailable() 
    {
        ASSERT(waitingForStyleAvailable());
        updateStateMachine(AnimationBase::AnimationStateInputStyleAvailable, -1);
    }

    const Animation* animation() const { return m_animation.get(); }

protected:
    virtual void overrideAnimations() { }
    virtual void resumeOverriddenAnimations() { }

    CompositeAnimation* compositeAnimation() { return m_compAnim; }

    // These are called when the corresponding timer fires so subclasses can do any extra work
    virtual void onAnimationStart(double /*elapsedTime*/) { }
    virtual void onAnimationIteration(double /*elapsedTime*/) { }
    virtual void onAnimationEnd(double /*elapsedTime*/) { }
    
    // timeOffset is an offset from the current time when the animation should start. Negative values are OK.
    // Return value indicates whether to expect an asynchronous notifyAnimationStarted() callback.
    virtual bool startAnimation(double /*timeOffset*/) { return false; }
    // timeOffset is the time at which the animation is being paused.
    virtual void pauseAnimation(double /*timeOffset*/) { }
    virtual void endAnimation() { }

    void goIntoEndingOrLoopingState();

    bool isAccelerated() const { return m_isAccelerated; }

    static void setNeedsStyleRecalc(Node*);
    
    void getTimeToNextEvent(double& time, bool& isLooping) const;

    double fractionalTime(double scale, double elapsedTime, double offset) const;

    AnimState m_animState;

    bool m_isAccelerated;
    bool m_transformFunctionListValid;
#if ENABLE(CSS_FILTERS)
    bool m_filterFunctionListsMatch;
#endif
    double m_startTime;
    double m_pauseTime;
    double m_requestedStartTime;

    double m_totalDuration;
    double m_nextIterationDuration;

    RenderObject* m_object;

    RefPtr<Animation> m_animation;
    CompositeAnimation* m_compAnim;
};

} // namespace WebCore

#endif // AnimationBase_h
