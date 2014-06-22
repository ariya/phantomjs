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

#ifndef RenderRubyRun_h
#define RenderRubyRun_h

#include "RenderBlock.h"

namespace WebCore {

class RenderRubyBase;
class RenderRubyText;

// RenderRubyRun are 'inline-block/table' like objects,and wrap a single pairing of a ruby base with its ruby text(s).
// See RenderRuby.h for further comments on the structure

class RenderRubyRun : public RenderBlock {
public:
    virtual ~RenderRubyRun();

    bool hasRubyText() const;
    bool hasRubyBase() const;
    bool isEmpty() const;
    RenderRubyText* rubyText() const;
    RenderRubyBase* rubyBase() const;
    RenderRubyBase* rubyBaseSafe(); // creates the base if it doesn't already exist

    virtual RenderObject* layoutSpecialExcludedChild(bool relayoutChildren);
    virtual void layout();

    virtual bool isChildAllowed(RenderObject*, RenderStyle*) const;
    virtual void addChild(RenderObject* child, RenderObject* beforeChild = 0);
    virtual void removeChild(RenderObject* child);

    virtual RenderBlock* firstLineBlock() const;
    virtual void updateFirstLetter();

    void getOverhang(bool firstLine, RenderObject* startRenderer, RenderObject* endRenderer, int& startOverhang, int& endOverhang) const;

    static RenderRubyRun* staticCreateRubyRun(const RenderObject* parentRuby);

protected:
    RenderRubyBase* createRubyBase() const;

private:
    RenderRubyRun();

    virtual bool isRubyRun() const { return true; }
    virtual const char* renderName() const { return "RenderRubyRun (anonymous)"; }
    virtual bool createsAnonymousWrapper() const { return true; }
    virtual void removeLeftoverAnonymousBlock(RenderBlock*) { }
};

inline RenderRubyRun* toRenderRubyRun(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRubyRun());
    return static_cast<RenderRubyRun*>(object);
}

inline const RenderRubyRun* toRenderRubyRun(const RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isBox());
    return static_cast<const RenderRubyRun*>(object);
}

void toRenderRubyRun(const RenderRubyRun*);

} // namespace WebCore

#endif // RenderRubyRun_h
