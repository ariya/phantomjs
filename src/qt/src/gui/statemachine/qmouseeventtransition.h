/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QMOUSEEVENTTRANSITION_H
#define QMOUSEEVENTTRANSITION_H

#include <QtCore/qeventtransition.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_STATEMACHINE

class QMouseEventTransitionPrivate;
class QPainterPath;
class Q_GUI_EXPORT QMouseEventTransition : public QEventTransition
{
    Q_OBJECT
    Q_PROPERTY(Qt::MouseButton button READ button WRITE setButton)
    Q_PROPERTY(Qt::KeyboardModifiers modifierMask READ modifierMask WRITE setModifierMask)
public:
    QMouseEventTransition(QState *sourceState = 0);
    QMouseEventTransition(QObject *object, QEvent::Type type,
                          Qt::MouseButton button, QState *sourceState = 0);
    ~QMouseEventTransition();

    Qt::MouseButton button() const;
    void setButton(Qt::MouseButton button);

    Qt::KeyboardModifiers modifierMask() const;
    void setModifierMask(Qt::KeyboardModifiers modifiers);

    QPainterPath hitTestPath() const;
    void setHitTestPath(const QPainterPath &path);

protected:
    void onTransition(QEvent *event);
    bool eventTest(QEvent *event);

private:
    Q_DISABLE_COPY(QMouseEventTransition)
    Q_DECLARE_PRIVATE(QMouseEventTransition)
};

#endif //QT_NO_STATEMACHINE

QT_END_NAMESPACE

QT_END_HEADER

#endif
