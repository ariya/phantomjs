/*
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFEConvolveMatrixElement.h"

#include "Attr.h"
#include "FilterEffect.h"
#include "FloatPoint.h"
#include "IntPoint.h"
#include "IntSize.h"
#include "SVGElementInstance.h"
#include "SVGFilterBuilder.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGFEConvolveMatrixElement, SVGNames::inAttr, In1, in1)
DEFINE_ANIMATED_INTEGER_MULTIPLE_WRAPPERS(SVGFEConvolveMatrixElement, SVGNames::orderAttr, orderXIdentifier(), OrderX, orderX)
DEFINE_ANIMATED_INTEGER_MULTIPLE_WRAPPERS(SVGFEConvolveMatrixElement, SVGNames::orderAttr, orderYIdentifier(), OrderY, orderY) 
DEFINE_ANIMATED_NUMBER_LIST(SVGFEConvolveMatrixElement, SVGNames::kernelMatrixAttr, KernelMatrix, kernelMatrix)
DEFINE_ANIMATED_NUMBER(SVGFEConvolveMatrixElement, SVGNames::divisorAttr, Divisor, divisor)
DEFINE_ANIMATED_NUMBER(SVGFEConvolveMatrixElement, SVGNames::biasAttr, Bias, bias)
DEFINE_ANIMATED_INTEGER(SVGFEConvolveMatrixElement, SVGNames::targetXAttr, TargetX, targetX)
DEFINE_ANIMATED_INTEGER(SVGFEConvolveMatrixElement, SVGNames::targetYAttr, TargetY, targetY)
DEFINE_ANIMATED_ENUMERATION(SVGFEConvolveMatrixElement, SVGNames::edgeModeAttr, EdgeMode, edgeMode, EdgeModeType)
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFEConvolveMatrixElement, SVGNames::kernelUnitLengthAttr, kernelUnitLengthXIdentifier(), KernelUnitLengthX, kernelUnitLengthX)
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFEConvolveMatrixElement, SVGNames::kernelUnitLengthAttr, kernelUnitLengthYIdentifier(), KernelUnitLengthY, kernelUnitLengthY)
DEFINE_ANIMATED_BOOLEAN(SVGFEConvolveMatrixElement, SVGNames::preserveAlphaAttr, PreserveAlpha, preserveAlpha)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGFEConvolveMatrixElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(in1)
    REGISTER_LOCAL_ANIMATED_PROPERTY(orderX)
    REGISTER_LOCAL_ANIMATED_PROPERTY(orderY)
    REGISTER_LOCAL_ANIMATED_PROPERTY(kernelMatrix)
    REGISTER_LOCAL_ANIMATED_PROPERTY(divisor)
    REGISTER_LOCAL_ANIMATED_PROPERTY(bias)
    REGISTER_LOCAL_ANIMATED_PROPERTY(targetX)
    REGISTER_LOCAL_ANIMATED_PROPERTY(targetY)
    REGISTER_LOCAL_ANIMATED_PROPERTY(edgeMode)
    REGISTER_LOCAL_ANIMATED_PROPERTY(kernelUnitLengthX)
    REGISTER_LOCAL_ANIMATED_PROPERTY(kernelUnitLengthY)
    REGISTER_LOCAL_ANIMATED_PROPERTY(preserveAlpha)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGFilterPrimitiveStandardAttributes)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGFEConvolveMatrixElement::SVGFEConvolveMatrixElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
    , m_edgeMode(EDGEMODE_DUPLICATE)
{
    ASSERT(hasTagName(SVGNames::feConvolveMatrixTag));
    registerAnimatedPropertiesForSVGFEConvolveMatrixElement();
}

PassRefPtr<SVGFEConvolveMatrixElement> SVGFEConvolveMatrixElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFEConvolveMatrixElement(tagName, document));
}

const AtomicString& SVGFEConvolveMatrixElement::kernelUnitLengthXIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGKernelUnitLengthX", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

const AtomicString& SVGFEConvolveMatrixElement::kernelUnitLengthYIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGKernelUnitLengthY", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

const AtomicString& SVGFEConvolveMatrixElement::orderXIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGOrderX", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

const AtomicString& SVGFEConvolveMatrixElement::orderYIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGOrderY", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

bool SVGFEConvolveMatrixElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        supportedAttributes.add(SVGNames::inAttr);
        supportedAttributes.add(SVGNames::orderAttr);
        supportedAttributes.add(SVGNames::kernelMatrixAttr);
        supportedAttributes.add(SVGNames::edgeModeAttr);
        supportedAttributes.add(SVGNames::divisorAttr);
        supportedAttributes.add(SVGNames::biasAttr);
        supportedAttributes.add(SVGNames::targetXAttr);
        supportedAttributes.add(SVGNames::targetYAttr);
        supportedAttributes.add(SVGNames::kernelUnitLengthAttr);
        supportedAttributes.add(SVGNames::preserveAlphaAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGFEConvolveMatrixElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGFilterPrimitiveStandardAttributes::parseAttribute(name, value);
        return;
    }

    if (name == SVGNames::inAttr) {
        setIn1BaseValue(value);
        return;
    }

    if (name == SVGNames::orderAttr) {
        float x, y;
        if (parseNumberOptionalNumber(value, x, y) && x >= 1 && y >= 1) {
            setOrderXBaseValue(x);
            setOrderYBaseValue(y);
        } else
            document()->accessSVGExtensions()->reportWarning(
                "feConvolveMatrix: problem parsing order=\"" + value
                + "\". Filtered element will not be displayed.");
        return;
    }

    if (name == SVGNames::edgeModeAttr) {
        EdgeModeType propertyValue = SVGPropertyTraits<EdgeModeType>::fromString(value);
        if (propertyValue > 0)
            setEdgeModeBaseValue(propertyValue);
        else
            document()->accessSVGExtensions()->reportWarning(
                "feConvolveMatrix: problem parsing edgeMode=\"" + value
                + "\". Filtered element will not be displayed.");
        return;
    }

    if (name == SVGNames::kernelMatrixAttr) {
        SVGNumberList newList;
        newList.parse(value);
        detachAnimatedKernelMatrixListWrappers(newList.size());
        setKernelMatrixBaseValue(newList);
        return;
    }

    if (name == SVGNames::divisorAttr) {
        float divisor = value.toFloat();
        if (divisor)
            setDivisorBaseValue(divisor);
        else
            document()->accessSVGExtensions()->reportWarning(
                "feConvolveMatrix: problem parsing divisor=\"" + value
                + "\". Filtered element will not be displayed.");
        return;
    }
    
    if (name == SVGNames::biasAttr) {
        setBiasBaseValue(value.toFloat());
        return;
    }

    if (name == SVGNames::targetXAttr) {
        setTargetXBaseValue(value.string().toUIntStrict());
        return;
    }

    if (name == SVGNames::targetYAttr) {
        setTargetYBaseValue(value.string().toUIntStrict());
        return;
    }

    if (name == SVGNames::kernelUnitLengthAttr) {
        float x, y;
        if (parseNumberOptionalNumber(value, x, y) && x > 0 && y > 0) {
            setKernelUnitLengthXBaseValue(x);
            setKernelUnitLengthYBaseValue(y);
        } else
            document()->accessSVGExtensions()->reportWarning(
                "feConvolveMatrix: problem parsing kernelUnitLength=\"" + value
                + "\". Filtered element will not be displayed.");
        return;
    }

    if (name == SVGNames::preserveAlphaAttr) {
        if (value == "true")
            setPreserveAlphaBaseValue(true);
        else if (value == "false")
            setPreserveAlphaBaseValue(false);
        else
            document()->accessSVGExtensions()->reportWarning(
                "feConvolveMatrix: problem parsing preserveAlphaAttr=\"" + value
                + "\". Filtered element will not be displayed.");
        return;
    }

    ASSERT_NOT_REACHED();
}

bool SVGFEConvolveMatrixElement::setFilterEffectAttribute(FilterEffect* effect, const QualifiedName& attrName)
{
    FEConvolveMatrix* convolveMatrix = static_cast<FEConvolveMatrix*>(effect);
    if (attrName == SVGNames::edgeModeAttr)
        return convolveMatrix->setEdgeMode(edgeMode());
    if (attrName == SVGNames::divisorAttr)
        return convolveMatrix->setDivisor(divisor());
    if (attrName == SVGNames::biasAttr)
        return convolveMatrix->setBias(bias());
    if (attrName == SVGNames::targetXAttr)
       return convolveMatrix->setTargetOffset(IntPoint(targetX(), targetY()));
    if (attrName == SVGNames::targetYAttr)
       return convolveMatrix->setTargetOffset(IntPoint(targetX(), targetY()));
    if (attrName == SVGNames::kernelUnitLengthAttr)
       return convolveMatrix->setKernelUnitLength(FloatPoint(kernelUnitLengthX(), kernelUnitLengthY()));
    if (attrName == SVGNames::preserveAlphaAttr)
        return convolveMatrix->setPreserveAlpha(preserveAlpha());

    ASSERT_NOT_REACHED();
    return false;
}

void SVGFEConvolveMatrixElement::setOrder(float x, float y)
{
    setOrderXBaseValue(x);
    setOrderYBaseValue(y);
    invalidate();
}

void SVGFEConvolveMatrixElement::setKernelUnitLength(float x, float y)
{
    setKernelUnitLengthXBaseValue(x);
    setKernelUnitLengthYBaseValue(y);
    invalidate();
}

void SVGFEConvolveMatrixElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);

    if (attrName == SVGNames::edgeModeAttr
        || attrName == SVGNames::divisorAttr
        || attrName == SVGNames::biasAttr
        || attrName == SVGNames::targetXAttr
        || attrName == SVGNames::targetYAttr
        || attrName == SVGNames::kernelUnitLengthAttr
        || attrName == SVGNames::preserveAlphaAttr) {
        primitiveAttributeChanged(attrName);
        return;
    }

    if (attrName == SVGNames::inAttr
        || attrName == SVGNames::orderAttr
        || attrName == SVGNames::kernelMatrixAttr) {
        invalidate();
        return;
    }

    ASSERT_NOT_REACHED();
}

PassRefPtr<FilterEffect> SVGFEConvolveMatrixElement::build(SVGFilterBuilder* filterBuilder, Filter* filter)
{
    FilterEffect* input1 = filterBuilder->getEffectById(in1());

    if (!input1)
        return 0;

    int orderXValue = orderX();
    int orderYValue = orderY();
    if (!hasAttribute(SVGNames::orderAttr)) {
        orderXValue = 3;
        orderYValue = 3;
    }
    // Spec says order must be > 0. Bail if it is not.
    if (orderXValue < 1 || orderYValue < 1)
        return 0;
    SVGNumberList& kernelMatrix = this->kernelMatrix();
    int kernelMatrixSize = kernelMatrix.size();
    // The spec says this is a requirement, and should bail out if fails
    if (orderXValue * orderYValue != kernelMatrixSize)
        return 0;

    int targetXValue = targetX();
    int targetYValue = targetY();
    if (hasAttribute(SVGNames::targetXAttr) && (targetXValue < 0 || targetXValue >= orderXValue))
        return 0;
    // The spec says the default value is: targetX = floor ( orderX / 2 ))
    if (!hasAttribute(SVGNames::targetXAttr))
        targetXValue = static_cast<int>(floorf(orderXValue / 2));
    if (hasAttribute(SVGNames::targetYAttr) && (targetYValue < 0 || targetYValue >= orderYValue))
        return 0;
    // The spec says the default value is: targetY = floor ( orderY / 2 ))
    if (!hasAttribute(SVGNames::targetYAttr))
        targetYValue = static_cast<int>(floorf(orderYValue / 2));

    // Spec says default kernelUnitLength is 1.0, and a specified length cannot be 0.
    int kernelUnitLengthXValue = kernelUnitLengthX();
    int kernelUnitLengthYValue = kernelUnitLengthY();
    if (!hasAttribute(SVGNames::kernelUnitLengthAttr)) {
        kernelUnitLengthXValue = 1;
        kernelUnitLengthYValue = 1;
    }
    if (kernelUnitLengthXValue <= 0 || kernelUnitLengthYValue <= 0)
        return 0;

    float divisorValue = divisor();
    if (hasAttribute(SVGNames::divisorAttr) && !divisorValue)
        return 0;
    if (!hasAttribute(SVGNames::divisorAttr)) {
        for (int i = 0; i < kernelMatrixSize; ++i)
            divisorValue += kernelMatrix.at(i);
        if (!divisorValue)
            divisorValue = 1;
    }

    RefPtr<FilterEffect> effect = FEConvolveMatrix::create(filter,
                    IntSize(orderXValue, orderYValue), divisorValue,
                    bias(), IntPoint(targetXValue, targetYValue), edgeMode(),
                    FloatPoint(kernelUnitLengthXValue, kernelUnitLengthYValue), preserveAlpha(), kernelMatrix);
    effect->inputEffects().append(input1);
    return effect.release();
}

} // namespace WebCore

#endif // ENABLE(SVG)
