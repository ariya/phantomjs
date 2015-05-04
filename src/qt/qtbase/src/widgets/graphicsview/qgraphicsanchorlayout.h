/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QGRAPHICSANCHORLAYOUT_H
#define QGRAPHICSANCHORLAYOUT_H

#include <QtWidgets/qgraphicsitem.h>
#include <QtWidgets/qgraphicslayout.h>


QT_BEGIN_NAMESPACE


#if !defined(QT_NO_GRAPHICSVIEW)

class QGraphicsAnchorPrivate;
class QGraphicsAnchorLayout;
class QGraphicsAnchorLayoutPrivate;

class Q_WIDGETS_EXPORT QGraphicsAnchor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing RESET unsetSpacing)
    Q_PROPERTY(QSizePolicy::Policy sizePolicy READ sizePolicy WRITE setSizePolicy)
public:
    void setSpacing(qreal spacing);
    void unsetSpacing();
    qreal spacing() const;
    void setSizePolicy(QSizePolicy::Policy policy);
    QSizePolicy::Policy sizePolicy() const;
    ~QGraphicsAnchor();
private:
    QGraphicsAnchor(QGraphicsAnchorLayout *parent);

    Q_DECLARE_PRIVATE(QGraphicsAnchor)

    friend class QGraphicsAnchorLayoutPrivate;
    friend struct AnchorData;
};

class Q_WIDGETS_EXPORT QGraphicsAnchorLayout : public QGraphicsLayout
{
public:
    QGraphicsAnchorLayout(QGraphicsLayoutItem *parent = 0);
    virtual ~QGraphicsAnchorLayout();

    QGraphicsAnchor *addAnchor(QGraphicsLayoutItem *firstItem, Qt::AnchorPoint firstEdge,
                               QGraphicsLayoutItem *secondItem, Qt::AnchorPoint secondEdge);
    QGraphicsAnchor *anchor(QGraphicsLayoutItem *firstItem, Qt::AnchorPoint firstEdge,
                            QGraphicsLayoutItem *secondItem, Qt::AnchorPoint secondEdge);

    void addCornerAnchors(QGraphicsLayoutItem *firstItem, Qt::Corner firstCorner,
                          QGraphicsLayoutItem *secondItem, Qt::Corner secondCorner);

    void addAnchors(QGraphicsLayoutItem *firstItem,
                    QGraphicsLayoutItem *secondItem,
                    Qt::Orientations orientations = Qt::Horizontal | Qt::Vertical);

    void setHorizontalSpacing(qreal spacing);
    void setVerticalSpacing(qreal spacing);
    void setSpacing(qreal spacing);
    qreal horizontalSpacing() const;
    qreal verticalSpacing() const;

    void removeAt(int index);
    void setGeometry(const QRectF &rect);
    int count() const;
    QGraphicsLayoutItem *itemAt(int index) const;

    void invalidate();
protected:
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

private:
    Q_DISABLE_COPY(QGraphicsAnchorLayout)
    Q_DECLARE_PRIVATE(QGraphicsAnchorLayout)

    friend class QGraphicsAnchor;
};

#endif

QT_END_NAMESPACE

#endif
