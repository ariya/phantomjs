/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowscestyle.h"

#if !defined(QT_NO_STYLE_WINDOWSCE) || defined(QT_PLUGIN)

#include "qpainterpath.h"
#include "qapplication.h"
#include "qdockwidget.h"
#include "qtoolbar.h"
#include "qpaintengine.h"
#include "qpainter.h"
#include "qstyleoption.h"
#include "qwindowscestyle_p.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

static const int windowsItemFrame        =  2; // menu item frame width
static const int windowsItemHMargin      =  3; // menu item hor text margin
static const int windowsItemVMargin      =  2; // menu item ver text margin
static const int windowsArrowHMargin	 =  6; // arrow horizontal margin
static const int windowsRightBorder      = 15; // right border on windows
static const int windowsCheckMarkWidth   = 14; // checkmarks width on windows

static const int windowsCEitemViewCheckBoxSize   = 14;
static const int windowsCEFrameGroupBoxOffset    = 9;
static const int windowsCEIndicatorSize          = 14;
static const int windowsCEExclusiveIndicatorSize = 14;
static const int windowsCESliderThickness        = 24;
static const int windowsCEIconSize               = 16;

static const QColor windowsCECheckBoxGradientColorBegin = QColor(222, 224, 214);
static const QColor windowsCECheckBoxGradientColorEnd   =  QColor(255, 255, 255);

enum QSliderDirection { SlUp, SlDown, SlLeft, SlRight };

QWindowsCEStyle::QWindowsCEStyle() : QWindowsStyle() {
    qApp->setEffectEnabled(Qt::UI_FadeMenu, false);
    qApp->setEffectEnabled(Qt::UI_AnimateMenu, false);
}

void QWindowsCEStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                    QPainter *painter, const QWidget *widget) const {

    bool doRestore = false;
    QRect rect = option->rect;

    switch (element) {
    case PE_PanelButtonTool: {
        if (
#ifndef QT_NO_TOOLBAR
             (widget && qobject_cast<QToolBar*>(widget->parentWidget())) ||
#endif
#ifndef QT_NO_DOCKWIDGET
             (widget && widget->inherits("QDockWidgetTitleButton")) ||
#endif
            (option->state & (State_Sunken | State_On)))
               QWindowsCEStylePrivate::drawWinCEButton(painter, option->rect.adjusted(0, 0, 0, 0),
                  option->palette, option->state & (State_Sunken | State_On),
                  &option->palette.button());
        if (option->state & (State_On)){
            QBrush fill = QBrush(option->palette.midlight().color(), Qt::Dense4Pattern);
            painter->fillRect(option->rect.adjusted(windowsItemFrame , windowsItemFrame ,
                              -windowsItemFrame , -windowsItemFrame ), fill);
        }
        break; }
    case PE_IndicatorButtonDropDown:
        QWindowsCEStylePrivate::drawWinCEButton(painter, option->rect, option->palette,
            option->state & (State_Sunken | State_On),
            &option->palette.brush(QPalette::Button));
        break;
#ifndef QT_NO_TABBAR
        case PE_IndicatorTabTear:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            bool rtl = tab->direction == Qt::RightToLeft;
            QRect rect = tab->rect;
            QPainterPath path;
            rect.setTop(rect.top() + ((tab->state & State_Selected) ? 1 : 3));
            rect.setBottom(rect.bottom() - ((tab->state & State_Selected) ? 0 : 2));
            path.moveTo(QPoint(rtl ? rect.right() : rect.left(), rect.top()));
            int count = 3;
            for(int jags = 1; jags <= count; ++jags, rtl = !rtl)
                path.lineTo(QPoint(rtl ? rect.left() : rect.right(), rect.top() + jags * rect.height()/count));

            painter->setPen(QPen(tab->palette.light(), qreal(.8)));
            painter->setBrush(tab->palette.background());
            painter->setRenderHint(QPainter::Antialiasing);
            painter->drawPath(path);
        }
        break;
#endif //QT_NO_TABBAR
#ifndef QT_NO_TOOLBAR
        case PE_IndicatorToolBarSeparator:
            //nothing to draw on WindowsCE
            break;
        case PE_IndicatorToolBarHandle:
            painter->save();
            painter->translate(option->rect.x(), option->rect.y());
            if (option->state & State_Horizontal) {
                int x = option->rect.width() / 2 - 4;
                if (QApplication::layoutDirection() == Qt::RightToLeft)
                    x -= 2;
                if (option->rect.height() > 4) {
                    QWindowsCEStylePrivate::drawWinCEButton(painter,x - 1, 0, 7, option->rect.height(),
                        option->palette, false,  0);
                    QWindowsCEStylePrivate::drawWinCEPanel(painter, x, 1, 3, option->rect.height() - 1,
                        option->palette, false, 0);
                    QWindowsCEStylePrivate::drawWinCEPanel(painter, x + 3, 1, 3, option->rect.height() - 1,
                        option->palette, false, 0);
                    painter->setPen(option->palette.button().color());
                    painter->drawLine(x + 4, 2, x + 4,option->rect.height() - 2);
                }
            } else {
                if (option->rect.width() > 4) {
                    int y = option->rect.height() / 2 - 4;
                    QWindowsCEStylePrivate::drawWinCEPanel(painter, 2, y, option->rect.width() - 2, 3,
                        option->palette, false,  0);
                    QWindowsCEStylePrivate::drawWinCEPanel(painter, 2, y + 3, option->rect.width() - 2, 3,
                        option->palette, false,  0);
                }
            }
            painter->restore();
            break;

#endif // QT_NO_TOOLBAR
        case PE_FrameButtonTool: {
#ifndef QT_NO_DOCKWIDGET
            if (widget && widget->inherits("QDockWidgetTitleButton")) {
                if (const QDockWidget *dw = qobject_cast<const QDockWidget *>(widget->parent()))
                    if (dw->isFloating()){
                        QWindowsCEStylePrivate::drawWinCEButton(painter, option->rect.adjusted(1, 1, 0, 0),
                                                                option->palette, option->state & (State_Sunken | State_On),
                            &option->palette.button());
                        return;
                    }
            }
#endif // QT_NO_DOCKWIDGET
        QBrush fill;
        bool stippled;
        bool panel = (element == PE_PanelButtonTool);
        if ((!(option->state & State_Sunken ))
            && (!(option->state & State_Enabled)
            || ((option->state & State_Enabled ) && !(option->state & State_MouseOver)))
            && (option->state & State_On)) {
                fill = QBrush(option->palette.light().color(), Qt::Dense4Pattern);
                stippled = true;
        } else {
            fill = option->palette.brush(QPalette::Button);
            stippled = false;
        }
        if (option->state & (State_Raised | State_Sunken | State_On)) {
            if (option->state & State_AutoRaise) {
                if(option->state & (State_Enabled | State_Sunken | State_On)){
                    if (panel)
                        QWindowsCEStylePrivate::drawWinCEPanel(painter, option->rect, option->palette,
                        option->state & (State_Sunken | State_On), &fill);
                    else
                        qDrawShadeRect(painter, option->rect, option->palette,
                        option->state & (State_Sunken | State_On), 1);
                }
                if (stippled) {
                    painter->setPen(option->palette.button().color());
                    painter->drawRect(option->rect.adjusted(1, 1, -2, -2));
                }
            } else {
                QWindowsCEStylePrivate::drawWinCEButton(painter, option->rect, option->palette,
                    option->state & (State_Sunken | State_On), panel ? &fill : 0);
            }
        } else {
            painter->fillRect(option->rect, fill);
        }
        break; }

    case PE_PanelButtonBevel: {
        QBrush fill;
        bool panel = element != PE_FrameButtonBevel;
        painter->setBrushOrigin(option->rect.topLeft());
        if (!(option->state & State_Sunken) && (option->state & State_On))
            fill = QBrush(option->palette.light().color(), Qt::Dense4Pattern);
        else
            fill = option->palette.brush(QPalette::Button);

        if (option->state & (State_Raised | State_On | State_Sunken)) {
            QWindowsCEStylePrivate::drawWinCEButton(painter, option->rect, option->palette,
                                                    option->state & (State_Sunken | State_On),
                panel ? &fill : 0); ;
        } else {
            if (panel)
                painter->fillRect(option->rect, fill);
            else
                painter->drawRect(option->rect);
        }
        break; }

    case PE_FrameGroupBox:
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            QRect fr = frame->rect;
            painter->setPen(frame->palette.shadow().color());
            painter->drawRect(fr.x(), fr.y(), fr.x() + fr.width() - 1,
                              fr.y() + fr.height() - windowsCEFrameGroupBoxOffset);
        }
        break;

   case PE_IndicatorCheckBox: {
        QBrush fill;
        if (option->state & State_NoChange)
            fill = QBrush(option->palette.base().color(), Qt::Dense4Pattern);
        else if (option->state & State_Sunken)
            fill = option->palette.button();
        else if (option->state & State_Enabled)
            fill = option->palette.base();
        else
            fill = option->palette.background();
        painter->save();
        doRestore = true;
        painter->fillRect(option->rect,fill);
        painter->setPen(option->palette.dark().color());
        painter->drawRect(option->rect);
        painter->setPen(option->palette.shadow().color());
        painter->drawLine(option->rect.x() + 1,option->rect.y() + 1,
                          option->rect.x() + option->rect.width() - 1, option->rect.y() + 1);
        painter->drawLine(option->rect.x() + 1,option->rect.y() + 1,
                          option->rect.x() + 1, option->rect.y() + option->rect.height() - 1);
        //fall through...
    }
    case PE_IndicatorViewItemCheck:
    case PE_Q3CheckListIndicator: {
        if (!doRestore) {
            painter->save();
            doRestore = true;
        }
        int arrowSize= 2;
        if (element == PE_Q3CheckListIndicator || element == PE_IndicatorViewItemCheck) {
            QLinearGradient linearGradient(QPoint(option->rect.x(),option->rect.y()), QPoint(option->rect.x()+option->rect.width(),
                                           option->rect.y()+option->rect.height()));
            linearGradient.setColorAt(0, windowsCECheckBoxGradientColorBegin);
            linearGradient.setColorAt(1, windowsCECheckBoxGradientColorEnd);
            painter->setBrush(linearGradient);
            painter->setPen(Qt::NoPen);
            if (option->state & State_NoChange)
                painter->setBrush(option->palette.brush(QPalette::Button));
            painter->setPen(option->palette.link().color());
            painter->drawRect(option->rect.x(), option->rect.y(), windowsCEitemViewCheckBoxSize, windowsCEitemViewCheckBoxSize);
            painter->setPen(option->palette.brightText().color());
            arrowSize= 3;
        }
        if (!(option->state & State_Off)) {
            QLineF lines[9];
            int i, xx, yy;
            xx = option->rect.x() + 4;
            yy = option->rect.y() + 6;
            for (i = 0; i < 4; ++i) {
                lines[i] = QLineF(xx, yy, xx, yy + arrowSize);
                ++xx;
                ++yy;
            }
            yy -= 2;
            for (i = 4; i < 9; ++i) {
                lines[i] = QLineF(xx, yy, xx, yy + arrowSize);
                ++xx;
                --yy;
            }
            painter->drawLines(lines, 9);
        }
        if (doRestore)
            painter->restore();

        break; }
    case PE_IndicatorRadioButton: {
        QRect ir = option->rect;
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(option->palette.light());
        painter->drawEllipse(option->rect);
        painter->setPen(option->palette.shadow().color());
        painter->setBrush(option->palette.shadow().color());
        painter->drawArc(option->rect, 0, 360 * 16);
        painter->drawArc(option->rect.x() + 1, option->rect.y() + 1, option->rect.width() - 2,
                         option->rect.height() - 2, 40 * 16, 180 * 16);
        painter->setPen(option->palette.light().color());
        painter->drawPoint(option->rect.x() + 11, option->rect.y() + 3);
        painter->drawPoint(option->rect.x() + 3,option->rect.y() + 3);
        painter->setPen(option->palette.shadow().color());
        painter->drawPoint(option->rect.x() +3,option->rect.y() + 12);
        if (option->state & (State_Sunken | State_On)) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(option->palette.text());
            painter->drawEllipse(option->rect.x() +3,option->rect.y()+ 2,9,10);
        }
        painter->restore();
        break; }
   case PE_PanelMenuBar:
       painter->save();
       painter->setPen(option->palette.shadow().color());
       painter->drawRect(option->rect);
       painter->restore();
       break;
   case PE_PanelButtonCommand:
       if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
           QBrush fill;
           State flags = option->state;
           QPalette pal = option->palette;
           QRect r = option->rect;
           if (! (flags & State_Sunken) && (flags & State_On))
               fill = QBrush(pal.light().color(), Qt::Dense4Pattern);
           else
               fill = pal.brush(QPalette::Button);
           if (btn->features & QStyleOptionButton::DefaultButton && flags & State_Sunken) {
               painter->setPen(pal.dark().color());
               painter->setBrush(fill);
               painter->drawRect(r.adjusted(0, 0, -1, -1));
           } else if (flags & (State_Raised | State_Sunken | State_On | State_Sunken)) {
               QWindowsCEStylePrivate::drawWinCEButton(painter, r, pal, flags & (State_Sunken | State_On),
                   &fill);
           } else {
               painter->fillRect(r, fill);
           }

       }
       break;
   case PE_FrameDefaultButton: {
       painter->setPen(option->palette.shadow().color());
       QRect rect = option->rect;
       rect.adjust(0, 0, -1, -1);
       painter->drawRect(rect);
       break; }
   case PE_IndicatorSpinPlus:
   case PE_IndicatorSpinMinus: {
       QRect r = option->rect;
       int fw = pixelMetric(PM_DefaultFrameWidth, option, widget)+2;
       QRect br = r.adjusted(fw, fw, -fw, -fw);
       int offset = (option->state & State_Sunken) ? 1 : 0;
       int step = (br.width() + 4) / 5;
       painter->fillRect(br.x() + offset, br.y() + offset +br.height() / 2 - step / 2,
           br.width(), step,
           option->palette.buttonText());
       if (element == PE_IndicatorSpinPlus)
           painter->fillRect(br.x() + br.width() / 2 - step / 2 + offset, br.y() + offset+4,
           step, br.height()-7,
           option->palette.buttonText());
       break; }
    case PE_IndicatorSpinUp:
    case PE_IndicatorSpinDown: {
        painter->save();
            QPoint points[7];
            switch (element) {
                case PE_IndicatorSpinUp:
                    points[0] = QPoint(-2, -4);
                    points[1] = QPoint(-2, 2);
                    points[2] = QPoint(-1, -3);
                    points[3] = QPoint(-1, 1);
                    points[4] = QPoint(0, -2);
                    points[5] = QPoint(0, 0);
                    points[6] = QPoint(1, -1);
                break;
                case PE_IndicatorSpinDown:
                    points[0] = QPoint(0, -4);
                    points[1] = QPoint(0, 2);
                    points[2] = QPoint(-1, -3);
                    points[3] = QPoint(-1, 1);
                    points[4] = QPoint(-2, -2);
                    points[5] = QPoint(-2, 0);
                    points[6] = QPoint(-3, -1);
                break;
                default:
                break;
            }
            if (option->state & State_Sunken)
                painter->translate(pixelMetric(PM_ButtonShiftHorizontal),
                pixelMetric(PM_ButtonShiftVertical));
            if (option->state & State_Enabled) {
                painter->translate(option->rect.x() + option->rect.width() / 2,
                                   option->rect.y() + option->rect.height() / 2);
                painter->setPen(option->palette.buttonText().color());
                painter->drawLine(points[0], points[1]);
                painter->drawLine(points[2], points[3]);
                painter->drawLine(points[4], points[5]);
                painter->drawPoint(points[6]);
            } else {
                painter->translate(option->rect.x() + option->rect.width() / 2 + 1,
                                   option->rect.y() + option->rect.height() / 2 + 1);
                painter->setPen(option->palette.light().color());
                painter->drawLine(points[0], points[1]);
                painter->drawLine(points[2], points[3]);
                painter->drawLine(points[4], points[5]);
                painter->drawPoint(points[6]);
                painter->translate(-1, -1);
                painter->setPen(option->palette.mid().color());
                painter->drawLine(points[0], points[1]);
                painter->drawLine(points[2], points[3]);
                painter->drawLine(points[4], points[5]);
                painter->drawPoint(points[6]);
            }

        painter->restore();
        break; }
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowRight:
    case PE_IndicatorArrowLeft: {
            painter->save();
            QPoint points[9];
            switch (element) {
                case PE_IndicatorArrowUp:

                    points[0] = QPoint(-4, 2);
                    points[1] = QPoint(4, 2);
                    points[2] = QPoint(-3, 1);
                    points[3] = QPoint(3, 1);
                    points[4] = QPoint(-2, 0);
                    points[5] = QPoint(2, 0);
                    points[6] = QPoint(-1, -1);
                    points[7] = QPoint(1, -1);
                    points[8] = QPoint(0, -2);
                break;
                case PE_IndicatorArrowDown:

                    points[0] = QPoint(-4, -2);
                    points[1] = QPoint(4, -2);
                    points[2] = QPoint(-3, -1);
                    points[3] = QPoint(3, -1);
                    points[4] = QPoint(-2, 0);
                    points[5] = QPoint(2, 0);
                    points[6] = QPoint(-1, 1);
                    points[7] = QPoint(1, 1);
                    points[8] = QPoint(0, 2);
                break;
                case PE_IndicatorArrowRight:
                    points[0] = QPoint(-3, -4);
                    points[1] = QPoint(-3, 4);
                    points[2] = QPoint(-2, -3);
                    points[3] = QPoint(-2, 3);
                    points[4] = QPoint(-1, -2);
                    points[5] = QPoint(-1, 2);
                    points[6] = QPoint(0, -1);
                    points[7] = QPoint(0, 1);
                    points[8] = QPoint(1, 0);
                break;
                case PE_IndicatorArrowLeft:
                    points[0] = QPoint(1, -4);
                    points[1] = QPoint(1, 4);
                    points[2] = QPoint(0, -3);
                    points[3] = QPoint(0, 3);
                    points[4] = QPoint(-1, -2);
                    points[5] = QPoint(-1, 2);
                    points[6] = QPoint(-2, -1);
                    points[7] = QPoint(-2, 1);
                    points[8] = QPoint(-3, 0);
                break;
                default:
                break;
            }
            if (option->state & State_Sunken)
                painter->translate(pixelMetric(PM_ButtonShiftHorizontal),
                pixelMetric(PM_ButtonShiftVertical));
            if (option->state & State_Enabled) {
                painter->translate(option->rect.x() + option->rect.width() / 2,
                    option->rect.y() + option->rect.height() / 2);
                painter->setPen(option->palette.buttonText().color());
                painter->drawLine(points[0], points[1]);
                painter->drawLine(points[2], points[3]);
                painter->drawLine(points[4], points[5]);
                painter->drawLine(points[6], points[7]);
                painter->drawPoint(points[8]);
            } else {
                painter->translate(option->rect.x() + option->rect.width() / 2 + 1,
                    option->rect.y() + option->rect.height() / 2 + 1);
                painter->setPen(option->palette.light().color());
                painter->drawLine(points[0], points[1]);
                painter->drawLine(points[2], points[3]);
                painter->drawLine(points[4], points[5]);
                painter->drawLine(points[6], points[7]);
                painter->drawPoint(points[8]);
                painter->translate(-1, -1);
                painter->setPen(option->palette.mid().color());
                painter->drawLine(points[0], points[1]);
                painter->drawLine(points[2], points[3]);
                painter->drawLine(points[4], points[5]);
                painter->drawLine(points[6], points[7]);
                painter->drawPoint(points[8]);
            }
        painter->restore();
        break; }

    case PE_FrameWindow: {
        QPalette popupPal = option->palette;
        popupPal.setColor(QPalette::Light, option->palette.background().color());
        popupPal.setColor(QPalette::Midlight, option->palette.light().color());
        QWindowsCEStylePrivate::drawWinCEPanel(painter, option->rect, popupPal, option->state & State_Sunken);
        break; }

    case PE_Frame:
    case PE_FrameMenu:
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            QPalette popupPal = frame->palette;
            QRect r = frame->rect;
            qDrawPlainRect(painter, r, frame->palette.shadow().color(),1);
        }
        break;
    case PE_FrameStatusBar:
        QWindowsCEStylePrivate::drawWinCEPanel(painter, option->rect, option->palette, true, 0);
        break;

    case PE_FrameTabWidget: {
        QRect rect = option->rect;
        QPalette pal = option->palette;
        QWindowsCEStylePrivate::drawWinCEButton(painter, option->rect, option->palette, false, 0);
        break; }
    default:
        QWindowsStyle::drawPrimitive(element, option, painter, widget);
        break;
    }
}

void QWindowsCEStyle::drawControl(ControlElement element, const QStyleOption *option,
                                  QPainter *painter, const QWidget *widget) const {
   switch (element) {
   #ifndef QT_NO_MENU
   case CE_MenuTearoff: {
        if(option->state & State_Selected) {
            if(pixelMetric(PM_MenuPanelWidth, option, widget) > 1)
                qDrawShadePanel(painter, option->rect.x(), option->rect.y(), option->rect.width(),
                                option->rect.height(), option->palette, false, 2,
                                &option->palette.brush(QPalette::Button));
            else
                qDrawShadePanel(painter, option->rect.x() + 1, option->rect.y() + 1, option->rect.width() - 2,
                                option->rect.height() - 2, option->palette, true, 1, &option->palette.brush(QPalette::Button));
        } else {
            painter->fillRect(option->rect, option->palette.brush(QPalette::Button));
        }
        painter->setPen(QPen(option->palette.dark().color(), 1, Qt::DashLine));
        painter->drawLine(option->rect.x()+2, option->rect.y()+option->rect.height()/2-1, option->rect.x()+option->rect.width()-4,
                    option->rect.y()+option->rect.height()/2-1);
        painter->setPen(QPen(option->palette.light().color(), 1, Qt::DashLine));
        painter->drawLine(option->rect.x()+2, option->rect.y()+option->rect.height()/2, option->rect.x()+option->rect.width()-4,
                    option->rect.y()+option->rect.height()/2);
        break; }


   case CE_MenuBarItem:
        if (const QStyleOptionMenuItem *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            bool active = mbi->state & State_Selected;
            bool hasFocus = mbi->state & State_HasFocus;
            bool down = mbi->state & State_Sunken;
            QStyleOptionMenuItem newMbi = *mbi;
            if (active || hasFocus) {
                QBrush b = mbi->palette.brush(QPalette::Highlight);
                if (active && down) {
                    painter->fillRect(mbi->rect.adjusted(0, 1, 0, -1), b);
                }
            }
            uint alignment = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip
                            | Qt::TextSingleLine;
            if (!styleHint(SH_UnderlineShortcut, mbi, widget))
                alignment |= Qt::TextHideMnemonic;

            painter->save();
            QFont f = painter->font();
            f.setBold(true);
            painter->setFont(f);
            QPixmap pix = mbi->icon.pixmap(pixelMetric(PM_SmallIconSize),
                                          (mbi->state & State_Enabled) ? QIcon::Normal : QIcon::Disabled);
            if (!pix.isNull())
                drawItemPixmap(painter,mbi->rect, alignment, pix);
            else
                if (active && down)
                  drawItemText(painter, mbi->rect, alignment, mbi->palette, mbi->state & State_Enabled,
                             mbi->text, QPalette::Light);
                else
                  drawItemText(painter, mbi->rect, alignment, mbi->palette, mbi->state & State_Enabled,
                             mbi->text, QPalette::ButtonText);
            painter->restore();
        }
        break;

   case CE_MenuBarEmptyArea:
        painter->save();
        painter->setPen(option->palette.shadow().color());
        if (widget && !widget->testAttribute(Qt::WA_NoSystemBackground)) {
            painter->eraseRect(option->rect);
            QRect r = option->rect;
            painter->drawLine(r.x() + 1, r.y() + 1, r.x()+ 1, r.y()+ r.height() - 2);
            painter->drawLine(r.x() - 2 + r.width(), r.y()  + 1, r.x() - 2 + r.width(), r.y() + r.height() - 2);
            painter->drawLine(r.x() + 1, r.y() +1, r.x() - 1 + r.width(), r.y() + 1);
            painter->drawLine(r.x() + 1, r.y() + r.height()-2 , r.x() - 2 + r.width(), r.y() + r.height() - 2);
        }
        painter->restore();
        break;

    case CE_MenuItem:
          if (const QStyleOptionMenuItem *menuitem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            int x, y, w, h;
            menuitem->rect.getRect(&x, &y, &w, &h);
            int tab = menuitem->tabWidth;
            bool dis = !(menuitem->state & State_Enabled);
            bool checked = menuitem->checkType != QStyleOptionMenuItem::NotCheckable
                            ? menuitem->checked : false;
            bool act = menuitem->state & State_Selected;

            // windows always has a check column, regardless whether we have an icon or not
            int checkcol = qMax(menuitem->maxIconWidth, windowsCheckMarkWidth);
            QBrush fill = menuitem->palette.brush(act ? QPalette::Highlight : QPalette::Button);
            painter->fillRect(menuitem->rect.adjusted(1, 1, 0, 0), fill);

            if (menuitem->menuItemType == QStyleOptionMenuItem::Separator) {
                int yoff = y-1 + h / 2;
                painter->setPen(menuitem->palette.shadow().color());
                painter->drawLine(x + 4, yoff + 1, x + w - 8, yoff + 1);
                return;
            }

            QRect vCheckRect = visualRect(option->direction, menuitem->rect, QRect(menuitem->rect.x(),
                                          menuitem->rect.y(), checkcol, menuitem->rect.height()));
            if (checked) {
                if (act && !dis) {
                    qDrawPlainRect(painter, vCheckRect,
                        menuitem->palette.button().color(), 1,
                                    &menuitem->palette.brush(QPalette::Button));
                } else {
                    QBrush fill(menuitem->palette.button().color(), Qt::Dense4Pattern);
                    qDrawPlainRect(painter, vCheckRect,menuitem->palette.button().color(), 1, &fill);
                }
            } else if (!act) {
                painter->fillRect(vCheckRect, menuitem->palette.brush(QPalette::Button));
            }
            // On Windows Style, if we have a checkable item and an icon we
            // draw the icon recessed to indicate an item is checked. If we
            // have no icon, we draw a checkmark instead.
            if (!menuitem->icon.isNull()) {
                QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;
                if (act && !dis)
                    mode = QIcon::Active;
                QPixmap pixmap;
                if (checked)
                    pixmap = menuitem->icon.pixmap(pixelMetric(PM_SmallIconSize), mode, QIcon::On);
                else
                    pixmap = menuitem->icon.pixmap(pixelMetric(PM_SmallIconSize), mode);
                int pixw = pixmap.width();
                int pixh = pixmap.height();
                if (act && !dis && !checked)
                    qDrawPlainRect(painter, vCheckRect,  menuitem->palette.button().color(), 1,
                                    &menuitem->palette.brush(QPalette::Button));
                QRect pmr(0, 0, pixw, pixh);
                pmr.moveCenter(vCheckRect.center());
                painter->setPen(menuitem->palette.text().color());
                painter->drawPixmap(pmr.topLeft(), pixmap);
            } else if (checked) {
                QStyleOptionMenuItem newMi = *menuitem;
                newMi.state = State_None;
                if (!dis)
                    newMi.state |= State_Enabled;
                if (act)
                    newMi.state |= State_On;
                newMi.rect = visualRect(option->direction, menuitem->rect, QRect(menuitem->rect.x()
                           + windowsItemFrame, menuitem->rect.y() + windowsItemFrame,
                             checkcol - 2 * windowsItemFrame, menuitem->rect.height() - 2*windowsItemFrame));
                drawPrimitive(PE_IndicatorMenuCheckMark, &newMi, painter, widget);
            }
            painter->setPen(act ? menuitem->palette.highlightedText().color() : menuitem->palette.buttonText().color());

            QColor discol;
            if (dis) {
                discol = menuitem->palette.text().color();
                painter->setPen(discol);
            }
            int xm = windowsItemFrame + checkcol + windowsItemHMargin;
            int xpos = menuitem->rect.x() + xm;
            QRect textRect(xpos, y + windowsItemVMargin, w - xm - windowsRightBorder - tab + 1, h - 2 * windowsItemVMargin);
            QRect vTextRect = visualRect(option->direction, menuitem->rect, textRect);
            QString s = menuitem->text;
            if (!s.isEmpty()) {                     // draw text
                painter->save();
                int t = s.indexOf(QLatin1Char('\t'));
                int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!styleHint(SH_UnderlineShortcut, menuitem, widget))
                    text_flags |= Qt::TextHideMnemonic;
                text_flags |= Qt::AlignLeft;
                if (t >= 0) {
                    QRect vShortcutRect = visualRect(option->direction, menuitem->rect,
                        QRect(textRect.topRight(), QPoint(menuitem->rect.right(), textRect.bottom())));
                    if (dis && !act)
                        painter->setPen(discol);
                    painter->drawText(vShortcutRect, text_flags, s.mid(t + 1));
                    s = s.left(t);
                }
                QFont font = menuitem->font;
                if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                    font.setBold(true);
                painter->setFont(font);
                if (dis && !act)
                    painter->setPen(discol);
                painter->drawText(vTextRect, text_flags, s.left(t));
                painter->restore();
            }
            if (menuitem->menuItemType == QStyleOptionMenuItem::SubMenu) {// draw sub menu arrow
                int dim = (h - 2 * windowsItemFrame) / 2;
                PrimitiveElement arrow;
                arrow = (option->direction == Qt::RightToLeft) ? PE_IndicatorSpinDown : PE_IndicatorSpinUp;
                xpos = x + w - windowsArrowHMargin - windowsItemFrame - dim;
                QRect  vSubMenuRect = visualRect(option->direction, menuitem->rect, QRect(xpos, y + h / 2 - dim / 2, dim, dim));
                QStyleOptionMenuItem newMI = *menuitem;
                newMI.rect = vSubMenuRect;
                newMI.state = dis ? State_None : State_Enabled;
                if (act)
                    newMI.palette.setColor(QPalette::ButtonText,
                                           newMI.palette.highlightedText().color());
                drawPrimitive(arrow, &newMI, painter, widget);
            }
        }
        break;
#endif // QT_NO_MENU
    case CE_MenuVMargin:
        painter->fillRect(option->rect, Qt::white);
        break;
    case CE_MenuEmptyArea:
        QWindowsStyle::drawControl(element,option, painter, widget);
        break;

#ifndef QT_NO_TABBAR
        case CE_TabBarTab:
            if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
                drawControl(CE_TabBarTabShape, tab, painter, widget);
                drawControl(CE_TabBarTabLabel, tab, painter, widget);
            }
            break;
    case CE_TabBarTabShape:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            bool rtlHorTabs = (tab->direction == Qt::RightToLeft
                               && (tab->shape == QTabBar::RoundedNorth
                                   || tab->shape == QTabBar::RoundedSouth));
            bool selected = tab->state & State_Selected;
            bool lastTab = ((!rtlHorTabs && tab->position == QStyleOptionTab::End)
                            || (rtlHorTabs
                                && tab->position == QStyleOptionTab::Beginning));
            bool firstTab = ((!rtlHorTabs
                               && tab->position == QStyleOptionTab::Beginning)
                             || (rtlHorTabs
                                 && tab->position == QStyleOptionTab::End));
            bool onlyOne = tab->position == QStyleOptionTab::OnlyOneTab;
            bool previousSelected =
                ((!rtlHorTabs
                  && tab->selectedPosition == QStyleOptionTab::PreviousIsSelected)
                || (rtlHorTabs
                    && tab->selectedPosition == QStyleOptionTab::NextIsSelected));
            bool nextSelected =
                ((!rtlHorTabs
                  && tab->selectedPosition == QStyleOptionTab::NextIsSelected)
                 || (rtlHorTabs
                     && tab->selectedPosition
                            == QStyleOptionTab::PreviousIsSelected));
            int tabBarAlignment = styleHint(SH_TabBar_Alignment, tab, widget);
            bool leftAligned = (!rtlHorTabs && tabBarAlignment == Qt::AlignLeft)
                                || (rtlHorTabs
                                    && tabBarAlignment == Qt::AlignRight);

            bool rightAligned = (!rtlHorTabs && tabBarAlignment == Qt::AlignRight)
                                 || (rtlHorTabs
                                         && tabBarAlignment == Qt::AlignLeft);
            QColor light = tab->palette.light().color();
            QColor midlight = tab->palette.midlight().color();
            QColor dark = tab->palette.dark().color();
            QColor shadow = tab->palette.shadow().color();
            QColor background = tab->palette.background().color();
            int borderThinkness = pixelMetric(PM_TabBarBaseOverlap, tab, widget);
            if (selected)
                borderThinkness /= 2;
            QRect r2(option->rect);
            int x1 = r2.left();
            int x2 = r2.right();
            int y1 = r2.top();
            int y2 = r2.bottom();
            switch (tab->shape) {
            default:
                QCommonStyle::drawControl(element, tab, painter, widget);
                break;
            case QTabBar::RoundedNorth: {
                if (!selected) {
                    y1 += 2;
                    x1 += firstTab ? borderThinkness : 0;
                    x2 -= lastTab ? borderThinkness : 0;
                }

                painter->fillRect(QRect(x1 + 1, y1 + 1, (x2 - x1) - 1, (y2 - y1) - 2), tab->palette.background());

                // Delete border
                if (selected) {
                    painter->setPen(background);
                    painter->drawLine(x1, y2 - 1, x2, y2 - 1);
                    painter->drawLine(x1, y2 + 1, x2, y2 + 1);
                    painter->drawLine(x1, y2, x2, y2);
                }
                // Left
                if (firstTab || selected || onlyOne || !previousSelected) {
                    painter->setPen(dark);
                    painter->drawLine(x1, y1 + 2, x1, y2 - ((onlyOne || firstTab) && selected && leftAligned ? 0 : borderThinkness));
                    painter->drawPoint(x1 + 1, y1 + 1);
                        painter->setPen(midlight);
                        painter->drawLine(x1 + 1, y1 + 2, x1 + 1, y2 -
                                         ((onlyOne || firstTab) && selected && leftAligned ? 0 : borderThinkness));

                }
                // Top
                {
                    int beg = x1 + (previousSelected ? 0 : 2);
                    int end = x2 - (nextSelected ? 0 : 2);
                    painter->setPen(dark);
                    painter->drawLine(beg, y1, end, y1);

                    painter->setPen(midlight);
                    painter->drawLine(beg, y1 + 1, end, y1 + 1);

                }
                // Right
                if (lastTab || selected || onlyOne || !nextSelected) {
                    painter->setPen(shadow);
                    painter->drawLine(x2, y1 + 2, x2, y2 - ((onlyOne || lastTab) && selected && rightAligned ? 0 : borderThinkness));
                    painter->drawPoint(x2 - 1, y1 + 1);
                    painter->setPen(dark);
                    painter->drawLine(x2 - 1, y1 + 2, x2 - 1, y2 - ((onlyOne || lastTab) && selected && rightAligned ? 0 : borderThinkness));
                }
                break; }
            case QTabBar::RoundedSouth: {
                if (!selected) {
                    y2 -= 2;
                    x1 += firstTab ? borderThinkness : 0;
                    x2 -= lastTab ? borderThinkness : 0;
                }

                painter->fillRect(QRect(x1 + 1, y1 + 2, (x2 - x1) - 1, (y2 - y1) - 1), tab->palette.background());

                // Delete border
                if (selected) {
                    painter->setPen(background);
                    painter->drawLine(x1, y1 + 1, x2 - 1, y1 + 1);
                    painter->drawLine(x1, y1 - 1, x2 - 1, y1 - 1);
                    painter->drawLine(x1, y1, x2 - 1, y1);
                }
                // Left
                if (firstTab || selected || onlyOne || !previousSelected) {
                    painter->setPen(dark);
                    painter->drawLine(x1, y2 - 2, x1, y1 + ((onlyOne || firstTab) && selected && leftAligned ? 0 : borderThinkness));
                    painter->drawPoint(x1 + 1, y2 - 1);
                    painter->setPen(midlight);
                    painter->drawLine(x1 + 1, y2 - 2, x1 + 1, y1 + ((onlyOne || firstTab) && selected && leftAligned ? 0 : borderThinkness));
                }
                // Bottom
                {
                    int beg = x1 + (previousSelected ? 0 : 2);
                    int end = x2 - (nextSelected ? 0 : 2);
                    painter->setPen(shadow);
                    painter->drawLine(beg, y2, end, y2);
                    painter->setPen(dark);
                    painter->drawLine(beg, y2 - 1, end, y2 - 1);
                }
                // Right
                if (lastTab || selected || onlyOne || !nextSelected) {
                    painter->setPen(shadow);
                    painter->drawLine(x2, y2 - 2, x2, y1 + ((onlyOne || lastTab) && selected &&
                                      rightAligned ? 0 : borderThinkness));
                    painter->drawPoint(x2 - 1, y2 - 1);
                    painter->setPen(dark);
                    painter->drawLine(x2 - 1, y2 - 2, x2 - 1, y1 + ((onlyOne || lastTab) && selected &&
                                      rightAligned ? 0 : borderThinkness));
                }
                break; }
            case QTabBar::RoundedWest: {
                if (!selected) {
                    x1 += 2;
                    y1 += firstTab ? borderThinkness : 0;
                    y2 -= lastTab ? borderThinkness : 0;
                }

                painter->fillRect(QRect(x1 + 1, y1 + 1, (x2 - x1) - 2, (y2 - y1) - 1), tab->palette.background());

                // Delete border
                if (selected) {
                    painter->setPen(background);
                    painter->drawLine(x2 - 1, y1, x2 - 1, y2);
                    painter->drawLine(x2, y1, x2, y2);
                }
                // Top
                if (firstTab || selected || onlyOne || !previousSelected) {
                    painter->setPen(dark);
                    painter->drawLine(x1 + 2, y1, x2 - ((onlyOne || firstTab) && selected &&
                                      leftAligned ? 0 : borderThinkness), y1);
                    painter->drawPoint(x1 + 1, y1 + 1);
                    painter->setPen(midlight);
                    painter->drawLine(x1 + 2, y1 + 1, x2 - ((onlyOne || firstTab) && selected &&
                                      leftAligned ? 0 : borderThinkness), y1 + 1);
                }
                // Left
                {
                    int beg = y1 + (previousSelected ? 0 : 2);
                    int end = y2 - (nextSelected ? 0 : 2);
                    painter->setPen(dark);
                    painter->drawLine(x1, beg, x1, end);
                    painter->setPen(midlight);
                    painter->drawLine(x1 + 1, beg, x1 + 1, end);
                }
                // Bottom
                if (lastTab || selected || onlyOne || !nextSelected) {
                    painter->setPen(shadow);
                    painter->drawLine(x1 + 3, y2, x2 - ((onlyOne || lastTab) && selected &&
                                      rightAligned ? 0 : borderThinkness), y2);
                    painter->drawPoint(x1 + 2, y2 - 1);
                    painter->setPen(dark);
                    painter->drawLine(x1 + 3, y2 - 1, x2 - ((onlyOne || lastTab) && selected &&
                                      rightAligned ? 0 : borderThinkness), y2 - 1);
                    painter->drawPoint(x1 + 1, y2 - 1);
                    painter->drawPoint(x1 + 2, y2);
                }
                break; }
            case QTabBar::RoundedEast: {
                if (!selected) {
                    x2 -= 2;
                    y1 += firstTab ? borderThinkness : 0;
                    y2 -= lastTab ? borderThinkness : 0;
                }

                painter->fillRect(QRect(x1 + 2, y1 + 1, (x2 - x1) - 1, (y2 - y1) - 1), tab->palette.background());

                // Delete border
                if (selected) {
                    painter->setPen(background);
                    painter->drawLine(x1 + 1, y1, x1 + 1, y2 - 1);
                    painter->drawLine(x1, y1, x1, y2 - 1);
                }
                // Top
                if (firstTab || selected || onlyOne || !previousSelected) {
                    painter->setPen(dark);
                    painter->drawLine(x2 - 2, y1, x1 + ((onlyOne || firstTab) && selected &&
                                      leftAligned ? 0 : borderThinkness), y1);
                    painter->drawPoint(x2 - 1, y1 + 1);
                    painter->setPen(midlight);
                    painter->drawLine(x2 - 3, y1 + 1, x1 + ((onlyOne || firstTab) &&
                                      selected && leftAligned ? 0 : borderThinkness), y1 + 1);
                    painter->drawPoint(x2 - 1, y1);

                }
                // Right
                {
                    int beg = y1 + (previousSelected ? 0 : 2);
                    int end = y2 - (nextSelected ? 0 : 2);
                    painter->setPen(shadow);
                    painter->drawLine(x2, beg, x2, end);
                    painter->setPen(dark);
                    painter->drawLine(x2 - 1, beg, x2 - 1, end);
                }
                // Bottom
                if (lastTab || selected || onlyOne || !nextSelected) {
                    painter->setPen(shadow);
                    painter->drawLine(x2 - 2, y2, x1 + ((onlyOne || lastTab) &&
                                      selected && rightAligned ? 0 : borderThinkness), y2);
                    painter->drawPoint(x2 - 1, y2 - 1);
                    painter->setPen(dark);
                    painter->drawLine(x2 - 2, y2 - 1, x1 + ((onlyOne || lastTab) &&
                                      selected && rightAligned ? 0 : borderThinkness), y2 - 1);
                }
                break; }
            }
        }
        break;
#endif // QT_NO_TABBAR

    case CE_ToolBar: {
        QRect rect = option->rect;
        painter->setPen(QPen(option->palette.dark().color()));
        painter->drawLine(rect.topRight().x()-1,
            rect.topRight().y(),
            rect.bottomRight().x()-1,
            rect.bottomRight().y());
        painter->drawLine(rect.bottomLeft().x(),
            rect.bottomLeft().y(),
            rect.bottomRight().x(),
            rect.bottomRight().y());
        painter->setPen(QPen(option->palette.light().color()));
        painter->drawLine(rect.topRight().x(),
            rect.topRight().y(),
            rect.bottomRight().x(),
            rect.bottomRight().y());
        painter->drawLine(rect.topLeft().x(),
            rect.topLeft().y(),
            rect.topRight().x(),
            rect.topRight().y());

        break; }
#ifndef QT_NO_SCROLLBAR
    case CE_ScrollBarSubLine:
    case CE_ScrollBarAddLine: {
        if (option->state & State_Sunken) {
            QStyleOption buttonOpt = *option;

            drawPrimitive(PE_PanelButtonBevel, &buttonOpt, painter, widget);
        } else {
            QStyleOption buttonOpt = *option;
            if (!(buttonOpt.state & State_Sunken))
                buttonOpt.state |= State_Raised;
            drawPrimitive(PE_PanelButtonBevel, &buttonOpt, painter, widget);
        }
        PrimitiveElement arrow;
        if (option->state & State_Horizontal) {
            if (element == CE_ScrollBarAddLine)
                arrow = option->direction == Qt::LeftToRight ? PE_IndicatorArrowRight : PE_IndicatorArrowLeft;
            else
                arrow = option->direction == Qt::LeftToRight ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight;
        } else {
            if (element == CE_ScrollBarAddLine)
                arrow = PE_IndicatorArrowDown;
            else
                arrow = PE_IndicatorArrowUp;
        }
        drawPrimitive(arrow, option, painter, widget);
        break; }
    case CE_ScrollBarAddPage:
    case CE_ScrollBarSubPage: {
            QBrush br;
            QBrush bg = painter->background();
            Qt::BGMode bg_mode = painter->backgroundMode();
            painter->setPen(Qt::NoPen);
            painter->setBackgroundMode(Qt::OpaqueMode);

            if (option->state & State_Sunken) {
                br = QBrush(option->palette.shadow().color(), Qt::Dense4Pattern);
                painter->setBackground(option->palette.dark().color());
                painter->setBrush(br);
            } else {
                QPixmap pm = option->palette.brush(QPalette::Light).texture();
                if (option->state & State_Enabled)
                    br = !pm.isNull() ? QBrush(pm) : QBrush(option->palette.button().color(), Qt::Dense4Pattern);
                else
                    br = !pm.isNull() ? QBrush(pm) : QBrush(option->palette.light().color(), Qt::Dense4Pattern);
                painter->setBackground(option->palette.base().color());
                painter->setBrush(br);
            }
            painter->drawRect(option->rect);
            painter->setBackground(bg);
            painter->setBackgroundMode(bg_mode);
            break; }
    case CE_ScrollBarSlider:
        if (!(option->state & State_Enabled)) {
            QStyleOptionButton buttonOpt;
            buttonOpt.QStyleOption::operator=(*option);
            buttonOpt.state = State_Enabled | State_Raised;
            drawPrimitive(PE_PanelButtonBevel, &buttonOpt, painter, widget);
            QPixmap pm = option->palette.brush(QPalette::Light).texture();
            QBrush br = !pm.isNull() ? QBrush(pm) : QBrush(option->palette.light().color(), Qt::Dense4Pattern);
            painter->setPen(Qt::NoPen);
            painter->setBrush(br);
            painter->setBackgroundMode(Qt::OpaqueMode);
            painter->drawRect(option->rect.adjusted(2, 2, -2, -2));
        } else {
            QStyleOptionButton buttonOpt;
            buttonOpt.QStyleOption::operator=(*option);
            buttonOpt.state = State_Enabled | State_Raised;
            drawPrimitive(PE_PanelButtonBevel, &buttonOpt, painter, widget);
        }
        break;
#endif // QT_NO_SCROLLBAR
    case CE_HeaderSection: {
        QBrush fill;
        if (option->state & State_On)
            fill = QBrush(option->palette.light().color(), Qt::Dense4Pattern);
        else
            fill = option->palette.brush(QPalette::Button);

        if (option->state & (State_Raised | State_Sunken)) {
            QWindowsCEStylePrivate::drawWinCEButton(painter, option->rect, option->palette,
                                                    option->state & State_Sunken, &fill);
        } else {
            painter->fillRect(option->rect, fill);
        }
        break; }

    case CE_DockWidgetTitle:
        QWindowsStyle::drawControl(element,option, painter, widget);
        break;

        case CE_PushButtonLabel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            painter->save();
            QFont f = painter->font();
            f.setBold(true);
            painter->setFont(f);
            QRect ir = btn->rect;
            uint tf = Qt::AlignVCenter | Qt::TextShowMnemonic;
            if (!styleHint(SH_UnderlineShortcut, btn, widget))
                tf |= Qt::TextHideMnemonic;

            if (btn->state & (State_On | State_Sunken))
                ir.translate(pixelMetric(PM_ButtonShiftHorizontal, option, widget),
                             pixelMetric(PM_ButtonShiftVertical, option, widget));
            if (!btn->icon.isNull()) {
                QIcon::Mode mode = btn->state & State_Enabled ? QIcon::Normal
                                                              : QIcon::Disabled;
                if (mode == QIcon::Normal && btn->state & State_HasFocus)
                    mode = QIcon::Active;
                QIcon::State state = QIcon::Off;
                if (btn->state & State_On)
                    state = QIcon::On;
                QPixmap pixmap = btn->icon.pixmap(btn->iconSize, mode, state);
                int pixw = pixmap.width();
                int pixh = pixmap.height();
                //Center the icon if there is no text

                QPoint point;
                if (btn->text.isEmpty()) {
                    point = QPoint(ir.x() + ir.width() / 2 - pixw / 2,
                                   ir.y() + ir.height() / 2 - pixh / 2);
                } else {
                    point = QPoint(ir.x() + 2, ir.y() + ir.height() / 2 - pixh / 2);
                }
                if (btn->direction == Qt::RightToLeft)
                    point.rx() += pixw;

                if ((btn->state & (State_On | State_Sunken)) && btn->direction == Qt::RightToLeft)
                    point.rx() -= pixelMetric(PM_ButtonShiftHorizontal, option, widget) * 2;

                painter->drawPixmap(visualPos(btn->direction, btn->rect, point), pixmap);

                if (btn->direction == Qt::RightToLeft)
                    ir.translate(-4, 0);
                else
                    ir.translate(pixw + 4, 0);
                ir.setWidth(ir.width() - (pixw + 4));
                // left-align text if there is
                if (!btn->text.isEmpty())
                    tf |= Qt::AlignLeft;
            } else {
                tf |= Qt::AlignHCenter;
            }
            drawItemText(painter, ir, tf, btn->palette, (btn->state & State_Enabled),
                         btn->text, QPalette::ButtonText);
            painter->restore();
        }
        break;
        default:
            QWindowsStyle::drawControl(element, option, painter, widget);
            break;
    }
}

void QWindowsCEStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                         QPainter *painter, const QWidget *widget) const {
    switch (control) {
        #ifndef QT_NO_SLIDER
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            int thickness  = pixelMetric(PM_SliderControlThickness, slider, widget);
            int len        = pixelMetric(PM_SliderLength, slider, widget);
            int ticks = slider->tickPosition;
            QRect groove = subControlRect(CC_Slider, slider, SC_SliderGroove, widget);
            QRect handle = subControlRect(CC_Slider, slider, SC_SliderHandle, widget);

            if ((slider->subControls & SC_SliderGroove) && groove.isValid()) {
                int mid = thickness / 2;
                if (ticks & QSlider::TicksAbove)
                    mid += len / 8;
                if (ticks & QSlider::TicksBelow)
                    mid -= len / 8;

                painter->setPen(slider->palette.shadow().color());
                if (slider->orientation == Qt::Horizontal) {
                    QWindowsCEStylePrivate::drawWinCEPanel(painter, groove.x(), groove.y() + mid - 2,
                                   groove.width(), 4, option->palette, true);
                    painter->drawLine(groove.x() + 1, groove.y() + mid - 1,
                                groove.x() + groove.width() - 3, groove.y() + mid - 1);
                } else {
                    QWindowsCEStylePrivate::drawWinCEPanel(painter, groove.x() + mid - 2, groove.y(),
                                  4, groove.height(), option->palette, true);
                    painter->drawLine(groove.x() + mid - 1, groove.y() + 1,
                                groove.x() + mid - 1, groove.y() + groove.height() - 3);
                }
            }
            if (slider->subControls & SC_SliderTickmarks) {
                QStyleOptionSlider tmpSlider = *slider;
                tmpSlider.subControls = SC_SliderTickmarks;
                QCommonStyle::drawComplexControl(control, &tmpSlider, painter, widget);
            }

            if (slider->subControls & SC_SliderHandle) {
                // 4444440
                // 4333310
                // 4322210
                // 4322210
                // 4322210
                // 4322210
                // *43210*
                // **440**
                // ***0***
                const QColor c0 = slider->palette.shadow().color();
                const QColor c1 = slider->palette.dark().color();
                // const QColor c2 = g.button();
                const QColor c3 = slider->palette.midlight().color();
                const QColor c4 = slider->palette.dark().color();
                QBrush handleBrush;

                if (slider->state & State_Enabled) {
                    handleBrush = slider->palette.color(QPalette::Button);
                } else {
                    handleBrush = QBrush(slider->palette.color(QPalette::Button),
                                         Qt::Dense4Pattern);
                }

                int x = handle.x(), y = handle.y(),
                   wi = handle.width(), he = handle.height();

                int x1 = x;
                int x2 = x + wi - 1;
                int y1 = y;
                int y2 = y + he - 1;

                Qt::Orientation orient = slider->orientation;
                bool tickAbove = slider->tickPosition == QSlider::TicksAbove;
                bool tickBelow = slider->tickPosition == QSlider::TicksBelow;

                if (slider->state & State_HasFocus) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*slider);
                    fropt.rect = subElementRect(SE_SliderFocusRect, slider, widget);
                    drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
                }
                if ((tickAbove && tickBelow) || (!tickAbove && !tickBelow)) {
                    Qt::BGMode oldMode = painter->backgroundMode();
                    painter->setBackgroundMode(Qt::OpaqueMode);
                    QWindowsCEStylePrivate::drawWinCEButton(painter, QRect(x, y, wi, he), slider->palette, false,
                                                            &handleBrush);
                    painter->setBackgroundMode(oldMode);
                    QBrush fill = QBrush(option->palette.light().color(), Qt::Dense4Pattern);
                    if (slider->state & State_Sunken)
                      painter->fillRect(QRectF(x1 + 2, y1 + 2, x2 - x1 - 3, y2 - y1 - 3),fill);
                    return;
                }
                QSliderDirection dir;
                if (orient == Qt::Horizontal)
                    if (tickAbove)
                        dir = SlUp;
                    else
                        dir = SlDown;
                else
                    if (tickAbove)
                        dir = SlLeft;
                    else
                        dir = SlRight;
                QPolygon a;
                int d = 0;
                switch (dir) {
                case SlUp:
                    x2++;
                    y1 = y1 + wi / 2;
                    d =  (wi + 1) / 2 - 1;
                    a.setPoints(5, x1, y1, x1, y2, x2, y2, x2, y1, x1 + d, y1 - d);
                    break;
                case SlDown:
                    x2++;
                    y2 = y2 - wi / 2;
                    d =  (wi + 1) / 2 - 1;
                    a.setPoints(5, x1, y1, x1, y2, x1 + d, y2+d, x2, y2, x2, y1);
                    break;
                case SlLeft:
                    d =  (he + 1) / 2 - 1;
                    x1 = x1 + he / 2;
                    a.setPoints(5, x1, y1, x1 - d, y1 + d, x1, y2, x2, y2, x2, y1);
                    y1--;
                    break;
                case SlRight:
                    d =  (he + 1) / 2 - 1;
                    x2 = x2 - he / 2;
                    a.setPoints(5, x1, y1, x1, y2, x2, y2, x2 + d, y1 + d, x2, y1);
                    y1--;
                    break;
                }
                QBrush oldBrush = painter->brush();
                painter->setPen(Qt::NoPen);
                painter->setBrush(handleBrush);
                Qt::BGMode oldMode = painter->backgroundMode();
                painter->setBackgroundMode(Qt::OpaqueMode);
                painter->drawRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
                painter->drawPolygon(a);
                QBrush fill = QBrush(option->palette.light().color(), Qt::Dense4Pattern);
                if (slider->state & State_Sunken)
                  painter->fillRect(QRectF(x1, y1, x2 - x1 + 1, y2 - y1 + 1),fill);
                painter->setBrush(oldBrush);
                painter->setBackgroundMode(oldMode);

                if (dir != SlUp) {
                    painter->setPen(c4);
                    painter->drawLine(x1, y1, x2, y1);
                    painter->setPen(c3);
                    painter->drawLine(x1, y1 + 1, x2, y1 + 1);
                }
                if (dir != SlLeft) {
                    painter->setPen(c3);
                    painter->drawLine(x1 + 1, y1 + 1, x1 + 1, y2);
                    painter->setPen(c4);
                    painter->drawLine(x1, y1, x1, y2);
                }
                if (dir != SlRight) {
                    painter->setPen(c0);
                    painter->drawLine(x2, y1, x2, y2);
                    painter->setPen(c1);
                    painter->drawLine(x2 - 1, y1 + 1, x2 - 1, y2 - 1);
                }
                if (dir != SlDown) {
                    painter->setPen(c0);
                    painter->drawLine(x1, y2, x2, y2);
                    painter->setPen(c1);
                    painter->drawLine(x1+1, y2 - 1, x2 - 1, y2 - 1);
                }

               switch (dir) {
                case SlUp:
                    if (slider->state & State_Sunken)
                      painter->fillRect(QRectF(x1 + 3, y1 - d + 2, x2 - x1 - 4,y1), fill);
                    painter->setPen(c4);
                    painter->drawLine(x1, y1, x1 + d, y1 - d);
                    painter->setPen(c0);
                    d = wi - d - 1;
                    painter->drawLine(x2, y1, x2 - d, y1 - d);
                    d--;
                    painter->setPen(c3);
                    painter->drawLine(x1 + 1, y1, x1 + 1 + d-1, y1 - d + 1);
                    painter->setPen(c1);
                    painter->drawLine(x2 - 1, y1, x2-1 - d, y1 - d);
                    break;
                case SlDown:
                    if (slider->state & State_Sunken)
                      painter->fillRect(QRectF(x1 + 3, y2 - d, x2 - x1 - 4,y2 - 8), fill);
                    painter->setPen(c4);
                    painter->drawLine(x1, y2, x1 + d, y2 + d);
                    painter->setPen(c0);
                    d = wi - d - 1;
                    painter->drawLine(x2, y2, x2 - d, y2 + d);
                    d--;
                    painter->setPen(c3);
                    painter->drawLine(x1 + 1, y2, x1 + 1 + d - 1, y2 + d - 1);
                    painter->setPen(c1);
                    painter->drawLine(x2 - 1, y2, x2 - 1 - d, y2 + d);
                    break;
                case SlLeft:
                    if (slider->state & State_Sunken)
                      painter->fillRect(QRectF(x1 - d + 2, y1 + 2, x1,y2 - y1 - 3), fill);
                    painter->setPen(c4);
                    painter->drawLine(x1, y1, x1 - d, y1 + d);
                    painter->setPen(c0);
                    d = he - d - 1;
                    painter->drawLine(x1, y2, x1 - d, y2 - d);
                    d--;
                    painter->setPen(c3);
                    painter->drawLine(x1, y1 + 1, x1 - d + 1, y1 + 1 + d - 1);
                    painter->setPen(c1);
                    painter->drawLine(x1, y2 - 1, x1 - d, y2 - 1 - d);
                    break;
                case SlRight:
                    if (slider->state & State_Sunken)
                      painter->fillRect(QRectF(x2 - d - 4, y1 + 2, x2 - 4, y2 - y1 - 3), fill);
                    painter->setPen(c4);
                    painter->drawLine(x2, y1, x2 + d, y1 + d);
                    painter->setPen(c0);
                    d = he - d - 1;
                    painter->drawLine(x2, y2, x2 + d, y2 - d);
                    d--;
                    painter->setPen(c3);
                    painter->drawLine(x2, y1 + 1, x2 + d - 1, y1 + 1 + d - 1);
                    painter->setPen(c1);
                    painter->drawLine(x2, y2 - 1, x2 + d, y2 - 1 - d);
                    break;
                }
            }
        }
        break;
#endif // QT_NO_SLIDER
    case CC_ToolButton:
        if (const QStyleOptionToolButton *toolbutton
                = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            QRect button, menuarea;

#ifndef QT_NO_TOOLBAR
            bool flat = !(widget ? qobject_cast<QToolBar*>(widget->parentWidget()) : 0);
#else
            bool flat = true;
#endif

            button = subControlRect(control, toolbutton, SC_ToolButton, widget);
            menuarea = subControlRect(control, toolbutton, SC_ToolButtonMenu, widget);

            if (flat && (toolbutton->subControls & SC_ToolButtonMenu)) {
                menuarea.setLeft(menuarea.left() - 4);
                button.setRight(button.right() - 4);
            }

            State bflags = toolbutton->state;

            if (bflags & State_AutoRaise)
                if (!(bflags & State_MouseOver)) {
                    bflags &= ~State_Raised;
                }
            State mflags = bflags;

            if (toolbutton->activeSubControls & SC_ToolButton)
                bflags |= State_Sunken;
            if (toolbutton->activeSubControls & SC_ToolButtonMenu)
                mflags |= State_Sunken;

            QStyleOption tool(0);
            tool.palette = toolbutton->palette;
            if (toolbutton->subControls & SC_ToolButton) {
                    tool.rect = button;
                    tool.state = bflags;
                    drawPrimitive(PE_PanelButtonTool, &tool, painter, widget);
                }

            if (toolbutton->subControls & SC_ToolButtonMenu) {
                tool.rect = menuarea;
                tool.state = mflags;
                tool.state = bflags;
                drawPrimitive(PE_IndicatorButtonDropDown, &tool, painter, widget);

                if (!flat) {

                    //connect buttons
                    painter->save();
                    painter->setPen(tool.palette.button().color());
                    painter->drawLine(tool.rect.x() - 2, tool.rect.y(), tool.rect.x() - 2, tool.rect.y() + tool.rect.height());
                    painter->drawLine(tool.rect.x() - 1, tool.rect.y(), tool.rect.x() - 1, tool.rect.y() + tool.rect.height());
                    painter->drawLine(tool.rect.x(), tool.rect.y(), tool.rect.x(), tool.rect.y() + tool.rect.height());
                    painter->drawLine(tool.rect.x() + 1, tool.rect.y(), tool.rect.x() + 1, tool.rect.y() + tool.rect.height());

                    if (tool.state & State_Sunken)
                    {
                        painter->setPen(tool.palette.midlight().color());
                        painter->drawLine(tool.rect.x() - 2, tool.rect.y() + tool.rect.height() - 2,
                            tool.rect.x() + 1, tool.rect.y() + tool.rect.height() -2 );
                        painter->setPen(tool.palette.shadow().color());
                        painter->drawLine(tool.rect.x() - 2, tool.rect.y() + 1,tool.rect.x() + 1, tool.rect.y() + 1);
                        painter->drawLine(tool.rect.x() - 2, tool.rect.y(), tool.rect.x() + 1, tool.rect.y());
                        painter->setPen(tool.palette.light().color());
                        painter->drawLine(tool.rect.x() - 2, tool.rect.y() + tool.rect.height() - 1,
                            tool.rect.x() + 1, tool.rect.y() + tool.rect.height() - 1);
                    }
                    else
                    {
                        painter->setPen(tool.palette.dark().color());
                        painter->drawLine(tool.rect.x() - 2, tool.rect.y(),tool.rect.x() + 1, tool.rect.y());
                        painter->drawLine(tool.rect.x() - 2, tool.rect.y()+tool.rect.height() - 2,tool.rect.x() + 1,
                            tool.rect.y() + tool.rect.height() - 2);
                        painter->setPen(tool.palette.midlight().color());
                        painter->drawLine(tool.rect.x() - 2, tool.rect.y() + 1,tool.rect.x() + 1, tool.rect.y() + 1);
                        painter->setPen(tool.palette.shadow().color());
                        painter->drawLine(tool.rect.x() - 2, tool.rect.y() + tool.rect.height() - 1,
                            tool.rect.x() + 1, tool.rect.y() + tool.rect.height() - 1);
                    }
                    painter->restore();
                }


                if (!flat) {
                    tool.rect.adjust(-3,0,-3,0);
                    painter->save();
                    painter->setPen(tool.palette.button().color());
                    if (tool.state & State_Sunken)
                        painter->drawLine(tool.rect.x() + 2, tool.rect.y() + 10,
                        tool.rect.x() + tool.rect.width(), tool.rect.y() + 10);
                    else
                        painter->drawLine(tool.rect.x() + 1, tool.rect.y() + 9, tool.rect.x() +
                        tool.rect.width() - 1, tool.rect.y() + 9);
                    painter->restore();
                } else {
                    tool.rect.adjust(-1,0,-1,0);
                }

                drawPrimitive(PE_IndicatorArrowDown, &tool, painter, widget);
            }

            if (toolbutton->state & State_HasFocus) {
                QStyleOptionFocusRect fr;
                fr.QStyleOption::operator=(*toolbutton);
                fr.rect.adjust(3, 3, -3, -3);
                if (toolbutton->features & QStyleOptionToolButton::Menu)
                    fr.rect.adjust(0, 0, -pixelMetric(QStyle::PM_MenuButtonIndicator,
                                                         toolbutton, widget), 0);
                drawPrimitive(PE_FrameFocusRect, &fr, painter, widget);
            }
            QStyleOptionToolButton label = *toolbutton;
            int fw = pixelMetric(PM_DefaultFrameWidth, option, widget);
            label.rect = button.adjusted(fw, fw, -fw, -fw);
            drawControl(CE_ToolButtonLabel, &label, painter, widget);
        }
        break;

#ifndef QT_NO_GROUPBOX
    case CC_GroupBox:
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            // Draw frame
            painter->save();
            QFont f = painter->font();
            f.setBold(true);
            painter->setFont(f);
            QStyleOptionGroupBox groupBoxFont = *groupBox;
            groupBoxFont.fontMetrics = QFontMetrics(f);
            QRect textRect = subControlRect(CC_GroupBox, &groupBoxFont, SC_GroupBoxLabel, widget);
            QRect checkBoxRect = subControlRect(CC_GroupBox, option, SC_GroupBoxCheckBox, widget);
            if (groupBox->subControls & QStyle::SC_GroupBoxFrame) {
                QStyleOptionFrameV2 frame;
                frame.QStyleOption::operator=(*groupBox);
                frame.features = groupBox->features;
                frame.lineWidth = groupBox->lineWidth;
                frame.midLineWidth = groupBox->midLineWidth;
                frame.rect = subControlRect(CC_GroupBox, option, SC_GroupBoxFrame, widget);
                painter->save();

                QRegion region(groupBox->rect);
                if (!groupBox->text.isEmpty()) {
                    bool ltr = groupBox->direction == Qt::LeftToRight;
                    QRect finalRect = checkBoxRect.united(textRect);
                    if (groupBox->subControls & QStyle::SC_GroupBoxCheckBox)
                        finalRect.adjust(ltr ? -4 : 0, 0, ltr ? 0 : 4, 0);
                    region -= finalRect;
                }
                painter->setClipRegion(region);
                drawPrimitive(PE_FrameGroupBox, &frame, painter, widget);
                painter->restore();
            }

            // Draw title
            if ((groupBox->subControls & QStyle::SC_GroupBoxLabel) && !groupBox->text.isEmpty()) {
                QColor textColor = groupBox->textColor;
                if (textColor.isValid())
                    painter->setPen(textColor);
                int alignment = int(groupBox->textAlignment);
                if (!styleHint(QStyle::SH_UnderlineShortcut, option, widget))
                    alignment |= Qt::TextHideMnemonic;

                drawItemText(painter, textRect,  Qt::TextShowMnemonic | Qt::AlignHCenter | alignment,
                             groupBox->palette, groupBox->state & State_Enabled, groupBox->text,
                             textColor.isValid() ? QPalette::NoRole : QPalette::WindowText);

                if (groupBox->state & State_HasFocus) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*groupBox);
                    fropt.rect = textRect;
                    drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
                }
            }
            // Draw checkbox
            if (groupBox->subControls & SC_GroupBoxCheckBox) {
                QStyleOptionButton box;
                box.QStyleOption::operator=(*groupBox);
                box.rect = checkBoxRect;
                drawPrimitive(PE_IndicatorCheckBox, &box, painter, widget);
            }
            painter->restore();
        }
        break;
#endif //QT_NO_GROUPBOX
#ifndef QT_NO_COMBOBOX
    case CC_ComboBox:
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            QBrush editBrush = cmb->palette.brush(QPalette::Base);
            if ((cmb->subControls & SC_ComboBoxFrame) && cmb->frame)
                QWindowsCEStylePrivate::drawWinCEPanel(painter, option->rect, option->palette, true, &editBrush);
            else
                painter->fillRect(option->rect, editBrush);

            if (cmb->subControls & SC_ComboBoxArrow) {
                State flags = State_None;

                QRect ar = subControlRect(CC_ComboBox, cmb, SC_ComboBoxArrow, widget);
                if (cmb->activeSubControls == SC_ComboBoxArrow) {
                    painter->setPen(cmb->palette.dark().color());
                    painter->setBrush(cmb->palette.brush(QPalette::Button));
                    painter->drawRect(ar.adjusted(0, 0, -1, -1));
                    QWindowsCEStylePrivate::drawWinCEButton(painter, ar.adjusted(0, 0, -1, -1), option->palette, true,
                    &cmb->palette.brush(QPalette::Button));
                } else {
                    // Make qDrawWinButton use the right colors for drawing the shade of the button

                    QWindowsCEStylePrivate::drawWinCEButton(painter, ar, option->palette, false,
                                   &cmb->palette.brush(QPalette::Button));
                }

                ar.adjust(2, 2, -2, -2);
                if (option->state & State_Enabled)
                    flags |= State_Enabled;

                if (cmb->activeSubControls == SC_ComboBoxArrow)
                    flags |= State_Sunken;
                QStyleOption arrowOpt(0);
                arrowOpt.rect = ar;
                arrowOpt.palette = cmb->palette;
                arrowOpt.state = flags;
                drawPrimitive(PE_IndicatorArrowDown, &arrowOpt, painter, widget);
            }
            if (cmb->subControls & SC_ComboBoxEditField) {
                QRect re = subControlRect(CC_ComboBox, cmb, SC_ComboBoxEditField, widget);
                if (cmb->state & State_HasFocus && !cmb->editable)
                    painter->fillRect(re.x(), re.y(), re.width(), re.height(),
                 cmb->palette.brush(QPalette::Highlight));
                 if (cmb->state & State_HasFocus) {
                    painter->setPen(cmb->palette.highlightedText().color());
                    painter->setBackground(cmb->palette.highlight());
                 } else {
                    painter->setPen(cmb->palette.text().color());
                    painter->setBackground(cmb->palette.background());
                 }
                 if (cmb->state & State_HasFocus && !cmb->editable) {
                    QStyleOptionFocusRect focus;
                    focus.QStyleOption::operator=(*cmb);
                    focus.rect = subElementRect(SE_ComboBoxFocusRect, cmb, widget);
                    focus.state |= State_FocusAtBorder;
                    focus.backgroundColor = cmb->palette.highlight().color();
                    drawPrimitive(PE_FrameFocusRect, &focus, painter, widget);
               }
            }
        }
        break;
#endif // QT_NO_COMBOBOX
#ifndef QT_NO_SPINBOX
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            QStyleOptionSpinBox copy = *sb;
            PrimitiveElement pe;

            if (sb->frame && (sb->subControls & SC_SpinBoxFrame)) {
                QRect r = subControlRect(CC_SpinBox, sb, SC_SpinBoxFrame, widget);
                QWindowsCEStylePrivate::drawWinCEPanel(painter, r, option->palette, true);
            }
            QPalette shadePal(option->palette);
            shadePal.setColor(QPalette::Button, option->palette.light().color());
            shadePal.setColor(QPalette::Light, option->palette.button().color());

            bool reverse = QApplication::layoutDirection() == Qt::RightToLeft;

            if (sb->subControls & SC_SpinBoxUp) {
                copy.subControls = SC_SpinBoxUp;
                QPalette pal2 = sb->palette;
                if (!(sb->stepEnabled & QAbstractSpinBox::StepUpEnabled)) {
                    pal2.setCurrentColorGroup(QPalette::Disabled);
                    copy.state &= ~State_Enabled;
                }
                copy.palette = pal2;
                if (sb->activeSubControls == SC_SpinBoxUp && (sb->state & State_Sunken)) {
                    copy.state |= State_On;
                    copy.state |= State_Sunken;
                } else {
                    copy.state |= State_Raised;
                    copy.state &= ~State_Sunken;
                }
                if (reverse)
                    pe = (sb->buttonSymbols == QAbstractSpinBox::PlusMinus ? PE_IndicatorSpinMinus
                    : PE_IndicatorSpinDown);
                else
                    pe = (sb->buttonSymbols == QAbstractSpinBox::PlusMinus ? PE_IndicatorSpinPlus
                    : PE_IndicatorSpinUp);
                copy.rect = subControlRect(CC_SpinBox, sb, SC_SpinBoxUp, widget);
                QWindowsCEStylePrivate::drawWinCEButton(painter, copy.rect, option->palette, copy.state & (State_Sunken | State_On),
                                &copy.palette.brush(QPalette::Button));
                copy.rect.adjust(3, 0, -4, 0);
                drawPrimitive(pe, &copy, painter, widget);
            }
            if (sb->subControls & SC_SpinBoxDown) {
                copy.subControls = SC_SpinBoxDown;
                copy.state = sb->state;
                QPalette pal2 = sb->palette;
                if (!(sb->stepEnabled & QAbstractSpinBox::StepDownEnabled)) {
                    pal2.setCurrentColorGroup(QPalette::Disabled);
                    copy.state &= ~State_Enabled;
                }
                copy.palette = pal2;

                if (sb->activeSubControls == SC_SpinBoxDown && (sb->state & State_Sunken)) {
                    copy.state |= State_On;
                    copy.state |= State_Sunken;
                } else {
                    copy.state |= State_Raised;
                    copy.state &= ~State_Sunken;
                }
                if (reverse)
                    pe = (sb->buttonSymbols == QAbstractSpinBox::PlusMinus ? PE_IndicatorSpinPlus
                    : PE_IndicatorSpinUp);
                else
                    pe = (sb->buttonSymbols == QAbstractSpinBox::PlusMinus ? PE_IndicatorSpinMinus
                    : PE_IndicatorSpinDown);
                copy.rect = subControlRect(CC_SpinBox, sb, SC_SpinBoxDown, widget);
                QWindowsCEStylePrivate::drawWinCEButton(painter, copy.rect, shadePal, copy.state & (State_Sunken | State_On),
                                &copy.palette.brush(QPalette::Button));

                copy.rect.adjust(3, 0, -4, 0);
                if (pe == PE_IndicatorArrowUp || pe == PE_IndicatorArrowDown) {
                    copy.rect = copy.rect.adjusted(1, 1, -1, -1);
                    drawPrimitive(pe, &copy, painter, widget);
                }
                else {
                    drawPrimitive(pe, &copy, painter, widget);
                }
                if (sb->frame && (sb->subControls & SC_SpinBoxFrame)) {
                    QRect r = subControlRect(CC_SpinBox, sb, SC_SpinBoxEditField, widget);
                    painter->save();
                    painter->setPen(option->palette.light().color());
                    painter->drawLine(r.x() + 1 + r.width(), r.y() - 2, r.x() + 1 + r.width(), r.y() + r.height() + 1);
                    painter->setPen(option->palette.midlight().color());
                    painter->drawLine(r.x() + r.width(), r.y() - 1, r.x() + r.width(), r.y() + r.height());
                    painter->restore();
                }
            }
        }
        break;
#endif // QT_NO_SPINBOX

    default:
        QWindowsStyle::drawComplexControl(control, option, painter, widget);
        break;
    }
}

void QWindowsCEStyle::drawItemText(QPainter *painter, const QRect &rect, int alignment, const QPalette &pal,
                          bool enabled, const QString& text, QPalette::ColorRole textRole) const {
    if (text.isEmpty())
        return;
    QPen savedPen;
    if (textRole != QPalette::NoRole) {
        savedPen = painter->pen();
        painter->setPen(pal.color(textRole));
    }
    if (!enabled) {
            QPen pen = painter->pen();
            painter->setPen(pal.light().color());
            //painter->drawText(rect.adjusted(1, 1, 1, 1), alignment, text);
            painter->setPen(pen);
        }
    painter->drawText(rect, alignment, text);
    if (textRole != QPalette::NoRole)
        painter->setPen(savedPen);
}


QSize QWindowsCEStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                        const QSize &size, const QWidget *widget) const {
    QSize newSize = QWindowsStyle::sizeFromContents(type, option, size, widget);
    switch (type) {
    case CT_PushButton:
       if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            newSize = QWindowsStyle::sizeFromContents(type, option, size, widget);
            int w = newSize.width(),
                h = newSize.height();
            int defwidth = 0;
            if (btn->features & QStyleOptionButton::AutoDefaultButton)
                defwidth = 2 * pixelMetric(PM_ButtonDefaultIndicator, btn, widget);
            if (w < 75 + defwidth && btn->icon.isNull())
                w = 75 + defwidth;
            if (h < 23 + defwidth)
                h = 23 + defwidth;
            newSize = QSize(w+14, h);
        }
        break;

    case CT_RadioButton:
    case CT_CheckBox:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            bool isRadio = (type == CT_RadioButton);
            QRect irect = visualRect(btn->direction, btn->rect,
                                     subElementRect(isRadio ? SE_RadioButtonIndicator
                                                            : SE_CheckBoxIndicator, btn, widget));
            int h = pixelMetric(isRadio ? PM_ExclusiveIndicatorHeight
                                        : PM_IndicatorHeight, btn, widget);
            int margins = (!btn->icon.isNull() && btn->text.isEmpty()) ? 0 : 10;
            newSize += QSize(irect.right() + margins, 4);
            newSize.setHeight(qMax(newSize.height(), h));
        }
        break;
    case CT_ComboBox:
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            int fw = cmb->frame ? pixelMetric(PM_ComboBoxFrameWidth, option, widget) * 2 : 0;
            newSize = QSize(newSize.width() + fw -1, qMax(24, newSize.height() + fw-1));
        }
        break;
#ifndef QT_NO_SPINBOX
    case CT_SpinBox:
        if (const QStyleOptionSpinBox *spnb = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            int fw = spnb->frame ? pixelMetric(PM_SpinBoxFrameWidth, option, widget) * 2 : 0;
            newSize = QSize(newSize.width() + fw - 5, newSize.height() + fw - 6);
        }
        break;
#endif
    case CT_LineEdit:
        newSize += QSize(0,1);
        break;
    case CT_MenuBarItem:
        newSize += QSize(5, 1);
        break;
    case CT_MenuItem:
        newSize += QSize(0, -2);
        break;
    case CT_MenuBar:
        newSize += QSize(0, -1);
        break;
    case CT_ToolButton:
        if (const QStyleOptionToolButton *b = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
          if (b->toolButtonStyle != Qt::ToolButtonIconOnly)
            newSize = QSize(newSize.width() + 1, newSize.height() - 1);
          else
            newSize = QSize(newSize.width() + 1, newSize.height());
        }
        break;

    default:
        break;
    }
    return newSize;
}

QRect QWindowsCEStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const {
    QRect rect = QWindowsStyle::subElementRect(element, option, widget);
    switch (element) {
#ifndef QT_NO_COMBOBOX
    case SE_ComboBoxFocusRect:
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            int margin = cb->frame ? 3 : 0;
            rect.setRect(margin, margin, option->rect.width() - 2*margin - 20, option->rect.height() - 2*margin);
            rect = visualRect(option->direction, option->rect, rect);
        }
        break;
#endif // QT_NO_COMBOBOX
    default:
        break;
    }
    return rect;
}

QRect QWindowsCEStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                      SubControl subControl, const QWidget *widget) const {
    QRect rect = QWindowsStyle::subControlRect(control, option, subControl, widget);
    switch (control) {
#ifndef QT_NO_SLIDER
        case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            int tickOffset = pixelMetric(PM_SliderTickmarkOffset, slider, widget);
            int thickness = pixelMetric(PM_SliderControlThickness, slider, widget);

            switch (subControl) {
            case SC_SliderHandle: {
                int sliderPos = 0;
                int len = pixelMetric(PM_SliderLength, slider, widget);
                bool horizontal = slider->orientation == Qt::Horizontal;
                sliderPos = sliderPositionFromValue(slider->minimum, slider->maximum,
                                                    slider->sliderPosition,
                                                    (horizontal ? slider->rect.width()
                                                                : slider->rect.height()) - len,
                                                    slider->upsideDown);
                if (horizontal)
                    rect.setRect(slider->rect.x() + sliderPos, slider->rect.y() + tickOffset, len, thickness);
                else
                    rect.setRect(slider->rect.x() + tickOffset, slider->rect.y() + sliderPos, thickness, len);
                break; }
            default:
                break;
            }
            rect = visualRect(slider->direction, slider->rect, rect);
        }
        break;
#endif //QT_NO_SLIDER
#ifndef QT_NO_COMBOBOX
    case CC_ComboBox:
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
        int x = cb->rect.x(),
            y = cb->rect.y(),
            wi = cb->rect.width(),
            he = cb->rect.height();
        int xpos = x;
        int margin = cb->frame ? 3 : 0;
        int bmarg = cb->frame ? 2 : 0;
        xpos += wi - (he - 2*bmarg) - bmarg;
        switch (subControl) {
          case SC_ComboBoxArrow:
            rect.setRect(xpos, y + bmarg, he - 2*bmarg, he - 2*bmarg);
            break;
           case SC_ComboBoxEditField:
                rect.setRect(x + margin, y + margin, wi - 2 * margin - (he - 2*bmarg), he - 2 * margin);
                break;
            case SC_ComboBoxListBoxPopup:
                rect = cb->rect;
                break;
            case SC_ComboBoxFrame:
                rect = cb->rect;
                break;
        default:
        break;
        }
        rect = visualRect(cb->direction, cb->rect, rect);
    }
#endif //QT_NO_COMBOBOX
#ifndef QT_NO_SPINBOX
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinbox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            QSize bs;
            int fw = spinbox->frame ? pixelMetric(PM_SpinBoxFrameWidth, spinbox, widget) : 0;
            bs.setWidth(qMax(18, (spinbox->rect.height() / 2 - fw + 1)));
            // 1.6 -approximate golden mean
            bs.setHeight(qMax(18, qMin((bs.height() * 8 / 5), (spinbox->rect.width() / 4))));
            bs = bs.expandedTo(QApplication::globalStrut());
            int y = fw;
            int x, lx, rx;
            x = spinbox->rect.width() - y - bs.width() * 2;
            lx = fw;
            rx = x - fw;
          switch (subControl) {
          case SC_SpinBoxUp:
                rect = QRect(x + bs.width(), y, bs.width(), bs.height());
                break;
          case SC_SpinBoxDown:
                rect = QRect(x, y , bs.width(), bs.height());
                break;
          case SC_SpinBoxEditField:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons) {
                    rect = QRect(lx, fw, spinbox->rect.width() - 2*fw - 2, spinbox->rect.height() - 2*fw);
                } else {
                    rect = QRect(lx, fw, rx-2, spinbox->rect.height() - 2*fw);
                }
                break;
          case SC_SpinBoxFrame:
                rect = spinbox->rect;
          default:
                break;
          }
          rect = visualRect(spinbox->direction, spinbox->rect, rect);
        }
        break;
#endif // Qt_NO_SPINBOX
#ifndef QT_NO_GROUPBOX
    case CC_GroupBox: {
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            switch (subControl) {
            case SC_GroupBoxFrame:
                // FALL THROUGH
            case SC_GroupBoxContents: {
                int topMargin = 0;
                int topHeight = 0;
                int bottomMargin = 0;
                int noLabelMargin = 0;
                QRect frameRect = groupBox->rect;
                int verticalAlignment = styleHint(SH_GroupBox_TextLabelVerticalAlignment, groupBox, widget);
                if (groupBox->text.size()) {
                    topHeight = groupBox->fontMetrics.height();
                    if (verticalAlignment & Qt::AlignVCenter)
                        topMargin = topHeight / 2;
                    else if (verticalAlignment & Qt::AlignTop)
                        topMargin = -topHeight/2;
                }
                else {
                  topHeight = groupBox->fontMetrics.height();
                  noLabelMargin = topHeight / 2;
                  if (verticalAlignment & Qt::AlignVCenter) {
                        topMargin = topHeight / 4 - 4;
                        bottomMargin = topHeight / 4 - 4;
                  }
                  else if (verticalAlignment & Qt::AlignTop) {
                        topMargin = topHeight/2 - 4;
                        bottomMargin = topHeight/2 - 4;
                  }
                }

                if (subControl == SC_GroupBoxFrame) {
                    frameRect.setTop(topMargin);
                    frameRect.setBottom(frameRect.height() + bottomMargin);
                    rect = frameRect;
                    break;
                }

                int frameWidth = 0;
                if ((groupBox->features & QStyleOptionFrameV2::Flat) == 0)
                    frameWidth = pixelMetric(PM_DefaultFrameWidth, groupBox, widget);
                rect = frameRect.adjusted(frameWidth, frameWidth + topHeight, -frameWidth, -frameWidth - noLabelMargin);
                break;
            }
            case SC_GroupBoxCheckBox:
                // FALL THROUGH
            case SC_GroupBoxLabel: {
                QFontMetrics fontMetrics = groupBox->fontMetrics;
                int h = fontMetrics.height();
                int tw = fontMetrics.size(Qt::TextShowMnemonic, groupBox->text + QLatin1Char(' ')).width();
                int marg = (groupBox->features & QStyleOptionFrameV2::Flat) ? 0 : 8;
                rect = groupBox->rect.adjusted(marg, 0, -marg, 0);
                rect.setHeight(h);

                int indicatorWidth = pixelMetric(PM_IndicatorWidth, option, widget);
                int indicatorSpace = pixelMetric(PM_CheckBoxLabelSpacing, option, widget) - 1;
                bool hasCheckBox = groupBox->subControls & QStyle::SC_GroupBoxCheckBox;
                int checkBoxSize = hasCheckBox ? (indicatorWidth + indicatorSpace) : 0;

                // Adjusted rect for label + indicatorWidth + indicatorSpace
                QRect totalRect = alignedRect(groupBox->direction, groupBox->textAlignment,
                                              QSize(tw + checkBoxSize, h), rect);

                // Adjust totalRect if checkbox is set
                if (hasCheckBox) {
                    bool ltr = groupBox->direction == Qt::LeftToRight;
                    int left = 0;
                    // Adjust for check box
                    if (subControl == SC_GroupBoxCheckBox) {
                        int indicatorHeight = pixelMetric(PM_IndicatorHeight, option, widget);
                        left = ltr ? totalRect.left() : (totalRect.right() - indicatorWidth);
                        int top = totalRect.top() + (fontMetrics.height() - indicatorHeight) / 2;
                        totalRect.setRect(left, top, indicatorWidth, indicatorHeight);
                    // Adjust for label
                    } else {
                        left = ltr ? (totalRect.left() + checkBoxSize - 2) : totalRect.left();
                        totalRect.setRect(left, totalRect.top(),
                                          totalRect.width() - checkBoxSize, totalRect.height());
                    }
                }
                rect = totalRect;
                break;
            }
            default:
                break;
            }
        }
        break;
    }
#endif // QT_NO_GROUPBOX
    default:
        break;
    }
    return rect;
}

QStyle::SubControl QWindowsCEStyle::hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                                          const QPoint &pos, const QWidget *widget) const {
    /*switch (control) {
    default:
        break;
    }*/
    return QWindowsStyle::hitTestComplexControl(control, option, pos, widget);
}


QPalette QWindowsCEStyle::standardPalette() const {
    QPalette palette (Qt::black,QColor(198, 195, 198), QColor(222, 223, 222 ),
                      QColor(132, 130, 132), QColor(198, 195, 198) ,  Qt::black,  Qt::white, Qt::white, QColor(198, 195, 198));
    palette.setColor(QPalette::Window, QColor(198, 195, 198));
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::Button, QColor(198, 195, 198));
    palette.setColor(QPalette::Highlight, QColor(0, 0, 132));
    palette.setColor(QPalette::Light, Qt::white);
    palette.setColor(QPalette::Midlight, QColor(222, 223, 222 ));
    palette.setColor(QPalette::Dark, QColor(132, 130, 132));
    palette.setColor(QPalette::Mid, QColor(132, 130, 132));
    palette.setColor(QPalette::Shadow, QColor(0, 0, 0));
    palette.setColor(QPalette::BrightText, QColor(33, 162, 33)); //color for ItemView checked indicator (arrow)
    palette.setColor(QPalette::Link, QColor(24,81,132)); // color for the box around the ItemView indicator

    return palette;
}

void QWindowsCEStyle::polish(QApplication *app) {
    QWindowsStyle::polish(app);
}

void QWindowsCEStyle::polish(QWidget *widget) {
    QWindowsStyle::polish(widget);
}

void QWindowsCEStyle::polish(QPalette &palette) {
    QWindowsStyle::polish(palette);
}

int QWindowsCEStyle::pixelMetric(PixelMetric pm, const QStyleOption *opt, const QWidget *widget) const {
    int ret;

    switch (pm) {
    case PM_DefaultFrameWidth:
        ret = 1;
        break;

    case PM_MenuBarHMargin:
        ret = 2;
        break;
    case PM_MenuBarVMargin:
        ret = 2;
        break;
    /*case PM_MenuBarItemSpacing:
        ret = 2;
        break;*/

    case PM_MenuButtonIndicator:
        ret = 10;
        break;

    case PM_SpinBoxFrameWidth:
        ret = 2;
        break;
    case PM_ButtonDefaultIndicator:
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        ret = 1;
        break;
#ifndef QT_NO_TABBAR
    case PM_TabBarTabShiftHorizontal:
        ret = 0;
        break;
    case PM_TabBarTabShiftVertical:
        ret = 6;
        break;
#endif
    case PM_MaximumDragDistance:
        ret = 60;
        break;

      case PM_IndicatorWidth:
        ret = windowsCEIndicatorSize;
        break;

    case PM_IndicatorHeight:
        ret = windowsCEIndicatorSize;
        break;

    case PM_ExclusiveIndicatorWidth:
        ret = windowsCEExclusiveIndicatorSize;
        break;

    case PM_ExclusiveIndicatorHeight:
        ret = windowsCEExclusiveIndicatorSize;;
        break;

#ifndef QT_NO_SLIDER
    case PM_SliderLength:
        ret = 12;
        break;
    case PM_SliderThickness:
        ret = windowsCESliderThickness;
        break;

     case PM_TabBarScrollButtonWidth:
        ret = 18;
        break;

        // Returns the number of pixels to use for the business part of the
        // slider (i.e., the non-tickmark portion). The remaining space is shared
        // equally between the tickmark regions.
    case PM_SliderControlThickness:
        if (const QStyleOptionSlider *sl = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            int space = (sl->orientation == Qt::Horizontal) ? sl->rect.height() : sl->rect.width();
            int ticks = sl->tickPosition;
            int n = 0;
            if (ticks & QSlider::TicksAbove)
                ++n;
            if (ticks & QSlider::TicksBelow)
                ++n;
            if (!n) {
                ret = space;
                break;
            }
            int thick = 12;
            if (ticks != QSlider::TicksBothSides && ticks != QSlider::NoTicks)
                thick += pixelMetric(PM_SliderLength, sl, widget) / 4;

            space -= thick;
            if (space > 0)
                thick += (space * 2) / (n + 2);
            ret = thick;
        } else {
            ret = 0;
        }
        break;
#endif // QT_NO_SLIDER

#ifndef QT_NO_MENU

    case PM_SmallIconSize:
        ret = windowsCEIconSize;
        break;
    case PM_ButtonMargin:
        ret = 6;
        break;

    case PM_LargeIconSize:
        ret = 32;
        break;

    case PM_IconViewIconSize:
        ret = pixelMetric(PM_LargeIconSize, opt, widget);
        break;

    case PM_ToolBarIconSize:
        ret = windowsCEIconSize;
        break;
    case PM_DockWidgetTitleMargin:
        ret = 2;
        break;
#if defined(Q_WS_WIN)
//    case PM_DockWidgetFrameWidth:
//        ret = GetSystemMetrics(SM_CXFRAME);
//        break;
#else
    case PM_DockWidgetFrameWidth:
        ret = 4;
        break;
#endif // Q_WS_WIN
    break;

#endif // QT_NO_MENU

   case PM_TitleBarHeight:
          ret = 30;
        break;
    case PM_ScrollBarExtent:
        ret = 19;
        break;
    case PM_SplitterWidth:
        ret = qMax(4, QApplication::globalStrut().width());
        break;

#if defined(Q_WS_WIN)
    case PM_MDIFrameWidth:
        ret = 3;
        break;
#endif
    case PM_ToolBarItemMargin:
        ret = 1;
        break;
    case PM_ToolBarItemSpacing:
        ret = 0;
        break;
    case PM_ToolBarHandleExtent:
        ret = 10;
        break;
    case PM_ButtonIconSize:
        ret = 22;
        break;
    default:
        ret = QWindowsStyle::pixelMetric(pm, opt, widget);
        break;
    }
    return ret;
}

QPixmap QWindowsCEStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                                      const QWidget *widget) const {
#ifndef QT_NO_IMAGEFORMAT_XPM
    /*switch (standardPixmap) {

    default:
        break;
    }*/
#endif //QT_NO_IMAGEFORMAT_XPM
    return QWindowsStyle::standardPixmap(standardPixmap, opt, widget);
}

int QWindowsCEStyle::styleHint(StyleHint hint, const QStyleOption *opt, const QWidget *widget,
                             QStyleHintReturn *returnData) const {
    int ret;
    switch (hint) {
    case SH_TabBar_ElideMode:
        ret = Qt::ElideMiddle;
        break;
    case SH_EtchDisabledText:
        ret = false;
        break;
    case SH_RequestSoftwareInputPanel:
        ret = RSIP_OnMouseClick;
        break;
    default:
        ret = QWindowsStyle::styleHint(hint, opt, widget, returnData);
        break;
    }
    return ret;
}

void QWindowsCEStylePrivate::drawWinShades(QPainter *p,
                           int x, int y, int w, int h,
                           const QColor &c1, const QColor &c2,
                           const QColor &c3, const QColor &c4,
                           const QBrush *fill) {
    if (w < 2 || h < 2)                        // can't do anything with that
        return;
    QPen oldPen = p->pen();
    QPoint a[3] = { QPoint(x, y+h-2), QPoint(x, y), QPoint(x+w-2, y) };
    p->setPen(c1);
    p->drawPolyline(a, 3);
    QPoint b[3] = { QPoint(x, y+h-1), QPoint(x+w-1, y+h-1), QPoint(x+w-1, y) };
    p->setPen(c2);
    p->drawPolyline(b, 3);
    if (w > 4 && h > 4) {
        QPoint c[3] = { QPoint(x+1, y+h-3), QPoint(x+1, y+1), QPoint(x+w-3, y+1) };
        p->setPen(c3);
        p->drawPolyline(c, 3);
        QPoint d[3] = { QPoint(x+1, y+h-2), QPoint(x+w-2, y+h-2), QPoint(x+w-2, y+1) };
        p->setPen(c4);
        p->drawPolyline(d, 3);
        if (fill)
            p->fillRect(QRect(x+2, y+2, w-4, h-4), *fill);
    }
    p->setPen(oldPen);
}

void QWindowsCEStylePrivate::drawWinCEShades(QPainter *p,
                           int x, int y, int w, int h,
                           const QColor &c1, const QColor &c2,
                           const QColor &c3, const QColor &c4,
                           const QBrush *fill) {
    if (w < 2 || h < 2)                        // can't do anything with that
        return;
    QPen oldPen = p->pen();
    QPoint b[3] = { QPoint(x, y+h-1), QPoint(x+w-1, y+h-1), QPoint(x+w-1, y) };
    p->setPen(c2);
    p->drawPolyline(b, 3);
    if (w > 4 && h > 4) {
        QPoint c[3] = { QPoint(x+1, y+h-3), QPoint(x+1, y+1), QPoint(x+w-3, y+1) };
        p->setPen(c3);
        p->drawPolyline(c, 3);
        QPoint d[5] = { QPoint(x, y+h-2), QPoint(x+w-2, y+h-2), QPoint(x+w-2, y), QPoint(x, y), QPoint(x, y+h-2) };
        p->setPen(c4);
        p->drawPolyline(d, 5);
        if (fill)
            p->fillRect(QRect(x+2, y+2, w-4, h-4), *fill);
    }
    QPoint a[3] = { QPoint(x+1, y+h-3), QPoint(x+1, y+1), QPoint(x+w-3, y+1) };
    p->setPen(c1);
    p->drawPolyline(a, 3);
    p->setPen(oldPen);
}

void QWindowsCEStylePrivate::drawWinCEShadesSunken(QPainter *p,
                           int x, int y, int w, int h,
                           const QColor &c1, const QColor &c2,
                           const QColor &c3, const QColor &c4,
                           const QBrush *fill) {
    if (w < 2 || h < 2)                        // can't do anything with that
        return;
    QPen oldPen = p->pen();

    QPoint b[3] = { QPoint(x, y+h-1), QPoint(x+w-1, y+h-1), QPoint(x+w-1, y) };
    p->setPen(c2);
    p->drawPolyline(b, 3);
    if (w > 4 && h > 4) {
        QPoint d[3] = { QPoint(x, y+h-2), QPoint(x+w-2, y+h-2), QPoint(x+w-2, y) };
        p->setPen(c4);
        p->drawPolyline(d, 3);
        QPoint c[3] = { QPoint(x, y+h-2), QPoint(x, y), QPoint(x+w-2, y) };
        p->setPen(c3);
        p->drawPolyline(c, 3);
        if (fill)
            p->fillRect(QRect(x+2, y+2, w-4, h-4), *fill);
    }
    QPoint a[3] = { QPoint(x+1, y+h-3), QPoint(x+1, y+1), QPoint(x+w-3, y+1) };
    p->setPen(c1);
    p->drawPolyline(a, 3);
    p->setPen(oldPen);
}


void QWindowsCEStylePrivate::drawWinCEButton(QPainter *p, int x, int y, int w, int h,
                     const QPalette &pal, bool sunken,
                     const QBrush *fill) {
    if (sunken)
        drawWinCEShadesSunken(p, x, y, w, h,
                       pal.shadow().color(), pal.light().color(), pal.shadow().color(),
                       pal.midlight().color(), fill);
    else
        drawWinCEShades(p, x, y, w, h,
                       pal.midlight().color(), pal.shadow().color(), pal.button().color(),
                       pal.dark().color(), fill);
}

void QWindowsCEStylePrivate::drawWinCEPanel(QPainter *p, int x, int y, int w, int h,
                    const QPalette &pal, bool        sunken,
                    const QBrush *fill) {
    if (sunken)
        drawWinShades(p, x, y, w, h,
                        pal.dark().color(), pal.light().color(), pal.shadow().color(),
                       pal.midlight().color(), fill);
    else
        drawWinShades(p, x, y, w, h,
                       pal.light().color(), pal.shadow().color(), pal.button().color(),
                       pal.midlight().color(), fill);
}

void QWindowsCEStylePrivate::drawWinCEButton(QPainter *p, const QRect &r,
                     const QPalette &pal, bool sunken, const QBrush *fill) {
    drawWinCEButton(p, r.x(), r.y(), r.width(), r.height(), pal, sunken, fill);
}

void QWindowsCEStylePrivate::drawWinCEPanel(QPainter *p, const QRect &r,
                    const QPalette &pal, bool sunken, const QBrush *fill) {
    drawWinCEPanel(p, r.x(), r.y(), r.width(), r.height(), pal, sunken, fill);
}

QT_END_NAMESPACE

#endif // QT_NO_STYLE_WINDOWSCE
