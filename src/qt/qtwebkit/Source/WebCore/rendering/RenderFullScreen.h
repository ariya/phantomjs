/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RenderFullScreen_h
#define RenderFullScreen_h

#if ENABLE(FULLSCREEN_API)

#include "RenderFlexibleBox.h"
#include "StyleInheritedData.h"

namespace WebCore {

class RenderFullScreen : public RenderFlexibleBox {
public:
    static RenderFullScreen* createAnonymous(Document*);

    virtual bool isRenderFullScreen() const { return true; }
    virtual const char* renderName() const { return "RenderFullScreen"; }

    void setPlaceholder(RenderBlock*);
    RenderBlock* placeholder() { return m_placeholder; }
    void createPlaceholder(PassRefPtr<RenderStyle>, const LayoutRect& frameRect);


    static RenderObject* wrapRenderer(RenderObject*, RenderObject*, Document*);
    void unwrapRenderer();

private:
    RenderFullScreen();
    virtual void willBeDestroyed();

protected:
    RenderBlock* m_placeholder;
};
    
inline RenderFullScreen* toRenderFullScreen(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(object->isRenderFullScreen());
    return static_cast<RenderFullScreen*>(object);
}
    
// This will catch anyone doing an unnecessary cast:
void toRenderFullScreen(RenderFullScreen*);
}

#endif

#endif
