/*
 * Copyright 2008, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(TOUCH_EVENTS)

#include "Touch.h"

#include "DOMWindow.h"
#include "Frame.h"
#include "FrameView.h"

namespace WebCore {

static int contentsX(Frame* frame)
{
    if (!frame)
        return 0;
    FrameView* frameView = frame->view();
    if (!frameView)
        return 0;
    return frameView->scrollX() / frame->pageZoomFactor() / frame->frameScaleFactor();
}

static int contentsY(Frame* frame)
{
    if (!frame)
        return 0;
    FrameView* frameView = frame->view();
    if (!frameView)
        return 0;
    return frameView->scrollY() / frame->pageZoomFactor() / frame->frameScaleFactor();
}

Touch::Touch(Frame* frame, EventTarget* target, unsigned identifier, int screenX, int screenY, int pageX, int pageY, int radiusX, int radiusY, float rotationAngle, float force)
    : m_target(target)
    , m_identifier(identifier)
    , m_clientX(pageX - contentsX(frame))
    , m_clientY(pageY - contentsY(frame))
    , m_screenX(screenX)
    , m_screenY(screenY)
    , m_pageX(pageX)
    , m_pageY(pageY)
    , m_radiusX(radiusX)
    , m_radiusY(radiusY)
    , m_rotationAngle(rotationAngle)
    , m_force(force)
{
    float scaleFactor = frame->pageZoomFactor() * frame->frameScaleFactor();
    float x = pageX * scaleFactor;
    float y = pageY * scaleFactor;
    m_absoluteLocation = roundedLayoutPoint(FloatPoint(x, y));
}

Touch::Touch(EventTarget* target, unsigned identifier, int clientX, int clientY, int screenX, int screenY, int pageX, int pageY, int radiusX, int radiusY, float rotationAngle, float force, LayoutPoint absoluteLocation)
    : m_target(target)
    , m_identifier(identifier)
    , m_clientX(clientX)
    , m_clientY(clientY)
    , m_screenX(screenX)
    , m_screenY(screenY)
    , m_pageX(pageX)
    , m_pageY(pageY)
    , m_radiusX(radiusX)
    , m_radiusY(radiusY)
    , m_rotationAngle(rotationAngle)
    , m_force(force)
    , m_absoluteLocation(absoluteLocation)
{
}

PassRefPtr<Touch> Touch::cloneWithNewTarget(EventTarget* eventTarget) const
{
    return adoptRef(new Touch(eventTarget, m_identifier, m_clientX, m_clientY, m_screenX, m_screenY, m_pageX, m_pageY, m_radiusX, m_radiusY, m_rotationAngle, m_force, m_absoluteLocation));
}

} // namespace WebCore

#endif
