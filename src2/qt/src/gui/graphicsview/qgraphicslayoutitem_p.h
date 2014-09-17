/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QGRAPHICSLAYOUTITEM_P_H
#define QGRAPHICSLAYOUTITEM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QSizeF>
#include <QtGui/QSizePolicy>

QT_BEGIN_NAMESPACE

class QGraphicsLayoutItem;
class Q_AUTOTEST_EXPORT QGraphicsLayoutItemPrivate
{
    Q_DECLARE_PUBLIC(QGraphicsLayoutItem)
public:
    virtual ~QGraphicsLayoutItemPrivate();
    QGraphicsLayoutItemPrivate(QGraphicsLayoutItem *parent, bool isLayout);
    static QGraphicsLayoutItemPrivate *get(QGraphicsLayoutItem *q) { return q->d_func();}
    static const QGraphicsLayoutItemPrivate *get(const QGraphicsLayoutItem *q) { return q->d_func();}

    void init();
    QSizeF *effectiveSizeHints(const QSizeF &constraint) const;
    QGraphicsItem *parentItem() const;
    void ensureUserSizeHints();
    void setSize(Qt::SizeHint which, const QSizeF &size);
    enum SizeComponent { Width, Height };
    void setSizeComponent(Qt::SizeHint which, SizeComponent component, qreal value);

    bool hasHeightForWidth() const;
    bool hasWidthForHeight() const;

    QSizePolicy sizePolicy;
    QGraphicsLayoutItem *parent;

    QSizeF *userSizeHints;
    mutable QSizeF cachedSizeHints[Qt::NSizeHints];
    mutable QSizeF cachedConstraint;
    mutable QSizeF cachedSizeHintsWithConstraints[Qt::NSizeHints];

    mutable quint32 sizeHintCacheDirty : 1;
    mutable quint32 sizeHintWithConstraintCacheDirty : 1;
    quint32 isLayout : 1;
    quint32 ownedByLayout : 1;

    QGraphicsLayoutItem *q_ptr;
    QRectF geom;
    QGraphicsItem *graphicsItem;
};

QT_END_NAMESPACE

#endif //QGRAPHICSLAYOUTITEM_P_H

