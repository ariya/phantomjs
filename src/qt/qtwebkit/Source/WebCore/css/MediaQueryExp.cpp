/*
 * CSS Media Query
 *
 * Copyright (C) 2006 Kimmo Kinnunen <kimmo.t.kinnunen@nokia.com>.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
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
#include "MediaQueryExp.h"

#include "CSSAspectRatioValue.h"
#include "CSSParser.h"
#include "CSSPrimitiveValue.h"
#include "CSSValueList.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

static inline bool featureWithCSSValueID(const AtomicString& mediaFeature, const CSSParserValue* value)
{
    if (!value->id)
        return false;

    return mediaFeature == MediaFeatureNames::orientationMediaFeature
#if ENABLE(VIEW_MODE_CSS_MEDIA)
        || mediaFeature == MediaFeatureNames::view_modeMediaFeature
#endif // ENABLE(VIEW_MODE_CSS_MEDIA)
        || mediaFeature == MediaFeatureNames::pointerMediaFeature;
}

static inline bool featureWithValidPositiveLenghtOrNumber(const AtomicString& mediaFeature, const CSSParserValue* value)
{
    if (!(((value->unit >= CSSPrimitiveValue::CSS_EMS && value->unit <= CSSPrimitiveValue::CSS_PC) || value->unit == CSSPrimitiveValue::CSS_REMS) || value->unit == CSSPrimitiveValue::CSS_NUMBER) || value->fValue < 0)
        return false;

    return mediaFeature == MediaFeatureNames::heightMediaFeature
        || mediaFeature == MediaFeatureNames::max_heightMediaFeature
        || mediaFeature == MediaFeatureNames::min_heightMediaFeature
        || mediaFeature == MediaFeatureNames::widthMediaFeature
        || mediaFeature == MediaFeatureNames::max_widthMediaFeature
        || mediaFeature == MediaFeatureNames::min_widthMediaFeature
        || mediaFeature == MediaFeatureNames::device_heightMediaFeature
        || mediaFeature == MediaFeatureNames::max_device_heightMediaFeature
        || mediaFeature == MediaFeatureNames::min_device_heightMediaFeature
        || mediaFeature == MediaFeatureNames::device_widthMediaFeature
        || mediaFeature == MediaFeatureNames::max_device_widthMediaFeature
        || mediaFeature == MediaFeatureNames::min_device_widthMediaFeature;
}

static inline bool featureWithValidDensity(const AtomicString& mediaFeature, const CSSParserValue* value)
{
    if ((value->unit != CSSPrimitiveValue::CSS_DPPX && value->unit != CSSPrimitiveValue::CSS_DPI && value->unit != CSSPrimitiveValue::CSS_DPCM) || value->fValue <= 0)
        return false;

    return mediaFeature == MediaFeatureNames::resolutionMediaFeature
        || mediaFeature == MediaFeatureNames::max_resolutionMediaFeature
        || mediaFeature == MediaFeatureNames::min_resolutionMediaFeature;
}

static inline bool featureWithPositiveInteger(const AtomicString& mediaFeature, const CSSParserValue* value)
{
    if (!value->isInt || value->fValue < 0)
        return false;

    return mediaFeature == MediaFeatureNames::colorMediaFeature
        || mediaFeature == MediaFeatureNames::max_colorMediaFeature
        || mediaFeature == MediaFeatureNames::min_colorMediaFeature
        || mediaFeature == MediaFeatureNames::color_indexMediaFeature
        || mediaFeature == MediaFeatureNames::max_color_indexMediaFeature
        || mediaFeature == MediaFeatureNames::min_color_indexMediaFeature
        || mediaFeature == MediaFeatureNames::min_monochromeMediaFeature
        || mediaFeature == MediaFeatureNames::max_monochromeMediaFeature;
}

static inline bool featureWithPositiveNumber(const AtomicString& mediaFeature, const CSSParserValue* value)
{
    if (value->unit != CSSPrimitiveValue::CSS_NUMBER || value->fValue < 0)
        return false;

    return mediaFeature == MediaFeatureNames::transform_2dMediaFeature
        || mediaFeature == MediaFeatureNames::transform_3dMediaFeature
        || mediaFeature == MediaFeatureNames::transitionMediaFeature
        || mediaFeature == MediaFeatureNames::animationMediaFeature
        || mediaFeature == MediaFeatureNames::device_pixel_ratioMediaFeature
        || mediaFeature == MediaFeatureNames::max_device_pixel_ratioMediaFeature
        || mediaFeature == MediaFeatureNames::min_device_pixel_ratioMediaFeature;
}

static inline bool featureWithZeroOrOne(const AtomicString& mediaFeature, const CSSParserValue* value)
{
    if (!value->isInt || !(value->fValue == 1 || !value->fValue))
        return false;

    return mediaFeature == MediaFeatureNames::gridMediaFeature
        || mediaFeature == MediaFeatureNames::hoverMediaFeature;
}

static inline bool featureWithAspectRatio(const AtomicString& mediaFeature)
{
    return mediaFeature == MediaFeatureNames::aspect_ratioMediaFeature
        || mediaFeature == MediaFeatureNames::device_aspect_ratioMediaFeature
        || mediaFeature == MediaFeatureNames::min_aspect_ratioMediaFeature
        || mediaFeature == MediaFeatureNames::max_aspect_ratioMediaFeature
        || mediaFeature == MediaFeatureNames::min_device_aspect_ratioMediaFeature
        || mediaFeature == MediaFeatureNames::max_device_aspect_ratioMediaFeature;
}

static inline bool featureWithoutValue(const AtomicString& mediaFeature)
{
    // Media features that are prefixed by min/max cannot be used without a value.
    return mediaFeature == MediaFeatureNames::monochromeMediaFeature
        || mediaFeature == MediaFeatureNames::colorMediaFeature
        || mediaFeature == MediaFeatureNames::color_indexMediaFeature
        || mediaFeature == MediaFeatureNames::gridMediaFeature
        || mediaFeature == MediaFeatureNames::heightMediaFeature
        || mediaFeature == MediaFeatureNames::widthMediaFeature
        || mediaFeature == MediaFeatureNames::device_heightMediaFeature
        || mediaFeature == MediaFeatureNames::device_widthMediaFeature
        || mediaFeature == MediaFeatureNames::orientationMediaFeature
        || mediaFeature == MediaFeatureNames::aspect_ratioMediaFeature
        || mediaFeature == MediaFeatureNames::device_aspect_ratioMediaFeature
        || mediaFeature == MediaFeatureNames::hoverMediaFeature
        || mediaFeature == MediaFeatureNames::transform_2dMediaFeature
        || mediaFeature == MediaFeatureNames::transform_3dMediaFeature
        || mediaFeature == MediaFeatureNames::transitionMediaFeature
        || mediaFeature == MediaFeatureNames::animationMediaFeature
#if ENABLE(VIEW_MODE_CSS_MEDIA)
        || mediaFeature == MediaFeatureNames::view_modeMediaFeature
#endif // ENABLE(VIEW_MODE_CSS_MEDIA)
        || mediaFeature == MediaFeatureNames::pointerMediaFeature
        || mediaFeature == MediaFeatureNames::device_pixel_ratioMediaFeature
        || mediaFeature == MediaFeatureNames::resolutionMediaFeature;
}

inline MediaQueryExp::MediaQueryExp(const AtomicString& mediaFeature, CSSParserValueList* valueList)
    : m_mediaFeature(mediaFeature)
    , m_value(0)
    , m_isValid(false)
{
    // Initialize media query expression that must have 1 or more values.
    if (valueList) {
        if (valueList->size() == 1) {
            CSSParserValue* value = valueList->current();

            // Media features that use CSSValueIDs.
            if (featureWithCSSValueID(mediaFeature, value))
                m_value = CSSPrimitiveValue::createIdentifier(value->id);

            // Media features that must have non-negative <density>, ie. dppx, dpi or dpcm.
            else if (featureWithValidDensity(mediaFeature, value))
                m_value = CSSPrimitiveValue::create(value->fValue, (CSSPrimitiveValue::UnitTypes) value->unit);

            // Media features that must have non-negative <lenght> or number value.
            else if (featureWithValidPositiveLenghtOrNumber(mediaFeature, value))
                m_value = CSSPrimitiveValue::create(value->fValue, (CSSPrimitiveValue::UnitTypes) value->unit);

            // Media features that must have non-negative integer value.
            else if (featureWithPositiveInteger(mediaFeature, value))
                m_value = CSSPrimitiveValue::create(value->fValue, CSSPrimitiveValue::CSS_NUMBER);

            // Media features that must have non-negative number value.
            else if (featureWithPositiveNumber(mediaFeature, value))
                m_value = CSSPrimitiveValue::create(value->fValue, CSSPrimitiveValue::CSS_NUMBER);

            // Media features that must have (0|1) value.
            else if (featureWithZeroOrOne(mediaFeature, value))
                m_value = CSSPrimitiveValue::create(value->fValue, CSSPrimitiveValue::CSS_NUMBER);

            m_isValid = m_value;
        } else if (valueList->size() == 3 && featureWithAspectRatio(mediaFeature)) {
            // Create list of values.
            // Currently accepts only <integer>/<integer>.
            // Applicable to device-aspect-ratio and aspec-ratio.
            bool isValid = true;
            float numeratorValue = 0;
            float denominatorValue = 0;

            // The aspect-ratio must be <integer> (whitespace)? / (whitespace)? <integer>.
            for (unsigned i = 0; i < 3; ++i, valueList->next()) {
                const CSSParserValue* value = valueList->current();
                if (i != 1 && value->unit == CSSPrimitiveValue::CSS_NUMBER && value->fValue > 0 && value->isInt) {
                    if (!i)
                        numeratorValue = value->fValue;
                    else
                        denominatorValue = value->fValue;
                } else if (i == 1 && value->unit == CSSParserValue::Operator && value->iValue == '/')
                    continue;
                else {
                    isValid = false;
                    break;
                }
            }

            if (isValid)
                m_value = CSSAspectRatioValue::create(numeratorValue, denominatorValue);

            m_isValid = m_value;
        }
    } else if (featureWithoutValue(mediaFeature))
        m_isValid = true;
}

PassOwnPtr<MediaQueryExp> MediaQueryExp::create(const AtomicString& mediaFeature, CSSParserValueList* values)
{
    return adoptPtr(new MediaQueryExp(mediaFeature, values));
}

MediaQueryExp::~MediaQueryExp()
{
}

String MediaQueryExp::serialize() const
{
    if (!m_serializationCache.isNull())
        return m_serializationCache;

    StringBuilder result;
    result.append('(');
    result.append(m_mediaFeature.lower());
    if (m_value) {
        result.appendLiteral(": ");
        result.append(m_value->cssText());
    }
    result.append(')');

    const_cast<MediaQueryExp*>(this)->m_serializationCache = result.toString();
    return m_serializationCache;
}

} // namespace
