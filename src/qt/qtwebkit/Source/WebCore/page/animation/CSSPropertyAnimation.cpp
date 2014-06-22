/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
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
#include "CSSPropertyAnimation.h"

#include "AnimationBase.h"
#include "CSSCrossfadeValue.h"
#include "CSSImageGeneratorValue.h"
#include "CSSImageValue.h"
#include "CSSPrimitiveValue.h"
#include "CSSPropertyNames.h"
#include "CachedImage.h"
#include "ClipPathOperation.h"
#include "FloatConversion.h"
#include "IdentityTransformOperation.h"
#include "Matrix3DTransformOperation.h"
#include "MatrixTransformOperation.h"
#include "RenderBox.h"
#include "RenderStyle.h"
#include "StyleCachedImage.h"
#include "StyleGeneratedImage.h"
#include "StylePropertyShorthand.h"
#include "StyleResolver.h"
#include <algorithm>
#include <wtf/Noncopyable.h>
#include <wtf/RefCounted.h>

namespace WebCore {

static inline int blendFunc(const AnimationBase*, int from, int to, double progress)
{
    return blend(from, to, progress);
}

static inline unsigned blendFunc(const AnimationBase*, unsigned from, unsigned to, double progress)
{
    return blend(from, to, progress);
}

static inline double blendFunc(const AnimationBase*, double from, double to, double progress)
{
    return blend(from, to, progress);
}

static inline float blendFunc(const AnimationBase*, float from, float to, double progress)
{
    return narrowPrecisionToFloat(from + (to - from) * progress);
}

static inline Color blendFunc(const AnimationBase*, const Color& from, const Color& to, double progress)
{
    return blend(from, to, progress);
}

static inline Length blendFunc(const AnimationBase*, const Length& from, const Length& to, double progress)
{
    return to.blend(from, narrowPrecisionToFloat(progress));
}

static inline LengthSize blendFunc(const AnimationBase* anim, const LengthSize& from, const LengthSize& to, double progress)
{
    return LengthSize(blendFunc(anim, from.width(), to.width(), progress),
                      blendFunc(anim, from.height(), to.height(), progress));
}

static inline IntSize blendFunc(const AnimationBase* anim, const IntSize& from, const IntSize& to, double progress)
{
    return IntSize(blendFunc(anim, from.width(), to.width(), progress),
                   blendFunc(anim, from.height(), to.height(), progress));
}

static inline ShadowStyle blendFunc(const AnimationBase* anim, ShadowStyle from, ShadowStyle to, double progress)
{
    if (from == to)
        return to;

    double fromVal = from == Normal ? 1 : 0;
    double toVal = to == Normal ? 1 : 0;
    double result = blendFunc(anim, fromVal, toVal, progress);
    return result > 0 ? Normal : Inset;
}

static inline PassOwnPtr<ShadowData> blendFunc(const AnimationBase* anim, const ShadowData* from, const ShadowData* to, double progress)
{
    ASSERT(from && to);
    if (from->style() != to->style())
        return adoptPtr(new ShadowData(*to));

    return adoptPtr(new ShadowData(blend(from->location(), to->location(), progress),
                                   blend(from->radius(), to->radius(), progress),
                                   blend(from->spread(), to->spread(), progress),
                                   blendFunc(anim, from->style(), to->style(), progress),
                                   from->isWebkitBoxShadow(),
                                   blend(from->color(), to->color(), progress)));
}

static inline TransformOperations blendFunc(const AnimationBase* anim, const TransformOperations& from, const TransformOperations& to, double progress)
{
    if (anim->isTransformFunctionListValid())
        return to.blendByMatchingOperations(from, progress);
    return to.blendByUsingMatrixInterpolation(from, progress, anim->renderer()->isBox() ? toRenderBox(anim->renderer())->borderBoxRect().size() : LayoutSize());
}

static inline PassRefPtr<ClipPathOperation> blendFunc(const AnimationBase*, ClipPathOperation* from, ClipPathOperation* to, double progress)
{
    // Other clip-path operations than BasicShapes can not be animated.
    if (from->getOperationType() != ClipPathOperation::SHAPE || to->getOperationType() != ClipPathOperation::SHAPE)
        return to;

    const BasicShape* fromShape = static_cast<ShapeClipPathOperation*>(from)->basicShape();
    const BasicShape* toShape = static_cast<ShapeClipPathOperation*>(to)->basicShape();

    if (!fromShape->canBlend(toShape))
        return to;

    return ShapeClipPathOperation::create(toShape->blend(fromShape, progress));
}

#if ENABLE(CSS_SHAPES)
static inline PassRefPtr<ShapeValue> blendFunc(const AnimationBase*, ShapeValue* from, ShapeValue* to, double progress)
{
    // FIXME Bug 102723: Shape-inside should be able to animate a value of 'outside-shape' when shape-outside is set to a BasicShape
    if (from->type() != ShapeValue::Shape || to->type() != ShapeValue::Shape)
        return to;

    const BasicShape* fromShape = from->shape();
    const BasicShape* toShape = to->shape();

    if (!fromShape->canBlend(toShape))
        return to;

    return ShapeValue::createShapeValue(toShape->blend(fromShape, progress));
}
#endif

#if ENABLE(CSS_FILTERS)
static inline PassRefPtr<FilterOperation> blendFunc(const AnimationBase* anim, FilterOperation* fromOp, FilterOperation* toOp, double progress, bool blendToPassthrough = false)
{
    ASSERT(toOp);
    if (toOp->blendingNeedsRendererSize()) {
        LayoutSize size = anim->renderer()->isBox() ? toRenderBox(anim->renderer())->borderBoxRect().size() : LayoutSize();
        return toOp->blend(fromOp, progress, size, blendToPassthrough);
    }
    return toOp->blend(fromOp, progress, blendToPassthrough);
}

static inline FilterOperations blendFunc(const AnimationBase* anim, const FilterOperations& from, const FilterOperations& to, double progress)
{
    FilterOperations result;

    // If we have a filter function list, use that to do a per-function animation.
    if (anim->filterFunctionListsMatch()) {
        size_t fromSize = from.operations().size();
        size_t toSize = to.operations().size();
        size_t size = max(fromSize, toSize);
        for (size_t i = 0; i < size; i++) {
            RefPtr<FilterOperation> fromOp = (i < fromSize) ? from.operations()[i].get() : 0;
            RefPtr<FilterOperation> toOp = (i < toSize) ? to.operations()[i].get() : 0;
            RefPtr<FilterOperation> blendedOp = toOp ? blendFunc(anim, fromOp.get(), toOp.get(), progress) : (fromOp ? blendFunc(anim, 0, fromOp.get(), progress, true) : 0);
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
    } else {
        // If the filter function lists don't match, we could try to cross-fade, but don't yet have a way to represent that in CSS.
        // For now we'll just fail to animate.
        result = to;
    }

    return result;
}
#endif // ENABLE(CSS_FILTERS)

static inline EVisibility blendFunc(const AnimationBase* anim, EVisibility from, EVisibility to, double progress)
{
    // Any non-zero result means we consider the object to be visible. Only at 0 do we consider the object to be
    // invisible. The invisible value we use (HIDDEN vs. COLLAPSE) depends on the specified from/to values.
    double fromVal = from == VISIBLE ? 1. : 0.;
    double toVal = to == VISIBLE ? 1. : 0.;
    if (fromVal == toVal)
        return to;
    double result = blendFunc(anim, fromVal, toVal, progress);
    return result > 0. ? VISIBLE : (to != VISIBLE ? to : from);
}

static inline LengthBox blendFunc(const AnimationBase* anim, const LengthBox& from, const LengthBox& to, double progress)
{
    LengthBox result(blendFunc(anim, from.top(), to.top(), progress),
                     blendFunc(anim, from.right(), to.right(), progress),
                     blendFunc(anim, from.bottom(), to.bottom(), progress),
                     blendFunc(anim, from.left(), to.left(), progress));
    return result;
}

#if ENABLE(SVG)
static inline SVGLength blendFunc(const AnimationBase*, const SVGLength& from, const SVGLength& to, double progress)
{
    return to.blend(from, narrowPrecisionToFloat(progress));
}
#endif

static inline PassRefPtr<StyleImage> crossfadeBlend(const AnimationBase*, StyleCachedImage* fromStyleImage, StyleCachedImage* toStyleImage, double progress)
{
    // If progress is at one of the extremes, we want getComputedStyle to show the image,
    // not a completed cross-fade, so we hand back one of the existing images.
    if (!progress)
        return fromStyleImage;
    if (progress == 1)
        return toStyleImage;

    CachedImage* fromCachedImage = static_cast<CachedImage*>(fromStyleImage->data());
    CachedImage* toCachedImage = static_cast<CachedImage*>(toStyleImage->data());

    RefPtr<CSSImageValue> fromImageValue = CSSImageValue::create(fromCachedImage->url(), fromStyleImage);
    RefPtr<CSSImageValue> toImageValue = CSSImageValue::create(toCachedImage->url(), toStyleImage);
    RefPtr<CSSCrossfadeValue> crossfadeValue = CSSCrossfadeValue::create(fromImageValue, toImageValue);

    crossfadeValue->setPercentage(CSSPrimitiveValue::create(progress, CSSPrimitiveValue::CSS_NUMBER));

    return StyleGeneratedImage::create(crossfadeValue.get());
}

static inline PassRefPtr<StyleImage> blendFunc(const AnimationBase* anim, StyleImage* from, StyleImage* to, double progress)
{
    if (!from || !to)
        return to;

    if (from->isCachedImage() && to->isCachedImage())
        return crossfadeBlend(anim, static_cast<StyleCachedImage*>(from), static_cast<StyleCachedImage*>(to), progress);

    // FIXME: Support transitioning generated images as well. (gradients, etc.)

    return to;
}

static inline NinePieceImage blendFunc(const AnimationBase* anim, const NinePieceImage& from, const NinePieceImage& to, double progress)
{
    if (!from.hasImage() || !to.hasImage())
        return to;

    // FIXME (74112): Support transitioning between NinePieceImages that differ by more than image content.

    if (from.imageSlices() != to.imageSlices() || from.borderSlices() != to.borderSlices() || from.outset() != to.outset() || from.fill() != to.fill() || from.horizontalRule() != to.horizontalRule() || from.verticalRule() != to.verticalRule())
        return to;

    if (from.image()->imageSize(anim->renderer(), 1.0) != to.image()->imageSize(anim->renderer(), 1.0))
        return to;

    RefPtr<StyleImage> newContentImage = blendFunc(anim, from.image(), to.image(), progress);

    return NinePieceImage(newContentImage, from.imageSlices(), from.fill(), from.borderSlices(), from.outset(), from.horizontalRule(), from.verticalRule());
}

class AnimationPropertyWrapperBase {
    WTF_MAKE_NONCOPYABLE(AnimationPropertyWrapperBase);
    WTF_MAKE_FAST_ALLOCATED;
public:
    AnimationPropertyWrapperBase(CSSPropertyID prop)
        : m_prop(prop)
    {
    }

    virtual ~AnimationPropertyWrapperBase() { }

    virtual bool isShorthandWrapper() const { return false; }
    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const = 0;
    virtual void blend(const AnimationBase*, RenderStyle*, const RenderStyle*, const RenderStyle*, double) const = 0;

    CSSPropertyID property() const { return m_prop; }

#if USE(ACCELERATED_COMPOSITING)
    virtual bool animationIsAccelerated() const { return false; }
#endif

private:
    CSSPropertyID m_prop;
};

static int gPropertyWrapperMap[numCSSProperties];
static const int cInvalidPropertyWrapperIndex = -1;
static Vector<AnimationPropertyWrapperBase*>* gPropertyWrappers = 0;

static void addPropertyWrapper(CSSPropertyID propertyID, AnimationPropertyWrapperBase* wrapper)
{
    int propIndex = propertyID - firstCSSProperty;

    ASSERT(gPropertyWrapperMap[propIndex] == cInvalidPropertyWrapperIndex);

    unsigned wrapperIndex = gPropertyWrappers->size();
    gPropertyWrappers->append(wrapper);
    gPropertyWrapperMap[propIndex] = wrapperIndex;
}

static AnimationPropertyWrapperBase* wrapperForProperty(CSSPropertyID propertyID)
{
    int propIndex = propertyID - firstCSSProperty;
    if (propIndex >= 0 && propIndex < numCSSProperties) {
        int wrapperIndex = gPropertyWrapperMap[propIndex];
        if (wrapperIndex >= 0)
            return (*gPropertyWrappers)[wrapperIndex];
    }
    return 0;
}

template <typename T>
class PropertyWrapperGetter : public AnimationPropertyWrapperBase {
public:
    PropertyWrapperGetter(CSSPropertyID prop, T (RenderStyle::*getter)() const)
        : AnimationPropertyWrapperBase(prop)
        , m_getter(getter)
    {
    }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        // If the style pointers are the same, don't bother doing the test.
        // If either is null, return false. If both are null, return true.
        if ((!a && !b) || a == b)
            return true;
        if (!a || !b)
            return false;
        return (a->*m_getter)() == (b->*m_getter)();
    }

protected:
    T (RenderStyle::*m_getter)() const;
};

template <typename T>
class PropertyWrapper : public PropertyWrapperGetter<T> {
public:
    PropertyWrapper(CSSPropertyID prop, T (RenderStyle::*getter)() const, void (RenderStyle::*setter)(T))
        : PropertyWrapperGetter<T>(prop, getter)
        , m_setter(setter)
    {
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        (dst->*m_setter)(blendFunc(anim, (a->*PropertyWrapperGetter<T>::m_getter)(), (b->*PropertyWrapperGetter<T>::m_getter)(), progress));
    }

protected:
    void (RenderStyle::*m_setter)(T);
};

template <typename T>
class RefCountedPropertyWrapper : public PropertyWrapperGetter<T*> {
public:
    RefCountedPropertyWrapper(CSSPropertyID prop, T* (RenderStyle::*getter)() const, void (RenderStyle::*setter)(PassRefPtr<T>))
        : PropertyWrapperGetter<T*>(prop, getter)
        , m_setter(setter)
    {
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        (dst->*m_setter)(blendFunc(anim, (a->*PropertyWrapperGetter<T*>::m_getter)(), (b->*PropertyWrapperGetter<T*>::m_getter)(), progress));
    }

protected:
    void (RenderStyle::*m_setter)(PassRefPtr<T>);
};


class PropertyWrapperClipPath : public RefCountedPropertyWrapper<ClipPathOperation> {
public:
    PropertyWrapperClipPath(CSSPropertyID prop, ClipPathOperation* (RenderStyle::*getter)() const, void (RenderStyle::*setter)(PassRefPtr<ClipPathOperation>))
        : RefCountedPropertyWrapper<ClipPathOperation>(prop, getter, setter)
    {
    }
};

#if ENABLE(CSS_SHAPES)
class PropertyWrapperShape : public RefCountedPropertyWrapper<ShapeValue> {
public:
    PropertyWrapperShape(CSSPropertyID prop, ShapeValue* (RenderStyle::*getter)() const, void (RenderStyle::*setter)(PassRefPtr<ShapeValue>))
        : RefCountedPropertyWrapper<ShapeValue>(prop, getter, setter)
    {
    }
};
#endif

class StyleImagePropertyWrapper : public RefCountedPropertyWrapper<StyleImage> {
public:
    StyleImagePropertyWrapper(CSSPropertyID prop, StyleImage* (RenderStyle::*getter)() const, void (RenderStyle::*setter)(PassRefPtr<StyleImage>))
        : RefCountedPropertyWrapper<StyleImage>(prop, getter, setter)
    {
    }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
       // If the style pointers are the same, don't bother doing the test.
       // If either is null, return false. If both are null, return true.
       if (a == b)
           return true;
       if (!a || !b)
            return false;

        StyleImage* imageA = (a->*m_getter)();
        StyleImage* imageB = (b->*m_getter)();
        return StyleImage::imagesEquivalent(imageA, imageB);
    }
};

class PropertyWrapperColor : public PropertyWrapperGetter<Color> {
public:
    PropertyWrapperColor(CSSPropertyID prop, Color (RenderStyle::*getter)() const, void (RenderStyle::*setter)(const Color&))
        : PropertyWrapperGetter<Color>(prop, getter)
        , m_setter(setter)
    {
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        (dst->*m_setter)(blendFunc(anim, (a->*PropertyWrapperGetter<Color>::m_getter)(), (b->*PropertyWrapperGetter<Color>::m_getter)(), progress));
    }

protected:
    void (RenderStyle::*m_setter)(const Color&);
};

#if USE(ACCELERATED_COMPOSITING)
class PropertyWrapperAcceleratedOpacity : public PropertyWrapper<float> {
public:
    PropertyWrapperAcceleratedOpacity()
        : PropertyWrapper<float>(CSSPropertyOpacity, &RenderStyle::opacity, &RenderStyle::setOpacity)
    {
    }

    virtual bool animationIsAccelerated() const { return true; }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        float fromOpacity = a->opacity();

        // This makes sure we put the object being animated into a RenderLayer during the animation
        dst->setOpacity(blendFunc(anim, (fromOpacity == 1) ? 0.999999f : fromOpacity, b->opacity(), progress));
    }
};

class PropertyWrapperAcceleratedTransform : public PropertyWrapper<const TransformOperations&> {
public:
    PropertyWrapperAcceleratedTransform()
        : PropertyWrapper<const TransformOperations&>(CSSPropertyWebkitTransform, &RenderStyle::transform, &RenderStyle::setTransform)
    {
    }

    virtual bool animationIsAccelerated() const { return true; }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        dst->setTransform(blendFunc(anim, a->transform(), b->transform(), progress));
    }
};

#if ENABLE(CSS_FILTERS)
class PropertyWrapperAcceleratedFilter : public PropertyWrapper<const FilterOperations&> {
public:
    PropertyWrapperAcceleratedFilter()
        : PropertyWrapper<const FilterOperations&>(CSSPropertyWebkitFilter, &RenderStyle::filter, &RenderStyle::setFilter)
    {
    }

    virtual bool animationIsAccelerated() const { return true; }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        dst->setFilter(blendFunc(anim, a->filter(), b->filter(), progress));
    }
};
#endif
#endif // USE(ACCELERATED_COMPOSITING)

static inline size_t shadowListLength(const ShadowData* shadow)
{
    size_t count;
    for (count = 0; shadow; shadow = shadow->next())
        ++count;
    return count;
}

static inline const ShadowData* shadowForBlending(const ShadowData* srcShadow, const ShadowData* otherShadow)
{
    DEFINE_STATIC_LOCAL(ShadowData, defaultShadowData, (IntPoint(), 0, 0, Normal, false, Color::transparent));
    DEFINE_STATIC_LOCAL(ShadowData, defaultInsetShadowData, (IntPoint(), 0, 0, Inset, false, Color::transparent));

    DEFINE_STATIC_LOCAL(ShadowData, defaultWebKitBoxShadowData, (IntPoint(), 0, 0, Normal, true, Color::transparent));
    DEFINE_STATIC_LOCAL(ShadowData, defaultInsetWebKitBoxShadowData, (IntPoint(), 0, 0, Inset, true, Color::transparent));

    if (srcShadow)
        return srcShadow;

    if (otherShadow->style() == Inset)
        return otherShadow->isWebkitBoxShadow() ? &defaultInsetWebKitBoxShadowData : &defaultInsetShadowData;

    return otherShadow->isWebkitBoxShadow() ? &defaultWebKitBoxShadowData : &defaultShadowData;
}

class PropertyWrapperShadow : public AnimationPropertyWrapperBase {
public:
    PropertyWrapperShadow(CSSPropertyID prop, const ShadowData* (RenderStyle::*getter)() const, void (RenderStyle::*setter)(PassOwnPtr<ShadowData>, bool))
        : AnimationPropertyWrapperBase(prop)
        , m_getter(getter)
        , m_setter(setter)
    {
    }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        const ShadowData* shadowA = (a->*m_getter)();
        const ShadowData* shadowB = (b->*m_getter)();

        while (true) {
            // end of both lists
            if (!shadowA && !shadowB)
                return true;

            // end of just one of the lists
            if (!shadowA || !shadowB)
                return false;

            if (*shadowA != *shadowB)
                return false;

            shadowA = shadowA->next();
            shadowB = shadowB->next();
        }

        return true;
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        const ShadowData* shadowA = (a->*m_getter)();
        const ShadowData* shadowB = (b->*m_getter)();

        int fromLength = shadowListLength(shadowA);
        int toLength = shadowListLength(shadowB);

        if (fromLength == toLength || (fromLength <= 1 && toLength <= 1)) {
            (dst->*m_setter)(blendSimpleOrMatchedShadowLists(anim, progress, shadowA, shadowB), false);
            return;
        }

        (dst->*m_setter)(blendMismatchedShadowLists(anim, progress, shadowA, shadowB, fromLength, toLength), false);
    }

private:
    PassOwnPtr<ShadowData*> blendSimpleOrMatchedShadowLists(const AnimationBase* anim, double progress, const ShadowData* shadowA, const ShadowData* shadowB) const
    {
        OwnPtr<ShadowData> newShadowData;
        ShadowData* lastShadow = 0;

        while (shadowA || shadowB) {
            const ShadowData* srcShadow = shadowForBlending(shadowA, shadowB);
            const ShadowData* dstShadow = shadowForBlending(shadowB, shadowA);

            OwnPtr<ShadowData> blendedShadow = blendFunc(anim, srcShadow, dstShadow, progress);
            ShadowData* blendedShadowPtr = blendedShadow.get();

            if (!lastShadow)
                newShadowData = blendedShadow.release();
            else
                lastShadow->setNext(blendedShadow.release());

            lastShadow = blendedShadowPtr;

            shadowA = shadowA ? shadowA->next() : 0;
            shadowB = shadowB ? shadowB->next() : 0;
        }

        return newShadowData.release();
    }

    PassOwnPtr<ShadowData*> blendMismatchedShadowLists(const AnimationBase* anim, double progress, const ShadowData* shadowA, const ShadowData* shadowB, int fromLength, int toLength) const
    {
        // The shadows in ShadowData are stored in reverse order, so when animating mismatched lists,
        // reverse them and match from the end.
        Vector<const ShadowData*, 4> fromShadows(fromLength);
        for (int i = fromLength - 1; i >= 0; --i) {
            fromShadows[i] = shadowA;
            shadowA = shadowA->next();
        }

        Vector<const ShadowData*, 4> toShadows(toLength);
        for (int i = toLength - 1; i >= 0; --i) {
            toShadows[i] = shadowB;
            shadowB = shadowB->next();
        }

        OwnPtr<ShadowData> newShadowData;

        int maxLength = max(fromLength, toLength);
        for (int i = 0; i < maxLength; ++i) {
            const ShadowData* fromShadow = i < fromLength ? fromShadows[i] : 0;
            const ShadowData* toShadow = i < toLength ? toShadows[i] : 0;

            const ShadowData* srcShadow = shadowForBlending(fromShadow, toShadow);
            const ShadowData* dstShadow = shadowForBlending(toShadow, fromShadow);

            OwnPtr<ShadowData> blendedShadow = blendFunc(anim, srcShadow, dstShadow, progress);
            // Insert at the start of the list to preserve the order.
            blendedShadow->setNext(newShadowData.release());
            newShadowData = blendedShadow.release();
        }

        return newShadowData.release();
    }

    const ShadowData* (RenderStyle::*m_getter)() const;
    void (RenderStyle::*m_setter)(PassOwnPtr<ShadowData>, bool);
};

class PropertyWrapperMaybeInvalidColor : public AnimationPropertyWrapperBase {
public:
    PropertyWrapperMaybeInvalidColor(CSSPropertyID prop, Color (RenderStyle::*getter)() const, void (RenderStyle::*setter)(const Color&))
        : AnimationPropertyWrapperBase(prop)
        , m_getter(getter)
        , m_setter(setter)
    {
    }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        Color fromColor = (a->*m_getter)();
        Color toColor = (b->*m_getter)();

        if (!fromColor.isValid() && !toColor.isValid())
            return true;

        if (!fromColor.isValid())
            fromColor = a->color();
        if (!toColor.isValid())
            toColor = b->color();

        return fromColor == toColor;
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        Color fromColor = (a->*m_getter)();
        Color toColor = (b->*m_getter)();

        if (!fromColor.isValid() && !toColor.isValid())
            return;

        if (!fromColor.isValid())
            fromColor = a->color();
        if (!toColor.isValid())
            toColor = b->color();
        (dst->*m_setter)(blendFunc(anim, fromColor, toColor, progress));
    }

private:
    Color (RenderStyle::*m_getter)() const;
    void (RenderStyle::*m_setter)(const Color&);
};


enum MaybeInvalidColorTag { MaybeInvalidColor };
class PropertyWrapperVisitedAffectedColor : public AnimationPropertyWrapperBase {
public:
    PropertyWrapperVisitedAffectedColor(CSSPropertyID prop, Color (RenderStyle::*getter)() const, void (RenderStyle::*setter)(const Color&),
                                        Color (RenderStyle::*visitedGetter)() const, void (RenderStyle::*visitedSetter)(const Color&))
        : AnimationPropertyWrapperBase(prop)
        , m_wrapper(adoptPtr(new PropertyWrapperColor(prop, getter, setter)))
        , m_visitedWrapper(adoptPtr(new PropertyWrapperColor(prop, visitedGetter, visitedSetter)))
    {
    }
    PropertyWrapperVisitedAffectedColor(CSSPropertyID prop, MaybeInvalidColorTag, Color (RenderStyle::*getter)() const, void (RenderStyle::*setter)(const Color&),
                                        Color (RenderStyle::*visitedGetter)() const, void (RenderStyle::*visitedSetter)(const Color&))
        : AnimationPropertyWrapperBase(prop)
        , m_wrapper(adoptPtr(new PropertyWrapperMaybeInvalidColor(prop, getter, setter)))
        , m_visitedWrapper(adoptPtr(new PropertyWrapperMaybeInvalidColor(prop, visitedGetter, visitedSetter)))
    {
    }
    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        return m_wrapper->equals(a, b) && m_visitedWrapper->equals(a, b);
    }
    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        m_wrapper->blend(anim, dst, a, b, progress);
        m_visitedWrapper->blend(anim, dst, a, b, progress);
    }

private:
    OwnPtr<AnimationPropertyWrapperBase> m_wrapper;
    OwnPtr<AnimationPropertyWrapperBase> m_visitedWrapper;
};

// Wrapper base class for an animatable property in a FillLayer
class FillLayerAnimationPropertyWrapperBase {
public:
    FillLayerAnimationPropertyWrapperBase()
    {
    }

    virtual ~FillLayerAnimationPropertyWrapperBase() { }

    virtual bool equals(const FillLayer*, const FillLayer*) const = 0;
    virtual void blend(const AnimationBase*, FillLayer*, const FillLayer*, const FillLayer*, double) const = 0;
};

template <typename T>
class FillLayerPropertyWrapperGetter : public FillLayerAnimationPropertyWrapperBase {
    WTF_MAKE_NONCOPYABLE(FillLayerPropertyWrapperGetter);
public:
    FillLayerPropertyWrapperGetter(T (FillLayer::*getter)() const)
        : m_getter(getter)
    {
    }

    virtual bool equals(const FillLayer* a, const FillLayer* b) const
    {
       // If the style pointers are the same, don't bother doing the test.
       // If either is null, return false. If both are null, return true.
       if ((!a && !b) || a == b)
           return true;
       if (!a || !b)
            return false;
        return (a->*m_getter)() == (b->*m_getter)();
    }

protected:
    T (FillLayer::*m_getter)() const;
};

template <typename T>
class FillLayerPropertyWrapper : public FillLayerPropertyWrapperGetter<T> {
public:
    FillLayerPropertyWrapper(T (FillLayer::*getter)() const, void (FillLayer::*setter)(T))
        : FillLayerPropertyWrapperGetter<T>(getter)
        , m_setter(setter)
    {
    }

    virtual void blend(const AnimationBase* anim, FillLayer* dst, const FillLayer* a, const FillLayer* b, double progress) const
    {
        (dst->*m_setter)(blendFunc(anim, (a->*FillLayerPropertyWrapperGetter<T>::m_getter)(), (b->*FillLayerPropertyWrapperGetter<T>::m_getter)(), progress));
    }

protected:
    void (FillLayer::*m_setter)(T);
};

template <typename T>
class FillLayerRefCountedPropertyWrapper : public FillLayerPropertyWrapperGetter<T*> {
public:
    FillLayerRefCountedPropertyWrapper(T* (FillLayer::*getter)() const, void (FillLayer::*setter)(PassRefPtr<T>))
        : FillLayerPropertyWrapperGetter<T*>(getter)
        , m_setter(setter)
    {
    }

    virtual void blend(const AnimationBase* anim, FillLayer* dst, const FillLayer* a, const FillLayer* b, double progress) const
    {
        (dst->*m_setter)(blendFunc(anim, (a->*FillLayerPropertyWrapperGetter<T*>::m_getter)(), (b->*FillLayerPropertyWrapperGetter<T*>::m_getter)(), progress));
    }

protected:
    void (FillLayer::*m_setter)(PassRefPtr<T>);
};

class FillLayerStyleImagePropertyWrapper : public FillLayerRefCountedPropertyWrapper<StyleImage> {
public:
    FillLayerStyleImagePropertyWrapper(StyleImage* (FillLayer::*getter)() const, void (FillLayer::*setter)(PassRefPtr<StyleImage>))
        : FillLayerRefCountedPropertyWrapper<StyleImage>(getter, setter)
    {
    }

    virtual bool equals(const FillLayer* a, const FillLayer* b) const
    {
       // If the style pointers are the same, don't bother doing the test.
       // If either is null, return false. If both are null, return true.
       if (a == b)
           return true;
       if (!a || !b)
            return false;

        StyleImage* imageA = (a->*m_getter)();
        StyleImage* imageB = (b->*m_getter)();
        return StyleImage::imagesEquivalent(imageA, imageB);
    }
};


class FillLayersPropertyWrapper : public AnimationPropertyWrapperBase {
public:
    typedef const FillLayer* (RenderStyle::*LayersGetter)() const;
    typedef FillLayer* (RenderStyle::*LayersAccessor)();

    FillLayersPropertyWrapper(CSSPropertyID prop, LayersGetter getter, LayersAccessor accessor)
        : AnimationPropertyWrapperBase(prop)
        , m_layersGetter(getter)
        , m_layersAccessor(accessor)
    {
        switch (prop) {
        case CSSPropertyBackgroundPositionX:
        case CSSPropertyWebkitMaskPositionX:
            m_fillLayerPropertyWrapper = new FillLayerPropertyWrapper<Length>(&FillLayer::xPosition, &FillLayer::setXPosition);
            break;
        case CSSPropertyBackgroundPositionY:
        case CSSPropertyWebkitMaskPositionY:
            m_fillLayerPropertyWrapper = new FillLayerPropertyWrapper<Length>(&FillLayer::yPosition, &FillLayer::setYPosition);
            break;
        case CSSPropertyBackgroundSize:
        case CSSPropertyWebkitBackgroundSize:
        case CSSPropertyWebkitMaskSize:
            m_fillLayerPropertyWrapper = new FillLayerPropertyWrapper<LengthSize>(&FillLayer::sizeLength, &FillLayer::setSizeLength);
            break;
        case CSSPropertyBackgroundImage:
            m_fillLayerPropertyWrapper = new FillLayerStyleImagePropertyWrapper(&FillLayer::image, &FillLayer::setImage);
            break;
        default:
            break;
        }
    }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        const FillLayer* fromLayer = (a->*m_layersGetter)();
        const FillLayer* toLayer = (b->*m_layersGetter)();

        while (fromLayer && toLayer) {
            if (!m_fillLayerPropertyWrapper->equals(fromLayer, toLayer))
                return false;

            fromLayer = fromLayer->next();
            toLayer = toLayer->next();
        }

        return true;
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        const FillLayer* aLayer = (a->*m_layersGetter)();
        const FillLayer* bLayer = (b->*m_layersGetter)();
        FillLayer* dstLayer = (dst->*m_layersAccessor)();

        while (aLayer && bLayer && dstLayer) {
            m_fillLayerPropertyWrapper->blend(anim, dstLayer, aLayer, bLayer, progress);
            aLayer = aLayer->next();
            bLayer = bLayer->next();
            dstLayer = dstLayer->next();
        }
    }

private:
    FillLayerAnimationPropertyWrapperBase* m_fillLayerPropertyWrapper;

    LayersGetter m_layersGetter;
    LayersAccessor m_layersAccessor;
};

class ShorthandPropertyWrapper : public AnimationPropertyWrapperBase {
public:
    ShorthandPropertyWrapper(CSSPropertyID property, const StylePropertyShorthand& shorthand)
        : AnimationPropertyWrapperBase(property)
    {
        for (unsigned i = 0; i < shorthand.length(); ++i) {
            AnimationPropertyWrapperBase* wrapper = wrapperForProperty(shorthand.properties()[i]);
            if (wrapper)
                m_propertyWrappers.append(wrapper);
        }
    }

    virtual bool isShorthandWrapper() const { return true; }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        Vector<AnimationPropertyWrapperBase*>::const_iterator end = m_propertyWrappers.end();
        for (Vector<AnimationPropertyWrapperBase*>::const_iterator it = m_propertyWrappers.begin(); it != end; ++it) {
            if (!(*it)->equals(a, b))
                return false;
        }
        return true;
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        Vector<AnimationPropertyWrapperBase*>::const_iterator end = m_propertyWrappers.end();
        for (Vector<AnimationPropertyWrapperBase*>::const_iterator it = m_propertyWrappers.begin(); it != end; ++it)
            (*it)->blend(anim, dst, a, b, progress);
    }

    const Vector<AnimationPropertyWrapperBase*> propertyWrappers() const { return m_propertyWrappers; }

private:
    Vector<AnimationPropertyWrapperBase*> m_propertyWrappers;
};

class PropertyWrapperFlex : public AnimationPropertyWrapperBase {
public:
    PropertyWrapperFlex() : AnimationPropertyWrapperBase(CSSPropertyWebkitFlex)
    {
    }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        // If the style pointers are the same, don't bother doing the test.
        // If either is null, return false. If both are null, return true.
        if ((!a && !b) || a == b)
            return true;
        if (!a || !b)
            return false;

        return a->flexBasis() == b->flexBasis() && a->flexGrow() == b->flexGrow() && a->flexShrink() == b->flexShrink();
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        dst->setFlexBasis(blendFunc(anim, a->flexBasis(), b->flexBasis(), progress));
        dst->setFlexGrow(blendFunc(anim, a->flexGrow(), b->flexGrow(), progress));
        dst->setFlexShrink(blendFunc(anim, a->flexShrink(), b->flexShrink(), progress));
    }
};

#if ENABLE(SVG)
class PropertyWrapperSVGPaint : public AnimationPropertyWrapperBase {
public:
    PropertyWrapperSVGPaint(CSSPropertyID prop, const SVGPaint::SVGPaintType& (RenderStyle::*paintTypeGetter)() const, Color (RenderStyle::*getter)() const, void (RenderStyle::*setter)(const Color&))
        : AnimationPropertyWrapperBase(prop)
        , m_paintTypeGetter(paintTypeGetter)
        , m_getter(getter)
        , m_setter(setter)
    {
    }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        if ((a->*m_paintTypeGetter)() != (b->*m_paintTypeGetter)())
            return false;

        // We only support animations between SVGPaints that are pure Color values.
        // For everything else we must return true for this method, otherwise
        // we will try to animate between values forever.
        if ((a->*m_paintTypeGetter)() == SVGPaint::SVG_PAINTTYPE_RGBCOLOR) {
            Color fromColor = (a->*m_getter)();
            Color toColor = (b->*m_getter)();

            if (!fromColor.isValid() && !toColor.isValid())
                return true;

            if (!fromColor.isValid())
                fromColor = Color();
            if (!toColor.isValid())
                toColor = Color();

            return fromColor == toColor;
        }
        return true;
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        if ((a->*m_paintTypeGetter)() != SVGPaint::SVG_PAINTTYPE_RGBCOLOR
            || (b->*m_paintTypeGetter)() != SVGPaint::SVG_PAINTTYPE_RGBCOLOR)
            return;

        Color fromColor = (a->*m_getter)();
        Color toColor = (b->*m_getter)();

        if (!fromColor.isValid() && !toColor.isValid())
            return;

        if (!fromColor.isValid())
            fromColor = Color();
        if (!toColor.isValid())
            toColor = Color();
        (dst->*m_setter)(blendFunc(anim, fromColor, toColor, progress));
    }

private:
    const SVGPaint::SVGPaintType& (RenderStyle::*m_paintTypeGetter)() const;
    Color (RenderStyle::*m_getter)() const;
    void (RenderStyle::*m_setter)(const Color&);
};
#endif

static void addShorthandProperties()
{
    static const CSSPropertyID animatableShorthandProperties[] = {
        CSSPropertyBackground, // for background-color, background-position, background-image
        CSSPropertyBackgroundPosition,
        CSSPropertyFont, // for font-size, font-weight
        CSSPropertyWebkitMask, // for mask-position
        CSSPropertyWebkitMaskPosition,
        CSSPropertyBorderTop, CSSPropertyBorderRight, CSSPropertyBorderBottom, CSSPropertyBorderLeft,
        CSSPropertyBorderColor,
        CSSPropertyBorderRadius,
        CSSPropertyBorderWidth,
        CSSPropertyBorder,
        CSSPropertyBorderImage,
        CSSPropertyBorderSpacing,
        CSSPropertyListStyle, // for list-style-image
        CSSPropertyMargin,
        CSSPropertyOutline,
        CSSPropertyPadding,
        CSSPropertyWebkitTextStroke,
        CSSPropertyWebkitColumnRule,
        CSSPropertyWebkitBorderRadius,
        CSSPropertyWebkitTransformOrigin
    };

    for (size_t i = 0; i < WTF_ARRAY_LENGTH(animatableShorthandProperties); ++i) {
        CSSPropertyID propertyID = animatableShorthandProperties[i];
        StylePropertyShorthand shorthand = shorthandForProperty(propertyID);
        if (shorthand.length() > 0)
            addPropertyWrapper(propertyID, new ShorthandPropertyWrapper(propertyID, shorthand));
    }
}

void CSSPropertyAnimation::ensurePropertyMap()
{
    // FIXME: This data is never destroyed. Maybe we should ref count it and toss it when the last AnimationController is destroyed?
    if (gPropertyWrappers)
        return;

    gPropertyWrappers = new Vector<AnimationPropertyWrapperBase*>();

    // build the list of property wrappers to do the comparisons and blends
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyLeft, &RenderStyle::left, &RenderStyle::setLeft));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyRight, &RenderStyle::right, &RenderStyle::setRight));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyTop, &RenderStyle::top, &RenderStyle::setTop));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyBottom, &RenderStyle::bottom, &RenderStyle::setBottom));

    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyWidth, &RenderStyle::width, &RenderStyle::setWidth));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMinWidth, &RenderStyle::minWidth, &RenderStyle::setMinWidth));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMaxWidth, &RenderStyle::maxWidth, &RenderStyle::setMaxWidth));

    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyHeight, &RenderStyle::height, &RenderStyle::setHeight));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMinHeight, &RenderStyle::minHeight, &RenderStyle::setMinHeight));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMaxHeight, &RenderStyle::maxHeight, &RenderStyle::setMaxHeight));

    gPropertyWrappers->append(new PropertyWrapperFlex());

    gPropertyWrappers->append(new PropertyWrapper<unsigned>(CSSPropertyBorderLeftWidth, &RenderStyle::borderLeftWidth, &RenderStyle::setBorderLeftWidth));
    gPropertyWrappers->append(new PropertyWrapper<unsigned>(CSSPropertyBorderRightWidth, &RenderStyle::borderRightWidth, &RenderStyle::setBorderRightWidth));
    gPropertyWrappers->append(new PropertyWrapper<unsigned>(CSSPropertyBorderTopWidth, &RenderStyle::borderTopWidth, &RenderStyle::setBorderTopWidth));
    gPropertyWrappers->append(new PropertyWrapper<unsigned>(CSSPropertyBorderBottomWidth, &RenderStyle::borderBottomWidth, &RenderStyle::setBorderBottomWidth));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMarginLeft, &RenderStyle::marginLeft, &RenderStyle::setMarginLeft));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMarginRight, &RenderStyle::marginRight, &RenderStyle::setMarginRight));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMarginTop, &RenderStyle::marginTop, &RenderStyle::setMarginTop));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMarginBottom, &RenderStyle::marginBottom, &RenderStyle::setMarginBottom));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyPaddingLeft, &RenderStyle::paddingLeft, &RenderStyle::setPaddingLeft));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyPaddingRight, &RenderStyle::paddingRight, &RenderStyle::setPaddingRight));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyPaddingTop, &RenderStyle::paddingTop, &RenderStyle::setPaddingTop));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyPaddingBottom, &RenderStyle::paddingBottom, &RenderStyle::setPaddingBottom));
    gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyColor, &RenderStyle::color, &RenderStyle::setColor, &RenderStyle::visitedLinkColor, &RenderStyle::setVisitedLinkColor));

    gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyBackgroundColor, &RenderStyle::backgroundColor, &RenderStyle::setBackgroundColor, &RenderStyle::visitedLinkBackgroundColor, &RenderStyle::setVisitedLinkBackgroundColor));

    gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyBackgroundImage, &RenderStyle::backgroundLayers, &RenderStyle::accessBackgroundLayers));
    gPropertyWrappers->append(new StyleImagePropertyWrapper(CSSPropertyListStyleImage, &RenderStyle::listStyleImage, &RenderStyle::setListStyleImage));
    gPropertyWrappers->append(new StyleImagePropertyWrapper(CSSPropertyWebkitMaskImage, &RenderStyle::maskImage, &RenderStyle::setMaskImage));

    gPropertyWrappers->append(new StyleImagePropertyWrapper(CSSPropertyBorderImageSource, &RenderStyle::borderImageSource, &RenderStyle::setBorderImageSource));
    gPropertyWrappers->append(new PropertyWrapper<LengthBox>(CSSPropertyBorderImageSlice, &RenderStyle::borderImageSlices, &RenderStyle::setBorderImageSlices));
    gPropertyWrappers->append(new PropertyWrapper<LengthBox>(CSSPropertyBorderImageWidth, &RenderStyle::borderImageWidth, &RenderStyle::setBorderImageWidth));
    gPropertyWrappers->append(new PropertyWrapper<LengthBox>(CSSPropertyBorderImageOutset, &RenderStyle::borderImageOutset, &RenderStyle::setBorderImageOutset));

    gPropertyWrappers->append(new StyleImagePropertyWrapper(CSSPropertyWebkitMaskBoxImageSource, &RenderStyle::maskBoxImageSource, &RenderStyle::setMaskBoxImageSource));
    gPropertyWrappers->append(new PropertyWrapper<const NinePieceImage&>(CSSPropertyWebkitMaskBoxImage, &RenderStyle::maskBoxImage, &RenderStyle::setMaskBoxImage));

    gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyBackgroundPositionX, &RenderStyle::backgroundLayers, &RenderStyle::accessBackgroundLayers));
    gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyBackgroundPositionY, &RenderStyle::backgroundLayers, &RenderStyle::accessBackgroundLayers));
    gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyBackgroundSize, &RenderStyle::backgroundLayers, &RenderStyle::accessBackgroundLayers));
    gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyWebkitBackgroundSize, &RenderStyle::backgroundLayers, &RenderStyle::accessBackgroundLayers));

    gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyWebkitMaskPositionX, &RenderStyle::maskLayers, &RenderStyle::accessMaskLayers));
    gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyWebkitMaskPositionY, &RenderStyle::maskLayers, &RenderStyle::accessMaskLayers));
    gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyWebkitMaskSize, &RenderStyle::maskLayers, &RenderStyle::accessMaskLayers));

    gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyFontSize,
        // Must pass a specified size to setFontSize if Text Autosizing is enabled, but a computed size
        // if text zoom is enabled (if neither is enabled it's irrelevant as they're probably the same).
        // FIXME: Find some way to assert that text zoom isn't activated when Text Autosizing is compiled in.
#if ENABLE(TEXT_AUTOSIZING)
        &RenderStyle::specifiedFontSize,
#else
        &RenderStyle::computedFontSize,
#endif
        &RenderStyle::setFontSize));
    gPropertyWrappers->append(new PropertyWrapper<unsigned short>(CSSPropertyWebkitColumnRuleWidth, &RenderStyle::columnRuleWidth, &RenderStyle::setColumnRuleWidth));
    gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyWebkitColumnGap, &RenderStyle::columnGap, &RenderStyle::setColumnGap));
    gPropertyWrappers->append(new PropertyWrapper<unsigned short>(CSSPropertyWebkitColumnCount, &RenderStyle::columnCount, &RenderStyle::setColumnCount));
    gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyWebkitColumnWidth, &RenderStyle::columnWidth, &RenderStyle::setColumnWidth));
    gPropertyWrappers->append(new PropertyWrapper<short>(CSSPropertyWebkitBorderHorizontalSpacing, &RenderStyle::horizontalBorderSpacing, &RenderStyle::setHorizontalBorderSpacing));
    gPropertyWrappers->append(new PropertyWrapper<short>(CSSPropertyWebkitBorderVerticalSpacing, &RenderStyle::verticalBorderSpacing, &RenderStyle::setVerticalBorderSpacing));
    gPropertyWrappers->append(new PropertyWrapper<int>(CSSPropertyZIndex, &RenderStyle::zIndex, &RenderStyle::setZIndex));
    gPropertyWrappers->append(new PropertyWrapper<short>(CSSPropertyOrphans, &RenderStyle::orphans, &RenderStyle::setOrphans));
    gPropertyWrappers->append(new PropertyWrapper<short>(CSSPropertyWidows, &RenderStyle::widows, &RenderStyle::setWidows));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyLineHeight, &RenderStyle::specifiedLineHeight, &RenderStyle::setLineHeight));
    gPropertyWrappers->append(new PropertyWrapper<int>(CSSPropertyOutlineOffset, &RenderStyle::outlineOffset, &RenderStyle::setOutlineOffset));
    gPropertyWrappers->append(new PropertyWrapper<unsigned short>(CSSPropertyOutlineWidth, &RenderStyle::outlineWidth, &RenderStyle::setOutlineWidth));
    gPropertyWrappers->append(new PropertyWrapper<int>(CSSPropertyLetterSpacing, &RenderStyle::letterSpacing, &RenderStyle::setLetterSpacing));
    gPropertyWrappers->append(new PropertyWrapper<int>(CSSPropertyWordSpacing, &RenderStyle::wordSpacing, &RenderStyle::setWordSpacing));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyTextIndent, &RenderStyle::textIndent, &RenderStyle::setTextIndent));

    gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyWebkitPerspective, &RenderStyle::perspective, &RenderStyle::setPerspective));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyWebkitPerspectiveOriginX, &RenderStyle::perspectiveOriginX, &RenderStyle::setPerspectiveOriginX));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyWebkitPerspectiveOriginY, &RenderStyle::perspectiveOriginY, &RenderStyle::setPerspectiveOriginY));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyWebkitTransformOriginX, &RenderStyle::transformOriginX, &RenderStyle::setTransformOriginX));
    gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyWebkitTransformOriginY, &RenderStyle::transformOriginY, &RenderStyle::setTransformOriginY));
    gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyWebkitTransformOriginZ, &RenderStyle::transformOriginZ, &RenderStyle::setTransformOriginZ));
    gPropertyWrappers->append(new PropertyWrapper<LengthSize>(CSSPropertyBorderTopLeftRadius, &RenderStyle::borderTopLeftRadius, &RenderStyle::setBorderTopLeftRadius));
    gPropertyWrappers->append(new PropertyWrapper<LengthSize>(CSSPropertyBorderTopRightRadius, &RenderStyle::borderTopRightRadius, &RenderStyle::setBorderTopRightRadius));
    gPropertyWrappers->append(new PropertyWrapper<LengthSize>(CSSPropertyBorderBottomLeftRadius, &RenderStyle::borderBottomLeftRadius, &RenderStyle::setBorderBottomLeftRadius));
    gPropertyWrappers->append(new PropertyWrapper<LengthSize>(CSSPropertyBorderBottomRightRadius, &RenderStyle::borderBottomRightRadius, &RenderStyle::setBorderBottomRightRadius));
    gPropertyWrappers->append(new PropertyWrapper<EVisibility>(CSSPropertyVisibility, &RenderStyle::visibility, &RenderStyle::setVisibility));
    gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyZoom, &RenderStyle::zoom, &RenderStyle::setZoomWithoutReturnValue));

    gPropertyWrappers->append(new PropertyWrapper<LengthBox>(CSSPropertyClip, &RenderStyle::clip, &RenderStyle::setClip));

#if USE(ACCELERATED_COMPOSITING)
    gPropertyWrappers->append(new PropertyWrapperAcceleratedOpacity());
    gPropertyWrappers->append(new PropertyWrapperAcceleratedTransform());
#if ENABLE(CSS_FILTERS)
    gPropertyWrappers->append(new PropertyWrapperAcceleratedFilter());
#endif
#else
    gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyOpacity, &RenderStyle::opacity, &RenderStyle::setOpacity));
    gPropertyWrappers->append(new PropertyWrapper<const TransformOperations&>(CSSPropertyWebkitTransform, &RenderStyle::transform, &RenderStyle::setTransform));
#if ENABLE(CSS_FILTERS)
    gPropertyWrappers->append(new PropertyWrapper<const FilterOperations&>(CSSPropertyWebkitFilter, &RenderStyle::filter, &RenderStyle::setFilter));
#endif
#endif

    gPropertyWrappers->append(new PropertyWrapperClipPath(CSSPropertyWebkitClipPath, &RenderStyle::clipPath, &RenderStyle::setClipPath));

#if ENABLE(CSS_SHAPES)
    gPropertyWrappers->append(new PropertyWrapperShape(CSSPropertyWebkitShapeInside, &RenderStyle::shapeInside, &RenderStyle::setShapeInside));
#endif

    gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyWebkitColumnRuleColor, MaybeInvalidColor, &RenderStyle::columnRuleColor, &RenderStyle::setColumnRuleColor, &RenderStyle::visitedLinkColumnRuleColor, &RenderStyle::setVisitedLinkColumnRuleColor));
    gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyWebkitTextStrokeColor, MaybeInvalidColor, &RenderStyle::textStrokeColor, &RenderStyle::setTextStrokeColor, &RenderStyle::visitedLinkTextStrokeColor, &RenderStyle::setVisitedLinkTextStrokeColor));
    gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyWebkitTextFillColor, MaybeInvalidColor, &RenderStyle::textFillColor, &RenderStyle::setTextFillColor, &RenderStyle::visitedLinkTextFillColor, &RenderStyle::setVisitedLinkTextFillColor));
    gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyBorderLeftColor, MaybeInvalidColor, &RenderStyle::borderLeftColor, &RenderStyle::setBorderLeftColor, &RenderStyle::visitedLinkBorderLeftColor, &RenderStyle::setVisitedLinkBorderLeftColor));
    gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyBorderRightColor, MaybeInvalidColor, &RenderStyle::borderRightColor, &RenderStyle::setBorderRightColor, &RenderStyle::visitedLinkBorderRightColor, &RenderStyle::setVisitedLinkBorderRightColor));
    gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyBorderTopColor, MaybeInvalidColor, &RenderStyle::borderTopColor, &RenderStyle::setBorderTopColor, &RenderStyle::visitedLinkBorderTopColor, &RenderStyle::setVisitedLinkBorderTopColor));
    gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyBorderBottomColor, MaybeInvalidColor, &RenderStyle::borderBottomColor, &RenderStyle::setBorderBottomColor, &RenderStyle::visitedLinkBorderBottomColor, &RenderStyle::setVisitedLinkBorderBottomColor));
    gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyOutlineColor, MaybeInvalidColor, &RenderStyle::outlineColor, &RenderStyle::setOutlineColor, &RenderStyle::visitedLinkOutlineColor, &RenderStyle::setVisitedLinkOutlineColor));

    gPropertyWrappers->append(new PropertyWrapperShadow(CSSPropertyBoxShadow, &RenderStyle::boxShadow, &RenderStyle::setBoxShadow));
    gPropertyWrappers->append(new PropertyWrapperShadow(CSSPropertyWebkitBoxShadow, &RenderStyle::boxShadow, &RenderStyle::setBoxShadow));
    gPropertyWrappers->append(new PropertyWrapperShadow(CSSPropertyTextShadow, &RenderStyle::textShadow, &RenderStyle::setTextShadow));

#if ENABLE(SVG)
    gPropertyWrappers->append(new PropertyWrapperSVGPaint(CSSPropertyFill, &RenderStyle::fillPaintType, &RenderStyle::fillPaintColor, &RenderStyle::setFillPaintColor));
    gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyFillOpacity, &RenderStyle::fillOpacity, &RenderStyle::setFillOpacity));

    gPropertyWrappers->append(new PropertyWrapperSVGPaint(CSSPropertyStroke, &RenderStyle::strokePaintType, &RenderStyle::strokePaintColor, &RenderStyle::setStrokePaintColor));
    gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyStrokeOpacity, &RenderStyle::strokeOpacity, &RenderStyle::setStrokeOpacity));
    gPropertyWrappers->append(new PropertyWrapper<SVGLength>(CSSPropertyStrokeWidth, &RenderStyle::strokeWidth, &RenderStyle::setStrokeWidth));
    gPropertyWrappers->append(new PropertyWrapper<SVGLength>(CSSPropertyStrokeDashoffset, &RenderStyle::strokeDashOffset, &RenderStyle::setStrokeDashOffset));
    gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyStrokeMiterlimit, &RenderStyle::strokeMiterLimit, &RenderStyle::setStrokeMiterLimit));

    gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyFloodOpacity, &RenderStyle::floodOpacity, &RenderStyle::setFloodOpacity));
    gPropertyWrappers->append(new PropertyWrapperMaybeInvalidColor(CSSPropertyFloodColor, &RenderStyle::floodColor, &RenderStyle::setFloodColor));

    gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyStopOpacity, &RenderStyle::stopOpacity, &RenderStyle::setStopOpacity));
    gPropertyWrappers->append(new PropertyWrapperMaybeInvalidColor(CSSPropertyStopColor, &RenderStyle::stopColor, &RenderStyle::setStopColor));

    gPropertyWrappers->append(new PropertyWrapperMaybeInvalidColor(CSSPropertyLightingColor, &RenderStyle::lightingColor, &RenderStyle::setLightingColor));

    gPropertyWrappers->append(new PropertyWrapper<SVGLength>(CSSPropertyBaselineShift, &RenderStyle::baselineShiftValue, &RenderStyle::setBaselineShiftValue));
    gPropertyWrappers->append(new PropertyWrapper<SVGLength>(CSSPropertyKerning, &RenderStyle::kerning, &RenderStyle::setKerning));
#endif

    // TODO:
    //
    //  CSSPropertyVerticalAlign
    //
    // Compound properties that have components that should be animatable:
    //
    //  CSSPropertyWebkitColumns
    //  CSSPropertyWebkitBoxReflect

    // Make sure unused slots have a value
    for (unsigned int i = 0; i < static_cast<unsigned int>(numCSSProperties); ++i)
        gPropertyWrapperMap[i] = cInvalidPropertyWrapperIndex;

    // First we put the non-shorthand property wrappers into the map, so the shorthand-building
    // code can find them.
    size_t n = gPropertyWrappers->size();
    for (unsigned int i = 0; i < n; ++i) {
        ASSERT((*gPropertyWrappers)[i]->property() - firstCSSProperty < numCSSProperties);
        gPropertyWrapperMap[(*gPropertyWrappers)[i]->property() - firstCSSProperty] = i;
    }

    // Now add the shorthand wrappers.
    addShorthandProperties();
}

static bool gatherEnclosingShorthandProperties(CSSPropertyID property, AnimationPropertyWrapperBase* wrapper, HashSet<CSSPropertyID>& propertySet)
{
    if (!wrapper->isShorthandWrapper())
        return false;

    ShorthandPropertyWrapper* shorthandWrapper = static_cast<ShorthandPropertyWrapper*>(wrapper);

    bool contained = false;
    for (size_t i = 0; i < shorthandWrapper->propertyWrappers().size(); ++i) {
        AnimationPropertyWrapperBase* currWrapper = shorthandWrapper->propertyWrappers()[i];

        if (gatherEnclosingShorthandProperties(property, currWrapper, propertySet) || currWrapper->property() == property)
            contained = true;
    }

    if (contained)
        propertySet.add(wrapper->property());

    return contained;
}

// Returns true if we need to start animation timers
bool CSSPropertyAnimation::blendProperties(const AnimationBase* anim, CSSPropertyID prop, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress)
{
    ASSERT(prop != CSSPropertyInvalid);

    ensurePropertyMap();

    AnimationPropertyWrapperBase* wrapper = wrapperForProperty(prop);
    if (wrapper) {
        wrapper->blend(anim, dst, a, b, progress);
#if USE(ACCELERATED_COMPOSITING)
        return !wrapper->animationIsAccelerated() || !anim->isAccelerated();
#else
        return true;
#endif
    }

    return false;
}

#if USE(ACCELERATED_COMPOSITING)
bool CSSPropertyAnimation::animationOfPropertyIsAccelerated(CSSPropertyID prop)
{
    ensurePropertyMap();
    AnimationPropertyWrapperBase* wrapper = wrapperForProperty(prop);
    return wrapper ? wrapper->animationIsAccelerated() : false;
}
#endif

// Note: this is inefficient. It's only called from pauseTransitionAtTime().
HashSet<CSSPropertyID> CSSPropertyAnimation::animatableShorthandsAffectingProperty(CSSPropertyID property)
{
    ensurePropertyMap();

    HashSet<CSSPropertyID> foundProperties;
    for (int i = 0; i < getNumProperties(); ++i)
        gatherEnclosingShorthandProperties(property, (*gPropertyWrappers)[i], foundProperties);

    return foundProperties;
}

bool CSSPropertyAnimation::propertiesEqual(CSSPropertyID prop, const RenderStyle* a, const RenderStyle* b)
{
    ensurePropertyMap();

    AnimationPropertyWrapperBase* wrapper = wrapperForProperty(prop);
    if (wrapper)
        return wrapper->equals(a, b);
    return true;
}

CSSPropertyID CSSPropertyAnimation::getPropertyAtIndex(int i, bool& isShorthand)
{
    ensurePropertyMap();

    if (i < 0 || i >= getNumProperties())
        return CSSPropertyInvalid;

    AnimationPropertyWrapperBase* wrapper = (*gPropertyWrappers)[i];
    isShorthand = wrapper->isShorthandWrapper();
    return wrapper->property();
}

int CSSPropertyAnimation::getNumProperties()
{
    ensurePropertyMap();

    return gPropertyWrappers->size();
}

}
