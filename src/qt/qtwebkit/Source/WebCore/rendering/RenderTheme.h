/*
 * This file is part of the theme implementation for form controls in WebCore.
 *
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2012 Apple Computer, Inc.
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

#ifndef RenderTheme_h
#define RenderTheme_h

#if USE(NEW_THEME)
#include "Theme.h"
#else
#include "ThemeTypes.h"
#endif
#include "RenderObject.h"
#include "ScrollTypes.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class CSSStyleSheet;
class Element;
class FileList;
class HTMLInputElement;
class PopupMenu;
class RenderMenuList;
#if ENABLE(METER_ELEMENT)
class RenderMeter;
#endif
#if ENABLE(PROGRESS_ELEMENT)
class RenderProgress;
#endif
class RenderSnapshottedPlugIn;

class RenderTheme : public RefCounted<RenderTheme> {
protected:
    RenderTheme();

public:
    virtual ~RenderTheme() { }

    // This function is to be implemented in your platform-specific theme implementation to hand back the
    // appropriate platform theme. When the theme is needed in non-page dependent code, a default theme is
    // used as fallback, which is returned for a nulled page, so the platform code needs to account for this.
    static PassRefPtr<RenderTheme> themeForPage(Page* page);

    // When the theme is needed in non-page dependent code, the defaultTheme() is used as fallback.
    static inline PassRefPtr<RenderTheme> defaultTheme()
    {
        return themeForPage(0);
    };

    // This method is called whenever style has been computed for an element and the appearance
    // property has been set to a value other than "none".  The theme should map in all of the appropriate
    // metrics and defaults given the contents of the style.  This includes sophisticated operations like
    // selection of control size based off the font, the disabling of appearance when certain other properties like
    // "border" are set, or if the appearance is not supported by the theme.
    void adjustStyle(StyleResolver*, RenderStyle*, Element*,  bool UAHasAppearance,
                     const BorderData&, const FillLayer&, const Color& backgroundColor);

    // This method is called to paint the widget as a background of the RenderObject.  A widget's foreground, e.g., the
    // text of a button, is always rendered by the engine itself.  The boolean return value indicates
    // whether the CSS border/background should also be painted.
    bool paint(RenderObject*, const PaintInfo&, const IntRect&);
    bool paintBorderOnly(RenderObject*, const PaintInfo&, const IntRect&);
    bool paintDecorations(RenderObject*, const PaintInfo&, const IntRect&);

    // The remaining methods should be implemented by the platform-specific portion of the theme, e.g.,
    // RenderThemeMac.cpp for Mac OS X.

    // These methods return the theme's extra style sheets rules, to let each platform
    // adjust the default CSS rules in html.css, quirks.css, mediaControls.css, or plugIns.css
    virtual String extraDefaultStyleSheet() { return String(); }
    virtual String extraQuirksStyleSheet() { return String(); }
    virtual String extraPlugInsStyleSheet() { return String(); }
#if ENABLE(VIDEO)
    virtual String extraMediaControlsStyleSheet() { return String(); }
#endif
#if ENABLE(FULLSCREEN_API)
    virtual String extraFullScreenStyleSheet() { return String(); }
#endif

    // A method to obtain the baseline position for a "leaf" control.  This will only be used if a baseline
    // position cannot be determined by examining child content. Checkboxes and radio buttons are examples of
    // controls that need to do this.
    virtual int baselinePosition(const RenderObject*) const;

    // A method for asking if a control is a container or not.  Leaf controls have to have some special behavior (like
    // the baseline position API above).
    bool isControlContainer(ControlPart) const;

    // A method asking if the control changes its tint when the window has focus or not.
    virtual bool controlSupportsTints(const RenderObject*) const { return false; }

    // Whether or not the control has been styled enough by the author to disable the native appearance.
    virtual bool isControlStyled(const RenderStyle*, const BorderData&, const FillLayer&, const Color& backgroundColor) const;

    // A general method asking if any control tinting is supported at all.
    virtual bool supportsControlTints() const { return false; }

    // Some controls may spill out of their containers (e.g., the check on an OS X checkbox).  When these controls repaint,
    // the theme needs to communicate this inflated rect to the engine so that it can invalidate the whole control.
    virtual void adjustRepaintRect(const RenderObject*, IntRect&);

    // This method is called whenever a relevant state changes on a particular themed object, e.g., the mouse becomes pressed
    // or a control becomes disabled.
    virtual bool stateChanged(RenderObject*, ControlState) const;

    // This method is called whenever the theme changes on the system in order to flush cached resources from the
    // old theme.
    virtual void themeChanged() { }

    // A method asking if the theme is able to draw the focus ring.
    virtual bool supportsFocusRing(const RenderStyle*) const;

    // A method asking if the theme's controls actually care about redrawing when hovered.
    virtual bool supportsHover(const RenderStyle*) const { return false; }

    // A method asking if the platform is able to show datalist suggestions for a given input type.
    virtual bool supportsDataListUI(const AtomicString&) const { return false; }

    // Text selection colors.
    Color activeSelectionBackgroundColor() const;
    Color inactiveSelectionBackgroundColor() const;
    Color activeSelectionForegroundColor() const;
    Color inactiveSelectionForegroundColor() const;

    // List box selection colors
    Color activeListBoxSelectionBackgroundColor() const;
    Color activeListBoxSelectionForegroundColor() const;
    Color inactiveListBoxSelectionBackgroundColor() const;
    Color inactiveListBoxSelectionForegroundColor() const;

    // Highlighting colors for TextMatches.
    virtual Color platformActiveTextSearchHighlightColor() const;
    virtual Color platformInactiveTextSearchHighlightColor() const;

    virtual Color disabledTextColor(const Color& textColor, const Color& backgroundColor) const;

    static Color focusRingColor();
    virtual Color platformFocusRingColor() const { return Color(0, 0, 0); }
    static void setCustomFocusRingColor(const Color&);
#if ENABLE(TOUCH_EVENTS)
    static Color tapHighlightColor();
    virtual Color platformTapHighlightColor() const { return RenderTheme::defaultTapHighlightColor; }
#endif
    virtual void platformColorsDidChange();

    virtual double caretBlinkInterval() const { return 0.5; }

    // System fonts and colors for CSS.
    virtual void systemFont(CSSValueID, FontDescription&) const = 0;
    virtual Color systemColor(CSSValueID) const;

    virtual int minimumMenuListSize(RenderStyle*) const { return 0; }

    virtual void adjustSliderThumbSize(RenderStyle*, Element*) const;

    virtual int popupInternalPaddingLeft(RenderStyle*) const { return 0; }
    virtual int popupInternalPaddingRight(RenderStyle*) const { return 0; }
    virtual int popupInternalPaddingTop(RenderStyle*) const { return 0; }
    virtual int popupInternalPaddingBottom(RenderStyle*) const { return 0; }
    virtual bool popupOptionSupportsTextIndent() const { return false; }

    virtual ScrollbarControlSize scrollbarControlSizeForPart(ControlPart) { return RegularScrollbar; }

    // Method for painting the caps lock indicator
    virtual bool paintCapsLockIndicator(RenderObject*, const PaintInfo&, const IntRect&) { return 0; };

#if ENABLE(PROGRESS_ELEMENT)
    // Returns the repeat interval of the animation for the progress bar.
    virtual double animationRepeatIntervalForProgressBar(RenderProgress*) const;
    // Returns the duration of the animation for the progress bar.
    virtual double animationDurationForProgressBar(RenderProgress*) const;
#endif

#if ENABLE(VIDEO)
    // Media controls
    virtual bool supportsClosedCaptioning() const { return false; }
    virtual bool hasOwnDisabledStateHandlingFor(ControlPart) const { return false; }
    virtual bool usesMediaControlStatusDisplay() { return false; }
    virtual bool usesMediaControlVolumeSlider() const { return true; }
    virtual bool usesVerticalVolumeSlider() const { return true; }
    virtual double mediaControlsFadeInDuration() { return 0.1; }
    virtual double mediaControlsFadeOutDuration() { return 0.3; }
    virtual String formatMediaControlsTime(float time) const;
    virtual String formatMediaControlsCurrentTime(float currentTime, float duration) const;
    virtual String formatMediaControlsRemainingTime(float currentTime, float duration) const;
    
    // Returns the media volume slider container's offset from the mute button.
    virtual IntPoint volumeSliderOffsetFromMuteButton(RenderBox*, const IntSize&) const;
#endif

#if ENABLE(METER_ELEMENT)
    virtual IntSize meterSizeForBounds(const RenderMeter*, const IntRect&) const;
    virtual bool supportsMeter(ControlPart) const;
#endif

#if ENABLE(DATALIST_ELEMENT)
    // Returns the threshold distance for snapping to a slider tick mark.
    virtual LayoutUnit sliderTickSnappingThreshold() const;
    // Returns size of one slider tick mark for a horizontal track.
    // For vertical tracks we rotate it and use it. i.e. Width is always length along the track.
    virtual IntSize sliderTickSize() const = 0;
    // Returns the distance of slider tick origin from the slider track center.
    virtual int sliderTickOffsetFromTrackCenter() const = 0;
    void paintSliderTicks(RenderObject*, const PaintInfo&, const IntRect&);
#endif

    virtual bool shouldShowPlaceholderWhenFocused() const { return false; }
    virtual bool shouldHaveSpinButton(HTMLInputElement*) const;

    // Functions for <select> elements.
    virtual bool delegatesMenuListRendering() const { return false; }
    virtual bool popsMenuByArrowKeys() const { return false; }
    virtual bool popsMenuBySpaceOrReturn() const { return false; }

    virtual String fileListDefaultLabel(bool multipleFilesAllowed) const;
    virtual String fileListNameForWidth(const FileList*, const Font&, int width, bool multipleFilesAllowed) const;

    virtual bool shouldOpenPickerWithF4Key() const;

protected:
    // The platform selection color.
    virtual Color platformActiveSelectionBackgroundColor() const;
    virtual Color platformInactiveSelectionBackgroundColor() const;
    virtual Color platformActiveSelectionForegroundColor() const;
    virtual Color platformInactiveSelectionForegroundColor() const;

    virtual Color platformActiveListBoxSelectionBackgroundColor() const;
    virtual Color platformInactiveListBoxSelectionBackgroundColor() const;
    virtual Color platformActiveListBoxSelectionForegroundColor() const;
    virtual Color platformInactiveListBoxSelectionForegroundColor() const;

    virtual bool supportsSelectionForegroundColors() const { return true; }
    virtual bool supportsListBoxSelectionForegroundColors() const { return true; }

#if !USE(NEW_THEME)
    // Methods for each appearance value.
    virtual void adjustCheckboxStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintCheckbox(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual void setCheckboxSize(RenderStyle*) const { }

    virtual void adjustRadioStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintRadio(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual void setRadioSize(RenderStyle*) const { }

    virtual void adjustButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintButton(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual void setButtonSize(RenderStyle*) const { }

    virtual void adjustInnerSpinButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintInnerSpinButton(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
#endif

    virtual void adjustTextFieldStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintTextField(RenderObject*, const PaintInfo&, const IntRect&) { return true; }

    virtual void adjustTextAreaStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintTextArea(RenderObject*, const PaintInfo&, const IntRect&) { return true; }

    virtual void adjustMenuListStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintMenuList(RenderObject*, const PaintInfo&, const IntRect&) { return true; }

    virtual void adjustMenuListButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintMenuListButton(RenderObject*, const PaintInfo&, const IntRect&) { return true; }

#if ENABLE(METER_ELEMENT)
    virtual void adjustMeterStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintMeter(RenderObject*, const PaintInfo&, const IntRect&);
#endif

#if ENABLE(PROGRESS_ELEMENT)
    virtual void adjustProgressBarStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintProgressBar(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
#endif

#if ENABLE(INPUT_SPEECH)
    virtual void adjustInputFieldSpeechButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintInputFieldSpeechButton(RenderObject*, const PaintInfo&, const IntRect&);
#endif

    virtual void adjustSliderTrackStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSliderTrack(RenderObject*, const PaintInfo&, const IntRect&) { return true; }

    virtual void adjustSliderThumbStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSliderThumb(RenderObject*, const PaintInfo&, const IntRect&) { return true; }

    virtual void adjustSearchFieldStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchField(RenderObject*, const PaintInfo&, const IntRect&) { return true; }

    virtual void adjustSearchFieldCancelButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldCancelButton(RenderObject*, const PaintInfo&, const IntRect&) { return true; }

    virtual void adjustSearchFieldDecorationStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldDecoration(RenderObject*, const PaintInfo&, const IntRect&) { return true; }

    virtual void adjustSearchFieldResultsDecorationStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldResultsDecoration(RenderObject*, const PaintInfo&, const IntRect&) { return true; }

    virtual void adjustSearchFieldResultsButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldResultsButton(RenderObject*, const PaintInfo&, const IntRect&) { return true; }

    virtual void adjustMediaControlStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintMediaFullscreenButton(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaPlayButton(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaOverlayPlayButton(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaMuteButton(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaSeekBackButton(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaSeekForwardButton(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaSliderTrack(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaSliderThumb(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaVolumeSliderContainer(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaVolumeSliderTrack(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaVolumeSliderThumb(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaRewindButton(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaReturnToRealtimeButton(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaToggleClosedCaptionsButton(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaControlsBackground(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaCurrentTime(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaTimeRemaining(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaFullScreenVolumeSliderTrack(RenderObject*, const PaintInfo&, const IntRect&) { return true; }
    virtual bool paintMediaFullScreenVolumeSliderThumb(RenderObject*, const PaintInfo&, const IntRect&) { return true; }

    virtual bool paintSnapshottedPluginOverlay(RenderObject*, const PaintInfo&, const IntRect&) { return true; }

public:
    // Methods for state querying
    ControlStates controlStatesForRenderer(const RenderObject* o) const;
    bool isActive(const RenderObject*) const;
    bool isChecked(const RenderObject*) const;
    bool isIndeterminate(const RenderObject*) const;
    bool isEnabled(const RenderObject*) const;
    bool isFocused(const RenderObject*) const;
    bool isPressed(const RenderObject*) const;
    bool isSpinUpButtonPartPressed(const RenderObject*) const;
    bool isHovered(const RenderObject*) const;
    bool isSpinUpButtonPartHovered(const RenderObject*) const;
    bool isReadOnlyControl(const RenderObject*) const;
    bool isDefault(const RenderObject*) const;

private:
    mutable Color m_activeSelectionBackgroundColor;
    mutable Color m_inactiveSelectionBackgroundColor;
    mutable Color m_activeSelectionForegroundColor;
    mutable Color m_inactiveSelectionForegroundColor;

    mutable Color m_activeListBoxSelectionBackgroundColor;
    mutable Color m_inactiveListBoxSelectionBackgroundColor;
    mutable Color m_activeListBoxSelectionForegroundColor;
    mutable Color m_inactiveListBoxSelectionForegroundColor;

#if ENABLE(TOUCH_EVENTS)
    // This color is expected to be drawn on a semi-transparent overlay,
    // making it more transparent than its alpha value indicates.
    static const RGBA32 defaultTapHighlightColor = 0x66000000;
#endif

#if USE(NEW_THEME)
    Theme* m_theme; // The platform-specific theme.
#endif
};

} // namespace WebCore

#endif // RenderTheme_h
