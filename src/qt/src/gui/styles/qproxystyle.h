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

#ifndef QPROXYSTYLE_H
#define QPROXYSTYLE_H

#include <QtGui/QCommonStyle>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if !defined(QT_NO_STYLE_PROXY)

class QProxyStylePrivate;
class Q_GUI_EXPORT QProxyStyle : public QCommonStyle
{
    Q_OBJECT

public:
    QProxyStyle(QStyle *baseStyle = 0);
    ~QProxyStyle();

    QStyle *baseStyle() const;
    void setBaseStyle(QStyle *style);

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = 0) const;
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = 0) const;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget = 0) const;
    void drawItemText(QPainter *painter, const QRect &rect, int flags, const QPalette &pal, bool enabled,
                      const QString &text, QPalette::ColorRole textRole = QPalette::NoRole) const;
    virtual void drawItemPixmap(QPainter *painter, const QRect &rect, int alignment, const QPixmap &pixmap) const;

    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const;

    QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *widget) const;
    QRect itemTextRect(const QFontMetrics &fm, const QRect &r, int flags, bool enabled, const QString &text) const;
    QRect itemPixmapRect(const QRect &r, int flags, const QPixmap &pixmap) const;

    SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option, const QPoint &pos, const QWidget *widget = 0) const;
    int styleHint(StyleHint hint, const QStyleOption *option = 0, const QWidget *widget = 0, QStyleHintReturn *returnData = 0) const;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = 0, const QWidget *widget = 0) const;

    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt, const QWidget *widget = 0) const;
    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap, const QStyleOption *opt) const;
    QPalette standardPalette() const;

    void polish(QWidget *widget);
    void polish(QPalette &pal);
    void polish(QApplication *app);

    void unpolish(QWidget *widget);
    void unpolish(QApplication *app);

protected:
    bool event(QEvent *e);

protected Q_SLOTS:
    QIcon standardIconImplementation(StandardPixmap standardIcon, const QStyleOption *option, const QWidget *widget) const;
    int layoutSpacingImplementation(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
                                    Qt::Orientation orientation, const QStyleOption *option = 0, const QWidget *widget = 0) const;
private:
    Q_DISABLE_COPY(QProxyStyle)
    Q_DECLARE_PRIVATE(QProxyStyle)
};

#endif // QT_NO_STYLE_PROXY

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPROXYSTYLE_H
