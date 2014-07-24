/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qplatformscreenpageflipper.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformScreenBuffer
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformScreenBuffer class provides an abstraction for screen buffers.
 */
QPlatformScreenBuffer::QPlatformScreenBuffer()
    : m_destroyed(false)
    , m_ready(true)
{

}

QPlatformScreenBuffer::~QPlatformScreenBuffer()
{

}

bool QPlatformScreenBuffer::isDestroyed() const
{
    return m_destroyed;
}

bool QPlatformScreenBuffer::isReady() const
{
    return m_ready;
}

void QPlatformScreenBuffer::aboutToBeDisplayed()
{
}

void QPlatformScreenBuffer::displayed()
{
}


/*!
    \class QPlatformScreenPageFlipper
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformScreenPageFlipper class provides an abstract interface for display buffer swapping

    Implement the displayBuffer() function to initiate a buffer swap. The
    bufferDisplayed() signal should be emitted once the buffer is actually displayed on
    the screen. The bufferReleased() signal should be emitted when the buffer data is no
    longer owned by the display hardware.
*/

QPlatformScreenPageFlipper::QPlatformScreenPageFlipper(QObject *parent)
    :QObject(parent)
{

}

/*!
    \fn bool QPlatformScreenPageFlipper::displayBuffer(QPlatformScreenBuffer *buffer)

    Implemented in subclasses to display \a buffer directly on the screen. Returns \c true
    if it is possible to display the buffer, and \c false if the buffer cannot be displayed.

    If this function returns \c true, the buffer must not be modified or destroyed before the
    bufferReleased() signal is emitted.  The signal bufferDisplayed() is emitted when the buffer
    is displayed on the screen. The two signals may be emitted in either order.

    This function is allowed to block.
*/

QT_END_NAMESPACE

