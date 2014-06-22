/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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

#ifndef RenderScrollbar_h
#define RenderScrollbar_h

#include "RenderStyleConstants.h"
#include "Scrollbar.h"
#include <wtf/HashMap.h>

namespace WebCore {

class Frame;
class Node;
class RenderBox;
class RenderScrollbarPart;
class RenderStyle;

class RenderScrollbar : public Scrollbar {
protected:
    RenderScrollbar(ScrollableArea*, ScrollbarOrientation, Node*, Frame*);

public:
    friend class Scrollbar;
    static PassRefPtr<Scrollbar> createCustomScrollbar(ScrollableArea*, ScrollbarOrientation, Node*, Frame* owningFrame = 0);
    virtual ~RenderScrollbar();

    RenderBox* owningRenderer() const;

    void paintPart(GraphicsContext*, ScrollbarPart, const IntRect&);

    IntRect buttonRect(ScrollbarPart);
    IntRect trackRect(int startLength, int endLength);
    IntRect trackPieceRectWithMargins(ScrollbarPart, const IntRect&);

    int minimumThumbLength();

    virtual bool isOverlayScrollbar() const { return false; }

private:
    virtual void setParent(ScrollView*);
    virtual void setEnabled(bool);

    virtual void paint(GraphicsContext*, const IntRect& damageRect);

    virtual void setHoveredPart(ScrollbarPart);
    virtual void setPressedPart(ScrollbarPart);

    virtual void styleChanged();

    virtual bool isCustomScrollbar() const { return true; }

    void updateScrollbarParts(bool destroy = false);

    PassRefPtr<RenderStyle> getScrollbarPseudoStyle(ScrollbarPart, PseudoId);
    void updateScrollbarPart(ScrollbarPart, bool destroy = false);

    // This Scrollbar(Widget) may outlive the DOM which created it (during tear down),
    // so we keep a reference to the Node which caused this custom scrollbar creation.
    // This will not create a reference cycle as the Widget tree is owned by our containing
    // FrameView which this Node pointer can in no way keep alive. See webkit bug 80610.
    RefPtr<Node> m_owner;

    Frame* m_owningFrame;
    HashMap<unsigned, RenderScrollbarPart*> m_parts;
};

inline RenderScrollbar* toRenderScrollbar(ScrollbarThemeClient* scrollbar)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!scrollbar || scrollbar->isCustomScrollbar());
    return static_cast<RenderScrollbar*>(scrollbar);
}

// This will catch anyone doing an unnecessary cast.
void toRenderScrollbar(const RenderScrollbar*);

} // namespace WebCore

#endif // RenderScrollbar_h
