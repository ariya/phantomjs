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

#include "config.h"

#include "AnimationControllerPrivate.h"
#include "CompositeAnimation.h"
#include "CSSPropertyNames.h"
#include "EventNames.h"
#include "ImplicitAnimation.h"
#include "KeyframeAnimation.h"
#include "RenderLayer.h"
#include "RenderLayerBacking.h"
#include <wtf/UnusedParam.h>

namespace WebCore {

ImplicitAnimation::ImplicitAnimation(const Animation* transition, int animatingProperty, RenderObject* renderer, CompositeAnimation* compAnim, RenderStyle* fromStyle)
    : AnimationBase(transition, renderer, compAnim)
    , m_transitionProperty(transition->property())
    , m_animatingProperty(animatingProperty)
    , m_overridden(false)
    , m_active(true)
    , m_fromStyle(fromStyle)
{
    ASSERT(animatingProperty != cAnimateAll);
}

ImplicitAnimation::~ImplicitAnimation()
{
    // // Make sure to tell the renderer that we are ending. This will make sure any accelerated animations are removed.
    if (!postActive())
        endAnimation();
}

bool ImplicitAnimation::shouldSendEventForListener(Document::ListenerType inListenerType) const
{
    return m_object->document()->hasListenerType(inListenerType);
}

void ImplicitAnimation::animate(CompositeAnimation*, RenderObject*, const RenderStyle*, RenderStyle* targetStyle, RefPtr<RenderStyle>& animatedStyle)
{
    // If we get this far and the animation is done, it means we are cleaning up a just finished animation.
    // So just return. Everything is already all cleaned up.
    if (postActive())
        return;

    // Reset to start the transition if we are new
    if (isNew())
        reset(targetStyle);

    // Run a cycle of animation.
    // We know we will need a new render style, so make one if needed
    if (!animatedStyle)
        animatedStyle = RenderStyle::clone(targetStyle);

    bool needsAnim = blendProperties(this, m_animatingProperty, animatedStyle.get(), m_fromStyle.get(), m_toStyle.get(), progress(1, 0, 0));
    // FIXME: we also need to detect cases where we have to software animate for other reasons,
    // such as a child using inheriting the transform. https://bugs.webkit.org/show_bug.cgi?id=23902
    if (needsAnim)
        setAnimating();
    else {
#if USE(ACCELERATED_COMPOSITING)
        // If we are running an accelerated animation, set a flag in the style which causes the style
        // to compare as different to any other style. This ensures that changes to the property
        // that is animating are correctly detected during the animation (e.g. when a transition
        // gets interrupted).
        animatedStyle->setIsRunningAcceleratedAnimation();
#endif
    }

    // Fire the start timeout if needed
    fireAnimationEventsIfNeeded();
}

void ImplicitAnimation::getAnimatedStyle(RefPtr<RenderStyle>& animatedStyle)
{
    if (!animatedStyle)
        animatedStyle = RenderStyle::clone(m_toStyle.get());

    blendProperties(this, m_animatingProperty, animatedStyle.get(), m_fromStyle.get(), m_toStyle.get(), progress(1, 0, 0));
}

bool ImplicitAnimation::startAnimation(double timeOffset)
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_object && m_object->hasLayer()) {
        RenderLayer* layer = toRenderBoxModelObject(m_object)->layer();
        if (layer->isComposited())
            return layer->backing()->startTransition(timeOffset, m_animatingProperty, m_fromStyle.get(), m_toStyle.get());
    }
#else
    UNUSED_PARAM(timeOffset);
#endif
    return false;
}

void ImplicitAnimation::pauseAnimation(double timeOffset)
{
    if (!m_object)
        return;

#if USE(ACCELERATED_COMPOSITING)
    if (m_object->hasLayer()) {
        RenderLayer* layer = toRenderBoxModelObject(m_object)->layer();
        if (layer->isComposited())
            layer->backing()->transitionPaused(timeOffset, m_animatingProperty);
    }
#else
    UNUSED_PARAM(timeOffset);
#endif
    // Restore the original (unanimated) style
    if (!paused())
        setNeedsStyleRecalc(m_object->node());
}

void ImplicitAnimation::endAnimation()
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_object && m_object->hasLayer()) {
        RenderLayer* layer = toRenderBoxModelObject(m_object)->layer();
        if (layer->isComposited())
            layer->backing()->transitionFinished(m_animatingProperty);
    }
#endif
}

void ImplicitAnimation::onAnimationEnd(double elapsedTime)
{
    // If we have a keyframe animation on this property, this transition is being overridden. The keyframe
    // animation keeps an unanimated style in case a transition starts while the keyframe animation is
    // running. But now that the transition has completed, we need to update this style with its new
    // destination. If we didn't, the next time through we would think a transition had started
    // (comparing the old unanimated style with the new final style of the transition).
    RefPtr<KeyframeAnimation> keyframeAnim = m_compAnim->getAnimationForProperty(m_animatingProperty);
    if (keyframeAnim)
        keyframeAnim->setUnanimatedStyle(m_toStyle);
    
    sendTransitionEvent(eventNames().webkitTransitionEndEvent, elapsedTime);
    endAnimation();
}

bool ImplicitAnimation::sendTransitionEvent(const AtomicString& eventType, double elapsedTime)
{
    if (eventType == eventNames().webkitTransitionEndEvent) {
        Document::ListenerType listenerType = Document::TRANSITIONEND_LISTENER;

        if (shouldSendEventForListener(listenerType)) {
            String propertyName;
            if (m_animatingProperty != cAnimateAll)
                propertyName = getPropertyName(static_cast<CSSPropertyID>(m_animatingProperty));
                
            // Dispatch the event
            RefPtr<Element> element = 0;
            if (m_object->node() && m_object->node()->isElementNode())
                element = static_cast<Element*>(m_object->node());

            ASSERT(!element || (element->document() && !element->document()->inPageCache()));
            if (!element)
                return false;

            // Schedule event handling
            m_compAnim->animationController()->addEventToDispatch(element, eventType, propertyName, elapsedTime);

            // Restore the original (unanimated) style
            if (eventType == eventNames().webkitTransitionEndEvent && element->renderer())
                setNeedsStyleRecalc(element.get());

            return true; // Did dispatch an event
        }
    }

    return false; // Didn't dispatch an event
}

void ImplicitAnimation::reset(RenderStyle* to)
{
    ASSERT(to);
    ASSERT(m_fromStyle);

    m_toStyle = to;

    // Restart the transition
    if (m_fromStyle && m_toStyle)
        updateStateMachine(AnimationStateInputRestartAnimation, -1);
        
    // set the transform animation list
    validateTransformFunctionList();
}

void ImplicitAnimation::setOverridden(bool b)
{
    if (b == m_overridden)
        return;

    m_overridden = b;
    updateStateMachine(m_overridden ? AnimationStateInputPauseOverride : AnimationStateInputResumeOverride, -1);
}

bool ImplicitAnimation::affectsProperty(int property) const
{
    return (m_animatingProperty == property);
}

bool ImplicitAnimation::isTargetPropertyEqual(int prop, const RenderStyle* targetStyle)
{
    // We can get here for a transition that has not started yet. This would make m_toStyle unset and null. 
    // So we check that here (see <https://bugs.webkit.org/show_bug.cgi?id=26706>)
    if (!m_toStyle)
        return false;
    return propertiesEqual(prop, m_toStyle.get(), targetStyle);
}

void ImplicitAnimation::blendPropertyValueInStyle(int prop, RenderStyle* currentStyle)
{
    // We should never add a transition with a 0 duration and delay. But if we ever did
    // it would have a null toStyle. So just in case, let's check that here. (See
    // <https://bugs.webkit.org/show_bug.cgi?id=24787>
    if (!m_toStyle)
        return;
        
    blendProperties(this, prop, currentStyle, m_fromStyle.get(), m_toStyle.get(), progress(1, 0, 0));
}

void ImplicitAnimation::validateTransformFunctionList()
{
    m_transformFunctionListValid = false;
    
    if (!m_fromStyle || !m_toStyle)
        return;
        
    const TransformOperations* val = &m_fromStyle->transform();
    const TransformOperations* toVal = &m_toStyle->transform();

    if (val->operations().isEmpty())
        val = toVal;

    if (val->operations().isEmpty())
        return;
        
    // See if the keyframes are valid
    if (val != toVal) {
        // A list of length 0 matches anything
        if (!toVal->operations().isEmpty()) {
            // If the sizes of the function lists don't match, the lists don't match
            if (val->operations().size() != toVal->operations().size())
                return;
        
            // If the types of each function are not the same, the lists don't match
            for (size_t j = 0; j < val->operations().size(); ++j) {
                if (!val->operations()[j]->isSameType(*toVal->operations()[j]))
                    return;
            }
        }
    }

    // Keyframes are valid
    m_transformFunctionListValid = true;
}

double ImplicitAnimation::timeToNextService()
{
    double t = AnimationBase::timeToNextService();
#if USE(ACCELERATED_COMPOSITING)
    if (t != 0 || preActive())
        return t;
        
    // A return value of 0 means we need service. But if this is an accelerated animation we 
    // only need service at the end of the transition.
    if (animationOfPropertyIsAccelerated(m_animatingProperty) && isAccelerated()) {
        bool isLooping;
        getTimeToNextEvent(t, isLooping);
    }
#endif
    return t;
}

} // namespace WebCore
