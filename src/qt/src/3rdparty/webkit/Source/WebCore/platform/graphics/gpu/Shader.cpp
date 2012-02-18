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

#include "Shader.h"

#include "AffineTransform.h"
#include "GraphicsContext3D.h"

#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

// static
void Shader::affineTo3x3(const AffineTransform& transform, float mat[9])
{
    mat[0] = transform.a();
    mat[1] = transform.b();
    mat[2] = 0.0f;
    mat[3] = transform.c();
    mat[4] = transform.d();
    mat[5] = 0.0f;
    mat[6] = transform.e();
    mat[7] = transform.f();
    mat[8] = 1.0f;
}

// static
void Shader::affineTo4x4(const AffineTransform& transform, float mat[16])
{
    mat[0] = transform.a();
    mat[1] = transform.b();
    mat[2] = 0.0f;
    mat[3] = 0.0f;
    mat[4] = transform.c();
    mat[5] = transform.d();
    mat[6] = 0.0f;
    mat[7] = 0.0f;
    mat[8] = 0.0f;
    mat[9] = 0.0f;
    mat[10] = 1.0f;
    mat[11] = 0.0f;
    mat[12] = transform.e();
    mat[13] = transform.f();
    mat[14] = 0.0f;
    mat[15] = 1.0f;
}

// static
unsigned Shader::loadShader(GraphicsContext3D* context, unsigned type, const String& shaderSource)
{
    unsigned shader = context->createShader(type);
    if (!shader)
        return 0;

    context->shaderSource(shader, shaderSource);
    context->compileShader(shader);
    int compileStatus = 0;
    context->getShaderiv(shader, GraphicsContext3D::COMPILE_STATUS, &compileStatus);
    if (!compileStatus) {
        String infoLog = context->getShaderInfoLog(shader);
        LOG_ERROR("%s", infoLog.utf8().data());
        context->deleteShader(shader);
        return 0;
    }
    return shader;
}

// static
unsigned Shader::loadProgram(GraphicsContext3D* context, const String& vertexShaderSource, const String& fragmentShaderSource)
{
    unsigned vertexShader = loadShader(context, GraphicsContext3D::VERTEX_SHADER, vertexShaderSource);
    if (!vertexShader)
        return 0;
    unsigned fragmentShader = loadShader(context, GraphicsContext3D::FRAGMENT_SHADER, fragmentShaderSource);
    if (!fragmentShader)
        return 0;
    unsigned program = context->createProgram();
    if (!program)
        return 0;
    context->attachShader(program, vertexShader);
    context->attachShader(program, fragmentShader);
    context->linkProgram(program);
    int linkStatus = 0;
    context->getProgramiv(program, GraphicsContext3D::LINK_STATUS, &linkStatus);
    if (!linkStatus)
        context->deleteProgram(program);
    context->deleteShader(vertexShader);
    context->deleteShader(fragmentShader);
    return program;
}

Shader::Shader(GraphicsContext3D* context, unsigned program)
    : m_context(context)
    , m_program(program)
{
}

Shader::~Shader()
{
    m_context->deleteProgram(m_program);
}

// static
String Shader::generateVertex(Shader::VertexType vertexType, Shader::FillType fillType)
{
    StringBuilder builder;
    switch (vertexType) {
    case TwoDimensional:
        builder.append(
            "uniform mat3 matrix;\n"
            "attribute vec2 position;\n");
        break;
    case LoopBlinnInterior:
        builder.append(
            "uniform mat4 worldViewProjection;\n"
            "attribute vec2 position;\n");
        break;
    case LoopBlinnExterior:
        builder.append(
            "uniform mat4 worldViewProjection;\n"
            "attribute vec2 position;\n"
            "attribute vec3 klm;\n"
            "varying vec3 v_klm;\n");
        break;
    }

    if (fillType == TextureFill) {
        builder.append(
            "uniform mat3 texMatrix;\n"
            "varying vec3 texCoord;\n");
    }

    builder.append(
        "void main() {\n");

    if (vertexType == TwoDimensional) {
        builder.append(
            "gl_Position = vec4(matrix * vec3(position, 1.0), 1.0);\n");
    } else {
        builder.append(
            "gl_Position = worldViewProjection * vec4(position, 0.0, 1.0);\n");
        if (vertexType == LoopBlinnExterior) {
            builder.append(
                "v_klm = klm;\n");
        }
    }

    if (fillType == TextureFill) {
        builder.append(
            "texCoord = texMatrix * vec3(position, 1.0);\n");
    }

    builder.append(
        "}\n");

    return builder.toString();
}

// static
String Shader::generateFragment(Shader::VertexType vertexType, Shader::FillType fillType, Shader::AntialiasType antialiasType)
{
    StringBuilder builder;
    builder.append(
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n");

    if (vertexType == LoopBlinnExterior) {
        if (antialiasType == Antialiased) {
            builder.append(
                "#extension GL_OES_standard_derivatives : enable\n");
        }
        builder.append(
            "varying vec3 v_klm;\n");
    }

    switch (fillType) {
    case SolidFill:
        builder.append(
            "uniform vec4 color;\n");
        break;
    case TextureFill:
        builder.append(
            "uniform sampler2D sampler;\n"
            "uniform float globalAlpha;\n"
            "varying vec3 texCoord;\n");
        break;
    }

    builder.append(
        "void main() {\n");

    if (vertexType != LoopBlinnExterior) {
        builder.append(
            "float alpha = 1.0;\n");
    } else {
        if (antialiasType == Antialiased) {
            builder.append(
                "  // Gradients\n"
                "  vec3 px = dFdx(v_klm);\n"
                "  vec3 py = dFdy(v_klm);\n"
                "\n"
                "  // Chain rule\n"
                "  float k2 = v_klm.x * v_klm.x;\n"
                "  float c = k2 * v_klm.x - v_klm.y * v_klm.z;\n"
                "  float k23 = 3.0 * k2;\n"
                "  float cx = k23 * px.x - v_klm.z * px.y - v_klm.y * px.z;\n"
                "  float cy = k23 * py.x - v_klm.z * py.y - v_klm.y * py.z;\n"
                "\n"
                "  // Signed distance\n"
                "  float sd = c / sqrt(cx * cx + cy * cy);\n"
                "\n"
                "  // Linear alpha\n"
                "  // FIXME: figure out why this needs to be\n"
                "  // negated compared to the HLSL version, and also why\n"
                "  // we need an adjustment by +1.0 for it to look good.\n"
                "  // float alpha = clamp(0.5 - sd, 0.0, 1.0);\n"
                "  float alpha = clamp(sd + 0.5, 0.0, 1.0);\n");
        } else {
            builder.append(
                "  float t = v_klm.x * v_klm.x * v_klm.x - v_klm.y * v_klm.z;\n"
                "  float alpha = clamp(sign(t), 0.0, 1.0);\n");
        }
    }

    switch (fillType) {
    case SolidFill:
        builder.append(
            "gl_FragColor = color * alpha;\n");
        break;
    case TextureFill:
        builder.append(
            "gl_FragColor = texture2D(sampler, texCoord.xy) * alpha * globalAlpha;\n");
        break;
    }

    builder.append(
        "}\n");

    return builder.toString();
}

} // namespace WebCore

#endif
