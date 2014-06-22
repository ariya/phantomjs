/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
 * Copyright (C) 2012 Company 100, Inc. All rights reserved.
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

#if ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)
#include "CustomFilterRenderer.h"

#include "CustomFilterArrayParameter.h"
#include "CustomFilterColorParameter.h"
#include "CustomFilterCompiledProgram.h"
#include "CustomFilterConstants.h"
#include "CustomFilterMesh.h"
#include "CustomFilterNumberParameter.h"
#include "CustomFilterParameter.h"
#include "CustomFilterTransformParameter.h"
#include "GraphicsContext3D.h"
#include "TransformationMatrix.h"

namespace WebCore {

static void orthogonalProjectionMatrix(TransformationMatrix& matrix, float left, float right, float bottom, float top)
{
    ASSERT(matrix.isIdentity());

    float deltaX = right - left;
    float deltaY = top - bottom;
    if (!deltaX || !deltaY)
        return;
    matrix.setM11(2.0f / deltaX);
    matrix.setM41(-(right + left) / deltaX);
    matrix.setM22(2.0f / deltaY);
    matrix.setM42(-(top + bottom) / deltaY);

    // Use big enough near/far values, so that simple rotations of rather large objects will not
    // get clipped. 10000 should cover most of the screen resolutions.
    const float farValue = 10000;
    const float nearValue = -10000;
    matrix.setM33(-2.0f / (farValue - nearValue));
    matrix.setM43(- (farValue + nearValue) / (farValue - nearValue));
    matrix.setM44(1.0f);
}

PassRefPtr<CustomFilterRenderer> CustomFilterRenderer::create(PassRefPtr<GraphicsContext3D> context, CustomFilterProgramType programType, const CustomFilterParameterList& parameters,
    unsigned meshRows, unsigned meshColumns, CustomFilterMeshType meshType)
{
    return adoptRef(new CustomFilterRenderer(context, programType, parameters, meshRows, meshColumns, meshType));
}

CustomFilterRenderer::CustomFilterRenderer(PassRefPtr<GraphicsContext3D> context, CustomFilterProgramType programType, const CustomFilterParameterList& parameters,
    unsigned meshRows, unsigned meshColumns, CustomFilterMeshType meshType)
    : m_context(context)
    , m_programType(programType)
    , m_parameters(parameters)
    , m_meshRows(meshRows)
    , m_meshColumns(meshColumns)
    , m_meshType(meshType)
{
}

CustomFilterRenderer::~CustomFilterRenderer()
{
}

bool CustomFilterRenderer::premultipliedAlpha() const
{
    return m_programType == PROGRAM_TYPE_BLENDS_ELEMENT_TEXTURE;
}

bool CustomFilterRenderer::programNeedsInputTexture() const
{
    ASSERT(m_compiledProgram.get());
    return m_compiledProgram->samplerLocation() != -1;
}

void CustomFilterRenderer::draw(Platform3DObject inputTexture, const IntSize& size)
{
    // FIXME: We would need something like CustomFilterRendererState that will contain the size and other parameters in the future. We should pass that to bindProgramBuffers instead of storing it.
    // https://bugs.webkit.org/show_bug.cgi?id=100107
    m_contextSize = size;

    bindProgramAndBuffers(inputTexture);
    m_context->drawElements(GraphicsContext3D::TRIANGLES, m_mesh->indicesCount(), GraphicsContext3D::UNSIGNED_SHORT, 0);
    unbindVertexAttributes();
}

void CustomFilterRenderer::setCompiledProgram(PassRefPtr<CustomFilterCompiledProgram> compiledProgram)
{
    m_compiledProgram = compiledProgram;
}

bool CustomFilterRenderer::prepareForDrawing()
{
    m_context->makeContextCurrent();
    if (!m_compiledProgram || !m_compiledProgram->isInitialized())
        return false;
    initializeMeshIfNeeded();
    return true;
}

void CustomFilterRenderer::initializeMeshIfNeeded()
{
    if (m_mesh.get())
        return;

    // FIXME: Sharing the mesh would just save the time needed to upload it to the GPU, so I assume we could
    // benchmark that for performance.
    // https://bugs.webkit.org/show_bug.cgi?id=88429
    m_mesh = CustomFilterMesh::create(m_context.get(), m_meshColumns, m_meshRows, FloatRect(0, 0, 1, 1), m_meshType);
}

void CustomFilterRenderer::bindVertexAttribute(int attributeLocation, unsigned size, unsigned offset)
{
    if (attributeLocation != -1) {
        m_context->vertexAttribPointer(attributeLocation, size, GraphicsContext3D::FLOAT, false, m_mesh->bytesPerVertex(), offset);
        m_context->enableVertexAttribArray(attributeLocation);
    }
}

void CustomFilterRenderer::unbindVertexAttribute(int attributeLocation)
{
    if (attributeLocation != -1)
        m_context->disableVertexAttribArray(attributeLocation);
}

void CustomFilterRenderer::bindProgramArrayParameters(int uniformLocation, CustomFilterArrayParameter* arrayParameter)
{
    unsigned parameterSize = arrayParameter->size();
    Vector<GC3Dfloat> floatVector;

    for (unsigned i = 0; i < parameterSize; ++i)
        floatVector.append(arrayParameter->valueAt(i));

    m_context->uniform1fv(uniformLocation, parameterSize, floatVector.data());
}

void CustomFilterRenderer::bindProgramColorParameters(int uniformLocation, CustomFilterColorParameter* colorParameter)
{
    float r, g, b, a;
    colorParameter->color().getRGBA(r, g, b, a);

    m_context->uniform4f(uniformLocation, r, g, b, a);
}

void CustomFilterRenderer::bindProgramMatrixParameters(int uniformLocation, CustomFilterArrayParameter* matrixParameter)
{
    unsigned parameterSize = matrixParameter->size();
    Vector<GC3Dfloat, 16> floatVector;

    for (unsigned i = 0; i < parameterSize; ++i)
        floatVector.append(matrixParameter->valueAt(i));

    switch (parameterSize) {
    case 4:
        m_context->uniformMatrix2fv(uniformLocation, 1, false, floatVector.data());
        break;
    case 9:
        m_context->uniformMatrix3fv(uniformLocation, 1, false, floatVector.data());
        break;
    case 16:
        m_context->uniformMatrix4fv(uniformLocation, 1, false, floatVector.data());
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

void CustomFilterRenderer::bindProgramNumberParameters(int uniformLocation, CustomFilterNumberParameter* numberParameter)
{
    switch (numberParameter->size()) {
    case 1:
        m_context->uniform1f(uniformLocation, numberParameter->valueAt(0));
        break;
    case 2:
        m_context->uniform2f(uniformLocation, numberParameter->valueAt(0), numberParameter->valueAt(1));
        break;
    case 3:
        m_context->uniform3f(uniformLocation, numberParameter->valueAt(0), numberParameter->valueAt(1), numberParameter->valueAt(2));
        break;
    case 4:
        m_context->uniform4f(uniformLocation, numberParameter->valueAt(0), numberParameter->valueAt(1), numberParameter->valueAt(2), numberParameter->valueAt(3));
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

void CustomFilterRenderer::bindProgramTransformParameter(int uniformLocation, CustomFilterTransformParameter* transformParameter)
{
    TransformationMatrix matrix;
    if (m_contextSize.width() && m_contextSize.height()) {
        // The viewport is a box with the size of 1 unit, so we are scaling up here to make sure that translations happen using real pixel
        // units. At the end we scale back down in order to map it back to the original box. Note that transforms come in reverse order, because it is
        // supposed to multiply to the left of the coordinates of the vertices.
        // Note that the origin (0, 0) of the viewport is in the middle of the context, so there's no need to change the origin of the transform
        // in order to rotate around the middle of mesh.
        matrix.scale3d(1.0 / m_contextSize.width(), 1.0 / m_contextSize.height(), 1);
        transformParameter->applyTransform(matrix, m_contextSize);
        matrix.scale3d(m_contextSize.width(), m_contextSize.height(), 1);
    }
    float glMatrix[16];
    matrix.toColumnMajorFloatArray(glMatrix);
    m_context->uniformMatrix4fv(uniformLocation, 1, false, &glMatrix[0]);
}

void CustomFilterRenderer::bindProgramParameters()
{
    // FIXME: Find a way to reset uniforms that are not specified in CSS. This is needed to avoid using values
    // set by other previous rendered filters.
    // https://bugs.webkit.org/show_bug.cgi?id=76440

    size_t parametersSize = m_parameters.size();
    for (size_t i = 0; i < parametersSize; ++i) {
        CustomFilterParameter* parameter = m_parameters.at(i).get();
        int uniformLocation = m_compiledProgram->uniformLocationByName(parameter->name());
        if (uniformLocation == -1)
            continue;
        switch (parameter->parameterType()) {
        case CustomFilterParameter::ARRAY:
            bindProgramArrayParameters(uniformLocation, static_cast<CustomFilterArrayParameter*>(parameter));
            break;
        case CustomFilterParameter::COLOR:        
            bindProgramColorParameters(uniformLocation, static_cast<CustomFilterColorParameter*>(parameter));
            break;
        case CustomFilterParameter::MATRIX:
            bindProgramMatrixParameters(uniformLocation, static_cast<CustomFilterArrayParameter*>(parameter));
            break;
        case CustomFilterParameter::NUMBER:
            bindProgramNumberParameters(uniformLocation, static_cast<CustomFilterNumberParameter*>(parameter));
            break;
        case CustomFilterParameter::TRANSFORM:
            bindProgramTransformParameter(uniformLocation, static_cast<CustomFilterTransformParameter*>(parameter));
            break;
        }
    }
}

void CustomFilterRenderer::bindProgramAndBuffers(Platform3DObject inputTexture)
{
    ASSERT(m_compiledProgram->isInitialized());

    m_context->useProgram(m_compiledProgram->program());

    if (programNeedsInputTexture()) {
        // We should be binding the DOM element texture sampler only if the author is using the CSS mix function.
        ASSERT(m_programType == PROGRAM_TYPE_BLENDS_ELEMENT_TEXTURE);
        ASSERT(m_compiledProgram->samplerLocation() != -1);

        m_context->activeTexture(GraphicsContext3D::TEXTURE0);
        m_context->uniform1i(m_compiledProgram->samplerLocation(), 0);
        m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, inputTexture);
        m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR);
        m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR);
        m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
        m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);
    }

    if (m_compiledProgram->projectionMatrixLocation() != -1) {
        TransformationMatrix projectionMatrix;
        orthogonalProjectionMatrix(projectionMatrix, -0.5, 0.5, -0.5, 0.5);
        float glProjectionMatrix[16];
        projectionMatrix.toColumnMajorFloatArray(glProjectionMatrix);
        m_context->uniformMatrix4fv(m_compiledProgram->projectionMatrixLocation(), 1, false, &glProjectionMatrix[0]);
    }

    ASSERT(m_meshColumns);
    ASSERT(m_meshRows);

    if (m_compiledProgram->meshSizeLocation() != -1)
        m_context->uniform2f(m_compiledProgram->meshSizeLocation(), m_meshColumns, m_meshRows);

    if (m_compiledProgram->tileSizeLocation() != -1)
        m_context->uniform2f(m_compiledProgram->tileSizeLocation(), 1.0 / m_meshColumns, 1.0 / m_meshRows);

    if (m_compiledProgram->meshBoxLocation() != -1) {
        // FIXME: This will change when filter margins will be implemented,
        // see https://bugs.webkit.org/show_bug.cgi?id=71400
        m_context->uniform4f(m_compiledProgram->meshBoxLocation(), -0.5, -0.5, 1.0, 1.0);
    }

    if (m_compiledProgram->samplerSizeLocation() != -1)
        m_context->uniform2f(m_compiledProgram->samplerSizeLocation(), m_contextSize.width(), m_contextSize.height());

    m_context->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, m_mesh->verticesBufferObject());
    m_context->bindBuffer(GraphicsContext3D::ELEMENT_ARRAY_BUFFER, m_mesh->elementsBufferObject());

    bindVertexAttribute(m_compiledProgram->positionAttribLocation(), PositionAttribSize, PositionAttribOffset);
    bindVertexAttribute(m_compiledProgram->texAttribLocation(), TexAttribSize, TexAttribOffset);
    bindVertexAttribute(m_compiledProgram->meshAttribLocation(), MeshAttribSize, MeshAttribOffset);
    if (m_meshType == MeshTypeDetached)
        bindVertexAttribute(m_compiledProgram->triangleAttribLocation(), TriangleAttribSize, TriangleAttribOffset);

    bindProgramParameters();
}

void CustomFilterRenderer::unbindVertexAttributes()
{
    unbindVertexAttribute(m_compiledProgram->positionAttribLocation());
    unbindVertexAttribute(m_compiledProgram->texAttribLocation());
    unbindVertexAttribute(m_compiledProgram->meshAttribLocation());
    if (m_meshType == MeshTypeDetached)
        unbindVertexAttribute(m_compiledProgram->triangleAttribLocation());
}

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)
