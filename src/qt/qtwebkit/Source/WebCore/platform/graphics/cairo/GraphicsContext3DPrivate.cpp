/*
 * Copyright (C) 2011, 2012 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#include "config.h"
#include "GraphicsContext3DPrivate.h"

#if USE(3D_GRAPHICS)

#include "HostWindow.h"
#include "NotImplemented.h"
#include "PlatformContextCairo.h"
#include <wtf/OwnArrayPtr.h>

#if USE(OPENGL_ES_2)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include "OpenGLShims.h"
#endif

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER) && USE(TEXTURE_MAPPER_GL)
#include <texmap/TextureMapperGL.h>
#endif

using namespace std;

namespace WebCore {

PassOwnPtr<GraphicsContext3DPrivate> GraphicsContext3DPrivate::create(GraphicsContext3D* context, GraphicsContext3D::RenderStyle renderStyle)
{
    return adoptPtr(new GraphicsContext3DPrivate(context, renderStyle));
}

GraphicsContext3DPrivate::GraphicsContext3DPrivate(GraphicsContext3D* context, GraphicsContext3D::RenderStyle renderStyle)
    : m_context(context)
    , m_renderStyle(renderStyle)
{
    switch (renderStyle) {
    case GraphicsContext3D::RenderOffscreen:
        m_glContext = GLContext::createOffscreenContext(GLContext::sharingContext());
        break;
    case GraphicsContext3D::RenderToCurrentGLContext:
        break;
    case GraphicsContext3D::RenderDirectlyToHostWindow:
        ASSERT_NOT_REACHED();
        break;
    }
}

GraphicsContext3DPrivate::~GraphicsContext3DPrivate()
{
}

bool GraphicsContext3DPrivate::makeContextCurrent()
{
    return m_glContext ? m_glContext->makeContextCurrent() : false;
}

PlatformGraphicsContext3D GraphicsContext3DPrivate::platformContext()
{
    return m_glContext ? m_glContext->platformContext() : GLContext::getCurrent()->platformContext();
}

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
void GraphicsContext3DPrivate::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity)
{
    if (!m_glContext)
        return;

    ASSERT(m_renderStyle == GraphicsContext3D::RenderOffscreen);

    m_context->markLayerComposited();

    // FIXME: We do not support mask for the moment with TextureMapperImageBuffer.
    if (textureMapper->accelerationMode() != TextureMapper::OpenGLMode) {
        GraphicsContext* context = textureMapper->graphicsContext();
        context->save();
        context->platformContext()->setGlobalAlpha(opacity);

        const int height = m_context->m_currentHeight;
        const int width = m_context->m_currentWidth;
        int totalBytes = width * height * 4;

        OwnArrayPtr<unsigned char> pixels = adoptArrayPtr(new unsigned char[totalBytes]);
        if (!pixels)
            return;

        // OpenGL keeps the pixels stored bottom up, so we need to flip the image here.
        context->translate(0, height);
        context->scale(FloatSize(1, -1));

        context->concatCTM(matrix.toAffineTransform());

        m_context->readRenderingResults(pixels.get(), totalBytes);

        // Premultiply alpha.
        for (int i = 0; i < totalBytes; i += 4)
            if (pixels[i + 3] != 255) {
                pixels[i + 0] = min(255, pixels[i + 0] * pixels[i + 3] / 255);
                pixels[i + 1] = min(255, pixels[i + 1] * pixels[i + 3] / 255);
                pixels[i + 2] = min(255, pixels[i + 2] * pixels[i + 3] / 255);
            }

        RefPtr<cairo_surface_t> imageSurface = adoptRef(cairo_image_surface_create_for_data(
            const_cast<unsigned char*>(pixels.get()), CAIRO_FORMAT_ARGB32, width, height, width * 4));

        context->platformContext()->drawSurfaceToContext(imageSurface.get(), targetRect, IntRect(0, 0, width, height), context);

        context->restore();
        return;
    }

#if USE(TEXTURE_MAPPER_GL)
    if (m_context->m_attrs.antialias && m_context->m_state.boundFBO == m_context->m_multisampleFBO) {
        GLContext* previousActiveContext = GLContext::getCurrent();
        if (previousActiveContext != m_glContext)
            m_context->makeContextCurrent();

        m_context->resolveMultisamplingIfNecessary();
        ::glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_context->m_state.boundFBO);

        if (previousActiveContext && previousActiveContext != m_glContext)
            previousActiveContext->makeContextCurrent();
    }

    TextureMapperGL* texmapGL = static_cast<TextureMapperGL*>(textureMapper);
    TextureMapperGL::Flags flags = TextureMapperGL::ShouldFlipTexture | (m_context->m_attrs.alpha ? TextureMapperGL::ShouldBlend : 0);
    IntSize textureSize(m_context->m_currentWidth, m_context->m_currentHeight);
    texmapGL->drawTexture(m_context->m_texture, flags, textureSize, targetRect, matrix, opacity);
#endif // USE(ACCELERATED_COMPOSITING_GL)
}
#endif // USE(ACCELERATED_COMPOSITING)

} // namespace WebCore

#endif // USE(3D_GRAPHICS)
