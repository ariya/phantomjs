/*
 * Copyright (c) 2010, Google Inc. All rights reserved.
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

#include "TexShader.h"

#include "GraphicsContext3D.h"

namespace WebCore {

TexShader::TexShader(GraphicsContext3D* context, unsigned program)
    : Shader(context, program)
{
    m_matrixLocation = context->getUniformLocation(program, "matrix");
    m_texMatrixLocation = context->getUniformLocation(program, "texMatrix");
    m_alphaLocation = context->getUniformLocation(program, "globalAlpha");
    m_positionLocation = context->getAttribLocation(program, "position");
    m_samplerLocation = context->getUniformLocation(program, "sampler");
}

PassOwnPtr<TexShader> TexShader::create(GraphicsContext3D* context)
{
    unsigned program = loadProgram(context,
                                   generateVertex(Shader::TwoDimensional, Shader::TextureFill),
                                   generateFragment(Shader::TwoDimensional, Shader::TextureFill, Shader::NotAntialiased));
    if (!program)
        return nullptr;
    return new TexShader(context, program);
}

void TexShader::use(const AffineTransform& transform, const AffineTransform& texTransform, int sampler, float alpha)
{
    m_context->useProgram(m_program);
    float matrix[9];
    affineTo3x3(transform, matrix);
    m_context->uniformMatrix3fv(m_matrixLocation, false /*transpose*/, matrix, 1 /*count*/);

    float texMatrix[9];
    affineTo3x3(texTransform, texMatrix);
    m_context->uniformMatrix3fv(m_texMatrixLocation, false /*transpose*/, texMatrix, 1 /*count*/);

    m_context->uniform1i(m_samplerLocation, sampler);
    m_context->uniform1f(m_alphaLocation, alpha);

    m_context->vertexAttribPointer(m_positionLocation, 2, GraphicsContext3D::FLOAT, false, 0, 0);

    m_context->enableVertexAttribArray(m_positionLocation);

}

}

#endif
