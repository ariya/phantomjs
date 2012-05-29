/*
 * This file is part of the theme implementation for form controls in WebCore.
 *
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef RenderThemeQt_h
#define RenderThemeQt_h

#include "RenderTheme.h"

#include <QStyle>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPainter;
class QWidget;
QT_END_NAMESPACE

namespace WebCore {

#if ENABLE(PROGRESS_TAG)
class RenderProgress;
#endif
class RenderStyle;
class HTMLMediaElement;
class ScrollbarThemeQt;

class RenderThemeQt : public RenderTheme {
private:
    RenderThemeQt(Page* page);
    virtual ~RenderThemeQt();

public:
    static PassRefPtr<RenderTheme> create(Page*);

    String extraDefaultStyleSheet();

    virtual bool supportsHover(const RenderStyle*) const;
    virtual bool supportsFocusRing(const RenderStyle* style) const;

    virtual int baselinePosition(const RenderObject* o) const;

    // A method asking if the control changes its tint when the window has focus or not.
    virtual bool controlSupportsTints(const RenderObject*) const;

    // A general method asking if any control tinting is supported at all.
    virtual bool supportsControlTints() const;

    virtual void adjustRepaintRect(const RenderObject* o, IntRect& r);

    // The platform selection color.
    virtual Color platformActiveSelectionBackgroundColor() const;
    virtual Color platformInactiveSelectionBackgroundColor() const;
    virtual Color platformActiveSelectionForegroundColor() const;
    virtual Color platformInactiveSelectionForegroundColor() const;

    virtual Color platformFocusRingColor() const;

    virtual void systemFont(int propId, FontDescription&) const;
    virtual Color systemColor(int cssValueId) const;

    virtual int minimumMenuListSize(RenderStyle*) const;

    virtual void adjustSliderThumbSize(RenderObject*) const;

    virtual double caretBlinkInterval() const;

    virtual bool isControlStyled(const RenderStyle*, const BorderData&, const FillLayer&, const Color& backgroundColor) const;
#if USE(QT_MOBILE_THEME)
    virtual int popupInternalPaddingBottom(RenderStyle*) const;
#endif

#if ENABLE(VIDEO)
    virtual String extraMediaControlsStyleSheet();
#endif

    QStyle* qStyle() const;

protected:
    virtual bool paintCheckbox(RenderObject* o, const PaintInfo& i, const IntRect& r);
    virtual void setCheckboxSize(RenderStyle*) const;

    virtual bool paintRadio(RenderObject* o, const PaintInfo& i, const IntRect& r);
    virtual void setRadioSize(RenderStyle*) const;

    virtual void adjustButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void setButtonSize(RenderStyle*) const;

    virtual bool paintTextField(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustTextFieldStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

    virtual bool paintTextArea(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustTextAreaStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

    virtual bool paintMenuList(RenderObject* o, const PaintInfo& i, const IntRect& r);
    virtual void adjustMenuListStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

    virtual bool paintMenuListButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustMenuListButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

#if ENABLE(PROGRESS_TAG)
    virtual void adjustProgressBarStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintProgressBar(RenderObject*, const PaintInfo&, const IntRect&);
#endif

    virtual bool paintSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustSliderTrackStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

    virtual bool paintSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustSliderThumbStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

    virtual bool paintSearchField(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustSearchFieldStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

    virtual void adjustSearchFieldCancelButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldCancelButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldDecorationStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldDecoration(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldResultsDecorationStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldResultsDecoration(RenderObject*, const PaintInfo&, const IntRect&);

#if ENABLE(PROGRESS_TAG)
    // Returns the repeat interval of the animation for the progress bar.
    virtual double animationRepeatIntervalForProgressBar(RenderProgress* renderProgress) const;
    // Returns the duration of the animation for the progress bar.
    virtual double animationDurationForProgressBar(RenderProgress* renderProgress) const;
#endif

#if ENABLE(VIDEO)
    virtual bool paintMediaFullscreenButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaPlayButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaMuteButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSeekBackButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSeekForwardButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaCurrentTime(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual String formatMediaControlsCurrentTime(float currentTime, float duration) const;
    virtual String formatMediaControlsRemainingTime(float currentTime, float duration) const;
    virtual bool hasOwnDisabledStateHandlingFor(ControlPart) const { return true; }
private:
    void paintMediaBackground(QPainter* painter, const IntRect& r) const;
    double mediaControlsBaselineOpacity() const;
    QColor getMediaControlForegroundColor(RenderObject* o = 0) const;
#endif
    void computeSizeBasedOnStyle(RenderStyle* renderStyle) const;

private:
    bool supportsFocus(ControlPart) const;

    ControlPart initializeCommonQStyleOptions(QStyleOption&, RenderObject*) const;

    void setButtonPadding(RenderStyle*) const;
    void setPopupPadding(RenderStyle*) const;

    void setPaletteFromPageClientIfExists(QPalette&) const;

    int findFrameLineWidth(QStyle* style) const;

    QStyle* fallbackStyle() const;

    IntRect convertToPaintingRect(RenderObject* inputRenderer, const RenderObject* partRenderer, IntRect partRect, const IntRect& localOffset) const;

    Page* m_page;

#ifdef Q_WS_MAC
    int m_buttonFontPixelSize;
#endif
    QString m_buttonFontFamily;

    QStyle* m_fallbackStyle;
    mutable QLineEdit* m_lineEdit;
};

class StylePainter {
public:
    explicit StylePainter(RenderThemeQt*, const PaintInfo&);
    explicit StylePainter(ScrollbarThemeQt*, GraphicsContext*);
    ~StylePainter();

    bool isValid() const { return painter && style; }

    QPainter* painter;
    QWidget* widget;
    QStyle* style;

    void drawPrimitive(QStyle::PrimitiveElement pe, const QStyleOption& opt)
    { style->drawPrimitive(pe, &opt, painter, widget); }
    void drawControl(QStyle::ControlElement ce, const QStyleOption& opt)
    { style->drawControl(ce, &opt, painter, widget); }
    void drawComplexControl(QStyle::ComplexControl cc, const QStyleOptionComplex& opt)
    { style->drawComplexControl(cc, &opt, painter, widget); }

private:
    void init(GraphicsContext* context, QStyle*);

    QBrush oldBrush;
    bool oldAntialiasing;

    Q_DISABLE_COPY(StylePainter)
};

}

#endif // RenderThemeQt_h
