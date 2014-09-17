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

#ifndef ThemeMac_h
#define ThemeMac_h

#include "Theme.h"

namespace WebCore {

class ThemeMac : public Theme {
public:
    ThemeMac() { }
    virtual ~ThemeMac() { }
    
    virtual int baselinePositionAdjustment(ControlPart) const;

    virtual FontDescription controlFont(ControlPart, const Font&, float zoomFactor) const;
    
    virtual LengthSize controlSize(ControlPart, const Font&, const LengthSize&, float zoomFactor) const;
    virtual LengthSize minimumControlSize(ControlPart, const Font&, float zoomFactor) const;

    virtual LengthBox controlPadding(ControlPart, const Font&, const LengthBox& zoomedBox, float zoomFactor) const;
    virtual LengthBox controlBorder(ControlPart, const Font&, const LengthBox& zoomedBox, float zoomFactor) const;

    virtual bool controlRequiresPreWhiteSpace(ControlPart part) const { return part == PushButtonPart; }

    virtual void paint(ControlPart, ControlStates, GraphicsContext*, const IntRect&, float zoomFactor, ScrollView*) const;
    virtual void inflateControlPaintRect(ControlPart, ControlStates, IntRect&, float zoomFactor) const;

    // FIXME: Once RenderThemeMac is converted over to use Theme then this can be internal to ThemeMac.
    static NSView* ensuredView(ScrollView*);
};

} // namespace WebCore

#endif // ThemeMac_h
