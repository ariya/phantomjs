/*
 * Copyright (C) 2004, 2006, 2007, 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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

#ifndef RenderHTMLCanvas_h
#define RenderHTMLCanvas_h

#include "RenderReplaced.h"

namespace WebCore {

class HTMLCanvasElement;

class RenderHTMLCanvas : public RenderReplaced {
public:
    explicit RenderHTMLCanvas(HTMLCanvasElement*);

    virtual bool isCanvas() const { return true; }
    virtual bool requiresLayer() const;

    void canvasSizeChanged();
    
private:
    virtual const char* renderName() const { return "RenderHTMLCanvas"; }
    virtual void paintReplaced(PaintInfo&, int tx, int ty);
    virtual void intrinsicSizeChanged() { canvasSizeChanged(); }
};

inline RenderHTMLCanvas* toRenderHTMLCanvas(RenderObject* object)
{
    ASSERT(!object || !strcmp(object->renderName(), "RenderHTMLCanvas"));
    return static_cast<RenderHTMLCanvas*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderHTMLCanvas(const RenderHTMLCanvas*);

} // namespace WebCore

#endif // RenderHTMLCanvas_h
