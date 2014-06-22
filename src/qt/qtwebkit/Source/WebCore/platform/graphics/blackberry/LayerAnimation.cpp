/*
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
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
#include "LayerAnimation.h"

#include "IdentityTransformOperation.h"
#include "LayerCompositingThread.h"
#include "TransformationMatrix.h"
#include "UnitBezier.h"

#include <algorithm>

namespace WebCore {

using namespace std;

// FIXME: Some functions below are copied from AnimationBase and KeyframeAnimation.
// We need to refactor these code to increase code reuse.
// https://bugs.webkit.org/show_bug.cgi?id=82293

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

static const TimingFunction* timingFunctionForAnimationValue(const AnimationValue* animValue, const LayerAnimation* anim)
{
    if (animValue->timingFunction())
        return animValue->timingFunction();
    if (anim->timingFunction())
        return anim->timingFunction();

    return CubicBezierTimingFunction::defaultTimingFunction();
}

static double progress(double elapsedTime, const LayerAnimation* layerAnimation, double scale, double offset, const TimingFunction* tf, bool& animationFinished)
{
    double dur = layerAnimation->duration();
    if (layerAnimation->iterationCount() > 0)
        dur *= layerAnimation->iterationCount();

    if (!layerAnimation->duration())
        return 1.0;
    if (layerAnimation->iterationCount() > 0 && elapsedTime >= dur) {
        animationFinished = true;
        return (layerAnimation->iterationCount() % 2) ? 1.0 : 0.0;
    }

    // Compute the fractional time, taking into account direction.
    // There is no need to worry about iterations, we assume that we would have
    // short circuited above if we were done.
    double fractionalTime = elapsedTime / layerAnimation->duration();
    int integralTime = static_cast<int>(fractionalTime);
    fractionalTime -= integralTime;

    if ((layerAnimation->direction() == Animation::AnimationDirectionAlternate) && (integralTime & 1))
        fractionalTime = 1 - fractionalTime;

    if (scale != 1 || offset)
        fractionalTime = (fractionalTime - offset) * scale;

    if (!tf)
        tf = layerAnimation->timingFunction();

    if (tf->isCubicBezierTimingFunction()) {
        const CubicBezierTimingFunction* ctf = static_cast<const CubicBezierTimingFunction*>(tf);
        return solveCubicBezierFunction(ctf->x1(), ctf->y1(), ctf->x2(), ctf->y2(), fractionalTime, layerAnimation->duration());
    }

    if (tf->isStepsTimingFunction()) {
        const StepsTimingFunction* stf = static_cast<const StepsTimingFunction*>(tf);
        return solveStepsFunction(stf->numberOfSteps(), stf->stepAtStart(), fractionalTime);
    }

    return fractionalTime;
}

static void fetchIntervalEndpoints(double elapsedTime, const LayerAnimation* layerAnimation, const AnimationValue*& fromValue, const AnimationValue*& toValue, double& prog, bool& animationFinished)
{
    // Find the first key.
    if (layerAnimation->duration() && layerAnimation->iterationCount() != Animation::IterationCountInfinite)
        elapsedTime = min(elapsedTime, layerAnimation->duration() * layerAnimation->iterationCount());

    double fractionalTime = layerAnimation->duration() ? (elapsedTime / layerAnimation->duration()) : 1;

    // FIXME: startTime can be before the current animation "frame" time. This is to sync with the frame time
    // concept in AnimationTimeController. So we need to somehow sync the two. Until then, the possible
    // error is small and will probably not be noticeable. Until we fix this, remove the assert.
    // https://bugs.webkit.org/show_bug.cgi?id=52037
    // ASSERT(fractionalTime >= 0);
    if (fractionalTime < 0)
        fractionalTime = 0;

    // FIXME: share this code with AnimationBase::progress().
    int iteration = static_cast<int>(fractionalTime);
    if (layerAnimation->iterationCount() != Animation::IterationCountInfinite)
        iteration = min(iteration, layerAnimation->iterationCount() - 1);

    fractionalTime -= iteration;

    bool reversing = (layerAnimation->direction() == Animation::AnimationDirectionAlternate) && (iteration & 1);
    if (reversing)
        fractionalTime = 1 - fractionalTime;

    size_t numKeyframes = layerAnimation->valueCount();
    if (!numKeyframes)
        return;

    ASSERT(!layerAnimation->valueAt(0)->keyTime());
    ASSERT(layerAnimation->valueAt(layerAnimation->valueCount() - 1)->keyTime() == 1);

    int prevIndex = -1;
    int nextIndex = -1;

    // FIXME: with a lot of keys, this linear search will be slow. We could binary search.
    for (size_t i = 0; i < numKeyframes; ++i) {
        const AnimationValue* currKeyframe = layerAnimation->valueAt(i);

        if (fractionalTime < currKeyframe->keyTime()) {
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
        nextIndex = layerAnimation->valueCount() - 1;

    const AnimationValue* prevKeyframe = layerAnimation->valueAt(prevIndex);
    const AnimationValue* nextKeyframe = layerAnimation->valueAt(nextIndex);

    fromValue = prevKeyframe;
    toValue = nextKeyframe;

    offset = prevKeyframe->keyTime();
    scale = 1.0 / (nextKeyframe->keyTime() - prevKeyframe->keyTime());

    const TimingFunction* timingFunction = timingFunctionForAnimationValue(prevKeyframe, layerAnimation);

    prog = progress(elapsedTime, layerAnimation, scale, offset, timingFunction, animationFinished);
}

void LayerAnimation::apply(LayerCompositingThread* layer, double elapsedTime)
{
    const AnimationValue* from = 0;
    const AnimationValue* to = 0;
    double progress = 0.0;
    bool animationFinished = false;

    fetchIntervalEndpoints(elapsedTime, this, from, to, progress, animationFinished);

    switch (property()) {
    case AnimatedPropertyWebkitTransform:
        layer->setTransform(blendTransform(static_cast<const TransformAnimationValue*>(from)->value(), static_cast<const TransformAnimationValue*>(to)->value(), progress));
        break;
    case AnimatedPropertyOpacity:
        layer->setOpacity(blendOpacity(static_cast<const FloatAnimationValue*>(from)->value(), static_cast<const FloatAnimationValue*>(to)->value(), progress));
        break;
    case AnimatedPropertyBackgroundColor:
    case AnimatedPropertyWebkitFilter:
    case AnimatedPropertyInvalid:
        ASSERT_NOT_REACHED();
        break;
    }

    m_finished = animationFinished;
}

TransformationMatrix LayerAnimation::blendTransform(const TransformOperations* from, const TransformOperations* to, double progress) const
{
    TransformationMatrix t;

    if (m_transformFunctionListValid) {
        // A trick to avoid touching the refcount of shared TransformOperations on the wrong thread.
        // Since TransforOperation is not ThreadSafeRefCounted, we are only allowed to touch the ref
        // count of shared operations when the WebKit thread and compositing thread are in sync.
        Vector<TransformOperation*> result;
        Vector<RefPtr<TransformOperation> > owned;

        unsigned fromSize = from->operations().size();
        unsigned toSize = to->operations().size();
        unsigned size = max(fromSize, toSize);
        for (unsigned i = 0; i < size; i++) {
            TransformOperation* fromOp = (i < fromSize) ? from->operations()[i].get() : 0;
            TransformOperation* toOp = (i < toSize) ? to->operations()[i].get() : 0;
            RefPtr<TransformOperation> blendedOp = toOp ? toOp->blend(fromOp, progress) : (fromOp ? fromOp->blend(0, progress, true) : PassRefPtr<TransformOperation>(0));
            if (blendedOp) {
                result.append(blendedOp.get());
                owned.append(blendedOp);
            } else {
                RefPtr<TransformOperation> identityOp = IdentityTransformOperation::create();
                owned.append(identityOp);
                if (progress > 0.5)
                    result.append(toOp ? toOp : identityOp.get());
                else
                    result.append(fromOp ? fromOp : identityOp.get());
            }
        }

        IntSize sz = boxSize();
        for (unsigned i = 0; i < result.size(); ++i)
            result[i]->apply(t, sz);
    } else {
        // Convert the TransformOperations into matrices.
        TransformationMatrix fromT;
        from->apply(boxSize(), fromT);
        to->apply(boxSize(), t);

        t.blend(fromT, progress);
    }

    return t;
}

float LayerAnimation::blendOpacity(float from, float to, double progress) const
{
    float opacity = from + (to - from) * progress;

    return max(0.0f, min(opacity, 1.0f));
}

void LayerAnimation::validateTransformLists()
{
    m_transformFunctionListValid = false;

    if (m_values.size() < 2 || property() != AnimatedPropertyWebkitTransform)
        return;

    // Empty transforms match anything, so find the first non-empty entry as the reference.
    size_t numKeyframes = m_values.size();
    size_t firstNonEmptyTransformKeyframeIndex = numKeyframes;

    for (size_t i = 0; i < numKeyframes; ++i) {
        const TransformAnimationValue* currentKeyframe = static_cast<const TransformAnimationValue*>(m_values.at(i));
        if (currentKeyframe->value()->size()) {
            firstNonEmptyTransformKeyframeIndex = i;
            break;
        }
    }

    if (firstNonEmptyTransformKeyframeIndex == numKeyframes)
        return;

    const TransformOperations* firstVal = static_cast<const TransformAnimationValue*>(m_values.at(firstNonEmptyTransformKeyframeIndex))->value();

    // See if the keyframes are valid.
    for (size_t i = firstNonEmptyTransformKeyframeIndex + 1; i < numKeyframes; ++i) {
        const TransformAnimationValue* currentKeyframe = static_cast<const TransformAnimationValue*>(m_values.at(i));
        const TransformOperations* val = currentKeyframe->value();

        // A null transform matches anything.
        if (val->operations().isEmpty())
            continue;

        // If the sizes of the function lists don't match, the lists don't match.
        if (firstVal->operations().size() != val->operations().size())
            return;

        // If the types of each function are not the same, the lists don't match.
        for (size_t j = 0; j < firstVal->operations().size(); ++j) {
            if (!firstVal->operations()[j]->isSameType(*val->operations()[j]))
                return;
        }
    }

    // Keyframes are valid.
    m_transformFunctionListValid = true;
}

}
