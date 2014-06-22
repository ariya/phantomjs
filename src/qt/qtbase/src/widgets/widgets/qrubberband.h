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

#ifndef QRUBBERBAND_H
#define QRUBBERBAND_H

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_RUBBERBAND

class QRubberBandPrivate;
class QStyleOptionRubberBand;

class Q_WIDGETS_EXPORT QRubberBand : public QWidget
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

#endif // QRUBBERBAND_H
