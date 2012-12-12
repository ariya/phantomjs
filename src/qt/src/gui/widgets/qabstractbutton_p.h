/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QABSTRACTBUTTON_P_H
#define QABSTRACTBUTTON_P_H

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

#include "QtCore/qbasictimer.h"
#include "private/qwidget_p.h"

QT_BEGIN_NAMESPACE

class QAbstractButtonPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QAbstractButton)
public:
    QAbstractButtonPrivate(QSizePolicy::ControlType type = QSizePolicy::DefaultType);

    QString text;
    QIcon icon;
    QSize iconSize;
#ifndef QT_NO_SHORTCUT
    QKeySequence shortcut;
    int shortcutId;
#endif
    uint checkable :1;
    uint checked :1;
    uint autoRepeat :1;
    uint autoExclusive :1;
    uint down :1;
    uint blockRefresh :1;
    uint pressed : 1;

#ifndef QT_NO_BUTTONGROUP
    QButtonGroup* group;
#endif
    QBasicTimer repeatTimer;
    QBasicTimer animateTimer;

    int autoRepeatDelay, autoRepeatInterval;

    QSizePolicy::ControlType controlType;
    mutable QSize sizeHint;

    void init();
    void click();
    void refresh();

    QList<QAbstractButton *>queryButtonList() const;
    QAbstractButton *queryCheckedButton() const;
    void notifyChecked();
    void moveFocus(int key);
    void fixFocusPolicy();

    void emitPressed();
    void emitReleased();
    void emitClicked();
};

QT_END_NAMESPACE

#endif // QABSTRACTBUTTON_P_H
