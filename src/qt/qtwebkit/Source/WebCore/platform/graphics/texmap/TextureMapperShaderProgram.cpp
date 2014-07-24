/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 Copyright (C) 2012 Igalia S.L.
 Copyright (C) 2011 Google Inc. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "TextureMapperShaderProgram.h"

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
#include "LengthFunctions.h"
#include "Logging.h"
#include "TextureMapperGL.h"

#include <wtf/text/StringBuilder.h>

#define STRINGIFY(...) #__VA_ARGS__

namespace WebCore {

static inline bool compositingLogEnabled()
{
#if !LOG_DISABLED
    return LogCompositing.state == WTFLogChannelOn;
#else
    return false;
#endif
}

TextureMapperShaderProgram::TextureMapperShaderProgram(PassRefPtr<GraphicsContext3D> context, const String& vertex, const String& fragment)
    : m_context(context)
{
    m_vertexShader = m_context->createShader(GraphicsContext3D::VERTEX_SHADER);
    m_fragmentShader = m_context->createShader(GraphicsContext3D::FRAGMENT_SHADER);
    m_context->shaderSource(m_vertexShader, vertex);
    m_context->shaderSource(m_fragmentShader, fragment);
    m_id = m_context->createProgram();
    m_context->compileShader(m_vertexShader);
    m_context->compileShader(m_fragmentShader);
    m_context->attachShader(m_id, m_vertexShader);
    m_context->attachShader(m_id, m_fragmentShader);
    m_context->linkProgram(m_id);

    if (!compositingLogEnabled())
        return;

    if (m_context->getError() == GraphicsContext3D::NO_ERROR)
        return;

    String log = m_context->getShaderInfoLog(m_vertexShader);
    LOG(Compositing, "Vertex shader log: %s\n", log.utf8().data());
    log = m_context->getShaderInfoLog(m_fragmentShader);
    LOG(Compositing, "Fragment shader log: %s\n", log.utf8().data());
    log = m_context->getProgramInfoLog(m_id);
    LOG(Compositing, "Program log: %s\n", log.utf8().data());
}

void TextureMapperShaderProgram::setMatrix(GC3Duint location, const TransformationMatrix& matrix)
{
    GC3Dfloat matrixAsFloats[] = {
        GC3Dfloat(matrix.m11()), GC3Dfloat(matrix.m12()), GC3Dfloat(matrix.m13()), GC3Dfloat(matrix.m14()),
        GC3Dfloat(matrix.m21()), GC3Dfloat(matrix.m22()), GC3Dfloat(matrix.m23()), GC3Dfloat(matrix.m24()),
        GC3Dfloat(matrix.m31()), GC3Dfloat(matrix.m32()), GC3Dfloat(matrix.m33()), GC3Dfloat(matrix.m34()),
        GC3Dfloat(matrix.m41()), GC3Dfloat(matrix.m42()), GC3Dfloat(matrix.m43()), GC3Dfloat(matrix.m44())
    };

    m_context->uniformMatrix4fv(location, 1, false, matrixAsFloats);
}

GC3Duint TextureMapperShaderProgram::getLocation(const AtomicString& name, VariableType type)
{
    HashMap<AtomicString, GC3Duint>::iterator it = m_variables.find(name);
    if (it != m_variables.end())
        return it->value;

    GC3Duint location = 0;
    switch (type) {
    case UniformVariable:
        location = m_context->getUniformLocation(m_id, name);
        break;
    case AttribVariable:
        location = m_context->getAttribLocation(m_id, name);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    m_variables.add(name, location);
    return location;
}

TextureMapperShaderProgram::~TextureMapperShaderProgram()
{
    Platform3DObject programID = m_id;
    if (!programID)
        return;

    m_context->detachShader(programID, m_vertexShader);
    m_context->deleteShader(m_vertexShader);
    m_context->detachShader(programID, m_fragmentShader);
    m_context->deleteShader(m_fragmentShader);
    m_context->deleteProgram(programID);
}

#define GLSL_DIRECTIVE(...) "#"#__VA_ARGS__"\n"
static const char* vertexTemplate =
    STRINGIFY(
        attribute vec4 a_vertex;
        uniform mat4 u_modelViewMatrix;
        uniform mat4 u_projectionMatrix;
        uniform highp mat4 u_textureSpaceMatrix;

        varying vec2 v_texCoord;
        varying vec2 v_transformedTexCoord;
        varying float v_antialias;

        void noop(inout vec2 dummyParameter) { }

        vec4 toViewportSpace(vec2 pos) { return vec4(pos, 0., 1.) * u_modelViewMatrix; }

        // This function relies on the assumption that we get edge triangles with control points,
        // a control point being the nearest point to the coordinate that is on the edge.
        void applyAntialiasing(inout vec2 position)
        {
            // We count on the fact that quad passed in is always a unit rect,
            // and the transformation matrix applies the real rect.
            const vec2 center = vec2(0.5, 0.5);
            const float antialiasInflationDistance = 1.;

            // We pass the control point as the zw coordinates of the vertex.
            // The control point is the point on the edge closest to the current position.
            // The control point is used to compute the antialias value.
            vec2 controlPoint = a_vertex.zw;

            // First we calculate the distance in viewport space.
            vec4 centerInViewportCoordinates = toViewportSpace(center);
            vec4 controlPointInViewportCoordinates = toViewportSpace(controlPoint);
            float viewportSpaceDistance = distance(centerInViewportCoordinates, controlPointInViewportCoordinates);

            // We add the inflation distance to the computed distance, and compute the ratio.
            float inflationRatio = (viewportSpaceDistance + antialiasInflationDistance) / viewportSpaceDistance;

            // v_antialias needs to be 0 for the outer edge and 1. for the inner edge.
            // Since the controlPoint is equal to the position in the edge vertices, the value is always 0 for those.
            // For the center point, the distance is always 0.5, so we normalize to 1. by multiplying by 2.
            // By multplying by inflationRatio and dividing by (inflationRatio - 1),
            // We make sure that the varying interpolates between 0 (outer edge), 1 (inner edge) and n > 1 (center).
            v_antialias = distance(controlPoint, position) * 2. * inflationRatio / (inflationRatio - 1.);

            // Now inflate the actual position. By using this formula instead of inflating position directly,
            // we ensure that the center vertex is never inflated.
            position = center + (position - center) * inflationRatio;
        }

        void main(void)
        {
            vec2 position = a_vertex.xy;
            applyAntialiasingIfNeeded(position);

            v_texCoord = position;
            vec4 clampedPosition = clamp(vec4(position, 0., 1.), 0., 1.);
            v_transformedTexCoord = (u_textureSpaceMatrix * clampedPosition).xy;
            gl_Position = u_projectionMatrix * u_modelViewMatrix * vec4(position, 0., 1.);
        }
    );

#define RECT_TEXTURE_DIRECTIVE \
    GLSL_DIRECTIVE(ifdef ENABLE_Rect) \
        GLSL_DIRECTIVE(define SamplerType sampler2DRect) \
        GLSL_DIRECTIVE(define SamplerFunction texture2DRect) \
    GLSL_DIRECTIVE(else) \
        GLSL_DIRECTIVE(define SamplerType sampler2D) \
        GLSL_DIRECTIVE(define SamplerFunction texture2D) \
    GLSL_DIRECTIVE(endif)

#define ANTIALIASING_TEX_COORD_DIRECTIVE \
    GLSL_DIRECTIVE(if defined(ENABLE_Antialiasing) && defined(ENABLE_Texture)) \
        GLSL_DIRECTIVE(define transformTexCoord fragmentTransformTexCoord) \
    GLSL_DIRECTIVE(else) \
        GLSL_DIRECTIVE(define transformTexCoord vertexTransformTexCoord) \
    GLSL_DIRECTIVE(endif)

#define ENABLE_APPLIER(Name) "#define ENABLE_"#Name"\n#define apply"#Name"IfNeeded apply"#Name"\n"
#define DISABLE_APPLIER(Name) "#define apply"#Name"IfNeeded noop\n"
#define BLUR_CONSTANTS \
    GLSL_DIRECTIVE(define GAUSSIAN_KERNEL_HALF_WIDTH 11) \
    GLSL_DIRECTIVE(define GAUSSIAN_KERNEL_STEP 0.2)


static const char* fragmentTemplate =
    RECT_TEXTURE_DIRECTIVE
    ANTIALIASING_TEX_COORD_DIRECTIVE
    BLUR_CONSTANTS
    STRINGIFY(
        precision mediump float;
        uniform SamplerType s_sampler;
        uniform sampler2D s_contentTexture;
        uniform float u_opacity;
        varying float v_antialias;
        varying vec2 v_texCoord;
        varying vec2 v_transformedTexCoord;
        uniform float u_filterAmount;
        uniform vec2 u_blurRadius;
        uniform vec2 u_shadowOffset;
        uniform vec4 u_color;
        uniform float u_gaussianKernel[GAUSSIAN_KERNEL_HALF_WIDTH];
        uniform highp mat4 u_textureSpaceMatrix;

        void noop(inout vec4 dummyParameter) { }
        void noop(inout vec4 dummyParameter, vec2 texCoord) { }

        float antialias() { return smoothstep(0., 1., v_antialias); }

        vec2 fragmentTransformTexCoord()
        {
            vec4 clampedPosition = clamp(vec4(v_texCoord, 0., 1.), 0., 1.);
            return (u_textureSpaceMatrix * clampedPosition).xy;
        }

        vec2 vertexTransformTexCoord() { return v_transformedTexCoord; }

        void applyTexture(inout vec4 color, vec2 texCoord) { color = SamplerFunction(s_sampler, texCoord); }
        void applyOpacity(inout vec4 color) { color *= u_opacity; }
        void applyAntialiasing(inout vec4 color) { color *= antialias(); }

        void applyGrayscaleFilter(inout vec4 color)
        {
            float amount = 1.0 - u_filterAmount;
            color = vec4((0.2126 + 0.7874 * amount) * color.r + (0.7152 - 0.7152 * amount) * color.g + (0.0722 - 0.0722 * amount) * color.b,
                (0.2126 - 0.2126 * amount) * color.r + (0.7152 + 0.2848 * amount) * color.g + (0.0722 - 0.0722 * amount) * color.b,
                (0.2126 - 0.2126 * amount) * color.r + (0.7152 - 0.7152 * amount) * color.g + (0.0722 + 0.9278 * amount) * color.b,
                color.a);
        }

        void applySepiaFilter(inout vec4 color)
        {
            float amount = 1.0 - u_filterAmount;
            color = vec4((0.393 + 0.607 * amount) * color.r + (0.769 - 0.769 * amount) * color.g + (0.189 - 0.189 * amount) * color.b,
                (0.349 - 0.349 * amount) * color.r + (0.686 + 0.314 * amount) * color.g + (0.168 - 0.168 * amount) * color.b,
                (0.272 - 0.272 * amount) * color.r + (0.534 - 0.534 * amount) * color.g + (0.131 + 0.869 * amount) * color.b,
                color.a);
        }

        void applySaturateFilter(inout vec4 color)
        {
            color = vec4((0.213 + 0.787 * u_filterAmount) * color.r + (0.715 - 0.715 * u_filterAmount) * color.g + (0.072 - 0.072 * u_filterAmount) * color.b,
                (0.213 - 0.213 * u_filterAmount) * color.r + (0.715 + 0.285 * u_filterAmount) * color.g + (0.072 - 0.072 * u_filterAmount) * color.b,
                (0.213 - 0.213 * u_filterAmount) * color.r + (0.715 - 0.715 * u_filterAmount) * color.g + (0.072 + 0.928 * u_filterAmount) * color.b,
                color.a);
        }

        void applyHueRotateFilter(inout vec4 color)
        {
            float pi = 3.14159265358979323846;
            float c = cos(u_filterAmount * pi / 180.0);
            float s = sin(u_filterAmount * pi / 180.0);
            color = vec4(color.r * (0.213 + c * 0.787 - s * 0.213) + color.g * (0.715 - c * 0.715 - s * 0.715) + color.b * (0.072 - c * 0.072 + s * 0.928),
                color.r * (0.213 - c * 0.213 + s * 0.143) + color.g * (0.715 + c * 0.285 + s * 0.140) + color.b * (0.072 - c * 0.072 - s * 0.283),
                color.r * (0.213 - c * 0.213 - s * 0.787) +  color.g * (0.715 - c * 0.715 + s * 0.715) + color.b * (0.072 + c * 0.928 + s * 0.072),
                color.a);
        }

        float invert(float n) { return (1.0 - n) * u_filterAmount + n * (1.0 - u_filterAmount); }
        void applyInvertFilter(inout vec4 color)
        {
            color = vec4(invert(color.r), invert(color.g), invert(color.b), color.a);
        }

        void applyBrightnessFilter(inout vec4 color)
        {
            color = vec4(color.rgb * u_filterAmount, color.a);
        }

        float contrast(float n) { return (n - 0.5) * u_filterAmount + 0.5; }
        void applyContrastFilter(inout vec4 color)
        {
            color = vec4(contrast(color.r), contrast(color.g), contrast(color.b), color.a);
        }

        void applyOpacityFilter(inout vec4 color)
        {
            color = vec4(color.r, color.g, color.b, color.a * u_filterAmount);
        }

        vec4 sampleColorAtRadius(float radius, vec2 texCoord)
        {
            vec2 coord = texCoord + radius * u_blurRadius;
            return SamplerFunction(s_sampler, coord) * float(coord.x > 0. && coord.y > 0. && coord.x < 1. && coord.y < 1.);
        }

        float sampleAlphaAtRadius(float radius, vec2 texCoord)
        {
            vec2 coord = texCoord - u_shadowOffset + radius * u_blurRadius;
            return SamplerFunction(s_sampler, coord).a * float(coord.x > 0. && coord.y > 0. && coord.x < 1. && coord.y < 1.);
        }

        void applyBlurFilter(inout vec4 color, vec2 texCoord)
        {
            vec4 total = sampleColorAtRadius(0., texCoord) * u_gaussianKernel[0];
            for (int i = 1; i < GAUSSIAN_KERNEL_HALF_WIDTH; i++) {
                total += sampleColorAtRadius(float(i) * GAUSSIAN_KERNEL_STEP, texCoord) * u_gaussianKernel[i];
                total += sampleColorAtRadius(float(-1 * i) * GAUSSIAN_KERNEL_STEP, texCoord) * u_gaussianKernel[i];
            }

            color = total;
        }

        void applyAlphaBlur(inout vec4 color, vec2 texCoord)
        {
            float total = sampleAlphaAtRadius(0., texCoord) * u_gaussianKernel[0];
            for (int i = 1; i < GAUSSIAN_KERNEL_HALF_WIDTH; i++) {
                total += sampleAlphaAtRadius(float(i) * GAUSSIAN_KERNEL_STEP, texCoord) * u_gaussianKernel[i];
                total += sampleAlphaAtRadius(float(-1 * i) * GAUSSIAN_KERNEL_STEP, texCoord) * u_gaussianKernel[i];
            }

            color *= total;
        }

        vec4 sourceOver(vec4 src, vec4 dst) { return src + dst * (1. - dst.a); }

        void applyContentTexture(inout vec4 color, vec2 texCoord)
        {
            vec4 contentColor = texture2D(s_contentTexture, texCoord);
            color = sourceOver(contentColor, color);
        }

        void applySolidColor(inout vec4 color) { color *= u_color; }

        void main(void)
        {
            vec4 color = vec4(1., 1., 1., 1.);
            vec2 texCoord = transformTexCoord();
            applyTextureIfNeeded(color, texCoord);
            applySolidColorIfNeeded(color);
            applyAntialiasingIfNeeded(color);
            applyOpacityIfNeeded(color);
            applyGrayscaleFilterIfNeeded(color);
            applySepiaFilterIfNeeded(color);
            applySaturateFilterIfNeeded(color);
            applyHueRotateFilterIfNeeded(color);
            applyInvertFilterIfNeeded(color);
            applyBrightnessFilterIfNeeded(color);
            applyContrastFilterIfNeeded(color);
            applyOpacityFilterIfNeeded(color);
            applyBlurFilterIfNeeded(color, texCoord);
            applyAlphaBlurIfNeeded(color, texCoord);
            applyContentTextureIfNeeded(color, texCoord);
            gl_FragColor = color;
        }
    );

PassRefPtr<TextureMapperShaderProgram> TextureMapperShaderProgram::create(PassRefPtr<GraphicsContext3D> context, TextureMapperShaderProgram::Options options)
{
    StringBuilder shaderBuilder;
#define SET_APPLIER_FROM_OPTIONS(Applier) \
    shaderBuilder.append(\
        (options & TextureMapperShaderProgram::Applier) ? ENABLE_APPLIER(Applier) : DISABLE_APPLIER(Applier))

    SET_APPLIER_FROM_OPTIONS(Texture);
    SET_APPLIER_FROM_OPTIONS(Rect);
    SET_APPLIER_FROM_OPTIONS(SolidColor);
    SET_APPLIER_FROM_OPTIONS(Opacity);
    SET_APPLIER_FROM_OPTIONS(Antialiasing);
    SET_APPLIER_FROM_OPTIONS(GrayscaleFilter);
    SET_APPLIER_FROM_OPTIONS(SepiaFilter);
    SET_APPLIER_FROM_OPTIONS(SaturateFilter);
    SET_APPLIER_FROM_OPTIONS(HueRotateFilter);
    SET_APPLIER_FROM_OPTIONS(BrightnessFilter);
    SET_APPLIER_FROM_OPTIONS(ContrastFilter);
    SET_APPLIER_FROM_OPTIONS(InvertFilter);
    SET_APPLIER_FROM_OPTIONS(OpacityFilter);
    SET_APPLIER_FROM_OPTIONS(BlurFilter);
    SET_APPLIER_FROM_OPTIONS(AlphaBlur);
    SET_APPLIER_FROM_OPTIONS(ContentTexture);
    StringBuilder vertexBuilder;
    vertexBuilder.append(shaderBuilder.toString());
    vertexBuilder.append(vertexTemplate);
    shaderBuilder.append(fragmentTemplate);

    String vertexSource = vertexBuilder.toString();
    String fragmentSource = shaderBuilder.toString();

    return adoptRef(new TextureMapperShaderProgram(context, vertexSource, fragmentSource));
}

}
#endif
