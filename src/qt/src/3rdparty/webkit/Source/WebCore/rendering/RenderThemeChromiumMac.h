/*
 * This file is part of the theme implementation for form controls in WebCore.
 *
 * Copyright (C) 2005 Apple Computer, Inc.
 * Copyright (C) 2008, 2009 Google, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef RenderThemeChromiumMac_h
#define RenderThemeChromiumMac_h

#import "RenderThemeMac.h"

namespace WebCore {

class RenderThemeChromiumMac : public RenderThemeMac {
public:
    static PassRefPtr<RenderTheme> create();
protected:
#if ENABLE(VIDEO)
    virtual void adjustMediaSliderThumbSize(RenderObject*) const;
    virtual bool paintMediaPlayButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaMuteButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaControlsBackground(RenderObject*, const PaintInfo&, const IntRect&);
    virtual String extraMediaControlsStyleSheet();
#if ENABLE(FULLSCREEN_API)
    virtual String extraFullScreenStyleSheet();
#endif
  
    virtual bool paintMediaSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderContainer(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual IntPoint volumeSliderOffsetFromMuteButton(RenderBox*, const IntSize&) const;
    virtual bool usesMediaControlStatusDisplay() { return false; }
    virtual bool hasOwnDisabledStateHandlingFor(ControlPart) const { return true; }
#endif

    virtual bool usesTestModeFocusRingColor() const;
    virtual NSView* documentViewFor(RenderObject*) const;

    virtual int popupInternalPaddingLeft(RenderStyle*) const;
    virtual int popupInternalPaddingRight(RenderStyle*) const;
private:
    virtual void updateActiveState(NSCell*, const RenderObject*);
};

} // namespace WebCore

#endif // RenderThemeChromiumMac_h
