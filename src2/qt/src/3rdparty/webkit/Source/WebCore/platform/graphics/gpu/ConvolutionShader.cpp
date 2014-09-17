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

#include "ConvolutionShader.h"

#include "GraphicsContext3D.h"
#include "StringExtras.h"

namespace WebCore {

ConvolutionShader::ConvolutionShader(GraphicsContext3D* context, unsigned program, int kernelWidth)
    : Shader(context, program)
    , m_kernelWidth(kernelWidth)
    , m_matrixLocation(context->getUniformLocation(program, "matrix"))
    , m_texMatrixLocation(context->getUniformLocation(program, "texMatrix"))
    , m_kernelLocation(context->getUniformLocation(program, "kernel"))
    , m_imageLocation(context->getUniformLocation(program, "image"))
    , m_imageIncrementLocation(context->getUniformLocation(program, "imageIncrement"))
    , m_positionLocation(context->getAttribLocation(program, "position"))
{
}

PassOwnPtr<ConvolutionShader> ConvolutionShader::create(GraphicsContext3D* context, int kernelWidth)
{
    static const char* vertexShaderRaw =
        "#define KERNEL_WIDTH %d\n"
        "uniform mat3 matrix;\n"
        "uniform mat3 texMatrix;\n"
        "uniform vec2 imageIncrement;\n"
        "attribute vec2 position;\n"
        "varying vec2 imageCoord;\n"
        "void main() {\n"
        "    vec3 pos = vec3(position, 1.0);\n"
        "    // Offset image coords by half of kernel width, in image texels\n"
        "    gl_Position = vec4(matrix * pos, 1.0);\n"
        "    float scale = (float(KERNEL_WIDTH) - 1.0) / 2.0;\n"
        "    imageCoord = (texMatrix * pos).xy - vec2(scale, scale) * imageIncrement;\n"
        "}\n";
    char vertexShaderSource[1024];
    snprintf(vertexShaderSource, sizeof(vertexShaderSource), vertexShaderRaw, kernelWidth);
    static const char* fragmentShaderRaw =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "#define KERNEL_WIDTH %d\n"
        "uniform sampler2D image;\n"
        "uniform float kernel[KERNEL_WIDTH];\n"
        "uniform vec2 imageIncrement;\n"
        "varying vec2 imageCoord;\n"
        "void main() {\n"
        "    vec2 coord = imageCoord;\n"
        "    vec4 sum = vec4(0, 0, 0, 0);\n"
        "    for (int i = 0; i < KERNEL_WIDTH; i++) {\n"
        "      sum += texture2D(image, coord) * kernel[i];\n"
        "      coord += imageIncrement;\n"
        "    }\n"
        "    gl_FragColor = sum;\n"
        "}\n";
    char fragmentShaderSource[1024];
    snprintf(fragmentShaderSource, sizeof(fragmentShaderSource), fragmentShaderRaw, kernelWidth);

    unsigned program = loadProgram(context, vertexShaderSource, fragmentShaderSource);
    if (!program)
        return nullptr;
    return new ConvolutionShader(context, program, kernelWidth);
}

void ConvolutionShader::use(const AffineTransform& transform, const AffineTransform& texTransform, const float* kernel, int kernelWidth, float imageIncrement[2])
{
    m_context->useProgram(m_program);
    float matrix[9];
    affineTo3x3(transform, matrix);
    m_context->uniformMatrix3fv(m_matrixLocation, false /*transpose*/, matrix, 1 /*count*/);

    float texMatrix[9];
    affineTo3x3(texTransform, texMatrix);
    m_context->uniformMatrix3fv(m_texMatrixLocation, false /*transpose*/, texMatrix, 1 /*count*/);

    m_context->uniform2f(m_imageIncrementLocation, imageIncrement[0], imageIncrement[1]);

    // For now, we always use texture unit 0. If that ever changes, we should
    // expose this parameter to the caller.
    m_context->uniform1i(m_imageLocation, 0);
    if (kernelWidth > m_kernelWidth)
        kernelWidth = m_kernelWidth;
    m_context->uniform1fv(m_kernelLocation, const_cast<float*>(kernel), kernelWidth);

    m_context->vertexAttribPointer(m_positionLocation, 2, GraphicsContext3D::FLOAT, false, 0, 0);

    m_context->enableVertexAttribArray(m_positionLocation);
}

}

#endif
