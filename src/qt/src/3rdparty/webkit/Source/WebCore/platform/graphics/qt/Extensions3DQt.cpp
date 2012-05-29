/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#if ENABLE(WEBGL)

#include "Extensions3DQt.h"

#include "GraphicsContext3D.h"
#include <QGLContext>

namespace WebCore {

Extensions3DQt::Extensions3DQt()
{
}

Extensions3DQt::~Extensions3DQt()
{
}

bool Extensions3DQt::supports(const String&)
{
    return false;
}

void Extensions3DQt::ensureEnabled(const String& name)
{
    ASSERT(supports(name));
}

bool Extensions3DQt::isEnabled(const String& name)
{
    return supports(name);
}

int Extensions3DQt::getGraphicsResetStatusARB()
{
    return GraphicsContext3D::NO_ERROR;
}

void Extensions3DQt::blitFramebuffer(long srcX0, long srcY0, long srcX1, long srcY1, long dstX0, long dstY0, long dstX1, long dstY1, unsigned long mask, unsigned long filter)
{
}

void Extensions3DQt::renderbufferStorageMultisample(unsigned long target, unsigned long samples, unsigned long internalformat, unsigned long width, unsigned long height)
{
}

Platform3DObject Extensions3DQt::createVertexArrayOES()
{
    return 0;
}

void Extensions3DQt::deleteVertexArrayOES(Platform3DObject)
{
}

GC3Dboolean Extensions3DQt::isVertexArrayOES(Platform3DObject)
{
    return GL_FALSE;
}

void Extensions3DQt::bindVertexArrayOES(Platform3DObject)
{
}

} // namespace WebCore

#endif // ENABLE(WEBGL)
