/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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
#include "EGLImageLayerWebKitThread.h"

#if USE(ACCELERATED_COMPOSITING)

#include "LayerCompositingThread.h"
#include "LayerRenderer.h"
#include "SharedGraphicsContext3D.h"
#include <BlackBerryPlatformGLES2ContextState.h>
#include <BlackBerryPlatformGLES2SharedTexture.h>
#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformLog.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

using namespace BlackBerry::Platform::Graphics;

namespace WebCore {

EGLImageLayerWebKitThread::EGLImageLayerWebKitThread(LayerType type)
    : LayerWebKitThread(type, 0)
    , m_client(EGLImageLayerCompositingThreadClient::create())
    , m_needsDisplay(false)
    , m_program(0)
    , m_texture(0)
    , m_textureAccessor(0)
{
    layerCompositingThread()->setClient(m_client.get());
    setLayerProgram(LayerProgramRGBA);
}

EGLImageLayerWebKitThread::~EGLImageLayerWebKitThread()
{
    // The subclass is responsible for calling deleteFrontBuffer()
    // before we get this far.
    ASSERT(!m_program);
    ASSERT(!m_texture);
    ASSERT(!m_textureAccessor);
}

void EGLImageLayerWebKitThread::setNeedsDisplay()
{
    m_needsDisplay = true;
    setNeedsCommit();
}

void EGLImageLayerWebKitThread::updateFrontBuffer(const IntSize& size, unsigned backBufferTexture)
{
    if (!m_needsDisplay)
        return;

    if (size.isEmpty()) {
        if (size != m_size) {
            deleteFrontBuffer();
            m_size = size;
        }

        m_needsDisplay = false;
        return;
    }

    GLenum currentActiveTexture = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, reinterpret_cast<GLint*>(&currentActiveTexture));
    glActiveTexture(GL_TEXTURE0);

    {
        GLES2ContextState::TextureAndFBOStateSaver textureAndFBOStateSaver;

        if (!createTextureIfNeeded(size))
            return;

        m_needsDisplay = false;

        blitToFrontBuffer(backBufferTexture);
    }

    glActiveTexture(currentActiveTexture);
}

void EGLImageLayerWebKitThread::deleteFrontBuffer()
{
    delete m_texture;
    m_texture = 0;
    glDeleteProgram(m_program);
    m_program = 0;

    // The texture accessor is in our EGLImageLayerCompositingThreadClient's custody
    // at this point, and that object is responsible for deleting it.
    m_textureAccessor = 0;

    // If anyone tries to render us after this, we're certainly going to need display.
    m_needsDisplay = true;
}

void EGLImageLayerWebKitThread::commitPendingTextureUploads()
{
    // This call is serialized with the compositing thread, so it's safe to update the
    // image and destroy any old images.

    m_client->setTextureAccessor(m_textureAccessor);
}

bool EGLImageLayerWebKitThread::createTextureIfNeeded(const IntSize& size)
{
    if (!m_texture || size != m_size) {
        delete m_texture;
        m_texture = new GLES2SharedTexture(size);
        if (!m_texture->isValid()) {
            delete m_texture;
            m_texture = 0;
            return false;
        }

        m_textureAccessor = m_texture->createTextureAccessor();

        m_size = size;
    }

    return true;
}

void EGLImageLayerWebKitThread::createShaderIfNeeded()
{
    // Shaders for drawing the layer contents.
    static char vertexShaderString[] =
        "attribute vec4 a_position;   \n"
        "attribute vec2 a_texCoord;   \n"
        "varying vec2 v_texCoord;     \n"
        "void main()                  \n"
        "{                            \n"
        "  gl_Position = a_position;  \n"
        "  v_texCoord = a_texCoord;   \n"
        "}                            \n";

    static char fragmentShaderStringRGBA[] =
        "varying mediump vec2 v_texCoord;                   \n"
        "uniform lowp sampler2D s_texture;                  \n"
        "void main()                                        \n"
        "{                                                  \n"
        "  gl_FragColor = texture2D(s_texture, v_texCoord); \n"
        "}                                                  \n";

    if (!m_program) {
        m_program = LayerRenderer::loadShaderProgram(vertexShaderString, fragmentShaderStringRGBA);
        if (!m_program)
            return;
        glBindAttribLocation(m_program, GLES2Program::PositionAttributeIndex, "a_position");
        glBindAttribLocation(m_program, GLES2Program::TexCoordAttributeIndex, "a_texCoord");
        glLinkProgram(m_program);
        unsigned samplerLocation = glGetUniformLocation(m_program, "s_texture");
        glUseProgram(m_program);
        glUniform1i(samplerLocation, 0);
    }
}

void EGLImageLayerWebKitThread::blitToFrontBuffer(unsigned backBufferTexture)
{
    if (!backBufferTexture)
        return;

    GLES2ContextState::ProgramStateSaver programStateSaver;

    createShaderIfNeeded();

    GLES2ContextState::BufferBindingSaver bufferBindingSaver;
    GLES2ContextState::ViewportSaver viewportSaver;
    GLES2ContextState::GlobalFlagStateSaver flagStateSaver(GLES2ContextState::GlobalFlagState::DontRestoreBlendFunc);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glViewport(0, 0, m_size.width(), m_size.height());
    makeBufferCurrent(m_texture->buffer(), GLES2);
    glUseProgram(m_program);
    glBindTexture(GL_TEXTURE_2D, backBufferTexture);
    glColorMask(true, true, true, true);

    {
        GLES2ContextState::VertexAttributeStateSaver vertexAttribStateSaver;
        GLES2ContextState::VertexArrayOESStateSaver vertexArrayOESStateSaver;

        static float texcoords[4 * 2] = { 0, 0,  0, 1,  1, 1,  1, 0 };
        static float vertices[] = { -1, -1, -1, 1, 1, 1, 1, -1 };
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glEnableVertexAttribArray(GLES2Program::PositionAttributeIndex);
        glEnableVertexAttribArray(GLES2Program::TexCoordAttributeIndex);
        glVertexAttribPointer(GLES2Program::PositionAttributeIndex, 2, GL_FLOAT, GL_FALSE, 0, vertices);
        glVertexAttribPointer(GLES2Program::TexCoordAttributeIndex, 2, GL_FLOAT, GL_FALSE, 0, texcoords);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // If we don't flush, the EGLImage may never update its appearance
        glFlush();

        glDisableVertexAttribArray(GLES2Program::PositionAttributeIndex);
        glDisableVertexAttribArray(GLES2Program::TexCoordAttributeIndex);
    }
}

}

#endif // USE(ACCELERATED_COMPOSITING) && ENABLE(ACCELERATED_2D_CANVAS)
