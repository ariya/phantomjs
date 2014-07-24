/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING) && ENABLE(CSS_FILTERS)

#include "LayerFilterRenderer.h"

#include "FilterOperation.h"
#include "LayerCompositingThread.h"
#include "LayerRenderer.h"
#include "NotImplemented.h"
#include "TextureCacheCompositingThread.h"

#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformLog.h>
#include <Vector.h>

#include <cstring>
#include <limits>
#include <wtf/text/WTFString.h>

namespace WebCore {

class SurfaceFunctor {
public:
    SurfaceFunctor(LayerRendererSurface* surface)
        : m_surface(surface)
    {
    }

protected:
    LayerRendererSurface* m_surface;
};

class InverseSurfaceWidth : public SurfaceFunctor {
public:
    InverseSurfaceWidth(LayerRendererSurface* surface) : SurfaceFunctor(surface) { }

    float operator() () { return 1.0f / m_surface->size().width(); }
};

class InverseSurfaceHeight : public SurfaceFunctor {
public:
    InverseSurfaceHeight(LayerRendererSurface* surface) : SurfaceFunctor(surface) { }

    float operator() () { return 1.0f / m_surface->size().height(); }
};

static int operationTypeToProgramID(const FilterOperation::OperationType& t)
{
    switch (t) {
    case FilterOperation::GRAYSCALE:
        return LayerData::CSSFilterShaderGrayscale;
    case FilterOperation::SEPIA:
        return LayerData::CSSFilterShaderSepia;
    case FilterOperation::SATURATE:
        return LayerData::CSSFilterShaderSaturate;
    case FilterOperation::HUE_ROTATE:
        return LayerData::CSSFilterShaderHueRotate;
    case FilterOperation::INVERT:
        return LayerData::CSSFilterShaderInvert;
    case FilterOperation::OPACITY:
        return LayerData::CSSFilterShaderOpacity;
    case FilterOperation::BRIGHTNESS:
        return LayerData::CSSFilterShaderBrightness;
    case FilterOperation::CONTRAST:
        return LayerData::CSSFilterShaderContrast;
    case FilterOperation::BLUR:
        return LayerData::CSSFilterShaderBlurY;
    case FilterOperation::DROP_SHADOW:
        return LayerData::CSSFilterShaderShadow;
#if ENABLE(CSS_SHADERS)
    case FilterOperation::CUSTOM:
        return LayerData::CSSFilterShaderCustom;
#endif
    default:
        ASSERT_NOT_REACHED();
        return -1;
    }
}

Uniform::Uniform(int c_location)
    : m_location(c_location)
{
}

void Uniform1f::apply()
{
    glUniform1f(location(), m_val);
}

PassRefPtr<Uniform> Uniform1f::create(int location, float val)
{
    return adoptRef(new Uniform1f(location, val));
}

Uniform1f::Uniform1f(int c_location, float c_val)
    : Uniform(c_location)
    , m_val(c_val)
{
}

void Uniform1i::apply()
{
    glUniform1i(location(), m_val);
}

PassRefPtr<Uniform> Uniform1i::create(int location, int val)
{
    return adoptRef(new Uniform1i(location, val));
}

Uniform1i::Uniform1i(int c_location, int c_val)
    : Uniform(c_location)
    , m_val(c_val)
{
}

void Uniform2f::apply()
{
    glUniform2f(location(), m_val[0], m_val[1]);
}

PassRefPtr<Uniform> Uniform2f::create(int location, float val0, float val1)
{
    return adoptRef(new Uniform2f(location, val0, val1));
}

Uniform2f::Uniform2f(int c_location, float c_val0, float c_val1)
    : Uniform(c_location)
{
    m_val[0] = c_val0;
    m_val[1] = c_val1;
}

void Uniform3f::apply()
{
    glUniform3f(location(), m_val[0], m_val[1], m_val[2]);
}

PassRefPtr<Uniform> Uniform3f::create(int location, float val0, float val1, float val2)
{
    return adoptRef(new Uniform3f(location, val0, val1, val2));
}

Uniform3f::Uniform3f(int c_location, float c_val0, float c_val1, float c_val2)
    : Uniform(c_location)
{
    m_val[0] = c_val0;
    m_val[1] = c_val1;
    m_val[2] = c_val2;
}

void Uniform4f::apply()
{
    glUniform4f(location(), m_val[0], m_val[1], m_val[2], m_val[3]);
}

PassRefPtr<Uniform> Uniform4f::create(int location, float val0, float val1, float val2, float val3)
{
    return adoptRef(new Uniform4f(location, val0, val1, val2, val3));
}

Uniform4f::Uniform4f(int c_location, float c_val0, float c_val1, float c_val2, float c_val3)
    : Uniform(c_location)
{
    m_val[0] = c_val0;
    m_val[1] = c_val1;
    m_val[2] = c_val2;
    m_val[3] = c_val3;
}

void Matrix4fv::apply()
{
    glUniformMatrix4fv(location(), m_size, m_transpose, m_array);
}

PassRefPtr<Parameter> Matrix4fv::create(GLint location, GLsizei size, GLboolean transpose, GLfloat* array)
{
    return adoptRef(new Matrix4fv(location, size, transpose, array));
}

Matrix4fv::Matrix4fv(GLint clocation, GLsizei size, GLboolean transpose, GLfloat* array)
    : Uniform(clocation)
    , m_size(size)
    , m_transpose(transpose)
    , m_array(0)
{
    m_array = new GLfloat[size * 4 * 4];
    std::memcpy(m_array, array, size * 4 * 4 * sizeof(GLfloat));
}

Matrix4fv::~Matrix4fv()
{
    delete[] m_array;
}

void Buffer::apply()
{
    glBindBuffer(m_buffer, m_object);
}

void Buffer::restoreState()
{
    glBindBuffer(m_buffer, 0);
}

PassRefPtr<Parameter> Buffer::create(GLenum buffer, GLuint object)
{
    return adoptRef(new Buffer(buffer, object));
}

Buffer::Buffer(GLenum buffer, GLuint object)
    : Parameter()
    , m_buffer(buffer)
    , m_object(object)
{
}

void VertexAttribf::apply()
{
    glVertexAttribPointer(m_location, m_size, GL_FLOAT, false, m_bytesPerVertex, reinterpret_cast<GLvoid*>(static_cast<intptr_t>(m_offset)));
    glEnableVertexAttribArray(m_location);
}

void VertexAttribf::restoreState()
{
    glDisableVertexAttribArray(m_location);
}

PassRefPtr<Parameter> VertexAttribf::create(int location, int size, int bytesPerVertex, int offset)
{
    return adoptRef(new VertexAttribf(location, size, bytesPerVertex, offset));
}


VertexAttribf::VertexAttribf(int location, int size, int bytesPerVertex, int offset)
    : Parameter()
    , m_location(location)
    , m_size(size)
    , m_bytesPerVertex(bytesPerVertex)
    , m_offset(offset)
{
}

PassRefPtr<LayerFilterRendererAction> LayerFilterRendererAction::create(int programId)
{
    return adoptRef(new LayerFilterRendererAction(programId));
}

LayerFilterRendererAction::LayerFilterRendererAction(int c_programId)
    : m_programId(c_programId)
    , m_pushSnapshot(false)
    , m_popSnapshot(false)
    , m_drawingMode(DrawTriangleFanArrays)
{
}

void LayerFilterRendererAction::useActionOn(LayerFilterRenderer* renderer)
{
    ASSERT(m_programId != -1);
    if (m_programId == -1) {
        glUseProgram(renderer->m_cssFilterProgramObject[LayerData::CSSFilterShaderPassthrough]);
        return;
    }
    glUseProgram(renderer->m_cssFilterProgramObject[m_programId]);
    for (unsigned i = 0; i < m_parameters.size(); ++i)
        m_parameters[i]->apply();
}

void LayerFilterRendererAction::restoreState()
{
    for (unsigned i = 0; i < m_parameters.size(); ++i)
        m_parameters[i]->restoreState();
}

PassOwnPtr<LayerFilterRenderer> LayerFilterRenderer::create(const int& positionLocation, const int& texCoordLocation)
{
    return adoptPtr(new LayerFilterRenderer(positionLocation, texCoordLocation));
}

LayerFilterRenderer::LayerFilterRenderer(const int& positionLocation, const int& texCoordLocation)
    : m_positionLocation(positionLocation)
    , m_texCoordLocation(texCoordLocation)
{
    // If you ever move this stuff, please make sure that
    // everything in this constructor is called BEFORE actionsForOperations.

    for (int i = 0; i < LayerData::NumberOfCSSFilterShaders; ++i)
        m_cssFilterProgramObject[i] = 0;

    if (!(m_enabled = initializeSharedGLObjects()))
        BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelWarn, "CSS Filters are not enabled due to failed initialization.");
}

// Binds the given attribute name to a common location across all programs
// used by the compositor. This allows the code to bind the attributes only once
// even when switching between programs.
//
// This is an extension of LayerRenderer::bindCommonAttribLocation and the locations
// will match those of LayerRenderer. See LayerFilterRenderer::LayerFilterRenderer()
void LayerFilterRenderer::bindCommonAttribLocation(int location, const char* attribName)
{
    for (int i = 0; i < LayerData::NumberOfCSSFilterShaders; ++i)
        glBindAttribLocation(m_cssFilterProgramObject[i], location, attribName);
}

bool LayerFilterRenderer::initializeSharedGLObjects()
{
    // See also TextureMapperShaderManager.cpp

    char standardVertexShaderString[] =
        "attribute vec4 a_position;   \n"
        "attribute vec2 a_texCoord;   \n"
        "varying vec2 v_texCoord;     \n"
        "void main()                  \n"
        "{                            \n"
        "  gl_Position = a_position;  \n"
        "  v_texCoord = a_texCoord;   \n"
        "}                            \n";

    char offsetVertexShaderString[] =
        "attribute vec4 a_position;              \n"
        "attribute vec2 a_texCoord;              \n"
        "uniform mediump vec2 u_offset;          \n"
        "varying vec2 v_texCoord;                \n"
        "void main()                             \n"
        "{                                       \n"
        "  gl_Position = a_position;             \n"
        "  v_texCoord = a_texCoord - u_offset;   \n"
        "}                                       \n";

#define STANDARD_FILTER(x...) \
        "precision mediump float; \n"\
        "\n"\
        "varying mediump vec2 v_texCoord;\n"\
        "uniform lowp sampler2D s_texture;\n"\
        "uniform highp float u_amount;\n"\
#x\
        "void main(void)\n { gl_FragColor = shade(texture2D(s_texture, v_texCoord)); }"

#define BLUR_FILTER(x...) \
        "precision highp float; \n"\
        "\n"\
        "varying mediump vec2 v_texCoord;\n"\
        "uniform lowp sampler2D s_texture;\n"\
        "uniform highp float u_amount;\n"\
        "uniform highp float u_blurSize;\n"\
        "const float pi = 3.1415927;\n"\
#x\
        "void main(void)\n"\
        "{\n"\
        "vec3 incr;\n"\
        "incr.x = 1.0 / (sqrt(2.0 * pi) * u_amount);\n"\
        "incr.y = exp(-0.5 / (u_amount * u_amount));\n"\
        "incr.z = incr.y * incr.y;\n"\
        "\n"\
        "vec4 avg = vec4(0.0, 0.0, 0.0, 0.0);\n"\
        "float coefficientSum = 0.0;\n"\
        "\n"\
        "avg += texture2D(s_texture, v_texCoord.xy) * incr.x;\n"\
        "coefficientSum += incr.x;\n"\
        "incr.xy *= incr.yz;\n"\
        "\n"\
        "for (float i = 1.0; i <= u_amount; i++) {\n"\
        "    avg += texture2D(s_texture, v_texCoord.xy - i * u_blurSize * blurMultiplyVec) * incr.x;\n"\
        "    avg += texture2D(s_texture, v_texCoord.xy + i * u_blurSize * blurMultiplyVec) * incr.x;\n"\
        "    coefficientSum += 2.0 * incr.x;\n"\
        "    incr.xy *= incr.yz;\n"\
        "}\n"\
        "\n"\
        "gl_FragColor = avg / coefficientSum;\n"\
        "}"

    const char* shaderStrs[LayerData::NumberOfCSSFilterShaders];

    shaderStrs[LayerData::CSSFilterShaderGrayscale] = STANDARD_FILTER(
        lowp vec4 shade(lowp vec4 color)
        {
            lowp float amount = 1.0 - u_amount;
            return vec4((0.2126 + 0.7874 * amount) * color.r + (0.7152 - 0.7152 * amount) * color.g + (0.0722 - 0.0722 * amount) * color.b,
                (0.2126 - 0.2126 * amount) * color.r + (0.7152 + 0.2848 * amount) * color.g + (0.0722 - 0.0722 * amount) * color.b,
                (0.2126 - 0.2126 * amount) * color.r + (0.7152 - 0.7152 * amount) * color.g + (0.0722 + 0.9278 * amount) * color.b,
                color.a);
        }
    );

    shaderStrs[LayerData::CSSFilterShaderSepia] = STANDARD_FILTER(
        lowp vec4 shade(lowp vec4 color)
        {
            lowp float amount = 1.0 - u_amount;
            return vec4((0.393 + 0.607 * amount) * color.r + (0.769 - 0.769 * amount) * color.g + (0.189 - 0.189 * amount) * color.b,
                (0.349 - 0.349 * amount) * color.r + (0.686 + 0.314 * amount) * color.g + (0.168 - 0.168 * amount) * color.b,
                (0.272 - 0.272 * amount) * color.r + (0.534 - 0.534 * amount) * color.g + (0.131 + 0.869 * amount) * color.b,
                color.a);
        }
    );

    shaderStrs[LayerData::CSSFilterShaderSaturate] = STANDARD_FILTER(
        lowp vec4 shade(lowp vec4 color)
        {
            return vec4((0.213 + 0.787 * u_amount) * color.r + (0.715 - 0.715 * u_amount) * color.g + (0.072 - 0.072 * u_amount) * color.b,
                (0.213 - 0.213 * u_amount) * color.r + (0.715 + 0.285 * u_amount) * color.g + (0.072 - 0.072 * u_amount) * color.b,
                (0.213 - 0.213 * u_amount) * color.r + (0.715 - 0.715 * u_amount) * color.g + (0.072 + 0.928 * u_amount) * color.b,
                color.a);
        }
    );

    shaderStrs[LayerData::CSSFilterShaderHueRotate] = STANDARD_FILTER(
        lowp vec4 shade(lowp vec4 color)
        {
            highp float pi = 3.14159265358979323846;
            highp float c = cos(u_amount * pi / 180.0);
            highp float s = sin(u_amount * pi / 180.0);
            return vec4(color.r * (0.213 + c * 0.787 - s * 0.213) + color.g * (0.715 - c * 0.715 - s * 0.715) + color.b * (0.072 - c * 0.072 + s * 0.928),
                color.r * (0.213 - c * 0.213 + s * 0.143) + color.g * (0.715 + c * 0.285 + s * 0.140) + color.b * (0.072 - c * 0.072 - s * 0.283),
                color.r * (0.213 - c * 0.213 - s * 0.787) +  color.g * (0.715 - c * 0.715 + s * 0.715) + color.b * (0.072 + c * 0.928 + s * 0.072),
                color.a);
        }
    );

    shaderStrs[LayerData::CSSFilterShaderInvert] = STANDARD_FILTER(
        lowp float invert(lowp float n) { return (1.0 - n) * u_amount + n * (1.0 - u_amount); }
        lowp vec4 shade(lowp vec4 color)
        {
            return vec4(invert(color.r), invert(color.g), invert(color.b), color.a);
        }
    );

    shaderStrs[LayerData::CSSFilterShaderBrightness] = STANDARD_FILTER(
        lowp vec4 shade(lowp vec4 color)
        {
            return vec4(color.rgb * (1.0 + u_amount), color.a);
        }
    );

    shaderStrs[LayerData::CSSFilterShaderContrast] = STANDARD_FILTER(
        lowp float contrast(lowp float n) { return (n - 0.5) * u_amount + 0.5; }
        lowp vec4 shade(lowp vec4 color)
        {
            return vec4(contrast(color.r), contrast(color.g), contrast(color.b), color.a);
        }
    );

    shaderStrs[LayerData::CSSFilterShaderOpacity] = STANDARD_FILTER(
        lowp vec4 shade(lowp vec4 color)
        {
            return vec4(color.r, color.g, color.b, color.a * u_amount);
        }
    );

    shaderStrs[LayerData::CSSFilterShaderBlurX] = BLUR_FILTER(
        const vec2  blurMultiplyVec      = vec2(1.0, 0.0);
    );

    shaderStrs[LayerData::CSSFilterShaderBlurY] = BLUR_FILTER(
        const vec2  blurMultiplyVec      = vec2(0.0, 1.0);
    );

    shaderStrs[LayerData::CSSFilterShaderShadow] = STANDARD_FILTER(
        uniform lowp vec3 u_color;
        lowp vec4 shade(lowp vec4 color)
        {
            if (color.a > 0.5)
                return vec4(u_color.r, u_color.g, u_color.b, color.a);
            return color;
        }
    );

    shaderStrs[LayerData::CSSFilterShaderPassthrough] = STANDARD_FILTER(
        lowp vec4 shade(lowp vec4 color)
        {
            return color;
        }
    );

    for (int i = 0; i < LayerData::NumberOfCSSFilterShaders; i++) {
        if (i == LayerData::CSSFilterShaderShadow)
            m_cssFilterProgramObject[i] = LayerRenderer::loadShaderProgram(offsetVertexShaderString, shaderStrs[i]);
        else
            m_cssFilterProgramObject[i] = LayerRenderer::loadShaderProgram(standardVertexShaderString, shaderStrs[i]);
        if (!m_cssFilterProgramObject[i]) {
            BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelWarn, "Could not load CSS Filter Shader %i", i);
            return false;
        }
    }

    // Set ATTRIB locations - these will be the same as the programs in LayerRenderer.cpp
    bindCommonAttribLocation(m_positionLocation, "a_position");
    bindCommonAttribLocation(m_texCoordLocation, "a_texCoord");

    // Re-link to take effect
    for (int i = 0; i < LayerData::NumberOfCSSFilterShaders; ++i)
        glLinkProgram(m_cssFilterProgramObject[i]);

    // Get UNIFORM locations
    for (int i = 0; i < LayerData::NumberOfCSSFilterShaders; ++i)
        m_amountLocation[i] = glGetUniformLocation(m_cssFilterProgramObject[i], "u_amount");

    m_blurAmountLocation[0] = glGetUniformLocation(m_cssFilterProgramObject[LayerData::CSSFilterShaderBlurY], "u_blurSize");
    m_blurAmountLocation[1] = glGetUniformLocation(m_cssFilterProgramObject[LayerData::CSSFilterShaderBlurX], "u_blurSize");

    m_shadowColorLocation = glGetUniformLocation(m_cssFilterProgramObject[LayerData::CSSFilterShaderShadow], "u_color");
    m_offsetLocation = glGetUniformLocation(m_cssFilterProgramObject[LayerData::CSSFilterShaderShadow], "u_offset");

    return true;
}

void LayerFilterRenderer::ping(LayerRendererSurface* surface)
{
    GLuint texid = m_texture->platformTexture();

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        texid,
        0
    );
    glBindTexture(
        GL_TEXTURE_2D,
        surface->texture()->platformTexture()
    );
}

void LayerFilterRenderer::pong(LayerRendererSurface* surface)
{
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        surface->texture()->platformTexture(),
        0
    );
    glBindTexture(
        GL_TEXTURE_2D,
        m_texture->platformTexture()
    );
}

void LayerFilterRenderer::pushSnapshot(LayerRendererSurface* surface, int sourceId)
{
    GLuint texid = m_snapshotTexture->platformTexture();

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        texid,
        0
    );

    glBindTexture(GL_TEXTURE_2D, sourceId);
    glClear(GL_COLOR_BUFFER_BIT); // to transparency

    glViewport(0, 0, surface->size().width(), surface->size().height());

    glUseProgram(m_cssFilterProgramObject[LayerData::CSSFilterShaderPassthrough]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void LayerFilterRenderer::popSnapshot()
{
    // The name is slightly misleading.
    // This DRAWS the previous texture using the current LayerFilterRendererAction, then sets the texture
    // to the snapshot texture. Next time glDrawArrays is called, the snapshot will be drawn.

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindTexture(
        GL_TEXTURE_2D,
        m_snapshotTexture->platformTexture()
    );
}

Vector<RefPtr<LayerFilterRendererAction> > LayerFilterRenderer::actionsForOperations(LayerRendererSurface* surface, const Vector<RefPtr<FilterOperation> >& ops)
{
    Vector<RefPtr<LayerFilterRendererAction> > ret;
    for (unsigned i = 0; i < ops.size(); ++i) {
        const FilterOperation& operation = *ops[i].get();
        if (operation.getOperationType() == FilterOperation::BLUR && static_cast<const BlurFilterOperation&>(operation).stdDeviation().value() < 0.1)
            continue;

        int programId = operationTypeToProgramID(operation.getOperationType());
        ret.append(LayerFilterRendererAction::create(programId));

        switch (operation.getOperationType()) {
        case FilterOperation::GRAYSCALE:
        case FilterOperation::SEPIA:
        case FilterOperation::SATURATE:
        case FilterOperation::HUE_ROTATE:
            ret.last()->appendParameter(Uniform1f::create(m_amountLocation[programId]
                , static_cast<const BasicColorMatrixFilterOperation&>(operation).amount()));
            break;
        case FilterOperation::INVERT:
        case FilterOperation::BRIGHTNESS:
        case FilterOperation::CONTRAST:
        case FilterOperation::OPACITY:
            ret.last()->appendParameter(Uniform1f::create(m_amountLocation[programId]
                , static_cast<const BasicComponentTransferFilterOperation&>(operation).amount()));
            break;
        case FilterOperation::BLUR:
            {
            // Blur is a two-step process:
            //     1. blur X
            //     2. blur Y
            // This way we can have 2n time instead of n^2 time)

            double amount = static_cast<const BlurFilterOperation&>(operation).stdDeviation().value();

            // BLUR Y:
            ret.last()->appendParameter(Uniform1f::create(m_amountLocation[LayerData::CSSFilterShaderBlurY], amount));
            ret.last()->appendParameter(Uniform1f::createWithFunctor(m_blurAmountLocation[0], InverseSurfaceHeight(surface)));

            // BLUR X:
            ret.append(LayerFilterRendererAction::create(LayerData::CSSFilterShaderBlurX));
            ret.last()->appendParameter(Uniform1f::create(m_amountLocation[LayerData::CSSFilterShaderBlurX], amount));
            ret.last()->appendParameter(Uniform1f::createWithFunctor(m_blurAmountLocation[1], InverseSurfaceWidth(surface)));

            }
            break;
        case FilterOperation::DROP_SHADOW:
            {
            // Shadow is a four-step process:
            //     1. capture snapshot
            //        turn into a solid offset mask
            //     2. blur X
            //     3. blur Y
            //     4. repaint original on top of mask
            const DropShadowFilterOperation& dsfo = static_cast<const DropShadowFilterOperation&>(operation);
            ret.last()->setPushSnapshot();
            ret.last()->appendParameter(Uniform2f::create(m_offsetLocation
                , float(dsfo.x()) / float(surface->size().width())
                , float(dsfo.y()) / float(surface->size().height())));
            ret.last()->appendParameter(Uniform3f::create(m_shadowColorLocation
                , float(dsfo.color().red()) / 255.0f
                , float(dsfo.color().green()) / 255.0f
                , float(dsfo.color().blue()) / 255.0f));

            // BLUR Y
            ret.append(LayerFilterRendererAction::create(LayerData::CSSFilterShaderBlurY));
            ret.last()->appendParameter(Uniform1f::create(m_amountLocation[LayerData::CSSFilterShaderBlurY]
                , dsfo.stdDeviation()));
            ret.last()->appendParameter(Uniform1f::createWithFunctor(m_blurAmountLocation[0], InverseSurfaceHeight(surface)));

            // BLUR X
            ret.append(LayerFilterRendererAction::create(LayerData::CSSFilterShaderBlurX));
            ret.last()->appendParameter(Uniform1f::create(m_amountLocation[LayerData::CSSFilterShaderBlurX]
                , dsfo.stdDeviation()));
            ret.last()->appendParameter(Uniform1f::createWithFunctor(m_blurAmountLocation[1], InverseSurfaceWidth(surface)));

            // Repaint original image
            ret.append(LayerFilterRendererAction::create(LayerData::CSSFilterShaderPassthrough));
            ret.last()->setPopSnapshot();
            }
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    if (ret.size() % 2) // We need an even number of actions. See ping-pong note in applyLayerFilters().
        ret.append(LayerFilterRendererAction::create(LayerData::CSSFilterShaderPassthrough));

    return ret;
}

void LayerFilterRenderer::applyActions(unsigned& fbo, LayerCompositingThread* layer, Vector<RefPtr<LayerFilterRendererAction> > actions)
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASSERT(!(actions.size() % 2)); // See ping-ponging note below.

    if (!m_enabled)
        return;

    if (!layer->filters().size())
        return;

    if (!m_texture)
        m_texture = textureCacheCompositingThread()->createTexture();

    bool requireSnapshot = false;
    for (unsigned i = 0; i < actions.size(); ++i) {
        if (actions[i]->shouldPushSnapshot())
            requireSnapshot = true;
    }

    if (!m_snapshotTexture && requireSnapshot)
        m_snapshotTexture = textureCacheCompositingThread()->createTexture();

    LayerRendererSurface* surface = layer->layerRendererSurface();

    // From the code in LayerRenderer, we see that the layer will have a trivial transform that will
    // result in the transformed bounds being a polygon with 4 vertices, i.e. a quad.
    // The LayerFilterRenderer depends on the layer being a quad, so assert to make sure.
    ASSERT(layer->transformedBounds().size() == 4);
    if (layer->transformedBounds().size() != 4)
        return;

    glVertexAttribPointer(m_positionLocation, 2, GL_FLOAT, GL_FALSE, 0, layer->transformedBounds().data());
    glEnableVertexAttribArray(m_positionLocation);
    glVertexAttribPointer(m_texCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, layer->textureCoordinates().data());
    glEnableVertexAttribArray(m_texCoordLocation);

    m_texture->protect(surface->texture()->size(), BlackBerry::Platform::Graphics::AlwaysBacked);
    if (requireSnapshot)
        m_snapshotTexture->protect(surface->texture()->size(), BlackBerry::Platform::Graphics::AlwaysBacked);

    if (!fbo)
        glGenFramebuffers(1, &fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    for (unsigned i = 0; i < actions.size(); ++i) {
        // NOTE ABOUT PING-PONGING
        // =======================
        // Under OpenGL ES 2.0, we cannot use the fbo we are writting to as a texture, so we need to play ping-pong:
        //  1) Draw parent surface to our texture with effect.
        //  2) Draw our surface to parent texture with effect.
        //  3) Repeat.
        // Because we eventually have to end on the parent texture, we need an even number of actions.
        // actionsForOperations takes care of that.

        if (actions[i]->shouldPushSnapshot()) {
            RefPtr<LayerTexture> currentTexture = (!(i % 2) ? surface->texture() : m_texture);
            pushSnapshot(surface, currentTexture->platformTexture());
        }
        if (!(i % 2))
            ping(surface); // Set framebuffer to ours, and texture to parent
        else
            pong(surface); // Set texture to parent, and framebuffer to us

        glClear(GL_COLOR_BUFFER_BIT); // to transparency
        glViewport(0, 0, surface->size().width(), surface->size().height());

        actions[i]->useActionOn(this);

        if (actions[i]->shouldPopSnapshot())
            popSnapshot();

        switch (actions[i]->drawingMode()) {
        case LayerFilterRendererAction::DrawTriangleFanArrays:
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            break;
        case LayerFilterRendererAction::DrawTriangleElementsUShort0:
            glDrawElements(GL_TRIANGLES, actions[i]->drawingModeParameter(), GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(static_cast<intptr_t>(0)));
            break;
        case LayerFilterRendererAction::NumberOfDrawingModes:
            ASSERT_NOT_REACHED();
            break;
        }

        actions[i]->restoreState();

        glVertexAttribPointer(m_positionLocation, 2, GL_FLOAT, GL_FALSE, 0, layer->transformedBounds().data());
        glEnableVertexAttribArray(m_positionLocation);
        glVertexAttribPointer(m_texCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, layer->textureCoordinates().data());
        glEnableVertexAttribArray(m_texCoordLocation);
    }

    m_texture->unprotect();
    if (requireSnapshot)
        m_snapshotTexture->unprotect();
    glBindTexture(GL_TEXTURE_2D, 0);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING) && USE(CSS_FILTERS)
