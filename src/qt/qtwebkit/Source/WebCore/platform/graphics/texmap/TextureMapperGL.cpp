/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 Copyright (C) 2012 Igalia S.L.
 Copyright (C) 2012 Adobe Systems Incorporated

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
#include "TextureMapperGL.h"

#include "Extensions3D.h"
#include "FilterOperations.h"
#include "GraphicsContext.h"
#include "Image.h"
#include "LengthFunctions.h"
#include "NotImplemented.h"
#include "TextureMapperShaderProgram.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnArrayPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/TemporaryChange.h>

#if PLATFORM(QT)
#include <QPaintEngine>
#include "NativeImageQt.h"
#endif

#if USE(CAIRO)
#include "CairoUtilities.h"
#include "RefPtrCairo.h"
#include <cairo.h>
#include <wtf/text/CString.h>
#endif

#if ENABLE(CSS_SHADERS)
#include "CustomFilterCompiledProgram.h"
#include "CustomFilterOperation.h"
#include "CustomFilterProgram.h"
#include "CustomFilterRenderer.h"
#include "CustomFilterValidatedProgram.h"
#include "ValidatedCustomFilterOperation.h"
#endif

#if !USE(TEXMAP_OPENGL_ES_2)
// FIXME: Move to Extensions3D.h.
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#define GL_UNPACK_SKIP_PIXELS 0x0CF4
#define GL_UNPACK_SKIP_ROWS 0x0CF3
#endif

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)

namespace WebCore {
struct TextureMapperGLData {
    WTF_MAKE_FAST_ALLOCATED;
public:
    struct SharedGLData : public RefCounted<SharedGLData> {

        typedef HashMap<PlatformGraphicsContext3D, SharedGLData*> GLContextDataMap;
        static GLContextDataMap& glContextDataMap()
        {
            static GLContextDataMap map;
            return map;
        }

        static PassRefPtr<SharedGLData> currentSharedGLData(GraphicsContext3D* context)
        {
            GLContextDataMap::iterator it = glContextDataMap().find(context->platformGraphicsContext3D());
            if (it != glContextDataMap().end())
                return it->value;

            return adoptRef(new SharedGLData(context));
        }

        PassRefPtr<TextureMapperShaderProgram> getShaderProgram(TextureMapperShaderProgram::Options options)
        {
            HashMap<TextureMapperShaderProgram::Options, RefPtr<TextureMapperShaderProgram> >::AddResult result = m_programs.add(options, 0);
            if (result.isNewEntry)
                result.iterator->value = TextureMapperShaderProgram::create(m_context, options);

            return result.iterator->value;
        }

        HashMap<TextureMapperShaderProgram::Options, RefPtr<TextureMapperShaderProgram> > m_programs;
        RefPtr<GraphicsContext3D> m_context;

        explicit SharedGLData(GraphicsContext3D* context)
            : m_context(context)
        {
            glContextDataMap().add(context->platformGraphicsContext3D(), this);
        }

        ~SharedGLData()
        {
            GLContextDataMap::const_iterator end = glContextDataMap().end();
            GLContextDataMap::iterator it;
            for (it = glContextDataMap().begin(); it != end; ++it) {
                if (it->value == this)
                    break;
            }

            ASSERT(it != end);
            glContextDataMap().remove(it);
        }
    };

    SharedGLData& sharedGLData() const
    {
        return *sharedData;
    }

    void initializeStencil();

    explicit TextureMapperGLData(GraphicsContext3D* context)
        : context(context)
        , PaintFlags(0)
        , previousProgram(0)
        , targetFrameBuffer(0)
        , didModifyStencil(false)
        , previousScissorState(0)
        , previousDepthState(0)
        , sharedData(TextureMapperGLData::SharedGLData::currentSharedGLData(this->context))
#if ENABLE(CSS_FILTERS)
        , filterInfo(0)
#endif
    { }

    ~TextureMapperGLData();
    Platform3DObject getStaticVBO(GC3Denum target, GC3Dsizeiptr, const void* data);

    GraphicsContext3D* context;
    TransformationMatrix projectionMatrix;
    TextureMapper::PaintFlags PaintFlags;
    GC3Dint previousProgram;
    GC3Dint targetFrameBuffer;
    bool didModifyStencil;
    GC3Dint previousScissorState;
    GC3Dint previousDepthState;
    GC3Dint viewport[4];
    GC3Dint previousScissor[4];
    RefPtr<SharedGLData> sharedData;
    RefPtr<BitmapTexture> currentSurface;
    HashMap<const void*, Platform3DObject> vbos;
#if ENABLE(CSS_FILTERS)
    const BitmapTextureGL::FilterInfo* filterInfo;
#endif
};

Platform3DObject TextureMapperGLData::getStaticVBO(GC3Denum target, GC3Dsizeiptr size, const void* data)
{
    HashMap<const void*, Platform3DObject>::AddResult result = vbos.add(data, 0);
    if (result.isNewEntry) {
        Platform3DObject vbo = context->createBuffer();
        context->bindBuffer(target, vbo);
        context->bufferData(target, size, data, GraphicsContext3D::STATIC_DRAW);
        result.iterator->value = vbo;
    }

    return result.iterator->value;
}

TextureMapperGLData::~TextureMapperGLData()
{
    HashMap<const void*, Platform3DObject>::iterator end = vbos.end();
    for (HashMap<const void*, Platform3DObject>::iterator it = vbos.begin(); it != end; ++it)
        context->deleteBuffer(it->value);
}

void TextureMapperGL::ClipStack::reset(const IntRect& rect, TextureMapperGL::ClipStack::YAxisMode mode)
{
    clipStack.clear();
    size = rect.size();
    yAxisMode = mode;
    clipState = TextureMapperGL::ClipState(rect);
    clipStateDirty = true;
}

void TextureMapperGL::ClipStack::intersect(const IntRect& rect)
{
    clipState.scissorBox.intersect(rect);
    clipStateDirty = true;
}

void TextureMapperGL::ClipStack::setStencilIndex(int stencilIndex)
{
    clipState.stencilIndex = stencilIndex;
    clipStateDirty = true;
}

void TextureMapperGL::ClipStack::push()
{
    clipStack.append(clipState);
    clipStateDirty = true;
}

void TextureMapperGL::ClipStack::pop()
{
    if (clipStack.isEmpty())
        return;
    clipState = clipStack.last();
    clipStack.removeLast();
    clipStateDirty = true;
}

void TextureMapperGL::ClipStack::apply(GraphicsContext3D* context)
{
    if (clipState.scissorBox.isEmpty())
        return;

    context->scissor(clipState.scissorBox.x(),
        (yAxisMode == InvertedYAxis) ? size.height() - clipState.scissorBox.maxY() : clipState.scissorBox.y(),
        clipState.scissorBox.width(), clipState.scissorBox.height());
    context->stencilOp(GraphicsContext3D::KEEP, GraphicsContext3D::KEEP, GraphicsContext3D::KEEP);
    context->stencilFunc(GraphicsContext3D::EQUAL, clipState.stencilIndex - 1, clipState.stencilIndex - 1);
    if (clipState.stencilIndex == 1)
        context->disable(GraphicsContext3D::STENCIL_TEST);
    else
        context->enable(GraphicsContext3D::STENCIL_TEST);
}

void TextureMapperGL::ClipStack::applyIfNeeded(GraphicsContext3D* context)
{
    if (!clipStateDirty)
        return;

    clipStateDirty = false;
    apply(context);
}

void TextureMapperGLData::initializeStencil()
{
    if (currentSurface) {
        static_cast<BitmapTextureGL*>(currentSurface.get())->initializeStencil();
        return;
    }

    if (didModifyStencil)
        return;

    context->clearStencil(0);
    context->clear(GraphicsContext3D::STENCIL_BUFFER_BIT);
    didModifyStencil = true;
}

BitmapTextureGL* toBitmapTextureGL(BitmapTexture* texture)
{
    if (!texture || !texture->isBackedByOpenGL())
        return 0;

    return static_cast<BitmapTextureGL*>(texture);
}

TextureMapperGL::TextureMapperGL()
    : TextureMapper(OpenGLMode)
    , m_enableEdgeDistanceAntialiasing(false)
{
    m_context3D = GraphicsContext3D::createForCurrentGLContext();
    m_data = new TextureMapperGLData(m_context3D.get());
}

TextureMapperGL::ClipStack& TextureMapperGL::clipStack()
{
    return data().currentSurface ? toBitmapTextureGL(data().currentSurface.get())->m_clipStack : m_clipStack;
}

void TextureMapperGL::beginPainting(PaintFlags flags)
{
    m_context3D->getIntegerv(GraphicsContext3D::CURRENT_PROGRAM, &data().previousProgram);
    data().previousScissorState = m_context3D->isEnabled(GraphicsContext3D::SCISSOR_TEST);
    data().previousDepthState = m_context3D->isEnabled(GraphicsContext3D::DEPTH_TEST);
#if PLATFORM(QT)
    if (m_context) {
        QPainter* painter = m_context->platformContext();
        painter->save();
        painter->beginNativePainting();
    }
#endif
    m_context3D->disable(GraphicsContext3D::DEPTH_TEST);
    m_context3D->enable(GraphicsContext3D::SCISSOR_TEST);
    data().didModifyStencil = false;
    m_context3D->depthMask(0);
    m_context3D->getIntegerv(GraphicsContext3D::VIEWPORT, data().viewport);
    m_context3D->getIntegerv(GraphicsContext3D::SCISSOR_BOX, data().previousScissor);
    m_clipStack.reset(IntRect(0, 0, data().viewport[2], data().viewport[3]), ClipStack::InvertedYAxis);
    m_context3D->getIntegerv(GraphicsContext3D::FRAMEBUFFER_BINDING, &data().targetFrameBuffer);
    data().PaintFlags = flags;
    bindSurface(0);
}

void TextureMapperGL::endPainting()
{
    if (data().didModifyStencil) {
        m_context3D->clearStencil(1);
        m_context3D->clear(GraphicsContext3D::STENCIL_BUFFER_BIT);
    }

    m_context3D->useProgram(data().previousProgram);

    m_context3D->scissor(data().previousScissor[0], data().previousScissor[1], data().previousScissor[2], data().previousScissor[3]);
    if (data().previousScissorState)
        m_context3D->enable(GraphicsContext3D::SCISSOR_TEST);
    else
        m_context3D->disable(GraphicsContext3D::SCISSOR_TEST);

    if (data().previousDepthState)
        m_context3D->enable(GraphicsContext3D::DEPTH_TEST);
    else
        m_context3D->disable(GraphicsContext3D::DEPTH_TEST);

#if PLATFORM(QT)
    if (!m_context)
        return;
    QPainter* painter = m_context->platformContext();
    painter->endNativePainting();
    painter->restore();
#endif
}

void TextureMapperGL::drawBorder(const Color& color, float width, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix)
{
    if (clipStack().isCurrentScissorBoxEmpty())
        return;

    RefPtr<TextureMapperShaderProgram> program = data().sharedGLData().getShaderProgram(TextureMapperShaderProgram::SolidColor);
    m_context3D->useProgram(program->programID());

    float r, g, b, a;
    Color(premultipliedARGBFromColor(color)).getRGBA(r, g, b, a);
    m_context3D->uniform4f(program->colorLocation(), r, g, b, a);
    m_context3D->lineWidth(width);

    draw(targetRect, modelViewMatrix, program.get(), GraphicsContext3D::LINE_LOOP, color.hasAlpha() ? ShouldBlend : 0);
}

// FIXME: drawNumber() should save a number texture-atlas and re-use whenever possible.
void TextureMapperGL::drawNumber(int number, const Color& color, const FloatPoint& targetPoint, const TransformationMatrix& modelViewMatrix)
{
    int pointSize = 8;
#if PLATFORM(QT)
    QString counterString = QString::number(number);

    QFont font(QString::fromLatin1("Monospace"), pointSize, QFont::Bold);
    font.setStyleHint(QFont::TypeWriter);

    QFontMetrics fontMetrics(font);
    int width = fontMetrics.width(counterString) + 4;
    int height = fontMetrics.height();

    IntSize size(width, height);
    IntRect sourceRect(IntPoint::zero(), size);
    IntRect targetRect(roundedIntPoint(targetPoint), size);

    QImage image(size, NativeImageQt::defaultFormatForAlphaEnabledImages());
    QPainter painter(&image);
    painter.fillRect(sourceRect, Color::createUnchecked(color.blue(), color.green(), color.red())); // Since we won't swap R+B when uploading a texture, paint with the swapped R+B color.
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(2, height * 0.85, counterString);

    RefPtr<BitmapTexture> texture = acquireTextureFromPool(size);
    const uchar* bits = image.bits();
    static_cast<BitmapTextureGL*>(texture.get())->updateContentsNoSwizzle(bits, sourceRect, IntPoint::zero(), image.bytesPerLine());
    drawTexture(*texture, targetRect, modelViewMatrix, 1.0f, AllEdges);

#elif USE(CAIRO)
    CString counterString = String::number(number).ascii();
    // cairo_text_extents() requires a cairo_t, so dimensions need to be guesstimated.
    int width = counterString.length() * pointSize * 1.2;
    int height = pointSize * 1.5;

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surface);

    float r, g, b, a;
    color.getRGBA(r, g, b, a);
    cairo_set_source_rgba(cr, b, g, r, a); // Since we won't swap R+B when uploading a texture, paint with the swapped R+B color.
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, pointSize);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_move_to(cr, 2, pointSize);
    cairo_show_text(cr, counterString.data());

    IntSize size(width, height);
    IntRect sourceRect(IntPoint::zero(), size);
    IntRect targetRect(roundedIntPoint(targetPoint), size);

    RefPtr<BitmapTexture> texture = acquireTextureFromPool(size);
    const unsigned char* bits = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    static_cast<BitmapTextureGL*>(texture.get())->updateContentsNoSwizzle(bits, sourceRect, IntPoint::zero(), stride);
    drawTexture(*texture, targetRect, modelViewMatrix, 1.0f, AllEdges);

    cairo_surface_destroy(surface);
    cairo_destroy(cr);

#else
    UNUSED_PARAM(number);
    UNUSED_PARAM(pointSize);
    UNUSED_PARAM(targetPoint);
    UNUSED_PARAM(modelViewMatrix);
    notImplemented();
#endif
}

#if ENABLE(CSS_FILTERS)

static TextureMapperShaderProgram::Options optionsForFilterType(FilterOperation::OperationType type, unsigned pass)
{
    switch (type) {
    case FilterOperation::GRAYSCALE:
        return TextureMapperShaderProgram::Texture | TextureMapperShaderProgram::GrayscaleFilter;
    case FilterOperation::SEPIA:
        return TextureMapperShaderProgram::Texture | TextureMapperShaderProgram::SepiaFilter;
    case FilterOperation::SATURATE:
        return TextureMapperShaderProgram::Texture | TextureMapperShaderProgram::SaturateFilter;
    case FilterOperation::HUE_ROTATE:
        return TextureMapperShaderProgram::Texture | TextureMapperShaderProgram::HueRotateFilter;
    case FilterOperation::INVERT:
        return TextureMapperShaderProgram::Texture | TextureMapperShaderProgram::InvertFilter;
    case FilterOperation::BRIGHTNESS:
        return TextureMapperShaderProgram::Texture | TextureMapperShaderProgram::BrightnessFilter;
    case FilterOperation::CONTRAST:
        return TextureMapperShaderProgram::Texture | TextureMapperShaderProgram::ContrastFilter;
    case FilterOperation::OPACITY:
        return TextureMapperShaderProgram::Texture | TextureMapperShaderProgram::OpacityFilter;
    case FilterOperation::BLUR:
        return TextureMapperShaderProgram::BlurFilter;
    case FilterOperation::DROP_SHADOW:
        return TextureMapperShaderProgram::AlphaBlur
            | (pass ? TextureMapperShaderProgram::ContentTexture | TextureMapperShaderProgram::SolidColor: 0);
    default:
        ASSERT_NOT_REACHED();
        return 0;
    }
}

static unsigned getPassesRequiredForFilter(FilterOperation::OperationType type)
{
    switch (type) {
    case FilterOperation::GRAYSCALE:
    case FilterOperation::SEPIA:
    case FilterOperation::SATURATE:
    case FilterOperation::HUE_ROTATE:
    case FilterOperation::INVERT:
    case FilterOperation::BRIGHTNESS:
    case FilterOperation::CONTRAST:
    case FilterOperation::OPACITY:
#if ENABLE(CSS_SHADERS)
    case FilterOperation::CUSTOM:
    case FilterOperation::VALIDATED_CUSTOM:
#endif
        return 1;
    case FilterOperation::BLUR:
    case FilterOperation::DROP_SHADOW:
        // We use two-passes (vertical+horizontal) for blur and drop-shadow.
        return 2;
    default:
        return 0;
    }
}

// Create a normal distribution of 21 values between -2 and 2.
static const unsigned GaussianKernelHalfWidth = 11;
static const float GaussianKernelStep = 0.2;

static inline float gauss(float x)
{
    return exp(-(x * x) / 2.);
}

static float* gaussianKernel()
{
    static bool prepared = false;
    static float kernel[GaussianKernelHalfWidth] = {0, };

    if (prepared)
        return kernel;

    kernel[0] = gauss(0);
    float sum = kernel[0];
    for (unsigned i = 1; i < GaussianKernelHalfWidth; ++i) {
        kernel[i] = gauss(i * GaussianKernelStep);
        sum += 2 * kernel[i];
    }

    // Normalize the kernel.
    float scale = 1 / sum;
    for (unsigned i = 0; i < GaussianKernelHalfWidth; ++i)
        kernel[i] *= scale;

    prepared = true;
    return kernel;
}

static void prepareFilterProgram(TextureMapperShaderProgram* program, const FilterOperation& operation, unsigned pass, const IntSize& size, GC3Duint contentTexture)
{
    RefPtr<GraphicsContext3D> context = program->context();
    context->useProgram(program->programID());

    switch (operation.getOperationType()) {
    case FilterOperation::GRAYSCALE:
    case FilterOperation::SEPIA:
    case FilterOperation::SATURATE:
    case FilterOperation::HUE_ROTATE:
        context->uniform1f(program->filterAmountLocation(), static_cast<const BasicColorMatrixFilterOperation&>(operation).amount());
        break;
    case FilterOperation::INVERT:
    case FilterOperation::BRIGHTNESS:
    case FilterOperation::CONTRAST:
    case FilterOperation::OPACITY:
        context->uniform1f(program->filterAmountLocation(), static_cast<const BasicComponentTransferFilterOperation&>(operation).amount());
        break;
    case FilterOperation::BLUR: {
        const BlurFilterOperation& blur = static_cast<const BlurFilterOperation&>(operation);
        FloatSize radius;

        // Blur is done in two passes, first horizontally and then vertically. The same shader is used for both.
        if (pass)
            radius.setHeight(floatValueForLength(blur.stdDeviation(), size.height()) / size.height());
        else
            radius.setWidth(floatValueForLength(blur.stdDeviation(), size.width()) / size.width());

        context->uniform2f(program->blurRadiusLocation(), radius.width(), radius.height());
        context->uniform1fv(program->gaussianKernelLocation(), GaussianKernelHalfWidth, gaussianKernel());
        break;
    }
    case FilterOperation::DROP_SHADOW: {
        const DropShadowFilterOperation& shadow = static_cast<const DropShadowFilterOperation&>(operation);
        context->uniform1fv(program->gaussianKernelLocation(), GaussianKernelHalfWidth, gaussianKernel());
        switch (pass) {
        case 0:
            // First pass: horizontal alpha blur.
            context->uniform2f(program->blurRadiusLocation(), shadow.stdDeviation() / float(size.width()), 0);
            context->uniform2f(program->shadowOffsetLocation(), float(shadow.location().x()) / float(size.width()), float(shadow.location().y()) / float(size.height()));
            break;
        case 1:
            // Second pass: we need the shadow color and the content texture for compositing.
            float r, g, b, a;
            Color(premultipliedARGBFromColor(shadow.color())).getRGBA(r, g, b, a);
            context->uniform4f(program->colorLocation(), r, g, b, a);
            context->uniform2f(program->blurRadiusLocation(), 0, shadow.stdDeviation() / float(size.height()));
            context->uniform2f(program->shadowOffsetLocation(), 0, 0);
            context->activeTexture(GraphicsContext3D::TEXTURE1);
            context->bindTexture(GraphicsContext3D::TEXTURE_2D, contentTexture);
            context->uniform1i(program->contentTextureLocation(), 1);
            break;
        }
        break;
    }
    default:
        break;
    }
}
#endif

void TextureMapperGL::drawTexture(const BitmapTexture& texture, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity, unsigned exposedEdges)
{
    if (!texture.isValid())
        return;

    if (clipStack().isCurrentScissorBoxEmpty())
        return;

    const BitmapTextureGL& textureGL = static_cast<const BitmapTextureGL&>(texture);
#if ENABLE(CSS_FILTERS)
    TemporaryChange<const BitmapTextureGL::FilterInfo*> filterInfo(data().filterInfo, textureGL.filterInfo());
#endif

    drawTexture(textureGL.id(), textureGL.isOpaque() ? 0 : ShouldBlend, textureGL.size(), targetRect, matrix, opacity, exposedEdges);
}

void TextureMapperGL::drawTexture(Platform3DObject texture, Flags flags, const IntSize& textureSize, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix, float opacity, unsigned exposedEdges)
{
    bool useRect = flags & ShouldUseARBTextureRect;
    bool useAntialiasing = m_enableEdgeDistanceAntialiasing
        && exposedEdges == AllEdges
        && !modelViewMatrix.mapQuad(targetRect).isRectilinear();

    TextureMapperShaderProgram::Options options = TextureMapperShaderProgram::Texture;
    if (useRect)
        options |= TextureMapperShaderProgram::Rect;
    if (opacity < 1)
        options |= TextureMapperShaderProgram::Opacity;
    if (useAntialiasing) {
        options |= TextureMapperShaderProgram::Antialiasing;
        flags |= ShouldAntialias;
    }

#if ENABLE(CSS_FILTERS)
    RefPtr<FilterOperation> filter = data().filterInfo ? data().filterInfo->filter: 0;
    GC3Duint filterContentTextureID = 0;

    if (filter) {
        if (data().filterInfo->contentTexture)
            filterContentTextureID = toBitmapTextureGL(data().filterInfo->contentTexture.get())->id();
        options |= optionsForFilterType(filter->getOperationType(), data().filterInfo->pass);
        if (filter->affectsOpacity())
            flags |= ShouldBlend;
    }
#endif

    if (useAntialiasing || opacity < 1)
        flags |= ShouldBlend;

    RefPtr<TextureMapperShaderProgram> program;
    program = data().sharedGLData().getShaderProgram(options);

#if ENABLE(CSS_FILTERS)
    if (filter)
        prepareFilterProgram(program.get(), *filter.get(), data().filterInfo->pass, textureSize, filterContentTextureID);
#endif

    drawTexturedQuadWithProgram(program.get(), texture, flags, textureSize, targetRect, modelViewMatrix, opacity);
}

void TextureMapperGL::drawSolidColor(const FloatRect& rect, const TransformationMatrix& matrix, const Color& color)
{
    Flags flags = 0;
    TextureMapperShaderProgram::Options options = TextureMapperShaderProgram::SolidColor;
    if (!matrix.mapQuad(rect).isRectilinear()) {
        options |= TextureMapperShaderProgram::Antialiasing;
        flags |= ShouldBlend | ShouldAntialias;
    }

    RefPtr<TextureMapperShaderProgram> program = data().sharedGLData().getShaderProgram(options);
    m_context3D->useProgram(program->programID());

    float r, g, b, a;
    Color(premultipliedARGBFromColor(color)).getRGBA(r, g, b, a);
    m_context3D->uniform4f(program->colorLocation(), r, g, b, a);
    if (a < 1)
        flags |= ShouldBlend;

    draw(rect, matrix, program.get(), GraphicsContext3D::TRIANGLE_FAN, flags);
}

void TextureMapperGL::drawEdgeTriangles(TextureMapperShaderProgram* program)
{
    const GC3Dfloat left = 0;
    const GC3Dfloat top = 0;
    const GC3Dfloat right = 1;
    const GC3Dfloat bottom = 1;
    const GC3Dfloat center = 0.5;

// Each 4d triangle consists of a center point and two edge points, where the zw coordinates
// of each vertex equals the nearest point to the vertex on the edge.
#define SIDE_TRIANGLE_DATA(x1, y1, x2, y2) \
    x1, y1, x1, y1, \
    x2, y2, x2, y2, \
    center, center, (x1 + x2) / 2, (y1 + y2) / 2

    static const GC3Dfloat unitRectSideTriangles[] = {
        SIDE_TRIANGLE_DATA(left, top, right, top),
        SIDE_TRIANGLE_DATA(left, top, left, bottom),
        SIDE_TRIANGLE_DATA(right, top, right, bottom),
        SIDE_TRIANGLE_DATA(left, bottom, right, bottom)
    };
#undef SIDE_TRIANGLE_DATA

    Platform3DObject vbo = data().getStaticVBO(GraphicsContext3D::ARRAY_BUFFER, sizeof(GC3Dfloat) * 48, unitRectSideTriangles);
    m_context3D->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, vbo);
    m_context3D->vertexAttribPointer(program->vertexLocation(), 4, GraphicsContext3D::FLOAT, false, 0, 0);
    m_context3D->drawArrays(GraphicsContext3D::TRIANGLES, 0, 12);
    m_context3D->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, 0);
}

void TextureMapperGL::drawUnitRect(TextureMapperShaderProgram* program, GC3Denum drawingMode)
{
    static const GC3Dfloat unitRect[] = { 0, 0, 1, 0, 1, 1, 0, 1 };
    Platform3DObject vbo = data().getStaticVBO(GraphicsContext3D::ARRAY_BUFFER, sizeof(GC3Dfloat) * 8, unitRect);
    m_context3D->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, vbo);
    m_context3D->vertexAttribPointer(program->vertexLocation(), 2, GraphicsContext3D::FLOAT, false, 0, 0);
    m_context3D->drawArrays(drawingMode, 0, 4);
    m_context3D->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, 0);
}

void TextureMapperGL::draw(const FloatRect& rect, const TransformationMatrix& modelViewMatrix, TextureMapperShaderProgram* shaderProgram, GC3Denum drawingMode, Flags flags)
{
    TransformationMatrix matrix =
        TransformationMatrix(modelViewMatrix).multiply(TransformationMatrix::rectToRect(FloatRect(0, 0, 1, 1), rect));

    m_context3D->enableVertexAttribArray(shaderProgram->vertexLocation());
    shaderProgram->setMatrix(shaderProgram->modelViewMatrixLocation(), matrix);
    shaderProgram->setMatrix(shaderProgram->projectionMatrixLocation(), data().projectionMatrix);

    if (isInMaskMode()) {
        m_context3D->blendFunc(GraphicsContext3D::ZERO, GraphicsContext3D::SRC_ALPHA);
        m_context3D->enable(GraphicsContext3D::BLEND);
    } else {
        if (flags & ShouldBlend) {
            m_context3D->blendFunc(GraphicsContext3D::ONE, GraphicsContext3D::ONE_MINUS_SRC_ALPHA);
            m_context3D->enable(GraphicsContext3D::BLEND);
        } else
            m_context3D->disable(GraphicsContext3D::BLEND);
    }

    if (flags & ShouldAntialias)
        drawEdgeTriangles(shaderProgram);
    else
        drawUnitRect(shaderProgram, drawingMode);

    m_context3D->disableVertexAttribArray(shaderProgram->vertexLocation());
    m_context3D->blendFunc(GraphicsContext3D::ONE, GraphicsContext3D::ONE_MINUS_SRC_ALPHA);
    m_context3D->enable(GraphicsContext3D::BLEND);
}

void TextureMapperGL::drawTexturedQuadWithProgram(TextureMapperShaderProgram* program, uint32_t texture, Flags flags, const IntSize& size, const FloatRect& rect, const TransformationMatrix& modelViewMatrix, float opacity)
{
    m_context3D->useProgram(program->programID());
    m_context3D->activeTexture(GraphicsContext3D::TEXTURE0);
    GC3Denum target = flags & ShouldUseARBTextureRect ? GC3Denum(Extensions3D::TEXTURE_RECTANGLE_ARB) : GC3Denum(GraphicsContext3D::TEXTURE_2D);
    m_context3D->bindTexture(target, texture);
    m_context3D->uniform1i(program->samplerLocation(), 0);
    if (wrapMode() == RepeatWrap) {
        m_context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::REPEAT);
        m_context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::REPEAT);
    }

    TransformationMatrix patternTransform = this->patternTransform();
    if (flags & ShouldFlipTexture)
        patternTransform.flipY();
    if (flags & ShouldUseARBTextureRect)
        patternTransform.scaleNonUniform(size.width(), size.height());
    if (flags & ShouldFlipTexture)
        patternTransform.translate(0, -1);

    program->setMatrix(program->textureSpaceMatrixLocation(), patternTransform);
    m_context3D->uniform1f(program->opacityLocation(), opacity);

    if (opacity < 1)
        flags |= ShouldBlend;

    draw(rect, modelViewMatrix, program, GraphicsContext3D::TRIANGLE_FAN, flags);
    m_context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
    m_context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);
}

BitmapTextureGL::BitmapTextureGL(TextureMapperGL* textureMapper)
    : m_id(0)
    , m_fbo(0)
    , m_rbo(0)
    , m_depthBufferObject(0)
    , m_shouldClear(true)
    , m_context3D(textureMapper->graphicsContext3D())
{
}

bool BitmapTextureGL::canReuseWith(const IntSize& contentsSize, Flags)
{
    return contentsSize == m_textureSize;
}

#if OS(DARWIN)
#define DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE GL_UNSIGNED_INT_8_8_8_8_REV
#else
#define DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE GraphicsContext3D::UNSIGNED_BYTE
#endif

static void swizzleBGRAToRGBA(uint32_t* data, const IntRect& rect, int stride = 0)
{
    stride = stride ? stride : rect.width();
    for (int y = rect.y(); y < rect.maxY(); ++y) {
        uint32_t* p = data + y * stride;
        for (int x = rect.x(); x < rect.maxX(); ++x)
            p[x] = ((p[x] << 16) & 0xff0000) | ((p[x] >> 16) & 0xff) | (p[x] & 0xff00ff00);
    }
}

// If GL_EXT_texture_format_BGRA8888 is supported in the OpenGLES
// internal and external formats need to be BGRA
static bool driverSupportsExternalTextureBGRA(GraphicsContext3D* context)
{
    if (context->isGLES2Compliant()) {
        static bool supportsExternalTextureBGRA = context->getExtensions()->supports("GL_EXT_texture_format_BGRA8888");
        return supportsExternalTextureBGRA;
    }

    return true;
}

static bool driverSupportsSubImage(GraphicsContext3D* context)
{
    if (context->isGLES2Compliant()) {
        static bool supportsSubImage = context->getExtensions()->supports("GL_EXT_unpack_subimage");
        return supportsSubImage;
    }

    return true;
}

void BitmapTextureGL::didReset()
{
    if (!m_id)
        m_id = m_context3D->createTexture();

    m_shouldClear = true;
    if (m_textureSize == contentSize())
        return;


    m_textureSize = contentSize();
    m_context3D->bindTexture(GraphicsContext3D::TEXTURE_2D, m_id);
    m_context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR);
    m_context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR);
    m_context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
    m_context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);

    Platform3DObject internalFormat = GraphicsContext3D::RGBA;
    Platform3DObject externalFormat = GraphicsContext3D::BGRA;
    if (m_context3D->isGLES2Compliant()) {
        if (driverSupportsExternalTextureBGRA(m_context3D.get()))
            internalFormat = GraphicsContext3D::BGRA;
        else
            externalFormat = GraphicsContext3D::RGBA;
    }

    m_context3D->texImage2DDirect(GraphicsContext3D::TEXTURE_2D, 0, internalFormat, m_textureSize.width(), m_textureSize.height(), 0, externalFormat, DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE, 0);
}

void BitmapTextureGL::updateContentsNoSwizzle(const void* srcData, const IntRect& targetRect, const IntPoint& sourceOffset, int bytesPerLine, unsigned bytesPerPixel, Platform3DObject glFormat)
{
    m_context3D->bindTexture(GraphicsContext3D::TEXTURE_2D, m_id);
    if (driverSupportsSubImage(m_context3D.get())) { // For ES drivers that don't support sub-images.
        // Use the OpenGL sub-image extension, now that we know it's available.
        m_context3D->pixelStorei(GL_UNPACK_ROW_LENGTH, bytesPerLine / bytesPerPixel);
        m_context3D->pixelStorei(GL_UNPACK_SKIP_ROWS, sourceOffset.y());
        m_context3D->pixelStorei(GL_UNPACK_SKIP_PIXELS, sourceOffset.x());
    }

    m_context3D->texSubImage2D(GraphicsContext3D::TEXTURE_2D, 0, targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(), glFormat, DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE, srcData);

    if (driverSupportsSubImage(m_context3D.get())) { // For ES drivers that don't support sub-images.
        m_context3D->pixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        m_context3D->pixelStorei(GL_UNPACK_SKIP_ROWS, 0);
        m_context3D->pixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    }
}

void BitmapTextureGL::updateContents(const void* srcData, const IntRect& targetRect, const IntPoint& sourceOffset, int bytesPerLine, UpdateContentsFlag updateContentsFlag)
{
    Platform3DObject glFormat = GraphicsContext3D::RGBA;
    m_context3D->bindTexture(GraphicsContext3D::TEXTURE_2D, m_id);

    const unsigned bytesPerPixel = 4;
    char* data = reinterpret_cast<char*>(const_cast<void*>(srcData));
    Vector<char> temporaryData;
    IntPoint adjustedSourceOffset = sourceOffset;

    // Texture upload requires subimage buffer if driver doesn't support subimage and we don't have full image upload.
    bool requireSubImageBuffer = !driverSupportsSubImage(m_context3D.get())
        && !(bytesPerLine == static_cast<int>(targetRect.width() * bytesPerPixel) && adjustedSourceOffset == IntPoint::zero());

    // prepare temporaryData if necessary
    if ((!driverSupportsExternalTextureBGRA(m_context3D.get()) && updateContentsFlag == UpdateCannotModifyOriginalImageData) || requireSubImageBuffer) {
        temporaryData.resize(targetRect.width() * targetRect.height() * bytesPerPixel);
        data = temporaryData.data();
        const char* bits = static_cast<const char*>(srcData);
        const char* src = bits + sourceOffset.y() * bytesPerLine + sourceOffset.x() * bytesPerPixel;
        char* dst = data;
        const int targetBytesPerLine = targetRect.width() * bytesPerPixel;
        for (int y = 0; y < targetRect.height(); ++y) {
            memcpy(dst, src, targetBytesPerLine);
            src += bytesPerLine;
            dst += targetBytesPerLine;
        }

        bytesPerLine = targetBytesPerLine;
        adjustedSourceOffset = IntPoint(0, 0);
    }

    if (driverSupportsExternalTextureBGRA(m_context3D.get()))
        glFormat = GraphicsContext3D::BGRA;
    else
        swizzleBGRAToRGBA(reinterpret_cast_ptr<uint32_t*>(data), IntRect(adjustedSourceOffset, targetRect.size()), bytesPerLine / bytesPerPixel);

    updateContentsNoSwizzle(data, targetRect, adjustedSourceOffset, bytesPerLine, bytesPerPixel, glFormat);
}

void BitmapTextureGL::updateContents(Image* image, const IntRect& targetRect, const IntPoint& offset, UpdateContentsFlag updateContentsFlag)
{
    if (!image)
        return;
    NativeImagePtr frameImage = image->nativeImageForCurrentFrame();
    if (!frameImage)
        return;

    int bytesPerLine;
    const char* imageData;

#if PLATFORM(QT)
    QImage qImage;
    QPaintEngine* paintEngine = frameImage->paintEngine();
    if (paintEngine && paintEngine->type() == QPaintEngine::Raster) {
        // QRasterPixmapData::toImage() will deep-copy the backing QImage if there's an active QPainter on it.
        // For performance reasons, we don't want that here, so we temporarily redirect the paint engine.
        QPaintDevice* currentPaintDevice = paintEngine->paintDevice();
        paintEngine->setPaintDevice(0);
        qImage = frameImage->toImage();
        paintEngine->setPaintDevice(currentPaintDevice);
    } else
        qImage = frameImage->toImage();
    imageData = reinterpret_cast<const char*>(qImage.constBits());
    bytesPerLine = qImage.bytesPerLine();
#elif USE(CAIRO)
    cairo_surface_t* surface = frameImage.get();
    imageData = reinterpret_cast<const char*>(cairo_image_surface_get_data(surface));
    bytesPerLine = cairo_image_surface_get_stride(surface);
#endif

    updateContents(imageData, targetRect, offset, bytesPerLine, updateContentsFlag);
}

#if ENABLE(CSS_SHADERS)
void TextureMapperGL::removeCachedCustomFilterProgram(CustomFilterProgram* program)
{
    m_customFilterPrograms.remove(program->programInfo());
}

bool TextureMapperGL::drawUsingCustomFilter(BitmapTexture& target, const BitmapTexture& source, const FilterOperation& filter)
{
    RefPtr<CustomFilterRenderer> renderer;
    switch (filter.getOperationType()) {
    case FilterOperation::CUSTOM: {
        // WebKit2 pipeline is using the CustomFilterOperation, that's because of the "de-serialization" that
        // happens in CoordinatedGraphicsArgumentCoders.
        const CustomFilterOperation* customFilter = static_cast<const CustomFilterOperation*>(&filter);
        RefPtr<CustomFilterProgram> program = customFilter->program();
        renderer = CustomFilterRenderer::create(m_context3D, program->programType(), customFilter->parameters(), 
            customFilter->meshRows(), customFilter->meshColumns(), customFilter->meshType());
        CustomFilterProgramMap::AddResult result = m_customFilterPrograms.add(program->programInfo(), 0);
        if (result.isNewEntry)
            result.iterator->value = CustomFilterCompiledProgram::create(m_context3D, program->vertexShaderString(), program->fragmentShaderString(), program->programType());
        renderer->setCompiledProgram(result.iterator->value);
        break;
    }
    case FilterOperation::VALIDATED_CUSTOM: {
        // WebKit1 uses the ValidatedCustomFilterOperation.
        const ValidatedCustomFilterOperation* customFilter = static_cast<const ValidatedCustomFilterOperation*>(&filter);
        RefPtr<CustomFilterValidatedProgram> program = customFilter->validatedProgram();
        renderer = CustomFilterRenderer::create(m_context3D, program->programInfo().programType(), customFilter->parameters(),
            customFilter->meshRows(), customFilter->meshColumns(), customFilter->meshType());
        RefPtr<CustomFilterCompiledProgram> compiledProgram;
        CustomFilterProgramMap::AddResult result = m_customFilterPrograms.add(program->programInfo(), 0);
        if (result.isNewEntry)
            result.iterator->value = CustomFilterCompiledProgram::create(m_context3D, program->validatedVertexShader(), program->validatedFragmentShader(), program->programInfo().programType());
        renderer->setCompiledProgram(result.iterator->value);
        break;
    }
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
    if (!renderer || !renderer->prepareForDrawing())
        return false;
    static_cast<BitmapTextureGL&>(target).initializeDepthBuffer();
    m_context3D->enable(GraphicsContext3D::BLEND);
    m_context3D->blendFunc(GraphicsContext3D::ONE, GraphicsContext3D::ONE_MINUS_SRC_ALPHA);
    m_context3D->enable(GraphicsContext3D::DEPTH_TEST);
    m_context3D->depthFunc(GraphicsContext3D::LESS);
    m_context3D->clearDepth(1);
    m_context3D->depthMask(1);
    m_context3D->clearColor(0, 0, 0, 0);
    m_context3D->clear(GraphicsContext3D::COLOR_BUFFER_BIT | GraphicsContext3D::DEPTH_BUFFER_BIT);
    renderer->draw(static_cast<const BitmapTextureGL&>(source).id(), source.size());
    m_context3D->disable(GraphicsContext3D::DEPTH_TEST);
    m_context3D->disable(GraphicsContext3D::BLEND);
    m_context3D->depthMask(0);
    return true;
}
#endif

#if ENABLE(CSS_FILTERS)
void TextureMapperGL::drawFiltered(const BitmapTexture& sampler, const BitmapTexture* contentTexture, const FilterOperation& filter, int pass)
{
    // For standard filters, we always draw the whole texture without transformations.
    TextureMapperShaderProgram::Options options = optionsForFilterType(filter.getOperationType(), pass);
    RefPtr<TextureMapperShaderProgram> program = data().sharedGLData().getShaderProgram(options);
    ASSERT(program);

    prepareFilterProgram(program.get(), filter, pass, sampler.contentSize(), contentTexture ? static_cast<const BitmapTextureGL*>(contentTexture)->id() : 0);
    FloatRect targetRect(IntPoint::zero(), sampler.contentSize());
    drawTexturedQuadWithProgram(program.get(), static_cast<const BitmapTextureGL&>(sampler).id(), 0, IntSize(1, 1), targetRect, TransformationMatrix(), 1);
}

static bool isCustomFilter(FilterOperation::OperationType type)
{
#if ENABLE(CSS_SHADERS)
    return type == FilterOperation::CUSTOM || type == FilterOperation::VALIDATED_CUSTOM;
#else
    return false;
#endif
}

PassRefPtr<BitmapTexture> BitmapTextureGL::applyFilters(TextureMapper* textureMapper, const FilterOperations& filters)
{
    if (filters.isEmpty())
        return this;

    TextureMapperGL* texmapGL = static_cast<TextureMapperGL*>(textureMapper);
    RefPtr<BitmapTexture> previousSurface = texmapGL->data().currentSurface;
    RefPtr<BitmapTexture> resultSurface = this;
    RefPtr<BitmapTexture> intermediateSurface;
    RefPtr<BitmapTexture> spareSurface;

    m_filterInfo = FilterInfo();

    for (size_t i = 0; i < filters.size(); ++i) {
        RefPtr<FilterOperation> filter = filters.operations()[i];
        ASSERT(filter);

        bool custom = isCustomFilter(filter->getOperationType());

        int numPasses = getPassesRequiredForFilter(filter->getOperationType());
        for (int j = 0; j < numPasses; ++j) {
            bool last = (i == filters.size() - 1) && (j == numPasses - 1);
            if (custom || !last) {
                if (!intermediateSurface)
                    intermediateSurface = texmapGL->acquireTextureFromPool(contentSize());
                texmapGL->bindSurface(intermediateSurface.get());
            }

#if ENABLE(CSS_SHADERS)
            if (custom) {
                if (texmapGL->drawUsingCustomFilter(*intermediateSurface.get(), *resultSurface.get(), *filter))
                    std::swap(resultSurface, intermediateSurface);
                continue;
            }
#endif
            if (last) {
                toBitmapTextureGL(resultSurface.get())->m_filterInfo = BitmapTextureGL::FilterInfo(filter, j, spareSurface);
                break;
            }

            texmapGL->drawFiltered(*resultSurface.get(), spareSurface.get(), *filter, j);
            if (!j && filter->getOperationType() == FilterOperation::DROP_SHADOW) {
                spareSurface = resultSurface;
                resultSurface.clear();
            }
            std::swap(resultSurface, intermediateSurface);
        }
    }

    texmapGL->bindSurface(previousSurface.get());
    return resultSurface;
}
#endif

static inline TransformationMatrix createProjectionMatrix(const IntSize& size, bool mirrored)
{
    const float nearValue = 9999999;
    const float farValue = -99999;

    return TransformationMatrix(2.0 / float(size.width()), 0, 0, 0,
                                0, (mirrored ? 2.0 : -2.0) / float(size.height()), 0, 0,
                                0, 0, -2.f / (farValue - nearValue), 0,
                                -1, mirrored ? -1 : 1, -(farValue + nearValue) / (farValue - nearValue), 1);
}

void BitmapTextureGL::initializeStencil()
{
    if (m_rbo)
        return;

    m_rbo = m_context3D->createRenderbuffer();
    m_context3D->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, m_rbo);
#ifdef TEXMAP_OPENGL_ES_2
    m_context3D->renderbufferStorage(GraphicsContext3D::RENDERBUFFER, GraphicsContext3D::STENCIL_INDEX8, m_textureSize.width(), m_textureSize.height());
#else
    m_context3D->renderbufferStorage(GraphicsContext3D::RENDERBUFFER, GraphicsContext3D::DEPTH_STENCIL, m_textureSize.width(), m_textureSize.height());
#endif
    m_context3D->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, 0);
    m_context3D->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::STENCIL_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, m_rbo);
    m_context3D->clearStencil(0);
    m_context3D->clear(GraphicsContext3D::STENCIL_BUFFER_BIT);
}

void BitmapTextureGL::initializeDepthBuffer()
{
    if (m_depthBufferObject)
        return;

    m_depthBufferObject = m_context3D->createRenderbuffer();
    m_context3D->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, m_depthBufferObject);
    m_context3D->renderbufferStorage(GraphicsContext3D::RENDERBUFFER, GraphicsContext3D::DEPTH_COMPONENT16, m_textureSize.width(), m_textureSize.height());
    m_context3D->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, 0);
    m_context3D->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::DEPTH_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, m_depthBufferObject);
}

void BitmapTextureGL::clearIfNeeded()
{
    if (!m_shouldClear)
        return;

    m_clipStack.reset(IntRect(IntPoint::zero(), m_textureSize), TextureMapperGL::ClipStack::DefaultYAxis);
    m_clipStack.applyIfNeeded(m_context3D.get());
    m_context3D->clearColor(0, 0, 0, 0);
    m_context3D->clear(GraphicsContext3D::COLOR_BUFFER_BIT);
    m_shouldClear = false;
}

void BitmapTextureGL::createFboIfNeeded()
{
    if (m_fbo)
        return;

    m_fbo = m_context3D->createFramebuffer();
    m_context3D->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
    m_context3D->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::TEXTURE_2D, id(), 0);
    m_shouldClear = true;
}

void BitmapTextureGL::bind(TextureMapperGL* textureMapper)
{
    m_context3D->bindTexture(GraphicsContext3D::TEXTURE_2D, 0);
    createFboIfNeeded();
    m_context3D->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
    m_context3D->viewport(0, 0, m_textureSize.width(), m_textureSize.height());
    clearIfNeeded();
    textureMapper->data().projectionMatrix = createProjectionMatrix(m_textureSize, true /* mirrored */);
    m_clipStack.apply(m_context3D.get());
}

BitmapTextureGL::~BitmapTextureGL()
{
    if (m_id)
        m_context3D->deleteTexture(m_id);

    if (m_fbo)
        m_context3D->deleteFramebuffer(m_fbo);

    if (m_rbo)
        m_context3D->deleteRenderbuffer(m_rbo);

    if (m_depthBufferObject)
        m_context3D->deleteRenderbuffer(m_depthBufferObject);
}

bool BitmapTextureGL::isValid() const
{
    return m_id;
}

IntSize BitmapTextureGL::size() const
{
    return m_textureSize;
}

TextureMapperGL::~TextureMapperGL()
{
    delete m_data;
}

void TextureMapperGL::bindDefaultSurface()
{
    m_context3D->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, data().targetFrameBuffer);
    IntSize viewportSize(data().viewport[2], data().viewport[3]);
    data().projectionMatrix = createProjectionMatrix(viewportSize, data().PaintFlags & PaintingMirrored);
    m_context3D->viewport(data().viewport[0], data().viewport[1], viewportSize.width(), viewportSize.height());
    m_clipStack.apply(m_context3D.get());
    data().currentSurface.clear();
}

void TextureMapperGL::bindSurface(BitmapTexture *surface)
{
    if (!surface) {
        bindDefaultSurface();
        return;
    }

    static_cast<BitmapTextureGL*>(surface)->bind(this);
    data().currentSurface = surface;
}

bool TextureMapperGL::beginScissorClip(const TransformationMatrix& modelViewMatrix, const FloatRect& targetRect)
{
    // 3D transforms are currently not supported in scissor clipping
    // resulting in cropped surfaces when z>0.
    if (!modelViewMatrix.isAffine())
        return false;

    FloatQuad quad = modelViewMatrix.projectQuad(targetRect);
    IntRect rect = quad.enclosingBoundingBox();

    // Only use scissors on rectilinear clips.
    if (!quad.isRectilinear() || rect.isEmpty())
        return false;

    clipStack().intersect(rect);
    clipStack().applyIfNeeded(m_context3D.get());
    return true;
}

void TextureMapperGL::beginClip(const TransformationMatrix& modelViewMatrix, const FloatRect& targetRect)
{
    clipStack().push();
    if (beginScissorClip(modelViewMatrix, targetRect))
        return;

    data().initializeStencil();

    RefPtr<TextureMapperShaderProgram> program = data().sharedGLData().getShaderProgram(TextureMapperShaderProgram::SolidColor);

    m_context3D->useProgram(program->programID());
    m_context3D->enableVertexAttribArray(program->vertexLocation());
    const GC3Dfloat unitRect[] = {0, 0, 1, 0, 1, 1, 0, 1};
    m_context3D->vertexAttribPointer(program->vertexLocation(), 2, GraphicsContext3D::FLOAT, false, 0, GC3Dintptr(unitRect));

    TransformationMatrix matrix = TransformationMatrix(modelViewMatrix)
        .multiply(TransformationMatrix::rectToRect(FloatRect(0, 0, 1, 1), targetRect));

    static const TransformationMatrix fullProjectionMatrix = TransformationMatrix::rectToRect(FloatRect(0, 0, 1, 1), FloatRect(-1, -1, 2, 2));

    int stencilIndex = clipStack().getStencilIndex();

    m_context3D->enable(GraphicsContext3D::STENCIL_TEST);

    // Make sure we don't do any actual drawing.
    m_context3D->stencilFunc(GraphicsContext3D::NEVER, stencilIndex, stencilIndex);

    // Operate only on the stencilIndex and above.
    m_context3D->stencilMask(0xff & ~(stencilIndex - 1));

    // First clear the entire buffer at the current index.
    program->setMatrix(program->projectionMatrixLocation(), fullProjectionMatrix);
    program->setMatrix(program->modelViewMatrixLocation(), TransformationMatrix());
    m_context3D->stencilOp(GraphicsContext3D::ZERO, GraphicsContext3D::ZERO, GraphicsContext3D::ZERO);
    m_context3D->drawArrays(GraphicsContext3D::TRIANGLE_FAN, 0, 4);

    // Now apply the current index to the new quad.
    m_context3D->stencilOp(GraphicsContext3D::REPLACE, GraphicsContext3D::REPLACE, GraphicsContext3D::REPLACE);
    program->setMatrix(program->projectionMatrixLocation(), data().projectionMatrix);
    program->setMatrix(program->modelViewMatrixLocation(), matrix);
    m_context3D->drawArrays(GraphicsContext3D::TRIANGLE_FAN, 0, 4);

    // Clear the state.
    m_context3D->disableVertexAttribArray(program->vertexLocation());
    m_context3D->stencilMask(0);

    // Increase stencilIndex and apply stencil testing.
    clipStack().setStencilIndex(stencilIndex * 2);
    clipStack().applyIfNeeded(m_context3D.get());
}

void TextureMapperGL::endClip()
{
    clipStack().pop();
    clipStack().applyIfNeeded(m_context3D.get());
}

IntRect TextureMapperGL::clipBounds()
{
    return clipStack().current().scissorBox;
}

PassRefPtr<BitmapTexture> TextureMapperGL::createTexture()
{
    BitmapTextureGL* texture = new BitmapTextureGL(this);
    return adoptRef(texture);
}

PassOwnPtr<TextureMapper> TextureMapper::platformCreateAccelerated()
{
    return TextureMapperGL::create();
}

};
#endif
