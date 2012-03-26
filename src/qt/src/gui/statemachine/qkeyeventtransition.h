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

#ifndef QKEYEVENTTRANSITION_H
#define QKEYEVENTTRANSITION_H

#include <QtCore/qeventtransition.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_STATEMACHINE

class QKeyEventTransitionPrivate;
class Q_GUI_EXPORT QKeyEventTransition : public QEventTransition
{
    Q_OBJECT
    Q_PROPERTY(int key READ key WRITE setKey)
    Q_PROPERTY(Qt::KeyboardModifiers modifierMask READ modifierMask WRITE setModifierMask)
public:
    QKeyEventTransition(QState *sourceState = 0);
    QKeyEventTransition(QObject *object, QEvent::Type type, int key,
                        QState *sourceState = 0);
    ~QKeyEventTransition();

    int key() const;
    void setKey(int key);

    Qt::KeyboardModifiers modifierMask() const;
    void setModifierMask(Qt::KeyboardModifiers modifiers);

protected:
    void onTransition(QEvent *event);
    bool eventTest(QEvent *event);

private:
    Q_DISABLE_COPY(QKeyEventTransition)
    Q_DECLARE_PRIVATE(QKeyEventTransition)
};

#endif //QT_NO_STATEMACHINE

QT_END_NAMESPACE

QT_END_HEADER

#endif
