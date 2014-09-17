/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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
#include "WebKitCSSTransformValue.h"

#include "CSSValueList.h"
#include "PlatformString.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

WebKitCSSTransformValue::WebKitCSSTransformValue(TransformOperationType op)
    : CSSValueList(false)
    , m_type(op)
{
}

WebKitCSSTransformValue::~WebKitCSSTransformValue()
{
}

String WebKitCSSTransformValue::cssText() const
{
    String result;
    switch (m_type) {
        case TranslateTransformOperation:
            result += "translate(";
            break;
        case TranslateXTransformOperation:
            result += "translateX(";
            break;
        case TranslateYTransformOperation:
            result += "translateY(";
            break;
        case RotateTransformOperation:
            result += "rotate(";
            break;
        case ScaleTransformOperation:
            result += "scale(";
            break;
        case ScaleXTransformOperation:
            result += "scaleX(";
            break;
        case ScaleYTransformOperation:
            result += "scaleY(";
            break;
        case SkewTransformOperation:
            result += "skew(";
            break;
        case SkewXTransformOperation:
            result += "skewX(";
            break;
        case SkewYTransformOperation:
            result += "skewY(";
            break;
        case MatrixTransformOperation:
            result += "matrix(";
            break;
        case TranslateZTransformOperation:
            result += "translateZ(";
            break;
        case Translate3DTransformOperation:
            result += "translate3d(";
            break;
        case RotateXTransformOperation:
            result += "rotateX(";
            break;
        case RotateYTransformOperation:
            result += "rotateY(";
            break;
        case RotateZTransformOperation:
            result += "rotateZ(";
            break;
        case Rotate3DTransformOperation:
            result += "rotate3d(";
            break;
        case ScaleZTransformOperation:
            result += "scaleZ(";
            break;
        case Scale3DTransformOperation:
            result += "scale3d(";
            break;
        case PerspectiveTransformOperation:
            result += "perspective(";
            break;
        case Matrix3DTransformOperation:
            result += "matrix3d(";
            break;
        default:
            break;
    }
    
    result += CSSValueList::cssText();
    
    result += ")";
    return result;
}

}
