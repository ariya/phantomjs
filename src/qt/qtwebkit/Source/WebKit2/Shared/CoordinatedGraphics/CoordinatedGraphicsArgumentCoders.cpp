/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2012 Company 100, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CoordinatedGraphicsArgumentCoders.h"

#if USE(COORDINATED_GRAPHICS)
#include "WebCoordinatedSurface.h"
#include "WebCoreArgumentCoders.h"
#include <WebCore/Animation.h>
#include <WebCore/Color.h>
#include <WebCore/CoordinatedGraphicsState.h>
#include <WebCore/FloatPoint3D.h>
#include <WebCore/GraphicsLayerAnimation.h>
#include <WebCore/IdentityTransformOperation.h>
#include <WebCore/IntPoint.h>
#include <WebCore/Length.h>
#include <WebCore/Matrix3DTransformOperation.h>
#include <WebCore/MatrixTransformOperation.h>
#include <WebCore/PerspectiveTransformOperation.h>
#include <WebCore/RotateTransformOperation.h>
#include <WebCore/ScaleTransformOperation.h>
#include <WebCore/SkewTransformOperation.h>
#include <WebCore/SurfaceUpdateInfo.h>
#include <WebCore/TimingFunction.h>
#include <WebCore/TransformationMatrix.h>
#include <WebCore/TranslateTransformOperation.h>

#if ENABLE(CSS_FILTERS)
#include <WebCore/FilterOperations.h>
#endif

#if ENABLE(CSS_SHADERS)
#include "WebCustomFilterProgramProxy.h"
#include <WebCore/CoordinatedCustomFilterOperation.h>
#include <WebCore/CoordinatedCustomFilterProgram.h>
#include <WebCore/CustomFilterArrayParameter.h>
#include <WebCore/CustomFilterConstants.h>
#include <WebCore/CustomFilterNumberParameter.h>
#include <WebCore/CustomFilterOperation.h>
#include <WebCore/CustomFilterProgram.h>
#include <WebCore/CustomFilterProgramInfo.h>
#include <WebCore/CustomFilterTransformParameter.h>
#include <WebCore/CustomFilterValidatedProgram.h>
#include <WebCore/ValidatedCustomFilterOperation.h>
#endif

#if USE(GRAPHICS_SURFACE)
#include <WebCore/GraphicsSurface.h>
#endif

using namespace WebCore;
using namespace WebKit;

namespace CoreIPC {

void ArgumentCoder<FloatPoint3D>::encode(ArgumentEncoder& encoder, const FloatPoint3D& floatPoint3D)
{
    SimpleArgumentCoder<FloatPoint3D>::encode(encoder, floatPoint3D);
}

bool ArgumentCoder<FloatPoint3D>::decode(ArgumentDecoder& decoder, FloatPoint3D& floatPoint3D)
{
    return SimpleArgumentCoder<FloatPoint3D>::decode(decoder, floatPoint3D);
}

void ArgumentCoder<Length>::encode(ArgumentEncoder& encoder, const Length& length)
{
    SimpleArgumentCoder<Length>::encode(encoder, length);
}

bool ArgumentCoder<Length>::decode(ArgumentDecoder& decoder, Length& length)
{
    return SimpleArgumentCoder<Length>::decode(decoder, length);
}

void ArgumentCoder<TransformationMatrix>::encode(ArgumentEncoder& encoder, const TransformationMatrix& transformationMatrix)
{
    SimpleArgumentCoder<TransformationMatrix>::encode(encoder, transformationMatrix);
}

bool ArgumentCoder<TransformationMatrix>::decode(ArgumentDecoder& decoder, TransformationMatrix& transformationMatrix)
{
    return SimpleArgumentCoder<TransformationMatrix>::decode(decoder, transformationMatrix);
}

#if ENABLE(CSS_FILTERS)
void ArgumentCoder<WebCore::FilterOperations>::encode(ArgumentEncoder& encoder, const WebCore::FilterOperations& filters)
{
    encoder << static_cast<uint32_t>(filters.size());
    for (size_t i = 0; i < filters.size(); ++i) {
        const FilterOperation* filter = filters.at(i);
        FilterOperation::OperationType type = filter->getOperationType();
        encoder.encodeEnum(type);
        switch (type) {
        case FilterOperation::GRAYSCALE:
        case FilterOperation::SEPIA:
        case FilterOperation::SATURATE:
        case FilterOperation::HUE_ROTATE:
            encoder << static_cast<double>(static_cast<const BasicColorMatrixFilterOperation*>(filter)->amount());
            break;
        case FilterOperation::INVERT:
        case FilterOperation::BRIGHTNESS:
        case FilterOperation::CONTRAST:
        case FilterOperation::OPACITY:
            encoder << static_cast<double>(static_cast<const BasicComponentTransferFilterOperation*>(filter)->amount());
            break;
        case FilterOperation::BLUR:
            ArgumentCoder<Length>::encode(encoder, static_cast<const BlurFilterOperation*>(filter)->stdDeviation());
            break;
        case FilterOperation::DROP_SHADOW: {
            const DropShadowFilterOperation* shadow = static_cast<const DropShadowFilterOperation*>(filter);
            ArgumentCoder<IntPoint>::encode(encoder, shadow->location());
            encoder << static_cast<int32_t>(shadow->stdDeviation());
            ArgumentCoder<Color>::encode(encoder, shadow->color());
            break;
        }
#if ENABLE(CSS_SHADERS)
        case FilterOperation::CUSTOM:
            // Custom Filters are converted to VALIDATED_CUSTOM before reaching this point.
            ASSERT_NOT_REACHED();
            break;
        case FilterOperation::VALIDATED_CUSTOM: {
            const ValidatedCustomFilterOperation* customOperation = static_cast<const ValidatedCustomFilterOperation*>(filter);
            ASSERT(customOperation->validatedProgram());
            RefPtr<CustomFilterValidatedProgram> program = customOperation->validatedProgram();
            ASSERT(program->isInitialized());
            ASSERT(program->platformCompiledProgram());
            ASSERT(program->platformCompiledProgram()->client());
            WebCustomFilterProgramProxy* customFilterProgramProxy = static_cast<WebCustomFilterProgramProxy*>(program->platformCompiledProgram()->client());
            const CustomFilterProgramInfo& programInfo = program->programInfo();
            encoder.encodeEnum(programInfo.meshType());
            encoder << customFilterProgramProxy->id();
            CustomFilterParameterList parameters = customOperation->parameters();
            encoder << static_cast<uint32_t>(parameters.size());
            for (size_t i = 0; i < parameters.size(); ++i) {
                RefPtr<CustomFilterParameter> parameter = parameters[i];
                encoder << parameter->name();
                encoder.encodeEnum(parameter->parameterType());

                switch (parameter->parameterType()) {
                case CustomFilterParameter::ARRAY: {
                    CustomFilterArrayParameter* arrayParameter = static_cast<CustomFilterArrayParameter*>(parameter.get());
                    encoder << static_cast<uint32_t>(arrayParameter->size());
                    for (size_t j = 0; j < arrayParameter->size(); ++j)
                        encoder << arrayParameter->valueAt(j);
                    break;
                }
                case CustomFilterParameter::NUMBER: {
                    CustomFilterNumberParameter* nubmerParameter = static_cast<CustomFilterNumberParameter*>(parameter.get());
                    encoder << static_cast<uint32_t>(nubmerParameter->size());
                    for (size_t j = 0; j < nubmerParameter->size(); ++j)
                        encoder << nubmerParameter->valueAt(j);
                    break;
                }
                case CustomFilterParameter::TRANSFORM: {
                    CustomFilterTransformParameter* transformParameter = static_cast<CustomFilterTransformParameter*>(parameter.get());
                    ArgumentCoder<TransformOperations>::encode(encoder, transformParameter->operations());
                    break;
                }
                default: {
                    ASSERT_NOT_REACHED();
                    break;
                }
                }
            }

            encoder << customOperation->meshRows();
            encoder << customOperation->meshColumns();
            break;
        }
#endif
        default:
            break;
        }
    }
}

bool ArgumentCoder<WebCore::FilterOperations>::decode(ArgumentDecoder& decoder, WebCore::FilterOperations& filters)
{
    uint32_t size;
    if (!decoder.decode(size))
        return false;

    Vector<RefPtr<FilterOperation> >& operations = filters.operations();

    for (size_t i = 0; i < size; ++i) {
        FilterOperation::OperationType type;
        RefPtr<FilterOperation> filter;
        if (!decoder.decodeEnum(type))
            return false;

        switch (type) {
        case FilterOperation::GRAYSCALE:
        case FilterOperation::SEPIA:
        case FilterOperation::SATURATE:
        case FilterOperation::HUE_ROTATE: {
            double value;
            if (!decoder.decode(value))
                return false;
            filter = BasicColorMatrixFilterOperation::create(value, type);
            break;
        }
        case FilterOperation::INVERT:
        case FilterOperation::BRIGHTNESS:
        case FilterOperation::CONTRAST:
        case FilterOperation::OPACITY: {
            double value;
            if (!decoder.decode(value))
                return false;
            filter = BasicComponentTransferFilterOperation::create(value, type);
            break;
        }
        case FilterOperation::BLUR: {
            Length length;
            if (!ArgumentCoder<Length>::decode(decoder, length))
                return false;
            filter = BlurFilterOperation::create(length, type);
            break;
        }
        case FilterOperation::DROP_SHADOW: {
            IntPoint location;
            int32_t stdDeviation;
            Color color;
            if (!ArgumentCoder<IntPoint>::decode(decoder, location))
                return false;
            if (!decoder.decode(stdDeviation))
                return false;
            if (!ArgumentCoder<Color>::decode(decoder, color))
                return false;
            filter = DropShadowFilterOperation::create(location, stdDeviation, color, type);
            break;
        }
#if ENABLE(CSS_SHADERS)
        case FilterOperation::CUSTOM:
            // Custom Filters are converted to VALIDATED_CUSTOM before reaching this point.
            ASSERT_NOT_REACHED();
            break;
        case FilterOperation::VALIDATED_CUSTOM: {
            // FIXME: CustomFilterOperation should not need the meshType.
            // https://bugs.webkit.org/show_bug.cgi?id=102529
            CustomFilterMeshType meshType;
            if (!decoder.decodeEnum(meshType))
                return false;
            int programID = 0;
            if (!decoder.decode(programID))
                return false;

            uint32_t parametersSize;
            if (!decoder.decode(parametersSize))
                return false;

            CustomFilterParameterList parameters(parametersSize);
            for (size_t i = 0; i < parametersSize; ++i) {
                String name;
                CustomFilterParameter::ParameterType parameterType;
                if (!decoder.decode(name))
                    return false;
                if (!decoder.decodeEnum(parameterType))
                    return false;

                switch (parameterType) {
                case CustomFilterParameter::ARRAY: {
                    RefPtr<CustomFilterArrayParameter> arrayParameter = CustomFilterArrayParameter::create(name);
                    uint32_t arrayParameterSize;
                    if (!decoder.decode(arrayParameterSize))
                        return false;
                    double arrayParameterValue;
                    for (size_t j = 0; j < arrayParameterSize; ++j) {
                        if (!decoder.decode(arrayParameterValue))
                            return false;
                        arrayParameter->addValue(arrayParameterValue);
                    }
                    parameters[i] = arrayParameter.release();
                    break;
                }
                case CustomFilterParameter::NUMBER: {
                    RefPtr<CustomFilterNumberParameter> numberParameter = CustomFilterNumberParameter::create(name);
                    uint32_t numberParameterSize;
                    if (!decoder.decode(numberParameterSize))
                        return false;
                    double numberParameterValue;
                    for (size_t j = 0; j < numberParameterSize; ++j) {
                        if (!decoder.decode(numberParameterValue))
                            return false;
                        numberParameter->addValue(numberParameterValue);
                    }
                    parameters[i] = numberParameter.release();
                    break;
                }
                case CustomFilterParameter::TRANSFORM: {
                    RefPtr<CustomFilterTransformParameter> transformParameter = CustomFilterTransformParameter::create(name);
                    TransformOperations operations;
                    if (!ArgumentCoder<TransformOperations>::decode(decoder, operations))
                        return false;
                    transformParameter->setOperations(operations);
                    parameters[i] = transformParameter.release();
                    break;
                }
                default: {
                    ASSERT_NOT_REACHED();
                    return false;
                }
                }
            }

            unsigned meshRows;
            unsigned meshColumns;
            if (!decoder.decode(meshRows))
                return false;
            if (!decoder.decode(meshColumns))
                return false;

            // At this point the Shaders are already validated, so we just use WebCustomFilterOperation for transportation.
            filter = CoordinatedCustomFilterOperation::create(0, programID, parameters, meshRows, meshColumns);
            break;
        }
#endif
        default:
            break;
        }

        if (filter)
            operations.append(filter);
    }

    return true;
}
#endif

#if ENABLE(CSS_SHADERS)
void ArgumentCoder<WebCore::CustomFilterProgramInfo>::encode(ArgumentEncoder& encoder, const CustomFilterProgramInfo& programInfo)
{
    encoder << programInfo.vertexShaderString();
    encoder << programInfo.fragmentShaderString();
    encoder.encodeEnum(programInfo.programType());
    encoder.encodeEnum(programInfo.meshType());
    const CustomFilterProgramMixSettings& mixSettings = programInfo.mixSettings();
    encoder.encodeEnum(mixSettings.blendMode);
    encoder.encodeEnum(mixSettings.compositeOperator);
}

bool ArgumentCoder<WebCore::CustomFilterProgramInfo>::decode(ArgumentDecoder& decoder, CustomFilterProgramInfo& programInfo)
{
    String vertexShaderString;
    String fragmentShaderString;
    CustomFilterProgramType programType;
    CustomFilterMeshType meshType;
    CustomFilterProgramMixSettings mixSettings;
    if (!decoder.decode(vertexShaderString))
        return false;
    if (!decoder.decode(fragmentShaderString))
        return false;
    if (!decoder.decodeEnum(programType))
        return false;
    if (!decoder.decodeEnum(meshType))
        return false;
    if (!decoder.decodeEnum(mixSettings.blendMode))
        return false;
    if (!decoder.decodeEnum(mixSettings.compositeOperator))
        return false;
    programInfo = CustomFilterProgramInfo(vertexShaderString, fragmentShaderString, programType, mixSettings, meshType);
    return true;
}
#endif // ENABLE(CSS_SHADERS)

void ArgumentCoder<TransformOperations>::encode(ArgumentEncoder& encoder, const TransformOperations& transformOperations)
{
    encoder << static_cast<uint32_t>(transformOperations.size());
    for (size_t i = 0; i < transformOperations.size(); ++i) {
        const TransformOperation* operation = transformOperations.at(i);
        encoder.encodeEnum(operation->getOperationType());

        switch (operation->getOperationType()) {
        case TransformOperation::SCALE_X:
        case TransformOperation::SCALE_Y:
        case TransformOperation::SCALE:
        case TransformOperation::SCALE_Z:
        case TransformOperation::SCALE_3D:
            encoder << static_cast<const ScaleTransformOperation*>(operation)->x();
            encoder << static_cast<const ScaleTransformOperation*>(operation)->y();
            encoder << static_cast<const ScaleTransformOperation*>(operation)->z();
            break;
        case TransformOperation::TRANSLATE_X:
        case TransformOperation::TRANSLATE_Y:
        case TransformOperation::TRANSLATE:
        case TransformOperation::TRANSLATE_Z:
        case TransformOperation::TRANSLATE_3D:
            ArgumentCoder<Length>::encode(encoder, static_cast<const TranslateTransformOperation*>(operation)->x());
            ArgumentCoder<Length>::encode(encoder, static_cast<const TranslateTransformOperation*>(operation)->y());
            ArgumentCoder<Length>::encode(encoder, static_cast<const TranslateTransformOperation*>(operation)->z());
            break;
        case TransformOperation::ROTATE:
        case TransformOperation::ROTATE_X:
        case TransformOperation::ROTATE_Y:
        case TransformOperation::ROTATE_3D:
            encoder << static_cast<const RotateTransformOperation*>(operation)->x();
            encoder << static_cast<const RotateTransformOperation*>(operation)->y();
            encoder << static_cast<const RotateTransformOperation*>(operation)->z();
            encoder << static_cast<const RotateTransformOperation*>(operation)->angle();
            break;
        case TransformOperation::SKEW_X:
        case TransformOperation::SKEW_Y:
        case TransformOperation::SKEW:
            encoder << static_cast<const SkewTransformOperation*>(operation)->angleX();
            encoder << static_cast<const SkewTransformOperation*>(operation)->angleY();
            break;
        case TransformOperation::MATRIX:
            ArgumentCoder<TransformationMatrix>::encode(encoder, static_cast<const MatrixTransformOperation*>(operation)->matrix());
            break;
        case TransformOperation::MATRIX_3D:
            ArgumentCoder<TransformationMatrix>::encode(encoder, static_cast<const Matrix3DTransformOperation*>(operation)->matrix());
            break;
        case TransformOperation::PERSPECTIVE:
            ArgumentCoder<Length>::encode(encoder, static_cast<const PerspectiveTransformOperation*>(operation)->perspective());
            break;
        case TransformOperation::IDENTITY:
            break;
        case TransformOperation::NONE:
            ASSERT_NOT_REACHED();
            break;
        }
    }
}

bool ArgumentCoder<TransformOperations>::decode(ArgumentDecoder& decoder, TransformOperations& transformOperations)
{
    uint32_t operationsSize;
    if (!decoder.decode(operationsSize))
        return false;

    for (size_t i = 0; i < operationsSize; ++i) {
        TransformOperation::OperationType operationType;
        if (!decoder.decodeEnum(operationType))
            return false;

        switch (operationType) {
        case TransformOperation::SCALE_X:
        case TransformOperation::SCALE_Y:
        case TransformOperation::SCALE:
        case TransformOperation::SCALE_Z:
        case TransformOperation::SCALE_3D: {
            double x, y, z;
            if (!decoder.decode(x))
                return false;
            if (!decoder.decode(y))
                return false;
            if (!decoder.decode(z))
                return false;
            transformOperations.operations().append(ScaleTransformOperation::create(x, y, z, operationType));
            break;
        }
        case TransformOperation::TRANSLATE_X:
        case TransformOperation::TRANSLATE_Y:
        case TransformOperation::TRANSLATE:
        case TransformOperation::TRANSLATE_Z:
        case TransformOperation::TRANSLATE_3D: {
            Length x, y, z;
            if (!ArgumentCoder<Length>::decode(decoder, x))
                return false;
            if (!ArgumentCoder<Length>::decode(decoder, y))
                return false;
            if (!ArgumentCoder<Length>::decode(decoder, z))
                return false;
            transformOperations.operations().append(TranslateTransformOperation::create(x, y, z, operationType));
            break;
        }
        case TransformOperation::ROTATE:
        case TransformOperation::ROTATE_X:
        case TransformOperation::ROTATE_Y:
        case TransformOperation::ROTATE_3D: {
            double x, y, z, angle;
            if (!decoder.decode(x))
                return false;
            if (!decoder.decode(y))
                return false;
            if (!decoder.decode(z))
                return false;
            if (!decoder.decode(angle))
                return false;
            transformOperations.operations().append(RotateTransformOperation::create(x, y, z, angle, operationType));
            break;
        }
        case TransformOperation::SKEW_X:
        case TransformOperation::SKEW_Y:
        case TransformOperation::SKEW: {
            double angleX, angleY;
            if (!decoder.decode(angleX))
                return false;
            if (!decoder.decode(angleY))
                return false;
            transformOperations.operations().append(SkewTransformOperation::create(angleX, angleY, operationType));
            break;
        }
        case TransformOperation::MATRIX: {
            TransformationMatrix matrix;
            if (!ArgumentCoder<TransformationMatrix>::decode(decoder, matrix))
                return false;
            transformOperations.operations().append(MatrixTransformOperation::create(matrix));
            break;
        }
        case TransformOperation::MATRIX_3D: {
            TransformationMatrix matrix;
            if (!ArgumentCoder<TransformationMatrix>::decode(decoder, matrix))
                return false;
            transformOperations.operations().append(Matrix3DTransformOperation::create(matrix));
            break;
        }
        case TransformOperation::PERSPECTIVE: {
            Length perspective;
            if (!ArgumentCoder<Length>::decode(decoder, perspective))
                return false;
            transformOperations.operations().append(PerspectiveTransformOperation::create(perspective));
            break;
        }
        case TransformOperation::IDENTITY:
            transformOperations.operations().append(IdentityTransformOperation::create());
            break;
        case TransformOperation::NONE:
            ASSERT_NOT_REACHED();
            break;
        }
    }
    return true;
}

static void encodeTimingFunction(ArgumentEncoder& encoder, const TimingFunction* timingFunction)
{
    if (!timingFunction) {
        encoder.encodeEnum(TimingFunction::TimingFunctionType(-1));
        return;
    }

    TimingFunction::TimingFunctionType type = timingFunction ? timingFunction->type() : TimingFunction::LinearFunction;
    encoder.encodeEnum(type);
    switch (type) {
    case TimingFunction::LinearFunction:
        break;
    case TimingFunction::CubicBezierFunction: {
        const CubicBezierTimingFunction* cubic = static_cast<const CubicBezierTimingFunction*>(timingFunction);
        CubicBezierTimingFunction::TimingFunctionPreset bezierPreset = cubic->timingFunctionPreset();
        encoder.encodeEnum(bezierPreset);
        if (bezierPreset == CubicBezierTimingFunction::Custom) {
            encoder << cubic->x1();
            encoder << cubic->y1();
            encoder << cubic->x2();
            encoder << cubic->y2();
        }
        break;
    }
    case TimingFunction::StepsFunction: {
        const StepsTimingFunction* steps = static_cast<const StepsTimingFunction*>(timingFunction);
        encoder << static_cast<uint32_t>(steps->numberOfSteps());
        encoder << steps->stepAtStart();
        break;
    }
    }
}

bool decodeTimingFunction(ArgumentDecoder& decoder, RefPtr<TimingFunction>& timingFunction)
{
    TimingFunction::TimingFunctionType type;
    if (!decoder.decodeEnum(type))
        return false;

    if (type == TimingFunction::TimingFunctionType(-1))
        return true;

    switch (type) {
    case TimingFunction::LinearFunction:
        timingFunction = LinearTimingFunction::create();
        return true;
    case TimingFunction::CubicBezierFunction: {
        double x1, y1, x2, y2;
        CubicBezierTimingFunction::TimingFunctionPreset bezierPreset;
        if (!decoder.decodeEnum(bezierPreset))
            return false;
        if (bezierPreset != CubicBezierTimingFunction::Custom) {
            timingFunction = CubicBezierTimingFunction::create(bezierPreset);
            return true;
        }
        if (!decoder.decode(x1))
            return false;
        if (!decoder.decode(y1))
            return false;
        if (!decoder.decode(x2))
            return false;
        if (!decoder.decode(y2))
            return false;

        timingFunction = CubicBezierTimingFunction::create(x1, y1, x2, y2);
        return true;
    }
    case TimingFunction::StepsFunction: {
        uint32_t numberOfSteps;
        bool stepAtStart;
        if (!decoder.decode(numberOfSteps))
            return false;
        if (!decoder.decode(stepAtStart))
            return false;

        timingFunction = StepsTimingFunction::create(numberOfSteps, stepAtStart);
        return true;
    }
    }

    return false;
}

void ArgumentCoder<GraphicsLayerAnimation>::encode(ArgumentEncoder& encoder, const GraphicsLayerAnimation& animation)
{
    encoder << animation.name();
    encoder << animation.boxSize();
    encoder.encodeEnum(animation.state());
    encoder << animation.startTime();
    encoder << animation.pauseTime();
    encoder << animation.listsMatch();

    RefPtr<Animation> animationObject = animation.animation();
    encoder.encodeEnum(animationObject->direction());
    encoder << static_cast<uint32_t>(animationObject->fillMode());
    encoder << animationObject->duration();
    encoder << animationObject->iterationCount();
    encodeTimingFunction(encoder, animationObject->timingFunction().get());

    const KeyframeValueList& keyframes = animation.keyframes();
    encoder.encodeEnum(keyframes.property());
    encoder << static_cast<uint32_t>(keyframes.size());
    for (size_t i = 0; i < keyframes.size(); ++i) {
        const AnimationValue& value = keyframes.at(i);
        encoder << value.keyTime();
        encodeTimingFunction(encoder, value.timingFunction());
        switch (keyframes.property()) {
        case AnimatedPropertyOpacity:
            encoder << static_cast<const FloatAnimationValue&>(value).value();
            break;
        case AnimatedPropertyWebkitTransform:
            encoder << static_cast<const TransformAnimationValue&>(value).value();
            break;
#if ENABLE(CSS_FILTERS)
        case AnimatedPropertyWebkitFilter:
            encoder << static_cast<const FilterAnimationValue&>(value).value();
            break;
#endif
        default:
            break;
        }
    }
}

bool ArgumentCoder<GraphicsLayerAnimation>::decode(ArgumentDecoder& decoder, GraphicsLayerAnimation& animation)
{
    String name;
    IntSize boxSize;
    GraphicsLayerAnimation::AnimationState state;
    double startTime;
    double pauseTime;
    bool listsMatch;

    Animation::AnimationDirection direction;
    unsigned fillMode;
    double duration;
    double iterationCount;
    RefPtr<TimingFunction> timingFunction;
    RefPtr<Animation> animationObject;

    if (!decoder.decode(name))
        return false;
    if (!decoder.decode(boxSize))
        return false;
    if (!decoder.decodeEnum(state))
        return false;
    if (!decoder.decode(startTime))
        return false;
    if (!decoder.decode(pauseTime))
        return false;
    if (!decoder.decode(listsMatch))
        return false;
    if (!decoder.decodeEnum(direction))
        return false;
    if (!decoder.decode(fillMode))
        return false;
    if (!decoder.decode(duration))
        return false;
    if (!decoder.decode(iterationCount))
        return false;
    if (!decodeTimingFunction(decoder, timingFunction))
        return false;

    animationObject = Animation::create();
    animationObject->setDirection(direction);
    animationObject->setFillMode(fillMode);
    animationObject->setDuration(duration);
    animationObject->setIterationCount(iterationCount);
    if (timingFunction)
        animationObject->setTimingFunction(timingFunction);

    AnimatedPropertyID property;
    if (!decoder.decodeEnum(property))
        return false;
    KeyframeValueList keyframes(property);
    unsigned keyframesSize;
    if (!decoder.decode(keyframesSize))
        return false;
    for (unsigned i = 0; i < keyframesSize; ++i) {
        float keyTime;
        RefPtr<TimingFunction> timingFunction;
        if (!decoder.decode(keyTime))
            return false;
        if (!decodeTimingFunction(decoder, timingFunction))
            return false;

        switch (property) {
        case AnimatedPropertyOpacity: {
            float value;
            if (!decoder.decode(value))
                return false;
            keyframes.insert(FloatAnimationValue::create(keyTime, value, timingFunction));
            break;
        }
        case AnimatedPropertyWebkitTransform: {
            TransformOperations transform;
            if (!decoder.decode(transform))
                return false;
            keyframes.insert(TransformAnimationValue::create(keyTime, transform, timingFunction));
            break;
        }
#if ENABLE(CSS_FILTERS)
        case AnimatedPropertyWebkitFilter: {
            FilterOperations filter;
            if (!decoder.decode(filter))
                return false;
            keyframes.insert(FilterAnimationValue::create(keyTime, filter, timingFunction));
            break;
        }
#endif
        default:
            break;
        }
    }

    animation = GraphicsLayerAnimation(name, keyframes, boxSize, animationObject.get(), startTime, listsMatch);
    animation.setState(state, pauseTime);

    return true;
}

void ArgumentCoder<GraphicsLayerAnimations>::encode(ArgumentEncoder& encoder, const GraphicsLayerAnimations& animations)
{
    encoder << animations.animations();
}

bool ArgumentCoder<GraphicsLayerAnimations>::decode(ArgumentDecoder& decoder, GraphicsLayerAnimations& animations)
{
    return decoder.decode(animations.animations());
}

#if USE(GRAPHICS_SURFACE)
void ArgumentCoder<WebCore::GraphicsSurfaceToken>::encode(ArgumentEncoder& encoder, const WebCore::GraphicsSurfaceToken& token)
{
#if OS(DARWIN)
    encoder << Attachment(token.frontBufferHandle, MACH_MSG_TYPE_MOVE_SEND);
    encoder << Attachment(token.backBufferHandle, MACH_MSG_TYPE_MOVE_SEND);
#elif OS(WINDOWS)
    encoder << reinterpret_cast<uint64_t>(token.frontBufferHandle);
    encoder << reinterpret_cast<uint64_t>(token.backBufferHandle);
#elif OS(LINUX)
    encoder << token.frontBufferHandle;
#endif
}

bool ArgumentCoder<WebCore::GraphicsSurfaceToken>::decode(ArgumentDecoder& decoder, WebCore::GraphicsSurfaceToken& token)
{
#if OS(WINDOWS)
    uint64_t frontBufferHandle;
    if (!decoder.decode(frontBufferHandle))
        return false;
    token.frontBufferHandle = reinterpret_cast<GraphicsSurfaceToken::BufferHandle>(frontBufferHandle);
    uint64_t backBufferHandle;
    if (!decoder.decode(backBufferHandle))
        return false;
    token.backBufferHandle = reinterpret_cast<GraphicsSurfaceToken::BufferHandle>(backBufferHandle);
#elif OS(DARWIN)
    Attachment frontAttachment, backAttachment;
    if (!decoder.decode(frontAttachment))
        return false;
    if (!decoder.decode(backAttachment))
        return false;

    token = GraphicsSurfaceToken(frontAttachment.port(), backAttachment.port());
#elif OS(LINUX)
    if (!decoder.decode(token.frontBufferHandle))
        return false;
#endif
    return true;
}
#endif

void ArgumentCoder<SurfaceUpdateInfo>::encode(ArgumentEncoder& encoder, const SurfaceUpdateInfo& surfaceUpdateInfo)
{
    SimpleArgumentCoder<SurfaceUpdateInfo>::encode(encoder, surfaceUpdateInfo);
}

bool ArgumentCoder<SurfaceUpdateInfo>::decode(ArgumentDecoder& decoder, SurfaceUpdateInfo& surfaceUpdateInfo)
{
    return SimpleArgumentCoder<SurfaceUpdateInfo>::decode(decoder, surfaceUpdateInfo);
}

void ArgumentCoder<CoordinatedGraphicsLayerState>::encode(ArgumentEncoder& encoder, const CoordinatedGraphicsLayerState& state)
{
    encoder << state.changeMask;

    if (state.flagsChanged)
        encoder << state.flags;

    if (state.positionChanged)
        encoder << state.pos;

    if (state.anchorPointChanged)
        encoder << state.anchorPoint;

    if (state.sizeChanged)
        encoder << state.size;

    if (state.transformChanged)
        encoder << state.transform;

    if (state.childrenTransformChanged)
        encoder << state.childrenTransform;

    if (state.contentsRectChanged)
        encoder << state.contentsRect;

    if (state.contentsTilingChanged) {
        encoder << state.contentsTileSize;
        encoder << state.contentsTilePhase;
    }

    if (state.opacityChanged)
        encoder << state.opacity;

    if (state.solidColorChanged)
        encoder << state.solidColor;

    if (state.debugBorderColorChanged)
        encoder << state.debugBorderColor;

    if (state.debugBorderWidthChanged)
        encoder << state.debugBorderWidth;

#if ENABLE(CSS_FILTERS)
    if (state.filtersChanged)
        encoder << state.filters;
#endif

    if (state.animationsChanged)
        encoder << state.animations;

    if (state.childrenChanged)
        encoder << state.children;

    encoder << state.tilesToCreate;
    encoder << state.tilesToRemove;

    if (state.replicaChanged)
        encoder << state.replica;

    if (state.maskChanged)
        encoder << state.mask;

    if (state.imageChanged)
        encoder << state.imageID;

    if (state.repaintCountChanged)
        encoder << state.repaintCount;

    encoder << state.tilesToUpdate;

#if USE(GRAPHICS_SURFACE)
    if (state.canvasChanged) {
        encoder << state.canvasSize;
        encoder << state.canvasToken;
        encoder << state.canvasFrontBuffer;
        encoder << state.canvasSurfaceFlags;
    }
#endif

    if (state.committedScrollOffsetChanged)
        encoder << state.committedScrollOffset;
}

bool ArgumentCoder<CoordinatedGraphicsLayerState>::decode(ArgumentDecoder& decoder, CoordinatedGraphicsLayerState& state)
{
    if (!decoder.decode(state.changeMask))
        return false;

    if (state.flagsChanged && !decoder.decode(state.flags))
        return false;

    if (state.positionChanged && !decoder.decode(state.pos))
        return false;

    if (state.anchorPointChanged && !decoder.decode(state.anchorPoint))
        return false;

    if (state.sizeChanged && !decoder.decode(state.size))
        return false;

    if (state.transformChanged && !decoder.decode(state.transform))
        return false;

    if (state.childrenTransformChanged && !decoder.decode(state.childrenTransform))
        return false;

    if (state.contentsRectChanged && !decoder.decode(state.contentsRect))
        return false;

    if (state.contentsTilingChanged) {
        if (!decoder.decode(state.contentsTileSize))
            return false;
        if (!decoder.decode(state.contentsTilePhase))
            return false;
    }

    if (state.opacityChanged && !decoder.decode(state.opacity))
        return false;

    if (state.solidColorChanged && !decoder.decode(state.solidColor))
        return false;

    if (state.debugBorderColorChanged && !decoder.decode(state.debugBorderColor))
        return false;

    if (state.debugBorderWidthChanged && !decoder.decode(state.debugBorderWidth))
        return false;

#if ENABLE(CSS_FILTERS)
    if (state.filtersChanged && !decoder.decode(state.filters))
        return false;
#endif

    if (state.animationsChanged && !decoder.decode(state.animations))
        return false;

    if (state.childrenChanged && !decoder.decode(state.children))
        return false;

    if (!decoder.decode(state.tilesToCreate))
        return false;

    if (!decoder.decode(state.tilesToRemove))
        return false;

    if (state.replicaChanged && !decoder.decode(state.replica))
        return false;

    if (state.maskChanged && !decoder.decode(state.mask))
        return false;

    if (state.imageChanged && !decoder.decode(state.imageID))
        return false;

    if (state.repaintCountChanged && !decoder.decode(state.repaintCount))
        return false;

    if (!decoder.decode(state.tilesToUpdate))
        return false;

#if USE(GRAPHICS_SURFACE)
    if (state.canvasChanged) {
        if (!decoder.decode(state.canvasSize))
            return false;

        if (!decoder.decode(state.canvasToken))
            return false;

        if (!decoder.decode(state.canvasFrontBuffer))
            return false;

        if (!decoder.decode(state.canvasSurfaceFlags))
            return false;
    }
#endif

    if (state.committedScrollOffsetChanged && !decoder.decode(state.committedScrollOffset))
        return false;

    return true;
}

void ArgumentCoder<TileUpdateInfo>::encode(ArgumentEncoder& encoder, const TileUpdateInfo& updateInfo)
{
    SimpleArgumentCoder<TileUpdateInfo>::encode(encoder, updateInfo);
}

bool ArgumentCoder<TileUpdateInfo>::decode(ArgumentDecoder& decoder, TileUpdateInfo& updateInfo)
{
    return SimpleArgumentCoder<TileUpdateInfo>::decode(decoder, updateInfo);
}

void ArgumentCoder<TileCreationInfo>::encode(ArgumentEncoder& encoder, const TileCreationInfo& updateInfo)
{
    SimpleArgumentCoder<TileCreationInfo>::encode(encoder, updateInfo);
}

bool ArgumentCoder<TileCreationInfo>::decode(ArgumentDecoder& decoder, TileCreationInfo& updateInfo)
{
    return SimpleArgumentCoder<TileCreationInfo>::decode(decoder, updateInfo);
}

static void encodeCoordinatedSurface(ArgumentEncoder& encoder, const RefPtr<CoordinatedSurface>& surface)
{
    bool isValidSurface = false;
    if (!surface) {
        encoder << isValidSurface;
        return;
    }

    WebCoordinatedSurface* webCoordinatedSurface = static_cast<WebCoordinatedSurface*>(surface.get());
    WebCoordinatedSurface::Handle handle;
    if (webCoordinatedSurface->createHandle(handle))
        isValidSurface = true;

    encoder << isValidSurface;

    if (isValidSurface)
        encoder << handle;
}

static bool decodeCoordinatedSurface(ArgumentDecoder& decoder, RefPtr<CoordinatedSurface>& surface)
{
    bool isValidSurface;
    if (!decoder.decode(isValidSurface))
        return false;

    if (!isValidSurface)
        return true;

    WebCoordinatedSurface::Handle handle;
    if (!decoder.decode(handle))
        return false;

    surface = WebCoordinatedSurface::create(handle);
    return true;
}

void ArgumentCoder<CoordinatedGraphicsState>::encode(ArgumentEncoder& encoder, const CoordinatedGraphicsState& state)
{
    encoder << state.rootCompositingLayer;
    encoder << state.scrollPosition;
    encoder << state.contentsSize;
    encoder << state.coveredRect;

    encoder << state.layersToCreate;
    encoder << state.layersToUpdate;
    encoder << state.layersToRemove;

    encoder << state.imagesToCreate;
    encoder << state.imagesToRemove;

    // We need to encode WebCoordinatedSurface::Handle right after it's creation.
    // That's why we cannot use simple std::pair encoder.
    encoder << static_cast<uint64_t>(state.imagesToUpdate.size());
    for (size_t i = 0; i < state.imagesToUpdate.size(); ++i) {
        encoder << state.imagesToUpdate[i].first;
        encodeCoordinatedSurface(encoder, state.imagesToUpdate[i].second);
    }
    encoder << state.imagesToClear;

    encoder << static_cast<uint64_t>(state.updateAtlasesToCreate.size());
    for (size_t i = 0; i < state.updateAtlasesToCreate.size(); ++i) {
        encoder << state.updateAtlasesToCreate[i].first;
        encodeCoordinatedSurface(encoder, state.updateAtlasesToCreate[i].second);
    }
    encoder << state.updateAtlasesToRemove;

#if ENABLE(CSS_SHADERS)
    encoder << static_cast<uint64_t>(state.customFiltersToCreate.size());
    for (size_t i = 0; i < state.customFiltersToCreate.size(); ++i) {
        encoder << state.customFiltersToCreate[i].first;
        encoder << state.customFiltersToCreate[i].second;
    }
    encoder << state.customFiltersToRemove;
#endif
}

bool ArgumentCoder<CoordinatedGraphicsState>::decode(ArgumentDecoder& decoder, CoordinatedGraphicsState& state)
{
    if (!decoder.decode(state.rootCompositingLayer))
        return false;

    if (!decoder.decode(state.scrollPosition))
        return false;

    if (!decoder.decode(state.contentsSize))
        return false;

    if (!decoder.decode(state.coveredRect))
        return false;

    if (!decoder.decode(state.layersToCreate))
        return false;

    if (!decoder.decode(state.layersToUpdate))
        return false;

    if (!decoder.decode(state.layersToRemove))
        return false;

    if (!decoder.decode(state.imagesToCreate))
        return false;

    if (!decoder.decode(state.imagesToRemove))
        return false;

    uint64_t sizeOfImagesToUpdate;
    if (!decoder.decode(sizeOfImagesToUpdate))
        return false;

    for (uint64_t i = 0; i < sizeOfImagesToUpdate; ++i) {
        CoordinatedImageBackingID imageID;
        if (!decoder.decode(imageID))
            return false;

        RefPtr<CoordinatedSurface> surface;
        if (!decodeCoordinatedSurface(decoder, surface))
            return false;

        state.imagesToUpdate.append(std::make_pair(imageID, surface.release()));
    }

    if (!decoder.decode(state.imagesToClear))
        return false;

    uint64_t sizeOfUpdateAtlasesToCreate;
    if (!decoder.decode(sizeOfUpdateAtlasesToCreate))
        return false;

    for (uint64_t i = 0; i < sizeOfUpdateAtlasesToCreate; ++i) {
        uint32_t atlasID;
        if (!decoder.decode(atlasID))
            return false;

        RefPtr<CoordinatedSurface> surface;
        if (!decodeCoordinatedSurface(decoder, surface))
            return false;

        state.updateAtlasesToCreate.append(std::make_pair(atlasID, surface.release()));
    }

    if (!decoder.decode(state.updateAtlasesToRemove))
        return false;

#if ENABLE(CSS_SHADERS)
    uint64_t sizeOfCustomFiltersToCreate;
    if (!decoder.decode(sizeOfCustomFiltersToCreate))
        return false;

    for (uint64_t i = 0; i < sizeOfCustomFiltersToCreate; ++i) {
        uint32_t filterID;
        if (!decoder.decode(filterID))
            return false;
        CustomFilterProgramInfo filterInfo;
        if (!decoder.decode(filterInfo))
            return false;
        state.customFiltersToCreate.append(std::make_pair(filterID, filterInfo));
    }

    if (!decoder.decode(state.customFiltersToRemove))
        return false;
#endif

    return true;
}

} // namespace CoreIPC

#endif // USE(COORDINATED_GRAPHICS)
