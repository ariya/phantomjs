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

#ifndef QCHECKBOX_H
#define QCHECKBOX_H

#include <QtGui/qabstractbutton.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QCheckBoxPrivate;
class QStyleOptionButton;

class Q_GUI_EXPORT QCheckBox : public QAbstractButton
{
    Q_OBJECT

    Q_PROPERTY(bool tristate READ isTristate WRITE setTristate)

public:
    explicit QCheckBox(QWidget *parent=0);
    explicit QCheckBox(const QString &text, QWidget *parent=0);


    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    void setTristate(bool y = true);
    bool isTristate() const;

    Qt::CheckState checkState() const;
    void setCheckState(Qt::CheckState state);

Q_SIGNALS:
    void stateChanged(int);

protected:
    bool event(QEvent *e);
    bool hitButton(const QPoint &pos) const;
    void checkStateSet();
    void nextCheckState();
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void initStyleOption(QStyleOptionButton *option) const;

#ifdef QT3_SUPPORT
public:
    enum ToggleState {
        Off =      Qt::Unchecked,
        NoChange = Qt::PartiallyChecked,
        On =       Qt::Checked
    };
    inline QT3_SUPPORT ToggleState state() const
        { return static_cast<QCheckBox::ToggleState>(static_cast<int>(checkState())); }
    inline QT3_SUPPORT void setState(ToggleState state)
        { setCheckState(static_cast<Qt::CheckState>(static_cast<int>(state))); }
    inline QT3_SUPPORT void setNoChange()
        { setCheckState(Qt::PartiallyChecked); }
    QT3_SUPPORT_CONSTRUCTOR QCheckBox(QWidget *parent, const char* name);
    QT3_SUPPORT_CONSTRUCTOR QCheckBox(const QString &text, QWidget *parent, const char* name);
#endif

private:
    Q_DECLARE_PRIVATE(QCheckBox)
    Q_DISABLE_COPY(QCheckBox)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QCHECKBOX_H
