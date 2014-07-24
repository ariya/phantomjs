/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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
#include "EGLImageLayerCompositingThreadClient.h"

#if USE(ACCELERATED_COMPOSITING)

#include "LayerCompositingThread.h"
#include <BlackBerryPlatformGLES2Program.h>
#include <BlackBerryPlatformGLES2SharedTexture.h>
#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformLog.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

using namespace BlackBerry::Platform::Graphics;

namespace WebCore {

EGLImageLayerCompositingThreadClient::~EGLImageLayerCompositingThreadClient()
{
    // Someone should have called deleteTextures() by now
    ASSERT(!m_textureAccessor);
}

void EGLImageLayerCompositingThreadClient::uploadTexturesIfNeeded(LayerCompositingThread*)
{
}

void EGLImageLayerCompositingThreadClient::drawTextures(LayerCompositingThread* layer, const GLES2Program& program, double /*scale*/, const FloatRect& /*clipRect*/)
{
    if (!m_textureAccessor || !m_textureAccessor->textureID())
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program.m_program);
    glUniform1f(program.opacityLocation(), layer->drawOpacity());
    glVertexAttribPointer(program.positionLocation(), 2, GL_FLOAT, GL_FALSE, 0, layer->transformedBounds().data());
    glVertexAttribPointer(program.texCoordLocation(), 2, GL_FLOAT, GL_FALSE, 0, layer->textureCoordinates(LayerCompositingThread::UpsideDown).data());
    glBindTexture(GL_TEXTURE_2D, m_textureAccessor->textureID());
    glDrawArrays(GL_TRIANGLE_FAN, 0, layer->transformedBounds().size());
}

void EGLImageLayerCompositingThreadClient::deleteTextures(LayerCompositingThread*)
{
    delete m_textureAccessor;
    m_textureAccessor = 0;
}

void EGLImageLayerCompositingThreadClient::bindContentsTexture(LayerCompositingThread*)
{
    if (m_textureAccessor)
        glBindTexture(GL_TEXTURE_2D, m_textureAccessor->textureID());
}

void EGLImageLayerCompositingThreadClient::setTextureAccessor(GLES2SharedTextureAccessor* textureAccessor)
{
    if (m_textureAccessor == textureAccessor)
        return;

    delete m_textureAccessor;
    m_textureAccessor = textureAccessor;
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
