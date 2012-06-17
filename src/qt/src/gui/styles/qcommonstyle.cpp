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

#include "qcommonstyle.h"
#include "qcommonstyle_p.h"

#include <qfile.h>
#include <qapplication.h>
#include <qbitmap.h>
#include <qcache.h>
#include <qdockwidget.h>
#include <qdrawutil.h>
#include <qdialogbuttonbox.h>
#include <qformlayout.h>
#include <qgroupbox.h>
#include <qmath.h>
#include <qmenu.h>
#include <qpainter.h>
#include <qpaintengine.h>
#include <qpainterpath.h>
#include <qslider.h>
#include <qstyleoption.h>
#include <qtabbar.h>
#include <qtabwidget.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qrubberband.h>
#include <private/qcommonstylepixmaps_p.h>
#include <private/qmath_p.h>
#include <qdebug.h>
#include <qtextformat.h>
#include <qwizard.h>
#include <qtabbar.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qsettings.h>
#include <qpixmapcache.h>
#include <private/qguiplatformplugin_p.h>

#include <limits.h>

#ifndef QT_NO_ITEMVIEWS
#   include "private/qtextengine_p.h"
#endif

#ifdef Q_WS_X11
#   include <private/qt_x11_p.h>
#elif defined(Q_WS_MAC)
#   include <private/qt_cocoa_helpers_mac_p.h>
#endif

#include <private/qstylehelper_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QCommonStyle
    \brief The QCommonStyle class encapsulates the common Look and Feel of a GUI.

    \ingroup appearance

    This abstract class implements some of the widget's look and feel
    that is common to all GUI styles provided and shipped as part of
    Qt.

    Since QCommonStyle inherits QStyle, all of its functions are fully documented
    in the QStyle documentation.
    \omit
    , although the
    extra functions that QCommonStyle provides, e.g.
    drawComplexControl(), drawControl(), drawPrimitive(),
    hitTestComplexControl(), subControlRect(), sizeFromContents(), and
    subElementRect() are documented here.
    \endomit

    \sa QStyle, QMotifStyle, QWindowsStyle
*/

/*!
    Constructs a QCommonStyle.
*/
QCommonStyle::QCommonStyle()
    : QStyle(*new QCommonStylePrivate)
{ }

/*! \internal
*/
QCommonStyle::QCommonStyle(QCommonStylePrivate &dd)
    : QStyle(dd)
{ }

/*!
    Destroys the style.
*/
QCommonStyle::~QCommonStyle()
{ }


/*!
    \reimp
*/
void QCommonStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                                 const QWidget *widget) const
{
    Q_D(const QCommonStyle);
    switch (pe) {
    case PE_FrameButtonBevel:
    case PE_FrameButtonTool:
        qDrawShadeRect(p, opt->rect, opt->palette,
                       opt->state & (State_Sunken | State_On), 1, 0);
        break;
    case PE_PanelButtonCommand:
    case PE_PanelButtonBevel:
    case PE_PanelButtonTool:
    case PE_IndicatorButtonDropDown:
        qDrawShadePanel(p, opt->rect, opt->palette,
                        opt->state & (State_Sunken | State_On), 1,
                        &opt->palette.brush(QPalette::Button));
        break;
    case PE_IndicatorViewItemCheck:
        proxy()->drawPrimitive(PE_IndicatorCheckBox, opt, p, widget);
        break;
    case PE_IndicatorCheckBox:
        if (opt->state & State_NoChange) {
            p->setPen(opt->palette.foreground().color());
            p->fillRect(opt->rect, opt->palette.brush(QPalette::Button));
            p->drawRect(opt->rect);
            p->drawLine(opt->rect.topLeft(), opt->rect.bottomRight());
        } else {
            qDrawShadePanel(p, opt->rect.x(), opt->rect.y(), opt->rect.width(), opt->rect.height(),
                            opt->palette, opt->state & (State_Sunken | State_On), 1,
                            &opt->palette.brush(QPalette::Button));
        }
        break;
    case PE_IndicatorRadioButton: {
        QRect ir = opt->rect;
        p->setPen(opt->palette.dark().color());
        p->drawArc(opt->rect, 0, 5760);
        if (opt->state & (State_Sunken | State_On)) {
            ir.adjust(2, 2, -2, -2);
            p->setBrush(opt->palette.foreground());
            p->drawEllipse(ir);
        }
        break; }
    case PE_FrameFocusRect:
        if (const QStyleOptionFocusRect *fropt = qstyleoption_cast<const QStyleOptionFocusRect *>(opt)) {
            QColor bg = fropt->backgroundColor;
            QPen oldPen = p->pen();
            if (bg.isValid()) {
                int h, s, v;
                bg.getHsv(&h, &s, &v);
                if (v >= 128)
                    p->setPen(Qt::black);
                else
                    p->setPen(Qt::white);
            } else {
                p->setPen(opt->palette.foreground().color());
            }
            QRect focusRect = opt->rect.adjusted(1, 1, -1, -1);
            p->drawRect(focusRect.adjusted(0, 0, -1, -1)); //draw pen inclusive
            p->setPen(oldPen);
        }
        break;
    case PE_IndicatorMenuCheckMark: {
        const int markW = opt->rect.width() > 7 ? 7 : opt->rect.width();
        const int markH = markW;
        int posX = opt->rect.x() + (opt->rect.width() - markW)/2 + 1;
        int posY = opt->rect.y() + (opt->rect.height() - markH)/2;

        QVector<QLineF> a;
        a.reserve(markH);

        int i, xx, yy;
        xx = posX;
        yy = 3 + posY;
        for (i = 0; i < markW/2; ++i) {
            a << QLineF(xx, yy, xx, yy + 2);
            ++xx;
            ++yy;
        }
        yy -= 2;
        for (; i < markH; ++i) {
            a << QLineF(xx, yy, xx, yy + 2);
            ++xx;
            --yy;
        }
        if (!(opt->state & State_Enabled) && !(opt->state & State_On)) {
            p->save();
            p->translate(1, 1);
            p->setPen(opt->palette.light().color());
            p->drawLines(a);
            p->restore();
        }
        p->setPen((opt->state & State_On) ? opt->palette.highlightedText().color() : opt->palette.text().color());
        p->drawLines(a);
        break; }
    case PE_Frame:
    case PE_FrameMenu:
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            if (pe == PE_FrameMenu || (frame->state & State_Sunken) || (frame->state & State_Raised)) {
                qDrawShadePanel(p, frame->rect, frame->palette, frame->state & State_Sunken,
                                frame->lineWidth);
            } else {
                qDrawPlainRect(p, frame->rect, frame->palette.foreground().color(), frame->lineWidth);
            }
        }
        break;
#ifndef QT_NO_TOOLBAR
    case PE_PanelMenuBar:
        if (widget && qobject_cast<QToolBar *>(widget->parentWidget()))
            break;
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(opt)){
            qDrawShadePanel(p, frame->rect, frame->palette, false, frame->lineWidth,
                            &frame->palette.brush(QPalette::Button));

        }
        else if (const QStyleOptionToolBar *frame = qstyleoption_cast<const QStyleOptionToolBar *>(opt)){
            qDrawShadePanel(p, frame->rect, frame->palette, false, frame->lineWidth,
                            &frame->palette.brush(QPalette::Button));
        }

        break;
   case PE_PanelMenu:
        break;
    case PE_PanelToolBar:
       break;
#endif // QT_NO_TOOLBAR
#ifndef QT_NO_PROGRESSBAR
    case PE_IndicatorProgressChunk:
        {
            bool vertical = false;
            if (const QStyleOptionProgressBarV2 *pb2 = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(opt))
                vertical = (pb2->orientation == Qt::Vertical);
            if (!vertical) {
                p->fillRect(opt->rect.x(), opt->rect.y() + 3, opt->rect.width() -2, opt->rect.height() - 6,
                            opt->palette.brush(QPalette::Highlight));
            } else {
                p->fillRect(opt->rect.x() + 2, opt->rect.y(), opt->rect.width() -6, opt->rect.height() - 2,
                            opt->palette.brush(QPalette::Highlight));
            }
        }
        break;
#endif // QT_NO_PROGRESSBAR
#ifdef QT3_SUPPORT
    case PE_Q3CheckListController:
#ifndef QT_NO_IMAGEFORMAT_XPM
        p->drawPixmap(opt->rect.topLeft(), QPixmap(check_list_controller_xpm));
#endif
        break;
    case PE_Q3CheckListExclusiveIndicator:
        if (const QStyleOptionQ3ListView *lv = qstyleoption_cast<const QStyleOptionQ3ListView *>(opt)) {
            if (lv->items.isEmpty())
                return;
            int x = lv->rect.x(),
                y = lv->rect.y();
#define INTARRLEN(x) sizeof(x)/(sizeof(int)*2)
            static const int pts1[] = {                // dark lines
                1,9, 1,8, 0,7, 0,4, 1,3, 1,2, 2,1, 3,1, 4,0, 7,0, 8,1, 9,1 };
            static const int pts2[] = {                // black lines
                2,8, 1,7, 1,4, 2,3, 2,2, 3,2, 4,1, 7,1, 8,2, 9,2 };
            static const int pts3[] = {                // background lines
                2,9, 3,9, 4,10, 7,10, 8,9, 9,9, 9,8, 10,7, 10,4, 9,3 };
            static const int pts4[] = {                // white lines
                2,10, 3,10, 4,11, 7,11, 8,10, 9,10, 10,9, 10,8, 11,7,
                11,4, 10,3, 10,2 };
            // static const int pts5[] = {                // inner fill
            //    4,2, 7,2, 9,4, 9,7, 7,9, 4,9, 2,7, 2,4 };
            //QPolygon a;

            if (lv->state & State_Enabled)
                p->setPen(lv->palette.text().color());
            else
                p->setPen(QPen(lv->viewportPalette.color(QPalette::Disabled, QPalette::Text)));
            QPolygon a(INTARRLEN(pts1), pts1);
            a.translate(x, y);
            //p->setPen(pal.dark());
            p->drawPolyline(a);
            a.setPoints(INTARRLEN(pts2), pts2);
            a.translate(x, y);
            p->drawPolyline(a);
            a.setPoints(INTARRLEN(pts3), pts3);
            a.translate(x, y);
            //                p->setPen(black);
            p->drawPolyline(a);
            a.setPoints(INTARRLEN(pts4), pts4);
            a.translate(x, y);
            //                        p->setPen(blue);
            p->drawPolyline(a);
            //                a.setPoints(INTARRLEN(pts5), pts5);
            //                a.translate(x, y);
            //        QColor fillColor = isDown() ? g.background() : g.base();
            //        p->setPen(fillColor);
            //        p->setBrush(fillColor);
            //        p->drawPolygon(a);
            if (opt->state & State_On) {
                p->setPen(Qt::NoPen);
                p->setBrush(opt->palette.text());
                p->drawRect(x + 5, y + 4, 2, 4);
                p->drawRect(x + 4, y + 5, 4, 2);
            }
#undef INTARRLEN
        }
        break;
    case PE_Q3CheckListIndicator:
        if (const QStyleOptionQ3ListView *lv = qstyleoption_cast<const QStyleOptionQ3ListView *>(opt)) {
            if(lv->items.isEmpty())
                break;
            QStyleOptionQ3ListViewItem item = lv->items.at(0);
            int x = lv->rect.x(),
                y = lv->rect.y(),
                w = lv->rect.width(),
                h = lv->rect.width(),
             marg = lv->itemMargin;

            if (lv->state & State_Enabled)
                p->setPen(QPen(lv->palette.text().color(), 2));
            else
                p->setPen(QPen(lv->viewportPalette.color(QPalette::Disabled, QPalette::Text), 2));
            if (opt->state & State_Selected && !lv->rootIsDecorated
                && !(item.features & QStyleOptionQ3ListViewItem::ParentControl)) {
                p->fillRect(0, 0, x + marg + w + 4, item.height,
                            lv->palette.brush(QPalette::Highlight));
                if (item.state & State_Enabled)
                    p->setPen(QPen(lv->palette.highlightedText().color(), 2));
            }

            if (lv->state & State_NoChange)
                p->setBrush(lv->palette.brush(QPalette::Button));
            p->drawRect(x + marg, y + 2, w - 4, h - 4);
            /////////////////////
                ++x;
                ++y;
                if (lv->state & State_On || lv->state & State_NoChange) {
                    QLineF lines[7];
                    int i,
                        xx = x + 1 + marg,
                        yy = y + 5;
                    for (i = 0; i < 3; ++i) {
                        lines[i] = QLineF(xx, yy, xx, yy + 2);
                        ++xx;
                        ++yy;
                    }
                    yy -= 2;
                    for (i = 3; i < 7; ++i) {
                        lines[i] = QLineF(xx, yy, xx, yy + 2);
                        ++xx;
                        --yy;
                    }
                    p->drawLines(lines, 7);
                }
        }
        break;
#endif // QT3_SUPPORT
    case PE_IndicatorBranch: {
        int mid_h = opt->rect.x() + opt->rect.width() / 2;
        int mid_v = opt->rect.y() + opt->rect.height() / 2;
        int bef_h = mid_h;
        int bef_v = mid_v;
        int aft_h = mid_h;
        int aft_v = mid_v;
#ifndef QT_NO_IMAGEFORMAT_XPM
        static const int decoration_size = 9;
        static QPixmap open(tree_branch_open_xpm);
        static QPixmap closed(tree_branch_closed_xpm);
        if (opt->state & State_Children) {
            int delta = decoration_size / 2;
            bef_h -= delta;
            bef_v -= delta;
            aft_h += delta;
            aft_v += delta;
            p->drawPixmap(bef_h, bef_v, opt->state & State_Open ? open : closed);
        }
#endif // QT_NO_IMAGEFORMAT_XPM
        if (opt->state & State_Item) {
            if (opt->direction == Qt::RightToLeft)
                p->drawLine(opt->rect.left(), mid_v, bef_h, mid_v);
            else
                p->drawLine(aft_h, mid_v, opt->rect.right(), mid_v);
        }
        if (opt->state & State_Sibling)
            p->drawLine(mid_h, aft_v, mid_h, opt->rect.bottom());
        if (opt->state & (State_Open | State_Children | State_Item | State_Sibling))
            p->drawLine(mid_h, opt->rect.y(), mid_h, bef_v);
        break; }
#ifdef QT3_SUPPORT
    case PE_Q3Separator:
        qDrawShadeLine(p, opt->rect.left(), opt->rect.top(), opt->rect.right(), opt->rect.bottom(),
                       opt->palette, opt->state & State_Sunken, 1, 0);
        break;
#endif // QT3_SUPPORT
    case PE_FrameStatusBarItem:
        qDrawShadeRect(p, opt->rect, opt->palette, true, 1, 0, 0);
        break;
    case PE_IndicatorHeaderArrow:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
            QPen oldPen = p->pen();
            if (header->sortIndicator & QStyleOptionHeader::SortUp) {
                QPolygon pa(3);
                p->setPen(QPen(opt->palette.light(), 0));
                p->drawLine(opt->rect.x() + opt->rect.width(), opt->rect.y(),
                            opt->rect.x() + opt->rect.width() / 2, opt->rect.y() + opt->rect.height());
                p->setPen(QPen(opt->palette.dark(), 0));
                pa.setPoint(0, opt->rect.x() + opt->rect.width() / 2, opt->rect.y() + opt->rect.height());
                pa.setPoint(1, opt->rect.x(), opt->rect.y());
                pa.setPoint(2, opt->rect.x() + opt->rect.width(), opt->rect.y());
                p->drawPolyline(pa);
            } else if (header->sortIndicator & QStyleOptionHeader::SortDown) {
                QPolygon pa(3);
                p->setPen(QPen(opt->palette.light(), 0));
                pa.setPoint(0, opt->rect.x(), opt->rect.y() + opt->rect.height());
                pa.setPoint(1, opt->rect.x() + opt->rect.width(), opt->rect.y() + opt->rect.height());
                pa.setPoint(2, opt->rect.x() + opt->rect.width() / 2, opt->rect.y());
                p->drawPolyline(pa);
                p->setPen(QPen(opt->palette.dark(), 0));
                p->drawLine(opt->rect.x(), opt->rect.y() + opt->rect.height(),
                            opt->rect.x() + opt->rect.width() / 2, opt->rect.y());
            }
            p->setPen(oldPen);
        }
        break;
#ifndef QT_NO_TABBAR
    case PE_FrameTabBarBase:
        if (const QStyleOptionTabBarBase *tbb
                = qstyleoption_cast<const QStyleOptionTabBarBase *>(opt)) {
            p->save();
            switch (tbb->shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                p->setPen(QPen(tbb->palette.light(), 0));
                p->drawLine(tbb->rect.topLeft(), tbb->rect.topRight());
                break;
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                p->setPen(QPen(tbb->palette.light(), 0));
                p->drawLine(tbb->rect.topLeft(), tbb->rect.bottomLeft());
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                p->setPen(QPen(tbb->palette.shadow(), 0));
                p->drawLine(tbb->rect.left(), tbb->rect.bottom(),
                            tbb->rect.right(), tbb->rect.bottom());
                p->setPen(QPen(tbb->palette.dark(), 0));
                p->drawLine(tbb->rect.left(), tbb->rect.bottom() - 1,
                            tbb->rect.right() - 1, tbb->rect.bottom() - 1);
                break;
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                p->setPen(QPen(tbb->palette.dark(), 0));
                p->drawLine(tbb->rect.topRight(), tbb->rect.bottomRight());
                break;
            }
            p->restore();
        }
        break;
    case PE_IndicatorTabClose: {
        if (d->tabBarcloseButtonIcon.isNull()) {
            d->tabBarcloseButtonIcon.addPixmap(QPixmap(
                        QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-closetab-16.png")),
                        QIcon::Normal, QIcon::Off);
            d->tabBarcloseButtonIcon.addPixmap(QPixmap(
                        QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-closetab-down-16.png")),
                        QIcon::Normal, QIcon::On);
            d->tabBarcloseButtonIcon.addPixmap(QPixmap(
                        QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-closetab-hover-16.png")),
                        QIcon::Active, QIcon::Off);
        }

        int size = proxy()->pixelMetric(QStyle::PM_SmallIconSize);
        QIcon::Mode mode = opt->state & State_Enabled ?
                            (opt->state & State_Raised ? QIcon::Active : QIcon::Normal)
                            : QIcon::Disabled;
        if (!(opt->state & State_Raised)
            && !(opt->state & State_Sunken)
            && !(opt->state & QStyle::State_Selected))
            mode = QIcon::Disabled;

        QIcon::State state = opt->state & State_Sunken ? QIcon::On : QIcon::Off;
        QPixmap pixmap = d->tabBarcloseButtonIcon.pixmap(size, mode, state);
        proxy()->drawItemPixmap(p, opt->rect, Qt::AlignCenter, pixmap);
        break;
    }
#endif // QT_NO_TABBAR
    case PE_FrameTabWidget:
    case PE_FrameWindow:
        qDrawWinPanel(p, opt->rect, opt->palette, false, 0);
        break;
    case PE_FrameLineEdit:
        proxy()->drawPrimitive(PE_Frame, opt, p, widget);
        break;
#ifndef QT_NO_GROUPBOX
    case PE_FrameGroupBox:
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            const QStyleOptionFrameV2 *frame2 = qstyleoption_cast<const QStyleOptionFrameV2 *>(opt);
            if (frame2 && (frame2->features & QStyleOptionFrameV2::Flat)) {
                QRect fr = frame->rect;
                QPoint p1(fr.x(), fr.y() + 1);
                QPoint p2(fr.x() + fr.width(), p1.y());
                qDrawShadeLine(p, p1, p2, frame->palette, true,
                               frame->lineWidth, frame->midLineWidth);
            } else {
                qDrawShadeRect(p, frame->rect.x(), frame->rect.y(), frame->rect.width(),
                               frame->rect.height(), frame->palette, true,
                               frame->lineWidth, frame->midLineWidth);
            }
        }
        break;
#endif // QT_NO_GROUPBOX
#ifndef QT_NO_DOCKWIDGET
    case PE_FrameDockWidget:
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            int lw = frame->lineWidth;
            if (lw <= 0)
                lw = proxy()->pixelMetric(PM_DockWidgetFrameWidth);

            qDrawShadePanel(p, frame->rect, frame->palette, false, lw);
        }
        break;
#endif // QT_NO_DOCKWIDGET
#ifndef QT_NO_TOOLBAR
    case PE_IndicatorToolBarHandle:
        p->save();
        p->translate(opt->rect.x(), opt->rect.y());
        if (opt->state & State_Horizontal) {
            int x = opt->rect.width() / 3;
            if (opt->direction == Qt::RightToLeft)
                x -= 2;
            if (opt->rect.height() > 4) {
                qDrawShadePanel(p, x, 2, 3, opt->rect.height() - 4,
                                opt->palette, false, 1, 0);
                qDrawShadePanel(p, x+3, 2, 3, opt->rect.height() - 4,
                                opt->palette, false, 1, 0);
            }
        } else {
            if (opt->rect.width() > 4) {
                int y = opt->rect.height() / 3;
                qDrawShadePanel(p, 2, y, opt->rect.width() - 4, 3,
                                opt->palette, false, 1, 0);
                qDrawShadePanel(p, 2, y+3, opt->rect.width() - 4, 3,
                                opt->palette, false, 1, 0);
            }
        }
        p->restore();
        break;
    case PE_Q3DockWindowSeparator:
        proxy()->drawPrimitive(PE_IndicatorToolBarSeparator, opt, p, widget);
        break;
    case PE_IndicatorToolBarSeparator:
        {
            QPoint p1, p2;
            if (opt->state & State_Horizontal) {
                p1 = QPoint(opt->rect.width()/2, 0);
                p2 = QPoint(p1.x(), opt->rect.height());
            } else {
                p1 = QPoint(0, opt->rect.height()/2);
                p2 = QPoint(opt->rect.width(), p1.y());
            }
            qDrawShadeLine(p, p1, p2, opt->palette, 1, 1, 0);
            break;
        }
#endif // QT_NO_TOOLBAR
#ifndef QT_NO_SPINBOX
    case PE_IndicatorSpinPlus:
    case PE_IndicatorSpinMinus: {
        QRect r = opt->rect;
        int fw = proxy()->pixelMetric(PM_DefaultFrameWidth, opt, widget);
        QRect br = r.adjusted(fw, fw, -fw, -fw);

        int offset = (opt->state & State_Sunken) ? 1 : 0;
        int step = (br.width() + 4) / 5;
        p->fillRect(br.x() + offset, br.y() + offset +br.height() / 2 - step / 2,
                    br.width(), step,
                    opt->palette.buttonText());
        if (pe == PE_IndicatorSpinPlus)
            p->fillRect(br.x() + br.width() / 2 - step / 2 + offset, br.y() + offset,
                        step, br.height(),
                        opt->palette.buttonText());

        break; }
    case PE_IndicatorSpinUp:
    case PE_IndicatorSpinDown: {
        QRect r = opt->rect;
        int fw = proxy()->pixelMetric(PM_DefaultFrameWidth, opt, widget);
        // QRect br = r.adjusted(fw, fw, -fw, -fw);
        int x = r.x(), y = r.y(), w = r.width(), h = r.height();
        int sw = w-4;
        if (sw < 3)
            break;
        else if (!(sw & 1))
            sw--;
        sw -= (sw / 7) * 2;        // Empty border
        int sh = sw/2 + 2;      // Must have empty row at foot of arrow

        int sx = x + w / 2 - sw / 2;
        int sy = y + h / 2 - sh / 2;

        if (pe == PE_IndicatorSpinUp && fw)
            --sy;

        QPolygon a;
        if (pe == PE_IndicatorSpinDown)
            a.setPoints(3, 0, 1,  sw-1, 1,  sh-2, sh-1);
        else
            a.setPoints(3, 0, sh-1,  sw-1, sh-1,  sh-2, 1);
        int bsx = 0;
        int bsy = 0;
        if (opt->state & State_Sunken) {
            bsx = proxy()->pixelMetric(PM_ButtonShiftHorizontal);
            bsy = proxy()->pixelMetric(PM_ButtonShiftVertical);
        }
        p->save();
        p->translate(sx + bsx, sy + bsy);
        p->setPen(opt->palette.buttonText().color());
        p->setBrush(opt->palette.buttonText());
        p->drawPolygon(a);
        p->restore();
        break; }
#endif // QT_NO_SPINBOX
    case PE_PanelTipLabel: {
        QBrush oldBrush = p->brush();
        QPen oldPen = p->pen();
        p->setPen(QPen(opt->palette.toolTipText(), 0));
        p->setBrush(opt->palette.toolTipBase());
        p->drawRect(opt->rect.adjusted(0, 0, -1, -1));
        p->setPen(oldPen);
        p->setBrush(oldBrush);
        break;
    }
#ifndef QT_NO_TABBAR
    case PE_IndicatorTabTear:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            bool rtl = tab->direction == Qt::RightToLeft;
            QRect rect = tab->rect;
            QPainterPath path;

            rect.setTop(rect.top() + ((tab->state & State_Selected) ? 1 : 3));
            rect.setBottom(rect.bottom() - ((tab->state & State_Selected) ? 0 : 2));

            path.moveTo(QPoint(rtl ? rect.right() : rect.left(), rect.top()));
            int count = 4;
            for(int jags = 1; jags <= count; ++jags, rtl = !rtl)
                path.lineTo(QPoint(rtl ? rect.left() : rect.right(), rect.top() + jags * rect.height()/count));

            p->setPen(QPen(tab->palette.light(), qreal(.8)));
            p->setBrush(tab->palette.background());
            p->setRenderHint(QPainter::Antialiasing);
            p->drawPath(path);
        }
        break;
#endif // QT_NO_TABBAR
#ifndef QT_NO_LINEEDIT
    case PE_PanelLineEdit:
        if (const QStyleOptionFrame *panel = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            p->fillRect(panel->rect.adjusted(panel->lineWidth, panel->lineWidth, -panel->lineWidth, -panel->lineWidth),
                        panel->palette.brush(QPalette::Base));

            if (panel->lineWidth > 0)
                proxy()->drawPrimitive(PE_FrameLineEdit, panel, p, widget);
        }
        break;
#endif // QT_NO_LINEEDIT
#ifndef QT_NO_COLUMNVIEW
    case PE_IndicatorColumnViewArrow: {
    if (const QStyleOptionViewItem *viewOpt = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
        bool reverse = (viewOpt->direction == Qt::RightToLeft);
        p->save();
        QPainterPath path;
        int x = viewOpt->rect.x() + 1;
        int offset = (viewOpt->rect.height() / 3);
        int height = (viewOpt->rect.height()) - offset * 2;
        if (height % 2 == 1)
            --height;
        int x2 = x + height - 1;
        if (reverse) {
            x = viewOpt->rect.x() + viewOpt->rect.width() - 1;
            x2 = x - height + 1;
        }
        path.moveTo(x, viewOpt->rect.y() + offset);
        path.lineTo(x, viewOpt->rect.y() + offset + height);
        path.lineTo(x2, viewOpt->rect.y() + offset+height/2);
        path.closeSubpath();
        if (viewOpt->state & QStyle::State_Selected ) {
            if (viewOpt->showDecorationSelected) {
                QColor color = viewOpt->palette.color(QPalette::Active, QPalette::HighlightedText);
                p->setPen(color);
                p->setBrush(color);
            } else {
                QColor color = viewOpt->palette.color(QPalette::Active, QPalette::WindowText);
                p->setPen(color);
                p->setBrush(color);
            }

        } else {
            QColor color = viewOpt->palette.color(QPalette::Active, QPalette::Mid);
            p->setPen(color);
            p->setBrush(color);
        }
        p->drawPath(path);

        // draw the vertical and top triangle line
        if (!(viewOpt->state & QStyle::State_Selected)) {
            QPainterPath lines;
            lines.moveTo(x, viewOpt->rect.y() + offset);
            lines.lineTo(x, viewOpt->rect.y() + offset + height);
            lines.moveTo(x, viewOpt->rect.y() + offset);
            lines.lineTo(x2, viewOpt->rect.y() + offset+height/2);
            QColor color = viewOpt->palette.color(QPalette::Active, QPalette::Dark);
            p->setPen(color);
            p->drawPath(lines);
        }
        p->restore();
    }
    break; }
#endif //QT_NO_COLUMNVIEW
    case PE_IndicatorItemViewItemDrop: {
        QRect rect = opt->rect;
        if (opt->rect.height() == 0)
            p->drawLine(rect.topLeft(), rect.topRight());
        else
            p->drawRect(rect);
        break; }
#ifndef QT_NO_ITEMVIEWS
    case PE_PanelItemViewRow:
        if (const QStyleOptionViewItemV4 *vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(opt)) {
            QPalette::ColorGroup cg = (widget ? widget->isEnabled() : (vopt->state & QStyle::State_Enabled))
                                      ? QPalette::Normal : QPalette::Disabled;
            if (cg == QPalette::Normal && !(vopt->state & QStyle::State_Active))
                cg = QPalette::Inactive;

            if ((vopt->state & QStyle::State_Selected) &&  proxy()->styleHint(QStyle::SH_ItemView_ShowDecorationSelected, opt, widget))
                p->fillRect(vopt->rect, vopt->palette.brush(cg, QPalette::Highlight));
            else if (vopt->features & QStyleOptionViewItemV2::Alternate)
                p->fillRect(vopt->rect, vopt->palette.brush(cg, QPalette::AlternateBase));
        }
        break;
    case PE_PanelItemViewItem:
        if (const QStyleOptionViewItemV4 *vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(opt)) {
            QPalette::ColorGroup cg = (widget ? widget->isEnabled() : (vopt->state & QStyle::State_Enabled))
                                      ? QPalette::Normal : QPalette::Disabled;
            if (cg == QPalette::Normal && !(vopt->state & QStyle::State_Active))
                cg = QPalette::Inactive;

            if (vopt->showDecorationSelected && (vopt->state & QStyle::State_Selected)) {
                p->fillRect(vopt->rect, vopt->palette.brush(cg, QPalette::Highlight));
            } else {
                if (vopt->backgroundBrush.style() != Qt::NoBrush) {
                    QPointF oldBO = p->brushOrigin();
                    p->setBrushOrigin(vopt->rect.topLeft());
                    p->fillRect(vopt->rect, vopt->backgroundBrush);
                    p->setBrushOrigin(oldBO);
                }

                if (vopt->state & QStyle::State_Selected) {
                    QRect textRect = subElementRect(QStyle::SE_ItemViewItemText,  opt, widget);
                    p->fillRect(textRect, vopt->palette.brush(cg, QPalette::Highlight));
                }
            }
        }
        break;
#endif //QT_NO_ITEMVIEWS
    case PE_PanelScrollAreaCorner: {
        const QBrush brush(opt->palette.brush(QPalette::Window));
        p->fillRect(opt->rect, brush);
        } break;
    default:
        break;
    }
}

#ifndef QT_NO_TOOLBUTTON
static void drawArrow(const QStyle *style, const QStyleOptionToolButton *toolbutton,
                      const QRect &rect, QPainter *painter, const QWidget *widget = 0)
{
    QStyle::PrimitiveElement pe;
    switch (toolbutton->arrowType) {
    case Qt::LeftArrow:
        pe = QStyle::PE_IndicatorArrowLeft;
        break;
    case Qt::RightArrow:
        pe = QStyle::PE_IndicatorArrowRight;
        break;
    case Qt::UpArrow:
        pe = QStyle::PE_IndicatorArrowUp;
        break;
    case Qt::DownArrow:
        pe = QStyle::PE_IndicatorArrowDown;
        break;
    default:
        return;
    }
    QStyleOption arrowOpt;
    arrowOpt.rect = rect;
    arrowOpt.palette = toolbutton->palette;
    arrowOpt.state = toolbutton->state;
    style->drawPrimitive(pe, &arrowOpt, painter, widget);
}
#endif // QT_NO_TOOLBUTTON

#ifndef QT_NO_ITEMVIEWS

QSize QCommonStylePrivate::viewItemSize(const QStyleOptionViewItemV4 *option, int role) const
{
    const QWidget *widget = option->widget;
    switch (role) {
    case Qt::CheckStateRole:
        if (option->features & QStyleOptionViewItemV2::HasCheckIndicator)
            return QSize(proxyStyle->pixelMetric(QStyle::PM_IndicatorWidth, option, widget),
                         proxyStyle->pixelMetric(QStyle::PM_IndicatorHeight, option, widget));
        break;
    case Qt::DisplayRole:
        if (option->features & QStyleOptionViewItemV2::HasDisplay) {
            QTextOption textOption;
            textOption.setWrapMode(QTextOption::WordWrap);
            QTextLayout textLayout;
            textLayout.setTextOption(textOption);
            textLayout.setFont(option->font);
            textLayout.setText(option->text);
            const bool wrapText = option->features & QStyleOptionViewItemV2::WrapText;
            const int textMargin = proxyStyle->pixelMetric(QStyle::PM_FocusFrameHMargin, option, widget) + 1;
            QRect bounds = option->rect;
            switch (option->decorationPosition) {
            case QStyleOptionViewItem::Left:
            case QStyleOptionViewItem::Right:
                bounds.setWidth(wrapText && bounds.isValid() ? bounds.width() - 2 * textMargin : QFIXED_MAX);
                break;
            case QStyleOptionViewItem::Top:
            case QStyleOptionViewItem::Bottom:
                bounds.setWidth(wrapText ? option->decorationSize.width() : QFIXED_MAX);
                break;
            default:
                break;
            }

            qreal height = 0, widthUsed = 0;
            textLayout.beginLayout();
            while (true) {
                QTextLine line = textLayout.createLine();
                if (!line.isValid())
                    break;
                line.setLineWidth(bounds.width());
                line.setPosition(QPointF(0, height));
                height += line.height();
                widthUsed = qMax(widthUsed, line.naturalTextWidth());
            }
            textLayout.endLayout();
            const QSize size(qCeil(widthUsed), qCeil(height));
            return QSize(size.width() + 2 * textMargin, size.height());
        }
        break;
    case Qt::DecorationRole:
        if (option->features & QStyleOptionViewItemV2::HasDecoration) {
            return option->decorationSize;
        }
        break;
    default:
        break;
    }

    return QSize(0, 0);
}

static QSizeF viewItemTextLayout(QTextLayout &textLayout, int lineWidth)
{
    qreal height = 0;
    qreal widthUsed = 0;
    textLayout.beginLayout();
    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    textLayout.endLayout();
    return QSizeF(widthUsed, height);
}


void QCommonStylePrivate::viewItemDrawText(QPainter *p, const QStyleOptionViewItemV4 *option, const QRect &rect) const
{
    const QWidget *widget = option->widget;
    const int textMargin = proxyStyle->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, widget) + 1;

    QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0); // remove width padding
    const bool wrapText = option->features & QStyleOptionViewItemV2::WrapText;
    QTextOption textOption;
    textOption.setWrapMode(wrapText ? QTextOption::WordWrap : QTextOption::ManualWrap);
    textOption.setTextDirection(option->direction);
    textOption.setAlignment(QStyle::visualAlignment(option->direction, option->displayAlignment));
    QTextLayout textLayout;
    textLayout.setTextOption(textOption);
    textLayout.setFont(option->font);
    textLayout.setText(option->text);

    viewItemTextLayout(textLayout, textRect.width());

    QString elidedText;
    qreal height = 0;
    qreal width = 0;
    int elidedIndex = -1;
    const int lineCount = textLayout.lineCount();
    for (int j = 0; j < lineCount; ++j) {
        const QTextLine line = textLayout.lineAt(j);
        if (j + 1 <= lineCount - 1) {
            const QTextLine nextLine = textLayout.lineAt(j + 1);
            if ((nextLine.y() + nextLine.height()) > textRect.height()) {
                int start = line.textStart();
                int length = line.textLength() + nextLine.textLength();
                const QStackTextEngine engine(textLayout.text().mid(start, length), option->font);
                elidedText = engine.elidedText(option->textElideMode, textRect.width());
                height += line.height();
                width = textRect.width();
                elidedIndex = j;
                break;
            }
        }
        if (line.naturalTextWidth() > textRect.width()) {
            int start = line.textStart();
            int length = line.textLength();
            const QStackTextEngine engine(textLayout.text().mid(start, length), option->font);
            elidedText = engine.elidedText(option->textElideMode, textRect.width());
            height += line.height();
            width = textRect.width();
            elidedIndex = j;
            break;
        }
        width = qMax<qreal>(width, line.width());
        height += line.height();
    }

    const QRect layoutRect = QStyle::alignedRect(option->direction, option->displayAlignment,
                                                 QSize(int(width), int(height)), textRect);
    const QPointF position = layoutRect.topLeft();
    for (int i = 0; i < lineCount; ++i) {
        const QTextLine line = textLayout.lineAt(i);
        if (i == elidedIndex) {
            qreal x = position.x() + line.x();
            qreal y = position.y() + line.y() + line.ascent();
            p->save();
            p->setFont(option->font);
            p->drawText(QPointF(x, y), elidedText);
            p->restore();
            break;
        }
        line.draw(p, position);
    }
}

/*! \internal
    compute the position for the different component of an item (pixmap, text, checkbox)

    Set sizehint to false to layout the elements inside opt->rect. Set sizehint to true to ignore
   opt->rect and return rectangles in infinite space

    Code duplicated in QItemDelegate::doLayout
*/
void QCommonStylePrivate::viewItemLayout(const QStyleOptionViewItemV4 *opt,  QRect *checkRect,
                                         QRect *pixmapRect, QRect *textRect, bool sizehint) const
{
    Q_ASSERT(checkRect && pixmapRect && textRect);
    *pixmapRect = QRect(QPoint(0, 0), viewItemSize(opt, Qt::DecorationRole));
    *textRect = QRect(QPoint(0, 0), viewItemSize(opt, Qt::DisplayRole));
    *checkRect = QRect(QPoint(0, 0), viewItemSize(opt, Qt::CheckStateRole));

    const QWidget *widget = opt->widget;
    const bool hasCheck = checkRect->isValid();
    const bool hasPixmap = pixmapRect->isValid();
    const bool hasText = textRect->isValid();
    const int textMargin = hasText ? proxyStyle->pixelMetric(QStyle::PM_FocusFrameHMargin, opt, widget) + 1 : 0;
    const int pixmapMargin = hasPixmap ? proxyStyle->pixelMetric(QStyle::PM_FocusFrameHMargin, opt, widget) + 1 : 0;
    const int checkMargin = hasCheck ? proxyStyle->pixelMetric(QStyle::PM_FocusFrameHMargin, opt, widget) + 1 : 0;
    int x = opt->rect.left();
    int y = opt->rect.top();
    int w, h;

    if (textRect->height() == 0 && (!hasPixmap || !sizehint)) {
        //if there is no text, we still want to have a decent height for the item sizeHint and the editor size
        textRect->setHeight(opt->fontMetrics.height());
    }

    QSize pm(0, 0);
    if (hasPixmap) {
        pm = pixmapRect->size();
        pm.rwidth() += 2 * pixmapMargin;
    }
    if (sizehint) {
        h = qMax(checkRect->height(), qMax(textRect->height(), pm.height()));
        if (opt->decorationPosition == QStyleOptionViewItem::Left
            || opt->decorationPosition == QStyleOptionViewItem::Right) {
            w = textRect->width() + pm.width();
        } else {
            w = qMax(textRect->width(), pm.width());
        }
    } else {
        w = opt->rect.width();
        h = opt->rect.height();
    }

    int cw = 0;
    QRect check;
    if (hasCheck) {
        cw = checkRect->width() + 2 * checkMargin;
        if (sizehint) w += cw;
        if (opt->direction == Qt::RightToLeft) {
            check.setRect(x + w - cw, y, cw, h);
        } else {
            check.setRect(x, y, cw, h);
        }
    }

    QRect display;
    QRect decoration;
    switch (opt->decorationPosition) {
    case QStyleOptionViewItem::Top: {
        if (hasPixmap)
            pm.setHeight(pm.height() + pixmapMargin); // add space
        h = sizehint ? textRect->height() : h - pm.height();

        if (opt->direction == Qt::RightToLeft) {
            decoration.setRect(x, y, w - cw, pm.height());
            display.setRect(x, y + pm.height(), w - cw, h);
        } else {
            decoration.setRect(x + cw, y, w - cw, pm.height());
            display.setRect(x + cw, y + pm.height(), w - cw, h);
        }
        break; }
    case QStyleOptionViewItem::Bottom: {
        if (hasText)
            textRect->setHeight(textRect->height() + textMargin); // add space
        h = sizehint ? textRect->height() + pm.height() : h;

        if (opt->direction == Qt::RightToLeft) {
            display.setRect(x, y, w - cw, textRect->height());
            decoration.setRect(x, y + textRect->height(), w - cw, h - textRect->height());
        } else {
            display.setRect(x + cw, y, w - cw, textRect->height());
            decoration.setRect(x + cw, y + textRect->height(), w - cw, h - textRect->height());
        }
        break; }
    case QStyleOptionViewItem::Left: {
        if (opt->direction == Qt::LeftToRight) {
            decoration.setRect(x + cw, y, pm.width(), h);
            display.setRect(decoration.right() + 1, y, w - pm.width() - cw, h);
        } else {
            display.setRect(x, y, w - pm.width() - cw, h);
            decoration.setRect(display.right() + 1, y, pm.width(), h);
        }
        break; }
    case QStyleOptionViewItem::Right: {
        if (opt->direction == Qt::LeftToRight) {
            display.setRect(x + cw, y, w - pm.width() - cw, h);
            decoration.setRect(display.right() + 1, y, pm.width(), h);
        } else {
            decoration.setRect(x, y, pm.width(), h);
            display.setRect(decoration.right() + 1, y, w - pm.width() - cw, h);
        }
        break; }
    default:
        qWarning("doLayout: decoration position is invalid");
        decoration = *pixmapRect;
        break;
    }

    if (!sizehint) { // we only need to do the internal layout if we are going to paint
        *checkRect = QStyle::alignedRect(opt->direction, Qt::AlignCenter,
                                         checkRect->size(), check);
        *pixmapRect = QStyle::alignedRect(opt->direction, opt->decorationAlignment,
                                          pixmapRect->size(), decoration);
        // the text takes up all available space, unless the decoration is not shown as selected
        if (opt->showDecorationSelected)
            *textRect = display;
        else
            *textRect = QStyle::alignedRect(opt->direction, opt->displayAlignment,
                                            textRect->size().boundedTo(display.size()), display);
    } else {
        *checkRect = check;
        *pixmapRect = decoration;
        *textRect = display;
    }
}
#endif // QT_NO_ITEMVIEWS


#ifndef QT_NO_TABBAR
/*! \internal
    Compute the textRect and the pixmapRect from the opt rect

    Uses the same computation than in QTabBar::tabSizeHint
 */
void QCommonStylePrivate::tabLayout(const QStyleOptionTabV3 *opt, const QWidget *widget, QRect *textRect, QRect *iconRect) const
{
    Q_ASSERT(textRect);
    Q_ASSERT(iconRect);
    QRect tr = opt->rect;
    bool verticalTabs = opt->shape == QTabBar::RoundedEast
                        || opt->shape == QTabBar::RoundedWest
                        || opt->shape == QTabBar::TriangularEast
                        || opt->shape == QTabBar::TriangularWest;
    if (verticalTabs)
        tr.setRect(0, 0, tr.height(), tr.width()); //0, 0 as we will have a translate transform

    int verticalShift = proxyStyle->pixelMetric(QStyle::PM_TabBarTabShiftVertical, opt, widget);
    int horizontalShift = proxyStyle->pixelMetric(QStyle::PM_TabBarTabShiftHorizontal, opt, widget);
    int hpadding = proxyStyle->pixelMetric(QStyle::PM_TabBarTabHSpace, opt, widget) / 2;
    int vpadding = proxyStyle->pixelMetric(QStyle::PM_TabBarTabVSpace, opt, widget) / 2;
    if (opt->shape == QTabBar::RoundedSouth || opt->shape == QTabBar::TriangularSouth)
        verticalShift = -verticalShift;
    tr.adjust(hpadding, verticalShift - vpadding, horizontalShift - hpadding, vpadding);
    bool selected = opt->state & QStyle::State_Selected;
    if (selected) {
        tr.setTop(tr.top() - verticalShift);
        tr.setRight(tr.right() - horizontalShift);
    }

    // left widget
    if (!opt->leftButtonSize.isEmpty()) {
        tr.setLeft(tr.left() + 4 +
            (verticalTabs ? opt->leftButtonSize.height() : opt->leftButtonSize.width()));
    }
    // right widget
    if (!opt->rightButtonSize.isEmpty()) {
        tr.setRight(tr.right() - 4 -
        (verticalTabs ? opt->rightButtonSize.height() : opt->rightButtonSize.width()));
    }

    // icon
    if (!opt->icon.isNull()) {
        QSize iconSize = opt->iconSize;
        if (!iconSize.isValid()) {
            int iconExtent = proxyStyle->pixelMetric(QStyle::PM_SmallIconSize);
            iconSize = QSize(iconExtent, iconExtent);
        }
        QSize tabIconSize = opt->icon.actualSize(iconSize,
                        (opt->state & QStyle::State_Enabled) ? QIcon::Normal : QIcon::Disabled,
                        (opt->state & QStyle::State_Selected) ? QIcon::On : QIcon::Off  );

        *iconRect = QRect(tr.left(), tr.center().y() - tabIconSize.height() / 2,
                    tabIconSize.width(), tabIconSize .height());
        if (!verticalTabs)
            *iconRect = proxyStyle->visualRect(opt->direction, opt->rect, *iconRect);
        tr.setLeft(tr.left() + tabIconSize.width() + 4);
    }

    if (!verticalTabs)
        tr = proxyStyle->visualRect(opt->direction, opt->rect, tr);

    *textRect = tr;
}
#endif //QT_NO_TABBAR


/*!
  \reimp
*/
void QCommonStyle::drawControl(ControlElement element, const QStyleOption *opt,
                               QPainter *p, const QWidget *widget) const
{
    Q_D(const QCommonStyle);
    switch (element) {

    case CE_PushButton:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            proxy()->drawControl(CE_PushButtonBevel, btn, p, widget);
            QStyleOptionButton subopt = *btn;
            subopt.rect = subElementRect(SE_PushButtonContents, btn, widget);
            proxy()->drawControl(CE_PushButtonLabel, &subopt, p, widget);
            if (btn->state & State_HasFocus) {
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*btn);
                fropt.rect = subElementRect(SE_PushButtonFocusRect, btn, widget);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, p, widget);
            }
        }
        break;
    case CE_PushButtonBevel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            QRect br = btn->rect;
            int dbi = proxy()->pixelMetric(PM_ButtonDefaultIndicator, btn, widget);
            if (btn->features & QStyleOptionButton::DefaultButton)
                proxy()->drawPrimitive(PE_FrameDefaultButton, opt, p, widget);
            if (btn->features & QStyleOptionButton::AutoDefaultButton)
                br.setCoords(br.left() + dbi, br.top() + dbi, br.right() - dbi, br.bottom() - dbi);
            if (!(btn->features & (QStyleOptionButton::Flat | QStyleOptionButton::CommandLinkButton))
                || btn->state & (State_Sunken | State_On)
                || (btn->features & QStyleOptionButton::CommandLinkButton && btn->state & State_MouseOver)) {
                QStyleOptionButton tmpBtn = *btn;
                tmpBtn.rect = br;
                proxy()->drawPrimitive(PE_PanelButtonCommand, &tmpBtn, p, widget);
            }
            if (btn->features & QStyleOptionButton::HasMenu) {
                int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, btn, widget);
                QRect ir = btn->rect;
                QStyleOptionButton newBtn = *btn;
                newBtn.rect = QRect(ir.right() - mbi + 2, ir.height()/2 - mbi/2 + 3, mbi - 6, mbi - 6);
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &newBtn, p, widget);
            }
        }
        break;
 case CE_PushButtonLabel:
        if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            QRect textRect = button->rect;
            uint tf = Qt::AlignVCenter | Qt::TextShowMnemonic;
            if (!proxy()->styleHint(SH_UnderlineShortcut, button, widget))
                tf |= Qt::TextHideMnemonic;

            if (!button->icon.isNull()) {
                //Center both icon and text
                QRect iconRect;
                QIcon::Mode mode = button->state & State_Enabled ? QIcon::Normal : QIcon::Disabled;
                if (mode == QIcon::Normal && button->state & State_HasFocus)
                    mode = QIcon::Active;
                QIcon::State state = QIcon::Off;
                if (button->state & State_On)
                    state = QIcon::On;

                QPixmap pixmap = button->icon.pixmap(button->iconSize, mode, state);
                int labelWidth = pixmap.width();
                int labelHeight = pixmap.height();
                int iconSpacing = 4;//### 4 is currently hardcoded in QPushButton::sizeHint()
                int textWidth = button->fontMetrics.boundingRect(opt->rect, tf, button->text).width();
                if (!button->text.isEmpty())
                    labelWidth += (textWidth + iconSpacing);

                iconRect = QRect(textRect.x() + (textRect.width() - labelWidth) / 2,
                                 textRect.y() + (textRect.height() - labelHeight) / 2,
                                 pixmap.width(), pixmap.height());

                iconRect = visualRect(button->direction, textRect, iconRect);

                tf |= Qt::AlignLeft; //left align, we adjust the text-rect instead

                if (button->direction == Qt::RightToLeft)
                    textRect.setRight(iconRect.left() - iconSpacing);
                else
                    textRect.setLeft(iconRect.left() + iconRect.width() + iconSpacing);

                if (button->state & (State_On | State_Sunken))
                    iconRect.translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, opt, widget),
                                       proxy()->pixelMetric(PM_ButtonShiftVertical, opt, widget));
                p->drawPixmap(iconRect, pixmap);
            } else {
                tf |= Qt::AlignHCenter;
            }
            if (button->state & (State_On | State_Sunken))
                textRect.translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, opt, widget),
                             proxy()->pixelMetric(PM_ButtonShiftVertical, opt, widget));

            if (button->features & QStyleOptionButton::HasMenu) {
                int indicatorSize = proxy()->pixelMetric(PM_MenuButtonIndicator, button, widget);
                if (button->direction == Qt::LeftToRight)
                    textRect = textRect.adjusted(0, 0, -indicatorSize, 0);
                else
                    textRect = textRect.adjusted(indicatorSize, 0, 0, 0);
            }
            proxy()->drawItemText(p, textRect, tf, button->palette, (button->state & State_Enabled),
                         button->text, QPalette::ButtonText);
        }
        break;
    case CE_RadioButton:
    case CE_CheckBox:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            bool isRadio = (element == CE_RadioButton);
            QStyleOptionButton subopt = *btn;
            subopt.rect = subElementRect(isRadio ? SE_RadioButtonIndicator
                                                 : SE_CheckBoxIndicator, btn, widget);
            proxy()->drawPrimitive(isRadio ? PE_IndicatorRadioButton : PE_IndicatorCheckBox,
                          &subopt, p, widget);
            subopt.rect = subElementRect(isRadio ? SE_RadioButtonContents
                                                 : SE_CheckBoxContents, btn, widget);
            proxy()->drawControl(isRadio ? CE_RadioButtonLabel : CE_CheckBoxLabel, &subopt, p, widget);
            if (btn->state & State_HasFocus) {
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*btn);
                fropt.rect = subElementRect(isRadio ? SE_RadioButtonFocusRect
                                                    : SE_CheckBoxFocusRect, btn, widget);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, p, widget);
            }
        }
        break;
    case CE_RadioButtonLabel:
    case CE_CheckBoxLabel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            uint alignment = visualAlignment(btn->direction, Qt::AlignLeft | Qt::AlignVCenter);

            if (!proxy()->styleHint(SH_UnderlineShortcut, btn, widget))
                alignment |= Qt::TextHideMnemonic;
            QPixmap pix;
            QRect textRect = btn->rect;
            if (!btn->icon.isNull()) {
                pix = btn->icon.pixmap(btn->iconSize, btn->state & State_Enabled ? QIcon::Normal : QIcon::Disabled);
                proxy()->drawItemPixmap(p, btn->rect, alignment, pix);
                if (btn->direction == Qt::RightToLeft)
                    textRect.setRight(textRect.right() - btn->iconSize.width() - 4);
                else
                    textRect.setLeft(textRect.left() + btn->iconSize.width() + 4);
            }
            if (!btn->text.isEmpty()){
                proxy()->drawItemText(p, textRect, alignment | Qt::TextShowMnemonic,
                    btn->palette, btn->state & State_Enabled, btn->text, QPalette::WindowText);
            }
        }
        break;
#ifndef QT_NO_MENU
    case CE_MenuScroller: {
        p->fillRect(opt->rect, opt->palette.background());
        QStyleOption arrowOpt = *opt;
        arrowOpt.state |= State_Enabled;
        proxy()->drawPrimitive(((opt->state & State_DownArrow) ? PE_IndicatorArrowDown : PE_IndicatorArrowUp),
                    &arrowOpt, p, widget);
        break; }
    case CE_MenuTearoff:
        if (opt->state & State_Selected)
            p->fillRect(opt->rect, opt->palette.brush(QPalette::Highlight));
        else
            p->fillRect(opt->rect, opt->palette.brush(QPalette::Button));
        p->setPen(QPen(opt->palette.dark().color(), 1, Qt::DashLine));
        p->drawLine(opt->rect.x() + 2, opt->rect.y() + opt->rect.height() / 2 - 1,
                    opt->rect.x() + opt->rect.width() - 4,
                    opt->rect.y() + opt->rect.height() / 2 - 1);
        p->setPen(QPen(opt->palette.light().color(), 1, Qt::DashLine));
        p->drawLine(opt->rect.x() + 2, opt->rect.y() + opt->rect.height() / 2,
                    opt->rect.x() + opt->rect.width() - 4, opt->rect.y() + opt->rect.height() / 2);
        break;
#endif // QT_NO_MENU
#ifndef QT_NO_MENUBAR
    case CE_MenuBarItem:
        if (const QStyleOptionMenuItem *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            uint alignment = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip
                            | Qt::TextSingleLine;
            if (!proxy()->styleHint(SH_UnderlineShortcut, mbi, widget))
                alignment |= Qt::TextHideMnemonic;
            QPixmap pix = mbi->icon.pixmap(proxy()->pixelMetric(PM_SmallIconSize), (mbi->state & State_Enabled) ? QIcon::Normal : QIcon::Disabled);
            if (!pix.isNull())
                proxy()->drawItemPixmap(p,mbi->rect, alignment, pix);
            else
                proxy()->drawItemText(p, mbi->rect, alignment, mbi->palette, mbi->state & State_Enabled,
                             mbi->text, QPalette::ButtonText);
        }
        break;
    case CE_MenuBarEmptyArea:
        if (widget && !widget->testAttribute(Qt::WA_NoSystemBackground))
            p->eraseRect(opt->rect);
        break;
#endif // QT_NO_MENUBAR
#ifndef QT_NO_PROGRESSBAR
    case CE_ProgressBar:
        if (const QStyleOptionProgressBar *pb
                = qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
            QStyleOptionProgressBarV2 subopt = *pb;
            subopt.rect = subElementRect(SE_ProgressBarGroove, pb, widget);
            proxy()->drawControl(CE_ProgressBarGroove, &subopt, p, widget);
            subopt.rect = subElementRect(SE_ProgressBarContents, pb, widget);
            proxy()->drawControl(CE_ProgressBarContents, &subopt, p, widget);
            if (pb->textVisible) {
                subopt.rect = subElementRect(SE_ProgressBarLabel, pb, widget);
                proxy()->drawControl(CE_ProgressBarLabel, &subopt, p, widget);
            }
        }
        break;
    case CE_ProgressBarGroove:
        if (opt->rect.isValid())
            qDrawShadePanel(p, opt->rect, opt->palette, true, 1,
                            &opt->palette.brush(QPalette::Window));
        break;
    case CE_ProgressBarLabel:
        if (const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
            bool vertical = false;
            if (const QStyleOptionProgressBarV2 *pb2 = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(opt)) {
                vertical = (pb2->orientation == Qt::Vertical);
            }
            if (!vertical) {
                QPalette::ColorRole textRole = QPalette::NoRole;
                if ((pb->textAlignment & Qt::AlignCenter) && pb->textVisible
                    && ((qint64(pb->progress) - qint64(pb->minimum)) * 2 >= (qint64(pb->maximum) - qint64(pb->minimum)))) {
                    textRole = QPalette::HighlightedText;
                    //Draw text shadow, This will increase readability when the background of same color
                    QRect shadowRect(pb->rect);
                    shadowRect.translate(1,1);
                    QColor shadowColor = (pb->palette.color(textRole).value() <= 128)
                       ? QColor(255,255,255,160) : QColor(0,0,0,160);
                    QPalette shadowPalette = pb->palette;
                    shadowPalette.setColor(textRole, shadowColor);
                    proxy()->drawItemText(p, shadowRect, Qt::AlignCenter | Qt::TextSingleLine, shadowPalette,
                                 pb->state & State_Enabled, pb->text, textRole);
                }
                proxy()->drawItemText(p, pb->rect, Qt::AlignCenter | Qt::TextSingleLine, pb->palette,
                             pb->state & State_Enabled, pb->text, textRole);
            }
        }
        break;
    case CE_ProgressBarContents:
        if (const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {

            QRect rect = pb->rect;
            bool vertical = false;
            bool inverted = false;
            qint64 minimum = qint64(pb->minimum);
            qint64 maximum = qint64(pb->maximum);
            qint64 progress = qint64(pb->progress);

            // Get extra style options if version 2
            const QStyleOptionProgressBarV2 *pb2 = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(opt);
            if (pb2) {
                vertical = (pb2->orientation == Qt::Vertical);
                inverted = pb2->invertedAppearance;
            }
            QMatrix m;

            if (vertical) {
                rect = QRect(rect.y(), rect.x(), rect.height(), rect.width()); // flip width and height
                m.rotate(90);
                m.translate(0, -(rect.height() + rect.y()*2));
            }

            QPalette pal2 = pb->palette;
            // Correct the highlight color if it is the same as the background
            if (pal2.highlight() == pal2.background())
                pal2.setColor(QPalette::Highlight, pb->palette.color(QPalette::Active,
                                                                     QPalette::Highlight));
            bool reverse = ((!vertical && (pb->direction == Qt::RightToLeft)) || vertical);
            if (inverted)
                reverse = !reverse;
            int w = rect.width();
            if (pb->minimum == 0 && pb->maximum == 0) {
                // draw busy indicator
                int x = (progress - minimum) % (w * 2);
                if (x > w)
                    x = 2 * w - x;
                x = reverse ? rect.right() - x : x + rect.x();
                p->setPen(QPen(pal2.highlight().color(), 4));
                p->drawLine(x, rect.y(), x, rect.height());
            } else {
                const int unit_width = proxy()->pixelMetric(PM_ProgressBarChunkWidth, pb, widget);
                if (!unit_width)
                    return;

                int u;
                if (unit_width > 1)
                    u = ((rect.width() + unit_width) / unit_width);
                else
                    u = w / unit_width;
                qint64 p_v = progress - minimum;
                qint64 t_s = (maximum - minimum) ? (maximum - minimum) : qint64(1);

                if (u > 0 && p_v >= INT_MAX / u && t_s >= u) {
                    // scale down to something usable.
                    p_v /= u;
                    t_s /= u;
                }

                // nu < tnu, if last chunk is only a partial chunk
                int tnu, nu;
                tnu = nu = p_v * u / t_s;

                if (nu * unit_width > w)
                    --nu;

                // Draw nu units out of a possible u of unit_width
                // width, each a rectangle bordered by background
                // color, all in a sunken panel with a percentage text
                // display at the end.
                int x = 0;
                int x0 = reverse ? rect.right() - ((unit_width > 1) ? unit_width : 0)
                                 : rect.x();

                QStyleOptionProgressBarV2 pbBits = *pb;
                pbBits.rect = rect;
                pbBits.palette = pal2;
                int myY = pbBits.rect.y();
                int myHeight = pbBits.rect.height();
                pbBits.state = State_None;
                for (int i = 0; i < nu; ++i) {
                    pbBits.rect.setRect(x0 + x, myY, unit_width, myHeight);
                    pbBits.rect = m.mapRect(QRectF(pbBits.rect)).toRect();
                    proxy()->drawPrimitive(PE_IndicatorProgressChunk, &pbBits, p, widget);
                    x += reverse ? -unit_width : unit_width;
                }

                // Draw the last partial chunk to fill up the
                // progress bar entirely
                if (nu < tnu) {
                    int pixels_left = w - (nu * unit_width);
                    int offset = reverse ? x0 + x + unit_width-pixels_left : x0 + x;
                    pbBits.rect.setRect(offset, myY, pixels_left, myHeight);
                    pbBits.rect = m.mapRect(QRectF(pbBits.rect)).toRect();
                    proxy()->drawPrimitive(PE_IndicatorProgressChunk, &pbBits, p, widget);
                }
            }
        }
        break;
#endif // QT_NO_PROGRESSBAR
    case CE_HeaderLabel:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
            QRect rect = header->rect;
            if (!header->icon.isNull()) {
                QPixmap pixmap
                    = header->icon.pixmap(proxy()->pixelMetric(PM_SmallIconSize), (header->state & State_Enabled) ? QIcon::Normal : QIcon::Disabled);
                int pixw = pixmap.width();

                QRect aligned = alignedRect(header->direction, QFlag(header->iconAlignment), pixmap.size(), rect);
                QRect inter = aligned.intersected(rect);
                p->drawPixmap(inter.x(), inter.y(), pixmap, inter.x() - aligned.x(), inter.y() - aligned.y(), inter.width(), inter.height());

                if (header->direction == Qt::LeftToRight)
                    rect.setLeft(rect.left() + pixw + 2);
                else
                    rect.setRight(rect.right() - pixw - 2);
            }
            if (header->state & QStyle::State_On) {
                QFont fnt = p->font();
                fnt.setBold(true);
                p->setFont(fnt);
            }
            proxy()->drawItemText(p, rect, header->textAlignment, header->palette,
                         (header->state & State_Enabled), header->text, QPalette::ButtonText);
        }
        break;
#ifndef QT_NO_TOOLBUTTON
    case CE_ToolButtonLabel:
        if (const QStyleOptionToolButton *toolbutton
                = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {
            QRect rect = toolbutton->rect;
            int shiftX = 0;
            int shiftY = 0;
            if (toolbutton->state & (State_Sunken | State_On)) {
                shiftX = proxy()->pixelMetric(PM_ButtonShiftHorizontal, toolbutton, widget);
                shiftY = proxy()->pixelMetric(PM_ButtonShiftVertical, toolbutton, widget);
            }
            // Arrow type always overrules and is always shown
            bool hasArrow = toolbutton->features & QStyleOptionToolButton::Arrow;
            if (((!hasArrow && toolbutton->icon.isNull()) && !toolbutton->text.isEmpty())
                || toolbutton->toolButtonStyle == Qt::ToolButtonTextOnly) {
                int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
                if (!proxy()->styleHint(SH_UnderlineShortcut, opt, widget))
                    alignment |= Qt::TextHideMnemonic;
                rect.translate(shiftX, shiftY);
                p->setFont(toolbutton->font);
                proxy()->drawItemText(p, rect, alignment, toolbutton->palette,
                             opt->state & State_Enabled, toolbutton->text,
                             QPalette::ButtonText);
            } else {
                QPixmap pm;
                QSize pmSize = toolbutton->iconSize;
                if (!toolbutton->icon.isNull()) {
                    QIcon::State state = toolbutton->state & State_On ? QIcon::On : QIcon::Off;
                    QIcon::Mode mode;
                    if (!(toolbutton->state & State_Enabled))
                        mode = QIcon::Disabled;
                    else if ((opt->state & State_MouseOver) && (opt->state & State_AutoRaise))
                        mode = QIcon::Active;
                    else
                        mode = QIcon::Normal;
                    pm = toolbutton->icon.pixmap(toolbutton->rect.size().boundedTo(toolbutton->iconSize),
                                                 mode, state);
                    pmSize = pm.size();
                }

                if (toolbutton->toolButtonStyle != Qt::ToolButtonIconOnly) {
                    p->setFont(toolbutton->font);
                    QRect pr = rect,
                    tr = rect;
                    int alignment = Qt::TextShowMnemonic;
                    if (!proxy()->styleHint(SH_UnderlineShortcut, opt, widget))
                        alignment |= Qt::TextHideMnemonic;

                    if (toolbutton->toolButtonStyle == Qt::ToolButtonTextUnderIcon) {
                        pr.setHeight(pmSize.height() + 6);
                        tr.adjust(0, pr.height() - 1, 0, -2);
                        pr.translate(shiftX, shiftY);
                        if (!hasArrow) {
                            proxy()->drawItemPixmap(p, pr, Qt::AlignCenter, pm);
                        } else {
                            drawArrow(this, toolbutton, pr, p, widget);
                        }
                        alignment |= Qt::AlignCenter;
                    } else {
                        pr.setWidth(pmSize.width() + 8);
                        tr.adjust(pr.width(), 0, 0, 0);
                        pr.translate(shiftX, shiftY);
                        if (!hasArrow) {
                            proxy()->drawItemPixmap(p, QStyle::visualRect(opt->direction, rect, pr), Qt::AlignCenter, pm);
                        } else {
                            drawArrow(this, toolbutton, pr, p, widget);
                        }
                        alignment |= Qt::AlignLeft | Qt::AlignVCenter;
                    }
                    tr.translate(shiftX, shiftY);
                    proxy()->drawItemText(p, QStyle::visualRect(opt->direction, rect, tr), alignment, toolbutton->palette,
                                 toolbutton->state & State_Enabled, toolbutton->text,
                                 QPalette::ButtonText);
                } else {
                    rect.translate(shiftX, shiftY);
                    if (hasArrow) {
                        drawArrow(this, toolbutton, rect, p, widget);
                    } else {
                        proxy()->drawItemPixmap(p, rect, Qt::AlignCenter, pm);
                    }
                }
            }
        }
        break;
#endif // QT_NO_TOOLBUTTON
#ifndef QT_NO_TOOLBOX
    case CE_ToolBoxTab:
        if (const QStyleOptionToolBox *tb = qstyleoption_cast<const QStyleOptionToolBox *>(opt)) {
            proxy()->drawControl(CE_ToolBoxTabShape, tb, p, widget);
            proxy()->drawControl(CE_ToolBoxTabLabel, tb, p, widget);
        }
        break;
    case CE_ToolBoxTabShape:
        if (const QStyleOptionToolBox *tb = qstyleoption_cast<const QStyleOptionToolBox *>(opt)) {
            int d = 20 + tb->rect.height() - 3;
            QPolygon a(7);
            if (tb->direction != Qt::RightToLeft) {
                a.setPoint(0, -1, tb->rect.height() + 1);
                a.setPoint(1, -1, 1);
                a.setPoint(2, tb->rect.width() - d, 1);
                a.setPoint(3, tb->rect.width() - 20, tb->rect.height() - 2);
                a.setPoint(4, tb->rect.width() - 1, tb->rect.height() - 2);
                a.setPoint(5, tb->rect.width() - 1, tb->rect.height() + 1);
                a.setPoint(6, -1, tb->rect.height() + 1);
            } else {
                a.setPoint(0, tb->rect.width(), tb->rect.height() + 1);
                a.setPoint(1, tb->rect.width(), 1);
                a.setPoint(2, d - 1, 1);
                a.setPoint(3, 20 - 1, tb->rect.height() - 2);
                a.setPoint(4, 0, tb->rect.height() - 2);
                a.setPoint(5, 0, tb->rect.height() + 1);
                a.setPoint(6, tb->rect.width(), tb->rect.height() + 1);
            }

            p->setPen(tb->palette.mid().color().darker(150));
            p->drawPolygon(a);
            p->setPen(tb->palette.light().color());
            if (tb->direction != Qt::RightToLeft) {
                p->drawLine(0, 2, tb->rect.width() - d, 2);
                p->drawLine(tb->rect.width() - d - 1, 2, tb->rect.width() - 21, tb->rect.height() - 1);
                p->drawLine(tb->rect.width() - 20, tb->rect.height() - 1,
                            tb->rect.width(), tb->rect.height() - 1);
            } else {
                p->drawLine(tb->rect.width() - 1, 2, d - 1, 2);
                p->drawLine(d, 2, 20, tb->rect.height() - 1);
                p->drawLine(19, tb->rect.height() - 1,
                            -1, tb->rect.height() - 1);
            }
            p->setBrush(Qt::NoBrush);
        }
        break;
#endif // QT_NO_TOOLBOX
#ifndef QT_NO_TABBAR
    case CE_TabBarTab:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            proxy()->drawControl(CE_TabBarTabShape, tab, p, widget);
            proxy()->drawControl(CE_TabBarTabLabel, tab, p, widget);
        }
        break;
    case CE_TabBarTabShape:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            p->save();

            QRect rect(tab->rect);
            bool selected = tab->state & State_Selected;
            bool onlyOne = tab->position == QStyleOptionTab::OnlyOneTab;
            int tabOverlap = onlyOne ? 0 : proxy()->pixelMetric(PM_TabBarTabOverlap, opt, widget);

            if (!selected) {
                switch (tab->shape) {
                case QTabBar::TriangularNorth:
                    rect.adjust(0, 0, 0, -tabOverlap);
                    if(!selected)
                        rect.adjust(1, 1, -1, 0);
                    break;
                case QTabBar::TriangularSouth:
                    rect.adjust(0, tabOverlap, 0, 0);
                    if(!selected)
                        rect.adjust(1, 0, -1, -1);
                    break;
                case QTabBar::TriangularEast:
                    rect.adjust(tabOverlap, 0, 0, 0);
                    if(!selected)
                        rect.adjust(0, 1, -1, -1);
                    break;
                case QTabBar::TriangularWest:
                    rect.adjust(0, 0, -tabOverlap, 0);
                    if(!selected)
                        rect.adjust(1, 1, 0, -1);
                    break;
                default:
                    break;
                }
            }

            p->setPen(QPen(tab->palette.foreground(), 0));
            if (selected) {
                p->setBrush(tab->palette.base());
            } else {
                if (widget && widget->parentWidget())
                    p->setBrush(widget->parentWidget()->palette().background());
                else
                    p->setBrush(tab->palette.background());
            }

            int y;
            int x;
            QPolygon a(10);
            switch (tab->shape) {
            case QTabBar::TriangularNorth:
            case QTabBar::TriangularSouth: {
                a.setPoint(0, 0, -1);
                a.setPoint(1, 0, 0);
                y = rect.height() - 2;
                x = y / 3;
                a.setPoint(2, x++, y - 1);
                ++x;
                a.setPoint(3, x++, y++);
                a.setPoint(4, x, y);

                int i;
                int right = rect.width() - 1;
                for (i = 0; i < 5; ++i)
                    a.setPoint(9 - i, right - a.point(i).x(), a.point(i).y());
                if (tab->shape == QTabBar::TriangularNorth)
                    for (i = 0; i < 10; ++i)
                        a.setPoint(i, a.point(i).x(), rect.height() - 1 - a.point(i).y());

                a.translate(rect.left(), rect.top());
                p->setRenderHint(QPainter::Antialiasing);
                p->translate(0, 0.5);

                QPainterPath path;
                path.addPolygon(a);
                p->drawPath(path);
                break; }
            case QTabBar::TriangularEast:
            case QTabBar::TriangularWest: {
                a.setPoint(0, -1, 0);
                a.setPoint(1, 0, 0);
                x = rect.width() - 2;
                y = x / 3;
                a.setPoint(2, x - 1, y++);
                ++y;
                a.setPoint(3, x++, y++);
                a.setPoint(4, x, y);
                int i;
                int bottom = rect.height() - 1;
                for (i = 0; i < 5; ++i)
                    a.setPoint(9 - i, a.point(i).x(), bottom - a.point(i).y());
                if (tab->shape == QTabBar::TriangularWest)
                    for (i = 0; i < 10; ++i)
                        a.setPoint(i, rect.width() - 1 - a.point(i).x(), a.point(i).y());
                a.translate(rect.left(), rect.top());
                p->setRenderHint(QPainter::Antialiasing);
                p->translate(0.5, 0);
                QPainterPath path;
                path.addPolygon(a);
                p->drawPath(path);
                break; }
            default:
                break;
            }
            p->restore();
        }
        break;
    case CE_ToolBoxTabLabel:
        if (const QStyleOptionToolBox *tb = qstyleoption_cast<const QStyleOptionToolBox *>(opt)) {
            bool enabled = tb->state & State_Enabled;
            bool selected = tb->state & State_Selected;
            QPixmap pm = tb->icon.pixmap(proxy()->pixelMetric(QStyle::PM_SmallIconSize, tb, widget),
                                         enabled ? QIcon::Normal : QIcon::Disabled);

            QRect cr = subElementRect(QStyle::SE_ToolBoxTabContents, tb, widget);
            QRect tr, ir;
            int ih = 0;
            if (pm.isNull()) {
                tr = cr;
                tr.adjust(4, 0, -8, 0);
            } else {
                int iw = pm.width() + 4;
                ih = pm.height();
                ir = QRect(cr.left() + 4, cr.top(), iw + 2, ih);
                tr = QRect(ir.right(), cr.top(), cr.width() - ir.right() - 4, cr.height());
            }

            if (selected && proxy()->styleHint(QStyle::SH_ToolBox_SelectedPageTitleBold, tb, widget)) {
                QFont f(p->font());
                f.setBold(true);
                p->setFont(f);
            }

            QString txt = tb->fontMetrics.elidedText(tb->text, Qt::ElideRight, tr.width());

            if (ih)
                p->drawPixmap(ir.left(), (tb->rect.height() - ih) / 2, pm);

            int alignment = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic;
            if (!proxy()->styleHint(QStyle::SH_UnderlineShortcut, tb, widget))
                alignment |= Qt::TextHideMnemonic;
            proxy()->drawItemText(p, tr, alignment, tb->palette, enabled, txt, QPalette::ButtonText);

            if (!txt.isEmpty() && opt->state & State_HasFocus) {
                QStyleOptionFocusRect opt;
                opt.rect = tr;
                opt.palette = tb->palette;
                opt.state = QStyle::State_None;
                proxy()->drawPrimitive(QStyle::PE_FrameFocusRect, &opt, p, widget);
            }
        }
        break;
    case CE_TabBarTabLabel:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            QStyleOptionTabV3 tabV2(*tab);
            QRect tr = tabV2.rect;
            bool verticalTabs = tabV2.shape == QTabBar::RoundedEast
                                || tabV2.shape == QTabBar::RoundedWest
                                || tabV2.shape == QTabBar::TriangularEast
                                || tabV2.shape == QTabBar::TriangularWest;

            int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
            if (!proxy()->styleHint(SH_UnderlineShortcut, opt, widget))
                alignment |= Qt::TextHideMnemonic;

            if (verticalTabs) {
                p->save();
                int newX, newY, newRot;
                if (tabV2.shape == QTabBar::RoundedEast || tabV2.shape == QTabBar::TriangularEast) {
                    newX = tr.width() + tr.x();
                    newY = tr.y();
                    newRot = 90;
                } else {
                    newX = tr.x();
                    newY = tr.y() + tr.height();
                    newRot = -90;
                }
                QTransform m = QTransform::fromTranslate(newX, newY);
                m.rotate(newRot);
                p->setTransform(m, true);
            }
            QRect iconRect;
            d->tabLayout(&tabV2, widget, &tr, &iconRect);
            tr = proxy()->subElementRect(SE_TabBarTabText, opt, widget); //we compute tr twice because the style may override subElementRect

            if (!tabV2.icon.isNull()) {
                QPixmap tabIcon = tabV2.icon.pixmap(tabV2.iconSize,
                                                    (tabV2.state & State_Enabled) ? QIcon::Normal
                                                                                  : QIcon::Disabled,
                                                    (tabV2.state & State_Selected) ? QIcon::On
                                                                                   : QIcon::Off);
                p->drawPixmap(iconRect.x(), iconRect.y(), tabIcon);
            }

            proxy()->drawItemText(p, tr, alignment, tab->palette, tab->state & State_Enabled, tab->text, QPalette::WindowText);
            if (verticalTabs)
                p->restore();

            if (tabV2.state & State_HasFocus) {
                const int OFFSET = 1 + pixelMetric(PM_DefaultFrameWidth);

                int x1, x2;
                x1 = tabV2.rect.left();
                x2 = tabV2.rect.right() - 1;

                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*tab);
                fropt.rect.setRect(x1 + 1 + OFFSET, tabV2.rect.y() + OFFSET,
                                   x2 - x1 - 2*OFFSET, tabV2.rect.height() - 2*OFFSET);
                drawPrimitive(PE_FrameFocusRect, &fropt, p, widget);
            }
        }
        break;
#endif // QT_NO_TABBAR
#ifndef QT_NO_SIZEGRIP
    case CE_SizeGrip: {
        p->save();
        int x, y, w, h;
        opt->rect.getRect(&x, &y, &w, &h);

        int sw = qMin(h, w);
        if (h > w)
            p->translate(0, h - w);
        else
            p->translate(w - h, 0);

        int sx = x;
        int sy = y;
        int s = sw / 3;

        Qt::Corner corner;
        if (const QStyleOptionSizeGrip *sgOpt = qstyleoption_cast<const QStyleOptionSizeGrip *>(opt))
            corner = sgOpt->corner;
        else if (opt->direction == Qt::RightToLeft)
            corner = Qt::BottomLeftCorner;
        else
            corner = Qt::BottomRightCorner;

        if (corner == Qt::BottomLeftCorner) {
            sx = x + sw;
            for (int i = 0; i < 4; ++i) {
                p->setPen(QPen(opt->palette.light().color(), 1));
                p->drawLine(x, sy - 1 , sx + 1, sw);
                p->setPen(QPen(opt->palette.dark().color(), 1));
                p->drawLine(x, sy, sx, sw);
                p->setPen(QPen(opt->palette.dark().color(), 1));
                p->drawLine(x, sy + 1, sx - 1, sw);
                sx -= s;
                sy += s;
            }
        } else if (corner == Qt::BottomRightCorner) {
            for (int i = 0; i < 4; ++i) {
                p->setPen(QPen(opt->palette.light().color(), 1));
                p->drawLine(sx - 1, sw, sw, sy - 1);
                p->setPen(QPen(opt->palette.dark().color(), 1));
                p->drawLine(sx, sw, sw, sy);
                p->setPen(QPen(opt->palette.dark().color(), 1));
                p->drawLine(sx + 1, sw, sw, sy + 1);
                sx += s;
                sy += s;
            }
        } else if (corner == Qt::TopRightCorner) {
            sy = y + sw;
            for (int i = 0; i < 4; ++i) {
                p->setPen(QPen(opt->palette.light().color(), 1));
                p->drawLine(sx - 1, y, sw, sy + 1);
                p->setPen(QPen(opt->palette.dark().color(), 1));
                p->drawLine(sx, y, sw, sy);
                p->setPen(QPen(opt->palette.dark().color(), 1));
                p->drawLine(sx + 1, y, sw, sy - 1);
                sx += s;
                sy -= s;
            }
        } else if (corner == Qt::TopLeftCorner) {
            for (int i = 0; i < 4; ++i) {
                p->setPen(QPen(opt->palette.light().color(), 1));
                p->drawLine(x, sy - 1, sx - 1, y);
                p->setPen(QPen(opt->palette.dark().color(), 1));
                p->drawLine(x, sy, sx, y);
                p->setPen(QPen(opt->palette.dark().color(), 1));
                p->drawLine(x, sy + 1, sx + 1, y);
                sx += s;
                sy += s;
            }
        }
        p->restore();
        break; }
#endif // QT_NO_SIZEGRIP
#ifndef QT_NO_RUBBERBAND
    case CE_RubberBand: {
        if (const QStyleOptionRubberBand *rbOpt = qstyleoption_cast<const QStyleOptionRubberBand *>(opt)) {
            QPixmap tiledPixmap(16, 16);
            QPainter pixmapPainter(&tiledPixmap);
            pixmapPainter.setPen(Qt::NoPen);
            pixmapPainter.setBrush(Qt::Dense4Pattern);
            pixmapPainter.setBackground(QBrush(opt->palette.base()));
            pixmapPainter.setBackgroundMode(Qt::OpaqueMode);
            pixmapPainter.drawRect(0, 0, tiledPixmap.width(), tiledPixmap.height());
            pixmapPainter.end();
            // ### workaround for borked XRENDER
            tiledPixmap = QPixmap::fromImage(tiledPixmap.toImage());

            p->save();
            QRect r = opt->rect;
            QStyleHintReturnMask mask;
            if (proxy()->styleHint(QStyle::SH_RubberBand_Mask, opt, widget, &mask))
                p->setClipRegion(mask.region);
            p->drawTiledPixmap(r.x(), r.y(), r.width(), r.height(), tiledPixmap);
            p->setPen(opt->palette.color(QPalette::Active, QPalette::WindowText));
            p->setBrush(Qt::NoBrush);
            p->drawRect(r.adjusted(0, 0, -1, -1));
            if (rbOpt->shape == QRubberBand::Rectangle)
                p->drawRect(r.adjusted(3, 3, -4, -4));
            p->restore();
        }
        break; }
#endif // QT_NO_RUBBERBAND
#ifndef QT_NO_DOCKWIDGET
    case CE_DockWidgetTitle:
        if (const QStyleOptionDockWidget *dwOpt = qstyleoption_cast<const QStyleOptionDockWidget *>(opt)) {
            QRect r = dwOpt->rect.adjusted(0, 0, -1, -1);
            if (dwOpt->movable) {
                p->setPen(dwOpt->palette.color(QPalette::Dark));
                p->drawRect(r);
            }

            if (!dwOpt->title.isEmpty()) {
                const QStyleOptionDockWidgetV2 *v2
                    = qstyleoption_cast<const QStyleOptionDockWidgetV2*>(opt);
                bool verticalTitleBar = v2 == 0 ? false : v2->verticalTitleBar;

                if (verticalTitleBar) {
                    QSize s = r.size();
                    s.transpose();
                    r.setSize(s);

                    p->save();
                    p->translate(r.left(), r.top() + r.width());
                    p->rotate(-90);
                    p->translate(-r.left(), -r.top());
                }

                const int indent = p->fontMetrics().descent();
                proxy()->drawItemText(p, r.adjusted(indent + 1, 1, -indent - 1, -1),
                              Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, dwOpt->palette,
                              dwOpt->state & State_Enabled, dwOpt->title,
                              QPalette::WindowText);

                if (verticalTitleBar)
                    p->restore();
            }
        }
        break;
#endif // QT_NO_DOCKWIDGET
    case CE_Header:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
            QRegion clipRegion = p->clipRegion();
            p->setClipRect(opt->rect);
            proxy()->drawControl(CE_HeaderSection, header, p, widget);
            QStyleOptionHeader subopt = *header;
            subopt.rect = subElementRect(SE_HeaderLabel, header, widget);
            if (subopt.rect.isValid())
                proxy()->drawControl(CE_HeaderLabel, &subopt, p, widget);
            if (header->sortIndicator != QStyleOptionHeader::None) {
                subopt.rect = subElementRect(SE_HeaderArrow, opt, widget);
                proxy()->drawPrimitive(PE_IndicatorHeaderArrow, &subopt, p, widget);
            }
            p->setClipRegion(clipRegion);
        }
        break;
    case CE_FocusFrame:
            p->fillRect(opt->rect, opt->palette.foreground());
        break;
    case CE_HeaderSection:
            qDrawShadePanel(p, opt->rect, opt->palette,
                        opt->state & State_Sunken, 1,
                        &opt->palette.brush(QPalette::Button));
        break;
    case CE_HeaderEmptyArea:
            p->fillRect(opt->rect, opt->palette.background());
        break;
#ifndef QT_NO_COMBOBOX
    case CE_ComboBoxLabel:
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            QRect editRect = proxy()->subControlRect(CC_ComboBox, cb, SC_ComboBoxEditField, widget);
            p->save();
            p->setClipRect(editRect);
            if (!cb->currentIcon.isNull()) {
                QIcon::Mode mode = cb->state & State_Enabled ? QIcon::Normal
                                                             : QIcon::Disabled;
                QPixmap pixmap = cb->currentIcon.pixmap(cb->iconSize, mode);
                QRect iconRect(editRect);
                iconRect.setWidth(cb->iconSize.width() + 4);
                iconRect = alignedRect(cb->direction,
                                       Qt::AlignLeft | Qt::AlignVCenter,
                                       iconRect.size(), editRect);
                if (cb->editable)
                    p->fillRect(iconRect, opt->palette.brush(QPalette::Base));
                proxy()->drawItemPixmap(p, iconRect, Qt::AlignCenter, pixmap);

                if (cb->direction == Qt::RightToLeft)
                    editRect.translate(-4 - cb->iconSize.width(), 0);
                else
                    editRect.translate(cb->iconSize.width() + 4, 0);
            }
            if (!cb->currentText.isEmpty() && !cb->editable) {
                proxy()->drawItemText(p, editRect.adjusted(1, 0, -1, 0),
                             visualAlignment(cb->direction, Qt::AlignLeft | Qt::AlignVCenter),
                             cb->palette, cb->state & State_Enabled, cb->currentText);
            }
            p->restore();
        }
        break;
#endif // QT_NO_COMBOBOX
#ifndef QT_NO_TOOLBAR
    case CE_ToolBar:
        if (const QStyleOptionToolBar *toolBar = qstyleoption_cast<const QStyleOptionToolBar *>(opt)) {
            // Compatibility with styles that use PE_PanelToolBar
            QStyleOptionFrame frame;
            frame.QStyleOption::operator=(*toolBar);
            frame.lineWidth = toolBar->lineWidth;
            frame.midLineWidth = toolBar->midLineWidth;
            proxy()->drawPrimitive(PE_PanelToolBar, opt, p, widget);

            if (widget && qobject_cast<QToolBar *>(widget->parentWidget()))
                break;
            qDrawShadePanel(p, toolBar->rect, toolBar->palette, false, toolBar->lineWidth,
                            &toolBar->palette.brush(QPalette::Button));
        }
        break;
#endif // QT_NO_TOOLBAR
    case CE_ColumnViewGrip: {
        // draw background gradients
        QLinearGradient g(0, 0, opt->rect.width(), 0);
        g.setColorAt(0, opt->palette.color(QPalette::Active, QPalette::Mid));
        g.setColorAt(0.5, Qt::white);
        p->fillRect(QRect(0, 0, opt->rect.width(), opt->rect.height()), g);

        // draw the two lines
        QPen pen(p->pen());
        pen.setWidth(opt->rect.width()/20);
        pen.setColor(opt->palette.color(QPalette::Active, QPalette::Dark));
        p->setPen(pen);

        int line1starting = opt->rect.width()*8 / 20;
        int line2starting = opt->rect.width()*13 / 20;
        int top = opt->rect.height()*20/75;
        int bottom = opt->rect.height() - 1 - top;
        p->drawLine(line1starting, top, line1starting, bottom);
        p->drawLine(line2starting, top, line2starting, bottom);
        }
        break;

#ifndef QT_NO_ITEMVIEWS
    case CE_ItemViewItem:
        if (const QStyleOptionViewItemV4 *vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(opt)) {
            p->save();
            p->setClipRect(opt->rect);

            QRect checkRect = subElementRect(SE_ItemViewItemCheckIndicator, vopt, widget);
            QRect iconRect = subElementRect(SE_ItemViewItemDecoration, vopt, widget);
            QRect textRect = subElementRect(SE_ItemViewItemText, vopt, widget);

            // draw the background
            proxy()->drawPrimitive(PE_PanelItemViewItem, opt, p, widget);

            // draw the check mark
            if (vopt->features & QStyleOptionViewItemV2::HasCheckIndicator) {
                QStyleOptionViewItemV4 option(*vopt);
                option.rect = checkRect;
                option.state = option.state & ~QStyle::State_HasFocus;

                switch (vopt->checkState) {
                case Qt::Unchecked:
                    option.state |= QStyle::State_Off;
                    break;
                case Qt::PartiallyChecked:
                    option.state |= QStyle::State_NoChange;
                    break;
                case Qt::Checked:
                    option.state |= QStyle::State_On;
                    break;
                }
                proxy()->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &option, p, widget);
            }

            // draw the icon
            QIcon::Mode mode = QIcon::Normal;
            if (!(vopt->state & QStyle::State_Enabled))
                mode = QIcon::Disabled;
            else if (vopt->state & QStyle::State_Selected)
                mode = QIcon::Selected;
            QIcon::State state = vopt->state & QStyle::State_Open ? QIcon::On : QIcon::Off;
            vopt->icon.paint(p, iconRect, vopt->decorationAlignment, mode, state);

            // draw the text
            if (!vopt->text.isEmpty()) {
                QPalette::ColorGroup cg = vopt->state & QStyle::State_Enabled
                                      ? QPalette::Normal : QPalette::Disabled;
                if (cg == QPalette::Normal && !(vopt->state & QStyle::State_Active))
                    cg = QPalette::Inactive;

                if (vopt->state & QStyle::State_Selected) {
                    p->setPen(vopt->palette.color(cg, QPalette::HighlightedText));
                } else {
                    p->setPen(vopt->palette.color(cg, QPalette::Text));
                }
                if (vopt->state & QStyle::State_Editing) {
                    p->setPen(vopt->palette.color(cg, QPalette::Text));
                    p->drawRect(textRect.adjusted(0, 0, -1, -1));
                }

                d->viewItemDrawText(p, vopt, textRect);
            }

            // draw the focus rect
             if (vopt->state & QStyle::State_HasFocus) {
                QStyleOptionFocusRect o;
                o.QStyleOption::operator=(*vopt);
                o.rect = proxy()->subElementRect(SE_ItemViewItemFocusRect, vopt, widget);
                o.state |= QStyle::State_KeyboardFocusChange;
                o.state |= QStyle::State_Item;
                QPalette::ColorGroup cg = (vopt->state & QStyle::State_Enabled)
                              ? QPalette::Normal : QPalette::Disabled;
                o.backgroundColor = vopt->palette.color(cg, (vopt->state & QStyle::State_Selected)
                                             ? QPalette::Highlight : QPalette::Window);
                proxy()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, p, widget);
            }

             p->restore();
        }
        break;

#endif // QT_NO_ITEMVIEWS
#ifndef QT_NO_FRAME
    case CE_ShapedFrame:
        if (const QStyleOptionFrameV3 *f = qstyleoption_cast<const QStyleOptionFrameV3 *>(opt)) {
            int frameShape  = f->frameShape;
            int frameShadow = QFrame::Plain;
            if (f->state & QStyle::State_Sunken) {
                frameShadow = QFrame::Sunken;
            } else if (f->state & QStyle::State_Raised) {
                frameShadow = QFrame::Raised;
            }

            int lw = f->lineWidth;
            int mlw = f->midLineWidth;
            QPalette::ColorRole foregroundRole = QPalette::WindowText;
            if (widget)
                foregroundRole = widget->foregroundRole();

            switch (frameShape) {
            case QFrame::Box:
                if (frameShadow == QFrame::Plain) {
                    qDrawPlainRect(p, f->rect, f->palette.color(foregroundRole), lw);
                } else {
                    qDrawShadeRect(p, f->rect, f->palette, frameShadow == QFrame::Sunken, lw, mlw);
                }
                break;
            case QFrame::StyledPanel:
                //keep the compatibility with Qt 4.4 if there is a proxy style.
                //be sure to call drawPrimitive(QStyle::PE_Frame) on the proxy style
                if (widget) {
                    widget->style()->drawPrimitive(QStyle::PE_Frame, opt, p, widget);
                } else {
                    proxy()->drawPrimitive(QStyle::PE_Frame, opt, p, widget);
                }
                break;
            case QFrame::Panel:
                if (frameShadow == QFrame::Plain) {
                    qDrawPlainRect(p, f->rect, f->palette.color(foregroundRole), lw);
                } else {
                    qDrawShadePanel(p, f->rect, f->palette, frameShadow == QFrame::Sunken, lw);
                }
                break;
            case QFrame::WinPanel:
                if (frameShadow == QFrame::Plain) {
                    qDrawPlainRect(p, f->rect, f->palette.color(foregroundRole), lw);
                } else {
                    qDrawWinPanel(p, f->rect, f->palette, frameShadow == QFrame::Sunken);
                }
                break;
            case QFrame::HLine:
            case QFrame::VLine: {
                QPoint p1, p2;
                if (frameShape == QFrame::HLine) {
                    p1 = QPoint(opt->rect.x(), opt->rect.height() / 2);
                    p2 = QPoint(opt->rect.x() + opt->rect.width(), p1.y());
                } else {
                    p1 = QPoint(opt->rect.x()+opt->rect.width() / 2, 0);
                    p2 = QPoint(p1.x(), opt->rect.height());
                }
                if (frameShadow == QFrame::Plain) {
                    QPen oldPen = p->pen();
                    p->setPen(QPen(opt->palette.brush(foregroundRole), lw));
                    p->drawLine(p1, p2);
                    p->setPen(oldPen);
                } else {
                    qDrawShadeLine(p, p1, p2, f->palette, frameShadow == QFrame::Sunken, lw, mlw);
                }
                break;
                }
            }
        }
        break;
#endif
    default:
        break;
    }
}

/*!
  \reimp
*/
QRect QCommonStyle::subElementRect(SubElement sr, const QStyleOption *opt,
                                   const QWidget *widget) const
{
    Q_D(const QCommonStyle);
    QRect r;
    switch (sr) {
    case SE_PushButtonContents:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            int dx1, dx2;
            dx1 = proxy()->pixelMetric(PM_DefaultFrameWidth, btn, widget);
            if (btn->features & QStyleOptionButton::AutoDefaultButton)
                dx1 += proxy()->pixelMetric(PM_ButtonDefaultIndicator, btn, widget);
            dx2 = dx1 * 2;
            r.setRect(opt->rect.x() + dx1, opt->rect.y() + dx1, opt->rect.width() - dx2,
                      opt->rect.height() - dx2);
            r = visualRect(opt->direction, opt->rect, r);
        }
        break;
    case SE_PushButtonFocusRect:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            int dbw1 = 0, dbw2 = 0;
            if (btn->features & QStyleOptionButton::AutoDefaultButton){
                dbw1 = proxy()->pixelMetric(PM_ButtonDefaultIndicator, btn, widget);
                dbw2 = dbw1 * 2;
            }

            int dfw1 = proxy()->pixelMetric(PM_DefaultFrameWidth, btn, widget) + 1,
                dfw2 = dfw1 * 2;

            r.setRect(btn->rect.x() + dfw1 + dbw1, btn->rect.y() + dfw1 + dbw1,
                      btn->rect.width() - dfw2 - dbw2, btn->rect.height()- dfw2 - dbw2);
            r = visualRect(opt->direction, opt->rect, r);
        }
        break;
    case SE_CheckBoxIndicator:
        {
            int h = proxy()->pixelMetric(PM_IndicatorHeight, opt, widget);
            r.setRect(opt->rect.x(), opt->rect.y() + ((opt->rect.height() - h) / 2),
                      proxy()->pixelMetric(PM_IndicatorWidth, opt, widget), h);
            r = visualRect(opt->direction, opt->rect, r);
        }
        break;

    case SE_CheckBoxContents:
        {
            // Deal with the logical first, then convert it back to screen coords.
            QRect ir = visualRect(opt->direction, opt->rect,
                                  subElementRect(SE_CheckBoxIndicator, opt, widget));
            int spacing = proxy()->pixelMetric(PM_CheckBoxLabelSpacing, opt, widget);
            r.setRect(ir.right() + spacing, opt->rect.y(), opt->rect.width() - ir.width() - spacing,
                      opt->rect.height());
            r = visualRect(opt->direction, opt->rect, r);
        }
        break;

    case SE_CheckBoxFocusRect:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            if (btn->icon.isNull() && btn->text.isEmpty()) {
                r = subElementRect(SE_CheckBoxIndicator, opt, widget);
                r.adjust(1, 1, -1, -1);
                break;
            }
            // As above, deal with the logical first, then convert it back to screen coords.
            QRect cr = visualRect(btn->direction, btn->rect,
                                  subElementRect(SE_CheckBoxContents, btn, widget));

            QRect iconRect, textRect;
            if (!btn->text.isEmpty()) {
                textRect = itemTextRect(opt->fontMetrics, cr, Qt::AlignAbsolute | Qt::AlignLeft
                                        | Qt::AlignVCenter | Qt::TextShowMnemonic,
                                        btn->state & State_Enabled, btn->text);
            }
            if (!btn->icon.isNull()) {
                iconRect = itemPixmapRect(cr, Qt::AlignAbsolute | Qt::AlignLeft | Qt::AlignVCenter
                                        | Qt::TextShowMnemonic,
                                   btn->icon.pixmap(btn->iconSize, QIcon::Normal));
                if (!textRect.isEmpty())
                    textRect.translate(iconRect.right() + 4, 0);
            }
            r = iconRect | textRect;
            r.adjust(-3, -2, 3, 2);
            r = r.intersected(btn->rect);
            r = visualRect(btn->direction, btn->rect, r);
        }
        break;

    case SE_RadioButtonIndicator:
        {
            int h = proxy()->pixelMetric(PM_ExclusiveIndicatorHeight, opt, widget);
            r.setRect(opt->rect.x(), opt->rect.y() + ((opt->rect.height() - h) / 2),
                    proxy()->pixelMetric(PM_ExclusiveIndicatorWidth, opt, widget), h);
            r = visualRect(opt->direction, opt->rect, r);
        }
        break;

    case SE_RadioButtonContents:
        {
            QRect ir = visualRect(opt->direction, opt->rect,
                                  subElementRect(SE_RadioButtonIndicator, opt, widget));
            int spacing = proxy()->pixelMetric(PM_RadioButtonLabelSpacing, opt, widget);
            r.setRect(ir.left() + ir.width() + spacing, opt->rect.y(), opt->rect.width() - ir.width() - spacing,
                      opt->rect.height());
            r = visualRect(opt->direction, opt->rect, r);
            break;
        }

    case SE_RadioButtonFocusRect:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            if (btn->icon.isNull() && btn->text.isEmpty()) {
                r = subElementRect(SE_RadioButtonIndicator, opt, widget);
                r.adjust(1, 1, -1, -1);
                break;
            }
            QRect cr = visualRect(btn->direction, btn->rect,
                                  subElementRect(SE_RadioButtonContents, opt, widget));

            QRect iconRect, textRect;
            if (!btn->text.isEmpty()){
                textRect = itemTextRect(opt->fontMetrics, cr, Qt::AlignAbsolute | Qt::AlignLeft | Qt::AlignVCenter
                                 | Qt::TextShowMnemonic, btn->state & State_Enabled, btn->text);
            }
            if (!btn->icon.isNull()) {
                iconRect = itemPixmapRect(cr, Qt::AlignAbsolute | Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                                   btn->icon.pixmap(btn->iconSize, QIcon::Normal));
                if (!textRect.isEmpty())
                    textRect.translate(iconRect.right() + 4, 0);
            }
            r = iconRect | textRect;
            r.adjust(-3, -2, 3, 2);
            r = r.intersected(btn->rect);
            r = visualRect(btn->direction, btn->rect, r);
        }
        break;
#ifndef QT_NO_SLIDER
    case SE_SliderFocusRect:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            int tickOffset = proxy()->pixelMetric(PM_SliderTickmarkOffset, slider, widget);
            int thickness  = proxy()->pixelMetric(PM_SliderControlThickness, slider, widget);
            if (slider->orientation == Qt::Horizontal)
                r.setRect(0, tickOffset - 1, slider->rect.width(), thickness + 2);
            else
                r.setRect(tickOffset - 1, 0, thickness + 2, slider->rect.height());
            r = r.intersected(slider->rect);
            r = visualRect(opt->direction, opt->rect, r);
        }
        break;
#endif // QT_NO_SLIDER
#ifndef QT_NO_PROGRESSBAR
    case SE_ProgressBarGroove:
    case SE_ProgressBarContents:
    case SE_ProgressBarLabel:
        if (const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
            int textw = 0;
            bool vertical = false;
            if (const QStyleOptionProgressBarV2 *pb2 = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(opt)) {
                vertical = (pb2->orientation == Qt::Vertical);
            }
            if (!vertical) {
                if (pb->textVisible)
                    textw = qMax(pb->fontMetrics.width(pb->text), pb->fontMetrics.width(QLatin1String("100%"))) + 6;
            }

            if ((pb->textAlignment & Qt::AlignCenter) == 0) {
                if (sr != SE_ProgressBarLabel)
                    r.setCoords(pb->rect.left(), pb->rect.top(),
                                pb->rect.right() - textw, pb->rect.bottom());
                else
                    r.setCoords(pb->rect.right() - textw, pb->rect.top(),
                                pb->rect.right(), pb->rect.bottom());
            } else {
                r = pb->rect;
            }
            r = visualRect(pb->direction, pb->rect, r);
        }
        break;
#endif // QT_NO_PROGRESSBAR
#ifdef QT3_SUPPORT
    case SE_Q3DockWindowHandleRect:
        if (const QStyleOptionQ3DockWindow *dw = qstyleoption_cast<const QStyleOptionQ3DockWindow *>(opt)) {
            if (!dw->docked || !dw->closeEnabled)
                r.setRect(0, 0, dw->rect.width(), dw->rect.height());
            else {
                if (dw->state & State_Horizontal)
                    r.setRect(0, 15, dw->rect.width(), dw->rect.height() - 15);
                else
                    r.setRect(0, 1, dw->rect.width() - 15, dw->rect.height() - 1);
            }
            r = visualRect(opt->direction, opt->rect, r);
        }
        break;
#endif // QT3_SUPPORT
#ifndef QT_NO_COMBOBOX
    case SE_ComboBoxFocusRect:
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            int margin = cb->frame ? 3 : 0;
            r.setRect(opt->rect.left() + margin, opt->rect.top() + margin,
                      opt->rect.width() - 2*margin - 16, opt->rect.height() - 2*margin);
            r = visualRect(opt->direction, opt->rect, r);
        }
        break;
#endif // QT_NO_COMBOBOX
#ifndef QT_NO_TOOLBOX
    case SE_ToolBoxTabContents:
        r = opt->rect;
        r.adjust(0, 0, -30, 0);
        break;
#endif // QT_NO_TOOLBOX
    case SE_HeaderLabel: {
        int margin = proxy()->pixelMetric(QStyle::PM_HeaderMargin, opt, widget);
        r.setRect(opt->rect.x() + margin, opt->rect.y() + margin,
                  opt->rect.width() - margin * 2, opt->rect.height() - margin * 2);

        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
            // Subtract width needed for arrow, if there is one
            if (header->sortIndicator != QStyleOptionHeader::None) {
                if (opt->state & State_Horizontal)
                    r.setWidth(r.width() - (opt->rect.height() / 2) - (margin * 2));
                else
                    r.setHeight(r.height() - (opt->rect.width() / 2) - (margin * 2));
            }
        }
        r = visualRect(opt->direction, opt->rect, r);
        break; }
    case SE_HeaderArrow: {
        int h = opt->rect.height();
        int w = opt->rect.width();
        int x = opt->rect.x();
        int y = opt->rect.y();
        int margin = proxy()->pixelMetric(QStyle::PM_HeaderMargin, opt, widget);

        if (opt->state & State_Horizontal) {
            int horiz_size = h / 2;
            r.setRect(x + w - margin * 2 - horiz_size, y + 5,
                      horiz_size, h - margin * 2 - 5);
        } else {
            int vert_size = w / 2;
            r.setRect(x + 5, y + h - margin * 2 - vert_size,
                      w - margin * 2 - 5, vert_size);
        }
        r = visualRect(opt->direction, opt->rect, r);
        break; }

    case SE_RadioButtonClickRect:
        r = subElementRect(SE_RadioButtonFocusRect, opt, widget);
        r |= subElementRect(SE_RadioButtonIndicator, opt, widget);
        break;
    case SE_CheckBoxClickRect:
        r = subElementRect(SE_CheckBoxFocusRect, opt, widget);
        r |= subElementRect(SE_CheckBoxIndicator, opt, widget);
        break;
#ifndef QT_NO_TABWIDGET
    case SE_TabWidgetTabBar:
        if (const QStyleOptionTabWidgetFrame *twf
                = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
            r.setSize(twf->tabBarSize);
            const uint alingMask = Qt::AlignLeft | Qt::AlignRight | Qt::AlignHCenter;
            switch (twf->shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                // Constrain the size now, otherwise, center could get off the page
                // This of course repeated for all the other directions
                r.setWidth(qMin(r.width(), twf->rect.width()
                                            - twf->leftCornerWidgetSize.width()
                                            - twf->rightCornerWidgetSize.width()));
                switch (proxy()->styleHint(SH_TabBar_Alignment, twf, widget) & alingMask) {
                default:
                case Qt::AlignLeft:
                    r.moveTopLeft(QPoint(twf->leftCornerWidgetSize.width(), 0));
                    break;
                case Qt::AlignHCenter:
                    r.moveTopLeft(QPoint(twf->rect.center().x() - qRound(r.width() / 2.0f)
                                         + (twf->leftCornerWidgetSize.width() / 2)
                                         - (twf->rightCornerWidgetSize.width() / 2), 0));
                    break;
                case Qt::AlignRight:
                    r.moveTopLeft(QPoint(twf->rect.width() - twf->tabBarSize.width()
                                         - twf->rightCornerWidgetSize.width(), 0));
                    break;
                }
                r = visualRect(twf->direction, twf->rect, r);
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                r.setWidth(qMin(r.width(), twf->rect.width()
                                            - twf->leftCornerWidgetSize.width()
                                            - twf->rightCornerWidgetSize.width()));
                switch (proxy()->styleHint(SH_TabBar_Alignment, twf, widget) & alingMask) {
                default:
                case Qt::AlignLeft:
                    r.moveTopLeft(QPoint(twf->leftCornerWidgetSize.width(),
                                         twf->rect.height() - twf->tabBarSize.height()));
                    break;
                case Qt::AlignHCenter:
                    r.moveTopLeft(QPoint(twf->rect.center().x() - qRound(r.width() / 2.0f)
                                         + (twf->leftCornerWidgetSize.width() / 2)
                                         - (twf->rightCornerWidgetSize.width() / 2),
                                         twf->rect.height() - twf->tabBarSize.height()));
                    break;
                case Qt::AlignRight:
                    r.moveTopLeft(QPoint(twf->rect.width() - twf->tabBarSize.width()
                                         - twf->rightCornerWidgetSize.width(),
                                         twf->rect.height() - twf->tabBarSize.height()));
                    break;
                }
                r = visualRect(twf->direction, twf->rect, r);
                break;
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                r.setHeight(qMin(r.height(), twf->rect.height()
                                            - twf->leftCornerWidgetSize.height()
                                            - twf->rightCornerWidgetSize.height()));
                switch (proxy()->styleHint(SH_TabBar_Alignment, twf, widget) & alingMask) {
                default:
                case Qt::AlignLeft:
                    r.moveTopLeft(QPoint(twf->rect.width() - twf->tabBarSize.width(),
                                         twf->leftCornerWidgetSize.height()));
                    break;
                case Qt::AlignHCenter:
                    r.moveTopLeft(QPoint(twf->rect.width() - twf->tabBarSize.width(),
                                         twf->rect.center().y() - r.height() / 2));
                    break;
                case Qt::AlignRight:
                    r.moveTopLeft(QPoint(twf->rect.width() - twf->tabBarSize.width(),
                                         twf->rect.height() - twf->tabBarSize.height()
                                         - twf->rightCornerWidgetSize.height()));
                    break;
                }
                break;
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                r.setHeight(qMin(r.height(), twf->rect.height()
                                             - twf->leftCornerWidgetSize.height()
                                             - twf->rightCornerWidgetSize.height()));
                switch (proxy()->styleHint(SH_TabBar_Alignment, twf, widget) & alingMask) {
                default:
                case Qt::AlignLeft:
                    r.moveTopLeft(QPoint(0, twf->leftCornerWidgetSize.height()));
                    break;
                case Qt::AlignHCenter:
                    r.moveTopLeft(QPoint(0, twf->rect.center().y() - r.height() / 2));
                    break;
                case Qt::AlignRight:
                    r.moveTopLeft(QPoint(0, twf->rect.height() - twf->tabBarSize.height()
                                         - twf->rightCornerWidgetSize.height()));
                    break;
                }
                break;
            }
        }
        break;
    case SE_TabWidgetTabPane:
    case SE_TabWidgetTabContents:
        if (const QStyleOptionTabWidgetFrame *twf = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
            QStyleOptionTab tabopt;
            tabopt.shape = twf->shape;
            int overlap = proxy()->pixelMetric(PM_TabBarBaseOverlap, &tabopt, widget);
            if (twf->lineWidth == 0)
                overlap = 0;
            switch (twf->shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                r = QRect(QPoint(0,qMax(twf->tabBarSize.height() - overlap, 0)),
                          QSize(twf->rect.width(), qMin(twf->rect.height() - twf->tabBarSize.height() + overlap, twf->rect.height())));
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                r = QRect(QPoint(0,0), QSize(twf->rect.width(), qMin(twf->rect.height() - twf->tabBarSize.height() + overlap, twf->rect.height())));
                break;
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                r = QRect(QPoint(0, 0), QSize(qMin(twf->rect.width() - twf->tabBarSize.width() + overlap, twf->rect.width()), twf->rect.height()));
                break;
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                r = QRect(QPoint(qMax(twf->tabBarSize.width() - overlap, 0), 0),
                          QSize(qMin(twf->rect.width() - twf->tabBarSize.width() + overlap, twf->rect.width()), twf->rect.height()));
                break;
            }
            if (sr == SE_TabWidgetTabContents && twf->lineWidth > 0)
               r.adjust(2, 2, -2, -2);
        }
        break;
    case SE_TabWidgetLeftCorner:
        if (const QStyleOptionTabWidgetFrame *twf = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
            QRect paneRect = subElementRect(SE_TabWidgetTabPane, twf, widget);
            switch (twf->shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                r = QRect(QPoint(paneRect.x(), paneRect.y() - twf->leftCornerWidgetSize.height()),
                          twf->leftCornerWidgetSize);
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                r = QRect(QPoint(paneRect.x(), paneRect.height()), twf->leftCornerWidgetSize);
               break;
            default:
               break;
            }
           r = visualRect(twf->direction, twf->rect, r);
        }
        break;
   case SE_TabWidgetRightCorner:
       if (const QStyleOptionTabWidgetFrame *twf = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
           QRect paneRect = subElementRect(SE_TabWidgetTabPane, twf, widget);
           switch (twf->shape) {
           case QTabBar::RoundedNorth:
           case QTabBar::TriangularNorth:
                r = QRect(QPoint(paneRect.width() - twf->rightCornerWidgetSize.width(),
                                 paneRect.y() - twf->rightCornerWidgetSize.height()),
                          twf->rightCornerWidgetSize);
               break;
           case QTabBar::RoundedSouth:
           case QTabBar::TriangularSouth:
                r = QRect(QPoint(paneRect.width() - twf->rightCornerWidgetSize.width(),
                                 paneRect.height()), twf->rightCornerWidgetSize);
               break;
           default:
               break;
           }
           r = visualRect(twf->direction, twf->rect, r);
        }
        break;
    case SE_TabBarTabText:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            QStyleOptionTabV3 tabV3(*tab);
            QRect dummyIconRect;
            d->tabLayout(&tabV3, widget, &r, &dummyIconRect);
        }
        break;
    case SE_TabBarTabLeftButton:
    case SE_TabBarTabRightButton:
        if (const QStyleOptionTabV3 *tab = qstyleoption_cast<const QStyleOptionTabV3 *>(opt)) {
            bool selected = tab->state & State_Selected;
            int verticalShift = proxy()->pixelMetric(QStyle::PM_TabBarTabShiftVertical, tab, widget);
            int horizontalShift = proxy()->pixelMetric(QStyle::PM_TabBarTabShiftHorizontal, tab, widget);
            int hpadding = proxy()->pixelMetric(QStyle::PM_TabBarTabHSpace, opt, widget) / 2;
            hpadding = qMax(hpadding, 4); //workaround KStyle returning 0 because they workaround an old bug in Qt

            bool verticalTabs = tab->shape == QTabBar::RoundedEast
                    || tab->shape == QTabBar::RoundedWest
                    || tab->shape == QTabBar::TriangularEast
                    || tab->shape == QTabBar::TriangularWest;

            QRect tr = tab->rect;
            if (tab->shape == QTabBar::RoundedSouth || tab->shape == QTabBar::TriangularSouth)
                verticalShift = -verticalShift;
            if (verticalTabs) {
                qSwap(horizontalShift, verticalShift);
                horizontalShift *= -1;
                verticalShift *= -1;
            }
            if (tab->shape == QTabBar::RoundedWest || tab->shape == QTabBar::TriangularWest)
                horizontalShift = -horizontalShift;

            tr.adjust(0, 0, horizontalShift, verticalShift);
            if (selected)
            {
                tr.setBottom(tr.bottom() - verticalShift);
                tr.setRight(tr.right() - horizontalShift);
            }

            QSize size = (sr == SE_TabBarTabLeftButton) ? tab->leftButtonSize : tab->rightButtonSize;
            int w = size.width();
            int h = size.height();
            int midHeight = static_cast<int>(qCeil(float(tr.height() - h) / 2));
            int midWidth = ((tr.width() - w) / 2);

            bool atTheTop = true;
            switch (tab->shape) {
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                atTheTop = (sr == SE_TabBarTabLeftButton);
                break;
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                atTheTop = (sr == SE_TabBarTabRightButton);
                break;
            default:
                if (sr == SE_TabBarTabLeftButton)
                    r = QRect(tab->rect.x() + hpadding, midHeight, w, h);
                else
                    r = QRect(tab->rect.right() - w - hpadding, midHeight, w, h);
                r = visualRect(tab->direction, tab->rect, r);
            }
            if (verticalTabs) {
                if (atTheTop)
                    r = QRect(midWidth, tr.y() + tab->rect.height() - hpadding - h, w, h);
                else
                    r = QRect(midWidth, tr.y() + hpadding, w, h);
            }
        }

        break;
#endif // QT_NO_TABWIDGET
#ifndef QT_NO_TABBAR
    case SE_TabBarTearIndicator:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            switch (tab->shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                r.setRect(tab->rect.left(), tab->rect.top(), 4, opt->rect.height());
                break;
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                r.setRect(tab->rect.left(), tab->rect.top(), opt->rect.width(), 4);
                break;
            default:
                break;
            }
            r = visualRect(opt->direction, opt->rect, r);
        }
        break;
#endif
    case SE_TreeViewDisclosureItem:
        r = opt->rect;
        break;
    case SE_LineEditContents:
        if (const QStyleOptionFrame *f = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            r = f->rect.adjusted(f->lineWidth, f->lineWidth, -f->lineWidth, -f->lineWidth);
            r = visualRect(opt->direction, opt->rect, r);
        }
        break;
    case SE_FrameContents:
        if (const QStyleOptionFrameV2 *f = qstyleoption_cast<const QStyleOptionFrameV2 *>(opt)) {
            int fw = proxy()->pixelMetric(PM_DefaultFrameWidth, f, widget);
            r = opt->rect.adjusted(fw, fw, -fw, -fw);
            r = visualRect(opt->direction, opt->rect, r);
        }
        break;
    case SE_ShapedFrameContents:
        if (const QStyleOptionFrameV3 *f = qstyleoption_cast<const QStyleOptionFrameV3 *>(opt)) {
            int frameShape  = f->frameShape;
            int frameShadow = QFrame::Plain;
            if (f->state & QStyle::State_Sunken) {
                frameShadow = QFrame::Sunken;
            } else if (f->state & QStyle::State_Raised) {
                frameShadow = QFrame::Raised;
            }

            int frameWidth = 0;

            switch (frameShape) {
            case QFrame::NoFrame:
                frameWidth = 0;
                break;

            case QFrame::Box:
            case QFrame::HLine:
            case QFrame::VLine:
                switch (frameShadow) {
                case QFrame::Plain:
                    frameWidth = f->lineWidth;
                    break;
                case QFrame::Raised:
                case QFrame::Sunken:
                    frameWidth = (short)(f->lineWidth*2 + f->midLineWidth);
                    break;
                }
                break;

            case QFrame::StyledPanel:
                //keep the compatibility with Qt 4.4 if there is a proxy style.
                //be sure to call drawPrimitive(QStyle::SE_FrameContents) on the proxy style
                if (widget)
                    return widget->style()->subElementRect(QStyle::SE_FrameContents, opt, widget);
                else
                    return subElementRect(QStyle::SE_FrameContents, opt, widget);
                break;

            case QFrame::WinPanel:
                frameWidth = 2;
                break;

            case QFrame::Panel:
                switch (frameShadow) {
                case QFrame::Plain:
                case QFrame::Raised:
                case QFrame::Sunken:
                    frameWidth = f->lineWidth;
                    break;
                }
                break;
            }
            r = f->rect.adjusted(frameWidth, frameWidth, -frameWidth, -frameWidth);
        }
        break;
#ifndef QT_NO_DOCKWIDGET
    case SE_DockWidgetCloseButton:
    case SE_DockWidgetFloatButton:
    case SE_DockWidgetTitleBarText:
    case SE_DockWidgetIcon: {
        int iconSize = proxy()->pixelMetric(PM_SmallIconSize, opt, widget);
        int buttonMargin = proxy()->pixelMetric(PM_DockWidgetTitleBarButtonMargin, opt, widget);
        int margin = proxy()->pixelMetric(QStyle::PM_DockWidgetTitleMargin, opt, widget);
        QRect rect = opt->rect;

        const QStyleOptionDockWidget *dwOpt
            = qstyleoption_cast<const QStyleOptionDockWidget*>(opt);
        bool canClose = dwOpt == 0 ? true : dwOpt->closable;
        bool canFloat = dwOpt == 0 ? false : dwOpt->floatable;
        const QStyleOptionDockWidgetV2 *v2
            = qstyleoption_cast<const QStyleOptionDockWidgetV2*>(opt);
        bool verticalTitleBar = v2 == 0 ? false : v2->verticalTitleBar;

        // If this is a vertical titlebar, we transpose and work as if it was
        // horizontal, then transpose again.

        if (verticalTitleBar) {
            QSize size = rect.size();
            size.transpose();
            rect.setSize(size);
        }

        do {

            int right = rect.right();
            int left = rect.left();

            QRect closeRect;
            if (canClose) {
                QSize sz = standardIcon(QStyle::SP_TitleBarCloseButton,
                                        opt, widget).actualSize(QSize(iconSize, iconSize));
                sz += QSize(buttonMargin, buttonMargin);
                if (verticalTitleBar)
                    sz.transpose();
                closeRect = QRect(right - sz.width(),
                                    rect.center().y() - sz.height()/2,
                                    sz.width(), sz.height());
                right = closeRect.left() - 1;
            }
            if (sr == SE_DockWidgetCloseButton) {
                r = closeRect;
                break;
            }

            QRect floatRect;
            if (canFloat) {
                QSize sz = standardIcon(QStyle::SP_TitleBarNormalButton,
                                        opt, widget).actualSize(QSize(iconSize, iconSize));
                sz += QSize(buttonMargin, buttonMargin);
                if (verticalTitleBar)
                    sz.transpose();
                floatRect = QRect(right - sz.width(),
                                    rect.center().y() - sz.height()/2,
                                    sz.width(), sz.height());
                right = floatRect.left() - 1;
            }
            if (sr == SE_DockWidgetFloatButton) {
                r = floatRect;
                break;
            }

            QRect iconRect;
            if (const QDockWidget *dw = qobject_cast<const QDockWidget*>(widget)) {
                QIcon icon;
                if (dw->isFloating())
                    icon = dw->windowIcon();
                if (!icon.isNull()
                        && icon.cacheKey() != QApplication::windowIcon().cacheKey()) {
                    QSize sz = icon.actualSize(QSize(r.height(), r.height()));
                    if (verticalTitleBar)
                        sz.transpose();
                    iconRect = QRect(left, rect.center().y() - sz.height()/2,
                                        sz.width(), sz.height());
                    left = iconRect.right() + margin;
                }
            }
            if (sr == SE_DockWidgetIcon) {
                r = iconRect;
                break;
            }

            QRect textRect = QRect(left, rect.top(),
                                    right - left, rect.height());
            if (sr == SE_DockWidgetTitleBarText) {
                r = textRect;
                break;
            }

        } while (false);

        if (verticalTitleBar) {
            r = QRect(rect.left() + r.top() - rect.top(),
                        rect.top() + rect.right() - r.right(),
                        r.height(), r.width());
        } else {
            r = visualRect(opt->direction, rect, r);
        }
        break;
    }
#endif
#ifndef QT_NO_ITEMVIEWS
    case SE_ItemViewItemCheckIndicator:
        if (!qstyleoption_cast<const QStyleOptionViewItemV4 *>(opt)) {
            r = subElementRect(SE_CheckBoxIndicator, opt, widget);
            break;
        }
    case SE_ItemViewItemDecoration:
    case SE_ItemViewItemText:
    case SE_ItemViewItemFocusRect:
        if (const QStyleOptionViewItemV4 *vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(opt)) {
            if (!d->isViewItemCached(*vopt)) {
                d->viewItemLayout(vopt, &d->checkRect, &d->decorationRect, &d->displayRect, false);
                if (d->cachedOption) {
                    delete d->cachedOption;
                    d->cachedOption = 0;
                }
                d->cachedOption = new QStyleOptionViewItemV4(*vopt);
            }
            if (sr == SE_ViewItemCheckIndicator)
                r = d->checkRect;
            else if (sr == SE_ItemViewItemDecoration)
                r = d->decorationRect;
            else if (sr == SE_ItemViewItemText || sr == SE_ItemViewItemFocusRect)
                r = d->displayRect;
                               }
        break;
#endif //QT_NO_ITEMVIEWS
#ifndef QT_NO_TOOLBAR
    case SE_ToolBarHandle:
        if (const QStyleOptionToolBar *tbopt = qstyleoption_cast<const QStyleOptionToolBar *>(opt)) {
            if (tbopt->features & QStyleOptionToolBar::Movable) {
                ///we need to access the widget here because the style option doesn't 
                //have all the information we need (ie. the layout's margin)
                const QToolBar *tb = qobject_cast<const QToolBar*>(widget);
                const int margin = tb && tb->layout() ? tb->layout()->margin() : 2;
                const int handleExtent = pixelMetric(QStyle::PM_ToolBarHandleExtent, opt, tb);
                if (tbopt->state & QStyle::State_Horizontal) {
                    r = QRect(margin, margin, handleExtent, tbopt->rect.height() - 2*margin);
                    r = QStyle::visualRect(tbopt->direction, tbopt->rect, r);
                } else {
                    r = QRect(margin, margin, tbopt->rect.width() - 2*margin, handleExtent);
                }
            }
        }
        break;
#endif //QT_NO_TOOLBAR
    default:
        break;
    }
    return r;
}

#ifndef QT_NO_DIAL

static QPolygonF calcArrow(const QStyleOptionSlider *dial, qreal &a)
{
    int width = dial->rect.width();
    int height = dial->rect.height();
    int r = qMin(width, height) / 2;
    int currentSliderPosition = dial->upsideDown ? dial->sliderPosition : (dial->maximum - dial->sliderPosition);

    if (dial->maximum == dial->minimum)
        a = Q_PI / 2;
    else if (dial->dialWrapping)
        a = Q_PI * 3 / 2 - (currentSliderPosition - dial->minimum) * 2 * Q_PI
            / (dial->maximum - dial->minimum);
    else
        a = (Q_PI * 8 - (currentSliderPosition - dial->minimum) * 10 * Q_PI
            / (dial->maximum - dial->minimum)) / 6;

    int xc = width / 2;
    int yc = height / 2;

    int len = r - QStyleHelper::calcBigLineSize(r) - 5;
    if (len < 5)
        len = 5;
    int back = len / 2;

    QPolygonF arrow(3);
    arrow[0] = QPointF(0.5 + xc + len * qCos(a),
                       0.5 + yc - len * qSin(a));
    arrow[1] = QPointF(0.5 + xc + back * qCos(a + Q_PI * 5 / 6),
                       0.5 + yc - back * qSin(a + Q_PI * 5 / 6));
    arrow[2] = QPointF(0.5 + xc + back * qCos(a - Q_PI * 5 / 6),
                       0.5 + yc - back * qSin(a - Q_PI * 5 / 6));
    return arrow;
}

#endif // QT_NO_DIAL

/*!
  \reimp
*/
void QCommonStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                      QPainter *p, const QWidget *widget) const
{
    switch (cc) {
#ifndef QT_NO_SLIDER
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            if (slider->subControls == SC_SliderTickmarks) {
                int tickOffset = proxy()->pixelMetric(PM_SliderTickmarkOffset, slider, widget);
                int ticks = slider->tickPosition;
                int thickness = proxy()->pixelMetric(PM_SliderControlThickness, slider, widget);
                int len = proxy()->pixelMetric(PM_SliderLength, slider, widget);
                int available = proxy()->pixelMetric(PM_SliderSpaceAvailable, slider, widget);
                int interval = slider->tickInterval;
                if (interval <= 0) {
                    interval = slider->singleStep;
                    if (QStyle::sliderPositionFromValue(slider->minimum, slider->maximum, interval,
                                                        available)
                        - QStyle::sliderPositionFromValue(slider->minimum, slider->maximum,
                                                          0, available) < 3)
                        interval = slider->pageStep;
                }
                if (!interval)
                    interval = 1;
                int fudge = len / 2;
                int pos;
                // Since there is no subrect for tickmarks do a translation here.
                p->save();
                p->translate(slider->rect.x(), slider->rect.y());
                p->setPen(slider->palette.foreground().color());
                int v = slider->minimum;
                while (v <= slider->maximum + 1) {
                    if (v == slider->maximum + 1 && interval == 1)
                        break;
                    const int v_ = qMin(v, slider->maximum);
                    pos = QStyle::sliderPositionFromValue(slider->minimum, slider->maximum,
                                                          v_, available) + fudge;
                    if (slider->orientation == Qt::Horizontal) {
                        if (ticks & QSlider::TicksAbove)
                            p->drawLine(pos, 0, pos, tickOffset - 2);
                        if (ticks & QSlider::TicksBelow)
                            p->drawLine(pos, tickOffset + thickness + 1, pos,
                                        slider->rect.height()-1);
                    } else {
                        if (ticks & QSlider::TicksAbove)
                            p->drawLine(0, pos, tickOffset - 2, pos);
                        if (ticks & QSlider::TicksBelow)
                            p->drawLine(tickOffset + thickness + 1, pos,
                                        slider->rect.width()-1, pos);
                    }
                    // in the case where maximum is max int
                    int nextInterval = v + interval;
                    if (nextInterval < v)
                        break;
                    v = nextInterval;
                }
                p->restore();
            }
        }
        break;
#endif // QT_NO_SLIDER
#ifndef QT_NO_SCROLLBAR
    case CC_ScrollBar:
        if (const QStyleOptionSlider *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            // Make a copy here and reset it for each primitive.
            QStyleOptionSlider newScrollbar = *scrollbar;
            State saveFlags = scrollbar->state;

            if (scrollbar->subControls & SC_ScrollBarSubLine) {
                newScrollbar.state = saveFlags;
                newScrollbar.rect = proxy()->subControlRect(cc, &newScrollbar, SC_ScrollBarSubLine, widget);
                if (newScrollbar.rect.isValid()) {
                    if (!(scrollbar->activeSubControls & SC_ScrollBarSubLine))
                        newScrollbar.state &= ~(State_Sunken | State_MouseOver);
                    proxy()->drawControl(CE_ScrollBarSubLine, &newScrollbar, p, widget);
                }
            }
            if (scrollbar->subControls & SC_ScrollBarAddLine) {
                newScrollbar.rect = scrollbar->rect;
                newScrollbar.state = saveFlags;
                newScrollbar.rect = proxy()->subControlRect(cc, &newScrollbar, SC_ScrollBarAddLine, widget);
                if (newScrollbar.rect.isValid()) {
                    if (!(scrollbar->activeSubControls & SC_ScrollBarAddLine))
                        newScrollbar.state &= ~(State_Sunken | State_MouseOver);
                    proxy()->drawControl(CE_ScrollBarAddLine, &newScrollbar, p, widget);
                }
            }
            if (scrollbar->subControls & SC_ScrollBarSubPage) {
                newScrollbar.rect = scrollbar->rect;
                newScrollbar.state = saveFlags;
                newScrollbar.rect = proxy()->subControlRect(cc, &newScrollbar, SC_ScrollBarSubPage, widget);
                if (newScrollbar.rect.isValid()) {
                    if (!(scrollbar->activeSubControls & SC_ScrollBarSubPage))
                        newScrollbar.state &= ~(State_Sunken | State_MouseOver);
                    proxy()->drawControl(CE_ScrollBarSubPage, &newScrollbar, p, widget);
                }
            }
            if (scrollbar->subControls & SC_ScrollBarAddPage) {
                newScrollbar.rect = scrollbar->rect;
                newScrollbar.state = saveFlags;
                newScrollbar.rect = proxy()->subControlRect(cc, &newScrollbar, SC_ScrollBarAddPage, widget);
                if (newScrollbar.rect.isValid()) {
                    if (!(scrollbar->activeSubControls & SC_ScrollBarAddPage))
                        newScrollbar.state &= ~(State_Sunken | State_MouseOver);
                    proxy()->drawControl(CE_ScrollBarAddPage, &newScrollbar, p, widget);
                }
            }
            if (scrollbar->subControls & SC_ScrollBarFirst) {
                newScrollbar.rect = scrollbar->rect;
                newScrollbar.state = saveFlags;
                newScrollbar.rect = proxy()->subControlRect(cc, &newScrollbar, SC_ScrollBarFirst, widget);
                if (newScrollbar.rect.isValid()) {
                    if (!(scrollbar->activeSubControls & SC_ScrollBarFirst))
                        newScrollbar.state &= ~(State_Sunken | State_MouseOver);
                    proxy()->drawControl(CE_ScrollBarFirst, &newScrollbar, p, widget);
                }
            }
            if (scrollbar->subControls & SC_ScrollBarLast) {
                newScrollbar.rect = scrollbar->rect;
                newScrollbar.state = saveFlags;
                newScrollbar.rect = proxy()->subControlRect(cc, &newScrollbar, SC_ScrollBarLast, widget);
                if (newScrollbar.rect.isValid()) {
                    if (!(scrollbar->activeSubControls & SC_ScrollBarLast))
                        newScrollbar.state &= ~(State_Sunken | State_MouseOver);
                    proxy()->drawControl(CE_ScrollBarLast, &newScrollbar, p, widget);
                }
            }
            if (scrollbar->subControls & SC_ScrollBarSlider) {
                newScrollbar.rect = scrollbar->rect;
                newScrollbar.state = saveFlags;
                newScrollbar.rect = proxy()->subControlRect(cc, &newScrollbar, SC_ScrollBarSlider, widget);
                if (newScrollbar.rect.isValid()) {
                    if (!(scrollbar->activeSubControls & SC_ScrollBarSlider))
                        newScrollbar.state &= ~(State_Sunken | State_MouseOver);
                    proxy()->drawControl(CE_ScrollBarSlider, &newScrollbar, p, widget);

                    if (scrollbar->state & State_HasFocus) {
                        QStyleOptionFocusRect fropt;
                        fropt.QStyleOption::operator=(newScrollbar);
                        fropt.rect.setRect(newScrollbar.rect.x() + 2, newScrollbar.rect.y() + 2,
                                           newScrollbar.rect.width() - 5,
                                           newScrollbar.rect.height() - 5);
                        proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, p, widget);
                    }
                }
            }
        }
        break;
#endif // QT_NO_SCROLLBAR
#ifdef QT3_SUPPORT
    case CC_Q3ListView:
        if (const QStyleOptionQ3ListView *lv = qstyleoption_cast<const QStyleOptionQ3ListView *>(opt)) {
            if (lv->subControls & SC_Q3ListView)
                p->fillRect(lv->rect, lv->viewportPalette.brush(lv->viewportBGRole));
        }
        break;
#endif // QT3_SUPPORT
#ifndef QT_NO_SPINBOX
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(opt)) {
            QStyleOptionSpinBox copy = *sb;
            PrimitiveElement pe;

            if (sb->frame && (sb->subControls & SC_SpinBoxFrame)) {
                QRect r = proxy()->subControlRect(CC_SpinBox, sb, SC_SpinBoxFrame, widget);
                qDrawWinPanel(p, r, sb->palette, true);
            }

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
                pe = (sb->buttonSymbols == QAbstractSpinBox::PlusMinus ? PE_IndicatorSpinPlus
                      : PE_IndicatorSpinUp);

                copy.rect = proxy()->subControlRect(CC_SpinBox, sb, SC_SpinBoxUp, widget);
                proxy()->drawPrimitive(PE_PanelButtonBevel, &copy, p, widget);
                copy.rect.adjust(3, 0, -4, 0);
                proxy()->drawPrimitive(pe, &copy, p, widget);
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
                pe = (sb->buttonSymbols == QAbstractSpinBox::PlusMinus ? PE_IndicatorSpinMinus
                      : PE_IndicatorSpinDown);

                copy.rect = proxy()->subControlRect(CC_SpinBox, sb, SC_SpinBoxDown, widget);
                proxy()->drawPrimitive(PE_PanelButtonBevel, &copy, p, widget);
                copy.rect.adjust(3, 0, -4, 0);
                proxy()->drawPrimitive(pe, &copy, p, widget);
            }
        }
        break;
#endif // QT_NO_SPINBOX
#ifndef QT_NO_TOOLBUTTON
    case CC_ToolButton:
        if (const QStyleOptionToolButton *toolbutton
            = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {
            QRect button, menuarea;
            button = proxy()->subControlRect(cc, toolbutton, SC_ToolButton, widget);
            menuarea = proxy()->subControlRect(cc, toolbutton, SC_ToolButtonMenu, widget);

            State bflags = toolbutton->state & ~State_Sunken;

            if (bflags & State_AutoRaise) {
                if (!(bflags & State_MouseOver) || !(bflags & State_Enabled)) {
                    bflags &= ~State_Raised;
                }
            }
            State mflags = bflags;
            if (toolbutton->state & State_Sunken) {
                if (toolbutton->activeSubControls & SC_ToolButton)
                    bflags |= State_Sunken;
                mflags |= State_Sunken;
            }

            QStyleOption tool(0);
            tool.palette = toolbutton->palette;
            if (toolbutton->subControls & SC_ToolButton) {
                if (bflags & (State_Sunken | State_On | State_Raised)) {
                    tool.rect = button;
                    tool.state = bflags;
                    proxy()->drawPrimitive(PE_PanelButtonTool, &tool, p, widget);
                }
            }

            if (toolbutton->state & State_HasFocus) {
                QStyleOptionFocusRect fr;
                fr.QStyleOption::operator=(*toolbutton);
                fr.rect.adjust(3, 3, -3, -3);
                if (toolbutton->features & QStyleOptionToolButton::MenuButtonPopup)
                    fr.rect.adjust(0, 0, -proxy()->pixelMetric(QStyle::PM_MenuButtonIndicator,
                                                      toolbutton, widget), 0);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fr, p, widget);
            }
            QStyleOptionToolButton label = *toolbutton;
            label.state = bflags;
            int fw = proxy()->pixelMetric(PM_DefaultFrameWidth, opt, widget);
            label.rect = button.adjusted(fw, fw, -fw, -fw);
            proxy()->drawControl(CE_ToolButtonLabel, &label, p, widget);

            if (toolbutton->subControls & SC_ToolButtonMenu) {
                tool.rect = menuarea;
                tool.state = mflags;
                if (mflags & (State_Sunken | State_On | State_Raised))
                    proxy()->drawPrimitive(PE_IndicatorButtonDropDown, &tool, p, widget);
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &tool, p, widget);
            } else if (toolbutton->features & QStyleOptionToolButton::HasMenu) {
                int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, toolbutton, widget);
                QRect ir = toolbutton->rect;
                QStyleOptionToolButton newBtn = *toolbutton;
                newBtn.rect = QRect(ir.right() + 5 - mbi, ir.y() + ir.height() - mbi + 4, mbi - 6, mbi - 6);
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &newBtn, p, widget);
            }
        }
        break;
#endif // QT_NO_TOOLBUTTON
    case CC_TitleBar:
        if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(opt)) {
            QRect ir;
            if (opt->subControls & SC_TitleBarLabel) {
                QColor left = tb->palette.highlight().color();
                QColor right = tb->palette.base().color();

                QBrush fillBrush(left);
                if (left != right) {
                    QPoint p1(tb->rect.x(), tb->rect.top() + tb->rect.height()/2);
                    QPoint p2(tb->rect.right(), tb->rect.top() + tb->rect.height()/2);
                    QLinearGradient lg(p1, p2);
                    lg.setColorAt(0, left);
                    lg.setColorAt(1, right);
                    fillBrush = lg;
                }

                p->fillRect(opt->rect, fillBrush);

                ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarLabel, widget);

                p->setPen(tb->palette.highlightedText().color());
                p->drawText(ir.x() + 2, ir.y(), ir.width() - 2, ir.height(),
                            Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, tb->text);
            }

            bool down = false;
            QPixmap pm;

            QStyleOption tool(0);
            tool.palette = tb->palette;
            if (tb->subControls & SC_TitleBarCloseButton && tb->titleBarFlags & Qt::WindowSystemMenuHint) {
                ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarCloseButton, widget);
                down = tb->activeSubControls & SC_TitleBarCloseButton && (opt->state & State_Sunken);
                if ((tb->titleBarFlags & Qt::WindowType_Mask) == Qt::Tool
#ifndef QT_NO_DOCKWIDGET
                    || qobject_cast<const QDockWidget *>(widget)
#endif
                    )
                    pm = standardIcon(SP_DockWidgetCloseButton, &tool, widget).pixmap(10, 10);
                else
                    pm = standardIcon(SP_TitleBarCloseButton, &tool, widget).pixmap(10, 10);
                tool.rect = ir;
                tool.state = down ? State_Sunken : State_Raised;
                proxy()->drawPrimitive(PE_PanelButtonTool, &tool, p, widget);

                p->save();
                if (down)
                    p->translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, tb, widget),
                                 proxy()->pixelMetric(PM_ButtonShiftVertical, tb, widget));
                proxy()->drawItemPixmap(p, ir, Qt::AlignCenter, pm);
                p->restore();
            }

            if (tb->subControls & SC_TitleBarMaxButton
                    && tb->titleBarFlags & Qt::WindowMaximizeButtonHint
                    && !(tb->titleBarState & Qt::WindowMaximized)) {
                ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarMaxButton, widget);

                down = tb->activeSubControls & SC_TitleBarMaxButton && (opt->state & State_Sunken);
                pm = standardIcon(SP_TitleBarMaxButton, &tool, widget).pixmap(10, 10);
                tool.rect = ir;
                tool.state = down ? State_Sunken : State_Raised;
                proxy()->drawPrimitive(PE_PanelButtonTool, &tool, p, widget);

                p->save();
                if (down)
                    p->translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, tb, widget),
                                 proxy()->pixelMetric(PM_ButtonShiftVertical, tb, widget));
                proxy()->drawItemPixmap(p, ir, Qt::AlignCenter, pm);
                p->restore();
            }

            if (tb->subControls & SC_TitleBarMinButton
                    && tb->titleBarFlags & Qt::WindowMinimizeButtonHint
                    && !(tb->titleBarState & Qt::WindowMinimized)) {
                ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarMinButton, widget);
                down = tb->activeSubControls & SC_TitleBarMinButton && (opt->state & State_Sunken);
                pm = standardIcon(SP_TitleBarMinButton, &tool, widget).pixmap(10, 10);
                tool.rect = ir;
                tool.state = down ? State_Sunken : State_Raised;
                proxy()->drawPrimitive(PE_PanelButtonTool, &tool, p, widget);

                p->save();
                if (down)
                    p->translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, tb, widget),
                                 proxy()->pixelMetric(PM_ButtonShiftVertical, tb, widget));
                proxy()->drawItemPixmap(p, ir, Qt::AlignCenter, pm);
                p->restore();
            }

            bool drawNormalButton = (tb->subControls & SC_TitleBarNormalButton)
                                    && (((tb->titleBarFlags & Qt::WindowMinimizeButtonHint)
                                    && (tb->titleBarState & Qt::WindowMinimized))
                                    || ((tb->titleBarFlags & Qt::WindowMaximizeButtonHint)
                                    && (tb->titleBarState & Qt::WindowMaximized)));

            if (drawNormalButton) {
                ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarNormalButton, widget);
                down = tb->activeSubControls & SC_TitleBarNormalButton && (opt->state & State_Sunken);
                pm = standardIcon(SP_TitleBarNormalButton, &tool, widget).pixmap(10, 10);
                tool.rect = ir;
                tool.state = down ? State_Sunken : State_Raised;
                proxy()->drawPrimitive(PE_PanelButtonTool, &tool, p, widget);

                p->save();
                if (down)
                    p->translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, tb, widget),
                                 proxy()->pixelMetric(PM_ButtonShiftVertical, tb, widget));
                proxy()->drawItemPixmap(p, ir, Qt::AlignCenter, pm);
                p->restore();
            }

            if (tb->subControls & SC_TitleBarShadeButton
                    && tb->titleBarFlags & Qt::WindowShadeButtonHint
                    && !(tb->titleBarState & Qt::WindowMinimized)) {
                ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarShadeButton, widget);
                down = (tb->activeSubControls & SC_TitleBarShadeButton && (opt->state & State_Sunken));
                pm = standardIcon(SP_TitleBarShadeButton, &tool, widget).pixmap(10, 10);
                tool.rect = ir;
                tool.state = down ? State_Sunken : State_Raised;
                proxy()->drawPrimitive(PE_PanelButtonTool, &tool, p, widget);
                p->save();
                if (down)
                    p->translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, tb, widget),
                                 proxy()->pixelMetric(PM_ButtonShiftVertical, tb, widget));
                proxy()->drawItemPixmap(p, ir, Qt::AlignCenter, pm);
                p->restore();
            }

            if (tb->subControls & SC_TitleBarUnshadeButton
                    && tb->titleBarFlags & Qt::WindowShadeButtonHint
                    && tb->titleBarState & Qt::WindowMinimized) {
                ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarUnshadeButton, widget);

                down = tb->activeSubControls & SC_TitleBarUnshadeButton  && (opt->state & State_Sunken);
                pm = standardIcon(SP_TitleBarUnshadeButton, &tool, widget).pixmap(10, 10);
                tool.rect = ir;
                tool.state = down ? State_Sunken : State_Raised;
                proxy()->drawPrimitive(PE_PanelButtonTool, &tool, p, widget);
                p->save();
                if (down)
                    p->translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, tb, widget),
                                 proxy()->pixelMetric(PM_ButtonShiftVertical, tb, widget));
                proxy()->drawItemPixmap(p, ir, Qt::AlignCenter, pm);
                p->restore();
            }
            if (tb->subControls & SC_TitleBarContextHelpButton
                    && tb->titleBarFlags & Qt::WindowContextHelpButtonHint) {
                ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarContextHelpButton, widget);

                down = tb->activeSubControls & SC_TitleBarContextHelpButton  && (opt->state & State_Sunken);
                pm = standardIcon(SP_TitleBarContextHelpButton, &tool, widget).pixmap(10, 10);
                tool.rect = ir;
                tool.state = down ? State_Sunken : State_Raised;
                proxy()->drawPrimitive(PE_PanelButtonTool, &tool, p, widget);
                p->save();
                if (down)
                    p->translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, tb, widget),
                                 proxy()->pixelMetric(PM_ButtonShiftVertical, tb, widget));
                proxy()->drawItemPixmap(p, ir, Qt::AlignCenter, pm);
                p->restore();
            }
            if (tb->subControls & SC_TitleBarSysMenu && tb->titleBarFlags & Qt::WindowSystemMenuHint) {
                ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarSysMenu, widget);
                if (!tb->icon.isNull()) {
                    tb->icon.paint(p, ir);
                } else {
                    int iconSize = proxy()->pixelMetric(PM_SmallIconSize, tb, widget);
                    pm = standardIcon(SP_TitleBarMenuButton, &tool, widget).pixmap(iconSize, iconSize);
                    tool.rect = ir;
                    p->save();
                    proxy()->drawItemPixmap(p, ir, Qt::AlignCenter, pm);
                    p->restore();
                }
            }
        }
        break;
#ifndef QT_NO_DIAL
    case CC_Dial:
        if (const QStyleOptionSlider *dial = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            // OK, this is more a port of things over
            p->save();

            // avoid dithering
            if (p->paintEngine()->hasFeature(QPaintEngine::Antialiasing))
                p->setRenderHint(QPainter::Antialiasing);

            int width = dial->rect.width();
            int height = dial->rect.height();
            qreal r = qMin(width, height) / 2;
            qreal d_ = r / 6;
            qreal dx = dial->rect.x() + d_ + (width - 2 * r) / 2 + 1;
            qreal dy = dial->rect.y() + d_ + (height - 2 * r) / 2 + 1;
            QRect br = QRect(int(dx), int(dy), int(r * 2 - 2 * d_ - 2), int(r * 2 - 2 * d_ - 2));

            QPalette pal = opt->palette;
            // draw notches
            if (dial->subControls & QStyle::SC_DialTickmarks) {
                p->setPen(pal.foreground().color());
                p->drawLines(QStyleHelper::calcLines(dial));
            }

            if (dial->state & State_Enabled) {
                p->setBrush(pal.brush(QPalette::ColorRole(proxy()->styleHint(SH_Dial_BackgroundRole,
                                                                    dial, widget))));
                p->setPen(Qt::NoPen);
                p->drawEllipse(br);
                p->setBrush(Qt::NoBrush);
            }
            p->setPen(QPen(pal.dark().color()));
            p->drawArc(br, 60 * 16, 180 * 16);
            p->setPen(QPen(pal.light().color()));
            p->drawArc(br, 240 * 16, 180 * 16);

            qreal a;
            QPolygonF arrow(calcArrow(dial, a));

            p->setPen(Qt::NoPen);
            p->setBrush(pal.button());
            p->drawPolygon(arrow);

            a = QStyleHelper::angle(QPointF(width / 2, height / 2), arrow[0]);
            p->setBrush(Qt::NoBrush);

            if (a <= 0 || a > 200) {
                p->setPen(pal.light().color());
                p->drawLine(arrow[2], arrow[0]);
                p->drawLine(arrow[1], arrow[2]);
                p->setPen(pal.dark().color());
                p->drawLine(arrow[0], arrow[1]);
            } else if (a > 0 && a < 45) {
                p->setPen(pal.light().color());
                p->drawLine(arrow[2], arrow[0]);
                p->setPen(pal.dark().color());
                p->drawLine(arrow[1], arrow[2]);
                p->drawLine(arrow[0], arrow[1]);
            } else if (a >= 45 && a < 135) {
                p->setPen(pal.dark().color());
                p->drawLine(arrow[2], arrow[0]);
                p->drawLine(arrow[1], arrow[2]);
                p->setPen(pal.light().color());
                p->drawLine(arrow[0], arrow[1]);
            } else if (a >= 135 && a < 200) {
                p->setPen(pal.dark().color());
                p->drawLine(arrow[2], arrow[0]);
                p->setPen(pal.light().color());
                p->drawLine(arrow[0], arrow[1]);
                p->drawLine(arrow[1], arrow[2]);
            }

            // draw focus rect around the dial
            QStyleOptionFocusRect fropt;
            fropt.rect = dial->rect;
            fropt.state = dial->state;
            fropt.palette = dial->palette;
            if (fropt.state & QStyle::State_HasFocus) {
                br.adjust(0, 0, 2, 2);
                if (dial->subControls & SC_DialTickmarks) {
                    int r = qMin(width, height) / 2;
                    br.translate(-r / 6, - r / 6);
                    br.setWidth(br.width() + r / 3);
                    br.setHeight(br.height() + r / 3);
                }
                fropt.rect = br.adjusted(-2, -2, 2, 2);
                proxy()->drawPrimitive(QStyle::PE_FrameFocusRect, &fropt, p, widget);
            }
            p->restore();
        }
        break;
#endif // QT_NO_DIAL
#ifndef QT_NO_GROUPBOX
    case CC_GroupBox:
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(opt)) {
            // Draw frame
            QRect textRect = proxy()->subControlRect(CC_GroupBox, opt, SC_GroupBoxLabel, widget);
            QRect checkBoxRect = proxy()->subControlRect(CC_GroupBox, opt, SC_GroupBoxCheckBox, widget);
            if (groupBox->subControls & QStyle::SC_GroupBoxFrame) {
                QStyleOptionFrameV2 frame;
                frame.QStyleOption::operator=(*groupBox);
                frame.features = groupBox->features;
                frame.lineWidth = groupBox->lineWidth;
                frame.midLineWidth = groupBox->midLineWidth;
                frame.rect = proxy()->subControlRect(CC_GroupBox, opt, SC_GroupBoxFrame, widget);
                p->save();
                QRegion region(groupBox->rect);
                if (!groupBox->text.isEmpty()) {
                    bool ltr = groupBox->direction == Qt::LeftToRight;
                    QRect finalRect;
                    if (groupBox->subControls & QStyle::SC_GroupBoxCheckBox) {
                        finalRect = checkBoxRect.united(textRect);
                        finalRect.adjust(ltr ? -4 : 0, 0, ltr ? 0 : 4, 0);
                    } else {
                        finalRect = textRect;
                    }
                    region -= finalRect;
                }
                p->setClipRegion(region);
                proxy()->drawPrimitive(PE_FrameGroupBox, &frame, p, widget);
                p->restore();
            }

            // Draw title
            if ((groupBox->subControls & QStyle::SC_GroupBoxLabel) && !groupBox->text.isEmpty()) {
                QColor textColor = groupBox->textColor;
                if (textColor.isValid())
                    p->setPen(textColor);
                int alignment = int(groupBox->textAlignment);
                if (!proxy()->styleHint(QStyle::SH_UnderlineShortcut, opt, widget))
                    alignment |= Qt::TextHideMnemonic;

                proxy()->drawItemText(p, textRect,  Qt::TextShowMnemonic | Qt::AlignHCenter | alignment,
                             groupBox->palette, groupBox->state & State_Enabled, groupBox->text,
                             textColor.isValid() ? QPalette::NoRole : QPalette::WindowText);

                if (groupBox->state & State_HasFocus) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*groupBox);
                    fropt.rect = textRect;
                    proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, p, widget);
                }
            }

            // Draw checkbox
            if (groupBox->subControls & SC_GroupBoxCheckBox) {
                QStyleOptionButton box;
                box.QStyleOption::operator=(*groupBox);
                box.rect = checkBoxRect;
                proxy()->drawPrimitive(PE_IndicatorCheckBox, &box, p, widget);
            }
        }
        break;
#endif // QT_NO_GROUPBOX
#ifndef QT_NO_WORKSPACE
    case CC_MdiControls:
        {
            QStyleOptionButton btnOpt;
            btnOpt.QStyleOption::operator=(*opt);
            btnOpt.state &= ~State_MouseOver;
            int bsx = 0;
            int bsy = 0;
            if (opt->subControls & QStyle::SC_MdiCloseButton) {
                if (opt->activeSubControls & QStyle::SC_MdiCloseButton && (opt->state & State_Sunken)) {
                    btnOpt.state |= State_Sunken;
                    btnOpt.state &= ~State_Raised;
                    bsx = proxy()->pixelMetric(PM_ButtonShiftHorizontal);
                    bsy = proxy()->pixelMetric(PM_ButtonShiftVertical);
                } else {
                    btnOpt.state |= State_Raised;
                    btnOpt.state &= ~State_Sunken;
                    bsx = 0;
                    bsy = 0;
                }
                btnOpt.rect = proxy()->subControlRect(CC_MdiControls, opt, SC_MdiCloseButton, widget);
                proxy()->drawPrimitive(PE_PanelButtonCommand, &btnOpt, p, widget);
                QPixmap pm = standardIcon(SP_TitleBarCloseButton).pixmap(16, 16);
                proxy()->drawItemPixmap(p, btnOpt.rect.translated(bsx, bsy), Qt::AlignCenter, pm);
            }
            if (opt->subControls & QStyle::SC_MdiNormalButton) {
                if (opt->activeSubControls & QStyle::SC_MdiNormalButton && (opt->state & State_Sunken)) {
                    btnOpt.state |= State_Sunken;
                    btnOpt.state &= ~State_Raised;
                    bsx = proxy()->pixelMetric(PM_ButtonShiftHorizontal);
                    bsy = proxy()->pixelMetric(PM_ButtonShiftVertical);
                } else {
                    btnOpt.state |= State_Raised;
                    btnOpt.state &= ~State_Sunken;
                    bsx = 0;
                    bsy = 0;
                }
                btnOpt.rect = proxy()->subControlRect(CC_MdiControls, opt, SC_MdiNormalButton, widget);
                proxy()->drawPrimitive(PE_PanelButtonCommand, &btnOpt, p, widget);
                QPixmap pm = standardIcon(SP_TitleBarNormalButton).pixmap(16, 16);
                proxy()->drawItemPixmap(p, btnOpt.rect.translated(bsx, bsy), Qt::AlignCenter, pm);
            }
            if (opt->subControls & QStyle::SC_MdiMinButton) {
                if (opt->activeSubControls & QStyle::SC_MdiMinButton && (opt->state & State_Sunken)) {
                    btnOpt.state |= State_Sunken;
                    btnOpt.state &= ~State_Raised;
                    bsx = proxy()->pixelMetric(PM_ButtonShiftHorizontal);
                    bsy = proxy()->pixelMetric(PM_ButtonShiftVertical);
                } else {
                    btnOpt.state |= State_Raised;
                    btnOpt.state &= ~State_Sunken;
                    bsx = 0;
                    bsy = 0;
                }
                btnOpt.rect = proxy()->subControlRect(CC_MdiControls, opt, SC_MdiMinButton, widget);
                proxy()->drawPrimitive(PE_PanelButtonCommand, &btnOpt, p, widget);
                QPixmap pm = standardIcon(SP_TitleBarMinButton).pixmap(16, 16);
                proxy()->drawItemPixmap(p, btnOpt.rect.translated(bsx, bsy), Qt::AlignCenter, pm);
            }
        }
        break;
#endif // QT_NO_WORKSPACE

    default:
        qWarning("QCommonStyle::drawComplexControl: Control %d not handled", cc);
    }
}

/*!
    \reimp
*/
QStyle::SubControl QCommonStyle::hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                                 const QPoint &pt, const QWidget *widget) const
{
    SubControl sc = SC_None;
    switch (cc) {
#ifndef QT_NO_SLIDER
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            QRect r = proxy()->subControlRect(cc, slider, SC_SliderHandle, widget);
            if (r.isValid() && r.contains(pt)) {
                sc = SC_SliderHandle;
            } else {
                r = proxy()->subControlRect(cc, slider, SC_SliderGroove ,widget);
                if (r.isValid() && r.contains(pt))
                    sc = SC_SliderGroove;
            }
        }
        break;
#endif // QT_NO_SLIDER
#ifndef QT_NO_SCROLLBAR
    case CC_ScrollBar:
        if (const QStyleOptionSlider *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            QRect r;
            uint ctrl = SC_ScrollBarAddLine;
            while (ctrl <= SC_ScrollBarGroove) {
                r = proxy()->subControlRect(cc, scrollbar, QStyle::SubControl(ctrl), widget);
                if (r.isValid() && r.contains(pt)) {
                    sc = QStyle::SubControl(ctrl);
                    break;
                }
                ctrl <<= 1;
            }
        }
        break;
#endif // QT_NO_SCROLLBAR
#ifndef QT_NO_TOOLBUTTON
    case CC_ToolButton:
        if (const QStyleOptionToolButton *toolbutton = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {
            QRect r;
            uint ctrl = SC_ToolButton;
            while (ctrl <= SC_ToolButtonMenu) {
                r = proxy()->subControlRect(cc, toolbutton, QStyle::SubControl(ctrl), widget);
                if (r.isValid() && r.contains(pt)) {
                    sc = QStyle::SubControl(ctrl);
                    break;
                }
                ctrl <<= 1;
            }
        }
        break;
#endif // QT_NO_TOOLBUTTON
#ifdef QT3_SUPPORT
    case CC_Q3ListView:
        if (const QStyleOptionQ3ListView *lv = qstyleoption_cast<const QStyleOptionQ3ListView *>(opt)) {
            if (pt.x() >= 0 && pt.x() < lv->treeStepSize)
                sc = SC_Q3ListViewExpand;
        }
        break;
#endif // QT3_SUPPORT
#ifndef QT_NO_SPINBOX
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinbox = qstyleoption_cast<const QStyleOptionSpinBox *>(opt)) {
            QRect r;
            uint ctrl = SC_SpinBoxUp;
            while (ctrl <= SC_SpinBoxEditField) {
                r = proxy()->subControlRect(cc, spinbox, QStyle::SubControl(ctrl), widget);
                if (r.isValid() && r.contains(pt)) {
                    sc = QStyle::SubControl(ctrl);
                    break;
                }
                ctrl <<= 1;
            }
        }
        break;
#endif // QT_NO_SPINBOX
    case CC_TitleBar:
        if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(opt)) {
            QRect r;
            uint ctrl = SC_TitleBarSysMenu;

            while (ctrl <= SC_TitleBarLabel) {
                r = proxy()->subControlRect(cc, tb, QStyle::SubControl(ctrl), widget);
                if (r.isValid() && r.contains(pt)) {
                    sc = QStyle::SubControl(ctrl);
                    break;
                }
                ctrl <<= 1;
            }
        }
        break;
#ifndef QT_NO_COMBOBOX
    case CC_ComboBox:
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            QRect r;
            uint ctrl = SC_ComboBoxArrow;  // Start here and go down.
            while (ctrl > 0) {
                r = proxy()->subControlRect(cc, cb, QStyle::SubControl(ctrl), widget);
                if (r.isValid() && r.contains(pt)) {
                    sc = QStyle::SubControl(ctrl);
                    break;
                }
                ctrl >>= 1;
            }
        }
        break;
#endif // QT_NO_COMBOBOX
#ifndef QT_NO_GROUPBOX
    case CC_GroupBox:
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(opt)) {
            QRect r;
            uint ctrl = SC_GroupBoxCheckBox;
            while (ctrl <= SC_GroupBoxFrame) {
                r = proxy()->subControlRect(cc, groupBox, QStyle::SubControl(ctrl), widget);
                if (r.isValid() && r.contains(pt)) {
                    sc = QStyle::SubControl(ctrl);
                    break;
                }
                ctrl <<= 1;
            }
        }
        break;
#endif // QT_NO_GROUPBOX
    case CC_MdiControls:
        {
            QRect r;
            uint ctrl = SC_MdiMinButton;
            while (ctrl <= SC_MdiCloseButton) {
                r = proxy()->subControlRect(CC_MdiControls, opt, QStyle::SubControl(ctrl), widget);
                if (r.isValid() && r.contains(pt) && (opt->subControls & ctrl)) {
                    sc = QStyle::SubControl(ctrl);
                    return sc;
                }
                ctrl <<= 1;
            }
        }
        break;
    default:
        qWarning("QCommonStyle::hitTestComplexControl: Case %d not handled", cc);
    }
    return sc;
}

/*!
    \reimp
*/
QRect QCommonStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                                   SubControl sc, const QWidget *widget) const
{
    QRect ret;
    switch (cc) {
#ifndef QT_NO_SLIDER
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            int tickOffset = proxy()->pixelMetric(PM_SliderTickmarkOffset, slider, widget);
            int thickness = proxy()->pixelMetric(PM_SliderControlThickness, slider, widget);

            switch (sc) {
            case SC_SliderHandle: {
                int sliderPos = 0;
                int len = proxy()->pixelMetric(PM_SliderLength, slider, widget);
                bool horizontal = slider->orientation == Qt::Horizontal;
                sliderPos = sliderPositionFromValue(slider->minimum, slider->maximum,
                                                    slider->sliderPosition,
                                                    (horizontal ? slider->rect.width()
                                                                : slider->rect.height()) - len,
                                                    slider->upsideDown);
                if (horizontal)
                    ret.setRect(slider->rect.x() + sliderPos, slider->rect.y() + tickOffset, len, thickness);
                else
                    ret.setRect(slider->rect.x() + tickOffset, slider->rect.y() + sliderPos, thickness, len);
                break; }
            case SC_SliderGroove:
                if (slider->orientation == Qt::Horizontal)
                    ret.setRect(slider->rect.x(), slider->rect.y() + tickOffset,
                                slider->rect.width(), thickness);
                else
                    ret.setRect(slider->rect.x() + tickOffset, slider->rect.y(),
                                thickness, slider->rect.height());
                break;
            default:
                break;
            }
            ret = visualRect(slider->direction, slider->rect, ret);
        }
        break;
#endif // QT_NO_SLIDER
#ifndef QT_NO_SCROLLBAR
    case CC_ScrollBar:
        if (const QStyleOptionSlider *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const QRect scrollBarRect = scrollbar->rect;
            int sbextent = proxy()->pixelMetric(PM_ScrollBarExtent, scrollbar, widget);
            int maxlen = ((scrollbar->orientation == Qt::Horizontal) ?
                          scrollBarRect.width() : scrollBarRect.height()) - (sbextent * 2);
            int sliderlen;

            // calculate slider length
            if (scrollbar->maximum != scrollbar->minimum) {
                uint range = scrollbar->maximum - scrollbar->minimum;
                sliderlen = (qint64(scrollbar->pageStep) * maxlen) / (range + scrollbar->pageStep);

                int slidermin = proxy()->pixelMetric(PM_ScrollBarSliderMin, scrollbar, widget);
                if (sliderlen < slidermin || range > INT_MAX / 2)
                    sliderlen = slidermin;
                if (sliderlen > maxlen)
                    sliderlen = maxlen;
            } else {
                sliderlen = maxlen;
            }

            int sliderstart = sbextent + sliderPositionFromValue(scrollbar->minimum,
                                                                 scrollbar->maximum,
                                                                 scrollbar->sliderPosition,
                                                                 maxlen - sliderlen,
                                                                 scrollbar->upsideDown);

            switch (sc) {
            case SC_ScrollBarSubLine:            // top/left button
                if (scrollbar->orientation == Qt::Horizontal) {
                    int buttonWidth = qMin(scrollBarRect.width() / 2, sbextent);
                    ret.setRect(0, 0, buttonWidth, scrollBarRect.height());
                } else {
                    int buttonHeight = qMin(scrollBarRect.height() / 2, sbextent);
                    ret.setRect(0, 0, scrollBarRect.width(), buttonHeight);
                }
                break;
            case SC_ScrollBarAddLine:            // bottom/right button
                if (scrollbar->orientation == Qt::Horizontal) {
                    int buttonWidth = qMin(scrollBarRect.width()/2, sbextent);
                    ret.setRect(scrollBarRect.width() - buttonWidth, 0, buttonWidth, scrollBarRect.height());
                } else {
                    int buttonHeight = qMin(scrollBarRect.height()/2, sbextent);
                    ret.setRect(0, scrollBarRect.height() - buttonHeight, scrollBarRect.width(), buttonHeight);
                }
                break;
            case SC_ScrollBarSubPage:            // between top/left button and slider
                if (scrollbar->orientation == Qt::Horizontal)
                    ret.setRect(sbextent, 0, sliderstart - sbextent, scrollBarRect.height());
                else
                    ret.setRect(0, sbextent, scrollBarRect.width(), sliderstart - sbextent);
                break;
            case SC_ScrollBarAddPage:            // between bottom/right button and slider
                if (scrollbar->orientation == Qt::Horizontal)
                    ret.setRect(sliderstart + sliderlen, 0,
                                maxlen - sliderstart - sliderlen + sbextent, scrollBarRect.height());
                else
                    ret.setRect(0, sliderstart + sliderlen, scrollBarRect.width(),
                                maxlen - sliderstart - sliderlen + sbextent);
                break;
            case SC_ScrollBarGroove:
                if (scrollbar->orientation == Qt::Horizontal)
                    ret.setRect(sbextent, 0, scrollBarRect.width() - sbextent * 2,
                                scrollBarRect.height());
                else
                    ret.setRect(0, sbextent, scrollBarRect.width(),
                                scrollBarRect.height() - sbextent * 2);
                break;
            case SC_ScrollBarSlider:
                if (scrollbar->orientation == Qt::Horizontal)
                    ret.setRect(sliderstart, 0, sliderlen, scrollBarRect.height());
                else
                    ret.setRect(0, sliderstart, scrollBarRect.width(), sliderlen);
                break;
            default:
                break;
            }
            ret = visualRect(scrollbar->direction, scrollBarRect, ret);
        }
        break;
#endif // QT_NO_SCROLLBAR
#ifndef QT_NO_SPINBOX
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinbox = qstyleoption_cast<const QStyleOptionSpinBox *>(opt)) {
            QSize bs;
            int fw = spinbox->frame ? proxy()->pixelMetric(PM_SpinBoxFrameWidth, spinbox, widget) : 0;
            bs.setHeight(qMax(8, spinbox->rect.height()/2 - fw));
            // 1.6 -approximate golden mean
            bs.setWidth(qMax(16, qMin(bs.height() * 8 / 5, spinbox->rect.width() / 4)));
            bs = bs.expandedTo(QApplication::globalStrut());
            int y = fw + spinbox->rect.y();
            int x, lx, rx;
            x = spinbox->rect.x() + spinbox->rect.width() - fw - bs.width();
            lx = fw;
            rx = x - fw;
            switch (sc) {
            case SC_SpinBoxUp:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                ret = QRect(x, y, bs.width(), bs.height());
                break;
            case SC_SpinBoxDown:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();

                ret = QRect(x, y + bs.height(), bs.width(), bs.height());
                break;
            case SC_SpinBoxEditField:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons) {
                    ret = QRect(lx, fw, spinbox->rect.width() - 2*fw, spinbox->rect.height() - 2*fw);
                } else {
                    ret = QRect(lx, fw, rx, spinbox->rect.height() - 2*fw);
                }
                break;
            case SC_SpinBoxFrame:
                ret = spinbox->rect;
            default:
                break;
            }
            ret = visualRect(spinbox->direction, spinbox->rect, ret);
        }
        break;
#endif // Qt_NO_SPINBOX
#ifndef QT_NO_TOOLBUTTON
    case CC_ToolButton:
        if (const QStyleOptionToolButton *tb = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {
            int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, tb, widget);
            ret = tb->rect;
            switch (sc) {
            case SC_ToolButton:
                if ((tb->features
                     & (QStyleOptionToolButton::MenuButtonPopup | QStyleOptionToolButton::PopupDelay))
                    == QStyleOptionToolButton::MenuButtonPopup)
                    ret.adjust(0, 0, -mbi, 0);
                break;
            case SC_ToolButtonMenu:
                if ((tb->features
                     & (QStyleOptionToolButton::MenuButtonPopup | QStyleOptionToolButton::PopupDelay))
                    == QStyleOptionToolButton::MenuButtonPopup)
                    ret.adjust(ret.width() - mbi, 0, 0, 0);
                break;
            default:
                break;
            }
            ret = visualRect(tb->direction, tb->rect, ret);
        }
        break;
#endif // QT_NO_TOOLBUTTON
#ifndef QT_NO_COMBOBOX
    case CC_ComboBox:
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            int x = cb->rect.x(),
                y = cb->rect.y(),
                wi = cb->rect.width(),
                he = cb->rect.height();
            int xpos = x;
            int margin = cb->frame ? 3 : 0;
            int bmarg = cb->frame ? 2 : 0;
            xpos += wi - bmarg - 16;


            switch (sc) {
            case SC_ComboBoxFrame:
                ret = cb->rect;
                break;
            case SC_ComboBoxArrow:
                ret.setRect(xpos, y + bmarg, 16, he - 2*bmarg);
                break;
            case SC_ComboBoxEditField:
                ret.setRect(x + margin, y + margin, wi - 2 * margin - 16, he - 2 * margin);
                break;
            case SC_ComboBoxListBoxPopup:
                ret = cb->rect;
                break;
            default:
                break;
            }
            ret = visualRect(cb->direction, cb->rect, ret);
        }
        break;
#endif // QT_NO_COMBOBOX
    case CC_TitleBar:
        if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(opt)) {
            const int controlMargin = 2;
            const int controlHeight = tb->rect.height() - controlMargin *2;
            const int delta = controlHeight + controlMargin;
            int offset = 0;

            bool isMinimized = tb->titleBarState & Qt::WindowMinimized;
            bool isMaximized = tb->titleBarState & Qt::WindowMaximized;

            switch (sc) {
            case SC_TitleBarLabel:
                if (tb->titleBarFlags & (Qt::WindowTitleHint | Qt::WindowSystemMenuHint)) {
                    ret = tb->rect;
                    if (tb->titleBarFlags & Qt::WindowSystemMenuHint)
                        ret.adjust(delta, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowMinimizeButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowMaximizeButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowShadeButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                    if (tb->titleBarFlags & Qt::WindowContextHelpButtonHint)
                        ret.adjust(0, 0, -delta, 0);
                }
                break;
            case SC_TitleBarContextHelpButton:
                if (tb->titleBarFlags & Qt::WindowContextHelpButtonHint)
                    offset += delta;
            case SC_TitleBarMinButton:
                if (!isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarMinButton)
                    break;
            case SC_TitleBarNormalButton:
                if (isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    offset += delta;
                else if (isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarNormalButton)
                    break;
            case SC_TitleBarMaxButton:
                if (!isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarMaxButton)
                    break;
            case SC_TitleBarShadeButton:
                if (!isMinimized && (tb->titleBarFlags & Qt::WindowShadeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarShadeButton)
                    break;
            case SC_TitleBarUnshadeButton:
                if (isMinimized && (tb->titleBarFlags & Qt::WindowShadeButtonHint))
                    offset += delta;
                else if (sc == SC_TitleBarUnshadeButton)
                    break;
            case SC_TitleBarCloseButton:
                if (tb->titleBarFlags & Qt::WindowSystemMenuHint)
                    offset += delta;
                else if (sc == SC_TitleBarCloseButton)
                    break;
                ret.setRect(tb->rect.right() - offset, tb->rect.top() + controlMargin,
                            controlHeight, controlHeight);
                break;
            case SC_TitleBarSysMenu:
                if (tb->titleBarFlags & Qt::WindowSystemMenuHint) {
                    ret.setRect(tb->rect.left() + controlMargin, tb->rect.top() + controlMargin,
                                controlHeight, controlHeight);
                }
                break;
            default:
                break;
            }
            ret = visualRect(tb->direction, tb->rect, ret);
        }
        break;
#ifndef QT_NO_GROUPBOX
    case CC_GroupBox: {
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(opt)) {
            switch (sc) {
            case SC_GroupBoxFrame:
                // FALL THROUGH
            case SC_GroupBoxContents: {
                int topMargin = 0;
                int topHeight = 0;
                int verticalAlignment = proxy()->styleHint(SH_GroupBox_TextLabelVerticalAlignment, groupBox, widget);
                if (groupBox->text.size() || (groupBox->subControls & QStyle::SC_GroupBoxCheckBox)) {
                    topHeight = groupBox->fontMetrics.height();
                    if (verticalAlignment & Qt::AlignVCenter)
                        topMargin = topHeight / 2;
                    else if (verticalAlignment & Qt::AlignTop)
                        topMargin = topHeight;
                }

                QRect frameRect = groupBox->rect;
                frameRect.setTop(topMargin);

                if (sc == SC_GroupBoxFrame) {
                    ret = frameRect;
                    break;
                }

                int frameWidth = 0;
                if (!(widget && widget->inherits("Q3GroupBox"))
                    && ((groupBox->features & QStyleOptionFrameV2::Flat) == 0)) {
                    frameWidth = proxy()->pixelMetric(PM_DefaultFrameWidth, groupBox, widget);
                }
                ret = frameRect.adjusted(frameWidth, frameWidth + topHeight - topMargin,
                                         -frameWidth, -frameWidth);
                break;
            }
            case SC_GroupBoxCheckBox:
                // FALL THROUGH
            case SC_GroupBoxLabel: {
                QFontMetrics fontMetrics = groupBox->fontMetrics;
                int h = fontMetrics.height();
                int tw = fontMetrics.size(Qt::TextShowMnemonic, groupBox->text + QLatin1Char(' ')).width();
                int marg = (groupBox->features & QStyleOptionFrameV2::Flat) ? 0 : 8;
                ret = groupBox->rect.adjusted(marg, 0, -marg, 0);
                ret.setHeight(h);

                int indicatorWidth = proxy()->pixelMetric(PM_IndicatorWidth, opt, widget);
                int indicatorSpace = proxy()->pixelMetric(PM_CheckBoxLabelSpacing, opt, widget) - 1;
                bool hasCheckBox = groupBox->subControls & QStyle::SC_GroupBoxCheckBox;
                int checkBoxSize = hasCheckBox ? (indicatorWidth + indicatorSpace) : 0;

                // Adjusted rect for label + indicatorWidth + indicatorSpace
                QRect totalRect = alignedRect(groupBox->direction, groupBox->textAlignment,
                                              QSize(tw + checkBoxSize, h), ret);

                // Adjust totalRect if checkbox is set
                if (hasCheckBox) {
                    bool ltr = groupBox->direction == Qt::LeftToRight;
                    int left = 0;
                    // Adjust for check box
                    if (sc == SC_GroupBoxCheckBox) {
                        int indicatorHeight = proxy()->pixelMetric(PM_IndicatorHeight, opt, widget);
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
                ret = totalRect;
                break;
            }
            default:
                break;
            }
        }
        break;
    }
#endif // QT_NO_GROUPBOX
#ifndef QT_NO_WORKSPACE
    case CC_MdiControls:
    {
        int numSubControls = 0;
        if (opt->subControls & SC_MdiCloseButton)
            ++numSubControls;
        if (opt->subControls & SC_MdiMinButton)
            ++numSubControls;
        if (opt->subControls & SC_MdiNormalButton)
            ++numSubControls;
        if (numSubControls == 0)
            break;

        int buttonWidth = opt->rect.width()/ numSubControls - 1;
        int offset = 0;
        switch (sc) {
        case SC_MdiCloseButton:
            // Only one sub control, no offset needed.
            if (numSubControls == 1)
                break;
            offset += buttonWidth + 2;
            //FALL THROUGH
        case SC_MdiNormalButton:
            // No offset needed if
            // 1) There's only one sub control
            // 2) We have a close button and a normal button (offset already added in SC_MdiClose)
            if (numSubControls == 1 || (numSubControls == 2 && !(opt->subControls & SC_MdiMinButton)))
                break;
            if (opt->subControls & SC_MdiNormalButton)
                offset += buttonWidth;
            break;
        default:
            break;
        }

        // Subtract one pixel if we only have one sub control. At this point
        // buttonWidth is the actual width + 1 pixel margin, but we don't want the
        // margin when there are no other controllers.
        if (numSubControls == 1)
            --buttonWidth;
        ret = QRect(offset, 0, buttonWidth, opt->rect.height());
        break;
    }
#endif // QT_NO_WORKSPACE
     default:
        qWarning("QCommonStyle::subControlRect: Case %d not handled", cc);
    }
    return ret;
}

/*! \reimp */
int QCommonStyle::pixelMetric(PixelMetric m, const QStyleOption *opt, const QWidget *widget) const
{
    int ret;

    switch (m) {
    case PM_FocusFrameVMargin:
    case PM_FocusFrameHMargin:
        ret = 2;
        break;
    case PM_MenuBarVMargin:
    case PM_MenuBarHMargin:
        ret = 0;
        break;
    case PM_DialogButtonsSeparator:
        ret = int(QStyleHelper::dpiScaled(5.));
        break;
    case PM_DialogButtonsButtonWidth:
        ret = int(QStyleHelper::dpiScaled(70.));
        break;
    case PM_DialogButtonsButtonHeight:
        ret = int(QStyleHelper::dpiScaled(30.));
        break;
    case PM_CheckListControllerSize:
    case PM_CheckListButtonSize:
        ret = int(QStyleHelper::dpiScaled(16.));
        break;
    case PM_TitleBarHeight: {
        if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(opt)) {
            if ((tb->titleBarFlags & Qt::WindowType_Mask) == Qt::Tool) {
                ret = qMax(widget ? widget->fontMetrics().height() : opt->fontMetrics.height(), 16);
#ifndef QT_NO_DOCKWIDGET
            } else if (qobject_cast<const QDockWidget*>(widget)) {
                ret = qMax(widget->fontMetrics().height(), int(QStyleHelper::dpiScaled(13)));
#endif
            } else {
                ret = qMax(widget ? widget->fontMetrics().height() : opt->fontMetrics.height(), 18);
            }
        } else {
            ret = int(QStyleHelper::dpiScaled(18.));
        }

        break; }
    case PM_ScrollBarSliderMin:
        ret = int(QStyleHelper::dpiScaled(9.));
        break;

    case PM_ButtonMargin:
        ret = int(QStyleHelper::dpiScaled(6.));
        break;

    case PM_DockWidgetTitleBarButtonMargin:
        ret = int(QStyleHelper::dpiScaled(2.));
        break;

    case PM_ButtonDefaultIndicator:
        ret = 0;
        break;

    case PM_MenuButtonIndicator:
        ret = int(QStyleHelper::dpiScaled(12.));
        break;

    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:

    case PM_DefaultFrameWidth:
        ret = 2;
        break;

    case PM_ComboBoxFrameWidth:
    case PM_SpinBoxFrameWidth:
    case PM_MenuPanelWidth:
    case PM_TabBarBaseOverlap:
    case PM_TabBarBaseHeight:
        ret = proxy()->pixelMetric(PM_DefaultFrameWidth, opt, widget);
        break;

    case PM_MdiSubWindowFrameWidth:
        ret = int(QStyleHelper::dpiScaled(4.));
        break;

    case PM_MdiSubWindowMinimizedWidth:
        ret = int(QStyleHelper::dpiScaled(196.));
        break;

#ifndef QT_NO_SCROLLBAR
    case PM_ScrollBarExtent:
        if (const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            int s = sb->orientation == Qt::Horizontal ?
                    QApplication::globalStrut().height()
                    : QApplication::globalStrut().width();
            ret = qMax(16, s);
        } else {
            ret = int(QStyleHelper::dpiScaled(16.));
        }
        break;
#endif
    case PM_MaximumDragDistance:
        ret = -1;
        break;

#ifndef QT_NO_SLIDER
    case PM_SliderThickness:
        ret = int(QStyleHelper::dpiScaled(16.));
        break;

    case PM_SliderTickmarkOffset:
        if (const QStyleOptionSlider *sl = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            int space = (sl->orientation == Qt::Horizontal) ? sl->rect.height()
                                                            : sl->rect.width();
            int thickness = proxy()->pixelMetric(PM_SliderControlThickness, sl, widget);
            int ticks = sl->tickPosition;

            if (ticks == QSlider::TicksBothSides)
                ret = (space - thickness) / 2;
            else if (ticks == QSlider::TicksAbove)
                ret = space - thickness;
            else
                ret = 0;
        } else {
            ret = 0;
        }
        break;

    case PM_SliderSpaceAvailable:
        if (const QStyleOptionSlider *sl = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            if (sl->orientation == Qt::Horizontal)
                ret = sl->rect.width() - proxy()->pixelMetric(PM_SliderLength, sl, widget);
            else
                ret = sl->rect.height() - proxy()->pixelMetric(PM_SliderLength, sl, widget);
        } else {
            ret = 0;
        }
        break;
#endif // QT_NO_SLIDER
#ifndef QT_NO_DOCKWIDGET
    case PM_DockWidgetSeparatorExtent:
        ret = int(QStyleHelper::dpiScaled(6.));
        break;

    case PM_DockWidgetHandleExtent:
        ret = int(QStyleHelper::dpiScaled(8.));
        break;
    case PM_DockWidgetTitleMargin:
        ret = 0;
        break;
    case PM_DockWidgetFrameWidth:
        ret = 1;
        break;
#endif // QT_NO_DOCKWIDGET

    case PM_SpinBoxSliderHeight:
    case PM_MenuBarPanelWidth:
        ret = 2;
        break;

    case PM_MenuBarItemSpacing:
        ret = 0;
        break;

#ifndef QT_NO_TOOLBAR
    case PM_ToolBarFrameWidth:
        ret = 1;
        break;

    case PM_ToolBarItemMargin:
        ret = 0;
        break;

    case PM_ToolBarItemSpacing:
        ret = int(QStyleHelper::dpiScaled(4.));
        break;

    case PM_ToolBarHandleExtent:
        ret = int(QStyleHelper::dpiScaled(8.));
        break;

    case PM_ToolBarSeparatorExtent:
        ret = int(QStyleHelper::dpiScaled(6.));
        break;

    case PM_ToolBarExtensionExtent:
        ret = int(QStyleHelper::dpiScaled(12.));
        break;
#endif // QT_NO_TOOLBAR

#ifndef QT_NO_TABBAR
    case PM_TabBarTabOverlap:
        ret = 3;
        break;

    case PM_TabBarTabHSpace:
        ret = int(QStyleHelper::dpiScaled(24.));
        break;

    case PM_TabBarTabShiftHorizontal:
        ret = 0;
        break;

    case PM_TabBarTabShiftVertical:
        ret = 2;
        break;

    case PM_TabBarTabVSpace: {
        const QStyleOptionTab *tb = qstyleoption_cast<const QStyleOptionTab *>(opt);
        if (tb && (tb->shape == QTabBar::RoundedNorth || tb->shape == QTabBar::RoundedSouth
                   || tb->shape == QTabBar::RoundedWest || tb->shape == QTabBar::RoundedEast))
            ret = 8;
        else
            if(tb && (tb->shape == QTabBar::TriangularWest || tb->shape == QTabBar::TriangularEast))
                ret = 3;
            else
                ret = 2;
        break; }
#endif

    case PM_ProgressBarChunkWidth:
        ret = 9;
        break;

    case PM_IndicatorWidth:
        ret = int(QStyleHelper::dpiScaled(13.));
        break;

    case PM_IndicatorHeight:
        ret = int(QStyleHelper::dpiScaled(13.));
        break;

    case PM_ExclusiveIndicatorWidth:
        ret = int(QStyleHelper::dpiScaled(12.));
        break;

    case PM_ExclusiveIndicatorHeight:
        ret = int(QStyleHelper::dpiScaled(12.));
        break;

    case PM_MenuTearoffHeight:
        ret = int(QStyleHelper::dpiScaled(10.));
        break;

    case PM_MenuScrollerHeight:
        ret = int(QStyleHelper::dpiScaled(10.));
        break;

    case PM_MenuDesktopFrameWidth:
    case PM_MenuHMargin:
    case PM_MenuVMargin:
        ret = 0;
        break;

    case PM_HeaderMargin:
        ret = int(QStyleHelper::dpiScaled(4.));
        break;
    case PM_HeaderMarkSize:
        ret = int(QStyleHelper::dpiScaled(32.));
        break;
    case PM_HeaderGripMargin:
        ret = int(QStyleHelper::dpiScaled(4.));
        break;
    case PM_TabBarScrollButtonWidth:
        ret = int(QStyleHelper::dpiScaled(16.));
        break;
    case PM_LayoutLeftMargin:
    case PM_LayoutTopMargin:
    case PM_LayoutRightMargin:
    case PM_LayoutBottomMargin:
        {
            bool isWindow = false;
            if (opt) {
                isWindow = (opt->state & State_Window);
            } else if (widget) {
                isWindow = widget->isWindow();
            }
            ret = proxy()->pixelMetric(isWindow ? PM_DefaultTopLevelMargin : PM_DefaultChildMargin);
        }
        break;
    case PM_LayoutHorizontalSpacing:
    case PM_LayoutVerticalSpacing:
        ret = proxy()->pixelMetric(PM_DefaultLayoutSpacing);
        break;

    case PM_DefaultTopLevelMargin:
        ret = int(QStyleHelper::dpiScaled(11.));
        break;
    case PM_DefaultChildMargin:
        ret = int(QStyleHelper::dpiScaled(9.));
        break;
    case PM_DefaultLayoutSpacing:
        ret = int(QStyleHelper::dpiScaled(6.));
        break;

    case PM_ToolBarIconSize:
        ret = qt_guiPlatformPlugin()->platformHint(QGuiPlatformPlugin::PH_ToolBarIconSize);
        if (!ret)
            ret = int(QStyleHelper::dpiScaled(24.));
        break;

    case PM_TabBarIconSize:
    case PM_ListViewIconSize:
        ret = proxy()->pixelMetric(PM_SmallIconSize, opt, widget);
        break;

    case PM_ButtonIconSize:
    case PM_SmallIconSize:
        ret = int(QStyleHelper::dpiScaled(16.));
        break;
    case PM_IconViewIconSize:
        ret = proxy()->pixelMetric(PM_LargeIconSize, opt, widget);
        break;

    case PM_LargeIconSize:
        ret = int(QStyleHelper::dpiScaled(32.));
        break;

    case PM_ToolTipLabelFrameWidth:
        ret = 1;
        break;
    case PM_CheckBoxLabelSpacing:
    case PM_RadioButtonLabelSpacing:
        ret = int(QStyleHelper::dpiScaled(6.));
        break;
    case PM_SizeGripSize:
        ret = int(QStyleHelper::dpiScaled(13.));
        break;
    case PM_MessageBoxIconSize:
#ifdef Q_WS_MAC
        if (QApplication::desktopSettingsAware()) {
            ret = 64; // No DPI scaling, it's handled elsewhere.
        } else
#endif
        {
            ret = int(QStyleHelper::dpiScaled(32.));
        }
        break;
    case PM_TextCursorWidth:
        ret = 1;
        break;
    case PM_TabBar_ScrollButtonOverlap:
        ret = 1;
        break;
    case PM_TabCloseIndicatorWidth:
    case PM_TabCloseIndicatorHeight:
        ret = int(QStyleHelper::dpiScaled(16.));
        break;
    case PM_ScrollView_ScrollBarSpacing:
        ret = 2 * proxy()->pixelMetric(PM_DefaultFrameWidth, opt, widget);
        break;
    case PM_SubMenuOverlap:
        ret = -proxy()->pixelMetric(QStyle::PM_MenuPanelWidth, opt, widget);
        break;
    default:
        ret = 0;
        break;
    }

    return ret;
}

/*!
    \reimp
*/
QSize QCommonStyle::sizeFromContents(ContentsType ct, const QStyleOption *opt,
                                     const QSize &csz, const QWidget *widget) const
{
    Q_D(const QCommonStyle);
    QSize sz(csz);
    switch (ct) {
    case CT_PushButton:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            int w = csz.width(),
                h = csz.height(),
                bm = proxy()->pixelMetric(PM_ButtonMargin, btn, widget),
            fw = proxy()->pixelMetric(PM_DefaultFrameWidth, btn, widget) * 2;
            w += bm + fw;
            h += bm + fw;
            if (btn->features & QStyleOptionButton::AutoDefaultButton){
                int dbw = proxy()->pixelMetric(PM_ButtonDefaultIndicator, btn, widget) * 2;
                w += dbw;
                h += dbw;
            }
            sz = QSize(w, h);
        }
        break;
    case CT_RadioButton:
    case CT_CheckBox:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            bool isRadio = (ct == CT_RadioButton);

            int w = proxy()->pixelMetric(isRadio ? PM_ExclusiveIndicatorWidth
                                        : PM_IndicatorWidth, btn, widget);
            int h = proxy()->pixelMetric(isRadio ? PM_ExclusiveIndicatorHeight
                                        : PM_IndicatorHeight, btn, widget);

            int margins = 0;
            // we add 4 pixels for label margins
            if (!btn->icon.isNull() || !btn->text.isEmpty())
                margins = 4 + proxy()->pixelMetric(isRadio ? PM_RadioButtonLabelSpacing
                                                  : PM_CheckBoxLabelSpacing, opt, widget);
            sz += QSize(w + margins, 4);
            sz.setHeight(qMax(sz.height(), h));
        }
        break;
#ifndef QT_NO_MENU
    case CT_MenuItem:
        if (const QStyleOptionMenuItem *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            bool checkable = mi->menuHasCheckableItems;
            int maxpmw = mi->maxIconWidth;
            int w = sz.width(), h = sz.height();
            if (mi->menuItemType == QStyleOptionMenuItem::Separator) {
                w = 10;
                h = 2;
            } else {
                h =  mi->fontMetrics.height() + 8;
                if (!mi->icon.isNull()) {
                    int iconExtent = proxy()->pixelMetric(PM_SmallIconSize);
                    h = qMax(h, mi->icon.actualSize(QSize(iconExtent, iconExtent)).height() + 4);
                }
            }
            if (mi->text.contains(QLatin1Char('\t')))
                w += 12;
            if (maxpmw > 0)
                w += maxpmw + 6;
            if (checkable && maxpmw < 20)
                w += 20 - maxpmw;
            if (checkable || maxpmw > 0)
                w += 2;
            w += 12;
            sz = QSize(w, h);
        }
        break;
#endif // QT_NO_MENU
#ifndef QT_NO_TOOLBUTTON
    case CT_ToolButton:
        sz = QSize(sz.width() + 6, sz.height() + 5);
        break;
#endif // QT_NO_TOOLBUTTON
#ifndef QT_NO_COMBOBOX
    case CT_ComboBox:
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            int fw = cmb->frame ? proxy()->pixelMetric(PM_ComboBoxFrameWidth, opt, widget) * 2 : 0;
            const int textMargins = 2*(proxy()->pixelMetric(PM_FocusFrameHMargin) + 1);
            // QItemDelegate::sizeHint expands the textMargins two times, thus the 2*textMargins...
            int other = qMax(23, 2*textMargins + proxy()->pixelMetric(QStyle::PM_ScrollBarExtent, opt, widget));
            sz = QSize(sz.width() + fw + other, sz.height() + fw);
        }
        break;
#endif // QT_NO_COMBOBOX
    case CT_HeaderSection:
        if (const QStyleOptionHeader *hdr = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
            bool nullIcon = hdr->icon.isNull();
            int margin = proxy()->pixelMetric(QStyle::PM_HeaderMargin, hdr, widget);
            int iconSize = nullIcon ? 0 : proxy()->pixelMetric(QStyle::PM_SmallIconSize, hdr, widget);
            QSize txt = hdr->fontMetrics.size(0, hdr->text);
            sz.setHeight(margin + qMax(iconSize, txt.height()) + margin);
            sz.setWidth((nullIcon ? 0 : margin) + iconSize
                        + (hdr->text.isNull() ? 0 : margin) + txt.width() + margin);
        }
        break;
    case CT_TabWidget:
        sz += QSize(4, 4);
        break;
    case CT_LineEdit:
        if (const QStyleOptionFrame *f = qstyleoption_cast<const QStyleOptionFrame *>(opt))
            sz += QSize(2*f->lineWidth, 2*f->lineWidth);
        break;
#ifndef QT_NO_GROUPBOX
    case CT_GroupBox:
        if (const QGroupBox *grb = static_cast<const QGroupBox *>(widget))
            sz += QSize(!grb->isFlat() ? 16 : 0, 0);
        break;
#endif // QT_NO_GROUPBOX
    case CT_MdiControls:
        if (const QStyleOptionComplex *styleOpt = qstyleoption_cast<const QStyleOptionComplex *>(opt)) {
            int width = 1;
            if (styleOpt->subControls & SC_MdiMinButton)
                width += 16 + 1;
            if (styleOpt->subControls & SC_MdiNormalButton)
                width += 16 + 1;
            if (styleOpt->subControls & SC_MdiCloseButton)
                width += 16 + 1;
            sz = QSize(width, 16);
        } else {
            sz = QSize(52, 16);
        }
        break;
#ifndef QT_NO_ITEMVIEWS
    case CT_ItemViewItem:
        if (const QStyleOptionViewItemV4 *vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(opt)) {
            QRect decorationRect, displayRect, checkRect;
            d->viewItemLayout(vopt, &checkRect, &decorationRect, &displayRect, true);
            sz = (decorationRect|displayRect|checkRect).size();
                      }
        break;
#endif // QT_NO_ITEMVIEWS
    case CT_ScrollBar:
    case CT_MenuBar:
    case CT_Menu:
    case CT_MenuBarItem:
    case CT_Q3Header:
    case CT_Slider:
    case CT_ProgressBar:
    case CT_TabBarTab:
        // just return the contentsSize for now
        // fall through intended
    default:
        break;
    }
    return sz;
}


/*! \reimp */
int QCommonStyle::styleHint(StyleHint sh, const QStyleOption *opt, const QWidget *widget,
                            QStyleHintReturn *hret) const
{
    int ret = 0;

    switch (sh) {
    case SH_Menu_KeyboardSearch:
        ret = false;
        break;
    case SH_Slider_AbsoluteSetButtons:
        ret = Qt::MidButton;
        break;
    case SH_Slider_PageSetButtons:
        ret = Qt::LeftButton;
        break;
    case SH_ScrollBar_ContextMenu:
        ret = true;
        break;
    case SH_DialogButtons_DefaultButton:  // This value not used anywhere.
        ret = QDialogButtonBox::AcceptRole;
        break;
#ifndef QT_NO_GROUPBOX
    case SH_GroupBox_TextLabelVerticalAlignment:
        ret = Qt::AlignVCenter;
        break;

    case SH_GroupBox_TextLabelColor:
        ret = opt ? int(opt->palette.color(QPalette::Text).rgba()) : 0;
        break;
#endif // QT_NO_GROUPBOX

    case SH_Q3ListViewExpand_SelectMouseType:
    case SH_TabBar_SelectMouseType:
        ret = QEvent::MouseButtonPress;
        break;

#ifdef QT3_SUPPORT
    case SH_GUIStyle:
        ret = Qt::WindowsStyle;
        break;
#endif

    case SH_TabBar_Alignment:
    case SH_Header_ArrowAlignment:
        ret = Qt::AlignLeft;
        break;

    case SH_TitleBar_AutoRaise:
        ret = false;
        break;

    case SH_Menu_SubMenuPopupDelay:
        ret = 256;
        break;

    case SH_ProgressDialog_TextLabelAlignment:
        ret = Qt::AlignCenter;
        break;

    case SH_BlinkCursorWhenTextSelected:
        ret = 1;
        break;

    case SH_Table_GridLineColor:
        if (opt)
            ret = opt->palette.color(QPalette::Mid).rgb();
        else
            ret = -1;
        break;
    case SH_LineEdit_PasswordCharacter: {
        const QFontMetrics &fm = opt ? opt->fontMetrics
                                     : (widget ? widget->fontMetrics() : QFontMetrics(QFont()));
        ret = 0;
        if (fm.inFont(QChar(0x25CF))) {
            ret = 0x25CF;
        } else if (fm.inFont(QChar(0x2022))) {
            ret = 0x2022;
        } else {
            ret = '*';
        }
        break;
    }


    case SH_ToolBox_SelectedPageTitleBold:
        ret = 1;
        break;

    case SH_UnderlineShortcut:
        ret = 1;
        break;

    case SH_SpinBox_ClickAutoRepeatRate:
        ret = 150;
        break;

    case SH_SpinBox_ClickAutoRepeatThreshold:
        ret = 500;
        break;

    case SH_SpinBox_KeyPressAutoRepeatRate:
        ret = 75;
        break;

    case SH_Menu_SelectionWrap:
        ret = true;
        break;

    case SH_Menu_FillScreenWithScroll:
        ret = true;
        break;

    case SH_ToolTipLabel_Opacity:
        ret = 255;
        break;

    case SH_Button_FocusPolicy:
        ret = Qt::StrongFocus;
        break;

    case SH_MenuBar_DismissOnSecondClick:
        ret = 1;
        break;

    case SH_MessageBox_UseBorderForButtonSpacing:
        ret = 0;
        break;

    case SH_ToolButton_PopupDelay:
        ret = 600;
        break;

    case SH_FocusFrame_Mask:
        ret = 1;
        if (widget) {
            if(QStyleHintReturnMask *mask = qstyleoption_cast<QStyleHintReturnMask*>(hret)) {
                mask->region = widget->rect();
                int vmargin = proxy()->pixelMetric(QStyle::PM_FocusFrameVMargin),
                    hmargin = proxy()->pixelMetric(QStyle::PM_FocusFrameHMargin);
                mask->region -= QRect(widget->rect().adjusted(hmargin, vmargin, -hmargin, -vmargin));
            }
        }
        break;
#ifndef QT_NO_RUBBERBAND
    case SH_RubberBand_Mask:
        if (const QStyleOptionRubberBand *rbOpt = qstyleoption_cast<const QStyleOptionRubberBand *>(opt)) {
            ret = 0;
            if (rbOpt->shape == QRubberBand::Rectangle) {
                ret = true;
                if(QStyleHintReturnMask *mask = qstyleoption_cast<QStyleHintReturnMask*>(hret)) {
                    mask->region = opt->rect;
                    int margin = proxy()->pixelMetric(PM_DefaultFrameWidth) * 2;
                    mask->region -= opt->rect.adjusted(margin, margin, -margin, -margin);
                }
            }
        }
        break;
#endif // QT_NO_RUBBERBAND
    case SH_SpinControls_DisableOnBounds:
        ret = 1;
        break;

    case SH_Dial_BackgroundRole:
        ret = QPalette::Window;
        break;

    case SH_ComboBox_LayoutDirection:
        ret = opt ? opt->direction : Qt::LeftToRight;
        break;

    case SH_ItemView_EllipsisLocation:
        ret = Qt::AlignTrailing;
        break;

    case SH_ItemView_ShowDecorationSelected:
        ret = false;
        break;

    case SH_ItemView_ActivateItemOnSingleClick:
        ret = qt_guiPlatformPlugin()->platformHint(QGuiPlatformPlugin::PH_ItemView_ActivateItemOnSingleClick);
        break;

    case SH_TitleBar_ModifyNotification:
        ret = true;
        break;
    case SH_ScrollBar_RollBetweenButtons:
        ret = false;
        break;
    case SH_TabBar_ElideMode:
        ret = Qt::ElideNone;
        break;
    case SH_DialogButtonLayout:
        ret = QDialogButtonBox::WinLayout;
#ifdef Q_WS_X11
        if (X11->desktopEnvironment == DE_KDE)
            ret = QDialogButtonBox::KdeLayout;
        else if (X11->desktopEnvironment == DE_GNOME)
            ret = QDialogButtonBox::GnomeLayout;
#endif
        break;
    case SH_ComboBox_PopupFrameStyle:
        ret = QFrame::StyledPanel | QFrame::Plain;
        break;
    case SH_MessageBox_TextInteractionFlags:
        ret = Qt::LinksAccessibleByMouse;
        break;
    case SH_DialogButtonBox_ButtonsHaveIcons:
#ifdef Q_WS_X11
        return true;
#endif
        ret = 0;
        break;
    case SH_SpellCheckUnderlineStyle:
        ret = QTextCharFormat::WaveUnderline;
        break;
    case SH_MessageBox_CenterButtons:
        ret = true;
        break;
    case SH_ItemView_MovementWithoutUpdatingSelection:
        ret = true;
        break;
    case SH_FocusFrame_AboveWidget:
        ret = false;
        break;
#ifndef QT_NO_TABWIDGET
    case SH_TabWidget_DefaultTabPosition:
        ret = QTabWidget::North;
        break;
#endif
    case SH_ToolBar_Movable:
        ret = true;
        break;
    case SH_TextControl_FocusIndicatorTextCharFormat:
        ret = true;
        if (QStyleHintReturnVariant *vret = qstyleoption_cast<QStyleHintReturnVariant*>(hret)) {
            QPen outline(opt->palette.color(QPalette::Text), 1, Qt::DotLine);
            QTextCharFormat fmt;
            fmt.setProperty(QTextFormat::OutlinePen, outline);
            vret->variant = fmt;
        }
        break;
#ifndef QT_NO_WIZARD
    case SH_WizardStyle:
        ret = QWizard::ClassicStyle;
        break;
#endif
    case SH_FormLayoutWrapPolicy:
        ret = QFormLayout::DontWrapRows;
        break;
    case SH_FormLayoutFieldGrowthPolicy:
        ret = QFormLayout::AllNonFixedFieldsGrow;
        break;
    case SH_FormLayoutFormAlignment:
        ret = Qt::AlignLeft | Qt::AlignTop;
        break;
    case SH_FormLayoutLabelAlignment:
        ret = Qt::AlignLeft;
        break;
    case SH_ItemView_ArrowKeysNavigateIntoChildren:
        ret = false;
        break;
    case SH_ItemView_DrawDelegateFrame:
        ret = 0;
        break;
#ifndef QT_NO_TABBAR
    case SH_TabBar_CloseButtonPosition:
        ret = QTabBar::RightSide;
        break;
#endif
    case SH_DockWidget_ButtonsHaveFrame:
        ret = true;
        break;
    case SH_ToolButtonStyle:
        ret = qt_guiPlatformPlugin()->platformHint(QGuiPlatformPlugin::PH_ToolButtonStyle);
        break;
    case SH_RequestSoftwareInputPanel:
        ret = RSIP_OnMouseClickAndAlreadyFocused;
        break;
    default:
        ret = 0;
        break;
    }

    return ret;
}

/*! \reimp */
QPixmap QCommonStyle::standardPixmap(StandardPixmap sp, const QStyleOption *option,
                                     const QWidget *widget) const
{
    const bool rtl = (option && option->direction == Qt::RightToLeft) || (!option && QApplication::isRightToLeft());
#ifdef QT_NO_IMAGEFORMAT_PNG
    Q_UNUSED(widget);
    Q_UNUSED(sp);
#else
    QPixmap pixmap;

    if (QApplication::desktopSettingsAware() && !QIcon::themeName().isEmpty()) {
        switch (sp) {
        case SP_DialogYesButton:
        case SP_DialogOkButton:
            pixmap = QIcon::fromTheme(QLatin1String("dialog-ok")).pixmap(16);
            break;
        case SP_DialogApplyButton:
            pixmap = QIcon::fromTheme(QLatin1String("dialog-ok-apply")).pixmap(16);
            break;
        case SP_DialogDiscardButton:
            pixmap = QIcon::fromTheme(QLatin1String("edit-delete")).pixmap(16);
            break;
        case SP_DialogCloseButton:
            pixmap = QIcon::fromTheme(QLatin1String("dialog-close")).pixmap(16);
            break;
        case SP_DirHomeIcon:
            pixmap = QIcon::fromTheme(QLatin1String("user-home")).pixmap(16);
            break;
        case SP_MessageBoxInformation:
            pixmap = QIcon::fromTheme(QLatin1String("messagebox_info")).pixmap(16);
            break;
        case SP_MessageBoxWarning:
            pixmap = QIcon::fromTheme(QLatin1String("messagebox_warning")).pixmap(16);
            break;
        case SP_MessageBoxCritical:
            pixmap = QIcon::fromTheme(QLatin1String("messagebox_critical")).pixmap(16);
            break;
        case SP_MessageBoxQuestion:
            pixmap = QIcon::fromTheme(QLatin1String("help")).pixmap(16);
            break;
        case SP_DialogOpenButton:
        case SP_DirOpenIcon:
            pixmap = QIcon::fromTheme(QLatin1String("folder-open")).pixmap(16);
            break;
        case SP_FileIcon:
            pixmap = QIcon::fromTheme(QLatin1String("text-x-generic"),
                                      QIcon::fromTheme(QLatin1String("empty"))).pixmap(16);
            break;
        case SP_DirClosedIcon:
        case SP_DirIcon:
                pixmap = QIcon::fromTheme(QLatin1String("folder")).pixmap(16);
                break;
        case SP_DriveFDIcon:
                pixmap = QIcon::fromTheme(QLatin1String("media-floppy"),
                                          QIcon::fromTheme(QLatin1String("3floppy_unmount"))).pixmap(16);
                break;
        case SP_ComputerIcon:
                pixmap = QIcon::fromTheme(QLatin1String("computer"),
                                          QIcon::fromTheme(QLatin1String("system"))).pixmap(16);
                break;
        case SP_DesktopIcon:
                pixmap = QIcon::fromTheme(QLatin1String("user-desktop"),
                                          QIcon::fromTheme(QLatin1String("desktop"))).pixmap(16);
                break;
        case SP_TrashIcon:
                pixmap = QIcon::fromTheme(QLatin1String("user-trash"),
                                          QIcon::fromTheme(QLatin1String("trashcan_empty"))).pixmap(16);
                break;
        case SP_DriveCDIcon:
        case SP_DriveDVDIcon:
                pixmap = QIcon::fromTheme(QLatin1String("media-optical"),
                                          QIcon::fromTheme(QLatin1String("cdrom_unmount"))).pixmap(16);
                break;
        case SP_DriveHDIcon:
                pixmap = QIcon::fromTheme(QLatin1String("drive-harddisk"),
                                          QIcon::fromTheme(QLatin1String("hdd_unmount"))).pixmap(16);
                break;
        case SP_FileDialogToParent:
                pixmap = QIcon::fromTheme(QLatin1String("go-up"),
                                          QIcon::fromTheme(QLatin1String("up"))).pixmap(16);
                break;
        case SP_FileDialogNewFolder:
                pixmap = QIcon::fromTheme(QLatin1String("folder_new")).pixmap(16);
                break;
        case SP_ArrowUp:
                pixmap = QIcon::fromTheme(QLatin1String("go-up"),
                                          QIcon::fromTheme(QLatin1String("up"))).pixmap(16);
                break;
        case SP_ArrowDown:
                pixmap = QIcon::fromTheme(QLatin1String("go-down"),
                                          QIcon::fromTheme(QLatin1String("down"))).pixmap(16);
                break;
        case SP_ArrowRight:
                pixmap = QIcon::fromTheme(QLatin1String("go-next"),
                                          QIcon::fromTheme(QLatin1String("forward"))).pixmap(16);
                break;
        case SP_ArrowLeft:
                pixmap = QIcon::fromTheme(QLatin1String("go-previous"),
                                          QIcon::fromTheme(QLatin1String("back"))).pixmap(16);
                break;
        case SP_FileDialogDetailedView:
                pixmap = QIcon::fromTheme(QLatin1String("view_detailed")).pixmap(16);
                break;
        case SP_FileDialogListView:
                pixmap = QIcon::fromTheme(QLatin1String("view_icon")).pixmap(16);
                break;
        case SP_BrowserReload:
                pixmap = QIcon::fromTheme(QLatin1String("reload")).pixmap(16);
                break;
        case SP_BrowserStop:
                pixmap = QIcon::fromTheme(QLatin1String("process-stop")).pixmap(16);
                break;
        case SP_MediaPlay:
                pixmap = QIcon::fromTheme(QLatin1String("media-playback-start")).pixmap(16);
                break;
        case SP_MediaPause:
                pixmap = QIcon::fromTheme(QLatin1String("media-playback-pause")).pixmap(16);
                break;
        case SP_MediaStop:
                pixmap = QIcon::fromTheme(QLatin1String("media-playback-stop")).pixmap(16);
                break;
        case SP_MediaSeekForward:
                pixmap = QIcon::fromTheme(QLatin1String("media-seek-forward")).pixmap(16);
                break;
        case SP_MediaSeekBackward:
                pixmap = QIcon::fromTheme(QLatin1String("media-seek-backward")).pixmap(16);
                break;
        case SP_MediaSkipForward:
                pixmap = QIcon::fromTheme(QLatin1String("media-skip-forward")).pixmap(16);
                break;
        case SP_MediaSkipBackward:
                pixmap = QIcon::fromTheme(QLatin1String("media-skip-backward")).pixmap(16);
                break;
        case SP_DialogResetButton:
                pixmap = QIcon::fromTheme(QLatin1String("edit-clear")).pixmap(24);
                break;
        case SP_DialogHelpButton:
                pixmap = QIcon::fromTheme(QLatin1String("help-contents")).pixmap(24);
                break;
        case SP_DialogNoButton:
        case SP_DialogCancelButton:
                pixmap = QIcon::fromTheme(QLatin1String("dialog-cancel"),
                                         QIcon::fromTheme(QLatin1String("process-stop"))).pixmap(24);
                break;
        case SP_DialogSaveButton:
                pixmap = QIcon::fromTheme(QLatin1String("document-save")).pixmap(24);
                break;
        case SP_FileLinkIcon:
            pixmap = QIcon::fromTheme(QLatin1String("emblem-symbolic-link")).pixmap(16);
            if (!pixmap.isNull()) {
                QPixmap fileIcon = QIcon::fromTheme(QLatin1String("text-x-generic")).pixmap(16);
                if (fileIcon.isNull())
                    fileIcon = QIcon::fromTheme(QLatin1String("empty")).pixmap(16);
                if (!fileIcon.isNull()) {
                    QPainter painter(&fileIcon);
                    painter.drawPixmap(0, 0, 16, 16, pixmap);
                    return fileIcon;
                }
            }
            break;
        case SP_DirLinkIcon:
                pixmap = QIcon::fromTheme(QLatin1String("emblem-symbolic-link")).pixmap(16);
                if (!pixmap.isNull()) {
                    QPixmap dirIcon = QIcon::fromTheme(QLatin1String("folder")).pixmap(16);
                    if (!dirIcon.isNull()) {
                        QPainter painter(&dirIcon);
                        painter.drawPixmap(0, 0, 16, 16, pixmap);
                        return dirIcon;
                    }
                }
                break;
        default:
            break;
        }
    }

    if (!pixmap.isNull())
        return pixmap;
#endif //QT_NO_IMAGEFORMAT_PNG
    switch (sp) {
#ifndef QT_NO_IMAGEFORMAT_XPM
    case SP_ToolBarHorizontalExtensionButton:
        if (rtl) {
            QImage im(tb_extension_arrow_h_xpm);
            im = im.convertToFormat(QImage::Format_ARGB32).mirrored(true, false);
            return QPixmap::fromImage(im);
        }
        return QPixmap(tb_extension_arrow_h_xpm);
    case SP_ToolBarVerticalExtensionButton:
        return QPixmap(tb_extension_arrow_v_xpm);
    case SP_FileDialogStart:
        return QPixmap(filedialog_start_xpm);
    case SP_FileDialogEnd:
        return QPixmap(filedialog_end_xpm);
#endif
#ifndef QT_NO_IMAGEFORMAT_PNG
    case SP_CommandLink:
    case SP_ArrowForward:
        if (rtl)
            return proxy()->standardPixmap(SP_ArrowLeft, option, widget);
        return proxy()->standardPixmap(SP_ArrowRight, option, widget);
    case SP_ArrowBack:
        if (rtl)
            return proxy()->standardPixmap(SP_ArrowRight, option, widget);
        return proxy()->standardPixmap(SP_ArrowLeft, option, widget);
    case SP_ArrowLeft:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/left-16.png"));
    case SP_ArrowRight:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/right-16.png"));
    case SP_ArrowUp:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/up-16.png"));
    case SP_ArrowDown:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/down-16.png"));
    case SP_FileDialogToParent:
        return proxy()->standardPixmap(SP_ArrowUp, option, widget);
    case SP_FileDialogNewFolder:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/newdirectory-16.png"));
    case SP_FileDialogDetailedView:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/viewdetailed-16.png"));
    case SP_FileDialogInfoView:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/fileinfo-16.png"));
    case SP_FileDialogContentsView:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/filecontents-16.png"));
    case SP_FileDialogListView:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/viewlist-16.png"));
    case SP_FileDialogBack:
        return proxy()->standardPixmap(SP_ArrowBack, option, widget);
    case SP_DriveHDIcon:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/harddrive-16.png"));
    case SP_TrashIcon:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/trash-16.png"));
    case SP_DriveFDIcon:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/floppy-16.png"));
    case SP_DriveNetIcon:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/networkdrive-16.png"));
    case SP_DesktopIcon:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/desktop-16.png"));
    case SP_ComputerIcon:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/computer-16.png"));
    case SP_DriveCDIcon:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/cdr-16.png"));
    case SP_DriveDVDIcon:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/dvd-16.png"));
    case SP_DirHomeIcon:
    case SP_DirOpenIcon:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/diropen-16.png"));
    case SP_DirIcon:
    case SP_DirClosedIcon:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/dirclosed-16.png"));
    case SP_DirLinkIcon:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/dirlink-16.png"));
    case SP_FileIcon:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/file-16.png"));
    case SP_FileLinkIcon:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/filelink-16.png"));
    case SP_DialogOkButton:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-ok-16.png"));
    case SP_DialogCancelButton:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-cancel-16.png"));
    case SP_DialogHelpButton:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-help-16.png"));
    case SP_DialogOpenButton:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-open-16.png"));
    case SP_DialogSaveButton:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-save-16.png"));
    case SP_DialogCloseButton:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-close-16.png"));
    case SP_DialogApplyButton:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-apply-16.png"));
    case SP_DialogResetButton:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-clear-16.png"));
    case SP_DialogDiscardButton:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-delete-16.png"));
    case SP_DialogYesButton:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-yes-16.png"));
    case SP_DialogNoButton:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-no-16.png"));
    case SP_BrowserReload:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/refresh-24.png"));
    case SP_BrowserStop:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/stop-24.png"));
    case SP_MediaPlay:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/media-play-32.png"));
    case SP_MediaPause:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/media-pause-32.png"));
    case SP_MediaStop:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/media-stop-32.png"));
    case SP_MediaSeekForward:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/media-seek-forward-32.png"));
    case SP_MediaSeekBackward:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/media-seek-backward-32.png"));
    case SP_MediaSkipForward:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/media-skip-forward-32.png"));
    case SP_MediaSkipBackward:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/media-skip-backward-32.png"));
    case SP_MediaVolume:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/media-volume-16.png"));
    case SP_MediaVolumeMuted:
        return QPixmap(QLatin1String(":/trolltech/styles/commonstyle/images/media-volume-muted-16.png"));
#endif // QT_NO_IMAGEFORMAT_PNG
    default:
        break;
    }
    return QPixmap();
}

/*!
    \internal
*/
QIcon QCommonStyle::standardIconImplementation(StandardPixmap standardIcon, const QStyleOption *option,
                                               const QWidget *widget) const
{
    QIcon icon;
    const bool rtl = (option && option->direction == Qt::RightToLeft) || (!option && QApplication::isRightToLeft());
    if (QApplication::desktopSettingsAware() && !QIcon::themeName().isEmpty()) {
        switch (standardIcon) {
        case SP_DirHomeIcon:
                icon = QIcon::fromTheme(QLatin1String("user-home"));
                break;
        case SP_MessageBoxInformation:
                icon = QIcon::fromTheme(QLatin1String("dialog-information"));
                break;
        case SP_MessageBoxWarning:
                icon = QIcon::fromTheme(QLatin1String("dialog-warning"));
                break;
        case SP_MessageBoxCritical:
                icon = QIcon::fromTheme(QLatin1String("dialog-error"));
                break;
        case SP_MessageBoxQuestion:
                icon = QIcon::fromTheme(QLatin1String("dialog-question"));
                break;
        case SP_DialogOpenButton:
        case SP_DirOpenIcon:
                icon = QIcon::fromTheme(QLatin1String("folder-open"));
                break;
        case SP_DialogSaveButton:
                icon = QIcon::fromTheme(QLatin1String("document-save"));
                break;
        case SP_DialogApplyButton:
                icon = QIcon::fromTheme(QLatin1String("dialog-ok-apply"));
                break;
        case SP_DialogYesButton:
        case SP_DialogOkButton:
                icon = QIcon::fromTheme(QLatin1String("dialog-ok"));
                break;
        case SP_DialogDiscardButton:
                icon = QIcon::fromTheme(QLatin1String("edit-delete"));
                break;
        case SP_DialogResetButton:
                icon = QIcon::fromTheme(QLatin1String("edit-clear"));
                break;
        case SP_DialogHelpButton:
                icon = QIcon::fromTheme(QLatin1String("help-contents"));
                break;
        case SP_FileIcon:
                icon = QIcon::fromTheme(QLatin1String("text-x-generic"));
                break;
        case SP_DirClosedIcon:
        case SP_DirIcon:
                icon = QIcon::fromTheme(QLatin1String("folder"));
                break;
        case SP_DriveFDIcon:
                icon = QIcon::fromTheme(QLatin1String("floppy_unmount"));
                break;
        case SP_ComputerIcon:
                icon = QIcon::fromTheme(QLatin1String("computer"),
                                        QIcon::fromTheme(QLatin1String("system")));
                break;
        case SP_DesktopIcon:
                icon = QIcon::fromTheme(QLatin1String("user-desktop"));
                break;
        case SP_TrashIcon:
                icon = QIcon::fromTheme(QLatin1String("user-trash"));
                break;
        case SP_DriveCDIcon:
        case SP_DriveDVDIcon:
                icon = QIcon::fromTheme(QLatin1String("media-optical"));
                break;
        case SP_DriveHDIcon:
                icon = QIcon::fromTheme(QLatin1String("drive-harddisk"));
                break;
        case SP_FileDialogToParent:
                icon = QIcon::fromTheme(QLatin1String("go-up"));
                break;
        case SP_FileDialogNewFolder:
                icon = QIcon::fromTheme(QLatin1String("folder-new"));
                break;
        case SP_ArrowUp:
                icon = QIcon::fromTheme(QLatin1String("go-up"));
                break;
        case SP_ArrowDown:
                icon = QIcon::fromTheme(QLatin1String("go-down"));
                break;
        case SP_ArrowRight:
                icon = QIcon::fromTheme(QLatin1String("go-next"));
                break;
        case SP_ArrowLeft:
                icon = QIcon::fromTheme(QLatin1String("go-previous"));
                break;
        case SP_DialogCancelButton:
                icon = QIcon::fromTheme(QLatin1String("dialog-cancel"),
                                        QIcon::fromTheme(QLatin1String("process-stop")));
                break;
        case SP_DialogCloseButton:
                icon = QIcon::fromTheme(QLatin1String("window-close"));
                break;
        case SP_FileDialogDetailedView:
                icon = QIcon::fromTheme(QLatin1String("view-list-details"));
                break;
        case SP_FileDialogListView:
                icon = QIcon::fromTheme(QLatin1String("view-list-icons"));
                break;
        case SP_BrowserReload:
                icon = QIcon::fromTheme(QLatin1String("view-refresh"));
                break;
        case SP_BrowserStop:
                icon = QIcon::fromTheme(QLatin1String("process-stop"));
                break;
        case SP_MediaPlay:
                icon = QIcon::fromTheme(QLatin1String("media-playback-start"));
                break;
        case SP_MediaPause:
                icon = QIcon::fromTheme(QLatin1String("media-playback-pause"));
                break;
        case SP_MediaStop:
                icon = QIcon::fromTheme(QLatin1String("media-playback-stop"));
                break;
        case SP_MediaSeekForward:
                icon = QIcon::fromTheme(QLatin1String("media-seek-forward"));
                break;
        case SP_MediaSeekBackward:
                icon = QIcon::fromTheme(QLatin1String("media-seek-backward"));
                break;
        case SP_MediaSkipForward:
                icon = QIcon::fromTheme(QLatin1String("media-skip-forward"));
                break;
        case SP_MediaSkipBackward:
                icon = QIcon::fromTheme(QLatin1String("media-skip-backward"));
                break;
        case SP_MediaVolume:
                icon = QIcon::fromTheme(QLatin1String("audio-volume-medium"));
                break;
        case SP_MediaVolumeMuted:
                icon = QIcon::fromTheme(QLatin1String("audio-volume-muted"));
                break;
        case SP_ArrowForward:
            if (rtl)
                return standardIconImplementation(SP_ArrowLeft, option, widget);
            return standardIconImplementation(SP_ArrowRight, option, widget);
        case SP_ArrowBack:
            if (rtl)
                return standardIconImplementation(SP_ArrowRight, option, widget);
            return standardIconImplementation(SP_ArrowLeft, option, widget);
        case SP_FileLinkIcon:
            {
                QIcon linkIcon = QIcon::fromTheme(QLatin1String("emblem-symbolic-link"));
                if (!linkIcon.isNull()) {
                    QIcon baseIcon = standardIconImplementation(SP_FileIcon, option, widget);
                    const QList<QSize> sizes = baseIcon.availableSizes(QIcon::Normal, QIcon::Off);
                    for (int i = 0 ; i < sizes.size() ; ++i) {
                        int size = sizes[i].width();
                        QPixmap basePixmap = baseIcon.pixmap(size);
                        QPixmap linkPixmap = linkIcon.pixmap(size/2);
                        QPainter painter(&basePixmap);
                        painter.drawPixmap(size/2, size/2, linkPixmap);
                        icon.addPixmap(basePixmap);
                    }
                }
            }
            break;
        case SP_DirLinkIcon:
            {
                QIcon linkIcon = QIcon::fromTheme(QLatin1String("emblem-symbolic-link"));
                if (!linkIcon.isNull()) {
                    QIcon baseIcon = standardIconImplementation(SP_DirIcon, option, widget);
                    const QList<QSize> sizes = baseIcon.availableSizes(QIcon::Normal, QIcon::Off);
                    for (int i = 0 ; i < sizes.size() ; ++i) {
                        int size = sizes[i].width();
                        QPixmap basePixmap = baseIcon.pixmap(size);
                        QPixmap linkPixmap = linkIcon.pixmap(size/2);
                        QPainter painter(&basePixmap);
                        painter.drawPixmap(size/2, size/2, linkPixmap);
                        icon.addPixmap(basePixmap);
                    }
                }
        }
        break;
        default:
            break;
        }
    } // if (QApplication::desktopSettingsAware() && !QIcon::themeName().isEmpty())
        if (!icon.isNull())
            return icon;
#if defined(Q_WS_MAC)
    if (QApplication::desktopSettingsAware()) {
        OSType iconType = 0;
        switch (standardIcon) {
        case QStyle::SP_MessageBoxQuestion:
            iconType = kQuestionMarkIcon;
            break;
        case QStyle::SP_MessageBoxInformation:
            iconType = kAlertNoteIcon;
            break;
        case QStyle::SP_MessageBoxWarning:
            iconType = kAlertCautionIcon;
            break;
        case QStyle::SP_MessageBoxCritical:
            iconType = kAlertStopIcon;
            break;
        case SP_DesktopIcon:
            iconType = kDesktopIcon;
            break;
        case SP_TrashIcon:
            iconType = kTrashIcon;
            break;
        case SP_ComputerIcon:
            iconType = kComputerIcon;
            break;
        case SP_DriveFDIcon:
            iconType = kGenericFloppyIcon;
            break;
        case SP_DriveHDIcon:
            iconType = kGenericHardDiskIcon;
            break;
        case SP_DriveCDIcon:
        case SP_DriveDVDIcon:
            iconType = kGenericCDROMIcon;
            break;
        case SP_DriveNetIcon:
            iconType = kGenericNetworkIcon;
            break;
        case SP_DirOpenIcon:
            iconType = kOpenFolderIcon;
            break;
        case SP_DirClosedIcon:
        case SP_DirLinkIcon:
            iconType = kGenericFolderIcon;
            break;
        case SP_FileLinkIcon:
        case SP_FileIcon:
            iconType = kGenericDocumentIcon;
            break;
        case SP_DirIcon: {
            // A rather special case
            QIcon closeIcon = QStyle::standardIcon(SP_DirClosedIcon, option, widget);
            QIcon openIcon = QStyle::standardIcon(SP_DirOpenIcon, option, widget);
            closeIcon.addPixmap(openIcon.pixmap(16, 16), QIcon::Normal, QIcon::On);
            closeIcon.addPixmap(openIcon.pixmap(32, 32), QIcon::Normal, QIcon::On);
            closeIcon.addPixmap(openIcon.pixmap(64, 64), QIcon::Normal, QIcon::On);
            closeIcon.addPixmap(openIcon.pixmap(128, 128), QIcon::Normal, QIcon::On);
            return closeIcon;
        }
        case SP_TitleBarNormalButton:
        case SP_TitleBarCloseButton: {
            QIcon titleBarIcon;
            if (standardIcon == SP_TitleBarCloseButton) {
                titleBarIcon.addFile(QLatin1String(":/trolltech/styles/macstyle/images/closedock-16.png"));
                titleBarIcon.addFile(QLatin1String(":/trolltech/styles/macstyle/images/closedock-down-16.png"), QSize(16, 16), QIcon::Normal, QIcon::On);
            } else {
                titleBarIcon.addFile(QLatin1String(":/trolltech/styles/macstyle/images/dockdock-16.png"));
                titleBarIcon.addFile(QLatin1String(":/trolltech/styles/macstyle/images/dockdock-down-16.png"), QSize(16, 16), QIcon::Normal, QIcon::On);
            }
            return titleBarIcon;
        }
        default:
            break;
        }
        if (iconType != 0) {
            QIcon retIcon;
            IconRef icon;
            IconRef overlayIcon = 0;
            if (iconType != kGenericApplicationIcon) {
                GetIconRef(kOnSystemDisk, kSystemIconsCreator, iconType, &icon);
            } else {
                FSRef fsRef;
                ProcessSerialNumber psn = { 0, kCurrentProcess };
                GetProcessBundleLocation(&psn, &fsRef);
                GetIconRefFromFileInfo(&fsRef, 0, 0, 0, 0, kIconServicesNormalUsageFlag, &icon, 0);
                if (standardIcon == SP_MessageBoxCritical) {
                    overlayIcon = icon;
                    GetIconRef(kOnSystemDisk, kSystemIconsCreator, kAlertCautionIcon, &icon);
                }
            }
            if (icon) {
                qt_mac_constructQIconFromIconRef(icon, overlayIcon, &retIcon, standardIcon);
                ReleaseIconRef(icon);
            }
            if (overlayIcon)
                ReleaseIconRef(overlayIcon);
            return retIcon;
        }
    } // if (QApplication::desktopSettingsAware())
#endif // Q_WS_MAC

    switch (standardIcon) {
#ifndef QT_NO_IMAGEFORMAT_PNG
     case SP_FileDialogNewFolder:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/newdirectory-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/newdirectory-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/newdirectory-128.png"), QSize(128, 128));
        break;
    case SP_FileDialogBack:
        return standardIconImplementation(SP_ArrowBack, option, widget);
    case SP_FileDialogToParent:
        return standardIconImplementation(SP_ArrowUp, option, widget);
    case SP_FileDialogDetailedView:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/viewdetailed-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/viewdetailed-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/viewdetailed-128.png"), QSize(128, 128));
        break;
    case SP_FileDialogInfoView:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/fileinfo-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/fileinfo-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/fileinfo-128.png"), QSize(128, 128));
        break;
    case SP_FileDialogContentsView:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/filecontents-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/filecontents-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/filecontents-128.png"), QSize(128, 128));
        break;
    case SP_FileDialogListView:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/viewlist-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/viewlist-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/viewlist-128.png"), QSize(128, 128));
        break;
    case SP_DialogOkButton:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-ok-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-ok-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-ok-128.png"), QSize(128, 128));
        break;
    case SP_DialogCancelButton:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-cancel-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-cancel-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-cancel-128.png"), QSize(128, 128));
        break;
    case SP_DialogHelpButton:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-help-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-help-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-help-128.png"), QSize(128, 128));
        break;
    case SP_DialogOpenButton:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-open-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-open-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-open-128.png"), QSize(128, 128));
        break;
    case SP_DialogSaveButton:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-save-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-save-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-save-128.png"), QSize(128, 128));
        break;
    case SP_DialogCloseButton:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-close-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-close-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-close-128.png"), QSize(128, 128));
        break;
    case SP_DialogApplyButton:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-apply-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-apply-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-apply-128.png"), QSize(128, 128));
        break;
    case SP_DialogResetButton:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-clear-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-clear-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-clear-128.png"), QSize(128, 128));
        break;
    case SP_DialogDiscardButton:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-delete-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-delete-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-delete-128.png"), QSize(128, 128));
        break;
    case SP_DialogYesButton:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-yes-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-yes-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-yes-128.png"), QSize(128, 128));
        break;
    case SP_DialogNoButton:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-no-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-no-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/standardbutton-no-128.png"), QSize(128, 128));
        break;
    case SP_ArrowForward:
        if (rtl)
            return standardIconImplementation(SP_ArrowLeft, option, widget);
        return standardIconImplementation(SP_ArrowRight, option, widget);
    case SP_ArrowBack:
        if (rtl)
            return standardIconImplementation(SP_ArrowRight, option, widget);
        return standardIconImplementation(SP_ArrowLeft, option, widget);
    case SP_ArrowLeft:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/left-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/left-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/left-128.png"), QSize(128, 128));
        break;
    case SP_ArrowRight:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/right-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/right-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/right-128.png"), QSize(128, 128));
        break;
    case SP_ArrowUp:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/up-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/up-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/up-128.png"), QSize(128, 128));
        break;
    case SP_ArrowDown:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/down-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/down-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/down-128.png"), QSize(128, 128));
        break;
   case SP_DirHomeIcon:
   case SP_DirIcon:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/dirclosed-16.png"),
                     QSize(), QIcon::Normal, QIcon::Off);
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/diropen-16.png"),
                     QSize(), QIcon::Normal, QIcon::On);
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/dirclosed-32.png"),
                     QSize(32, 32), QIcon::Normal, QIcon::Off);
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/diropen-32.png"),
                     QSize(32, 32), QIcon::Normal, QIcon::On);
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/dirclosed-128.png"),
                     QSize(128, 128), QIcon::Normal, QIcon::Off);
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/diropen-128.png"),
                     QSize(128, 128), QIcon::Normal, QIcon::On);
        break;
    case SP_DriveCDIcon:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/cdr-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/cdr-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/cdr-128.png"), QSize(128, 128));
        break;
    case SP_DriveDVDIcon:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/dvd-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/dvd-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/dvd-128.png"), QSize(128, 128));
        break;
    case SP_FileIcon:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/file-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/file-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/file-128.png"), QSize(128, 128));
        break;
    case SP_FileLinkIcon:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/filelink-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/filelink-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/filelink-128.png"), QSize(128, 128));
        break;
    case SP_TrashIcon:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/trash-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/trash-32.png"), QSize(32, 32));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/trash-128.png"), QSize(128, 128));
        break;
    case SP_BrowserReload:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/refresh-24.png"), QSize(24, 24));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/refresh-32.png"), QSize(32, 32));
        break;
    case SP_BrowserStop:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/stop-24.png"), QSize(24, 24));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/stop-32.png"), QSize(32, 32));
        break;
    case SP_MediaPlay:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-play-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-play-32.png"), QSize(32, 32));
        break;
    case SP_MediaPause:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-pause-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-pause-32.png"), QSize(32, 32));
        break;
    case SP_MediaStop:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-stop-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-stop-32.png"), QSize(32, 32));
        break;
    case SP_MediaSeekForward:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-seek-forward-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-seek-forward-32.png"), QSize(32, 32));
        break;
    case SP_MediaSeekBackward:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-seek-backward-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-seek-backward-32.png"), QSize(32, 32));
        break;
    case SP_MediaSkipForward:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-skip-forward-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-skip-forward-32.png"), QSize(32, 32));
        break;
    case SP_MediaSkipBackward:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-skip-backward-16.png"), QSize(16, 16));
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-skip-backward-32.png"), QSize(32, 32));
        break;
    case SP_MediaVolume:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-volume-16.png"), QSize(16, 16));
        break;
    case SP_MediaVolumeMuted:
        icon.addFile(QLatin1String(":/trolltech/styles/commonstyle/images/media-volume-muted-16.png"), QSize(16, 16));
        break;
#endif // QT_NO_IMAGEFORMAT_PNG
    default:
        icon.addPixmap(proxy()->standardPixmap(standardIcon, option, widget));
        break;
    }
    return icon;
}

static inline uint qt_intensity(uint r, uint g, uint b)
{
    // 30% red, 59% green, 11% blue
    return (77 * r + 150 * g + 28 * b) / 255;
}

/*! \reimp */
QPixmap QCommonStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                          const QStyleOption *opt) const
{
    switch (iconMode) {
    case QIcon::Disabled: {
        QImage im = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);

        // Create a colortable based on the background (black -> bg -> white)
        QColor bg = opt->palette.color(QPalette::Disabled, QPalette::Window);
        int red = bg.red();
        int green = bg.green();
        int blue = bg.blue();
        uchar reds[256], greens[256], blues[256];
        for (int i=0; i<128; ++i) {
            reds[i]   = uchar((red   * (i<<1)) >> 8);
            greens[i] = uchar((green * (i<<1)) >> 8);
            blues[i]  = uchar((blue  * (i<<1)) >> 8);
        }
        for (int i=0; i<128; ++i) {
            reds[i+128]   = uchar(qMin(red   + (i << 1), 255));
            greens[i+128] = uchar(qMin(green + (i << 1), 255));
            blues[i+128]  = uchar(qMin(blue  + (i << 1), 255));
        }

        int intensity = qt_intensity(red, green, blue);
        const int factor = 191;

        // High intensity colors needs dark shifting in the color table, while
        // low intensity colors needs light shifting. This is to increase the
        // percieved contrast.
        if ((red - factor > green && red - factor > blue)
            || (green - factor > red && green - factor > blue)
            || (blue - factor > red && blue - factor > green))
            intensity = qMin(255, intensity + 91);
        else if (intensity <= 128)
            intensity -= 51;

        for (int y=0; y<im.height(); ++y) {
            QRgb *scanLine = (QRgb*)im.scanLine(y);
            for (int x=0; x<im.width(); ++x) {
                QRgb pixel = *scanLine;
                // Calculate color table index, taking intensity adjustment
                // and a magic offset into account.
                uint ci = uint(qGray(pixel)/3 + (130 - intensity / 3));
                *scanLine = qRgba(reds[ci], greens[ci], blues[ci], qAlpha(pixel));
                ++scanLine;
            }
        }

        return QPixmap::fromImage(im);
    }
    case QIcon::Selected: {
        QImage img = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        QColor color = opt->palette.color(QPalette::Normal, QPalette::Highlight);
        color.setAlphaF(qreal(0.3));
        QPainter painter(&img);
        painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        painter.fillRect(0, 0, img.width(), img.height(), color);
        painter.end();
        return QPixmap::fromImage(img); }
    case QIcon::Active:
        return pixmap;
    default:
        break;
    }
    return pixmap;
}

/*!
  \reimp
*/
void QCommonStyle::polish(QPalette &pal)
{
    QStyle::polish(pal);
}

/*!
    \reimp
 */
void QCommonStyle::polish(QWidget *widget)
{
    QStyle::polish(widget);
}

/*!
    \reimp
 */
void QCommonStyle::unpolish(QWidget *widget)
{
    QStyle::unpolish(widget);
}

/*!
  \reimp
*/
void QCommonStyle::polish(QApplication *app)
{
    QStyle::polish(app);
}

/*!
    \reimp
 */
void QCommonStyle::unpolish(QApplication *application)
{
    Q_D(const QCommonStyle);
    d->tabBarcloseButtonIcon = QIcon();
    QStyle::unpolish(application);
}


QT_END_NAMESPACE
