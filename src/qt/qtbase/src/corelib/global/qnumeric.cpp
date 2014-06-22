/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qnumeric.h"
#include "qnumeric_p.h"
#include <string.h>

QT_BEGIN_NAMESPACE

/*!
    Returns \c true if the double \a {d} is equivalent to infinity.
    \relates <QtGlobal>
*/
Q_CORE_EXPORT bool qIsInf(double d) { return qt_is_inf(d); }

/*!
    Returns \c true if the double \a {d} is not a number (NaN).
    \relates <QtGlobal>
*/
Q_CORE_EXPORT bool qIsNaN(double d) { return qt_is_nan(d); }

/*!
    Returns \c true if the double \a {d} is a finite number.
    \relates <QtGlobal>
*/
Q_CORE_EXPORT bool qIsFinite(double d) { return qt_is_finite(d); }

/*!
    Returns \c true if the float \a {f} is equivalent to infinity.
    \relates <QtGlobal>
*/
Q_CORE_EXPORT bool qIsInf(float f) { return qt_is_inf(f); }

/*!
    Returns \c true if the float \a {f} is not a number (NaN).
    \relates <QtGlobal>
*/
Q_CORE_EXPORT bool qIsNaN(float f) { return qt_is_nan(f); }

/*!
    Returns \c true if the float \a {f} is a finite number.
    \relates <QtGlobal>
*/
Q_CORE_EXPORT bool qIsFinite(float f) { return qt_is_finite(f); }

/*!
    Returns the bit pattern of a signalling NaN as a double.
    \relates <QtGlobal>
*/
Q_CORE_EXPORT double qSNaN() { return qt_snan(); }

/*!
    Returns the bit pattern of a quiet NaN as a double.
    \relates <QtGlobal>
*/
Q_CORE_EXPORT double qQNaN() { return qt_qnan(); }

/*!
    Returns the bit pattern for an infinite number as a double.
    \relates <QtGlobal>
*/
Q_CORE_EXPORT double qInf() { return qt_inf(); }



/*!
   \internal
 */
static inline quint32 f2i(float f)
{
    quint32 i;
    memcpy(&i, &f, sizeof(f));
    return i;
}

/*!
    Returns the number of representable floating-point numbers between \a a and \a b.

    This function provides an alternative way of doing approximated comparisons of floating-point
    numbers similar to qFuzzyCompare(). However, it returns the distance between two numbers, which
    gives the caller a possibility to choose the accepted error. Errors are relative, so for
    instance the distance between 1.0E-5 and 1.00001E-5 will give 110, while the distance between
    1.0E36 and 1.00001E36 will give 127.

    This function is useful if a floating point comparison requires a certain precision.
    Therefore, if \a a and \a b are equal it will return 0. The maximum value it will return for 32-bit
    floating point numbers is 4,278,190,078. This is the distance between \c{-FLT_MAX} and
    \c{+FLT_MAX}.

    The function does not give meaningful results if any of the arguments are \c Infinite or \c NaN.
    You can check for this by calling qIsFinite().

    The return value can be considered as the "error", so if you for instance want to compare
    two 32-bit floating point numbers and all you need is an approximated 24-bit precision, you can
    use this function like this:

    \code
    if (qFloatDistance(a, b) < (1 << 7)) {   // The last 7 bits are not
                                            // significant
        // precise enough
    }
    \endcode

    \sa qFuzzyCompare()
    \relates <QtGlobal>
*/
Q_CORE_EXPORT quint32 qFloatDistance(float a, float b)
{
    static const quint32 smallestPositiveFloatAsBits = 0x00000001;  // denormalized, (SMALLEST), (1.4E-45)
    /* Assumes:
       * IEE754 format.
       * Integers and floats have the same endian
    */
    Q_STATIC_ASSERT(sizeof(quint32) == sizeof(float));
    Q_ASSERT(qIsFinite(a) && qIsFinite(b));
    if (a == b)
        return 0;
    if ((a < 0) != (b < 0)) {
        // if they have different signs
        if (a < 0)
            a = -a;
        else /*if (b < 0)*/
            b = -b;
        return qFloatDistance(0.0F, a) + qFloatDistance(0.0F, b);
    }
    if (a < 0) {
        a = -a;
        b = -b;
    }
    // at this point a and b should not be negative

    // 0 is special
    if (!a)
        return f2i(b) - smallestPositiveFloatAsBits + 1;
    if (!b)
        return f2i(a) - smallestPositiveFloatAsBits + 1;

    // finally do the common integer subtraction
    return a > b ? f2i(a) - f2i(b) : f2i(b) - f2i(a);
}


/*!
   \internal
 */
static inline quint64 d2i(double d)
{
    quint64 i;
    memcpy(&i, &d, sizeof(d));
    return i;
}

/*!
    Returns the number of representable floating-point numbers between \a a and \a b.

    This function serves the same purpose as \c{qFloatDistance(float, float)}, but
    returns the distance between two \c double numbers. Since the range is larger
    than for two \c float numbers (\c{[-DBL_MAX,DBL_MAX]}), the return type is quint64.


    \sa qFuzzyCompare()
    \relates <QtGlobal>
*/
Q_CORE_EXPORT quint64 qFloatDistance(double a, double b)
{
    static const quint64 smallestPositiveFloatAsBits = 0x1;  // denormalized, (SMALLEST)
    /* Assumes:
       * IEE754 format double precision
       * Integers and floats have the same endian
    */
    Q_STATIC_ASSERT(sizeof(quint64) == sizeof(double));
    Q_ASSERT(qIsFinite(a) && qIsFinite(b));
    if (a == b)
        return 0;
    if ((a < 0) != (b < 0)) {
        // if they have different signs
        if (a < 0)
            a = -a;
        else /*if (b < 0)*/
            b = -b;
        return qFloatDistance(0.0, a) + qFloatDistance(0.0, b);
    }
    if (a < 0) {
        a = -a;
        b = -b;
    }
    // at this point a and b should not be negative

    // 0 is special
    if (!a)
        return d2i(b) - smallestPositiveFloatAsBits + 1;
    if (!b)
        return d2i(a) - smallestPositiveFloatAsBits + 1;

    // finally do the common integer subtraction
    return a > b ? d2i(a) - d2i(b) : d2i(b) - d2i(a);
}


QT_END_NAMESPACE
