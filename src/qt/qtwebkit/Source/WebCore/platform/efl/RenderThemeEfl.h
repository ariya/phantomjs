/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2010 Samsung Electronics
 * All rights reserved.
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

#ifndef RenderThemeEfl_h
#define RenderThemeEfl_h

#if ENABLE(VIDEO)
#include "MediaControlElements.h"
#endif
#include "RenderTheme.h"

#include <Eina.h>
#include <cairo.h>
#include <wtf/efl/RefPtrEfl.h>

namespace WebCore {

enum FormType { // KEEP IN SYNC WITH edjeGroupFromFormType()
    Button,
    RadioButton,
    TextField,
    CheckBox,
    ComboBox,
#if ENABLE(PROGRESS_ELEMENT)
    ProgressBar,
#endif
    SearchField,
    SearchFieldDecoration,
    SearchFieldResultsButton,
    SearchFieldResultsDecoration,
    SearchFieldCancelButton,
    SliderVertical,
    SliderHorizontal,
    SliderThumbVertical,
    SliderThumbHorizontal,
#if ENABLE(VIDEO)
    PlayPauseButton,
    MuteUnMuteButton,
    SeekForwardButton,
    SeekBackwardButton,
    FullScreenButton,
#endif
#if ENABLE(VIDEO_TRACK)
    ToggleCaptionsButton,
#endif
    Spinner,
    FormTypeLast
};

class RenderThemeEfl : public RenderTheme {
private:
    explicit RenderThemeEfl(Page*);
    ~RenderThemeEfl();

public:
    static PassRefPtr<RenderTheme> create(Page*);

    // A method asking if the theme's controls actually care about redrawing when hovered.
    virtual bool supportsHover(const RenderStyle*) const { return true; }

    // A method asking if the theme is able to draw the focus ring.
    virtual bool supportsFocusRing(const RenderStyle*) const;

    // A method asking if the control changes its tint when the window has focus or not.
    virtual bool controlSupportsTints(const RenderObject*) const;

    // A general method asking if any control tinting is supported at all.
    virtual bool supportsControlTints() const { return true; }

    // A general method asking if foreground colors of selection are supported.
    virtual bool supportsSelectionForegroundColors() const;

    // A method to obtain the baseline position for a "leaf" control. This will only be used if a baseline
    // position cannot be determined by examining child content. Checkboxes and radio buttons are examples of
    // controls that need to do this.
    virtual int baselinePosition(const RenderObject*) const;

    virtual Color platformActiveSelectionBackgroundColor() const;
    virtual Color platformInactiveSelectionBackgroundColor() const;
    virtual Color platformActiveSelectionForegroundColor() const;
    virtual Color platformInactiveSelectionForegroundColor() const;
    virtual Color platformFocusRingColor() const;

    // Set platform colors; remember to call platformColorDidChange after.
    void setColorFromThemeClass(const char* colorClass);

    void setButtonTextColor(int foreR, int foreG, int foreB, int foreA, int backR, int backG, int backB, int backA);
    void setComboTextColor(int foreR, int foreG, int foreB, int foreA, int backR, int backG, int backB, int backA);
    void setEntryTextColor(int foreR, int foreG, int foreB, int foreA, int backR, int backG, int backB, int backA);
    void setSearchTextColor(int foreR, int foreG, int foreB, int foreA, int backR, int backG, int backB, int backA);

    void adjustSizeConstraints(RenderStyle*, FormType) const;

    // System fonts.
    virtual void systemFont(CSSValueID, FontDescription&) const;

    virtual void adjustCheckboxStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintCheckbox(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustRadioStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintRadio(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustTextFieldStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintTextField(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustTextAreaStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintTextArea(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustMenuListStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintMenuList(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustMenuListButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintMenuListButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldResultsDecorationStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldResultsDecoration(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldDecorationStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldDecoration(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchField(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldResultsButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldResultsButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldCancelButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldCancelButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSliderTrackStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSliderThumbStyle(StyleResolver*, RenderStyle*, Element*) const;

    virtual void adjustSliderThumbSize(RenderStyle*, Element*) const;

#if ENABLE(DATALIST_ELEMENT)
    virtual IntSize sliderTickSize() const OVERRIDE;
    virtual int sliderTickOffsetFromTrackCenter() const OVERRIDE;
    virtual LayoutUnit sliderTickSnappingThreshold() const OVERRIDE;
#endif

    virtual bool supportsDataListUI(const AtomicString&) const OVERRIDE;

    virtual bool paintSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustInnerSpinButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintInnerSpinButton(RenderObject*, const PaintInfo&, const IntRect&);

    static void setDefaultFontSize(int fontsize);

#if ENABLE(PROGRESS_ELEMENT)
    virtual void adjustProgressBarStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintProgressBar(RenderObject*, const PaintInfo&, const IntRect&);
    virtual double animationRepeatIntervalForProgressBar(RenderProgress*) const;
    virtual double animationDurationForProgressBar(RenderProgress*) const;
#endif

#if ENABLE(VIDEO)
    virtual String extraMediaControlsStyleSheet();
#if ENABLE(FULLSCREEN_API)
    virtual String extraFullScreenStyleSheet();
#endif
    virtual String formatMediaControlsCurrentTime(float currentTime, float duration) const;
    virtual bool hasOwnDisabledStateHandlingFor(ControlPart) const;

    virtual bool paintMediaFullscreenButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaPlayButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaMuteButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSeekBackButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSeekForwardButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderContainer(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaCurrentTime(RenderObject*, const PaintInfo&, const IntRect&);
#endif
#if ENABLE(VIDEO_TRACK)
    virtual bool supportsClosedCaptioning() const OVERRIDE;
    virtual bool paintMediaToggleClosedCaptionsButton(RenderObject*, const PaintInfo&, const IntRect&) OVERRIDE;
#endif
    virtual bool shouldShowPlaceholderWhenFocused() const OVERRIDE { return true; }

    void setThemePath(const String&);
    String themePath() const;

protected:
    static float defaultFontSize;

private:
    bool loadTheme();
    ALWAYS_INLINE bool loadThemeIfNeeded() const
    {
        return m_edje || (!m_themePath.isEmpty() && const_cast<RenderThemeEfl*>(this)->loadTheme());
    }

    ALWAYS_INLINE Ecore_Evas* canvas() const { return m_canvas.get(); }
    ALWAYS_INLINE Evas_Object* edje() const { return m_edje.get(); }

    void applyPartDescriptionsFrom(const String& themePath);

    void applyEdjeStateFromForm(Evas_Object*, ControlStates);
    void applyEdjeRTLState(Evas_Object*, RenderObject*, FormType, const IntRect&);
    bool paintThemePart(RenderObject*, FormType, const PaintInfo&, const IntRect&);

#if ENABLE(VIDEO)
    bool emitMediaButtonSignal(FormType, MediaControlElementType, const IntRect&);
#endif

    Page* m_page;
    Color m_activeSelectionBackgroundColor;
    Color m_activeSelectionForegroundColor;
    Color m_inactiveSelectionBackgroundColor;
    Color m_inactiveSelectionForegroundColor;
    Color m_focusRingColor;
    Color m_sliderThumbColor;

#if ENABLE(VIDEO)
    Color m_mediaPanelColor;
    Color m_mediaSliderColor;
#endif

    String m_themePath;
    // Order so that the canvas gets destroyed at last.
    OwnPtr<Ecore_Evas> m_canvas;
    RefPtr<Evas_Object> m_edje;

    bool m_supportsSelectionForegroundColor;

    struct ThemePartDesc {
        FormType type;
        LengthSize min;
        LengthSize max;
        LengthBox padding;
    };
    void applyPartDescriptionFallback(struct ThemePartDesc*);
    void applyPartDescription(Evas_Object*, struct ThemePartDesc*);

    struct ThemePartCacheEntry {
        static PassOwnPtr<RenderThemeEfl::ThemePartCacheEntry> create(const String& themePath, FormType, const IntSize&);
        void reuse(const String& themePath, FormType, const IntSize&);

        ALWAYS_INLINE Ecore_Evas* canvas() { return m_canvas.get(); }
        ALWAYS_INLINE Evas_Object* edje() { return m_edje.get(); }
        ALWAYS_INLINE cairo_surface_t* surface() { return m_surface.get(); }

        FormType type;
        IntSize size;

    private:
        // Order so that the canvas gets destroyed at last.
        OwnPtr<Ecore_Evas> m_canvas;
        RefPtr<Evas_Object> m_edje;
        RefPtr<cairo_surface_t> m_surface;
    };

    struct ThemePartDesc m_partDescs[FormTypeLast];

    // List of ThemePartCacheEntry* sorted so that the most recently
    // used entries come first. We use a list for efficient moving
    // of items within the container.
    Eina_List* m_partCache;

    ThemePartCacheEntry* getThemePartFromCache(FormType, const IntSize&);
    void clearThemePartCache();
};
}

#endif // RenderThemeEfl_h
