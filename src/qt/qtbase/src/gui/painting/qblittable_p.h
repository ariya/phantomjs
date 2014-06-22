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

#ifndef QBLITTABLE_P_H
#define QBLITTABLE_P_H

#include <QtCore/qsize.h>
#include <QtGui/private/qpixmap_blitter_p.h>


#ifndef QT_NO_BLITTABLE
QT_BEGIN_NAMESPACE

class QImage;
class QBlittablePrivate;

class Q_GUI_EXPORT QBlittable
{
    Q_DECLARE_PRIVATE(QBlittable);
public:
    enum Capability {

        SolidRectCapability             = 0x0001,
        SourcePixmapCapability          = 0x0002,
        SourceOverPixmapCapability      = 0x0004,
        SourceOverScaledPixmapCapability = 0x0008,
        AlphaFillRectCapability         = 0x0010,
        OpacityPixmapCapability         = 0x0020,

        // Internal ones
        OutlineCapability               = 0x0001000
    };
    Q_DECLARE_FLAGS (Capabilities, Capability);

    QBlittable(const QSize &size, Capabilities caps);
    virtual ~QBlittable();

    Capabilities capabilities() const;
    QSize size() const;

    virtual void fillRect(const QRectF &rect, const QColor &color) = 0;
    virtual void drawPixmap(const QRectF &rect, const QPixmap &pixmap, const QRectF &subrect) = 0;
    virtual void alphaFillRect(const QRectF &rect, const QColor &color, QPainter::CompositionMode cmode) {
        Q_UNUSED(rect);
        Q_UNUSED(color);
        Q_UNUSED(cmode);
        qWarning("Please implement alphaFillRect function in your platform or remove AlphaFillRectCapability from it");
    }
    virtual void drawPixmapOpacity(const QRectF &rect, const QPixmap &pixmap, const QRectF &subrect, QPainter::CompositionMode cmode, qreal opacity) {
        Q_UNUSED(rect);
        Q_UNUSED(pixmap);
        Q_UNUSED(subrect);
        Q_UNUSED(cmode);
        Q_UNUSED(opacity);
        qWarning("Please implement drawPixmapOpacity function in your platform or remove OpacityPixmapCapability from it");
    }

    QImage *lock();
    void unlock();

    bool isLocked() const;

protected:
    virtual QImage *doLock() = 0;
    virtual void doUnlock() = 0;
    QBlittablePrivate *d_ptr;
};

QT_END_NAMESPACE
#endif //QT_NO_BLITTABLE
#endif //QBLITTABLE_P_H
