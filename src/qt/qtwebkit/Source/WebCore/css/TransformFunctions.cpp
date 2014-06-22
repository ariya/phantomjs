/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2012, 2013 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "TransformFunctions.h"

#include "CSSPrimitiveValueMappings.h"
#include "CSSValueList.h"
#include "CSSValuePool.h"
#include "Matrix3DTransformOperation.h"
#include "MatrixTransformOperation.h"
#include "PerspectiveTransformOperation.h"
#include "RenderStyle.h"
#include "RotateTransformOperation.h"
#include "ScaleTransformOperation.h"
#include "SkewTransformOperation.h"
#include "TranslateTransformOperation.h"
#include "WebKitCSSTransformValue.h"

namespace WebCore {

static TransformOperation::OperationType transformOperationType(WebKitCSSTransformValue::TransformOperationType type)
{
    switch (type) {
    case WebKitCSSTransformValue::ScaleTransformOperation: return TransformOperation::SCALE;
    case WebKitCSSTransformValue::ScaleXTransformOperation: return TransformOperation::SCALE_X;
    case WebKitCSSTransformValue::ScaleYTransformOperation: return TransformOperation::SCALE_Y;
    case WebKitCSSTransformValue::ScaleZTransformOperation: return TransformOperation::SCALE_Z;
    case WebKitCSSTransformValue::Scale3DTransformOperation: return TransformOperation::SCALE_3D;
    case WebKitCSSTransformValue::TranslateTransformOperation: return TransformOperation::TRANSLATE;
    case WebKitCSSTransformValue::TranslateXTransformOperation: return TransformOperation::TRANSLATE_X;
    case WebKitCSSTransformValue::TranslateYTransformOperation: return TransformOperation::TRANSLATE_Y;
    case WebKitCSSTransformValue::TranslateZTransformOperation: return TransformOperation::TRANSLATE_Z;
    case WebKitCSSTransformValue::Translate3DTransformOperation: return TransformOperation::TRANSLATE_3D;
    case WebKitCSSTransformValue::RotateTransformOperation: return TransformOperation::ROTATE;
    case WebKitCSSTransformValue::RotateXTransformOperation: return TransformOperation::ROTATE_X;
    case WebKitCSSTransformValue::RotateYTransformOperation: return TransformOperation::ROTATE_Y;
    case WebKitCSSTransformValue::RotateZTransformOperation: return TransformOperation::ROTATE_Z;
    case WebKitCSSTransformValue::Rotate3DTransformOperation: return TransformOperation::ROTATE_3D;
    case WebKitCSSTransformValue::SkewTransformOperation: return TransformOperation::SKEW;
    case WebKitCSSTransformValue::SkewXTransformOperation: return TransformOperation::SKEW_X;
    case WebKitCSSTransformValue::SkewYTransformOperation: return TransformOperation::SKEW_Y;
    case WebKitCSSTransformValue::MatrixTransformOperation: return TransformOperation::MATRIX;
    case WebKitCSSTransformValue::Matrix3DTransformOperation: return TransformOperation::MATRIX_3D;
    case WebKitCSSTransformValue::PerspectiveTransformOperation: return TransformOperation::PERSPECTIVE;
    case WebKitCSSTransformValue::UnknownTransformOperation: return TransformOperation::NONE;
    }
    return TransformOperation::NONE;
}

static Length convertToFloatLength(const CSSPrimitiveValue* primitiveValue, const RenderStyle* style, const RenderStyle* rootStyle, double multiplier)
{
    return primitiveValue ? primitiveValue->convertToLength<FixedFloatConversion | PercentConversion | CalculatedConversion | FractionConversion | ViewportPercentageConversion>(style, rootStyle, multiplier) : Length(Undefined);
}

bool transformsForValue(const RenderStyle* style, const RenderStyle* rootStyle, CSSValue* value, TransformOperations& outOperations)
{
    if (!value || !value->isValueList()) {
        outOperations.clear();
        return false;
    }

    float zoomFactor = style ? style->effectiveZoom() : 1;
    TransformOperations operations;
    for (CSSValueListIterator i = value; i.hasMore(); i.advance()) {
        CSSValue* currValue = i.value();

        if (!currValue->isWebKitCSSTransformValue())
            continue;

        WebKitCSSTransformValue* transformValue = static_cast<WebKitCSSTransformValue*>(i.value());
        if (!transformValue->length())
            continue;

        bool haveNonPrimitiveValue = false;
        for (unsigned j = 0; j < transformValue->length(); ++j) {
            if (!transformValue->itemWithoutBoundsCheck(j)->isPrimitiveValue()) {
                haveNonPrimitiveValue = true;
                break;
            }
        }
        if (haveNonPrimitiveValue)
            continue;

        CSSPrimitiveValue* firstValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(0));

        switch (transformValue->operationType()) {
        case WebKitCSSTransformValue::ScaleTransformOperation:
        case WebKitCSSTransformValue::ScaleXTransformOperation:
        case WebKitCSSTransformValue::ScaleYTransformOperation: {
            double sx = 1.0;
            double sy = 1.0;
            if (transformValue->operationType() == WebKitCSSTransformValue::ScaleYTransformOperation)
                sy = firstValue->getDoubleValue();
            else {
                sx = firstValue->getDoubleValue();
                if (transformValue->operationType() != WebKitCSSTransformValue::ScaleXTransformOperation) {
                    if (transformValue->length() > 1) {
                        CSSPrimitiveValue* secondValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1));
                        sy = secondValue->getDoubleValue();
                    } else
                        sy = sx;
                }
            }
            operations.operations().append(ScaleTransformOperation::create(sx, sy, 1.0, transformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::ScaleZTransformOperation:
        case WebKitCSSTransformValue::Scale3DTransformOperation: {
            double sx = 1.0;
            double sy = 1.0;
            double sz = 1.0;
            if (transformValue->operationType() == WebKitCSSTransformValue::ScaleZTransformOperation)
                sz = firstValue->getDoubleValue();
            else if (transformValue->operationType() == WebKitCSSTransformValue::ScaleYTransformOperation)
                sy = firstValue->getDoubleValue();
            else {
                sx = firstValue->getDoubleValue();
                if (transformValue->operationType() != WebKitCSSTransformValue::ScaleXTransformOperation) {
                    if (transformValue->length() > 2) {
                        CSSPrimitiveValue* thirdValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(2));
                        sz = thirdValue->getDoubleValue();
                    }
                    if (transformValue->length() > 1) {
                        CSSPrimitiveValue* secondValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1));
                        sy = secondValue->getDoubleValue();
                    } else
                        sy = sx;
                }
            }
            operations.operations().append(ScaleTransformOperation::create(sx, sy, sz, transformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::TranslateTransformOperation:
        case WebKitCSSTransformValue::TranslateXTransformOperation:
        case WebKitCSSTransformValue::TranslateYTransformOperation: {
            Length tx = Length(0, Fixed);
            Length ty = Length(0, Fixed);
            if (transformValue->operationType() == WebKitCSSTransformValue::TranslateYTransformOperation)
                ty = convertToFloatLength(firstValue, style, rootStyle, zoomFactor);
            else {
                tx = convertToFloatLength(firstValue, style, rootStyle, zoomFactor);
                if (transformValue->operationType() != WebKitCSSTransformValue::TranslateXTransformOperation) {
                    if (transformValue->length() > 1) {
                        CSSPrimitiveValue* secondValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1));
                        ty = convertToFloatLength(secondValue, style, rootStyle, zoomFactor);
                    }
                }
            }

            if (tx.isUndefined() || ty.isUndefined())
                return false;

            operations.operations().append(TranslateTransformOperation::create(tx, ty, Length(0, Fixed), transformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::TranslateZTransformOperation:
        case WebKitCSSTransformValue::Translate3DTransformOperation: {
            Length tx = Length(0, Fixed);
            Length ty = Length(0, Fixed);
            Length tz = Length(0, Fixed);
            if (transformValue->operationType() == WebKitCSSTransformValue::TranslateZTransformOperation)
                tz = convertToFloatLength(firstValue, style, rootStyle, zoomFactor);
            else if (transformValue->operationType() == WebKitCSSTransformValue::TranslateYTransformOperation)
                ty = convertToFloatLength(firstValue, style, rootStyle, zoomFactor);
            else {
                tx = convertToFloatLength(firstValue, style, rootStyle, zoomFactor);
                if (transformValue->operationType() != WebKitCSSTransformValue::TranslateXTransformOperation) {
                    if (transformValue->length() > 2) {
                        CSSPrimitiveValue* thirdValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(2));
                        tz = convertToFloatLength(thirdValue, style, rootStyle, zoomFactor);
                    }
                    if (transformValue->length() > 1) {
                        CSSPrimitiveValue* secondValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1));
                        ty = convertToFloatLength(secondValue, style, rootStyle, zoomFactor);
                    }
                }
            }

            if (tx.isUndefined() || ty.isUndefined() || tz.isUndefined())
                return false;

            operations.operations().append(TranslateTransformOperation::create(tx, ty, tz, transformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::RotateTransformOperation: {
            double angle = firstValue->computeDegrees();
            operations.operations().append(RotateTransformOperation::create(0, 0, 1, angle, transformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::RotateXTransformOperation:
        case WebKitCSSTransformValue::RotateYTransformOperation:
        case WebKitCSSTransformValue::RotateZTransformOperation: {
            double x = 0;
            double y = 0;
            double z = 0;
            double angle = firstValue->computeDegrees();

            if (transformValue->operationType() == WebKitCSSTransformValue::RotateXTransformOperation)
                x = 1;
            else if (transformValue->operationType() == WebKitCSSTransformValue::RotateYTransformOperation)
                y = 1;
            else
                z = 1;
            operations.operations().append(RotateTransformOperation::create(x, y, z, angle, transformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::Rotate3DTransformOperation: {
            if (transformValue->length() < 4)
                break;
            CSSPrimitiveValue* secondValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1));
            CSSPrimitiveValue* thirdValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(2));
            CSSPrimitiveValue* fourthValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(3));
            double x = firstValue->getDoubleValue();
            double y = secondValue->getDoubleValue();
            double z = thirdValue->getDoubleValue();
            double angle = fourthValue->computeDegrees();
            operations.operations().append(RotateTransformOperation::create(x, y, z, angle, transformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::SkewTransformOperation:
        case WebKitCSSTransformValue::SkewXTransformOperation:
        case WebKitCSSTransformValue::SkewYTransformOperation: {
            double angleX = 0;
            double angleY = 0;
            double angle = firstValue->computeDegrees();
            if (transformValue->operationType() == WebKitCSSTransformValue::SkewYTransformOperation)
                angleY = angle;
            else {
                angleX = angle;
                if (transformValue->operationType() == WebKitCSSTransformValue::SkewTransformOperation) {
                    if (transformValue->length() > 1) {
                        CSSPrimitiveValue* secondValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1));
                        angleY = secondValue->computeDegrees();
                    }
                }
            }
            operations.operations().append(SkewTransformOperation::create(angleX, angleY, transformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::MatrixTransformOperation: {
            if (transformValue->length() < 6)
                break;
            double a = firstValue->getDoubleValue();
            double b = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1))->getDoubleValue();
            double c = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(2))->getDoubleValue();
            double d = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(3))->getDoubleValue();
            double e = zoomFactor * static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(4))->getDoubleValue();
            double f = zoomFactor * static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(5))->getDoubleValue();
            operations.operations().append(MatrixTransformOperation::create(a, b, c, d, e, f));
            break;
        }
        case WebKitCSSTransformValue::Matrix3DTransformOperation: {
            if (transformValue->length() < 16)
                break;
            TransformationMatrix matrix(static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(0))->getDoubleValue(),
                                static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1))->getDoubleValue(),
                                static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(2))->getDoubleValue(),
                                static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(3))->getDoubleValue(),
                                static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(4))->getDoubleValue(),
                                static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(5))->getDoubleValue(),
                                static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(6))->getDoubleValue(),
                                static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(7))->getDoubleValue(),
                                static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(8))->getDoubleValue(),
                                static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(9))->getDoubleValue(),
                                static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(10))->getDoubleValue(),
                                static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(11))->getDoubleValue(),
                                zoomFactor * static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(12))->getDoubleValue(),
                                zoomFactor * static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(13))->getDoubleValue(),
                                static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(14))->getDoubleValue(),
                                static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(15))->getDoubleValue());
            operations.operations().append(Matrix3DTransformOperation::create(matrix));
            break;
        }
        case WebKitCSSTransformValue::PerspectiveTransformOperation: {
            Length p = Length(0, Fixed);
            if (firstValue->isLength())
                p = convertToFloatLength(firstValue, style, rootStyle, zoomFactor);
            else {
                // This is a quirk that should go away when 3d transforms are finalized.
                double val = firstValue->getDoubleValue();
                p = val >= 0 ? Length(clampToPositiveInteger(val), Fixed) : Length(Undefined);
            }

            if (p.isUndefined())
                return false;

            operations.operations().append(PerspectiveTransformOperation::create(p));
            break;
        }
        case WebKitCSSTransformValue::UnknownTransformOperation:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    outOperations = operations;
    return true;
}

}
