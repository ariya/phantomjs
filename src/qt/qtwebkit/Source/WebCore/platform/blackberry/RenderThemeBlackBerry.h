/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef RenderThemeBlackBerry_h
#define RenderThemeBlackBerry_h

#include "RenderTheme.h"

namespace WebCore {

class RenderThemeBlackBerry : public RenderTheme {
public:
    static PassRefPtr<RenderTheme> create();
    virtual ~RenderThemeBlackBerry();

    virtual String extraDefaultStyleSheet();

#if ENABLE(VIDEO)
    virtual String extraMediaControlsStyleSheet();
    virtual bool usesVerticalVolumeSlider() const { return false; }
#endif
#if ENABLE(FULLSCREEN_API)
    virtual String extraFullScreenStyleSheet();
#endif
    virtual bool supportsHover(const RenderStyle*) const { return true; }

    virtual double caretBlinkInterval() const;

    virtual void systemFont(CSSValueID, FontDescription&) const;
    virtual bool paintCheckbox(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void setCheckboxSize(RenderStyle*) const;
    virtual bool paintRadio(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void setRadioSize(RenderStyle*) const;
    virtual bool paintButton(RenderObject*, const PaintInfo&, const IntRect&);
    void calculateButtonSize(RenderStyle*) const;
    virtual void adjustMenuListStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintMenuListButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustSliderThumbSize(RenderStyle*, Element*) const;
    virtual bool paintSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);

#if ENABLE(DATALIST_ELEMENT)
    virtual IntSize sliderTickSize() const OVERRIDE;
    virtual int sliderTickOffsetFromTrackCenter() const OVERRIDE;
#endif

#if ENABLE(TOUCH_EVENTS)
    virtual Color platformTapHighlightColor() const;
#endif

    virtual Color platformFocusRingColor() const;
    virtual bool supportsFocusRing(const RenderStyle* style) const { return style->hasAppearance(); }

    virtual void adjustButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual void adjustTextFieldStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintTextField(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustTextAreaStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintTextArea(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual void adjustSearchFieldCancelButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchField(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintSearchFieldCancelButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustMenuListButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintMenuList(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustMediaControlStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual void adjustSliderTrackStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintMediaFullscreenButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaPlayButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaMuteButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaRewindButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintProgressBar(RenderObject*, const PaintInfo&, const IntRect&);
    virtual double animationRepeatIntervalForProgressBar(RenderProgress*) const;
    virtual double animationDurationForProgressBar(RenderProgress*) const;

    virtual Color platformActiveSelectionBackgroundColor() const;

    // Highlighting colors for TextMatches.
    virtual Color platformActiveTextSearchHighlightColor() const;
    virtual Color platformInactiveTextSearchHighlightColor() const;

    virtual bool supportsDataListUI(const AtomicString&) const;

private:
    static const String& defaultGUIFont();

    // The default variable-width font size. We use this as the default font
    // size for the "system font", and as a base size (which we then shrink) for
    // form control fonts.
    static float defaultFontSize;

    RenderThemeBlackBerry();
    void setButtonStyle(RenderStyle*) const;

    bool paintTextFieldOrTextAreaOrSearchField(RenderObject*, const PaintInfo&, const IntRect&);

    bool paintSliderTrackRect(RenderObject*, const PaintInfo&, const IntRect&, Image*);

    bool paintProgressTrackRect(const PaintInfo&, const IntRect&, Image*);

    IntRect convertToPaintingRect(RenderObject* inputRenderer, const RenderObject* partRenderer, LayoutRect partRect, const IntRect& localOffset) const;

};

} // namespace WebCore

#endif // RenderThemeBlackBerry_h
