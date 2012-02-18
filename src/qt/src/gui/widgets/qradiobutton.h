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

#ifndef QRADIOBUTTON_H
#define QRADIOBUTTON_H

#include <QtGui/qabstractbutton.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QRadioButtonPrivate;
class QStyleOptionButton;

class Q_GUI_EXPORT QRadioButton : public QAbstractButton
{
    Q_OBJECT

public:
    explicit QRadioButton(QWidget *parent=0);
    explicit QRadioButton(const QString &text, QWidget *parent=0);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

protected:
    bool event(QEvent *e);
    bool hitButton(const QPoint &) const;
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void initStyleOption(QStyleOptionButton *button) const;

#ifdef QT3_SUPPORT
public:
    QT3_SUPPORT_CONSTRUCTOR QRadioButton(QWidget *parent, const char* name);
    QT3_SUPPORT_CONSTRUCTOR QRadioButton(const QString &text, QWidget *parent, const char* name);
#endif

private:
    Q_DECLARE_PRIVATE(QRadioButton)
    Q_DISABLE_COPY(QRadioButton)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QRADIOBUTTON_H
