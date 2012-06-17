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

#ifndef QGTKSTYLE_H
#define QGTKSTYLE_H

#include <QtGui/QCleanlooksStyle>
#include <QtGui/QPalette>
#include <QtGui/QFont>
#include <QtGui/QFileDialog>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if !defined(QT_NO_STYLE_GTK)

class QPainterPath;
class QGtkStylePrivate;

class Q_GUI_EXPORT QGtkStyle : public QCleanlooksStyle
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QGtkStyle)

public:
    QGtkStyle();
    QGtkStyle(QGtkStylePrivate &dd);

    ~QGtkStyle();

    QPalette standardPalette() const;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const;
    void drawControl(ControlElement control, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const;
    void drawItemPixmap(QPainter *painter, const QRect &rect, int alignment,
                        const QPixmap &pixmap) const;
    void drawItemText(QPainter *painter, const QRect &rect, int alignment, const QPalette &pal,
                      bool enabled, const QString& text, QPalette::ColorRole textRole) const;

    int pixelMetric(PixelMetric metric, const QStyleOption *option = 0,
                    const QWidget *widget = 0) const;
    int styleHint(StyleHint hint, const QStyleOption *option,
                  const QWidget *widget, QStyleHintReturn *returnData) const;

    QStyle::SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                              const QPoint &pt, const QWidget *w) const;

    QRect subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                         SubControl subControl, const QWidget *widget) const;
    QRect subElementRect(SubElement sr, const QStyleOption *opt, const QWidget *w) const;
    QRect itemPixmapRect(const QRect &r, int flags, const QPixmap &pixmap) const;


    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &size, const QWidget *widget) const;
    QPixmap standardPixmap(StandardPixmap sp, const QStyleOption *option,
                           const QWidget *widget) const;
    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                const QStyleOption *opt) const;

    void polish(QWidget *widget);
    void polish(QApplication *app);
    void polish(QPalette &palette);

    void unpolish(QWidget *widget);
    void unpolish(QApplication *app);

    static bool getGConfBool(const QString &key, bool fallback = 0);
    static QString getGConfString(const QString &key, const QString &fallback = QString());


protected Q_SLOTS:
    QIcon standardIconImplementation(StandardPixmap standardIcon, const QStyleOption *option,
                                     const QWidget *widget = 0) const;
};

#endif //!defined(QT_NO_STYLE_QGTK)

QT_END_NAMESPACE

QT_END_HEADER

#endif //QGTKSTYLE_H
