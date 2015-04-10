/*
 * Copyright (C) 2012 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DOMWindowExtension.h"

#include "DOMWindow.h"
#include "DOMWrapperWorld.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"

namespace WebCore {

DOMWindowExtension::DOMWindowExtension(Frame* frame, DOMWrapperWorld* world)
    : DOMWindowProperty(frame)
    , m_world(world)
    , m_wasDetached(false)
{
    ASSERT(this->frame());
    ASSERT(m_world);
}

void DOMWindowExtension::disconnectFrameForPageCache()
{
    // Calling out to the client might result in this DOMWindowExtension being destroyed
    // while there is still work to do.
    RefPtr<DOMWindowExtension> protector = this;
    
    Frame* frame = this->frame();
    frame->loader()->client()->dispatchWillDisconnectDOMWindowExtensionFromGlobalObject(this);

    m_disconnectedFrame = frame;

    DOMWindowProperty::disconnectFrameForPageCache();
}

void DOMWindowExtension::reconnectFrameFromPageCache(Frame* frame)
{
    ASSERT(m_disconnectedFrame == frame);
    
    DOMWindowProperty::reconnectFrameFromPageCache(frame);
    m_disconnectedFrame = 0;

    this->frame()->loader()->client()->dispatchDidReconnectDOMWindowExtensionToGlobalObject(this);
}

void DOMWindowExtension::willDestroyGlobalObjectInCachedFrame()
{
    ASSERT(m_disconnectedFrame);

    // Calling out to the client might result in this DOMWindowExtension being destroyed
    // while there is still work to do.
    RefPtr<DOMWindowExtension> protector = this;
    
    m_disconnectedFrame->loader()->client()->dispatchWillDestroyGlobalObjectForDOMWindowExtension(this);
    m_disconnectedFrame = 0;

    DOMWindowProperty::willDestroyGlobalObjectInCachedFrame();
}

void DOMWindowExtension::willDestroyGlobalObjectInFrame()
{
    ASSERT(!m_disconnectedFrame);

    // Calling out to the client might result in this DOMWindowExtension being destroyed
    // while there is still work to do.
    RefPtr<DOMWindowExtension> protector = this;

    if (!m_wasDetached) {
        Frame* frame = this->frame();
        ASSERT(frame);
        frame->loader()->client()->dispatchWillDestroyGlobalObjectForDOMWindowExtension(this);
    }

    DOMWindowProperty::willDestroyGlobalObjectInFrame();
}

void DOMWindowExtension::willDetachGlobalObjectFromFrame()
{
    ASSERT(!m_disconnectedFrame);
    ASSERT(!m_wasDetached);

    // Calling out to the client might result in this DOMWindowExtension being destroyed
    // while there is still work to do.
    RefPtr<DOMWindowExtension> protector = this;

    Frame* frame = this->frame();
    ASSERT(frame);
    frame->loader()->client()->dispatchWillDestroyGlobalObjectForDOMWindowExtension(this);

    m_wasDetached = true;
    DOMWindowProperty::willDetachGlobalObjectFromFrame();
}

} // namespace WebCore
