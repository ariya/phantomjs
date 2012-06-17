/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qnumeric.h"
#include "qnumeric_p.h"

QT_BEGIN_NAMESPACE

/*!
    Returns true if the double \a {d} is equivalent to infinity.
*/
Q_CORE_EXPORT bool qIsInf(double d) { return qt_is_inf(d); }

/*!
    Returns true if the double \a {d} is not a number (NaN).
*/
Q_CORE_EXPORT bool qIsNaN(double d) { return qt_is_nan(d); }

/*!
    Returns true if the double \a {d} is a finite number.
*/
Q_CORE_EXPORT bool qIsFinite(double d) { return qt_is_finite(d); }

/*!
    Returns true if the float \a {f} is equivalent to infinity.
*/
Q_CORE_EXPORT bool qIsInf(float f) { return qt_is_inf(f); }

/*!
    Returns true if the float \a {f} is not a number (NaN).
*/
Q_CORE_EXPORT bool qIsNaN(float f) { return qt_is_nan(f); }

/*!
    Returns true if the float \a {f} is a finite number.
*/
Q_CORE_EXPORT bool qIsFinite(float f) { return qt_is_finite(f); }

/*!
    Returns the bit pattern of a signalling NaN as a double.
*/
Q_CORE_EXPORT double qSNaN() { return qt_snan(); }

/*!
    Returns the bit pattern of a quiet NaN as a double.
*/
Q_CORE_EXPORT double qQNaN() { return qt_qnan(); }

/*!
    Returns the bit pattern for an infinite number as a double.
*/
Q_CORE_EXPORT double qInf() { return qt_inf(); }


QT_END_NAMESPACE
