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

#include <QtGui>

#include "javastyle.h"
#include <math.h>

static const int windowsItemFrame        =  2;
static const int windowsSepHeight        =  2;
static const int windowsItemHMargin      =  3;
static const int windowsItemVMargin      =  2;
static const int windowsArrowHMargin     =  6;
static const int windowsTabSpacing       = 12;
static const int windowsCheckMarkHMargin =  2;
static const int windowsRightBorder      = 15;
static const int windowsCheckMarkWidth   = 12;

JavaStyle::JavaStyle()
{
    qApp->setPalette(standardPalette());
}


inline QPoint JavaStyle::adjustScrollPoint(const QPoint &point,
                    Qt::Orientation orientation,
                    bool add) const
{
    int adder = add ? -1 : 1;
    QPoint retPoint;

    if (orientation == Qt::Horizontal) {
        retPoint = QPoint(point.y() * adder, point.x());
    } else {
        retPoint = QPoint(point.x(), point.y() * adder);
    }

    return retPoint;
}

QPalette JavaStyle::standardPalette() const
{
    QPalette palette = QCommonStyle::standardPalette();

    palette.setBrush(QPalette::Active, QPalette::Button,
                     QColor(184, 207, 229));
    palette.setBrush(QPalette::Active, QPalette::WindowText,
                     Qt::black);
    palette.setBrush(QPalette::Active, QPalette::Background,
                     QColor(238, 238, 238));
    palette.setBrush(QPalette::Active, QPalette::Window,
                     QColor(238 ,238, 238));
    palette.setBrush(QPalette::Active, QPalette::Base, Qt::white);
    palette.setBrush(QPalette::Active, QPalette::AlternateBase, QColor(238, 238, 238));
    palette.setBrush(QPalette::Active, QPalette::Text, Qt::black);
    palette.setBrush(QPalette::Active, QPalette::BrightText, Qt::white);

    palette.setBrush(QPalette::Active, QPalette::Light, QColor(163, 184, 204)); // focusFrameColor
    palette.setBrush(QPalette::Active, QPalette::Midlight, QColor(99, 130, 191)); // tabBarBorderColor
    palette.setBrush(QPalette::Active, QPalette::Dark, QColor(106, 104, 100));
    palette.setBrush(QPalette::Active, QPalette::Mid, QColor(122, 138, 153)); //defaultFrameColor
    palette.setBrush(QPalette::Active, QPalette::Shadow, QColor(122, 138, 153)); // defaultFrame

    palette.setBrush(QPalette::Active, QPalette::Highlight, QColor(184, 207, 229));
    palette.setBrush(QPalette::Active, QPalette::HighlightedText, Qt::black);

    palette.setBrush(QPalette::Inactive, QPalette::Highlight, QColor(184, 207, 229));
    palette.setBrush(QPalette::Inactive, QPalette::HighlightedText, Qt::black);

    palette.setBrush(QPalette::Disabled, QPalette::Button,
                     QColor(238, 238, 238));
    palette.setBrush(QPalette::Disabled, QPalette::WindowText,
                     QColor(153, 153, 153));
    palette.setBrush(QPalette::Disabled, QPalette::Background, QColor(238, 238, 238));

    palette.setBrush(QPalette::Inactive, QPalette::Button,
                     QColor(184, 207, 229));
    palette.setBrush(QPalette::Inactive, QPalette::Background,
                     QColor(238, 238, 238));
    palette.setBrush(QPalette::Inactive, QPalette::Window,
                     QColor(238 ,238, 238));
    palette.setBrush(QPalette::Inactive, QPalette::Light, QColor(163, 184, 204)); // focusFrameColor
    palette.setBrush(QPalette::Inactive, QPalette::Midlight, QColor(99, 130, 191)); // tabBarBorderColor
    palette.setBrush(QPalette::Inactive, QPalette::Dark,QColor(106, 104, 100));
    palette.setBrush(QPalette::Inactive, QPalette::Mid, QColor(122, 138, 153)); //defaultFrame
    palette.setBrush(QPalette::Inactive, QPalette::Shadow, QColor(122, 138, 153)); // defaultFrame

    return palette;
}

inline void JavaStyle::drawScrollBarArrow(const QRect &rect, QPainter *painter,
                      const QStyleOptionSlider *option,
                      bool add) const
{

    painter->save();

    Qt::Orientation orient = option->orientation;
    QPoint offset;

    if (add) {
        if (orient == Qt::Vertical) {
            offset = rect.bottomLeft();
        } else {
            offset = rect.topRight();
        }
    } else {
        offset = rect.topLeft();
    }

    QPainterPath arrow;
    arrow.moveTo(offset + adjustScrollPoint(QPoint(4, 8), orient, add));
    arrow.lineTo(offset + adjustScrollPoint(QPoint(7, 5), orient, add));
    arrow.lineTo(offset + adjustScrollPoint(QPoint(8, 5), orient, add));
    arrow.lineTo(offset + adjustScrollPoint(QPoint(11, 8), orient, add));
    arrow.lineTo(offset + adjustScrollPoint(QPoint(4, 8), orient, add));

    QColor fillColor;
    if (option->state & State_Sunken)
        fillColor = QColor(option->palette.color(QPalette::Button));
    else
        fillColor = option->palette.color(QPalette::Background);

    painter->fillRect(rect, fillColor);

    painter->setPen(option->palette.color(QPalette::Base));
    int adjust = option->state & State_Sunken ? 0 : 1;
    painter->drawRect(rect.adjusted(adjust, adjust, -1, -1));
    painter->setPen(option->palette.color(QPalette::Mid));
    painter->drawRect(rect.adjusted(0, 0, -1, -1));

    painter->setPen(option->palette.color(QPalette::WindowText));
    painter->setBrush(option->palette.color(QPalette::WindowText));
    painter->drawPath(arrow);

    painter->restore();
}

inline QPoint JavaStyle::adjustScrollHandlePoint(Qt::Orientation orig,
                         const QPoint &point) const
{
    QPoint retPoint;

    if (orig == Qt::Vertical)
        retPoint = point;
    else
        retPoint = QPoint(point.y(), point.x());

    return retPoint;
}

void JavaStyle::drawControl(ControlElement control, const QStyleOption *option,
                            QPainter *painter, const QWidget *widget) const
{

    painter->save();

    switch (control) {
        case CE_ToolBoxTabShape: {
            const QStyleOptionToolBox *box =
            qstyleoption_cast<const QStyleOptionToolBox *>(option);

            painter->save();

            if (box->direction == Qt::RightToLeft) {
                painter->rotate(1);
                painter->translate(box->rect.width(), -box->rect.height());
            }

            int textWidth = box->fontMetrics.width(box->text) + 20;

            QPolygon innerLine;
            innerLine << (box->rect.topLeft() + QPoint(0, 1)) <<
                (box->rect.topLeft() + QPoint(textWidth, 1)) <<
                (box->rect.bottomLeft() + QPoint(textWidth + 15, -3)) <<
                (box->rect.bottomRight() + QPoint(0, -3)) <<
                box->rect.bottomRight() <<
                box->rect.bottomLeft() <<
                box->rect.topLeft();

            painter->setPen(box->palette.color(QPalette::Base));
            painter->setBrush(QColor(200, 221, 242));
            painter->drawPolygon(innerLine);

            QPolygon outerLine;
            outerLine << (box->rect.bottomRight() + QPoint(0, -3)) <<
               box->rect.bottomRight() <<
               box->rect.bottomLeft() <<
               box->rect.topLeft() <<
               (box->rect.topLeft() + QPoint(textWidth, 0)) <<
               (box->rect.bottomLeft() + QPoint(textWidth + 15, -4)) <<
               (box->rect.bottomRight() + QPoint(0, -4));

            painter->setPen(box->palette.color(QPalette::Midlight));
            painter->setBrush(Qt::NoBrush);
            painter->drawPolyline(outerLine);

            painter->restore();
            break;
        }
        case CE_DockWidgetTitle: {
            const QStyleOptionDockWidgetV2 *docker =
                new QStyleOptionDockWidgetV2(
                *qstyleoption_cast<const QStyleOptionDockWidget *>(option));

            QRect rect = docker->rect;
            QRect titleRect = rect;
            if (docker->verticalTitleBar) {
                QRect r = rect;
                QSize s = r.size();
                s.transpose();
                r.setSize(s);

                titleRect = QRect(r.left() + rect.bottom()
                                - titleRect.bottom(),
                                r.top() + titleRect.left() - rect.left(),
                                titleRect.height(), titleRect.width());

                painter->translate(r.left(), r.top() + r.width());
                painter->rotate(-90);
                painter->translate(-r.left(), -r.top());

                rect = r;
            }

            QLinearGradient gradient(rect.topLeft(),
                                     rect.bottomLeft());
            gradient.setColorAt(1.0, QColor(191, 212, 231));
            gradient.setColorAt(0.3, Qt::white);
            gradient.setColorAt(0.0, QColor(221, 232, 243));

            painter->setPen(Qt::NoPen);
            painter->setBrush(gradient);
            painter->drawRect(rect.adjusted(0, 0, -1, -1));

            if (!docker->title.isEmpty()) {
                QRect textRect = docker->fontMetrics.boundingRect(docker->title);
                textRect.moveCenter(rect.center());

                QFont font = painter->font();
                font.setPointSize(font.pointSize() - 1);
                painter->setFont(font);
                painter->setPen(docker->palette.text().color());
                painter->drawText(textRect, docker->title,
                                  QTextOption(Qt::AlignHCenter |
                                  Qt::AlignVCenter));
            }
            break;
        }
        case CE_RubberBand: {
            painter->setPen(option->palette.color(QPalette::Active,
                            QPalette::WindowText));
            painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
            break;
        }
        case CE_SizeGrip: {
            break;
        }
        case CE_HeaderSection: {
            const QStyleOptionHeader *header =
                qstyleoption_cast<const QStyleOptionHeader *>(option);

            painter->setPen(Qt::NoPen);
            painter->setBrush(option->palette.color(QPalette::Active,
                              QPalette::Background));
            painter->drawRect(option->rect);

            painter->setPen(header->palette.color(QPalette::Mid));
            if (header->orientation == Qt::Horizontal) {
                if (header->position == QStyleOptionHeader::Beginning ||
                    header->position == QStyleOptionHeader::OnlyOneSection) {
                    painter->drawRect(header->rect.adjusted(0, 0, -1, -1));
                    painter->setPen(header->palette.color(QPalette::Base));
                    painter->drawLine(header->rect.bottomLeft() + QPoint(1, -1),
                                      header->rect.topLeft() + QPoint(1, 1));
                    painter->drawLine(header->rect.topLeft() + QPoint(1, 1),
                                      header->rect.topRight() + QPoint(-1, 1));
                } else {
                    painter->drawLine(header->rect.bottomRight(),
                                      header->rect.topRight());
                    painter->drawLine(header->rect.topLeft(),
                                      header->rect.topRight());
                    painter->drawLine(header->rect.bottomLeft(),
                                      header->rect.bottomRight());
                    painter->setPen(option->palette.color(QPalette::Base));
                    painter->drawLine(header->rect.bottomLeft() + QPoint(0, -1),
                                      header->rect.topLeft() + QPoint(0, 1));
                    painter->drawLine(header->rect.topLeft() + QPoint(1, 1),
                                      header->rect.topRight() + QPoint(-1, 1));
                }
            } else { // Vertical
                if (header->position == QStyleOptionHeader::Beginning ||
                    header->position == QStyleOptionHeader::OnlyOneSection) {
                    painter->drawRect(header->rect.adjusted(0, 0, -1, -1));
                    painter->setPen(header->palette.color(QPalette::Base));
                    painter->drawLine(header->rect.bottomLeft() + QPoint(1, -1),
                                  header->rect.topLeft() + QPoint(1, 1));
                    painter->drawLine(header->rect.topLeft() + QPoint(1, 1),
                                  header->rect.topRight() + QPoint(-1, 1));
                } else {
                    painter->drawLine(header->rect.bottomLeft(),
                                      header->rect.bottomRight());
                    painter->drawLine(header->rect.topLeft(),
                                      header->rect.bottomLeft());
                    painter->drawLine(header->rect.topRight(),
                                      header->rect.bottomRight());
                    painter->setPen(header->palette.color(QPalette::Base));
                    painter->drawLine(header->rect.topLeft(),
                                      header->rect.topRight() + QPoint(-1, 0));
                    painter->drawLine(header->rect.bottomLeft() + QPoint(1, -1),
                                      header->rect.topLeft() + QPoint(1, 0));
                }
            }
            break;
        }
        case CE_ToolBar: {
            QRect rect = option->rect;

            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            gradient.setColorAt(1.0, QColor(221, 221, 221));
            gradient.setColorAt(0.0, QColor(241, 241, 241));

            if (option->state & State_Horizontal) {
                painter->setPen(QColor(204, 204, 204));
                painter->setBrush(gradient);
            } else {
                painter->setPen(Qt::NoPen);
                painter->setBrush(option->palette.color(QPalette::Background));
            }
            painter->drawRect(rect.adjusted(0, 0, -1, -1));
            break;
        }
        case CE_ProgressBar: {
            const QStyleOptionProgressBar *bar1 =
                qstyleoption_cast<const QStyleOptionProgressBar *>(option);

            QStyleOptionProgressBarV2 *bar = new QStyleOptionProgressBarV2(*bar1);

            QRect rect = bar->rect;
            if (bar->orientation == Qt::Vertical) {
                rect = QRect(rect.left(), rect.top(), rect.height(), rect.width());
                QMatrix m;
                m.translate(rect.height()-1, 0);
                m.rotate(90.0);
                painter->setMatrix(m);
            }

            painter->setPen(bar->palette.color(QPalette::Mid));
            painter->drawRect(rect.adjusted(0, 0, -1, -1));

            QRect grooveRect = subElementRect(SE_ProgressBarGroove, bar,
                                              widget);
            if (bar->orientation == Qt::Vertical) {
                grooveRect = QRect(grooveRect.left(), grooveRect.top(),
                                   grooveRect.height(), grooveRect.width());
            }

            QStyleOptionProgressBar grooveBar = *bar;
            grooveBar.rect = grooveRect;

            drawControl(CE_ProgressBarGroove, &grooveBar, painter, widget);

            QRect progressRect = subElementRect(SE_ProgressBarContents, bar,
                                                widget);
            if (bar->orientation == Qt::Vertical) {
                progressRect = QRect(progressRect.left(), progressRect.top(),
                progressRect.height(), progressRect.width());
                progressRect.adjust(0, 0, 0, -1);
            }
            QStyleOptionProgressBar progressOpt = *bar;
            progressOpt.rect = progressRect;
            drawControl(CE_ProgressBarContents, &progressOpt, painter, widget);

            QRect labelRect = subElementRect(SE_ProgressBarLabel, bar, widget);
            if (bar->orientation == Qt::Vertical) {
                labelRect = QRect(labelRect.left(), labelRect.top(),
                labelRect.height(), labelRect.width());
            }
            QStyleOptionProgressBar subBar = *bar;
            subBar.rect = labelRect;
            if (bar->textVisible)
                drawControl(CE_ProgressBarLabel, &subBar, painter, widget);

            delete bar;
            break;
        }
        case CE_ProgressBarGroove: {
            painter->setBrush(option->palette.color(QPalette::Background));
            painter->setPen(Qt::NoPen);
            painter->drawRect(option->rect.adjusted(0, 0, -1, -1));

            painter->setPen(option->palette.color(QPalette::Button));
            painter->drawLine(option->rect.topLeft() + QPoint(0, 0),
                              option->rect.topRight() + QPoint(0, 0));
            break;
        }
        case CE_ProgressBarContents: {
            const QStyleOptionProgressBar *bar =
                qstyleoption_cast<const QStyleOptionProgressBar *>(option);
            int progress = int((double(bar->progress) /
                                double(bar->maximum - bar->minimum)) *
                                bar->rect.width());

            painter->setBrush(bar->palette.color(QPalette::Light));
            painter->setPen(Qt::NoPen);
            QRect progressRect = QRect(bar->rect.topLeft(), QPoint(progress,
            bar->rect.bottom()));
            painter->drawRect(progressRect);

            painter->setPen(bar->palette.color(QPalette::Midlight));
            painter->setBrush(Qt::NoBrush);

            painter->drawLine(bar->rect.bottomLeft(), bar->rect.topLeft());
            painter->drawLine(bar->rect.topLeft(), QPoint(progress,
                              bar->rect.top()));
            break;
        }
        case CE_ProgressBarLabel: {
            painter->save();
            const QStyleOptionProgressBar *bar =
                qstyleoption_cast<const QStyleOptionProgressBar *>(option);

            QRect rect = bar->rect;
                QRect leftRect;

            int progressIndicatorPos = int((double(bar->progress) /
                    double(bar->maximum - bar->minimum)) *
                    bar->rect.width());

            QFont font;
            font.setBold(true);
            painter->setFont(font);
            painter->setPen(bar->palette.color(QPalette::Midlight));

            if (progressIndicatorPos >= 0 &&
                progressIndicatorPos <= rect.width()) {
                leftRect = QRect(bar->rect.topLeft(),
                                 QPoint(progressIndicatorPos,
                bar->rect.bottom()));
            } else if (progressIndicatorPos > rect.width()) {
                painter->setPen(bar->palette.color(QPalette::Base));
            } else {
                painter->setPen(bar->palette.color(QPalette::Midlight));
            }

            QRect textRect = QFontMetrics(font).boundingRect(bar->text);
            textRect.moveCenter(option->rect.center());
            painter->drawText(textRect, bar->text,
                              QTextOption(Qt::AlignCenter));
            if (!leftRect.isNull()) {
                painter->setPen(bar->palette.color(QPalette::Base));
                painter->setClipRect(leftRect, Qt::IntersectClip);
                painter->drawText(textRect, bar->text,
                                  QTextOption(Qt::AlignCenter));
            }

            painter->restore();
            break;
        }
        case CE_MenuBarEmptyArea: {
            QRect emptyArea = option->rect.adjusted(0, 0, -1, -1);
            QLinearGradient gradient(emptyArea.topLeft(), emptyArea.bottomLeft()
                                     - QPoint(0, 1));
            gradient.setColorAt(0.0, option->palette.color(QPalette::Base));
            gradient.setColorAt(1.0, QColor(223, 223, 223));

            painter->setPen(QColor(238, 238, 238));
            painter->setBrush(gradient);
            painter->drawRect(emptyArea.adjusted(0, 0, 0, -1));
            break;
        }
        case CE_MenuBarItem: {
            if (!(option->state & State_Sunken)) {
                QLinearGradient gradient(option->rect.topLeft(),
                                         option->rect.bottomLeft());
                gradient.setColorAt(0.0, Qt::white);
                gradient.setColorAt(1.0, QColor(223, 223, 223));

                painter->setPen(Qt::NoPen);
                painter->setBrush(gradient);
            } else {
                painter->setBrush(option->palette.color(QPalette::Light));
            }

            painter->drawRect(option->rect);
            if (option->state & State_Sunken) {
                painter->setPen(option->palette.color(QPalette::Mid));
                painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
                painter->setPen(option->palette.color(QPalette::Base));
                painter->setBrush(Qt::NoBrush);
                painter->drawLine(option->rect.bottomRight() + QPoint(0, -1),
                                  option->rect.topRight() + QPoint(0, -1));
            }
            QCommonStyle::drawControl(control, option, painter, widget);
            break;
        }
        case CE_MenuItem: {
            const QStyleOptionMenuItem *menuItem =
                qstyleoption_cast<const QStyleOptionMenuItem *>(option);

            bool selected = menuItem->state & State_Selected;
            bool checkable = menuItem->checkType !=
                             QStyleOptionMenuItem::NotCheckable;
            bool checked = menuItem->checked;

            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                QPoint center = menuItem->rect.center();

                painter->setPen(menuItem->palette.color(QPalette::Midlight));
                painter->drawLine(QPoint(menuItem->rect.left() - 2, center.y()),
                                  QPoint(menuItem->rect.right(), center.y()));
                painter->setPen(menuItem->palette.color(QPalette::Base));
                painter->drawLine(QPoint(menuItem->rect.left() - 2,
                                  center.y() + 1),
                QPoint(menuItem->rect.right(),
                      center.y() + 1));

                break;
            }

            if (selected) {
                painter->setBrush(menuItem->palette.color(QPalette::Light));
                painter->setPen(Qt::NoPen);
                painter->drawRect(menuItem->rect);
                painter->setPen(menuItem->palette.color(QPalette::Midlight));
                painter->drawLine(menuItem->rect.topLeft(),
                                  menuItem->rect.topRight());
                painter->setPen(menuItem->palette.color(QPalette::Base));
                painter->drawLine(menuItem->rect.bottomLeft(),
                                  menuItem->rect.bottomRight());
            }

            if (checkable) {
                QRect checkRect(option->rect.left() + 5,
                                option->rect.center().y() - 5, 10, 10);
                if (menuItem->checkType & QStyleOptionMenuItem::Exclusive) {
                    QStyleOptionButton button;
                    button.rect = checkRect;
                    button.state = menuItem->state;
                    if (button.state & State_Sunken)
                        button.state ^= State_Sunken;
                    if (checked)
                        button.state |= State_On;
                    button.palette = menuItem->palette;
                    drawPrimitive(PE_IndicatorRadioButton, &button, painter,
                              widget);
                } else {
                    QBrush buttonBrush = gradientBrush(option->rect);
                    painter->setBrush(buttonBrush);
                    painter->setPen(option->palette.color(QPalette::Mid));

                    painter->drawRect(checkRect);

                    if (checked) {
                        QImage image(":/images/checkboxchecked.png");
                        painter->drawImage(QPoint(option->rect.left() + 5,
                                           option->rect.center().y() - 8), image);
                    }
                }
            }

            bool dis = !(menuItem->state & State_Enabled);
            bool act = menuItem->state & State_Selected;
            const QStyleOption *opt = option;
            const QStyleOptionMenuItem *menuitem = menuItem;
            int checkcol = qMax(menuitem->maxIconWidth, 20);
            if (menuItem->icon.isNull())
                checkcol = 0;

            QPainter *p = painter;
            QRect vCheckRect = visualRect(opt->direction, menuitem->rect,
                                          QRect(menuitem->rect.x(),
                                          menuitem->rect.y(),
                                          checkcol, menuitem->rect.height()));
            if (!menuItem->icon.isNull()) {
                QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;
                if (act && !dis)
                    mode = QIcon::Active;
                QPixmap pixmap;
                if (checked)
                    pixmap = menuItem->icon.pixmap(
                    pixelMetric(PM_SmallIconSize), mode, QIcon::On);
                else
                    pixmap = menuItem->icon.pixmap(
                        pixelMetric(PM_SmallIconSize), mode);
                int pixw = pixmap.width();
                int pixh = pixmap.height();

                int adjustedIcon = checkable ? 15 : 0;
                QRect pmr(0, 0, pixw, pixh);
                pmr.moveCenter(vCheckRect.center());
                painter->setPen(menuItem->palette.text().color());
                if (checkable && checked)
                    painter->drawPixmap(QPoint(pmr.left() +
                    adjustedIcon, pmr.top() + 1), pixmap);
                else
                    painter->drawPixmap(pmr.topLeft() +
                    QPoint(adjustedIcon, 0), pixmap);
            }

            if (selected) {
                painter->setPen(menuItem->palette.highlightedText().color());
            } else {
                painter->setPen(menuItem->palette.text().color());
            }
            int x, y, w, h;
            menuitem->rect.getRect(&x, &y, &w, &h);
            int tab =  menuitem->tabWidth;
            QColor discol;
            if (dis) {
                discol = menuitem->palette.text().color();
                p->setPen(discol);
            }
            int xm = windowsItemFrame + checkcol + windowsItemHMargin;
            int xpos = menuitem->rect.x() + xm;
        QRect textRect;
        if (!menuItem->icon.isNull())
        textRect.setRect(xpos, y + windowsItemVMargin, w - xm -
            windowsRightBorder - tab + 1, h - 2 * windowsItemVMargin);
        else
        textRect.setRect(menuItem->rect.left() + 9,
                 y + windowsItemVMargin,
                 w - xm - windowsRightBorder - tab,
                 h - 2 * windowsItemVMargin);

        if (checkable)
        textRect.adjust(10, 0, 10, 0);

            QRect vTextRect = visualRect(opt->direction, menuitem->rect,
                     textRect);
            QString s = menuitem->text;
            if (!s.isEmpty()) {
                int t = s.indexOf(QLatin1Char('\t'));
                int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic |
                 Qt::TextDontClip | Qt::TextSingleLine;
                if (!styleHint(SH_UnderlineShortcut, menuitem, widget))
                    text_flags |= Qt::TextHideMnemonic;
                text_flags |= Qt::AlignLeft;
                if (t >= 0) {
                    QRect vShortcutRect = visualRect(opt->direction,
                             menuitem->rect,
                        QRect(textRect.topRight(),
            QPoint(menuitem->rect.right(), textRect.bottom())));
                    if (dis && !act) {
                        p->setPen(menuitem->palette.light().color());
                        p->drawText(vShortcutRect.adjusted(1, 1, 1, 1),
                    text_flags,
                    s.mid(t + 1));
                        p->setPen(discol);
                    }
                    p->drawText(vShortcutRect, text_flags, s.mid(t + 1));
                    s = s.left(t);
                }
                QFont font = menuitem->font;
                if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                    font.setBold(true);
                p->setFont(font);
                if (dis && !act) {
                    p->setPen(menuitem->palette.light().color());
                    p->drawText(vTextRect.adjusted(1,1,1,1), text_flags,
                s.left(t));
                    p->setPen(discol);
                }
                p->drawText(vTextRect, text_flags, s.left(t));
            }

            if (menuItem->menuItemType & QStyleOptionMenuItem::SubMenu) {
                QPoint center = menuItem->rect.center();
                QPoint drawStart(menuItem->rect.right() - 6, center.y() + 4);

                QPainterPath arrow;
                arrow.moveTo(drawStart);
                arrow.lineTo(drawStart + QPoint(0, -8));
                arrow.lineTo(drawStart + QPoint(4, -5));
                arrow.lineTo(drawStart + QPoint(4, -4));
                arrow.lineTo(drawStart + QPoint(0, 0));

                painter->save();
                painter->setBrush(menuItem->palette.color(QPalette::Text));
                painter->setPen(Qt::NoPen);
                painter->drawPath(arrow);
                painter->restore();
            }

            break;
        }
        case CE_MenuVMargin: {
            break;
        }
        case CE_MenuHMargin: {
            break;
        }
        case CE_Splitter: {
            drawSplitter(option, painter, option->state & State_Horizontal);
            break;
        }
        case CE_ScrollBarAddPage: {
        case CE_ScrollBarSubPage:
            const QStyleOptionSlider *scrollBar =
                qstyleoption_cast<const QStyleOptionSlider *>(option);
            QRect myRect;
            if (scrollBar->orientation == Qt::Horizontal) {
                myRect = QRect(option->rect.topLeft(),
                option->rect.bottomRight()).adjusted(0, 0, 1, -1);
            } else {
                myRect = option->rect;
            }

            painter->setPen(Qt::NoPen);
            painter->setBrush(option->palette.color(QPalette::Background));
            painter->drawRect(myRect);

            painter->setBrush(Qt::NoBrush);
            painter->setPen(scrollBar->palette.color(QPalette::Mid));
            painter->drawRect(myRect.adjusted(0, 0, -1, 0));
            painter->setPen(scrollBar->palette.color(QPalette::Button));
            painter->drawLine(myRect.bottomLeft() + QPoint(1, 0),
                              myRect.topLeft() + QPoint(1, 1));
            painter->drawLine(myRect.topLeft() + QPoint(1, 1),
                              myRect.topRight() + QPoint(-1, 1));
            break;
        }
        case CE_ScrollBarSubLine: {
            const QStyleOptionSlider *scrollBar =
                qstyleoption_cast<const QStyleOptionSlider *>(option);
            int scrollBarExtent = pixelMetric(PM_ScrollBarExtent);
            QRect scrollBarSubLine = option->rect;

            QRect button1;
            QRect button2;

            if (scrollBar->orientation == Qt::Horizontal) {
                button1.setRect(scrollBarSubLine.left(), scrollBarSubLine.top(),
                16, scrollBarExtent);
                button2.setRect(scrollBarSubLine.right() - 15,
                scrollBarSubLine.top(), 16, scrollBarExtent);
            } else {
                button1.setRect(scrollBarSubLine.left(), scrollBarSubLine.top(),
                scrollBarExtent, 16);
                button2.setRect(scrollBarSubLine.left(),
                scrollBarSubLine.bottom() - 15, scrollBarExtent, 16);
            }

            painter->fillRect(button2, Qt::blue);

            drawScrollBarArrow(button1, painter, scrollBar);
            drawScrollBarArrow(button2, painter, scrollBar);
            break;
        }
        case CE_ScrollBarAddLine: {
            const QStyleOptionSlider *scrollBar =
                qstyleoption_cast<const QStyleOptionSlider *>(option);
            QRect button(option->rect.left(), option->rect.top(), 16, 16);
            drawScrollBarArrow(button, painter, scrollBar, true);
            break;
        }
        case CE_ScrollBarSlider: {
            const QStyleOptionSlider *scrollBar =
                qstyleoption_cast<const QStyleOptionSlider *>(option);

            painter->setPen(scrollBar->palette.color(QPalette::Midlight));
            painter->drawRect(scrollBar->rect.adjusted(-1, 0, -3, -1));

            QPoint g1, g2;
            if (scrollBar->orientation == Qt::Horizontal) {
                g1 = option->rect.topLeft();
                g2 = option->rect.bottomLeft();
            } else {
                g1 = option->rect.topLeft();
                g2 = option->rect.topRight();
            }

            if (scrollBar->state & State_Enabled) {
                QLinearGradient gradient(g1, g2);
                gradient.setColorAt(1.0, QColor(188, 210, 230));
                gradient.setColorAt(0.3, Qt::white);
                gradient.setColorAt(0.0, QColor(223, 233, 243));
                painter->setBrush(gradient);
            } else {
                painter->setPen(scrollBar->palette.buttonText().color());
                painter->setBrush(scrollBar->palette.button());
            }
            painter->drawRect(scrollBar->rect.adjusted(0, 0, -1, -1));

            int sliderLength = option->rect.height();
            int drawPos = scrollBar->orientation == Qt::Vertical ?
                (sliderLength / 2) + 1 : 1 - ((option->rect.width() / 2));

            QPoint origin;
            if (scrollBar->orientation == Qt::Vertical)
                origin = option->rect.bottomLeft();
            else
                origin = option->rect.topLeft();

            painter->setPen(scrollBar->palette.color(QPalette::Base));
            painter->drawLine(origin + adjustScrollHandlePoint(
                             scrollBar->orientation,
                    QPoint(4, -drawPos)),
                    origin + adjustScrollHandlePoint(
                    scrollBar->orientation,
                    QPoint(13, -drawPos)));
            painter->drawLine(origin + adjustScrollHandlePoint(
                        scrollBar->orientation,
                      QPoint(4,  2 - drawPos)),
                      origin + adjustScrollHandlePoint(
                      scrollBar->orientation,
                      QPoint(13, 2 - drawPos)));
            painter->drawLine(origin + adjustScrollHandlePoint(
                    scrollBar->orientation,
                    QPoint(4,  4 - drawPos)),
                    origin + adjustScrollHandlePoint(
                    scrollBar->orientation,
                    QPoint(13, 4 - drawPos)));

            painter->setPen(option->palette.color(QPalette::Midlight));
            painter->drawLine(origin + adjustScrollHandlePoint(
                    scrollBar->orientation,
                    QPoint(3, -(drawPos + 1))),
                    origin + adjustScrollHandlePoint(
                    scrollBar->orientation,
                    QPoint(12, -(drawPos + 1))));
            painter->drawLine(origin + adjustScrollHandlePoint(
                    scrollBar->orientation,
                    QPoint(3, 1 - drawPos)),
                    origin + adjustScrollHandlePoint(
                    scrollBar->orientation,
                    QPoint(12, 1 - drawPos)));
            painter->drawLine(origin + adjustScrollHandlePoint(
                    scrollBar->orientation,
                    QPoint(3, 3 - drawPos)),
                    origin + adjustScrollHandlePoint(
                    scrollBar->orientation,
                    QPoint(12, 3 - drawPos)));

            break;
        }
        case CE_TabBarTabLabel: {
            QStyleOptionTab copy =
                *qstyleoption_cast<const QStyleOptionTab *>(option);
            if (copy.state & State_HasFocus)
                copy.state ^= State_HasFocus;
                painter->setBrush(Qt::NoBrush);
                QCommonStyle::drawControl(CE_TabBarTabLabel, &copy, painter,
                                          widget);
            break;
        }
        case CE_TabBarTabShape: {
            const QStyleOptionTab *tab =
                qstyleoption_cast<const QStyleOptionTab *>(option);
            QRect myRect = option->rect;
            QPoint bottomLeft, bottomRight, topLeft, topRight;

            if ((tab->position == QStyleOptionTab::Beginning) ||
                (tab->position == QStyleOptionTab::OnlyOneTab)) {
                if (tab->shape == QTabBar::RoundedSouth ||
                    tab->shape == QTabBar::RoundedNorth) {
                    myRect = myRect.adjusted(2, 0, 0, 0);
                } else {
                    myRect = myRect.adjusted(0, 2, 0, 0);
                }
            }

            switch (tab->shape) {
                case QTabBar::RoundedNorth:
                    topLeft = myRect.topLeft();
                    topRight = myRect.topRight();
                    bottomLeft = myRect.bottomLeft();
                    bottomRight = myRect.bottomRight();
                    break;
                case QTabBar::RoundedSouth:
                    topLeft = myRect.bottomLeft();
                    topRight = myRect.bottomRight();
                    bottomLeft = myRect.topLeft();
                    bottomRight = myRect.topRight();
                    break;
                case QTabBar::RoundedWest:
                    topLeft = myRect.topLeft();
                    topRight = myRect.bottomLeft();
                    bottomLeft = myRect.topRight();
                    bottomRight = myRect.bottomRight();
                    break;
                case QTabBar::RoundedEast:
                    topLeft = myRect.topRight();
                    topRight = myRect.bottomRight();
                    bottomLeft = myRect.topLeft();
                    bottomRight = myRect.bottomLeft();
                    break;
                default:
                    ;
            }

            QPainterPath outerPath;
            outerPath.moveTo(bottomLeft + adjustTabPoint(QPoint(0, -2),
                             tab->shape));
            outerPath.lineTo(bottomLeft + adjustTabPoint(QPoint(0, -14),
                             tab->shape));
            outerPath.lineTo(topLeft + adjustTabPoint(QPoint(6 , 0),
                             tab->shape));
            outerPath.lineTo(topRight + adjustTabPoint(QPoint(0, 0),
                             tab->shape));
            outerPath.lineTo(bottomRight + adjustTabPoint(QPoint(0, -2),
                             tab->shape));

            if (tab->state & State_Selected ||
                tab->position == QStyleOptionTab::OnlyOneTab) {
                QPainterPath innerPath;
                innerPath.moveTo(topLeft + adjustTabPoint(QPoint(6, 2),
                                 tab->shape));
                innerPath.lineTo(topRight + adjustTabPoint(QPoint(-1, 2),
                                 tab->shape));
                innerPath.lineTo(bottomRight + adjustTabPoint(QPoint(-1 , -2),
                                 tab->shape));
                innerPath.lineTo(bottomLeft + adjustTabPoint(QPoint(2 , -2),
                                 tab->shape));
                innerPath.lineTo(bottomLeft + adjustTabPoint(QPoint(2 , -14),
                                 tab->shape));
                innerPath.lineTo(topLeft + adjustTabPoint(QPoint(6, 2),
                                 tab->shape));

                QPainterPath whitePath;
                whitePath.moveTo(bottomLeft + adjustTabPoint(QPoint(1, -2),
                                 tab->shape));
                whitePath.lineTo(bottomLeft + adjustTabPoint(QPoint(1, -14),
                                 tab->shape));
                whitePath.lineTo(topLeft + adjustTabPoint(QPoint(6, 1),
                                 tab->shape));
                whitePath.lineTo(topRight + adjustTabPoint(QPoint(-1, 1),
                                 tab->shape));

                painter->setPen(tab->palette.color(QPalette::Midlight));
                painter->setBrush(QColor(200, 221, 242));
                painter->drawPath(outerPath);
                painter->setPen(QColor(200, 221, 242));
                painter->drawRect(QRect(bottomLeft + adjustTabPoint(
                                        QPoint(2, -3), tab->shape),
                                        bottomRight + adjustTabPoint(
                                        QPoint(-2, 0), tab->shape)));
                painter->setPen(tab->palette.color(QPalette::Base));
                painter->setBrush(Qt::NoBrush);
                painter->drawPath(whitePath);

                if (option->state & State_HasFocus) {
                    painter->setPen(option->palette.color(QPalette::Mid));
                    painter->drawPath(innerPath);
                }
            } else {
                painter->setPen(tab->palette.color(QPalette::Mid));
                painter->drawPath(outerPath);
            }
            break;
        }
        case CE_PushButtonLabel:
            painter->save();

            if (const QStyleOptionButton *button =
                qstyleoption_cast<const QStyleOptionButton *>(option)) {
                QRect ir = button->rect;
                uint tf = Qt::AlignVCenter | Qt::TextShowMnemonic;
                if (!styleHint(SH_UnderlineShortcut, button, widget))
                    tf |= Qt::TextHideMnemonic;

                if (!button->icon.isNull()) {
                    QPoint point;

                    QIcon::Mode mode = button->state & State_Enabled ? QIcon::Normal
                                                                  : QIcon::Disabled;
                    if (mode == QIcon::Normal && button->state & State_HasFocus)
                        mode = QIcon::Active;
                    QIcon::State state = QIcon::Off;
                    if (button->state & State_On)
                        state = QIcon::On;

                    QPixmap pixmap = button->icon.pixmap(button->iconSize, mode,
                             state);
                    int w = pixmap.width();
                    int h = pixmap.height();

                    if (!button->text.isEmpty())
                        w += button->fontMetrics.width(button->text) + 2;

                    point = QPoint(ir.x() + ir.width() / 2 - w / 2,
                                   ir.y() + ir.height() / 2 - h / 2);

                    if (button->direction == Qt::RightToLeft)
                        point.rx() += pixmap.width();

                    painter->drawPixmap(visualPos(button->direction, button->rect,
                              point), pixmap);

                    if (button->direction == Qt::RightToLeft)
                        ir.translate(-point.x() - 2, 0);
                    else
                        ir.translate(point.x() + pixmap.width(), 0);

                    if (!button->text.isEmpty())
                        tf |= Qt::AlignLeft;

                } else {
                    tf |= Qt::AlignHCenter;
                }

                if (button->fontMetrics.height() > 14)
                    ir.translate(0, 1);

                drawItemText(painter, ir, tf, button->palette, (button->state &
                                 State_Enabled),
                             button->text, QPalette::ButtonText);
            }

            painter->restore();
            break;

        default:
            QCommonStyle::drawControl(control, option, painter, widget);
    }
    painter->restore();
}

inline QPoint JavaStyle::adjustTabPoint(const QPoint &point,
                                        QTabBar::Shape shape) const
{
    QPoint rPoint;

    switch (shape) {
        case QTabBar::RoundedWest:
            rPoint = QPoint(point.y(), point.x());
            break;
        case QTabBar::RoundedSouth:
            rPoint = QPoint(point.x(), point.y() * -1);
            break;
        case QTabBar::RoundedEast:
            rPoint = QPoint(point.y() * -1, point.x());
            break;
        default:
            rPoint = point;
    }
    return rPoint;
}

QRect JavaStyle::subControlRect(ComplexControl control,
                                const QStyleOptionComplex *option,
                                SubControl subControl,
                                const QWidget *widget) const
{
    QRect rect = QCommonStyle::subControlRect(control, option, subControl,
                                              widget);

    switch (control) {
        case CC_TitleBar: {
            const QStyleOptionTitleBar *bar =
            qstyleoption_cast<const QStyleOptionTitleBar *>(option);

            switch (subControl) {
                case SC_TitleBarMinButton: {
                    rect = QRect(bar->rect.topRight() + QPoint(-68, 2),
                    QSize(15, 15));
                    break;
                }
                case SC_TitleBarMaxButton: {
                    rect = QRect(bar->rect.topRight() + QPoint(-43, 3),
                                 QSize(15, 15));
                    break;
                }
                case SC_TitleBarCloseButton: {
                    rect = QRect(bar->rect.topRight() + QPoint(-18, 3),
                                 QSize(15, 15));
                    break;
                }
                case SC_TitleBarLabel: {
                    QRect labelRect = bar->fontMetrics.boundingRect(bar->text);
                    rect = labelRect;
                    rect.translate(bar->rect.left() + 30, 0);
                    rect.moveTop(bar->rect.top());
                    rect.adjust(0, 2, 2, 2);
                    break;
                }
                case SC_TitleBarSysMenu: {
                    rect = QRect(bar->rect.topLeft() + QPoint(6, 3),
                                 QSize(16, 16));
                    break;
                }
                default:
                    ;
            }
            break;
        }
        case CC_GroupBox: {
            const QStyleOptionGroupBox *box =
                qstyleoption_cast<const QStyleOptionGroupBox *>(option);
            bool hasCheckbox = box->subControls & SC_GroupBoxCheckBox;
            int checkAdjust = 13;

            QRect textRect = box->fontMetrics.boundingRect(box->text);

            switch (subControl) {
                case SC_GroupBoxFrame: {
                    rect = box->rect;
                    break;
                }
                case SC_GroupBoxCheckBox: {
                    if (hasCheckbox) {
                        rect = QRect(box->rect.topLeft() + QPoint(7, 4 +
                                     (textRect.height() / 2 - checkAdjust / 2)),
                        QSize(checkAdjust, checkAdjust));
                    }
                    else {
                        rect = QRect();
                    }
                    break;
                }
                case SC_GroupBoxLabel: {
                    rect = QRect(box->rect.topLeft() + QPoint(7 + (hasCheckbox ?
                    checkAdjust + 2 : 0), 4), textRect.size());
                    break;
                }
                case SC_GroupBoxContents: {
                    rect = box->rect.adjusted(10, 10 + textRect.height(), -10,
                                              -10);
                    break;
                }
                default:
                    ;
            }
            break;
        }
        case CC_SpinBox: {
            const QStyleOptionSpinBox *spinBox =
            qstyleoption_cast<const QStyleOptionSpinBox *>(option);
            int spinnerWidth = 16;
            QRect myRect = spinBox->rect;
            QPoint center = myRect.center();
            int frameWidth = pixelMetric(PM_SpinBoxFrameWidth, spinBox, widget);

            switch (subControl) {
                case SC_SpinBoxUp: {
                    rect = QRect(myRect.topRight() + QPoint(-16, 0),
                    QSize(16, center.y() - myRect.topRight().y()));
                    break;
                }
                case SC_SpinBoxDown: {
                    rect = QRect(QPoint(myRect.bottomRight().x() - 16,
                                 center.y() + 1),
                                 QSize(16, myRect.bottomRight().y() -
                                       center.y() - 1));
                    break;
                }
                case SC_SpinBoxFrame: {
                    rect = QRect(myRect.topLeft(), myRect.bottomRight() +
                                 QPoint(-16, 0));
                    break;
                }
                case SC_SpinBoxEditField: {
                    rect = QRect(myRect.topLeft() + QPoint(2, 2),
                    myRect.bottomRight() + QPoint(-15 - frameWidth, -2));
                    break;
                }
                default:
                    ;
            }
            break;
        }
        case CC_ToolButton: {
            const QStyleOptionToolButton *button =
                qstyleoption_cast<const QStyleOptionToolButton *>(option);

            switch (subControl) {
                case SC_ToolButton: {
                    rect = option->rect.adjusted(1, 1, -1, -1);
                    break;
                }
                case SC_ToolButtonMenu: {
                    rect = QRect(option->rect.bottomRight() +
                                 QPoint(-11, -11), QSize(10, 10));
                    break;
                }
            }
            break;
        }
        case CC_ComboBox: {
            const QStyleOptionComboBox *combo =
                qstyleoption_cast<const QStyleOptionComboBox *>(option);

            bool reverse = combo->direction == Qt::RightToLeft;

            switch (subControl) {
                case SC_ComboBoxFrame:
                    rect = combo->rect;
                    break;
                case SC_ComboBoxArrow:
                    if (reverse) {
                        rect = QRect(combo->rect.topLeft(),
                        combo->rect.bottomLeft() + QPoint(17, 0));
                    } else {
                        rect = QRect(combo->rect.topRight() + QPoint(-17, 0),
                        combo->rect.bottomRight());
                    }
                    break;
                case SC_ComboBoxEditField:
                    if (reverse) {
                        rect = QRect(combo->rect.topLeft() + QPoint(19, 2),
                        combo->rect.bottomRight() + QPoint(-2, 2));
                    } else {
                        rect = QRect(combo->rect.topLeft() + QPoint(2, 2),
                        combo->rect.bottomRight() + QPoint(-19, -2));
                    }
                    break;
                case SC_ComboBoxListBoxPopup:
                    rect = combo->rect;
                    break;
            }
            break;
        }
        case CC_ScrollBar: {
            const QStyleOptionSlider *scrollBar =
                qstyleoption_cast<const QStyleOptionSlider *>(option);
            int scrollBarExtent = pixelMetric(PM_ScrollBarExtent, scrollBar,
                          widget);
            int sliderMaxLength = ((scrollBar->orientation == Qt::Horizontal) ?
                                   scrollBar->rect.width() :
                                    scrollBar->rect.height()) - (16 * 3);
            int sliderMinLength = pixelMetric(PM_ScrollBarSliderMin, scrollBar,
                                              widget);
            int sliderLength;

            if (scrollBar->maximum != scrollBar->minimum) {
                uint valueRange = scrollBar->maximum - scrollBar->minimum;
                sliderLength = (scrollBar->pageStep * sliderMaxLength) /
                (valueRange + scrollBar->pageStep);

                if (sliderLength < sliderMinLength || valueRange > INT_MAX / 2)
                    sliderLength = sliderMinLength;
                if (sliderLength > sliderMaxLength)
                    sliderLength = sliderMaxLength;
            } else {
                sliderLength = sliderMaxLength;
            }
            int sliderStart = 16 + sliderPositionFromValue(scrollBar->minimum,
                                                           scrollBar->maximum,
                                                    scrollBar->sliderPosition,
                                                sliderMaxLength - sliderLength,
                                                        scrollBar->upsideDown);
            QRect scrollBarRect = scrollBar->rect;

            switch (subControl) {
                case SC_ScrollBarSubLine:
                    if (scrollBar->orientation == Qt::Horizontal) {
                        rect.setRect(scrollBarRect.left(), scrollBarRect.top(),
                        scrollBarRect.width() - 16, scrollBarExtent);
                    } else {
                        rect.setRect(scrollBarRect.left(), scrollBarRect.top(),
                        scrollBarExtent, scrollBarRect.height() - 16);
                    }
                    break;
                case SC_ScrollBarAddLine:
                    if (scrollBar->orientation == Qt::Horizontal) {
                        rect.setRect(scrollBarRect.right() - 15,
                        scrollBarRect.top(), 16, scrollBarExtent);
                    } else {
                        rect.setRect(scrollBarRect.left(), scrollBarRect.bottom()
                                     - 15, scrollBarExtent, 16);
                    }
                    break;
                case SC_ScrollBarSubPage:
                    if (scrollBar->orientation == Qt::Horizontal) {
                        rect.setRect(scrollBarRect.left() + 16, scrollBarRect.top(),
                                     sliderStart - (scrollBarRect.left() + 16),
                                     scrollBarExtent);
                    } else {
                        rect.setRect(scrollBarRect.left(), scrollBarRect.top() + 16,
                                     scrollBarExtent,
                        sliderStart - (scrollBarRect.left() + 16));
                    }
                    break;
                case SC_ScrollBarAddPage:
                    if (scrollBar->orientation == Qt::Horizontal)
                        rect.setRect(sliderStart + sliderLength, 0,
                                     sliderMaxLength - sliderStart -
                        sliderLength + 16, scrollBarExtent);
                    else
                        rect.setRect(0, sliderStart + sliderLength,
                                     scrollBarExtent, sliderMaxLength -
                    sliderStart - sliderLength + 16);
                    break;
                case SC_ScrollBarGroove:
                    if (scrollBar->orientation == Qt::Horizontal) {
                        rect = scrollBarRect.adjusted(16, 0, -32, 0);
                    } else {
                        rect = scrollBarRect.adjusted(0, 16, 0, -32);
                    }
                    break;
                case SC_ScrollBarSlider:
                    if (scrollBar->orientation == Qt::Horizontal) {
                        rect.setRect(sliderStart, 0, sliderLength,
                                     scrollBarExtent);
                    } else {
                        rect.setRect(0, sliderStart, scrollBarExtent,
                                     sliderLength);
                    }
                    break;
                default:
                    return QCommonStyle::subControlRect(control, option,
                                                        subControl, widget);
            }
            break;
        }
        case CC_Slider: {
            const QStyleOptionSlider *slider =
                qstyleoption_cast<const QStyleOptionSlider *>(option);
            rect = slider->rect;
            int tickSize = pixelMetric(PM_SliderTickmarkOffset, option, widget);
            int handleSize = pixelMetric(PM_SliderControlThickness, option,
                                         widget);

            int dist = slider->orientation == Qt::Vertical ? slider->rect.height() :
                                              slider->rect.width();
            int pos = QStyle::sliderPositionFromValue(slider->minimum,
                slider->maximum, slider->sliderValue, dist - handleSize);

            switch (subControl) {
                case SC_SliderGroove: {
                    QPoint center = rect.center();

                    if (slider->orientation == Qt::Horizontal) {
                        rect.setHeight(handleSize);
                        if (slider->tickPosition == QSlider::TicksBelow) {
                            center.ry() -= tickSize;
                        }
                    } else {
                        rect.adjust(0, 0, 0, 0);
                        rect.setWidth(handleSize);
                        if (slider->tickPosition == QSlider::TicksBelow) {
                            center.rx() -= tickSize;
                        }
                    }
                    rect.moveCenter(center);
                    break;
                }
                case SC_SliderHandle: {
                    QPoint center = rect.center();

                    if (slider->orientation == Qt::Horizontal) {
                        rect.setHeight(handleSize);
                        if (slider->tickPosition == QSlider::TicksBelow) {
                            center.ry() -= tickSize;
                        }

                        rect.moveCenter(center);

                        if (slider->upsideDown)
                            rect.setLeft(slider->rect.right() -
                                         pos - (handleSize - 1));
                        else
                            rect.setLeft(pos);

                        rect.setWidth(handleSize - 1);
                    } else {
                        rect.setWidth(handleSize);
                        if (slider->tickPosition == QSlider::TicksBelow) {
                            center.rx() -= tickSize;
                        }

                        rect.moveCenter(center);

                        if (slider->upsideDown)
                            rect.setTop(slider->rect.bottom() -
                                        ((pos + handleSize) - 2));
                        else
                            rect.setTop(slider->rect.top() + pos);

                        rect.setHeight(handleSize);
                    }
                    break;
                }
                case SC_SliderTickmarks: {
                    QPoint center = slider->rect.center();

                    if (slider->tickPosition & QSlider::TicksBelow) {
                        if (slider->orientation == Qt::Horizontal) {
                            rect.setHeight(tickSize);
                            center.ry() += tickSize / 2;
                            rect.adjust(6, 0, -10, 0);
                        } else {
                            rect.setWidth(tickSize);
                            center.rx() += tickSize / 2;
                            rect.adjust(0, 6, 0, -10);
                        }
                    } else {
                        rect = QRect();
                    }
                    rect.moveCenter(center);
                    break;
                }
                default:
                    ;
            }
            break;
        }
        default:
            return QCommonStyle::subControlRect(control, option, subControl,
                                                widget);
    }
    return rect;
}

static const char * const sliderHandleImage[] = {
    "15 16 7 1",
    "   c None",
    "+  c #FFFFFF",
    "@  c #FFFFFF",
    "$  c #FFFFFF",
    "(  c #E5EDF5",
    ")  c #F2F6FA",
    "[  c #FFFFFF",
    " +++++++++++++ ",
    "+@@@@@@@@@@@@@+",
    "+@(((((((((((@+",
    "+@(((((((((((@+",
    "+@)))))))))))@+",
    "+@[[[[[[[[[[[@+",
    "+@[[[[[[[[[[[@+",
    "+@)))))))))))@+",
    "+@)))))))))))@+",
    " +@)))))))))@+ ",
    "  +@(((((((@+  ",
    "   +@(((((@+   ",
    "    +@(((@+    ",
    "     +@(@+     ",
    "      +@+      ",
    "       +       "};


void JavaStyle::drawComplexControl(ComplexControl control,
                                   const QStyleOptionComplex *option,
                                   QPainter *painter,
                                   const QWidget *widget) const
{
    painter->save();

    switch (control) {
        case CC_TitleBar: {
            const QStyleOptionTitleBar *bar =
                qstyleoption_cast<const QStyleOptionTitleBar *>(option);

            bool sunken = bar->state & State_Sunken;

            QLinearGradient gradient(bar->rect.bottomLeft(),
                                     bar->rect.topLeft());
            gradient.setColorAt(0.0, QColor(191, 212, 231));
            gradient.setColorAt(0.7, Qt::white);
            gradient.setColorAt(1.0, QColor(221, 232, 243));

            painter->setPen(Qt::NoPen);
            if (bar->titleBarState & State_Active) {
                painter->setBrush(gradient);
            }
            else
                painter->setBrush(bar->palette.color(QPalette::Active,
                                  QPalette::Background));

            painter->drawRect(bar->rect.adjusted(0, 0, -1, -1));

            painter->setBrush(QColor(233, 233, 233));
            painter->drawRect(QRect(bar->rect.bottomLeft() + QPoint(0, 1),
                                    bar->rect.bottomRight() + QPoint(0, 2)));

            QRect minButtonRect = subControlRect(control, bar,
                                                 SC_TitleBarMinButton);
            QRect maxButtonRect = subControlRect(control, bar,
                                                 SC_TitleBarMaxButton);
            QRect closeButtonRect = subControlRect(control, bar,
                                                   SC_TitleBarCloseButton);
            QRect systemButtonRect = subControlRect(control, bar,
                                                    SC_TitleBarSysMenu);
            QRect labelRect = subControlRect(control, bar, SC_TitleBarLabel);
            QRect gripRect = QRect(QPoint(labelRect.right() + 5, bar->rect.top() + 5),
                                   QPoint(minButtonRect.left() - 5,
                                          bar->rect.bottom() - 4));

            QColor textColor = option->palette.color(QPalette::Text);
            painter->setPen(textColor);
            painter->setBrush(Qt::NoBrush);

            drawItemText(painter, labelRect, Qt::TextShowMnemonic |
                         Qt::AlignHCenter | Qt::AlignCenter,
                         bar->palette, bar->state & State_Enabled, bar->text,
                         textColor.isValid() ? QPalette::NoRole :
                         QPalette::WindowText);

            for (int i = 0; i < gripRect.width(); ++i) {
                painter->setPen(i % 2 ? bar->palette.color(QPalette::Midlight)
                                      : Qt::white);

                for (int j = 0; j < 4; ++j) {
                    painter->drawPoint(i + gripRect.left(),
                               gripRect.top() - 2 + i % 4 + 4 * j);
                }
            }

            QPixmap maximizePixmap(":/images/internalmaximize.png");
            QPixmap minimizePixmap(":/images/internalminimize.png");
            QPixmap closePixmap(":/images/internalclose.png");
            QPixmap internalPixmap(":/images/internalsystem.png");
            QPixmap internalCloseDownPixmap(":/images/internalclosedown.png");
            QPixmap minimizeDownPixmap(":/images/internalminimizedown.png");
            QPixmap maximizeDownPixmap(":/images/internalmaximizedown.png");

            if (bar->activeSubControls & SC_TitleBarCloseButton &&
                bar->state & State_Sunken)
                painter->drawPixmap(closeButtonRect.topLeft(),
                                    internalCloseDownPixmap);
            else
                painter->drawPixmap(closeButtonRect.topLeft(), closePixmap);

            if (bar->activeSubControls & SC_TitleBarMinButton &&
                bar->state & State_Sunken)
                painter->drawPixmap(minButtonRect.topLeft(),
                                    minimizeDownPixmap);
            else
                painter->drawPixmap(minButtonRect.topLeft(), minimizePixmap);

            if (bar->activeSubControls & SC_TitleBarMaxButton &&
                bar->state & State_Sunken)
                painter->drawPixmap(maxButtonRect.topLeft(),
                                    maximizeDownPixmap);
            else
                painter->drawPixmap(maxButtonRect.topLeft(), maximizePixmap);

            painter->drawPixmap(systemButtonRect.topLeft(), internalPixmap);

            break;
        }
        case CC_GroupBox: {
            const QStyleOptionGroupBox *box =
                qstyleoption_cast<const QStyleOptionGroupBox *>(option);

            QRect frameRect = subControlRect(control, box, SC_GroupBoxFrame);
            QRect labelRect = subControlRect(control, box, SC_GroupBoxLabel);
            QRect contentsRect = subControlRect(control, box,
                                                SC_GroupBoxContents);
            QRect checkerRect = subControlRect(control, box,
                                               SC_GroupBoxCheckBox);

            int y = labelRect.center().y();

            painter->setPen(box->palette.color(QPalette::Button));
            painter->drawRect(frameRect.adjusted(2, y - frameRect.top(), -2,
                                                 -2));

            painter->setPen(box->palette.color(QPalette::Background));

            if (box->subControls & SC_GroupBoxCheckBox) {
                painter->drawLine(checkerRect.left() - 1, y,
                                  checkerRect.right() + 2, y);
                QStyleOptionButton checker;
                checker.QStyleOption::operator=(*box);
                checker.rect = checkerRect;
                drawPrimitive(PE_IndicatorCheckBox, &checker, painter, widget);
            }

            if (box->subControls & SC_GroupBoxLabel && !box->text.isEmpty()) {
                painter->drawLine(labelRect.left() - 1, y,
                              labelRect.right() +1, y);

                QColor textColor = box->textColor;
                if (textColor.isValid())
                    painter->setPen(textColor);

                drawItemText(painter, labelRect,  Qt::TextShowMnemonic |
                Qt::AlignHCenter | int(box->textAlignment),
                box->palette, box->state & State_Enabled,
                box->text, textColor.isValid() ? QPalette::NoRole :
                QPalette::WindowText);
            }
            break;
        }
        case CC_SpinBox: {
            const QStyleOptionSpinBox *spinner =
            qstyleoption_cast<const QStyleOptionSpinBox *>(option);

            QRect frameRect = subControlRect(control, spinner, SC_SpinBoxFrame);
            QRect upRect = subControlRect(control, spinner, SC_SpinBoxUp);
            QRect downRect = subControlRect(control, spinner, SC_SpinBoxDown);

            painter->setPen(Qt::white);
            painter->drawRect(frameRect.adjusted(1, 1, -1, -1));
            painter->drawPoint(frameRect.bottomLeft());

            painter->setPen(spinner->palette.color(QPalette::Mid));
            painter->drawRect(frameRect.adjusted(0, 0, -1, -2));

            bool isEnabled = (spinner->state & State_Enabled);
            bool hover = isEnabled && (spinner->state & State_MouseOver);
            bool sunken = (spinner->state & State_Sunken);
            bool upIsActive = (spinner->activeSubControls == SC_SpinBoxUp);
            bool downIsActive = (spinner->activeSubControls == SC_SpinBoxDown);
            bool stepUpEnabled = spinner->stepEnabled &
                             QAbstractSpinBox::StepUpEnabled;
            bool stepDownEnabled = spinner->stepEnabled &
                               QAbstractSpinBox::StepDownEnabled;

            painter->setBrush(spinner->palette.color(QPalette::Background));

            painter->drawRect(upRect);
            if (upIsActive && stepUpEnabled) {
                if (sunken) {
                    drawSunkenButtonShadow(painter, upRect,
                                       spinner->palette.color(QPalette::Mid));
                } else if (hover) {
                    drawButtonHoverFrame(painter, upRect,
                    spinner->palette.color(QPalette::Mid),
                    spinner->palette.color(QPalette::Button));
                }
            }

            QStyleOptionSpinBox upSpin = *spinner;
            upSpin.rect = upRect;
            drawPrimitive(PE_IndicatorSpinUp, &upSpin, painter, widget);

            painter->drawRect(downRect);
            if (downIsActive && stepDownEnabled) {
                if (sunken) {
                    drawSunkenButtonShadow(painter, downRect,
                    spinner->palette.color(QPalette::Mid));
                } else if (hover) {
                    drawButtonHoverFrame(painter, downRect,
                    spinner->palette.color(QPalette::Mid),
                    spinner->palette.color(QPalette::Button));
                }
            }

            QStyleOptionSpinBox downSpin = *spinner;
            downSpin.rect = downRect;
            drawPrimitive(PE_IndicatorSpinDown, &downSpin, painter, widget);

            break;
        }
        case CC_ToolButton: {
            const QStyleOptionToolButton *button =
                qstyleoption_cast<const QStyleOptionToolButton *>(option);

            painter->setPen(Qt::white);
            painter->drawRect(button->rect.adjusted(1, 1, -1, -1));

            QStyleOptionToolButton panelOption = *button;
            QRect panelRect;
            if (!(button->state & State_MouseOver) &&
                !(button->state & State_On)) {
                painter->setPen(QColor(153, 153, 153));
                painter->drawRect(button->rect.adjusted(0, 0, -2, -2));

                panelRect = subControlRect(control, option, SC_ToolButton);
                panelOption.rect = panelRect;
            } else {
                panelOption.rect.adjust(0, 0, -1, -1);
            }

            QRect menuRect = subControlRect(control, option, SC_ToolButtonMenu);

            drawPrimitive(PE_PanelButtonTool, &panelOption, painter, widget);

            QStyleOptionToolButton menuOption = *button;
            menuOption.rect = menuRect;

            QStyleOptionToolButton label = *button;
            int fw = 5;

            drawControl(CE_ToolButtonLabel, &label, painter, widget);
            if (button->subControls & SC_ToolButtonMenu) {
                painter->setPen(button->palette.color(QPalette::WindowText));
                drawPrimitive(PE_IndicatorArrowDown, &menuOption, painter, widget);
            }

            if (button->state & State_HasFocus) {
                QStyleOptionToolButton focusOption = *button;
                focusOption.rect = label.rect.adjusted(-1, -1, 1, 1);

                drawPrimitive(PE_FrameFocusRect, &focusOption, painter, widget);
            }

            break;
        }
        case CC_ComboBox: {
            const QStyleOptionComboBox *combo =
                qstyleoption_cast<const QStyleOptionComboBox *>(option);

            QRect frameRect = subControlRect(control, option, SC_ComboBoxFrame,
                                         widget);
            painter->setPen(combo->palette.color(QPalette::Mid));

            if (option->state & State_HasFocus)
                painter->setBrush(option->palette.color(QPalette::Light));
            else
                painter->setBrush(combo->palette.color(QPalette::Background));

            painter->drawRect(frameRect.adjusted(0, 0, -1, -1));

            QRect arrowRect = subControlRect(control, option, SC_ComboBoxArrow,
                                             widget);
            painter->setPen(combo->palette.color(QPalette::Button));
            painter->setBrush(Qt::NoBrush);

            if (combo->direction == Qt::LeftToRight) {
                painter->drawRect(QRect(frameRect.topLeft() + QPoint(1, 1),
                                  arrowRect.bottomLeft() + QPoint(-2, -2)));
            } else {
                painter->drawRect(QRect(arrowRect.topLeft() + QPoint(1, 1),
                                    frameRect.bottomRight() + QPoint(-2, -2)));
            }

            QStyleOptionButton button;
            button.rect = arrowRect;
            button.state = combo->state;
            button.palette = combo->palette;

            if (button.state & State_On)
                button.state ^= State_On;

            painter->save();
            drawButtonBackground(&button, painter, false);
            painter->restore();

            QPoint center = arrowRect.center();
            QPoint offset = QPoint(arrowRect.bottomLeft().x() + 1,
                               center.y() + 7);
            QPainterPath arrow;
            arrow.moveTo(offset + QPoint(4, -8));
            arrow.lineTo(offset + QPoint(7, -5));
            arrow.lineTo(offset + QPoint(8, -5));
            arrow.lineTo(offset + QPoint(11, -8));
            arrow.lineTo(offset + QPoint(4, -8));

            painter->setBrush(combo->palette.color(QPalette::WindowText));
            painter->setPen(combo->palette.color(QPalette::WindowText));

            painter->drawPath(arrow);

            QRect fieldRect = subControlRect(control, option,
                                             SC_ComboBoxEditField, widget);

            break;
        }
        case CC_Slider: {
            const QStyleOptionSlider *slider =
                qstyleoption_cast<const QStyleOptionSlider *>(option);

            bool horizontal = slider->orientation == Qt::Horizontal;

            QRect groove = subControlRect(control, option, SC_SliderGroove,
                                          widget);
            QRect ticks = subControlRect(control, option, SC_SliderTickmarks,
                                         widget);
            QRect handle = subControlRect(control, option, SC_SliderHandle,
                                          widget);

            QRect afterHandle = QRect(handle.topLeft() + xySwitch(QPoint(4, 6), horizontal),
                                      groove.bottomRight() + xySwitch(QPoint(-4, -6), horizontal));
            QRect beforeHandle = QRect(groove.topLeft() + xySwitch(QPoint(4, 6), horizontal),
                                       handle.bottomRight() + xySwitch(QPoint(-4, -6), horizontal));

            if (slider->upsideDown || !horizontal) {
                QRect remember;
                remember = afterHandle;
                afterHandle = beforeHandle;
                beforeHandle = remember;
            }

            painter->setPen(slider->palette.color(QPalette::Mid));
            painter->setBrush(option->palette.color(QPalette::Background));
            painter->drawRect(afterHandle);
            painter->setPen(slider->palette.color(QPalette::Light));
            painter->drawLine(afterHandle.topLeft() + xySwitch(QPoint(0, 1), horizontal),
                          afterHandle.topRight() + xySwitch(QPoint(0, 1), horizontal));
            painter->setPen(option->palette.color(QPalette::Midlight));

            if (horizontal) {
                painter->setBrush(gradientBrush(QRect(QPoint(groove.x(),
                                                handle.y() + 1),
                                                QSize(groove.width(),
                                                handle.height() + 1))));
            } else {
                QRect rect = QRect(QPoint(groove.x(),
                                   handle.x() - 1),
                                   QSize(groove.height(),
                                   handle.width() + 1));
                QLinearGradient gradient(groove.bottomLeft(),
                                         groove.bottomRight());
                gradient.setColorAt(1.0, QColor(188, 210, 230));
                gradient.setColorAt(0.3, Qt::white);
                gradient.setColorAt(0.0, QColor(223, 233, 243));

                painter->setBrush(gradient);
            }

            painter->drawRect(beforeHandle);

            QPainterPath handlePath;
            QPainterPath innerPath;
            QPoint topLeft, topRight, bottomLeft;
            if (horizontal) {
                topLeft = handle.topLeft();
                topRight = handle.topRight();
                bottomLeft = handle.bottomLeft();
            } else {
                topLeft = handle.bottomLeft();
                topRight = handle.topLeft();
                bottomLeft = handle.topRight();
            }

            if (horizontal) {
                QImage image(sliderHandleImage);

                image.setColor(1,
                    option->palette.color(QPalette::Midlight).rgb());
                image.setColor(2,
                    option->palette.color(QPalette::Button).rgb());

                if (!(slider->state & State_Enabled)) {
                    image.setColor(4, slider->palette.color(QPalette::Background).rgb());
                    image.setColor(5, slider->palette.color(QPalette::Background).rgb());
                    image.setColor(6, slider->palette.color(QPalette::Background).rgb());
                }

                painter->drawImage(handle.topLeft(), image);
            } else {
                QImage image(":/images/verticalsliderhandle.png");
                painter->drawImage(handle.topLeft(), image);
            }

            if (slider->tickPosition & QSlider::TicksBelow) {
                painter->setPen(slider->palette.color(QPalette::Light));
                int tickInterval = slider->tickInterval ? slider->tickInterval :
                                                          slider->pageStep;

                for (int i = 0; i <= slider->maximum; i += tickInterval) {
                    if (horizontal) {
                        int pos = int(((i / double(slider->maximum)) *
                            ticks.width()) - 1);
                        painter->drawLine(QPoint(ticks.left() + pos,
                        ticks.top() + 2), QPoint(ticks.left() + pos, ticks.top() + 8));
                    } else {
                        int pos = int(((i / double(slider->maximum)) *
                            ticks.height()) - 1);
                        painter->drawLine(QPoint(ticks.left() + 2, ticks.bottom() - pos),
                                          QPoint(ticks.right() - 2, ticks.bottom() - pos));
                    }
                }
                if (horizontal) {
                    painter->drawLine(QPoint(ticks.right(), ticks.top() + 2),
                                      QPoint(ticks.right(), ticks.top() + 8));
                } else {
                    painter->drawLine(QPoint(ticks.left() + 2, ticks.top()),
                              QPoint(ticks.right() - 2, ticks.top()));
                }
            }
            break;
        }
        default:
            QCommonStyle::drawComplexControl(control, option, painter, widget);
    }
    painter->restore();
}

inline void JavaStyle::drawSunkenButtonShadow(QPainter *painter,
                                              QRect rect,
                                              const QColor &frameColor,
                                              bool reverse) const
{
    painter->save();

    painter->setPen(frameColor);

    if (!reverse) {
        painter->drawLine(QLine(QPoint(rect.x() + 1, rect.y() + 1),
                                QPoint(rect.x() + rect.width() - 1, rect.y() + 1)));
        painter->drawLine(QLine(QPoint(rect.x() + 1, rect.y()),
                                QPoint(rect.x() + 1, rect.y() + rect.height())));
    } else {
        painter->drawLine(QLine(QPoint(rect.right(), rect.bottom()),
                                QPoint(rect.right(), rect.top())));
        painter->drawLine(QLine(QPoint(rect.left(), rect.top() + 1),
                                QPoint(rect.right(), rect.top() + 1)));
    }
    painter->restore();
}

inline void JavaStyle::drawButtonHoverFrame(QPainter *painter, QRect rect,
                                            const QColor &frameColor,
                                            const QColor &activeFrame) const
{
    painter->save();

    painter->setPen(activeFrame);
    painter->drawRect(rect);
    rect.adjust(1, 1, -1, -1);
    painter->setPen(frameColor);
    painter->drawRect(rect);
    rect.adjust(1, 1, -1, -1);
    painter->setPen(activeFrame);
    painter->drawRect(rect);

    painter->restore();
}

QStyle::SubControl JavaStyle::hitTestComplexControl(ComplexControl control,
                                                    const QStyleOptionComplex *option,
                                                    const QPoint &pos,
                                                    const QWidget *widget) const
{
    SubControl ret = SC_None;

    switch (control) {
        case CC_TitleBar: {
            const QStyleOptionTitleBar *bar =
                qstyleoption_cast<const QStyleOptionTitleBar *>(option);

            QRect maximize = subControlRect(control, bar, SC_TitleBarMaxButton);
            if (maximize.contains(pos)) {
                ret = SC_TitleBarMaxButton;
                break;
            }
            QRect minimize = subControlRect(control, bar, SC_TitleBarMinButton);
            if (minimize.contains(pos)) {
                ret = SC_TitleBarMinButton;
                break;
            }
            QRect close = subControlRect(control, bar, SC_TitleBarCloseButton);
            if (close.contains(pos)) {
                ret = SC_TitleBarCloseButton;
                break;
            }
            QRect system = subControlRect(control, bar, SC_TitleBarSysMenu);
            if (system.contains(pos)) {
                ret = SC_TitleBarSysMenu;
                break;
            }
            ret = SC_TitleBarLabel;
            break;
        }
        case CC_ScrollBar:
            if (const QStyleOptionSlider *scrollBar =
                qstyleoption_cast<const QStyleOptionSlider *>(option)) {
                    QRect slider = subControlRect(control, scrollBar,
                                                  SC_ScrollBarSlider, widget);
                if (slider.contains(pos)) {
                    ret = SC_ScrollBarSlider;
                    break;
                }

                QRect scrollBarAddLine = subControlRect(control, scrollBar,
                    SC_ScrollBarAddLine, widget);
                if (scrollBarAddLine.contains(pos)) {
                    ret = SC_ScrollBarAddLine;
                    break;
                }

                QRect scrollBarSubPage = subControlRect(control, scrollBar,
                            SC_ScrollBarSubPage, widget);
                if (scrollBarSubPage.contains(pos)) {
                    ret = SC_ScrollBarSubPage;
                    break;
                }

                QRect scrollBarAddPage = subControlRect(control, scrollBar,
                            SC_ScrollBarAddPage, widget);
                if (scrollBarAddPage.contains(pos)) {
                    ret = SC_ScrollBarAddPage;
                    break;
                }

                QRect scrollBarSubLine = subControlRect(control, scrollBar,
                            SC_ScrollBarSubLine, widget);
                if (scrollBarSubLine.contains(pos)) {
                    ret = SC_ScrollBarSubLine;
                    break;
                }
            }
            break;

        default:
            ret = QCommonStyle::hitTestComplexControl(control, option, pos,
                                                      widget);
    }
    return ret;
}

void JavaStyle::polish(QWidget *widget)
{
    if (qobject_cast<QCheckBox *>(widget) ||
        qobject_cast<QRadioButton *>(widget) ||
        qobject_cast<QPushButton *>(widget) ||
        qobject_cast<QToolButton *>(widget) ||
        qobject_cast<QSpinBox *>(widget) ||
        qobject_cast<QGroupBox *>(widget))
            widget->setAttribute(Qt::WA_Hover, true);
}

void JavaStyle::unpolish(QWidget *widget)
{
    if (qobject_cast<QPushButton *>(widget) ||
        qobject_cast<QCheckBox *>(widget) ||
        qobject_cast<QRadioButton *>(widget) ||
        qobject_cast<QToolButton *>(widget) ||
        qobject_cast<QSpinBox *>(widget) ||
        qobject_cast<QGroupBox *>(widget))
            widget->setAttribute(Qt::WA_Hover, false);
}

void JavaStyle::drawSplitter(const QStyleOption *option, QPainter *painter,
                 bool horizontal) const
{
    QRect rect = option->rect;

    painter->setPen(Qt::NoPen);
    painter->setBrush(option->palette.color(QPalette::Background));

    painter->drawRect(rect);

    QColor colors[] = { Qt::white, option->palette.color(QPalette::Mid) };
    int iterations = horizontal ? rect.height() - 1 : rect.width() - 1;
    for (int i = 0; i < iterations; ++i) {
        painter->setPen(colors[i % 2]);
        painter->drawPoint(xySwitch(QPoint(rect.x() + 0 + (i % 4),
                                           rect.y() + i), horizontal));
    }
}

inline QPoint JavaStyle::xySwitch(const QPoint &point, bool horizontal) const
{
    QPoint retPoint = point;

    if (!horizontal) {
        retPoint = QPoint(point.y(), point.x());
    }

    return retPoint;
}

void JavaStyle::drawPrimitive(PrimitiveElement element,
                              const QStyleOption *option,
                              QPainter *painter,
                              const QWidget *widget) const
{
    painter->save();

    switch (element) {
        case PE_PanelButtonBevel:
        case PE_FrameButtonBevel: {
            painter->save();
            painter->setBrush(option->palette.background());
            painter->setPen(Qt::NoPen);
            painter->drawRect(option->rect);
            painter->restore();
            break;
        }
        case PE_IndicatorBranch: {
            painter->save();
            QColor lineColor(204, 204, 255);
            QPixmap openPixmap(":/images/jtreeopen.png");
            QPixmap closedPixmap(":/images/jtreeclosed.png");
            QRect pixmapRect(QPoint(0, 0), QSize(12, 12));
            pixmapRect.moveCenter(option->rect.center());
            pixmapRect.translate(2, 0);
            QPoint center = option->rect.center();

            painter->setPen(lineColor);
            painter->setBrush(Qt::NoBrush);

            if (option->state & State_Item) {
                painter->drawLine(center,
                                  QPoint(option->rect.right(), center.y()));

                painter->drawLine(center, QPoint(center.x(),
                                  option->rect.top()));

                if (option->state & State_Sibling) {
                    painter->drawLine(center, QPoint(center.x(),
                                      option->rect.bottom()));
                }

                if (option->state & State_Children)
                    if (option->state & State_Open)
                        painter->drawPixmap(pixmapRect.topLeft(), closedPixmap);
                    else
                        painter->drawPixmap(pixmapRect.topLeft(), openPixmap);
            } else if (option->state & State_Sibling) {
                painter->drawLine(center.x(), option->rect.top(), center.x(),
                                  option->rect.bottom());
            }

            painter->restore();
            break;
        }
        case PE_IndicatorViewItemCheck: {
            break;
        }
        case PE_FrameWindow: {
            painter->save();
            bool active = option->state & State_Active;

            painter->setPen(Qt::NoPen);
            painter->setBrush(active ? option->palette.color(QPalette::Midlight)
                                     : option->palette.color(QPalette::Mid));

            painter->drawRect(QRect(option->rect.topLeft(), option->rect.bottomLeft() + QPoint(5, 0)));
            painter->drawRect(QRect(option->rect.bottomLeft(), option->rect.bottomRight() + QPoint(0, -5)));
            painter->drawRect(QRect(option->rect.bottomRight() + QPoint(-5, 0), option->rect.topRight()));
            painter->drawRect(QRect(option->rect.topLeft(), option->rect.topRight() + QPoint(0, 4)));

            painter->setBrush(Qt::NoBrush);
            painter->setPen(option->palette.color(QPalette::Active, QPalette::WindowText));
            painter->drawLine(option->rect.topLeft() + QPoint(2, 14),
                              option->rect.bottomLeft() + QPoint(2, -14));

            painter->drawLine(option->rect.topRight() + QPoint(-2, 14),
                              option->rect.bottomRight() + QPoint(-2, -14));

            painter->drawLine(option->rect.topLeft() + QPoint(14, 2),
                              option->rect.topRight() + QPoint(-14, 2));

            painter->drawLine(option->rect.bottomLeft() + QPoint(14, -2),
                              option->rect.bottomRight() + QPoint(-14, -2));

            painter->setPen(active ? option->palette.color(QPalette::Light) :
                            option->palette.color(QPalette::Button));
            painter->drawLine(option->rect.topLeft() + QPoint(3, 15),
                              option->rect.bottomLeft() + QPoint(3, -13));

            painter->drawLine(option->rect.topRight() + QPoint(-1, 15),
                              option->rect.bottomRight() + QPoint(-1, -13));

            painter->drawLine(option->rect.topLeft() + QPoint(15, 3),
                              option->rect.topRight() + QPoint(-13, 3));

            painter->drawLine(option->rect.bottomLeft() + QPoint(15, -1),
                              option->rect.bottomRight() + QPoint(-13, -1));

            painter->restore();
            break;
        }
        case PE_IndicatorSpinUp: {
            const QStyleOptionSpinBox *spinner =
                qstyleoption_cast<const QStyleOptionSpinBox *>(option);
            int add = spinner->state & State_Sunken &&
                    spinner->activeSubControls & SC_SpinBoxUp ? 1 : 0;

            QPoint center = option->rect.center();
            painter->drawLine(center.x() + add, center.y() + 1 + add,
                              center.x() + 2 + add, center.y() + 1 + add);
            painter->drawPoint(center.x() + 1 + add, center.y() + add);
            break;
        }
        case PE_IndicatorSpinDown: {
            const QStyleOptionSpinBox *spinner =
                qstyleoption_cast<const QStyleOptionSpinBox *>(option);

            int add = spinner->state & State_Sunken &&
                      spinner->activeSubControls & SC_SpinBoxDown ? 1 : 0;
            QPoint center = option->rect.center();
            painter->drawLine(center.x() + add, center.y() + add,
                              center.x() + 2 + add, center.y() + add);
            painter->drawPoint(center.x() + 1 + add, center.y() + 1 + add);
            break;
        }
        case PE_FrameDockWidget: {
            drawPrimitive(PE_FrameWindow, option, painter, widget);
            break;
        }
        case PE_IndicatorToolBarHandle: {
            QPoint offset;
            bool horizontal = option->state & State_Horizontal;

            if (horizontal)
                offset = option->rect.topLeft();
            else
                offset = option->rect.topLeft();

            int iterations = horizontal ? option->rect.height() :
                                          option->rect.width();

            for (int i = 0; i < iterations; ++i) {
                painter->setPen(i % 2 ? Qt::white :
                    option->palette.color(QPalette::Mid));
                int add = i % 4;
                painter->drawPoint(offset + xySwitch(QPoint(add, i),
                                                     horizontal));
                painter->drawPoint(offset + xySwitch(QPoint(add + 4, i),
                                                     horizontal));
                if (add + 8 < 10)
                    painter->drawPoint(offset + xySwitch(QPoint(add + 8, i),
                                       horizontal));
            }

            break;
        }
        case PE_IndicatorToolBarSeparator: {
            break;
        }
        case PE_PanelButtonTool: {
            const QStyleOptionToolButton *button =
            qstyleoption_cast<const QStyleOptionToolButton *>(option);

            if (!button)  {
                painter->setPen(Qt::red);
                if (!(option->state & State_Enabled))
                    painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
                drawButtonBackground(option, painter, false);
                break;
            }

            if (button->state & State_MouseOver || button->state & State_On) {
                QStyleOptionButton bevel;
                bevel.state = button->state;
                bevel.rect = button->rect;
                bevel.palette = button->palette;

                drawButtonBackground(&bevel, painter, false);
            } else {
                painter->setPen(Qt::NoPen);
                painter->setBrush(button->palette.color(QPalette::Background));

                painter->drawRect(button->rect.adjusted(0, 0, -1, -1));
            }
            break;
        }
        case PE_FrameMenu: {
            painter->setPen(option->palette.color(QPalette::Midlight));
            painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
            break;
        }
        case PE_PanelButtonCommand: {
            const QStyleOptionButton *btn =
                qstyleoption_cast<const QStyleOptionButton *>(option);
            bool hover = (btn->state & State_Enabled) &&
                         (btn->state & State_MouseOver);
            bool sunken = btn->state & State_Sunken;
            bool isDefault = btn->features & QStyleOptionButton::DefaultButton;
            bool on = option->state & State_On;

            drawButtonBackground(option, painter, false);

            QRect rect = option->rect.adjusted(0, 0, -1, -1);
            if (hover && !sunken && !isDefault && !on) {
                drawButtonHoverFrame(painter, rect,
                    btn->palette.color(QPalette::Mid),
                    btn->palette.color(QPalette::Button));
            } else if (isDefault) {
                drawPrimitive(PE_FrameDefaultButton, option, painter, widget);
            }
            break;
        }
        case PE_FrameDefaultButton: {
            painter->setPen(option->palette.color(QPalette::Mid));
            QRect rect = option->rect.adjusted(0, 0, -1, -1);
            painter->drawRect(rect);
            painter->drawRect(rect.adjusted(1, 1, -1, -1));
            break;
        }
//! [0]
        case PE_IndicatorCheckBox: {
            painter->save();
            drawButtonBackground(option, painter, true);

            if (option->state & State_Enabled &&
                option->state & State_MouseOver &&
                !(option->state & State_Sunken)) {
                painter->setPen(option->palette.color(QPalette::Button));
                QRect rect = option->rect.adjusted(1, 1, -2, -2);
                painter->drawRect(rect);
                rect = rect.adjusted(1, 1, -1, -1);
                painter->drawRect(rect);
            }

            if (option->state & State_On) {
                QImage image(":/images/checkboxchecked.png");
                painter->drawImage(option->rect.topLeft(), image);
            }
            painter->restore();
            break;
//! [0]
        }
        case PE_IndicatorRadioButton: {
            painter->save();
            QBrush radioBrush = option->palette.button();

            if (!(option->state & State_Sunken) &&
                option->state & State_Enabled)
                radioBrush = gradientBrush(option->rect);

            painter->setBrush(radioBrush);
            if (option->state & State_Enabled)
                painter->setPen(option->palette.color(QPalette::Mid));
            else
                painter->setPen(option->palette.color(QPalette::Disabled,
                                                      QPalette::WindowText));
            painter->drawEllipse(option->rect.adjusted(0, 0, -1, -1));

            if (option->state & State_MouseOver &&
                option->state & State_Enabled &&
                !(option->state & State_Sunken)) {
                gradientBrush(option->rect);
                painter->setPen(option->palette.color(QPalette::Button));
                painter->setBrush(Qt::NoBrush);
                QRect rect = option->rect.adjusted(1, 1, -2, -2);
                painter->drawEllipse(rect);
                rect = rect.adjusted(1, 1, -1, -1);
                painter->drawEllipse(rect);
            }

            if (option->state & State_On) {
                painter->setBrush(option->palette.color(QPalette::Text));
                painter->setPen(Qt::NoPen);
                painter->drawEllipse(option->rect.adjusted(3, 3, -3, -3));
            }
            if (option->state & State_Sunken &&
                option->state & State_Enabled) {
                painter->setPen(option->palette.color(QPalette::Mid));
                painter->drawArc(option->rect.adjusted(1, 1, -2, -2), 80 * 16,
                                 100 * 16);
            }
            painter->restore();
            break;
        }
        case PE_FrameTabWidget: {
            painter->setPen(option->palette.color(QPalette::Midlight));
            painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
            painter->setPen(Qt::white);
            painter->drawRect(option->rect.adjusted(1, 1, -2, -2));
            break;
        }
        case PE_Frame:
        case PE_FrameLineEdit: {
            const QStyleOptionFrame *frame =
                qstyleoption_cast<const QStyleOptionFrame *>(option);
            const QStyleOptionFrameV2 frameV2(*frame);

            painter->setPen(frame->palette.color(QPalette::Mid));
            painter->drawRect(frameV2.rect.adjusted(0, 0, -2, -2));
            painter->setPen(Qt::white);
            painter->drawRect(frameV2.rect.adjusted(1, 1, -1, -1));
            painter->setPen(frameV2.palette.color(QPalette::Active,
                                                  QPalette::Background));
            painter->drawLine(frameV2.rect.bottomLeft(),
            frameV2.rect.bottomLeft() + QPoint(1, -1));
            painter->drawLine(frameV2.rect.topRight(),
            frameV2.rect.topRight() + QPoint(-1, 1));
            break;
        }
        case PE_FrameFocusRect: {
            painter->setPen(option->palette.color(QPalette::Light));
            painter->setBrush(Qt::NoBrush);
            QRect rect = option->rect;
            rect = rect.adjusted(0,0, -1, -1);
            painter->drawRect(rect);
            break;
        }
        default:
            QCommonStyle::drawPrimitive(element, option, painter, widget);
    }
    painter->restore();
}

//! [1]
void JavaStyle::drawButtonBackground(const QStyleOption *option,
                                     QPainter *painter, bool isCheckbox) const
{
    QBrush buttonBrush = option->palette.button();
    bool sunken = option->state & State_Sunken;
    bool disabled = !(option->state & State_Enabled);
    bool on = option->state & State_On;

    if (!sunken && !disabled && (!on || isCheckbox))
        buttonBrush = gradientBrush(option->rect);

        painter->fillRect(option->rect, buttonBrush);

        QRect rect = option->rect.adjusted(0, 0, -1, -1);

        if (disabled)
            painter->setPen(option->palette.color(QPalette::Disabled,
                                                  QPalette::WindowText));
        else
            painter->setPen(option->palette.color(QPalette::Mid));

        painter->drawRect(rect);

        if (sunken && !disabled) {
            drawSunkenButtonShadow(painter, rect,
                   option->palette.color(QPalette::Mid),
                   option->direction == Qt::RightToLeft);
    }
}
//! [1]

QBrush JavaStyle::gradientBrush(const QRect &rect) const
{
    QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
    gradient.setColorAt(1.0, QColor(188, 210, 230));
    gradient.setColorAt(0.3, Qt::white);
    gradient.setColorAt(0.0, QColor(223, 233, 243));

    return QBrush(gradient);
}

QRect JavaStyle::subElementRect(SubElement element,
                                const QStyleOption *option,
                                const QWidget *widget) const
{
    QRect rect;

    switch (element) {
        case SE_ToolBoxTabContents: {
            const QStyleOptionToolBox *box =
                qstyleoption_cast<const QStyleOptionToolBox *>(option);

            rect.moveTopLeft(box->rect.topLeft() + QPoint(0, 2));
            rect.setHeight(box->rect.height() - 4);
            rect.setWidth(box->fontMetrics.width(box->text) + 15);
            break;
        }
        case SE_ProgressBarLabel:
        case SE_ProgressBarGroove:
        case SE_ProgressBarContents: {
            rect = option->rect.adjusted(1, 1, -1, -1);
            break;
        }
        case SE_PushButtonFocusRect: {
            const QStyleOptionButton *btn =
                qstyleoption_cast<const QStyleOptionButton *>(option);

            rect = btn->fontMetrics.boundingRect(btn->text);
            rect = QRect(0, 0, btn->fontMetrics.width(btn->text),
                         rect.height());

            if (!btn->icon.isNull()) {
                rect.adjust(0, 0, btn->iconSize.width(), btn->iconSize.height()
                    > rect.height() ? btn->iconSize.height() - rect.height() : 0);
                rect.translate(-btn->iconSize.width(), 0);
                rect.adjust(-1, -1, 1, 1);
            }
            rect = QRect(int(ceil((btn->rect.width() - rect.width()) / 2.0)),
                         int(ceil((btn->rect.height() - rect.height()) / 2.0)),
                         rect.width() - 1, rect.height());
            rect.adjust(-1, 0, 1, 0);

            break;
        }
        default:
            rect = QCommonStyle::subElementRect(element, option, widget);
    }
    return rect;
}

int JavaStyle::pixelMetric(PixelMetric metric,
                           const QStyleOption* /* option */,
                           const QWidget* /*widget*/) const
{
    int value = 0;

    switch (metric) {
        case PM_ButtonShiftHorizontal:
        case PM_ButtonShiftVertical:
        case PM_TabBarTabShiftHorizontal:
        case PM_ButtonDefaultIndicator:
        case PM_TabBarTabShiftVertical:
            value = 0;
            break;
        case PM_TabBarBaseOverlap:
        case PM_DefaultFrameWidth:
            value = 2;
            break;
        case PM_TabBarTabVSpace:
            value = 4;
            break;
        case PM_ScrollBarExtent:
            value = 16;
            break;
        case PM_ScrollBarSliderMin:
            value = 26;
            break;
        case PM_SplitterWidth:
            value = 8;
            break;
        case PM_SliderThickness:
            value = 16;
            break;
        case PM_SliderControlThickness:
            value = 16;
            break;
        case PM_SliderTickmarkOffset:
            value = 10;
            break;
        case PM_SliderSpaceAvailable:
            break;
        case PM_MenuPanelWidth:
            value = 1;
            break;
        case PM_MenuVMargin:
            value = 2;
            break;
        case PM_MenuBarPanelWidth:
            value = 1;
            break;
        case PM_MenuBarItemSpacing:
            value = 0;
            break;
        case PM_MenuBarHMargin:
            value = 3;
            break;
        case PM_MenuBarVMargin:
            value = 0;
            break;
        case PM_ComboBoxFrameWidth:
            value = 1;
            break;
        case PM_MenuButtonIndicator:
            value = 15;
            break;
        case PM_ToolBarItemMargin:
            value = 3;
            break;
        case PM_ToolBarHandleExtent:
            value = 13;
            break;
        case PM_SpinBoxFrameWidth:
            value = 2;
            break;
        case PM_TitleBarHeight: {
            value = 21;
            break;
        case PM_MDIFrameWidth:
            value = 6;
            break;
        }
        case PM_DockWidgetFrameWidth: {
            value = 5;
            break;
        }
        default:
            value = QCommonStyle::pixelMetric(metric);
    }
    return value;
}


int JavaStyle::styleHint(StyleHint hint, const QStyleOption *option,
                         const QWidget *widget,
                         QStyleHintReturn *returnData) const
{
    int ret;

    switch (hint) {
        case SH_Table_GridLineColor: {
            ret = static_cast<int>(option->palette.color(QPalette::Mid).rgb());
            break;
        }
        case QStyle::SH_Menu_Scrollable:
            ret = 1;
            break;
        default:
            ret = QCommonStyle::styleHint(hint, option, widget, returnData);
    }
    return ret;
}

QPixmap JavaStyle::standardPixmap(StandardPixmap standardPixmap,
                  const QStyleOption *option,
                  const QWidget *widget) const
{
    QPixmap pixmap = QCommonStyle::standardPixmap(standardPixmap, option,
                                                  widget);

    QPixmap maximizePixmap(":/images/internalmaximize.png");
    QPixmap minimizePixmap(":/images/internalminimize.png");
    QPixmap closePixmap(":/images/internalclose.png");
    QPixmap internalPixmap(":/images/internalsystem.png");
    QPixmap internalCloseDownPixmap(":/images/internalclosedown.png");
    QPixmap minimizeDownPixmap(":/images/internalminimizedown.png");
    QPixmap maximizeDownPixmap(":/images/internalmaximizedown.png");
    QPixmap dirOpenPixmap(":/images/open24.png");
    QPixmap filePixmap(":/images/file.png");

    switch (standardPixmap) {
        case SP_DirLinkIcon:
        case SP_DirClosedIcon:
        case SP_DirIcon:
        case SP_DirOpenIcon: {
            pixmap = closePixmap;
            break;
        }
        case SP_FileIcon: {
            pixmap = filePixmap;
            break;
        }
        case SP_FileDialogBack: {
            pixmap = QPixmap(":/images/fileback.png");
            break;
        }
        case SP_FileDialogToParent: {
            pixmap = QPixmap(":/images/fileparent.png");
            break;
        }
        case SP_FileDialogNewFolder: {
            pixmap = QPixmap(":/images/open24.png");
            break;
        }
        case SP_FileDialogListView: {
            pixmap = QPixmap(":/images/filelist.png");
            break;
        }
        case SP_FileDialogDetailedView: {
            pixmap = QPixmap(":/images/filedetail.png");
            break;
        }
        case SP_MessageBoxInformation: {
            pixmap = QPixmap(":/images/information.png");
            break;
        }
        case SP_MessageBoxWarning: {
            pixmap = QPixmap(":/images/warning.png");
        }
        case SP_MessageBoxCritical: {
            pixmap = QPixmap(":/images/critical.png");
            break;
        }
        case SP_MessageBoxQuestion: {
            pixmap = QPixmap(":/images/question.png");
            break;
        }
        case SP_TitleBarNormalButton:
            pixmap = maximizePixmap;
            break;
        case SP_TitleBarCloseButton:
            pixmap = closePixmap;
            break;
        default:
            ;
    }

    return pixmap;
}

QSize JavaStyle::sizeFromContents(ContentsType type,
                                  const QStyleOption *option,
                                  const QSize &contentsSize,
                                  const QWidget *widget) const
{
    switch (type) {
        case CT_ComboBox: {
            return QSize(contentsSize.width() + 27, contentsSize.height());
        }
        case CT_Slider: {
            const QStyleOptionSlider *slider =
                qstyleoption_cast<const QStyleOptionSlider *>(option);
            if (slider->tickPosition == QSlider::TicksBelow) {
                return QSize(contentsSize.width(), contentsSize.height() + 15);
            } else {
                return contentsSize;
            }
        }
        case CT_MenuBarItem: {
            const QStyleOptionMenuItem *menuItem =
                qstyleoption_cast<const QStyleOptionMenuItem *>(option);
            QFontMetrics metrics(menuItem->font);
            QRect boundingRect = metrics.boundingRect(menuItem->text);
            int width = boundingRect.width() + 14;
            int height = boundingRect.height() + 3;
            if (height < 20)
                height = 20;

            return QSize(width, height);
        }
        case CT_MenuItem: {
            const QStyleOptionMenuItem *menuItem =
                qstyleoption_cast<const QStyleOptionMenuItem *>(option);
            QSize defaultSize =  QCommonStyle::sizeFromContents(type, option,
                contentsSize, widget);

            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator)
                return defaultSize;

            int width = 30;
            int height = 0;

            if (!menuItem->icon.isNull()) {
                width += 20;
                height += 20;
            }
            if (!menuItem->text.isEmpty()) {
                QFontMetrics metrics(menuItem->font);
                QString text = menuItem->text;
                text.remove(QLatin1Char('\t'));
                QRect textRect = metrics.boundingRect(text);
                width += textRect.width();
                if (height < textRect.height())
                    height += textRect.height();
            }
            if (menuItem->checkType != QStyleOptionMenuItem::NotCheckable) {
                width += 10;
                if (height < 10)
                    height = 10;
            }
            return QSize(width, height);
        }
        default:
            return QCommonStyle::sizeFromContents(type, option, contentsSize,
                                                  widget);
    }
}
