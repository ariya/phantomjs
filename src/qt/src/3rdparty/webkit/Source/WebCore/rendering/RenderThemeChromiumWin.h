/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2006 Apple Computer, Inc.
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

#ifndef RenderThemeChromiumWin_h
#define RenderThemeChromiumWin_h

#include "RenderThemeChromiumSkia.h"

#if WIN32
typedef void* HANDLE;
typedef struct HINSTANCE__* HINSTANCE;
typedef HINSTANCE HMODULE;
#endif

namespace WebCore {

    struct ThemeData {
        ThemeData() : m_part(0), m_state(0), m_classicState(0) {}

        unsigned m_part;
        unsigned m_state;
        unsigned m_classicState;
    };

    class RenderThemeChromiumWin : public RenderThemeChromiumSkia {
    public:
        static PassRefPtr<RenderTheme> create();

        // A method asking if the theme is able to draw the focus ring.
        virtual bool supportsFocusRing(const RenderStyle*) const;

        // The platform selection color.
        virtual Color platformActiveSelectionBackgroundColor() const;
        virtual Color platformInactiveSelectionBackgroundColor() const;
        virtual Color platformActiveSelectionForegroundColor() const;
        virtual Color platformInactiveSelectionForegroundColor() const;
        virtual Color platformActiveTextSearchHighlightColor() const;
        virtual Color platformInactiveTextSearchHighlightColor() const;

        // System fonts.
        virtual void systemFont(int propId, FontDescription&) const;
        virtual Color systemColor(int cssValueId) const;

        virtual void adjustSliderThumbSize(RenderObject*) const;

        // Various paint functions.
        virtual bool paintCheckbox(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintRadio(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintButton(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintTextField(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);

        // MenuList refers to an unstyled menulist (meaning a menulist without
        // background-color or border set) and MenuListButton refers to a styled
        // menulist (a menulist with background-color or border set). They have
        // this distinction to support showing aqua style themes whenever they
        // possibly can, which is something we don't want to replicate.
        //
        // In short, we either go down the MenuList code path or the MenuListButton
        // codepath. We never go down both. And in both cases, they render the
        // entire menulist.
        virtual bool paintMenuList(RenderObject*, const PaintInfo&, const IntRect&);

        // Override RenderThemeChromiumSkia's setDefaultFontSize method to also reset the local font property caches.
        // See comment in RenderThemeChromiumSkia::setDefaultFontSize() regarding ugliness of this hack.
        static void setDefaultFontSize(int);

        virtual void adjustInnerSpinButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
        virtual bool paintInnerSpinButton(RenderObject*, const PaintInfo&, const IntRect&);

#if ENABLE(PROGRESS_TAG)
        virtual double animationRepeatIntervalForProgressBar(RenderProgress*) const;
        virtual double animationDurationForProgressBar(RenderProgress*) const;
        virtual void adjustProgressBarStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
        virtual bool paintProgressBar(RenderObject*, const PaintInfo&, const IntRect&);
#endif

    protected:
        virtual double caretBlinkIntervalInternal() const;

    private:
        enum ControlSubPart {
            None,
            SpinButtonDown,
            SpinButtonUp,
        };

        RenderThemeChromiumWin() { }
        virtual ~RenderThemeChromiumWin() { }

        unsigned determineState(RenderObject*, ControlSubPart = None);
        unsigned determineSliderThumbState(RenderObject*);
        unsigned determineClassicState(RenderObject*, ControlSubPart = None);

        ThemeData getThemeData(RenderObject*, ControlSubPart = None);

        bool paintTextFieldInternal(RenderObject*, const PaintInfo&, const IntRect&, bool);
    };

} // namespace WebCore

#endif
