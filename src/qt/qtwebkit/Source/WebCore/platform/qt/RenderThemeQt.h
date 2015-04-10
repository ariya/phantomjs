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

#include <QBrush>
#include <QPalette>
#include <QSharedPointer>
#include <QString>

QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE

namespace WebCore {

#if ENABLE(PROGRESS_ELEMENT)
class RenderProgress;
#endif
class RenderStyle;
class HTMLMediaElement;
class StylePainter;
class ScrollbarTheme;

typedef PassRefPtr<RenderTheme> (*QtThemeFactoryFunction)(Page* page);

class RenderThemeQt : public RenderTheme {

public:
    RenderThemeQt(Page*);

    static void setCustomTheme(QtThemeFactoryFunction, ScrollbarTheme* customScrollbarTheme);
    static ScrollbarTheme* customScrollbarTheme();

    String extraDefaultStyleSheet();

    virtual bool supportsHover(const RenderStyle*) const;
    virtual bool supportsFocusRing(const RenderStyle*) const;

    virtual int baselinePosition(const RenderObject*) const;

    // A method asking if the control changes its tint when the window has focus or not.
    virtual bool controlSupportsTints(const RenderObject*) const;

    // A general method asking if any control tinting is supported at all.
    virtual bool supportsControlTints() const;

    virtual void adjustRepaintRect(const RenderObject*, IntRect&);

    // The platform selection color.
    virtual Color platformActiveSelectionBackgroundColor() const;
    virtual Color platformInactiveSelectionBackgroundColor() const;
    virtual Color platformActiveSelectionForegroundColor() const;
    virtual Color platformInactiveSelectionForegroundColor() const;

    virtual Color platformFocusRingColor() const;

    virtual void systemFont(CSSValueID, FontDescription&) const;
    virtual Color systemColor(CSSValueID) const;

    virtual int minimumMenuListSize(RenderStyle*) const;

    virtual void adjustSliderThumbSize(RenderStyle*, Element*) const;

#if ENABLE(DATALIST_ELEMENT)
    virtual IntSize sliderTickSize() const OVERRIDE;
    virtual int sliderTickOffsetFromTrackCenter() const OVERRIDE;
#endif

    virtual double caretBlinkInterval() const;

    virtual bool isControlStyled(const RenderStyle*, const BorderData&, const FillLayer&, const Color&) const;

#if ENABLE(VIDEO)
    virtual String extraMediaControlsStyleSheet();
#endif

protected:
    virtual bool paintCheckbox(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void setCheckboxSize(RenderStyle*) const;

    virtual bool paintRadio(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void setRadioSize(RenderStyle*) const;

    virtual void setButtonSize(RenderStyle*) const;

    virtual void adjustTextFieldStyle(StyleResolver*, RenderStyle*, Element*) const;

    virtual bool paintTextArea(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustTextAreaStyle(StyleResolver*, RenderStyle*, Element*) const;

    virtual void adjustMenuListStyle(StyleResolver*, RenderStyle*, Element*) const;

    virtual void adjustMenuListButtonStyle(StyleResolver*, RenderStyle*, Element*) const;

#if ENABLE(PROGRESS_ELEMENT)
    virtual void adjustProgressBarStyle(StyleResolver*, RenderStyle*, Element*) const;
    // Returns the repeat interval of the animation for the progress bar.
    virtual double animationRepeatIntervalForProgressBar(RenderProgress*) const;
#endif

    virtual void adjustSliderTrackStyle(StyleResolver*, RenderStyle*, Element*) const;

    virtual void adjustSliderThumbStyle(StyleResolver*, RenderStyle*, Element*) const;

    virtual bool paintSearchField(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustSearchFieldStyle(StyleResolver*, RenderStyle*, Element*) const;

    virtual void adjustSearchFieldCancelButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldCancelButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldDecorationStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldDecoration(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldResultsDecorationStyle(StyleResolver*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldResultsDecoration(RenderObject*, const PaintInfo&, const IntRect&);

#ifndef QT_NO_SPINBOX
    virtual void adjustInnerSpinButtonStyle(StyleResolver*, RenderStyle*, Element*) const;
#endif

#if ENABLE(VIDEO)
    virtual bool paintMediaFullscreenButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaPlayButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaMuteButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSeekBackButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSeekForwardButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaToggleClosedCaptionsButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaCurrentTime(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual String formatMediaControlsCurrentTime(float currentTime, float duration) const;
    virtual String formatMediaControlsRemainingTime(float currentTime, float duration) const;
    virtual bool hasOwnDisabledStateHandlingFor(ControlPart) const { return true; }
#if ENABLE(VIDEO_TRACK)
    virtual bool supportsClosedCaptioning() const { return true; }
#endif

    void paintMediaBackground(QPainter*, const IntRect&) const;
    double mediaControlsBaselineOpacity() const;
    QColor getMediaControlForegroundColor(RenderObject* = 0) const;
#endif
    virtual void computeSizeBasedOnStyle(RenderStyle*) const = 0;

    virtual String fileListNameForWidth(const FileList*, const Font&, int width, bool multipleFilesAllowed) const OVERRIDE;

    virtual QRect inflateButtonRect(const QRect& originalRect) const;

    virtual void setPopupPadding(RenderStyle*) const = 0;

    virtual QSharedPointer<StylePainter> getStylePainter(const PaintInfo&) = 0;

    bool supportsFocus(ControlPart) const;

    IntRect convertToPaintingRect(RenderObject* inputRenderer, const RenderObject* partRenderer, IntRect partRect, const IntRect& localOffset) const;

    virtual QPalette colorPalette() const;

    Page* m_page;

    QString m_buttonFontFamily;

};

class StylePainter {
public:
    virtual ~StylePainter();

    bool isValid() const { return painter; }

    QPainter* painter;

protected:
    StylePainter(GraphicsContext*);

private:
    QBrush m_previousBrush;
    bool m_previousAntialiasing;

};

}

#endif // RenderThemeQt_h
