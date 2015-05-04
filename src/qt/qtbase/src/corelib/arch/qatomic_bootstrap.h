/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2011 Thiago Macieira <thiago@kde.org>
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

#ifndef QATOMIC_BOOTSTRAP_H
#define QATOMIC_BOOTSTRAP_H

#include <QtCore/qgenericatomic.h>

QT_BEGIN_NAMESPACE

#if 0
// silence syncqt warnings
QT_END_NAMESPACE
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

template <typename T> struct QAtomicOps: QGenericAtomicOps<QAtomicOps<T> >
{
    typedef T Type;

    static bool ref(T &_q_value) Q_DECL_NOTHROW
    {
        return ++_q_value != 0;
    }
    static bool deref(T &_q_value) Q_DECL_NOTHROW
    {
        return --_q_value != 0;
    }

    static bool testAndSetRelaxed(T &_q_value, T expectedValue, T newValue, T *currentValue = 0) Q_DECL_NOTHROW
    {
        if (currentValue)
            *currentValue = _q_value;
        if (_q_value == expectedValue) {
            _q_value = newValue;
            return true;
        }
        return false;
    }

    static T fetchAndStoreRelaxed(T &_q_value, T newValue) Q_DECL_NOTHROW
    {
        T tmp = _q_value;
        _q_value = newValue;
        return tmp;
    }

    template <typename AdditiveType> static
    T fetchAndAddRelaxed(T &_q_value, AdditiveType valueToAdd) Q_DECL_NOTHROW
    {
        T returnValue = _q_value;
        _q_value += valueToAdd;
        return returnValue;
    }
};

QT_END_NAMESPACE

#endif // QATOMIC_BOOTSTRAP_H
