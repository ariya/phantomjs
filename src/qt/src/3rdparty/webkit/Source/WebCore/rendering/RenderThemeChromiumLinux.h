/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008, 2009 Google, Inc.
 * All rights reserved.
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

#ifndef RenderThemeChromiumLinux_h
#define RenderThemeChromiumLinux_h

#include "RenderThemeChromiumSkia.h"

namespace WebCore {

    class RenderThemeChromiumLinux : public RenderThemeChromiumSkia {
    public:
        static PassRefPtr<RenderTheme> create();
        virtual String extraDefaultStyleSheet();

        virtual Color systemColor(int cssValidId) const;

        // A method asking if the control changes its tint when the window has focus or not.
        virtual bool controlSupportsTints(const RenderObject*) const;

        // List Box selection color
        virtual Color activeListBoxSelectionBackgroundColor() const;
        virtual Color activeListBoxSelectionForegroundColor() const;
        virtual Color inactiveListBoxSelectionBackgroundColor() const;
        virtual Color inactiveListBoxSelectionForegroundColor() const;

        virtual Color platformActiveSelectionBackgroundColor() const;
        virtual Color platformInactiveSelectionBackgroundColor() const;
        virtual Color platformActiveSelectionForegroundColor() const;
        virtual Color platformInactiveSelectionForegroundColor() const;

        virtual void adjustSliderThumbSize(RenderObject*) const;

        static void setCaretBlinkInterval(double interval);
        virtual double caretBlinkIntervalInternal() const;

        virtual bool paintCheckbox(RenderObject*, const PaintInfo&, const IntRect&);
        virtual void setCheckboxSize(RenderStyle*) const;

        virtual bool paintRadio(RenderObject*, const PaintInfo&, const IntRect&);
        virtual void setRadioSize(RenderStyle*) const;

        virtual bool paintButton(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintTextField(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintMenuList(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
        virtual bool paintSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);

        virtual void adjustInnerSpinButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
        virtual bool paintInnerSpinButton(RenderObject*, const PaintInfo&, const IntRect&);

#if ENABLE(PROGRESS_TAG)
        virtual bool paintProgressBar(RenderObject*, const PaintInfo&, const IntRect&);
#endif

        static void setSelectionColors(unsigned activeBackgroundColor,
                                       unsigned activeForegroundColor,
                                       unsigned inactiveBackgroundColor,
                                       unsigned inactiveForegroundColor);

    private:
        RenderThemeChromiumLinux();
        virtual ~RenderThemeChromiumLinux();

        // A general method asking if any control tinting is supported at all.
        virtual bool supportsControlTints() const;

        static double m_caretBlinkInterval;

        static unsigned m_activeSelectionBackgroundColor;
        static unsigned m_activeSelectionForegroundColor;
        static unsigned m_inactiveSelectionBackgroundColor;
        static unsigned m_inactiveSelectionForegroundColor;
    };

} // namespace WebCore

#endif // RenderThemeChromiumLinux_h
