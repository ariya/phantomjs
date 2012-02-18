/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QWINDOWSVISTASTYLE_H
#define QWINDOWSVISTASTYLE_H

#include <QtGui/qwindowsxpstyle.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if !defined(QT_NO_STYLE_WINDOWSVISTA)

class QWindowsVistaStylePrivate;
class Q_GUI_EXPORT QWindowsVistaStyle : public QWindowsXPStyle
{
    Q_OBJECT
public:
    QWindowsVistaStyle();
    
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget = 0) const;
    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const;
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
    void unpolish(QWidget *widget);
    void polish(QPalette &pal);
    void polish(QApplication *app);
    void unpolish(QApplication *app);
    bool event(QEvent *event);
    QPalette standardPalette() const;

protected Q_SLOTS:
    QIcon standardIconImplementation(StandardPixmap standardIcon, const QStyleOption *option,
                                     const QWidget *widget = 0) const;

private:
    Q_DISABLE_COPY(QWindowsVistaStyle)
    Q_DECLARE_PRIVATE(QWindowsVistaStyle)
    friend class QStyleFactory;
};
#endif //QT_NO_STYLE_WINDOWSVISTA

QT_END_NAMESPACE

QT_END_HEADER

#endif //QWINDOWSVISTASTYLE_H
