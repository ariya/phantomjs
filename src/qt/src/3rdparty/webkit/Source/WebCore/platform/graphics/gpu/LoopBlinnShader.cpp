/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "LoopBlinnShader.h"

#include "GraphicsContext3D.h"

namespace WebCore {

LoopBlinnShader::LoopBlinnShader(GraphicsContext3D* context, unsigned program)
    : Shader(context, program)
{
    m_worldViewProjectionLocation = context->getUniformLocation(program, "worldViewProjection");
    m_positionLocation = context->getAttribLocation(program, "position");
    m_klmLocation = context->getAttribLocation(program, "klm");
}

void LoopBlinnShader::use(unsigned vertexOffset, unsigned klmOffset, const AffineTransform& transform)
{
    m_context->useProgram(m_program);

    float matrix[16];
    affineTo4x4(transform, matrix);
    m_context->uniformMatrix4fv(m_worldViewProjectionLocation, false /*transpose*/, matrix, 1 /*count*/);

    m_context->vertexAttribPointer(m_positionLocation, 2, GraphicsContext3D::FLOAT, false, 0, vertexOffset);
    m_context->enableVertexAttribArray(m_positionLocation);

    if (m_klmLocation != -1) {
        m_context->vertexAttribPointer(m_klmLocation, 3, GraphicsContext3D::FLOAT, false, 0, klmOffset);
        m_context->enableVertexAttribArray(m_klmLocation);
    }
}

} // namespace WebCore
