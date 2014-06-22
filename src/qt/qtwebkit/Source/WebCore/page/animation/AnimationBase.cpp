/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#include "config.h"
#include "AnimationBase.h"

#include "AnimationControllerPrivate.h"
#include "CSSPrimitiveValue.h"
#include "CSSPropertyAnimation.h"
#include "CompositeAnimation.h"
#include "Document.h"
#include "EventNames.h"
#include "FloatConversion.h"
#include "Logging.h"
#include "RenderBox.h"
#include "RenderStyle.h"
#include "UnitBezier.h"
#include <algorithm>
#include <wtf/CurrentTime.h>

using namespace std;

namespace WebCore {

// The epsilon value we pass to UnitBezier::solve given that the animation is going to run over |dur| seconds. The longer the
// animation, the more precision we need in the timing function result to avoid ugly discontinuities.
static inline double solveEpsilon(double duration)
{
    return 1.0 / (200.0 * duration);
}

static inline double solveCubicBezierFunction(double p1x, double p1y, double p2x, double p2y, double t, double duration)
{
    // Convert from input time to parametric value in curve, then from
    // that to output time.
    UnitBezier bezier(p1x, p1y, p2x, p2y);
    return bezier.solve(t, solveEpsilon(duration));
}

static inline double solveStepsFunction(int numSteps, bool stepAtStart, double t)
{
    if (stepAtStart)
        return min(1.0, (floor(numSteps * t) + 1) / numSteps);
    return floor(numSteps * t) / numSteps;
}

AnimationBase::AnimationBase(const Animation* transition, RenderObject* renderer, CompositeAnimation* compAnim)
    : m_animState(AnimationStateNew)
    , m_isAccelerated(false)
    , m_transformFunctionListValid(false)
#if ENABLE(CSS_FILTERS)
    , m_filterFunctionListsMatch(false)
#endif
    , m_startTime(0)
    , m_pauseTime(-1)
    , m_requestedStartTime(0)
    , m_totalDuration(-1)
    , m_nextIterationDuration(-1)
    , m_object(renderer)
    , m_animation(const_cast<Animation*>(transition))
    , m_compAnim(compAnim)
{
    // Compute the total duration
    if (m_animation->iterationCount() > 0)
        m_totalDuration = m_animation->duration() * m_animation->iterationCount();
}

void AnimationBase::setNeedsStyleRecalc(Node* node)
{
    ASSERT(!node || (node->document() && !node->document()->inPageCache()));
    if (node)
        node->setNeedsStyleRecalc(SyntheticStyleChange);
}

double AnimationBase::duration() const
{
    return m_animation->duration();
}

bool AnimationBase::playStatePlaying() const
{
    return m_animation->playState() == AnimPlayStatePlaying;
}

bool AnimationBase::animationsMatch(const Animation* anim) const
{
    return m_animation->animationsMatch(anim);
}

#if !LOG_DISABLED
static const char* nameForState(AnimationBase::AnimState state)
{
    switch (state) {
    case AnimationBase::AnimationStateNew: return "New";
    case AnimationBase::AnimationStateStartWaitTimer: return "StartWaitTimer";
    case AnimationBase::AnimationStateStartWaitStyleAvailable: return "StartWaitStyleAvailable";
    case AnimationBase::AnimationStateStartWaitResponse: return "StartWaitResponse";
    case AnimationBase::AnimationStateLooping: return "Looping";
    case AnimationBase::AnimationStateEnding: return "Ending";
    case AnimationBase::AnimationStatePausedNew: return "PausedNew";
    case AnimationBase::AnimationStatePausedWaitTimer: return "PausedWaitTimer";
    case AnimationBase::AnimationStatePausedWaitStyleAvailable: return "PausedWaitStyleAvailable";
    case AnimationBase::AnimationStatePausedWaitResponse: return "PausedWaitResponse";
    case AnimationBase::AnimationStatePausedRun: return "PausedRun";
    case AnimationBase::AnimationStateDone: return "Done";
    case AnimationBase::AnimationStateFillingForwards: return "FillingForwards";
    }
    return "";
}
#endif

void AnimationBase::updateStateMachine(AnimStateInput input, double param)
{
    if (!m_compAnim)
        return;

    // If we get AnimationStateInputRestartAnimation then we force a new animation, regardless of state.
    if (input == AnimationStateInputMakeNew) {
        if (m_animState == AnimationStateStartWaitStyleAvailable)
            m_compAnim->animationController()->removeFromAnimationsWaitingForStyle(this);
        LOG(Animations, "%p AnimationState %s -> New", this, nameForState(m_animState));
        m_animState = AnimationStateNew;
        m_startTime = 0;
        m_pauseTime = -1;
        m_requestedStartTime = 0;
        m_nextIterationDuration = -1;
        endAnimation();
        return;
    }

    if (input == AnimationStateInputRestartAnimation) {
        if (m_animState == AnimationStateStartWaitStyleAvailable)
            m_compAnim->animationController()->removeFromAnimationsWaitingForStyle(this);
        LOG(Animations, "%p AnimationState %s -> New", this, nameForState(m_animState));
        m_animState = AnimationStateNew;
        m_startTime = 0;
        m_pauseTime = -1;
        m_requestedStartTime = 0;
        m_nextIterationDuration = -1;
        endAnimation();

        if (!paused())
            updateStateMachine(AnimationStateInputStartAnimation, -1);
        return;
    }

    if (input == AnimationStateInputEndAnimation) {
        if (m_animState == AnimationStateStartWaitStyleAvailable)
            m_compAnim->animationController()->removeFromAnimationsWaitingForStyle(this);
        LOG(Animations, "%p AnimationState %s -> Done", this, nameForState(m_animState));
        m_animState = AnimationStateDone;
        endAnimation();
        return;
    }

    if (input == AnimationStateInputPauseOverride) {
        if (m_animState == AnimationStateStartWaitResponse) {
            // If we are in AnimationStateStartWaitResponse, the animation will get canceled before 
            // we get a response, so move to the next state.
            endAnimation();
            updateStateMachine(AnimationStateInputStartTimeSet, beginAnimationUpdateTime());
        }
        return;
    }

    if (input == AnimationStateInputResumeOverride) {
        if (m_animState == AnimationStateLooping || m_animState == AnimationStateEnding) {
            // Start the animation
            startAnimation(beginAnimationUpdateTime() - m_startTime);
        }
        return;
    }

    // Execute state machine
    switch (m_animState) {
        case AnimationStateNew:
            ASSERT(input == AnimationStateInputStartAnimation || input == AnimationStateInputPlayStateRunning || input == AnimationStateInputPlayStatePaused);
            if (input == AnimationStateInputStartAnimation || input == AnimationStateInputPlayStateRunning) {
                m_requestedStartTime = beginAnimationUpdateTime();
                LOG(Animations, "%p AnimationState %s -> StartWaitTimer", this, nameForState(m_animState));
                m_animState = AnimationStateStartWaitTimer;
            } else {
                // We are pausing before we even started.
                LOG(Animations, "%p AnimationState %s -> AnimationStatePausedNew", this, nameForState(m_animState));
                m_animState = AnimationStatePausedNew;
            }
            break;
        case AnimationStateStartWaitTimer:
            ASSERT(input == AnimationStateInputStartTimerFired || input == AnimationStateInputPlayStatePaused);

            if (input == AnimationStateInputStartTimerFired) {
                ASSERT(param >= 0);
                // Start timer has fired, tell the animation to start and wait for it to respond with start time
                LOG(Animations, "%p AnimationState %s -> StartWaitStyleAvailable", this, nameForState(m_animState));
                m_animState = AnimationStateStartWaitStyleAvailable;
                m_compAnim->animationController()->addToAnimationsWaitingForStyle(this);

                // Trigger a render so we can start the animation
                if (m_object)
                    m_compAnim->animationController()->addNodeChangeToDispatch(m_object->node());
            } else {
                ASSERT(!paused());
                // We're waiting for the start timer to fire and we got a pause. Cancel the timer, pause and wait
                m_pauseTime = beginAnimationUpdateTime();
                LOG(Animations, "%p AnimationState %s -> PausedWaitTimer", this, nameForState(m_animState));
                m_animState = AnimationStatePausedWaitTimer;
            }
            break;
        case AnimationStateStartWaitStyleAvailable:
            ASSERT(input == AnimationStateInputStyleAvailable || input == AnimationStateInputPlayStatePaused);

            if (input == AnimationStateInputStyleAvailable) {
                // Start timer has fired, tell the animation to start and wait for it to respond with start time
                LOG(Animations, "%p AnimationState %s -> StartWaitResponse", this, nameForState(m_animState));
                m_animState = AnimationStateStartWaitResponse;

                overrideAnimations();

                // Start the animation
                if (overridden()) {
                    // We won't try to start accelerated animations if we are overridden and
                    // just move on to the next state.
                    LOG(Animations, "%p AnimationState %s -> StartWaitResponse", this, nameForState(m_animState));
                    m_animState = AnimationStateStartWaitResponse;
                    m_isAccelerated = false;
                    updateStateMachine(AnimationStateInputStartTimeSet, beginAnimationUpdateTime());
                } else {
                    double timeOffset = 0;
                    // If the value for 'animation-delay' is negative then the animation appears to have started in the past.
                    if (m_animation->delay() < 0)
                        timeOffset = -m_animation->delay();
                    bool started = startAnimation(timeOffset);

                    m_compAnim->animationController()->addToAnimationsWaitingForStartTimeResponse(this, started);
                    m_isAccelerated = started;
                }
            } else {
                // We're waiting for the style to be available and we got a pause. Pause and wait
                m_pauseTime = beginAnimationUpdateTime();
                LOG(Animations, "%p AnimationState %s -> PausedWaitStyleAvailable", this, nameForState(m_animState));
                m_animState = AnimationStatePausedWaitStyleAvailable;
            }
            break;
        case AnimationStateStartWaitResponse:
            ASSERT(input == AnimationStateInputStartTimeSet || input == AnimationStateInputPlayStatePaused);

            if (input == AnimationStateInputStartTimeSet) {
                ASSERT(param >= 0);
                // We have a start time, set it, unless the startTime is already set
                if (m_startTime <= 0) {
                    m_startTime = param;
                    // If the value for 'animation-delay' is negative then the animation appears to have started in the past.
                    if (m_animation->delay() < 0)
                        m_startTime += m_animation->delay();
                }

                // Now that we know the start time, fire the start event.
                onAnimationStart(0); // The elapsedTime is 0.

                // Decide whether to go into looping or ending state
                goIntoEndingOrLoopingState();

                // Dispatch updateStyleIfNeeded so we can start the animation
                if (m_object)
                    m_compAnim->animationController()->addNodeChangeToDispatch(m_object->node());
            } else {
                // We are pausing while waiting for a start response. Cancel the animation and wait. When 
                // we unpause, we will act as though the start timer just fired
                m_pauseTime = beginAnimationUpdateTime();
                pauseAnimation(beginAnimationUpdateTime() - m_startTime);
                LOG(Animations, "%p AnimationState %s -> PausedWaitResponse", this, nameForState(m_animState));
                m_animState = AnimationStatePausedWaitResponse;
            }
            break;
        case AnimationStateLooping:
            ASSERT(input == AnimationStateInputLoopTimerFired || input == AnimationStateInputPlayStatePaused);

            if (input == AnimationStateInputLoopTimerFired) {
                ASSERT(param >= 0);
                // Loop timer fired, loop again or end.
                onAnimationIteration(param);

                // Decide whether to go into looping or ending state
                goIntoEndingOrLoopingState();
            } else {
                // We are pausing while running. Cancel the animation and wait
                m_pauseTime = beginAnimationUpdateTime();
                pauseAnimation(beginAnimationUpdateTime() - m_startTime);
                LOG(Animations, "%p AnimationState %s -> PausedRun", this, nameForState(m_animState));
                m_animState = AnimationStatePausedRun;
            }
            break;
        case AnimationStateEnding:
#if !LOG_DISABLED
            if (input != AnimationStateInputEndTimerFired && input != AnimationStateInputPlayStatePaused)
                LOG_ERROR("State is AnimationStateEnding, but input is not AnimationStateInputEndTimerFired or AnimationStateInputPlayStatePaused. It is %d.", input);
#endif
            if (input == AnimationStateInputEndTimerFired) {

                ASSERT(param >= 0);
                // End timer fired, finish up
                onAnimationEnd(param);

                LOG(Animations, "%p AnimationState %s -> Done", this, nameForState(m_animState));
                m_animState = AnimationStateDone;
                
                if (m_object) {
                    if (m_animation->fillsForwards()) {
                        LOG(Animations, "%p AnimationState %s -> FillingForwards", this, nameForState(m_animState));
                        m_animState = AnimationStateFillingForwards;
                    } else
                        resumeOverriddenAnimations();

                    // Fire off another style change so we can set the final value
                    m_compAnim->animationController()->addNodeChangeToDispatch(m_object->node());
                }
            } else {
                // We are pausing while running. Cancel the animation and wait
                m_pauseTime = beginAnimationUpdateTime();
                pauseAnimation(beginAnimationUpdateTime() - m_startTime);
                LOG(Animations, "%p AnimationState %s -> PausedRun", this, nameForState(m_animState));
                m_animState = AnimationStatePausedRun;
            }
            // |this| may be deleted here
            break;
        case AnimationStatePausedWaitTimer:
            ASSERT(input == AnimationStateInputPlayStateRunning);
            ASSERT(paused());
            // Update the times
            m_startTime += beginAnimationUpdateTime() - m_pauseTime;
            m_pauseTime = -1;

            // we were waiting for the start timer to fire, go back and wait again
            LOG(Animations, "%p AnimationState %s -> New", this, nameForState(m_animState));
            m_animState = AnimationStateNew;
            updateStateMachine(AnimationStateInputStartAnimation, 0);
            break;
        case AnimationStatePausedNew:
        case AnimationStatePausedWaitResponse:
        case AnimationStatePausedWaitStyleAvailable:
        case AnimationStatePausedRun:
            // We treat these two cases the same. The only difference is that, when we are in
            // AnimationStatePausedWaitResponse, we don't yet have a valid startTime, so we send 0 to startAnimation.
            // When the AnimationStateInputStartTimeSet comes in and we were in AnimationStatePausedRun, we will notice
            // that we have already set the startTime and will ignore it.
            ASSERT(input == AnimationStateInputPlayStateRunning || input == AnimationStateInputStartTimeSet || input == AnimationStateInputStyleAvailable || input == AnimationStateInputStartAnimation);
            ASSERT(paused());

            if (input == AnimationStateInputPlayStateRunning) {
                if (m_animState == AnimationStatePausedNew) {
                    // We were paused before we even started, and now we're supposed
                    // to start, so jump back to the New state and reset.
                    LOG(Animations, "%p AnimationState %s -> AnimationStateNew", this, nameForState(m_animState));
                    m_animState = AnimationStateNew;
                    updateStateMachine(input, param);
                    break;
                }

                // Update the times
                if (m_animState == AnimationStatePausedRun)
                    m_startTime += beginAnimationUpdateTime() - m_pauseTime;
                else
                    m_startTime = 0;
                m_pauseTime = -1;

                if (m_animState == AnimationStatePausedWaitStyleAvailable) {
                    LOG(Animations, "%p AnimationState %s -> StartWaitStyleAvailable", this, nameForState(m_animState));
                    m_animState = AnimationStateStartWaitStyleAvailable;
                } else {
                    // We were either running or waiting for a begin time response from the animation.
                    // Either way we need to restart the animation (possibly with an offset if we
                    // had already been running) and wait for it to start.
                    LOG(Animations, "%p AnimationState %s -> StartWaitResponse", this, nameForState(m_animState));
                    m_animState = AnimationStateStartWaitResponse;

                    // Start the animation
                    if (overridden()) {
                        // We won't try to start accelerated animations if we are overridden and
                        // just move on to the next state.
                        updateStateMachine(AnimationStateInputStartTimeSet, beginAnimationUpdateTime());
                        m_isAccelerated = true;
                    } else {
                        bool started = startAnimation(beginAnimationUpdateTime() - m_startTime);
                        m_compAnim->animationController()->addToAnimationsWaitingForStartTimeResponse(this, started);
                        m_isAccelerated = started;
                    }
                }
                break;
            }
            
            if (input == AnimationStateInputStartTimeSet) {
                ASSERT(m_animState == AnimationStatePausedWaitResponse);
                
                // We are paused but we got the callback that notifies us that an accelerated animation started.
                // We ignore the start time and just move into the paused-run state.
                LOG(Animations, "%p AnimationState %s -> PausedRun", this, nameForState(m_animState));
                m_animState = AnimationStatePausedRun;
                ASSERT(m_startTime == 0);
                m_startTime = param;
                m_pauseTime += m_startTime;
                break;
            }

            ASSERT(m_animState == AnimationStatePausedWaitStyleAvailable);
            // We are paused but we got the callback that notifies us that style has been updated.
            // We move to the AnimationStatePausedWaitResponse state
            LOG(Animations, "%p AnimationState %s -> PausedWaitResponse", this, nameForState(m_animState));
            m_animState = AnimationStatePausedWaitResponse;
            overrideAnimations();
            break;
        case AnimationStateFillingForwards:
        case AnimationStateDone:
            // We're done. Stay in this state until we are deleted
            break;
    }
}
    
void AnimationBase::fireAnimationEventsIfNeeded()
{
    if (!m_compAnim)
        return;

    // If we are waiting for the delay time to expire and it has, go to the next state
    if (m_animState != AnimationStateStartWaitTimer && m_animState != AnimationStateLooping && m_animState != AnimationStateEnding)
        return;

    // We have to make sure to keep a ref to the this pointer, because it could get destroyed
    // during an animation callback that might get called. Since the owner is a CompositeAnimation
    // and it ref counts this object, we will keep a ref to that instead. That way the AnimationBase
    // can still access the resources of its CompositeAnimation as needed.
    RefPtr<AnimationBase> protector(this);
    RefPtr<CompositeAnimation> compProtector(m_compAnim);
    
    // Check for start timeout
    if (m_animState == AnimationStateStartWaitTimer) {
        if (beginAnimationUpdateTime() - m_requestedStartTime >= m_animation->delay())
            updateStateMachine(AnimationStateInputStartTimerFired, 0);
        return;
    }
    
    double elapsedDuration = beginAnimationUpdateTime() - m_startTime;
    // FIXME: we need to ensure that elapsedDuration is never < 0. If it is, this suggests that
    // we had a recalcStyle() outside of beginAnimationUpdate()/endAnimationUpdate().
    // Also check in getTimeToNextEvent().
    elapsedDuration = max(elapsedDuration, 0.0);
    
    // Check for end timeout
    if (m_totalDuration >= 0 && elapsedDuration >= m_totalDuration) {
        // We may still be in AnimationStateLooping if we've managed to skip a
        // whole iteration, in which case we should jump to the end state.
        LOG(Animations, "%p AnimationState %s -> Ending", this, nameForState(m_animState));
        m_animState = AnimationStateEnding;

        // Fire an end event
        updateStateMachine(AnimationStateInputEndTimerFired, m_totalDuration);
    } else {
        // Check for iteration timeout
        if (m_nextIterationDuration < 0) {
            // Hasn't been set yet, set it
            double durationLeft = m_animation->duration() - fmod(elapsedDuration, m_animation->duration());
            m_nextIterationDuration = elapsedDuration + durationLeft;
        }
        
        if (elapsedDuration >= m_nextIterationDuration) {
            // Set to the next iteration
            double previous = m_nextIterationDuration;
            double durationLeft = m_animation->duration() - fmod(elapsedDuration, m_animation->duration());
            m_nextIterationDuration = elapsedDuration + durationLeft;
            
            // Send the event
            updateStateMachine(AnimationStateInputLoopTimerFired, previous);
        }
    }
}

void AnimationBase::updatePlayState(EAnimPlayState playState)
{
    if (!m_compAnim)
        return;

    // When we get here, we can have one of 4 desired states: running, paused, suspended, paused & suspended.
    // The state machine can be in one of two states: running, paused.
    // Set the state machine to the desired state.
    bool pause = playState == AnimPlayStatePaused || m_compAnim->isSuspended();

    if (pause == paused() && !isNew())
        return;

    updateStateMachine(pause ?  AnimationStateInputPlayStatePaused : AnimationStateInputPlayStateRunning, -1);
}

double AnimationBase::timeToNextService()
{
    // Returns the time at which next service is required. -1 means no service is required. 0 means 
    // service is required now, and > 0 means service is required that many seconds in the future.
    if (paused() || isNew() || m_animState == AnimationStateFillingForwards)
        return -1;
    
    if (m_animState == AnimationStateStartWaitTimer) {
        double timeFromNow = m_animation->delay() - (beginAnimationUpdateTime() - m_requestedStartTime);
        return max(timeFromNow, 0.0);
    }
    
    fireAnimationEventsIfNeeded();
        
    // In all other cases, we need service right away.
    return 0;
}

// Compute the fractional time, taking into account direction.
// There is no need to worry about iterations, we assume that we would have
// short circuited above if we were done.

double AnimationBase::fractionalTime(double scale, double elapsedTime, double offset) const
{
    double fractionalTime = m_animation->duration() ? (elapsedTime / m_animation->duration()) : 1;
    // FIXME: startTime can be before the current animation "frame" time. This is to sync with the frame time
    // concept in AnimationTimeController. So we need to somehow sync the two. Until then, the possible
    // error is small and will probably not be noticeable. Until we fix this, remove the assert.
    // https://bugs.webkit.org/show_bug.cgi?id=52037
    // ASSERT(fractionalTime >= 0);
    if (fractionalTime < 0)
        fractionalTime = 0;

    int integralTime = static_cast<int>(fractionalTime);
    const int integralIterationCount = static_cast<int>(m_animation->iterationCount());
    const bool iterationCountHasFractional = m_animation->iterationCount() - integralIterationCount;
    if (m_animation->iterationCount() != Animation::IterationCountInfinite && !iterationCountHasFractional)
        integralTime = min(integralTime, integralIterationCount - 1);

    fractionalTime -= integralTime;

    if (((m_animation->direction() == Animation::AnimationDirectionAlternate) && (integralTime & 1))
        || ((m_animation->direction() == Animation::AnimationDirectionAlternateReverse) && !(integralTime & 1))
        || m_animation->direction() == Animation::AnimationDirectionReverse)
        fractionalTime = 1 - fractionalTime;

    if (scale != 1 || offset)
        fractionalTime = (fractionalTime - offset) * scale;

    return fractionalTime;
}

double AnimationBase::progress(double scale, double offset, const TimingFunction* tf) const
{
    if (preActive())
        return 0;

    double elapsedTime = getElapsedTime();

    double dur = m_animation->duration();
    if (m_animation->iterationCount() > 0)
        dur *= m_animation->iterationCount();

    if (postActive() || !m_animation->duration())
        return 1.0;

    if (m_animation->iterationCount() > 0 && elapsedTime >= dur) {
        const int integralIterationCount = static_cast<int>(m_animation->iterationCount());
        const bool iterationCountHasFractional = m_animation->iterationCount() - integralIterationCount;
        return (integralIterationCount % 2 || iterationCountHasFractional) ? 1.0 : 0.0;
    }

    const double fractionalTime = this->fractionalTime(scale, elapsedTime, offset);

    if (!tf)
        tf = m_animation->timingFunction().get();

    if (tf->isCubicBezierTimingFunction()) {
        const CubicBezierTimingFunction* ctf = static_cast<const CubicBezierTimingFunction*>(tf);
        return solveCubicBezierFunction(ctf->x1(),
                                        ctf->y1(),
                                        ctf->x2(),
                                        ctf->y2(),
                                        fractionalTime, m_animation->duration());
    }
    
    if (tf->isStepsTimingFunction()) {
        const StepsTimingFunction* stf = static_cast<const StepsTimingFunction*>(tf);
        return solveStepsFunction(stf->numberOfSteps(), stf->stepAtStart(), fractionalTime);
    }

    return fractionalTime;
}

void AnimationBase::getTimeToNextEvent(double& time, bool& isLooping) const
{
    // Decide when the end or loop event needs to fire
    const double elapsedDuration = max(beginAnimationUpdateTime() - m_startTime, 0.0);
    double durationLeft = 0;
    double nextIterationTime = m_totalDuration;

    if (m_totalDuration < 0 || elapsedDuration < m_totalDuration) {
        durationLeft = m_animation->duration() > 0 ? (m_animation->duration() - fmod(elapsedDuration, m_animation->duration())) : 0;
        nextIterationTime = elapsedDuration + durationLeft;
    }
    
    if (m_totalDuration < 0 || nextIterationTime < m_totalDuration) {
        // We are not at the end yet
        ASSERT(nextIterationTime > 0);
        isLooping = true;
    } else {
        // We are at the end
        isLooping = false;
    }
    
    time = durationLeft;
}

void AnimationBase::goIntoEndingOrLoopingState()
{
    double t;
    bool isLooping;
    getTimeToNextEvent(t, isLooping);
    LOG(Animations, "%p AnimationState %s -> %s", this, nameForState(m_animState), isLooping ? "Looping" : "Ending");
    m_animState = isLooping ? AnimationStateLooping : AnimationStateEnding;
}
  
void AnimationBase::freezeAtTime(double t)
{
    if (!m_compAnim)
        return;

    if (!m_startTime) {
        // If we haven't started yet, make it as if we started.
        LOG(Animations, "%p AnimationState %s -> StartWaitResponse", this, nameForState(m_animState));
        m_animState = AnimationStateStartWaitResponse;
        onAnimationStartResponse(currentTime());
    }

    ASSERT(m_startTime);        // if m_startTime is zero, we haven't started yet, so we'll get a bad pause time.
    if (t <= m_animation->delay())
        m_pauseTime = m_startTime;
    else
        m_pauseTime = m_startTime + t - m_animation->delay();

#if USE(ACCELERATED_COMPOSITING)
    if (m_object && m_object->isComposited())
        toRenderBoxModelObject(m_object)->suspendAnimations(m_pauseTime);
#endif
}

double AnimationBase::beginAnimationUpdateTime() const
{
    if (!m_compAnim)
        return 0;

    return m_compAnim->animationController()->beginAnimationUpdateTime();
}

double AnimationBase::getElapsedTime() const
{
    if (paused())    
        return m_pauseTime - m_startTime;
    if (m_startTime <= 0)
        return 0;
    if (postActive())
        return 1;

    return beginAnimationUpdateTime() - m_startTime;
}

void AnimationBase::setElapsedTime(double time)
{
    // FIXME: implement this method
    UNUSED_PARAM(time);
}

void AnimationBase::play()
{
    // FIXME: implement this method
}

void AnimationBase::pause()
{
    // FIXME: implement this method
}

} // namespace WebCore
