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

#ifndef QSCROLLBAR_H
#define QSCROLLBAR_H

#include <QtWidgets/qwidget.h>
#include <QtWidgets/qabstractslider.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_SCROLLBAR

class QScrollBarPrivate;
class QStyleOptionSlider;

class Q_WIDGETS_EXPORT QScrollBar : public QAbstractSlider
{
    Q_OBJECT
public:
    explicit QScrollBar(QWidget *parent=0);
    explicit QScrollBar(Qt::Orientation, QWidget *parent=0);
    ~QScrollBar();

    QSize sizeHint() const;
    bool event(QEvent *event);

protected:
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *);
#endif
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void hideEvent(QHideEvent*);
    void sliderChange(SliderChange change);
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *);
#endif
    void initStyleOption(QStyleOptionSlider *option) const;


private:
    friend class QAbstractScrollAreaPrivate;
    friend Q_WIDGETS_EXPORT QStyleOptionSlider qt_qscrollbarStyleOption(QScrollBar *scrollBar);

    Q_DISABLE_COPY(QScrollBar)
    Q_DECLARE_PRIVATE(QScrollBar)
};

#endif // QT_NO_SCROLLBAR

QT_END_NAMESPACE

#endif // QSCROLLBAR_H
