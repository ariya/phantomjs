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

#ifndef ScrollbarThemeComposite_h
#define ScrollbarThemeComposite_h

#include "ScrollbarTheme.h"

namespace WebCore {

class ScrollbarThemeComposite : public ScrollbarTheme {
public:
    // Implement ScrollbarTheme interface
    virtual bool paint(ScrollbarThemeClient*, GraphicsContext*, const IntRect& damageRect);
    virtual ScrollbarPart hitTest(ScrollbarThemeClient*, const IntPoint&);
    virtual void invalidatePart(ScrollbarThemeClient*, ScrollbarPart);
    virtual int thumbPosition(ScrollbarThemeClient*);
    virtual int thumbLength(ScrollbarThemeClient*);
    virtual int trackPosition(ScrollbarThemeClient*);
    virtual int trackLength(ScrollbarThemeClient*);
    virtual void paintScrollCorner(ScrollView*, GraphicsContext*, const IntRect& cornerRect);
    virtual void paintOverhangAreas(ScrollView*, GraphicsContext*, const IntRect& horizontalOverhangArea, const IntRect& verticalOverhangArea, const IntRect& dirtyRect);

    virtual bool hasButtons(ScrollbarThemeClient*) = 0;
    virtual bool hasThumb(ScrollbarThemeClient*) = 0;

    virtual IntRect backButtonRect(ScrollbarThemeClient*, ScrollbarPart, bool painting = false) = 0;
    virtual IntRect forwardButtonRect(ScrollbarThemeClient*, ScrollbarPart, bool painting = false) = 0;
    virtual IntRect trackRect(ScrollbarThemeClient*, bool painting = false) = 0;
    virtual IntRect thumbRect(ScrollbarThemeClient*);

    virtual void splitTrack(ScrollbarThemeClient*, const IntRect& track, IntRect& startTrack, IntRect& thumb, IntRect& endTrack);
    
    virtual int minimumThumbLength(ScrollbarThemeClient*);

    virtual void paintScrollbarBackground(GraphicsContext*, ScrollbarThemeClient*) { }
    virtual void paintTrackBackground(GraphicsContext*, ScrollbarThemeClient*, const IntRect&) { }
    virtual void paintTrackPiece(GraphicsContext*, ScrollbarThemeClient*, const IntRect&, ScrollbarPart) { }
    virtual void paintButton(GraphicsContext*, ScrollbarThemeClient*, const IntRect&, ScrollbarPart) { }
    virtual void paintThumb(GraphicsContext*, ScrollbarThemeClient*, const IntRect&) { }

    virtual IntRect constrainTrackRectToTrackPieces(ScrollbarThemeClient*, const IntRect& rect) { return rect; }
};

}
#endif
