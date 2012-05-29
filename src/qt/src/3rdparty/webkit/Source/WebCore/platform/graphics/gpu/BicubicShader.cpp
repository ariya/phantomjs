/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(ACCELERATED_2D_CANVAS)

#include "BicubicShader.h"

#include "GraphicsContext3D.h"

namespace WebCore {

BicubicShader::BicubicShader(GraphicsContext3D* context, unsigned program)
    : Shader(context, program)
    , m_matrixLocation(context->getUniformLocation(program, "matrix"))
    , m_texMatrixLocation(context->getUniformLocation(program, "texMatrix"))
    , m_coefficientsLocation(context->getUniformLocation(program, "coefficients"))
    , m_imageIncrementLocation(context->getUniformLocation(program, "imageIncrement"))
    , m_imageLocation(context->getUniformLocation(program, "image"))
    , m_alphaLocation(context->getUniformLocation(program, "alpha"))
    , m_positionLocation(context->getAttribLocation(program, "position"))
{
}

PassOwnPtr<BicubicShader> BicubicShader::create(GraphicsContext3D* context)
{
    static const char* vertexShaderSource =
        "uniform mat3 matrix;\n"
        "uniform mat3 texMatrix;\n"
        "attribute vec2 position;\n"
        "varying vec2 texCoord;\n"
        "void main() {\n"
        "    vec3 pos = vec3(position, 1.0);\n"
        "    texCoord = (texMatrix * pos).xy;\n"
        "    gl_Position = vec4(matrix * pos, 1.0);\n"
        "}\n";
    static const char* fragmentShaderSource =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "uniform sampler2D image;\n"
        "uniform vec2 imageIncrement;\n"
        "uniform mat4 coefficients;\n"
        "uniform float alpha;\n"
        "varying vec2 texCoord;\n"
        "vec4 cubicBlend(float t, vec4 c0, vec4 c1, vec4 c2, vec4 c3) {\n"
        "    vec4 ts = vec4(1.0, t, t * t, t * t * t);\n"
        "    vec4 result = coefficients * ts;\n"
        "    return result.w * c0 + result.z * c1 + result.y * c2 + result.x * c3;\n"
        "}\n"
        "void main() {\n"
        "    vec2 imageCoord = texCoord;\n"
        "    vec2 f = fract(imageCoord / imageIncrement) - vec2(0.5, 0.5);\n"
        "    vec4 t00 = texture2D(image, imageCoord + imageIncrement * vec2(-1, -1));\n"
        "    vec4 t10 = texture2D(image, imageCoord + imageIncrement * vec2( 0, -1));\n"
        "    vec4 t20 = texture2D(image, imageCoord + imageIncrement * vec2( 1, -1));\n"
        "    vec4 t30 = texture2D(image, imageCoord + imageIncrement * vec2( 2, -1));\n"
        "    vec4 t0 = cubicBlend(f.x, t00, t10, t20, t30);\n"
        "    vec4 t01 = texture2D(image, imageCoord + imageIncrement * vec2(-1, 0));\n"
        "    vec4 t11 = texture2D(image, imageCoord + imageIncrement * vec2( 0, 0));\n"
        "    vec4 t21 = texture2D(image, imageCoord + imageIncrement * vec2( 1, 0));\n"
        "    vec4 t31 = texture2D(image, imageCoord + imageIncrement * vec2( 2, 0));\n"
        "    vec4 t1 = cubicBlend(f.x, t01, t11, t21, t31);\n"
        "    vec4 t02 = texture2D(image, imageCoord + imageIncrement * vec2(-1, 1));\n"
        "    vec4 t12 = texture2D(image, imageCoord + imageIncrement * vec2( 0, 1));\n"
        "    vec4 t22 = texture2D(image, imageCoord + imageIncrement * vec2( 1, 1));\n"
        "    vec4 t32 = texture2D(image, imageCoord + imageIncrement * vec2( 2, 1));\n"
        "    vec4 t2 = cubicBlend(f.x, t02, t12, t22, t32);\n"
        "    vec4 t03 = texture2D(image, imageCoord + imageIncrement * vec2(-1, 2));\n"
        "    vec4 t13 = texture2D(image, imageCoord + imageIncrement * vec2( 0, 2));\n"
        "    vec4 t23 = texture2D(image, imageCoord + imageIncrement * vec2( 1, 2));\n"
        "    vec4 t33 = texture2D(image, imageCoord + imageIncrement * vec2( 2, 2));\n"
        "    vec4 t3 = cubicBlend(f.x, t03, t13, t23, t33);\n"
        "    gl_FragColor = cubicBlend(f.y, t0, t1, t2, t3);\n"
        "}\n";

    unsigned program = loadProgram(context, vertexShaderSource, fragmentShaderSource);
    if (!program)
        return nullptr;

    return new BicubicShader(context, program);
}

void BicubicShader::use(const AffineTransform& transform, const AffineTransform& texTransform, const float coefficients[16], const float imageIncrement[2], float alpha)
{
    m_context->useProgram(m_program);
    float matrix[9];
    affineTo3x3(transform, matrix);
    m_context->uniformMatrix3fv(m_matrixLocation, false /*transpose*/, matrix, 1 /*count*/);

    float texMatrix[9];
    affineTo3x3(texTransform, texMatrix);
    m_context->uniformMatrix3fv(m_texMatrixLocation, false /*transpose*/, texMatrix, 1 /*count*/);
    m_context->uniformMatrix4fv(m_coefficientsLocation, false /*transpose*/, const_cast<float *>(coefficients), 1 /*count*/);

    m_context->uniform2f(m_imageIncrementLocation, imageIncrement[0], imageIncrement[1]);

    // For now, we always use texture unit 0. If that ever changes, we should
    // expose this parameter to the caller.
    m_context->uniform1i(m_imageLocation, 0);
    m_context->uniform1f(m_alphaLocation, alpha);

    m_context->vertexAttribPointer(m_positionLocation, 2, GraphicsContext3D::FLOAT, false, 0, 0);

    m_context->enableVertexAttribArray(m_positionLocation);
}

}

#endif
