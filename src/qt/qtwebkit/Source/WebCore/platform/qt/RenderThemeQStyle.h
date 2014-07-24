/*
 * This file is part of the theme implementation for form controls in WebCore.
 *
 * Copyright (C) 2011-2012 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef RenderThemeQStyle_h
#define RenderThemeQStyle_h

#include "QStyleFacade.h"
#include "RenderThemeQt.h"

namespace WebCore {

class ScrollbarThemeQStyle;

class Page;
class QStyleFacade;
struct QStyleFacadeOption;

typedef QStyleFacade* (*QtStyleFactoryFunction)(Page*);

class RenderThemeQStyle : public RenderThemeQt {
private:
    friend class StylePainterQStyle;

    RenderThemeQStyle(Page*);
    virtual ~RenderThemeQStyle();

public:
    static PassRefPtr<RenderTheme> create(Page*);

    static void setStyleFactoryFunction(QtStyleFactoryFunction);
    static QtStyleFactoryFunction styleFactory();

    virtual void adjustSliderThumbSize(RenderStyle*, Element*) const;

    QStyleFacade* qStyle() { return m_qStyle.get(); }

protected:
    virtual void adjustButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual bool paintTextField(RenderObject*, const PaintInfo&, const IntRect&);

    virtual bool paintTextArea(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustTextAreaStyle(StyleResolver*, RenderStyle*, Element*) const;

    virtual bool paintMenuList(RenderObject*, const PaintInfo&, const IntRect&);

    virtual bool paintMenuListButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustMenuListButtonStyle(StyleResolver*, RenderStyle*, Element*) const;

#if ENABLE(PROGRESS_ELEMENT)
    // Returns the duration of the animation for the progress bar.
    virtual double animationDurationForProgressBar(RenderProgress*) const;
    virtual bool paintProgressBar(RenderObject*, const PaintInfo&, const IntRect&);
#endif

    virtual bool paintSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustSliderTrackStyle(StyleResolver*, RenderStyle*, Element*) const;

    virtual bool paintSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustSliderThumbStyle(StyleResolver*, RenderStyle*, Element*) const;

    virtual bool paintSearchField(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldDecorationStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldDecoration(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldResultsDecorationStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldResultsDecoration(RenderObject*, const PaintInfo&, const IntRect&);

#ifndef QT_NO_SPINBOX
    virtual bool paintInnerSpinButton(RenderObject*, const PaintInfo&, const IntRect&);
#endif

protected:
    virtual void computeSizeBasedOnStyle(RenderStyle*) const;

    virtual QSharedPointer<StylePainter> getStylePainter(const PaintInfo&);

    virtual QRect inflateButtonRect(const QRect& originalRect) const;

    virtual void setPopupPadding(RenderStyle*) const;

    virtual QPalette colorPalette() const;

private:
    ControlPart initializeCommonQStyleOptions(QStyleFacadeOption&, RenderObject*) const;

    void setButtonPadding(RenderStyle*) const;

    void setPaletteFromPageClientIfExists(QPalette&) const;

#ifdef Q_OS_MAC
    int m_buttonFontPixelSize;
#endif

    OwnPtr<QStyleFacade> m_qStyle;
};

class StylePainterQStyle : public StylePainter {
public:
    explicit StylePainterQStyle(RenderThemeQStyle*, const PaintInfo&, RenderObject*);
    explicit StylePainterQStyle(ScrollbarThemeQStyle*, GraphicsContext*);

    bool isValid() const { return qStyle && qStyle->isValid() && StylePainter::isValid(); }

    QStyleFacade* qStyle;
    QStyleFacadeOption styleOption;
    ControlPart appearance;

    void paintButton(QStyleFacade::ButtonType type)
    { qStyle->paintButton(painter, type, styleOption); }
    void paintTextField()
    { qStyle->paintTextField(painter, styleOption); }
    void paintComboBox()
    { qStyle->paintComboBox(painter, styleOption); }
    void paintComboBoxArrow()
    { qStyle->paintComboBoxArrow(painter, styleOption); }
    void paintSliderTrack()
    { qStyle->paintSliderTrack(painter, styleOption); }
    void paintSliderThumb()
    { qStyle->paintSliderThumb(painter, styleOption); }
    void paintInnerSpinButton(bool spinBoxUp)
    { qStyle->paintInnerSpinButton(painter, styleOption, spinBoxUp); }
    void paintProgressBar(double progress, double animationProgress)
    { qStyle->paintProgressBar(painter, styleOption, progress, animationProgress); }
    void paintScrollCorner(const QRect& rect)
    { qStyle->paintScrollCorner(painter, rect); }
    void paintScrollBar()
    { qStyle->paintScrollBar(painter, styleOption); }

private:
    void setupStyleOption();

    Q_DISABLE_COPY(StylePainterQStyle)
};

}

#endif // RenderThemeQStyle_h
