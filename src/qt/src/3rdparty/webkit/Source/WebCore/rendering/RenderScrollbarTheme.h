/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#ifndef RenderScrollbarTheme_h
#define RenderScrollbarTheme_h

#include "ScrollbarThemeComposite.h"

namespace WebCore {

class PlatformMouseEvent;
class Scrollbar;
class ScrollView;

class RenderScrollbarTheme : public ScrollbarThemeComposite {
public:
    virtual ~RenderScrollbarTheme() {};
    
    virtual int scrollbarThickness(ScrollbarControlSize controlSize) { return ScrollbarTheme::nativeTheme()->scrollbarThickness(controlSize); }

    virtual ScrollbarButtonsPlacement buttonsPlacement() const { return ScrollbarTheme::nativeTheme()->buttonsPlacement(); }

    virtual bool supportsControlTints() const { return true; }

    virtual void paintScrollCorner(ScrollView*, GraphicsContext* context, const IntRect& cornerRect);

    virtual bool shouldCenterOnThumb(Scrollbar* scrollbar, const PlatformMouseEvent& event) { return ScrollbarTheme::nativeTheme()->shouldCenterOnThumb(scrollbar, event); }
    
    virtual double initialAutoscrollTimerDelay() { return ScrollbarTheme::nativeTheme()->initialAutoscrollTimerDelay(); }
    virtual double autoscrollTimerDelay() { return ScrollbarTheme::nativeTheme()->autoscrollTimerDelay(); }

    virtual void registerScrollbar(Scrollbar* scrollbar) { return ScrollbarTheme::nativeTheme()->registerScrollbar(scrollbar); }
    virtual void unregisterScrollbar(Scrollbar* scrollbar) { return ScrollbarTheme::nativeTheme()->unregisterScrollbar(scrollbar); }

    virtual int minimumThumbLength(Scrollbar*);

    void buttonSizesAlongTrackAxis(Scrollbar* scrollbar, int& beforeSize, int& afterSize);
    
    static RenderScrollbarTheme* renderScrollbarTheme();

protected:
    virtual bool hasButtons(Scrollbar*);
    virtual bool hasThumb(Scrollbar*);

    virtual IntRect backButtonRect(Scrollbar*, ScrollbarPart, bool painting = false);
    virtual IntRect forwardButtonRect(Scrollbar*, ScrollbarPart, bool painting = false);
    virtual IntRect trackRect(Scrollbar*, bool painting = false);
    
    virtual void paintScrollbarBackground(GraphicsContext*, Scrollbar*);    
    virtual void paintTrackBackground(GraphicsContext*, Scrollbar*, const IntRect&);
    virtual void paintTrackPiece(GraphicsContext*, Scrollbar*, const IntRect&, ScrollbarPart);
    virtual void paintButton(GraphicsContext*, Scrollbar*, const IntRect&, ScrollbarPart);
    virtual void paintThumb(GraphicsContext*, Scrollbar*, const IntRect&);

    virtual IntRect constrainTrackRectToTrackPieces(Scrollbar*, const IntRect&);
};

} // namespace WebCore

#endif // RenderScrollbarTheme_h
