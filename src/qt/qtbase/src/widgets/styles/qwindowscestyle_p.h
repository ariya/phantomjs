/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSCESTYLE_P_H
#define QWINDOWSCESTYLE_P_H

#include <private/qwindowsstyle_p.h>

QT_BEGIN_NAMESPACE


#if !defined(QT_NO_STYLE_WINDOWSCE)

class QWindowsCEStyle : public QWindowsStyle
{
   Q_OBJECT
public:
    QWindowsCEStyle();
    ~QWindowsCEStyle();

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget = 0) const;

    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const;

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const;

    virtual void drawItemText(QPainter *painter, const QRect &rect,
                              int flags, const QPalette &pal, bool enabled,
                              const QString &text, QPalette::ColorRole textRole = QPalette::NoRole) const;

    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &size, const QWidget *widget) const;

    QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                         SubControl sc, const QWidget *widget) const;

    SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                     const QPoint &pos, const QWidget *widget = 0) const;

    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                           const QWidget *widget = 0) const;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = 0, const QWidget *widget = 0) const;

    int styleHint(StyleHint hint, const QStyleOption *opt = 0, const QWidget *widget = 0,
                  QStyleHintReturn *returnData = 0) const;

    void polish(QWidget *widget);
    void polish(QPalette &palette);
    void polish(QApplication *app);
    QPalette standardPalette() const;
};

#endif // QT_NO_STYLE_WINDOWSCE

QT_END_NAMESPACE

#endif // QWINDOWSCESTYLE_P_H
