/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef RenderIFrame_h
#define RenderIFrame_h

#include "RenderFrameBase.h"

namespace WebCore {

class RenderView;

class RenderIFrame : public RenderFrameBase {
public:
    explicit RenderIFrame(Element*);

    bool flattenFrame() const;
    bool isSeamless() const;

private:
    virtual LayoutUnit minPreferredLogicalWidth() const OVERRIDE;
    virtual LayoutUnit maxPreferredLogicalWidth() const OVERRIDE;

    virtual bool shouldComputeSizeAsReplaced() const OVERRIDE;
    virtual bool isInlineBlockOrInlineTable() const OVERRIDE;

    virtual void layout() OVERRIDE;

    virtual bool isRenderIFrame() const OVERRIDE { return true; }

    virtual const char* renderName() const OVERRIDE { return "RenderPartObject"; } // Lying for now to avoid breaking tests

    virtual bool requiresLayer() const OVERRIDE;

    void layoutSeamlessly();

    RenderView* contentRootRenderer() const;
};

inline RenderIFrame* toRenderIFrame(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderIFrame());
    return static_cast<RenderIFrame*>(object);
}

inline const RenderIFrame* toRenderIFrame(const RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderIFrame());
    return static_cast<const RenderIFrame*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderIFrame(const RenderIFrame*);


} // namespace WebCore

#endif // RenderIFrame_h
