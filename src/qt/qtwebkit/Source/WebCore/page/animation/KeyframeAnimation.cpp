/*
 * Copyright (C) 2007, 2012 Apple Inc. All rights reserved.
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
#include "KeyframeAnimation.h"

#include "AnimationControllerPrivate.h"
#include "CSSPropertyAnimation.h"
#include "CSSPropertyNames.h"
#include "CompositeAnimation.h"
#include "EventNames.h"
#include "RenderBoxModelObject.h"
#include "RenderStyle.h"
#include "StyleResolver.h"

using namespace std;

namespace WebCore {

KeyframeAnimation::KeyframeAnimation(const Animation* animation, RenderObject* renderer, int index, CompositeAnimation* compAnim, RenderStyle* unanimatedStyle)
    : AnimationBase(animation, renderer, compAnim)
    , m_keyframes(renderer, animation->name())
    , m_index(index)
    , m_startEventDispatched(false)
    , m_unanimatedStyle(unanimatedStyle)
{
    // Get the keyframe RenderStyles
    if (m_object && m_object->node() && m_object->node()->isElementNode())
        m_object->document()->ensureStyleResolver()->keyframeStylesForAnimation(toElement(m_object->node()), unanimatedStyle, m_keyframes);

    // Update the m_transformFunctionListValid flag based on whether the function lists in the keyframes match.
    validateTransformFunctionList();
#if ENABLE(CSS_FILTERS)
    checkForMatchingFilterFunctionLists();
#endif
}

KeyframeAnimation::~KeyframeAnimation()
{
    // Make sure to tell the renderer that we are ending. This will make sure any accelerated animations are removed.
    if (!postActive())
        endAnimation();
}

static const Animation* getAnimationFromStyleByName(const RenderStyle* style, const AtomicString& name)
{
    if (!style->animations())
        return 0;

    for (size_t i = 0; i < style->animations()->size(); i++) {
        if (name == style->animations()->animation(i)->name())
            return style->animations()->animation(i);
    }

    return 0;
}

void KeyframeAnimation::fetchIntervalEndpointsForProperty(CSSPropertyID property, const RenderStyle*& fromStyle, const RenderStyle*& toStyle, double& prog) const
{
    // Find the first key
    double elapsedTime = getElapsedTime();
    if (m_animation->duration() && m_animation->iterationCount() != Animation::IterationCountInfinite)
        elapsedTime = min(elapsedTime, m_animation->duration() * m_animation->iterationCount());

    const double fractionalTime = this->fractionalTime(1, elapsedTime, 0);

    size_t numKeyframes = m_keyframes.size();
    if (!numKeyframes)
        return;
    
    ASSERT(!m_keyframes[0].key());
    ASSERT(m_keyframes[m_keyframes.size() - 1].key() == 1);
    
    int prevIndex = -1;
    int nextIndex = -1;
    
    // FIXME: with a lot of keys, this linear search will be slow. We could binary search.
    for (size_t i = 0; i < numKeyframes; ++i) {
        const KeyframeValue& currKeyFrame = m_keyframes[i];

        if (!currKeyFrame.containsProperty(property))
            continue;

        if (fractionalTime < currKeyFrame.key()) {
            nextIndex = i;
            break;
        }
        
        prevIndex = i;
    }

    double scale = 1;
    double offset = 0;

    if (prevIndex == -1)
        prevIndex = 0;

    if (nextIndex == -1)
        nextIndex = m_keyframes.size() - 1;

    const KeyframeValue& prevKeyframe = m_keyframes[prevIndex];
    const KeyframeValue& nextKeyframe = m_keyframes[nextIndex];

    fromStyle = prevKeyframe.style();
    toStyle = nextKeyframe.style();
    
    offset = prevKeyframe.key();
    scale = 1.0 / (nextKeyframe.key() - prevKeyframe.key());

    const TimingFunction* timingFunction = 0;
    if (const Animation* matchedAnimation = getAnimationFromStyleByName(fromStyle, name()))
        timingFunction = matchedAnimation->timingFunction().get();

    prog = progress(scale, offset, timingFunction);
}

void KeyframeAnimation::animate(CompositeAnimation* compositeAnimation, RenderObject*, const RenderStyle*, RenderStyle* targetStyle, RefPtr<RenderStyle>& animatedStyle)
{
    // Fire the start timeout if needed
    fireAnimationEventsIfNeeded();
    
    // If we have not yet started, we will not have a valid start time, so just start the animation if needed.
    if (isNew() && m_animation->playState() == AnimPlayStatePlaying && !compositeAnimation->isSuspended())
        updateStateMachine(AnimationStateInputStartAnimation, -1);

    // If we get this far and the animation is done, it means we are cleaning up a just finished animation.
    // If so, we need to send back the targetStyle.
    if (postActive()) {
        if (!animatedStyle)
            animatedStyle = const_cast<RenderStyle*>(targetStyle);
        return;
    }

    // If we are waiting for the start timer, we don't want to change the style yet.
    // Special case 1 - if the delay time is 0, then we do want to set the first frame of the
    // animation right away. This avoids a flash when the animation starts.
    // Special case 2 - if there is a backwards fill mode, then we want to continue
    // through to the style blend so that we get the fromStyle.
    if (waitingToStart() && m_animation->delay() > 0 && !m_animation->fillsBackwards())
        return;
    
    // If we have no keyframes, don't animate.
    if (!m_keyframes.size()) {
        updateStateMachine(AnimationStateInputEndAnimation, -1);
        return;
    }

    // Run a cycle of animation.
    // We know we will need a new render style, so make one if needed.
    if (!animatedStyle)
        animatedStyle = RenderStyle::clone(targetStyle);

    // FIXME: we need to be more efficient about determining which keyframes we are animating between.
    // We should cache the last pair or something.
    HashSet<CSSPropertyID>::const_iterator endProperties = m_keyframes.endProperties();
    for (HashSet<CSSPropertyID>::const_iterator it = m_keyframes.beginProperties(); it != endProperties; ++it) {
        // Get the from/to styles and progress between
        const RenderStyle* fromStyle = 0;
        const RenderStyle* toStyle = 0;
        double progress = 0.0;
        fetchIntervalEndpointsForProperty(*it, fromStyle, toStyle, progress);
#if USE(ACCELERATED_COMPOSITING)
        bool needsAnim = CSSPropertyAnimation::blendProperties(this, *it, animatedStyle.get(), fromStyle, toStyle, progress);
        if (!needsAnim)
            // If we are running an accelerated animation, set a flag in the style
            // to indicate it. This can be used to make sure we get an updated
            // style for hit testing, etc.
            animatedStyle->setIsRunningAcceleratedAnimation();
#endif
    }
}

void KeyframeAnimation::getAnimatedStyle(RefPtr<RenderStyle>& animatedStyle)
{
    // If we're in the delay phase and we're not backwards filling, tell the caller
    // to use the current style.
    if (waitingToStart() && m_animation->delay() > 0 && !m_animation->fillsBackwards())
        return;

    if (!m_keyframes.size())
        return;

    if (!animatedStyle)
        animatedStyle = RenderStyle::clone(m_object->style());

    HashSet<CSSPropertyID>::const_iterator endProperties = m_keyframes.endProperties();
    for (HashSet<CSSPropertyID>::const_iterator it = m_keyframes.beginProperties(); it != endProperties; ++it) {
        // Get the from/to styles and progress between
        const RenderStyle* fromStyle = 0;
        const RenderStyle* toStyle = 0;
        double progress = 0.0;
        fetchIntervalEndpointsForProperty(*it, fromStyle, toStyle, progress);

        CSSPropertyAnimation::blendProperties(this, *it, animatedStyle.get(), fromStyle, toStyle, progress);
    }
}

bool KeyframeAnimation::hasAnimationForProperty(CSSPropertyID property) const
{
    return m_keyframes.containsProperty(property);
}

bool KeyframeAnimation::startAnimation(double timeOffset)
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_object && m_object->isComposited()) {
        return toRenderBoxModelObject(m_object)->startAnimation(timeOffset, m_animation.get(), m_keyframes);
    }
#else
    UNUSED_PARAM(timeOffset);
#endif
    return false;
}

void KeyframeAnimation::pauseAnimation(double timeOffset)
{
    if (!m_object)
        return;

#if USE(ACCELERATED_COMPOSITING)
    if (m_object->isComposited())
        toRenderBoxModelObject(m_object)->animationPaused(timeOffset, m_keyframes.animationName());
#else
    UNUSED_PARAM(timeOffset);
#endif
    // Restore the original (unanimated) style
    if (!paused())
        setNeedsStyleRecalc(m_object->node());
}

void KeyframeAnimation::endAnimation()
{
    if (!m_object)
        return;

#if USE(ACCELERATED_COMPOSITING)
    if (m_object->isComposited())
        toRenderBoxModelObject(m_object)->animationFinished(m_keyframes.animationName());
#endif
    // Restore the original (unanimated) style
    if (!paused())
        setNeedsStyleRecalc(m_object->node());
}

bool KeyframeAnimation::shouldSendEventForListener(Document::ListenerType listenerType) const
{
    return m_object->document()->hasListenerType(listenerType);
}

void KeyframeAnimation::onAnimationStart(double elapsedTime)
{
    sendAnimationEvent(eventNames().webkitAnimationStartEvent, elapsedTime);
}

void KeyframeAnimation::onAnimationIteration(double elapsedTime)
{
    sendAnimationEvent(eventNames().webkitAnimationIterationEvent, elapsedTime);
}

void KeyframeAnimation::onAnimationEnd(double elapsedTime)
{
    sendAnimationEvent(eventNames().webkitAnimationEndEvent, elapsedTime);
    // End the animation if we don't fill forwards. Forward filling
    // animations are ended properly in the class destructor.
    if (!m_animation->fillsForwards())
        endAnimation();
}

bool KeyframeAnimation::sendAnimationEvent(const AtomicString& eventType, double elapsedTime)
{
    Document::ListenerType listenerType;
    if (eventType == eventNames().webkitAnimationIterationEvent)
        listenerType = Document::ANIMATIONITERATION_LISTENER;
    else if (eventType == eventNames().webkitAnimationEndEvent)
        listenerType = Document::ANIMATIONEND_LISTENER;
    else {
        ASSERT(eventType == eventNames().webkitAnimationStartEvent);
        if (m_startEventDispatched)
            return false;
        m_startEventDispatched = true;
        listenerType = Document::ANIMATIONSTART_LISTENER;
    }

    if (shouldSendEventForListener(listenerType)) {
        // Dispatch the event
        RefPtr<Element> element;
        if (m_object->node() && m_object->node()->isElementNode())
            element = toElement(m_object->node());

        ASSERT(!element || (element->document() && !element->document()->inPageCache()));
        if (!element)
            return false;

        // Schedule event handling
        m_compAnim->animationController()->addEventToDispatch(element, eventType, m_keyframes.animationName(), elapsedTime);

        // Restore the original (unanimated) style
        if (eventType == eventNames().webkitAnimationEndEvent && element->renderer())
            setNeedsStyleRecalc(element.get());

        return true; // Did dispatch an event
    }

    return false; // Did not dispatch an event
}

void KeyframeAnimation::overrideAnimations()
{
    // This will override implicit animations that match the properties in the keyframe animation
    HashSet<CSSPropertyID>::const_iterator end = m_keyframes.endProperties();
    for (HashSet<CSSPropertyID>::const_iterator it = m_keyframes.beginProperties(); it != end; ++it)
        compositeAnimation()->overrideImplicitAnimations(*it);
}

void KeyframeAnimation::resumeOverriddenAnimations()
{
    // This will resume overridden implicit animations
    HashSet<CSSPropertyID>::const_iterator end = m_keyframes.endProperties();
    for (HashSet<CSSPropertyID>::const_iterator it = m_keyframes.beginProperties(); it != end; ++it)
        compositeAnimation()->resumeOverriddenImplicitAnimations(*it);
}

bool KeyframeAnimation::affectsProperty(CSSPropertyID property) const
{
    return m_keyframes.containsProperty(property);
}

void KeyframeAnimation::validateTransformFunctionList()
{
    m_transformFunctionListValid = false;
    
    if (m_keyframes.size() < 2 || !m_keyframes.containsProperty(CSSPropertyWebkitTransform))
        return;

    // Empty transforms match anything, so find the first non-empty entry as the reference
    size_t numKeyframes = m_keyframes.size();
    size_t firstNonEmptyTransformKeyframeIndex = numKeyframes;

    for (size_t i = 0; i < numKeyframes; ++i) {
        const KeyframeValue& currentKeyframe = m_keyframes[i];
        if (currentKeyframe.style()->transform().operations().size()) {
            firstNonEmptyTransformKeyframeIndex = i;
            break;
        }
    }
    
    if (firstNonEmptyTransformKeyframeIndex == numKeyframes)
        return;
        
    const TransformOperations* firstVal = &m_keyframes[firstNonEmptyTransformKeyframeIndex].style()->transform();
    
    // See if the keyframes are valid
    for (size_t i = firstNonEmptyTransformKeyframeIndex + 1; i < numKeyframes; ++i) {
        const KeyframeValue& currentKeyframe = m_keyframes[i];
        const TransformOperations* val = &currentKeyframe.style()->transform();
        
        // An emtpy transform list matches anything.
        if (val->operations().isEmpty())
            continue;
        
        if (!firstVal->operationsMatch(*val))
            return;
    }

    // Keyframes are valid
    m_transformFunctionListValid = true;
}

#if ENABLE(CSS_FILTERS)
void KeyframeAnimation::checkForMatchingFilterFunctionLists()
{
    m_filterFunctionListsMatch = false;

    if (m_keyframes.size() < 2 || !m_keyframes.containsProperty(CSSPropertyWebkitFilter))
        return;

    // Empty filters match anything, so find the first non-empty entry as the reference
    size_t numKeyframes = m_keyframes.size();
    size_t firstNonEmptyFilterKeyframeIndex = numKeyframes;

    for (size_t i = 0; i < numKeyframes; ++i) {
        const KeyframeValue& currentKeyframe = m_keyframes[i];
        if (currentKeyframe.style()->filter().operations().size()) {
            firstNonEmptyFilterKeyframeIndex = i;
            break;
        }
    }
    
    if (firstNonEmptyFilterKeyframeIndex == numKeyframes)
        return;
        
    const FilterOperations* firstVal = &m_keyframes[firstNonEmptyFilterKeyframeIndex].style()->filter();
    
    for (size_t i = firstNonEmptyFilterKeyframeIndex + 1; i < numKeyframes; ++i) {
        const KeyframeValue& currentKeyframe = m_keyframes[i];
        const FilterOperations* val = &currentKeyframe.style()->filter();
        
        // An emtpy filter list matches anything.
        if (val->operations().isEmpty())
            continue;
        
        if (!firstVal->operationsMatch(*val))
            return;
    }
    
    m_filterFunctionListsMatch = true;
}
#endif

double KeyframeAnimation::timeToNextService()
{
    double t = AnimationBase::timeToNextService();
#if USE(ACCELERATED_COMPOSITING)
    if (t != 0 || preActive())
        return t;
        
    // A return value of 0 means we need service. But if we only have accelerated animations we 
    // only need service at the end of the transition
    HashSet<CSSPropertyID>::const_iterator endProperties = m_keyframes.endProperties();
    bool acceleratedPropertiesOnly = true;
    
    for (HashSet<CSSPropertyID>::const_iterator it = m_keyframes.beginProperties(); it != endProperties; ++it) {
        if (!CSSPropertyAnimation::animationOfPropertyIsAccelerated(*it) || !isAccelerated()) {
            acceleratedPropertiesOnly = false;
            break;
        }
    }

    if (acceleratedPropertiesOnly) {
        bool isLooping;
        getTimeToNextEvent(t, isLooping);
    }
#endif
    return t;
}

} // namespace WebCore
