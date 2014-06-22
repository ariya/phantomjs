/*
 * This file is part of the theme implementation for form controls in WebCore.
 *
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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
#ifndef QStyleFacade_h
#define QStyleFacade_h

#include <QPalette>
#include <QRect>

QT_BEGIN_NAMESPACE
class QStyle;
QT_END_NAMESPACE

namespace WebCore {

class Page;
class QStyleFacadeOption;

class QStyleFacade {
public:
    enum ButtonSubElement {
        PushButtonLayoutItem,
        PushButtonContents
    };

#define FOR_EACH_MAPPED_STATE(F, SEPARATOR) \
    F(State_None, 0x00000000) SEPARATOR \
    F(State_Enabled, 0x00000001) SEPARATOR \
    F(State_Raised, 0x00000002) SEPARATOR \
    F(State_Sunken, 0x00000004) SEPARATOR \
    F(State_Off, 0x00000008) SEPARATOR \
    F(State_NoChange, 0x00000010) SEPARATOR \
    F(State_On, 0x00000020) SEPARATOR \
    F(State_DownArrow, 0x00000040) SEPARATOR \
    F(State_Horizontal, 0x00000080) SEPARATOR \
    F(State_HasFocus, 0x00000100) SEPARATOR \
    F(State_Top, 0x00000200) SEPARATOR \
    F(State_Bottom, 0x00000400) SEPARATOR \
    F(State_FocusAtBorder, 0x00000800) SEPARATOR \
    F(State_AutoRaise, 0x00001000) SEPARATOR \
    F(State_MouseOver, 0x00002000) SEPARATOR \
    F(State_UpArrow, 0x00004000) SEPARATOR \
    F(State_Selected, 0x00008000) SEPARATOR \
    F(State_Active, 0x00010000) SEPARATOR \
    F(State_Window, 0x00020000) SEPARATOR \
    F(State_Open, 0x00040000) SEPARATOR \
    F(State_Children, 0x00080000) SEPARATOR \
    F(State_Item, 0x00100000) SEPARATOR \
    F(State_Sibling, 0x00200000) SEPARATOR \
    F(State_Editing, 0x00400000) SEPARATOR \
    F(State_KeyboardFocusChange, 0x00800000) SEPARATOR \
    F(State_ReadOnly, 0x02000000) SEPARATOR \
    F(State_Small, 0x04000000) SEPARATOR \
    F(State_Mini, 0x0800000)

#define COMMA ,
#define SEMICOLON ;
#define DEFINE_MAPPED_STATE(Name, Value) \
    Name = Value

    // ### Remove unused states.
    enum StateFlag {
        FOR_EACH_MAPPED_STATE(DEFINE_MAPPED_STATE, COMMA)
    };
    Q_DECLARE_FLAGS(State, StateFlag)

#define FOR_EACH_MAPPED_METRIC(F, SEPARATOR) \
    F(PM_ButtonMargin) SEPARATOR \
    F(PM_DefaultFrameWidth) SEPARATOR \
    F(PM_IndicatorWidth) SEPARATOR \
    F(PM_ExclusiveIndicatorWidth) SEPARATOR \
    F(PM_ButtonIconSize)

#define DEFINE_METRIC(F) F

    enum PixelMetric {
        FOR_EACH_MAPPED_METRIC(DEFINE_METRIC, COMMA)
    };

#define FOR_EACH_SUBCONTROL(F, SEPARATOR) \
    F(SC_None, 0x00000000) SEPARATOR \
    F(SC_ScrollBarAddLine, 0x00000001) SEPARATOR \
    F(SC_ScrollBarSubLine, 0x00000002) SEPARATOR \
    F(SC_ScrollBarAddPage, 0x00000004) SEPARATOR \
    F(SC_ScrollBarSubPage, 0x00000008) SEPARATOR \
    F(SC_ScrollBarFirst, 0x00000010) SEPARATOR \
    F(SC_ScrollBarLast, 0x00000020) SEPARATOR \
    F(SC_ScrollBarSlider, 0x00000040) SEPARATOR \
    F(SC_ScrollBarGroove, 0x00000080)

#define DEFINE_SUBCONTROL(F, Value) F

    enum SubControl {
        FOR_EACH_SUBCONTROL(DEFINE_SUBCONTROL, COMMA)
    };

    virtual ~QStyleFacade() { }

    virtual QRect buttonSubElementRect(ButtonSubElement buttonElement, State, const QRect& originalRect) const = 0;

    virtual int findFrameLineWidth() const = 0;
    virtual int simplePixelMetric(PixelMetric, State = State_None) const = 0;
    virtual int buttonMargin(State, const QRect& originalRect) const = 0;
    virtual int sliderLength(Qt::Orientation) const = 0;
    virtual int sliderThickness(Qt::Orientation) const = 0;
    virtual int progressBarChunkWidth(const QSize&) const = 0;
    virtual void getButtonMetrics(QString* buttonFontFamily, int* buttonFontPixelSize) const = 0;

    virtual QSize comboBoxSizeFromContents(State, const QSize& contentsSize) const = 0;
    virtual QSize pushButtonSizeFromContents(State, const QSize& contentsSize) const = 0;

    enum ButtonType {
        PushButton,
        RadioButton,
        CheckBox
    };

    virtual void paintButton(QPainter*, ButtonType, const QStyleFacadeOption&) = 0;
    virtual void paintTextField(QPainter*, const QStyleFacadeOption&) = 0;
    virtual void paintComboBox(QPainter*, const QStyleFacadeOption&) = 0;
    virtual void paintComboBoxArrow(QPainter*, const QStyleFacadeOption&) = 0;

    virtual void paintSliderTrack(QPainter*, const QStyleFacadeOption&) = 0;
    virtual void paintSliderThumb(QPainter*, const QStyleFacadeOption&) = 0;
    virtual void paintInnerSpinButton(QPainter*, const QStyleFacadeOption&, bool spinBoxUp) = 0;
    virtual void paintProgressBar(QPainter*, const QStyleFacadeOption&, double progress, double animationProgress) = 0;

    virtual int scrollBarExtent(bool mini) = 0;
    virtual bool scrollBarMiddleClickAbsolutePositionStyleHint() const = 0;
    virtual void paintScrollCorner(QPainter*, const QRect&) = 0;

    virtual SubControl hitTestScrollBar(const QStyleFacadeOption&, const QPoint& pos) = 0;
    virtual QRect scrollBarSubControlRect(const QStyleFacadeOption&, SubControl) = 0;
    virtual void paintScrollBar(QPainter*, const QStyleFacadeOption&) = 0;

    virtual QObject* widgetForPainter(QPainter*) = 0;

    virtual bool isValid() const = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QStyleFacade::State)

struct QStyleFacadeOption {
    QStyleFacadeOption()
        : state(QStyleFacade::State_None)
        , direction(Qt::LayoutDirectionAuto)
    {
        slider.orientation = Qt::Horizontal;
        slider.upsideDown = false;
        slider.minimum = 0;
        slider.maximum = 0;
        slider.position = 0;
        slider.value = 0;
        slider.singleStep = 0;
        slider.pageStep = 0;
        slider.activeSubControls = QStyleFacade::SC_None;
    }

    QStyleFacade::State state;
    QRect rect;
    Qt::LayoutDirection direction;
    QPalette palette;

    // Slider features
    struct {
        Qt::Orientation orientation;
        bool upsideDown;
        int minimum;
        int maximum;
        int position;
        int value;
        int singleStep;
        int pageStep;
        QStyleFacade::SubControl activeSubControls;
    } slider;
};

}

#endif // QStyleFacade_h
