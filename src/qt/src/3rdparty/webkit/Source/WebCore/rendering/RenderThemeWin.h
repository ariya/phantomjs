/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2006, 2008 Apple Computer, Inc.
 * Copyright (C) 2009 Kenneth Rohde Christiansen
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

#ifndef RenderThemeWin_h
#define RenderThemeWin_h

#include "RenderTheme.h"

#if WIN32
typedef void* HANDLE;
typedef struct HINSTANCE__* HINSTANCE;
typedef HINSTANCE HMODULE;
#endif

namespace WebCore {

struct ThemeData {
    ThemeData() :m_part(0), m_state(0), m_classicState(0) {}
    ThemeData(int part, int state)
        : m_part(part)
        , m_state(state)
        , m_classicState(0)
    { }

    unsigned m_part;
    unsigned m_state;
    unsigned m_classicState;
};

class RenderThemeWin : public RenderTheme {
public:
    static PassRefPtr<RenderTheme> create();

    virtual String extraDefaultStyleSheet();
    virtual String extraQuirksStyleSheet();

    // A method asking if the theme's controls actually care about redrawing when hovered.
    virtual bool supportsHover(const RenderStyle*) const;

    virtual Color platformActiveSelectionBackgroundColor() const;
    virtual Color platformInactiveSelectionBackgroundColor() const;
    virtual Color platformActiveSelectionForegroundColor() const;
    virtual Color platformInactiveSelectionForegroundColor() const;

    // System fonts.
    virtual void systemFont(int propId, FontDescription&) const;
    virtual Color systemColor(int cssValueId) const;

    virtual bool paintCheckbox(RenderObject* o, const PaintInfo& i, const IntRect& r)
    { return paintButton(o, i, r); }
    virtual void setCheckboxSize(RenderStyle*) const;

    virtual bool paintRadio(RenderObject* o, const PaintInfo& i, const IntRect& r)
    { return paintButton(o, i, r); }
    virtual void setRadioSize(RenderStyle* style) const
    { return setCheckboxSize(style); }

    virtual bool paintButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustInnerSpinButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintInnerSpinButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual bool paintTextField(RenderObject*, const PaintInfo&, const IntRect&);

    virtual bool paintTextArea(RenderObject* o, const PaintInfo& i, const IntRect& r)
    { return paintTextField(o, i, r); }

    virtual void adjustMenuListStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const;
    virtual bool paintMenuList(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustMenuListButtonStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const;

    virtual bool paintMenuListButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual bool paintSliderTrack(RenderObject* o, const PaintInfo& i, const IntRect& r);
    virtual bool paintSliderThumb(RenderObject* o, const PaintInfo& i, const IntRect& r);
    virtual void adjustSliderThumbSize(RenderObject*) const;

    virtual bool popupOptionSupportsTextIndent() const { return true; }

    virtual void adjustSearchFieldStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSearchField(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldCancelButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldCancelButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldDecorationStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldDecoration(RenderObject*, const PaintInfo&, const IntRect&) { return false; }

    virtual void adjustSearchFieldResultsDecorationStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldResultsDecoration(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldResultsButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldResultsButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void themeChanged();

    virtual void adjustButtonStyle(CSSStyleSelector*, RenderStyle* style, Element*) const {}
    virtual void adjustTextFieldStyle(CSSStyleSelector*, RenderStyle* style, Element*) const {}
    virtual void adjustTextAreaStyle(CSSStyleSelector*, RenderStyle* style, Element*) const {}

    static void setWebKitIsBeingUnloaded();

    virtual bool supportsFocusRing(const RenderStyle*) const;

#if ENABLE(VIDEO)
    virtual String extraMediaControlsStyleSheet();
    virtual bool supportsClosedCaptioning() const;
    virtual bool paintMediaControlsBackground(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaFullscreenButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaMuteButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaPlayButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaRewindButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSeekBackButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSeekForwardButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaToggleClosedCaptionsButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderContainer(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual IntPoint volumeSliderOffsetFromMuteButton(RenderBox*, const IntSize&) const;
#endif

private:
    enum ControlSubPart {
        None,
        SpinButtonDown,
        SpinButtonUp,
    };

    RenderThemeWin();
    ~RenderThemeWin();

    void addIntrinsicMargins(RenderStyle*) const;
    void close();

    unsigned determineState(RenderObject*);
    unsigned determineClassicState(RenderObject*, ControlSubPart = None);
    unsigned determineSliderThumbState(RenderObject*);
    unsigned determineButtonState(RenderObject*);
    unsigned determineSpinButtonState(RenderObject*, ControlSubPart = None);

    bool supportsFocus(ControlPart) const;

    ThemeData getThemeData(RenderObject*, ControlSubPart = None);
    ThemeData getClassicThemeData(RenderObject* o, ControlSubPart = None);

    HANDLE buttonTheme() const;
    HANDLE textFieldTheme() const;
    HANDLE menuListTheme() const;
    HANDLE sliderTheme() const;
    HANDLE spinButtonTheme() const;

    mutable HANDLE m_buttonTheme;
    mutable HANDLE m_textFieldTheme;
    mutable HANDLE m_menuListTheme;
    mutable HANDLE m_sliderTheme;
    mutable HANDLE m_spinButtonTheme;
};

};

#endif
