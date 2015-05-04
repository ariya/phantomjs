/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGTKSTYLE_P_H
#define QGTKSTYLE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qwindowsstyle_p.h>
#include <QtGui/QPalette>
#include <QtGui/QFont>
#include <QtWidgets/QFileDialog>

QT_BEGIN_NAMESPACE


#if !defined(QT_NO_STYLE_GTK)

class QPainterPath;
class QGtkStylePrivate;

class QGtkStyle : public QCommonStyle
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
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = 0,
                       const QWidget *widget = 0) const;
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
};

#endif //!defined(QT_NO_STYLE_QGTK)

QT_END_NAMESPACE

#endif //QGTKSTYLE_P_H
