/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "WebKitCSSMatrix.h"

#include "CSSParser.h"
#include "CSSStyleSelector.h"
#include "CSSMutableStyleDeclaration.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "ExceptionCode.h"
#include <wtf/MathExtras.h>

namespace WebCore {

WebKitCSSMatrix::WebKitCSSMatrix(const TransformationMatrix& m)
    : m_matrix(m)
{
}

WebKitCSSMatrix::WebKitCSSMatrix(const String& s, ExceptionCode& ec) 
{
    setMatrixValue(s, ec);
}

WebKitCSSMatrix::~WebKitCSSMatrix()
{
}

void WebKitCSSMatrix::setMatrixValue(const String& string, ExceptionCode& ec)
{
    RefPtr<CSSMutableStyleDeclaration> styleDeclaration = CSSMutableStyleDeclaration::create();
    if (CSSParser::parseValue(styleDeclaration.get(), CSSPropertyWebkitTransform, string, true, true)) {
        // Convert to TransformOperations. This can fail if a property 
        // requires style (i.e., param uses 'ems' or 'exs')
        RefPtr<CSSValue> value = styleDeclaration->getPropertyCSSValue(CSSPropertyWebkitTransform);

        // Check for a "none" or empty transform. In these cases we can use the default identity matrix.
        if (!value || (value->isPrimitiveValue() && (static_cast<CSSPrimitiveValue*>(value.get()))->getIdent() == CSSValueNone))
            return;

        TransformOperations operations;
        if (!CSSStyleSelector::createTransformOperations(value.get(), 0, 0, operations)) {
            ec = SYNTAX_ERR;
            return;
        }
        
        // Convert transform operations to a TransformationMatrix. This can fail
        // if a param has a percentage ('%')
        TransformationMatrix t;
        for (unsigned i = 0; i < operations.operations().size(); ++i) {
            if (operations.operations()[i].get()->apply(t, IntSize(0, 0))) {
                ec = SYNTAX_ERR;
                return;
            }
        }
        
        // set the matrix
        m_matrix = t;
    } else if (!string.isEmpty()) // There is something there but parsing failed
        ec = SYNTAX_ERR;
}

// Perform a concatenation of the matrices (this * secondMatrix)
PassRefPtr<WebKitCSSMatrix> WebKitCSSMatrix::multiply(WebKitCSSMatrix* secondMatrix) const
{
    if (!secondMatrix)
        return 0;

    return WebKitCSSMatrix::create(TransformationMatrix(m_matrix).multiply(secondMatrix->m_matrix));
}

PassRefPtr<WebKitCSSMatrix> WebKitCSSMatrix::inverse(ExceptionCode& ec) const
{
    if (!m_matrix.isInvertible()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }
    
    return WebKitCSSMatrix::create(m_matrix.inverse());
}

PassRefPtr<WebKitCSSMatrix> WebKitCSSMatrix::translate(double x, double y, double z) const
{
    if (isnan(x))
        x = 0;
    if (isnan(y))
        y = 0;
    if (isnan(z))
        z = 0;
    return WebKitCSSMatrix::create(TransformationMatrix(m_matrix).translate3d(x, y, z));
}

PassRefPtr<WebKitCSSMatrix> WebKitCSSMatrix::scale(double scaleX, double scaleY, double scaleZ) const
{
    if (isnan(scaleX))
        scaleX = 1;
    if (isnan(scaleY))
        scaleY = scaleX;
    if (isnan(scaleZ))
        scaleZ = 1;
    return WebKitCSSMatrix::create(TransformationMatrix(m_matrix).scale3d(scaleX, scaleY, scaleZ));
}

PassRefPtr<WebKitCSSMatrix> WebKitCSSMatrix::rotate(double rotX, double rotY, double rotZ) const
{
    if (isnan(rotX))
        rotX = 0;
        
    if (isnan(rotY) && isnan(rotZ)) {
        rotZ = rotX;
        rotX = 0;
        rotY = 0;
    }

    if (isnan(rotY))
        rotY = 0;
    if (isnan(rotZ))
        rotZ = 0;
    return WebKitCSSMatrix::create(TransformationMatrix(m_matrix).rotate3d(rotX, rotY, rotZ));
}

PassRefPtr<WebKitCSSMatrix> WebKitCSSMatrix::rotateAxisAngle(double x, double y, double z, double angle) const
{
    if (isnan(x))
        x = 0;
    if (isnan(y))
        y = 0;
    if (isnan(z))
        z = 0;
    if (isnan(angle))
        angle = 0;
    if (x == 0 && y == 0 && z == 0)
        z = 1;
    return WebKitCSSMatrix::create(TransformationMatrix(m_matrix).rotate3d(x, y, z, angle));
}

PassRefPtr<WebKitCSSMatrix> WebKitCSSMatrix::skewX(double angle) const
{
    if (isnan(angle))
        angle = 0;
    return WebKitCSSMatrix::create(TransformationMatrix(m_matrix).skewX(angle));
}

PassRefPtr<WebKitCSSMatrix> WebKitCSSMatrix::skewY(double angle) const
{
    if (isnan(angle))
        angle = 0;
    return WebKitCSSMatrix::create(TransformationMatrix(m_matrix).skewY(angle));
}

String WebKitCSSMatrix::toString() const
{
    // FIXME - Need to ensure valid CSS floating point values (https://bugs.webkit.org/show_bug.cgi?id=20674)
    if (m_matrix.isAffine())
        return String::format("matrix(%f, %f, %f, %f, %f, %f)",
                                m_matrix.a(), m_matrix.b(), m_matrix.c(), m_matrix.d(), m_matrix.e(), m_matrix.f());
    return String::format("matrix3d(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)",
                            m_matrix.m11(), m_matrix.m12(), m_matrix.m13(), m_matrix.m14(),
                            m_matrix.m21(), m_matrix.m22(), m_matrix.m23(), m_matrix.m24(),
                            m_matrix.m31(), m_matrix.m32(), m_matrix.m33(), m_matrix.m34(),
                            m_matrix.m41(), m_matrix.m42(), m_matrix.m43(), m_matrix.m44());
}

} // namespace WebCore
