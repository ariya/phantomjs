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

#include "qmargins.h"
#include "qdatastream.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

/*!
    \class QMargins
    \ingroup painting
    \since 4.6

    \brief The QMargins class defines the four margins of a rectangle. 

    QMargin defines a set of four margins; left, top, right and bottom,
    that describe the size of the borders surrounding a rectangle.

    The isNull() function returns true only if all margins are set to zero.

    QMargin objects can be streamed as well as compared.
*/


/*****************************************************************************
  QMargins member functions
 *****************************************************************************/

/*!
    \fn QMargins::QMargins()

    Constructs a margins object with all margins set to 0.

    \sa isNull()
*/

/*!
    \fn QMargins::QMargins(int left, int top, int right, int bottom)

    Constructs margins with the given \a left, \a top, \a right, \a bottom

    \sa setLeft(), setRight(), setTop(), setBottom()
*/

/*!
    \fn bool QMargins::isNull() const

    Returns true if all margins are is 0; otherwise returns
    false.
*/


/*!
    \fn int QMargins::left() const

    Returns the left margin.

    \sa setLeft()
*/

/*!
    \fn int QMargins::top() const

    Returns the top margin.

    \sa setTop()
*/

/*!
    \fn int QMargins::right() const

    Returns the right margin.
*/

/*!
    \fn int QMargins::bottom() const

    Returns the bottom margin.
*/


/*!
    \fn void QMargins::setLeft(int left)

    Sets the left margin to \a left.
*/

/*!
    \fn void QMargins::setTop(int Top)

    Sets the Top margin to \a Top.
*/

/*!
    \fn void QMargins::setRight(int right)

    Sets the right margin to \a right.
*/

/*!
    \fn void QMargins::setBottom(int bottom)

    Sets the bottom margin to \a bottom.
*/

/*!
    \fn bool operator==(const QMargins &m1, const QMargins &m2)
    \relates QMargins

    Returns true if \a m1 and \a m2 are equal; otherwise returns false.
*/

/*!
    \fn bool operator!=(const QMargins &m1, const QMargins &m2)
    \relates QMargins

    Returns true if \a m1 and \a m2 are different; otherwise returns false.
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QMargins &m) {
    dbg.nospace() << "QMargins(" << m.left() << ", "
            << m.top() << ", " << m.right() << ", " << m.bottom() << ')';
    return dbg.space();
}
#endif

QT_END_NAMESPACE
