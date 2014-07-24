/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)
#include "GraphicsLayerAnimation.h"

#include "LayoutSize.h"
#include "UnitBezier.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

#if ENABLE(CSS_FILTERS)
static inline PassRefPtr<FilterOperation> blendFunc(FilterOperation* fromOp, FilterOperation* toOp, double progress, const IntSize& size, bool blendToPassthrough = false)
{
    ASSERT(toOp);
    if (toOp->blendingNeedsRendererSize())
        return toOp->blend(fromOp, progress, LayoutSize(size.width(), size.height()), blendToPassthrough);

    return toOp->blend(fromOp, progress, blendToPassthrough);
}


static FilterOperations applyFilterAnimation(const FilterOperations& from, const FilterOperations& to, double progress, const IntSize& boxSize)
{
    // First frame of an animation.
    if (!progress)
        return from;

    // Last frame of an animation.
    if (progress == 1)
        return to;

    if (!from.isEmpty() && !to.isEmpty() && !from.operationsMatch(to))
        return to;

    FilterOperations result;

    size_t fromSize = from.operations().size();
    size_t toSize = to.operations().size();
    size_t size = std::max(fromSize, toSize);
    for (size_t i = 0; i < size; i++) {
        RefPtr<FilterOperation> fromOp = (i < fromSize) ? from.operations()[i].get() : 0;
        RefPtr<FilterOperation> toOp = (i < toSize) ? to.operations()[i].get() : 0;
        RefPtr<FilterOperation> blendedOp = toOp ? blendFunc(fromOp.get(), toOp.get(), progress, boxSize) : (fromOp ? blendFunc(0, fromOp.get(), progress, boxSize, true) : 0);
        if (blendedOp)
            result.operations().append(blendedOp);
        else {
            RefPtr<FilterOperation> identityOp = PassthroughFilterOperation::create();
            if (progress > 0.5)
                result.operations().append(toOp ? toOp : identityOp);
            else
                result.operations().append(fromOp ? fromOp : identityOp);
        }
    }

    return result;
}
#endif

static bool shouldReverseAnimationValue(Animation::AnimationDirection direction, int loopCount)
{
    if (((direction == Animation::AnimationDirectionAlternate) && (loopCount & 1))
        || ((direction == Animation::AnimationDirectionAlternateReverse) && !(loopCount & 1))
        || direction == Animation::AnimationDirectionReverse)
        return true;
    return false;
}

static double normalizedAnimationValue(double runningTime, double duration, Animation::AnimationDirection direction, double iterationCount)
{
    if (!duration)
        return 0;

    const int loopCount = runningTime / duration;
    const double lastFullLoop = duration * double(loopCount);
    const double remainder = runningTime - lastFullLoop;
    // Ignore remainder when we've reached the end of animation.
    const double normalized = (loopCount == iterationCount) ? 1.0 : (remainder / duration);

    return shouldReverseAnimationValue(direction, loopCount) ? 1 - normalized : normalized;
}

static double normalizedAnimationValueForFillsForwards(double iterationCount, Animation::AnimationDirection direction)
{
    if (direction == Animation::AnimationDirectionNormal)
        return 1;
    if (direction == Animation::AnimationDirectionReverse)
        return 0;
    return shouldReverseAnimationValue(direction, iterationCount) ? 1 : 0;
}

static float applyOpacityAnimation(float fromOpacity, float toOpacity, double progress)
{
    // Optimization: special case the edge values (0 and 1).
    if (progress == 1.0)
        return toOpacity;

    if (!progress)
        return fromOpacity;

    return fromOpacity + progress * (toOpacity - fromOpacity);
}

static inline double solveEpsilon(double duration)
{
    return 1.0 / (200.0 * duration);
}

static inline double solveCubicBezierFunction(double p1x, double p1y, double p2x, double p2y, double t, double duration)
{
    return UnitBezier(p1x, p1y, p2x, p2y).solve(t, solveEpsilon(duration));
}

static inline double solveStepsFunction(int numSteps, bool stepAtStart, double t)
{
    if (stepAtStart)
        return std::min(1.0, (floor(numSteps * t) + 1) / numSteps);
    return floor(numSteps * t) / numSteps;
}

static inline float applyTimingFunction(const TimingFunction* timingFunction, float progress, double duration)
{
    if (!timingFunction)
        return progress;

    if (timingFunction->isCubicBezierTimingFunction()) {
        const CubicBezierTimingFunction* ctf = static_cast<const CubicBezierTimingFunction*>(timingFunction);
        return solveCubicBezierFunction(ctf->x1(), ctf->y1(), ctf->x2(), ctf->y2(), progress, duration);
    }

    if (timingFunction->isStepsTimingFunction()) {
        const StepsTimingFunction* stf = static_cast<const StepsTimingFunction*>(timingFunction);
        return solveStepsFunction(stf->numberOfSteps(), stf->stepAtStart(), double(progress));
    }

    return progress;
}

static TransformationMatrix applyTransformAnimation(const TransformOperations& from, const TransformOperations& to, double progress, const IntSize& boxSize, bool listsMatch)
{
    TransformationMatrix matrix;

    // First frame of an animation.
    if (!progress) {
        from.apply(boxSize, matrix);
        return matrix;
    }

    // Last frame of an animation.
    if (progress == 1) {
        to.apply(boxSize, matrix);
        return matrix;
    }

    // If we have incompatible operation lists, we blend the resulting matrices.
    if (!listsMatch) {
        TransformationMatrix fromMatrix;
        to.apply(boxSize, matrix);
        from.apply(boxSize, fromMatrix);
        matrix.blend(fromMatrix, progress);
        return matrix;
    }

    // Animation to "-webkit-transform: none".
    if (!to.size()) {
        TransformOperations blended(from);
        for (size_t i = 0; i < blended.operations().size(); ++i)
            blended.operations()[i]->blend(0, progress, true)->apply(matrix, boxSize);
        return matrix;
    }

    // Animation from "-webkit-transform: none".
    if (!from.size()) {
        TransformOperations blended(to);
        for (size_t i = 0; i < blended.operations().size(); ++i)
            blended.operations()[i]->blend(0, 1. - progress, true)->apply(matrix, boxSize);
        return matrix;
    }

    // Normal animation with a matching operation list.
    TransformOperations blended(to);
    for (size_t i = 0; i < blended.operations().size(); ++i)
        blended.operations()[i]->blend(from.at(i), progress, !from.at(i))->apply(matrix, boxSize);
    return matrix;
}

static const TimingFunction* timingFunctionForAnimationValue(const AnimationValue& animValue, const Animation* anim)
{
    if (animValue.timingFunction())
        return animValue.timingFunction();
    if (anim->timingFunction())
        return anim->timingFunction().get();

    return CubicBezierTimingFunction::defaultTimingFunction();
}

GraphicsLayerAnimation::GraphicsLayerAnimation(const String& name, const KeyframeValueList& keyframes, const IntSize& boxSize, const Animation* animation, double startTime, bool listsMatch)
    : m_keyframes(keyframes)
    , m_boxSize(boxSize)
    , m_animation(Animation::create(animation))
    , m_name(name)
    , m_listsMatch(listsMatch)
    , m_startTime(startTime)
    , m_pauseTime(0)
    , m_totalRunningTime(0)
    , m_lastRefreshedTime(m_startTime)
    , m_state(PlayingState)
{
}

void GraphicsLayerAnimation::applyInternal(Client* client, const AnimationValue& from, const AnimationValue& to, float progress)
{
    switch (m_keyframes.property()) {
    case AnimatedPropertyOpacity:
        client->setAnimatedOpacity(applyOpacityAnimation((static_cast<const FloatAnimationValue&>(from).value()), (static_cast<const FloatAnimationValue&>(to).value()), progress));
        return;
    case AnimatedPropertyWebkitTransform:
        client->setAnimatedTransform(applyTransformAnimation(static_cast<const TransformAnimationValue&>(from).value(), static_cast<const TransformAnimationValue&>(to).value(), progress, m_boxSize, m_listsMatch));
        return;
#if ENABLE(CSS_FILTERS)
    case AnimatedPropertyWebkitFilter:
        client->setAnimatedFilters(applyFilterAnimation(static_cast<const FilterAnimationValue&>(from).value(), static_cast<const FilterAnimationValue&>(to).value(), progress, m_boxSize));
        return;
#endif
    default:
        ASSERT_NOT_REACHED();
    }
}

bool GraphicsLayerAnimation::isActive() const
{
    if (state() != StoppedState)
        return true;

    return m_animation->fillsForwards();
}

bool GraphicsLayerAnimations::hasActiveAnimationsOfType(AnimatedPropertyID type) const
{
    for (size_t i = 0; i < m_animations.size(); ++i) {
        if (m_animations[i].isActive() && m_animations[i].property() == type)
            return true;
    }
    return false;
}

bool GraphicsLayerAnimations::hasRunningAnimations() const
{
    for (size_t i = 0; i < m_animations.size(); ++i) {
        if (m_animations[i].state() == GraphicsLayerAnimation::PlayingState)
            return true;
    }

    return false;
}

void GraphicsLayerAnimation::apply(Client* client)
{
    if (!isActive())
        return;

    double totalRunningTime = computeTotalRunningTime();
    double normalizedValue = normalizedAnimationValue(totalRunningTime, m_animation->duration(), m_animation->direction(), m_animation->iterationCount());

    if (m_animation->iterationCount() != Animation::IterationCountInfinite && totalRunningTime >= m_animation->duration() * m_animation->iterationCount()) {
        setState(StoppedState);
        if (m_animation->fillsForwards())
            normalizedValue = normalizedAnimationValueForFillsForwards(m_animation->iterationCount(), m_animation->direction());
    }

    if (!normalizedValue) {
        applyInternal(client, m_keyframes.at(0), m_keyframes.at(1), 0);
        return;
    }

    if (normalizedValue == 1.0) {
        applyInternal(client, m_keyframes.at(m_keyframes.size() - 2), m_keyframes.at(m_keyframes.size() - 1), 1);
        return;
    }
    if (m_keyframes.size() == 2) {
        const TimingFunction* timingFunction = timingFunctionForAnimationValue(m_keyframes.at(0), m_animation.get());
        normalizedValue = applyTimingFunction(timingFunction, normalizedValue, m_animation->duration());
        applyInternal(client, m_keyframes.at(0), m_keyframes.at(1), normalizedValue);
        return;
    }

    for (size_t i = 0; i < m_keyframes.size() - 1; ++i) {
        const AnimationValue& from = m_keyframes.at(i);
        const AnimationValue& to = m_keyframes.at(i + 1);
        if (from.keyTime() > normalizedValue || to.keyTime() < normalizedValue)
            continue;

        normalizedValue = (normalizedValue - from.keyTime()) / (to.keyTime() - from.keyTime());
        const TimingFunction* timingFunction = timingFunctionForAnimationValue(from, m_animation.get());
        normalizedValue = applyTimingFunction(timingFunction, normalizedValue, m_animation->duration());
        applyInternal(client, from, to, normalizedValue);
        break;
    }
}

double GraphicsLayerAnimation::computeTotalRunningTime()
{
    if (state() == PausedState)
        return m_pauseTime;

    double oldLastRefreshedTime = m_lastRefreshedTime;
    m_lastRefreshedTime = WTF::currentTime();
    m_totalRunningTime += m_lastRefreshedTime - oldLastRefreshedTime;
    return m_totalRunningTime;
}

void GraphicsLayerAnimation::pause(double time)
{
    setState(PausedState);
    m_pauseTime = time;
}

void GraphicsLayerAnimation::resume()
{
    setState(PlayingState);
    m_totalRunningTime = m_pauseTime;
    m_lastRefreshedTime = WTF::currentTime();
}

void GraphicsLayerAnimations::add(const GraphicsLayerAnimation& animation)
{
    // Remove the old state if we are resuming a paused animation.
    remove(animation.name(), animation.property());

    m_animations.append(animation);
}

void GraphicsLayerAnimations::pause(const String& name, double offset)
{
    for (size_t i = 0; i < m_animations.size(); ++i) {
        if (m_animations[i].name() == name)
            m_animations[i].pause(offset);
    }
}

void GraphicsLayerAnimations::suspend(double offset)
{
    for (size_t i = 0; i < m_animations.size(); ++i)
        m_animations[i].pause(offset);
}

void GraphicsLayerAnimations::resume()
{
    for (size_t i = 0; i < m_animations.size(); ++i)
        m_animations[i].resume();
}

void GraphicsLayerAnimations::remove(const String& name)
{
    for (int i = m_animations.size() - 1; i >= 0; --i) {
        if (m_animations[i].name() == name)
            m_animations.remove(i);
    }
}

void GraphicsLayerAnimations::remove(const String& name, AnimatedPropertyID property)
{
    for (int i = m_animations.size() - 1; i >= 0; --i) {
        if (m_animations[i].name() == name && m_animations[i].property() == property)
            m_animations.remove(i);
    }
}

void GraphicsLayerAnimations::apply(GraphicsLayerAnimation::Client* client)
{
    for (size_t i = 0; i < m_animations.size(); ++i)
        m_animations[i].apply(client);
}

GraphicsLayerAnimations GraphicsLayerAnimations::getActiveAnimations() const
{
    GraphicsLayerAnimations active;
    for (size_t i = 0; i < m_animations.size(); ++i) {
        if (m_animations[i].isActive())
            active.add(m_animations[i]);
    }
    return active;
}
}
#endif

