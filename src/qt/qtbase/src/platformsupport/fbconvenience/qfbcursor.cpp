/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qfbcursor_p.h"
#include "qfbscreen_p.h"
#include <QtGui/QPainter>

QT_BEGIN_NAMESPACE

QFbCursor::QFbCursor(QFbScreen *screen)
        : mScreen(screen), mDirty(false), mOnScreen(false)
{
    mGraphic = new QPlatformCursorImage(0, 0, 0, 0, 0, 0);
    setCursor(Qt::ArrowCursor);
}

QRect QFbCursor::getCurrentRect()
{
    QRect rect = mGraphic->image()->rect().translated(-mGraphic->hotspot().x(),
                                                     -mGraphic->hotspot().y());
    rect.translate(QCursor::pos());
    QPoint mScreenOffset = mScreen->geometry().topLeft();
    rect.translate(-mScreenOffset);  // global to local translation
    return rect;
}


void QFbCursor::pointerEvent(const QMouseEvent & e)
{
    Q_UNUSED(e);
    QPoint mScreenOffset = mScreen->geometry().topLeft();
    mCurrentRect = getCurrentRect();
    // global to local translation
    if (mOnScreen || mScreen->geometry().intersects(mCurrentRect.translated(mScreenOffset))) {
        setDirty();
    }
}

QRect QFbCursor::drawCursor(QPainter & painter)
{
    mDirty = false;
    if (mCurrentRect.isNull())
        return QRect();

    // We need this because the cursor might be mDirty due to moving off mScreen
    QPoint mScreenOffset = mScreen->geometry().topLeft();
    // global to local translation
    if (!mCurrentRect.translated(mScreenOffset).intersects(mScreen->geometry()))
        return QRect();

    mPrevRect = mCurrentRect;
    painter.drawImage(mPrevRect, *mGraphic->image());
    mOnScreen = true;
    return mPrevRect;
}

QRect QFbCursor::dirtyRect()
{
    if (mOnScreen) {
        mOnScreen = false;
        return mPrevRect;
    }
    return QRect();
}

void QFbCursor::setCursor(Qt::CursorShape shape)
{
    mGraphic->set(shape);
}

void QFbCursor::setCursor(const QImage &image, int hotx, int hoty)
{
    mGraphic->set(image, hotx, hoty);
}

void QFbCursor::setCursor(const uchar *data, const uchar *mask, int width, int height, int hotX, int hotY)
{
    mGraphic->set(data, mask, width, height, hotX, hotY);
}

#ifndef QT_NO_CURSOR
void QFbCursor::changeCursor(QCursor * widgetCursor, QWindow *window)
{
    Q_UNUSED(window);
    const Qt::CursorShape shape = widgetCursor ? widgetCursor->shape() : Qt::ArrowCursor;

    if (shape == Qt::BitmapCursor) {
        // application supplied cursor
        QPoint spot = widgetCursor->hotSpot();
        setCursor(widgetCursor->pixmap().toImage(), spot.x(), spot.y());
    } else {
        // system cursor
        setCursor(shape);
    }
    mCurrentRect = getCurrentRect();
    QPoint mScreenOffset = mScreen->geometry().topLeft(); // global to local translation
    if (mOnScreen || mScreen->geometry().intersects(mCurrentRect.translated(mScreenOffset)))
        setDirty();
}
#endif

void QFbCursor::setDirty()
{
    if (!mDirty) {
        mDirty = true;
        mScreen->scheduleUpdate();
    }
}

QT_END_NAMESPACE
