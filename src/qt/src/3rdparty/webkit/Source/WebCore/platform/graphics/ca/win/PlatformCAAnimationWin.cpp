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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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

#if USE(ACCELERATED_COMPOSITING)

#include "PlatformCAAnimation.h"

#include "FloatConversion.h"
#include "PlatformString.h"
#include "TimingFunction.h"
#include <QuartzCore/CACFAnimation.h>
#include <QuartzCore/CACFTiming.h>
#include <QuartzCore/CACFTimingFunction.h>
#include <QuartzCore/CACFValueFunction.h>
#include <QuartzCore/CACFVector.h>
#include <wtf/UnusedParam.h>

using namespace WebCore;

static CFStringRef toCACFFillModeType(PlatformCAAnimation::FillModeType type)
{
    switch (type) {
    case PlatformCAAnimation::NoFillMode:
    case PlatformCAAnimation::Forwards: return kCACFFillModeForwards;
    case PlatformCAAnimation::Backwards: return kCACFFillModeBackwards;
    case PlatformCAAnimation::Both: return kCACFFillModeBoth;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

static PlatformCAAnimation::FillModeType fromCACFFillModeType(CFStringRef string)
{
    if (string == kCACFFillModeBackwards)
        return PlatformCAAnimation::Backwards;

    if (string == kCACFFillModeBoth)
        return PlatformCAAnimation::Both;

    return PlatformCAAnimation::Forwards;
}

static CFStringRef toCACFValueFunctionType(PlatformCAAnimation::ValueFunctionType type)
{
    switch (type) {
    case PlatformCAAnimation::NoValueFunction: return 0;
    case PlatformCAAnimation::RotateX: return kCACFValueFunctionRotateX;
    case PlatformCAAnimation::RotateY: return kCACFValueFunctionRotateY;
    case PlatformCAAnimation::RotateZ: return kCACFValueFunctionRotateZ;
    case PlatformCAAnimation::ScaleX: return kCACFValueFunctionScaleX;
    case PlatformCAAnimation::ScaleY: return kCACFValueFunctionScaleY;
    case PlatformCAAnimation::ScaleZ: return kCACFValueFunctionScaleZ;
    case PlatformCAAnimation::Scale: return kCACFValueFunctionScale;
    case PlatformCAAnimation::TranslateX: return kCACFValueFunctionTranslateX;
    case PlatformCAAnimation::TranslateY: return kCACFValueFunctionTranslateY;
    case PlatformCAAnimation::TranslateZ: return kCACFValueFunctionTranslateZ;
    case PlatformCAAnimation::Translate: return kCACFValueFunctionTranslate;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

static PlatformCAAnimation::ValueFunctionType fromCACFValueFunctionType(CFStringRef string)
{
    if (string == kCACFValueFunctionRotateX)
        return PlatformCAAnimation::RotateX;

    if (string == kCACFValueFunctionRotateY)
        return PlatformCAAnimation::RotateY;

    if (string == kCACFValueFunctionRotateZ)
        return PlatformCAAnimation::RotateZ;

    if (string == kCACFValueFunctionScaleX)
        return PlatformCAAnimation::ScaleX;

    if (string == kCACFValueFunctionScaleY)
        return PlatformCAAnimation::ScaleY;

    if (string == kCACFValueFunctionScaleZ)
        return PlatformCAAnimation::ScaleZ;

    if (string == kCACFValueFunctionScale)
        return PlatformCAAnimation::Scale;

    if (string == kCACFValueFunctionTranslateX)
        return PlatformCAAnimation::TranslateX;

    if (string == kCACFValueFunctionTranslateY)
        return PlatformCAAnimation::TranslateY;

    if (string == kCACFValueFunctionTranslateZ)
        return PlatformCAAnimation::TranslateZ;

    if (string == kCACFValueFunctionTranslate)
        return PlatformCAAnimation::Translate;

    return PlatformCAAnimation::NoValueFunction;
}

static RetainPtr<CACFTimingFunctionRef> toCACFTimingFunction(const TimingFunction* timingFunction)
{
    ASSERT(timingFunction);
    if (timingFunction->isCubicBezierTimingFunction()) {
        const CubicBezierTimingFunction* ctf = static_cast<const CubicBezierTimingFunction*>(timingFunction);
        return RetainPtr<CACFTimingFunctionRef>(AdoptCF, CACFTimingFunctionCreate(static_cast<float>(ctf->x1()), static_cast<float>(ctf->y1()), static_cast<float>(ctf->x2()), static_cast<float>(ctf->y2())));
    }
    
    return CACFTimingFunctionGetFunctionWithName(kCACFTimingFunctionLinear);
}

PassRefPtr<PlatformCAAnimation> PlatformCAAnimation::create(AnimationType type, const String& keyPath)
{
    return adoptRef(new PlatformCAAnimation(type, keyPath));
}

PassRefPtr<PlatformCAAnimation> PlatformCAAnimation::create(PlatformAnimationRef animation)
{
    return adoptRef(new PlatformCAAnimation(animation));
}

PlatformCAAnimation::PlatformCAAnimation(AnimationType type, const String& keyPath)
    : m_type(type)
{
    if (type == Basic)
        m_animation.adoptCF(CACFAnimationCreate(kCACFBasicAnimation));
    else
        m_animation.adoptCF(CACFAnimationCreate(kCACFKeyframeAnimation));
    
    RetainPtr<CFStringRef> s(AdoptCF, keyPath.createCFString());
    CACFAnimationSetKeyPath(m_animation.get(), s.get());
}

PlatformCAAnimation::PlatformCAAnimation(PlatformAnimationRef animation)
{
    if (CACFAnimationGetClass(animation) == kCACFBasicAnimation)
        m_type = Basic;
    else if (CACFAnimationGetClass(animation) == kCACFKeyframeAnimation)
        m_type = Keyframe;
    else {
        ASSERT_NOT_REACHED();
        return;
    }
    
    m_animation = animation;
}

PassRefPtr<PlatformCAAnimation> PlatformCAAnimation::copy() const
{
    RefPtr<PlatformCAAnimation> animation = create(animationType(), keyPath());
    
    animation->setBeginTime(beginTime());
    animation->setDuration(duration());
    animation->setSpeed(speed());
    animation->setTimeOffset(timeOffset());
    animation->setRepeatCount(repeatCount());
    animation->setAutoreverses(autoreverses());
    animation->setFillMode(fillMode());
    animation->setRemovedOnCompletion(isRemovedOnCompletion());
    animation->setAdditive(isAdditive());
    animation->copyTimingFunctionFrom(this);
    animation->setValueFunction(valueFunction());
    
    // Copy the specific Basic or Keyframe values
    if (animationType() == Keyframe) {
        animation->copyValuesFrom(this);
        animation->copyKeyTimesFrom(this);
        animation->copyTimingFunctionsFrom(this);
    } else {
        animation->copyFromValueFrom(this);
        animation->copyToValueFrom(this);
    }
    
    return animation;
}

PlatformCAAnimation::~PlatformCAAnimation()
{
}

bool PlatformCAAnimation::supportsValueFunction()
{
    return true;
}

PlatformAnimationRef PlatformCAAnimation::platformAnimation() const
{
    return m_animation.get();
}

String PlatformCAAnimation::keyPath() const
{
    return CACFAnimationGetKeyPath(m_animation.get());
}

CFTimeInterval PlatformCAAnimation::beginTime() const
{
    return CACFAnimationGetBeginTime(m_animation.get());
}

void PlatformCAAnimation::setBeginTime(CFTimeInterval value)
{
    CACFAnimationSetBeginTime(m_animation.get(), value);
}

CFTimeInterval PlatformCAAnimation::duration() const
{
    return CACFAnimationGetDuration(m_animation.get());
}

void PlatformCAAnimation::setDuration(CFTimeInterval value)
{
    CACFAnimationSetDuration(m_animation.get(), value);
}

float PlatformCAAnimation::speed() const
{
    return CACFAnimationGetSpeed(m_animation.get());
}

void PlatformCAAnimation::setSpeed(float value)
{
    CACFAnimationSetSpeed(m_animation.get(), value);
}

CFTimeInterval PlatformCAAnimation::timeOffset() const
{
    return CACFAnimationGetTimeOffset(m_animation.get());
}

void PlatformCAAnimation::setTimeOffset(CFTimeInterval value)
{
    CACFAnimationSetTimeOffset(m_animation.get(), value);
}

float PlatformCAAnimation::repeatCount() const
{
    return CACFAnimationGetRepeatCount(m_animation.get());
}

void PlatformCAAnimation::setRepeatCount(float value)
{
    CACFAnimationSetRepeatCount(m_animation.get(), value);
}

bool PlatformCAAnimation::autoreverses() const
{
    return CACFAnimationGetAutoreverses(m_animation.get());
}

void PlatformCAAnimation::setAutoreverses(bool value)
{
    CACFAnimationSetAutoreverses(m_animation.get(), value);
}

PlatformCAAnimation::FillModeType PlatformCAAnimation::fillMode() const
{
    return fromCACFFillModeType(CACFAnimationGetFillMode(m_animation.get()));
}

void PlatformCAAnimation::setFillMode(FillModeType value)
{
    CACFAnimationSetFillMode(m_animation.get(), toCACFFillModeType(value));
}

void PlatformCAAnimation::setTimingFunction(const TimingFunction* value)
{
    CACFAnimationSetTimingFunction(m_animation.get(), toCACFTimingFunction(value).get());
}

void PlatformCAAnimation::copyTimingFunctionFrom(const PlatformCAAnimation* value)
{
    CACFAnimationSetTimingFunction(m_animation.get(), CACFAnimationGetTimingFunction(value->m_animation.get()));
}

bool PlatformCAAnimation::isRemovedOnCompletion() const
{
    return CACFAnimationIsRemovedOnCompletion(m_animation.get());
}

void PlatformCAAnimation::setRemovedOnCompletion(bool value)
{
    CACFAnimationSetRemovedOnCompletion(m_animation.get(), value);
}

bool PlatformCAAnimation::isAdditive() const
{
    return CACFAnimationIsAdditive(m_animation.get());
}

void PlatformCAAnimation::setAdditive(bool value)
{
    CACFAnimationSetAdditive(m_animation.get(), value);
}

PlatformCAAnimation::ValueFunctionType PlatformCAAnimation::valueFunction() const
{
    return fromCACFValueFunctionType(CACFValueFunctionGetName(CACFAnimationGetValueFunction(m_animation.get())));
}

void PlatformCAAnimation::setValueFunction(ValueFunctionType value)
{
    CACFAnimationSetValueFunction(m_animation.get(), CACFValueFunctionGetFunctionWithName(toCACFValueFunctionType(value)));
}

void PlatformCAAnimation::setFromValue(float value)
{
    if (animationType() != Basic)
        return;

    RetainPtr<CFNumberRef> v(AdoptCF, CFNumberCreate(0, kCFNumberFloatType, &value));
    CACFAnimationSetFromValue(m_animation.get(), v.get());
}

void PlatformCAAnimation::setFromValue(const WebCore::TransformationMatrix& value)
{
    if (animationType() != Basic)
        return;
    
    RetainPtr<CACFVectorRef> v(AdoptCF, CACFVectorCreateTransform(value));
    CACFAnimationSetFromValue(m_animation.get(), v.get());
}

void PlatformCAAnimation::setFromValue(const FloatPoint3D& value)
{
    if (animationType() != Basic)
        return;

    float a[3] = { value.x(), value.y(), value.z() };
    RetainPtr<CACFVectorRef> v(AdoptCF, CACFVectorCreate(3, a));
    CACFAnimationSetFromValue(m_animation.get(), v.get());
}

void PlatformCAAnimation::setFromValue(const WebCore::Color& value)
{
    if (animationType() != Basic)
        return;

    float a[4] = { value.red(), value.green(), value.blue(), value.alpha() };
    RetainPtr<CACFVectorRef> v(AdoptCF, CACFVectorCreate(4, a));
    CACFAnimationSetFromValue(m_animation.get(), v.get());
}

void PlatformCAAnimation::copyFromValueFrom(const PlatformCAAnimation* value)
{
    if (animationType() != Basic || value->animationType() != Basic)
        return;
    
    CACFAnimationSetFromValue(m_animation.get(), CACFAnimationGetFromValue(value->platformAnimation()));
}

void PlatformCAAnimation::setToValue(float value)
{
    if (animationType() != Basic)
        return;

    RetainPtr<CFNumberRef> v(AdoptCF, CFNumberCreate(0, kCFNumberFloatType, &value));
    CACFAnimationSetToValue(m_animation.get(), v.get());
}

void PlatformCAAnimation::setToValue(const WebCore::TransformationMatrix& value)
{
    if (animationType() != Basic)
        return;

    RetainPtr<CACFVectorRef> v(AdoptCF, CACFVectorCreateTransform(value));
    CACFAnimationSetToValue(m_animation.get(), v.get());
}

void PlatformCAAnimation::setToValue(const FloatPoint3D& value)
{
    if (animationType() != Basic)
        return;

    float a[3] = { value.x(), value.y(), value.z() };
    RetainPtr<CACFVectorRef> v(AdoptCF, CACFVectorCreate(3, a));
    CACFAnimationSetToValue(m_animation.get(), v.get());
}

void PlatformCAAnimation::setToValue(const WebCore::Color& value)
{
    if (animationType() != Basic)
        return;

    float a[4] = { value.red(), value.green(), value.blue(), value.alpha() };
    RetainPtr<CACFVectorRef> v(AdoptCF, CACFVectorCreate(4, a));
    CACFAnimationSetToValue(m_animation.get(), v.get());
}

void PlatformCAAnimation::copyToValueFrom(const PlatformCAAnimation* value)
{
    if (animationType() != Basic || value->animationType() != Basic)
        return;
        
    CACFAnimationSetToValue(m_animation.get(), CACFAnimationGetToValue(value->platformAnimation()));
}


// Keyframe-animation properties.
void PlatformCAAnimation::setValues(const Vector<float>& value)
{
    if (animationType() != Keyframe)
        return;

    RetainPtr<CFMutableArrayRef> array(AdoptCF, CFArrayCreateMutable(0, value.size(), &kCFTypeArrayCallBacks));
    for (size_t i = 0; i < value.size(); ++i) {
        RetainPtr<CFNumberRef> v(AdoptCF, CFNumberCreate(0, kCFNumberFloatType, &value[i]));
        CFArrayAppendValue(array.get(), v.get());
    }

    CACFAnimationSetValues(m_animation.get(), array.get());
}

void PlatformCAAnimation::setValues(const Vector<WebCore::TransformationMatrix>& value)
{
    if (animationType() != Keyframe)
        return;

    RetainPtr<CFMutableArrayRef> array(AdoptCF, CFArrayCreateMutable(0, value.size(), &kCFTypeArrayCallBacks));
    for (size_t i = 0; i < value.size(); ++i) {
        RetainPtr<CACFVectorRef> v(AdoptCF, CACFVectorCreateTransform(value[i]));
        CFArrayAppendValue(array.get(), v.get());
    }

    CACFAnimationSetValues(m_animation.get(), array.get());
}

void PlatformCAAnimation::setValues(const Vector<FloatPoint3D>& value)
{
    if (animationType() != Keyframe)
        return;
        
    RetainPtr<CFMutableArrayRef> array(AdoptCF, CFArrayCreateMutable(0, value.size(), &kCFTypeArrayCallBacks));
    for (size_t i = 0; i < value.size(); ++i) {
        float a[3] = { value[i].x(), value[i].y(), value[i].z() };
        RetainPtr<CACFVectorRef> v(AdoptCF, CACFVectorCreate(3, a));
        CFArrayAppendValue(array.get(), v.get());
    }

    CACFAnimationSetValues(m_animation.get(), array.get());
}

void PlatformCAAnimation::setValues(const Vector<WebCore::Color>& value)
{
    if (animationType() != Keyframe)
        return;
        
    RetainPtr<CFMutableArrayRef> array(AdoptCF, CFArrayCreateMutable(0, value.size(), &kCFTypeArrayCallBacks));
    for (size_t i = 0; i < value.size(); ++i) {
        float a[4] = { value[i].red(), value[i].green(), value[i].blue(), value[i].alpha() };
        RetainPtr<CACFVectorRef> v(AdoptCF, CACFVectorCreate(4, a));
        CFArrayAppendValue(array.get(), v.get());
    }

    CACFAnimationSetValues(m_animation.get(), array.get());
}

void PlatformCAAnimation::copyValuesFrom(const PlatformCAAnimation* value)
{
    if (animationType() != Keyframe || value->animationType() != Keyframe)
        return;
    
    CACFAnimationSetValues(m_animation.get(), CACFAnimationGetValues(value->platformAnimation()));
}

void PlatformCAAnimation::setKeyTimes(const Vector<float>& value)
{
    if (animationType() != Keyframe)
        return;
        
    RetainPtr<CFMutableArrayRef> array(AdoptCF, CFArrayCreateMutable(0, value.size(), &kCFTypeArrayCallBacks));
    for (size_t i = 0; i < value.size(); ++i) {
        RetainPtr<CFNumberRef> v(AdoptCF, CFNumberCreate(0, kCFNumberFloatType, &value[i]));
        CFArrayAppendValue(array.get(), v.get());
    }

    CACFAnimationSetKeyTimes(m_animation.get(), array.get());
}

void PlatformCAAnimation::copyKeyTimesFrom(const PlatformCAAnimation* value)
{
    if (animationType() != Keyframe)
        return;

    CACFAnimationSetKeyTimes(m_animation.get(), CACFAnimationGetKeyTimes(value->platformAnimation()));
}

void PlatformCAAnimation::setTimingFunctions(const Vector<const TimingFunction*>& value)
{
    if (animationType() != Keyframe)
        return;

    RetainPtr<CFMutableArrayRef> array(AdoptCF, CFArrayCreateMutable(0, value.size(), &kCFTypeArrayCallBacks));
    for (size_t i = 0; i < value.size(); ++i) {
        RetainPtr<CFNumberRef> v(AdoptCF, CFNumberCreate(0, kCFNumberFloatType, &value[i]));
        CFArrayAppendValue(array.get(), toCACFTimingFunction(value[i]).get());
    }

    CACFAnimationSetTimingFunctions(m_animation.get(), array.get());
}

void PlatformCAAnimation::copyTimingFunctionsFrom(const PlatformCAAnimation* value)
{
    CACFAnimationSetTimingFunctions(m_animation.get(), CACFAnimationGetTimingFunctions(value->platformAnimation()));
}

#endif // USE(ACCELERATED_COMPOSITING)
