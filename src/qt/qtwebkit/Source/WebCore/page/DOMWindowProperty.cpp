/*
 * Copyright (C) 2011 Google, Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
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
#include "DOMWindowProperty.h"

#include "DOMWindow.h"
#include "Document.h"
#include "Frame.h"

namespace WebCore {

DOMWindowProperty::DOMWindowProperty(Frame* frame)
    : m_frame(frame)
    , m_associatedDOMWindow(0)
{
    // FIXME: For now it *is* acceptable for a DOMWindowProperty to be created with a null frame.
    // See fast/dom/navigator-detached-no-crash.html for the recipe.
    // We should fix that.  <rdar://problem/11567132>
    if (m_frame) {
        m_associatedDOMWindow = m_frame->document()->domWindow();
        m_associatedDOMWindow->registerProperty(this);
    }
}

DOMWindowProperty::~DOMWindowProperty()
{
    if (m_associatedDOMWindow)
        m_associatedDOMWindow->unregisterProperty(this);

    m_associatedDOMWindow = 0;
    m_frame = 0;
}

void DOMWindowProperty::disconnectFrameForPageCache()
{
    // If this property is being disconnected from its Frame to enter the PageCache, it must have
    // been created with a Frame in the first place.
    ASSERT(m_frame);
    ASSERT(m_associatedDOMWindow);

    m_frame = 0;
}

void DOMWindowProperty::reconnectFrameFromPageCache(Frame* frame)
{
    // If this property is being reconnected to its Frame to enter the PageCache, it must have
    // been disconnected from its Frame in the first place and it should still have an associated DOMWindow.
    ASSERT(frame);
    ASSERT(!m_frame);
    ASSERT(frame->document()->domWindow() == m_associatedDOMWindow);

    m_frame = frame;
}

void DOMWindowProperty::willDestroyGlobalObjectInCachedFrame()
{
    // If the property has been disconnected from its Frame for the page cache, then it must have originally had a Frame
    // and therefore should still have an associated DOMWindow.
    ASSERT(!m_frame);
    ASSERT(m_associatedDOMWindow);

    // DOMWindowProperty lifetime isn't tied directly to the DOMWindow itself so it is important that it unregister
    // itself from any DOMWindow it is associated with if that DOMWindow is going away.
    if (m_associatedDOMWindow)
        m_associatedDOMWindow->unregisterProperty(this);
    m_associatedDOMWindow = 0;
    m_frame = 0;
}

void DOMWindowProperty::willDestroyGlobalObjectInFrame()
{
    // If the property is getting this callback it must have been created with a Frame/DOMWindow and it should still have them.
    ASSERT(m_frame);
    ASSERT(m_associatedDOMWindow);

    // DOMWindowProperty lifetime isn't tied directly to the DOMWindow itself so it is important that it unregister
    // itself from any DOMWindow it is associated with if that DOMWindow is going away.
    if (m_associatedDOMWindow)
        m_associatedDOMWindow->unregisterProperty(this);
    m_associatedDOMWindow = 0;
    m_frame = 0;
}

void DOMWindowProperty::willDetachGlobalObjectFromFrame()
{
    // If the property is getting this callback it must have been created with a Frame/DOMWindow and it should still have them.
    ASSERT(m_frame);
    ASSERT(m_associatedDOMWindow);
}

}
