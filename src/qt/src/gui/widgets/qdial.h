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


#ifndef QDIAL_H
#define QDIAL_H

#include <QtGui/qabstractslider.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_DIAL

class QDialPrivate;
class QStyleOptionSlider;

class Q_GUI_EXPORT QDial: public QAbstractSlider
{
    Q_OBJECT

    Q_PROPERTY(bool wrapping READ wrapping WRITE setWrapping)
    Q_PROPERTY(int notchSize READ notchSize)
    Q_PROPERTY(qreal notchTarget READ notchTarget WRITE setNotchTarget)
    Q_PROPERTY(bool notchesVisible READ notchesVisible WRITE setNotchesVisible)
public:
    explicit QDial(QWidget *parent = 0);

    ~QDial();

    bool wrapping() const;

    int notchSize() const;

    void setNotchTarget(double target);
    qreal notchTarget() const;
    bool notchesVisible() const;

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

public Q_SLOTS:
    void setNotchesVisible(bool visible);
    void setWrapping(bool on);

protected:
    bool event(QEvent *e);
    void resizeEvent(QResizeEvent *re);
    void paintEvent(QPaintEvent *pe);

    void mousePressEvent(QMouseEvent *me);
    void mouseReleaseEvent(QMouseEvent *me);
    void mouseMoveEvent(QMouseEvent *me);

    void sliderChange(SliderChange change);
    void initStyleOption(QStyleOptionSlider *option) const;

#ifdef QT3_SUPPORT
public:
    QT3_SUPPORT_CONSTRUCTOR QDial(int minValue, int maxValue, int pageStep, int value,
                                QWidget* parent = 0, const char* name = 0);
    QT3_SUPPORT_CONSTRUCTOR QDial(QWidget *parent, const char *name);

Q_SIGNALS:
    QT_MOC_COMPAT void dialPressed();
    QT_MOC_COMPAT void dialMoved(int value);
    QT_MOC_COMPAT void dialReleased();
#endif

private:
    Q_DECLARE_PRIVATE(QDial)
    Q_DISABLE_COPY(QDial)
};

#endif  // QT_NO_DIAL

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDIAL_H
