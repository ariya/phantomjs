/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2006, 2008 Apple Computer, Inc.
 * Copyright (C) 2009 Torch Mobile, Inc.
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

#ifndef RenderThemeWinCE_h
#define RenderThemeWinCE_h

#include "RenderTheme.h"

typedef void* HANDLE;
typedef struct HINSTANCE__* HINSTANCE;
typedef HINSTANCE HMODULE;

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

    class RenderThemeWinCE : public RenderTheme {
    public:
        static PassRefPtr<RenderTheme> create();
        ~RenderThemeWinCE();

        virtual String extraDefaultStyleSheet();
        virtual String extraQuirksStyleSheet();

        // A method asking if the theme's controls actually care about redrawing when hovered.
        virtual bool supportsHover(const RenderStyle*) const;

        virtual Color platformActiveSelectionBackgroundColor() const;
        virtual Color platformInactiveSelectionBackgroundColor() const;
        virtual Color platformActiveSelectionForegroundColor() const;
        virtual Color platformInactiveSelectionForegroundColor() const;

        // System fonts.
        virtual void systemFont(CSSValueID, FontDescription&) const;
        virtual Color systemColor(CSSValueID) const;

        virtual bool paintCheckbox(RenderObject* o, const PaintInfo& i, const IntRect& r)
        { return paintButton(o, i, r); }
        virtual void setCheckboxSize(RenderStyle*) const;

        virtual bool paintRadio(RenderObject* o, const PaintInfo& i, const IntRect& r)
        { return paintButton(o, i, r); }
        virtual void setRadioSize(RenderStyle* style) const
        { return setCheckboxSize(style); }

        virtual bool paintButton(RenderObject*, const PaintInfo&, const IntRect&);

        virtual bool paintTextField(RenderObject*, const PaintInfo&, const IntRect&);

        virtual bool paintTextArea(RenderObject* o, const PaintInfo& i, const IntRect& r)
        { return paintTextField(o, i, r); }

        virtual void adjustMenuListStyle(StyleResolver*, RenderStyle*, Element*) const;
        virtual bool paintMenuList(RenderObject*, const PaintInfo&, const IntRect&);
        virtual void adjustMenuListButtonStyle(StyleResolver*, RenderStyle*, Element*) const;

        virtual bool paintMenuListButton(RenderObject*, const PaintInfo&, const IntRect&);

        virtual bool paintSliderTrack(RenderObject* o, const PaintInfo& i, const IntRect& r);
        virtual bool paintSliderThumb(RenderObject* o, const PaintInfo& i, const IntRect& r);
        virtual void adjustSliderThumbSize(RenderStyle*, Element*) const;

        virtual bool popupOptionSupportsTextIndent() const { return true; }

        virtual void adjustSearchFieldStyle(StyleResolver*, RenderStyle*, Element*) const;
        virtual bool paintSearchField(RenderObject*, const PaintInfo&, const IntRect&);

        virtual void adjustSearchFieldCancelButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
        virtual bool paintSearchFieldCancelButton(RenderObject*, const PaintInfo&, const IntRect&);

        virtual void adjustSearchFieldDecorationStyle(StyleResolver*, RenderStyle*, Element*) const;
        virtual bool paintSearchFieldDecoration(RenderObject*, const PaintInfo&, const IntRect&) { return false; }

        virtual void adjustSearchFieldResultsDecorationStyle(StyleResolver*, RenderStyle*, Element*) const;
        virtual bool paintSearchFieldResultsDecoration(RenderObject*, const PaintInfo&, const IntRect&);

        virtual void adjustSearchFieldResultsButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
        virtual bool paintSearchFieldResultsButton(RenderObject*, const PaintInfo&, const IntRect&);

        virtual void themeChanged();

        virtual void adjustButtonStyle(StyleResolver*, RenderStyle* style, Element*) const { }
        virtual void adjustTextFieldStyle(StyleResolver*, RenderStyle* style, Element*) const { }
        virtual void adjustTextAreaStyle(StyleResolver*, RenderStyle* style, Element*) const { }

        static void setWebKitIsBeingUnloaded();

        virtual bool supportsFocusRing(const RenderStyle*) const;

    #if ENABLE(VIDEO)
        virtual bool paintMediaFullscreenButton(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintMediaPlayButton(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintMediaMuteButton(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintMediaSeekBackButton(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintMediaSeekForwardButton(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintMediaSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintMediaSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    #endif

    private:
        RenderThemeWinCE();

        unsigned determineClassicState(RenderObject*);
        bool supportsFocus(ControlPart) const;

        ThemeData getThemeData(RenderObject*);
    };

};

#endif // RenderThemeWinCE_h
