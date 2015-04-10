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

#ifndef DOMWindowExtension_h
#define DOMWindowExtension_h

#include "DOMWindowProperty.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class DOMWindowExtension;
class DOMWrapperWorld;
class Frame;

class DOMWindowExtension : public RefCounted<DOMWindowExtension>, public DOMWindowProperty {
public:
    static PassRefPtr<DOMWindowExtension> create(Frame* frame, DOMWrapperWorld* world)
    {
        return adoptRef(new DOMWindowExtension(frame, world));
    }

    virtual void disconnectFrameForPageCache() OVERRIDE;
    virtual void reconnectFrameFromPageCache(Frame*) OVERRIDE;
    virtual void willDestroyGlobalObjectInCachedFrame() OVERRIDE;
    virtual void willDestroyGlobalObjectInFrame() OVERRIDE;
    virtual void willDetachGlobalObjectFromFrame() OVERRIDE;

    DOMWrapperWorld* world() const { return m_world.get(); }

private:
    DOMWindowExtension(Frame*, DOMWrapperWorld*);

    RefPtr<DOMWrapperWorld> m_world;
    RefPtr<Frame> m_disconnectedFrame;
    bool m_wasDetached;
};

}

#endif // DOMWindowExtension_h
