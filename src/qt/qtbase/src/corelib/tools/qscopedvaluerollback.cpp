/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qscopedvaluerollback.h"

QT_BEGIN_NAMESPACE

/*!
    \class QScopedValueRollback
    \inmodule QtCore
    \brief The QScopedValueRollback class resets a variable to its previous value on destruction.
    \since 4.8
    \ingroup misc

    The QScopedValueRollback class can be used to revert state when an
    exception is thrown without needing to write try-catch blocks.

    It can also be used to manage variables that are temporarily set,
    such as reentrancy guards. By using this class, the variable will
    be reset whether the function is exited normally, exited early by
    a return statement, or exited by an exception.

    The template can only be instantiated with a type that supports assignment.

    \sa QScopedPointer
*/

/*!
    \fn QScopedValueRollback::QScopedValueRollback(T &var)

    Stores the previous value of \a var internally, for revert on destruction.
*/

/*!
    \fn QScopedValueRollback::QScopedValueRollback(T &var, T value)

    Assigns \a value to \ var and stores the previous value of \a var
    internally, for revert on destruction.

    \since 5.4
*/

/*!
    \fn QScopedValueRollback::~QScopedValueRollback()

    Assigns the previous value to the managed variable.
    This is the value at construction time, or at the last call to commit()
*/

/*!
    \fn void QScopedValueRollback::commit()

    Updates the previous value of the managed variable to its current value.
*/

QT_END_NAMESPACE
