/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef RenderRuby_h
#define RenderRuby_h

#include "RenderBlock.h"
#include "RenderInline.h"

namespace WebCore {

// Following the HTML 5 spec, the box object model for a <ruby> element allows several runs of ruby
// bases with their respective ruby texts looks as follows:
//
// 1 RenderRuby object, corresponding to the whole <ruby> HTML element
//      1+ RenderRubyRun (anonymous)
//          0 or 1 RenderRubyText - shuffled to the front in order to re-use existing block layouting
//              0-n inline object(s)
//          0 or 1 RenderRubyBase - contains the inline objects that make up the ruby base
//              1-n inline object(s)
//
// Note: <rp> elements are defined as having 'display:none' and thus normally are not assigned a renderer.
//
// Generated :before/:after content is shunted into anonymous inline blocks

// <ruby> when used as 'display:inline'
class RenderRubyAsInline : public RenderInline {
public:
    RenderRubyAsInline(Node*);
    virtual ~RenderRubyAsInline();

    virtual void addChild(RenderObject* child, RenderObject* beforeChild = 0);
    virtual void removeChild(RenderObject* child);

protected:
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

private:
    virtual bool isRuby() const { return true; }
    virtual const char* renderName() const { return "RenderRuby (inline)"; }
    virtual bool createsAnonymousWrapper() const { return true; }
    virtual void removeLeftoverAnonymousBlock(RenderBlock*) { ASSERT_NOT_REACHED(); }
};

// <ruby> when used as 'display:block' or 'display:inline-block'
class RenderRubyAsBlock : public RenderBlock {
public:
    RenderRubyAsBlock(Node*);
    virtual ~RenderRubyAsBlock();

    virtual void addChild(RenderObject* child, RenderObject* beforeChild = 0);
    virtual void removeChild(RenderObject* child);

protected:
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

private:
    virtual bool isRuby() const { return true; }
    virtual const char* renderName() const { return "RenderRuby (block)"; }
    virtual bool createsAnonymousWrapper() const { return true; }
    virtual void removeLeftoverAnonymousBlock(RenderBlock*) { ASSERT_NOT_REACHED(); }
};

} // namespace WebCore

#endif // RenderRuby_h
