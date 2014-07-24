/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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
#include "GLES2Context.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "SurfacePool.h"
#include "WebPageClient.h"
#include "WebPage_p.h"

#include <GLES2/gl2.h>

#include <wtf/Assertions.h>

using BlackBerry::Platform::Graphics::Window;

namespace BlackBerry {
namespace WebKit {

Platform::Graphics::Buffer* GLES2Context::buffer() const
{
    if (m_window)
        return m_window->buffer();

    ASSERT_NOT_REACHED();
    return 0;
}

PassOwnPtr<GLES2Context> GLES2Context::create(WebPagePrivate* page)
{
    return adoptPtr(new GLES2Context(page));
}

GLES2Context::GLES2Context(WebPagePrivate* page)
    : m_window(0)
{
    if (Window* window = page->m_client->window()) {
        if (window->windowUsage() == Window::GLES2Usage)
            m_window = window;
    }
}

GLES2Context::~GLES2Context()
{
}

Platform::IntSize GLES2Context::surfaceSize() const
{
    if (m_window)
        return m_window->surfaceSize();

    ASSERT_NOT_REACHED();
    return Platform::IntSize();
}

bool GLES2Context::makeCurrent()
{
    return Platform::Graphics::makeBufferCurrent(buffer(), Platform::Graphics::GLES2);
}

bool GLES2Context::swapBuffers()
{
    ASSERT(glGetError() == GL_NO_ERROR);

    // Nothing to do here, the backing store will swap it when the time is right.

    return true;
}

} // namespace WebKit
} // namespace BlackBerry
