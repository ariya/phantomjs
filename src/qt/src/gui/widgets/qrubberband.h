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

#ifndef QRUBBERBAND_H
#define QRUBBERBAND_H

#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_RUBBERBAND

class QRubberBandPrivate;
class QStyleOptionRubberBand;

class Q_GUI_EXPORT QRubberBand : public QWidget
{
    Q_OBJECT

public:
    enum Shape { Line, Rectangle };
    explicit QRubberBand(Shape, QWidget * =0);
    ~QRubberBand();

    Shape shape() const;

    void setGeometry(const QRect &r);

    inline void setGeometry(int x, int y, int w, int h);
    inline void move(int x, int y);
    inline void move(const QPoint &p)
    { move(p.x(), p.y()); }
    inline void resize(int w, int h)
    { setGeometry(geometry().x(), geometry().y(), w, h); }
    inline void resize(const QSize &s)
    { resize(s.width(), s.height()); }

protected:
    bool event(QEvent *e);
    void paintEvent(QPaintEvent *);
    void changeEvent(QEvent *);
    void showEvent(QShowEvent *);
    void resizeEvent(QResizeEvent *);
    void moveEvent(QMoveEvent *);
    void initStyleOption(QStyleOptionRubberBand *option) const;

private:
    Q_DECLARE_PRIVATE(QRubberBand)
};

inline void QRubberBand::setGeometry(int ax, int ay, int aw, int ah)
{ setGeometry(QRect(ax, ay, aw, ah)); }
inline void QRubberBand::move(int ax, int ay)
{ setGeometry(ax, ay, width(), height()); }

#endif // QT_NO_RUBBERBAND

QT_END_NAMESPACE

QT_END_HEADER

#endif // QRUBBERBAND_H
