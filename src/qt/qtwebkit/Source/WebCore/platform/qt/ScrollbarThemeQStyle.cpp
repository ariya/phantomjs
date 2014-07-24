/*
 * Copyright (C) 2007, 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2007 Staikos Computing Services Inc. <info@staikos.net>
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ScrollbarThemeQStyle.h"

#include "GraphicsContext.h"
#include "PlatformMouseEvent.h"
#include "RenderThemeQStyle.h"
#include "ScrollView.h"
#include "Scrollbar.h"

#include <QGuiApplication>

namespace WebCore {

ScrollbarThemeQStyle::ScrollbarThemeQStyle()
{
    m_qStyle = adoptPtr(RenderThemeQStyle::styleFactory()(/*page*/ 0));
}

ScrollbarThemeQStyle::~ScrollbarThemeQStyle()
{
}

static QStyleFacade::SubControl scPart(const ScrollbarPart& part)
{
    switch (part) {
    case NoPart:
        return QStyleFacade::SC_None;
    case BackButtonStartPart:
    case BackButtonEndPart:
        return QStyleFacade::SC_ScrollBarSubLine;
    case BackTrackPart:
        return QStyleFacade::SC_ScrollBarSubPage;
    case ThumbPart:
        return QStyleFacade::SC_ScrollBarSlider;
    case ForwardTrackPart:
        return QStyleFacade::SC_ScrollBarAddPage;
    case ForwardButtonStartPart:
    case ForwardButtonEndPart:
        return QStyleFacade::SC_ScrollBarAddLine;
    }

    return QStyleFacade::SC_None;
}

static ScrollbarPart scrollbarPart(const QStyleFacade::SubControl& sc)
{
    switch (sc) {
    case QStyleFacade::SC_None:
        return NoPart;
    case QStyleFacade::SC_ScrollBarSubLine:
        return BackButtonStartPart;
    case QStyleFacade::SC_ScrollBarSubPage:
        return BackTrackPart;
    case QStyleFacade::SC_ScrollBarSlider:
        return ThumbPart;
    case QStyleFacade::SC_ScrollBarAddPage:
        return ForwardTrackPart;
    case QStyleFacade::SC_ScrollBarAddLine:
        return ForwardButtonStartPart;
    }
    return NoPart;
}

static QStyleFacadeOption initSliderStyleOption(ScrollbarThemeClient* scrollbar, QObject* widget = 0)
{
    QStyleFacadeOption opt;
    if (widget) {
        opt.palette = widget->property("palette").value<QPalette>();
        opt.rect = widget->property("rect").value<QRect>();
        opt.direction = static_cast<Qt::LayoutDirection>(widget->property("layoutDirection").toInt());
    } else {
        opt.state |= QStyleFacade::State_Active;
        opt.direction = QGuiApplication::layoutDirection();
    }

    opt.state &= ~QStyleFacade::State_HasFocus;

    opt.rect = scrollbar->frameRect();
    if (scrollbar->enabled())
        opt.state |= QStyleFacade::State_Enabled;
    if (scrollbar->controlSize() != RegularScrollbar)
        opt.state |= QStyleFacade::State_Mini;
    opt.slider.orientation = (scrollbar->orientation() == VerticalScrollbar) ? Qt::Vertical : Qt::Horizontal;

    if (scrollbar->orientation() == HorizontalScrollbar)
        opt.state |= QStyleFacade::State_Horizontal;
    else
        opt.state &= ~QStyleFacade::State_Horizontal;

    opt.slider.value = scrollbar->value();
    opt.slider.position = opt.slider.value;
    opt.slider.pageStep = scrollbar->pageStep();
    opt.slider.singleStep = scrollbar->lineStep();
    opt.slider.minimum = 0;
    opt.slider.maximum = qMax(0, scrollbar->maximum());
    if (opt.slider.orientation == Qt::Horizontal && opt.direction == Qt::RightToLeft)
        opt.slider.upsideDown = true;
    ScrollbarPart pressedPart = scrollbar->pressedPart();
    ScrollbarPart hoveredPart = scrollbar->hoveredPart();
    if (pressedPart != NoPart) {
        opt.slider.activeSubControls = scPart(scrollbar->pressedPart());
        if (pressedPart == BackButtonStartPart || pressedPart == ForwardButtonStartPart
            || pressedPart == BackButtonEndPart || pressedPart == ForwardButtonEndPart
            || pressedPart == ThumbPart)
            opt.state |= QStyleFacade::State_Sunken;
    } else
        opt.slider.activeSubControls = scPart(hoveredPart);
    if (hoveredPart != NoPart)
        opt.state |= QStyleFacade::State_MouseOver;
    return opt;
}

bool ScrollbarThemeQStyle::paint(ScrollbarThemeClient* scrollbar, GraphicsContext* graphicsContext, const IntRect& dirtyRect)
{
    if (graphicsContext->updatingControlTints()) {
        scrollbar->invalidateRect(dirtyRect);
        return false;
    }

    StylePainterQStyle p(this, graphicsContext);
    if (!p.isValid())
        return true;

    p.painter->save();
    p.styleOption = initSliderStyleOption(scrollbar, m_qStyle->widgetForPainter(p.painter));

    p.painter->setClipRect(p.styleOption.rect.intersected(dirtyRect), Qt::IntersectClip);
    p.paintScrollBar();
    p.painter->restore();
    return true;
}

ScrollbarPart ScrollbarThemeQStyle::hitTest(ScrollbarThemeClient* scrollbar, const IntPoint& position)
{
    QStyleFacadeOption opt = initSliderStyleOption(scrollbar);
    const QPoint pos = scrollbar->convertFromContainingWindow(position);
    opt.rect.moveTo(QPoint(0, 0));
    QStyleFacade::SubControl sc = m_qStyle->hitTestScrollBar(opt, pos);
    return scrollbarPart(sc);
}

bool ScrollbarThemeQStyle::shouldCenterOnThumb(ScrollbarThemeClient*, const PlatformMouseEvent& evt)
{
    // Middle click centers slider thumb (if supported).
    return m_qStyle->scrollBarMiddleClickAbsolutePositionStyleHint() && evt.button() == MiddleButton;
}

void ScrollbarThemeQStyle::invalidatePart(ScrollbarThemeClient* scrollbar, ScrollbarPart)
{
    // FIXME: Do more precise invalidation.
    scrollbar->invalidate();
}

int ScrollbarThemeQStyle::scrollbarThickness(ScrollbarControlSize controlSize)
{
    const bool mini = controlSize != RegularScrollbar;
    return m_qStyle->scrollBarExtent(mini);
}

int ScrollbarThemeQStyle::thumbPosition(ScrollbarThemeClient* scrollbar)
{
    if (scrollbar->enabled()) {
        float pos = (float)scrollbar->currentPos() * (trackLength(scrollbar) - thumbLength(scrollbar)) / scrollbar->maximum();
        return (pos < 1 && pos > 0) ? 1 : pos;
    }
    return 0;
}

int ScrollbarThemeQStyle::thumbLength(ScrollbarThemeClient* scrollbar)
{
    QStyleFacadeOption opt = initSliderStyleOption(scrollbar);
    QRect thumb = m_qStyle->scrollBarSubControlRect(opt, QStyleFacade::SC_ScrollBarSlider);
    return scrollbar->orientation() == HorizontalScrollbar ? thumb.width() : thumb.height();
}

int ScrollbarThemeQStyle::trackPosition(ScrollbarThemeClient* scrollbar)
{
    QStyleFacadeOption opt = initSliderStyleOption(scrollbar);
    QRect track = m_qStyle->scrollBarSubControlRect(opt, QStyleFacade::SC_ScrollBarGroove);
    return scrollbar->orientation() == HorizontalScrollbar ? track.x() : track.y();
}

int ScrollbarThemeQStyle::trackLength(ScrollbarThemeClient* scrollbar)
{
    QStyleFacadeOption opt = initSliderStyleOption(scrollbar);
    QRect track = m_qStyle->scrollBarSubControlRect(opt, QStyleFacade::SC_ScrollBarGroove);
    return scrollbar->orientation() == HorizontalScrollbar ? track.width() : track.height();
}

void ScrollbarThemeQStyle::paintScrollCorner(ScrollView*, GraphicsContext* context, const IntRect& rect)
{
    StylePainterQStyle p(this, context);
    if (!p.isValid())
        return;

    p.paintScrollCorner(rect);
}

}

