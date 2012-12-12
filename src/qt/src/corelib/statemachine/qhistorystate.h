/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QHISTORYSTATE_H
#define QHISTORYSTATE_H

#include <QtCore/qabstractstate.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef QT_NO_STATEMACHINE

class QHistoryStatePrivate;
class Q_CORE_EXPORT QHistoryState : public QAbstractState
{
    Q_OBJECT
    Q_PROPERTY(QAbstractState* defaultState READ defaultState WRITE setDefaultState)
    Q_PROPERTY(HistoryType historyType READ historyType WRITE setHistoryType)
    Q_ENUMS(HistoryType)
public:
    enum HistoryType {
        ShallowHistory,
        DeepHistory
    };

    QHistoryState(QState *parent = 0);
    QHistoryState(HistoryType type, QState *parent = 0);
    ~QHistoryState();

    QAbstractState *defaultState() const;
    void setDefaultState(QAbstractState *state);

    HistoryType historyType() const;
    void setHistoryType(HistoryType type);

protected:
    void onEntry(QEvent *event);
    void onExit(QEvent *event);

    bool event(QEvent *e);

private:
    Q_DISABLE_COPY(QHistoryState)
    Q_DECLARE_PRIVATE(QHistoryState)
};

#endif //QT_NO_STATEMACHINE

QT_END_NAMESPACE

QT_END_HEADER

#endif
