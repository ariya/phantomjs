/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

//! [0]
    opt.initFrom(q);
        if (down)
        opt.state |= QStyle::State_Sunken;
    if (tristate && noChange)
        opt.state |= QStyle::State_NoChange;
    else
        opt.state |= checked ? QStyle::State_On :
        QStyle::State_Off;
    if (q->testAttribute(Qt::WA_Hover) &&  q->underMouse()) {
        if (hovering)
        opt.state |= QStyle::State_MouseOver;
        else
        opt.state &= ~QStyle::State_MouseOver;
    }
    opt.text = text;
    opt.icon = icon;
    opt.iconSize = q->iconSize();
//! [0]


//! [1]
    state = QStyle::State_None;
    if (widget->isEnabled())
        state |= QStyle::State_Enabled;
    if (widget->hasFocus())
        state |= QStyle::State_HasFocus;
    if (widget->window()->testAttribute(Qt::WA_KeyboardFocusChange))
        state |= QStyle::State_KeyboardFocusChange;
    if (widget->underMouse())
        state |= QStyle::State_MouseOver;
    if (widget->window()->isActiveWindow())
        state |= QStyle::State_Active;
#ifdef Q_WS_MAC
    extern bool qt_mac_can_clickThrough(const QWidget *w); //qwidget_mac.cpp
    if (!(state & QStyle::State_Active) && !qt_mac_can_clickThrough(widget))
        state &= ~QStyle::State_Enabled;
#endif
#ifdef QT_KEYPAD_NAVIGATION
    if (widget->hasEditFocus())
        state |= QStyle::State_HasEditFocus;
#endif

    direction = widget->layoutDirection();
    rect = widget->rect();
    palette = widget->palette();
    fontMetrics = widget->fontMetrics();
//! [1]


//! [2]
    QStylePainter p(this);
    QStyleOptionButton opt = d->getStyleOption();
    p.drawControl(QStyle::CE_CheckBox, opt);
//! [2]


//! [3]
    QStyleOptionButton subopt = *btn;
    subopt.rect = subElementRect(SE_CheckBoxIndicator, btn, widget);
    drawPrimitive(PE_IndicatorCheckBox, &subopt, p, widget);
    subopt.rect = subElementRect(SE_CheckBoxContents, btn, widget);
    drawControl(CE_CheckBoxLabel, &subopt, p, widget);

    if (btn->state & State_HasFocus) {
        QStyleOptionFocusRect fropt;
        fropt.QStyleOption::operator=(*btn);
        fropt.rect = subElementRect(SE_CheckBoxFocusRect, btn, widget);
        drawPrimitive(PE_FrameFocusRect, &fropt, p, widget);
    }
//! [3]


//! [4]
    const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt);
    uint alignment = visualAlignment(btn->direction, Qt::AlignLeft | Qt::AlignVCenter);

    if (!styleHint(SH_UnderlineShortcut, btn, widget))
        alignment |= Qt::TextHideMnemonic;
    QPixmap pix;
    QRect textRect = btn->rect;
    if (!btn->icon.isNull()) {
        pix = btn->icon.pixmap(btn->iconSize, btn->state & State_Enabled ? QIcon::Normal : QIcon::Disabled);
        drawItemPixmap(p, btn->rect, alignment, pix);
        if (btn->direction == Qt::RightToLeft)
            textRect.setRight(textRect.right() - btn->iconSize.width() - 4);
        else
            textRect.setLeft(textRect.left() + btn->iconSize.width() + 4);
    }
    if (!btn->text.isEmpty()){
        drawItemText(p, textRect, alignment | Qt::TextShowMnemonic,
            btn->palette, btn->state & State_Enabled, btn->text, QPalette::WindowText);
    }
//! [4]
